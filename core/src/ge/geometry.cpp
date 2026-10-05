// Shared GE front end (see geometry.hpp): vertex decode (morphing, skinning,
// lighting), transform, texel fetch.
// [INFERRED] sprite UV mapping and transform-mode clipping (vertices behind
// the eye are marked clipped, no near-plane clipping) as in the first
// software renderer.
#include "p3p3ds/ge/geometry.hpp"

#include "psprecomp/guest_memory.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace p3p3ds::ge {
namespace {

std::uint32_t component_size(std::uint32_t format, std::uint32_t one, std::uint32_t two, std::uint32_t three) {
    return format == 1 ? one : format == 2 ? two : format == 3 ? three : 0u;
}

std::uint32_t r24(const GeRegisters &g, std::uint32_t i) { return g.reg[i] & 0xFFFFFFu; }

constexpr std::uint32_t kBitsPerTexel[11] = {16, 16, 16, 32, 4, 8, 16, 32, 4, 8, 8};
std::uint32_t texel_bits(std::uint32_t format) { return format < 11 ? kBitsPerTexel[format] : 32u; }

std::uint32_t clut_lookup(const GeRegisters &g, std::uint32_t index) {
    const auto cf = r24(g, 0xC5);
    const auto shift = (cf >> 2) & 0x1F, mask = (cf >> 8) & 0xFF, start = (cf >> 16) & 0x1F;
    const auto i = (((index >> shift) & mask) | (start << 4)) & 0x1FF;
    const auto pf = cf & 3;
    if (pf == 3) return g.clut[i & 0xFF];
    const auto word = g.clut[(i >> 1) & 0xFF];
    return (i & 1u) ? (word >> 16) : (word & 0xFFFF);
}

Rgba convert_entry(std::uint32_t entry, std::uint32_t clut_format) {
    return clut_format == 3 ? unpack32(entry) : unpack16(entry, clut_format);
}

std::uint32_t texel_offset(const TextureInfo &t, std::uint32_t bpp, std::uint32_t x, std::uint32_t y) {
    const std::uint32_t row_bytes = t.stride * bpp / 8u;
    const std::uint32_t xbits = x * bpp;
    if (t.swizzled) {
        const auto xbytes = xbits / 8u;
        const auto bx = xbytes / 16u, by = y / 8u;
        return (by * (row_bytes / 16u) + bx) * 128u + (y % 8u) * 16u + xbytes % 16u;
    }
    return y * row_bytes + xbits / 8u;
}

std::uint32_t pack_rgba(const Rgba &c) {
    return c[0] | (c[1] << 8) | (c[2] << 16) | (static_cast<std::uint32_t>(c[3]) << 24);
}

} // namespace

Rgba unpack16(std::uint32_t c, std::uint32_t format) {
    auto expand = [](std::uint32_t v, int bits) { return static_cast<std::uint8_t>((v << (8 - bits)) | (v >> (2 * bits - 8 > 0 ? 2 * bits - 8 : 0))); };
    switch (format) {
    case 0: return {expand(c & 0x1F, 5), expand((c >> 5) & 0x3F, 6), expand((c >> 11) & 0x1F, 5), 255};
    case 1: return {expand(c & 0x1F, 5), expand((c >> 5) & 0x1F, 5), expand((c >> 10) & 0x1F, 5), static_cast<std::uint8_t>((c >> 15) ? 255 : 0)};
    default: return {static_cast<std::uint8_t>((c & 0xF) * 17), static_cast<std::uint8_t>(((c >> 4) & 0xF) * 17),
                     static_cast<std::uint8_t>(((c >> 8) & 0xF) * 17), static_cast<std::uint8_t>(((c >> 12) & 0xF) * 17)};
    }
}

Rgba unpack32(std::uint32_t c) {
    return {static_cast<std::uint8_t>(c), static_cast<std::uint8_t>(c >> 8), static_cast<std::uint8_t>(c >> 16), static_cast<std::uint8_t>(c >> 24)};
}

