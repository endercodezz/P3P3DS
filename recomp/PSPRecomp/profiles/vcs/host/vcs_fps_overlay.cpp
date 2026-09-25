#include "vcs_fps_overlay.hpp"

#include "ge_gpu_backend.hpp"
#include "vcs_config.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

namespace vcs {
namespace {

struct TargetCandidate {
    GeGpuDrawDescriptor draw{};
};

struct TrackedTarget {
    std::uint32_t address{};
    TargetCandidate candidate{};
};
constexpr std::size_t kMaxTrackedTargets = 16u;
std::array<TrackedTarget, kMaxTrackedTargets> g_targets{};
std::size_t g_target_count{};
std::uint32_t g_last_observed_target{};

void clear_targets() noexcept {
    g_target_count = 0u;
    g_last_observed_target = 0u;
}

TargetCandidate *find_target(std::uint32_t address) noexcept {
    for (std::size_t i = 0u; i < g_target_count; ++i)
        if (g_targets[i].address == address) return &g_targets[i].candidate;
    return nullptr;
}
std::chrono::steady_clock::time_point g_measurement_start{};
std::uint32_t g_measurement_frames{};
double g_fps{};

[[nodiscard]] bool enabled() noexcept {
    const VcsConfiguration &config = vcs_configuration();
    return config.initialized && config.display.show_fps;
}

// Five columns, seven rows, most-significant row first. Only the glyphs used
// by "FPS 00.0" are carried, keeping the overlay independent from a font file.
struct Glyph {
    char character;
    std::array<std::uint8_t, 7> rows;
};

constexpr std::array<Glyph, 14> kGlyphs{{
    {'0', {0x0Eu, 0x11u, 0x13u, 0x15u, 0x19u, 0x11u, 0x0Eu}},
    {'1', {0x04u, 0x0Cu, 0x04u, 0x04u, 0x04u, 0x04u, 0x0Eu}},
    {'2', {0x0Eu, 0x11u, 0x01u, 0x02u, 0x04u, 0x08u, 0x1Fu}},
    {'3', {0x1Eu, 0x01u, 0x01u, 0x0Eu, 0x01u, 0x01u, 0x1Eu}},
    {'4', {0x02u, 0x06u, 0x0Au, 0x12u, 0x1Fu, 0x02u, 0x02u}},
    {'5', {0x1Fu, 0x10u, 0x10u, 0x1Eu, 0x01u, 0x01u, 0x1Eu}},
    {'6', {0x0Eu, 0x10u, 0x10u, 0x1Eu, 0x11u, 0x11u, 0x0Eu}},
    {'7', {0x1Fu, 0x01u, 0x02u, 0x04u, 0x08u, 0x08u, 0x08u}},
    {'8', {0x0Eu, 0x11u, 0x11u, 0x0Eu, 0x11u, 0x11u, 0x0Eu}},
    {'9', {0x0Eu, 0x11u, 0x11u, 0x0Fu, 0x01u, 0x01u, 0x0Eu}},
    {'F', {0x1Fu, 0x10u, 0x10u, 0x1Eu, 0x10u, 0x10u, 0x10u}},
    {'P', {0x1Eu, 0x11u, 0x11u, 0x1Eu, 0x10u, 0x10u, 0x10u}},
    {'S', {0x0Fu, 0x10u, 0x10u, 0x0Eu, 0x01u, 0x01u, 0x1Eu}},
    {'.', {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x0Cu, 0x0Cu}},
}};

[[nodiscard]] const Glyph *glyph_for(char character) noexcept {
    for (const Glyph &glyph : kGlyphs)
        if (glyph.character == character) return &glyph;
    return nullptr;
}

void emit_quad(std::vector<GeGpuVertex> &vertices, float x0, float y0,
               float x1, float y1, std::uint32_t color) {
    GeGpuVertex a{}, b{}, c{}, d{};
    a.x = x0; a.y = y0; a.z = 0.0f; a.rgba = color;
    b.x = x1; b.y = y0; b.z = 0.0f; b.rgba = color;
    c.x = x1; c.y = y1; c.z = 0.0f; c.rgba = color;
    d.x = x0; d.y = y1; d.z = 0.0f; d.rgba = color;
    vertices.push_back(a); vertices.push_back(b); vertices.push_back(c);
    vertices.push_back(a); vertices.push_back(c); vertices.push_back(d);
}

void emit_text(std::vector<GeGpuVertex> &vertices, const char *text,
               float origin_x, float origin_y, float scale,
               std::uint32_t color) {
    float pen_x = origin_x;
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        const Glyph *glyph = glyph_for(*cursor);
        if (glyph != nullptr) {
            for (std::size_t row = 0; row < glyph->rows.size(); ++row) {
                for (std::size_t column = 0; column < 5u; ++column) {
                    if ((glyph->rows[row] & (1u << (4u - column))) == 0u) continue;
                    const float x = pen_x + static_cast<float>(column) * scale;
                    const float y = origin_y + static_cast<float>(row) * scale;
                    emit_quad(vertices, x, y, x + scale, y + scale, color);
                }
            }
        }
        pen_x += 6.0f * scale;
    }
}

} // namespace

