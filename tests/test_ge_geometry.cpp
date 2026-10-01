// Shared GE front end (geometry.hpp): decode_texture must equal fetch_texel
// for every texel of every non-DXT format, linear and swizzled, with CLUT
// shift/mask/start; texture_hash must change with texture bytes and palette.
#include "p3p3ds/ge/geometry.hpp"
#include "psprecomp/guest_memory.hpp"

#include <cstdio>
#include <vector>

using namespace p3p3ds::ge;

namespace {
int failures = 0;
void check(bool ok, const char *what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
}
constexpr std::uint32_t kTex = 0x08910000u;

std::uint32_t rng_state = 0x12345678u;
std::uint32_t rng() { rng_state = rng_state * 1664525u + 1013904223u; return rng_state >> 8; }

GeRegisters regs_for(std::uint32_t format, std::uint32_t clut_format, bool swizzled, std::uint32_t wlog, std::uint32_t hlog,
                     std::uint32_t stride, std::uint32_t clut_shift, std::uint32_t clut_mask, std::uint32_t clut_start) {
    GeRegisters g;
    g.reg[0xA0] = kTex & 0xFFFFF0u;
    g.reg[0xA8] = stride | (((kTex >> 24) & 0xFFu) << 16);
    g.reg[0xB8] = wlog | (hlog << 8);
    g.reg[0xC3] = format;
    g.reg[0xC2] = swizzled ? 1u : 0u;
    g.reg[0xC5] = clut_format | (clut_shift << 2) | (clut_mask << 8) | (clut_start << 16);
    g.clut_words = 256;
    for (auto &w : g.clut) w = rng() ^ (rng() << 16);
    return g;
}

void compare_case(psprecomp::GuestMemory &m, std::uint32_t format, std::uint32_t clut_format, bool swizzled,
                  std::uint32_t wlog, std::uint32_t hlog, std::uint32_t shift, std::uint32_t mask, std::uint32_t start) {
    const std::uint32_t width = 1u << wlog;
    const std::uint32_t stride = width < 16u ? 16u : width; // swizzled rows are whole 16-byte blocks
    const auto g = regs_for(format, clut_format, swizzled, wlog, hlog, stride, shift, mask, start);
    for (std::uint32_t i = 0; i < 0x20000u; i += 4) m.store32(kTex + i, rng() ^ (rng() << 16));
    const auto t = texture_info(g);
    std::vector<std::uint32_t> decoded;
    decode_texture(m, g, t, decoded);
    bool same = decoded.size() == static_cast<std::size_t>(t.width) * t.height;
    for (std::uint32_t y = 0; same && y < t.height; ++y)
        for (std::uint32_t x = 0; x < t.width; ++x) {
            const auto c = fetch_texel(m, g, t, static_cast<std::int32_t>(x), static_cast<std::int32_t>(y));
            const std::uint32_t expect = c[0] | (c[1] << 8) | (c[2] << 16) | (static_cast<std::uint32_t>(c[3]) << 24);
            if (decoded[static_cast<std::size_t>(y) * t.width + x] != expect) { same = false; break; }
        }
    char what[96];
    std::snprintf(what, sizeof what, "decode_texture fmt=%u clut=%u swz=%d %ux%u", format, clut_format, swizzled ? 1 : 0, width, 1u << hlog);
    check(same, what);
}
} // namespace

int main() {
    psprecomp::GuestMemory m;
    for (std::uint32_t format = 0; format < 8; ++format)
        for (std::uint32_t clut_format = 0; clut_format < 4; ++clut_format) {
            if (format < 4 && clut_format != 0) continue;
            for (int swz = 0; swz < 2; ++swz) {
                compare_case(m, format, clut_format, swz != 0, 6, 5, 0, 0xFF, 0);
                compare_case(m, format, clut_format, swz != 0, 3, 3, 0, 0xFF, 0);
            }
        }
    compare_case(m, 5, 2, false, 5, 5, 4, 0x0F, 3); // CLUT8 with shift/mask/start
    compare_case(m, 4, 3, true, 7, 4, 0, 0x0F, 1);  // CLUT4 32-bit palette, start block

    // texture_hash reacts to texture bytes and to the palette.
    auto g = regs_for(5, 1, false, 5, 5, 32, 0, 0xFF, 0);
    const auto t = texture_info(g);
    const auto h0 = texture_hash(m, g, t);
    check(h0 == texture_hash(m, g, t), "texture_hash is stable");
    m.store8(kTex + 17u, static_cast<std::uint8_t>(m.load8(kTex + 17u) ^ 0x5Au));
    const auto h1 = texture_hash(m, g, t);
    check(h1 != h0, "texture_hash changes with texture bytes");
    g.clut[3] ^= 0x00010000u;
    check(texture_hash(m, g, t) != h1, "texture_hash changes with the palette");

    if (failures == 0) std::printf("test_ge_geometry: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
