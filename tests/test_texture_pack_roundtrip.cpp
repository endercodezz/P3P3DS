// Round trip between the two implementations of the New 3DS texture pack
// format (issue #63): `p3p-texpack build` (Rust, tools/texpack/src/build.rs)
// writes packs from tools/texpack/tests/fixtures (textures.ini there), this
// test reads them with the runtime's TexturePack (core/src/ge/texture_pack.cpp)
// and compares every texel with the formulas the fixture images were generated
// from (tools/texpack/tests/fixtures/make_fixtures.py).
//
//   test_texture_pack_roundtrip <pack built with --format rgba8> <pack built with --format auto>
#include "p3p3ds/ge/texture_pack.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace p3p3ds::ge;

namespace {
int failures = 0;
void check(bool ok, const std::string &what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what.c_str());
        ++failures;
    }
}

struct Rgba {
    std::uint32_t r, g, b, a;
};

// make_fixtures.py images.
Rgba all_values(std::uint32_t x, std::uint32_t y) {
    const std::uint32_t v = y * 16 + x;
    return {v, 255 - v, (v * 7) & 255u, (v * 13 + 5) & 255u};
}
Rgba pattern(std::uint32_t x, std::uint32_t y) {
    return {(x * 16) & 255u, (y * 32) & 255u, (x * y * 7) & 255u, (x + y) % 3 ? 255u : (x * 20) & 255u};
}
Rgba rgb(std::uint32_t x, std::uint32_t y) {
    Rgba c = all_values(x, y);
    c.a = 255; // stored without alpha
    return c;
}
Rgba gray(std::uint32_t x, std::uint32_t y) {
    const std::uint32_t g = (x * 32 + y * 4) & 255u;
    return {g, g, g, 255};
}

struct Image {
    std::uint64_t cachekey;
    std::uint32_t width, height;
    Rgba (*pixel)(std::uint32_t, std::uint32_t);
    std::uint8_t auto_format; // citro3d GPU_TEXCOLOR `build --format auto` picks
    const char *name;
};
const Image kImages[] = {
    {1, 16, 16, all_values, 4, "all_values_16x16"}, // partial alpha -> RGBA4
    {2, 16, 8, pattern, 4, "pattern_16x8"},         // partial alpha -> RGBA4
    {3, 16, 16, rgb, 3, "rgb_16x16"},               // opaque -> RGB565
    {4, 8, 8, gray, 3, "gray_8x8"},                 // opaque -> RGB565
};

// What a texel becomes after encoding to `format` and decoding back:
// tex3ds rules (alpha 0 -> transparent black, channels truncated) and the
// reader's bit expansion.
Rgba through_format(Rgba c, std::uint8_t format) {
    if (c.a == 0) return {0, 0, 0, 0};
    auto expand = [](std::uint32_t v, int bits) { return (v << (8 - bits)) | (v >> (2 * bits - 8 > 0 ? 2 * bits - 8 : 0)); };
    switch (format) {
    case 0: return c;
    case 3: return {expand(c.r >> 3, 5), expand(c.g >> 2, 6), expand(c.b >> 3, 5), 255};
    case 4: return {(c.r >> 4) * 17, (c.g >> 4) * 17, (c.b >> 4) * 17, (c.a >> 4) * 17};
    default: return {0xFFFFFFFFu, 0, 0, 0}; // never equal
    }
}

void check_pack(const char *dir, bool exact_rgba8) {
    std::string error;
    const auto pack = TexturePack::open(dir, error);
    check(pack != nullptr, std::string(dir) + ": open (" + error + ")");
    if (!pack) return;
    check(pack->size() == 5, std::string(dir) + ": 5 entries");
    check(pack->ignore_address(), std::string(dir) + ": ignoreAddress flag");
    const PackEntry *keep = pack->find({5, 5});
    check(keep && keep->width == 0, std::string(dir) + ": keep-original entry");
    for (const Image &img : kImages) {
        const std::string what = std::string(dir) + " " + img.name;
        const PackEntry *e = pack->find({img.cachekey, static_cast<std::uint32_t>(img.cachekey)});
        check(e != nullptr, what + ": entry found");
        if (!e) continue;
        const std::uint8_t format = exact_rgba8 ? 0 : img.auto_format;
        check(e->width == img.width && e->height == img.height, what + ": size");
        check(e->format == format, what + ": format " + std::to_string(e->format));
        check(e->offset % 128u == 0, what + ": 128-byte aligned");
        std::vector<std::uint8_t> data;
        check(pack->read(*e, data), what + ": read");
        std::vector<std::uint32_t> rgba;
        decode_pack_texture(*e, data, rgba);
        if (rgba.size() != static_cast<std::size_t>(img.width) * img.height) continue;
        int bad = 0;
        for (std::uint32_t y = 0; y < img.height; ++y)
            for (std::uint32_t x = 0; x < img.width; ++x) {
                const Rgba want = through_format(img.pixel(x, y), format);
                const std::uint32_t got = rgba[static_cast<std::size_t>(y) * img.width + x];
                const bool same = (got & 0xFFu) == want.r && ((got >> 8) & 0xFFu) == want.g && ((got >> 16) & 0xFFu) == want.b &&
                                  (got >> 24) == want.a;
                if (!same && bad++ == 0)
                    std::printf("  %s: texel (%u,%u) got %08x want %02x%02x%02x%02x (abgr)\n", what.c_str(), x, y, got, want.a, want.b,
                                want.g, want.r);
            }
        check(bad == 0, what + ": " + std::to_string(bad) + " texels differ");
    }
}
} // namespace

int main(int argc, char **argv) {
    if (argc != 3) {
        std::printf("usage: test_texture_pack_roundtrip <rgba8 pack dir> <auto pack dir>\n");
        return 2;
    }
    check_pack(argv[1], true);
    check_pack(argv[2], false);
    if (failures == 0) std::printf("test_texture_pack_roundtrip: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
