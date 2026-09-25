#pragma once

#include "mods/file_overlay.hpp"
#include "mods/mod_library.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

// The mod folders this game's community already uses, so that mods made for
// mhp3reload and its mod manager work unchanged. Implemented from their
// published documentation and from the files themselves, not from their code
// (see docs/SOURCE_PROVENANCE.md):
//
// - A mod is a folder with a mod.ini whose [MOD INFO] section names it, its
//   Type, its Files, and for file and patch mods the Target file ids.
// - A file id is a DATA.BIN entry index in four upper-case hex digits.
// - A folder without a mod.ini that holds files named by file id (0601) and
//   patches named by file id and P (0601P), as mhp3reload's own files folder
//   does, is taken as one mod.
//
// Game-specific: the machinery that uses this is in mod_library.hpp.
namespace mhp3rd::mods::p3rd {

class ModFolderFormat final : public ModFormat {
public:
    // `entries`: how many files DATA.BIN has; targets beyond it are refused.
    // 0 when not known yet.
    explicit ModFolderFormat(std::uint32_t entries = 0u) : entries_(entries) {}
    void set_entries(std::uint32_t entries) noexcept { entries_ = entries; }

    [[nodiscard]] std::optional<Mod> read(const std::filesystem::path &folder) const override;
    [[nodiscard]] std::string file_name(FileId file) const override;
    [[nodiscard]] std::optional<FileId> parse_file(const std::string &text) const override;

private:
    std::uint32_t entries_;
};

// Applies one patch file to a file's bytes. A patch is a list of blocks, each
// a guest address, a length and that many bytes, ended by the address
// FFFFFFFF (the "last 8 bytes FFFFFFFF00000000" of the format, which the
// manager strips when it installs a patch; both forms are read).
//
// - A block for an address inside a code overlay's image (the overlay's
//   header gives where it loads) changes those bytes of the file, which the
//   game copies to that address unchanged.
// - A block for other guest memory is written there once the overlay has
//   loaded (FileContent::after_load).
// - A block whose address is smaller than the file is taken as an offset
//   into the file, for files that load to no fixed address.
// - A block marked to run as it loads (the top bit of its length) is code,
//   which this port cannot run that way; it is skipped and reported.
[[nodiscard]] PatchOutcome apply_patch(std::vector<std::uint8_t> &bytes, const std::filesystem::path &patch);

// A code overlay's header at the start of a file: where it loads, and its
// image size (header, code and data). Nothing when the file is not one.
struct OverlayImage {
    std::uint32_t load{};
    std::uint32_t code_end{};  // guest address after the code
    std::uint32_t size{};
};
[[nodiscard]] std::optional<OverlayImage> overlay_image(const std::vector<std::uint8_t> &bytes);

} // namespace mhp3rd::mods::p3rd
