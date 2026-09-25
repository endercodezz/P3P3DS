#include "camera/game_camera.hpp"

#include "camera/camera_input.hpp"
#include "settings/settings.hpp"
#include "psprecomp/runtime.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace mhp3rd::camera {

namespace {
// The mouse and a drag on a touch screen move the camera the same way,
// aiming included: a pointer's motion rather than a held rate.
Turn combine(const Turn &a, const Turn &b) {
    return {a.yaw_degrees + b.yaw_degrees, a.pitch_degrees + b.pitch_degrees, a.yaw_held || b.yaw_held,
            a.pitch_held || b.pitch_held};
}
Turn take_pointer() {
    const Turn mouse = take(Source::Mouse);
    return combine(mouse, take(Source::Touch));
}
Turn peek_pointer() { return combine(peek(Source::Mouse), peek(Source::Touch)); }
} // namespace
namespace {

// NPJB-40001: the ordinary camera update calls the rotation helper at
// 0x088E6264. At that call s1 is the camera, a1 points to the rotation angles,
// and sp+0x30 holds the eye offset from the look-at point. The game's
// subsequent transform, terrain and wall collision checks still run on the
// adjusted offset. These facts come from this project's own reading of the
// executable; each one the driver relies on is listed in kSignature.
constexpr std::uint32_t kRotationHelper = 0x08878B70u;
constexpr std::uint32_t kCameraReturn = 0x088E626Cu;
constexpr float kRadians = 3.14159265358979323846f / 180.0f;
constexpr float kAngleUnits = 65536.0f / 360.0f;

// Camera structure, relative to s1.
constexpr std::uint32_t kEyeY = 0x04u;
constexpr std::uint32_t kPreset = 0x70u;
constexpr std::uint32_t kMode = 0x76u;
constexpr std::uint32_t kYawTarget = 0x80u;
constexpr std::uint32_t kYawCurrent = 0x82u;
constexpr std::uint32_t kButtons = 0x84u;
constexpr std::uint32_t kSnap = 0x8Eu;
// The aim the weapon reports while the player aims a bow or a bowgun, -1
// otherwise. The camera update asks the weapon's code each update (0x088E5434)
// and, while this is not negative, turns the camera after the aim itself
// (0x088E7AFC). The aim is moved with the same stick.
constexpr std::uint32_t kAim = 0x91u;
// While a weapon aims, the aim itself lives in the hunter the camera follows
// (s5 at the ordinary call). The weapon's aim code (in game_task, around
// 0x0A0FE4D8) reads the stick as on/off commands and, in the states where
// aiming may move, steps the facing by a fixed 512 or 624 and one of three
// vertical aims by a fixed amount, which is why its aim feels like a D-pad.
// The driver leaves every decision to that code -- whether the aim may move
// now, and which way -- and only replaces how far each step goes.
//
// The facing, in the camera's yaw units, the game keeps at kHunterHeading
// and copies into kHunterYaw each frame; writing only the copy is undone.
constexpr std::uint32_t kHunterYaw = 0x74u;
constexpr std::uint32_t kHunterHeading = 0x188u;
// The largest single step the aim code makes, with room for its diagonal
// scaling; anything larger is the game setting the facing for another reason.
constexpr int kLargestYawStep = 1024;

// The three vertical aims the aim code steps, depending on how the weapon
// aims. The byte ones step by 5 or 8 and stop at 100; the halfword one steps
// by that times 64 and stops at 8192. One byte unit is taken as 3.43 / 8 of
// a degree, the angle of the game's yaw step for its vertical step.
struct PitchField {
    std::uint32_t offset;
    bool halfword;
    int limit;
    int largest_step;
    float units_per_degree;
};
constexpr float kAimPitchUnitsPerDegree = 8.0f / (624.0f / kAngleUnits);
constexpr std::array<PitchField, 3> kPitchFields{{
    {0xC22u, false, 100, 16, kAimPitchUnitsPerDegree},           // 0x088A68A8
    {0x1457u, false, 100, 16, kAimPitchUnitsPerDegree},          // 0x0A11FCF0
    {0xC24u, true, 8192, 1024, kAimPitchUnitsPerDegree * 64.0f}, // 0x0A11FC9C
}};
constexpr std::uint32_t kHunterExtent = 0x1458u;
// A bowgun's scope (the full-screen reticle, entered with a short press of
// R): bit 0x1000 of the hunter's word at +0xBB0, which the camera update
// tests at 0x088E4DCC before it builds the scope's view. The camera mode stays
// 0 and the weapon reports no aim at +0x91, so without this the scope looks
// like the ordinary follow camera. The scope's aim code steps the same facing
// (by 400) and +0xC22 (by 4) from either stick, so it is driven like an aim.
constexpr std::uint32_t kHunterWeaponFlags = 0xBB0u;
constexpr std::uint32_t kScopeFlag = 0x1000u;
// Preset, relative to its pointer, and the update's stack frame.
constexpr std::uint32_t kPresetTargetHeight = 0x10u;
constexpr std::uint32_t kStackEyeY = 0x34u;
constexpr std::uint32_t kStackEyeZ = 0x38u;

// The mouse while aiming. Its degrees wait for a step of the game to size
// (the game may step an update after the stick showed the direction); up to
// this much, and for this many updates without a step, after which they are
// dropped, as a push of the stick is while the game makes no step.
constexpr float kMouseAimCarry = 45.0f;
constexpr unsigned kMouseAimWaits = 3u;
// Less than this, on an axis, is not presented to the game as a push on it.
constexpr float kMouseAimThreshold = 0.05f;
// Degrees of mouse yaw in one frame that switch on the game's own turn.
constexpr float kMouseStockTurn = 0.5f;

// The one camera mode with a driver: the ordinary follow camera in a quest.
// Others (aiming with a bow or a bowgun among them) keep the stock camera and
// the stock stick until each is traced and given its own driver below.
constexpr std::uint8_t kFollowMode = 0u;

// Game code the driver depends on, read from NPJB-40001. If any word differs,
// the executable is not the one these facts were read from, and the driver
// stays out rather than write to places that may mean something else.
constexpr std::array<CodeWord, 24> kSignature{{
    {0x088E6264u, 0x0E21E2DCu, "jal 0x08878B70: the camera update calls the rotation helper"},
    {0x088E6254u, 0x27A50050u, "addiu a1,sp,0x50: the helper's angles are on the update's stack"},
    {0x08878B70u, 0x27BDFFE0u, "addiu sp,sp,-0x20: the rotation helper's first instruction"},
    {0x088E6214u, 0x92230076u, "lbu v1,0x76(s1): the camera mode"},
    {0x088E68F0u, 0x8E220070u, "lw v0,0x70(s1): the camera's preset"},
    {0x088E68F8u, 0xE7A00034u, "swc1 f0,0x34(sp): the eye offset's height, from the preset"},
    {0x088E6904u, 0xE7A20038u, "swc1 f2,0x38(sp): the eye offset's distance, from the preset"},
    {0x088E6908u, 0xC4410010u, "lwc1 f1,0x10(v0): the preset's look-at height"},
    {0x088E6910u, 0x0A23988Fu, "j 0x088E623C: modes 0-2 join the path that calls the helper"},
    {0x088E61A0u, 0xA6220080u, "sh v0,0x80(s1): the target yaw"},
    {0x088E61A4u, 0x86220082u, "lh v0,0x82(s1): the filtered yaw"},
    {0x088E61ACu, 0x9224008Eu, "lbu a0,0x8e(s1): the recentre snap"},
    {0x088E77D4u, 0x96260084u, "lhu a2,0x84(s1): the camera's buttons"},
    {0x088E6B58u, 0xC6220004u, "lwc1 f2,0x4(s1): the eye height the 1/8 filter eases"},
    {0x0896D47Cu, 0x3E000000u, "0.125: that filter's coefficient"},
    {0x088E5434u, 0x0E83D878u, "jal 0x0A0F61E0: the camera asks the weapon whether it is aiming"},
    {0x088E5448u, 0x82220091u, "lb v0,0x91(s1): the aim the weapon reported"},
    {0x088E7AFCu, 0x82260091u, "lb a2,0x91(s1): the camera follows the aim while it is not negative"},
    {0x088E7B20u, 0x8EA20074u, "lw v0,0x74(s5): what it follows is the hunter's facing"},
    {0x088E34C8u, 0x82630C22u, "lb v1,0xc22(s3): and the hunter's vertical aim"},
    {0x088A68A8u, 0x90820C22u, "lbu v0,0xc22(a0): the game's own step of the vertical aim"},
    {0x088A68C8u, 0x24020064u, "addiu v0,zero,0x64: which it limits to 100"},
    {0x088E4DCCu, 0x8C420BB0u, "lw v0,0xbb0(v0): the camera reads the hunter's weapon flags"},
    {0x088E4DD0u, 0x30421000u, "andi v0,v0,0x1000: and builds the scope's view while this bit is set"},
}};

struct State {
    bool prepared{};
    bool hooked{};
    RotationFunction original{};
    std::uint64_t frame{};
    std::uint64_t last_update{};
    std::uint32_t address{};
    bool available{};
    float yaw_remainder{};
    bool pitch_owned{};
    float pitch{};
    unsigned updates{};
    bool aiming{};
    std::uint32_t aim_hunter{};
    std::uint16_t aim_heading{};
    std::array<int, 3> aim_pitch{};
    float aim_yaw_remainder{};
    float aim_pitch_remainder{};
    // The mouse's aim not yet spent on a step, in degrees, and the frame
    // after the one its direction was last shown to the game (0: never).
    float mouse_yaw{};
    float mouse_pitch{};
    unsigned mouse_yaw_waits{};
    unsigned mouse_pitch_waits{};
    std::uint64_t mouse_shown{};
    // The direction last shown, each axis -1, 0 or +1.
    int shown_x{};
    int shown_y{};
    // The game's own steps for a shown direction, learned from the steps it
    // made in this aim: size (straight and diagonal) and which way it went
    // for a positive push (+1, -1; 0: not seen yet). Scope and aim step by
    // different amounts, so a change between them starts over.
    bool learned_scoped{};
    std::array<int, 2> yaw_step{};
    int yaw_sign{};
    std::array<int, 2> pitch_step{};
    int pitch_sign{};
    std::size_t pitch_field{};
    // The game's next step, taken off the aim in advance at the flip (see
    // game_camera_anticipate_aim), and the values that left in the hunter.
    int anticipated_yaw{};
    std::uint16_t anticipated_heading{};
    int anticipated_pitch{};
    int anticipated_pitch_value{};
};
State state;

bool option_on() {
    const auto &s = settings::current();
    // Right stick describes a physical stick. On Android a finger drag turns
    // the camera whatever it says, so there Analog camera alone decides.
    return s.analog_camera && (s.right_stick == settings::RightStick::Camera ||
                               settings::kPlatform == settings::Platform::Android);
}

bool driving_allowed() { return state.hooked && option_on(); }

float load_float(const psprecomp::GuestMemory &memory, std::uint32_t address) {
    return std::bit_cast<float>(memory.load32(address));
}

void store_float(psprecomp::GuestMemory &memory, std::uint32_t address, float value) {
    memory.store32(address, std::bit_cast<std::uint32_t>(value));
}

void release() {
    state.available = false;
    state.pitch_owned = false;
    state.yaw_remainder = 0.0f;
    state.aiming = false;
    state.aim_yaw_remainder = 0.0f;
    state.aim_pitch_remainder = 0.0f;
    state.mouse_yaw = 0.0f;
    state.mouse_pitch = 0.0f;
    state.mouse_yaw_waits = 0u;
    state.mouse_pitch_waits = 0u;
}

void trace(const psprecomp::GuestMemory &memory, std::uint32_t address, std::uint32_t stack, const Turn &turn,
           bool active, bool recentre, bool vertical_command, float y, float z, float target_height) {
    static const char *path = std::getenv("MHP3RD_TRACE_CAMERA_STATE");
    if (path == nullptr) return;
    static std::ofstream out(path);
    const float eye_dx = load_float(memory, address) - load_float(memory, address + 0x10u);
    const float eye_dy = load_float(memory, address + 4u) - load_float(memory, address + 0x14u);
    const float eye_dz = load_float(memory, address + 8u) - load_float(memory, address + 0x18u);
    out << state.frame << ',' << std::hex << address << std::dec << ',' << active << ',' << turn.yaw_degrees << ','
        << turn.pitch_degrees << ',' << memory.load16(address + kYawTarget) << ','
        << memory.load16(address + kYawCurrent) << ',' << y << ',' << z << ',' << target_height << ','
        << state.pitch_owned << ',' << state.pitch << ',' << recentre << ',' << vertical_command << ','
        << load_float(memory, stack + kStackEyeY) << ',' << load_float(memory, stack + kStackEyeZ) << ','
        << std::atan2(eye_dy, std::hypot(eye_dx, eye_dz)) / kRadians << '\n';
    if (state.updates % 30u == 0u) out.flush();
}

// The ordinary follow camera: yaw on the game's own angle, pitch by orbiting
// the eye around its look-at point at the preset's distance.
void drive_follow(psprecomp::GuestMemory &memory, psprecomp::AllegrexContext &ctx, std::uint32_t address,
                  std::uint32_t stack) {
    if (state.address != address || state.frame - state.last_update > 2u) {
        state.pitch_owned = false;
        state.yaw_remainder = 0.0f;
    }
    state.address = address;
    state.last_update = state.frame;
    state.available = true;
    state.aiming = false;
    ++state.updates;

    const bool active = driving_allowed();
    // Taken even when inactive, so nothing built up while the option was off
    // is spent the moment it comes back on.
    const Turn turn = take();
    const float y = load_float(memory, stack + kStackEyeY);
    const float z = load_float(memory, stack + kStackEyeZ);
    const auto preset = memory.load32(address + kPreset);
    if (!memory.raw_pointer(preset, 0x20u)) return;
    const float target_height = load_float(memory, preset + kPresetTargetHeight);
    const float radius = std::hypot(y - target_height, z);
    if (!std::isfinite(radius) || radius < 1.0f || radius > 10000.0f) return;

    // A real D-pad command, recentre or target-camera snap cancels our pitch.
    // The game has already selected the new preset; these stay its controls.
    const auto buttons = memory.load16(address + kButtons);
    const bool recentre = (buttons & 0x100u) != 0u || memory.load8(address + kSnap) != 0u;
    const bool vertical_command = (buttons & 0x50u) != 0u;
    if (!active || recentre || vertical_command) state.pitch_owned = false;

    if (active && !recentre && turn.yaw_held) {
        state.yaw_remainder -= turn.yaw_degrees * kAngleUnits;
        const int step = static_cast<int>(state.yaw_remainder);
        state.yaw_remainder -= static_cast<float>(step);
        const auto yaw = static_cast<std::uint16_t>(memory.load16(address + kYawCurrent) + step);
        // Keeping target and filtered yaw together prevents a delayed turn
        // after release: the filter at 0x088E61A4 has nothing left to close.
        memory.store16(address + kYawTarget, yaw);
        memory.store16(address + kYawCurrent, yaw);
        memory.store32(ctx.gpr[5] + 4u, static_cast<std::uint32_t>(static_cast<std::int16_t>(yaw)));
    } else {
        state.yaw_remainder = 0.0f;
    }

    if (active && !recentre && !vertical_command) {
        float previous_pitch = state.pitch;
        if (turn.pitch_held) {
            if (!state.pitch_owned) {
                state.pitch = std::atan2(y - target_height, std::fabs(z)) / kRadians;
                previous_pitch = state.pitch;
                state.pitch_owned = true;
            }
            state.pitch = std::clamp(state.pitch + turn.pitch_degrees, -60.0f, 70.0f);
        }
        if (state.pitch_owned) {
            // Rotate the eye about its look-at point, keeping the current
            // preset's distance. Only this update's stack values change; shared
            // preset tables and other cameras are never written.
            const float pitch = state.pitch * kRadians;
            store_float(memory, stack + kStackEyeY, target_height + radius * std::sin(pitch));
            store_float(memory, stack + kStackEyeZ, std::copysign(radius * std::cos(pitch), z));
            // 0x088E6B58 eases eye.y towards target.y by 1/8 each update.
            // Advance its current value by just the player's height change;
            // otherwise releasing the stick leaves many frames of catch-up.
            // Terrain movement and collision corrections still use the game's
            // filter, and all subsequent collision checks remain in place.
            if (state.pitch != previous_pitch) {
                const float delta = radius * (std::sin(pitch) - std::sin(previous_pitch * kRadians));
                store_float(memory, address + kEyeY, load_float(memory, address + kEyeY) + delta);
            }
        }
    }
    trace(memory, address, stack, turn, active, recentre, vertical_command, y, z, target_height);
}

// MHP3RD_TRACE_CAMERA_MODES=path.csv: every call of the rotation helper, from
// any caller, with the camera mode and the stick, and the whole camera
// structure whenever the mode is not the ordinary one. For finding what the
// aiming camera keeps where, before it has a driver.
std::ofstream *modes_trace() {
    static const char *path = std::getenv("MHP3RD_TRACE_CAMERA_MODES");
    if (path == nullptr) return nullptr;
    static std::ofstream out(path);
    return &out;
}

void dump_camera(std::ostream &out, const psprecomp::GuestMemory &memory, std::uint32_t address) {
    const unsigned mode = memory.load8(address + kMode);
    out << ",mode=" << mode << ",yaw=" << memory.load16(address + kYawTarget) << ':'
        << memory.load16(address + kYawCurrent) << ",buttons=" << std::hex << memory.load16(address + kButtons)
        << std::dec;
    out << ",words=" << std::hex;
    for (std::uint32_t offset = 0u; offset < 0x190u; offset += 4u)
        out << (offset ? ":" : "") << memory.load32(address + offset);
    out << std::dec;
}

// Once a flip as well, from the last camera the ordinary update named: an
// aiming camera may never call the helper at all.
void trace_flip(const psprecomp::GuestMemory &memory) {
    std::ofstream *out = modes_trace();
    if (out == nullptr || state.address == 0u || memory.raw_pointer(state.address, 0x190u) == nullptr) return;
    const Rate stick = rate(Source::Stick);
    *out << state.frame << ",flip,s1=" << std::hex << state.address << std::dec << ",stick=" << stick.yaw << ':'
         << stick.pitch << ",aim=" << static_cast<int>(static_cast<std::int8_t>(memory.load8(state.address + kAim)));
    // The flag 0x088E53E8 tests before asking the weapon at all.
    const std::uint32_t game = memory.load32(0x08AB3640u);
    if (memory.raw_pointer(game + 0x60474u, 4u) != nullptr)
        *out << ",aimflag=" << std::hex << memory.load32(game + 0x60474u) << std::dec;
    dump_camera(*out, memory, state.address);
    *out << std::endl;
}

void trace_modes(const psprecomp::GuestMemory &memory, const psprecomp::AllegrexContext &ctx) {
    std::ofstream *trace_out = modes_trace();
    if (trace_out == nullptr) return;
    std::ofstream &out = *trace_out;
    // Only the camera's own calls; the helper also turns every other object.
    if (ctx.gpr[31] != kCameraReturn && ctx.gpr[31] != 0x088E5E24u) return;
    const auto address = ctx.gpr[17];
    const bool camera = memory.raw_pointer(address, 0x190u) != nullptr;
    const Rate stick = rate(Source::Stick);
    out << state.frame << ",ra=" << std::hex << ctx.gpr[31] << ",s1=" << address << std::dec;
    if (memory.raw_pointer(ctx.gpr[5], 12u) != nullptr)
        out << ",angles=" << static_cast<std::int32_t>(memory.load32(ctx.gpr[5])) << ':'
            << static_cast<std::int32_t>(memory.load32(ctx.gpr[5] + 4u)) << ':'
            << static_cast<std::int32_t>(memory.load32(ctx.gpr[5] + 8u));
    out << ",stick=" << stick.yaw << ':' << stick.pitch;
    if (camera) dump_camera(out, memory, address);
    // While the weapon aims, the object the camera follows (s5 at the
    // ordinary call), which carries the aim the camera turns after.
    const auto followed = ctx.gpr[21];
    if (camera && ctx.gpr[31] == kCameraReturn &&
        static_cast<std::int8_t>(memory.load8(address + kAim)) >= 0 &&
        memory.raw_pointer(followed, 0x2000u) != nullptr) {
        out << ",s5=" << std::hex << followed << ",followed=";
        for (std::uint32_t offset = 0u; offset < 0x2000u; offset += 4u)
            out << (offset ? ":" : "") << memory.load32(followed + offset);
        out << std::dec;
    }
    out << '\n';
    out.flush();
}

bool scoped(const psprecomp::GuestMemory &memory, std::uint32_t hunter) {
    return memory.raw_pointer(hunter, kHunterExtent) != nullptr &&
           (memory.load32(hunter + kHunterWeaponFlags) & kScopeFlag) != 0u;
}

// MHP3RD_TRACE_AIM=path.csv: one line per aiming camera update with what the
// game stepped and what the driver made of it, and one per direction the
// mouse showed the game. For telling the game's steps from the driver's.
std::ofstream *aim_trace() {
    static const char *path = std::getenv("MHP3RD_TRACE_AIM");
    if (path == nullptr) return nullptr;
    static std::ofstream out(path);
    return &out;
}

int load_pitch(const psprecomp::GuestMemory &memory, std::uint32_t hunter, const PitchField &field) {
    return field.halfword ? static_cast<std::int16_t>(memory.load16(hunter + field.offset))
                          : static_cast<std::int8_t>(memory.load8(hunter + field.offset));
}

void store_pitch(psprecomp::GuestMemory &memory, std::uint32_t hunter, const PitchField &field, int value) {
    if (field.halfword)
        memory.store16(hunter + field.offset, static_cast<std::uint16_t>(static_cast<std::int16_t>(value)));
    else
        memory.store8(hunter + field.offset, static_cast<std::uint8_t>(static_cast<std::int8_t>(value)));
}

void remember_aim(const psprecomp::GuestMemory &memory, std::uint32_t hunter) {
    state.aim_hunter = hunter;
    state.aim_heading = memory.load16(hunter + kHunterHeading);
    for (std::size_t i = 0; i < kPitchFields.size(); ++i) state.aim_pitch[i] = load_pitch(memory, hunter, kPitchFields[i]);
}

void store_heading(psprecomp::GuestMemory &memory, std::uint32_t hunter, std::uint16_t heading) {
    memory.store16(hunter + kHunterHeading, heading);
    memory.store16(hunter + kHunterYaw, heading);
}

int sign_of(float value) { return value > 0.0f ? 1 : (value < 0.0f ? -1 : 0); }

// The direction the mouse shows the game this frame, each axis -1, 0 or +1:
// every axis with degrees waiting is pushed all the way, so the game steps it
// and the driver sizes the step. A push the game sees as a clear on or off per
// axis is also one whose step can be told in advance.
std::optional<std::pair<int, int>> mouse_direction() {
    if (!game_camera_aim_boost()) return std::nullopt;
    const Turn pending = peek_pointer();
    const float yaw = state.mouse_yaw + pending.yaw_degrees;
    const float pitch = state.mouse_pitch + pending.pitch_degrees;
    const int x = std::fabs(yaw) >= kMouseAimThreshold ? sign_of(yaw) : 0;
    const int y = std::fabs(pitch) >= kMouseAimThreshold ? sign_of(pitch) : 0;
    if (x == 0 && y == 0) return std::nullopt;
    return std::pair{x, y};
}

// Puts back what game_camera_anticipate_aim took off, if it is still there:
// the game has since added its own step to it, or has not run its aim code at
// all. A value the game has set outright (a roll, the aim ending) is left be.
void settle_anticipation(psprecomp::GuestMemory &memory) {
    const std::uint32_t hunter = state.aim_hunter;
    const bool valid = memory.raw_pointer(hunter, kHunterExtent) != nullptr;
    if (valid && state.anticipated_yaw != 0) {
        const auto heading = memory.load16(hunter + kHunterHeading);
        const int game = static_cast<std::int16_t>(static_cast<std::uint16_t>(heading - state.anticipated_heading));
        if (std::abs(game) <= kLargestYawStep)
            store_heading(memory, hunter, static_cast<std::uint16_t>(heading + state.anticipated_yaw));
    }
    if (valid && state.anticipated_pitch != 0) {
        const PitchField &field = kPitchFields[state.pitch_field];
        const int value = load_pitch(memory, hunter, field);
        if (std::abs(value - state.anticipated_pitch_value) <= field.largest_step)
            store_pitch(memory, hunter, field,
                        std::clamp(value + state.anticipated_pitch, -field.limit, field.limit));
    }
    state.anticipated_yaw = 0;
    state.anticipated_pitch = 0;
}

void forget_steps(bool scope) {
    state.learned_scoped = scope;
    state.yaw_step = {};
    state.yaw_sign = 0;
    state.pitch_step = {};
    state.pitch_sign = 0;
}

// Learns how the game steps for the direction it was shown: how far, and
// which way for a positive push.
void learn_steps(bool scope, int game_yaw, int game_pitch, std::size_t pitch_field) {
    if (scope != state.learned_scoped) forget_steps(scope);
    const std::size_t diagonal = state.shown_x != 0 && state.shown_y != 0 ? 1u : 0u;
    if (state.shown_x != 0 && game_yaw != 0) {
        state.yaw_step[diagonal] = std::abs(game_yaw);
        state.yaw_sign = (game_yaw > 0 ? 1 : -1) * state.shown_x;
    }
    if (state.shown_y != 0 && game_pitch != 0) {
        state.pitch_field = pitch_field;
        state.pitch_step[diagonal] = std::abs(game_pitch);
        state.pitch_sign = (game_pitch > 0 ? 1 : -1) * state.shown_y;
    }
}

// A bow or a bowgun aiming, or a bowgun's scope. The stick reaches the game
// stretched to full length (game_camera_aim_boost), so its aim code steps
// whenever the player pushes at all and the game allows it; here each step
// the game made since the previous update is replaced by one in proportion to
// the stick, in the game's direction. No step from the game -- rolling,
// moving, firing, a state that locks an axis -- means no movement from the
// port either. A step made while the right stick is idle (the left stick in
// the scope) is kept as is.
//
// The mouse works the same way through the direction game_camera_mouse_aim()
// shows the game: a step made while the stick is idle and the mouse's
// direction was shown is sized by the mouse's degrees instead, and none left
// means the step is taken back.
void drive_aim(psprecomp::GuestMemory &memory, const psprecomp::AllegrexContext &ctx, std::uint32_t address) {
    const auto hunter = ctx.gpr[21];
    state.address = address;
    const int anticipated_yaw = state.anticipated_yaw;
    const int anticipated_pitch = state.anticipated_pitch;
    settle_anticipation(memory);
    if (!driving_allowed() || memory.raw_pointer(hunter, kHunterExtent) == nullptr) {
        release();
        return;
    }
    // The camera and the stick are the game's while aiming.
    state.available = false;
    state.pitch_owned = false;
    state.yaw_remainder = 0.0f;
    state.last_update = state.frame;
    ++state.updates;
    const Turn mouse = take_pointer();
    const Turn turn = take();
    const bool scope = scoped(memory, hunter);
    if (!state.aiming || state.aim_hunter != hunter) {
        // The first update of an aim only learns where the aim starts.
        state.aiming = true;
        state.aim_yaw_remainder = 0.0f;
        state.aim_pitch_remainder = 0.0f;
        state.mouse_yaw = 0.0f;
        state.mouse_pitch = 0.0f;
        state.mouse_yaw_waits = 0u;
        state.mouse_pitch_waits = 0u;
        forget_steps(scope);
        remember_aim(memory, hunter);
        return;
    }
    state.mouse_yaw = std::clamp(state.mouse_yaw + mouse.yaw_degrees, -kMouseAimCarry, kMouseAimCarry);
    state.mouse_pitch = std::clamp(state.mouse_pitch + mouse.pitch_degrees, -kMouseAimCarry, kMouseAimCarry);
    // The game's step answers the mouse only if the stick was idle and the
    // mouse's direction was on the second stick this frame or the last.
    const bool mouse_shown = state.mouse_shown != 0u && state.frame + 1u - state.mouse_shown <= 1u;
    const bool shown_now = state.mouse_shown == state.frame + 1u;
    bool yaw_spent = false;
    bool pitch_spent = false;

    const float carried_yaw = state.mouse_yaw;
    const auto heading = memory.load16(hunter + kHunterHeading);
    const int game_yaw = static_cast<std::int16_t>(static_cast<std::uint16_t>(heading - state.aim_heading));
    if (game_yaw != 0 && std::abs(game_yaw) <= kLargestYawStep && (turn.yaw_held || mouse_shown)) {
        float degrees = std::fabs(turn.yaw_degrees);
        if (!turn.yaw_held) {
            degrees = std::fabs(state.mouse_yaw);
            state.mouse_yaw = 0.0f;
            yaw_spent = true;
        }
        state.aim_yaw_remainder += degrees * kAngleUnits;
        const int step = static_cast<int>(state.aim_yaw_remainder);
        state.aim_yaw_remainder -= static_cast<float>(step);
        store_heading(memory, hunter, static_cast<std::uint16_t>(state.aim_heading + (game_yaw > 0 ? step : -step)));
    } else if (game_yaw == 0) {
        state.aim_yaw_remainder = 0.0f;
    }

    int game_pitch_seen = 0;
    std::size_t pitch_field_seen = 0u;
    bool pitch_stepped = false;
    for (std::size_t i = 0; i < kPitchFields.size(); ++i) {
        const PitchField &field = kPitchFields[i];
        const int current = load_pitch(memory, hunter, field);
        const int game_pitch = current - state.aim_pitch[i];
        if (game_pitch == 0 || std::abs(game_pitch) > field.largest_step) continue;
        if (game_pitch_seen == 0) {
            game_pitch_seen = game_pitch;
            pitch_field_seen = i;
        }
        if (!(turn.pitch_held || mouse_shown)) continue;
        pitch_stepped = true;
        float degrees = std::fabs(turn.pitch_degrees);
        if (!turn.pitch_held) {
            degrees = std::fabs(state.mouse_pitch);
            pitch_spent = true;
        }
        const float wanted = state.aim_pitch_remainder + degrees;
        const int step = static_cast<int>(wanted * field.units_per_degree);
        state.aim_pitch_remainder = wanted - static_cast<float>(step) / field.units_per_degree;
        const int next = std::clamp(state.aim_pitch[i] + (game_pitch > 0 ? step : -step), -field.limit, field.limit);
        if (next == field.limit || next == -field.limit) state.aim_pitch_remainder = 0.0f;
        store_pitch(memory, hunter, field, next);
    }
    if (!pitch_stepped) state.aim_pitch_remainder = 0.0f;
    // Only a step that answered this frame's direction says how the game
    // answers a direction.
    if (shown_now && !turn.yaw_held && !turn.pitch_held)
        learn_steps(scope, std::abs(game_yaw) <= kLargestYawStep ? game_yaw : 0, game_pitch_seen, pitch_field_seen);
    if (std::ofstream *out = aim_trace()) {
        const int applied = static_cast<std::int16_t>(
            static_cast<std::uint16_t>(memory.load16(hunter + kHunterHeading) - state.aim_heading));
        *out << state.frame << ",update,scope=" << scope << ",stick=" << turn.yaw_degrees << ':'
             << turn.pitch_degrees << ",mouse=" << mouse.yaw_degrees << ':' << mouse.pitch_degrees
             << ",carried=" << carried_yaw << ",shown=" << mouse_shown << ",anticipated=" << anticipated_yaw << ':'
             << anticipated_pitch << ",game=" << game_yaw << ':' << game_pitch_seen << ",yaw=" << applied
             << ",pitch=" << load_pitch(memory, hunter, kPitchFields[0]) - state.aim_pitch[0] << '\n';
    }
    if (pitch_spent) state.mouse_pitch = 0.0f;
    // A stick in use owns the aim; otherwise mouse degrees the game has not
    // stepped for in a few updates are dropped, each axis on its own, so an
    // axis the game keeps still does not save up degrees for a jump later.
    if (turn.yaw_held || turn.pitch_held) {
        state.mouse_yaw = 0.0f;
        state.mouse_pitch = 0.0f;
    }
    const auto wait = [](float &degrees, unsigned &waits, bool spent) {
        if (spent || degrees == 0.0f) {
            waits = 0u;
        } else if (++waits > kMouseAimWaits) {
            degrees = 0.0f;
            waits = 0u;
        }
    };
    wait(state.mouse_yaw, state.mouse_yaw_waits, yaw_spent);
    wait(state.mouse_pitch, state.mouse_pitch_waits, pitch_spent);
    remember_aim(memory, hunter);
}

void adjust_camera(psprecomp::Runtime &runtime, psprecomp::AllegrexContext &ctx) {
    trace_modes(runtime.memory(), ctx);
    if (ctx.gpr[31] != kCameraReturn) return;
    auto &memory = runtime.memory();
    const auto address = ctx.gpr[17];
    const auto stack = ctx.gpr[29];
    if (!memory.raw_pointer(address, 0x188u) || !memory.raw_pointer(stack, 0x190u) ||
        !memory.raw_pointer(ctx.gpr[5], 12u))
        return;
    switch (memory.load8(address + kMode)) {
    case kFollowMode:
        // Aiming a bow or a bowgun, or looking through a bowgun's scope: the
        // stick moves the aim and the game's camera follows it.
        if (static_cast<std::int8_t>(memory.load8(address + kAim)) >= 0 || scoped(memory, ctx.gpr[21])) {
            drive_aim(memory, ctx, address);
            return;
        }
        drive_follow(memory, ctx, address, stack);
        return;
    // A driver for the aiming camera goes here, once traced.
    default:
        release();
        return;
    }
}

void camera_rotation(psprecomp::Runtime &runtime, psprecomp::AllegrexContext &ctx) {
    adjust_camera(runtime, ctx);
    // Continue the original helper with the same CPU context and return PC.
    // Do not invoke an isolated guest call: normal scheduling must be retained.
    state.original(runtime, ctx);
}

} // namespace