bool draw_is_textured(const GeRegisters &regs, const VertexLayout &layout) {
    return (regs.reg[0x1E] & 1u) != 0u && layout.uv_format != 0u && (r24(regs, 0xD3) & 1u) == 0u;
}

TextureInfo texture_info(const GeRegisters &regs) {
    TextureInfo t;
    t.address = (r24(regs, 0xA0) & 0xFFFFF0u) | ((r24(regs, 0xA8) >> 16) & 0xFFu) << 24;
    t.stride = r24(regs, 0xA8) & 0xFFFFu;
    t.width = 1u << (r24(regs, 0xB8) & 0xFu);
    t.height = 1u << ((r24(regs, 0xB8) >> 8) & 0xFu);
    t.format = r24(regs, 0xC3) & 0xFu;
    t.swizzled = (r24(regs, 0xC2) & 1u) != 0u;
    t.clut_format = r24(regs, 0xC5) & 3u;
    return t;
}

namespace {

// Component reader over a host pointer to one vertex (little endian).
// scale8/scale16 are 1, 128 or 32768: powers of two, so multiplying by the
// reciprocal gives exactly the quotient (and avoids a division per component).
inline float read_component(const std::uint8_t *p, std::uint32_t format, bool signed_value, float scale8, float scale16) {
    switch (format) {
    case 1: return (signed_value ? static_cast<float>(static_cast<std::int8_t>(p[0])) : static_cast<float>(p[0])) * (1.0f / scale8);
    case 2: {
        const std::uint16_t raw = static_cast<std::uint16_t>(p[0] | (p[1] << 8));
        return (signed_value ? static_cast<float>(static_cast<std::int16_t>(raw)) : static_cast<float>(raw)) * (1.0f / scale16);
    }
    case 3: { float f; std::memcpy(&f, p, 4); return f; }
    default: return 0.0f;
    }
}

Rgba read_color(const std::uint8_t *p, std::uint32_t format) {
    if (format == 7u) return unpack32(static_cast<std::uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16)) | (static_cast<std::uint32_t>(p[3]) << 24));
    return unpack16(static_cast<std::uint32_t>(p[0] | (p[1] << 8)), format == 4u ? 0u : format == 5u ? 1u : 2u);
}

// One morph target of one vertex. Weights: u8 0x80 and u16 0x8000 are 1.0
// [INFERRED from the s8/s16 position scaling]; normals scale like positions
// (they are normalised before lighting).
inline void decode_one(const std::uint8_t *p, const VertexLayout &l, ModelVertex &v) {
    const auto pe = component_size(l.pos_format, 1, 2, 4);
    if (l.through) {
        // Through mode: s16/float screen coordinates (x, y), u16 z; integer texel UVs.
        v.pos[0] = read_component(p + l.pos_offset, l.pos_format, true, 1.0f, 1.0f);
        v.pos[1] = read_component(p + l.pos_offset + pe, l.pos_format, true, 1.0f, 1.0f);
        v.pos[2] = read_component(p + l.pos_offset + 2 * pe, l.pos_format, false, 1.0f, 1.0f);
    } else {
        for (int i = 0; i < 3; ++i) v.pos[i] = read_component(p + l.pos_offset + i * pe, l.pos_format, true, 128.0f, 32768.0f);
    }
    if (l.uv_format) {
        const auto ue = component_size(l.uv_format, 1, 2, 4);
        const float s8 = l.through ? 1.0f : 128.0f, s16 = l.through ? 1.0f : 32768.0f;
        v.uv[0] = read_component(p + l.uv_offset, l.uv_format, false, s8, s16);
        v.uv[1] = read_component(p + l.uv_offset + ue, l.uv_format, false, s8, s16);
    }
    if (l.normal_format) {
        const auto ne = component_size(l.normal_format, 1, 2, 4);
        for (int i = 0; i < 3; ++i) v.normal[i] = read_component(p + l.normal_offset + i * ne, l.normal_format, true, 128.0f, 32768.0f);
    }
    if (l.weights) {
        const auto we = component_size(l.weight_format, 1, 2, 4);
        for (std::uint32_t i = 0; i < l.weights && i < 8u; ++i)
            v.weights[i] = read_component(p + l.weight_offset + i * we, l.weight_format, false, 128.0f, 32768.0f);
    }
    if (l.color_format >= 4u) v.color = read_color(p + l.color_offset, l.color_format);
}

