#include "perf_overlay.hpp"

#include "frame_stats.hpp"

#include <algorithm>
#include <cstdio>

namespace mhp3rd::perf {
namespace {

// A 5x7 font written for this overlay: upper-case letters, digits and the
// few symbols the numbers need. Each row is five bits, the leftmost pixel in
// bit 4.
struct Glyph {
    char character;
    std::uint8_t rows[7];
};

constexpr Glyph kFont[] = {
    {'0', {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110}},
    {'1', {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}},
    {'2', {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}},
    {'3', {0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110}},
    {'4', {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}},
    {'5', {0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110}},
    {'6', {0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}},
    {'7', {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}},
    {'8', {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}},
    {'9', {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100}},
    {'A', {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}},
    {'B', {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110}},
    {'C', {0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110}},
    {'D', {0b11100, 0b10010, 0b10001, 0b10001, 0b10001, 0b10010, 0b11100}},
    {'E', {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}},
    {'F', {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000}},
    {'G', {0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111}},
    {'H', {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}},
    {'I', {0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}},
    {'J', {0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100}},
    {'K', {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001}},
    {'L', {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}},
    {'M', {0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001}},
    {'N', {0b10001, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001}},
    {'O', {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}},
    {'P', {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}},
    {'Q', {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101}},
    {'R', {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001}},
    {'S', {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110}},
    {'T', {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}},
    {'U', {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}},
    {'V', {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100}},
    {'W', {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010}},
    {'X', {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001}},
    {'Y', {0b10001, 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100}},
    {'Z', {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111}},
    {'.', {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b01100}},
    {'%', {0b11000, 0b11001, 0b00010, 0b00100, 0b01000, 0b10011, 0b00011}},
    {':', {0b00000, 0b01100, 0b01100, 0b00000, 0b01100, 0b01100, 0b00000}},
    {'-', {0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000}},
    {'/', {0b00000, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b00000}},
};

constexpr std::uint32_t kGlyphAdvance = 6u;
constexpr std::uint32_t kLineAdvance = 9u;
constexpr std::uint32_t kMargin = 2u;
constexpr std::uint32_t kGraphTop = kMargin + 4u * kLineAdvance + 1u;
constexpr std::uint32_t kGraphHeight = kOverlayHeight - kGraphTop - kMargin;
constexpr float kGraphRangeMs = 50.0f;

constexpr std::uint32_t rgb(std::uint32_t r, std::uint32_t g, std::uint32_t b) {
    return r | (g << 8u) | (b << 16u) | 0xFF000000u;
}
constexpr std::uint32_t kBackground = rgb(16u, 16u, 16u);
constexpr std::uint32_t kText = rgb(235u, 235u, 235u);
constexpr std::uint32_t kGuide = rgb(70u, 70u, 70u);
constexpr std::uint32_t kGood = rgb(80u, 210u, 90u);
constexpr std::uint32_t kSlow = rgb(235u, 200u, 60u);
constexpr std::uint32_t kBad = rgb(235u, 70u, 60u);

const Glyph *find_glyph(char character) {
    if (character >= 'a' && character <= 'z') character = static_cast<char>(character - 'a' + 'A');
    for (const Glyph &glyph : kFont)
        if (glyph.character == character) return &glyph;
    return nullptr;
}

void draw_text(std::uint32_t *pixels, std::uint32_t x, std::uint32_t y, const char *text) {
    for (; *text != '\0' && x + 5u <= kOverlayWidth; ++text, x += kGlyphAdvance) {
        const Glyph *glyph = find_glyph(*text);
        if (glyph == nullptr) continue;
        for (std::uint32_t row = 0; row < 7u; ++row) {
            for (std::uint32_t column = 0; column < 5u; ++column) {
                if ((glyph->rows[row] & (0x10u >> column)) != 0u)
                    pixels[(y + row) * kOverlayWidth + x + column] = kText;
            }
        }
    }
}

std::uint32_t graph_row(float ms) {
    const float clamped = std::clamp(ms, 0.0f, kGraphRangeMs);
    return kGraphTop + kGraphHeight - 1u -
           static_cast<std::uint32_t>(clamped / kGraphRangeMs * static_cast<float>(kGraphHeight - 1u));
}

} // namespace

std::uint32_t overlay_scale(std::uint32_t height) { return std::max(1u, height / 360u); }

void draw_overlay(std::uint32_t *pixels) {
    std::fill(pixels, pixels + kOverlayWidth * kOverlayHeight, kBackground);

    const Summary &s = last_second();
    char line[48];
    std::uint32_t y = kMargin;
    if (s.valid) {
        std::snprintf(line, sizeof(line), "FPS %.1f GAME %.1f SPD %.0f%%", s.fps, s.game_fps, s.speed * 100.0);
        draw_text(pixels, kMargin, y, line);
        if (s.gpu_valid)
            std::snprintf(line, sizeof(line), "FRAME %.1f MAX %.1f GPU %.1f", s.frame_avg_ms, s.frame_max_ms,
                          s.gpu_avg_ms);
        else
            std::snprintf(line, sizeof(line), "FRAME %.1f MAX %.1f MS", s.frame_avg_ms, s.frame_max_ms);
        draw_text(pixels, kMargin, y += kLineAdvance, line);
        std::snprintf(line, sizeof(line), "GUEST %.1f RENDER %.1f WAIT %.1f", s.guest_ms, s.render_ms, s.wait_ms);
        draw_text(pixels, kMargin, y += kLineAdvance, line);
        if (s.refresh_hz > 0.0f)
            std::snprintf(line, sizeof(line), "%s %.0fHZ %uX%u", s.present_mode.c_str(), s.refresh_hz, s.width,
                          s.height);
        else
            std::snprintf(line, sizeof(line), "%s %uX%u", s.present_mode.c_str(), s.width, s.height);
        draw_text(pixels, kMargin, y += kLineAdvance, line);
    } else {
        draw_text(pixels, kMargin, y, "MEASURING");
    }

    // Guides at one and two 60 Hz frames, then one bar per frame, oldest first.
    for (const float guide : {1000.0f / 60.0f, 2000.0f / 60.0f}) {
        const std::uint32_t row = graph_row(guide);
        for (std::uint32_t x = kMargin; x < kOverlayWidth - kMargin; x += 2u) pixels[row * kOverlayWidth + x] = kGuide;
    }
    const auto &history = frame_history();
    const std::size_t cursor = history_cursor();
    const std::uint32_t bottom = kGraphTop + kGraphHeight - 1u;
    for (std::size_t i = 0; i < kHistoryFrames && kMargin + i < kOverlayWidth - kMargin; ++i) {
        const float ms = history[(cursor + i) % kHistoryFrames];
        if (ms <= 0.0f) continue;
        const std::uint32_t color = ms <= 34.0f ? kGood : ms <= 50.0f ? kSlow : kBad;
        const std::uint32_t x = kMargin + static_cast<std::uint32_t>(i);
        for (std::uint32_t row = graph_row(ms); row <= bottom; ++row) pixels[row * kOverlayWidth + x] = color;
    }
}

} // namespace mhp3rd::perf
