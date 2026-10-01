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
class HostFileSystem final : public FileSystem {
public:
    explicit HostFileSystem(std::filesystem::path root);
    [[nodiscard]] std::shared_ptr<const Source> open(std::string_view path) const override;
    [[nodiscard]] std::optional<Entry> stat(std::string_view path) const override;
    [[nodiscard]] std::optional<std::vector<Entry>> list(std::string_view path) const override;

private:
    [[nodiscard]] std::optional<std::filesystem::path> resolve(std::string_view path) const;
    std::filesystem::path root_;
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