// v = m * (x, y, z, w) for a GE 4x3 matrix (four columns of three floats).
void transform43(const float *m, const float *p, float w, float *out) {
    for (int j = 0; j < 3; ++j) out[j] = p[0] * m[j] + p[1] * m[3 + j] + p[2] * m[6 + j] + w * m[9 + j];
}

float clamp01(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }

} // namespace

bool decode_model_vertex(psprecomp::GuestMemory &memory, const GeRegisters &regs, const VertexLayout &layout,
                         std::uint32_t vertex_address, std::uint32_t index, ModelVertex &out) {
    out = ModelVertex{};
    const std::uint32_t morphs = layout.through ? 1u : std::max<std::uint32_t>(layout.morphs, 1u);
    const std::uint32_t stride = layout.size * morphs;
    const std::uint8_t *p = memory.raw_pointer(vertex_address + index * stride, std::max<std::uint32_t>(stride, 1u));
    if (p == nullptr) return false;
    if (morphs == 1u) {
        out.color = unpack32((r24(regs, 0x55) & 0xFFFFFFu) | ((r24(regs, 0x58) & 0xFFu) << 24));
        out.has_color = layout.color_format >= 4u;
        decode_one(p, layout, out);
        return true;
    }
    // Morphing: every attribute is the weighted sum of the morph targets
    // (MORPH_WEIGHT0.. 0x2C-0x33, PSPSDK sceGuMorphWeight).
    ModelVertex sum;
    float color[4] = {};
    for (std::uint32_t k = 0; k < morphs; ++k) {
        ModelVertex t;
        t.color = unpack32((r24(regs, 0x55) & 0xFFFFFFu) | ((r24(regs, 0x58) & 0xFFu) << 24));
        decode_one(p + k * layout.size, layout, t);
        const float w = ge_float(r24(regs, 0x2C + k));
        for (int i = 0; i < 3; ++i) { sum.pos[i] += w * t.pos[i]; sum.normal[i] += w * t.normal[i]; }
        for (int i = 0; i < 2; ++i) sum.uv[i] += w * t.uv[i];
        for (int i = 0; i < 8; ++i) sum.weights[i] += w * t.weights[i];
        for (int i = 0; i < 4; ++i) color[i] += w * t.color[i];
    }
    for (int i = 0; i < 4; ++i) sum.color[i] = static_cast<std::uint8_t>(std::clamp(color[i], 0.0f, 255.0f));
    sum.has_color = layout.color_format >= 4u;
    out = sum;
    return true;
}

bool decode_model_vertices(psprecomp::GuestMemory &memory, const GeRegisters &regs, const VertexLayout &layout,
                           std::uint32_t vertex_address, std::uint32_t first, std::uint32_t count, ModelVertex *out) {
    if (layout.through || layout.morphs > 1u || count == 0u) return false;
    const std::uint32_t size = layout.size;
    const std::uint8_t *base = memory.raw_pointer(vertex_address + first * size, std::max<std::uint32_t>(count * size, 1u));
    if (base == nullptr) return false;
    ModelVertex blank;
    blank.color = unpack32((r24(regs, 0x55) & 0xFFFFFFu) | ((r24(regs, 0x58) & 0xFFu) << 24));
    blank.has_color = layout.color_format >= 4u;
    for (std::uint32_t i = 0; i < count; ++i) {
        out[i] = blank;
        decode_one(base + i * size, layout, out[i]);
    }
    return true;
}

std::uint32_t vertex_index(psprecomp::GuestMemory &memory, const VertexLayout &layout, std::uint32_t index_address, std::uint32_t i) {
    if (layout.index_format == 1u) { const auto *p = memory.raw_pointer(index_address + i, 1u); return p ? p[0] : 0u; }
    if (layout.index_format == 2u) { const auto *p = memory.raw_pointer(index_address + 2u * i, 2u); return p ? static_cast<std::uint32_t>(p[0] | (p[1] << 8)) : 0u; }
    return i;
}

