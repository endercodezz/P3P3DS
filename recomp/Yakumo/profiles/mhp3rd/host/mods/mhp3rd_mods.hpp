#pragma once

#include "mods/mod_session.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

namespace psprecomp {
class Runtime;
}

namespace mhp3rd {
class IsoImage;
}

// Mods for this game: the mods folder in the per-user data directory, read in
// the community's format (mhp3rd_mod_format.hpp), served to the game through
// its DATA.BIN reads (mhp3rd_data_bin.hpp).
//
// With no mods folder, or no mod on, none of this changes a byte the game
// reads: the file I/O asks serving() and reads the disc as before.
//
//   MHP3RD_MODS_DIR    the mods folder, instead of mods/ in the data directory
//   MHP3RD_NO_MODS=1   no mod applies this run; the menu still lists them
//   MHP3RD_TRACE_MODS  logs what the mods change and every read they serve
namespace mhp3rd::mods {

// The disc image, once it is open, before the game runs. Reads the mods
// folder and the choices in mods.ini.
void attach_disc(IsoImage *disc);

// Where DATA.BIN is on the disc image, in bytes. Nothing without a disc.
struct DiscRange {
    std::uint64_t offset{};
    std::uint64_t size{};
};
[[nodiscard]] std::optional<DiscRange> data_bin_on_disc();

// Whether the mods change what the game reads from DATA.BIN now. Cheap.
[[nodiscard]] bool serving();
// DATA.BIN's size as the game sees it: larger when a mod's file grew it.
[[nodiscard]] std::uint64_t data_bin_size();
// Reads DATA.BIN as the game sees it, at an offset into the archive.
std::size_t read_data_bin(std::uint64_t offset, std::span<std::uint8_t> out);

// The game flushed its instruction cache, which it does right after copying a
// code overlay into place: the point where a code overlay has finished
// loading. Writes the patches that go to memory outside a loaded overlay.
void code_loaded(psprecomp::Runtime &runtime);

// The session the menu manages; null when there is no disc.
[[nodiscard]] ModSession *session();
[[nodiscard]] std::filesystem::path mods_directory();
// "Applies at the next load" or "Restart to apply": what the last change needs.
[[nodiscard]] std::string pending_note();
// Problems met while serving a mod's file this run, newest last.
[[nodiscard]] const std::vector<std::string> &problems();

} // namespace mhp3rd::mods
