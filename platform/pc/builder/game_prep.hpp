#pragma once
// ISO -> decrypted executable path shared by the Builder (pipeline.cpp) and
// the developer setup (p3p_prepare_elf, called by tools/prepare_game.py):
// PARAM.SFO identity, EBOOT.BIN decryption and the reference SHA-256 check.
// Error messages match tools/prepare_game.py, which reports them unchanged.
#include "p3p3ds/vfs.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace p3p3ds::builder {

// SHA-256 of the encrypted retail EBOOT.BIN of ULUS-10512 (tools/prepare_game.py).
// A different value is only reported: the decrypted ELF hash is what must match.
inline constexpr const char *kExpectedEbootSha256 = "eeb19d9dcab8c42743151bae65dc6a40031b05f35b89db27deee5665a0be2855";
inline constexpr const char *kSupportedDiscId = "ULUS10512";

std::vector<std::uint8_t> read_source(const std::shared_ptr<const vfs::Source> &src);

// PARAM.SFO -> value of `key` (string entries only), empty when absent.
std::string sfo_string(const std::vector<std::uint8_t> &sfo, const std::string &key);

struct PreparedExecutable {
    std::string disc_id;  // DISC_ID without '-'
    std::string title;
    std::string version;  // DISC_VERSION, else APP_VER, else "1.00"
    bool from_override = false;
    std::uint64_t eboot_size = 0;   // encrypted EBOOT.BIN (0 with an override)
    std::string eboot_sha256;       // empty with an override
    std::vector<std::uint8_t> elf;
    std::string elf_sha256;
};

// Validates the image, then decrypts its EBOOT.BIN or, when `decrypted_override`
// is not empty, reads that ELF instead. Throws std::runtime_error when the image
// is not ULUS-10512 or the executable does not match kExpectedElfSha256.
PreparedExecutable prepare_executable(const std::filesystem::path &iso, const std::filesystem::path &decrypted_override = {});

} // namespace p3p3ds::builder
