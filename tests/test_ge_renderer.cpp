// SoftwareRenderer unit tests: through-mode sprites, CLUT + swizzled textures,
// alpha blending, clear mode, scissor, block transfer and transform-mode culling. Expected values are
// derived from the PSPSDK pspgu.h register semantics (see renderer.hpp).
#include "p3p3ds/ge/renderer.hpp"
#include "psprecomp/guest_memory.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace p3p3ds::ge;

namespace {
int failures = 0;
void check(bool ok, const char *what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
constexpr std::uint32_t kFb = 0x04000000u, kVerts = 0x08900000u, kTex = 0x08910000u, kStride = 512u;

GeRegisters base_regs() {
    GeRegisters g;
    g.reg[0x9C] = 0;                    // FBP low 24 bits
    g.reg[0x9D] = kStride;              // FBW
    g.reg[0xD2] = 3;                    // PSM 8888
    g.reg[0xD4] = 0;                    // scissor (0,0)
    g.reg[0xD5] = 479u | (271u << 10);  // scissor (479,271)
    return g;
}
std::uint32_t pixel(psprecomp::GuestMemory &m, std::uint32_t x, std::uint32_t y) { return m.load32(kFb + (y * kStride + x) * 4u); }
void fill_fb(psprecomp::GuestMemory &m, std::uint32_t value) {
    for (std::uint32_t i = 0; i < kStride * 272u; ++i) m.store32(kFb + i * 4u, value);
}
// Through-mode sprite: two vertices (optional s16 UV, colour 8888, s16 xyz).
void put_sprite(psprecomp::GuestMemory &m, std::uint32_t vtype, std::uint32_t at, int x0, int y0, int x1, int y1,
                std::uint32_t color, int u0 = 0, int v0 = 0, int u1 = 0, int v1 = 0) {
    const auto l = vertex_layout(vtype);
    const int xs[2] = {x0, x1}, ys[2] = {y0, y1}, us[2] = {u0, u1}, vs[2] = {v0, v1};
    for (int i = 0; i < 2; ++i) {
        const auto v = at + l.size * static_cast<std::uint32_t>(i);
        if (l.uv_format == 2) {
            m.store16(v + l.uv_offset, static_cast<std::uint16_t>(us[i]));
            m.store16(v + l.uv_offset + 2, static_cast<std::uint16_t>(vs[i]));
        }
        if (l.color_format == 7) m.store32(v + l.color_offset, color);
        m.store16(v + l.pos_offset, static_cast<std::uint16_t>(xs[i]));
        m.store16(v + l.pos_offset + 2, static_cast<std::uint16_t>(ys[i]));
        m.store16(v + l.pos_offset + 4, 0);
    }
}
constexpr std::uint32_t kColorVtype = (7u << 2) | (2u << 7) | (1u << 23);
constexpr std::uint32_t kTexVtype = 2u | (7u << 2) | (2u << 7) | (1u << 23);
} // namespace

int main() {
    // 1. Flat-colour sprite covers [x0,x1) x [y0,y1) and respects the scissor.
    {
        psprecomp::GuestMemory m; SoftwareRenderer r; auto g = base_regs();
        fill_fb(m, 0xFF000000u);
        g.reg[0x12] = kColorVtype;
        put_sprite(m, kColorVtype, kVerts, 10, 20, 30, 40, 0xFF0000FFu);
        r.draw(m, g, Prim::Sprites, 2, kVerts, 0);
        check((pixel(m, 10, 20) & 0xFFFFFFu) == 0x0000FFu, "sprite top-left pixel is red");
        check((pixel(m, 29, 39) & 0xFFFFFFu) == 0x0000FFu, "sprite bottom-right inner pixel is red");
        check(pixel(m, 30, 40) == 0xFF000000u && pixel(m, 9, 20) == 0xFF000000u, "sprite excludes right/bottom edge");
        g.reg[0xD5] = 14u | (271u << 10);
        put_sprite(m, kColorVtype, kVerts, 10, 50, 30, 60, 0xFF00FF00u);
        r.draw(m, g, Prim::Sprites, 2, kVerts, 0);
        check((pixel(m, 14, 50) & 0xFFFFFFu) == 0x00FF00u && (pixel(m, 15, 50) & 0xFFFFFFu) == 0, "scissor clips at x2 inclusive");
        check(r.stats().sprites == 2, "sprite stats counted");
    }
    // 2. CLUT4 texture: linear and swizzled storage produce identical output.
    for (int swizzled = 0; swizzled < 2; ++swizzled) {
        psprecomp::GuestMemory m; SoftwareRenderer r; auto g = base_regs();
        fill_fb(m, 0);
        // 32x8 texels at 4bpp = 16 bytes per row (one swizzle block wide); texel(x,y) = (x+y)&15.
        for (std::uint32_t y = 0; y < 8; ++y)
            for (std::uint32_t x = 0; x < 32; x += 2) {
                const auto lo = (x + y) & 15u, hi = (x + 1u + y) & 15u;
                const auto xb = x / 2u;
                const auto off = swizzled ? (y / 8u + xb / 16u) * 128u + (y % 8u) * 16u + xb % 16u : y * 16u + xb;
                m.store8(kTex + off, static_cast<std::uint8_t>(lo | (hi << 4)));
            }
        for (std::uint32_t i = 0; i < 16; ++i) g.clut[i] = 0xFF000000u | (i * 0x10u) | ((255u - i * 0x10u) << 16);
        g.reg[0x12] = kTexVtype;
        g.reg[0x1E] = 1;                                   // texture enable
        g.reg[0xA0] = kTex & 0xFFFFF0u;
        g.reg[0xA8] = 32u | ((kTex >> 24) << 16);
        g.reg[0xB8] = 5u | (3u << 8);                      // 32x8
        g.reg[0xC3] = 4;                                   // CLUT4
        g.reg[0xC2] = static_cast<std::uint32_t>(swizzled);
        g.reg[0xC5] = 3u | (0xFFu << 8);                   // CLUT 8888, mask 0xFF
        g.reg[0xC9] = 3;                                   // replace
        put_sprite(m, kTexVtype, kVerts, 0, 0, 32, 8, 0xFFFFFFFFu, 0, 0, 32, 8);
        r.draw(m, g, Prim::Sprites, 2, kVerts, 0);
        bool ok = true;
        for (std::uint32_t y = 0; y < 8; ++y)
            for (std::uint32_t x = 0; x < 32; ++x) {
                const auto i = (x + y) & 15u;
                ok &= (pixel(m, x, y) & 0xFFFFFFu) == ((i * 0x10u) | ((255u - i * 0x10u) << 16));
            }
        check(ok, swizzled ? "swizzled CLUT4 texture sampled exactly" : "linear CLUT4 texture sampled exactly");
    }
    // 3. Alpha blending: src*a + dst*(1-a), a = 0x80.
    {
        psprecomp::GuestMemory m; SoftwareRenderer r; auto g = base_regs();
        fill_fb(m, 0xFF0000FFu);                           // red destination
        g.reg[0x12] = kColorVtype;
        g.reg[0x21] = 1;
        g.reg[0xDF] = 2u | (3u << 4);                      // SRC_ALPHA, ONE_MINUS_SRC_ALPHA, add
        put_sprite(m, kColorVtype, kVerts, 0, 0, 4, 4, 0x80FF0000u); // blue, alpha 0x80
        r.draw(m, g, Prim::Sprites, 2, kVerts, 0);
        const auto p = pixel(m, 1, 1);
        const int red = static_cast<int>(p & 0xFF), blue = static_cast<int>((p >> 16) & 0xFF);
        check(std::abs(red - 127) <= 1 && std::abs(blue - 128) <= 1, "50% alpha blend mixes red and blue");
    }
    // 4. Clear mode writes the vertex colour unconditionally.
    {
        psprecomp::GuestMemory m; SoftwareRenderer r; auto g = base_regs();
        fill_fb(m, 0x11223344u);
        g.reg[0x12] = kColorVtype;
        g.reg[0xD3] = 1u | (1u << 8);                      // clear mode, colour
        put_sprite(m, kColorVtype, kVerts, 0, 0, 480, 272, 0x00000000u);
        r.draw(m, g, Prim::Sprites, 2, kVerts, 0);
        check((pixel(m, 0, 0) & 0xFFFFFFu) == 0 && (pixel(m, 479, 271) & 0xFFFFFFu) == 0, "clear sprite clears the full screen");
        check(r.stats().clears == 1, "clear counted");
    }
    // 5. Block transfer copies a 32-bit rectangle.
    {
        psprecomp::GuestMemory m; SoftwareRenderer r; auto g = base_regs();
        for (std::uint32_t i = 0; i < 64; ++i) m.store32(kTex + i * 4u, 0xA0000000u + i);
        g.reg[0xEA] = 1;                                   // 32-bit
        g.reg[0xB2] = kTex & 0xFFFFF0u;
        g.reg[0xB3] = 8u | ((kTex >> 24) << 16);
        g.reg[0xB4] = kFb & 0xFFFFF0u;
        g.reg[0xB5] = kStride | ((kFb >> 24) << 16);
        g.reg[0xEB] = 2u | (1u << 10);                     // src (2,1)
        g.reg[0xEC] = 100u | (5u << 10);                   // dst (100,5)
        g.reg[0xEE] = 3u | (2u << 10);                     // 4x3
        r.transfer(m, g);
        check(pixel(m, 100, 5) == 0xA0000000u + 10u && pixel(m, 103, 7) == 0xA0000000u + 29u, "block transfer rectangle");
    }
    // 6. Transform-mode culling: CULL (0x9B) = 1 keeps the triangles that are
    // counter-clockwise on screen (y down) and culls the others; measured in
    // P3P's first battle (the opposite rule drew the models inside out).
    for (int order = 0; order < 2; ++order) {
        psprecomp::GuestMemory m; SoftwareRenderer r; auto g = base_regs();
        fill_fb(m, 0xFF000000u);
        constexpr std::uint32_t kVtype = (7u << 2) | (3u << 7); // colour 8888, float xyz, transform mode
        g.reg[0x12] = kVtype;
        for (int i = 0; i < 12; ++i) g.world[i] = g.view[i] = (i % 4 == 0) ? 1.0f : 0.0f; // 4x3 identity
        for (int i = 0; i < 16; ++i) g.proj[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        auto ge = [](float f) { std::uint32_t b; std::memcpy(&b, &f, 4); return b >> 8; };
        g.reg[0x42] = ge(100.0f); g.reg[0x43] = ge(-100.0f); g.reg[0x44] = ge(0.0f);
        g.reg[0x45] = ge(2048.0f + 240.0f); g.reg[0x46] = ge(2048.0f + 136.0f); g.reg[0x47] = ge(0.0f);
        g.reg[0x4C] = 2048u << 4; g.reg[0x4D] = 2048u << 4;
        g.reg[0x1D] = 1; g.reg[0x9B] = 1;
        // Screen (190,186), (290,186), (240,86): clockwise on screen; swapped: counter-clockwise.
        const float pts[3][2] = {{-0.5f, -0.5f}, {0.5f, -0.5f}, {0.0f, 0.5f}};
        const int idx[2][3] = {{0, 1, 2}, {0, 2, 1}};
        const auto l = vertex_layout(kVtype);
        for (int k = 0; k < 3; ++k) {
            const auto v = kVerts + l.size * static_cast<std::uint32_t>(k);
            float xyz[3] = {pts[idx[order][k]][0], pts[idx[order][k]][1], 0.0f};
            for (int c = 0; c < 3; ++c) { std::uint32_t b; std::memcpy(&b, &xyz[c], 4); m.store32(v + l.pos_offset + 4u * static_cast<std::uint32_t>(c), b); }
            m.store32(v + l.color_offset, 0xFF00FF00u);
        }
        r.draw(m, g, Prim::Triangles, 3, kVerts, 0);
        const bool drawn = (pixel(m, 240, 150) & 0xFFFFFFu) == 0x00FF00u;
        check(drawn == (order == 1), order ? "transform mode keeps the counter-clockwise triangle" : "transform mode culls the clockwise triangle");
    }
    std::printf("%s (%d failures)\n", failures ? "FAILED" : "PASS", failures);
    return failures ? 1 : 0;
}
