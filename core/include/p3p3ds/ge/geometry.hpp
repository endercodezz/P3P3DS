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

// One vertex as stored (morph targets already blended): model-space position
// and normal (s8/s16 scaled to [-1, 1) in transform mode, raw in through
// mode), UV (texels in through mode, else normalised), colour (the material
// ambient colour when the vertex has none) and skinning weights.
struct ModelVertex {
    float pos[3]{}, normal[3]{}, uv[2]{}, weights[8]{};
    Rgba color{255, 255, 255, 255};
    bool has_color{};
};

// Vertex `index` of the buffer at `vertex_address`; false outside guest memory.
bool decode_model_vertex(psprecomp::GuestMemory &memory, const GeRegisters &regs, const VertexLayout &layout,
                         std::uint32_t vertex_address, std::uint32_t index, ModelVertex &out);
// Vertices first .. first + count - 1, the same values as decode_model_vertex,
// in one pass over guest memory. False (out untouched) for morphing, through
// mode or a range outside guest memory: the caller then decodes per vertex.
bool decode_model_vertices(psprecomp::GuestMemory &memory, const GeRegisters &regs, const VertexLayout &layout,
                           std::uint32_t vertex_address, std::uint32_t first, std::uint32_t count, ModelVertex *out);
// i-th index of an indexed draw (i itself when VTYPE has no index buffer).
[[nodiscard]] std::uint32_t vertex_index(psprecomp::GuestMemory &memory, const VertexLayout &layout,
                                         std::uint32_t index_address, std::uint32_t i);
// Bone-weighted position and normal (the model ones when VTYPE has no weights).
void skin_vertex(const GeRegisters &regs, const VertexLayout &layout, const ModelVertex &v, float pos[3], float normal[3]);

// GE lighting parameters of a draw (LIGHTING_ENABLE 0x17 .. light colours 0x9A).
struct LightingSetup {
    struct Light {
        bool enabled{};
        std::uint32_t kind{};       // 0 directional, 1 point, 2 spot
        std::uint32_t components{}; // 0 diffuse, 1 diffuse + specular, 2 powered diffuse
        float position[3]{}, direction[3]{}, attenuation[3]{};
        float spot_exponent{}, spot_cutoff{};
        std::array<float, 4> ambient{}, diffuse{}, specular{};
    };
    bool enabled{};
    bool vertex_ambient{}, vertex_diffuse{}, vertex_specular{}, reverse_normals{};
    std::array<float, 4> emissive{}, material_ambient{}, material_diffuse{}, material_specular{}, scene_ambient{};
    float specular_power{};
    std::array<Light, 4> lights{};
};
[[nodiscard]] LightingSetup lighting_setup(const GeRegisters &regs, const VertexLayout &layout);
// Lit colour of a vertex from its world-space position and normal.
[[nodiscard]] Rgba light_vertex(const LightingSetup &s, const float pos[3], const float normal[3], const Rgba &vertex_color);

// Decodes `count` vertices (indexed when VTYPE says so), skins, lights and
// maps them to screen space. `out` is resized to `count`.
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
