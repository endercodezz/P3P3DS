#pragma once

#include "mods/mod_library.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The bytes a game file has once the enabled mods are applied: a
// replacement's, or the game's own, with the patches applied in order. The
// game's archive reader serves these in place of its own bytes; the game's
// code notices nothing.
//
// Game-independent: the game supplies how to read one of its files and how to
// apply a patch file to it.
namespace mhp3rd::mods {

// A write a patch makes to guest memory outside the file itself, done once the
// game has finished loading the file (see ModPatcher).
struct MemoryWrite {
    std::uint32_t address{};
    std::vector<std::uint8_t> bytes;
};

struct PatchOutcome {
    std::size_t applied{};                // parts of the patch applied to the file's bytes
    std::vector<MemoryWrite> after_load;  // parts to write to memory after the load
    std::vector<std::string> problems;    // parts skipped, and why
};

struct FileContent {
    std::vector<std::uint8_t> bytes;
    bool replaced{};
    std::size_t patches{};  // patch files applied
    std::vector<MemoryWrite> after_load;
    std::vector<std::string> problems;
};

class FileOverlay {
public:
    struct Game {
        // The game's own bytes of a file.
        std::function<std::vector<std::uint8_t>(FileId)> original;
        // Its size, without reading it.
        std::function<std::uint64_t(FileId)> original_size;
        // Applies one patch file to a file's bytes.
        std::function<PatchOutcome(FileId, std::vector<std::uint8_t> &, const std::filesystem::path &)> patch;
    };

    explicit FileOverlay(Game game) : game_(std::move(game)) {}

    // Uses `resolution` from now on; contents built for the previous one are dropped.
    void set(Resolution resolution);
    [[nodiscard]] const Resolution &resolution() const noexcept { return resolution_; }

    // Whether the mods change this file at all.
    [[nodiscard]] bool touches(FileId file) const {
        return resolution_.replacements.contains(file) || resolution_.patches.contains(file);
    }
    // The size the file has with the mods, without building it: a
    // replacement's size, or the original's (patches never resize a file).
    [[nodiscard]] std::optional<std::uint64_t> size(FileId file) const;
    // Every file the mods change, with its size.
    [[nodiscard]] std::map<FileId, std::uint64_t> sizes() const;

    // Built on first use and kept. Null when the mods do not change the file,
    // or a replacement cannot be read (the game then gets its own file).
    [[nodiscard]] std::shared_ptr<const FileContent> content(FileId file);

private:
    Game game_;
    Resolution resolution_;
    std::map<FileId, std::shared_ptr<const FileContent>> cache_;
};

} // namespace mhp3rd::mods