void fps_overlay_observe_draw(const GeGpuDrawDescriptor &draw,
                              std::uint32_t vertex_weight) noexcept {
    if (!enabled() || draw.clear_mode || vertex_weight == 0u) return;
    const std::uint32_t target = draw.framebuffer_address & 0x001FFFF0u;
    if (target == 0u) return;
    // The overlay only needs framebuffer layout; every other state field is
    // overwritten when the glyphs are emitted. Heavy world frames can contain
    // ~1000 draws to the same target, so do not linearly scan/copy a descriptor
    // on each one. The first non-clear draw for a target is sufficient.
    if (g_last_observed_target == target) return;
    g_last_observed_target = target;
    if (find_target(target) != nullptr) return;
    if (g_target_count >= g_targets.size()) return;
    TrackedTarget &tracked = g_targets[g_target_count++];
    tracked.address = target;
    tracked.candidate = TargetCandidate{};
    tracked.candidate.draw = draw;
}

void fps_overlay_render_frame(std::uint32_t selected_framebuffer) noexcept {
    if (!enabled() || !ge_gpu_backend_graphics_ready()) {
        clear_targets();
        return;
    }

    const std::uint32_t target = selected_framebuffer & 0x001FFFF0u;
    TargetCandidate *found = target != 0u ? find_target(target) : nullptr;
    if (found == nullptr) {
        clear_targets();
        return;
    }
    GeGpuDrawDescriptor draw = found->draw;
    clear_targets();

    const auto now = std::chrono::steady_clock::now();
    if (g_measurement_start.time_since_epoch().count() == 0) {
        g_measurement_start = now;
        g_measurement_frames = 0u;
    }
    ++g_measurement_frames;
    const double seconds = std::chrono::duration<double>(now - g_measurement_start).count();
    if (seconds >= 0.5) {
        g_fps = static_cast<double>(g_measurement_frames) / seconds;
        g_measurement_frames = 0u;
        g_measurement_start = now;
    }

    char label[32]{};
    if (g_fps > 0.0)
        std::snprintf(label, sizeof(label), "FPS %.1f", std::clamp(g_fps, 0.0, 999.9));
    else
        std::snprintf(label, sizeof(label), "FPS --.-");

    static thread_local std::vector<GeGpuVertex> vertices;
    vertices.clear();
    vertices.reserve(1200u);
    // One-pixel black offset keeps the counter legible against sky and HUD;
    // the foreground is PSP-style warm white, not a debug-neon color.
    emit_text(vertices, label, 9.0f, 9.0f, 1.5f, 0xFF000000u);
    emit_text(vertices, label, 8.0f, 8.0f, 1.5f, 0xFFE8F8FFu);

    draw.primitive = 3u;
    draw.vertex_count = static_cast<std::uint32_t>(vertices.size());
    draw.vertex_type = 0u;
    draw.through = true;
    draw.widescreen_hud = false;
    draw.texture_enabled = false;
    draw.texture_address = 0u;
    draw.texture_format = 0u;
    draw.texture_content_signature = 0u;
    draw.texture_use_alpha = false;
    draw.texture_double_color = false;
    draw.blend_enabled = false;
    draw.color_write_mask = 0u;
    draw.alpha_test_enabled = false;
    draw.depth_test_enabled = false;
    draw.depth_write_enabled = false;
    draw.fog_enabled = false;
    draw.clear_mode = false;
    draw.scissor_x0 = 0;
    draw.scissor_y0 = 0;
    draw.scissor_x1 = 479;
    draw.scissor_y1 = 271;
    ge_gpu_backend_accumulate_color_triangles(draw, vertices);
}

} // namespace vcs
