#pragma once
// Virtual file system backends for IoFileMgr. Paths handed to a FileSystem are
// relative, '/'-separated and already normalized (no device, no "." / "..").
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace p3p3ds::vfs {

struct Entry {
    std::string name;
    bool directory{};
    std::uint64_t size{};
};

// Random-access read-only byte source.
class Source {
public:
    virtual ~Source() = default;
    [[nodiscard]] virtual std::uint64_t size() const = 0;
    virtual std::size_t read(std::uint64_t offset, void *destination, std::size_t length) const = 0;
};

class FileSystem {
public:
    virtual ~FileSystem() = default;
    [[nodiscard]] virtual std::shared_ptr<const Source> open(std::string_view path) const = 0;
    [[nodiscard]] virtual std::optional<Entry> stat(std::string_view path) const = 0;
    [[nodiscard]] virtual std::optional<std::vector<Entry>> list(std::string_view path) const = 0;
};

// Normalizes a PSP path (device already removed): collapses separators,
// drops "."; returns nullopt for ".." escaping the root.
[[nodiscard]] std::optional<std::string> normalize(std::string_view path);

// ISO9660 image (PSP UMD). Lookups are case-insensitive, like the UMD driver.
class IsoFileSystem final : public FileSystem {
public:
    explicit IsoFileSystem(std::filesystem::path image);
    [[nodiscard]] std::shared_ptr<const Source> open(std::string_view path) const override;
    [[nodiscard]] std::optional<Entry> stat(std::string_view path) const override;
    [[nodiscard]] std::optional<std::vector<Entry>> list(std::string_view path) const override;
    [[nodiscard]] std::size_t file_count() const noexcept { return files_; }

private:
    struct Node {
        std::string name;
        bool directory{};
        std::uint32_t lba{};
        std::uint64_t size{};
        std::vector<std::string> children; // upper-case keys
    };
    void load_directory(const std::string &key, std::uint32_t lba, std::uint32_t size, int depth);
    std::shared_ptr<std::ifstream> stream_;
    std::shared_ptr<std::mutex> lock_;
    std::map<std::string, Node> nodes_; // upper-case path -> node ("" = root)
    std::size_t files_{};
};

// A directory on the host (loose-file overrides, extracted data, ms0:).
// fixed_contents: the tree does not change while the game runs (read-only
// mod folders). Each directory is then listed from the host once and lookups
// are answered from memory: the Mod Support patch stats ms0:/PSP/P3P/bind/<file>
// before every game file it loads (voices, scripts), almost always missing,
// and each host lookup costs several SD card operations on the 3DS.
class HostFileSystem final : public FileSystem {
public:
    explicit HostFileSystem(std::filesystem::path root, bool fixed_contents = false);
    [[nodiscard]] std::shared_ptr<const Source> open(std::string_view path) const override;
    [[nodiscard]] std::optional<Entry> stat(std::string_view path) const override;
    [[nodiscard]] std::optional<std::vector<Entry>> list(std::string_view path) const override;

private:
    struct Listed { std::string name; bool directory{}; };
    using Listing = std::map<std::string, Listed>; // upper-case name -> entry
    [[nodiscard]] std::optional<std::filesystem::path> resolve(std::string_view path) const;
    // fixed_contents: host path and whether it is a directory, from cached listings.
    [[nodiscard]] std::optional<std::pair<std::filesystem::path, bool>> resolve_cached(std::string_view path) const;
    [[nodiscard]] std::shared_ptr<const Listing> listing(const std::string &key, const std::filesystem::path &directory) const;
    std::filesystem::path root_;
    bool fixed_contents_{};
    mutable std::mutex cache_lock_;
    mutable std::map<std::string, std::shared_ptr<const Listing>> listings_; // upper-case relative dir -> listing (null: missing)
};

// In-memory byte source (tests, synthesized overlays).
class MemorySource final : public Source {
public:
    explicit MemorySource(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}
    [[nodiscard]] std::uint64_t size() const override { return bytes_.size(); }
    std::size_t read(std::uint64_t offset, void *destination, std::size_t length) const override;

private:
    std::vector<std::uint8_t> bytes_;
};

} // namespace p3p3ds::vfs
