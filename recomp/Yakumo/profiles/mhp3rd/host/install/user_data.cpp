#include "install/user_data.hpp"

#include "psprecomp/common.hpp"

#include <cstdlib>
#include <fstream>
#include <system_error>

#if defined(MHP3RD_HAS_SDL)
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#endif

namespace mhp3rd::install {
namespace {

constexpr const char *kOrganization = "Yakumo";
constexpr const char *kApplication = "MHP3rd";

std::string trim(const std::string &text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1u);
}

#if !defined(MHP3RD_HAS_SDL)
std::filesystem::path environment_path(const char *name) {
    const char *value = std::getenv(name);
    return value != nullptr && *value != '\0' ? path_from_utf8(value) : std::filesystem::path{};
}
#endif

} // namespace

std::string path_to_utf8(const std::filesystem::path &path) {
    const std::u8string text = path.u8string();
    return {text.begin(), text.end()};
}

std::filesystem::path path_from_utf8(const std::string &text) {
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

std::filesystem::path user_data_directory() {
    if (const char *dir = std::getenv("MHP3RD_DATA_DIR"); dir != nullptr && *dir != '\0') return path_from_utf8(dir);
#if defined(MHP3RD_HAS_SDL)
    char *pref = SDL_GetPrefPath(kOrganization, kApplication);
    if (pref == nullptr) throw psprecomp::Error(std::string("Cannot determine the user data directory: ") + SDL_GetError());
    std::filesystem::path result = path_from_utf8(pref);
    SDL_free(pref);
    return result;
#else
    // The locations SDL_GetPrefPath uses, so builds with and without SDL agree.
    std::filesystem::path base;
#if defined(_WIN32)
    base = environment_path("APPDATA");
#elif defined(__APPLE__)
    if (const auto home = environment_path("HOME"); !home.empty()) base = home / "Library" / "Application Support";
#else
    base = environment_path("XDG_DATA_HOME");
    if (const auto home = environment_path("HOME"); base.empty() && !home.empty()) base = home / ".local" / "share";
#endif
    if (base.empty()) throw psprecomp::Error("Cannot determine the user data directory; set MHP3RD_DATA_DIR");
    return base / kOrganization / kApplication;
#endif
}

SettingsEntries read_settings_file(const std::filesystem::path &data_dir) {
    SettingsEntries entries;
    std::ifstream in(data_dir / kSettingsFile);
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        const auto equals = line.find('=');
        if (equals == std::string::npos) continue;
        const std::string key = trim(line.substr(0, equals));
        if (!key.empty()) entries[key] = trim(line.substr(equals + 1u));
    }
    return entries;
}

void write_settings_file(const std::filesystem::path &data_dir, const SettingsEntries &entries) {
    std::filesystem::create_directories(data_dir);
    const std::filesystem::path target = data_dir / kSettingsFile;
    const std::filesystem::path partial = data_dir / (std::string(kSettingsFile) + ".part");
    {
        std::ofstream out(partial, std::ios::trunc);
        out << "# Written by Yakumo: the installer and the in-game menu (Esc, or L3+R3 on a gamepad).\n"
            << "# disc_image: the disc image to play from; a relative path is inside this directory.\n";
        for (const auto &[key, value] : entries) out << key << "=" << value << "\n";
        if (!out) throw psprecomp::Error("Cannot write " + path_to_utf8(partial));
    }
    std::filesystem::rename(partial, target);
}

UserSettings load_settings(const std::filesystem::path &data_dir) {
    UserSettings settings;
    const SettingsEntries entries = read_settings_file(data_dir);
    if (const auto found = entries.find("disc_image"); found != entries.end() && !found->second.empty())
        settings.disc_image = path_from_utf8(found->second);
    return settings;
}

void save_settings(const std::filesystem::path &data_dir, const UserSettings &settings) {
    SettingsEntries entries = read_settings_file(data_dir);
    entries["disc_image"] = path_to_utf8(settings.disc_image);
    write_settings_file(data_dir, entries);
}

std::optional<Installation> find_installation(const std::filesystem::path &data_dir) {
    std::error_code ec;
    const std::filesystem::path executable = data_dir / kExecutableFile;
    if (!std::filesystem::is_regular_file(executable, ec)) return std::nullopt;
    const UserSettings settings = load_settings(data_dir);
    Installation installation;
    installation.executable = executable;
    installation.disc_image = settings.disc_image.is_absolute() ? settings.disc_image : data_dir / settings.disc_image;
    installation.image_copied = !settings.disc_image.is_absolute();
    return installation;
}

} // namespace mhp3rd::install
