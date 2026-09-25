#pragma once

#include "ge_state.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// HD texture packs in the replacement format PPSSPP defined: a folder of PNG
// files and a textures.ini that maps texture hashes to them.
//
//   [options]
//   version = 1
//   hash = xxh64
//   ignoreAddress = true
//
//   [hashes]
//   0000000022585cbda625131a = ui/capcom.png
//
// A key is 24 hex digits. The first 16 are a 64-bit cache key: the texture's
// address in the high 32 bits (zero when the pack sets ignoreAddress), and in
// the low 32 bits its dimension word (log2 height << 8 | log2 width), XORed
// with the palette's hash for paletted formats. The last 8 are the hash of
// the texture's bytes as they sit in guest memory. The code here was written
// for this project from the format's public description and from the keys
// real packs use; see docs/SOURCE_PROVENANCE.md.
//
// This file knows nothing about Vulkan: it hashes textures, finds their
// replacement files and decodes those on worker threads. The renderer uploads
// the decoded images (replacement_textures.hpp).
namespace mhp3rd::gpu {

enum class ReplacementFilter : std::uint8_t { Auto, Nearest, Linear };

// One image file of a pack. Every texture whose key leads to the same file
// shares one of these, and so one GPU image.
struct Replacement {
    enum class State : std::uint8_t {
        Unloaded,  // nothing in memory; the renderer may request it
        Queued,    // waiting for a loader thread
        Decoded,   // pixels below are ready for the renderer to upload
        Resident,  // uploaded; the pixels have been freed
        Failed,    // missing or unreadable; never tried again
    };

    std::filesystem::path file;  // absolute
    std::string name;            // as the pack names it, for messages
    ReplacementFilter filter{ReplacementFilter::Auto};
    // [hashranges] let a pack replace only the top-left covered_width x
    // covered_height texels of a texture_width x texture_height texture; the
    // loader pads the image so it still covers the whole texture.
    std::uint32_t texture_width{};
    std::uint32_t texture_height{};
    std::uint32_t covered_width{};
    std::uint32_t covered_height{};

    std::atomic<State> state{State::Unloaded};
    // Written by a loader thread before it publishes Decoded, then read and
    // freed by the renderer's thread.
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<std::uint8_t> pixels;  // RGBA8, rows packed

    // Owned by the renderer's thread: its GPU copy, or null.
    void *gpu{};
};

// The two halves of a pack key.
struct TexturePackKey {
    std::uint64_t cache_key{};
    std::uint32_t data_hash{};
    bool operator==(const TexturePackKey &) const = default;
};

// MHP3RD_TRACE_TEXTURE_PACK: log every lookup, decode and upload.
[[nodiscard]] bool texture_pack_trace();

// "0000000022585cbda625131a"
[[nodiscard]] std::string format_texture_pack_key(const TexturePackKey &key);

enum class TexturePackHash : std::uint8_t { Xxh32, Xxh64 };

// What [options] says, plus the parts of the other sections that change a
// texture's key.
struct TexturePackOptions {
    TexturePackHash hash{TexturePackHash::Xxh64};
    bool ignore_address{};
    bool reduce_hash{};
    float reduce_hash_default{0.5f};
    bool ignore_mipmap{};
    std::uint32_t skip_last_dxt1_blocks_128x64{};
    std::uint32_t skip_last_dxt1_blocks_128x128{};
    // [hashranges] address,w,h = w,h
    std::unordered_map<std::uint64_t, std::pair<std::uint32_t, std::uint32_t>> hash_ranges;
    // [reducehashranges] w,h = factor
    std::unordered_map<std::uint32_t, float> reduce_hash_ranges;
};

// Hashes a texture the way the format defines. `max_seen_v` is the highest
// V a through-mode draw has read from a 512-texel-tall texture, which limits
// how many rows are hashed (0: not known, hash them all). Returns false when
// the texture cannot be hashed. Sets `covered_width` and `covered_height` to
// the part of the texture a [hashranges] entry names, or its full size.
bool compute_texture_pack_key(const GuestMemory &memory, const TextureState &texture, std::uint16_t max_seen_v,
                              const TexturePackOptions &options, TexturePackKey &key, std::uint32_t &covered_width,
                              std::uint32_t &covered_height);

// What a pack holds, for checking one before it is installed.
struct TexturePackInfo {
    TexturePackOptions options;
    std::size_t keys{};               // texture keys, from textures.ini and hash-named images
    std::vector<std::string> files;   // image files the keys name, relative to the pack, each once
    std::vector<std::string> games;   // the game IDs textures.ini's [games] lists
};

// The game IDs a textures.ini's [games] section lists; empty when it has none
// or cannot be read.
[[nodiscard]] std::vector<std::string> texture_pack_games(const std::filesystem::path &ini);

class TexturePack {
public:
    // Reads textures.ini (and the hash-named images in the folder itself) from
    // `directory`. Returns null with `error` set when there is no usable pack.
    static std::unique_ptr<TexturePack> open(const std::filesystem::path &directory, const std::string &game_id,
                                             std::string &error);
    // Reads a pack as open() does, without starting anything. False with
    // `error` set when open() would refuse it.
    static bool inspect(const std::filesystem::path &directory, const std::string &game_id, TexturePackInfo &info,
                        std::string &error);
    ~TexturePack();
    TexturePack(const TexturePack &) = delete;
    TexturePack &operator=(const TexturePack &) = delete;

