// Reference software rasterizer for GE primitives (see renderer.hpp).
// [INFERRED] details not yet compared with hardware captures: pixel-centre
// sampling, top-left fill rule, sprite UV mapping, transform-mode clipping
// (primitives with any vertex behind the eye are dropped, no near clipping).
#include "p3p3ds/ge/renderer.hpp"

#include "psprecomp/guest_memory.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace p3p3ds::ge {
namespace {

struct Vertex {
    float x{}, y{}, z{}, w{1.0f};
    float u{}, v{};
    std::array<std::uint8_t, 4> color{255, 255, 255, 255};
    bool clipped{};
};

std::uint32_t component_size(std::uint32_t format, std::uint32_t one, std::uint32_t two, std::uint32_t three) {
    return format == 1 ? one : format == 2 ? two : format == 3 ? three : 0u;
}

std::array<std::uint8_t, 4> unpack16(std::uint32_t c, std::uint32_t format) {
    auto expand = [](std::uint32_t v, int bits) { return static_cast<std::uint8_t>((v << (8 - bits)) | (v >> (2 * bits - 8 > 0 ? 2 * bits - 8 : 0))); };
    switch (format) {
    case 0: return {expand(c & 0x1F, 5), expand((c >> 5) & 0x3F, 6), expand((c >> 11) & 0x1F, 5), 255};
    case 1: return {expand(c & 0x1F, 5), expand((c >> 5) & 0x1F, 5), expand((c >> 10) & 0x1F, 5), static_cast<std::uint8_t>((c >> 15) ? 255 : 0)};
    default: return {static_cast<std::uint8_t>((c & 0xF) * 17), static_cast<std::uint8_t>(((c >> 4) & 0xF) * 17),
                     static_cast<std::uint8_t>(((c >> 8) & 0xF) * 17), static_cast<std::uint8_t>(((c >> 12) & 0xF) * 17)};
    }
}
std::array<std::uint8_t, 4> unpack32(std::uint32_t c) {
    return {static_cast<std::uint8_t>(c), static_cast<std::uint8_t>(c >> 8), static_cast<std::uint8_t>(c >> 16), static_cast<std::uint8_t>(c >> 24)};
}
std::uint32_t pack(const std::array<std::uint8_t, 4> &c, std::uint32_t format) {
    switch (format) {
    case 0: return (c[0] >> 3) | ((c[1] >> 2) << 5) | ((c[2] >> 3) << 11);
    case 1: return (c[0] >> 3) | ((c[1] >> 3) << 5) | ((c[2] >> 3) << 10) | ((c[3] >> 7) << 15);
    case 2: return (c[0] >> 4) | ((c[1] >> 4) << 4) | ((c[2] >> 4) << 8) | ((c[3] >> 4) << 12);
    default: return c[0] | (c[1] << 8) | (c[2] << 16) | (static_cast<std::uint32_t>(c[3]) << 24);
    }
}

struct Context {
    psprecomp::GuestMemory &memory;
    const GeRegisters &g;
    std::uint32_t r(std::uint32_t i) const { return g.reg[i] & 0xFFFFFFu; }
    bool enabled(std::uint32_t i) const { return (g.reg[i] & 1u) != 0u; }
};

// ---- Texture sampling --------------------------------------------------------
struct Texture {
    std::uint32_t address{}, stride{}, width{1}, height{1}, format{}, clut_format{};
    bool swizzled{};
};

std::uint32_t clut_lookup(const Context &c, std::uint32_t index) {
    const auto cf = c.r(0xC5);
    const auto shift = (cf >> 2) & 0x1F, mask = (cf >> 8) & 0xFF, start = (cf >> 16) & 0x1F;
    const auto i = (((index >> shift) & mask) | (start << 4)) & 0x1FF;
    const auto pf = cf & 3;
    if (pf == 3) return c.g.clut[i & 0xFF];
    const auto word = c.g.clut[(i >> 1) & 0xFF];
    return (i & 1u) ? (word >> 16) : (word & 0xFFFF);
}

std::array<std::uint8_t, 4> fetch_texel(const Context &c, const Texture &t, std::int32_t x, std::int32_t y) {
    static constexpr std::uint32_t bits_per_texel[11] = {16, 16, 16, 32, 4, 8, 16, 32, 4, 8, 8};
    const auto bpp = t.format < 11 ? bits_per_texel[t.format] : 32u;
    if (t.format >= 8) {
        // DXT: not observed yet; render magenta to make the gap visible.
        return {255, 0, 255, 255};
    }
    const std::uint32_t row_bytes = t.stride * bpp / 8u;
    const std::uint32_t xbits = static_cast<std::uint32_t>(x) * bpp;
    std::uint32_t offset;
    if (t.swizzled) {
        const auto xbytes = xbits / 8u;
        const auto bx = xbytes / 16u, by = static_cast<std::uint32_t>(y) / 8u;
        offset = (by * (row_bytes / 16u) + bx) * 128u + (static_cast<std::uint32_t>(y) % 8u) * 16u + xbytes % 16u;
    } else {
        offset = static_cast<std::uint32_t>(y) * row_bytes + xbits / 8u;
    }
    const auto a = t.address + offset;
    if (!c.memory.contains(a, bpp >= 16 ? bpp / 8u : 1u)) return {0, 0, 0, 0};
    std::uint32_t raw;
    switch (bpp) {
    case 4: raw = (c.memory.load8(a) >> ((xbits & 4u) ? 4 : 0)) & 0xF; break;
    case 8: raw = c.memory.load8(a); break;
    case 16: raw = c.memory.load16(a); break;
    default: raw = c.memory.load32(a); break;
    }
    if (t.format <= 2) return unpack16(raw, t.format);
    if (t.format == 3) return unpack32(raw);
    const auto entry = clut_lookup(c, raw);
    return t.clut_format == 3 ? unpack32(entry) : unpack16(entry, t.clut_format);
}

std::array<std::uint8_t, 4> sample(const Context &c, const Texture &t, float u, float v) {
    // u, v in texels. Nearest filtering; wrap or clamp per TEXWRAP.
    const auto wrap = c.r(0xC7);
    auto coord = [](float f, std::uint32_t size, bool clamp) {
        auto i = static_cast<std::int32_t>(std::floor(f));
        if (clamp) return std::clamp<std::int32_t>(i, 0, static_cast<std::int32_t>(size) - 1);
        return static_cast<std::int32_t>(static_cast<std::uint32_t>(i) & (size - 1u));
    };
    return fetch_texel(c, t, coord(u, t.width, wrap & 1u), coord(v, t.height, (wrap >> 8) & 1u));
}

// ---- Fragment pipeline ---------------------------------------------------------
struct Target {
    std::uint32_t color_address{}, color_stride{}, format{}, depth_address{}, depth_stride{};
    std::int32_t x1{}, y1{}, x2{}, y2{}; // inclusive scissor
};

bool compare(std::uint32_t func, std::uint32_t a, std::uint32_t b) {
    switch (func & 7) {
    case 0: return false; case 1: return true; case 2: return a == b; case 3: return a != b;
    case 4: return a < b; case 5: return a <= b; case 6: return a > b; default: return a >= b;
    }
}

std::array<std::uint8_t, 4> blend(const Context &c, std::array<std::uint8_t, 4> s, std::array<std::uint8_t, 4> d) {
    const auto mode = c.r(0xDF);
    const auto fa = unpack32(c.r(0xE0)), fb = unpack32(c.r(0xE1));
    auto factor = [&](std::uint32_t f, bool src_side, int ch) -> int {
        const int sa = s[3], da = d[3];
        switch (f) {
        case 0: return src_side ? d[ch] : s[ch];
        case 1: return 255 - (src_side ? d[ch] : s[ch]);
        case 2: return sa; case 3: return 255 - sa;
        case 4: return da; case 5: return 255 - da;
        case 6: return std::min(255, 2 * sa); case 7: return std::min(255, 2 * (255 - sa));
        case 8: return std::min(255, 2 * da); case 9: return std::min(255, 2 * (255 - da));
        default: return src_side ? fa[ch] : fb[ch];
        }
    };
    std::array<std::uint8_t, 4> out = s;
    for (int ch = 0; ch < 3; ++ch) {
        const int a = s[ch] * factor(mode & 0xF, true, ch) / 255, b = d[ch] * factor((mode >> 4) & 0xF, false, ch) / 255;
        int v;
        switch ((mode >> 8) & 0xF) {
        case 1: v = a - b; break; case 2: v = b - a; break;
        case 3: v = std::min<int>(s[ch], d[ch]); break; case 4: v = std::max<int>(s[ch], d[ch]); break;
        case 5: v = std::abs(s[ch] - d[ch]); break; default: v = a + b; break;
        }
        out[ch] = static_cast<std::uint8_t>(std::clamp(v, 0, 255));
    }
    return out;
}

// GE writes go straight to EDRAM (no CPU-store diagnostics).
std::uint8_t *pixel_ptr(const Context &c, std::uint32_t address, std::uint32_t bytes) {
    return c.memory.raw_pointer(address, bytes);
}
std::array<std::uint8_t, 4> read_color(const std::uint8_t *p, std::uint32_t format) {
    if (format == 3) { std::uint32_t v; std::memcpy(&v, p, 4); return unpack32(v); }
    std::uint16_t v; std::memcpy(&v, p, 2); return unpack16(v, format);
}
void store_color(std::uint8_t *p, const std::array<std::uint8_t, 4> &c, std::uint32_t format) {
    if (format == 3) { const auto v = pack(c, 3); std::memcpy(p, &v, 4); }
    else { const auto v = static_cast<std::uint16_t>(pack(c, format)); std::memcpy(p, &v, 2); }
}

void write_pixel(const Context &c, const Target &t, std::int32_t x, std::int32_t y, std::array<std::uint8_t, 4> color,
                 float z, DrawStats &stats) {
    if (x < t.x1 || x > t.x2 || y < t.y1 || y > t.y2 || x < 0 || y < 0) return;
    const auto clear = c.r(0xD3);
    const bool clear_mode = (clear & 1u) != 0u;
    const auto zi = static_cast<std::uint16_t>(std::clamp(z, 0.0f, 65535.0f));
    const std::uint32_t bpp = t.format == 3 ? 4u : 2u;
    auto *cp = pixel_ptr(c, t.color_address + (static_cast<std::uint32_t>(y) * t.color_stride + static_cast<std::uint32_t>(x)) * bpp, bpp);
    if (cp == nullptr) return;
    auto *dp = pixel_ptr(c, t.depth_address + (static_cast<std::uint32_t>(y) * t.depth_stride + static_cast<std::uint32_t>(x)) * 2u, 2u);
    const auto old = read_color(cp, t.format);
    if (clear_mode) {
        std::array<std::uint8_t, 4> out = old;
        if (clear & 0x100u) { out[0] = color[0]; out[1] = color[1]; out[2] = color[2]; }
        if (clear & 0x200u) out[3] = color[3];
        if (clear & 0x300u) store_color(cp, out, t.format);
        if ((clear & 0x400u) && dp != nullptr) std::memcpy(dp, &zi, 2);
        ++stats.pixels;
        return;
    }
    if (c.enabled(0x22)) { // alpha test
        const auto at = c.r(0xDB);
        const auto mask = (at >> 16) & 0xFF;
        if (!compare(at, color[3] & mask, ((at >> 8) & 0xFF) & mask)) return;
    }
    if (c.enabled(0x23) && dp != nullptr) { // depth test
        std::uint16_t dz; std::memcpy(&dz, dp, 2);
        if (!compare(c.r(0xDE), zi, dz)) return;
    }
    auto out = c.enabled(0x21) ? blend(c, color, old) : color;
    const auto rgb_mask = c.r(0xE8); // set bits keep the old value
    for (int ch = 0; ch < 3; ++ch) {
        const auto m = (rgb_mask >> (8 * ch)) & 0xFF;
        out[ch] = static_cast<std::uint8_t>((out[ch] & ~m) | (old[ch] & m));
    }
    // [INFERRED] the alpha channel holds stencil and only changes through
    // stencil operations, which are not modelled yet.
    out[3] = old[3];
    store_color(cp, out, t.format);
    if (c.enabled(0x23) && (c.r(0xE7) & 1u) == 0u && dp != nullptr) std::memcpy(dp, &zi, 2);
    ++stats.pixels;
}

std::array<std::uint8_t, 4> shade(const Context &c, const Texture *tex, std::array<std::uint8_t, 4> color, float u, float v) {
    if (tex == nullptr) return color;
    const auto texel = sample(c, *tex, u, v);
    const auto func = c.r(0xC9);
    const bool use_alpha = (func & 0x100u) != 0u;
    const auto env = unpack32(c.r(0xCA));
    std::array<std::uint8_t, 4> out{};
    for (int ch = 0; ch < 3; ++ch) {
        int value;
        switch (func & 7) {
        case 0: value = color[ch] * texel[ch] / 255; break;                                        // modulate
        case 1: value = (color[ch] * (255 - texel[3]) + texel[ch] * texel[3]) / 255; break;        // decal
        case 2: value = (color[ch] * (255 - texel[ch]) + env[ch] * texel[ch]) / 255; break;        // blend
        case 3: value = texel[ch]; break;                                                         // replace
        default: value = std::min(255, color[ch] + texel[ch]); break;                             // add
        }
        if (func & 0x10000u) value = std::min(255, value * 2);                                     // color doubling
        out[ch] = static_cast<std::uint8_t>(value);
    }
    if ((func & 7) == 1) out[3] = color[3];
    else if ((func & 7) == 3) out[3] = use_alpha ? texel[3] : color[3];
    else out[3] = use_alpha ? static_cast<std::uint8_t>(color[3] * texel[3] / 255) : color[3];
    return out;
}

} // namespace

