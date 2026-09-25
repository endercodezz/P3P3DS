#pragma once

#include <cstdint>

namespace mhp3rd::perf {

// The overlay is drawn on the CPU into a small RGBA image, which the renderer
// scales onto the top-left corner of the presented frame.
inline constexpr std::uint32_t kOverlayWidth = 196u;
inline constexpr std::uint32_t kOverlayHeight = 72u;

// Fills `pixels` (kOverlayWidth x kOverlayHeight, red in the low byte) with the
// last second's numbers and a graph of recent frame times.
void draw_overlay(std::uint32_t *pixels);

// Integer scale for an overlay drawn over an image `height` pixels tall, so it
// stays readable from a 544-line window up to a 4K screen.
[[nodiscard]] std::uint32_t overlay_scale(std::uint32_t height);

} // namespace mhp3rd::perf