    // Hashes `texture` and finds its replacement: null when the pack has none
    // or says to leave the texture alone. Hashing reads the whole texture, so
    // call it once per texture upload, never per draw.
    std::shared_ptr<Replacement> find(const GuestMemory &memory, const TextureState &texture,
                                      std::uint16_t max_seen_v);

    // Starts decoding an Unloaded replacement on a loader thread. Only the
    // renderer's thread calls this and consumed().
    void request(Replacement &replacement);
    // The renderer has uploaded a Decoded replacement: frees its pixels and
    // marks it Resident.
    void consumed(Replacement &replacement);

    [[nodiscard]] const std::filesystem::path &directory() const noexcept { return directory_; }
    [[nodiscard]] const TexturePackOptions &options() const noexcept { return options_; }
    [[nodiscard]] std::size_t entry_count() const noexcept { return entries_.size(); }

private:
    TexturePack() = default;
    // open() without the loader threads.
    static std::unique_ptr<TexturePack> parse(const std::filesystem::path &directory, const std::string &game_id,
                                              std::string &error);
    struct KeyHash {
        std::size_t operator()(const TexturePackKey &key) const noexcept {
            return static_cast<std::size_t>(key.cache_key * 0x9E3779B97F4A7C15ull ^ key.data_hash);
        }
    };
    bool load_ini(const std::filesystem::path &file, bool is_override, std::string &error);
    void scan_hash_named_files();
    // The pack's rule for a key, trying the wildcard forms the format allows
    // in its order; null when there is none.
    template <typename Map>
    [[nodiscard]] typename Map::const_iterator lookup(const Map &map, const TexturePackKey &key) const;
    [[nodiscard]] std::filesystem::path resolve(const std::string &name);
    void loader_main();
    void decode(Replacement &replacement);

    std::filesystem::path directory_;
    TexturePackOptions options_;
    std::vector<std::string> games_;  // [games]
    // Key -> file name relative to the pack; an empty name means "keep the
    // original texture".
    std::unordered_map<TexturePackKey, std::string, KeyHash> entries_;
    std::unordered_map<TexturePackKey, ReplacementFilter, KeyHash> filters_;
    // One Replacement per file name.
    std::unordered_map<std::string, std::shared_ptr<Replacement>> files_;
    // Lower-case relative path -> path on disk, filled on the first name that
    // does not open as written: packs made on Windows do not always match
    // their files' case, and Linux file systems care.
    std::unordered_map<std::string, std::filesystem::path> folded_names_;
    bool folded_scanned_{};
    std::mutex resolve_mutex_;

    std::mutex mutex_;
    std::condition_variable wake_;
    std::condition_variable room_;
    std::deque<Replacement *> queue_;
    std::vector<std::thread> loaders_;
    bool stopping_{};
    // Bytes decoded but not yet taken by the renderer; the loaders pause above
    // a limit so a burst of requests cannot fill the host's memory.
    std::size_t decoded_bytes_{};
};

// MHP3RD_TEXTURE_DUMP=<folder>: writes every texture the game uploads, once,
// as <key>.png into the folder, keyed the way a pack with `options` would key
// it, so a pack can be started from the dump. Runs on a background thread.
class TextureDumper {
public:
    explicit TextureDumper(std::filesystem::path directory);
    ~TextureDumper();
    TextureDumper(const TextureDumper &) = delete;
    TextureDumper &operator=(const TextureDumper &) = delete;
    // `pixels` is width x height RGBA8 (red in the low byte).
    void dump(const GuestMemory &memory, const TextureState &texture, std::uint16_t max_seen_v,
              const TexturePackOptions &options, const std::uint32_t *pixels);

private:
    struct Job {
        std::filesystem::path path;
        std::uint32_t width{};
        std::uint32_t height{};
        std::vector<std::uint32_t> pixels;
    };
    void writer_main();

    std::filesystem::path directory_;
    std::mutex mutex_;
    std::condition_variable wake_;
    std::deque<Job> jobs_;
    std::unordered_map<std::string, bool> written_;
    bool stopping_{};
    std::thread writer_;
};

} // namespace mhp3rd::gpu