std::span<const CodeWord> game_code_signature() { return kSignature; }

bool prepare_game_camera(psprecomp::Runtime &runtime, RotationFunction original) {
    state = State{};
    reset();
    if (!original || !runtime.has_function(kRotationHelper)) {
        std::cerr << "[camera] the rotation helper is not in the generated code; analog camera unavailable\n";
        return false;
    }
    for (const CodeWord &expected : kSignature) {
        const std::uint32_t found = runtime.memory().load32(expected.address);
        if (found == expected.word) continue;
        std::cerr << "[camera] game code differs at 0x" << std::hex << std::uppercase << std::setfill('0')
                  << std::setw(8) << expected.address << ": 0x" << std::setw(8) << found << ", expected 0x"
                  << std::setw(8) << expected.word << std::dec << std::nouppercase << std::setfill(' ') << " ("
                  << expected.what << "); analog camera unavailable\n";
        return false;
    }
    state.original = original;
    state.prepared = true;
    return true;
}

void game_camera_frame(psprecomp::Runtime &runtime) {
    ++state.frame;
    trace_flip(runtime.memory());
    // A step taken off in advance that no aim update has put back: the aim
    // code and the camera did not run, so it goes back now.
    settle_anticipation(runtime.memory());
    if (state.prepared && !state.hooked && option_on()) {
        // Only ever at the game's flip, from an import: no generated frame is
        // live on the host stack, so the dispatch tables can change here.
        // A host registration disables this unit's direct-call shortcut, so the
        // generated cross-unit camera call reaches the wrapper without
        // regeneration. It stays until exit; with the option off again the
        // wrapper only passes through.
        runtime.register_function(kRotationHelper, &camera_rotation, "mhp3rd_camera_rotation");
        state.hooked = true;
    }
    if (state.frame - state.last_update > 1u) release();
    if (!driving_allowed()) {
        state.pitch_owned = false;
        state.yaw_remainder = 0.0f;
    }
    // Nothing takes the input while the camera update is not running.
    if (!game_camera_driving()) discard();
}

