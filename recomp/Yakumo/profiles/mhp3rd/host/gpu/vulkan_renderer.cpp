#include "vulkan_renderer.hpp"

#include "frame_interpolation.hpp"
#include "frame_pacing.hpp"
#include "replacement_textures.hpp"
#include "texture_decode.hpp"
#include "triangle_indices.hpp"
#include "texture_pack.hpp"
#include "texture_pack_import.hpp"

#include "install/game_identity.hpp"
#include "install/user_data.hpp"

#include "perf/frame_stats.hpp"
#include "perf/perf_overlay.hpp"
#include "input/bindings.hpp"
#include "settings/settings.hpp"

#if defined(MHP3RD_ANDROID_APP)
#include "platform/android_jni.hpp"
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

#include <atomic>
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <bit>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <set>
#include <thread>
#include <vector>

namespace mhp3rd::gpu {
namespace {

constexpr std::uint32_t kPspWidth = 480u;
constexpr std::uint32_t kPspHeight = 272u;
// Auto resolution draws the game at the window's size in pixels, up to this
// many lines: 6 x 272, the largest multiple the menu offers.
constexpr std::uint32_t kMaxAutoLines = 6u * kPspHeight;
// Widest target, in pixels; every Vulkan device takes images this wide.
constexpr std::uint32_t kMaxTargetWidth = 8192u;
// Frames a new window size has to hold before the targets follow it, so
// dragging a window's edge does not rebuild them on every frame.
constexpr std::uint32_t kSettleFrames = 12u;
// A through-mode draw at least this wide (in PSP pixels) covers the screen:
// a fade, a backdrop or a copy of the picture, which spreads with the 3D view
// instead of keeping the interface's proportions.
constexpr float kScreenWideDraw = 470.0f;
// Room for one frame's vertices. With frame interpolation a frame cannot reuse
// its region part way through, so all of it must fit: measured at 16.1 MiB in
// the busiest areas found (Flooded Forest area 3, Tundra area 2), where 16 MiB
// left the interface, drawn last, and parts of the ground out. Twice that.
constexpr VkDeviceSize kVertexBufferBytes = 32u * 1024u * 1024u;
// Index lists of merged draws (see submit()), kept apart from the vertices so
// that the draws of one group have consecutive indices.
constexpr VkDeviceSize kIndexBufferBytes = 4u * 1024u * 1024u;
// Frame interpolation draws a frame again after the game has moved on, from
// the vertices, indices and lighting blocks that frame wrote. So the vertex
// and index buffers hold three frames' worth: the frame being drawn, the
// newer and the older of the two being blended. Without interpolation only
// the first region is used, as before. After them, the vertex buffer has a
// scratch area for each of the two presents that can be in flight, for the
// blended vertices of skinned draws and the lighting blocks of blended draws.
constexpr std::uint32_t kFrameRegions = 3u;
constexpr VkDeviceSize kPresentScratchBytes = 6u * 1024u * 1024u;
constexpr VkDeviceSize kVertexBufferTotal = kFrameRegions * kVertexBufferBytes + 2u * kPresentScratchBytes;
constexpr VkDeviceSize kIndexBufferTotal = kFrameRegions * kIndexBufferBytes;

// Says, once in a while, that a frame's geometry did not fit and some of its
// draws were left out, instead of dropping them silently.
void report_frame_space_full(const char *what) {
    static std::uint64_t count = 0;
    if (count++ % 600u == 0u)
        std::cout << "[render] a frame ran out of " << what << " space; the draws past it are not drawn (" << count
                  << " so far)\n" << std::flush;
}
constexpr std::size_t kMaxCachedTextures = 1024u;
// Descriptor sets for sampling render targets as textures: two per target
// (with its alpha, and with alpha forced to one for 5650 textures).
constexpr std::size_t kMaxFramebufferTextureSets = 64u;
// GPU timestamps: a pair around each command buffer a frame submits. A frame
// normally submits one; each GE block transfer that reads a framebuffer back
// splits it once more.
constexpr std::uint32_t kGpuTimerSegments = 16u;

// Compiled SPIR-V, generated from host/gpu/shaders by the build.
#include "ge_shaders.inc"

struct PushConstants {
    std::array<float, 16> transform{};
    std::array<float, 4> viewport{};        // x,y: target size; z: through flag; w: 1 fog + 2 lighting
    std::array<float, 4> texture_params{};  // x: enabled, y: function, z: alpha ref, w: alpha func
    std::array<float, 4> uv_transform{1.0f, 1.0f, 0.0f, 0.0f};
    std::array<float, 4> view_z{};          // row of view * world that gives view-space z, for fog
};

// 128 bytes is the most every Vulkan implementation has to accept.
static_assert(sizeof(PushConstants) == 128u, "PushConstants must fit the guaranteed push constant size");
constexpr float kPushFog = 1.0f;
constexpr float kPushLighting = 2.0f;

struct GpuVertex {
    float x{}, y{}, z{}, w{1.0f};
    float u{}, v{};
    std::uint32_t color{};
    float nx{}, ny{}, nz{};
};

constexpr float kNoClamp = 1e30f;

void set_uv_rect(GpuVertex &vertex, float u_min, float v_min, float u_max, float v_max) {
    vertex.nx = u_min;
    vertex.ny = v_min;
    vertex.nz = u_max;
    vertex.w = v_max;
}

// A through-mode tile is an axis-aligned rectangle of pixels showing an
// axis-aligned rectangle of texels, usually a piece of an atlas. At 480x272
// the raster samples the texels only between the centres of its edge pixels;
// at a higher internal resolution it also samples between those centres and
// the edges, where bilinear filtering pulls in the neighbouring atlas texels
// (or the far side of the texture) and leaves a faint grid along the tile
// edges. The six vertices from `first` get the texel range the 480x272
// raster samples, which the fragment shader clamps to. That range contains
// every sample a 480x272 target takes, so at x1 nothing changes.
void clamp_through_quad(std::vector<GpuVertex> &vertices, std::size_t first, float texture_width,
                        float texture_height) {
    if (first + 6u > vertices.size()) return;
    float x0 = vertices[first].x, x1 = x0, y0 = vertices[first].y, y1 = y0;
    for (std::size_t i = first; i < first + 6u; ++i) {
        x0 = std::min(x0, vertices[i].x);
        x1 = std::max(x1, vertices[i].x);
        y0 = std::min(y0, vertices[i].y);
        y1 = std::max(y1, vertices[i].y);
    }
    if (x1 - x0 < 1.0f || y1 - y0 < 1.0f) return;
    // Texture coordinates must follow x and y alone: one u at each vertical
    // edge, one v at each horizontal edge.
    float u_at_x0 = 0.0f, u_at_x1 = 0.0f, v_at_y0 = 0.0f, v_at_y1 = 0.0f;
    bool seen[4]{};
    for (std::size_t i = first; i < first + 6u; ++i) {
        const GpuVertex &vertex = vertices[i];
        const bool left = vertex.x == x0, top = vertex.y == y0;
        if (!left && vertex.x != x1) return;
        if (!top && vertex.y != y1) return;
        float &u = left ? u_at_x0 : u_at_x1;
        float &v = top ? v_at_y0 : v_at_y1;
        bool &u_seen = seen[left ? 0 : 1];
        bool &v_seen = seen[top ? 2 : 3];
        if (u_seen && u != vertex.u) return;
        if (v_seen && v != vertex.v) return;
        u = vertex.u;
        v = vertex.v;
        u_seen = v_seen = true;
    }
    const float u_min = std::min(u_at_x0, u_at_x1), u_max = std::max(u_at_x0, u_at_x1);
    const float v_min = std::min(v_at_y0, v_at_y1), v_max = std::max(v_at_y0, v_at_y1);
    // A range beyond the texture repeats it on purpose.
    if (u_min < 0.0f || v_min < 0.0f || u_max > texture_width || v_max > texture_height) return;
    // Half a pixel, in texels.
    const float inset_u = 0.5f * (u_max - u_min) / (x1 - x0);
    const float inset_v = 0.5f * (v_max - v_min) / (y1 - y0);
    for (std::size_t i = first; i < first + 6u; ++i)
        set_uv_rect(vertices[i], u_min + inset_u, v_min + inset_v, u_max - inset_u, v_max - inset_v);
}

// Lighting reaches the shaders in two std140 uniform blocks that live in the
// vertex buffer and are bound through dynamic offsets (set 1).
//
// The environment is what the game changes a few times a frame: the global
// ambient light, the four lights and the fog parameters. It is written once per
// change of GeState's environment version and shared by every draw after it.
struct EnvironmentBlock {
    std::array<float, 4> ambient{};
    std::array<float, 4> fog{};
    std::array<float, 4> fog_color{};
    std::array<std::array<float, 4>, 4> light_position{};
    std::array<std::array<float, 4>, 4> light_direction{};
    std::array<std::array<float, 4>, 4> light_attenuation{};
    std::array<std::array<float, 4>, 4> light_spot{};
    std::array<std::array<float, 4>, 4> light_ambient{};
    std::array<std::array<float, 4>, 4> light_diffuse{};
    std::array<std::array<float, 4>, 4> light_specular{};
};

static_assert(sizeof(EnvironmentBlock) == 496u, "EnvironmentBlock must match the std140 layout in ge.vert");

// What a lit draw adds: its world matrix and material. Consecutive draws of
// one mesh share it, so it is written only when it differs from the last one.
struct ObjectBlock {
    std::array<float, 16> world{};
    std::array<float, 4> flags{};             // y: vertex colour, w: material update mask
    std::array<float, 4> emissive{};          // w: specular power
    std::array<float, 4> material_ambient{};
    std::array<float, 4> material_diffuse{};  // w: separate specular
    std::array<float, 4> material_specular{}; // w: reverse normals
};

static_assert(sizeof(ObjectBlock) == 144u, "ObjectBlock must match the std140 layout in ge.vert");

// GPU vertex decode (MHP3RD_GPU_DECODE): what the raw vertex shader needs to
// decode and skin a draw's own bytes; the layout matches the Raw block in
// ge.vert. Only the bones the vertex type weights are written.
constexpr std::uint32_t kRawNoField = 0xFFu;
struct RawBlock {
    std::array<std::uint32_t, 4> format{};  // stride, vertex type, packed field offsets, position offset
    std::array<std::uint32_t, 4> extra{};   // colour of a vertex without one, check slot
    std::array<float, 8u * 16u> bones{};    // four vec4 rows per bone
};
static_assert(sizeof(RawBlock) == 544u, "RawBlock must match the std140 layout in ge.vert");
// A draw MHP3RD_CHECK_GPU_DECODE compares.
struct CheckDraw {
    std::uint32_t slot{};   // in the check buffer, in vec4s
    std::uint32_t first{};  // into FrameSlot::check_expected
    std::uint32_t count{};
    std::uint32_t vertex_type{};
};
constexpr std::size_t kRawHeaderBytes = 32u;
constexpr std::size_t kRawBoneBytes = 64u;

// A GE colour register (0x00BBGGRR) as 0..1 floats, with an explicit alpha.
std::array<float, 4> unpack_color(std::uint32_t color, float alpha = 1.0f) {
    return {static_cast<float>(color & 0xFFu) / 255.0f, static_cast<float>((color >> 8u) & 0xFFu) / 255.0f,
            static_cast<float>((color >> 16u) & 0xFFu) / 255.0f, alpha};
}

// Pipeline variants the GE state can produce.
struct PipelineKey {
    bool blend{};
    std::uint32_t source_factor{};
    std::uint32_t destination_factor{};
    std::uint32_t equation{};
    bool depth_test{};
    bool depth_write{};
    std::uint32_t depth_function{};
    bool cull{};
    bool cull_clockwise{};
    std::uint32_t color_mask{VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                             VK_COLOR_COMPONENT_A_BIT};
    // The fragment shader's alpha test (kAlphaTest in ge.frag). Draws without
    // one get a pipeline whose shader cannot discard.
    bool alpha_test{true};
    // The raw vertex shader, which decodes the guest's vertex bytes, and its
    // check build (MHP3RD_CHECK_GPU_DECODE).
    bool raw{};

    auto operator<=>(const PipelineKey &) const = default;
};

// GU_FIX takes its factor from a colour register rather than from the source or
// destination pixel. Vulkan offers a single blend constant per attachment, so a
// fixed factor collapses to ONE or ZERO when the colour is white or black, and
// only the remaining cases need the constant itself.
constexpr std::uint32_t kFactorFixed = 10u;
constexpr std::uint32_t kFactorOne = 16u;
constexpr std::uint32_t kFactorZero = 17u;
constexpr std::uint32_t kFactorInverseConstant = 18u;

std::uint32_t resolve_fixed_factor(std::uint32_t factor, std::uint32_t color) {
    if (factor != kFactorFixed) return factor;
    const std::uint32_t rgb = color & 0x00FFFFFFu;
    if (rgb == 0x00FFFFFFu) return kFactorOne;
    if (rgb == 0u) return kFactorZero;
    return kFactorFixed;
}

VkBlendFactor to_blend_factor(std::uint32_t factor, bool source) {
    switch (factor) {
    // Factor 0 names the *other* pixel: the source side scales by the
    // destination colour and the destination side by the source colour.
    case 0u: return source ? VK_BLEND_FACTOR_DST_COLOR : VK_BLEND_FACTOR_SRC_COLOR;
    case 1u: return source ? VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR : VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case 2u: return VK_BLEND_FACTOR_SRC_ALPHA;
    case 3u: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case 4u: return VK_BLEND_FACTOR_DST_ALPHA;
    case 5u: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case 6u: return VK_BLEND_FACTOR_SRC_ALPHA;             // doubled variants
    case 7u: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case 8u: return VK_BLEND_FACTOR_DST_ALPHA;
    case 9u: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case kFactorFixed: return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case kFactorOne: return VK_BLEND_FACTOR_ONE;
    case kFactorZero: return VK_BLEND_FACTOR_ZERO;
    case kFactorInverseConstant: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    default: return source ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_ZERO;
    }
}

VkBlendOp to_blend_op(std::uint32_t equation) {
    switch (equation) {
    // GU_SUBTRACT is source minus destination; GU_REVERSE_SUBTRACT is the other
    // way round, and it is what darkening effects such as blob shadows use.
    case 1u: return VK_BLEND_OP_SUBTRACT;
    case 2u: return VK_BLEND_OP_REVERSE_SUBTRACT;
    case 3u: return VK_BLEND_OP_MIN;
    case 4u: return VK_BLEND_OP_MAX;
    default: return VK_BLEND_OP_ADD;
    }
}

VkCompareOp to_compare_op(std::uint32_t function) {
    switch (function) {
    case 0u: return VK_COMPARE_OP_NEVER;
    case 1u: return VK_COMPARE_OP_ALWAYS;
    case 2u: return VK_COMPARE_OP_EQUAL;
    case 3u: return VK_COMPARE_OP_NOT_EQUAL;
    case 4u: return VK_COMPARE_OP_LESS;
    case 5u: return VK_COMPARE_OP_LESS_OR_EQUAL;
    case 6u: return VK_COMPARE_OP_GREATER;
    default: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    }
}

std::array<float, 16> multiply(const std::array<float, 16> &a, const std::array<float, 16> &b) {
    std::array<float, 16> result{};
    for (std::uint32_t column = 0; column < 4u; ++column) {
        for (std::uint32_t row = 0; row < 4u; ++row) {
            float sum = 0.0f;
            for (std::uint32_t k = 0; k < 4u; ++k) sum += a[k * 4u + row] * b[column * 4u + k];
            result[column * 4u + row] = sum;
        }
    }
    return result;
}

bool check(VkResult result, const char *what, std::string &error) {
    if (result == VK_SUCCESS) return true;
    error = std::string(what) + " failed with VkResult " + std::to_string(static_cast<int>(result));
    return false;
}

const char *present_mode_name(VkPresentModeKHR mode) {
    switch (mode) {
    case VK_PRESENT_MODE_IMMEDIATE_KHR: return "IMMEDIATE";
    case VK_PRESENT_MODE_MAILBOX_KHR: return "MAILBOX";
    case VK_PRESENT_MODE_FIFO_KHR: return "FIFO";
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "FIFO_RELAXED";
    default: return "OTHER";
    }
}

// Writes 4-byte pixels, top row first, as a 24-bit bottom-up BMP: no encoder
// needed and every viewer reads it.
bool write_bmp(const std::string &path, const std::uint8_t *pixels, std::uint32_t width, std::uint32_t height,
               bool bgra) {
    const std::uint32_t row_bytes = (width * 3u + 3u) & ~3u;
    const std::uint32_t image_bytes = row_bytes * height;
    std::vector<std::uint8_t> file(54u + image_bytes, 0u);
    const auto put32 = [&](std::size_t at, std::uint32_t value) {
        file[at] = static_cast<std::uint8_t>(value);
        file[at + 1u] = static_cast<std::uint8_t>(value >> 8u);
        file[at + 2u] = static_cast<std::uint8_t>(value >> 16u);
        file[at + 3u] = static_cast<std::uint8_t>(value >> 24u);
    };
    file[0] = 'B';
    file[1] = 'M';
    put32(2u, 54u + image_bytes);
    put32(10u, 54u);
    put32(14u, 40u);
    put32(18u, width);
    put32(22u, height);
    file[26] = 1u;
    file[28] = 24u;
    put32(34u, image_bytes);
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint8_t *source = pixels + static_cast<std::size_t>(y) * width * 4u;
        std::uint8_t *destination = file.data() + 54u + static_cast<std::size_t>(height - 1u - y) * row_bytes;
        for (std::uint32_t x = 0; x < width; ++x) {
            destination[x * 3u + 0u] = source[x * 4u + (bgra ? 0u : 2u)];
            destination[x * 3u + 1u] = source[x * 4u + 1u];
            destination[x * 3u + 2u] = source[x * 4u + (bgra ? 2u : 0u)];
        }
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char *>(file.data()), static_cast<std::streamsize>(file.size()));
    return static_cast<bool>(out);
}

struct PadTuning {
    float dead_zone{0.15f};
    float trigger{0.25f};
    settings::TriggerProfile triggers{settings::TriggerProfile::Standard};
    float right_stick{0.5f};
    settings::RightStick right_stick_mode{settings::RightStick::Camera};
    bool invert_x{};
    bool invert_y{};
    bool confirm_south{};
    bool trace{false};
};

// Read on every poll, so the in-game menu's changes apply at once.
PadTuning pad_tuning() {
    static const bool trace = std::getenv("MHP3RD_TRACE_PAD") != nullptr;
    const settings::Settings &player = settings::current();
    PadTuning value{};
    value.dead_zone = player.dead_zone;
    value.trigger = player.trigger;
    value.triggers = player.trigger_profile;
    value.right_stick = player.right_stick_zone;
    // The right stick is a real nub on this release, so driving the D-pad
    // from it as well would turn the camera twice.
    value.right_stick_mode = player.right_stick;
    value.invert_x = player.invert_camera_x;
    value.invert_y = player.invert_camera_y;
    // A PlayStation pad already carries the PSP's own face buttons, so the
    // positional mapping puts confirm on circle where the prompts want it.
    value.confirm_south = player.confirm_south;
    value.trace = trace;
    return value;
}

// Adds one gamepad's state to the pad bits and to the analog offsets the
// keyboard path also writes, so the two sources simply OR together.
void read_gamepad(SDL_Gamepad *device, PadState &pad, int &analog_x, int &analog_y) {
    const PadTuning tuning = pad_tuning();
    std::uint32_t &buttons = pad.buttons;
    const auto held = [&](SDL_GamepadButton button, std::uint32_t bit) {
        if (SDL_GetGamepadButton(device, button)) buttons |= bit;
    };
    held(SDL_GAMEPAD_BUTTON_DPAD_UP, 0x0010u);
    held(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0x0020u);
    held(SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0x0040u);
    held(SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0x0080u);
    held(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 0x0100u);
    held(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0x0200u);
    held(SDL_GAMEPAD_BUTTON_START, 0x0008u);
    held(SDL_GAMEPAD_BUTTON_BACK, 0x0001u);
    held(SDL_GAMEPAD_BUTTON_NORTH, 0x1000u);
    held(SDL_GAMEPAD_BUTTON_WEST, 0x8000u);
    // The game prompts "circle Enter / cross Back", and on a PlayStation pad
    // those are the same two buttons in the same two places.
    held(SDL_GAMEPAD_BUTTON_SOUTH, tuning.confirm_south ? 0x2000u : 0x4000u);
    held(SDL_GAMEPAD_BUTTON_EAST, tuning.confirm_south ? 0x4000u : 0x2000u);

    // The PSP triggers are digital, but hunters hold L for the camera all the
    // time, so the analog triggers press the same bits as the shoulders.
    const auto axis = [&](SDL_GamepadAxis id) {
        return std::clamp(static_cast<float>(SDL_GetGamepadAxis(device, id)) / 32767.0f, -1.0f, 1.0f);
    };
    // The trigger profiles copy other buttons for shooting: R on L2, where it
    // is held to aim, and the weapon's attack on R2 -- triangle for a bow,
    // circle for a bowgun. The buttons copied keep working.
    std::uint32_t left_trigger = 0x0100u;   // L
    std::uint32_t right_trigger = 0x0200u;  // R
    if (tuning.triggers == settings::TriggerProfile::Bows) {
        left_trigger = 0x0200u;
        right_trigger = 0x1000u;  // triangle
    } else if (tuning.triggers == settings::TriggerProfile::Bowguns) {
        left_trigger = 0x0200u;
        right_trigger = 0x2000u;  // circle
    }
    if (axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > tuning.trigger) buttons |= left_trigger;
    if (axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > tuning.trigger) buttons |= right_trigger;

    const float right_x = axis(SDL_GAMEPAD_AXIS_RIGHTX);
    const float right_y = axis(SDL_GAMEPAD_AXIS_RIGHTY);
    // The HD release has its own right-stick camera, so the stick normally goes
    // there. Pressing the D-pad bits as well would turn the camera twice, hence
    // the either/or: the claw emulation is only for builds where that path is
    // not wanted.
    if (tuning.right_stick_mode == settings::RightStick::DPad) {
        if (right_x < -tuning.right_stick) buttons |= 0x0080u;
        if (right_x > tuning.right_stick) buttons |= 0x0020u;
        if (right_y < -tuning.right_stick) buttons |= 0x0010u;
        if (right_y > tuning.right_stick) buttons |= 0x0040u;
    }

    // Rescale the live range, otherwise leaving the dead zone snaps the stick
    // straight to a sixth of its travel.
    const auto deflect = [&](float x, float y, std::uint8_t &out_x, std::uint8_t &out_y) {
        const float length = std::sqrt(x * x + y * y);
        if (length <= tuning.dead_zone) return;
        const float scale = std::min((length - tuning.dead_zone) / (1.0f - tuning.dead_zone), 1.0f) / length;
        out_x = static_cast<std::uint8_t>(std::clamp(0x80 + static_cast<int>(x * scale * 127.0f), 0, 255));
        out_y = static_cast<std::uint8_t>(std::clamp(0x80 + static_cast<int>(y * scale * 127.0f), 0, 255));
    };
    if (tuning.right_stick_mode == settings::RightStick::Camera)
        deflect(tuning.invert_x ? -right_x : right_x, tuning.invert_y ? -right_y : right_y, pad.right_x, pad.right_y);

    const float left_x = axis(SDL_GAMEPAD_AXIS_LEFTX);
    const float left_y = axis(SDL_GAMEPAD_AXIS_LEFTY);
    std::uint8_t nub_x = 0x80u;
    std::uint8_t nub_y = 0x80u;
    deflect(left_x, left_y, nub_x, nub_y);
    analog_x += static_cast<int>(nub_x) - 0x80;
    analog_y += static_cast<int>(nub_y) - 0x80;
}

// Background texture decoding (MHP3RD_SYNC_TEXTURE_DECODE turns it off): a
// texture first drawn while a frame is recorded is copied out of guest
// memory, decoded on these threads while the frame goes on, and uploaded
// ahead of the frame's commands when the frame is submitted.
struct DecodeJob {
    TextureSnapshot snapshot;
    std::vector<std::uint32_t> pixels;
    bool ok{};
    std::atomic<bool> done{};
};

class DecodePool {
public:
    explicit DecodePool(std::uint32_t threads) {
        for (std::uint32_t i = 0; i < threads; ++i) workers_.emplace_back([this] { work(); });
    }
    ~DecodePool() {
        {
            std::lock_guard<std::mutex> guard(lock_);
            stopping_ = true;
        }
        wake_.notify_all();
        for (std::thread &worker : workers_) worker.join();
    }
    DecodePool(const DecodePool &) = delete;
    DecodePool &operator=(const DecodePool &) = delete;

    void submit(std::shared_ptr<DecodeJob> job) {
        {
            std::lock_guard<std::mutex> guard(lock_);
            queue_.push_back(std::move(job));
        }
        wake_.notify_one();
    }
    // Waits for `job`, decoding jobs still queued on this thread meanwhile.
    void wait(DecodeJob &job) {
        std::unique_lock<std::mutex> guard(lock_);
        while (!job.done.load(std::memory_order_acquire)) {
            if (!queue_.empty()) {
                std::shared_ptr<DecodeJob> next = std::move(queue_.front());
                queue_.pop_front();
                guard.unlock();
                run(*next);
                guard.lock();
                continue;
            }
            finished_.wait(guard);
        }
    }

private:
    void run(DecodeJob &job) {
        job.ok = decode_snapshot(job.snapshot, job.pixels);
        {
            std::lock_guard<std::mutex> guard(lock_);
            job.done.store(true, std::memory_order_release);
        }
        finished_.notify_all();
    }
    void work() {
        std::unique_lock<std::mutex> guard(lock_);
        for (;;) {
            wake_.wait(guard, [this] { return stopping_ || !queue_.empty(); });
            if (stopping_) return;
            std::shared_ptr<DecodeJob> job = std::move(queue_.front());
            queue_.pop_front();
            guard.unlock();
            run(*job);
            guard.lock();
        }
    }

    std::mutex lock_;
    std::condition_variable wake_;
    std::condition_variable finished_;
    std::deque<std::shared_ptr<DecodeJob>> queue_;
    std::vector<std::thread> workers_;
    bool stopping_{};
};

} // namespace

struct VulkanRenderer::Impl {
    struct Texture {
        VkImage image{};
        VkDeviceMemory memory{};
        VkImageView view{};
        VkDescriptorSet descriptor{};
        std::uint64_t last_used{};
        // The texture pack's image for this texture, looked up once when the
        // texture was uploaded; drawn instead once it is on the GPU.
        std::shared_ptr<Replacement> replacement;
        // How far down a 512-tall texture has been drawn, which decides how
        // much of it the pack's hash covers; see texture_pack.hpp.
        std::uint16_t max_seen_v{};
    };

    RendererConfig config;
    SDL_Window *window{};
    VkInstance instance{};
    VkSurfaceKHR surface{};
    VkPhysicalDevice physical_device{};
    VkDevice device{};
    std::uint32_t queue_family{};
    VkQueue queue{};
    VkSwapchainKHR swapchain{};
    VkFormat swapchain_format{VK_FORMAT_B8G8R8A8_UNORM};
    VkExtent2D swapchain_extent{};
    // The depth buffer's format: 32-bit float where the device can render to
    // it, which desktop GPUs all can; phones may offer only 24- or 16-bit.
    VkFormat depth_format{VK_FORMAT_D32_SFLOAT};
    [[nodiscard]] VkImageAspectFlags depth_aspect() const {
        return depth_format == VK_FORMAT_D24_UNORM_S8_UINT || depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT
                   ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
                   : VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    void choose_depth_format() {
        const VkFormat candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT,
                                       VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_D16_UNORM,
                                       VK_FORMAT_D32_SFLOAT_S8_UINT};
        for (const VkFormat format : candidates) {
            VkFormatProperties properties{};
            vkGetPhysicalDeviceFormatProperties(physical_device, format, &properties);
            if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0u) {
                depth_format = format;
                if (format != VK_FORMAT_D32_SFLOAT)
                    std::cout << "[render] no 32-bit float depth buffer; using format " << static_cast<int>(format)
                              << "\n";
                return;
            }
        }
    }
    // The part of the window the game and the overlay may use, in pixels:
    // the whole window, except on Android, where a display cutout or a
    // system bar can take an edge (SDL's safe area).
    VkRect2D content_rect{};
    void update_content_rect() {
        content_rect = {{0, 0}, swapchain_extent};
#if defined(MHP3RD_ANDROID_APP)
        // Only the display cutout: SDL's safe area also counts the gesture
        // areas of the hidden system bars, which would shrink the picture for
        // nothing.
        int window_width = 0;
        int window_height = 0;
        if (window == nullptr || !SDL_GetWindowSizeInPixels(window, &window_width, &window_height) ||
            window_width <= 0 || window_height <= 0)
            return;
        const android::Insets cutout = android::cutout_insets();
        const double x_scale = static_cast<double>(swapchain_extent.width) / window_width;
        const double y_scale = static_cast<double>(swapchain_extent.height) / window_height;
        const auto left = static_cast<std::int32_t>(std::lround(cutout.left * x_scale));
        const auto top = static_cast<std::int32_t>(std::lround(cutout.top * y_scale));
        const auto right = static_cast<std::int32_t>(swapchain_extent.width) -
                           static_cast<std::int32_t>(std::lround(cutout.right * x_scale));
        const auto bottom = static_cast<std::int32_t>(swapchain_extent.height) -
                            static_cast<std::int32_t>(std::lround(cutout.bottom * y_scale));
        if (right - left < 16 || bottom - top < 16) return;
        content_rect = {{left, top},
                        {static_cast<std::uint32_t>(right - left), static_cast<std::uint32_t>(bottom - top)}};
#endif
    }
    std::vector<VkImage> swapchain_images;
    VkCommandPool command_pool{};
    VkCommandBuffer command_buffer{};
    VkFence frame_fence{};
    // GPU time per frame (perf line): timestamps written at the start of each
    // command buffer of the frame and after its last draw, before the copy to
    // the window. Read after the frame fence; null when the queue cannot time
    // (timestampValidBits 0) or MHP3RD_NO_GPU_TIMESTAMPS is set.
    VkQueryPool gpu_timer{};
    double gpu_timer_ns_per_tick{};
    std::uint64_t gpu_timer_mask{};
    std::uint32_t gpu_timer_used{};     // queries written into this frame so far
    bool gpu_timer_open{};
    // Each frame slot (see FrameSlot) has its own range of the pool.
    [[nodiscard]] std::uint32_t gpu_timer_base() const { return slot * 2u * kGpuTimerSegments; }
    void begin_gpu_segment(VkCommandBuffer commands) {
        if (gpu_timer == VK_NULL_HANDLE || gpu_timer_open || gpu_timer_used + 2u > 2u * kGpuTimerSegments) return;
        vkCmdWriteTimestamp(commands, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, gpu_timer, gpu_timer_base() + gpu_timer_used);
        gpu_timer_open = true;
    }
    void end_gpu_segment(VkCommandBuffer commands) {
        if (!gpu_timer_open) return;
        vkCmdWriteTimestamp(commands, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, gpu_timer,
                            gpu_timer_base() + gpu_timer_used + 1u);
        gpu_timer_used += 2u;
        gpu_timer_open = false;
    }
    // After the fence of the frame that wrote them: adds that frame's GPU time.
    void collect_gpu_time(std::uint32_t from);
    // Signalled by the presentation engine for each swapchain image a frame
    // or a present between flips acquires, one per command buffer that
    // presents (see acquire_semaphore()); and signalled by a submission for
    // the present of the image it drew, one per swapchain image, as a
    // present may still be waiting for its own when the next one is made.
    std::array<VkSemaphore, 4> image_available{};
    std::vector<VkSemaphore> render_finished;

    // MHP3RD_GPU_BREADCRUMBS (VK_AMD_buffer_marker): each marked point of
    // every command buffer writes its number twice into a host-visible
    // buffer, once when the GPU reaches it and once when everything before it
    // has finished. After a GPU hang (VK_ERROR_DEVICE_LOST) the log names the
    // last point finished and the last one reached: the work that hung lies
    // between them.
    struct Breadcrumbs {
        bool available{};
        VkBuffer buffer{};
        VkDeviceMemory memory{};
        volatile std::uint32_t *mapped{};
        PFN_vkCmdWriteBufferMarkerAMD write{};
        std::uint32_t next{};
        std::array<std::string, 1024> labels;  // by marker number, modulo the size
    } breadcrumbs;
    void breadcrumb(VkCommandBuffer commands, const std::string &label) {
        Breadcrumbs &b = breadcrumbs;
        if (b.write == nullptr) return;
        const std::uint32_t id = ++b.next;
        b.labels[id % b.labels.size()] = label + " (frame " + std::to_string(frames) + ")";
        b.write(commands, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, b.buffer, 0u, id);
        b.write(commands, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, b.buffer, 4u, id);
    }
    // Reports a lost device, with the breadcrumbs when they are on, and ends
    // the process: nothing more can be drawn.
    [[noreturn]] void device_lost(const char *where) {
        std::cout << "[render] the GPU stopped responding (VK_ERROR_DEVICE_LOST) in " << where << "\n";
        Breadcrumbs &b = breadcrumbs;
        if (b.mapped != nullptr) {
            const std::uint32_t reached = b.mapped[0];
            const std::uint32_t finished = b.mapped[1];
            std::cout << "[render] breadcrumbs: last finished " << finished << " "
                      << b.labels[finished % b.labels.size()] << "; last reached " << reached << " "
                      << b.labels[reached % b.labels.size()] << "; last recorded " << b.next << "\n";
            const std::uint32_t first = finished > 8u ? finished - 8u : 1u;
            for (std::uint32_t id = first; id <= std::min(b.next, finished + 24u); ++id)
                std::cout << "[render]   " << id << (id <= finished ? " done " : id <= reached ? " busy " : " queued ")
                          << b.labels[id % b.labels.size()] << "\n";
        }
        std::cout << std::flush;
        std::_Exit(3);
    }
    void wait_fence(VkFence fence, const char *where) {
        const VkResult result = vkWaitForFences(device, 1u, &fence, VK_TRUE, UINT64_MAX);
        if (result == VK_ERROR_DEVICE_LOST) device_lost(where);
    }
    VkPresentModeKHR present_mode{VK_PRESENT_MODE_FIFO_KHR};
    VkSurfaceFormatKHR surface_format{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    VkImageUsageFlags swapchain_usage{};
    std::vector<VkPresentModeKHR> present_modes;
    std::vector<VkImageView> swapchain_views;
    std::uint32_t swapchain_min_images{2u};
    // Set by a resize, a present mode change or an out-of-date swapchain;
    // the swapchain is rebuilt before the next acquire.
    bool swapchain_dirty{};
    // Set when the swapchain may no longer suit the surface (a suboptimal
    // present, a turned display); see check_swapchain().
    bool swapchain_check{};

    // Display settings.
    settings::PresentMode requested_present{settings::PresentMode::Fifo};
    settings::Aspect aspect{settings::Aspect::Original};
    std::uint32_t requested_scale{2u};  // multiples of 480x272; 0: the window's size
    // A target size the window asks for, applied once it has held for
    // kSettleFrames frames; a changed setting applies at the next chance.
    VkExtent2D pending_extent{};
    std::uint32_t pending_frames{};
    bool resize_now{};
    // The framebuffers the game showed last: its interface is drawn into them.
    std::array<std::uint32_t, 2> display_addresses{};
    bool sharp_screen{};
    bool sharp_textures{};
    std::string device_name;

    // Dear ImGui draws in its own render pass over the finished swapchain
    // image, after the game frame and the performance overlay.
    VkRenderPass ui_render_pass{};
    std::vector<VkFramebuffer> ui_framebuffers;
#if defined(__ANDROID__)
    // Pre-rotation for a display turned sideways. The swapchain's images are
    // in the panel's orientation (swapchain_image_extent) and carry the
    // surface's transform; everything is drawn upright at swapchain_extent
    // into one of these per swapchain image, and a last pass turns it into
    // the swapchain image. The compositor then has nothing left to rotate.
    struct UprightImage {
        VkImage image{};
        VkDeviceMemory memory{};
        VkImageView view{};
        VkDescriptorSet set{};
        VkFramebuffer rotate_framebuffer{};
    };
    std::vector<UprightImage> upright_images;
    VkExtent2D swapchain_image_extent{};
    std::int32_t swapchain_quarter_turns{};
    // The surface transform the swapchain was made for.
    VkSurfaceTransformFlagBitsKHR swapchain_transform{VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR};
    std::uint32_t swapchain_builds{};
    VkRenderPass rotate_render_pass{};
    VkDescriptorSetLayout rotate_set_layout{};
    VkDescriptorPool rotate_pool{};
    VkPipelineLayout rotate_layout{};
    VkPipeline rotate_pipeline{};
    VkSampler rotate_sampler{};
    VkShaderModule rotate_vertex{};
    VkShaderModule rotate_fragment{};
    [[nodiscard]] bool prerotated() const { return !upright_images.empty(); }
    bool create_rotation_pipeline(std::string &error);
    bool create_upright_images(std::string &error);
    void destroy_upright_images();
    void destroy_rotation_pipeline();
    void record_rotation(VkCommandBuffer commands, std::uint32_t image_index, VkImageLayout layout);
#endif
    bool ui_ready{};
    ImDrawData *ui_draw_data{};
    std::function<bool(const SDL_Event &)> event_hook;
    bool game_input{true};
    bool suppress_held{};
    std::uint32_t suppressed_buttons{};
    // Keyboard and mouse (input/bindings.hpp). Mouse buttons are followed
    // through their events, so a scripted click counts like a real one.
    bool pointer_free{};
    bool scripted_input{};
    bool mouse_captured{};
    std::uint32_t mouse_buttons{};  // bit n: SDL mouse button n held
    MouseMotion mouse_motion{};
    // Touch screen: the on-screen controls, in window coordinates.
    input::touch::Controls touch;
    bool touch_visible{};
    bool real_mouse_seen{};
    MouseMotion touch_motion{};
    struct TouchLayoutKey {
        int width{};
        int height{};
        VkRect2D content{};
        float size{};
        bool dpad{};
    } touch_layout_key{};
    void update_touch_layout() {
        int width = 0;
        int height = 0;
        if (window == nullptr || !SDL_GetWindowSize(window, &width, &height) || width <= 0 || height <= 0) return;
        const float size = settings::current().touch_size;
        const bool dpad = settings::current().touch_dpad;
        const TouchLayoutKey key{width, height, content_rect, size, dpad};
        if (key.width == touch_layout_key.width && key.height == touch_layout_key.height &&
            key.size == touch_layout_key.size && key.dpad == touch_layout_key.dpad &&
            key.content.offset.x == touch_layout_key.content.offset.x &&
            key.content.offset.y == touch_layout_key.content.offset.y &&
            key.content.extent.width == touch_layout_key.content.extent.width &&
            key.content.extent.height == touch_layout_key.content.extent.height)
            return;
        touch_layout_key = key;
        // The content area (clear of a cutout) in window coordinates, plus a
        // small margin from the rounded corners.
        input::touch::Insets insets;
        if (swapchain_extent.width != 0u && swapchain_extent.height != 0u) {
            const float x_scale = static_cast<float>(width) / static_cast<float>(swapchain_extent.width);
            const float y_scale = static_cast<float>(height) / static_cast<float>(swapchain_extent.height);
            insets.left = static_cast<float>(content_rect.offset.x) * x_scale;
            insets.top = static_cast<float>(content_rect.offset.y) * y_scale;
            insets.right = static_cast<float>(swapchain_extent.width - content_rect.offset.x -
                                              content_rect.extent.width) * x_scale;
            insets.bottom = static_cast<float>(swapchain_extent.height - content_rect.offset.y -
                                               content_rect.extent.height) * y_scale;
        }
        const float margin = static_cast<float>(std::min(width, height)) * 0.02f;
        insets.left += margin;
        insets.top += margin;
        insets.right += margin;
        insets.bottom += margin;
        touch.set_layout(
            input::touch::make_layout(static_cast<float>(width), static_cast<float>(height), insets, size, dpad));
    }
    void handle_touch(const SDL_Event &event) {
        const bool finger = event.type == SDL_EVENT_FINGER_DOWN || event.type == SDL_EVENT_FINGER_MOTION ||
                            event.type == SDL_EVENT_FINGER_UP || event.type == SDL_EVENT_FINGER_CANCELED;
        if (!finger) {
            // A gamepad, the keyboard or a real mouse takes over: hide.
            const bool other = event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_KEY_DOWN ||
                               (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.which != SDL_TOUCH_MOUSEID) ||
                               (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION &&
                                std::abs(static_cast<int>(event.gaxis.value)) > 16000);
            if (other && touch_visible) {
                touch_visible = false;
                touch.release_all();
            }
            return;
        }
        if (!settings::current().touch_controls || !game_input) {
            touch.release_all();
            return;
        }
        int width = 0;
        int height = 0;
        if (!SDL_GetWindowSize(window, &width, &height)) return;
        update_touch_layout();
        const input::touch::Point at{event.tfinger.x * static_cast<float>(width),
                                     event.tfinger.y * static_cast<float>(height)};
        const std::uint64_t id = event.tfinger.fingerID;
        if (event.type == SDL_EVENT_FINGER_DOWN) {
            touch_visible = true;
            touch.finger_down(id, at);
        } else if (event.type == SDL_EVENT_FINGER_MOTION) {
            touch.finger_move(id, at);
        } else {
            touch.finger_up(id);
        }
        const input::touch::Point drag = touch.take_camera_drag();
        touch_motion.x += drag.x / static_cast<float>(height);
        touch_motion.y += drag.y / static_cast<float>(height);
    }
    std::array<bool, input::kKeyPositions> scripted_keys{};

    // Captures the pointer for the game when everything allows it and frees
    // it otherwise. Whatever the mouse did or held across a change is dropped,
    // so nothing stays pressed and the camera does not jump.
    // The PSP pad from the keyboard, the mouse's buttons and the gamepad as
    // they are now.
    void sample_pad(bool focused);
    void update_pointer(bool focused) {
        const bool minimized = (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0u;
        bool wanted = settings::current().mouse && game_input && !pointer_free && !minimized &&
                      (focused || scripted_input);
#if defined(__ANDROID__)
        // A phone has no mouse to capture until one is actually used.
        wanted = wanted && real_mouse_seen;
#endif
        if (wanted == mouse_captured) return;
        mouse_captured = wanted;
        // A scripted run never takes the real pointer from the person at the machine.
        if (!scripted_input) SDL_SetWindowRelativeMouseMode(window, wanted);
        if (pad_tuning().trace) std::cout << "[pad] pointer " << (wanted ? "captured" : "free") << std::endl;
        mouse_buttons = 0u;
        mouse_motion = {};
        if (wanted) suppress_held = true;
    }

    // Window capture: the presented image is copied here and written after
    // its frame completes.
    std::string capture_path;
    VkBuffer capture_buffer{};
    VkDeviceMemory capture_memory{};
    VkExtent2D capture_extent{};
    bool capture_recorded{};

    // Performance overlay: drawn on the CPU, copied through a mapped staging
    // buffer into a small image and scaled onto the swapchain image after the
    // game frame, so screenshots of the window include it.
    bool overlay_visible{};
    bool overlay_ready{};
    VkImage overlay_image{};
    VkDeviceMemory overlay_memory{};
    VkImageView overlay_view{};
    // One staging buffer per command buffer that can draw the overlay: the
    // frame slots' and the two presents between flips' (command_index()).
    struct Staging {
        VkBuffer buffer{};
        VkDeviceMemory memory{};
        void *mapped{};
    };
    std::array<Staging, 4> overlay_staging{};
    std::vector<std::uint32_t> overlay_pixels;

    // Frames the game writes to memory without the GE (upload_frame): copied
    // through a mapped staging buffer into an image the size of the frame,
    // then scaled into the target of the address the game shows.
    VkImage upload_image{};
    VkDeviceMemory upload_memory{};
    VkImageView upload_view{};
    VkExtent2D upload_extent{};
    void destroy_upload();
    bool create_upload(std::uint32_t width, std::uint32_t height, std::string &error);

    // One offscreen target per guest framebuffer address. The game draws into
    // several (double buffering, render to texture), and only the address passed
    // to sceDisplaySetFrameBuf is shown.
    struct Target {
        VkImage color{};
        VkDeviceMemory color_memory{};
        VkImageView color_view{};
        VkImage depth{};
        VkDeviceMemory depth_memory{};
        VkImageView depth_view{};
        VkFramebuffer framebuffer{};
        bool initialized{};
        // The guest's view of the buffer when it was last drawn to: row length
        // in pixels and pixel format (0:5650 1:5551 2:4444 3:8888).
        std::uint32_t stride{512u};
        std::uint32_t format{3u};
        std::uint64_t last_drawn_frame{};
        // Bumped by every draw into the target, so a copy made for sampling
        // knows when it is out of date.
        std::uint64_t draw_serial{};
        // One guest word from every 256 bytes of the buffer, read when it was
        // last drawn to. The renderer never writes guest memory, so a word that
        // has changed since means the game put something else there.
        std::vector<std::uint32_t> guest_words;
        // The copy that draws sample when the game textures from this buffer:
        // a render pass cannot read its own attachment.
        VkImage copy{};
        VkDeviceMemory copy_memory{};
        VkImageView copy_view{};
        VkImageView copy_opaque_view{};
        std::array<VkDescriptorSet, 2> copy_descriptors{};
        std::uint64_t copy_serial{};
        bool copy_valid{};
    };
    std::map<std::uint32_t, Target> targets;
    // Write-back of the displayed framebuffer to guest VRAM (write_back_frame):
    // present() scales the target to 480x272 and copies it into a mapped
    // buffer; begin_frame(), after the frame fence, takes the pixels; the next
    // write_back_frame() stores them in the guest's format.
    VkImage writeback_image{};
    VkDeviceMemory writeback_memory{};
    VkImageView writeback_view{};
    // The write-back buffer is read by the CPU every frame. Uncached memory
    // (radv's default host-visible type) makes that copy take ~3 ms on a
    // Steam Deck, so a HOST_CACHED type is preferred, and a non-coherent one
    // is invalidated before each read. MHP3RD_NO_CACHED_READBACK keeps the
    // first host-visible coherent type, as before.
    bool writeback_cached{};
    bool writeback_coherent{true};
    void invalidate_writeback(VkDeviceMemory memory) {
        if (writeback_coherent || memory == VK_NULL_HANDLE) return;
        VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
        range.memory = memory;
        range.offset = 0u;
        range.size = VK_WHOLE_SIZE;
        vkInvalidateMappedMemoryRanges(device, 1u, &range);
    }
    struct WritebackFrame {
        std::uint32_t address{};
        std::uint32_t stride{};
        std::uint32_t format{};
    };
    WritebackFrame writeback_ready{};     // pixels waiting in writeback_pixels
    bool writeback_has_pixels{};
    std::vector<std::uint32_t> writeback_pixels;

    // Frames in flight. With two (the default) the CPU records a frame while
    // the GPU still draws the one before, so their times overlap instead of
    // adding up; MHP3RD_FRAMES_IN_FLIGHT=1 waits for each frame before
    // recording the next, as before. What the CPU writes for a frame, or
    // reads back from it, belongs to its slot until the slot's fence says the
    // frame has finished.
    static constexpr std::uint32_t kMaxSlots = 2u;
    struct FrameSlot {
        VkCommandBuffer commands{};
        VkFence fence{};
        VkCommandBuffer uploads{};
        // Staging ring for texture uploads (stage_upload()).
        VkBuffer ring{};
        VkDeviceMemory ring_memory{};
        void *ring_mapped{};
        VkDeviceSize ring_size{};
        std::vector<std::pair<VkBuffer, VkDeviceMemory>> retired_buffers;
        std::vector<Texture> retired_textures;
        // The frame written back to guest memory (record_writeback()).
        Staging writeback{};
        bool writeback_in_flight{};
        WritebackFrame writeback_recorded{};
        // A frame the game wrote itself (upload_frame()).
        Staging movie{};
        std::uint32_t gpu_timer_pending{};  // queries the frame wrote, read after its fence
        std::uint32_t region{kNoRegion};    // of the vertex and index buffers
        // MHP3RD_CHECK_GPU_DECODE: the draws the frame checks and the host's
        // own decode of their vertices.
        std::vector<CheckDraw> checks;
        std::vector<GpuVertex> check_expected;
        std::vector<std::uint8_t> check_drawn;  // for each: the draw's indices name it, so the shader ran for it
        std::uint32_t check_used{};         // vertices of the slot's check area handed out
    };
    static constexpr std::uint32_t kNoRegion = 0xFFFFFFFFu;
    std::array<FrameSlot, kMaxSlots> slots{};
    std::uint32_t slot_count{kMaxSlots};
    std::uint32_t slot{};
    // Regions the presents between flips read, by present slot, so a frame
    // does not write a region one of them may still draw from.
    std::array<std::uint32_t, 2> present_regions{};
    std::uint32_t replay_regions{};  // set by replay()
    // 0 and 1: the frame slots' command buffers; 2 and 3: the presents'.
    [[nodiscard]] std::uint32_t command_index(VkCommandBuffer commands) const {
        for (std::uint32_t i = 0; i < kMaxSlots; ++i)
            if (commands == slots[i].commands) return i;
        return commands == present_commands[1] ? 3u : 2u;
    }
    [[nodiscard]] VkSemaphore acquire_semaphore(VkCommandBuffer commands) const {
        return image_available[command_index(commands)];
    }
    // Takes the pixels a slot's frame wrote back, waiting for it if `wait`.
    void collect_writeback(std::uint32_t from, bool wait);
    // After a slot's fence: compares what its raw vertex shader decoded with
    // the host's decode (MHP3RD_CHECK_GPU_DECODE).
    void compare_gpu_decode(std::uint32_t from);
    bool create_writeback(std::string &error);
    void destroy_writeback();
    void record_writeback(std::uint32_t address);
    // Stores 480x272 RGBA pixels into the guest framebuffer `frame` describes.
    void store_frame(GuestMemory &memory, const WritebackFrame &frame, const std::uint32_t *pixels);
    // Numbers draws into targets across all of them, for Target::draw_serial.
    std::uint64_t target_draw_counter{};
    std::uint32_t current_target{};
    std::uint32_t last_drawn_target{};
    std::uint32_t presented_target{};
    // A copy of the frame shown when hold_frame() began; only its colour
    // image is used.
    Target held{};
    bool holding{};
    // A load running fast: presents are thinned out (set_fast_forward).
    bool fast_forward{};
    std::chrono::steady_clock::time_point fast_forward_shown{};
    // Per-frame tally, so "no 3D" can be told from "3D drawn somewhere else".
    std::uint32_t frame_through_draws{};
    std::uint32_t frame_transformed_draws{};
    std::uint32_t frame_transformed_vertices{};
    std::uint32_t frame_onscreen_vertices{};
    std::uint32_t frame_behind_camera{};
    std::array<float, 3> frame_ndc_min{1e30f, 1e30f, 1e30f};
    std::array<float, 3> frame_ndc_max{-1e30f, -1e30f, -1e30f};
    std::map<std::uint32_t, std::uint32_t> frame_transformed_targets;
    // MHP3RD_TRACE_CAMERA: the view matrices this frame's transformed draws
    // used and how many vertices each of them covered. A frame holds a handful
    // (the scene, a reflection, a shadow pass), and the busiest one is the
    // camera the player sees, so the frame's line can report that one. Only
    // filled while the trace is on.
    std::vector<std::pair<std::array<float, 16>, std::uint32_t>> frame_views;
    // The yaw of the previous traced frame, so each line can carry the turn.
    float traced_yaw{};
    CameraReading reading{};
    bool pass_active{};
    VkRenderPass render_pass{};
    // The same pass with the colour (bit 0) or depth (bit 1) attachment not
    // loaded, for a pass whose first draw is a clear that writes all of it:
    // a tiled GPU then does not read the target from memory first.
    // MHP3RD_NO_CLEAR_LOAD always loads, as before.
    std::array<VkRenderPass, 4> discard_passes{};
    VkExtent2D target_extent{};

    VkShaderModule vertex_shader{};
    VkShaderModule raw_vertex_shader{};  // GPU vertex decode, or its check build
    // GPU vertex decode (MHP3RD_GPU_DECODE): the renderer asks GeState for
    // the guest's vertex bytes of transformed triangle draws and the vertex
    // shader decodes and skins them.
    bool gpu_decode_available{};
    bool check_gpu_decode{};  // MHP3RD_CHECK_GPU_DECODE
    std::uint32_t raw_offset{};  // the raw block most recently written this frame
    RawBlock last_raw{};
    std::size_t last_raw_bytes{};
    bool raw_valid{};
    // MHP3RD_CHECK_GPU_DECODE: the shader writes what it decoded to this
    // buffer (three vec4s a vertex, from each draw's slot), and after the
    // frame's fence the host compares it with its own decode, kept here.
    VkBuffer check_buffer{};
    VkDeviceMemory check_memory{};
    void *check_mapped{};
    static constexpr std::uint32_t kCheckVertices = 65536u;  // compared per frame slot
    struct CheckTally {
        std::uint64_t draws{};
        std::uint64_t vertices{};
        std::uint64_t exact{};
        std::uint64_t close{};
        std::uint64_t differed{};
        double max_error{};
        std::array<std::array<std::uint64_t, 12>, 2> fields{};  // inexact values by kind (unskinned, skinned) and field
    } check_tally;
    VkShaderModule fragment_shader{};
    VkPipelineLayout pipeline_layout{};
    VkDescriptorSetLayout descriptor_layout{};
    VkDescriptorPool descriptor_pool{};
    VkDescriptorSetLayout lighting_layout{};
    VkDescriptorSet lighting_descriptor{};  // binding 0: environment, binding 1: object
    VkDeviceSize uniform_alignment{256u};
    VkSampler sampler{};        // linear
    VkSampler sharp_sampler{};  // nearest, for the sharp texture setting
    // The same, clamped to the edge, for render targets sampled as textures:
    // the game's texture is usually larger than the 480x272 the target holds.
    VkSampler clamp_sampler{};
    VkSampler clamp_sharp_sampler{};
    std::map<PipelineKey, VkPipeline> pipelines;
    // Pipelines compiled in earlier runs, kept in the data directory
    // (pipeline_cache.bin) so that a new area does not compile its shaders
    // again: drivers on phones compile slowly and some keep no cache of their
    // own. MHP3RD_NO_PIPELINE_CACHE creates every pipeline from scratch.
    VkPipelineCache pipeline_cache{};
    std::filesystem::path pipeline_cache_path;
    bool pipeline_cache_dirty{};
    std::chrono::steady_clock::time_point pipeline_cache_changed{};
    // Pipelines created since the last report, and how long they took.
    std::uint32_t pipelines_reported{};
    std::uint32_t pipelines_new{};
    double pipeline_new_ms{};
    void load_pipeline_cache();
    void save_pipeline_cache();
    // Pipeline prewarming (MHP3RD_NO_PIPELINE_PREWARM turns it off): the keys
    // of every pipeline a run made are kept in pipeline_keys.bin, and the next
    // run makes them on a thread of its own from the start, so a new area or
    // effect finds its pipelines ready instead of compiling them in its
    // first frame.
    struct Prewarm {
        std::thread thread;
        std::mutex lock;
        std::map<PipelineKey, VkPipeline> made;  // not yet asked for
        std::atomic<bool> stop{};
    };
    std::unique_ptr<Prewarm> prewarm = std::make_unique<Prewarm>();
    std::uint32_t pipelines_prewarm_used{};
    bool pipeline_keys_dirty{};
    std::filesystem::path pipeline_keys_path;
    void prewarm_pipelines();
    void save_pipeline_keys();
    [[nodiscard]] VkPipeline create_pipeline(const PipelineKey &key) const;
    // Every few seconds after a pipeline was created: saves the cache and
    // says how many pipelines the game needed and what they cost.
    void report_pipelines(bool final);
    // The pipeline of the previous lookup: consecutive draws mostly share it.
    // MHP3RD_NO_LOOKUP_CACHE looks every draw up in the map, as before.
    PipelineKey last_pipeline_key{};
    VkPipeline last_pipeline{};

    VkBuffer vertex_buffer{};
    VkDeviceMemory vertex_memory{};
    void *vertex_mapped{};
    VkDeviceSize vertex_offset{};
    // The region of the vertex and index buffers this frame writes: always
    // the first without frame interpolation, the next of kFrameRegions in
    // turn with it (see kFrameRegions).
    std::uint32_t frame_region{};
    std::uint32_t next_region{};
    VkDeviceSize vertex_limit{kVertexBufferBytes};
    VkDeviceSize index_limit{kIndexBufferBytes};
    void enter_region(std::uint32_t index) {
        // How much of its region the frame being left used, for the perf line.
        perf::note_frame_space(vertex_offset - static_cast<VkDeviceSize>(frame_region) * kVertexBufferBytes,
                               index_offset - static_cast<VkDeviceSize>(frame_region) * kIndexBufferBytes);
        frame_region = index;
        vertex_offset = static_cast<VkDeviceSize>(index) * kVertexBufferBytes;
        vertex_limit = vertex_offset + kVertexBufferBytes;
        index_offset = static_cast<VkDeviceSize>(index) * kIndexBufferBytes;
        index_limit = index_offset + kIndexBufferBytes;
    }
    // The lighting blocks most recently written this frame, reused while the
    // state stays the same; begin_frame() drops them with the vertex buffer.
    std::uint64_t environment_version{};    // 0: none written this frame
    std::uint32_t environment_offset{};
    ObjectBlock last_object{};
    std::uint32_t object_offset{};
    bool object_valid{};
    // The material colours of the last lit draw, by GeState's material version.
    std::uint64_t material_version{};       // 0: none unpacked yet
    std::array<std::array<float, 4>, 4> material{};
    // What the command buffer has bound, so that unchanged state is not bound
    // again; begin_frame() and begin_pass() forget it.
    VkPipeline bound_pipeline{};
    std::array<std::uint32_t, 3> bound_lighting_offsets{};
    bool lighting_bound{};

    // Copies a uniform block into the vertex buffer at the next aligned offset.
    // Returns false, writing nothing, when the buffer is full.
    bool write_uniform(const void *data, std::size_t size, std::uint32_t &offset) {
        const VkDeviceSize at = (vertex_offset + uniform_alignment - 1u) / uniform_alignment * uniform_alignment;
        if (at + size > vertex_limit) {
            report_frame_space_full("vertex");
            return false;
        }
        std::memcpy(static_cast<std::uint8_t *>(vertex_mapped) + at, data, size);
        offset = static_cast<std::uint32_t>(at);
        vertex_offset = at + size;
        return true;
    }
    void forget_bindings() {
        bound_pipeline = VK_NULL_HANDLE;
        lighting_bound = false;
        state_known = false;
    }

    // Draw merging (MHP3RD_NO_DRAW_MERGE turns it off). A transformed draw
    // whose recorded state equals the previous one's, and whose vertices
    // follow the previous draw's in the vertex buffer, joins its group: its
    // indices, rebased onto the group's first vertex, follow the group's in
    // the index buffer, and the whole group is one vkCmdDrawIndexed. The
    // triangles, their order and their vertex data are unchanged. The state a
    // draw is recorded with is also compared with what the command buffer
    // already has, and only what differs is set.
    struct DrawState {
        VkPipeline pipeline{};
        VkDescriptorSet texture{};
        std::array<std::uint32_t, 3> lighting{};  // environment, object, raw block
        VkViewport viewport{};
        VkRect2D scissor{};
        std::array<float, 4> blend{};
        PushConstants push{};
    };
    DrawState recorded{};
    bool state_known{};
    struct DrawGroup {
        bool open{};
        VkDeviceSize vertex_base{};  // bytes into the vertex buffer
        VkDeviceSize vertex_end{};
        VkDeviceSize index_base{};   // bytes into the index buffer
        std::uint32_t index_count{};
        std::uint32_t draws{};
        bool raw{};                  // the guest's vertex bytes, decoded on the GPU
        std::uint32_t stride{};      // of a raw group's vertices
    };
    DrawGroup group{};
    bool group_skinned{};  // the open group is of skinned draws recorded for drawing again
    std::uint32_t draws_since_poll{};
    VkBuffer index_buffer{};
    VkDeviceMemory index_memory{};
    void *index_mapped{};
    VkDeviceSize index_offset{};
    void flush_group() {
        if (!group.open) return;
        const perf::SplitScope split(perf::Split::Record);
        group.open = false;
        vkCmdBindVertexBuffers(command_buffer, 0u, 1u, &vertex_buffer, &group.vertex_base);
        vkCmdBindIndexBuffer(command_buffer, index_buffer, group.index_base, VK_INDEX_TYPE_UINT16);
        // A raw group finds its bytes at its first instance (ge.vert).
        vkCmdDrawIndexed(command_buffer, group.index_count, 1u, 0u, 0,
                         group.raw ? static_cast<std::uint32_t>(group.vertex_base) : 0u);
        perf::count_recorded_draws(1u);
    }
    // Sets what differs between `state` and what is recorded already.
    void record_state(const DrawState &state) {
        const perf::SplitScope split(perf::Split::Record);
        if (!state_known || std::memcmp(&state.viewport, &recorded.viewport, sizeof(VkViewport)) != 0)
            vkCmdSetViewport(command_buffer, 0u, 1u, &state.viewport);
        if (!state_known || state.blend != recorded.blend)
            vkCmdSetBlendConstants(command_buffer, state.blend.data());
        if (!state_known || std::memcmp(&state.scissor, &recorded.scissor, sizeof(VkRect2D)) != 0)
            vkCmdSetScissor(command_buffer, 0u, 1u, &state.scissor);
        if (state.pipeline != bound_pipeline) {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline);
            bound_pipeline = state.pipeline;
        }
        if (!state_known || state.texture != recorded.texture)
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0u, 1u,
                                    &state.texture, 0u, nullptr);
        if (!lighting_bound || state.lighting != bound_lighting_offsets) {
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1u, 1u,
                                    &lighting_descriptor, static_cast<std::uint32_t>(state.lighting.size()),
                                    state.lighting.data());
            bound_lighting_offsets = state.lighting;
            lighting_bound = true;
        }
        if (!state_known || std::memcmp(&state.push, &recorded.push, sizeof(PushConstants)) != 0)
            vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0u, sizeof(PushConstants), &state.push);
        recorded = state;
        state_known = true;
    }
    [[nodiscard]] static bool same_state(const DrawState &a, const DrawState &b) {
        return a.pipeline == b.pipeline && a.texture == b.texture && a.lighting == b.lighting &&
               std::memcmp(&a.viewport, &b.viewport, sizeof(VkViewport)) == 0 &&
               std::memcmp(&a.scissor, &b.scissor, sizeof(VkRect2D)) == 0 && a.blend == b.blend &&
               std::memcmp(&a.push, &b.push, sizeof(PushConstants)) == 0;
    }

    // Frame interpolation (#39; frame_interpolation.hpp, frame_pacing.hpp).
    //
    // While it is on, each frame is recorded as it is drawn: a summary of
    // every draw, for matching it in the next frame, and the draw calls the
    // frame recorded, with their state and where their vertices, indices and
    // lighting blocks lie in the buffers. The buffers keep three frames (see
    // kFrameRegions), so drawing a frame again copies no vertices: an
    // in-between present records the older frame's draw calls again, the
    // merged groups as they were, with blended transforms in their push
    // constants. Only a skinned draw writes new vertices (its own blended
    // with the newer frame's, from copies kept on the CPU) and only a lit
    // draw whose world matrix moved writes a new object block.
    static constexpr std::uint32_t kNone = 0xFFFFFFFFu;
    struct ReplayGroup {
        DrawState state{};
        std::uint32_t target{};
        VkDeviceSize vertex_base{};   // bytes into the vertex buffer
        VkBuffer index_buffer{};      // null for a draw without indices
        VkDeviceSize index_base{};
        std::uint32_t count{};        // indices, or vertices without them
        std::uint32_t vertex_count{}; // vertices from vertex_base
        std::uint32_t first_draw{};   // into FrameRecord::summaries
        std::uint32_t draws{};
        std::uint32_t object{kNone};  // FrameRecord::objects, for a lit group
        std::uint32_t skinned{kNone}; // FrameRecord::skinned: the vertices of a group of skinned draws
        bool raw{};                   // drawn from the guest's vertex bytes at vertex_base (first instance)
        std::uint32_t raw_block{kNone}; // FrameRecord::raws: its format and bones
    };
    struct FrameRecord {
        std::vector<interpolation::DrawSummary> summaries;
        std::vector<std::uint32_t> group_of;  // for each summary, its group
        std::vector<ReplayGroup> groups;
        std::vector<GpuVertex> skinned;       // skinned draws' vertices
        std::vector<ObjectBlock> objects;
        std::uint32_t object_offset{kNone};   // where objects.back() lies
        std::vector<RawBlock> raws;           // raw blocks of raw groups, for blending their bones
        std::uint32_t raw_offset{kNone};      // where raws.back() lies
        std::uint32_t displayed{};
        std::int64_t moment_us{};
        std::uint64_t texture_clock{};        // texture_clock when the frame began
        bool recorded{};                      // recorded for drawing again, in full
        bool valid{};
        void clear() {
            summaries.clear();
            group_of.clear();
            groups.clear();
            skinned.clear();
            objects.clear();
            raws.clear();
            raw_offset = kNone;
            object_offset = kNone;
            recorded = false;
            valid = false;
        }
    };
    struct InterpolationStats {
        std::chrono::steady_clock::time_point window_start{std::chrono::steady_clock::now()};
        std::uint32_t frames{};
        std::uint64_t eligible{};
        std::uint64_t matched{};
        std::uint32_t continued{};
        float max_camera_angle{};
        float max_camera_distance{};
        std::map<std::string, std::uint32_t> cuts;
        std::uint32_t presents{};
        std::uint32_t blended{};
        std::uint32_t skipped{};
        std::uint32_t blocked{};     // not made: the display had no image free
        std::uint32_t over_budget{}; // not made: presents while the code ran took half a frame
        // Why presents showed a frame as it is rather than a blend.
        std::uint32_t plain_newest{};   // at or past the newest frame's moment (1 per frame at 60/90/120)
        std::uint32_t plain_oldest{};   // at the older frame's moment
        std::uint32_t plain_cut{};      // the pair is not blended (cut, not recorded)
        std::uint32_t plain_textures{}; // a texture the older frame drew with was dropped
        std::uint64_t groups{};  // draw calls recorded again
        std::uint64_t flipbook_steps{};  // texture offsets not blended: a flipbook's step
        std::uint64_t followed{};        // draw calls without a partner moved with the camera only
        std::uint32_t rejected{};        // pairs given up: moved too far on their own
        std::uint32_t rejected_shared{}; // of those, with other draws of the same mesh
        float max_own_motion{};
        std::chrono::steady_clock::duration blend_time{};  // CPU time of blended presents
        std::chrono::steady_clock::duration plain_time{};  // CPU time of the other presents
        std::chrono::steady_clock::duration max_late{};    // latest present after its time
        std::chrono::steady_clock::duration replay_time{}; // of blend_time: recording the draw calls again
        double gpu_ms{};  // GPU time of the blended presents' replays
        std::uint32_t gpu_samples{};
    };
    settings::FrameRate frame_rate{settings::FrameRate::Fps30};
    bool trace_interpolation{};
    bool interpolating{};          // the frame being drawn is recorded for drawing again
    bool cycle_active{};           // presents between flips are scheduled
    FrameRecord recording_frame;   // the frame the game is drawing
    FrameRecord newer_frame;       // the frame it flipped last
    FrameRecord older_frame;       // the frame before that
    interpolation::Matcher matcher;
    interpolation::CutThresholds cut_thresholds;
    interpolation::Matching matching;  // older_frame against newer_frame
    interpolation::RigidMotion camera_motion;  // matching.camera taken apart
    pacing::PresentClock present_clock;
    pacing::RateGovernor governor;
    InterpolationStats interpolation_stats;
    // Measured costs, kept across seconds for the governor.
    double blend_cost_ms{};
    double plain_cost_ms{};
    std::uint64_t governor_second{};  // perf::Summary::second last given to the governor
    // The pictures of the older and newer frames, copied at their flips, and
    // a target per present slot for the blended frames.
    std::array<Target, 2> pictures{};
    std::uint32_t newer_picture{};
    bool older_picture_valid{};
    bool newer_picture_valid{};
    std::array<Target, 2> blend_targets{};
    // The presents between flips alternate between two command buffers, each
    // with its fence, its scratch area of the vertex buffer and its pair of
    // GPU timestamps.
    std::array<VkCommandBuffer, 2> present_commands{};
    std::array<VkFence, 2> present_fences{};
    std::array<bool, 2> present_timed{};
    VkQueryPool present_timer{};
    std::uint32_t present_slot{};
    ImDrawData *frame_ui{};  // the interface drawn over this game frame
    // CPU time of the presents made while the game's code ran, since the
    // flip; they stop at half a game frame.
    std::chrono::steady_clock::duration busy_presents{};
    // A swapchain image acquired before recording a present between flips,
    // for submit_and_present to present.
    std::optional<std::uint32_t> acquired_image;
    float display_hz{};
    // The newest texture_clock value a destroyed texture had been drawn with:
    // a recorded frame that began before it may name its descriptor.
    std::uint64_t destroyed_texture_clock{};
    [[nodiscard]] bool interpolation_wanted() const;
    [[nodiscard]] double wanted_rate() const;
    void finish_interpolated_frame(VkImage source, std::uint32_t displayed, std::int64_t moment_us);
    void record_draw_for_replay(const DrawCall &call, bool lit, std::uint32_t skinned, const DrawState &state,
                                VkDeviceSize vertex_start, VkBuffer indices, VkDeviceSize index_start,
                                std::uint32_t count, std::uint32_t vertex_count, bool joined, bool raw = false);
    // `idle`: the kernel is waiting for real time, so the present costs the
    // game nothing; otherwise it is made while the game's code runs, within
    // a budget per game frame. Returns whether a present was made.
    bool poll_presents(std::chrono::steady_clock::time_point now, bool account, bool idle);
    bool present_between(const pacing::PresentClock::Present &present, bool account);
    void replay(VkCommandBuffer commands, std::uint32_t slot, float t);
    void report_interpolation();
    void reset_interpolation();
    void destroy_interpolation_targets();
    void check_replay();
    std::vector<std::uint32_t> read_image(VkImage image);
    bool create_target(Target &target, std::string &error);
    void initialize_layouts(VkCommandBuffer commands, Target &target);
    void submit_frame();

    Texture white_texture{};
    std::map<std::uint64_t, Texture> textures;
    // HD texture pack (texture_pack.hpp). `pack` is null while the setting
    // is off or no pack is installed; turning the setting on or off takes
    // effect at the next begin_frame().
    std::unique_ptr<TexturePack> pack;
    std::unique_ptr<TextureDumper> dumper;
    ReplacementTextures replacements;
    bool pack_wanted{};
    bool pack_applied{};
    bool pack_held{};    // an import is swapping the pack's folder: none is open
    bool pack_reload{};  // open the pack again, e.g. after an import
    TexturePackLocation pack_location;
    std::string pack_status;
    std::uint64_t replaced_draws{};  // this second, for MHP3RD_TRACE_TEXTURE_PACK
    void apply_texture_pack();
    [[nodiscard]] VkDescriptorSet texture_descriptor(const GuestMemory &memory, const DrawCall &call);
    std::uint64_t texture_clock{};
    // texture_key results for the display list being walked, by the state that
    // feeds the key; cleared by begin_display_list().
    struct TextureKeyInput {
        std::uint32_t address{};
        std::uint32_t buffer_width{};
        std::uint32_t size{};
        std::uint32_t format{};
        std::uint32_t clut_address{};
        std::uint32_t clut_format{};
        bool swizzled{};
        auto operator<=>(const TextureKeyInput &) const = default;
    };
    // The texture each input resolved to, while `textures_erased` has not
    // moved since: erasing is the only change that moves or frees a cached
    // texture. The previous draw's entry is kept at hand, as consecutive draws
    // mostly sample the same texture.
    struct ListTexture {
        std::uint64_t key{};
        Texture *texture{};
        std::uint64_t erased{};
    };
    std::map<TextureKeyInput, ListTexture> list_texture_keys;
    std::uint64_t textures_erased{};
    TextureKeyInput last_texture_input{};
    ListTexture *last_texture{};

    std::vector<GpuVertex> scratch;
    // Kept from one use to the next instead of allocated each time, unless
    // MHP3RD_NO_BUFFER_REUSE: decoded texture pixels, a framebuffer read back
    // for a block transfer, and the staging buffer and command buffer of
    // texture uploads (free again once an upload has waited for the queue).
    std::vector<std::uint32_t> decoded_pixels;
    std::vector<std::uint32_t> readback_pixels;
    VkBuffer upload_buffer{};
    VkDeviceMemory upload_buffer_memory{};
    void *upload_buffer_mapped{};
    VkDeviceSize upload_buffer_size{};
    VkCommandBuffer upload_commands{};
    // Texture uploads without waiting (MHP3RD_SYNC_UPLOADS waits, as
    // before). A texture first drawn while a frame is recorded is copied by a
    // command buffer of its own, submitted with the frame ahead of the
    // frame's commands, from a staging ring the frame fence frees again. A
    // texture evicted from the cache is destroyed once the frames that may
    // still draw it have finished, instead of after a queue idle wait.
    VkCommandBuffer frame_uploads{};  // the slot's, while its frame is recorded
    bool frame_uploads_open{};
    VkDeviceSize upload_ring_used{};
    std::uint32_t frame_upload_count{};
    [[nodiscard]] static bool async_uploads() {
        static const bool sync = std::getenv("MHP3RD_SYNC_UPLOADS") != nullptr;
        return !sync && !perf::alternate_off(perf::NewPath::Uploads);
    }
    // Stages `bytes` of pixels for a copy recorded into frame_uploads; null
    // when there is no room and no memory.
    [[nodiscard]] bool stage_upload(const void *pixels, VkDeviceSize bytes, VkBuffer &buffer, VkDeviceSize &offset);
    // The command buffers a submission of the frame's own commands runs:
    // the frame's uploads first, when it has any.
    std::uint32_t frame_batch(VkCommandBuffer commands, std::array<VkCommandBuffer, 2> &batch);
    // After a slot's fence: its ring is free again and what it retired can go.
    void release_frame_uploads(std::uint32_t from);
    [[nodiscard]] static bool reuse_buffers() {
        static const bool no_reuse = std::getenv("MHP3RD_NO_BUFFER_REUSE") != nullptr;
        return !no_reuse && !perf::alternate_off(perf::NewPath::Reuse);
    }
    void destroy_upload_buffer() {
        if (upload_buffer_mapped != nullptr) vkUnmapMemory(device, upload_buffer_memory);
        vkDestroyBuffer(device, upload_buffer, nullptr);
        vkFreeMemory(device, upload_buffer_memory, nullptr);
        upload_buffer = VK_NULL_HANDLE;
        upload_buffer_memory = VK_NULL_HANDLE;
        upload_buffer_mapped = nullptr;
        upload_buffer_size = 0u;
    }
    // Index list of a draw whose decoded vertices go straight into the vertex
    // buffer (see submit()).
    std::vector<std::uint16_t> direct_indices;
    // Background texture decoding: the pool (made on first use) and the
    // textures of the frame being recorded whose pixels are still decoding.
    std::unique_ptr<DecodePool> decode_pool;
    struct PendingTexture {
        std::shared_ptr<DecodeJob> job;
        VkImage image{};
        std::uint32_t width{};
        std::uint32_t height{};
    };
    std::vector<PendingTexture> pending_textures;
    [[nodiscard]] static bool background_decode() {
        static const bool sync = std::getenv("MHP3RD_SYNC_TEXTURE_DECODE") != nullptr;
        return !sync && !perf::alternate_off(perf::NewPath::TextureDecode);
    }
    // Waits for the frame's pending textures and records their uploads.
    void finish_pending_textures();
    // MHP3RD_CHECK_DIRECT_VERTICES: draws compared with the expansion, and
    // those that differed.
    std::uint64_t direct_checked{};
    std::uint64_t direct_mismatched{};
    PadState pad{};
    SDL_Gamepad *gamepad{};
    SDL_JoystickID gamepad_id{};
    bool recording{};
    bool quit{};
    bool ready{};
    std::uint64_t frames{};
    std::uint64_t draws{};

    // Only the first pad is used; a second one arriving is ignored rather than
    // stealing the stick from whoever is already playing.
    void open_gamepad(SDL_JoystickID id) {
        if (gamepad != nullptr) {
            // The virtual pad of MHP3RD_INPUT_SCRIPT takes over from a real
            // one, so a controller within reach does not steal a scripted run.
            const char *name = SDL_GetGamepadNameForID(id);
            if (name == nullptr || std::strcmp(name, "Yakumo input script") != 0) return;
            SDL_CloseGamepad(gamepad);
            gamepad = nullptr;
        }
        SDL_Gamepad *device = SDL_OpenGamepad(id);
        if (device == nullptr) {
            std::cout << "[pad] SDL_OpenGamepad failed: " << SDL_GetError() << "\n";
            return;
        }
#if defined(__ANDROID__)
        // Android reports a keyboard with arrow keys (the emulator's qwerty2,
        // many tablets' keyboards) as a D-pad device, and SDL lists it as a
        // gamepad. It has no sticks; taking it as the pad would leave a real
        // controller connected later unused. Its keys still reach the game
        // through the keyboard.
        if (SDL_GetNumJoystickAxes(SDL_GetGamepadJoystick(device)) <= 0) {
            const char *skipped = SDL_GetGamepadName(device);
            std::cout << "[pad] " << (skipped != nullptr ? skipped : "gamepad")
                      << " has no sticks (a keyboard's D-pad), ignored\n";
            SDL_CloseGamepad(device);
            return;
        }
#endif
        gamepad = device;
        gamepad_id = id;
        const char *name = SDL_GetGamepadName(device);
        const PadTuning tuning = pad_tuning();
        std::cout << "[pad] " << (name != nullptr ? name : "gamepad") << " connected; confirm on "
                  << (tuning.confirm_south ? "the south button" : "circle") << ", right stick "
                  << (tuning.right_stick_mode == settings::RightStick::DPad     ? "as D-pad"
                      : tuning.right_stick_mode == settings::RightStick::Camera ? "as camera"
                                                                                : "off")
                  << "\n";
    }

    // Picks up a pad that was already plugged in before the window existed, and
    // falls back to a still-connected second pad when the first one is unplugged.
    void scan_gamepads() {
        int count = 0;
        SDL_JoystickID *ids = SDL_GetGamepads(&count);
        if (ids != nullptr) {
            for (int i = 0; i < count && gamepad == nullptr; ++i) open_gamepad(ids[i]);
            SDL_free(ids);
        }
        if (gamepad != nullptr) return;
        // Say why there is no pad rather than staying silent: a stick with no
        // entry in SDL's mapping database enumerates as a joystick only, which
        // looks identical to "nothing plugged in" from the player's side.
        int joysticks = 0;
        SDL_JoystickID *sticks = SDL_GetJoysticks(&joysticks);
        if (sticks != nullptr) {
            for (int i = 0; i < joysticks; ++i) {
                if (SDL_IsGamepad(sticks[i])) continue;
                const char *name = SDL_GetJoystickNameForID(sticks[i]);
                std::cout << "[pad] " << (name != nullptr ? name : "joystick")
                          << " has no gamepad mapping, ignored\n";
            }
            SDL_free(sticks);
        }
        if (joysticks == 0) std::cout << "[pad] no gamepad connected, keyboard only\n";
    }

    void close_gamepad(SDL_JoystickID id) {
        if (gamepad == nullptr || id != gamepad_id) return;
        SDL_CloseGamepad(gamepad);
        gamepad = nullptr;
        gamepad_id = 0;
        std::cout << "[pad] gamepad disconnected\n";
        scan_gamepads();
    }

    [[nodiscard]] std::uint32_t find_memory_type(std::uint32_t mask, VkMemoryPropertyFlags properties) const {
        VkPhysicalDeviceMemoryProperties memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
        for (std::uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
            if ((mask & (1u << i)) != 0u &&
                (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }
        return 0u;
    }

    bool create_image(std::uint32_t width, std::uint32_t height, VkFormat format, VkImageUsageFlags usage,
                      VkImage &image, VkDeviceMemory &memory, VkImageView &view, VkImageAspectFlags aspect,
                      std::string &error) {
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = format;
        info.extent = {width, height, 1u};
        info.mipLevels = 1u;
        info.arrayLayers = 1u;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (!check(vkCreateImage(device, &info, nullptr, &image), "vkCreateImage", error)) return false;

        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(device, image, &requirements);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate.allocationSize = requirements.size;
        allocate.memoryTypeIndex = find_memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (!check(vkAllocateMemory(device, &allocate, nullptr, &memory), "vkAllocateMemory", error)) return false;
        vkBindImageMemory(device, image, memory, 0u);

        // An image only copied to and from (the overlay, the write-back, a
        // movie frame) has no view: one is not allowed without a usage that
        // reads or renders through it (VUID-VkImageViewCreateInfo-image-04441).
        constexpr VkImageUsageFlags kViewed = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if ((usage & kViewed) == 0u) {
            view = VK_NULL_HANDLE;
            return true;
        }
        VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format;
        view_info.subresourceRange = {aspect, 0u, 1u, 0u, 1u};
        return check(vkCreateImageView(device, &view_info, nullptr, &view), "vkCreateImageView", error);
    }

    void transition(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to,
                    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT) {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout = from;
        barrier.newLayout = to;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = {aspect, 0u, 1u, 0u, 1u};
        barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0u,
                             0u, nullptr, 0u, nullptr, 1u, &barrier);
    }

    [[nodiscard]] VkSampler texture_sampler() const { return sharp_textures ? sharp_sampler : sampler; }
    [[nodiscard]] VkSampler framebuffer_sampler() const {
        return sharp_textures ? clamp_sharp_sampler : clamp_sampler;
    }
    [[nodiscard]] VkPresentModeKHR wanted_present_mode() const;
    bool create_swapchain(std::string &error);
    void destroy_swapchain_views();
    void recreate_swapchain();
#if defined(__ANDROID__)
    // Android takes the window's surface away while the app is in the
    // background (the home screen, a system picker) and gives a new one when
    // it returns: nothing is presented in between, and the Vulkan surface and
    // swapchain are made again for the new one.
    // Set from SDL's event watch, which Android calls on its own thread; one
    // window, so one pair for the process.
    static inline std::atomic<bool> surface_lost{};
    static inline std::atomic<bool> surface_returned{};
    void reset_surface();
    // The native window the Vulkan surface was made for. A different one
    // means Android replaced it while this thread was busy (a system picker
    // blocks it through the whole pause), so no lifecycle event was seen.
    void *native_window{};
    void *current_native_window() const {
        return SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER,
                                      nullptr);
    }
    void check_native_window() {
        void *now = current_native_window();
        if (now != nullptr && now != native_window) surface_returned = true;
    }
#endif
    // Marks the swapchain for rebuilding if it no longer suits the surface.
    // On Android a present is suboptimal while the swapchain's transform
    // differs from the display's, and some drivers keep saying so after the
    // swapchain matches; rebuilding on every such present rebuilt it every
    // frame. So the swapchain is rebuilt only when the display's transform or
    // the window's size really changed: once per turn of the phone.
    void check_swapchain() {
        if (!std::exchange(swapchain_check, false) || swapchain == VK_NULL_HANDLE) return;
#if defined(__ANDROID__)
        VkSurfaceCapabilitiesKHR capabilities{};
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities) != VK_SUCCESS) return;
        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        if (capabilities.currentTransform != swapchain_transform ||
            (width > 0 && height > 0 &&
             (static_cast<std::uint32_t>(width) != swapchain_extent.width ||
              static_cast<std::uint32_t>(height) != swapchain_extent.height)))
            swapchain_dirty = true;
#else
        swapchain_dirty = true;
#endif
    }
#if defined(__ANDROID__)
    static bool SDLCALL watch_lifecycle(void *, SDL_Event *event) {
        if (event->type == SDL_EVENT_WILL_ENTER_BACKGROUND || event->type == SDL_EVENT_DID_ENTER_BACKGROUND)
            surface_lost = true;
        if (event->type == SDL_EVENT_DID_ENTER_FOREGROUND && surface_lost) surface_returned = true;
        return true;
    }
#endif
    bool create_ui_framebuffers(std::string &error);
    void record_game_blit(VkCommandBuffer commands, VkImage source, VkImage destination);
    // Target size for the current settings and window.
    [[nodiscard]] VkExtent2D wanted_target_extent() const;
    // Rebuilds every target at `extent`, its picture scaled into it. Not while
    // a frame is being recorded.
    void resize_targets(VkExtent2D extent);
    // After a present: follows the window's size or a changed setting.
    void follow_window();
    // Fill makes every one of the game's 480x272 pixels wider (or taller)
    // than square. The interface is drawn at `fit_x` x `fit_y` of its size
    // about the screen's centre, which gives it square pixels again. False
    // when nothing needs fitting.
    [[nodiscard]] bool interface_fit(float &fit_x, float &fit_y) const;
    [[nodiscard]] bool shows(std::uint32_t address) const {
        return address != 0u && (address == display_addresses[0] || address == display_addresses[1]);
    }
    void submit_and_present(VkImage source, bool game_frame) {
        submit_and_present(command_buffer, frame_fence, source, game_frame, true);
    }
    // `main_frame`: `commands` is the frame's own command buffer, which ends
    // the frame's recording and GPU timing; otherwise it is a present between
    // the game's flips.
    void submit_and_present(VkCommandBuffer commands, VkFence fence, VkImage source, bool game_frame,
                            bool main_frame);
    void write_capture(VkFence fence);
    void destroy_target(Target &target);
    void run_commands(const std::function<void(VkCommandBuffer)> &record);
    Target *target_for(std::uint32_t address, std::string &error);
    bool create_overlay(std::string &error);
    void record_overlay(VkCommandBuffer commands, VkImage destination);
    void update_display_info();
    // `overwritten`: bit 0, the pass's first draw writes every pixel's colour
    // and alpha; bit 1, every pixel's depth.
    void begin_pass(std::uint32_t address, std::uint32_t overwritten = 0u);
    void end_pass();
    VkPipeline pipeline_for(const PipelineKey &key);
    Texture &texture_for(const GuestMemory &memory, const DrawCall &call);
    void trace_framebuffer_texture(const DrawCall &call);
    // A render target the texture reads, with the texture's first texel as a
    // pixel position inside it; see framebuffer_texture().
    struct FramebufferTexture {
        Target *target{};
        std::uint32_t x{};
        std::uint32_t y{};
    };
    [[nodiscard]] FramebufferTexture find_framebuffer_texture(const GuestMemory &memory,
                                                              const TextureState &texture);
    VkDescriptorSet framebuffer_descriptor(Target &target, bool opaque);
    void snapshot_guest_words(const GuestMemory &memory, std::uint32_t address, Target &target);
    Texture create_texture(std::uint32_t width, std::uint32_t height, const std::uint32_t *pixels);
    // The image, view and descriptor of a texture, with no pixels yet.
    Texture create_texture_image(std::uint32_t width, std::uint32_t height);
    // Copies pixels into a texture's image: ahead of the frame's commands
    // while one is recorded, else at once (waiting for the queue).
    void upload_texture(VkImage image, std::uint32_t width, std::uint32_t height, const std::uint32_t *pixels);
    void destroy_texture(Texture &texture);
};

void VulkanRenderer::Impl::compare_gpu_decode(std::uint32_t from) {
    FrameSlot &frame = slots[from];
    if (!check_gpu_decode || check_mapped == nullptr) return;
    const auto *out = static_cast<const float *>(check_mapped);
    CheckTally &tally = check_tally;
    for (const CheckDraw &draw : frame.checks) {
        ++tally.draws;
        const bool skinned = ((draw.vertex_type >> 9u) & 3u) != 0u;
        for (std::uint32_t v = 0; v < draw.count; ++v) {
            if (frame.check_drawn[draw.first + v] == 0u) continue;
            const float *got = out + (static_cast<std::size_t>(draw.slot) + v * 3u) * 4u;
            const GpuVertex &want = frame.check_expected[draw.first + v];
            const float expected[12]{want.x,  want.y,  want.z,  want.u,
                                     want.nx, want.ny, want.nz, want.v,
                                     static_cast<float>(want.color & 0xFFu) / 255.0f,
                                     static_cast<float>((want.color >> 8u) & 0xFFu) / 255.0f,
                                     static_cast<float>((want.color >> 16u) & 0xFFu) / 255.0f,
                                     static_cast<float>(want.color >> 24u) / 255.0f};
            bool exact = true;
            double worst = 0.0;
            for (std::uint32_t i = 0; i < 12u; ++i) {
                if (got[i] == expected[i]) continue;
                exact = false;
                ++tally.fields[skinned ? 1u : 0u][i];
                // Relative to the size of the vector the value belongs to
                // (a position's largest component, say), at least 1: a small
                // coordinate of a large skinned position is the difference
                // of large terms, and carries their rounding.
                const std::uint32_t group_first = i < 3u ? 0u : (i >= 4u && i < 7u ? 4u : i);
                const std::uint32_t group_size = i < 3u || (i >= 4u && i < 7u) ? 3u : 1u;
                double size = 1.0;
                for (std::uint32_t k = group_first; k < group_first + group_size; ++k)
                    size = std::max(size, std::fabs(static_cast<double>(expected[k])));
                const double error = std::fabs(static_cast<double>(got[i]) - expected[i]) / size;
                worst = std::max(worst, error);
            }
            ++tally.vertices;
            if (exact) {
                ++tally.exact;
            } else if (worst <= (skinned ? 1e-5 : 1e-6)) {
                ++tally.close;
                tally.max_error = std::max(tally.max_error, worst);
            } else {
                if (++tally.differed <= 20u)
                    std::cout << "[gpu-decode-check] vertex type 0x" << std::hex << draw.vertex_type << std::dec
                              << " vertex " << v << " differs by " << worst << ": got (" << got[0] << ", " << got[1]
                              << ", " << got[2] << ") expected (" << expected[0] << ", " << expected[1] << ", "
                              << expected[2] << ")\n";
                tally.max_error = std::max(tally.max_error, worst);
            }
        }
    }
    frame.checks.clear();
    frame.check_expected.clear();
    frame.check_drawn.clear();
    frame.check_used = 0u;
    if (tally.draws != 0u && frames % 300u == 0u)
        std::cout << "[gpu-decode-check] " << tally.draws << " draws, " << tally.vertices << " vertices: "
                  << tally.exact << " exact, " << tally.close << " within rounding (largest " << tally.max_error
                  << "), " << tally.differed << " differed" << std::endl;
    if (tally.draws != 0u && frames % 300u == 0u) {
        // Inexact values of unskinned and skinned vertices, by field: x y z u
        // nx ny nz v r g b a.
        for (std::uint32_t kind = 0; kind < 2u; ++kind) {
            std::cout << "[gpu-decode-check] inexact " << (kind == 0u ? "unskinned" : "skinned") << ":";
            for (const std::uint64_t count : tally.fields[kind]) std::cout << " " << count;
            std::cout << std::endl;
        }
    }
}

void VulkanRenderer::Impl::collect_gpu_time(std::uint32_t from) {
    FrameSlot &frame = slots[from];
    if (gpu_timer == VK_NULL_HANDLE || frame.gpu_timer_pending == 0u) return;
    std::array<std::uint64_t, 4u * kGpuTimerSegments> results{};
    const std::uint32_t count = frame.gpu_timer_pending;
    frame.gpu_timer_pending = 0u;
    const VkResult read = vkGetQueryPoolResults(
        device, gpu_timer, from * 2u * kGpuTimerSegments, count,
        static_cast<std::size_t>(count) * 2u * sizeof(std::uint64_t), results.data(), 2u * sizeof(std::uint64_t),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
    if (read != VK_SUCCESS && read != VK_NOT_READY) return;
    std::uint64_t ticks = 0u;
    bool any = false;
    for (std::uint32_t pair = 0; pair + 1u < count; pair += 2u) {
        const std::uint64_t *begin = &results[pair * 2u];
        const std::uint64_t *end = &results[(pair + 1u) * 2u];
        if (begin[1] == 0u || end[1] == 0u) continue;  // not available
        ticks += (end[0] - begin[0]) & gpu_timer_mask;
        any = true;
    }
    if (any) perf::add_gpu_time(static_cast<double>(ticks) * gpu_timer_ns_per_tick / 1.0e6);
}

VulkanRenderer::VulkanRenderer() : impl_(std::make_unique<Impl>()) {}
VulkanRenderer::~VulkanRenderer() { shutdown(); }

bool VulkanRenderer::available() const noexcept { return impl_ && impl_->ready; }
bool VulkanRenderer::quit_requested() const noexcept { return impl_ && impl_->quit; }
std::uint64_t VulkanRenderer::frames_presented() const noexcept { return impl_ ? impl_->frames : 0u; }
CameraReading VulkanRenderer::camera() const noexcept { return impl_ ? impl_->reading : CameraReading{}; }
std::uint64_t VulkanRenderer::draws_submitted() const noexcept { return impl_ ? impl_->draws : 0u; }

bool VulkanRenderer::initialize(const RendererConfig &config, std::string &error) {
    Impl &impl = *impl_;
    impl.config = config;
    const settings::Settings &player = settings::current();
    impl.requested_scale = std::min(player.internal_scale, settings::kMaxInternalScale);
    // Until the window's size is known, which Auto and Fill need.
    const std::uint32_t scale = impl.requested_scale != 0u ? impl.requested_scale : 2u;
    impl.target_extent = {kPspWidth * scale, kPspHeight * scale};
    impl.requested_present = player.present_mode;
    impl.aspect = player.aspect;
    impl.sharp_screen = player.sharp_screen;
    impl.sharp_textures = player.sharp_textures;
    impl.trace_interpolation = std::getenv("MHP3RD_TRACE_INTERPOLATION") != nullptr;
    if (std::getenv("MHP3RD_INTERPOLATION_NO_MOTION_GUARD") != nullptr) impl.cut_thresholds.max_own_motion = 0.0f;
    impl.frame_rate = player.frame_rate;
    impl.governor.set_automatic(player.frame_rate_auto);
    const std::uint32_t window_scale = std::clamp<std::uint32_t>(player.window_scale, 1u, settings::kMaxWindowScale);

#if defined(__APPLE__)
    // MoltenVK turns a frame's Vulkan commands into Metal ones when they are
    // submitted, on the calling thread: 1-2 ms of the game's thread a frame.
    // Its asynchronous submits do that on a thread of their own, but they
    // crashed a release build after minutes of play (a freed Objective-C
    // object retained on MoltenVK's dispatch queue), so they are opt-in:
    // MHP3RD_MOLTENVK_ASYNC_SUBMITS=1, or MoltenVK's own variable set to 0.
    if (const char *async = std::getenv("MHP3RD_MOLTENVK_ASYNC_SUBMITS"); async != nullptr && *async == '1')
        setenv("MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS", "0", 0);
#endif
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        error = std::string("SDL_Init failed: ") + SDL_GetError();
        return false;
    }
    // A missing gamepad subsystem is not fatal; the keyboard still drives the pad.
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) std::cout << "[pad] no gamepad support: " << SDL_GetError() << "\n";
    else impl.scan_gamepads();
    SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    if (player.fullscreen) window_flags |= SDL_WINDOW_FULLSCREEN;
#if defined(__ANDROID__)
    // A phone runs the game full screen with the system bars hidden: shown,
    // they cover the top and bottom of the picture (Android 15 draws apps
    // under them). The safe area keeps the picture clear of a camera cutout.
    window_flags |= SDL_WINDOW_FULLSCREEN;
#endif
    impl.window = SDL_CreateWindow(config.title.c_str(), static_cast<int>(kPspWidth * window_scale),
                                   static_cast<int>(kPspHeight * window_scale), window_flags);
    if (impl.window == nullptr) {
        error = std::string("SDL_CreateWindow failed: ") + SDL_GetError();
        return false;
    }

    std::uint32_t extension_count = 0u;
    const char *const *sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&extension_count);
    std::vector<const char *> extensions(sdl_extensions, sdl_extensions + extension_count);
    // Only a loader that offers portability enumeration may be asked for it:
    // an instance extension the loader does not list fails vkCreateInstance.
    std::uint32_t available_count = 0u;
    vkEnumerateInstanceExtensionProperties(nullptr, &available_count, nullptr);
    std::vector<VkExtensionProperties> available(available_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &available_count, available.data());
    bool portability_enumeration = false;
    for (const VkExtensionProperties &extension : available)
        if (std::strcmp(extension.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0)
            portability_enumeration = true;
    if (portability_enumeration) extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    application.pApplicationName = "Yakumo";
    application.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &application;
    instance_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    instance_info.ppEnabledExtensionNames = extensions.data();
    // MoltenVK reports itself as a portability driver and refuses the instance
    // without this flag.
    if (portability_enumeration) instance_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    if (!check(vkCreateInstance(&instance_info, nullptr, &impl.instance), "vkCreateInstance", error)) return false;

#if defined(__ANDROID__)
    SDL_AddEventWatch(&Impl::watch_lifecycle, &impl);
    impl.native_window = impl.current_native_window();
#endif
    if (!SDL_Vulkan_CreateSurface(impl.window, impl.instance, nullptr, &impl.surface)) {
        error = std::string("SDL_Vulkan_CreateSurface failed: ") + SDL_GetError();
        return false;
    }

    std::uint32_t device_count = 0u;
    vkEnumeratePhysicalDevices(impl.instance, &device_count, nullptr);
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(impl.instance, &device_count, devices.data());
    if (devices.empty()) {
        error = "no Vulkan device found";
        return false;
    }
    impl.physical_device = devices.front();
    for (VkPhysicalDevice candidate : devices) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            impl.physical_device = candidate;
            break;
        }
    }

    std::uint32_t family_count = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(impl.physical_device, &family_count, nullptr);
    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(impl.physical_device, &family_count, families.data());
    bool found_family = false;
    for (std::uint32_t i = 0; i < family_count; ++i) {
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(impl.physical_device, i, impl.surface, &present);
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u && present == VK_TRUE) {
            impl.queue_family = i;
            found_family = true;
            break;
        }
    }
    if (!found_family) {
        error = "no graphics queue with presentation support";
        return false;
    }

    std::uint32_t device_extension_count = 0u;
    vkEnumerateDeviceExtensionProperties(impl.physical_device, nullptr, &device_extension_count, nullptr);
    std::vector<VkExtensionProperties> device_extensions(device_extension_count);
    vkEnumerateDeviceExtensionProperties(impl.physical_device, nullptr, &device_extension_count,
                                         device_extensions.data());
    std::vector<const char *> enabled_device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const bool want_breadcrumbs = std::getenv("MHP3RD_GPU_BREADCRUMBS") != nullptr;
    for (const VkExtensionProperties &extension : device_extensions) {
        if (std::strcmp(extension.extensionName, "VK_KHR_portability_subset") == 0)
            enabled_device_extensions.push_back("VK_KHR_portability_subset");
        if (want_breadcrumbs && std::strcmp(extension.extensionName, VK_AMD_BUFFER_MARKER_EXTENSION_NAME) == 0) {
            enabled_device_extensions.push_back(VK_AMD_BUFFER_MARKER_EXTENSION_NAME);
            impl.breadcrumbs.available = true;
        }
    }
    if (want_breadcrumbs && !impl.breadcrumbs.available)
        std::cout << "[render] MHP3RD_GPU_BREADCRUMBS needs VK_AMD_buffer_marker, which this device lacks\n";

    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = impl.queue_family;
    queue_info.queueCount = 1u;
    queue_info.pQueuePriorities = &priority;
    // MHP3RD_CHECK_GPU_DECODE: the check build of the raw vertex shader
    // writes what it decoded to a storage buffer.
    VkPhysicalDeviceFeatures supported_features{};
    vkGetPhysicalDeviceFeatures(impl.physical_device, &supported_features);
    VkPhysicalDeviceFeatures enabled_features{};
    if (std::getenv("MHP3RD_CHECK_GPU_DECODE") != nullptr) {
        if (supported_features.vertexPipelineStoresAndAtomics == VK_TRUE) {
            enabled_features.vertexPipelineStoresAndAtomics = VK_TRUE;
            impl.check_gpu_decode = true;
        } else {
            std::cout << "[render] MHP3RD_CHECK_GPU_DECODE needs vertexPipelineStoresAndAtomics, which this device "
                         "lacks\n";
        }
    }
    // Out-of-range buffer reads (a vertex, an index, the raw vertex bytes the
    // vertex shader reads, a uniform) return zeros or stay within the buffer
    // instead of reading whatever memory follows it, which on some GPUs can
    // fault or hang. MHP3RD_NO_ROBUST_BUFFERS leaves it off, as before.
    if (supported_features.robustBufferAccess == VK_TRUE && std::getenv("MHP3RD_NO_ROBUST_BUFFERS") == nullptr)
        enabled_features.robustBufferAccess = VK_TRUE;
    std::cout << "[render] robust buffer access " << (enabled_features.robustBufferAccess ? "on" : "off") << "\n";
    VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.pEnabledFeatures = &enabled_features;
    device_info.queueCreateInfoCount = 1u;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = static_cast<std::uint32_t>(enabled_device_extensions.size());
    device_info.ppEnabledExtensionNames = enabled_device_extensions.data();
    if (!check(vkCreateDevice(impl.physical_device, &device_info, nullptr, &impl.device), "vkCreateDevice", error))
        return false;
    vkGetDeviceQueue(impl.device, impl.queue_family, 0u, &impl.queue);
    if (impl.breadcrumbs.available) {
        Impl::Breadcrumbs &b = impl.breadcrumbs;
        VkBufferCreateInfo marker_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        marker_info.size = 256u;
        marker_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VkMemoryRequirements marker_requirements{};
        void *mapped = nullptr;
        if (vkCreateBuffer(impl.device, &marker_info, nullptr, &b.buffer) == VK_SUCCESS) {
            vkGetBufferMemoryRequirements(impl.device, b.buffer, &marker_requirements);
            VkMemoryAllocateInfo marker_allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            marker_allocate.allocationSize = marker_requirements.size;
            marker_allocate.memoryTypeIndex = impl.find_memory_type(
                marker_requirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (vkAllocateMemory(impl.device, &marker_allocate, nullptr, &b.memory) == VK_SUCCESS &&
                vkBindBufferMemory(impl.device, b.buffer, b.memory, 0u) == VK_SUCCESS &&
                vkMapMemory(impl.device, b.memory, 0u, 256u, 0u, &mapped) == VK_SUCCESS) {
                b.mapped = static_cast<volatile std::uint32_t *>(mapped);
                b.mapped[0] = b.mapped[1] = 0u;
                b.write = reinterpret_cast<PFN_vkCmdWriteBufferMarkerAMD>(
                    vkGetDeviceProcAddr(impl.device, "vkCmdWriteBufferMarkerAMD"));
            }
        }
        std::cout << "[render] GPU breadcrumbs " << (b.write != nullptr ? "on" : "unavailable") << "\n";
    }
    impl.load_pipeline_cache();

    // Swapchain.
    std::uint32_t format_count = 0u;
    vkGetPhysicalDeviceSurfaceFormatsKHR(impl.physical_device, impl.surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(impl.physical_device, impl.surface, &format_count, formats.data());
    // The GE's colours are already gamma-encoded, so they must reach the display
    // unchanged: prefer a plain 8-bit UNORM format. Gamescope (Steam Deck Game
    // Mode) lists an _SRGB format first, and presenting through it encodes the
    // colours a second time and washes the picture out.
    if (!formats.empty()) {
        impl.surface_format = formats.front();
        for (const VkSurfaceFormatKHR &candidate : formats) {
            if ((candidate.format == VK_FORMAT_B8G8R8A8_UNORM || candidate.format == VK_FORMAT_R8G8B8A8_UNORM) &&
                candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                impl.surface_format = candidate;
                break;
            }
        }
    }
    impl.swapchain_format = impl.surface_format.format;
    std::uint32_t mode_count = 0u;
    vkGetPhysicalDeviceSurfacePresentModesKHR(impl.physical_device, impl.surface, &mode_count, nullptr);
    impl.present_modes.resize(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(impl.physical_device, impl.surface, &mode_count,
                                              impl.present_modes.data());
    if (!impl.create_swapchain(error)) return false;
    // No target exists yet: they are made at this size when first drawn.
    impl.target_extent = impl.wanted_target_extent();

    std::array<VkAttachmentDescription, 2> attachments{};
    attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[1] = attachments[0];
    impl.choose_depth_format();
    attachments[1].format = impl.depth_format;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference color_reference{0u, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_reference{1u, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1u;
    subpass.pColorAttachments = &color_reference;
    subpass.pDepthStencilAttachment = &depth_reference;
    VkRenderPassCreateInfo render_pass_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1u;
    render_pass_info.pSubpasses = &subpass;
    if (!check(vkCreateRenderPass(impl.device, &render_pass_info, nullptr, &impl.render_pass), "vkCreateRenderPass",
               error))
        return false;
    if (std::getenv("MHP3RD_NO_CLEAR_LOAD") == nullptr) {
        for (std::uint32_t variant = 1u; variant < 4u; ++variant) {
            std::array<VkAttachmentDescription, 2> skipped = attachments;
            if ((variant & 1u) != 0u) skipped[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            if ((variant & 2u) != 0u) skipped[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            VkRenderPassCreateInfo variant_info = render_pass_info;
            variant_info.pAttachments = skipped.data();
            if (vkCreateRenderPass(impl.device, &variant_info, nullptr, &impl.discard_passes[variant]) != VK_SUCCESS)
                impl.discard_passes[variant] = VK_NULL_HANDLE;
        }
    }

    // Shaders, descriptors and pipeline layout.
    const auto create_shader = [&](const std::uint32_t *code, std::size_t size, VkShaderModule &module) {
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = size;
        info.pCode = code;
        return check(vkCreateShaderModule(impl.device, &info, nullptr, &module), "vkCreateShaderModule", error);
    };
    if (!create_shader(kGeVertexShader, sizeof(kGeVertexShader), impl.vertex_shader)) return false;
    if (impl.check_gpu_decode) {
        if (!create_shader(kGeCheckVertexShader, sizeof(kGeCheckVertexShader), impl.raw_vertex_shader)) return false;
    } else if (!create_shader(kGeRawVertexShader, sizeof(kGeRawVertexShader), impl.raw_vertex_shader)) {
        return false;
    }
    if (!create_shader(kGeFragmentShader, sizeof(kGeFragmentShader), impl.fragment_shader)) return false;

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0u;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1u;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = 1u;
    layout_info.pBindings = &binding;
    if (!check(vkCreateDescriptorSetLayout(impl.device, &layout_info, nullptr, &impl.descriptor_layout),
               "vkCreateDescriptorSetLayout", error))
        return false;

    // Set 1: the lighting environment and the lit object, windows into the
    // vertex buffer.
    // Bindings 2 to 4 are GPU vertex decode's: its raw block, the vertex
    // buffer read as words, and the check output (ge.vert).
    std::array<VkDescriptorSetLayoutBinding, 5> lighting_bindings{};
    lighting_bindings[0].binding = 0u;
    lighting_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    lighting_bindings[0].descriptorCount = 1u;
    lighting_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    lighting_bindings[1].binding = 1u;
    lighting_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    lighting_bindings[1].descriptorCount = 1u;
    lighting_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    lighting_bindings[2] = lighting_bindings[1];
    lighting_bindings[2].binding = 2u;
    lighting_bindings[3] = lighting_bindings[1];
    lighting_bindings[3].binding = 3u;
    lighting_bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lighting_bindings[4] = lighting_bindings[3];
    lighting_bindings[4].binding = 4u;
    VkDescriptorSetLayoutCreateInfo lighting_layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    lighting_layout_info.bindingCount = static_cast<std::uint32_t>(lighting_bindings.size());
    lighting_layout_info.pBindings = lighting_bindings.data();
    if (!check(vkCreateDescriptorSetLayout(impl.device, &lighting_layout_info, nullptr, &impl.lighting_layout),
               "vkCreateDescriptorSetLayout", error))
        return false;

    // Evicted textures keep their sets until their frame has finished, so
    // the pool has room for a second cache's worth.
    const std::array<VkDescriptorPoolSize, 3> pool_sizes{
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                             static_cast<std::uint32_t>(2u * kMaxCachedTextures + 1u + kMaxFramebufferTextureSets)},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 3u},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u},
    };
    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = static_cast<std::uint32_t>(2u * kMaxCachedTextures + 2u + kMaxFramebufferTextureSets);
    pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    if (!check(vkCreateDescriptorPool(impl.device, &pool_info, nullptr, &impl.descriptor_pool),
               "vkCreateDescriptorPool", error))
        return false;

    VkPushConstantRange push_range{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0u,
                                   sizeof(PushConstants)};
    VkPipelineLayoutCreateInfo pipeline_layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    const std::array<VkDescriptorSetLayout, 2> set_layouts{impl.descriptor_layout, impl.lighting_layout};
    pipeline_layout_info.setLayoutCount = static_cast<std::uint32_t>(set_layouts.size());
    pipeline_layout_info.pSetLayouts = set_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = 1u;
    pipeline_layout_info.pPushConstantRanges = &push_range;
    if (!check(vkCreatePipelineLayout(impl.device, &pipeline_layout_info, nullptr, &impl.pipeline_layout),
               "vkCreatePipelineLayout", error))
        return false;

    VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.maxLod = 1.0f;
    if (!check(vkCreateSampler(impl.device, &sampler_info, nullptr, &impl.sampler), "vkCreateSampler", error))
        return false;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    if (!check(vkCreateSampler(impl.device, &sampler_info, nullptr, &impl.sharp_sampler), "vkCreateSampler", error))
        return false;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (!check(vkCreateSampler(impl.device, &sampler_info, nullptr, &impl.clamp_sharp_sampler), "vkCreateSampler",
               error))
        return false;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    if (!check(vkCreateSampler(impl.device, &sampler_info, nullptr, &impl.clamp_sampler), "vkCreateSampler", error))
        return false;

    // Command buffer, synchronization and the vertex staging buffer.
    VkCommandPoolCreateInfo command_pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = impl.queue_family;
    if (!check(vkCreateCommandPool(impl.device, &command_pool_info, nullptr, &impl.command_pool), "vkCreateCommandPool",
               error))
        return false;
    VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_info.commandPool = impl.command_pool;
    command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_info.commandBufferCount = 1u;
    VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (const char *text = std::getenv("MHP3RD_FRAMES_IN_FLIGHT"); text != nullptr)
        impl.slot_count = std::clamp<std::uint32_t>(static_cast<std::uint32_t>(std::atoi(text)), 1u, Impl::kMaxSlots);
    std::cout << "[render] " << impl.slot_count << " frame" << (impl.slot_count == 1u ? "" : "s") << " in flight\n";
    for (Impl::FrameSlot &frame : impl.slots) {
        if (!check(vkAllocateCommandBuffers(impl.device, &command_info, &frame.commands), "vkAllocateCommandBuffers",
                   error) ||
            !check(vkAllocateCommandBuffers(impl.device, &command_info, &frame.uploads), "vkAllocateCommandBuffers",
                   error) ||
            !check(vkCreateFence(impl.device, &fence_info, nullptr, &frame.fence), "vkCreateFence", error))
            return false;
    }
    impl.command_buffer = impl.slots[0].commands;
    impl.frame_fence = impl.slots[0].fence;
    impl.frame_uploads = impl.slots[0].uploads;
    // Frame interpolation's presents between flips.
    command_info.commandBufferCount = static_cast<std::uint32_t>(impl.present_commands.size());
    if (!check(vkAllocateCommandBuffers(impl.device, &command_info, impl.present_commands.data()),
               "vkAllocateCommandBuffers", error))
        return false;
    for (VkFence &fence : impl.present_fences) vkCreateFence(impl.device, &fence_info, nullptr, &fence);
    VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (VkSemaphore &semaphore : impl.image_available) vkCreateSemaphore(impl.device, &semaphore_info, nullptr, &semaphore);

    // GPU timestamps, where the queue has them. Without them the perf line
    // reads "gpu n/a" and nothing else changes.
    {
        VkPhysicalDeviceProperties timer_properties{};
        vkGetPhysicalDeviceProperties(impl.physical_device, &timer_properties);
        const std::uint32_t valid_bits = families[impl.queue_family].timestampValidBits;
        const float period = timer_properties.limits.timestampPeriod;
        const char *why = nullptr;
        if (std::getenv("MHP3RD_NO_GPU_TIMESTAMPS") != nullptr) why = "turned off (MHP3RD_NO_GPU_TIMESTAMPS)";
        else if (valid_bits == 0u) why = "the graphics queue has no timestamps";
        else if (!(period > 0.0f)) why = "the device reports no timestamp period";
        if (why == nullptr) {
            VkQueryPoolCreateInfo query_info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
            query_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
            query_info.queryCount = Impl::kMaxSlots * 2u * kGpuTimerSegments;
            if (vkCreateQueryPool(impl.device, &query_info, nullptr, &impl.gpu_timer) != VK_SUCCESS) {
                impl.gpu_timer = VK_NULL_HANDLE;
                why = "vkCreateQueryPool failed";
            }
        }
        if (why == nullptr) {
            // A pair more for each of the two presents between flips.
            VkQueryPoolCreateInfo present_info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
            present_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
            present_info.queryCount = 4u;
            if (vkCreateQueryPool(impl.device, &present_info, nullptr, &impl.present_timer) != VK_SUCCESS)
                impl.present_timer = VK_NULL_HANDLE;
            impl.gpu_timer_ns_per_tick = static_cast<double>(period);
            impl.gpu_timer_mask = valid_bits >= 64u ? ~0ull : (1ull << valid_bits) - 1ull;
            std::cout << "[perf] GPU timestamps: " << valid_bits << " bits, " << period << " ns per tick\n";
        } else {
            perf::set_gpu_time_unavailable();
            std::cout << "[perf] no GPU time: " << why << "\n";
        }
    }

    VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = kVertexBufferTotal;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (!check(vkCreateBuffer(impl.device, &buffer_info, nullptr, &impl.vertex_buffer), "vkCreateBuffer", error))
        return false;
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(impl.device, impl.vertex_buffer, &requirements);
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = impl.find_memory_type(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!check(vkAllocateMemory(impl.device, &allocate, nullptr, &impl.vertex_memory), "vkAllocateMemory", error))
        return false;
    vkBindBufferMemory(impl.device, impl.vertex_buffer, impl.vertex_memory, 0u);
    vkMapMemory(impl.device, impl.vertex_memory, 0u, kVertexBufferTotal, 0u, &impl.vertex_mapped);

    buffer_info.size = kIndexBufferTotal;
    buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (!check(vkCreateBuffer(impl.device, &buffer_info, nullptr, &impl.index_buffer), "vkCreateBuffer", error))
        return false;
    vkGetBufferMemoryRequirements(impl.device, impl.index_buffer, &requirements);
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = impl.find_memory_type(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!check(vkAllocateMemory(impl.device, &allocate, nullptr, &impl.index_memory), "vkAllocateMemory", error))
        return false;
    vkBindBufferMemory(impl.device, impl.index_buffer, impl.index_memory, 0u);
    vkMapMemory(impl.device, impl.index_memory, 0u, kIndexBufferTotal, 0u, &impl.index_mapped);

    {
        VkPhysicalDeviceProperties device_properties{};
        vkGetPhysicalDeviceProperties(impl.physical_device, &device_properties);
        impl.uniform_alignment = std::max<VkDeviceSize>(device_properties.limits.minUniformBufferOffsetAlignment, 16u);
        VkDescriptorSetAllocateInfo lighting_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        lighting_info.descriptorPool = impl.descriptor_pool;
        lighting_info.descriptorSetCount = 1u;
        lighting_info.pSetLayouts = &impl.lighting_layout;
        if (!check(vkAllocateDescriptorSets(impl.device, &lighting_info, &impl.lighting_descriptor),
                   "vkAllocateDescriptorSets", error))
            return false;
        // The vertex buffer read as words must fit the storage range every
        // device offers at least (2^27 bytes); GPU vertex decode needs it.
        impl.gpu_decode_available = kVertexBufferTotal <= device_properties.limits.maxStorageBufferRange;
        if (impl.check_gpu_decode) {
            // Two frame slots' worth, each with room past the compared part
            // for draws beyond it to write into.
            const VkDeviceSize bytes = 2u * 2u * Impl::kCheckVertices * 3u * 16u;
            VkBufferCreateInfo check_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            check_info.size = bytes;
            check_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            VkMemoryRequirements check_requirements{};
            if (vkCreateBuffer(impl.device, &check_info, nullptr, &impl.check_buffer) == VK_SUCCESS) {
                vkGetBufferMemoryRequirements(impl.device, impl.check_buffer, &check_requirements);
                VkMemoryAllocateInfo check_allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
                check_allocate.allocationSize = check_requirements.size;
                check_allocate.memoryTypeIndex = impl.find_memory_type(
                    check_requirements.memoryTypeBits,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                vkAllocateMemory(impl.device, &check_allocate, nullptr, &impl.check_memory);
                vkBindBufferMemory(impl.device, impl.check_buffer, impl.check_memory, 0u);
                vkMapMemory(impl.device, impl.check_memory, 0u, bytes, 0u, &impl.check_mapped);
            }
        }
        const std::array<VkDescriptorBufferInfo, 5> lighting_buffers{
            VkDescriptorBufferInfo{impl.vertex_buffer, 0u, sizeof(EnvironmentBlock)},
            VkDescriptorBufferInfo{impl.vertex_buffer, 0u, sizeof(ObjectBlock)},
            VkDescriptorBufferInfo{impl.vertex_buffer, 0u, sizeof(RawBlock)},
            VkDescriptorBufferInfo{impl.vertex_buffer, 0u, VK_WHOLE_SIZE},
            VkDescriptorBufferInfo{impl.check_buffer, 0u, VK_WHOLE_SIZE},
        };
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = impl.lighting_descriptor;
        write.descriptorCount = 1u;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        std::array<VkWriteDescriptorSet, 5> writes{write, write, write, write, write};
        for (std::uint32_t i = 0; i < 5u; ++i) {
            writes[i].dstBinding = i;
            writes[i].pBufferInfo = &lighting_buffers[i];
            if (i >= 3u) writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        }
        const std::uint32_t write_count = impl.check_buffer != VK_NULL_HANDLE ? 5u : 4u;
        vkUpdateDescriptorSets(impl.device, write_count, writes.data(), 0u, nullptr);
    }

    const std::uint32_t white = 0xFFFFFFFFu;
    impl.white_texture = impl.create_texture(1u, 1u, &white);

    // Texture packs are optional too: without the GPU side, none is loaded.
    std::string replacement_error;
    if (impl.replacements.initialize({impl.physical_device, impl.device, impl.queue, impl.queue_family,
                                      impl.descriptor_layout},
                                     replacement_error)) {
        impl.replacements.set_sharp(impl.sharp_textures);
        impl.pack_wanted = player.texture_pack;
        impl.pack_applied = !impl.pack_wanted;
        impl.apply_texture_pack();
    } else {
        std::cout << "[texpack] unavailable: " << replacement_error << "\n";
    }
    if (const char *dump = std::getenv("MHP3RD_TEXTURE_DUMP"); dump != nullptr && *dump != '\0') {
        impl.dumper = std::make_unique<TextureDumper>(dump);
        std::cout << "[texpack] writing new textures to " << dump << "\n";
    }

    // The overlay is optional: without it the game still runs, only unmeasured
    // on screen.
    std::string overlay_error;
    impl.overlay_ready = impl.create_overlay(overlay_error);
    if (!impl.overlay_ready) std::cout << "[perf] overlay unavailable: " << overlay_error << "\n";
    impl.overlay_visible = impl.overlay_ready && perf::options().overlay;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(impl.physical_device, &properties);
    impl.device_name = properties.deviceName;
    impl.prewarm_pipelines();
    // What a player's log needs to tell drivers and memory apart.
    {
        VkPhysicalDeviceMemoryProperties memory{};
        vkGetPhysicalDeviceMemoryProperties(impl.physical_device, &memory);
        VkMemoryRequirements vertex_requirements{};
        vkGetBufferMemoryRequirements(impl.device, impl.vertex_buffer, &vertex_requirements);
        const std::uint32_t vertex_type = impl.find_memory_type(
            vertex_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        const VkMemoryPropertyFlags flags = memory.memoryTypes[vertex_type].propertyFlags;
        std::cout << "[render] Vulkan " << VK_API_VERSION_MAJOR(properties.apiVersion) << "."
                  << VK_API_VERSION_MINOR(properties.apiVersion) << "." << VK_API_VERSION_PATCH(properties.apiVersion)
                  << ", driver 0x" << std::hex << properties.driverVersion << ", vendor 0x" << properties.vendorID
                  << ", device 0x" << properties.deviceID << std::dec << "; vertex buffer "
                  << (kVertexBufferTotal >> 20u) << " MiB in memory type " << vertex_type << " ("
                  << ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0u ? "device local, " : "")
                  << ((flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0u ? "cached" : "uncached")
                  << "); depth format " << static_cast<int>(impl.depth_format) << "\n";
    }
    std::cout << "Renderer: Vulkan on " << properties.deviceName << ", target " << impl.target_extent.width << "x"
              << impl.target_extent.height << "\n";
    impl.ready = true;
    return true;
}

bool VulkanRenderer::Impl::create_overlay(std::string &error) {
    if (!create_image(perf::kOverlayWidth, perf::kOverlayHeight, VK_FORMAT_R8G8B8A8_UNORM,
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, overlay_image, overlay_memory,
                      overlay_view, VK_IMAGE_ASPECT_COLOR_BIT, error))
        return false;
    overlay_pixels.assign(static_cast<std::size_t>(perf::kOverlayWidth) * perf::kOverlayHeight, 0u);
    const VkDeviceSize bytes = overlay_pixels.size() * sizeof(std::uint32_t);
    VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = bytes;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    for (Staging &staging : overlay_staging) {
    if (!check(vkCreateBuffer(device, &buffer_info, nullptr, &staging.buffer), "vkCreateBuffer", error)) return false;
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, staging.buffer, &requirements);
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = find_memory_type(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!check(vkAllocateMemory(device, &allocate, nullptr, &staging.memory), "vkAllocateMemory", error))
        return false;
    vkBindBufferMemory(device, staging.buffer, staging.memory, 0u);
    if (!check(vkMapMemory(device, staging.memory, 0u, bytes, 0u, &staging.mapped), "vkMapMemory", error))
        return false;
    }
    return true;
}

// Draws the overlay over the top-left corner of the swapchain image, which is
// in TRANSFER_DST layout with the game frame already blitted into it. The
// staging buffer is free to rewrite: the frame fence has been waited on before
// any recording of this frame started.
void VulkanRenderer::Impl::record_overlay(VkCommandBuffer commands, VkImage destination) {
    const std::uint32_t scale = perf::overlay_scale(content_rect.extent.height);
    const std::uint32_t inset = 4u * scale;
    const std::uint32_t width = perf::kOverlayWidth * scale;
    const std::uint32_t height = perf::kOverlayHeight * scale;
    if (inset + width > content_rect.extent.width || inset + height > content_rect.extent.height) return;

    perf::draw_overlay(overlay_pixels.data());
    // Free to rewrite: this command buffer's previous use has finished.
    const Staging &staging = overlay_staging[command_index(commands)];
    std::memcpy(staging.mapped, overlay_pixels.data(), overlay_pixels.size() * sizeof(std::uint32_t));
    transition(commands, overlay_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    copy.imageExtent = {perf::kOverlayWidth, perf::kOverlayHeight, 1u};
    vkCmdCopyBufferToImage(commands, staging.buffer, overlay_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u,
                           &copy);
    transition(commands, overlay_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    // The game frame was blitted into the same image just before; order the
    // two writes.
    transition(commands, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageBlit blit{};
    blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    blit.srcOffsets[1] = {static_cast<std::int32_t>(perf::kOverlayWidth),
                          static_cast<std::int32_t>(perf::kOverlayHeight), 1};
    blit.dstSubresource = blit.srcSubresource;
    const std::int32_t left = content_rect.offset.x + static_cast<std::int32_t>(inset);
    const std::int32_t top = content_rect.offset.y + static_cast<std::int32_t>(inset);
    blit.dstOffsets[0] = {left, top, 0};
    blit.dstOffsets[1] = {left + static_cast<std::int32_t>(width), top + static_cast<std::int32_t>(height), 1};
    vkCmdBlitImage(commands, overlay_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &blit, VK_FILTER_NEAREST);
}

void VulkanRenderer::Impl::update_display_info() {
    float refresh = 0.0f;
    if (const SDL_DisplayID display = SDL_GetDisplayForWindow(window); display != 0u) {
        if (const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display); mode != nullptr)
            refresh = mode->refresh_rate;
    }
    display_hz = refresh;
    perf::set_display_info(present_mode_name(present_mode), swapchain_extent.width, swapchain_extent.height, refresh);
}

VkPresentModeKHR VulkanRenderer::Impl::wanted_present_mode() const {
    VkPresentModeKHR wanted = VK_PRESENT_MODE_FIFO_KHR;
    if (requested_present == settings::PresentMode::Mailbox) wanted = VK_PRESENT_MODE_MAILBOX_KHR;
    if (requested_present == settings::PresentMode::Immediate) wanted = VK_PRESENT_MODE_IMMEDIATE_KHR;
    // FIFO is the one mode every surface supports.
    return std::find(present_modes.begin(), present_modes.end(), wanted) != present_modes.end()
               ? wanted
               : VK_PRESENT_MODE_FIFO_KHR;
}

// Builds the swapchain for the window's current size, replacing the old one.
bool VulkanRenderer::Impl::create_swapchain(std::string &error) {
    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == 0xFFFFFFFFu) {
        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        extent.width = std::clamp(static_cast<std::uint32_t>(std::max(width, 0)), capabilities.minImageExtent.width,
                                  capabilities.maxImageExtent.width);
        extent.height = std::clamp(static_cast<std::uint32_t>(std::max(height, 0)),
                                   capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    if (extent.width == 0u || extent.height == 0u) extent = target_extent;
    const VkSurfaceTransformFlagBitsKHR transform = capabilities.currentTransform;
    VkExtent2D image_extent = extent;
#if defined(__ANDROID__)
    // A display turned sideways reports a quarter or half turn. The frame is
    // drawn upright at the window's size and turned by a last pass into
    // images in the panel's orientation (see UprightImage).
    swapchain_quarter_turns = transform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR    ? 1
                              : transform == VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR ? 2
                              : transform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR ? 3
                                                                                     : 0;
    {
        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        if (width > 0 && height > 0)
            extent = {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
    }
    image_extent = swapchain_quarter_turns % 2 == 1 ? VkExtent2D{extent.height, extent.width} : extent;
    swapchain_image_extent = image_extent;
    swapchain_transform = transform;
    // Once per start and per turn of the phone; a run of these in a player's
    // log (or logcat) means the swapchain is rebuilt over and over.
    ++swapchain_builds;
    std::cout << "[render] swapchain " << swapchain_builds << " for " << extent.width << "x" << extent.height
              << ", surface transform 0x" << std::hex << static_cast<unsigned>(transform) << std::dec
              << " (a quarter turn x" << swapchain_quarter_turns << ")\n";
    SDL_Log("Yakumo: swapchain %u for %ux%u, surface transform 0x%x", swapchain_builds, extent.width, extent.height,
            static_cast<unsigned>(transform));
#endif
    swapchain_extent = extent;
    update_content_rect();
    present_mode = wanted_present_mode();

    std::uint32_t image_count =
        std::max(capabilities.minImageCount, present_mode == VK_PRESENT_MODE_MAILBOX_KHR ? 3u : 2u);
    if (capabilities.maxImageCount != 0u) image_count = std::min(image_count, capabilities.maxImageCount);
    // Transfer source only where offered: it is just for window captures.
    swapchain_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

    const VkSwapchainKHR old_swapchain = swapchain;
    VkSwapchainCreateInfoKHR swapchain_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = swapchain_format;
    swapchain_info.imageColorSpace = surface_format.colorSpace;
    swapchain_info.imageExtent = image_extent;
    swapchain_info.imageArrayLayers = 1u;
    swapchain_info.imageUsage = swapchain_usage;
    swapchain_info.preTransform = transform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = old_swapchain;
    VkSwapchainKHR created{};
    const VkResult result = vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &created);
    if (old_swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, old_swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
    if (!check(result, "vkCreateSwapchainKHR", error)) return false;
    swapchain = created;
    swapchain_min_images = image_count;

    std::uint32_t count = 0u;
    vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr);
    swapchain_images.resize(count);
    vkGetSwapchainImagesKHR(device, swapchain, &count, swapchain_images.data());
    // One semaphore per image, kept across swapchains: an old one may still
    // be waited on by a present of the swapchain this replaced.
    while (render_finished.size() < count) {
        VkSemaphoreCreateInfo semaphore_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkSemaphore semaphore{};
        if (vkCreateSemaphore(device, &semaphore_info, nullptr, &semaphore) != VK_SUCCESS) break;
        render_finished.push_back(semaphore);
    }
    for (VkImage image : swapchain_images) {
        VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = swapchain_format;
        view_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};
        VkImageView view{};
        if (!check(vkCreateImageView(device, &view_info, nullptr, &view), "vkCreateImageView", error)) return false;
        swapchain_views.push_back(view);
    }
#if defined(__ANDROID__)
    if (swapchain_quarter_turns != 0 && !create_upright_images(error)) return false;
#endif
    if (ui_render_pass != VK_NULL_HANDLE && !create_ui_framebuffers(error)) return false;
    swapchain_dirty = false;
    update_display_info();
    std::cout << "[render] swapchain " << image_extent.width << "x" << image_extent.height << ", " << count
              << " images, " << present_mode_name(present_mode) << "\n";
    return true;
}

void VulkanRenderer::Impl::destroy_swapchain_views() {
    for (VkFramebuffer framebuffer : ui_framebuffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
    ui_framebuffers.clear();
    for (VkImageView view : swapchain_views) vkDestroyImageView(device, view, nullptr);
    swapchain_views.clear();
#if defined(__ANDROID__)
    destroy_upright_images();
#endif
}

#if defined(__ANDROID__)
void VulkanRenderer::Impl::reset_surface() {
    surface_returned = false;
    vkDeviceWaitIdle(device);
    destroy_swapchain_views();
    if (swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
    if (surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance, surface, nullptr);
    surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        std::cout << "[render] cannot make the window's surface again: " << SDL_GetError() << "\n";
        return;
    }
    surface_lost = false;
    native_window = current_native_window();
    std::string error;
    if (!create_swapchain(error)) std::cout << "[render] cannot recreate the swapchain: " << error << "\n";
    else std::cout << "[render] surface made again after the app returned\n";
}
#endif

void VulkanRenderer::Impl::recreate_swapchain() {
    // A minimised window has no area to present to; keep the old swapchain
    // and try again once it has one.
    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(window, &width, &height);
    if (width <= 0 || height <= 0) return;
    vkDeviceWaitIdle(device);
    destroy_swapchain_views();
    const std::uint32_t previous_min_images = swapchain_min_images;
    std::string error;
    if (!create_swapchain(error)) {
        std::cout << "[render] cannot recreate the swapchain: " << error << "\n";
        return;
    }
    if (ui_ready && swapchain_min_images != previous_min_images) ImGui_ImplVulkan_SetMinImageCount(swapchain_min_images);
}

#if defined(__ANDROID__)
// The pass that turns the upright frame into a swapchain image, created the
// first time a sideways display needs it.
bool VulkanRenderer::Impl::create_rotation_pipeline(std::string &error) {
    if (rotate_pipeline != VK_NULL_HANDLE) return true;
    VkAttachmentDescription attachment{};
    attachment.format = swapchain_format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference reference{0u, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1u;
    subpass.pColorAttachments = &reference;
    VkRenderPassCreateInfo pass_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    pass_info.attachmentCount = 1u;
    pass_info.pAttachments = &attachment;
    pass_info.subpassCount = 1u;
    pass_info.pSubpasses = &subpass;
    if (!check(vkCreateRenderPass(device, &pass_info, nullptr, &rotate_render_pass), "vkCreateRenderPass", error))
        return false;

    VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (!check(vkCreateSampler(device, &sampler_info, nullptr, &rotate_sampler), "vkCreateSampler", error))
        return false;

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0u;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1u;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo set_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    set_info.bindingCount = 1u;
    set_info.pBindings = &binding;
    if (!check(vkCreateDescriptorSetLayout(device, &set_info, nullptr, &rotate_set_layout),
               "vkCreateDescriptorSetLayout", error))
        return false;
    constexpr std::uint32_t kMaxImages = 8u;
    VkDescriptorPoolSize pool_size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxImages};
    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = kMaxImages;
    pool_info.poolSizeCount = 1u;
    pool_info.pPoolSizes = &pool_size;
    if (!check(vkCreateDescriptorPool(device, &pool_info, nullptr, &rotate_pool), "vkCreateDescriptorPool", error))
        return false;

    VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT, 0u, sizeof(std::int32_t)};
    VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_info.setLayoutCount = 1u;
    layout_info.pSetLayouts = &rotate_set_layout;
    layout_info.pushConstantRangeCount = 1u;
    layout_info.pPushConstantRanges = &push;
    if (!check(vkCreatePipelineLayout(device, &layout_info, nullptr, &rotate_layout), "vkCreatePipelineLayout", error))
        return false;

    const auto create_shader = [&](const std::uint32_t *code, std::size_t size, VkShaderModule &module) {
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = size;
        info.pCode = code;
        return check(vkCreateShaderModule(device, &info, nullptr, &module), "vkCreateShaderModule", error);
    };
    if (!create_shader(kRotateVertexShader, sizeof(kRotateVertexShader), rotate_vertex)) return false;
    if (!create_shader(kRotateFragmentShader, sizeof(kRotateFragmentShader), rotate_fragment)) return false;
    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = rotate_vertex;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = rotate_fragment;
    stages[1].pName = "main";
    VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = 1u;
    viewport.scissorCount = 1u;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1u;
    blend.pAttachments = &blend_attachment;
    const std::array<VkDynamicState, 2> dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
    dynamic.pDynamicStates = dynamic_states.data();
    VkGraphicsPipelineCreateInfo pipeline_info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_info.stageCount = static_cast<std::uint32_t>(stages.size());
    pipeline_info.pStages = stages.data();
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &assembly;
    pipeline_info.pViewportState = &viewport;
    pipeline_info.pRasterizationState = &raster;
    pipeline_info.pMultisampleState = &multisample;
    pipeline_info.pColorBlendState = &blend;
    pipeline_info.pDynamicState = &dynamic;
    pipeline_info.layout = rotate_layout;
    pipeline_info.renderPass = rotate_render_pass;
    return check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1u, &pipeline_info, nullptr, &rotate_pipeline),
                 "vkCreateGraphicsPipelines", error);
}

bool VulkanRenderer::Impl::create_upright_images(std::string &error) {
    if (!create_rotation_pipeline(error)) return false;
    upright_images.resize(swapchain_images.size());
    for (std::size_t i = 0; i < upright_images.size(); ++i) {
        UprightImage &upright = upright_images[i];
        if (!create_image(swapchain_extent.width, swapchain_extent.height, swapchain_format,
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          upright.image, upright.memory, upright.view, VK_IMAGE_ASPECT_COLOR_BIT, error))
            return false;
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocate.descriptorPool = rotate_pool;
        allocate.descriptorSetCount = 1u;
        allocate.pSetLayouts = &rotate_set_layout;
        if (!check(vkAllocateDescriptorSets(device, &allocate, &upright.set), "vkAllocateDescriptorSets", error))
            return false;
        VkDescriptorImageInfo image_info{rotate_sampler, upright.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = upright.set;
        write.descriptorCount = 1u;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image_info;
        vkUpdateDescriptorSets(device, 1u, &write, 0u, nullptr);
        VkFramebufferCreateInfo framebuffer_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_info.renderPass = rotate_render_pass;
        framebuffer_info.attachmentCount = 1u;
        framebuffer_info.pAttachments = &swapchain_views[i];
        framebuffer_info.width = swapchain_image_extent.width;
        framebuffer_info.height = swapchain_image_extent.height;
        framebuffer_info.layers = 1u;
        if (!check(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &upright.rotate_framebuffer),
                   "vkCreateFramebuffer", error))
            return false;
    }
    std::cout << "[render] pre-rotating a quarter turn x" << swapchain_quarter_turns << " into "
              << swapchain_image_extent.width << "x" << swapchain_image_extent.height << "\n";
    return true;
}

void VulkanRenderer::Impl::destroy_upright_images() {
    for (UprightImage &upright : upright_images) {
        if (upright.rotate_framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(device, upright.rotate_framebuffer, nullptr);
        if (upright.set != VK_NULL_HANDLE) vkFreeDescriptorSets(device, rotate_pool, 1u, &upright.set);
        if (upright.view != VK_NULL_HANDLE) vkDestroyImageView(device, upright.view, nullptr);
        if (upright.image != VK_NULL_HANDLE) vkDestroyImage(device, upright.image, nullptr);
        if (upright.memory != VK_NULL_HANDLE) vkFreeMemory(device, upright.memory, nullptr);
    }
    upright_images.clear();
}

void VulkanRenderer::Impl::destroy_rotation_pipeline() {
    destroy_upright_images();
    if (rotate_pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, rotate_pipeline, nullptr);
    if (rotate_layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, rotate_layout, nullptr);
    if (rotate_pool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, rotate_pool, nullptr);
    if (rotate_set_layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, rotate_set_layout, nullptr);
    if (rotate_sampler != VK_NULL_HANDLE) vkDestroySampler(device, rotate_sampler, nullptr);
    if (rotate_vertex != VK_NULL_HANDLE) vkDestroyShaderModule(device, rotate_vertex, nullptr);
    if (rotate_fragment != VK_NULL_HANDLE) vkDestroyShaderModule(device, rotate_fragment, nullptr);
    if (rotate_render_pass != VK_NULL_HANDLE) vkDestroyRenderPass(device, rotate_render_pass, nullptr);
    rotate_pipeline = VK_NULL_HANDLE;
    rotate_layout = VK_NULL_HANDLE;
    rotate_pool = VK_NULL_HANDLE;
    rotate_set_layout = VK_NULL_HANDLE;
    rotate_sampler = VK_NULL_HANDLE;
    rotate_vertex = VK_NULL_HANDLE;
    rotate_fragment = VK_NULL_HANDLE;
    rotate_render_pass = VK_NULL_HANDLE;
}

// Turns the finished upright frame into the swapchain image, which the pass
// leaves ready to present.
void VulkanRenderer::Impl::record_rotation(VkCommandBuffer commands, std::uint32_t image_index, VkImageLayout layout) {
    const UprightImage &upright = upright_images[image_index];
    transition(commands, upright.image, layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass.renderPass = rotate_render_pass;
    pass.framebuffer = upright.rotate_framebuffer;
    pass.renderArea = {{0, 0}, swapchain_image_extent};
    vkCmdBeginRenderPass(commands, &pass, VK_SUBPASS_CONTENTS_INLINE);
    const VkViewport viewport{0.0f, 0.0f, static_cast<float>(swapchain_image_extent.width),
                              static_cast<float>(swapchain_image_extent.height), 0.0f, 1.0f};
    const VkRect2D scissor{{0, 0}, swapchain_image_extent};
    vkCmdSetViewport(commands, 0u, 1u, &viewport);
    vkCmdSetScissor(commands, 0u, 1u, &scissor);
    vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, rotate_pipeline);
    vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, rotate_layout, 0u, 1u, &upright.set, 0u,
                            nullptr);
    vkCmdPushConstants(commands, rotate_layout, VK_SHADER_STAGE_VERTEX_BIT, 0u, sizeof(std::int32_t),
                       &swapchain_quarter_turns);
    vkCmdDraw(commands, 3u, 1u, 0u, 0u);
    vkCmdEndRenderPass(commands);
}
#endif

bool VulkanRenderer::Impl::create_ui_framebuffers(std::string &error) {
    std::vector<VkImageView> views = swapchain_views;
#if defined(__ANDROID__)
    if (prerotated()) {
        views.clear();
        for (const UprightImage &upright : upright_images) views.push_back(upright.view);
    }
#endif
    for (VkImageView view : views) {
        VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        info.renderPass = ui_render_pass;
        info.attachmentCount = 1u;
        info.pAttachments = &view;
        info.width = swapchain_extent.width;
        info.height = swapchain_extent.height;
        info.layers = 1u;
        VkFramebuffer framebuffer{};
        if (!check(vkCreateFramebuffer(device, &info, nullptr, &framebuffer), "vkCreateFramebuffer", error))
            return false;
        ui_framebuffers.push_back(framebuffer);
    }
    return true;
}

// Scales the game frame onto the swapchain image, which is in TRANSFER_DST
// layout: stretched over the whole window, or at the PSP's aspect ratio with
// black bars. Fill's target already has the window's shape.
void VulkanRenderer::Impl::record_game_blit(VkCommandBuffer commands, VkImage source, VkImage destination) {
    const auto width = static_cast<std::int32_t>(content_rect.extent.width);
    const auto height = static_cast<std::int32_t>(content_rect.extent.height);
    const std::int32_t x0 = content_rect.offset.x;
    const std::int32_t y0 = content_rect.offset.y;
    VkOffset3D low{x0, y0, 0};
    VkOffset3D high{x0 + width, y0 + height, 1};
    const bool partial = width < static_cast<std::int32_t>(swapchain_extent.width) ||
                         height < static_cast<std::int32_t>(swapchain_extent.height);
    if (aspect == settings::Aspect::Original || partial) {
        std::int32_t shown_width = width;
        std::int32_t shown_height = height;
        if (aspect == settings::Aspect::Original) {
            const double scale =
                std::min(static_cast<double>(width) / kPspWidth, static_cast<double>(height) / kPspHeight);
            shown_width = std::clamp(static_cast<std::int32_t>(std::lround(kPspWidth * scale)), 1, width);
            shown_height = std::clamp(static_cast<std::int32_t>(std::lround(kPspHeight * scale)), 1, height);
        }
        low = {x0 + (width - shown_width) / 2, y0 + (height - shown_height) / 2, 0};
        high = {low.x + shown_width, low.y + shown_height, 1};
        if (partial || shown_width < width || shown_height < height) {
            const VkClearColorValue black{{0.0f, 0.0f, 0.0f, 1.0f}};
            const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};
            vkCmdClearColorImage(commands, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &black, 1u, &range);
            // Order the clear before the blit into the same image.
            transition(commands, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        }
    }
    VkImageBlit blit{};
    blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    blit.srcOffsets[1] = {static_cast<std::int32_t>(target_extent.width),
                          static_cast<std::int32_t>(target_extent.height), 1};
    blit.dstSubresource = blit.srcSubresource;
    blit.dstOffsets[0] = low;
    blit.dstOffsets[1] = high;
    vkCmdBlitImage(commands, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &blit, sharp_screen ? VK_FILTER_NEAREST : VK_FILTER_LINEAR);
    perf::count_target_copy();
}

// Finishes the frame being recorded: the game frame (or a plain background
// when `source` is null), the performance overlay on game frames, the
// interface, then submit and present.
void VulkanRenderer::Impl::submit_and_present(VkCommandBuffer commands, VkFence fence, VkImage source,
                                              bool game_frame, bool main_frame) {
#if defined(__ANDROID__)
    check_native_window();
    if (surface_returned) reset_surface();
    // No window to show it in: the frame is recorded and finished, not shown
    // (an image acquired before the window went is still given back).
    if (!surface_lost)
#endif
    if (!acquired_image) {
        check_swapchain();
        if (swapchain_dirty || swapchain == VK_NULL_HANDLE) recreate_swapchain();
    }
    ImDrawData *ui = ui_ready ? ui_draw_data : nullptr;
    ui_draw_data = nullptr;
    const bool draw_ui = ui != nullptr && ui->CmdListsCount > 0;
    // A game flip before anything was drawn has nothing to show.
    const bool has_content = source != VK_NULL_HANDLE || !game_frame || draw_ui;

    // The GPU time covers the frame's own rendering, not the copy to the
    // window, which may wait for the presentation engine to release an image.
    if (main_frame) {
        end_gpu_segment(commands);
        slots[slot].gpu_timer_pending = gpu_timer_used;
    }
    const VkSemaphore acquire = acquire_semaphore(commands);

    std::uint32_t image_index = 0u;
    VkResult acquired = VK_ERROR_OUT_OF_DATE_KHR;
    if (acquired_image) {
        image_index = *acquired_image;
        acquired = VK_SUCCESS;
        acquired_image.reset();
    } else if (has_content && swapchain != VK_NULL_HANDLE
#if defined(__ANDROID__)
               && !surface_lost
#endif
    ) {
        const perf::Clock::time_point acquire_start = perf::Clock::now();
        acquired = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, acquire, VK_NULL_HANDLE, &image_index);
        perf::add_wait_time(perf::Clock::now() - acquire_start, perf::Stall::Acquire);
        if (acquired == VK_ERROR_OUT_OF_DATE_KHR) swapchain_dirty = true;
        if (acquired == VK_SUBOPTIMAL_KHR) swapchain_check = true;
#if defined(__ANDROID__)
        if (acquired == VK_ERROR_SURFACE_LOST_KHR) surface_returned = true;
#endif
    }
    const bool can_present = acquired == VK_SUCCESS || acquired == VK_SUBOPTIMAL_KHR;
    bool capture = false;
    breadcrumb(commands, main_frame ? "the frame's copy to the window" : "a present's copy to the window");
    if (can_present) {
        VkImage target = swapchain_images[image_index];
#if defined(__ANDROID__)
        if (prerotated() && image_index < upright_images.size()) target = upright_images[image_index].image;
#endif
        transition(commands, target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        if (source != VK_NULL_HANDLE) {
            transition(commands, source, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            record_game_blit(commands, source, target);
            transition(commands, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        } else {
            // Behind the setup screens: the dark brown of the project's logo.
            const VkClearColorValue background{{0.075f, 0.055f, 0.045f, 1.0f}};
            const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};
            vkCmdClearColorImage(commands, target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &background, 1u,
                                 &range);
        }
        if (game_frame && overlay_visible) {
            const perf::Clock::time_point overlay_start = perf::Clock::now();
            record_overlay(commands, target);
            perf::add_overlay_time(perf::Clock::now() - overlay_start);
        }
        VkImageLayout layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        if (draw_ui && image_index < ui_framebuffers.size()) {
            transition(commands, target, layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            pass.renderPass = ui_render_pass;
            pass.framebuffer = ui_framebuffers[image_index];
            pass.renderArea = {{0, 0}, swapchain_extent};
            vkCmdBeginRenderPass(commands, &pass, VK_SUBPASS_CONTENTS_INLINE);
            perf::count_render_pass();
            ImGui_ImplVulkan_RenderDrawData(ui, commands);
            vkCmdEndRenderPass(commands);
        }
        if (!capture_path.empty() && (swapchain_usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0u) {
            const VkDeviceSize bytes = static_cast<VkDeviceSize>(swapchain_extent.width) * swapchain_extent.height * 4u;
            VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            buffer_info.size = bytes;
            buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            if (vkCreateBuffer(device, &buffer_info, nullptr, &capture_buffer) == VK_SUCCESS) {
                VkMemoryRequirements requirements{};
                vkGetBufferMemoryRequirements(device, capture_buffer, &requirements);
                VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
                allocate.allocationSize = requirements.size;
                allocate.memoryTypeIndex =
                    find_memory_type(requirements.memoryTypeBits,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                vkAllocateMemory(device, &allocate, nullptr, &capture_memory);
                vkBindBufferMemory(device, capture_buffer, capture_memory, 0u);
                transition(commands, target, layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
                layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                VkBufferImageCopy copy{};
                copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
                copy.imageExtent = {swapchain_extent.width, swapchain_extent.height, 1u};
                vkCmdCopyImageToBuffer(commands, target, layout, capture_buffer, 1u, &copy);
                capture_extent = swapchain_extent;
                capture = true;
            }
        }
#if defined(__ANDROID__)
        if (prerotated() && image_index < upright_images.size()) record_rotation(commands, image_index, layout);
        else
#endif
            transition(commands, target, layout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }
    vkEndCommandBuffer(commands);

    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    std::array<VkCommandBuffer, 2> batch{};
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = frame_batch(commands, batch);
    submit.pCommandBuffers = batch.data();
    if (can_present) {
        submit.waitSemaphoreCount = 1u;
        submit.pWaitSemaphores = &acquire;
        submit.pWaitDstStageMask = &wait_stage;
        submit.signalSemaphoreCount = 1u;
        submit.pSignalSemaphores = &render_finished[image_index];
    }
    // MoltenVK waits for the next drawable here rather than in the acquire.
    const perf::Clock::time_point submit_start = perf::Clock::now();
    if (vkQueueSubmit(queue, 1u, &submit, fence) == VK_ERROR_DEVICE_LOST) device_lost("a frame's submit");
    perf::add_wait_time(perf::Clock::now() - submit_start, perf::Stall::Submit);

    if (can_present) {
        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount = 1u;
        present.pWaitSemaphores = &render_finished[image_index];
        present.swapchainCount = 1u;
        present.pSwapchains = &swapchain;
        present.pImageIndices = &image_index;
        const perf::Clock::time_point present_start = perf::Clock::now();
        const VkResult presented = vkQueuePresentKHR(queue, &present);
        perf::add_wait_time(perf::Clock::now() - present_start, perf::Stall::Present);
        if (presented == VK_ERROR_OUT_OF_DATE_KHR) swapchain_dirty = true;
        if (presented == VK_SUBOPTIMAL_KHR) swapchain_check = true;
#if defined(__ANDROID__)
        if (presented == VK_ERROR_SURFACE_LOST_KHR) surface_returned = true;
#endif
    }
    if (main_frame) recording = false;
    if (capture) write_capture(fence);
}

// Writes the image copied by submit_and_present once its frame has finished.
void VulkanRenderer::Impl::write_capture(VkFence fence) {
    wait_fence(fence, "a window capture");
    void *mapped = nullptr;
    vkMapMemory(device, capture_memory, 0u, VK_WHOLE_SIZE, 0u, &mapped);
    const bool bgra = swapchain_format == VK_FORMAT_B8G8R8A8_UNORM || swapchain_format == VK_FORMAT_B8G8R8A8_SRGB;
    if (write_bmp(capture_path, static_cast<const std::uint8_t *>(mapped), capture_extent.width,
                  capture_extent.height, bgra))
        std::cout << "[render] window " << capture_extent.width << "x" << capture_extent.height << " -> "
                  << capture_path << std::endl;
    else
        std::cout << "[render] cannot write " << capture_path << std::endl;
    vkUnmapMemory(device, capture_memory);
    vkDestroyBuffer(device, capture_buffer, nullptr);
    vkFreeMemory(device, capture_memory, nullptr);
    capture_buffer = VK_NULL_HANDLE;
    capture_memory = VK_NULL_HANDLE;
    capture_path.clear();
}

void VulkanRenderer::Impl::destroy_target(Target &target) {
    for (VkDescriptorSet &descriptor : target.copy_descriptors)
        if (descriptor != VK_NULL_HANDLE) vkFreeDescriptorSets(device, descriptor_pool, 1u, &descriptor);
    vkDestroyImageView(device, target.copy_opaque_view, nullptr);
    vkDestroyImageView(device, target.copy_view, nullptr);
    vkDestroyImage(device, target.copy, nullptr);
    vkFreeMemory(device, target.copy_memory, nullptr);
    vkDestroyFramebuffer(device, target.framebuffer, nullptr);
    vkDestroyImageView(device, target.depth_view, nullptr);
    vkDestroyImage(device, target.depth, nullptr);
    vkFreeMemory(device, target.depth_memory, nullptr);
    vkDestroyImageView(device, target.color_view, nullptr);
    vkDestroyImage(device, target.color, nullptr);
    vkFreeMemory(device, target.color_memory, nullptr);
    target = Target{};
}

// Records commands into a one-time buffer and waits for them to finish.
void VulkanRenderer::Impl::run_commands(const std::function<void(VkCommandBuffer)> &record) {
    VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_info.commandPool = command_pool;
    command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_info.commandBufferCount = 1u;
    VkCommandBuffer commands{};
    vkAllocateCommandBuffers(device, &command_info, &commands);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commands, &begin);
    record(commands);
    vkEndCommandBuffer(commands);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1u;
    submit.pCommandBuffers = &commands;
    vkQueueSubmit(queue, 1u, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, command_pool, 1u, &commands);
}

VulkanRenderer::Impl::Target *VulkanRenderer::Impl::target_for(std::uint32_t address, std::string &error) {
    const auto found = targets.find(address);
    if (found != targets.end()) return &found->second;

    Target target{};
    if (!create_target(target, error)) {
        destroy_target(target);
        return nullptr;
    }
    static const bool trace = std::getenv("MHP3RD_TRACE_FB_TEXTURES") != nullptr;
    if (trace) std::cout << "[fbtex] frame " << frames << " new render target 0x" << std::hex << address << std::dec << "\n";
    return &targets.emplace(address, target).first->second;
}

bool VulkanRenderer::Impl::create_target(Target &target, std::string &error) {
    if (!create_image(target_extent.width, target_extent.height, VK_FORMAT_R8G8B8A8_UNORM,
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                      target.color,
                      target.color_memory, target.color_view, VK_IMAGE_ASPECT_COLOR_BIT, error))
        return false;
    if (!create_image(target_extent.width, target_extent.height, depth_format,
                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, target.depth, target.depth_memory,
                      target.depth_view, depth_aspect(), error))
        return false;
    const std::array<VkImageView, 2> views{target.color_view, target.depth_view};
    VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    info.renderPass = render_pass;
    info.attachmentCount = static_cast<std::uint32_t>(views.size());
    info.pAttachments = views.data();
    info.width = target_extent.width;
    info.height = target_extent.height;
    info.layers = 1u;
    return check(vkCreateFramebuffer(device, &info, nullptr, &target.framebuffer), "vkCreateFramebuffer", error);
}

// Attachments are loaded, not cleared, so a new target starts undefined:
// move it into the layouts the render pass expects once.
void VulkanRenderer::Impl::initialize_layouts(VkCommandBuffer commands, Target &target) {
    if (target.initialized) return;
    transition(commands, target.color, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    transition(commands, target.depth, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
               depth_aspect());
    target.initialized = true;
}

void VulkanRenderer::Impl::destroy_upload() {
    for (FrameSlot &frame : slots) {
        Staging &movie = frame.movie;
        if (movie.mapped != nullptr) vkUnmapMemory(device, movie.memory);
        vkDestroyBuffer(device, movie.buffer, nullptr);
        vkFreeMemory(device, movie.memory, nullptr);
        movie = Staging{};
    }
    vkDestroyImageView(device, upload_view, nullptr);
    vkDestroyImage(device, upload_image, nullptr);
    vkFreeMemory(device, upload_memory, nullptr);
    upload_view = VK_NULL_HANDLE;
    upload_image = VK_NULL_HANDLE;
    upload_memory = VK_NULL_HANDLE;
    upload_extent = {};
}

bool VulkanRenderer::Impl::create_upload(std::uint32_t width, std::uint32_t height, std::string &error) {
    destroy_upload();
    if (!create_image(width, height, VK_FORMAT_R8G8B8A8_UNORM,
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, upload_image, upload_memory,
                      upload_view, VK_IMAGE_ASPECT_COLOR_BIT, error))
        return false;
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(width) * height * 4u;
    VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = bytes;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // A staging buffer per frame slot: the frame before may still copy from its own.
    for (FrameSlot &frame : slots) {
        Staging &movie = frame.movie;
        if (!check(vkCreateBuffer(device, &buffer_info, nullptr, &movie.buffer), "vkCreateBuffer", error)) return false;
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device, movie.buffer, &requirements);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate.allocationSize = requirements.size;
        allocate.memoryTypeIndex = find_memory_type(
            requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (!check(vkAllocateMemory(device, &allocate, nullptr, &movie.memory), "vkAllocateMemory", error))
            return false;
        vkBindBufferMemory(device, movie.buffer, movie.memory, 0u);
        if (!check(vkMapMemory(device, movie.memory, 0u, bytes, 0u, &movie.mapped), "vkMapMemory", error))
            return false;
    }
    upload_extent = {width, height};
    return true;
}

void VulkanRenderer::Impl::begin_pass(std::uint32_t address, std::uint32_t overwritten) {
    std::string error;
    Target *target = target_for(address, error);
    if (target == nullptr) return;
    if (!target->initialized) {
        // Attachments are loaded, not cleared, so a new target starts undefined:
        // move it into the layouts the render pass expects once.
        transition(command_buffer, target->color, VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        transition(command_buffer, target->depth, VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, depth_aspect());
        target->initialized = true;
    }
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass.renderPass = discard_passes[overwritten & 3u] != VK_NULL_HANDLE ? discard_passes[overwritten & 3u] : render_pass;
    pass.framebuffer = target->framebuffer;
    pass.renderArea = {{0, 0}, target_extent};
    if (breadcrumbs.write != nullptr) {
        char label[64];
        std::snprintf(label, sizeof(label), "render pass into 0x%08x%s", address,
                      (overwritten & 3u) != 0u ? " (cleared)" : "");
        breadcrumb(command_buffer, label);
    }
    vkCmdBeginRenderPass(command_buffer, &pass, VK_SUBPASS_CONTENTS_INLINE);
    perf::count_render_pass();
    if ((overwritten & 3u) != 0u) perf::count_cleared_pass();
    forget_bindings();
    current_target = address;
    last_drawn_target = address;
    pass_active = true;
}

void VulkanRenderer::Impl::end_pass() {
    flush_group();
    if (!pass_active) return;
    vkCmdEndRenderPass(command_buffer);
    pass_active = false;
}

VulkanRenderer::Impl::Texture VulkanRenderer::Impl::create_texture(std::uint32_t width, std::uint32_t height,
                                                                   const std::uint32_t *pixels) {
    Texture texture = create_texture_image(width, height);
    if (texture.descriptor != VK_NULL_HANDLE && pixels != nullptr) upload_texture(texture.image, width, height, pixels);
    return texture;
}

VulkanRenderer::Impl::Texture VulkanRenderer::Impl::create_texture_image(std::uint32_t width, std::uint32_t height) {
    Texture texture{};
    std::string error;
    if (!create_image(width, height, VK_FORMAT_R8G8B8A8_UNORM,
                      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, texture.image, texture.memory,
                      texture.view, VK_IMAGE_ASPECT_COLOR_BIT, error))
        return texture;
    VkDescriptorSetAllocateInfo descriptor_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    descriptor_info.descriptorPool = descriptor_pool;
    descriptor_info.descriptorSetCount = 1u;
    descriptor_info.pSetLayouts = &descriptor_layout;
    vkAllocateDescriptorSets(device, &descriptor_info, &texture.descriptor);
    VkDescriptorImageInfo image_info{texture_sampler(), texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = texture.descriptor;
    write.descriptorCount = 1u;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image_info;
    vkUpdateDescriptorSets(device, 1u, &write, 0u, nullptr);
    texture.last_used = ++texture_clock;
    return texture;
}

void VulkanRenderer::Impl::upload_texture(VkImage image, std::uint32_t width, std::uint32_t height,
                                          const std::uint32_t *pixels) {
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(width) * height * 4u;
    VkBuffer ring_buffer{};
    VkDeviceSize ring_offset = 0u;
    if (recording && async_uploads() && stage_upload(pixels, bytes, ring_buffer, ring_offset)) {
        if (!frame_uploads_open) {
            VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(frame_uploads, &begin);
            frame_uploads_open = true;
        }
        transition(frame_uploads, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkBufferImageCopy copy{};
        copy.bufferOffset = ring_offset;
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
        copy.imageExtent = {width, height, 1u};
        vkCmdCopyBufferToImage(frame_uploads, ring_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u,
                               &copy);
        // The barrier's second scope covers the frame's commands, submitted
        // after these in the same batch.
        transition(frame_uploads, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        ++frame_upload_count;
    } else {
    const bool reuse = reuse_buffers();
    VkBuffer staging{};
    VkDeviceMemory staging_memory{};
    void *mapped = nullptr;
    if (reuse && upload_buffer_size >= bytes) {
        staging = upload_buffer;
        mapped = upload_buffer_mapped;
    } else {
        // A kept buffer grows to a power of two of at least 1 MiB, so a few
        // sizes cover every texture.
        VkDeviceSize size = bytes;
        if (reuse) {
            size = VkDeviceSize{1u} << 20u;
            while (size < bytes) size <<= 1u;
        }
        VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        vkCreateBuffer(device, &buffer_info, nullptr, &staging);
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device, staging, &requirements);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate.allocationSize = requirements.size;
        allocate.memoryTypeIndex = find_memory_type(
            requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkAllocateMemory(device, &allocate, nullptr, &staging_memory);
        vkBindBufferMemory(device, staging, staging_memory, 0u);
        vkMapMemory(device, staging_memory, 0u, size, 0u, &mapped);
        if (reuse) {
            // The old buffer's last upload has finished: every upload waits.
            destroy_upload_buffer();
            upload_buffer = staging;
            upload_buffer_memory = staging_memory;
            upload_buffer_mapped = mapped;
            upload_buffer_size = size;
        }
    }
    std::memcpy(mapped, pixels, static_cast<std::size_t>(bytes));
    if (!reuse) vkUnmapMemory(device, staging_memory);

    VkCommandBuffer commands{};
    if (reuse && upload_commands != VK_NULL_HANDLE) {
        commands = upload_commands;
        vkResetCommandBuffer(commands, 0u);
    } else {
        VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        command_info.commandPool = command_pool;
        command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_info.commandBufferCount = 1u;
        vkAllocateCommandBuffers(device, &command_info, &commands);
        if (reuse) upload_commands = commands;
    }
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commands, &begin);
    transition(commands, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    copy.imageExtent = {width, height, 1u};
    vkCmdCopyBufferToImage(commands, staging, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copy);
    transition(commands, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vkEndCommandBuffer(commands);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1u;
    submit.pCommandBuffers = &commands;
    // Uploads are synchronous: the GPU finishes everything queued before this
    // returns, which counts as waiting rather than rendering.
    const perf::Clock::time_point wait_start = perf::Clock::now();
    vkQueueSubmit(queue, 1u, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    perf::add_wait_time(perf::Clock::now() - wait_start, perf::Stall::Upload);
    if (!reuse) {
        vkFreeCommandBuffers(device, command_pool, 1u, &commands);
        vkDestroyBuffer(device, staging, nullptr);
        vkFreeMemory(device, staging_memory, nullptr);
    }
    }
}

bool VulkanRenderer::Impl::stage_upload(const void *pixels, VkDeviceSize bytes, VkBuffer &buffer,
                                        VkDeviceSize &offset) {
    const auto make = [&](VkDeviceSize size, VkBuffer &made, VkDeviceMemory &memory, void *&mapped) {
        VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if (vkCreateBuffer(device, &buffer_info, nullptr, &made) != VK_SUCCESS) return false;
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device, made, &requirements);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate.allocationSize = requirements.size;
        allocate.memoryTypeIndex = find_memory_type(
            requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (vkAllocateMemory(device, &allocate, nullptr, &memory) != VK_SUCCESS ||
            vkBindBufferMemory(device, made, memory, 0u) != VK_SUCCESS ||
            vkMapMemory(device, memory, 0u, size, 0u, &mapped) != VK_SUCCESS) {
            vkDestroyBuffer(device, made, nullptr);
            vkFreeMemory(device, memory, nullptr);
            made = VK_NULL_HANDLE;
            memory = VK_NULL_HANDLE;
            return false;
        }
        return true;
    };
    if (frame_uploads == VK_NULL_HANDLE) return false;
    // 8 MiB covers what a frame uploads when a new area comes into view.
    constexpr VkDeviceSize kRingBytes = 8u * 1024u * 1024u;
    FrameSlot &frame = slots[slot];
    if (frame.ring == VK_NULL_HANDLE) {
        void *mapped = nullptr;
        if (!make(kRingBytes, frame.ring, frame.ring_memory, mapped)) return false;
        frame.ring_mapped = mapped;
        frame.ring_size = kRingBytes;
        upload_ring_used = 0u;
    }
    const VkDeviceSize at = (upload_ring_used + 15u) & ~VkDeviceSize{15u};
    if (at + bytes <= frame.ring_size) {
        std::memcpy(static_cast<std::uint8_t *>(frame.ring_mapped) + at, pixels, static_cast<std::size_t>(bytes));
        upload_ring_used = at + bytes;
        buffer = frame.ring;
        offset = at;
        return true;
    }
    // Past the ring: a buffer of its own, freed with the frame.
    VkBuffer own{};
    VkDeviceMemory own_memory{};
    void *mapped = nullptr;
    if (!make(bytes, own, own_memory, mapped)) return false;
    std::memcpy(mapped, pixels, static_cast<std::size_t>(bytes));
    vkUnmapMemory(device, own_memory);
    frame.retired_buffers.emplace_back(own, own_memory);
    buffer = own;
    offset = 0u;
    return true;
}

void VulkanRenderer::Impl::finish_pending_textures() {
    if (pending_textures.empty()) return;
    const perf::SplitScope split(perf::Split::Texture);
    const perf::Clock::time_point wait_start = perf::Clock::now();
    for (PendingTexture &pending : pending_textures) {
        decode_pool->wait(*pending.job);
        if (!pending.job->ok) pending.job->pixels.assign(static_cast<std::size_t>(pending.width) * pending.height, 0u);
        upload_texture(pending.image, pending.width, pending.height, pending.job->pixels.data());
    }
    pending_textures.clear();
    perf::note_stall(perf::Stall::Decode, perf::Clock::now() - wait_start);
}

std::uint32_t VulkanRenderer::Impl::frame_batch(VkCommandBuffer commands, std::array<VkCommandBuffer, 2> &batch) {
    // Textures still decoding are uploaded ahead of the draws that use them.
    if (commands == command_buffer) finish_pending_textures();
    std::uint32_t count = 0u;
    if (commands == command_buffer && frame_uploads_open) {
        breadcrumb(frame_uploads, "texture uploads (" + std::to_string(frame_upload_count) + ")");
        vkEndCommandBuffer(frame_uploads);
        frame_uploads_open = false;
        batch[count++] = frame_uploads;
    }
    batch[count++] = commands;
    return count;
}

void VulkanRenderer::Impl::release_frame_uploads(std::uint32_t from) {
    FrameSlot &frame = slots[from];
    if (from == slot) {
        upload_ring_used = 0u;
        frame_upload_count = 0u;
    }
    for (const auto &[buffer, memory] : frame.retired_buffers) {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }
    frame.retired_buffers.clear();
    if (frame.retired_textures.empty()) return;
    // A present between flips recorded before the eviction may still draw an
    // evicted texture; its fence says when it has finished.
    for (VkFence fence : present_fences)
        if (fence != VK_NULL_HANDLE && vkGetFenceStatus(device, fence) != VK_SUCCESS) return;
    for (Texture &texture : frame.retired_textures) destroy_texture(texture);
    frame.retired_textures.clear();
}

void VulkanRenderer::Impl::destroy_texture(Texture &texture) {
    if (texture.descriptor != VK_NULL_HANDLE) vkFreeDescriptorSets(device, descriptor_pool, 1u, &texture.descriptor);
    if (texture.view != VK_NULL_HANDLE) vkDestroyImageView(device, texture.view, nullptr);
    if (texture.image != VK_NULL_HANDLE) vkDestroyImage(device, texture.image, nullptr);
    if (texture.memory != VK_NULL_HANDLE) vkFreeMemory(device, texture.memory, nullptr);
    texture = Texture{};
}

namespace {

// The largest V a draw reads from a 512-tall texture, the way texture packs
// track it: through-mode draws give theirs, and anything else counts as the
// whole texture.
std::uint16_t drawn_max_v(const DrawCall &call) {
    if (!call.through) return 512u;
    float max_v = 0.0f;
    for (const Vertex &vertex : call.vertices) max_v = std::max(max_v, vertex.texcoord[1]);
    return static_cast<std::uint16_t>(std::clamp(max_v, 0.0f, 512.0f));
}

// A texture's seen height after a draw that read down to `drawn`.
std::uint16_t update_max_seen_v(std::uint16_t seen, std::uint16_t drawn, bool through) {
    if (!through) return 512u;
    if (seen == 0u) return drawn > 0u ? std::max<std::uint16_t>(272u, drawn) : 0u;
    return drawn > seen ? 512u : seen;
}

} // namespace

VulkanRenderer::Impl::Texture &VulkanRenderer::Impl::texture_for(const GuestMemory &memory, const DrawCall &call) {
    const TextureState &state = call.texture;
    const TextureKeyInput input{state.address,
                                state.buffer_width,
                                static_cast<std::uint32_t>(state.width) << 16u | state.height,
                                static_cast<std::uint32_t>(state.format),
                                state.clut_address,
                                state.clut_format,
                                state.swizzled};
    static const bool no_lookup_env = std::getenv("MHP3RD_NO_LOOKUP_CACHE") != nullptr;
    const bool no_lookup_cache = no_lookup_env || perf::alternate_off(perf::NewPath::Lookup);
    ListTexture *memo = nullptr;
    if (!no_lookup_cache && last_texture != nullptr && input == last_texture_input) {
        memo = last_texture;
    } else {
        auto [entry, inserted] = list_texture_keys.try_emplace(input);
        if (inserted) entry->second.key = texture_key(memory, state);
        memo = &entry->second;
        last_texture_input = input;
        last_texture = memo;
    }
    // A cached texture drawn again. A 512-tall one drawn further down than
    // before is hashed again over the rows now in use, and may find another
    // image in the texture pack.
    const auto use = [&](Texture &texture) -> Texture & {
        texture.last_used = ++texture_clock;
        if (pack && state.height == 512u && texture.max_seen_v < 512u) {
            const std::uint16_t seen = update_max_seen_v(texture.max_seen_v, drawn_max_v(call), call.through);
            if (seen != texture.max_seen_v) {
                texture.max_seen_v = seen;
                texture.replacement = pack->find(memory, state, seen);
            }
        }
        return texture;
    };
    if (!no_lookup_cache && memo->texture != nullptr && memo->erased == textures_erased) return use(*memo->texture);
    const std::uint64_t key = memo->key;
    const auto found = textures.find(key);
    if (found != textures.end()) {
        memo->texture = &found->second;
        memo->erased = textures_erased;
        return use(found->second);
    }
    // Decoded in the background when it can be: copied out of guest memory
    // now, uploaded when the frame is submitted (finish_pending_textures).
    std::shared_ptr<DecodeJob> job;
    if (recording && async_uploads() && background_decode() && !dumper) {
        job = std::make_shared<DecodeJob>();
        if (!snapshot_texture(memory, state, job->snapshot)) job.reset();
    }
    std::vector<std::uint32_t> fresh_pixels;
    std::vector<std::uint32_t> &pixels = reuse_buffers() ? decoded_pixels : fresh_pixels;
    if (!job && (!decode_texture(memory, state, pixels) || pixels.empty())) {
        // MHP3RD_TRACE_WHITE_TEXTURES: each texture that could not be decoded
        // and is drawn white instead, once.
        static const bool trace_white = std::getenv("MHP3RD_TRACE_WHITE_TEXTURES") != nullptr;
        static std::set<std::uint64_t> reported;
        if (trace_white && reported.insert(key).second) {
            std::cerr << "[white-texture] addr=0x" << std::hex << state.address << " buffer_width=" << std::dec
                      << state.buffer_width << " size=" << state.width << "x" << state.height
                      << " format=" << static_cast<int>(state.format) << " swizzled=" << state.swizzled << " clut=0x"
                      << std::hex << state.clut_address << " clut_format=" << state.clut_format << std::dec
                      << " in_memory=" << memory.contains(state.address, 1u) << "\n";
        }
        return white_texture;
    }
    const std::uint16_t max_seen_v =
        state.height == 512u ? update_max_seen_v(0u, drawn_max_v(call), call.through) : 0u;
    if (dumper) {
        static const TexturePackOptions kDumpOptions = [] {
            TexturePackOptions options;
            options.ignore_address = true;
            return options;
        }();
        dumper->dump(memory, state, max_seen_v, pack ? pack->options() : kDumpOptions, pixels.data());
    }

    // MHP3RD_TEXTURE_CACHE_LIMIT keeps fewer textures, to test eviction.
    static const std::size_t cache_limit = [] {
        const char *text = std::getenv("MHP3RD_TEXTURE_CACHE_LIMIT");
        const std::size_t limit = text != nullptr ? std::strtoull(text, nullptr, 10) : 0u;
        return limit != 0u ? std::min(limit, kMaxCachedTextures) : kMaxCachedTextures;
    }();
    if (textures.size() >= cache_limit) {
        auto oldest = textures.begin();
        for (auto it = textures.begin(); it != textures.end(); ++it) {
            if (it->second.last_used < oldest->second.last_used) oldest = it;
        }
        destroyed_texture_clock = std::max(destroyed_texture_clock, oldest->second.last_used);
        if (async_uploads()) {
            // The frame being recorded may have drawn it already.
            slots[slot].retired_textures.push_back(oldest->second);
        } else {
            const perf::Clock::time_point wait_start = perf::Clock::now();
            vkQueueWaitIdle(queue);
            perf::add_wait_time(perf::Clock::now() - wait_start, perf::Stall::Evict);
            destroy_texture(oldest->second);
        }
        textures.erase(oldest);
        ++textures_erased;
    }
    Texture texture = job ? create_texture_image(state.width, state.height)
                          : create_texture(state.width, state.height, pixels.data());
    if (texture.descriptor == VK_NULL_HANDLE) return white_texture;
    if (job) {
        if (!decode_pool) {
            const std::uint32_t cores = std::max(1u, std::thread::hardware_concurrency());
            decode_pool = std::make_unique<DecodePool>(std::clamp(cores / 2u, 1u, 3u));
        }
        decode_pool->submit(job);
        pending_textures.push_back({std::move(job), texture.image, state.width, state.height});
    }
    texture.max_seen_v = max_seen_v;
    // The pack's hash reads the whole texture, so it is taken here, once per
    // upload, and never on the per-draw path above.
    if (pack) texture.replacement = pack->find(memory, state, max_seen_v);
    Texture &cached = textures.emplace(key, std::move(texture)).first->second;
    memo->texture = &cached;
    memo->erased = textures_erased;
    return cached;
}

VkDescriptorSet VulkanRenderer::Impl::texture_descriptor(const GuestMemory &memory, const DrawCall &call) {
    const perf::SplitScope split(perf::Split::Texture);
    Texture &texture = texture_for(memory, call);
    if (texture.replacement && pack) {
        // Until the image is decoded and on the GPU, the original is drawn.
        if (const VkDescriptorSet replaced = replacements.descriptor(*texture.replacement, *pack, frames)) {
            ++replaced_draws;
            return replaced;
        }
    }
    return texture.descriptor;
}

void VulkanRenderer::Impl::apply_texture_pack() {
    const bool wanted = pack_wanted && !pack_held;
    if (pack_applied == wanted && !pack_reload) return;
    pack_applied = wanted;
    pack_reload = false;
    // Every cached texture is dropped, so the next draws decode the originals
    // again and, with the pack on, look them up in it. Off draws exactly what
    // no pack draws.
    vkDeviceWaitIdle(device);
    replacements.clear();
    for (FrameSlot &frame : slots) {
        for (Texture &texture : frame.retired_textures) destroy_texture(texture);
        frame.retired_textures.clear();
    }
    for (auto &[key, texture] : textures) destroy_texture(texture);
    textures.clear();
    destroyed_texture_clock = texture_clock;
    ++textures_erased;
    list_texture_keys.clear();
    last_texture = nullptr;
    pack.reset();
    pack_status.clear();
    if (!wanted) {
        pack_status = pack_held ? "Updating" : "Off";
        return;
    }
    // MHP3RD_TEXTURE_PACK may name the folder, or the player may use a pack
    // where it is; otherwise the pack lives in textures/<disc id>/ in the
    // per-user data directory (texture_pack_import.hpp).
    pack_location = texture_pack_location(VulkanRenderer::textures_root(), install::kDiscId,
                                          settings::current().texture_pack_folder);
    const std::filesystem::path &folder = pack_location.folder;
    std::string error;
    pack = TexturePack::open(folder, install::kDiscId, error);
    if (!pack) {
        std::error_code ec;
        if (std::filesystem::is_directory(folder, ec)) pack_status = "Not loaded: " + error;
        else if (pack_location.source == TexturePackLocation::Source::Installed) pack_status = "Not installed";
        else pack_status = "Folder missing: " + install::path_to_utf8(folder);
        std::cout << "[texpack] " << error << "\n";
        return;
    }
    pack_status = std::to_string(pack->entry_count()) + " textures";
    std::cout << "[texpack] " << pack->entry_count() << " textures from " << folder.string() << "\n";
}

namespace {

std::uint32_t texture_bits_per_pixel(TextureFormat format) {
    switch (format) {
    case TextureFormat::Rgba8888:
    case TextureFormat::Clut32: return 32u;
    case TextureFormat::Clut8: return 8u;
    case TextureFormat::Clut4:
    case TextureFormat::Dxt1: return 4u;
    case TextureFormat::Dxt3:
    case TextureFormat::Dxt5: return 8u;
    default: return 16u;
    }
}

std::uint32_t framebuffer_bytes_per_pixel(std::uint32_t format) { return format == 3u ? 4u : 2u; }

} // namespace

// MHP3RD_TRACE_FB_TEXTURES: every distinct texture whose memory overlaps a
// guest framebuffer the renderer has drawn to, once, with where it is drawn.
void VulkanRenderer::Impl::trace_framebuffer_texture(const DrawCall &call) {
    const TextureState &texture = call.texture;
    const std::uint64_t texture_bytes = static_cast<std::uint64_t>(std::max<std::uint32_t>(
                                            texture.buffer_width, texture.width)) *
                                        texture.height * texture_bits_per_pixel(texture.format) / 8u;
    // Textures filled by a DMA copy out of VRAM, and large textures in
    // general: a screen-sized background the game copied with the CPU shows up
    // as one of these, next to [vram] lines for the reads.
    static std::map<std::array<std::uint32_t, 4>, std::uint32_t> seen_textures;
    const std::array<std::uint32_t, 4> texture_id{texture.address, static_cast<std::uint32_t>(texture.format),
                                                  static_cast<std::uint32_t>(texture.width) << 16u | texture.height,
                                                  texture.buffer_width};
    std::uint32_t copy_source = 0u;
    std::uint32_t copy_destination = 0u;
    const bool copied = find_vram_copy(texture.address, copy_source, copy_destination);
    if ((copied || (texture.width >= 256u && texture.height >= 128u)) && seen_textures.size() < 400u &&
        seen_textures[texture_id]++ == 0u) {
        std::cout << "[fbtex] frame " << frames << " large or copied texture 0x" << std::hex << texture.address
                  << std::dec << " fmt=" << static_cast<int>(texture.format) << " " << texture.width << "x"
                  << texture.height << " bufw=" << texture.buffer_width << " swizzled=" << (texture.swizzled ? 1 : 0)
                  << " drawn to 0x" << std::hex << call.target.color_address << std::dec
                  << (call.through ? " through" : " transform");
        if (copied)
            std::cout << " copied from VRAM 0x" << std::hex << copy_source << " to 0x" << copy_destination << std::dec;
        std::cout << "\n";
    }

    static std::map<std::array<std::uint32_t, 7>, std::uint32_t> seen;
    for (const auto &[address, target] : targets) {
        const std::uint64_t target_bytes =
            static_cast<std::uint64_t>(target.stride) * kPspHeight * framebuffer_bytes_per_pixel(target.format);
        const std::uint64_t start = texture.address;
        if (start + texture_bytes <= address || start >= address + target_bytes) continue;
        const std::array<std::uint32_t, 7> key{texture.address, static_cast<std::uint32_t>(texture.format),
                                               texture.width,   texture.height,
                                               texture.buffer_width, address, call.target.color_address};
        std::uint32_t &count = seen[key];
        if (count++ != 0u || seen.size() > 400u) continue;
        const Vertex &first = call.vertices.front();
        const Vertex &last = call.vertices.back();
        std::cout << "[fbtex] frame " << frames << " texture 0x" << std::hex << texture.address << std::dec
                  << " fmt=" << static_cast<int>(texture.format) << " " << texture.width << "x" << texture.height
                  << " bufw=" << texture.buffer_width << " swizzled=" << (texture.swizzled ? 1 : 0)
                  << " filter=" << texture.min_filter << "/" << texture.mag_filter << " wrap=" << texture.wrap_s
                  << "/" << texture.wrap_t << " in framebuffer 0x" << std::hex << address << std::dec
                  << " (stride=" << target.stride << " fmt=" << target.format
                  << " last drawn frame " << target.last_drawn_frame << ", offset "
                  << (texture.address - address) << ") drawn to 0x" << std::hex << call.target.color_address
                  << std::dec << " fmt=" << call.target.color_format << (call.through ? " through" : " transform")
                  << " prim=" << static_cast<int>(call.primitive) << " verts=" << call.vertices.size()
                  << " pos=(" << first.position[0] << "," << first.position[1] << ")-(" << last.position[0] << ","
                  << last.position[1] << ") uv=(" << first.texcoord[0] << "," << first.texcoord[1] << ")-("
                  << last.texcoord[0] << "," << last.texcoord[1] << ") uvscale=(" << texture.scale_u << ","
                  << texture.scale_v << "," << texture.offset_u << "," << texture.offset_v << ") blend="
                  << (call.blend.enabled ? 1 : 0) << " tfx=" << texture.function << "\n";
    }
}

bool VulkanRenderer::Impl::create_writeback(std::string &error) {
    if (!create_image(kPspWidth, kPspHeight, VK_FORMAT_R8G8B8A8_UNORM,
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, writeback_image,
                      writeback_memory, writeback_view, VK_IMAGE_ASPECT_COLOR_BIT, error))
        return false;
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(kPspWidth) * kPspHeight * 4u;
    VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = bytes;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // A buffer per frame slot; the image is only used on the GPU.
    for (FrameSlot &frame : slots) {
    Staging &writeback = frame.writeback;
    if (!check(vkCreateBuffer(device, &buffer_info, nullptr, &writeback.buffer), "vkCreateBuffer", error))
        return false;
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, writeback.buffer, &requirements);
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = find_memory_type(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    writeback_cached = false;
    writeback_coherent = true;
    static const bool no_cached = std::getenv("MHP3RD_NO_CACHED_READBACK") != nullptr;
    if (!no_cached) {
        // Cached and coherent first, then cached alone.
        VkPhysicalDeviceMemoryProperties memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
        const VkMemoryPropertyFlags cached = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        std::int32_t chosen = -1;
        for (const VkMemoryPropertyFlags wanted : {cached | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, cached}) {
            for (std::uint32_t i = 0; i < memory_properties.memoryTypeCount && chosen < 0; ++i) {
                if ((requirements.memoryTypeBits & (1u << i)) != 0u &&
                    (memory_properties.memoryTypes[i].propertyFlags & wanted) == wanted)
                    chosen = static_cast<std::int32_t>(i);
            }
            if (chosen >= 0) break;
        }
        if (chosen >= 0) {
            allocate.memoryTypeIndex = static_cast<std::uint32_t>(chosen);
            writeback_cached = true;
            writeback_coherent =
                (memory_properties.memoryTypes[chosen].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0u;
        }
    }
    if (&frame == &slots[0])
        std::cout << "[render] frame write-back in memory type " << allocate.memoryTypeIndex
                  << (writeback_cached ? " (host cached" : " (host uncached")
                  << (writeback_coherent ? ", coherent)" : ", not coherent)") << "\n";
    if (!check(vkAllocateMemory(device, &allocate, nullptr, &writeback.memory), "vkAllocateMemory", error))
        return false;
    vkBindBufferMemory(device, writeback.buffer, writeback.memory, 0u);
    if (!check(vkMapMemory(device, writeback.memory, 0u, bytes, 0u, &writeback.mapped), "vkMapMemory", error))
        return false;
    }
    return true;
}

void VulkanRenderer::Impl::destroy_writeback() {
    for (FrameSlot &frame : slots) {
        if (frame.writeback.mapped != nullptr) vkUnmapMemory(device, frame.writeback.memory);
        vkDestroyBuffer(device, frame.writeback.buffer, nullptr);
        vkFreeMemory(device, frame.writeback.memory, nullptr);
        frame.writeback = Staging{};
        frame.writeback_in_flight = false;
    }
    vkDestroyImageView(device, writeback_view, nullptr);
    vkDestroyImage(device, writeback_image, nullptr);
    vkFreeMemory(device, writeback_memory, nullptr);
    writeback_view = VK_NULL_HANDLE;
    writeback_image = VK_NULL_HANDLE;
    writeback_memory = VK_NULL_HANDLE;
    writeback_has_pixels = false;
}

void VulkanRenderer::Impl::collect_writeback(std::uint32_t from, bool wait) {
    FrameSlot &frame = slots[from];
    if (!frame.writeback_in_flight) return;
    if (wait) {
        const perf::Clock::time_point wait_start = perf::Clock::now();
        wait_fence(frame.fence, "the write-back of a frame");
        perf::add_wait_time(perf::Clock::now() - wait_start, perf::Stall::Fence);
    }
    const perf::Clock::time_point copy_start = perf::Clock::now();
    writeback_pixels.resize(static_cast<std::size_t>(kPspWidth) * kPspHeight);
    invalidate_writeback(frame.writeback.memory);
    std::memcpy(writeback_pixels.data(), frame.writeback.mapped, writeback_pixels.size() * 4u);
    writeback_ready = frame.writeback_recorded;
    writeback_has_pixels = true;
    frame.writeback_in_flight = false;
    perf::note_stall(perf::Stall::Copy, perf::Clock::now() - copy_start);
}

// Records the copy of the displayed target that write_back_frame() stores in
// guest memory. Only framebuffers in VRAM that the GE drew are written back;
// a frame the CPU wrote itself is in memory already.
void VulkanRenderer::Impl::record_writeback(std::uint32_t address) {
    const auto found = targets.find(address);
    if (found == targets.end() || (GuestMemory::canonical(address) & 0x1F000000u) != 0x04000000u) return;
    Target &target = found->second;
    if (!target.initialized || target.guest_words.empty() || target.stride < kPspWidth) return;
    if (writeback_image == VK_NULL_HANDLE) {
        std::string error;
        if (!create_writeback(error)) {
            std::cout << "[render] cannot write frames back to guest memory: " << error << "\n";
            destroy_writeback();
            return;
        }
    }
    transition(command_buffer, target.color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    transition(command_buffer, writeback_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageBlit blit{};
    blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    blit.srcOffsets[1] = {static_cast<std::int32_t>(target_extent.width),
                          static_cast<std::int32_t>(target_extent.height), 1};
    blit.dstSubresource = blit.srcSubresource;
    blit.dstOffsets[1] = {static_cast<std::int32_t>(kPspWidth), static_cast<std::int32_t>(kPspHeight), 1};
    vkCmdBlitImage(command_buffer, target.color, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, writeback_image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &blit, VK_FILTER_LINEAR);
    perf::count_target_copy();
    transition(command_buffer, writeback_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    copy.imageExtent = {kPspWidth, kPspHeight, 1u};
    FrameSlot &frame = slots[slot];
    vkCmdCopyImageToBuffer(command_buffer, writeback_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           frame.writeback.buffer, 1u, &copy);
    if (writeback_cached) {
        // Make the copy visible to the host reads after the fence.
        VkBufferMemoryBarrier to_host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
        to_host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        to_host.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_host.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_host.buffer = frame.writeback.buffer;
        to_host.offset = 0u;
        to_host.size = VK_WHOLE_SIZE;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u, 0u,
                             nullptr, 1u, &to_host, 0u, nullptr);
    }
    transition(command_buffer, target.color, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    frame.writeback_recorded = {address, target.stride, target.format};
    frame.writeback_in_flight = true;
}

// Reads one guest word from every 256 bytes of the buffer a target stands for.
void VulkanRenderer::Impl::snapshot_guest_words(const GuestMemory &memory, std::uint32_t address, Target &target) {
    const std::uint32_t bytes = target.stride * kPspHeight * framebuffer_bytes_per_pixel(target.format);
    target.guest_words.clear();
    if (!memory.contains(address, bytes)) return;
    target.guest_words.reserve(bytes / 256u + 1u);
    for (std::uint32_t offset = 0; offset + 4u <= bytes; offset += 256u)
        target.guest_words.push_back(memory.load32(address + offset));
}

// The render target a texture lies in, if the texture reads it the way it was
// drawn: the same row length, a direct colour format of the same pixel size,
// and guest memory under the texture unchanged since the target was last drawn
// to. Anything else (a palette or swizzled view of a framebuffer, or a texture
// the game has since put where a framebuffer was) is decoded from guest memory.
VulkanRenderer::Impl::FramebufferTexture VulkanRenderer::Impl::find_framebuffer_texture(
    const GuestMemory &memory, const TextureState &texture) {
    if (texture.swizzled || static_cast<std::uint32_t>(texture.format) > 3u || texture.width == 0u ||
        texture.height == 0u)
        return {};
    const std::uint32_t texture_address = GuestMemory::canonical(texture.address);
    Target *best = nullptr;
    std::uint32_t best_base = 0u;
    for (auto &[address, target] : targets) {
        if (!target.initialized || target.guest_words.empty()) continue;
        const std::uint32_t bytes_per_pixel = framebuffer_bytes_per_pixel(target.format);
        if (texture_bits_per_pixel(texture.format) != bytes_per_pixel * 8u) continue;
        const std::uint32_t base = GuestMemory::canonical(address);
        const std::uint32_t bytes = target.stride * kPspHeight * bytes_per_pixel;
        if (texture_address < base || texture_address - base >= bytes) continue;
        if (texture.buffer_width != target.stride || (texture_address - base) % bytes_per_pixel != 0u) continue;
        if (best == nullptr || target.draw_serial > best->draw_serial) {
            best = &target;
            best_base = base;
        }
    }
    if (best == nullptr) return {};

    const std::uint32_t bytes_per_pixel = framebuffer_bytes_per_pixel(best->format);
    const std::uint64_t texture_end = static_cast<std::uint64_t>(texture_address) +
                                      static_cast<std::uint64_t>(texture.buffer_width) * texture.height *
                                          bytes_per_pixel;
    for (std::size_t i = 0; i < best->guest_words.size(); ++i) {
        const std::uint32_t word_address = best_base + static_cast<std::uint32_t>(i) * 256u;
        if (word_address < texture_address || word_address >= texture_end) continue;
        if (!memory.contains(word_address, 4u) || memory.load32(word_address) != best->guest_words[i]) return {};
    }
    const std::uint32_t pixel = (texture_address - best_base) / bytes_per_pixel;
    FramebufferTexture found{best, pixel % best->stride, pixel / best->stride};
    if (found.x >= kPspWidth || found.y >= kPspHeight) return {};
    return found;
}

// A sampled copy of the target, brought up to date with its latest draw. The
// copy is recorded between render passes, so the pass in progress ends here.
VkDescriptorSet VulkanRenderer::Impl::framebuffer_descriptor(Target &target, bool opaque) {
    const perf::SplitScope split(perf::Split::Texture);
    if (target.copy == VK_NULL_HANDLE) {
        std::string error;
        if (!create_image(target_extent.width, target_extent.height, VK_FORMAT_R8G8B8A8_UNORM,
                          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, target.copy,
                          target.copy_memory, target.copy_view, VK_IMAGE_ASPECT_COLOR_BIT, error)) {
            std::cout << "[render] cannot sample a render target: " << error << "\n";
            return VK_NULL_HANDLE;
        }
        // A 5650 texture has no alpha: the GE reads it as opaque, whatever the
        // target's alpha channel holds.
        VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = target.copy;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
        view_info.components.a = VK_COMPONENT_SWIZZLE_ONE;
        view_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};
        if (vkCreateImageView(device, &view_info, nullptr, &target.copy_opaque_view) != VK_SUCCESS)
            return VK_NULL_HANDLE;
        for (std::size_t i = 0; i < target.copy_descriptors.size(); ++i) {
            VkDescriptorSetAllocateInfo descriptor_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            descriptor_info.descriptorPool = descriptor_pool;
            descriptor_info.descriptorSetCount = 1u;
            descriptor_info.pSetLayouts = &descriptor_layout;
            if (vkAllocateDescriptorSets(device, &descriptor_info, &target.copy_descriptors[i]) != VK_SUCCESS) {
                target.copy_descriptors[i] = VK_NULL_HANDLE;
                return VK_NULL_HANDLE;
            }
            VkDescriptorImageInfo image_info{framebuffer_sampler(), i == 0u ? target.copy_view : target.copy_opaque_view,
                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = target.copy_descriptors[i];
            write.descriptorCount = 1u;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &image_info;
            vkUpdateDescriptorSets(device, 1u, &write, 0u, nullptr);
        }
        target.copy_valid = false;
    }
    if (!target.copy_valid || target.copy_serial != target.draw_serial) {
        end_pass();
        transition(command_buffer, target.color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        transition(command_buffer, target.copy,
                   target.copy_valid ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkImageCopy region{};
        region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
        region.dstSubresource = region.srcSubresource;
        region.extent = {target_extent.width, target_extent.height, 1u};
        vkCmdCopyImage(command_buffer, target.color, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, target.copy,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &region);
        perf::count_target_copy();
        transition(command_buffer, target.copy, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        transition(command_buffer, target.color, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        target.copy_serial = target.draw_serial;
        target.copy_valid = true;
    }
    return target.copy_descriptors[opaque ? 1u : 0u];
}

void VulkanRenderer::Impl::load_pipeline_cache() {
    if (std::getenv("MHP3RD_NO_PIPELINE_CACHE") != nullptr) {
        std::cout << "[render] pipeline cache off (MHP3RD_NO_PIPELINE_CACHE)\n";
        return;
    }
    pipeline_cache_path = install::user_data_directory() / "pipeline_cache.bin";
    std::vector<char> data;
    {
        std::ifstream in(pipeline_cache_path, std::ios::binary);
        if (in) data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    // The header names the device and driver the data is for; anything else
    // is dropped here rather than trusted to every driver to reject.
    const char *dropped = nullptr;
    if (!data.empty()) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physical_device, &properties);
        std::uint32_t header[4]{};
        if (data.size() < 16u + VK_UUID_SIZE) {
            dropped = "too short";
        } else {
            std::memcpy(header, data.data(), sizeof(header));
            if (header[1] != VK_PIPELINE_CACHE_HEADER_VERSION_ONE || header[0] < 16u + VK_UUID_SIZE ||
                header[2] != properties.vendorID || header[3] != properties.deviceID ||
                std::memcmp(data.data() + 16, properties.pipelineCacheUUID, VK_UUID_SIZE) != 0)
                dropped = "made by another device or driver";
        }
        if (dropped != nullptr) data.clear();
    }
    VkPipelineCacheCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    info.initialDataSize = data.size();
    info.pInitialData = data.empty() ? nullptr : data.data();
    if (vkCreatePipelineCache(device, &info, nullptr, &pipeline_cache) != VK_SUCCESS) {
        info.initialDataSize = 0u;
        info.pInitialData = nullptr;
        if (vkCreatePipelineCache(device, &info, nullptr, &pipeline_cache) != VK_SUCCESS) {
            pipeline_cache = VK_NULL_HANDLE;
            std::cout << "[render] no pipeline cache: vkCreatePipelineCache failed\n";
            return;
        }
        data.clear();
    }
    if (!data.empty())
        std::cout << "[render] pipeline cache: " << (data.size() + 1023u) / 1024u << " KiB from "
                  << install::path_to_utf8(pipeline_cache_path) << "\n";
    else
        std::cout << "[render] pipeline cache: new" << (dropped != nullptr ? std::string(", the old one was ") + dropped : "")
                  << "\n";
}

void VulkanRenderer::Impl::save_pipeline_cache() {
    if (pipeline_cache == VK_NULL_HANDLE || !pipeline_cache_dirty) return;
    pipeline_cache_dirty = false;
    std::size_t size = 0u;
    if (vkGetPipelineCacheData(device, pipeline_cache, &size, nullptr) != VK_SUCCESS || size == 0u) return;
    std::vector<char> data(size);
    if (vkGetPipelineCacheData(device, pipeline_cache, &size, data.data()) != VK_SUCCESS) return;
    data.resize(size);
    // Written next to the file and renamed over it, so a crash or a phone
    // killing the app mid-write leaves the old cache, not half a new one.
    std::filesystem::path partial = pipeline_cache_path;
    partial += ".partial";
    {
        std::ofstream out(partial, std::ios::binary | std::ios::trunc);
        if (!out) return;
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
        if (!out) return;
    }
    std::error_code ec;
    std::filesystem::rename(partial, pipeline_cache_path, ec);
    if (ec) std::filesystem::remove(partial, ec);
}

namespace {

constexpr std::uint32_t kPipelineKeysMagic = 0x4B504B59u;  // "YKPK"
constexpr std::uint32_t kPipelineKeysVersion = 1u;
constexpr std::uint32_t kPipelineKeyWords = 12u;
constexpr std::uint32_t kMaxPipelineKeys = 4096u;

std::array<std::uint32_t, kPipelineKeyWords> key_words(const PipelineKey &key) {
    return {key.blend ? 1u : 0u, key.source_factor,  key.destination_factor, key.equation,
            key.depth_test ? 1u : 0u, key.depth_write ? 1u : 0u, key.depth_function, key.cull ? 1u : 0u,
            key.cull_clockwise ? 1u : 0u, key.color_mask, key.alpha_test ? 1u : 0u, key.raw ? 1u : 0u};
}

PipelineKey key_from(const std::array<std::uint32_t, kPipelineKeyWords> &words) {
    PipelineKey key{};
    key.blend = words[0] != 0u;
    key.source_factor = words[1];
    key.destination_factor = words[2];
    key.equation = words[3];
    key.depth_test = words[4] != 0u;
    key.depth_write = words[5] != 0u;
    key.depth_function = words[6];
    key.cull = words[7] != 0u;
    key.cull_clockwise = words[8] != 0u;
    key.color_mask = words[9] & 0xFu;
    key.alpha_test = words[10] != 0u;
    key.raw = words[11] != 0u;
    return key;
}

} // namespace

void VulkanRenderer::Impl::save_pipeline_keys() {
    if (!pipeline_keys_dirty || pipeline_keys_path.empty()) return;
    pipeline_keys_dirty = false;
    // The pipelines made this run and those the prewarm made unasked.
    std::vector<std::uint32_t> data{kPipelineKeysMagic, kPipelineKeysVersion, kPipelineKeyWords, 0u};
    std::uint32_t count = 0u;
    const auto add = [&](const PipelineKey &key) {
        if (count >= kMaxPipelineKeys) return;
        const auto words = key_words(key);
        data.insert(data.end(), words.begin(), words.end());
        ++count;
    };
    for (const auto &[key, pipeline] : pipelines) add(key);
    {
        std::lock_guard<std::mutex> guard(prewarm->lock);
        for (const auto &[key, pipeline] : prewarm->made) add(key);
    }
    data[3] = count;
    std::filesystem::path partial = pipeline_keys_path;
    partial += ".partial";
    {
        std::ofstream out(partial, std::ios::binary | std::ios::trunc);
        if (!out) return;
        out.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size() * 4u));
        if (!out) return;
    }
    std::error_code ec;
    std::filesystem::rename(partial, pipeline_keys_path, ec);
    if (ec) std::filesystem::remove(partial, ec);
}

void VulkanRenderer::Impl::prewarm_pipelines() {
    if (pipeline_cache == VK_NULL_HANDLE || std::getenv("MHP3RD_NO_PIPELINE_PREWARM") != nullptr) return;
    pipeline_keys_path = pipeline_cache_path.parent_path() / "pipeline_keys.bin";
    std::vector<std::uint32_t> data;
    {
        std::ifstream in(pipeline_keys_path, std::ios::binary);
        if (!in) return;
        in.seekg(0, std::ios::end);
        const std::streamoff size = in.tellg();
        if (size < 16 || size % 4 != 0 || size > 16 + static_cast<std::streamoff>(kMaxPipelineKeys) * 48) return;
        in.seekg(0);
        data.resize(static_cast<std::size_t>(size / 4));
        in.read(reinterpret_cast<char *>(data.data()), size);
        if (!in) return;
    }
    if (data[0] != kPipelineKeysMagic || data[1] != kPipelineKeysVersion || data[2] != kPipelineKeyWords ||
        data.size() != 4u + static_cast<std::size_t>(data[3]) * kPipelineKeyWords)
        return;
    std::vector<PipelineKey> keys;
    for (std::uint32_t i = 0; i < data[3]; ++i) {
        std::array<std::uint32_t, kPipelineKeyWords> words{};
        std::copy_n(data.begin() + 4 + static_cast<std::ptrdiff_t>(i) * kPipelineKeyWords, kPipelineKeyWords,
                    words.begin());
        keys.push_back(key_from(words));
    }
    prewarm->thread = std::thread([this, keys = std::move(keys)] {
        const auto start = std::chrono::steady_clock::now();
        std::uint32_t made = 0u;
        for (const PipelineKey &key : keys) {
            if (prewarm->stop.load(std::memory_order_relaxed)) break;
            {
                std::lock_guard<std::mutex> guard(prewarm->lock);
                if (prewarm->made.contains(key)) continue;
            }
            const VkPipeline pipeline = create_pipeline(key);
            if (pipeline == VK_NULL_HANDLE) continue;
            std::lock_guard<std::mutex> guard(prewarm->lock);
            if (!prewarm->made.emplace(key, pipeline).second) vkDestroyPipeline(device, pipeline, nullptr);
            ++made;
        }
        std::cout << "[render] " << made << " pipelines of earlier runs made in the background in " << std::fixed
                  << std::setprecision(1)
                  << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()
                  << std::defaultfloat << std::setprecision(6) << " ms\n"
                  << std::flush;
    });
}

void VulkanRenderer::Impl::report_pipelines(bool final) {
    if (pipelines_new == 0u && !pipeline_cache_dirty) return;
    // A burst of new pipelines (a new area, an effect seen for the first
    // time) is reported, and the cache written, once it has settled.
    if (!final && std::chrono::steady_clock::now() - pipeline_cache_changed < std::chrono::seconds(3)) return;
    if (pipelines_new != 0u) {
        pipelines_reported += pipelines_new;
        std::cout << "[render] " << pipelines_new << " new pipeline" << (pipelines_new == 1u ? "" : "s") << " in "
                  << std::fixed << std::setprecision(1) << pipeline_new_ms << std::defaultfloat
                  << std::setprecision(6) << " ms, " << pipelines_reported << " so far"
                  << (pipeline_cache != VK_NULL_HANDLE ? "; pipeline cache saved" : "") << "\n"
                  << std::flush;
        pipelines_new = 0u;
        pipeline_new_ms = 0.0;
    }
    save_pipeline_cache();
    save_pipeline_keys();
}

VkPipeline VulkanRenderer::Impl::pipeline_for(const PipelineKey &key) {
    static const bool no_lookup_env = std::getenv("MHP3RD_NO_LOOKUP_CACHE") != nullptr;
    const bool no_lookup_cache = no_lookup_env || perf::alternate_off(perf::NewPath::Lookup);
    if (!no_lookup_cache && last_pipeline != VK_NULL_HANDLE && key == last_pipeline_key) return last_pipeline;
    const auto found = pipelines.find(key);
    if (found != pipelines.end()) {
        last_pipeline_key = key;
        last_pipeline = found->second;
        return found->second;
    }
    // Made in the background from the last run's list (prewarm_pipelines).
    if (prewarm->thread.joinable()) {
        std::lock_guard<std::mutex> guard(prewarm->lock);
        if (const auto ready = prewarm->made.find(key); ready != prewarm->made.end()) {
            const VkPipeline pipeline = ready->second;
            prewarm->made.erase(ready);
            pipelines.emplace(key, pipeline);
            last_pipeline_key = key;
            last_pipeline = pipeline;
            ++pipelines_prewarm_used;
            return pipeline;
        }
    }
    const perf::Clock::time_point create_start = perf::Clock::now();
    VkPipeline pipeline = create_pipeline(key);
    const perf::Clock::duration create_time = perf::Clock::now() - create_start;
    perf::note_stall(perf::Stall::Pipeline, create_time);
    ++pipelines_new;
    pipeline_new_ms += std::chrono::duration<double, std::milli>(create_time).count();
    if (pipeline_cache != VK_NULL_HANDLE) {
        pipeline_cache_dirty = true;
        pipeline_cache_changed = std::chrono::steady_clock::now();
    }
    if (pipeline == VK_NULL_HANDLE) return VK_NULL_HANDLE;
    pipelines.emplace(key, pipeline);
    pipeline_keys_dirty = true;
    last_pipeline_key = key;
    last_pipeline = pipeline;
    return pipeline;
}

// The pipeline for `key`; touches nothing but immutable state and the
// (internally synchronised) pipeline cache, so the prewarm thread uses it too.
VkPipeline VulkanRenderer::Impl::create_pipeline(const PipelineKey &key) const {
    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = key.raw ? raw_vertex_shader : vertex_shader;
    stages[0].pName = "main";
    stages[1] = stages[0];
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragment_shader;

    VkVertexInputBindingDescription binding{0u, sizeof(GpuVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    std::array<VkVertexInputAttributeDescription, 4> attributes{
        VkVertexInputAttributeDescription{0u, 0u, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(GpuVertex, x)},
        VkVertexInputAttributeDescription{1u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof(GpuVertex, u)},
        VkVertexInputAttributeDescription{2u, 0u, VK_FORMAT_R8G8B8A8_UNORM, offsetof(GpuVertex, color)},
        VkVertexInputAttributeDescription{3u, 0u, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GpuVertex, nx)},
    };
    VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    // The raw vertex shader reads the vertex buffer itself.
    if (!key.raw) {
        vertex_input.vertexBindingDescriptionCount = 1u;
        vertex_input.pVertexBindingDescriptions = &binding;
        vertex_input.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
        vertex_input.pVertexAttributeDescriptions = attributes.data();
    }

    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // The GE viewport and scissor change per draw, so they are dynamic state.
    VkPipelineViewportStateCreateInfo viewport_state{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state.viewportCount = 1u;
    viewport_state.scissorCount = 1u;
    const std::array<VkDynamicState, 3> dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
                                                       VK_DYNAMIC_STATE_BLEND_CONSTANTS};
    VkPipelineDynamicStateCreateInfo dynamic_state{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = key.cull ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    raster.frontFace = key.cull_clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth.depthTestEnable = key.depth_test ? VK_TRUE : VK_FALSE;
    depth.depthWriteEnable = key.depth_write ? VK_TRUE : VK_FALSE;
    depth.depthCompareOp = to_compare_op(key.depth_function);

    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.blendEnable = key.blend ? VK_TRUE : VK_FALSE;
    blend_attachment.srcColorBlendFactor = to_blend_factor(key.source_factor, true);
    blend_attachment.dstColorBlendFactor = to_blend_factor(key.destination_factor, false);
    blend_attachment.colorBlendOp = to_blend_op(key.equation);
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.colorWriteMask = key.color_mask;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1u;
    blend.pAttachments = &blend_attachment;

    // kAlphaTest in ge.frag.
    const VkBool32 alpha_test = key.alpha_test ? VK_TRUE : VK_FALSE;
    const VkSpecializationMapEntry alpha_entry{0u, 0u, sizeof(VkBool32)};
    VkSpecializationInfo specialization{};
    specialization.mapEntryCount = 1u;
    specialization.pMapEntries = &alpha_entry;
    specialization.dataSize = sizeof(alpha_test);
    specialization.pData = &alpha_test;
    stages[1].pSpecializationInfo = &specialization;

    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.stageCount = static_cast<std::uint32_t>(stages.size());
    info.pStages = stages.data();
    info.pVertexInputState = &vertex_input;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport_state;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = &depth;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic_state;
    info.layout = pipeline_layout;
    info.renderPass = render_pass;
    VkPipeline pipeline{};
    if (vkCreateGraphicsPipelines(device, pipeline_cache, 1u, &info, nullptr, &pipeline) != VK_SUCCESS)
        return VK_NULL_HANDLE;
    return pipeline;
}

bool VulkanRenderer::pump_events() {
    if (!impl_ || impl_->window == nullptr) return false;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Closing the window is the only way out. Esc used to quit as well, but
        // Steam's desktop controller layout on a Steam Deck sends Esc from the B
        // button, which is also the game's confirm button, so confirming a menu
        // closed the game. Esc is reserved for the in-game menu instead.
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) impl_->quit = true;
        // Hot-plug is handled before the text-input branch below, which skips
        // every other event while the on-screen keyboard is up.
        if (event.type == SDL_EVENT_GAMEPAD_ADDED) impl_->open_gamepad(event.gdevice.which);
        if (event.type == SDL_EVENT_GAMEPAD_REMOVED) impl_->close_gamepad(event.gdevice.which);
        // F3 toggles the performance overlay. No pad combination: L3+R3 is
        // reserved for the in-game menu.
        if (event.type == SDL_EVENT_WINDOW_DISPLAY_CHANGED) impl_->update_display_info();
        if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) impl_->swapchain_dirty = true;
        // A phone turned from one landscape to the other keeps its size; only
        // the display's transform changes.
        if (event.type == SDL_EVENT_DISPLAY_ORIENTATION) impl_->swapchain_check = true;
        if (event.type == SDL_EVENT_WINDOW_SAFE_AREA_CHANGED) {
            impl_->update_content_rect();
            impl_->resize_now = true;
        }
        // The mouse, while captured for the game. Releases always count.
        // Touches also arrive as mouse events; they are the touch controls'
        // alone and never reach the game as a mouse.
        if ((event.type == SDL_EVENT_MOUSE_MOTION && event.motion.which != SDL_TOUCH_MOUSEID) ||
            ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) &&
             event.button.which != SDL_TOUCH_MOUSEID))
            impl_->real_mouse_seen = true;
        impl_->handle_touch(event);
        if (event.type == SDL_EVENT_MOUSE_MOTION && impl_->mouse_captured && event.motion.which != SDL_TOUCH_MOUSEID &&
            (!impl_->scripted_input || event.motion.which == kScriptedMouse)) {
            impl_->mouse_motion.x += event.motion.xrel;
            impl_->mouse_motion.y += event.motion.yrel;
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && impl_->mouse_captured && event.button.button < 32u &&
            event.button.which != SDL_TOUCH_MOUSEID &&
            (!impl_->scripted_input || event.button.which == kScriptedMouse))
            impl_->mouse_buttons |= 1u << event.button.button;
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button < 32u)
            impl_->mouse_buttons &= ~(1u << event.button.button);
        if (impl_->event_hook && impl_->event_hook(event)) continue;
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F3 && !event.key.repeat && impl_->overlay_ready)
            impl_->overlay_visible = !impl_->overlay_visible;
    }
    // Only sample while the window has focus: a key still down when focus is
    // lost stays down in SDL's snapshot, which the guest sees as a held
    // direction it can never release.
    const bool focused = (SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_INPUT_FOCUS) != 0u;
    impl_->update_pointer(focused);
    impl_->sample_pad(focused);
    return !impl_->quit;
}

void VulkanRenderer::sample_pad() {
    if (!impl_ || impl_->window == nullptr) return;
    // New input from the devices, without handling window events: those,
    // and the interface's, wait for the next pump_events(). The keyboard's
    // and the gamepads' state follow what was pumped.
    SDL_PumpEvents();
    impl_->sample_pad((SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_INPUT_FOCUS) != 0u);
}

void VulkanRenderer::Impl::sample_pad(bool focused) {
    Impl *const impl_ = this;
    // While a menu or the on-screen keyboard is open, nothing reaches the game.
    if (!impl_->game_input) {
        impl_->pad = PadState{};
        return;
    }

    // Keyboard and mouse buttons to the PSP pad, through the player's
    // bindings. Bits follow SceCtrlButtons; the stick is centred at 0x80.
    const settings::Settings &player = settings::current();
    const bool *keys = SDL_GetKeyboardState(nullptr);
    const input::PadInput typed = input::read(player.bindings, [&](input::Binding binding) {
        if (const int button = input::mouse_button_of(binding))
            return impl_->mouse_captured && (impl_->mouse_buttons & (1u << button)) != 0u;
        const int position = input::key_position(binding);
        return position >= 0 && ((focused && position < SDL_SCANCODE_COUNT && keys[position]) ||
                                 impl_->scripted_keys[static_cast<std::size_t>(position)]);
    });
    PadState pad{};
    pad.buttons = typed.buttons;
    int analog_x = typed.stick_x;
    int analog_y = typed.stick_y;
    // Camera keys push the second stick fully, as a right stick would: in
    // the D-pad mode they press the D-pad instead, and with it off, nothing.
    if (player.right_stick == settings::RightStick::Camera) {
        pad.right_x = static_cast<std::uint8_t>(0x80 + typed.camera_x);
        pad.right_y = static_cast<std::uint8_t>(0x80 + typed.camera_y);
    } else if (player.right_stick == settings::RightStick::DPad) {
        if (typed.camera_x < 0) pad.buttons |= 0x0080u;
        if (typed.camera_x > 0) pad.buttons |= 0x0020u;
        if (typed.camera_y < 0) pad.buttons |= 0x0010u;
        if (typed.camera_y > 0) pad.buttons |= 0x0040u;
    }

    // The gamepad adds to the same bits and offsets, so both sources are live.
    if (impl_->gamepad != nullptr) read_gamepad(impl_->gamepad, pad, analog_x, analog_y);
    // So do the on-screen controls.
    if (impl_->touch_visible) {
        pad.buttons |= impl_->touch.buttons();
        const input::touch::Point stick = impl_->touch.stick();
        analog_x += static_cast<int>(std::lround(stick.x * 127.0f));
        analog_y += static_cast<int>(std::lround(stick.y * 127.0f));
    }

    pad.analog_x = static_cast<std::uint8_t>(std::clamp(0x80 + analog_x, 0, 255));
    pad.analog_y = static_cast<std::uint8_t>(std::clamp(0x80 + analog_y, 0, 255));

    // Unattended runs (overlay bootstrapping) press confirm periodically so the
    // game walks through title screens and dialogs on its own.
    static const std::uint64_t auto_confirm = [] {
        const char *text = std::getenv("MHP3RD_AUTO_CONFIRM");
        return text != nullptr ? std::strtoull(text, nullptr, 10) : 0ull;
    }();
    if (auto_confirm != 0u) {
        const std::uint64_t phase = impl_->frames % auto_confirm;
        if (phase < auto_confirm / 8u) pad.buttons |= 0x2000u;  // circle
    }
    // Buttons held when the game got its input back stay hidden from it until
    // they are released.
    if (impl_->suppress_held) {
        impl_->suppressed_buttons = pad.buttons;
        impl_->suppress_held = false;
    }
    impl_->suppressed_buttons &= pad.buttons;
    pad.buttons &= ~impl_->suppressed_buttons;
    if (pad_tuning().trace && (pad.buttons != impl_->pad.buttons || pad.analog_x != impl_->pad.analog_x ||
                               pad.analog_y != impl_->pad.analog_y || pad.right_x != impl_->pad.right_x ||
                               pad.right_y != impl_->pad.right_y)) {
        std::cout << "[pad] buttons 0x" << std::hex << pad.buttons << std::dec << " analog "
                  << static_cast<int>(pad.analog_x) << "," << static_cast<int>(pad.analog_y) << " right "
                  << static_cast<int>(pad.right_x) << "," << static_cast<int>(pad.right_y) << std::endl;
    }
    impl_->pad = pad;
}

PadState VulkanRenderer::pad() const noexcept { return impl_ ? impl_->pad : PadState{}; }

bool VulkanRenderer::touch_controls_visible() const noexcept {
    return impl_ && impl_->touch_visible && impl_->game_input && settings::current().touch_controls;
}

const input::touch::Controls &VulkanRenderer::touch_controls() const {
    impl_->update_touch_layout();
    return impl_->touch;
}

MouseMotion VulkanRenderer::take_touch_motion() noexcept {
    return impl_ ? std::exchange(impl_->touch_motion, MouseMotion{}) : MouseMotion{};
}

bool VulkanRenderer::take_touch_menu() noexcept { return impl_ && impl_->touch.take_menu(); }

MouseMotion VulkanRenderer::take_mouse_motion() noexcept {
    return impl_ ? std::exchange(impl_->mouse_motion, MouseMotion{}) : MouseMotion{};
}

bool VulkanRenderer::mouse_captured() const noexcept { return impl_ && impl_->mouse_captured; }

void VulkanRenderer::set_pointer_free(bool free) {
    if (impl_) impl_->pointer_free = free;
}

void VulkanRenderer::set_scripted_key(int position, bool down) {
    if (impl_ && position > 0 && position < static_cast<int>(input::kKeyPositions))
        impl_->scripted_keys[static_cast<std::size_t>(position)] = down;
}

void VulkanRenderer::set_scripted_input(bool scripted) {
    if (impl_) impl_->scripted_input = scripted;
}

void VulkanRenderer::set_event_hook(std::function<bool(const SDL_Event &)> hook) {
    if (impl_) impl_->event_hook = std::move(hook);
}

void VulkanRenderer::set_game_input(bool enabled) {
    if (!impl_) return;
    if (enabled && !impl_->game_input) impl_->suppress_held = true;
    if (!enabled) impl_->touch.release_all();
    impl_->game_input = enabled;
}

void VulkanRenderer::request_quit() noexcept {
    if (impl_) impl_->quit = true;
}

void VulkanRenderer::hold_frame(bool hold) {
    if (!impl_) return;
    Impl &impl = *impl_;
    if (!impl.ready || hold == impl.holding) return;
    if (!hold) {
        impl.holding = false;
        vkDeviceWaitIdle(impl.device);
        impl.destroy_target(impl.held);
        impl.held = {};
        return;
    }
    const auto shown = impl.targets.find(impl.presented_target);
    if (shown == impl.targets.end() || !shown->second.initialized) return;
    std::string error;
    if (!impl.create_image(impl.target_extent.width, impl.target_extent.height, VK_FORMAT_R8G8B8A8_UNORM,
                           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                               VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                           impl.held.color, impl.held.color_memory, impl.held.color_view, VK_IMAGE_ASPECT_COLOR_BIT,
                           error)) {
        std::cerr << "Renderer: cannot hold the frame (" << error << ")\n";
        impl.destroy_target(impl.held);
        impl.held = {};
        return;
    }
    // The shown target and the copy both rest in the layout presenting
    // expects of a game frame.
    const VkImage source = shown->second.color;
    const VkImage copy_to = impl.held.color;
    const VkExtent2D extent = impl.target_extent;
    impl.run_commands([&](VkCommandBuffer commands) {
        impl.transition(commands, source, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        impl.transition(commands, copy_to, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkImageCopy region{};
        region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
        region.dstSubresource = region.srcSubresource;
        region.extent = {extent.width, extent.height, 1u};
        vkCmdCopyImage(commands, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, copy_to,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &region);
        impl.transition(commands, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        impl.transition(commands, copy_to, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    });
    impl.holding = true;
}

SDL_Window *VulkanRenderer::window() const noexcept { return impl_ ? impl_->window : nullptr; }
std::string VulkanRenderer::device_name() const { return impl_ ? impl_->device_name : std::string{}; }
SDL_Gamepad *VulkanRenderer::gamepad() const noexcept { return impl_ ? impl_->gamepad : nullptr; }

VkExtent2D VulkanRenderer::Impl::wanted_target_extent() const {
    const bool known = content_rect.extent.width != 0u && content_rect.extent.height != 0u;
    const double window_width = known ? content_rect.extent.width : static_cast<double>(target_extent.width);
    const double window_height = known ? content_rect.extent.height : static_cast<double>(target_extent.height);
    if (aspect != settings::Aspect::Fill) {
        std::uint32_t scale = requested_scale;
        if (scale == 0u) {
            // The smallest multiple that covers the picture on the window.
            const double x = window_width / kPspWidth;
            const double y = window_height / kPspHeight;
            const double cover = aspect == settings::Aspect::Original ? std::min(x, y) : std::max(x, y);
            scale = static_cast<std::uint32_t>(std::clamp(std::ceil(cover - 0.01), 1.0,
                                                          static_cast<double>(kMaxAutoLines / kPspHeight)));
        }
        return {kPspWidth * scale, kPspHeight * scale};
    }
    // Fill: the window's shape, 272 lines per step or the window's own lines.
    const double shape = std::clamp(window_width / std::max(window_height, 1.0), 0.25, 8.0);
    double lines = requested_scale != 0u ? static_cast<double>(kPspHeight * requested_scale)
                                         : std::clamp(window_height, static_cast<double>(kPspHeight),
                                                      static_cast<double>(kMaxAutoLines));
    lines = std::min(lines, kMaxTargetWidth / shape);
    const auto height = static_cast<std::uint32_t>(std::max(1.0, std::round(lines)));
    const auto width = static_cast<std::uint32_t>(
        std::clamp(std::round(lines * shape), 1.0, static_cast<double>(kMaxTargetWidth)));
    return {width, height};
}

bool VulkanRenderer::Impl::interface_fit(float &fit_x, float &fit_y) const {
    if (aspect != settings::Aspect::Fill) return false;
    const float x = static_cast<float>(target_extent.width) / static_cast<float>(kPspWidth);
    const float y = static_cast<float>(target_extent.height) / static_cast<float>(kPspHeight);
    const float square = std::min(x, y);
    fit_x = square / x;
    fit_y = square / y;
    return fit_x < 0.999f || fit_y < 0.999f;
}

void VulkanRenderer::Impl::follow_window() {
    if (!ready || recording) return;
    const VkExtent2D wanted = wanted_target_extent();
    if (wanted.width == target_extent.width && wanted.height == target_extent.height) {
        pending_frames = 0u;
        resize_now = false;
        return;
    }
    // The keyboard's held frame has the old size; a new window size waits
    // for it, a changed setting does not.
    if (holding && !resize_now) return;
    if (!resize_now) {
        if (wanted.width != pending_extent.width || wanted.height != pending_extent.height) {
            pending_extent = wanted;
            pending_frames = 0u;
        }
        if (++pending_frames < kSettleFrames) return;
    }
    pending_frames = 0u;
    resize_now = false;
    resize_targets(wanted);
}

void VulkanRenderer::Impl::resize_targets(VkExtent2D extent) {
    if (!ready || recording ||
        (extent.width == target_extent.width && extent.height == target_extent.height))
        return;
    // Every target is rebuilt at the new size with its current picture scaled
    // into it, so the paused frame behind the menu, and render-to-texture
    // targets the game reads back, stay intact.
    vkDeviceWaitIdle(device);
    // A held frame has the old size; let the game's own frames show again.
    if (holding) {
        holding = false;
        destroy_target(held);
        held = {};
    }
    std::map<std::uint32_t, Target> old_targets = std::move(targets);
    targets.clear();
    const VkExtent2D old_extent = target_extent;
    target_extent = extent;
    std::vector<std::pair<Target *, const Target *>> copies;
    for (const auto &[address, old_target] : old_targets) {
        std::string error;
        Target *target = target_for(address, error);
        if (target == nullptr) {
            std::cout << "[render] cannot resize a render target: " << error << "\n";
            continue;
        }
        target->stride = old_target.stride;
        target->format = old_target.format;
        target->last_drawn_frame = old_target.last_drawn_frame;
        target->draw_serial = old_target.draw_serial;
        target->guest_words = old_target.guest_words;
        if (old_target.initialized) copies.emplace_back(target, &old_target);
    }
    run_commands([&](VkCommandBuffer commands) {
        for (const auto &[target, old_target] : copies) {
            transition(commands, old_target->color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            transition(commands, target->color, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            VkImageBlit blit{};
            blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
            blit.srcOffsets[1] = {static_cast<std::int32_t>(old_extent.width),
                                  static_cast<std::int32_t>(old_extent.height), 1};
            blit.dstSubresource = blit.srcSubresource;
            blit.dstOffsets[1] = {static_cast<std::int32_t>(extent.width), static_cast<std::int32_t>(extent.height), 1};
            vkCmdBlitImage(commands, old_target->color, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, target->color,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &blit, VK_FILTER_LINEAR);
            transition(commands, target->color, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            transition(commands, target->depth, VK_IMAGE_LAYOUT_UNDEFINED,
                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, depth_aspect());
            target->initialized = true;
        }
    });
    for (auto &[address, old_target] : old_targets) destroy_target(old_target);
    // Recorded frames name the old targets and hold viewports of the old size.
    reset_interpolation();
    destroy_interpolation_targets();
    std::cout << "[render] internal resolution " << extent.width << "x" << extent.height << "\n";
}

void VulkanRenderer::set_internal_scale(std::uint32_t scale) {
    if (!impl_) return;
    impl_->requested_scale = std::min(scale, settings::kMaxInternalScale);
    impl_->resize_now = true;
    impl_->follow_window();
}

void VulkanRenderer::set_window_scale(std::uint32_t scale) {
    if (!impl_ || impl_->window == nullptr) return;
    scale = std::clamp<std::uint32_t>(scale, 1u, settings::kMaxWindowScale);
    if ((SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_FULLSCREEN) != 0u) return;
    SDL_SetWindowSize(impl_->window, static_cast<int>(kPspWidth * scale), static_cast<int>(kPspHeight * scale));
    impl_->swapchain_dirty = true;
}

void VulkanRenderer::set_fullscreen(bool fullscreen) {
    if (!impl_ || impl_->window == nullptr) return;
#if defined(__ANDROID__)
    fullscreen = true;
#endif
    SDL_SetWindowFullscreen(impl_->window, fullscreen);
    impl_->swapchain_dirty = true;
}

void VulkanRenderer::set_present_mode(settings::PresentMode mode) {
    if (!impl_) return;
    impl_->requested_present = mode;
    impl_->swapchain_dirty = true;
}

bool VulkanRenderer::supports_present_mode(settings::PresentMode mode) const {
    if (!impl_) return false;
    VkPresentModeKHR wanted = VK_PRESENT_MODE_FIFO_KHR;
    if (mode == settings::PresentMode::Mailbox) wanted = VK_PRESENT_MODE_MAILBOX_KHR;
    if (mode == settings::PresentMode::Immediate) wanted = VK_PRESENT_MODE_IMMEDIATE_KHR;
    const auto &modes = impl_->present_modes;
    return wanted == VK_PRESENT_MODE_FIFO_KHR || std::find(modes.begin(), modes.end(), wanted) != modes.end();
}

void VulkanRenderer::set_aspect(settings::Aspect aspect) {
    if (!impl_) return;
    impl_->aspect = aspect;
    impl_->resize_now = true;
    impl_->follow_window();
}

float VulkanRenderer::game_aspect() const noexcept {
    if (!impl_ || impl_->aspect != settings::Aspect::Fill || impl_->target_extent.height == 0u)
        return static_cast<float>(kPspWidth) / static_cast<float>(kPspHeight);
    return static_cast<float>(impl_->target_extent.width) / static_cast<float>(impl_->target_extent.height);
}

std::array<std::uint32_t, 2> VulkanRenderer::target_size() const noexcept {
    if (!impl_) return {kPspWidth, kPspHeight};
    return {impl_->target_extent.width, impl_->target_extent.height};
}

void VulkanRenderer::set_sharp_screen(bool sharp) {
    if (impl_) impl_->sharp_screen = sharp;
}

void VulkanRenderer::set_texture_pack(bool enabled) {
    // Applied at the next begin_frame(), where no frame is being recorded.
    if (impl_) impl_->pack_wanted = enabled;
}

std::string VulkanRenderer::texture_pack_status() const {
    if (!impl_) return {};
    const Impl &impl = *impl_;
    if (impl.pack_held) return "Updating";
    if (impl.pack_wanted != impl.pack_applied || impl.pack_reload) return impl.pack_wanted ? "Loading" : "Off";
    if (!impl.pack) return impl.pack_status;
    return impl.pack_status + ", " + std::to_string(impl.replacements.resident_count()) + " on the GPU (" +
           std::to_string(impl.replacements.resident_bytes() >> 20u) + " MB)";
}

void VulkanRenderer::reload_texture_pack() {
    if (impl_) impl_->pack_reload = true;
}

void VulkanRenderer::hold_texture_pack(bool hold) {
    if (impl_) impl_->pack_held = hold;
}

bool VulkanRenderer::texture_pack_held() const {
    return impl_ && impl_->pack_held && !impl_->pack_applied && !impl_->pack;
}

std::filesystem::path VulkanRenderer::textures_root() { return install::user_data_directory() / "textures"; }

std::string VulkanRenderer::texture_pack_folder() const {
    const std::string in_place = settings::current().texture_pack_folder;
    return install::path_to_utf8(texture_pack_location(textures_root(), install::kDiscId, in_place).folder);
}

void VulkanRenderer::set_sharp_textures(bool sharp) {
    Impl &impl = *impl_;
    if (!impl.ready || impl.recording || impl.sharp_textures == sharp) return;
    impl.sharp_textures = sharp;
    // The sampler is part of each texture's descriptor; rewrite them all.
    vkDeviceWaitIdle(impl.device);
    const auto rewrite = [&](Impl::Texture &texture) {
        if (texture.descriptor == VK_NULL_HANDLE) return;
        VkDescriptorImageInfo image_info{impl.texture_sampler(), texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = texture.descriptor;
        write.descriptorCount = 1u;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image_info;
        vkUpdateDescriptorSets(impl.device, 1u, &write, 0u, nullptr);
    };
    rewrite(impl.white_texture);
    for (auto &[key, texture] : impl.textures) rewrite(texture);
    impl.replacements.set_sharp(sharp);
    for (auto &[address, target] : impl.targets) {
        for (std::size_t i = 0; i < target.copy_descriptors.size(); ++i) {
            if (target.copy_descriptors[i] == VK_NULL_HANDLE) continue;
            VkDescriptorImageInfo image_info{impl.framebuffer_sampler(), i == 0u ? target.copy_view : target.copy_opaque_view,
                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = target.copy_descriptors[i];
            write.descriptorCount = 1u;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &image_info;
            vkUpdateDescriptorSets(impl.device, 1u, &write, 0u, nullptr);
        }
    }
}

void VulkanRenderer::set_perf_overlay(bool visible) {
    if (impl_) impl_->overlay_visible = impl_->overlay_ready && visible;
}

void VulkanRenderer::capture_window(const std::string &path) {
    if (!impl_ || !impl_->ready) return;
    if ((impl_->swapchain_usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0u) {
        std::cout << "[render] this swapchain cannot be read back; no window capture\n";
        return;
    }
    impl_->capture_path = path;
}

bool VulkanRenderer::initialize_ui(std::string &error) {
    Impl &impl = *impl_;
    if (!impl.ready) {
        error = "no renderer";
        return false;
    }
    if (impl.ui_ready) return true;
    // Loads what the game frame and the overlay left in the swapchain image
    // and draws over it; submit_and_present moves it on to presentation.
    VkAttachmentDescription attachment{};
    attachment.format = impl.swapchain_format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference reference{0u, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1u;
    subpass.pColorAttachments = &reference;
    VkRenderPassCreateInfo pass_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    pass_info.attachmentCount = 1u;
    pass_info.pAttachments = &attachment;
    pass_info.subpassCount = 1u;
    pass_info.pSubpasses = &subpass;
    if (!check(vkCreateRenderPass(impl.device, &pass_info, nullptr, &impl.ui_render_pass), "vkCreateRenderPass",
               error))
        return false;
    if (!impl.create_ui_framebuffers(error)) return false;

    ImGui_ImplVulkan_InitInfo info{};
    info.ApiVersion = VK_API_VERSION_1_1;
    info.Instance = impl.instance;
    info.PhysicalDevice = impl.physical_device;
    info.Device = impl.device;
    info.QueueFamily = impl.queue_family;
    info.Queue = impl.queue;
    info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE;
    info.MinImageCount = impl.swapchain_min_images;
    // ImGui keeps this many vertex and index buffers and writes the next one
    // for each draw of the interface. Two frames in flight and two presents
    // between flips can all still be reading one, so there are enough for
    // all of them however few images the swapchain has.
    info.ImageCount = std::max<std::uint32_t>({static_cast<std::uint32_t>(impl.swapchain_images.size()),
                                               impl.swapchain_min_images, 8u});
    info.PipelineInfoMain.RenderPass = impl.ui_render_pass;
    info.PipelineInfoMain.Subpass = 0u;
    info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    if (!ImGui_ImplVulkan_Init(&info)) {
        error = "ImGui_ImplVulkan_Init failed";
        return false;
    }
    impl.ui_ready = true;
    return true;
}

void VulkanRenderer::shutdown_ui() {
    Impl &impl = *impl_;
    if (!impl.ui_ready) return;
    vkDeviceWaitIdle(impl.device);
    ImGui_ImplVulkan_Shutdown();
    impl.ui_ready = false;
    for (VkFramebuffer framebuffer : impl.ui_framebuffers) vkDestroyFramebuffer(impl.device, framebuffer, nullptr);
    impl.ui_framebuffers.clear();
    vkDestroyRenderPass(impl.device, impl.ui_render_pass, nullptr);
    impl.ui_render_pass = VK_NULL_HANDLE;
}

void VulkanRenderer::begin_ui_frame() {
    if (impl_ && impl_->ui_ready) ImGui_ImplVulkan_NewFrame();
}

void VulkanRenderer::set_ui_draw_data(ImDrawData *draw_data) {
    if (impl_) impl_->ui_draw_data = draw_data;
}

void VulkanRenderer::present_ui(bool show_game) {
    Impl &impl = *impl_;
    if (!impl.ready) return;
    if (!impl.recording) begin_frame();
    impl.end_pass();
    VkImage source = VK_NULL_HANDLE;
    if (show_game) {
        if (const auto shown = impl.targets.find(impl.presented_target); shown != impl.targets.end())
            source = shown->second.color;
    }
    impl.submit_and_present(source, false);
    impl.follow_window();
}

void VulkanRenderer::begin_frame() {
    Impl &impl = *impl_;
    if (!impl.ready || impl.recording) return;
    // The next slot: its fence is the frame slot_count frames back.
    impl.slot = (impl.slot + 1u) % impl.slot_count;
    Impl::FrameSlot &frame = impl.slots[impl.slot];
    impl.command_buffer = frame.commands;
    impl.frame_fence = frame.fence;
    impl.frame_uploads = frame.uploads;
    const perf::Clock::time_point wait_start = perf::Clock::now();
    impl.wait_fence(impl.frame_fence, "the wait for a frame two back");
    perf::add_wait_time(perf::Clock::now() - wait_start, perf::Stall::Fence);
    vkResetFences(impl.device, 1u, &impl.frame_fence);
    impl.release_frame_uploads(impl.slot);
    impl.collect_gpu_time(impl.slot);
    impl.compare_gpu_decode(impl.slot);
    // With one frame in flight its write-back is taken here, as before;
    // with more, write_back_frame() takes it.
    impl.collect_writeback(impl.slot, false);
    impl.report_pipelines(false);
    impl.apply_texture_pack();
    impl.replacements.begin_frame(impl.frames);
    if (texture_pack_trace() && impl.pack && impl.frames % 60u == 0u) {
        std::cout << "[texpack] frame " << impl.frames << ": " << impl.replaced_draws << " draws replaced in 60 frames, "
                  << impl.replacements.resident_count() << " images on the GPU ("
                  << (impl.replacements.resident_bytes() >> 20u) << " MB)\n";
        impl.replaced_draws = 0u;
    }
    vkResetCommandBuffer(impl.command_buffer, 0u);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(impl.command_buffer, &begin);
    impl.breadcrumb(impl.command_buffer, "frame begins");
    if (impl.gpu_timer != VK_NULL_HANDLE) {
        vkCmdResetQueryPool(impl.command_buffer, impl.gpu_timer, impl.gpu_timer_base(), 2u * kGpuTimerSegments);
        impl.gpu_timer_used = 0u;
        impl.gpu_timer_open = false;
        impl.begin_gpu_segment(impl.command_buffer);
    }
    // With frame interpolation each frame writes the next region and is
    // recorded for drawing again; without it, the first region as before.
    impl.interpolating = impl.interpolation_wanted();
    const std::uint32_t region = impl.interpolating ? impl.next_region : impl.slot;
    // A region another frame in flight, or a present between flips, may
    // still draw from is waited for first (only when interpolation has just
    // been turned on or off, as their regions follow each other otherwise).
    for (std::uint32_t other = 0; other < Impl::kMaxSlots; ++other) {
        if (other != impl.slot && impl.slots[other].region == region &&
            vkGetFenceStatus(impl.device, impl.slots[other].fence) != VK_SUCCESS) {
            const perf::Clock::time_point region_start = perf::Clock::now();
            impl.wait_fence(impl.slots[other].fence, "the wait for the frame before");
            perf::add_wait_time(perf::Clock::now() - region_start, perf::Stall::Fence);
        }
    }
    for (std::uint32_t present = 0; present < impl.present_fences.size(); ++present) {
        if ((impl.present_regions[present] & (1u << region)) != 0u) {
            const perf::Clock::time_point region_start = perf::Clock::now();
            impl.wait_fence(impl.present_fences[present], "the wait for a present");
            perf::add_wait_time(perf::Clock::now() - region_start, perf::Stall::Fence);
            impl.present_regions[present] = 0u;
        }
    }
    frame.region = region;
    impl.enter_region(region);
    if (impl.interpolating && !impl.recording_frame.recorded) {
        impl.recording_frame.clear();
        impl.recording_frame.recorded = true;
        impl.recording_frame.texture_clock = impl.texture_clock;
    }
    impl.environment_version = 0u;
    impl.object_valid = false;
    impl.raw_valid = false;
    impl.forget_bindings();
    impl.pass_active = false;
    impl.recording = true;
}

void VulkanRenderer::upload_frame(std::uint32_t display_address, const std::uint8_t *pixels, std::uint32_t width,
                                  std::uint32_t height, std::uint32_t stride) {
    Impl &impl = *impl_;
    if (!impl.ready || pixels == nullptr || width == 0u || height == 0u || stride < width) return;
    if (!impl.recording) begin_frame();
    impl.end_pass();
    std::string error;
    if (impl.upload_extent.width != width || impl.upload_extent.height != height) {
        // Nothing recorded so far this frame uses the old image yet.
        const perf::Clock::time_point idle_start = perf::Clock::now();
        vkDeviceWaitIdle(impl.device);
        perf::note_stall(perf::Stall::Idle, perf::Clock::now() - idle_start);
        if (!impl.create_upload(width, height, error)) {
            std::cerr << "Renderer: cannot upload frames (" << error << ")\n";
            impl.destroy_upload();
            return;
        }
    }
    Impl::Target *target = impl.target_for(display_address, error);
    if (target == nullptr) return;

    // The slot's staging buffer is free: its fence was waited on in begin_frame.
    const Impl::Staging &movie = impl.slots[impl.slot].movie;
    auto *staging = static_cast<std::uint8_t *>(movie.mapped);
    for (std::uint32_t row = 0; row < height; ++row)
        std::memcpy(staging + static_cast<std::size_t>(row) * width * 4u,
                    pixels + static_cast<std::size_t>(row) * stride * 4u, static_cast<std::size_t>(width) * 4u);
    impl.transition(impl.command_buffer, impl.upload_image, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    copy.imageExtent = {width, height, 1u};
    vkCmdCopyBufferToImage(impl.command_buffer, movie.buffer, impl.upload_image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copy);
    impl.transition(impl.command_buffer, impl.upload_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    // A new target has no contents to keep; an old one rests in the layout
    // the render pass expects.
    impl.transition(impl.command_buffer, target->color,
                    target->initialized ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    if (!target->initialized) {
        impl.transition(impl.command_buffer, target->depth, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, impl.depth_aspect());
        target->initialized = true;
    }
    VkImageBlit blit{};
    blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    blit.srcOffsets[1] = {static_cast<std::int32_t>(width), static_cast<std::int32_t>(height), 1};
    blit.dstSubresource = blit.srcSubresource;
    blit.dstOffsets[1] = {static_cast<std::int32_t>(impl.target_extent.width),
                          static_cast<std::int32_t>(impl.target_extent.height), 1};
    // A movie has the PSP's shape: under Fill it keeps it, with black beside
    // (or above and below) it.
    float fit_x = 1.0f, fit_y = 1.0f;
    if (impl.interface_fit(fit_x, fit_y)) {
        const auto inset_x = static_cast<std::int32_t>(std::lround(0.5f * (1.0f - fit_x) * impl.target_extent.width));
        const auto inset_y = static_cast<std::int32_t>(std::lround(0.5f * (1.0f - fit_y) * impl.target_extent.height));
        blit.dstOffsets[0] = {inset_x, inset_y, 0};
        blit.dstOffsets[1] = {static_cast<std::int32_t>(impl.target_extent.width) - inset_x,
                              static_cast<std::int32_t>(impl.target_extent.height) - inset_y, 1};
        const VkClearColorValue black{{0.0f, 0.0f, 0.0f, 1.0f}};
        const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};
        vkCmdClearColorImage(impl.command_buffer, target->color, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &black, 1u,
                             &range);
        impl.transition(impl.command_buffer, target->color, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    }
    vkCmdBlitImage(impl.command_buffer, impl.upload_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, target->color,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &blit, VK_FILTER_LINEAR);
    impl.transition(impl.command_buffer, target->color, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    impl.last_drawn_target = display_address;
    // The frame came from guest memory, which texturing reads correctly anyway.
    ++target->draw_serial;
    target->guest_words.clear();
}

bool VulkanRenderer::gpu_decode() const {
    if (!impl_ || !impl_->ready || !impl_->gpu_decode_available) return false;
    // Off unless MHP3RD_GPU_DECODE=1 asks for it: on the Steam Deck (radv)
    // the raw vertex path hung the GPU (ring gfx timeout) at the character
    // select screen, and the CPU decode did not. Also off for the paths and
    // traces that need decoded vertices.
    static const bool off = [] {
        if (std::getenv("MHP3RD_CHECK_GPU_DECODE") != nullptr) return false;
        const char *text = std::getenv("MHP3RD_GPU_DECODE");
        return text == nullptr || std::strcmp(text, "1") != 0;
    }();
    static const bool needs_vertices =
        std::getenv("MHP3RD_NO_DIRECT_VERTICES") != nullptr || std::getenv("MHP3RD_CHECK_DIRECT_VERTICES") != nullptr ||
        std::getenv("MHP3RD_TRACE_GE") != nullptr || std::getenv("MHP3RD_TRACE_3D") != nullptr ||
        std::getenv("MHP3RD_TRACE_SPRITES") != nullptr || std::getenv("MHP3RD_TRACE_LIGHTING") != nullptr ||
        std::getenv("MHP3RD_TRACE_FB_TEXTURES") != nullptr;
    return !off && !needs_vertices && !perf::alternate_off(perf::NewPath::Direct) &&
           !perf::alternate_off(perf::NewPath::GpuDecode);
}

bool VulkanRenderer::check_gpu_decode() const { return impl_ && impl_->check_gpu_decode; }

void VulkanRenderer::begin_display_list() {
    if (!impl_) return;
    impl_->list_texture_keys.clear();
    impl_->last_texture = nullptr;
}

void VulkanRenderer::submit(const DrawCall &call, const GuestMemory &memory) {
    Impl &impl = *impl_;
    if (!impl.ready) return;
    const perf::SplitScope split(perf::Split::Draw);
    if (!impl.recording) begin_frame();
    // Presents between flips fall due while the game draws, too: a busy
    // frame spends most of its time in here.
    if (impl.cycle_active && (++impl.draws_since_poll & 15u) == 0u)
        impl.poll_presents(std::chrono::steady_clock::now(), false, false);
    // GPU vertex decode: the draw carries the guest's vertex bytes instead of
    // decoded vertices (GeState::set_raw_vertices).
    const bool raw = call.raw_vertices != nullptr;
    if (call.vertices.empty() && !raw) return;

    // Everything becomes a triangle list; sprites expand to two triangles.
    impl.scratch.clear();
    // A vertex format without a colour field leaves every vertex white, and the
    // GE supplies the colour from the material registers instead. That is where
    // the marker over an NPC's head and the shadow blobs under characters get
    // both their colour and the alpha that makes them faint.
    //
    // Only unlit draws, though. Lighting on means the material colour is one term
    // of a sum the lights complete, which the vertex shader evaluates; handing
    // it over as the finished colour turned every character a flat muddy brown.
    // MHP3RD_NO_LIGHTING leaves lit geometry with the old white stand-in and
    // turns fog off too, which is how everything was drawn before either
    // existed; MHP3RD_NO_FOG turns off fog alone.
    static const bool no_material_color = std::getenv("MHP3RD_NO_MATERIAL_COLOR") != nullptr;
    static const bool no_lighting = std::getenv("MHP3RD_NO_LIGHTING") != nullptr;
    static const bool no_fog = no_lighting || std::getenv("MHP3RD_NO_FOG") != nullptr;
    const bool lit = call.lighting_enabled && !no_lighting && !call.through && !call.clear_mode;
    const bool use_material_color = !no_material_color && !call.has_vertex_color && !call.lighting_enabled;
    const auto to_gpu = [&](const Vertex &vertex) {
        GpuVertex out{};
        out.x = vertex.position[0];
        out.y = vertex.position[1];
        out.z = vertex.position[2];
        out.u = vertex.texcoord[0];
        out.v = vertex.texcoord[1];
        out.color = use_material_color ? call.material_color : vertex.color;
        out.nx = vertex.normal[0];
        out.ny = vertex.normal[1];
        out.nz = vertex.normal[2];
        return out;
    };
    const auto push_vertex = [&](const Vertex &vertex) { impl.scratch.push_back(to_gpu(vertex)); };
    const auto vertex_at = [&](std::size_t index) -> const Vertex & {
        if (!call.indices.empty()) {
            const std::size_t mapped = call.indices[index];
            return call.vertices[std::min(mapped, call.vertices.size() - 1u)];
        }
        return call.vertices[std::min(index, call.vertices.size() - 1u)];
    };
    const std::size_t vertex_total = raw ? call.raw_count : call.vertices.size();
    const std::size_t count = call.indices.empty() ? vertex_total : call.indices.size();

    static const bool trace = std::getenv("MHP3RD_TRACE_GE") != nullptr;
    static const bool trace3d = std::getenv("MHP3RD_TRACE_3D") != nullptr;
    // MHP3RD_TRACE_SPRITES=N: every through-mode sprite of frame N, with the
    // texture state it samples, to find the tiles a 2D screen is built from.
    static const std::uint64_t trace_sprites_frame = [] {
        const char *text = std::getenv("MHP3RD_TRACE_SPRITES");
        return text != nullptr ? std::strtoull(text, nullptr, 10) : ~0ull;
    }();

    // Transformed triangles, strips and fans go straight into the vertex
    // buffer: each decoded vertex once, converted as it is written, and a
    // 16-bit index list in the order the expansion below writes vertices in,
    // so the GPU draws the same triangles from the same vertex data without
    // the copies and the repeated strip vertices. MHP3RD_NO_DIRECT_VERTICES
    // expands every draw as before; the traces that read expanded vertices
    // keep the expansion too.
    static const bool legacy_vertices = std::getenv("MHP3RD_NO_DIRECT_VERTICES") != nullptr;
    static const bool check_direct = std::getenv("MHP3RD_CHECK_DIRECT_VERTICES") != nullptr;
    const bool direct = raw || !legacy_vertices && !perf::alternate_off(perf::NewPath::Direct) && !call.through &&
                        (call.primitive == PrimitiveType::Triangles ||
                         call.primitive == PrimitiveType::TriangleStrip ||
                         call.primitive == PrimitiveType::TriangleFan) &&
                        !trace && !trace3d && impl.frames != trace_sprites_frame;

    const auto expand = [&]() -> bool {
        switch (call.primitive) {
        case PrimitiveType::Triangles:
            for (std::size_t i = 0; i + 2u < count; i += 3u) {
                push_vertex(vertex_at(i));
                push_vertex(vertex_at(i + 1u));
                push_vertex(vertex_at(i + 2u));
            }
            break;
        case PrimitiveType::TriangleStrip:
            for (std::size_t i = 0; i + 2u < count; ++i) {
                const bool odd = (i & 1u) != 0u;
                push_vertex(vertex_at(i));
                push_vertex(vertex_at(odd ? i + 2u : i + 1u));
                push_vertex(vertex_at(odd ? i + 1u : i + 2u));
            }
            break;
        case PrimitiveType::TriangleFan:
            for (std::size_t i = 1u; i + 1u < count; ++i) {
                push_vertex(vertex_at(0));
                push_vertex(vertex_at(i));
                push_vertex(vertex_at(i + 1u));
            }
            break;
        case PrimitiveType::Sprites:
            // Vertex pairs describe the opposite corners of a rectangle.
            for (std::size_t i = 0; i + 1u < count; i += 2u) {
                Vertex a = vertex_at(i);
                const Vertex &b = vertex_at(i + 1u);
                // The GE flat-shades sprites from the second vertex: its depth (and
                // colour) cover the whole rectangle. sceGuClear relies on this, as
                // its first vertex carries z = 0 and only the second the clear depth.
                a.position[2] = b.position[2];
                a.color = b.color;
                a.normal = b.normal;
                Vertex top_right = b;
                top_right.position[1] = a.position[1];
                top_right.texcoord[1] = a.texcoord[1];
                Vertex bottom_left = a;
                bottom_left.position[1] = b.position[1];
                bottom_left.texcoord[1] = b.texcoord[1];
                push_vertex(a);
                push_vertex(top_right);
                push_vertex(b);
                push_vertex(a);
                push_vertex(b);
                push_vertex(bottom_left);
            }
            break;
        default:
            return false;  // points and lines are not drawn yet
        }
        return true;
    };
    if (direct) {
        triangle_indices(call.primitive, count, call.indices, vertex_total, impl.direct_indices);
        if (impl.direct_indices.empty()) return;
        if (check_direct && !raw) {
            // Every index must name a vertex equal, byte for byte, to the one
            // the expansion puts in its place.
            if (!expand()) return;
            ++impl.direct_checked;
            bool same = impl.scratch.size() == impl.direct_indices.size();
            for (std::size_t i = 0; same && i < impl.scratch.size(); ++i) {
                const GpuVertex converted = to_gpu(call.vertices[impl.direct_indices[i]]);
                same = std::memcmp(&converted, &impl.scratch[i], sizeof(GpuVertex)) == 0;
            }
            if (!same && ++impl.direct_mismatched <= 20u)
                std::cout << "[direct-check] draw " << impl.draws << " prim=" << static_cast<int>(call.primitive)
                          << " count=" << count << " vertices=" << call.vertices.size() << " expanded "
                          << impl.scratch.size() << " indices " << impl.direct_indices.size() << " differ\n";
        }
    } else {
        if (!expand()) return;
        if (impl.scratch.empty()) return;
    }

    // Through-mode vertices carry the texel range they may sample in the
    // otherwise unused normal and w; see clamp_through_quad().
    // MHP3RD_NO_SPRITE_CLAMP lets them sample anywhere, as before.
    if (call.through) {
        for (GpuVertex &vertex : impl.scratch) set_uv_rect(vertex, -kNoClamp, -kNoClamp, kNoClamp, kNoClamp);
        static const bool no_sprite_clamp = std::getenv("MHP3RD_NO_SPRITE_CLAMP") != nullptr;
        const bool quads = call.primitive == PrimitiveType::Sprites ||
                           ((call.primitive == PrimitiveType::TriangleStrip ||
                             call.primitive == PrimitiveType::TriangleFan) &&
                            count == 4u);
        if (!no_sprite_clamp && call.texture.enabled && !call.clear_mode && quads) {
            for (std::size_t first = 0; first + 6u <= impl.scratch.size(); first += 6u)
                clamp_through_quad(impl.scratch, first, static_cast<float>(call.texture.width),
                                   static_cast<float>(call.texture.height));
        }
    }

    if (trace && impl.draws < 400u) {
        const GpuVertex &first = impl.scratch.front();
        const GpuVertex &second = impl.scratch[std::min<std::size_t>(1u, impl.scratch.size() - 1u)];
        std::cout << "[ge] prim=" << static_cast<int>(call.primitive) << (call.through ? " through" : " transform")
                  << " verts=" << impl.scratch.size() << " p0=(" << first.x << "," << first.y << "," << first.z
                  << ") p1=(" << second.x << "," << second.y << ") uv0=(" << first.u << "," << first.v << ") uv1=("
                  << second.u << "," << second.v << ") tex=" << (call.texture.enabled ? 1 : 0)
                  << " fmt=" << static_cast<int>(call.texture.format) << " size=" << call.texture.width << "x"
                  << call.texture.height << " bufw=" << call.texture.buffer_width
                  << " swizzled=" << (call.texture.swizzled ? 1 : 0) << " addr=0x" << std::hex
                  << call.texture.address << " target=0x" << call.target.color_address << std::dec
                  << " stride=" << call.target.color_stride << " blend=" << (call.blend.enabled ? 1 : 0) << "\n";
    }


    if (impl.frames == trace_sprites_frame && call.primitive != PrimitiveType::Sprites) {
        float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f, u0 = 1e9f, v0 = 1e9f, u1 = -1e9f, v1 = -1e9f;
        for (std::size_t i = 0; i < count; ++i) {
            const Vertex &v = vertex_at(i);
            x0 = std::min(x0, v.position[0]); x1 = std::max(x1, v.position[0]);
            y0 = std::min(y0, v.position[1]); y1 = std::max(y1, v.position[1]);
            u0 = std::min(u0, v.texcoord[0]); u1 = std::max(u1, v.texcoord[0]);
            v0 = std::min(v0, v.texcoord[1]); v1 = std::max(v1, v.texcoord[1]);
        }
        std::cout << "[sprite] other prim=" << static_cast<int>(call.primitive) << (call.through ? " through" : " transform")
                  << " n=" << count << " pos=(" << x0 << "," << y0 << ")-(" << x1 << "," << y1 << ") uv=(" << u0
                  << "," << v0 << ")-(" << u1 << "," << v1 << ") tex=" << (call.texture.enabled ? 1 : 0) << " 0x"
                  << std::hex << call.texture.address << std::dec << " " << call.texture.width << "x"
                  << call.texture.height << " fmt=" << static_cast<int>(call.texture.format)
                  << " filter=" << call.texture.min_filter << "/" << call.texture.mag_filter << "\n";
    }
    if (call.through && call.primitive == PrimitiveType::Sprites && impl.frames == trace_sprites_frame) {
        for (std::size_t i = 0; i + 1u < count; i += 2u) {
            const Vertex &a = vertex_at(i);
            const Vertex &b = vertex_at(i + 1u);
            std::cout << "[sprite] pos=(" << a.position[0] << "," << a.position[1] << ")-(" << b.position[0] << ","
                      << b.position[1] << ") uv=(" << a.texcoord[0] << "," << a.texcoord[1] << ")-("
                      << b.texcoord[0] << "," << b.texcoord[1] << ") tex=" << (call.texture.enabled ? 1 : 0)
                      << " 0x" << std::hex << call.texture.address << std::dec << " " << call.texture.width << "x"
                      << call.texture.height << " fmt=" << static_cast<int>(call.texture.format)
                      << " filter=" << call.texture.min_filter << "/" << call.texture.mag_filter
                      << " wrap=" << call.texture.wrap_s << "/" << call.texture.wrap_t
                      << " blend=" << (call.blend.enabled ? 1 : 0) << " color=0x" << std::hex << b.color << std::dec
                      << "\n";
        }
    }

    // Deep dump of the first transformed draws: matrices, raw positions and the
    // same positions after a CPU-side transform, so a geometry that never shows
    // up can be traced to the stage that loses it.
    static const bool trace_camera = std::getenv("MHP3RD_TRACE_CAMERA") != nullptr;
    // The camera hunt reads the same measurement without printing it.
    static const bool watch_camera = trace_camera || std::getenv("MHP3RD_FIND_CAMERA") != nullptr;
    static std::uint32_t traced_3d = 0u;
    static std::uint32_t traced_clears = 0u;
    if (trace3d && call.clear_mode && traced_clears < 4u) {
        ++traced_clears;
        const GpuVertex &first = impl.scratch.front();
        const GpuVertex &last = impl.scratch.back();
        std::cout << "[3d] clear#" << traced_clears << " flags=" << call.clear_flags
                  << " (colour=" << ((call.clear_flags & 1u) != 0u) << " alpha=" << ((call.clear_flags & 2u) != 0u)
                  << " depth=" << ((call.clear_flags & 4u) != 0u) << ") through=" << (call.through ? 1 : 0)
                  << " verts=" << impl.scratch.size() << " z=" << first.z << ".." << last.z << "\n";
    }
    if (trace3d && !call.through && traced_3d < 12u) {
        ++traced_3d;
        const auto wvp = multiply(call.projection, multiply(call.view, call.world));
        const auto dump = [](const char *name, const std::array<float, 16> &m) {
            std::cout << "  " << name << " =";
            for (std::uint32_t row = 0; row < 4u; ++row) {
                std::cout << " [";
                for (std::uint32_t col = 0; col < 4u; ++col)
                    std::cout << (col ? " " : "") << m[col * 4u + row];
                std::cout << "]";
            }
            std::cout << "\n";
        };
        std::cout << "[3d] draw#" << traced_3d << " prim=" << static_cast<int>(call.primitive)
                  << " vtype=0x" << std::hex << call.vertex_type << std::dec << " verts=" << impl.scratch.size()
                  << " clear=" << (call.clear_mode ? 1 : 0) << "/" << call.clear_flags
                  << " ztest=" << (call.depth.test_enabled ? 1 : 0) << " zfunc=" << call.depth.function
                  << " zwrite=" << (call.depth.write_enabled ? 1 : 0) << " zrange=[" << call.depth.range_near << ","
                  << call.depth.range_far << "]"
                  << " cull=" << (call.culling_enabled ? 1 : 0) << " cw=" << (call.cull_clockwise ? 1 : 0)
                  << " blend=" << (call.blend.enabled ? 1 : 0) << " tex=" << (call.texture.enabled ? 1 : 0) << "\n";
        std::cout << "  viewport scale=(" << call.viewport.x_scale << "," << call.viewport.y_scale << ","
                  << call.viewport.z_scale << ") offset=(" << call.viewport.x_offset << "," << call.viewport.y_offset
                  << "," << call.viewport.z_offset << ") region=(" << call.viewport.offset_x << ","
                  << call.viewport.offset_y << ") scissor=(" << call.viewport.scissor_x1 << ","
                  << call.viewport.scissor_y1 << ")-(" << call.viewport.scissor_x2 << "," << call.viewport.scissor_y2
                  << ")\n";
        dump("world", call.world);
        dump("view ", call.view);
        dump("proj ", call.projection);
        dump("wvp  ", wvp);
        for (std::size_t i = 0; i < std::min<std::size_t>(3u, impl.scratch.size()); ++i) {
            const GpuVertex &v = impl.scratch[i];
            float clip[4]{};
            for (std::uint32_t row = 0; row < 4u; ++row)
                clip[row] = wvp[0 * 4u + row] * v.x + wvp[1 * 4u + row] * v.y + wvp[2 * 4u + row] * v.z +
                            wvp[3 * 4u + row];
            std::cout << "  v" << i << " obj=(" << v.x << "," << v.y << "," << v.z << ") clip=(" << clip[0] << ","
                      << clip[1] << "," << clip[2] << "," << clip[3] << ")";
            if (clip[3] != 0.0f)
                std::cout << " ndc=(" << clip[0] / clip[3] << "," << clip[1] / clip[3] << "," << clip[2] / clip[3]
                          << ")";
            std::cout << "\n";
        }
    }

    if (call.through) {
        ++impl.frame_through_draws;
    } else {
        ++impl.frame_transformed_draws;
        // Only MHP3RD_TRACE_3D reads it: a map insert per draw otherwise.
        if (trace3d) ++impl.frame_transformed_targets[call.target.color_address];
        if (watch_camera) {
            const auto same = std::find_if(impl.frame_views.begin(), impl.frame_views.end(),
                                           [&](const auto &entry) { return entry.first == call.view; });
            const auto vertices =
                static_cast<std::uint32_t>(direct ? impl.direct_indices.size() : impl.scratch.size());
            if (same == impl.frame_views.end())
                impl.frame_views.emplace_back(call.view, vertices);
            else
                same->second += vertices;
        }
        if (trace3d) {
            // Where does this frame's transformed geometry actually land? A
            // bounding box in normalised device coordinates separates "clipped
            // away" from "drawn but invisible".
            const auto wvp = multiply(call.projection, multiply(call.view, call.world));
            for (const GpuVertex &v : impl.scratch) {
                float clip[4]{};
                for (std::uint32_t row = 0; row < 4u; ++row)
                    clip[row] = wvp[0 * 4u + row] * v.x + wvp[1 * 4u + row] * v.y + wvp[2 * 4u + row] * v.z +
                                wvp[3 * 4u + row];
                if (clip[3] <= 0.0f) {
                    ++impl.frame_behind_camera;
                    continue;
                }
                const float ndc[3]{clip[0] / clip[3], clip[1] / clip[3], clip[2] / clip[3]};
                for (std::uint32_t axis = 0; axis < 3u; ++axis) {
                    impl.frame_ndc_min[axis] = std::min(impl.frame_ndc_min[axis], ndc[axis]);
                    impl.frame_ndc_max[axis] = std::max(impl.frame_ndc_max[axis], ndc[axis]);
                }
                if (ndc[0] >= -1.0f && ndc[0] <= 1.0f && ndc[1] >= -1.0f && ndc[1] <= 1.0f && ndc[2] >= -1.0f &&
                    ndc[2] <= 1.0f)
                    ++impl.frame_onscreen_vertices;
                ++impl.frame_transformed_vertices;
            }
        }
    }

    // Lighting and fog read the environment block; lit draws also read an
    // object block. Each is written into the vertex buffer only when it
    // changed, and the vertices follow whatever was written.
    const bool fogged = call.fog.enabled && !no_fog && !call.through && !call.clear_mode;
    const auto view_world = multiply(call.view, call.world);
    if ((lit || fogged) && impl.environment_version != call.environment_version) {
        const LightingState &state = call.lighting;
        EnvironmentBlock environment{};
        environment.ambient = unpack_color(state.ambient_color, static_cast<float>(state.ambient_alpha) / 255.0f);
        environment.fog = {call.fog.end, call.fog.scale, 0.0f, 0.0f};
        environment.fog_color = unpack_color(call.fog.color);
        for (std::size_t i = 0; i < state.lights.size(); ++i) {
            const LightState &light = state.lights[i];
            environment.light_position[i] = {light.position[0], light.position[1], light.position[2],
                                             light.enabled ? 1.0f : 0.0f};
            environment.light_direction[i] = {light.direction[0], light.direction[1], light.direction[2],
                                              static_cast<float>(light.type)};
            environment.light_attenuation[i] = {light.attenuation[0], light.attenuation[1], light.attenuation[2],
                                                static_cast<float>(light.kind)};
            environment.light_spot[i] = {light.spot_exponent, light.spot_cutoff, 0.0f, 0.0f};
            environment.light_ambient[i] = unpack_color(light.ambient);
            environment.light_diffuse[i] = unpack_color(light.diffuse);
            environment.light_specular[i] = unpack_color(light.specular);
        }
        if (!impl.write_uniform(&environment, sizeof(environment), impl.environment_offset)) return;
        impl.environment_version = call.environment_version;
    }
    if (lit) {
        const LightingState &state = call.lighting;
        if (impl.material_version != call.material_version) {
            impl.material[0] = unpack_color(state.material_emissive, state.specular_power);
            impl.material[1] =
                unpack_color(call.material_color, static_cast<float>(call.material_color >> 24u) / 255.0f);
            impl.material[2] = unpack_color(state.material_diffuse, static_cast<float>(state.mode));
            impl.material[3] = unpack_color(state.material_specular, state.reverse_normals ? 1.0f : 0.0f);
            impl.material_version = call.material_version;
        }
        ObjectBlock object{};
        object.world = call.world;
        object.flags = {1.0f, call.has_vertex_color ? 1.0f : 0.0f, 0.0f, static_cast<float>(state.material_update)};
        object.emissive = impl.material[0];
        object.material_ambient = impl.material[1];
        object.material_diffuse = impl.material[2];
        object.material_specular = impl.material[3];
        if (!impl.object_valid || std::memcmp(&object, &impl.last_object, sizeof(object)) != 0) {
            if (!impl.write_uniform(&object, sizeof(object), impl.object_offset)) return;
            impl.last_object = object;
            impl.object_valid = true;
        }
    }
    // A raw draw's format, colour and bones, written when they differ from
    // the last raw draw's. With MHP3RD_CHECK_GPU_DECODE every raw draw gets
    // its own, naming the slot its shader writes what it decoded into.
    std::uint32_t check_slot = 0u;
    bool checked = false;
    if (raw) {
        const VertexFormat format = vertex_format(call.vertex_type);
        const auto field = [](std::uint32_t offset) { return offset == kNoVertexField ? kRawNoField : offset; };
        RawBlock block{};
        block.format = {call.raw_stride, call.vertex_type,
                        field(format.weight_offset) | field(format.texcoord_offset) << 8u |
                            field(format.color_offset) << 16u | field(format.normal_offset) << 24u,
                        format.position_offset};
        // A vertex without a colour is drawn with the material colour when
        // unlit, white when lit (to_gpu).
        block.extra = {use_material_color ? call.material_color : 0xFFFFFFFFu, 0u, 0u, 0u};
        std::size_t bytes = kRawHeaderBytes;
        if (((call.vertex_type >> 9u) & 3u) != 0u && call.bone_matrices != nullptr) {
            const std::uint32_t bones = std::min<std::uint32_t>(((call.vertex_type >> 14u) & 7u) + 1u, 8u);
            for (std::uint32_t bone = 0; bone < bones; ++bone)
                for (std::uint32_t row = 0; row < 4u; ++row)
                    for (std::uint32_t axis = 0; axis < 3u; ++axis)
                        block.bones[bone * 16u + row * 4u + axis] = call.bone_matrices[bone * 12u + row * 3u + axis];
            bytes += bones * kRawBoneBytes;
        }
        if (impl.check_gpu_decode) {
            Impl::FrameSlot &frame = impl.slots[impl.slot];
            const std::uint32_t half = impl.slot * 2u * Impl::kCheckVertices * 3u;
            if (frame.check_used + call.raw_count <= Impl::kCheckVertices) {
                check_slot = half + frame.check_used * 3u;
                frame.check_used += call.raw_count;
                checked = true;
            } else {
                // Past the compared part: written, never read.
                check_slot = half + Impl::kCheckVertices * 3u;
            }
            block.extra[1] = check_slot;
        }
        if (impl.check_gpu_decode || !impl.raw_valid || bytes != impl.last_raw_bytes ||
            std::memcmp(&block, &impl.last_raw, bytes) != 0) {
            if (!impl.write_uniform(&block, bytes, impl.raw_offset)) return;
            impl.last_raw = block;
            impl.last_raw_bytes = bytes;
            impl.raw_valid = true;
        }
        if (checked) {
            Impl::FrameSlot &frame = impl.slots[impl.slot];
            frame.checks.push_back({check_slot, static_cast<std::uint32_t>(frame.check_expected.size()),
                                    call.raw_count, call.vertex_type});
            const std::size_t first = frame.check_expected.size();
            for (const Vertex &vertex : call.vertices) frame.check_expected.push_back(to_gpu(vertex));
            frame.check_drawn.resize(frame.check_expected.size(), 0u);
            for (const std::uint16_t index : impl.direct_indices)
                if (first + index < frame.check_drawn.size()) frame.check_drawn[first + index] = 1u;
        }
    }
    // A texture in a framebuffer the renderer drew is read from that render
    // target: the pixels never reach guest memory, which holds whatever was
    // there before. MHP3RD_NO_FB_TEXTURES decodes guest memory as before.
    static const bool no_fb_textures = std::getenv("MHP3RD_NO_FB_TEXTURES") != nullptr;
    const Impl::FramebufferTexture framebuffer_source =
        call.texture.enabled && !call.clear_mode && !no_fb_textures
            ? impl.find_framebuffer_texture(memory, call.texture)
            : Impl::FramebufferTexture{};

    // Under Fill the game's 480x272 screen is spread over a target of the
    // window's shape, which suits the 3D view the game now draws at that
    // shape, but would stretch the 2D interface. Its draws into the shown
    // framebuffer are pulled in about the screen's centre to square pixels
    // again. Draws that cover the screen's width (fades, backdrops) and
    // draws that sample a rendered picture (blur, the quest-reward
    // background) belong with the 3D view and stay spread.
    float fit_x = 1.0f, fit_y = 1.0f;
    bool fitted = false;
    if (call.through && !call.clear_mode && framebuffer_source.target == nullptr &&
        impl.shows(call.target.color_address) && impl.interface_fit(fit_x, fit_y)) {
        float left = impl.scratch.front().x, right = left;
        for (const GpuVertex &vertex : impl.scratch) {
            left = std::min(left, vertex.x);
            right = std::max(right, vertex.x);
        }
        if (right - left < kScreenWideDraw) {
            fitted = true;
            const float centre_x = 0.5f * static_cast<float>(kPspWidth);
            const float centre_y = 0.5f * static_cast<float>(kPspHeight);
            for (GpuVertex &vertex : impl.scratch) {
                vertex.x = centre_x + (vertex.x - centre_x) * fit_x;
                vertex.y = centre_y + (vertex.y - centre_y) * fit_y;
            }
        }
    }

    static const bool no_merge_env = std::getenv("MHP3RD_NO_DRAW_MERGE") != nullptr;
    const bool merge = !no_merge_env && !perf::alternate_off(perf::NewPath::Merge);
    // A skinned draw recorded for drawing again keeps a copy of its vertices,
    // to blend them with the next frame's; ge_state.cpp skins exactly the
    // transformed vertices whose type has weights.
    // A raw draw is blended through its bones instead (replay()).
    const bool skinned = impl.interpolating && !raw && !call.through && ((call.vertex_type >> 9u) & 3u) != 0u;
    const std::uint32_t skinned_first =
        skinned ? static_cast<std::uint32_t>(impl.recording_frame.skinned.size()) : Impl::kNone;
    VkDeviceSize vertex_start = impl.vertex_offset;
    VkDeviceSize index_start = 0u;
    VkDeviceSize draw_end = 0u;
    std::uint32_t draw_count = 0u;
    if (direct) {
        // Floats want 4-byte alignment and index buffer offsets a multiple of
        // the index size; a vertex run starts on 16 bytes. Merged draws keep
        // their indices in the index buffer, written once the draw's group is
        // known.
        vertex_start = (impl.vertex_offset + 15u) & ~VkDeviceSize{15u};
        // A draw that can join the open group follows its vertices directly,
        // so its indices rebase onto the group's first vertex by the stride.
        // A GpuVertex is 40 bytes: rounding its start up to 16 bytes as
        // before broke that for every other draw, and half the draws that
        // could have merged did not (MHP3RD_NO_TIGHT_MERGE rounds as before).
        static const bool loose_merge = std::getenv("MHP3RD_NO_TIGHT_MERGE") != nullptr;
        if (merge && impl.group.open && impl.group.raw == raw && impl.vertex_offset == impl.group.vertex_end &&
            (raw ? impl.group.stride == call.raw_stride && !impl.check_gpu_decode : !loose_merge))
            vertex_start = impl.vertex_offset;
        const VkDeviceSize vertex_bytes =
            raw ? static_cast<VkDeviceSize>(call.raw_count) * call.raw_stride : call.vertices.size() * sizeof(GpuVertex);
        const VkDeviceSize vertex_end = vertex_start + vertex_bytes;
        index_start = (vertex_end + 3u) & ~VkDeviceSize{3u};
        draw_end = merge ? vertex_end : index_start + impl.direct_indices.size() * sizeof(std::uint16_t);
        if (draw_end > impl.vertex_limit) {
            report_frame_space_full("vertex");
            return;
        }
        if (merge && impl.index_offset + 4u + impl.direct_indices.size() * sizeof(std::uint16_t) > impl.index_limit) {
            report_frame_space_full("index");
            return;
        }
        auto *out = static_cast<std::uint8_t *>(impl.vertex_mapped) + vertex_start;
        if (raw) std::memcpy(out, call.raw_vertices, static_cast<std::size_t>(vertex_bytes));
        else for (const Vertex &vertex : call.vertices) {
            const GpuVertex converted = to_gpu(vertex);
            std::memcpy(out, &converted, sizeof(GpuVertex));
            out += sizeof(GpuVertex);
            if (skinned) impl.recording_frame.skinned.push_back(converted);
        }
        if (!merge)
            std::memcpy(static_cast<std::uint8_t *>(impl.vertex_mapped) + index_start, impl.direct_indices.data(),
                        impl.direct_indices.size() * sizeof(std::uint16_t));
        draw_count = static_cast<std::uint32_t>(impl.direct_indices.size());
    } else {
        const VkDeviceSize bytes = impl.scratch.size() * sizeof(GpuVertex);
        if (impl.vertex_offset + bytes > impl.vertex_limit) {
            report_frame_space_full("vertex");
            return;
        }
        std::memcpy(static_cast<std::uint8_t *>(impl.vertex_mapped) + impl.vertex_offset, impl.scratch.data(),
                    static_cast<std::size_t>(bytes));
        draw_end = impl.vertex_offset + bytes;
        draw_count = static_cast<std::uint32_t>(impl.scratch.size());
        if (skinned)
            impl.recording_frame.skinned.insert(impl.recording_frame.skinned.end(), impl.scratch.begin(),
                                                impl.scratch.end());
    }
    const auto drawn_vertices = static_cast<std::uint32_t>(direct ? vertex_total : impl.scratch.size());

    PipelineKey key{};
    if (call.clear_mode) {
        // sceGuClear draws a screen-sized sprite with CLEARMODE on: blending,
        // the depth test and the texture are bypassed and bits 8..10 say which
        // buffers it is allowed to write. Treating it as an ordinary draw left
        // the depth buffer at whatever the allocation happened to contain.
        key.blend = false;
        // Vulkan ties depth writes to the depth test: with depthTestEnable false
        // the attachment is never updated, so the test has to stay on and always
        // pass for the clear to reach the depth buffer at all.
        key.depth_test = true;
        key.depth_function = 1u;  // always
        key.depth_write = (call.clear_flags & 4u) != 0u;
        key.cull = false;
        key.color_mask = ((call.clear_flags & 1u) != 0u ? (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                          VK_COLOR_COMPONENT_B_BIT)
                                                       : 0u) |
                         ((call.clear_flags & 2u) != 0u ? VK_COLOR_COMPONENT_A_BIT : 0u);
    } else {
        key.blend = call.blend.enabled;
        key.source_factor = resolve_fixed_factor(call.blend.source_factor, call.blend.fixed_source);
        key.destination_factor = resolve_fixed_factor(call.blend.destination_factor, call.blend.fixed_destination);
        // Both sides fixed is common for fog and steam, and one constant cannot
        // serve two colours. When the pair adds up to white the destination is
        // exactly the complement of the source, which Vulkan does express.
        if (key.source_factor == kFactorFixed && key.destination_factor == kFactorFixed &&
            ((call.blend.fixed_source + call.blend.fixed_destination) & 0x00FFFFFFu) == 0x00FFFFFFu)
            key.destination_factor = kFactorInverseConstant;
        key.equation = call.blend.equation;
        key.depth_test = call.depth.test_enabled && !call.through;
        key.depth_write = call.depth.write_enabled;
        key.depth_function = call.depth.function;
        key.cull = call.culling_enabled && !call.through && call.primitive != PrimitiveType::Sprites;
        key.cull_clockwise = call.cull_clockwise;
    }
    // Matches texture_params.w below: without an alpha test the pipeline's
    // shader has no discard. MHP3RD_NO_ALPHA_VARIANTS keeps the one shader
    // that tests alpha for every draw, as before.
    static const bool no_alpha_variants = std::getenv("MHP3RD_NO_ALPHA_VARIANTS") != nullptr;
    key.alpha_test = no_alpha_variants || perf::alternate_off(perf::NewPath::Alpha) ||
                     (!call.clear_mode && call.alpha_test.enabled && call.alpha_test.function != 0u);
    key.raw = raw;
    // Escape hatch for bisecting "nothing is visible" reports.
    static const bool no_cull = std::getenv("MHP3RD_NO_CULL") != nullptr;
    static const bool no_depth = std::getenv("MHP3RD_NO_DEPTH") != nullptr;
    if (no_cull) key.cull = false;
    if (no_depth && !call.clear_mode) key.depth_test = false;
    VkPipeline pipeline = impl.pipeline_for(key);
    if (pipeline == VK_NULL_HANDLE) return;

    PushConstants push{};
    push.transform = multiply(call.projection, view_world);
    push.viewport = {static_cast<float>(kPspWidth), static_cast<float>(kPspHeight), call.through ? 1.0f : 0.0f,
                     (fogged ? kPushFog : 0.0f) + (lit ? kPushLighting : 0.0f)};
    push.view_z = {view_world[2], view_world[6], view_world[10], view_world[14]};
    push.texture_params = {call.texture.enabled ? 1.0f : 0.0f, static_cast<float>(call.texture.function),
                           static_cast<float>(call.alpha_test.enabled ? call.alpha_test.reference : 0u),
                           static_cast<float>(call.alpha_test.enabled ? call.alpha_test.function : 0u)};
    // Through-mode texture coordinates are in texels, transformed ones in [0,1].
    push.uv_transform = call.through && call.texture.width != 0u
                            ? std::array<float, 4>{1.0f / static_cast<float>(call.texture.width),
                                                   1.0f / static_cast<float>(call.texture.height), 0.0f, 0.0f}
                            : std::array<float, 4>{call.texture.scale_u, call.texture.scale_v, call.texture.offset_u,
                                                   call.texture.offset_v};

    if (call.clear_mode) push.texture_params = {0.0f, 0.0f, 0.0f, 0.0f};

    static const bool trace_fb = std::getenv("MHP3RD_TRACE_FB_TEXTURES") != nullptr;
    if (trace_fb && call.texture.enabled && !call.clear_mode) impl.trace_framebuffer_texture(call);

    VkDescriptorSet texture_descriptor = impl.white_texture.descriptor;
    if (call.texture.enabled && !call.clear_mode) {
        const Impl::FramebufferTexture &source = framebuffer_source;
        const VkDescriptorSet copy =
            source.target != nullptr
                ? impl.framebuffer_descriptor(*source.target, call.texture.format == TextureFormat::Rgba5650)
                : VK_NULL_HANDLE;
        if (copy != VK_NULL_HANDLE) {
            // Texture coordinates address the game's texture, whose first texel
            // is pixel (x, y) of a target that holds 480x272 guest pixels at
            // any internal scale.
            texture_descriptor = copy;
            const float width = static_cast<float>(call.texture.width);
            const float height = static_cast<float>(call.texture.height);
            const std::array<float, 4> uv = push.uv_transform;
            push.uv_transform = {uv[0] * width / static_cast<float>(kPspWidth),
                                 uv[1] * height / static_cast<float>(kPspHeight),
                                 (uv[2] * width + static_cast<float>(source.x)) / static_cast<float>(kPspWidth),
                                 (uv[3] * height + static_cast<float>(source.y)) / static_cast<float>(kPspHeight)};
        } else {
            texture_descriptor = impl.texture_descriptor(memory, call);
        }
    }

    if (!impl.pass_active || impl.current_target != call.target.color_address) {
        impl.end_pass();
        // A clear that covers the whole screen, first in its pass, writes
        // every pixel of what it clears: nothing needs loading for it.
        std::uint32_t overwritten = 0u;
        static const bool no_clear_load = std::getenv("MHP3RD_NO_CLEAR_LOAD") != nullptr;
        if (call.clear_mode && call.through && call.primitive == PrimitiveType::Sprites && !no_clear_load &&
            !perf::alternate_off(perf::NewPath::ClearLoad) && call.viewport.scissor_x1 == 0u &&
            call.viewport.scissor_y1 == 0u && call.viewport.scissor_x2 >= kPspWidth - 1u &&
            call.viewport.scissor_y2 >= kPspHeight - 1u && call.vertices.size() == 2u) {
            const Vertex &a = call.vertices[0];
            const Vertex &b = call.vertices[1];
            const bool covers = std::min(a.position[0], b.position[0]) <= 0.0f &&
                                std::min(a.position[1], b.position[1]) <= 0.0f &&
                                std::max(a.position[0], b.position[0]) >= static_cast<float>(kPspWidth) &&
                                std::max(a.position[1], b.position[1]) >= static_cast<float>(kPspHeight);
            if (covers && call.indices.empty()) {
                if ((call.clear_flags & 3u) == 3u) overwritten |= 1u;
                if ((call.clear_flags & 4u) != 0u) overwritten |= 2u;
            }
        }
        impl.begin_pass(call.target.color_address, overwritten);
        if (!impl.pass_active) return;
    }
    {
        Impl::Target &drawn = impl.targets.at(call.target.color_address);
        const bool layout_changed =
            drawn.stride != call.target.color_stride || drawn.format != call.target.color_format;
        drawn.stride = call.target.color_stride;
        drawn.format = call.target.color_format;
        if (layout_changed || drawn.guest_words.empty() || drawn.last_drawn_frame != impl.frames)
            impl.snapshot_guest_words(memory, call.target.color_address, drawn);
        drawn.last_drawn_frame = impl.frames;
        drawn.draw_serial = ++impl.target_draw_counter;
    }
    // The GE viewport maps normalised device coordinates onto the screen as
    //   screen = ndc * scale + offset - region_offset
    // with a negative y scale (PSP device y points up, the screen down) and a z
    // scale/offset that drives the 16-bit depth buffer. Folding all of that into
    // the Vulkan viewport reproduces the PSP's screen space exactly, including a
    // reversed depth range, and keeps triangle winding as the GE sees it. Through
    // draws bypass the transform, so they keep the plain full-target viewport.
    // A target holds the game's 480x272 screen whatever its size, so a PSP
    // pixel is scale_x by scale_y of its pixels: equal at a multiple of
    // 480x272, different under Fill.
    const float scale_x = static_cast<float>(impl.target_extent.width) / static_cast<float>(kPspWidth);
    const float scale_y = static_cast<float>(impl.target_extent.height) / static_cast<float>(kPspHeight);
    VkViewport vk_viewport{0.0f, 0.0f, static_cast<float>(impl.target_extent.width),
                           static_cast<float>(impl.target_extent.height), 0.0f, 1.0f};
    if (!call.through && call.viewport.x_scale != 0.0f && call.viewport.y_scale != 0.0f) {
        const ViewportState &vp = call.viewport;
        vk_viewport.x = (vp.x_offset - vp.offset_x - vp.x_scale) * scale_x;
        vk_viewport.y = (vp.y_offset - vp.offset_y - vp.y_scale) * scale_y;
        vk_viewport.width = 2.0f * vp.x_scale * scale_x;
        // The GE's y scale is negative (device y points up, the screen down), so
        // this is a flipping viewport. That keeps framebuffer space identical to
        // the PSP's screen space, which is what the cull winding is defined in.
        vk_viewport.height = 2.0f * vp.y_scale * scale_y;
        if (vp.z_scale != 0.0f) {
            // The shader hands over device z in [0, 1]; this undoes that halving
            // and applies the GE's z scale/offset, reproducing a reversed range
            // when the guest set one with sceGuDepthRange(65535, 0).
            vk_viewport.minDepth = std::clamp((vp.z_offset - vp.z_scale) / 65535.0f, 0.0f, 1.0f);
            vk_viewport.maxDepth = std::clamp((vp.z_offset + vp.z_scale) / 65535.0f, 0.0f, 1.0f);
        }
    }

    // Whichever side asked for the constant decides its colour; the source wins
    // when both do, because the destination is then its complement.
    const std::uint32_t constant_color =
        key.source_factor == kFactorFixed ? call.blend.fixed_source : call.blend.fixed_destination;
    const std::array<float, 4> blend_constants{static_cast<float>(constant_color & 0xFFu) / 255.0f,
                                               static_cast<float>((constant_color >> 8u) & 0xFFu) / 255.0f,
                                               static_cast<float>((constant_color >> 16u) & 0xFFu) / 255.0f, 1.0f};

    const auto clamp_axis = [](std::uint32_t value, std::uint32_t limit) { return std::min(value, limit); };
    const std::uint32_t sx1 = clamp_axis(call.viewport.scissor_x1, kPspWidth - 1u);
    const std::uint32_t sy1 = clamp_axis(call.viewport.scissor_y1, kPspHeight - 1u);
    const std::uint32_t sx2 = clamp_axis(std::max(call.viewport.scissor_x2, sx1), kPspWidth - 1u);
    const std::uint32_t sy2 = clamp_axis(std::max(call.viewport.scissor_y2, sy1), kPspHeight - 1u);
    // The scissor's edges, in PSP pixels; a fitted draw's scissor is fitted
    // with it, so a list the interface clips still clips at its own edges.
    float edges[4]{static_cast<float>(sx1), static_cast<float>(sy1), static_cast<float>(sx2 + 1u),
                   static_cast<float>(sy2 + 1u)};
    if (fitted) {
        const float centre_x = 0.5f * static_cast<float>(kPspWidth);
        const float centre_y = 0.5f * static_cast<float>(kPspHeight);
        edges[0] = centre_x + (edges[0] - centre_x) * fit_x;
        edges[2] = centre_x + (edges[2] - centre_x) * fit_x;
        edges[1] = centre_y + (edges[1] - centre_y) * fit_y;
        edges[3] = centre_y + (edges[3] - centre_y) * fit_y;
    }
    const auto to_pixels = [](float edge, float scale, std::uint32_t size) {
        return static_cast<std::int32_t>(
            std::clamp(std::lround(edge * scale), 0l, static_cast<long>(size)));
    };
    const std::int32_t left = to_pixels(edges[0], scale_x, impl.target_extent.width);
    const std::int32_t top = to_pixels(edges[1], scale_y, impl.target_extent.height);
    const std::int32_t right = std::max(left, to_pixels(edges[2], scale_x, impl.target_extent.width));
    const std::int32_t bottom = std::max(top, to_pixels(edges[3], scale_y, impl.target_extent.height));
    VkRect2D vk_scissor{};
    vk_scissor.offset = {left, top};
    vk_scissor.extent = {static_cast<std::uint32_t>(right - left), static_cast<std::uint32_t>(bottom - top)};
    const std::array<std::uint32_t, 3> lighting_offsets{impl.environment_offset, impl.object_offset, impl.raw_offset};
    if (merge) {
        const Impl::DrawState state{pipeline, texture_descriptor, lighting_offsets, vk_viewport, vk_scissor,
                                    blend_constants, push};
        if (!direct) {
            impl.flush_group();
            impl.record_state(state);
            {
                const perf::SplitScope split(perf::Split::Record);
                vkCmdBindVertexBuffers(impl.command_buffer, 0u, 1u, &impl.vertex_buffer, &vertex_start);
                vkCmdDraw(impl.command_buffer, draw_count, 1u, 0u, 0u);
            }
            perf::count_recorded_draws(1u);
            impl.record_draw_for_replay(call, lit, skinned_first, state, vertex_start, VK_NULL_HANDLE, 0u,
                                        draw_count, drawn_vertices, false);
        } else {
            Impl::DrawGroup &group = impl.group;
            const auto vertex_count = static_cast<std::uint32_t>(vertex_total);
            std::uint32_t rebase = 0u;
            bool join = group.open && vertex_start == group.vertex_end && impl.state_known &&
                        Impl::same_state(state, impl.recorded) && group.raw == raw &&
                        (!raw || group.stride == call.raw_stride) && !impl.check_gpu_decode;
            // Recorded for drawing again, skinned draws join only skinned
            // ones: their vertices are kept on the CPU too, one after another
            // like the group's, so blended ones can replace the group's.
            if (join && impl.interpolating && skinned != impl.group_skinned) join = false;
            if (join) {
                rebase = static_cast<std::uint32_t>((vertex_start - group.vertex_base) /
                                                    (raw ? call.raw_stride : sizeof(GpuVertex)));
                join = rebase + vertex_count <= 65536u;
            }
            if (!join) {
                impl.flush_group();
                impl.record_state(state);
                // Metal wants index buffer offsets on 4 bytes.
                impl.index_offset = (impl.index_offset + 3u) & ~VkDeviceSize{3u};
                group = Impl::DrawGroup{true, vertex_start, vertex_start, impl.index_offset, 0u, 0u, raw, call.raw_stride};
                rebase = 0u;
                impl.group_skinned = skinned;
            }
            impl.record_draw_for_replay(call, lit, skinned_first, state, vertex_start, impl.index_buffer,
                                        group.index_base, draw_count, vertex_count, join, raw);
            auto *indices = reinterpret_cast<std::uint8_t *>(impl.index_mapped) + impl.index_offset;
            if (rebase == 0u) {
                std::memcpy(indices, impl.direct_indices.data(), impl.direct_indices.size() * sizeof(std::uint16_t));
            } else {
                for (std::uint16_t &index : impl.direct_indices) index = static_cast<std::uint16_t>(index + rebase);
                std::memcpy(indices, impl.direct_indices.data(), impl.direct_indices.size() * sizeof(std::uint16_t));
            }
            impl.index_offset += impl.direct_indices.size() * sizeof(std::uint16_t);
            group.vertex_end = draw_end;
            group.index_count += draw_count;
            ++group.draws;
        }
        impl.vertex_offset = draw_end;
        ++impl.draws;
        perf::count_draw();
        return;
    }

    vkCmdSetViewport(impl.command_buffer, 0u, 1u, &vk_viewport);
    vkCmdSetBlendConstants(impl.command_buffer, blend_constants.data());
    vkCmdSetScissor(impl.command_buffer, 0u, 1u, &vk_scissor);

    if (pipeline != impl.bound_pipeline) {
        vkCmdBindPipeline(impl.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        impl.bound_pipeline = pipeline;
    }
    vkCmdBindDescriptorSets(impl.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, impl.pipeline_layout, 0u, 1u,
                            &texture_descriptor, 0u, nullptr);
    // Every pipeline shares one layout, so set 1 stays bound across pipeline
    // and texture changes; it is bound again only when a block moved.
    if (!impl.lighting_bound || lighting_offsets != impl.bound_lighting_offsets) {
        vkCmdBindDescriptorSets(impl.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, impl.pipeline_layout, 1u, 1u,
                                &impl.lighting_descriptor, static_cast<std::uint32_t>(lighting_offsets.size()),
                                lighting_offsets.data());
        impl.bound_lighting_offsets = lighting_offsets;
        impl.lighting_bound = true;
    }
    vkCmdPushConstants(impl.command_buffer, impl.pipeline_layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0u, sizeof(push), &push);
    vkCmdBindVertexBuffers(impl.command_buffer, 0u, 1u, &impl.vertex_buffer, &vertex_start);
    if (direct) {
        vkCmdBindIndexBuffer(impl.command_buffer, impl.vertex_buffer, index_start, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(impl.command_buffer, draw_count, 1u, 0u, 0, raw ? static_cast<std::uint32_t>(vertex_start) : 0u);
    } else {
        vkCmdDraw(impl.command_buffer, draw_count, 1u, 0u, 0u);
    }
    if (impl.interpolating) {
        const Impl::DrawState state{pipeline, texture_descriptor, lighting_offsets, vk_viewport, vk_scissor,
                                    blend_constants, push};
        impl.record_draw_for_replay(call, lit, skinned_first, state, vertex_start,
                                    direct ? impl.vertex_buffer : VK_NULL_HANDLE, index_start, draw_count,
                                    drawn_vertices, false, raw);
    }
    impl.vertex_offset = draw_end;
    ++impl.draws;
    perf::count_draw();
    perf::count_recorded_draws(1u);
}

void VulkanRenderer::write_back_frame(GuestMemory &memory) {
    Impl &impl = *impl_;
    if (!impl.ready) return;
    // The frame before this one, still in flight with two slots: its
    // pixels reach guest memory now, as they did when each frame was waited
    // for before the next was recorded.
    if (impl.slot_count > 1u) {
        const std::uint32_t previous = (impl.slot + impl.slot_count - 1u) % impl.slot_count;
        if (previous != impl.slot) impl.collect_writeback(previous, true);
    }
    if (!impl.writeback_has_pixels) return;
    impl.writeback_has_pixels = false;
    const perf::SplitScope split(perf::Split::Writeback);
    const perf::Clock::time_point store_start = perf::Clock::now();
    impl.store_frame(memory, impl.writeback_ready, impl.writeback_pixels.data());
    perf::note_stall(perf::Stall::Store, perf::Clock::now() - store_start);
}

void VulkanRenderer::read_back_framebuffer(std::uint32_t source, GuestMemory &memory) {
    Impl &impl = *impl_;
    static const bool disabled = std::getenv("MHP3RD_NO_FB_TEXTURES") != nullptr;
    if (!impl.ready || disabled) return;
    const std::uint32_t wanted = GuestMemory::canonical(source);
    std::uint32_t found = 0u;
    bool any = false;
    for (const auto &[address, target] : impl.targets) {
        if (!target.initialized || target.guest_words.empty()) continue;
        const std::uint32_t base = GuestMemory::canonical(address);
        const std::uint32_t bytes = target.stride * kPspHeight * framebuffer_bytes_per_pixel(target.format);
        if (wanted >= base && wanted - base < bytes) {
            found = address;
            any = true;
            break;
        }
    }
    if (!any) return;
    if (!impl.recording) begin_frame();
    impl.end_pass();
    impl.record_writeback(found);
    Impl::FrameSlot &frame = impl.slots[impl.slot];
    if (!frame.writeback_in_flight) return;
    // Run everything recorded so far and carry on recording afterwards.
    impl.end_gpu_segment(impl.command_buffer);
    vkEndCommandBuffer(impl.command_buffer);
    std::array<VkCommandBuffer, 2> batch{};
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = impl.frame_batch(impl.command_buffer, batch);
    submit.pCommandBuffers = batch.data();
    const perf::Clock::time_point wait_start = perf::Clock::now();
    vkQueueSubmit(impl.queue, 1u, &submit, impl.frame_fence);
    // The frame before, submitted earlier, has finished too; its write-back
    // is taken first, as it is older than this one.
    for (std::uint32_t other = 0; other < impl.slot_count; ++other)
        if (other != impl.slot) impl.collect_writeback(other, true);
    impl.wait_fence(impl.frame_fence, "a framebuffer read back");
    perf::add_wait_time(perf::Clock::now() - wait_start, perf::Stall::Readback);
    vkResetFences(impl.device, 1u, &impl.frame_fence);
    // Uploads and evictions recorded so far have finished with the frame.
    impl.release_frame_uploads(impl.slot);
    vkResetCommandBuffer(impl.command_buffer, 0u);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(impl.command_buffer, &begin);
    impl.breadcrumb(impl.command_buffer, "frame goes on after a read-back");
    // The queries written so far stay valid: they were reset at the start of
    // the frame, and the next pair follows them.
    impl.begin_gpu_segment(impl.command_buffer);
    // A frame recorded for drawing again keeps what it wrote so far.
    if (!impl.interpolating) impl.enter_region(impl.frame_region);
    impl.environment_version = 0u;
    impl.object_valid = false;
    impl.raw_valid = false;
    impl.forget_bindings();
    frame.writeback_in_flight = false;
    const perf::Clock::time_point copy_start = perf::Clock::now();
    std::vector<std::uint32_t> fresh_pixels;
    std::vector<std::uint32_t> &pixels = Impl::reuse_buffers() ? impl.readback_pixels : fresh_pixels;
    pixels.resize(static_cast<std::size_t>(kPspWidth) * kPspHeight);
    impl.invalidate_writeback(frame.writeback.memory);
    std::memcpy(pixels.data(), frame.writeback.mapped, pixels.size() * 4u);
    const perf::Clock::time_point store_start = perf::Clock::now();
    perf::note_stall(perf::Stall::Copy, store_start - copy_start);
    impl.store_frame(memory, frame.writeback_recorded, pixels.data());
    perf::note_stall(perf::Stall::Store, perf::Clock::now() - store_start);
    // A write-back still waiting from an earlier frame is older than this one.
    if (impl.writeback_ready.address == found) impl.writeback_has_pixels = false;
    static const bool trace = std::getenv("MHP3RD_TRACE_FB_TEXTURES") != nullptr;
    if (trace)
        std::cout << "[fbtex] frame " << impl.frames << " read framebuffer 0x" << std::hex << found << std::dec
                  << " back for a block transfer\n";
}

namespace {

// 480x272 RGBA pixels (red in the low byte) into guest framebuffer rows of
// `stride` pixels in `format`: 8888 copied, the 16-bit formats packed a row at
// a time into a buffer the compiler vectorises. The same bits as the
// per-pixel conversion in store_frame().
void store_rows(std::uint8_t *out, std::uint32_t stride, std::uint32_t format, const std::uint32_t *pixels) {
    const std::size_t bytes_per_pixel = format == 3u ? 4u : 2u;
    std::array<std::uint16_t, kPspWidth> packed{};
    for (std::uint32_t y = 0; y < kPspHeight; ++y) {
        const std::uint32_t *row = pixels + static_cast<std::size_t>(y) * kPspWidth;
        std::uint8_t *line = out + static_cast<std::size_t>(y) * stride * bytes_per_pixel;
        if (format == 3u) {
            std::memcpy(line, row, static_cast<std::size_t>(kPspWidth) * 4u);
            continue;
        }
        if (format == 0u) {
            for (std::uint32_t x = 0; x < kPspWidth; ++x) {
                const std::uint32_t pixel = row[x];
                packed[x] = static_cast<std::uint16_t>(((pixel & 0xFFu) >> 3u) | (((pixel >> 8u) & 0xFFu) >> 2u) << 5u |
                                                       (((pixel >> 16u) & 0xFFu) >> 3u) << 11u);
            }
        } else if (format == 1u) {
            for (std::uint32_t x = 0; x < kPspWidth; ++x) {
                const std::uint32_t pixel = row[x];
                packed[x] = static_cast<std::uint16_t>(((pixel & 0xFFu) >> 3u) | (((pixel >> 8u) & 0xFFu) >> 3u) << 5u |
                                                       (((pixel >> 16u) & 0xFFu) >> 3u) << 10u |
                                                       (pixel >> 31u) << 15u);
            }
        } else {
            for (std::uint32_t x = 0; x < kPspWidth; ++x) {
                const std::uint32_t pixel = row[x];
                packed[x] = static_cast<std::uint16_t>(((pixel & 0xFFu) >> 4u) | (((pixel >> 8u) & 0xFFu) >> 4u) << 4u |
                                                       (((pixel >> 16u) & 0xFFu) >> 4u) << 8u |
                                                       (pixel >> 28u) << 12u);
            }
        }
        // Guest memory is little-endian, as every host this runs on.
        static_assert(std::endian::native == std::endian::little);
        std::memcpy(line, packed.data(), static_cast<std::size_t>(kPspWidth) * 2u);
    }
}

} // namespace

void VulkanRenderer::Impl::store_frame(GuestMemory &memory, const WritebackFrame &frame,
                                       const std::uint32_t *pixels) {
    Impl &impl = *this;
    const std::uint32_t bytes_per_pixel = framebuffer_bytes_per_pixel(frame.format);
    const std::size_t bytes = static_cast<std::size_t>(frame.stride) * kPspHeight * bytes_per_pixel;
    std::uint8_t *out = memory.raw_pointer(frame.address, bytes);
    if (out == nullptr) return;
    // MHP3RD_NO_FAST_STORE converts pixel by pixel into guest memory, as
    // before, instead of a whole row at a time into a buffer the compiler can
    // vectorise; the bytes written are the same.
    static const bool slow_store = std::getenv("MHP3RD_NO_FAST_STORE") != nullptr;
    if (!slow_store && !perf::alternate_off(perf::NewPath::Store)) {
        store_rows(out, frame.stride, frame.format, pixels);
        const auto target = impl.targets.find(frame.address);
        if (target != impl.targets.end()) impl.snapshot_guest_words(memory, frame.address, target->second);
        return;
    }
    for (std::uint32_t y = 0; y < kPspHeight; ++y) {
        const std::uint32_t *row = pixels + static_cast<std::size_t>(y) * kPspWidth;
        std::uint8_t *line = out + static_cast<std::size_t>(y) * frame.stride * bytes_per_pixel;
        if (frame.format == 3u) {
            std::memcpy(line, row, static_cast<std::size_t>(kPspWidth) * 4u);
            continue;
        }
        for (std::uint32_t x = 0; x < kPspWidth; ++x) {
            // Red in the low bits, as texture_decode's expand_* read them.
            const std::uint32_t pixel = row[x];
            const std::uint32_t r = pixel & 0xFFu;
            const std::uint32_t g = (pixel >> 8u) & 0xFFu;
            const std::uint32_t b = (pixel >> 16u) & 0xFFu;
            const std::uint32_t a = pixel >> 24u;
            std::uint32_t value = 0u;
            if (frame.format == 0u)
                value = (r >> 3u) | (g >> 2u) << 5u | (b >> 3u) << 11u;
            else if (frame.format == 1u)
                value = (r >> 3u) | (g >> 3u) << 5u | (b >> 3u) << 10u | (a >> 7u) << 15u;
            else
                value = (r >> 4u) | (g >> 4u) << 4u | (b >> 4u) << 8u | (a >> 4u) << 12u;
            line[x * 2u] = static_cast<std::uint8_t>(value & 0xFFu);
            line[x * 2u + 1u] = static_cast<std::uint8_t>(value >> 8u);
        }
    }
    // The bytes now differ from the snapshot taken when the target was drawn,
    // and are exactly what it shows: take the snapshot again.
    const auto target = impl.targets.find(frame.address);
    if (target != impl.targets.end()) impl.snapshot_guest_words(memory, frame.address, target->second);
}

// Frame interpolation ------------------------------------------------------

namespace {

std::int64_t to_us(std::chrono::steady_clock::time_point time) {
    return std::chrono::duration_cast<std::chrono::microseconds>(time.time_since_epoch()).count();
}

} // namespace

double VulkanRenderer::Impl::wanted_rate() const {
    double rate = 30.0;
    switch (frame_rate) {
    case settings::FrameRate::Fps30: rate = 30.0; break;
    case settings::FrameRate::Fps45: rate = 45.0; break;
    case settings::FrameRate::Fps60: rate = 60.0; break;
    case settings::FrameRate::Fps90: rate = 90.0; break;
    case settings::FrameRate::Fps120: rate = 120.0; break;
    case settings::FrameRate::Display: rate = display_hz >= 1.0f ? static_cast<double>(display_hz) : 60.0; break;
    }
    // With vsync the display takes no more than its own rate: more presents
    // would wait for it with the game's time.
    if (present_mode == VK_PRESENT_MODE_FIFO_KHR && display_hz >= 1.0f)
        rate = std::min(rate, static_cast<double>(display_hz));
    return std::max(rate, 30.0);
}

bool VulkanRenderer::Impl::interpolation_wanted() const {
    // Emulated time running ahead of real time already presents faster than
    // the game's own rate; the keyboard's held frame is shown as it is.
    return frame_rate != settings::FrameRate::Fps30 && !settings::current().unthrottled && !holding && !fast_forward &&
           governor.rate() > 30.5;
}

// Keeps what drawing a draw again needs: a new group for a draw call the
// frame recorded, or one more draw in the group it joined.
void VulkanRenderer::Impl::record_draw_for_replay(const DrawCall &call, bool lit, std::uint32_t skinned,
                                                  const DrawState &state, VkDeviceSize vertex_start,
                                                  VkBuffer indices, VkDeviceSize index_start, std::uint32_t count,
                                                  std::uint32_t vertex_count, bool joined, bool raw) {
    if (!interpolating) return;
    FrameRecord &frame = recording_frame;
    if (!frame.recorded) return;
    const perf::SplitScope split(perf::Split::Summary);
    if (joined && !frame.groups.empty()) {
        ReplayGroup &last = frame.groups.back();
        // The copies must follow each other as the vertices do; a draw given
        // up after its vertices were copied breaks that, and the group is
        // then drawn with its own vertices.
        if (last.skinned != kNone && skinned != last.skinned + last.vertex_count) last.skinned = kNone;
        last.count += count;
        last.vertex_count += vertex_count;
    } else {
        ReplayGroup added{};
        added.state = state;
        added.target = call.target.color_address;
        added.vertex_base = vertex_start;
        added.index_buffer = indices;
        added.index_base = index_start;
        added.count = count;
        added.vertex_count = vertex_count;
        added.first_draw = static_cast<std::uint32_t>(frame.summaries.size());
        if (lit) {
            if (frame.objects.empty() || frame.object_offset != object_offset) {
                frame.objects.push_back(last_object);
                frame.object_offset = object_offset;
            }
            added.object = static_cast<std::uint32_t>(frame.objects.size() - 1u);
        }
        added.skinned = skinned;
        added.raw = raw;
        if (raw && ((call.vertex_type >> 9u) & 3u) != 0u) {
            // A skinned raw group is blended through its bones.
            if (frame.raws.empty() || frame.raw_offset != raw_offset) {
                frame.raws.push_back(last_raw);
                frame.raw_offset = raw_offset;
            }
            added.raw_block = static_cast<std::uint32_t>(frame.raws.size() - 1u);
        }
        frame.groups.push_back(added);
    }
    ++frame.groups.back().draws;
    frame.summaries.push_back(interpolation::summarize(call));
    frame.group_of.push_back(static_cast<std::uint32_t>(frame.groups.size() - 1u));
}

// Ends the frame's recording and submits it without presenting it: with
// frame interpolation the presents between flips show it.
void VulkanRenderer::Impl::submit_frame() {
    const perf::SplitScope split(perf::Split::Present);
    end_pass();
    end_gpu_segment(command_buffer);
    slots[slot].gpu_timer_pending = gpu_timer_used;
    vkEndCommandBuffer(command_buffer);
    std::array<VkCommandBuffer, 2> batch{};
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = frame_batch(command_buffer, batch);
    submit.pCommandBuffers = batch.data();
    const perf::Clock::time_point submit_start = perf::Clock::now();
    if (vkQueueSubmit(queue, 1u, &submit, frame_fence) == VK_ERROR_DEVICE_LOST) device_lost("a frame's submit");
    perf::add_wait_time(perf::Clock::now() - submit_start, perf::Stall::Submit);
    recording = false;
}

// The frame the game just flipped becomes the newer one: its picture is
// copied, and it is matched against the frame before.
void VulkanRenderer::Impl::finish_interpolated_frame(VkImage source, std::uint32_t displayed,
                                                     std::int64_t moment_us) {
    const perf::SplitScope split(perf::Split::Interp);
    FrameRecord &frame = recording_frame;
    frame.recorded = frame.recorded && interpolating;
    interpolation::mark_eligible(frame.summaries, displayed);
    frame.displayed = displayed;
    frame.moment_us = moment_us;
    frame.valid = true;
    std::swap(older_frame, newer_frame);
    std::swap(newer_frame, recording_frame);
    recording_frame.clear();
    next_region = (frame_region + 1u) % kFrameRegions;

    // The picture, before the game draws anything else into its target.
    std::string error;
    Target &picture = pictures[newer_picture ^ 1u];
    if (picture.color == VK_NULL_HANDLE && !create_target(picture, error)) {
        std::cout << "[render] frame interpolation has no room for its pictures: " << error << "\n";
        destroy_target(picture);
        newer_picture_valid = older_picture_valid = false;
        return;
    }
    newer_picture ^= 1u;
    older_picture_valid = newer_picture_valid;
    initialize_layouts(command_buffer, picture);
    transition(command_buffer, source, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    transition(command_buffer, picture.color, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageCopy copy{};
    copy.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    copy.dstSubresource = copy.srcSubresource;
    copy.extent = {target_extent.width, target_extent.height, 1u};
    vkCmdCopyImage(command_buffer, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, picture.color,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &copy);
    perf::count_target_copy();
    transition(command_buffer, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    transition(command_buffer, picture.color, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    newer_picture_valid = true;

    if (older_frame.valid && older_frame.recorded && newer_frame.recorded) {
        matching = matcher.match(older_frame.summaries, newer_frame.summaries, cut_thresholds);
    } else {
        matcher.forget_motion();
        matching = interpolation::Matching{};
        matching.cut = older_frame.valid ? "not recorded" : "no frame before";
    }
    camera_motion = matching.camera_found ? interpolation::rigid_motion(matching.camera) : interpolation::RigidMotion{};
    InterpolationStats &stats = interpolation_stats;
    ++stats.frames;
    stats.eligible += matching.eligible_newer;
    stats.matched += matching.matched;
    if (matching.continued) ++stats.continued;
    stats.rejected += matching.rejected;
    stats.rejected_shared += matching.rejected_shared;
    stats.max_own_motion = std::max(stats.max_own_motion, matching.max_own_motion);
    if (matching.camera_found) {
        stats.max_camera_angle = std::max(stats.max_camera_angle, matching.camera_angle_degrees);
        stats.max_camera_distance = std::max(stats.max_camera_distance, matching.camera_distance);
    }
    if (matching.cut != nullptr) ++stats.cuts[matching.cut];
    static const bool trace_frames = [] {
        const char *text = std::getenv("MHP3RD_TRACE_INTERPOLATION");
        return text != nullptr && (std::strcmp(text, "frames") == 0 || std::strcmp(text, "presents") == 0);
    }();
    if (trace_frames) {
        std::printf("[interp] flip %llu at moment %lld us (%+.1f ms after the one before), %.1f ms after it\n",
                    static_cast<unsigned long long>(frames), static_cast<long long>(moment_us),
                    older_frame.valid ? static_cast<double>(moment_us - older_frame.moment_us) / 1000.0 : 0.0,
                    static_cast<double>(to_us(std::chrono::steady_clock::now()) - moment_us) / 1000.0);
    }
    if (trace_frames || (trace_interpolation && matching.cut != nullptr && matching.eligible_newer != 0u) ||
        (trace_interpolation && matching.rejected >= 50u)) {
        std::printf("[interp] frame %llu: eligible %u/%u matched %u camera %.2f deg %.2f units, rejected %u (%u "
                    "shared, up to %.0f units, kept up to %.0f)%s%s%s\n",
                    static_cast<unsigned long long>(frames), matching.eligible_older, matching.eligible_newer,
                    matching.matched, static_cast<double>(matching.camera_angle_degrees),
                    static_cast<double>(matching.camera_distance), matching.rejected, matching.rejected_shared,
                    static_cast<double>(matching.max_rejected_motion), static_cast<double>(matching.max_own_motion),
                    matching.continued ? " (continued)" : "",
                    matching.cut != nullptr ? " cut: " : "", matching.cut != nullptr ? matching.cut : "");
        std::fflush(stdout);
    }
}

// Presents the one present that is due at `now`, if any. `account`: its
// time is added to the frame statistics, which the callers inside the
// renderer's own timed work do not want.
bool VulkanRenderer::Impl::poll_presents(std::chrono::steady_clock::time_point now, bool account, bool idle) {
    if (!cycle_active) return false;
    const auto due = present_clock.next_due();
    if (!due || *due > to_us(now)) return false;
    if (!idle && busy_presents * 2 > std::chrono::microseconds(present_clock.frame_us())) {
        // Presents made while the game's code ran have taken half a frame:
        // the rest wait for the kernel's idle time or the next flip.
        ++interpolation_stats.over_budget;
        (void)present_clock.take(to_us(now));
        return false;
    }
    const auto present = present_clock.take(to_us(now));
    if (!present) return false;
    const auto start = std::chrono::steady_clock::now();
    const bool made = present_between(*present, account);
    if (!idle) busy_presents += std::chrono::steady_clock::now() - start;
    return made;
}

bool VulkanRenderer::Impl::present_between(const pacing::PresentClock::Present &present, bool account) {
    const perf::SplitScope split(perf::Split::Present);
    using Clock = std::chrono::steady_clock;
    const Clock::time_point start = Clock::now();
    InterpolationStats &stats = interpolation_stats;
    stats.skipped += present.skipped;
    stats.max_late = std::max(stats.max_late, Clock::duration(std::chrono::microseconds(
                                                  std::max<std::int64_t>(0, to_us(start) - present.time_us -
                                                                                present_clock.delay_us()))));
    const std::uint32_t slot = present_slot;
    present_slot ^= 1u;
    VkCommandBuffer commands = present_commands[slot];
    VkFence fence = present_fences[slot];
    const perf::Clock::time_point wait_start = perf::Clock::now();
    wait_fence(fence, "the wait for a present slot");
    perf::add_wait_time(perf::Clock::now() - wait_start, perf::Stall::Fence);
    if (present_timed[slot]) {
        std::array<std::uint64_t, 4> results{};
        if (vkGetQueryPoolResults(device, present_timer, slot * 2u, 2u, sizeof(results), results.data(),
                                  2u * sizeof(std::uint64_t),
                                  VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) == VK_SUCCESS &&
            results[1] != 0u && results[3] != 0u) {
            const double ms =
                static_cast<double>((results[2] - results[0]) & gpu_timer_mask) * gpu_timer_ns_per_tick / 1.0e6;
            perf::add_gpu_time(ms);
            stats.gpu_ms += ms;
            ++stats.gpu_samples;
        }
        present_timed[slot] = false;
    }
    // An image first, waiting at most 3 ms for one: when the display has
    // none free (it refreshes slower than the presents come), this present
    // is dropped rather than hold the game until the next refresh.
#if defined(__ANDROID__)
    check_native_window();
    if (surface_returned) reset_surface();
    if (surface_lost) return false;
#endif
    check_swapchain();
    if (swapchain_dirty || swapchain == VK_NULL_HANDLE) recreate_swapchain();
    if (swapchain == VK_NULL_HANDLE) return false;
    std::uint32_t image_index = 0u;
    const perf::Clock::time_point acquire_start = perf::Clock::now();
    const VkResult acquired =
        vkAcquireNextImageKHR(device, swapchain, 3'000'000u, acquire_semaphore(commands), VK_NULL_HANDLE, &image_index);
    perf::add_wait_time(perf::Clock::now() - acquire_start, perf::Stall::Acquire);
    if (acquired == VK_ERROR_OUT_OF_DATE_KHR) swapchain_dirty = true;
    if (acquired == VK_SUBOPTIMAL_KHR) swapchain_check = true;
#if defined(__ANDROID__)
    if (acquired == VK_ERROR_SURFACE_LOST_KHR) surface_returned = true;
#endif
    if (acquired != VK_SUCCESS && acquired != VK_SUBOPTIMAL_KHR) {
        ++stats.blocked;
        if (account) perf::add_render_time(Clock::now() - start);
        return false;
    }
    acquired_image = image_index;
    vkResetFences(device, 1u, &fence);
    present_regions[slot] = 0u;
    vkResetCommandBuffer(commands, 0u);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commands, &begin);

    const Target &newer_image = pictures[newer_picture];
    const Target &older_image = pictures[newer_picture ^ 1u];
    VkImage image = newer_image.color;
    bool blended = false;
    if (present.t >= 0.999f || !older_picture_valid) ++stats.plain_newest;
    if (present.t < 0.999f && older_picture_valid) {
        image = older_image.color;
        const bool paired = matching.cut == nullptr && older_frame.valid && older_frame.recorded && newer_frame.recorded;
        const bool textures_kept = destroyed_texture_clock <= older_frame.texture_clock;
        const bool can_blend = present.t > 0.001f && paired && textures_kept;
        if (present.t <= 0.001f) ++stats.plain_oldest;
        else if (!paired) ++stats.plain_cut;
        else if (!textures_kept) ++stats.plain_textures;
        std::string error;
        Target &target = blend_targets[slot];
        if (can_blend && (target.color != VK_NULL_HANDLE || create_target(target, error))) {
            if (present_timer != VK_NULL_HANDLE) {
                vkCmdResetQueryPool(commands, present_timer, slot * 2u, 2u);
                vkCmdWriteTimestamp(commands, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, present_timer, slot * 2u);
            }
            const Clock::time_point replay_start = Clock::now();
            replay(commands, slot, present.t);
            present_regions[slot] = replay_regions;
            stats.replay_time += Clock::now() - replay_start;
            if (present_timer != VK_NULL_HANDLE) {
                vkCmdWriteTimestamp(commands, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, present_timer, slot * 2u + 1u);
                present_timed[slot] = true;
            }
            image = target.color;
            blended = true;
        } else if (!error.empty()) {
            std::cout << "[render] frame interpolation has no room to blend: " << error << "\n";
            destroy_target(target);
        }
    }
    ui_draw_data = frame_ui;
    submit_and_present(commands, fence, image, true, false);
    // MHP3RD_INTERPOLATION_EXTRA_MS: busy CPU time added to each blended
    // present, to try the step-down on a fast machine as if it were a slow one.
    static const double extra_ms = [] {
        const char *text = std::getenv("MHP3RD_INTERPOLATION_EXTRA_MS");
        return text != nullptr ? std::clamp(std::strtod(text, nullptr), 0.0, 50.0) : 0.0;
    }();
    if (blended && extra_ms > 0.0) {
        const Clock::time_point until =
            Clock::now() + std::chrono::microseconds(static_cast<std::int64_t>(extra_ms * 1000.0));
        while (Clock::now() < until) {
        }
    }

    const Clock::duration spent = Clock::now() - start;
    static const bool trace_presents = [] {
        const char *text = std::getenv("MHP3RD_TRACE_INTERPOLATION");
        return text != nullptr && std::strcmp(text, "presents") == 0;
    }();
    if (trace_presents)
        std::printf("[interp] present at moment %+.1f ms, t %.3f, %s, late %.1f ms, skipped %u, %.2f ms\n",
                    static_cast<double>(present.time_us - newer_frame.moment_us) / 1000.0,
                    static_cast<double>(present.t),
                    blended ? "blended" : image == newer_image.color ? "newer" : "older",
                    static_cast<double>(to_us(start) - present.time_us - present_clock.delay_us()) / 1000.0,
                    present.skipped, std::chrono::duration<double, std::milli>(spent).count());
    ++stats.presents;
    if (blended) {
        ++stats.blended;
        stats.blend_time += spent;
    } else {
        stats.plain_time += spent;
    }
    if (account) perf::add_render_time(spent);
    perf::count_present();
    return true;
}

// Draws the older frame into the slot's blend target with every matched
// draw's transforms blended a fraction `t` towards the newer frame's. Draws
// without a partner in the newer frame, and those never blended (2D, the
// interface, other framebuffers' content), are drawn as the older frame drew
// them.
void VulkanRenderer::Impl::replay(VkCommandBuffer commands, std::uint32_t slot, float t) {
    const perf::SplitScope split(perf::Split::Replay);
    breadcrumb(commands, "blended present, " + std::to_string(older_frame.groups.size()) + " draw calls");
    const FrameRecord &older = older_frame;
    const FrameRecord &newer = newer_frame;
    Target &target = blend_targets[slot];
    initialize_layouts(commands, target);
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass.renderPass = render_pass;
    pass.framebuffer = target.framebuffer;
    pass.renderArea = {{0, 0}, target_extent};
    vkCmdBeginRenderPass(commands, &pass, VK_SUBPASS_CONTENTS_INLINE);
    perf::count_render_pass();
    // The game clears its framebuffer itself; start from a known state for
    // any part it does not.
    std::array<VkClearAttachment, 2> clears{};
    clears[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    clears[0].colorAttachment = 0u;
    clears[0].clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};
    clears[1].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    clears[1].clearValue.depthStencil = {0.0f, 0u};
    const VkClearRect whole{{{0, 0}, target_extent}, 0u, 1u};
    vkCmdClearAttachments(commands, static_cast<std::uint32_t>(clears.size()), clears.data(), 1u, &whole);

    // The slot's scratch area: blended vertices and object blocks.
    VkDeviceSize scratch_at =
        static_cast<VkDeviceSize>(kFrameRegions) * kVertexBufferBytes + slot * kPresentScratchBytes;
    const VkDeviceSize scratch_end = scratch_at + kPresentScratchBytes;
    auto *mapped = static_cast<std::uint8_t *>(vertex_mapped);

    // What the command buffer has, so that unchanged state is not set again.
    DrawState set{};
    bool known = false;
    // Blended transforms are computed once while the matrices they come from
    // repeat: the view and projection usually stay the same for the whole
    // frame, and consecutive groups often share a world matrix.
    struct Pair {
        const interpolation::Matrix *from{};
        const interpolation::Matrix *to{};
        [[nodiscard]] bool same(const interpolation::Matrix &a, const interpolation::Matrix &b) const {
            return from != nullptr && (from == &a || std::memcmp(from, &a, sizeof(a)) == 0) &&
                   (to == &b || std::memcmp(to, &b, sizeof(b)) == 0);
        }
    };
    Pair view_pair{}, projection_pair{}, world_pair{};
    interpolation::Matrix projection{}, world{}, view_world{}, transform{};
    bool transform_valid = false;
    std::uint32_t written_object = kNone;
    interpolation::Matrix written_world{};
    std::uint32_t written_offset = 0u;
    std::uint64_t replayed = 0u;
    std::uint64_t flipbook_steps = 0u;
    std::uint64_t followed = 0u;
    // MHP3RD_INTERPOLATION_NO_FLIPBOOK_GUARD blends texture offsets up to
    // half the texture, as before; MHP3RD_INTERPOLATION_NO_MOTION_GUARD
    // leaves draws without a partner where the older frame drew them.
    static const bool flipbook_guard = std::getenv("MHP3RD_INTERPOLATION_NO_FLIPBOOK_GUARD") == nullptr;
    static const bool motion_guard = std::getenv("MHP3RD_INTERPOLATION_NO_MOTION_GUARD") == nullptr;

    replay_regions = 0u;
    for (const ReplayGroup &drawn : older.groups) {
        if (drawn.target != older.displayed) continue;
        // The regions this replay reads, for present_regions.
        if (drawn.vertex_base < static_cast<VkDeviceSize>(kFrameRegions) * kVertexBufferBytes)
            replay_regions |= 1u << static_cast<std::uint32_t>(drawn.vertex_base / kVertexBufferBytes);
        DrawState state = drawn.state;
        VkDeviceSize vertex_base = drawn.vertex_base;
        std::int32_t partner = -1;
        std::uint32_t member = drawn.first_draw;
        for (std::uint32_t i = drawn.first_draw; i < drawn.first_draw + drawn.draws; ++i) {
            if (matching.newer_of[i] < 0) continue;
            partner = matching.newer_of[i];
            member = i;
            break;
        }
        if (partner < 0 && t > 0.0f && motion_guard && camera_motion.valid &&
            older.summaries[drawn.first_draw].eligible) {
            // A 3D draw without a partner (drawn only in the older frame, or
            // paired with a draw too far away to be itself) goes with the
            // camera and nothing else: left where the older frame drew it,
            // it would stand still on screen while the scene turns, and jump
            // back at the next frame.
            const interpolation::DrawSummary &from = older.summaries[drawn.first_draw];
            const interpolation::Matrix eye = interpolation::multiply(
                interpolation::rigid_at(camera_motion, t), interpolation::multiply(from.view, from.world));
            state.push.transform = interpolation::multiply(from.projection, eye);
            state.push.view_z = {eye[2], eye[6], eye[10], eye[14]};
            transform_valid = false;
            view_pair = projection_pair = world_pair = Pair{};
            ++followed;
        }
        if (partner >= 0 && t > 0.0f) {
            const interpolation::DrawSummary &from = older.summaries[member];
            const interpolation::DrawSummary &to = newer.summaries[static_cast<std::size_t>(partner)];
            const ReplayGroup &next = newer.groups[newer.group_of[static_cast<std::size_t>(partner)]];
            // Eye space (view times world) is blended as a whole, following
            // the camera's motion along its arcs (blend_eye): blending view
            // and world apart let scenery swing on chords across a turn and
            // the followed character wobble against it. The world matrix is
            // still blended on its own for the lighting block.
            bool changed = !transform_valid;
            bool eye_changed = !transform_valid;
            if (!view_pair.same(from.view, to.view)) {
                view_pair = {&from.view, &to.view};
                eye_changed = true;
            }
            if (!projection_pair.same(from.projection, to.projection)) {
                projection = interpolation::blend_linear(from.projection, to.projection, t);
                projection_pair = {&from.projection, &to.projection};
                changed = true;
            }
            if (!world_pair.same(from.world, to.world)) {
                world = interpolation::blend_affine(from.world, to.world, t);
                world_pair = {&from.world, &to.world};
                eye_changed = true;
            }
            if (eye_changed) {
                view_world = interpolation::blend_eye(interpolation::multiply(from.view, from.world),
                                                      interpolation::multiply(to.view, to.world), camera_motion, t);
                changed = true;
            }
            if (changed) {
                transform = interpolation::multiply(projection, view_world);
                transform_valid = true;
            }
            state.push.transform = transform;
            state.push.view_z = {view_world[2], view_world[6], view_world[10], view_world[14]};
            // A texture that scrolls moves its offset a little each frame;
            // a flipbook jumps to its next cell, and a wrap by the whole
            // texture: both are held.
            for (std::size_t axis = 2u; axis < 4u; ++axis) {
                const float a = drawn.state.push.uv_transform[axis];
                const float b = next.state.push.uv_transform[axis];
                if (a == b || drawn.state.push.uv_transform[axis - 2u] != next.state.push.uv_transform[axis - 2u])
                    continue;
                // A flipbook's step to its next atlas cell is held, not
                // blended (interpolation::blend_offset).
                const float max_scroll = flipbook_guard ? interpolation::kMaxScrollStep : 0.5f;
                if (!interpolation::scrolls(a, b, max_scroll)) ++flipbook_steps;
                state.push.uv_transform[axis] = interpolation::blend_offset(a, b, t, max_scroll);
            }
            // A lit draw's object block carries the world matrix its lights
            // are evaluated in; everything else in it stays.
            if (drawn.object != kNone && world != older.objects[drawn.object].world) {
                if (drawn.object != written_object || world != written_world) {
                    ObjectBlock block = older.objects[drawn.object];
                    block.world = world;
                    const VkDeviceSize at =
                        (scratch_at + uniform_alignment - 1u) / uniform_alignment * uniform_alignment;
                    if (at + sizeof(ObjectBlock) <= scratch_end) {
                        std::memcpy(mapped + at, &block, sizeof(block));
                        scratch_at = at + sizeof(ObjectBlock);
                        written_object = drawn.object;
                        written_world = world;
                        written_offset = static_cast<std::uint32_t>(at);
                    }
                }
                if (drawn.object == written_object && world == written_world) state.lighting[1] = written_offset;
            }
            // A raw skinned group is skinned on the GPU: its bones are blended
            // instead, which blends the skinned vertices the same way.
            if (drawn.raw_block != kNone && next.raw_block != kNone && next.vertex_count == drawn.vertex_count) {
                const RawBlock &a = older.raws[drawn.raw_block];
                const RawBlock &b = newer.raws[next.raw_block];
                if (a.format == b.format) {
                    RawBlock blended = a;
                    for (std::size_t i = 0; i < blended.bones.size(); ++i)
                        blended.bones[i] += (b.bones[i] - blended.bones[i]) * t;
                    const VkDeviceSize at =
                        (scratch_at + uniform_alignment - 1u) / uniform_alignment * uniform_alignment;
                    if (at + sizeof(RawBlock) <= scratch_end) {
                        std::memcpy(mapped + at, &blended, sizeof(RawBlock));
                        scratch_at = at + sizeof(RawBlock);
                        state.lighting[2] = static_cast<std::uint32_t>(at);
                    }
                }
            }
            // Skinning is linear in the bone matrices, so blending the skinned
            // vertices blends the bones.
            if (drawn.skinned != kNone && next.skinned != kNone && next.vertex_count == drawn.vertex_count) {
                const VkDeviceSize at = (scratch_at + 15u) & ~VkDeviceSize{15u};
                const VkDeviceSize bytes = static_cast<VkDeviceSize>(drawn.vertex_count) * sizeof(GpuVertex);
                if (at + bytes <= scratch_end) {
                    const GpuVertex *a = older.skinned.data() + drawn.skinned;
                    const GpuVertex *b = newer.skinned.data() + next.skinned;
                    auto *out = reinterpret_cast<GpuVertex *>(mapped + at);
                    for (std::uint32_t v = 0; v < drawn.vertex_count; ++v) {
                        GpuVertex blended = a[v];
                        blended.x += (b[v].x - blended.x) * t;
                        blended.y += (b[v].y - blended.y) * t;
                        blended.z += (b[v].z - blended.z) * t;
                        blended.nx += (b[v].nx - blended.nx) * t;
                        blended.ny += (b[v].ny - blended.ny) * t;
                        blended.nz += (b[v].nz - blended.nz) * t;
                        out[v] = blended;
                    }
                    scratch_at = at + bytes;
                    vertex_base = at;
                }
            }
        }

        if (!known || std::memcmp(&state.viewport, &set.viewport, sizeof(VkViewport)) != 0)
            vkCmdSetViewport(commands, 0u, 1u, &state.viewport);
        if (!known || state.blend != set.blend) vkCmdSetBlendConstants(commands, state.blend.data());
        if (!known || std::memcmp(&state.scissor, &set.scissor, sizeof(VkRect2D)) != 0)
            vkCmdSetScissor(commands, 0u, 1u, &state.scissor);
        if (!known || state.pipeline != set.pipeline)
            vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline);
        if (!known || state.texture != set.texture)
            vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0u, 1u,
                                    &state.texture, 0u, nullptr);
        if (!known || state.lighting != set.lighting)
            vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1u, 1u,
                                    &lighting_descriptor, static_cast<std::uint32_t>(state.lighting.size()),
                                    state.lighting.data());
        if (!known || std::memcmp(&state.push, &set.push, sizeof(PushConstants)) != 0)
            vkCmdPushConstants(commands, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0u, sizeof(PushConstants), &state.push);
        set = state;
        known = true;
        vkCmdBindVertexBuffers(commands, 0u, 1u, &vertex_buffer, &vertex_base);
        if (drawn.index_buffer != VK_NULL_HANDLE) {
            vkCmdBindIndexBuffer(commands, drawn.index_buffer, drawn.index_base, VK_INDEX_TYPE_UINT16);
            vkCmdDrawIndexed(commands, drawn.count, 1u, 0u, 0,
                             drawn.raw ? static_cast<std::uint32_t>(vertex_base) : 0u);
        } else {
            vkCmdDraw(commands, drawn.count, 1u, 0u, 0u);
        }
        ++replayed;
    }
    vkCmdEndRenderPass(commands);
    interpolation_stats.groups += replayed;
    interpolation_stats.flipbook_steps += flipbook_steps;
    interpolation_stats.followed += followed;
}

// Once a second while a faster frame rate is chosen: the [interp] line
// (MHP3RD_TRACE_INTERPOLATION) and the governor's turn.
void VulkanRenderer::Impl::report_interpolation() {
    using Clock = std::chrono::steady_clock;
    InterpolationStats &stats = interpolation_stats;
    const Clock::time_point now = Clock::now();
    if (now - stats.window_start < std::chrono::seconds(1)) return;
    const auto ms = [](Clock::duration duration) {
        return std::chrono::duration<double, std::milli>(duration).count();
    };
    const std::uint32_t plain = stats.presents - stats.blended;
    // Costs are kept across seconds: a second at 30 measures none.
    if (stats.blended != 0u) blend_cost_ms = ms(stats.blend_time) / stats.blended;
    if (plain != 0u) plain_cost_ms = ms(stats.plain_time) / plain;
    const double frames_now = std::max(1u, stats.frames);
    const std::uint32_t reasons_known =
        stats.plain_newest + stats.plain_oldest + stats.plain_cut + stats.plain_textures;
    if (trace_interpolation) {
        std::string cuts;
        for (const auto &[reason, count] : stats.cuts) cuts += ", " + reason + " " + std::to_string(count);
        std::printf("[interp] %.0f of %.0f fps, %u frames: matched %.1f%% of %.0f draws per frame; camera up to "
                    "%.2f deg %.2f units, %u continued; cuts %s; presents %u, %u blended (%.2f ms each, %.2f "
                    "recording %.0f draw calls, gpu %.2f ms), %u plain (%.2f ms each), %u skipped, late up to %.1f "
                    "ms; delay %.1f ms (code %.1f ms)\n",
                    governor.rate(), governor.requested(), stats.frames,
                    stats.eligible != 0u
                        ? 100.0 * static_cast<double>(stats.matched) / static_cast<double>(stats.eligible)
                        : 0.0,
                    static_cast<double>(stats.eligible) / frames_now, static_cast<double>(stats.max_camera_angle),
                    static_cast<double>(stats.max_camera_distance), stats.continued,
                    cuts.empty() ? "none" : cuts.substr(2).c_str(), stats.presents, stats.blended,
                    stats.blended != 0u ? ms(stats.blend_time) / stats.blended : 0.0,
                    stats.blended != 0u ? ms(stats.replay_time) / stats.blended : 0.0,
                    stats.blended != 0u ? static_cast<double>(stats.groups) / stats.blended : 0.0,
                    stats.gpu_samples != 0u ? stats.gpu_ms / stats.gpu_samples : 0.0, plain,
                    plain != 0u ? ms(stats.plain_time) / plain : 0.0, stats.skipped, ms(stats.max_late),
                    static_cast<double>(present_clock.delay_us()) / 1000.0,
                    static_cast<double>(present_clock.work_us()) / 1000.0);
        std::printf("[interp] plain: %u at the newest frame, %u at the older, %u not blended (cut), %u textures "
                    "dropped, %u other; not made: %u display busy, %u over budget\n",
                    stats.plain_newest, stats.plain_oldest, stats.plain_cut, stats.plain_textures,
                    plain > reasons_known ? plain - reasons_known : 0u, stats.blocked, stats.over_budget);
        std::printf("[interp] guards: %u pairs moved too far on their own (%u with the same mesh drawn more than "
                    "once), own motion kept up to %.1f units; %.0f draw calls a blend followed the camera only; "
                    "%.0f texture offsets a blend held (flipbook steps)\n",
                    stats.rejected, stats.rejected_shared, static_cast<double>(stats.max_own_motion),
                    stats.blended != 0u ? static_cast<double>(stats.followed) / stats.blended : 0.0,
                    stats.blended != 0u ? static_cast<double>(stats.flipbook_steps) / stats.blended : 0.0);
        std::fflush(stdout);
    }
    const perf::Summary &summary = perf::last_second();
    if (summary.valid && summary.second != governor_second) {
        governor_second = summary.second;
        pacing::Second second{};
        second.speed = summary.speed;
        second.idle_ms = summary.pacing_ms;
        second.blend_ms = blend_cost_ms;
        second.plain_ms = plain_cost_ms;
        second.interpolation_ms = (ms(stats.blend_time) + ms(stats.plain_time)) / frames_now;
        second.presents = stats.presents;
        second.skipped = stats.skipped + stats.over_budget;
        second.blocked = stats.blocked;
        const double before = governor.rate();
        if (governor.update(second)) {
            std::printf("[interp] frame rate %.0f -> %.0f: %s (speed %.0f%%, spare %.1f ms a frame, %.2f ms a "
                        "blended present)\n",
                        before, governor.rate(), governor.reason(), summary.speed * 100.0, summary.pacing_ms,
                        blend_cost_ms);
            std::fflush(stdout);
        }
        perf::set_frame_rate_info(governor.rate(), governor.requested());
    }
    stats = InterpolationStats{};
    stats.window_start = now;
}

// Copies a colour image resting in COLOR_ATTACHMENT_OPTIMAL to the CPU. Waits
// for the device; only for checks.
std::vector<std::uint32_t> VulkanRenderer::Impl::read_image(VkImage image) {
    std::vector<std::uint32_t> pixels;
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(target_extent.width) * target_extent.height * 4u;
    VkBuffer buffer{};
    VkDeviceMemory memory{};
    VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = bytes;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) return pixels;
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer, &requirements);
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = find_memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device, &allocate, nullptr, &memory) == VK_SUCCESS) {
        vkBindBufferMemory(device, buffer, memory, 0u);
        run_commands([&](VkCommandBuffer commands) {
            transition(commands, image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            VkBufferImageCopy copy{};
            copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
            copy.imageExtent = {target_extent.width, target_extent.height, 1u};
            vkCmdCopyImageToBuffer(commands, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1u, &copy);
            transition(commands, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        });
        void *mapped = nullptr;
        if (vkMapMemory(device, memory, 0u, VK_WHOLE_SIZE, 0u, &mapped) == VK_SUCCESS) {
            pixels.resize(static_cast<std::size_t>(bytes / 4u));
            std::memcpy(pixels.data(), mapped, static_cast<std::size_t>(bytes));
            vkUnmapMemory(device, memory);
        }
    }
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
    return pixels;
}

// MHP3RD_CHECK_REPLAY: every 150 flips, draws the older frame again from its
// recording without blending and compares it with the older frame's own
// picture, pixel for pixel; with MHP3RD_SCREENSHOT_DIR it also writes both,
// the frame halfway to the newer one and the newer frame as BMPs.
void VulkanRenderer::Impl::check_replay() {
    if (!older_frame.valid || !older_frame.recorded || !newer_frame.recorded || !older_picture_valid ||
        matching.cut != nullptr || destroyed_texture_clock > older_frame.texture_clock)
        return;
    std::string error;
    if (blend_targets[0].color == VK_NULL_HANDLE && !create_target(blend_targets[0], error)) return;
    vkDeviceWaitIdle(device);
    const auto draw = [&](float t) {
        run_commands([&](VkCommandBuffer commands) { replay(commands, 0u, t); });
        return read_image(blend_targets[0].color);
    };
    const std::vector<std::uint32_t> older_pixels = read_image(pictures[newer_picture ^ 1u].color);
    const std::vector<std::uint32_t> replayed = draw(0.0f);
    std::size_t differ = 0u;
    for (std::size_t i = 0; i < std::min(older_pixels.size(), replayed.size()); ++i)
        if (older_pixels[i] != replayed[i]) ++differ;
    std::printf("[interp] replay check at frame %llu: %zu of %zu pixels differ from the frame drawn again "
                "(%zu draw calls)\n",
                static_cast<unsigned long long>(frames), differ, older_pixels.size(), older_frame.groups.size());
    std::fflush(stdout);
    static const char *directory = std::getenv("MHP3RD_SCREENSHOT_DIR");
    if (directory == nullptr) return;
    const std::string prefix = std::string(directory) + "/replay_" + std::to_string(frames);
    const auto write = [&](const std::string &name, const std::vector<std::uint32_t> &pixels) {
        if (!pixels.empty())
            write_bmp(prefix + name, reinterpret_cast<const std::uint8_t *>(pixels.data()), target_extent.width,
                      target_extent.height, false);
    };
    write("_older.bmp", older_pixels);
    write("_again.bmp", replayed);
    write("_half.bmp", draw(0.5f));
    write("_newer.bmp", read_image(pictures[newer_picture].color));
}

void VulkanRenderer::Impl::reset_interpolation() {
    recording_frame.clear();
    newer_frame.clear();
    older_frame.clear();
    older_picture_valid = newer_picture_valid = false;
    present_clock.reset();
    matcher.forget_motion();
    matching = interpolation::Matching{};
    cycle_active = false;
}

void VulkanRenderer::Impl::destroy_interpolation_targets() {
    for (Target &picture : pictures) destroy_target(picture);
    for (Target &target : blend_targets) destroy_target(target);
    older_picture_valid = newer_picture_valid = false;
}

bool VulkanRenderer::present(std::uint32_t display_address,
                             std::optional<std::chrono::steady_clock::time_point> moment) {
    Impl &impl = *impl_;
    if (!impl.ready) return false;
    if (!impl.recording) begin_frame();
    impl.end_pass();

    static const bool trace3d = std::getenv("MHP3RD_TRACE_3D") != nullptr;
    static const bool trace_camera = std::getenv("MHP3RD_TRACE_CAMERA") != nullptr;
    // The camera hunt reads the same measurement without printing it.
    static const bool watch_camera = trace_camera || std::getenv("MHP3RD_FIND_CAMERA") != nullptr;
    if (trace3d && impl.frame_transformed_draws != 0u) {
        std::cout << "[3d] frame " << impl.frames << " through=" << impl.frame_through_draws
                  << " transformed=" << impl.frame_transformed_draws << " showing=0x" << std::hex << display_address
                  << " transformed targets:";
        for (const auto &[address, count] : impl.frame_transformed_targets)
            std::cout << " 0x" << address << "x" << std::dec << count << std::hex;
        std::cout << std::dec << "\n";
        std::cout << "     verts=" << impl.frame_transformed_vertices << " onscreen=" << impl.frame_onscreen_vertices
                  << " behind=" << impl.frame_behind_camera << " ndc x[" << impl.frame_ndc_min[0] << ","
                  << impl.frame_ndc_max[0] << "] y[" << impl.frame_ndc_min[1] << "," << impl.frame_ndc_max[1]
                  << "] z[" << impl.frame_ndc_min[2] << "," << impl.frame_ndc_max[2] << "]\n";
    }
    if (watch_camera && !impl.frame_views.empty()) {
        // The busiest view matrix of the frame is the scene the player looks
        // at; the others belong to reflections and shadow passes.
        const auto scene = std::max_element(impl.frame_views.begin(), impl.frame_views.end(),
                                            [](const auto &a, const auto &b) { return a.second < b.second; });
        const std::array<float, 16> &view = scene->first;
        // A view matrix holds the camera's own axes as the rows of its
        // rotation part, and the array is column major, so row 2 — the
        // direction the camera looks along — is elements 2, 6 and 10.
        const float forward_x = view[2];
        const float forward_y = view[6];
        const float forward_z = view[10];
        constexpr float kDegrees = 57.29577951308232f;
        const float yaw = std::atan2(forward_x, forward_z) * kDegrees;
        const float pitch = std::asin(std::clamp(forward_y, -1.0f, 1.0f)) * kDegrees;
        // The camera's world position is the rotation applied backwards to the
        // translation, which tells a turn in place from the hunter walking.
        const float tx = view[12];
        const float ty = view[13];
        const float tz = view[14];
        const float px = -(view[0] * tx + view[1] * ty + view[2] * tz);
        const float py = -(view[4] * tx + view[5] * ty + view[6] * tz);
        const float pz = -(view[8] * tx + view[9] * ty + view[10] * tz);
        float turn = yaw - impl.traced_yaw;
        while (turn > 180.0f) turn -= 360.0f;
        while (turn < -180.0f) turn += 360.0f;
        impl.traced_yaw = yaw;
        impl.reading = CameraReading{true, yaw, pitch, turn, {px, py, pz}, view};
        if (trace_camera) {
        const std::ios::fmtflags flags = std::cout.flags();
        const std::streamsize precision = std::cout.precision();
        std::cout << std::fixed << std::setprecision(4) << "[camera] frame " << impl.frames << " stick="
                  << static_cast<int>(impl.pad.right_x) - 0x80 << "," << static_cast<int>(impl.pad.right_y) - 0x80
                  << " yaw=" << yaw << " pitch=" << pitch << " turn=" << turn << " pos=" << std::setprecision(1) << px
                  << "," << py << "," << pz << " views=" << impl.frame_views.size() << "\n";
        std::cout.flags(flags);
        // Precision is not one of a stream's flags, so restoring the flags
        // leaves it wherever the line left it -- here at one significant digit
        // for the position -- and every number printed afterwards by anything
        // else comes out rounded to one digit for the rest of the run.
        std::cout.precision(precision);
        }
    }
    impl.frame_views.clear();
    impl.frame_through_draws = 0u;
    impl.frame_transformed_draws = 0u;
    impl.frame_transformed_vertices = 0u;
    impl.frame_onscreen_vertices = 0u;
    impl.frame_behind_camera = 0u;
    impl.frame_ndc_min = {1e30f, 1e30f, 1e30f};
    impl.frame_ndc_max = {-1e30f, -1e30f, -1e30f};
    impl.frame_transformed_targets.clear();

    static const bool check_direct = std::getenv("MHP3RD_CHECK_DIRECT_VERTICES") != nullptr;
    if (check_direct && impl.frames % 300u == 0u)
        std::cout << "[direct-check] " << impl.direct_checked << " draws compared, " << impl.direct_mismatched
                  << " differed" << std::endl;

    static const bool no_writeback = std::getenv("MHP3RD_NO_FB_TEXTURES") != nullptr;
    if (!no_writeback) impl.record_writeback(display_address);

    // Show the target the guest flipped to; fall back to whatever was drawn last.
    auto displayed = impl.targets.find(display_address);
    if (displayed == impl.targets.end()) displayed = impl.targets.find(impl.last_drawn_target);
    VkImage source = displayed != impl.targets.end() ? displayed->second.color : VK_NULL_HANDLE;
    impl.presented_target = displayed != impl.targets.end() ? displayed->first : 0u;
    if (impl.holding) source = impl.held.color;
    if (display_address != impl.display_addresses[0]) {
        impl.display_addresses[1] = impl.display_addresses[0];
        impl.display_addresses[0] = display_address;
    }
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    // MHP3RD_FRAME_RATE_CYCLE=30,60,90: the frame rate moves to the next one
    // listed every MHP3RD_FRAME_RATE_CYCLE_SECONDS (default 10), so one run
    // measures them all on the same scene.
    static const std::vector<settings::FrameRate> cycle = [] {
        std::vector<settings::FrameRate> rates;
        const char *text = std::getenv("MHP3RD_FRAME_RATE_CYCLE");
        if (text == nullptr) return rates;
        const std::string list = std::string(text) + ",";
        std::size_t start = 0u;
        for (std::size_t comma = list.find(','); comma != std::string::npos; comma = list.find(',', start)) {
            const std::string name = list.substr(start, comma - start);
            start = comma + 1u;
            const std::pair<const char *, settings::FrameRate> known[] = {
                {"30", settings::FrameRate::Fps30},   {"45", settings::FrameRate::Fps45},
                {"60", settings::FrameRate::Fps60},   {"90", settings::FrameRate::Fps90},
                {"120", settings::FrameRate::Fps120}, {"display", settings::FrameRate::Display}};
            for (const auto &[spelling, rate] : known)
                if (name == spelling) rates.push_back(rate);
        }
        return rates;
    }();
    if (!cycle.empty()) {
        static const auto period = std::chrono::seconds([] {
            const char *text = std::getenv("MHP3RD_FRAME_RATE_CYCLE_SECONDS");
            return text != nullptr ? std::max(1, std::atoi(text)) : 10;
        }());
        static std::size_t next = 0u;
        static std::chrono::steady_clock::time_point switch_at = now;
        if (now >= switch_at) {
            const settings::FrameRate rate = cycle[next];
            next = (next + 1u) % cycle.size();
            switch_at = now + period;
            set_frame_rate(rate);
            std::printf("[interp] cycle: frame rate %s\n",
                        rate == settings::FrameRate::Fps30    ? "30"
                        : rate == settings::FrameRate::Fps45  ? "45"
                        : rate == settings::FrameRate::Fps60  ? "60"
                        : rate == settings::FrameRate::Fps90  ? "90"
                        : rate == settings::FrameRate::Fps120 ? "120"
                                                              : "display");
            std::fflush(stdout);
        }
    }
    if (impl.frame_rate != settings::FrameRate::Fps30) {
        const double wanted = impl.wanted_rate();
        if (std::fabs(wanted - impl.governor.requested()) > 0.5) {
            impl.governor.set_requested(wanted);
            perf::set_frame_rate_info(impl.governor.rate(), impl.governor.requested());
        }
    }
    if (impl.interpolation_wanted() && source != VK_NULL_HANDLE) {
        // Frame interpolation: the frame is submitted now and shown by the
        // presents between this flip and the next (frame_pacing.hpp).
        impl.frame_ui = impl.ui_draw_data;
        impl.ui_draw_data = nullptr;
        const std::int64_t moment_us = moment ? to_us(*moment) : to_us(now);
        impl.finish_interpolated_frame(source, impl.presented_target, moment_us);
        impl.submit_frame();
        ++impl.frames;
        impl.present_clock.set_presents_per_frame(pacing::presents_per_frame(impl.governor.rate()));
        impl.present_clock.flip(moment_us, to_us(now));
        impl.cycle_active = impl.newer_picture_valid;
        static const bool check = std::getenv("MHP3RD_CHECK_REPLAY") != nullptr;
        if (check && impl.frames % 150u == 0u) impl.check_replay();
        impl.report_interpolation();
        impl.follow_window();
        // A present due now is made at once; it counts itself.
        impl.busy_presents = {};
        impl.poll_presents(std::chrono::steady_clock::now(), false, false);
        return false;
    }
    // The game's frame as it is, at its flip.
    if (impl.cycle_active || impl.newer_frame.valid) impl.reset_interpolation();
    if (impl.fast_forward) {
        // A load running fast: the flip is drawn, and shown only when the
        // window has not had a picture for a while, so the display's refresh
        // never holds the load back.
        if (now - impl.fast_forward_shown < kFastForwardPresentInterval) {
            impl.ui_draw_data = nullptr;
            impl.submit_frame();
            ++impl.frames;
            return false;
        }
        impl.fast_forward_shown = now;
    }
    {
        const perf::SplitScope split(perf::Split::Present);
        impl.submit_and_present(source, true);
    }
    ++impl.frames;
    if (impl.frame_rate != settings::FrameRate::Fps30) impl.report_interpolation();
    impl.follow_window();
    return true;
}

void VulkanRenderer::present_due() {
    if (!impl_ || !impl_->ready || !impl_->cycle_active) return;
    impl_->poll_presents(std::chrono::steady_clock::now(), true, false);
}

void VulkanRenderer::present_until(std::chrono::steady_clock::time_point wake) {
    if (!impl_ || !impl_->ready) return;
    Impl &impl = *impl_;
    using Clock = std::chrono::steady_clock;
    while (impl.cycle_active) {
        const std::optional<std::int64_t> due = impl.present_clock.next_due();
        if (!due) return;
        const Clock::time_point at{std::chrono::microseconds(*due)};
        if (at > wake) return;
        const Clock::time_point now = Clock::now();
        if (at > now) {
            std::this_thread::sleep_until(at);
            perf::add_pacing_time(Clock::now() - now);
        }
        // A present not made (the display had no image free) ends the wait's
        // presents: the next ones would find none either.
        if (!impl.poll_presents(Clock::now(), true, true)) return;
    }
}

void VulkanRenderer::set_fast_forward(bool on) {
    if (!impl_ || impl_->fast_forward == on) return;
    impl_->fast_forward = on;
    // The first flip of a fast stretch is shown.
    impl_->fast_forward_shown = {};
}

void VulkanRenderer::pause_interpolation() {
    if (!impl_) return;
    // The game stands still: the presents it had scheduled are dropped and
    // the frames' moments start over when it resumes.
    impl_->present_clock.reset();
    impl_->cycle_active = false;
}

void VulkanRenderer::set_frame_rate(settings::FrameRate rate) {
    if (!impl_) return;
    Impl &impl = *impl_;
    if (impl.frame_rate == rate) return;
    impl.frame_rate = rate;
    impl.governor.set_requested(rate == settings::FrameRate::Fps30 ? 30.0 : impl.wanted_rate());
    impl.reset_interpolation();
    perf::set_frame_rate_info(rate == settings::FrameRate::Fps30 ? 0.0 : impl.governor.rate(),
                              rate == settings::FrameRate::Fps30 ? 0.0 : impl.governor.requested());
}

void VulkanRenderer::set_frame_rate_auto(bool automatic) {
    if (!impl_) return;
    impl_->governor.set_automatic(automatic);
    if (impl_->frame_rate != settings::FrameRate::Fps30)
        perf::set_frame_rate_info(impl_->governor.rate(), impl_->governor.requested());
}

float VulkanRenderer::display_refresh() const noexcept { return impl_ ? impl_->display_hz : 0.0f; }

double VulkanRenderer::frame_rate_now() const noexcept {
    if (!impl_ || impl_->frame_rate == settings::FrameRate::Fps30) return 30.0;
    return impl_->governor.rate();
}

bool VulkanRenderer::capture_frame(const std::string &path) {
    Impl &impl = *impl_;
    if (!impl.ready) return false;
    vkDeviceWaitIdle(impl.device);

    auto shown = impl.targets.find(impl.presented_target);
    if (shown == impl.targets.end()) return false;
    const VkImage captured = shown->second.color;
    const std::uint32_t width = impl.target_extent.width;
    const std::uint32_t height = impl.target_extent.height;
    const VkDeviceSize bytes = static_cast<VkDeviceSize>(width) * height * 4u;
    VkBuffer staging{};
    VkDeviceMemory staging_memory{};
    VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = bytes;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (vkCreateBuffer(impl.device, &buffer_info, nullptr, &staging) != VK_SUCCESS) return false;
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(impl.device, staging, &requirements);
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = impl.find_memory_type(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(impl.device, &allocate, nullptr, &staging_memory);
    vkBindBufferMemory(impl.device, staging, staging_memory, 0u);

    VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_info.commandPool = impl.command_pool;
    command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_info.commandBufferCount = 1u;
    VkCommandBuffer commands{};
    vkAllocateCommandBuffers(impl.device, &command_info, &commands);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commands, &begin);
    impl.transition(commands, captured, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u};
    copy.imageExtent = {width, height, 1u};
    vkCmdCopyImageToBuffer(commands, captured, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging, 1u, &copy);
    impl.transition(commands, captured, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkEndCommandBuffer(commands);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1u;
    submit.pCommandBuffers = &commands;
    vkQueueSubmit(impl.queue, 1u, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(impl.queue);
    vkFreeCommandBuffers(impl.device, impl.command_pool, 1u, &commands);

    void *mapped = nullptr;
    vkMapMemory(impl.device, staging_memory, 0u, bytes, 0u, &mapped);
    std::vector<std::uint8_t> pixels(static_cast<const std::uint8_t *>(mapped),
                                     static_cast<const std::uint8_t *>(mapped) + bytes);
    vkUnmapMemory(impl.device, staging_memory);
    vkDestroyBuffer(impl.device, staging, nullptr);
    vkFreeMemory(impl.device, staging_memory, nullptr);

    // The capture is the game's own target, before the window blit; draw the
    // overlay over it the same way, so captures show what the window shows.
    if (impl.overlay_visible) {
        const std::uint32_t scale = perf::overlay_scale(height);
        const std::uint32_t inset = 4u * scale;
        if (inset + perf::kOverlayWidth * scale <= width && inset + perf::kOverlayHeight * scale <= height) {
            for (std::uint32_t y = 0; y < perf::kOverlayHeight * scale; ++y) {
                std::uint8_t *row = pixels.data() + static_cast<std::size_t>(inset + y) * width * 4u;
                for (std::uint32_t x = 0; x < perf::kOverlayWidth * scale; ++x) {
                    const std::uint32_t pixel = impl.overlay_pixels[(y / scale) * perf::kOverlayWidth + x / scale];
                    std::memcpy(row + (inset + x) * 4u, &pixel, 4u);
                }
            }
        }
    }
    return write_bmp(path, pixels.data(), width, height, false);
}

void VulkanRenderer::shutdown() {
    if (impl_ && impl_->gamepad != nullptr) {
        SDL_CloseGamepad(impl_->gamepad);
        impl_->gamepad = nullptr;
    }
    if (!impl_ || impl_->device == VK_NULL_HANDLE) {
        if (impl_ && impl_->window != nullptr) {
            SDL_DestroyWindow(impl_->window);
            impl_->window = nullptr;
        }
        return;
    }
    Impl &impl = *impl_;
    vkDeviceWaitIdle(impl.device);
    // The interface may still be up when the game quits from its menu.
    if (impl.ui_ready && ImGui::GetCurrentContext() != nullptr) ImGui_ImplVulkan_Shutdown();
    impl.ui_ready = false;
    impl.destroy_swapchain_views();
#if defined(__ANDROID__)
    impl.destroy_rotation_pipeline();
#endif
    if (impl.ui_render_pass != VK_NULL_HANDLE) vkDestroyRenderPass(impl.device, impl.ui_render_pass, nullptr);
    for (auto &[key, texture] : impl.textures) impl.destroy_texture(texture);
    impl.textures.clear();
    for (Impl::FrameSlot &frame : impl.slots) {
        for (Impl::Texture &texture : frame.retired_textures) impl.destroy_texture(texture);
        frame.retired_textures.clear();
        for (const auto &[buffer, memory] : frame.retired_buffers) {
            vkDestroyBuffer(impl.device, buffer, nullptr);
            vkFreeMemory(impl.device, memory, nullptr);
        }
        frame.retired_buffers.clear();
        if (frame.ring_mapped != nullptr) vkUnmapMemory(impl.device, frame.ring_memory);
        vkDestroyBuffer(impl.device, frame.ring, nullptr);
        vkFreeMemory(impl.device, frame.ring_memory, nullptr);
    }
    impl.replacements.shutdown();
    impl.pack.reset();
    impl.dumper.reset();
    impl.destroy_texture(impl.white_texture);
    for (Impl::Staging &staging : impl.overlay_staging) {
        if (staging.mapped != nullptr) vkUnmapMemory(impl.device, staging.memory);
        vkDestroyBuffer(impl.device, staging.buffer, nullptr);
        vkFreeMemory(impl.device, staging.memory, nullptr);
    }
    vkDestroyImageView(impl.device, impl.overlay_view, nullptr);
    vkDestroyImage(impl.device, impl.overlay_image, nullptr);
    vkFreeMemory(impl.device, impl.overlay_memory, nullptr);
    impl.destroy_upload();
    impl.destroy_writeback();
    if (impl.breadcrumbs.mapped != nullptr) vkUnmapMemory(impl.device, impl.breadcrumbs.memory);
    vkDestroyBuffer(impl.device, impl.breadcrumbs.buffer, nullptr);
    vkFreeMemory(impl.device, impl.breadcrumbs.memory, nullptr);
    impl.prewarm->stop = true;
    if (impl.prewarm->thread.joinable()) impl.prewarm->thread.join();
    if (impl.pipelines_prewarm_used != 0u)
        std::cout << "[render] " << impl.pipelines_prewarm_used << " pipelines came from the background prewarm\n";
    impl.report_pipelines(true);
    impl.pending_textures.clear();
    impl.decode_pool.reset();
    if (impl.pipeline_cache != VK_NULL_HANDLE) vkDestroyPipelineCache(impl.device, impl.pipeline_cache, nullptr);
    impl.pipeline_cache = VK_NULL_HANDLE;
    for (auto &[key, pipeline] : impl.pipelines) vkDestroyPipeline(impl.device, pipeline, nullptr);
    impl.pipelines.clear();
    for (auto &[key, pipeline] : impl.prewarm->made) vkDestroyPipeline(impl.device, pipeline, nullptr);
    impl.prewarm->made.clear();
    impl.last_pipeline = VK_NULL_HANDLE;
    if (impl.vertex_mapped != nullptr) vkUnmapMemory(impl.device, impl.vertex_memory);
    vkDestroyBuffer(impl.device, impl.vertex_buffer, nullptr);
    vkFreeMemory(impl.device, impl.vertex_memory, nullptr);
    vkDestroyBuffer(impl.device, impl.index_buffer, nullptr);
    vkFreeMemory(impl.device, impl.index_memory, nullptr);
    vkDestroySampler(impl.device, impl.sampler, nullptr);
    vkDestroySampler(impl.device, impl.sharp_sampler, nullptr);
    vkDestroySampler(impl.device, impl.clamp_sampler, nullptr);
    vkDestroySampler(impl.device, impl.clamp_sharp_sampler, nullptr);
    vkDestroyDescriptorPool(impl.device, impl.descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(impl.device, impl.descriptor_layout, nullptr);
    vkDestroyDescriptorSetLayout(impl.device, impl.lighting_layout, nullptr);
    vkDestroyPipelineLayout(impl.device, impl.pipeline_layout, nullptr);
    vkDestroyShaderModule(impl.device, impl.vertex_shader, nullptr);
    vkDestroyShaderModule(impl.device, impl.raw_vertex_shader, nullptr);
    if (impl.check_mapped != nullptr) vkUnmapMemory(impl.device, impl.check_memory);
    vkDestroyBuffer(impl.device, impl.check_buffer, nullptr);
    vkFreeMemory(impl.device, impl.check_memory, nullptr);
    vkDestroyShaderModule(impl.device, impl.fragment_shader, nullptr);
    for (auto &[address, target] : impl.targets) impl.destroy_target(target);
    impl.targets.clear();
    impl.destroy_target(impl.held);
    impl.held = {};
    impl.holding = false;
    impl.destroy_interpolation_targets();
    for (VkFence fence : impl.present_fences) vkDestroyFence(impl.device, fence, nullptr);
    if (impl.present_timer != VK_NULL_HANDLE) vkDestroyQueryPool(impl.device, impl.present_timer, nullptr);
    vkDestroyRenderPass(impl.device, impl.render_pass, nullptr);
    for (VkRenderPass pass : impl.discard_passes)
        if (pass != VK_NULL_HANDLE) vkDestroyRenderPass(impl.device, pass, nullptr);
    for (VkSemaphore semaphore : impl.image_available) vkDestroySemaphore(impl.device, semaphore, nullptr);
    for (VkSemaphore semaphore : impl.render_finished) vkDestroySemaphore(impl.device, semaphore, nullptr);
    for (Impl::FrameSlot &frame : impl.slots) vkDestroyFence(impl.device, frame.fence, nullptr);
    if (impl.gpu_timer != VK_NULL_HANDLE) vkDestroyQueryPool(impl.device, impl.gpu_timer, nullptr);
    impl.destroy_upload_buffer();
    vkDestroyCommandPool(impl.device, impl.command_pool, nullptr);
    vkDestroySwapchainKHR(impl.device, impl.swapchain, nullptr);
    vkDestroyDevice(impl.device, nullptr);
#if defined(__ANDROID__)
    SDL_RemoveEventWatch(&Impl::watch_lifecycle, &impl);
#endif
    if (impl.surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(impl.instance, impl.surface, nullptr);
    vkDestroyInstance(impl.instance, nullptr);
    if (impl.window != nullptr) SDL_DestroyWindow(impl.window);
    impl = Impl{};
}

} // namespace mhp3rd::gpu
