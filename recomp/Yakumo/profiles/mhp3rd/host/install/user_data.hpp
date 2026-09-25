#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace mhp3rd::install {

// The per-user data directory holds what the installer sets up:
//
//   EBOOT.ELF      the executable prepared from the player's disc image
//   disc.iso       the copied disc image (absent when the image is used in place)
//   settings.ini   where the disc image is, and the player's settings
//
// Save data is not here yet: ms0 stays where the host has always kept it.
inline constexpr const char *kExecutableFile = "EBOOT.ELF";
inline constexpr const char *kCopiedImageFile = "disc.iso";
// Android: asks the next start to run the setup, as --install does elsewhere.
inline constexpr const char *kSetupMarkerFile = "setup-requested";
inline constexpr const char *kSettingsFile = "settings.ini";

// MHP3RD_DATA_DIR when set, otherwise SDL_GetPrefPath("Yakumo", "MHP3rd")
// (or the same location computed by hand in a build without SDL). SDL creates
// the directory if it does not exist yet.
[[nodiscard]] std::filesystem::path user_data_directory();

// Every key=value line of settings.ini. The installer owns disc_image; the
// player's settings (host/settings) keep their keys next to it, and writing
// one never drops the others.
using SettingsEntries = std::map<std::string, std::string>;

[[nodiscard]] SettingsEntries read_settings_file(const std::filesystem::path &data_dir);
// Replaces settings.ini with `entries`, creating data_dir if needed.
void write_settings_file(const std::filesystem::path &data_dir, const SettingsEntries &entries);

struct UserSettings {
    // Disc image to read. Relative paths are relative to the data directory.
    std::filesystem::path disc_image = kCopiedImageFile;
};

[[nodiscard]] UserSettings load_settings(const std::filesystem::path &data_dir);
void save_settings(const std::filesystem::path &data_dir, const UserSettings &settings);

struct Installation {
    std::filesystem::path executable;
    std::filesystem::path disc_image; // absolute
    bool image_copied{};              // disc image lives in the data directory
};

// The installation in data_dir, if the installer has completed there. The disc
// image is not checked for existence: callers report a missing image.
[[nodiscard]] std::optional<Installation> find_installation(const std::filesystem::path &data_dir);

// UTF-8 conversions for paths shown in dialogs, stored in settings or received
// from SDL.
[[nodiscard]] std::string path_to_utf8(const std::filesystem::path &path);
[[nodiscard]] std::filesystem::path path_from_utf8(const std::string &text);

} // namespace mhp3rd::install
