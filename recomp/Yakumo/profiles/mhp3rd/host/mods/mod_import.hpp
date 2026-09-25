#pragma once

#include "mods/mod_library.hpp"

#include <filesystem>
#include <string>
#include <vector>

// Importing mods the player downloaded and unpacked: the mod folders found in
// a chosen folder are copied into the mods folder. Nothing is deleted: a mod
// folder of the same name that is already there moves to mods/.backup first.
//
// Game-independent; the game's ModFormat says what a mod folder is.
namespace mhp3rd::mods {

struct ImportCandidate {
    std::filesystem::path folder;  // where it is now
    std::string id;                // its folder name, and so its name in the mods folder
    Mod mod;
    bool replaces{};               // a mod of that name is installed already
};

struct ImportCheck {
    std::filesystem::path chosen;
    std::vector<ImportCandidate> mods;
    std::string problem;  // why nothing can be imported; empty when something can
};

// The mods in `chosen`: the folder itself when it is a mod, otherwise the mod
// folders in it, or one level further down (an unpacked archive often wraps
// its mods in a folder of its own).
[[nodiscard]] ImportCheck check_import(const std::filesystem::path &chosen, const ModFormat &format,
                                       const std::filesystem::path &mods_folder);

struct ImportResult {
    std::vector<std::string> imported;              // ids
    std::vector<std::filesystem::path> backups;     // where replaced mods went
    std::string error;                              // the first failure; the rest were still tried
};

// Copies the candidates into `mods_folder`. Blocking; mods are small.
[[nodiscard]] ImportResult import_mods(const ImportCheck &check, const std::filesystem::path &mods_folder);

} // namespace mhp3rd::mods
