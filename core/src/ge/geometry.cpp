// Shared GE front end (see geometry.hpp). The vertex decode/transform and
// texel fetch were moved here unchanged from software_renderer.cpp.
// [INFERRED] as there: sprite UV mapping and transform-mode clipping
// (vertices behind the eye are marked clipped, no near-plane clipping).
#include "p3p3ds/ge/geometry.hpp"

#include "psprecomp/guest_memory.hpp"

#include <algorithm>
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

void decode_screen_vertices(psprecomp::GuestMemory &memory, const GeRegisters &regs, std::uint32_t count,
                            std::uint32_t vertex_address, std::uint32_t index_address, std::vector<ScreenVertex> &out) {
    const auto layout = vertex_layout(r24(regs, 0x12));
    auto read_component = [&](std::uint32_t at, std::uint32_t format, bool signed_value, float normal_scale8, float normal_scale16) -> float {
        switch (format) {
        case 1: return signed_value ? static_cast<std::int8_t>(memory.load8(at)) / normal_scale8 : memory.load8(at) / normal_scale8;
        case 2: return signed_value ? static_cast<std::int16_t>(memory.load16(at)) / normal_scale16 : memory.load16(at) / normal_scale16;
        case 3: { const auto bits = memory.load32(at); float f; std::memcpy(&f, &bits, 4); return f; }
        default: return 0.0f;
        }
    };
    const auto material = (r24(regs, 0x55) & 0xFFFFFFu) | ((r24(regs, 0x58) & 0xFFu) << 24);
    auto decode = [&](std::uint32_t index) {
        ScreenVertex v;
        const auto base = vertex_address + index * layout.size;
        if (!memory.contains(base, std::max<std::uint32_t>(layout.size, 1u))) { v.clipped = true; return v; }
        const auto pe = component_size(layout.pos_format, 1, 2, 4);
        if (layout.through) {
            // Through mode: s16/float screen coordinates (x, y), u16 z; integer texel UVs.
            const bool f = layout.pos_format == 3;
            v.x = f ? read_component(base + layout.pos_offset, 3, true, 1, 1) : read_component(base + layout.pos_offset, layout.pos_format, true, 1.0f, 1.0f);
            v.y = f ? read_component(base + layout.pos_offset + 4, 3, true, 1, 1) : read_component(base + layout.pos_offset + pe, layout.pos_format, true, 1.0f, 1.0f);
            v.z = f ? read_component(base + layout.pos_offset + 8, 3, true, 1, 1) : read_component(base + layout.pos_offset + 2 * pe, layout.pos_format, false, 1.0f, 1.0f);
        } else {
            v.x = read_component(base + layout.pos_offset, layout.pos_format, true, 128.0f, 32768.0f);
            v.y = read_component(base + layout.pos_offset + pe, layout.pos_format, true, 128.0f, 32768.0f);
            v.z = read_component(base + layout.pos_offset + 2 * pe, layout.pos_format, true, 128.0f, 32768.0f);
        }
        if (layout.uv_format) {
            const auto ue = component_size(layout.uv_format, 1, 2, 4);
            if (layout.through) {
                v.u = read_component(base + layout.uv_offset, layout.uv_format, false, 1.0f, 1.0f);
                v.v = read_component(base + layout.uv_offset + ue, layout.uv_format, false, 1.0f, 1.0f);
            } else {
                v.u = read_component(base + layout.uv_offset, layout.uv_format, false, 128.0f, 32768.0f);
                v.v = read_component(base + layout.uv_offset + ue, layout.uv_format, false, 128.0f, 32768.0f);
            }
        }
        // Colour (material ambient when the vertex has none)
        if (layout.color_format == 7u) v.color = unpack32(memory.load32(base + layout.color_offset));
        else if (layout.color_format >= 4u) v.color = unpack16(memory.load16(base + layout.color_offset), layout.color_format == 4u ? 0u : layout.color_format == 5u ? 1u : 2u);
        else v.color = unpack32(material);
        return v;
    };

    out.resize(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint32_t index = i;
        if (layout.index_format == 1u) index = memory.load8(index_address + i);
        else if (layout.index_format == 2u) index = memory.load16(index_address + 2u * i);
        out[i] = decode(index);
    }
    if (layout.through) return;

    const bool textured = draw_is_textured(regs, layout);
    const auto tex = textured ? texture_info(regs) : TextureInfo{};
    const float offset_x = static_cast<float>(r24(regs, 0x4C) & 0xFFFFu) / 16.0f, offset_y = static_cast<float>(r24(regs, 0x4D) & 0xFFFFu) / 16.0f;
    for (auto &v : out) {
        if (v.clipped) continue;
        const auto &w = regs.world, &vw = regs.view;
        const float wx = v.x * w[0] + v.y * w[3] + v.z * w[6] + w[9];
        const float wy = v.x * w[1] + v.y * w[4] + v.z * w[7] + w[10];
        const float wz = v.x * w[2] + v.y * w[5] + v.z * w[8] + w[11];
        const float ex = wx * vw[0] + wy * vw[3] + wz * vw[6] + vw[9];
        const float ey = wx * vw[1] + wy * vw[4] + wz * vw[7] + vw[10];
        const float ez = wx * vw[2] + wy * vw[5] + wz * vw[8] + vw[11];
        const auto &p = regs.proj;
        const float cx = ex * p[0] + ey * p[4] + ez * p[8] + p[12];
        const float cy = ex * p[1] + ey * p[5] + ez * p[9] + p[13];
        const float cz = ex * p[2] + ey * p[6] + ez * p[10] + p[14];
        const float cw = ex * p[3] + ey * p[7] + ez * p[11] + p[15];
        if (cw <= 0.0f) { v.clipped = true; continue; }
        v.x = ge_float(r24(regs, 0x42)) * cx / cw + ge_float(r24(regs, 0x45)) - offset_x;
        v.y = ge_float(r24(regs, 0x43)) * cy / cw + ge_float(r24(regs, 0x46)) - offset_y;
        v.z = ge_float(r24(regs, 0x44)) * cz / cw + ge_float(r24(regs, 0x47));
        v.w = cw;
        if (textured) {
            v.u = (v.u * ge_float(r24(regs, 0x48)) + ge_float(r24(regs, 0x4A))) * static_cast<float>(tex.width);
            v.v = (v.v * ge_float(r24(regs, 0x49)) + ge_float(r24(regs, 0x4B))) * static_cast<float>(tex.height);
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
