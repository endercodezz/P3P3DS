#pragma once
// Target-agnostic GE front end shared by every renderer backend: vertex
// decoding + transform to screen space, texture parameters and texel decoding.
// SoftwareRenderer rasterizes the result on the CPU; the New 3DS backend
// (platform/3ds/gpu_renderer.cpp) hands it to the PICA200.
// Register meanings: PSPSDK psp/pspsdk/src/gu/pspge.h / pspgu.h.
#include "p3p3ds/ge/renderer.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace p3p3ds::ge {

using Rgba = std::array<std::uint8_t, 4>; // r, g, b, a

struct ScreenVertex {
    // Screen pixels (x, y), depth 0..65535 (z) and clip w (1 in through mode).
    float x{}, y{}, z{}, w{1.0f};
    float u{}, v{}; // texels
    Rgba color{255, 255, 255, 255};
    bool clipped{}; // outside guest memory, or behind the eye (w <= 0)
};

struct TextureInfo {
    std::uint32_t address{}, stride{}, width{1}, height{1}, format{}, clut_format{};
    bool swizzled{};
};

[[nodiscard]] Rgba unpack16(std::uint32_t c, std::uint32_t format);
[[nodiscard]] Rgba unpack32(std::uint32_t c);

// Texturing applies to this draw (TEXTURE_ENABLE, UVs present, not clear mode).
[[nodiscard]] bool draw_is_textured(const GeRegisters &regs, const VertexLayout &layout);
[[nodiscard]] TextureInfo texture_info(const GeRegisters &regs);

// Decodes `count` vertices (indexed when VTYPE says so) and maps them to
// screen space. `out` is resized to `count`.
void decode_screen_vertices(psprecomp::GuestMemory &memory, const GeRegisters &regs, std::uint32_t count,
                            std::uint32_t vertex_address, std::uint32_t index_address, std::vector<ScreenVertex> &out);

// Level-0 texel (x, y) after CLUT lookup; {0,0,0,0} outside guest memory,
// magenta for DXT (not observed in P3P).
[[nodiscard]] Rgba fetch_texel(psprecomp::GuestMemory &memory, const GeRegisters &regs, const TextureInfo &t,
                               std::int32_t x, std::int32_t y);

// Whole level-0 texture, row-major from the top row, one RGBA texel per
// uint32 (r in the low byte); identical to fetch_texel for every texel.
void decode_texture(psprecomp::GuestMemory &memory, const GeRegisters &regs, const TextureInfo &t,
                    std::vector<std::uint32_t> &out);

// 64-bit content hash of the texture bytes plus, for CLUT formats, the
// palette entries it can reach. Used to detect texture changes.
[[nodiscard]] std::uint64_t texture_hash(psprecomp::GuestMemory &memory, const GeRegisters &regs, const TextureInfo &t);

} // namespace p3p3ds::ge