bool game_camera_driving() { return driving_allowed() && state.available; }

bool game_camera_aim_boost() {
    return driving_allowed() && state.aiming && state.frame - state.last_update <= 1u;
}

std::optional<StickDirection> game_camera_mouse_aim() {
    const auto direction = mouse_direction();
    if (!direction) return std::nullopt;
    const auto [x, y] = *direction;
    if (std::ofstream *out = aim_trace(); out != nullptr && state.mouse_shown != state.frame + 1u)
        *out << state.frame << ",show," << x << ':' << y << '\n';
    state.mouse_shown = state.frame + 1u;
    state.shown_x = x;
    state.shown_y = y;
    const float length = std::hypot(static_cast<float>(x), static_cast<float>(y));
    return StickDirection{static_cast<float>(x) / length, static_cast<float>(y) / length};
}

void game_camera_anticipate_aim(psprecomp::Runtime &runtime) {
    auto &memory = runtime.memory();
    settle_anticipation(memory);
    const auto direction = mouse_direction();
    const std::uint32_t hunter = state.aim_hunter;
    if (!direction || memory.raw_pointer(hunter, kHunterExtent) == nullptr) return;
    // A held stick is shown to the game instead of the mouse.
    for (const Source source : {Source::Stick, Source::Keys, Source::Touch}) {
        const Rate held = rate(source);
        if (held.yaw != 0.0f || held.pitch != 0.0f) return;
    }
    const auto [x, y] = *direction;
    const std::size_t diagonal = x != 0 && y != 0 ? 1u : 0u;
    // A step size not seen yet in this aim is taken from the other one: the
    // game's diagonal steps are its straight ones times cos 45 degrees.
    const auto step_for = [diagonal](const std::array<int, 2> &steps) {
        if (steps[diagonal] != 0) return steps[diagonal];
        constexpr float kDiagonal = 0.70710678f;
        const float other = static_cast<float>(steps[1u - diagonal]);
        return static_cast<int>(std::lround(diagonal != 0u ? other * kDiagonal : other / kDiagonal));
    };
    const auto heading = memory.load16(hunter + kHunterHeading);
    const int yaw_step = step_for(state.yaw_step);
    if (x != 0 && state.yaw_sign != 0 && yaw_step != 0 && heading == state.aim_heading) {
        state.anticipated_yaw = state.yaw_sign * x * yaw_step;
        state.anticipated_heading = static_cast<std::uint16_t>(heading - state.anticipated_yaw);
        store_heading(memory, hunter, state.anticipated_heading);
    }
    const PitchField &field = kPitchFields[state.pitch_field];
    const int pitch = load_pitch(memory, hunter, field);
    const int pitch_step = step_for(state.pitch_step);
    if (y != 0 && state.pitch_sign != 0 && pitch_step != 0 && pitch == state.aim_pitch[state.pitch_field]) {
        const int ahead = std::clamp(pitch - state.pitch_sign * y * pitch_step, -field.limit, field.limit);
        state.anticipated_pitch = pitch - ahead;
        state.anticipated_pitch_value = ahead;
        if (state.anticipated_pitch != 0) store_pitch(memory, hunter, field, ahead);
    }
}

int game_camera_mouse_stock_turn() {
    if (game_camera_driving() || game_camera_aim_boost()) return 0;
    const float yaw = peek_pointer().yaw_degrees;
    if (!(std::fabs(yaw) >= kMouseStockTurn)) return 0;
    return yaw > 0.0f ? 1 : -1;
}

float game_camera_degrees_per_second() {
    const auto &s = settings::current();
    return state.aiming ? s.aim_speed : s.camera_speed;
}

} // namespace mhp3rd::camera
