// Builder EBOOT decryption (platform/pc/builder/psp_eboot.cpp): SHA-256 and
// AES-128 known-answer vectors, and -- when the maintainer's ISO is given --
// the decrypted ULUS-10512 executable must match tools/prepare_game.py's
// reference SHA-256.
#include "psp_eboot.hpp"
#include "p3p3ds/vfs.hpp"

#include <cstdio>
#include <cstring>

using namespace p3p3ds::builder;

int main(int argc, char **argv) {
    int failures = 0;
    auto check = [&](bool ok, const char *what) { if (!ok) { std::printf("FAIL: %s\n", what); ++failures; } };

    // FIPS 180-4 example "abc".
    check(sha256_hex({'a', 'b', 'c'}) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "sha256(abc)");
    // FIPS-197 appendix C.1: AES-128 decryption of 69c4e0d86a7b0430d8cdb78070b4c55a.
    const std::uint8_t key[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    const std::uint8_t ct[16] = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30, 0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};
    const auto pt = aes128_cbc_decrypt(key, ct, 16);
    const std::uint8_t expect[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    check(std::memcmp(pt.data(), expect, 16) == 0, "aes128 FIPS-197 C.1");

    if (argc > 1) {
        p3p3ds::vfs::IsoFileSystem iso(argv[1]);
        const auto source = iso.open("PSP_GAME/SYSDIR/EBOOT.BIN");
        check(source != nullptr, "EBOOT.BIN in the ISO");
        if (source) {
            std::vector<std::uint8_t> eboot(static_cast<std::size_t>(source->size()));
            source->read(0, eboot.data(), eboot.size());
            const auto elf = decrypt_eboot(eboot);
            check(sha256_hex(elf) == kExpectedElfSha256, "decrypted ELF matches the reference SHA-256");
        }
    } else {
        std::printf("no ISO given: ELF check skipped\n");
    }
    if (failures == 0) std::printf("test_psp_eboot: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