VertexLayout vertex_layout(std::uint32_t vtype) {
    VertexLayout l;
    l.uv_format = vtype & 3u;
    l.color_format = (vtype >> 2) & 7u;
    l.normal_format = (vtype >> 5) & 3u;
    l.pos_format = (vtype >> 7) & 3u;
    l.weight_format = (vtype >> 9) & 3u;
    l.index_format = (vtype >> 11) & 3u;
    l.weights = l.weight_format ? ((vtype >> 14) & 7u) + 1u : 0u;
    l.morphs = ((vtype >> 18) & 7u) + 1u;
    l.through = (vtype & (1u << 23)) != 0u;
    std::uint32_t offset = 0, align = 1;
    auto add = [&](std::uint32_t element, std::uint32_t count) {
        if (element == 0u) return offset;
        offset = (offset + element - 1u) & ~(element - 1u);
        const auto at = offset;
        offset += element * count;
        align = std::max(align, element);
        return at;
    };
    l.weight_offset = add(component_size(l.weight_format, 1, 2, 4), l.weights);
    l.uv_offset = add(component_size(l.uv_format, 1, 2, 4), 2);
    l.color_offset = add(l.color_format >= 4u ? (l.color_format == 7u ? 4u : 2u) : 0u, 1);
    l.normal_offset = add(component_size(l.normal_format, 1, 2, 4), 3);
    l.pos_offset = add(component_size(l.pos_format, 1, 2, 4), 3);
    l.size = (offset + align - 1u) & ~(align - 1u);
    return l;
}

