#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace mhp3rd::install {

// Turns PSP_GAME/SYSDIR/EBOOT.BIN of the supported disc into the executable
// the recompiled code was generated from.
//
// This is deliberately not a general tool: it accepts only the one file whose
// SHA-256 is kEncryptedExecutableSha256, handles only the header layout that
// file uses, and checks the result against kExecutableSha256. Anything else is
// refused with psprecomp::Error. Only the installer calls it, with a callback
// that receives the bytes decrypted so far and the total.
[[nodiscard]] std::vector<std::uint8_t>
prepare_executable(std::span<const std::uint8_t> eboot_bin,
                   const std::function<void(std::uint64_t done, std::uint64_t total)> &progress = {});

} // namespace mhp3rd::install
