#pragma once

// Moving saves between this installation and elsewhere: a PSP memory stick,
// PPSSPP, or another installation. The in-game menu's Import and Export use
// it; nothing here draws or needs a window, so it is tested headless.
//
// Import checks a folder before it copies anything, and never deletes a save:
// one it replaces is moved to <ms0>/PSP/SAVEDATA/.backup/<time>/<name>/.
// Export writes a fresh folder laid out like a memory stick, so it never
// replaces anything either.
#include "save_data/aes128.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mhp3rd::savedata {

// The folders this game keeps on a memory stick: the game data, the
// downloaded quests, and the install data (a cache the game can rebuild).
inline constexpr std::string_view kGameName = "ULJM05800";
inline constexpr std::string_view kSaveFolderNames[] = {"ULJM05800", "ULJM05800QST", "ULJM05800DAT"};

[[nodiscard]] bool is_game_save_name(std::string_view folder_name);
// "Game data", "Downloaded quests", "Install data", or the name itself.
[[nodiscard]] std::string save_label(std::string_view folder_name);

// The key the game passes to the save-data utility, remembered from its first
// request (the game reads its save at boot), and the memory stick it saves
// to. Both are empty until the game has set them.
void remember_game_key(const std::string &game_name, const Block &key);
[[nodiscard]] std::optional<Block> game_key();
void set_memory_stick(const std::filesystem::path &memory_stick);
[[nodiscard]] std::filesystem::path memory_stick();

// What a folder holds, for showing an imported save beside the one it replaces.
struct FolderSummary {
    bool exists{};
    std::uint64_t bytes{};
    std::chrono::system_clock::time_point modified{};  // the newest file's time
};
[[nodiscard]] FolderSummary summarize_folder(const std::filesystem::path &folder);

// The result of checking a save folder: its name (from PARAM.SFO) and, when
// it cannot be imported, why, in words for the player.
struct SaveCheck {
    std::filesystem::path folder;
    std::string name;     // SAVEDATA_DIRECTORY, e.g. ULJM05800QST
    std::string problem;  // empty: the save can be imported
    bool other_game{};    // a save, but not this game's
    [[nodiscard]] bool ok() const { return problem.empty(); }
};

// Checks a save folder: PARAM.SFO parses and names one of this game's
// folders, its own hashes match, and every data file it lists is there,
// matches its hash and decrypts with `key`. Without a key, an encrypted data
// file cannot be checked and the folder is refused.
[[nodiscard]] SaveCheck check_save_folder(const std::filesystem::path &folder, const std::optional<Block> &key);

// The save folders in what the player picked: the folder itself when it holds
// a PARAM.SFO, otherwise the save folders in it, in its SAVEDATA or in its
// PSP/SAVEDATA (a memory stick's root). Each is checked; those of other
// games are included, marked other_game, so they can be counted.
[[nodiscard]] std::vector<SaveCheck> find_saves(const std::filesystem::path &picked, const std::optional<Block> &key);

// "2026-09-19_18-05-42", for folder names; "2026-09-19 18:05" for people.
[[nodiscard]] std::string timestamp_for_path(std::chrono::system_clock::time_point time);
[[nodiscard]] std::string timestamp_for_display(std::chrono::system_clock::time_point time);

// Where a save replaced at `time` is kept: <savedata>/.backup/<time>/, with
// "-2", "-3"... appended when that folder is already taken.
[[nodiscard]] std::filesystem::path backup_directory(const std::filesystem::path &savedata_root,
                                                     std::chrono::system_clock::time_point time);

struct ImportResult {
    bool ok{};
    std::string error;
    std::filesystem::path destination;
    std::filesystem::path backup;  // where the replaced save went; empty when none was replaced
};

// Copies a checked save folder into <memory_stick>/PSP/SAVEDATA/<name>. The
// copy is made beside the destination first; an existing save is then moved
// into `backup_dir`, and the copy moved into place. Nothing is deleted except
// the partial copy of a failed import.
ImportResult import_save(const SaveCheck &save, const std::filesystem::path &memory_stick,
                         const std::filesystem::path &backup_dir);

struct ExportResult {
    bool ok{};
    std::string error;
    std::filesystem::path folder;       // <target>/MHP3rd saves <time>
    std::vector<std::string> exported;  // folder names
};

// Copies this game's save folders from the memory stick to
// <target>/MHP3rd saves <time>/PSP/SAVEDATA/, the layout of a memory stick,
// so the PSP folder can be copied onto one as it is.
ExportResult export_saves(const std::filesystem::path &memory_stick, const std::filesystem::path &target,
                          std::chrono::system_clock::time_point time);

// The folders a backup holds: every folder of this game on the memory stick,
// the install data included.
[[nodiscard]] std::vector<std::string> saves_to_back_up(const std::filesystem::path &memory_stick);

// Where a backup goes. With a time, a new folder named by it:
// <target>/2026-09-19_19-05-12/ULJM05800, ... ("-2" and so on when taken).
// Without, the save folders go straight into <target>.
[[nodiscard]] std::filesystem::path backup_folder(const std::filesystem::path &target,
                                                  std::optional<std::chrono::system_clock::time_point> time);

// The save folders an untimed backup into `folder` would replace.
[[nodiscard]] std::vector<std::string> backup_conflicts(const std::filesystem::path &memory_stick,
                                                        const std::filesystem::path &folder);

struct BackupResult {
    bool ok{};
    std::string error;
    std::filesystem::path folder;
    std::vector<std::string> saved;  // folder names
};

// Copies the save folders into `folder` (from backup_folder). A save folder
// already there is replaced only with `replace`, which the player confirms;
// it is replaced whole, and only once its new copy is complete.
BackupResult back_up_saves(const std::filesystem::path &memory_stick, const std::filesystem::path &folder,
                           bool replace);

} // namespace mhp3rd::savedata
