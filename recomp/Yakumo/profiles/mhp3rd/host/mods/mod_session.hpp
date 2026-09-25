#pragma once

#include "mods/mod_library.hpp"

#include <filesystem>
#include <functional>
#include <string>

// One run's mods: the library read from the mods folder, the choices saved
// next to settings.ini, and what the game is using now. A change the player
// makes applies at once when the game can take it while running, and
// otherwise at the next start.
//
// Game-independent. The game says whether a new set of mods can be switched
// to while it runs, and switches.
namespace mhp3rd::mods {

class ModSession {
public:
    struct Paths {
        std::filesystem::path folder;   // the mods folder
        std::filesystem::path choices;  // mods.ini
        // The environment variable that turned every mod off for this run,
        // or null. The library is still read and shown.
        const char *disabled_by{};
    };
    struct Game {
        // Whether the game can switch to `next` while it runs.
        std::function<bool(const Resolution &next)> can_switch;
        // Makes `next` the mods the game gets from its next file load on.
        std::function<void(const Resolution &next)> activate;
    };

    ModSession(const ModFormat &format, Paths paths, Game game);

    // Reads the folder and the choices and activates them. Called before the
    // game reads its first file.
    void start();
    // Reads the folder again, for mods added or removed while running.
    void rescan();
    // Saves the player's changes and applies them now if the game can take
    // them; otherwise they wait for the restart.
    void commit();

    [[nodiscard]] ModLibrary &library() noexcept { return library_; }
    [[nodiscard]] const Paths &paths() const noexcept { return paths_; }
    // The mods the game uses now, and the ones the choices ask for.
    [[nodiscard]] const Resolution &active() const noexcept { return active_; }
    [[nodiscard]] const Resolution &wanted() const noexcept { return wanted_; }
    [[nodiscard]] bool restart_pending() const { return !active_.same_files(wanted_); }
    [[nodiscard]] const std::string &error() const noexcept { return error_; }

private:
    [[nodiscard]] Resolution resolve() const;

    ModLibrary library_;
    Paths paths_;
    Game game_;
    Resolution active_;
    Resolution wanted_;
    std::string error_;
};

} // namespace mhp3rd::mods
