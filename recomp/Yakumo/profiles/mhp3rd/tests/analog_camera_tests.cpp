// No game code or game data is needed. The original rotation helper is a
// stand-in which records calls; the real runtime dispatches the camera hook.
#include "camera/camera_input.hpp"
#include "camera/game_camera.hpp"
#include "settings/settings.hpp"
#include "psprecomp/runtime.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace mhp3rd::settings {
Settings &current() {
    static Settings settings;
    return settings;
}
}

namespace {
using namespace mhp3rd::camera;
constexpr float frame_seconds = 1.0f / 30.0f;
constexpr std::uint32_t helper = 0x08878B70u;
constexpr std::uint32_t return_pc = 0x088E626Cu;
constexpr std::uint32_t camera_address = 0x08900000u;
constexpr std::uint32_t stack_address = 0x08901000u;
constexpr std::uint32_t preset_address = 0x08902000u;
int failures{};
unsigned original_calls{};

void check(bool condition, const char *message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void original(psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
    ++original_calls;
    ctx.pc = ctx.gpr[31];
}

void write_float(psprecomp::GuestMemory &memory, std::uint32_t address, float value) {
    memory.store32(address, std::bit_cast<std::uint32_t>(value));
}
float read_float(psprecomp::GuestMemory &memory, std::uint32_t address) {
    return std::bit_cast<float>(memory.load32(address));
}

struct Fixture {
    psprecomp::Runtime runtime;
    psprecomp::AllegrexContext ctx{};
    Fixture() {
        mhp3rd::settings::current() = {};
        // Each test turns the option on itself, to see what changes.
        mhp3rd::settings::current().analog_camera = false;
        runtime.register_generated_unit(29u, 0x08878000u, 0x4000u, &original, nullptr);
        runtime.register_function(helper, &original, "recomp_unit_test");
        auto &memory = runtime.memory();
        // A stand-in for the game code the driver checks before it installs.
        for (const CodeWord &word : game_code_signature()) memory.store32(word.address, word.word);
        check(prepare_game_camera(runtime, &original), "the matching game code is accepted");
        memory.store32(camera_address + 0x70u, preset_address);
        memory.store16(camera_address + 0x80u, 32000u);
        memory.store16(camera_address + 0x82u, 32000u);
        memory.store8(camera_address + 0x91u, 0xFFu);  // not aiming
        write_float(memory, preset_address + 0x10u, 190.0f);
        write_float(memory, camera_address + 4u, 150.0f);
        ctx.gpr[17] = camera_address;
        ctx.gpr[29] = stack_address;
        ctx.gpr[5] = stack_address + 0x50u;
        ctx.gpr[31] = return_pc;
        reset_offset();
    }
    void reset_offset() {
        write_float(runtime.memory(), stack_address + 0x34u, 150.0f);
        write_float(runtime.memory(), stack_address + 0x38u, 490.0f);
    }
    // One game flip followed by the camera update it leads to, as in
    // present_frame(): the stick's rate, the flip, then the time it covers.
    void flip(float x, float y, float seconds = frame_seconds) {
        set_rate(Source::Stick, x, y);
        game_camera_frame(runtime);
        advance(seconds, game_camera_degrees_per_second());
    }
    void update() {
        reset_offset();
        check(runtime.invoke_isolated_aot(helper, ctx), "the rotation helper dispatches");
        check(ctx.pc == ctx.gpr[31], "original helper retains its return PC");
    }
    void frame(float x, float y, float seconds = frame_seconds) {
        flip(x, y, seconds);
        update();
    }
    std::vector<std::uint8_t> snapshot() {
        const auto *p = runtime.memory().raw_pointer(camera_address, 0x2100u);
        return {p, p + 0x2100u};
    }
    void enable() {
        auto &s = mhp3rd::settings::current();
        s.analog_camera = true;
        s.camera_speed = 90.0f;
    }
    float pitch() {
        auto &m = runtime.memory();
        return std::atan2(read_float(m, stack_address + 0x34u) - 190.0f,
                          read_float(m, stack_address + 0x38u)) * 180.0f / 3.14159265358979323846f;
    }
};

void test_passthrough() {
    Fixture f;
    auto before = f.snapshot();
    f.frame(1.0f, 1.0f);
    check(f.snapshot() == before, "Off leaves guest camera, stack and preset unchanged");
    check(!game_camera_driving(), "Off preserves right-stick input");
    f.enable();
    f.ctx.gpr[31] = 0x08812340u;
    f.frame(1.0f, 1.0f);
    check(f.snapshot() == before, "unrelated rotation calls are unchanged");
    f.ctx.gpr[31] = return_pc;
    f.runtime.memory().store8(camera_address + 0x76u, 3u);
    before = f.snapshot();
    f.frame(1.0f, 1.0f);
    check(f.snapshot() == before && !game_camera_driving(), "special camera modes retain control");
}

void test_rates_and_release() {
    Fixture f;
    f.enable();
    const float initial = f.pitch();
    for (int i = 0; i < 20; ++i) f.frame(0.5f, 0.1f);
    const auto yaw = f.runtime.memory().load16(camera_address + 0x80u);
    check(yaw == 26539u, "half stick integrates 30 degrees in twenty camera updates, including fractions");
    check(f.runtime.memory().load16(camera_address + 0x82u) == yaw, "yaw target and filter agree");
    check(std::fabs(f.pitch() - initial - 6.0f) < 0.001f, "small vertical deflections produce proportional pitch");
    const float held = f.pitch();
    for (int i = 0; i < 30; ++i) f.frame(0.0f, 0.0f);
    check(f.runtime.memory().load16(camera_address + 0x80u) == yaw, "released yaw stops accumulating");
    check(std::fabs(f.pitch() - held) < 0.001f, "released pitch holds its angle");
    check(game_camera_driving(), "both digital stick commands are suppressed");
    for (int i = 0; i < 100; ++i) f.frame(0.0f, 1.0f);
    check(std::fabs(f.pitch() - 70.0f) < 0.001f, "upper pitch limit is bounded");
    for (int i = 0; i < 100; ++i) f.frame(0.0f, -1.0f);
    check(std::fabs(f.pitch() + 60.0f) < 0.001f, "lower pitch limit is bounded");
}

void test_ownership() {
    Fixture f;
    f.enable();
    f.frame(0.0f, 1.0f);
    const auto baseline = [&] {
        return read_float(f.runtime.memory(), stack_address + 0x34u) == 150.0f &&
               read_float(f.runtime.memory(), stack_address + 0x38u) == 490.0f;
    };
    f.runtime.memory().store16(camera_address + 0x84u, 0x100u);
    const auto yaw = f.runtime.memory().load16(camera_address + 0x80u);
    f.frame(1.0f, 1.0f);
    check(baseline() && f.runtime.memory().load16(camera_address + 0x80u) == yaw,
          "recentre takes priority over both axes");
    f.runtime.memory().store16(camera_address + 0x84u, 0x10u);
    f.frame(0.0f, 1.0f);
    check(baseline(), "physical D-pad vertical command takes priority");
    f.runtime.memory().store16(camera_address + 0x84u, 0u);
    f.frame(0.0f, 1.0f);
    mhp3rd::settings::current().analog_camera = false;
    f.frame(0.0f, 0.0f);
    check(baseline() && !game_camera_driving(), "Off restores the stock preset and input");
    mhp3rd::settings::current().analog_camera = true;
    f.frame(0.0f, 1.0f);
    for (int i = 0; i < 3; ++i) f.flip(0.0f, 0.0f);
    check(!game_camera_driving(), "leaving the camera releases input ownership");
    f.frame(0.0f, 0.0f);
    check(baseline(), "returning after a scene change discards stale pitch");
    mhp3rd::settings::current().right_stick = mhp3rd::settings::RightStick::DPad;
    const auto before = f.snapshot();
    f.frame(1.0f, 1.0f);
    check(before == f.snapshot() && !game_camera_driving(), "D-pad mapping disables analog integration");
}

void test_dispatch_and_write_extent() {
    Fixture f;
    f.enable();
    const auto before = f.snapshot();
    f.flip(0.5f, 0.5f);
    check(!f.runtime.invoke_chained_direct<&original, 29u, 1u, helper>(f.ctx),
          "generated direct calls unwind to the registered camera hook");
    check(f.ctx.pc == helper, "dispatch fallback preserves the helper address");
    check(f.runtime.invoke_isolated_aot(helper, f.ctx), "outer dispatch reaches the hook");
    const auto after = f.snapshot();
    for (std::size_t i = 0; i < before.size(); ++i) {
        const bool allowed = (i >= 4u && i < 8u) || (i >= 0x80u && i < 0x84u) || (i >= 0x1034u && i < 0x103Cu) ||
                             (i >= 0x1054u && i < 0x1058u);
        if (!allowed) check(before[i] == after[i], "writes stay inside the identified angles and stack arguments");
    }
}

void test_vertical_filter() {
    Fixture f;
    f.enable();
    for (int i = 0; i < 40; ++i) {
        f.frame(0.0f, i < 20 ? 0.25f : 0.0f);
        auto &m = f.runtime.memory();
        const float target = read_float(m, stack_address + 0x34u);
        const float current = read_float(m, camera_address + 4u);
        // The measured game filter must not introduce latency into stick
        // movement or continue that movement after release.
        const float filtered = current + (target - current) * 0.125f;
        check(std::fabs(filtered - target) < 0.001f, "manual vertical movement leaves no filter catch-up");
        write_float(m, camera_address + 4u, filtered);
    }
}
}

void test_signature_mismatch() {
    Fixture f;
    const CodeWord &first = game_code_signature().front();
    f.runtime.memory().store32(first.address, first.word ^ 1u);
    check(!prepare_game_camera(f.runtime, &original), "different game code is refused");
    f.enable();
    const auto before = f.snapshot();
    f.frame(1.0f, 1.0f);
    check(f.snapshot() == before && !game_camera_driving(), "refused code is never written to");
}

void test_hook_waits_for_the_option() {
    Fixture f;
    f.flip(0.0f, 0.0f);
    check(f.runtime.invoke_chained_direct<&original, 29u, 1u, helper>(f.ctx),
          "with the option off the helper's unit keeps its direct calls");
    f.enable();
    f.flip(0.0f, 0.0f);
    check(!f.runtime.invoke_chained_direct<&original, 29u, 1u, helper>(f.ctx),
          "turning the option on installs the hook");
}

void test_frame_rate_independence() {
    const auto turn_for_one_second = [](int updates) {
        Fixture f;
        f.enable();
        f.frame(0.0f, 0.0f);
        const auto start = f.runtime.memory().load16(camera_address + 0x80u);
        for (int i = 0; i < updates; ++i) f.frame(0.5f, 0.0f, 1.0f / static_cast<float>(updates));
        f.frame(0.0f, 0.0f);
        return static_cast<int>(start) - static_cast<int>(f.runtime.memory().load16(camera_address + 0x80u));
    };
    const int at_30 = turn_for_one_second(30);
    const int at_20 = turn_for_one_second(20);
    // Half deflection at 90 degrees a second: 45 degrees in either case.
    check(std::abs(at_30 - 8192) <= 1, "a second of stick turns 45 degrees at 30 updates a second");
    check(std::abs(at_20 - at_30) <= 1, "a slower game turns the camera the same amount in the same time");
}

void test_motion_source() {
    Fixture f;
    f.enable();
    f.frame(0.0f, 0.0f);
    const auto start = f.runtime.memory().load16(camera_address + 0x80u);
    add_motion(Source::Mouse, 10.0f, 0.0f);
    f.frame(0.0f, 0.0f);
    const int turned = static_cast<int>(start) - static_cast<int>(f.runtime.memory().load16(camera_address + 0x80u));
    check(std::abs(turned - 1820) <= 1, "motion turns by its degrees once");
    f.frame(0.0f, 0.0f);
    check(static_cast<int>(start) - static_cast<int>(f.runtime.memory().load16(camera_address + 0x80u)) == turned,
          "motion is not repeated on the next update");
    add_motion(Source::Mouse, 10.0f, 0.0f);
    mhp3rd::settings::current().analog_camera = false;
    f.frame(0.0f, 0.0f);
    mhp3rd::settings::current().analog_camera = true;
    f.frame(0.0f, 0.0f);
    check(static_cast<int>(start) - static_cast<int>(f.runtime.memory().load16(camera_address + 0x80u)) == turned,
          "motion made while the camera is not driven is dropped");
}

// The weapon's aim code, as far as the driver sees it: while its commands are
// on it steps the facing and one vertical aim by fixed amounts, and in some
// states it does not step at all.
struct GameAim {
    std::uint32_t hunter;
    int yaw_step{};    // +-624 while a horizontal command is on
    int pitch_step{};  // +-8 while a vertical command is on
    std::uint32_t pitch_offset{0xC22u};
    void step(psprecomp::GuestMemory &m) const {
        const auto heading = static_cast<std::uint16_t>(m.load16(hunter + 0x188u) + yaw_step);
        m.store16(hunter + 0x188u, heading);
        m.store16(hunter + 0x74u, heading);
        if (pitch_offset == 0xC24u) {
            const int v = std::clamp(static_cast<std::int16_t>(m.load16(hunter + 0xC24u)) + pitch_step * 64, -8192, 8192);
            m.store16(hunter + 0xC24u, static_cast<std::uint16_t>(static_cast<std::int16_t>(v)));
        } else {
            const int v = std::clamp(static_cast<std::int8_t>(m.load8(hunter + pitch_offset)) + pitch_step, -100, 100);
            m.store8(hunter + pitch_offset, static_cast<std::uint8_t>(static_cast<std::int8_t>(v)));
        }
    }
};

void test_aiming_sizes_the_games_steps() {
    constexpr std::uint32_t hunter = 0x08904000u;
    Fixture f;
    f.enable();
    auto &m = f.runtime.memory();
    f.ctx.gpr[21] = hunter;
    m.store16(hunter + 0x188u, 20000u);
    m.store16(hunter + 0x74u, 20000u);
    GameAim game{hunter};
    const auto aim_frame = [&](float x, float y) {
        f.flip(x, y);
        game.step(m);
        f.update();
    };
    f.frame(0.0f, 0.0f);
    m.store8(camera_address + 0x91u, 0u);  // the weapon reports an aim
    f.reset_offset();
    const auto camera_before = f.snapshot();
    mhp3rd::settings::current().aim_speed = 60.0f;
    aim_frame(0.0f, 0.0f);  // learns where the aim starts; picks up Aim speed
    check(game_camera_aim_boost() && !game_camera_driving(), "aiming sends the stretched stick to the game");

    // The game steps left by 624 each frame; the push is half: 30 degrees in a second.
    game.yaw_step = 624;
    for (int i = 0; i < 30; ++i) aim_frame(-0.5f, 0.0f);
    f.reset_offset();
    check(f.snapshot() == camera_before, "aiming leaves the camera to the game");
    const int turned = static_cast<int>(m.load16(hunter + 0x188u)) - 20000;
    check(std::abs(turned - 5461) <= 2, "each game step is replaced by one in proportion to the stick");
    check(m.load16(hunter + 0x74u) == m.load16(hunter + 0x188u), "the facing and its copy agree");

    // A roll, or any state where the game does not step: the aim stays.
    game.yaw_step = 0;
    const auto held = m.load16(hunter + 0x188u);
    for (int i = 0; i < 10; ++i) aim_frame(-1.0f, 0.0f);
    check(m.load16(hunter + 0x188u) == held, "no step from the game, no movement from the port");

    // The game's step with the right stick idle (the left stick in the scope) is kept.
    game.yaw_step = -624;
    aim_frame(0.0f, 0.0f);
    check(m.load16(hunter + 0x188u) == static_cast<std::uint16_t>(held - 624), "a step made by another input is kept");

    // Vertical: a quarter push up, the game steps +8; a third of a second at 60 deg/s is 5 degrees.
    game.yaw_step = 0;
    game.pitch_step = 8;
    for (int i = 0; i < 10; ++i) aim_frame(0.0f, -0.25f);
    const int up = static_cast<std::int8_t>(m.load8(hunter + 0xC22u));
    check(std::abs(up - 12) <= 1, "a small push up raises the aim slowly");
    for (int i = 0; i < 60; ++i) aim_frame(0.0f, -1.0f);
    check(static_cast<std::int8_t>(m.load8(hunter + 0xC22u)) == 100, "the aim stops at the game's limit");
    // Moving while aiming: the game makes no vertical step, so neither does the port.
    game.pitch_step = 0;
    for (int i = 0; i < 10; ++i) aim_frame(0.0f, 1.0f);
    check(static_cast<std::int8_t>(m.load8(hunter + 0xC22u)) == 100, "a locked axis stays locked");

    // The halfword vertical aim scales the same way.
    game.pitch_step = -8;
    game.pitch_offset = 0xC24u;
    for (int i = 0; i < 10; ++i) aim_frame(0.0f, 0.25f);
    const int down = static_cast<std::int16_t>(m.load16(hunter + 0xC24u));
    check(std::abs(down + 12 * 64) <= 64, "the halfword vertical aim is sized alike");

    mhp3rd::settings::current().analog_camera = false;
    game.yaw_step = 624;
    const auto before_off = m.load16(hunter + 0x188u);
    aim_frame(0.2f, 0.0f);
    check(!game_camera_aim_boost() && m.load16(hunter + 0x188u) == static_cast<std::uint16_t>(before_off + 624),
          "with the option off the game's aim is untouched");
    mhp3rd::settings::current().analog_camera = true;
    m.store8(camera_address + 0x91u, 0xFFu);
    f.frame(0.0f, 0.0f);
    f.frame(1.0f, 0.0f);
    check(game_camera_driving() && !game_camera_aim_boost(), "the ordinary camera is driven again after aiming");
}

// The mouse while aiming: it has no stick, so the direction it shows the game
// makes the game step, and the driver sizes the step from its degrees.
void test_mouse_aim() {
    constexpr std::uint32_t hunter = 0x08904000u;
    Fixture f;
    f.enable();
    auto &m = f.runtime.memory();
    f.ctx.gpr[21] = hunter;
    m.store16(hunter + 0x188u, 20000u);
    m.store16(hunter + 0x74u, 20000u);
    GameAim game{hunter};
    // One frame: the flip, the mouse's motion from the pump after it, the
    // ctrl read (the direction the game sees), the game's aim code, which
    // steps its own way if it sees a push (or late, by `late`), and the
    // camera update.
    bool pending_step = false;
    const auto mouse_frame = [&](float yaw, float pitch, bool late = false, bool game_steps = true) {
        f.flip(0.0f, 0.0f);
        if (yaw != 0.0f || pitch != 0.0f) add_motion(Source::Mouse, yaw, pitch);
        game_camera_anticipate_aim(f.runtime);
        const auto shown = game_camera_mouse_aim();
        game.yaw_step = 0;
        game.pitch_step = 0;
        const bool step_now = late ? pending_step : shown.has_value();
        pending_step = shown.has_value();
        if (step_now && game_steps) {
            const auto &d = shown ? *shown : StickDirection{1.0f, 0.0f};
            if (std::fabs(d.x) >= std::fabs(d.y)) game.yaw_step = d.x > 0.0f ? 624 : -624;
            else game.pitch_step = d.y > 0.0f ? -8 : 8;
        }
        game.step(m);
        f.update();
    };
    f.frame(0.0f, 0.0f);
    m.store8(camera_address + 0x91u, 0u);
    mouse_frame(0.0f, 0.0f);  // learns where the aim starts
    check(game_camera_aim_boost() && !game_camera_mouse_aim(), "an idle mouse shows the game nothing");

    for (int i = 0; i < 10; ++i) mouse_frame(3.0f, 0.0f);
    int turned = static_cast<int>(m.load16(hunter + 0x188u)) - 20000;
    check(std::abs(turned - 5461) <= 2, "each step the mouse makes the game take is sized by its degrees");

    // The game steps an update after it saw the push: the degrees wait for it,
    // and a step after the mouse has stopped is taken back.
    const auto before_late = m.load16(hunter + 0x188u);
    pending_step = false;
    mouse_frame(5.0f, 0.0f, true);
    mouse_frame(0.0f, 0.0f, true);
    turned = static_cast<int>(m.load16(hunter + 0x188u)) - static_cast<int>(before_late);
    check(std::abs(turned - 910) <= 1, "a late step still gets the mouse's degrees");
    const auto after_late = m.load16(hunter + 0x188u);
    mouse_frame(0.0f, 0.0f, true);
    mouse_frame(0.0f, 0.0f, true);
    check(m.load16(hunter + 0x188u) == after_late, "a step with nothing left to spend is taken back");

    // Vertical: the game's aim is up for a mouse pushed forward.
    game.pitch_offset = 0xC22u;
    const int pitch_before = static_cast<std::int8_t>(m.load8(hunter + 0xC22u));
    for (int i = 0; i < 4; ++i) mouse_frame(0.0f, -1.25f);
    const int raised = static_cast<std::int8_t>(m.load8(hunter + 0xC22u)) - pitch_before;
    check(std::abs(raised - 12) <= 1, "vertical mouse motion is sized like the stick");

    // A roll: the game makes no step; the mouse's degrees are not saved up.
    const auto rolling = m.load16(hunter + 0x188u);
    for (int i = 0; i < 6; ++i) mouse_frame(4.0f, 0.0f, false, false);
    check(m.load16(hunter + 0x188u) == rolling, "no step from the game, no movement from the mouse");
    mouse_frame(0.0f, 0.0f);
    const int after_roll = static_cast<int>(m.load16(hunter + 0x188u)) - static_cast<int>(rolling);
    check(std::abs(after_roll) < 16 * 182, "degrees the game never stepped for are not all saved up");

    // Another input's step with the mouse idle is kept.
    for (int i = 0; i < 3; ++i) mouse_frame(0.0f, 0.0f);
    const auto idle = m.load16(hunter + 0x188u);
    f.flip(0.0f, 0.0f);
    game.yaw_step = -624;
    game.pitch_step = 0;
    game.step(m);
    f.update();
    check(m.load16(hunter + 0x188u) == static_cast<std::uint16_t>(idle - 624), "a step by another input is kept");

    m.store8(camera_address + 0x91u, 0xFFu);
    f.frame(0.0f, 0.0f);
    add_motion(Source::Mouse, 3.0f, 0.0f);
    check(!game_camera_mouse_aim(), "out of the aim the mouse shows the game nothing");
}

// The game's camera, and the scope's view, read the aim after the game's own
// step and before the driver resizes it. With the mouse the game's step comes
// and goes, so what the camera reads must already be the resized aim of the
// previous update, not that plus a step of 624 in whichever way the game went.
void test_mouse_aim_view_is_steady() {
    constexpr std::uint32_t hunter = 0x08904000u;
    Fixture f;
    f.enable();
    auto &m = f.runtime.memory();
    f.ctx.gpr[21] = hunter;
    m.store16(hunter + 0x188u, 20000u);
    m.store16(hunter + 0x74u, 20000u);
    GameAim game{hunter};
    std::vector<int> seen;     // what the camera reads, after the game's step
    std::vector<int> settled;  // the aim after the driver
    std::vector<int> seen_pitch;
    std::vector<int> settled_pitch;
    const auto mouse_frame = [&](float yaw, float pitch) {
        f.flip(0.0f, 0.0f);
        if (yaw != 0.0f || pitch != 0.0f) add_motion(Source::Mouse, yaw, pitch);
        game_camera_anticipate_aim(f.runtime);
        const auto shown = game_camera_mouse_aim();
        game.yaw_step = 0;
        game.pitch_step = 0;
        if (shown) {
            // The game's diagonal steps are shorter, as in the game.
            const bool diagonal = shown->x != 0.0f && shown->y != 0.0f;
            if (shown->x != 0.0f) game.yaw_step = (shown->x > 0.0f ? 1 : -1) * (diagonal ? 441 : 624);
            if (shown->y != 0.0f) game.pitch_step = (shown->y > 0.0f ? -1 : 1) * (diagonal ? 5 : 8);
        }
        game.step(m);
        seen.push_back(static_cast<std::int16_t>(m.load16(hunter + 0x74u)));
        seen_pitch.push_back(static_cast<std::int8_t>(m.load8(hunter + 0xC22u)));
        f.update();
        settled.push_back(static_cast<std::int16_t>(m.load16(hunter + 0x188u)));
        settled_pitch.push_back(static_cast<std::int8_t>(m.load8(hunter + 0xC22u)));
    };
    f.frame(0.0f, 0.0f);
    m.store8(camera_address + 0x91u, 0u);
    mouse_frame(0.0f, 0.0f);
    // The first step teaches the driver the game's; after that, a hand that
    // speeds up, stops, reverses and goes diagonal.
    const std::vector<std::pair<float, float>> motion{
        {0.5f, 0.0f}, {0.2f, 0.0f}, {0.0f, 0.0f}, {0.3f, 0.0f}, {-0.4f, 0.0f}, {-1.5f, 0.0f}, {0.0f, 0.0f},
        {0.0f, 0.0f}, {0.8f, -0.9f}, {0.8f, 0.0f}, {-0.2f, -1.4f}, {0.1f, 0.0f}, {0.0f, 1.2f}, {0.0f, -3.0f},
        {-2.0f, 0.1f}};
    for (const auto &[yaw, pitch] : motion) mouse_frame(yaw, pitch);
    // From the second step on, each frame the camera reads the previous
    // settled aim (nothing of the game's own step shows).
    int worst = 0;
    int worst_pitch = 0;
    for (std::size_t i = 3; i < seen.size(); ++i) {
        worst = std::max(worst, std::abs(static_cast<std::int16_t>(static_cast<std::uint16_t>(seen[i] - settled[i - 1]))));
        // The first vertical step only teaches the driver the game's.
        if (i > 9u) worst_pitch = std::max(worst_pitch, std::abs(seen_pitch[i] - settled_pitch[i - 1]));
    }
    // A step size not seen yet is estimated from the other (8 from 5 is 7).
    check(worst <= 1, "the camera never sees the game's own mouse-aim step");
    check(worst_pitch <= 1, "nor its own vertical step");
    int total = 0;
    for (const auto &[yaw, pitch] : motion) total += static_cast<int>(std::lround(yaw * 65536.0f / 360.0f));
    const int turned = static_cast<std::int16_t>(static_cast<std::uint16_t>(settled.back() - 20000));
    check(std::abs(turned - total) <= 3, "the settled aim follows the mouse's degrees");
    // 4.1 degrees down at 8 / 3.43 units a degree, the game's aim up for a push up.
    check(std::abs(static_cast<std::int8_t>(m.load8(hunter + 0xC22u)) - 9) <= 1, "and the vertical aim moves with it");

    // The game stops stepping (a roll): nothing taken off in advance stays.
    const auto before_roll = m.load16(hunter + 0x188u);
    f.flip(0.0f, 0.0f);
    add_motion(Source::Mouse, 1.0f, 0.0f);
    game_camera_anticipate_aim(f.runtime);
    (void)game_camera_mouse_aim();
    game.yaw_step = 0;
    game.pitch_step = 0;
    game.step(m);
    f.update();
    check(m.load16(hunter + 0x188u) == before_roll, "a step the game did not make is put back");

    // The aim ends before the game steps: the next flip puts the aim back.
    f.flip(0.0f, 0.0f);
    add_motion(Source::Mouse, 1.0f, 0.0f);
    game_camera_anticipate_aim(f.runtime);
    check(m.load16(hunter + 0x188u) != before_roll, "the step is taken off in advance");
    m.store8(camera_address + 0x91u, 0xFFu);
    f.flip(0.0f, 0.0f);
    check(m.load16(hunter + 0x188u) == before_roll && m.load16(hunter + 0x74u) == before_roll,
          "an anticipation no update used is undone at the next flip");
}

// A bowgun's scope: the camera stays in its ordinary mode and the weapon
// reports no aim, but the scope flag in the hunter makes it an aim: the second
// stick goes to the game and the mouse steers the scope.
void test_scope_is_an_aim() {
    constexpr std::uint32_t hunter = 0x08904000u;
    Fixture f;
    f.enable();
    auto &m = f.runtime.memory();
    f.ctx.gpr[21] = hunter;
    m.store16(hunter + 0x188u, 20000u);
    m.store16(hunter + 0x74u, 20000u);
    GameAim game{hunter};
    f.frame(0.0f, 0.0f);
    check(game_camera_driving(), "the ordinary camera is driven");
    m.store32(hunter + 0xBB0u, 0x1000u);  // the scope is up; +0x91 stays -1
    f.frame(0.0f, 0.0f);
    check(!game_camera_driving() && game_camera_aim_boost(), "in the scope the stick goes to the game");
    const auto yaw_before = m.load16(camera_address + 0x80u);
    for (int i = 0; i < 10; ++i) {
        f.flip(0.0f, 0.0f);
        add_motion(Source::Mouse, 2.0f, 0.0f);
        game_camera_anticipate_aim(f.runtime);
        const auto shown = game_camera_mouse_aim();
        check(shown.has_value(), "the mouse shows the scope a direction");
        game.yaw_step = shown && shown->x > 0.0f ? 400 : 0;  // the scope's own step
        game.step(m);
        f.update();
    }
    const int turned = static_cast<int>(m.load16(hunter + 0x188u)) - 20000;
    check(std::abs(turned - 3641) <= 2, "the scope turns by the mouse's degrees");
    check(m.load16(camera_address + 0x80u) == yaw_before, "the follow camera is left alone in the scope");
    m.store32(hunter + 0xBB0u, 0u);
    f.frame(0.0f, 0.0f);
    f.frame(0.0f, 0.0f);
    check(game_camera_driving() && !game_camera_aim_boost(), "leaving the scope drives the camera again");
}

// A mouse moved mostly sideways: the vertical axis is shown too, so the game
// steps it alongside and its degrees never pile up into a jump later.
void test_mouse_axes_do_not_pile_up() {
    constexpr std::uint32_t hunter = 0x08904000u;
    Fixture f;
    f.enable();
    auto &m = f.runtime.memory();
    f.ctx.gpr[21] = hunter;
    m.store16(hunter + 0x188u, 20000u);
    GameAim game{hunter};
    f.frame(0.0f, 0.0f);
    m.store8(camera_address + 0x91u, 0u);
    f.frame(0.0f, 0.0f);
    int largest = 0;
    int previous = 0;
    for (int i = 0; i < 40; ++i) {
        f.flip(0.0f, 0.0f);
        add_motion(Source::Mouse, 3.0f, i < 30 ? -0.2f : -2.0f);
        game_camera_anticipate_aim(f.runtime);
        const auto shown = game_camera_mouse_aim();
        game.yaw_step = shown && shown->x != 0.0f ? (shown->x > 0.0f ? 624 : -624) : 0;
        // This game only steps vertically while nothing is sideways, the way a
        // weapon may lock an axis.
        game.pitch_step = shown && shown->x == 0.0f && shown->y != 0.0f ? (shown->y > 0.0f ? -8 : 8) : 0;
        game.step(m);
        f.update();
        const int pitch = static_cast<std::int8_t>(m.load8(hunter + 0xC22u));
        largest = std::max(largest, std::abs(pitch - previous));
        previous = pitch;
    }
    check(largest == 0, "vertical degrees the game never stepped for are dropped, not saved up");
    f.flip(0.0f, 0.0f);
    add_motion(Source::Mouse, 0.0f, -1.0f);
    game_camera_anticipate_aim(f.runtime);
    const auto shown = game_camera_mouse_aim();
    game.yaw_step = 0;
    game.pitch_step = shown && shown->y != 0.0f ? (shown->y > 0.0f ? -8 : 8) : 0;
    game.step(m);
    f.update();
    const int raised = static_cast<std::int8_t>(m.load8(hunter + 0xC22u));
    check(raised > 0 && raised <= 4, "a later vertical motion moves the aim by its own degrees only");
}

// Where the port does not drive the camera the mouse switches the game's turn.
void test_mouse_stock_turn() {
    Fixture f;
    f.flip(0.0f, 0.0f);
    add_motion(Source::Mouse, 0.3f, 5.0f);
    check(game_camera_mouse_stock_turn() == 0, "slow or vertical motion does not turn the game's camera");
    add_motion(Source::Mouse, 0.4f, 0.0f);
    check(game_camera_mouse_stock_turn() == 1, "sideways motion switches the turn on that way");
    f.flip(0.0f, 0.0f);
    check(game_camera_mouse_stock_turn() == 0, "and only for the frame it moved in");
    add_motion(Source::Mouse, -2.0f, 0.0f);
    check(game_camera_mouse_stock_turn() == -1, "left turns left");
    f.enable();
    f.frame(0.0f, 0.0f);
    f.flip(0.0f, 0.0f);
    add_motion(Source::Mouse, 2.0f, 0.0f);
    check(game_camera_mouse_stock_turn() == 0, "while the port drives the camera the mouse goes to the driver");
    f.update();
}

int main() {
    test_passthrough();
    test_rates_and_release();
    test_ownership();
    test_dispatch_and_write_extent();
    test_vertical_filter();
    test_signature_mismatch();
    test_hook_waits_for_the_option();
    test_frame_rate_independence();
    test_motion_source();
    test_aiming_sizes_the_games_steps();
    test_mouse_aim();
    test_mouse_aim_view_is_steady();
    test_scope_is_an_aim();
    test_mouse_axes_do_not_pile_up();
    test_mouse_stock_turn();
    check(original_calls > 0u, "original rotation helper is called");
    std::cout << (failures ? "FAIL" : "PASS") << ": analog camera (" << failures << " failures)\n";
    return failures ? 1 : 0;
}
