#pragma once
// Writes a PSP framebuffer from guest VRAM as a 24-bit BMP (480x272 visible).
#include "psprecomp/guest_memory.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>
namespace p3p3ds {
inline std::vector<std::uint8_t> framebuffer_rgb(const psprecomp::GuestMemory &memory, std::uint32_t address,
                                                 std::uint32_t stride, std::uint32_t format,
                                                 std::uint32_t width = 480, std::uint32_t height = 272) {
    std::vector<std::uint8_t> rgb(width * height * 3u, 0);
    const std::uint32_t bpp = format == 3 ? 4u : 2u;
    for (std::uint32_t y = 0; y < height; ++y)
        for (std::uint32_t x = 0; x < width; ++x) {
            const auto a = address + (y * stride + x) * bpp;
            if (!memory.contains(a, bpp)) continue;
            std::uint8_t r, g, b;
            if (format == 3) { const auto c = memory.load32(a); r = c & 0xFF; g = (c >> 8) & 0xFF; b = (c >> 16) & 0xFF; }
            else {
                const auto c = memory.load16(a);
                if (format == 0) { r = (c & 0x1F) << 3; g = ((c >> 5) & 0x3F) << 2; b = ((c >> 11) & 0x1F) << 3; }
                else if (format == 1) { r = (c & 0x1F) << 3; g = ((c >> 5) & 0x1F) << 3; b = ((c >> 10) & 0x1F) << 3; }
                else { r = (c & 0xF) * 17; g = ((c >> 4) & 0xF) * 17; b = ((c >> 8) & 0xF) * 17; }
            }
            auto *p = &rgb[(y * width + x) * 3u];
            p[0] = r; p[1] = g; p[2] = b;
        }
    return rgb;
}
inline void write_bmp(const std::filesystem::path &path, const std::vector<std::uint8_t> &rgb,
                      std::uint32_t width = 480, std::uint32_t height = 272) {
    const std::uint32_t row = (width * 3u + 3u) & ~3u, size = 54u + row * height;
    std::vector<std::uint8_t> out(size, 0);
    auto put32 = [&](std::size_t at, std::uint32_t v) { for (int i = 0; i < 4; ++i) out[at + i] = static_cast<std::uint8_t>(v >> (8 * i)); };
    out[0] = 'B'; out[1] = 'M'; put32(2, size); put32(10, 54); put32(14, 40); put32(18, width); put32(22, height);
    out[26] = 1; out[28] = 24; put32(34, row * height);
    for (std::uint32_t y = 0; y < height; ++y)
        for (std::uint32_t x = 0; x < width; ++x) {
            const auto *p = &rgb[((height - 1u - y) * width + x) * 3u];
            auto *q = &out[54u + y * row + x * 3u];
            q[0] = p[2]; q[1] = p[1]; q[2] = p[0];
        }
    std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char *>(out.data()), static_cast<std::streamsize>(out.size()));
}
inline std::uint64_t fnv1a(const std::vector<std::uint8_t> &data) {
    std::uint64_t h = 0xCBF29CE484222325ull;
    for (auto b : data) { h ^= b; h *= 0x100000001B3ull; }
    return h;
}
} // namespace p3p3ds
