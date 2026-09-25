#pragma once

#include "texture_pack.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

// Installing a texture pack from a folder the player chooses: finding the
// pack in it, checking it before anything is copied, copying it beside the
// installed one and swapping them, and keeping the pack it replaces.
//
// The pack folder is found in any of the layouts packs come in, where
// <chosen> is the folder picked and <id> the game's disc ID:
//
//   <chosen>/textures.ini                       the pack folder itself
//   <chosen>/textures/<id>/textures.ini         a data folder like Yakumo's
//   <chosen>/<id>/textures.ini                  a TEXTURES folder
//   <chosen>/PSP/TEXTURES/<id>/textures.ini     PPSSPP's memstick
//   <chosen>/.../<other id>/textures.ini        a pack named for another
//                                               release whose [games] lists <id>
//
// Folder names are compared without regard to case. Nothing here draws or
// knows about the renderer.
namespace mhp3rd::gpu {

// Where the pack is read from: the folder MHP3RD_TEXTURE_PACK names, when it
// names one; else the folder a pack used in place was left in (the
// video.texture_pack_folder setting, `in_place`); else <textures_root>/<id>,
// where an import copies packs.
struct TexturePackLocation {
    enum class Source : std::uint8_t { Installed, InPlace, Variable };
    std::filesystem::path folder;
    Source source{Source::Installed};
};
[[nodiscard]] TexturePackLocation texture_pack_location(const std::filesystem::path &textures_root,
                                                        const std::string &game_id, const std::string &in_place);

struct TexturePackCheck {
    std::filesystem::path chosen;  // what the player picked
    std::filesystem::path folder;  // the pack folder found in it; empty when none
    std::string layout;            // how it was found, for the review screen
    std::string made_for;          // a pack folder named for another release, installed under the game's ID
    std::string problem;           // why it cannot be installed; empty when it can

    TexturePackHash hash{TexturePackHash::Xxh64};
    bool ignore_address{};
    std::size_t keys{};
    std::size_t images{};               // image files in the folder (PNG and the formats packs also carry)
    std::size_t files{};                // every file that would be copied
    std::uint64_t bytes{};              // their total size
    std::size_t missing{};              // files textures.ini names that are not there
    std::vector<std::string> missing_names;  // the first few, for the screen

    [[nodiscard]] bool found() const noexcept { return !folder.empty(); }
    [[nodiscard]] bool ok() const noexcept { return found() && problem.empty(); }
};

// Finds and checks the pack in `chosen` for the game `game_id`.
[[nodiscard]] TexturePackCheck check_texture_pack(const std::filesystem::path &chosen, const std::string &game_id);

// The pack a folder holds now, for "what it would replace". `exists` is false
// when there is no such folder.
struct InstalledTexturePack {
    bool exists{};
    std::size_t keys{};
    std::size_t files{};
    std::uint64_t bytes{};
    std::string problem;  // when it does not load
};
[[nodiscard]] InstalledTexturePack summarize_texture_pack(const std::filesystem::path &folder,
                                                          const std::string &game_id);

// Free bytes where `folder` is or would be created; nothing when unknown.
[[nodiscard]] std::optional<std::uint64_t> texture_pack_free_space(const std::filesystem::path &folder);
// Room left on top of a pack's own size, for the file system's overhead.
inline constexpr std::uint64_t kTexturePackSpaceMargin = 64ull * 1024u * 1024u;

// textures/.backup/<date>_<time>, made unique with -2, -3... The pack it
// replaces moves into it under its own name.
[[nodiscard]] std::filesystem::path texture_pack_backup_directory(const std::filesystem::path &textures_root,
                                                                  std::chrono::system_clock::time_point time);

// Copies a checked pack into a staging folder beside the installed one,
// textures/.incomplete-<date>_<time>, on a thread of its own. The installed
// pack is not touched until install_staged_texture_pack().
class TexturePackCopy {
public:
    struct Progress {
        std::uint64_t bytes{};
        std::uint64_t total_bytes{};
        std::size_t files{};
        std::size_t total_files{};
        std::string current;  // the file being copied, relative to the pack
    };
    enum class State { Idle, Copying, Done, Failed, Cancelled };

    TexturePackCopy() = default;
    ~TexturePackCopy();
    TexturePackCopy(const TexturePackCopy &) = delete;
    TexturePackCopy &operator=(const TexturePackCopy &) = delete;

    // Starts copying `check.folder` into `textures_root`. Staging folders left
    // by an earlier copy that never finished are removed first.
    void start(const TexturePackCheck &check, const std::filesystem::path &textures_root);
    // Asks the copy to stop; the staging folder is removed.
    void cancel() noexcept { cancel_ = true; }
    // Waits for the thread; afterwards state() is final.
    void join();

    [[nodiscard]] State state() const noexcept { return state_.load(); }
    [[nodiscard]] Progress progress() const;
    // Once Done: the folder holding the copy.
    [[nodiscard]] const std::filesystem::path &staging() const noexcept { return staging_; }
    // Once Failed: why.
    [[nodiscard]] std::string error() const;

private:
    void run(std::filesystem::path source);

    std::thread thread_;
    std::atomic<State> state_{State::Idle};
    std::atomic<bool> cancel_{false};
    mutable std::mutex mutex_;
    Progress progress_;
    std::string error_;
    std::filesystem::path staging_;
};

// Puts a finished copy in place: the pack at textures_root/<game_id>, if any,
// moves into `backup_dir`, then the staging folder takes its name. On failure
// the old pack is put back. `backup` is set to where the old pack went.
bool install_staged_texture_pack(const std::filesystem::path &staging, const std::filesystem::path &textures_root,
                                 const std::string &game_id, const std::filesystem::path &backup_dir,
                                 std::filesystem::path &backup, std::string &error);

// Removes a staging folder this import made. Never called on anything else.
void discard_staged_texture_pack(const std::filesystem::path &staging);

} // namespace mhp3rd::gpu
