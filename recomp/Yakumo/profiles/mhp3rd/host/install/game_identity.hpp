#pragma once

// Identity of the one release this profile supports. The hashes match
// config/mhp3rd_npjb40001.toml and the profile README.
namespace mhp3rd::install {

// DISC_ID in PSP_GAME/PARAM.SFO.
inline constexpr const char *kDiscId = "NPJB40001";
inline constexpr const char *kDiscIdDisplay = "NPJB-40001";
inline constexpr const char *kGameTitle = "Monster Hunter Portable 3rd HD Ver.";

inline constexpr const char *kExecutablePathOnDisc = "PSP_GAME/SYSDIR/EBOOT.BIN";
inline constexpr const char *kParamSfoPathOnDisc = "PSP_GAME/PARAM.SFO";

// SHA-256 of PSP_GAME/SYSDIR/EBOOT.BIN as it is on the disc.
inline constexpr const char *kEncryptedExecutableSha256 =
    "79e25f3512d56e0f7bf5c48351d7d0d255269675ffc8811ac5599322bb66945e";
// SHA-256 of the executable the recompiled code was generated from.
inline constexpr const char *kExecutableSha256 =
    "55c0598436c0753b04331f8e95d406f832d9217806e3a896fed0e88b33637d8c";

} // namespace mhp3rd::install
