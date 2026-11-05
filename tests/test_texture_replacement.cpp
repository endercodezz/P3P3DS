// PPSSPP-compatible texture keys (core/src/ge/texture_replacement.cpp) and the
// xxHash they use (core/src/ge/xxhash.cpp).
// - XXH32/XXH64: vectors produced by the independent xxhash-rust crate
//   (xxh32/xxh64 features) over bytes data[i] = (i * 2654435761) >> 13.
// - Key parts, lookup order and the CLUT byte rules of texture_replacement.hpp
//   on synthetic register files and guest memory.
// The end-to-end check is the hit rate of a real pack on the PC runner
// (--texture-keys, docs/CURRENT_STATE.md).
#include "p3p3ds/ge/texture_replacement.hpp"
#include "p3p3ds/ge/xxhash.hpp"
#include "psprecomp/guest_memory.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

using namespace p3p3ds::ge;

namespace {
int failures = 0;
void check(bool ok, const char *what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

struct Vector {
    std::size_t length;
    std::uint32_t seed, h32;
    std::uint64_t h64;
};
const Vector kVectors[] = {
        {0, 0x00000000u, 0x02CC5D05u, 0xEF46DB3751D8E999ull},
        {0, 0xBACD7814u, 0xD81D1328u, 0xD0D2CF1C5037EFFFull},
        {0, 0xC0108888u, 0x8C64D961u, 0x48138B9840A4C6B6ull},
        {1, 0x00000000u, 0xCF65B03Eu, 0xE934A84ADB052768ull},
        {1, 0xBACD7814u, 0xAF0592E8u, 0x3E343F14983643CBull},
        {1, 0xC0108888u, 0xB9A2994Eu, 0xC712709BC0413F08ull},
        {3, 0x00000000u, 0xC3A1C35Cu, 0x28823E205E353F69ull},
        {3, 0xBACD7814u, 0x4320008Eu, 0x2F19A8CA3EA30430ull},
        {3, 0xC0108888u, 0x805D2290u, 0xF4EE16EFA2D624C3ull},
        {4, 0x00000000u, 0xB7BE83E5u, 0x91B65BBE720C34CFull},
        {4, 0xBACD7814u, 0x0F778587u, 0x33BF8E028CB1E54Cull},
        {4, 0xC0108888u, 0xF4491807u, 0x92A137C11BB716F7ull},
        {15, 0x00000000u, 0xC0E32965u, 0x4EE77AB371D06E31ull},
        {15, 0xBACD7814u, 0xCF82F247u, 0x1E27B8D7DDA5D81Cull},
        {15, 0xC0108888u, 0x5C1296EAu, 0xC774E8A1FF4B2F7Full},
        {16, 0x00000000u, 0x0789A02Au, 0xB1A375C6BD6BA7AFull},
        {16, 0xBACD7814u, 0xE54A82F2u, 0x36D1A98686B23259ull},
        {16, 0xC0108888u, 0x04630ADAu, 0x6464DC30573E419Full},
        {17, 0x00000000u, 0xD16E7800u, 0xBBE746D4E95F47CCull},
        {17, 0xBACD7814u, 0x86E328BCu, 0x5D43AF15EC0523FAull},
        {17, 0xC0108888u, 0x10244EFFu, 0x4F3651C13EA4354Full},
        {31, 0x00000000u, 0x535B3CBBu, 0xB74BAA9042B94DEEull},
        {31, 0xBACD7814u, 0x42776848u, 0x0DEE399751D76BE9ull},
        {31, 0xC0108888u, 0x8E58861Fu, 0xEE0DE078DA572005ull},
        {32, 0x00000000u, 0x81BA5047u, 0x4E13111CED6F735Dull},
        {32, 0xBACD7814u, 0x8BA2B38Eu, 0xA4BA1F4EB8950E98ull},
        {32, 0xC0108888u, 0x42F761D5u, 0x21232787867C622Bull},
        {33, 0x00000000u, 0xD6BC9BC8u, 0xF7B9FAF20B3BCE63ull},
        {33, 0xBACD7814u, 0xC18F85F0u, 0x286222B884B4DDDAull},
        {33, 0xC0108888u, 0x54412033u, 0x3D6453B568D9EF59ull},
        {63, 0x00000000u, 0x68FE5F24u, 0x35F935E6044A08ADull},
        {63, 0xBACD7814u, 0x2EEEF205u, 0xD52FED9494017035ull},
        {63, 0xC0108888u, 0x8DAA15FCu, 0x5A1DA996401EADDBull},
        {64, 0x00000000u, 0x8092ECEFu, 0xCD91DAEC2C21766Full},
        {64, 0xBACD7814u, 0xB6BEEAD8u, 0x83F24961539D1C46ull},
        {64, 0xC0108888u, 0xB3B052D5u, 0x150E652A138965CDull},
        {100, 0x00000000u, 0x8F3D6FC3u, 0xD61EA92C5AE13676ull},
        {100, 0xBACD7814u, 0x885EDC9Eu, 0xCC1F84DF6E024312ull},
        {100, 0xC0108888u, 0xFDDB5498u, 0x2631F88CF2FBBC07ull},
        {300, 0x00000000u, 0x248CE8E6u, 0x09238F35B40D0B40ull},
        {300, 0xBACD7814u, 0x175F038Au, 0xFCDC232DD3996D0Eull},
        {300, 0xC0108888u, 0xA42C50C5u, 0x8FC54A4E0AEC83EAull},
};

void set_reg(GeRegisters &regs, std::uint32_t op, std::uint32_t value) { regs.reg[op] = value & 0xFFFFFFu; }
} // namespace

int main() {
    std::vector<std::uint8_t> data(300);
    for (std::uint32_t i = 0; i < data.size(); ++i) data[i] = static_cast<std::uint8_t>((i * 2654435761u) >> 13);
    std::size_t checked = 0;
    for (const auto &v : kVectors) {
        char what[96];
        std::snprintf(what, sizeof what, "xxh32 len %zu seed %08x", v.length, v.seed);
        check(xxh32(data.data(), v.length, v.seed) == v.h32, what);
        std::snprintf(what, sizeof what, "xxh64 len %zu seed %08x", v.length, v.seed);
        check(xxh64(data.data(), v.length, v.seed) == v.h64, what);
        ++checked;
    }
    check(checked == 42, "all xxHash vectors present");

    // Pack keys.
    const auto key = parse_replacement_key("00000000a4a55853f1750742");
    check(key && key->cachekey == 0xa4a55853ull && key->hash == 0xf1750742u, "parse key");
    check(!parse_replacement_key("00000000a4a55853f175074") && !parse_replacement_key("00000000a4a55853f175074g"), "reject malformed keys");

    // Lookup order with ignoreAddress (address bits dropped first).
    const ReplacementKey full{0x0956'23B0'0000'0806ull, 0x11223344u};
    ReplacementKeySet pack{{0x806ull, 0}, {0, 0x11223344u}};
    auto found = find_replacement(pack, full, true);
    check(found && found->cachekey == 0x806ull && found->hash == 0, "CLUT-only key before data-only key");
    pack.insert({0x806ull, 0x11223344u});
    found = find_replacement(pack, full, true);
    check(found && found->hash == 0x11223344u && found->cachekey == 0x806ull, "exact key first");
    ReplacementKeySet with_address{{0x0956'23B0'0000'0000ull, 0}};
    check(!find_replacement(with_address, full, true), "address-only keys need ignoreAddress off");
    check(find_replacement(with_address, full, false).has_value(), "address-only key without ignoreAddress");

    // CLUT hash: bytes = min(last load + start index bytes, largest load), and
    // the CLUTFORMAT command word (0xC5 byte included) is xored in.
    GeRegisters regs;
    set_reg(regs, 0xC3, 5);               // CLUT8
    set_reg(regs, 0xC5, 0x00FF03u);       // 32-bit palette, mask 0xFF, start 0
    PpssppClut clut;
    std::vector<std::uint8_t> palette(1024);
    for (std::size_t i = 0; i < palette.size(); ++i) palette[i] = static_cast<std::uint8_t>(i * 7);
    clut.load(palette.data(), 1024);
    clut.load(palette.data(), 64);        // a smaller later load keeps the rest of the buffer
    std::uint32_t expect = xxh32(clut.raw.data(), 64, 0xC0108888u) ^ 0xC500FF03u;
    check(replacement_clut_hash(regs, clut) == expect, "CLUT hash of the last load");
    set_reg(regs, 0xC5, 0x02FF03u);       // start index 2 * 16 entries * 4 bytes = 128 more bytes
    expect = xxh32(clut.raw.data(), 64 + 128, 0xC0108888u) ^ 0xC502FF03u;
    check(replacement_clut_hash(regs, clut) == expect, "CLUT hash includes the start index bytes");
    set_reg(regs, 0xC3, 3);               // 8888: no CLUT
    check(replacement_clut_hash(regs, clut) == 0, "no CLUT hash for direct colour");

    // Cachekey: address << 32 | dim, CLUT hash xored for CLUT formats.
    set_reg(regs, 0xA0, 0x5623B0u);
    set_reg(regs, 0xA8, 0x090100u);       // upper address 0x09, buffer width 256
    set_reg(regs, 0xB8, 0x0608u);         // 256 x 64
    check(replacement_texture_address(regs) == 0x095623B0u, "texture address");
    check(replacement_cachekey(regs, 0x12345678u) == (0x095623B0ull << 32 | 0x608u), "direct colour ignores the CLUT hash");
    set_reg(regs, 0xC3, 4);
    check(replacement_cachekey(regs, 0x12345678u) == ((0x095623B0ull << 32 | 0x608u) ^ 0x12345678u), "CLUT formats xor it");

    // Data hash: contiguous when the buffer is not wider than the texture,
    // otherwise rows combined as h = h * 11 ^ xxh64(row).
    psprecomp::GuestMemory memory;
    const std::uint32_t base = 0x08900000u;
    std::uint8_t *ram = memory.raw_pointer(base, 64 * 1024);
    check(ram != nullptr, "guest RAM");
    for (std::uint32_t i = 0; i < 64 * 1024; ++i) ram[i] = static_cast<std::uint8_t>(i * 31 + 7);
    GeRegisters tex;
    set_reg(tex, 0xA0, base & 0xFFFFFFu);
    set_reg(tex, 0xA8, (0x08u << 16) | 64u); // buffer width 64
    set_reg(tex, 0xB8, 0x0506u);             // 64 x 32
    set_reg(tex, 0xC3, 3);                   // 8888
    check(replacement_data_hash(memory, tex, 0) == static_cast<std::uint32_t>(xxh64(ram, 64 * 32 * 4, 0xBACD7814u)), "contiguous data hash");
    set_reg(tex, 0xA8, (0x08u << 16) | 128u); // buffer wider than the texture
    std::uint32_t rows = 0;
    for (std::uint32_t y = 0; y < 32; ++y) rows = rows * 11u ^ static_cast<std::uint32_t>(xxh64(ram + y * 128 * 4, 64 * 4, 0xBACD7814u));
    check(replacement_data_hash(memory, tex, 0) == rows, "row-wise data hash");
    set_reg(tex, 0xA8, (0x08u << 16) | 512u);
    set_reg(tex, 0xB8, 0x0909u);              // 512 x 512, maxSeenV 272 -> 272 rows hashed
    const std::uint8_t *tall = memory.raw_pointer(base, 512 * 512 * 4);
    check(tall != nullptr && replacement_data_hash(memory, tex, 272) == static_cast<std::uint32_t>(xxh64(tall, 512 * 272 * 4, 0xBACD7814u)),
          "512-tall texture hashes maxSeenV rows");

    // maxSeenV: through mode keeps max(272, V) on first use, 512 once V grows.
    TextureKeys keys;
    set_reg(tex, 0xC3, 3);
    const auto k1 = keys.key(memory, tex, clut, true, 100);
    check(k1.hash == replacement_data_hash(memory, tex, 272), "first through-mode use hashes 272 rows");
    const auto k2 = keys.key(memory, tex, clut, true, 300);
    check(k2.hash == replacement_data_hash(memory, tex, 512), "a taller draw switches to all 512 rows");

    if (failures == 0) std::printf("test_texture_replacement: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
