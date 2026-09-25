#pragma once

#include "psprecomp/elf32.hpp"
#include "psprecomp/runtime.hpp"

#include <cstdint>
#include <filesystem>
#include <iterator>

namespace mhp3rd {

inline constexpr std::uint32_t kLoadBase = psprecomp::kDefaultPspUserLoadBase;
inline constexpr std::uint32_t kGuestRamBytes = 64u * 1024u * 1024u;

struct ProfilePaths {
    std::filesystem::path disc_image;    // UMD ISO; empty disables disc0:
    std::filesystem::path memory_stick;  // host directory backing ms0:
};

// Installs the kernel and HLE modules, binds logging stubs for the remaining
// imports (unless MHP3RD_STRICT_HLE is set) and prepares the loader thread
// that runs module_start.
void install_profile(psprecomp::Runtime &runtime, const psprecomp::Elf32Image &elf, const ProfilePaths &paths);

// The host directory that backs ms0:, where the game's saves live under
// PSP/SAVEDATA. This is the only place that decides it; the rest of the host
// receives the result, so moving saves to a per-user location changes only
// this function.
[[nodiscard]] std::filesystem::path memory_stick_directory(const std::filesystem::path &game_dir);

// Overlay slots, from the executable's section table. They sit inside the load
// image's reserved BSS, and the game copies code into them at run time, so a
// jump into one stops the runtime until that overlay has its own corpus.
// The end of each slot is the start of the next one.
inline constexpr std::uint32_t kOverlaySlots[] = {
    0x0A001780u,  // demo_sub, game_sub
    0x0A055E80u,  // P_m*/P_v* maps
    0x0A05E600u,  // *_task mode overlays
    0x0A1BB000u,  // em*m0 monsters, result
    0x0A1EFE80u,  // em*m1
    0x0A224D00u,  // em*m2
    0x0A239780u,  // em*m3
    0x0A24E200u,  // we*player00 weapons
    0x0A25BA80u,  // we*player01
    0x0A269300u,  // we*player02
    0x0A276B80u,  // we*player03, tutorialm1
    0x0A284400u,  // P_v00 and village maps
    0x0A285200u,  // end of the load image
};

} // namespace mhp3rd