void skin_vertex(const GeRegisters &regs, const VertexLayout &layout, const ModelVertex &v, float pos[3], float normal[3]) {
    if (layout.weights == 0u) {
        std::memcpy(pos, v.pos, sizeof v.pos);
        std::memcpy(normal, v.normal, sizeof v.normal);
        return;
    }
    // Skinning: sum over the vertex weights of BONE_MATRIX[i] (4x3, PSPSDK
    // sceGuBoneMatrix) applied to the position (w = 1) and normal (w = 0).
    pos[0] = pos[1] = pos[2] = normal[0] = normal[1] = normal[2] = 0.0f;
    for (std::uint32_t i = 0; i < layout.weights && i < 8u; ++i) {
        const float w = v.weights[i];
        if (w == 0.0f) continue;
        float p[3], n[3];
        transform43(&regs.bone[i * 12u], v.pos, 1.0f, p);
        transform43(&regs.bone[i * 12u], v.normal, 0.0f, n);
        for (int k = 0; k < 3; ++k) { pos[k] += w * p[k]; normal[k] += w * n[k]; }
    }
}

LightingSetup lighting_setup(const GeRegisters &regs, const VertexLayout &layout) {
    LightingSetup s;
    s.enabled = !layout.through && (regs.reg[0x17] & 1u) != 0u;
    if (!s.enabled) return s;
    auto color = [&](std::uint32_t r) {
        const auto c = r24(regs, r);
        return std::array<float, 4>{(c & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f, ((c >> 16) & 0xFF) / 255.0f, 0.0f};
    };
    s.emissive = color(0x54);
    s.material_ambient = color(0x55);
    s.material_ambient[3] = (r24(regs, 0x58) & 0xFF) / 255.0f;
    s.material_diffuse = color(0x56);
    s.material_specular = color(0x57);
    s.scene_ambient = color(0x5C);
    s.scene_ambient[3] = (r24(regs, 0x5D) & 0xFF) / 255.0f;
    s.specular_power = ge_float(r24(regs, 0x5B));
    const auto update = layout.color_format >= 4u ? r24(regs, 0x53) & 7u : 0u;
    s.vertex_ambient = (update & 1u) != 0u;
    s.vertex_diffuse = (update & 2u) != 0u;
    s.vertex_specular = (update & 4u) != 0u;
    s.reverse_normals = (r24(regs, 0x51) & 1u) != 0u;
    for (std::uint32_t i = 0; i < 4u; ++i) {
        auto &l = s.lights[i];
        l.enabled = (regs.reg[0x18 + i] & 1u) != 0u;
        if (!l.enabled) continue;
        const auto type = r24(regs, 0x5F + i);
        l.kind = (type >> 8) & 3u;         // 0 directional, 1 point, 2 spot
        l.components = type & 3u;          // 0 diffuse, 1 diffuse + specular, 2 powered diffuse
        for (int k = 0; k < 3; ++k) {
            l.position[k] = ge_float(r24(regs, 0x63 + i * 3 + k));
            l.direction[k] = ge_float(r24(regs, 0x6F + i * 3 + k));
            l.attenuation[k] = ge_float(r24(regs, 0x7B + i * 3 + k));
        }
        l.spot_exponent = ge_float(r24(regs, 0x87 + i));
        l.spot_cutoff = ge_float(r24(regs, 0x8B + i));
        l.ambient = color(0x8F + i * 3);
        l.diffuse = color(0x90 + i * 3);
        l.specular = color(0x91 + i * 3);
    }
    return s;
}

// GE vertex lighting in world space. Semantics (behaviour, not code) as
// documented by PSPSDK pspgu.h (sceGuLight*, sceGuMaterial, sceGuAmbient) and
// cross-checked against PPSSPP GPU/Software/Lighting.cpp: colour = emissive +
// material ambient x scene ambient + per light (ambient + N.L diffuse +
// (N.H)^power specular with H = L + (0,0,1)) x attenuation 1/(a + b d + c d^2)
// x spot (cos >= cutoff: cos^exponent); alpha = material ambient alpha x
// scene ambient alpha. [INFERRED] float arithmetic instead of the hardware's
// fixed-point rounding; the separate specular colour mode is added to the
// primary colour.
Rgba light_vertex(const LightingSetup &s, const float pos[3], const float normal_in[3], const Rgba &vertex_color) {
    float vc[4];
    for (int i = 0; i < 4; ++i) vc[i] = vertex_color[i] / 255.0f;
    const float *ma = s.vertex_ambient ? vc : s.material_ambient.data();
    const float *md = s.vertex_diffuse ? vc : s.material_diffuse.data();
    const float *ms = s.vertex_specular ? vc : s.material_specular.data();
    float n[3] = {normal_in[0], normal_in[1], normal_in[2]};
    if (s.reverse_normals) for (auto &c : n) c = -c;
    const float nl = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (nl > 0.0f) for (auto &c : n) c /= nl;
    float out[4];
    for (int i = 0; i < 3; ++i) out[i] = s.emissive[i] + ma[i] * s.scene_ambient[i];
    out[3] = ma[3] * s.scene_ambient[3];
    for (const auto &l : s.lights) {
        if (!l.enabled) continue;
        float L[3] = {l.position[0], l.position[1], l.position[2]};
        float att = 1.0f;
        if (l.kind != 0u) {
            for (int k = 0; k < 3; ++k) L[k] -= pos[k];
            const float d = std::sqrt(L[0] * L[0] + L[1] * L[1] + L[2] * L[2]);
            if (d > 0.0f) for (auto &c : L) c /= d;
            const float denom = l.attenuation[0] + l.attenuation[1] * d + l.attenuation[2] * d * d;
            att = denom > 0.0f ? clamp01(1.0f / denom) : 0.0f;
        } else {
            const float d = std::sqrt(L[0] * L[0] + L[1] * L[1] + L[2] * L[2]);
            if (d > 0.0f) for (auto &c : L) c /= d;
            else { L[0] = 0.0f; L[1] = 0.0f; L[2] = 1.0f; }
        }
        if (l.kind == 2u) {
            float dir[3] = {l.direction[0], l.direction[1], l.direction[2]};
            const float dl = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
            if (dl > 0.0f) for (auto &c : dir) c /= dl;
            const float cosine = dir[0] * L[0] + dir[1] * L[1] + dir[2] * L[2];
            // Below the cutoff: 0; exponent <= 0: 1; else cos^exponent (0 for cos <= 0).
            att *= cosine < l.spot_cutoff ? 0.0f : l.spot_exponent <= 0.0f ? 1.0f : cosine > 0.0f ? std::pow(cosine, l.spot_exponent) : 0.0f;
        }
        float dot = n[0] * L[0] + n[1] * L[1] + n[2] * L[2];
        if (l.components == 2u && s.specular_power > 0.0f && dot > 0.0f) dot = std::pow(dot, s.specular_power);
        for (int i = 0; i < 3; ++i) out[i] += att * l.ambient[i] * ma[i];
        if (dot > 0.0f) for (int i = 0; i < 3; ++i) out[i] += att * dot * l.diffuse[i] * md[i];
        if (l.components == 1u && dot >= 0.0f) {
            float h[3] = {L[0], L[1], L[2] + 1.0f};
            const float hl = std::sqrt(h[0] * h[0] + h[1] * h[1] + h[2] * h[2]);
            if (hl > 0.0f) for (auto &c : h) c /= hl;
            float spec = n[0] * h[0] + n[1] * h[1] + n[2] * h[2];
            if (spec > 0.0f) {
                spec = s.specular_power > 0.0f ? std::pow(spec, s.specular_power) : 1.0f;
                for (int i = 0; i < 3; ++i) out[i] += att * spec * l.specular[i] * ms[i];
            }
        }
    }
    Rgba c;
    for (int i = 0; i < 4; ++i) c[i] = static_cast<std::uint8_t>(clamp01(out[i]) * 255.0f + 0.5f);
    return c;
}

void decode_screen_vertices(psprecomp::GuestMemory &memory, const GeRegisters &regs, std::uint32_t count,
                            std::uint32_t vertex_address, std::uint32_t index_address, std::vector<ScreenVertex> &out) {
    const auto layout = vertex_layout(r24(regs, 0x12));
    out.resize(count);
    ModelVertex m;
    if (layout.through) {
        for (std::uint32_t i = 0; i < count; ++i) {
            ScreenVertex &v = out[i];
            v = ScreenVertex{};
            if (!decode_model_vertex(memory, regs, layout, vertex_address, vertex_index(memory, layout, index_address, i), m)) {
                v.clipped = true;
                continue;
            }
            v.x = m.pos[0]; v.y = m.pos[1]; v.z = m.pos[2];
            v.u = m.uv[0]; v.v = m.uv[1];
            v.color = m.color;
        }
        return;
    }

    const bool textured = draw_is_textured(regs, layout);
    const auto tex = textured ? texture_info(regs) : TextureInfo{};
    const LightingSetup lighting = lighting_setup(regs, layout);
    const float offset_x = static_cast<float>(r24(regs, 0x4C) & 0xFFFFu) / 16.0f, offset_y = static_cast<float>(r24(regs, 0x4D) & 0xFFFFu) / 16.0f;
    for (std::uint32_t i = 0; i < count; ++i) {
        ScreenVertex &v = out[i];
        v = ScreenVertex{};
        if (!decode_model_vertex(memory, regs, layout, vertex_address, vertex_index(memory, layout, index_address, i), m)) {
            v.clipped = true;
            continue;
        }
        float model_pos[3], model_normal[3], world_pos[3], world_normal[3];
        skin_vertex(regs, layout, m, model_pos, model_normal);
        transform43(regs.world.data(), model_pos, 1.0f, world_pos);
        v.color = m.color;
        if (lighting.enabled) {
            transform43(regs.world.data(), model_normal, 0.0f, world_normal);
            v.color = light_vertex(lighting, world_pos, world_normal, m.color);
        }
        float e[3];
        transform43(regs.view.data(), world_pos, 1.0f, e);
        const auto &p = regs.proj;
        const float cx = e[0] * p[0] + e[1] * p[4] + e[2] * p[8] + p[12];
        const float cy = e[0] * p[1] + e[1] * p[5] + e[2] * p[9] + p[13];
        const float cz = e[0] * p[2] + e[1] * p[6] + e[2] * p[10] + p[14];
        const float cw = e[0] * p[3] + e[1] * p[7] + e[2] * p[11] + p[15];
        if (cw <= 0.0f) { v.clipped = true; continue; }
        v.x = ge_float(r24(regs, 0x42)) * cx / cw + ge_float(r24(regs, 0x45)) - offset_x;
        v.y = ge_float(r24(regs, 0x43)) * cy / cw + ge_float(r24(regs, 0x46)) - offset_y;
        v.z = ge_float(r24(regs, 0x44)) * cz / cw + ge_float(r24(regs, 0x47));
        v.w = cw;
        if (textured) {
            v.u = (m.uv[0] * ge_float(r24(regs, 0x48)) + ge_float(r24(regs, 0x4A))) * static_cast<float>(tex.width);
            v.v = (m.uv[1] * ge_float(r24(regs, 0x49)) + ge_float(r24(regs, 0x4B))) * static_cast<float>(tex.height);
        }
    }
}

Rgba fetch_texel(psprecomp::GuestMemory &memory, const GeRegisters &regs, const TextureInfo &t, std::int32_t x, std::int32_t y) {
    const auto bpp = texel_bits(t.format);
    if (t.format >= 8) return {255, 0, 255, 255}; // DXT: not observed yet; magenta makes the gap visible
    const auto xbits = static_cast<std::uint32_t>(x) * bpp;
    const auto a = t.address + texel_offset(t, bpp, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
    if (!memory.contains(a, bpp >= 16 ? bpp / 8u : 1u)) return {0, 0, 0, 0};
    std::uint32_t raw;
    switch (bpp) {
    case 4: raw = (memory.load8(a) >> ((xbits & 4u) ? 4 : 0)) & 0xF; break;
    case 8: raw = memory.load8(a); break;
    case 16: raw = memory.load16(a); break;
    default: raw = memory.load32(a); break;
    }
    if (t.format <= 2) return unpack16(raw, t.format);
    if (t.format == 3) return unpack32(raw);
    return convert_entry(clut_lookup(regs, raw), t.clut_format);
}

void decode_texture(psprecomp::GuestMemory &memory, const GeRegisters &regs, const TextureInfo &t, std::vector<std::uint32_t> &out) {
    const std::uint32_t w = t.width, h = t.height;
    out.resize(static_cast<std::size_t>(w) * h);
    const auto bpp = texel_bits(t.format);
    const std::uint32_t span = texel_offset(t, bpp, w - 1u, h - 1u) + (bpp >= 8u ? bpp / 8u : 1u);
    const std::uint8_t *base = t.format < 8 ? memory.raw_pointer(t.address, span) : nullptr;
    if (base == nullptr) { // DXT, or a texture crossing the end of guest memory: per-texel path
        for (std::uint32_t y = 0; y < h; ++y)
            for (std::uint32_t x = 0; x < w; ++x)
                out[static_cast<std::size_t>(y) * w + x] = pack_rgba(fetch_texel(memory, regs, t, static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)));
        return;
    }
    // CLUT formats index a palette built once per texture.
    std::uint32_t palette[256];
    if (t.format >= 4) {
        const std::uint32_t entries = bpp >= 8u ? 256u : 16u;
        for (std::uint32_t i = 0; i < entries; ++i) palette[i] = pack_rgba(convert_entry(clut_lookup(regs, i), t.clut_format));
    }
    for (std::uint32_t y = 0; y < h; ++y) {
        auto *row = out.data() + static_cast<std::size_t>(y) * w;
        for (std::uint32_t x = 0; x < w; ++x) {
            const std::uint8_t *p = base + texel_offset(t, bpp, x, y);
            switch (t.format) {
            case 0: case 1: case 2: row[x] = pack_rgba(unpack16(p[0] | (p[1] << 8), t.format)); break;
            case 3: row[x] = p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<std::uint32_t>(p[3]) << 24); break;
            case 4: row[x] = palette[(p[0] >> ((x & 1u) ? 4 : 0)) & 0xF]; break;
            case 5: row[x] = palette[p[0]]; break;
            case 6: row[x] = pack_rgba(convert_entry(clut_lookup(regs, p[0] | (p[1] << 8)), t.clut_format)); break;
            default: row[x] = pack_rgba(convert_entry(clut_lookup(regs, p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<std::uint32_t>(p[3]) << 24)), t.clut_format)); break;
            }
        }
    }
}

