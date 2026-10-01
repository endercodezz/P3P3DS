#pragma once
// Decryption of the ULUS-10512 retail EBOOT.BIN (~PSP container, tag
// 0xD91613F0) into the ELF the recompiler reads. C++ port of
// tools/prepare_game.py (decrypt_eboot), checked against its output by
// tests/test_psp_eboot.cpp via the reference SHA-256.
#include <cstdint>
#include <string>
#include <vector>

namespace p3p3ds::builder {

// SHA-256 of the decrypted ULUS-10512 executable (tools/prepare_game.py).
inline constexpr const char *kExpectedElfSha256 = "be2abbd43a4ae7ce5aa2f1f146083ffe0d924fc2eb1874e6fcdc21cba40d49db";

// AES-128-CBC decryption with a zero IV (the only mode the container uses).
std::vector<std::uint8_t> aes128_cbc_decrypt(const std::uint8_t key[16], const std::uint8_t *data, std::size_t size);

// Returns the decrypted ELF, or throws std::runtime_error with a user-facing message.
std::vector<std::uint8_t> decrypt_eboot(const std::vector<std::uint8_t> &eboot);

std::string sha256_hex(const std::vector<std::uint8_t> &data);

} // namespace p3p3ds::builder
