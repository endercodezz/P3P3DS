#pragma once

// The PSP save-data utility's protection of a save: the data file's
// encryption with the game's key, the file's hash kept in PARAM.SFO's
// SAVEDATA_FILE_LIST, and the hashes of PARAM.SFO itself in SAVEDATA_PARAMS.
//
// Everything is built from AES-128 with keys the PSP's KIRK engine holds in
// fixed slots (the keys of KIRK commands 4 and 7, selected by a key seed) and
// the save-data keys, all published values:
//
//   hash     AES-CMAC under one KIRK key, masked with a save-data key; with a
//            game key, the result is XORed with it and encrypted once more.
//   cipher   A 16-byte header precedes the data. The header XOR the game key,
//            masked and decrypted with a KIRK key, gives a 12-byte prefix.
//            Blocks "prefix || 32-bit counter" (counter from 1) are decrypted
//            with a second KIRK key in CBC mode; the result is XORed with the
//            data, so encryption and decryption are the same operation.
//
// Three modes exist; this game's saves use mode 5 (SAVEDATA_PARAMS flag 0x41).
#include "save_data/aes128.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace mhp3rd::savedata {

enum class CryptMode : std::uint8_t {
    Mode1 = 1,  // no game key
    Mode3 = 3,  // game key, firmware 2.x titles
    Mode5 = 5,  // game key, SDK 4 and later
};

// Length of the header the utility puts in front of an encrypted data file.
inline constexpr std::size_t kEncryptedHeaderSize = 16u;

// SAVEDATA_PARAMS (128 bytes) layout.
inline constexpr std::size_t kParamsSize = 128u;
inline constexpr std::size_t kParamsFlagsOffset = 0x00u;
inline constexpr std::size_t kParamsHashMode1Offset = 0x10u;    // mode 1 hash of the whole PARAM.SFO
inline constexpr std::size_t kParamsHashConsoleOffset = 0x20u;  // made with a key unique to each PSP
inline constexpr std::size_t kParamsHashModeOffset = 0x70u;     // mode 3 or 5 hash, without the 0x10 one

[[nodiscard]] bool is_zero(const Block &block);

// The mode recorded in SAVEDATA_PARAMS's flag byte, if it is one this code
// understands. Flag 0 means the save is not encrypted.
[[nodiscard]] std::optional<CryptMode> mode_from_flags(std::uint8_t flags);
[[nodiscard]] std::uint8_t flags_for_mode(CryptMode mode);

// AES-CMAC (NIST SP 800-38B / RFC 4493).
[[nodiscard]] Block cmac(const Aes128 &cipher, std::span<const std::uint8_t> data);

// Hash of an encrypted data file as stored in SAVEDATA_FILE_LIST.
[[nodiscard]] Block data_file_hash(std::span<const std::uint8_t> encrypted_file, CryptMode mode,
                                   const Block *game_key);

// Encrypts `plain`, padded with zeros to a multiple of 16 bytes; the result
// starts with the 16-byte header. `random` stands in for the random number
// the PSP draws for the header.
[[nodiscard]] std::vector<std::uint8_t> encrypt_data(std::span<const std::uint8_t> plain, CryptMode mode,
                                                     const Block *game_key, const Block &random);
// Decrypts a data file written by encrypt_data or by a PSP. The result is
// the file's length minus the header.
[[nodiscard]] std::optional<std::vector<std::uint8_t>> decrypt_data(std::span<const std::uint8_t> file, CryptMode mode,
                                                                   const Block *game_key);

// Sets SAVEDATA_PARAMS's flags and hashes inside a serialized PARAM.SFO
// whose SAVEDATA_PARAMS starts at `params_offset`.
void sign_param_sfo(std::vector<std::uint8_t> &sfo, std::size_t params_offset, CryptMode mode);
// True when the hashes of a serialized PARAM.SFO match its flags (the
// console-specific one cannot be checked).
[[nodiscard]] bool verify_param_sfo(std::span<const std::uint8_t> sfo, std::size_t params_offset);

} // namespace mhp3rd::savedata