void SoftwareRenderer::draw(psprecomp::GuestMemory &memory, const GeRegisters &regs, Prim prim, std::uint32_t count,
                            std::uint32_t vertex_address, std::uint32_t index_address) {
    Context c{memory, regs};
    ++stats_.prims;
    const auto layout = vertex_layout(c.r(0x12));
    // Decode the referenced vertices.
    auto read_component = [&](std::uint32_t at, std::uint32_t format, bool signed_value, float normal_scale8, float normal_scale16) -> float {
        switch (format) {
        case 1: return signed_value ? static_cast<std::int8_t>(memory.load8(at)) / normal_scale8 : memory.load8(at) / normal_scale8;
        case 2: return signed_value ? static_cast<std::int16_t>(memory.load16(at)) / normal_scale16 : memory.load16(at) / normal_scale16;
        case 3: { const auto bits = memory.load32(at); float f; std::memcpy(&f, &bits, 4); return f; }
        default: return 0.0f;
        }
    };
    const auto material = (c.r(0x55) & 0xFFFFFFu) | ((c.r(0x58) & 0xFFu) << 24);
    auto decode = [&](std::uint32_t index) {
        Vertex v;
        const auto base = vertex_address + index * layout.size;
        if (!memory.contains(base, std::max<std::uint32_t>(layout.size, 1u))) { v.clipped = true; return v; }
        // Positions
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
        // Texture coordinates
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

    std::vector<Vertex> vertices(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint32_t index = i;
        if (layout.index_format == 1u) index = memory.load8(index_address + i);
        else if (layout.index_format == 2u) index = memory.load16(index_address + 2u * i);
        vertices[i] = decode(index);
    }
    stats_.vertices += count;

    // Texture setup
    Texture texture;
    const bool textured = c.enabled(0x1E) && layout.uv_format != 0u && (c.r(0xD3) & 1u) == 0u;
    if (textured) {
        texture.address = (c.r(0xA0) & 0xFFFFF0u) | ((c.r(0xA8) >> 16) & 0xFFu) << 24;
        texture.stride = c.r(0xA8) & 0xFFFFu;
        texture.width = 1u << (c.r(0xB8) & 0xFu);
        texture.height = 1u << ((c.r(0xB8) >> 8) & 0xFu);
        texture.format = c.r(0xC3) & 0xFu;
        texture.swizzled = (c.r(0xC2) & 1u) != 0u;
        texture.clut_format = c.r(0xC5) & 3u;
    }

    // Transform to screen space.
    const float offset_x = static_cast<float>(c.r(0x4C) & 0xFFFFu) / 16.0f, offset_y = static_cast<float>(c.r(0x4D) & 0xFFFFu) / 16.0f;
    for (auto &v : vertices) {
        if (layout.through || v.clipped) continue;
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
        v.x = ge_float(c.r(0x42)) * cx / cw + ge_float(c.r(0x45)) - offset_x;
        v.y = ge_float(c.r(0x43)) * cy / cw + ge_float(c.r(0x46)) - offset_y;
        v.z = ge_float(c.r(0x44)) * cz / cw + ge_float(c.r(0x47));
        v.w = cw;
        if (textured) {
            v.u = (v.u * ge_float(c.r(0x48)) + ge_float(c.r(0x4A))) * static_cast<float>(texture.width);
            v.v = (v.v * ge_float(c.r(0x49)) + ge_float(c.r(0x4B))) * static_cast<float>(texture.height);
        }
    }

    Target target;
    target.color_address = 0x04000000u | ((c.r(0x9C) & 0xFFFFF0u) | ((c.r(0x9D) >> 16) & 0xFFu) << 24);
    target.color_stride = c.r(0x9D) & 0x7FCu;
    target.format = c.r(0xD2) & 3u;
    target.depth_address = 0x04000000u | ((c.r(0x9E) & 0xFFFFF0u) | ((c.r(0x9F) >> 16) & 0xFFu) << 24);
    target.depth_stride = c.r(0x9F) & 0x7FCu;
    target.x1 = static_cast<std::int32_t>(c.r(0xD4) & 0x3FFu);
    target.y1 = static_cast<std::int32_t>((c.r(0xD4) >> 10) & 0x3FFu);
    target.x2 = static_cast<std::int32_t>(c.r(0xD5) & 0x3FFu);
    target.y2 = static_cast<std::int32_t>((c.r(0xD5) >> 10) & 0x3FFu);
    if (target.color_stride == 0u) return;
    const bool flat = (c.r(0x50) & 1u) == 0u;
    if ((c.r(0xD3) & 1u) != 0u) ++stats_.clears;
    const Texture *tex = textured ? &texture : nullptr;

    auto sprite = [&](const Vertex &a, const Vertex &b) {
        ++stats_.sprites;
        const float x0 = std::min(a.x, b.x), x1 = std::max(a.x, b.x), y0 = std::min(a.y, b.y), y1 = std::max(a.y, b.y);
        const auto ix0 = static_cast<std::int32_t>(std::ceil(x0 - 0.5f)), ix1 = static_cast<std::int32_t>(std::ceil(x1 - 0.5f));
        const auto iy0 = static_cast<std::int32_t>(std::ceil(y0 - 0.5f)), iy1 = static_cast<std::int32_t>(std::ceil(y1 - 0.5f));
        const float du = (x1 != x0) ? (b.u - a.u) / (b.x - a.x) : 0.0f, dv = (y1 != y0) ? (b.v - a.v) / (b.y - a.y) : 0.0f;
        for (std::int32_t y = std::max(iy0, target.y1); y < iy1 && y <= target.y2; ++y)
            for (std::int32_t x = std::max(ix0, target.x1); x < ix1 && x <= target.x2; ++x) {
                const float u = a.u + (x + 0.5f - a.x) * du, v = a.v + (y + 0.5f - a.y) * dv;
                write_pixel(c, target, x, y, shade(c, tex, b.color, u, v), b.z, stats_);
            }
    };
    auto triangle = [&](const Vertex &a, const Vertex &b, const Vertex &d) {
        if (a.clipped || b.clipped || d.clipped) return;
        ++stats_.triangles;
        const float area = (b.x - a.x) * (d.y - a.y) - (b.y - a.y) * (d.x - a.x);
        if (area == 0.0f) return;
        if (c.enabled(0x1D)) { // back-face culling: CULL register selects the kept winding
            const bool ccw = area > 0.0f;
            if (ccw == ((c.r(0x9B) & 1u) != 0u)) return;
        }
        const auto minx = std::max<std::int32_t>(target.x1, static_cast<std::int32_t>(std::floor(std::min({a.x, b.x, d.x}))));
        const auto maxx = std::min<std::int32_t>(target.x2, static_cast<std::int32_t>(std::ceil(std::max({a.x, b.x, d.x}))));
        const auto miny = std::max<std::int32_t>(target.y1, static_cast<std::int32_t>(std::floor(std::min({a.y, b.y, d.y}))));
        const auto maxy = std::min<std::int32_t>(target.y2, static_cast<std::int32_t>(std::ceil(std::max({a.y, b.y, d.y}))));
        for (std::int32_t y = miny; y <= maxy; ++y)
            for (std::int32_t x = minx; x <= maxx; ++x) {
                const float px = x + 0.5f, py = y + 0.5f;
                float w0 = ((b.x - px) * (d.y - py) - (b.y - py) * (d.x - px)) / area;
                float w1 = ((d.x - px) * (a.y - py) - (d.y - py) * (a.x - px)) / area;
                float w2 = 1.0f - w0 - w1;
                if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;
                std::array<std::uint8_t, 4> color = d.color;
                if (!flat)
                    for (int ch = 0; ch < 4; ++ch)
                        color[ch] = static_cast<std::uint8_t>(std::clamp(w0 * a.color[ch] + w1 * b.color[ch] + w2 * d.color[ch], 0.0f, 255.0f));
                // Perspective-correct UV in transform mode, affine in through mode.
                float u, v;
                if (layout.through) { u = w0 * a.u + w1 * b.u + w2 * d.u; v = w0 * a.v + w1 * b.v + w2 * d.v; }
                else {
                    const float iw = w0 / a.w + w1 / b.w + w2 / d.w;
                    u = (w0 * a.u / a.w + w1 * b.u / b.w + w2 * d.u / d.w) / iw;
                    v = (w0 * a.v / a.w + w1 * b.v / b.w + w2 * d.v / d.w) / iw;
                }
                write_pixel(c, target, x, y, shade(c, tex, color, u, v), w0 * a.z + w1 * b.z + w2 * d.z, stats_);
            }
    };

    switch (prim) {
    case Prim::Sprites: for (std::uint32_t i = 0; i + 1 < count; i += 2) sprite(vertices[i], vertices[i + 1]); break;
    case Prim::Triangles: for (std::uint32_t i = 0; i + 2 < count; i += 3) triangle(vertices[i], vertices[i + 1], vertices[i + 2]); break;
    case Prim::TriangleStrip:
        for (std::uint32_t i = 0; i + 2 < count; ++i)
            (i & 1u) ? triangle(vertices[i + 1], vertices[i], vertices[i + 2]) : triangle(vertices[i], vertices[i + 1], vertices[i + 2]);
        break;
    case Prim::TriangleFan: for (std::uint32_t i = 1; i + 1 < count; ++i) triangle(vertices[0], vertices[i], vertices[i + 1]); break;
    default:
        // Points/lines: single pixels at each vertex ([INFERRED] lines not rasterized yet).
        for (const auto &v : vertices)
            if (!v.clipped) write_pixel(c, target, static_cast<std::int32_t>(v.x), static_cast<std::int32_t>(v.y), shade(c, tex, v.color, v.u, v.v), v.z, stats_);
        break;
    }
}

void SoftwareRenderer::transfer(psprecomp::GuestMemory &memory, const GeRegisters &regs) {
    Context c{memory, regs};
    ++stats_.transfers;
    const std::uint32_t bpp = (c.r(0xEA) & 1u) ? 4u : 2u;
    const auto src = (c.r(0xB2) & 0xFFFFF0u) | ((c.r(0xB3) >> 16) & 0xFFu) << 24;
    const auto dst = (c.r(0xB4) & 0xFFFFF0u) | ((c.r(0xB5) >> 16) & 0xFFu) << 24;
    const auto src_stride = c.r(0xB3) & 0xFFF8u, dst_stride = c.r(0xB5) & 0xFFF8u;
    const auto sx = c.r(0xEB) & 0x3FFu, sy = (c.r(0xEB) >> 10) & 0x3FFu;
    const auto dx = c.r(0xEC) & 0x3FFu, dy = (c.r(0xEC) >> 10) & 0x3FFu;
    const auto w = (c.r(0xEE) & 0x3FFu) + 1u, h = ((c.r(0xEE) >> 10) & 0x3FFu) + 1u;
    for (std::uint32_t y = 0; y < h; ++y) {
        auto *from = memory.raw_pointer(src + ((sy + y) * src_stride + sx) * bpp, w * bpp);
        auto *to = memory.raw_pointer(dst + ((dy + y) * dst_stride + dx) * bpp, w * bpp);
        if (from == nullptr || to == nullptr) return;
        std::memmove(to, from, w * bpp);
    }
}

} // namespace p3p3ds::ge
