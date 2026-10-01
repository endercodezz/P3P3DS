// Reference software rasterizer for GE primitives (see renderer.hpp).
// [INFERRED] details not yet compared with hardware captures: pixel-centre
// sampling, top-left fill rule, sprite UV mapping, transform-mode clipping
// (primitives with any vertex behind the eye are dropped, no near clipping).
#include "p3p3ds/ge/renderer.hpp"
#include "p3p3ds/ge/geometry.hpp"

#include "psprecomp/guest_memory.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace p3p3ds::ge {
namespace {

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

// ---- Texture sampling (texels from geometry.cpp) ----------------------------------
using Texture = TextureInfo;
using Vertex = ScreenVertex;

std::array<std::uint8_t, 4> sample(const Context &c, const Texture &t, float u, float v) {
    // u, v in texels. Nearest filtering; wrap or clamp per TEXWRAP.
    const auto wrap = c.r(0xC7);
    auto coord = [](float f, std::uint32_t size, bool clamp) {
        auto i = static_cast<std::int32_t>(std::floor(f));
        if (clamp) return std::clamp<std::int32_t>(i, 0, static_cast<std::int32_t>(size) - 1);
        return static_cast<std::int32_t>(static_cast<std::uint32_t>(i) & (size - 1u));
    };
    return fetch_texel(c.memory, c.g, t, coord(u, t.width, wrap & 1u), coord(v, t.height, (wrap >> 8) & 1u));
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

namespace {
std::uint32_t component_size(std::uint32_t format, std::uint32_t one, std::uint32_t two, std::uint32_t three) {
    return format == 1 ? one : format == 2 ? two : format == 3 ? three : 0u;
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
    std::vector<Vertex> vertices;
    decode_screen_vertices(memory, regs, count, vertex_address, index_address, vertices);
    stats_.vertices += count;

    // Texture setup
    const bool textured = draw_is_textured(regs, layout);
    const Texture texture = textured ? texture_info(regs) : Texture{};

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
