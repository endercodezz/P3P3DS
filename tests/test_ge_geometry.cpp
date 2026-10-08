// Shared GE front end (geometry.hpp): decode_texture must equal fetch_texel
// for every texel of every non-DXT format, linear and swizzled, with CLUT
// shift/mask/start; texture_hash must change with texture bytes and palette; decode_model_vertices
// must equal decode_model_vertex for every vertex type.
#include "p3p3ds/ge/geometry.hpp"
#include "p3p3ds/ge/vertex_cache.hpp"
#include "psprecomp/guest_memory.hpp"

#include <cstdio>
#include <cstring>
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

// VertexCache: reuse only for identical guest bytes, volatile sources left
// uncached, budget with eviction of sources not used recently, and the cached
// conversion of real decoded vertices equal to a fresh one after a CPU write.
void vertex_cache_checks(psprecomp::GuestMemory &m) {
    constexpr std::uint32_t kSrc = 0x08930000u;
    constexpr std::size_t kRaw = 18u * 6u, kOut = 36u * 6u;
    for (std::uint32_t i = 0; i < 0x1000u; ++i) m.store8(kSrc + i, static_cast<std::uint8_t>(rng()));
    int converts = 0;
    auto convert_from = [&](std::uint32_t address) {
        return [&m, &converts, address](std::uint8_t *out) {
            ++converts;
            for (std::size_t i = 0; i < kOut; ++i) out[i] = static_cast<std::uint8_t>(m.load8(address + static_cast<std::uint32_t>(i % kRaw)) ^ 0xA5u);
        };
    };
    auto expected = [&](std::uint32_t address, const std::uint8_t *got) {
        for (std::size_t i = 0; i < kOut; ++i)
            if (got[i] != static_cast<std::uint8_t>(m.load8(address + static_cast<std::uint32_t>(i % kRaw)) ^ 0xA5u)) return false;
        return true;
    };
    VertexCache cache(1u << 20);
    const VertexCacheKey key{kSrc, 0x15Au, 0, 6, 0};
    const std::uint8_t *raw = m.raw_pointer(kSrc, kRaw);
    check(raw != nullptr, "raw_pointer of guest RAM");
    const std::uint8_t *a = cache.get(key, raw, kRaw, kOut, 1, convert_from(kSrc));
    check(a != nullptr && converts == 1 && expected(kSrc, a), "vertex cache: first use converts");
    const std::uint8_t *b = cache.get(key, raw, kRaw, kOut, 2, convert_from(kSrc));
    check(b != nullptr && converts == 1 && expected(kSrc, b) && cache.stats().hits == 1u, "vertex cache: unchanged bytes hit");
    m.store8(kSrc + 7u, static_cast<std::uint8_t>(m.load8(kSrc + 7u) ^ 0x01u));
    const std::uint8_t *c = cache.get(key, raw, kRaw, kOut, 3, convert_from(kSrc));
    check(c != nullptr && converts == 2 && expected(kSrc, c), "vertex cache: a changed byte converts again");
    VertexCacheKey other = key;
    other.material = 0x336699u;
    check(cache.get(other, raw, kRaw, kOut, 3, convert_from(kSrc)) != nullptr && converts == 3, "vertex cache: material is part of the key");

    // Changes on kVolatileAfter uses in a row: uncached for kVolatileFrames.
    std::uint64_t frame = 4;
    check(cache.get(key, raw, kRaw, kOut, frame++, convert_from(kSrc)) != nullptr, "vertex cache: hit resets the change count");
    const std::uint8_t *v = nullptr;
    for (std::uint32_t i = 0; i < VertexCache::kVolatileAfter; ++i) {
        m.store8(kSrc + 3u, static_cast<std::uint8_t>(m.load8(kSrc + 3u) + 1u));
        v = cache.get(key, raw, kRaw, kOut, frame++, convert_from(kSrc));
    }
    check(v == nullptr && cache.stats().volatile_skips == 1u, "vertex cache: a source changing every use becomes volatile");
    check(cache.get(key, raw, kRaw, kOut, frame, convert_from(kSrc)) == nullptr, "vertex cache: volatile source stays uncached");
    const int before = converts;
    const std::uint8_t *back = cache.get(key, raw, kRaw, kOut, frame + VertexCache::kVolatileFrames, convert_from(kSrc));
    check(back != nullptr && converts == before + 1 && expected(kSrc, back), "vertex cache: cached again after the volatile window");

    // Budget: room for two sources; a third in the same frame is refused, and
    // admitted once the others were not used for two frames.
    VertexCache small(2u * (kRaw + kOut + VertexCache::kEntryOverhead));
    const VertexCacheKey k1{kSrc, 1, 0, 6, 0}, k2{kSrc, 2, 0, 6, 0}, k3{kSrc, 3, 0, 6, 0};
    check(small.get(k1, raw, kRaw, kOut, 10, convert_from(kSrc)) != nullptr, "vertex cache budget: first source");
    check(small.get(k2, raw, kRaw, kOut, 10, convert_from(kSrc)) != nullptr, "vertex cache budget: second source");
    check(small.get(k3, raw, kRaw, kOut, 10, convert_from(kSrc)) == nullptr && small.stats().full == 1u, "vertex cache budget: full");
    check(small.get(k3, raw, kRaw, kOut, 12, convert_from(kSrc)) != nullptr && small.stats().entries == 1u,
          "vertex cache budget: stale sources evicted");

    // Real conversion: decoded model vertices through the cache equal a fresh
    // decode, also after the game rewrites one vertex.
    GeRegisters r;
    r.reg[0x55] = 0x336699u; r.reg[0x58] = 0x80u;
    const auto layout = vertex_layout(0x00015Au); // battle models: s16 position/normal, u16 UV, 4444 colour
    constexpr std::uint32_t kN = 9;
    constexpr std::size_t kPacked = 16u * sizeof(float) + 4u; // like the 3DS shader vertex
    const std::size_t raw_size = layout.size * kN, out_size = kN * kPacked;
    VertexCache models(1u << 20);
    auto decode_into = [&](std::uint8_t *out) {
        std::vector<ModelVertex> tmp(kN);
        decode_model_vertices(m, r, layout, kSrc, 0, kN, tmp.data());
        for (const auto &mv : tmp) {
            std::memcpy(out, mv.pos, sizeof mv.pos);
            std::memcpy(out + 12, mv.normal, sizeof mv.normal);
            std::memcpy(out + 24, mv.uv, sizeof mv.uv);
            std::memcpy(out + 32, mv.weights, sizeof mv.weights);
            std::memcpy(out + 64, mv.color.data(), 4);
            out += kPacked;
        }
    };
    bool equal = true;
    for (int round = 0; round < 3; ++round) {
        if (round == 2) m.store16(kSrc + layout.pos_offset, static_cast<std::uint16_t>(m.load16(kSrc + layout.pos_offset) + 0x100u));
        const std::uint8_t *got = models.get({kSrc, 0x15Au, 0, kN, 0}, m.raw_pointer(kSrc, raw_size), raw_size, out_size,
                                             static_cast<std::uint64_t>(20 + round), decode_into);
        std::vector<std::uint8_t> fresh(out_size);
        decode_into(fresh.data());
        equal &= got != nullptr && std::memcmp(got, fresh.data(), out_size) == 0;
    }
    check(equal && models.stats().hits == 1u && models.stats().misses == 2u, "vertex cache: cached decode equals fresh decode");
}

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

    // decode_model_vertices equals decode_model_vertex for every vertex type
    // (weights, UV, colour, normal, position formats; transform mode).
    {
        constexpr std::uint32_t kVerts = 0x08920000u;
        for (std::uint32_t i = 0; i < 0x4000u; i += 4) m.store32(kVerts + i, rng() ^ (rng() << 16));
        GeRegisters r;
        r.reg[0x55] = 0x336699u; r.reg[0x58] = 0x80u;
        bool same = true;
        int layouts = 0;
        for (std::uint32_t wf = 0; wf < 4; ++wf)
            for (std::uint32_t wc = 0; wc < 8; wc += 3)
                for (std::uint32_t uv = 0; uv < 4; ++uv)
                    for (std::uint32_t col : {0u, 4u, 5u, 6u, 7u})
                        for (std::uint32_t nf = 0; nf < 4; ++nf)
                            for (std::uint32_t pf = 1; pf < 4; ++pf) {
                                const std::uint32_t vtype = uv | (col << 2) | (nf << 5) | (pf << 7) | (wf << 9) | (wc << 14);
                                const auto l = vertex_layout(vtype);
                                std::vector<ModelVertex> fast(17);
                                if (!decode_model_vertices(m, r, l, kVerts, 3, 17, fast.data())) { same = false; continue; }
                                ++layouts;
                                for (std::uint32_t k = 0; k < 17u; ++k) {
                                    ModelVertex slow;
                                    decode_model_vertex(m, r, l, kVerts, 3 + k, slow);
                                    const auto &f = fast[k];
                                    // bitwise: random float components may be NaN
                                    same &= f.color == slow.color && f.has_color == slow.has_color &&
                                            std::memcmp(f.pos, slow.pos, sizeof f.pos) == 0 &&
                                            std::memcmp(f.normal, slow.normal, sizeof f.normal) == 0 &&
                                            std::memcmp(f.uv, slow.uv, sizeof f.uv) == 0 &&
                                            std::memcmp(f.weights, slow.weights, sizeof f.weights) == 0;
                                }
                            }
        check(same && layouts > 0, "decode_model_vertices equals decode_model_vertex");
    }

    vertex_cache_checks(m);

    if (failures == 0) std::printf("test_ge_geometry: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