std::uint64_t texture_hash(psprecomp::GuestMemory &memory, const GeRegisters &regs, const TextureInfo &t) {
    const auto bpp = texel_bits(t.format);
    const std::uint32_t span = texel_offset(t, bpp, t.width - 1u, t.height - 1u) + (bpp >= 8u ? bpp / 8u : 1u);
    std::uint64_t h = 0x9E3779B97F4A7C15ull ^ (static_cast<std::uint64_t>(span) << 32);
    auto mix = [&h](std::uint64_t v) { h = (h ^ v) * 0x100000001B3ull; h ^= h >> 29; };
    if (const std::uint8_t *p = memory.raw_pointer(t.address, span)) {
        std::size_t i = 0;
        for (; i + 8 <= span; i += 8) { std::uint64_t v; std::memcpy(&v, p + i, 8); mix(v); }
        for (; i < span; ++i) mix(p[i]);
    } else {
        for (std::uint32_t i = 0; i < span; ++i) mix(memory.contains(t.address + i, 1) ? memory.load8(t.address + i) : 0u);
    }
    if (t.format >= 4) {
        mix(r24(regs, 0xC5));
        for (std::uint32_t i = 0; i < regs.clut_words; ++i) mix(regs.clut[i]);
    }
    return h;
}

} // namespace p3p3ds::ge
