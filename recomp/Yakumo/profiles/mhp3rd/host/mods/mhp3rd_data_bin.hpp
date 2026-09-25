#pragma once

#include "mods/file_overlay.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

// DATA.BIN, the archive every asset and code overlay of this game comes from
// (docs/DATA_BIN.md), as the mods see it. This game's mods address a file by
// its index in the archive, so a mod's file id is an entry index.
//
// Game-specific: the archive's directory, its obfuscation, and a view of the
// archive with the mods' files in it that the file I/O serves to the game.
namespace mhp3rd::mods::p3rd {

inline constexpr std::uint32_t kBlock = 2048u;

// The obfuscation (docs/DATA_BIN.md): an XOR keystream seeded from the block
// address at which an entry starts, then a byte substitution. `offset` is the
// position of data[0] inside the entry, so any part of an entry can be done on
// its own.
void encrypt(std::span<std::uint8_t> data, std::uint32_t block, std::uint64_t offset);
void decrypt(std::span<std::uint8_t> data, std::uint32_t block, std::uint64_t offset);
// Re-keys an entry's bytes from one starting block to another.
void transcode(std::span<std::uint8_t> data, std::uint32_t from_block, std::uint32_t to_block, std::uint64_t offset);
// Entries stored as they are, not obfuscated: PRX stubs and PSMF movies. The
// game tells them by their first four bytes, and so does this.
[[nodiscard]] bool verbatim_magic(std::span<const std::uint8_t> head);

// The archive's directory: where each entry starts, and the exact size of the
// entries whose size is not a whole number of blocks.
struct Directory {
    std::vector<std::uint32_t> blocks;  // entries + 1: the last is the archive's size in blocks
    std::vector<std::pair<std::uint32_t, std::uint32_t>> sizes;  // (entry, bytes), by entry
    std::uint32_t directory_blocks{};   // blocks the directory itself takes; blocks[0]
    std::vector<std::uint8_t> trailer;  // encrypted bytes after the tables, kept as they are

    [[nodiscard]] std::size_t entries() const { return blocks.empty() ? 0u : blocks.size() - 1u; }
    [[nodiscard]] std::uint64_t span(std::uint32_t entry) const {
        return static_cast<std::uint64_t>(blocks[entry + 1u] - blocks[entry]) * kBlock;
    }
    [[nodiscard]] std::uint64_t size(std::uint32_t entry) const;
    [[nodiscard]] bool has_exact_size(std::uint32_t entry) const;
    [[nodiscard]] std::uint64_t archive_bytes() const {
        return blocks.empty() ? 0u : static_cast<std::uint64_t>(blocks.back()) * kBlock;
    }
    // The entry that holds archive block `block`, or -1 for the directory.
    [[nodiscard]] std::int64_t entry_at(std::uint32_t block) const;

    // Parses the archive's first `directory_blocks` blocks as stored.
    [[nodiscard]] static std::optional<Directory> parse(std::span<const std::uint8_t> encrypted,
                                                        std::uint64_t archive_bytes);
    // The directory as stored: encrypted tables followed by the trailer.
    [[nodiscard]] std::vector<std::uint8_t> encode() const;
};

// The directory with the mods' files in it. An entry keeps its place unless
// a mod's file does not fit in its blocks; it then grows, and every entry
// after it moves up by as many blocks. A size the size table cannot express
// (a file that is not a whole number of blocks where the disc's was) is
// padded with zeros to its blocks.
struct Layout {
    Directory directory;
    std::map<FileId, std::uint64_t> padded;  // entries served with zero padding: the file's own size

    [[nodiscard]] static Layout build(const Directory &disc, const std::map<FileId, std::uint64_t> &sizes);
    [[nodiscard]] bool same_as(const Layout &other) const {
        return directory.blocks == other.directory.blocks && directory.sizes == other.directory.sizes;
    }
    // Whether entries moved: reads of the files after the first grown one are
    // then re-keyed from the disc's blocks to the new ones.
    [[nodiscard]] bool moved(const Directory &disc) const { return directory.blocks != disc.blocks; }
};

// Reads the archive as the game should see it: the directory of the layout,
// the mods' files encrypted for their blocks, the other entries from the disc,
// re-keyed where they moved.
class ArchiveView {
public:
    using RawRead = std::function<std::size_t(std::uint64_t offset, std::span<std::uint8_t> out)>;

    ArchiveView(const Directory &disc, RawRead raw) : disc_(&disc), raw_(std::move(raw)) {}

    void set(std::shared_ptr<const Layout> layout, FileOverlay *overlay);
    [[nodiscard]] const Layout *layout() const noexcept { return layout_.get(); }
    [[nodiscard]] std::uint64_t size() const;

    // Reads at an offset into the archive as the game sees it. Returns the
    // bytes read. `touched` gets each entry the read served from a mod.
    std::size_t read(std::uint64_t offset, std::span<std::uint8_t> out, std::vector<FileId> *touched = nullptr);

    // The game's own bytes of an entry, decrypted.
    [[nodiscard]] std::vector<std::uint8_t> original(FileId entry);

private:
    struct Encoded {
        std::shared_ptr<const FileContent> content;
        std::vector<std::uint8_t> bytes;  // encrypted for the entry's block, padded to its span
        bool verbatim{};
    };
    const Encoded &encoded(FileId entry);
    [[nodiscard]] bool disc_verbatim(FileId entry);

    const Directory *disc_;
    RawRead raw_;
    std::shared_ptr<const Layout> layout_;
    FileOverlay *overlay_{};
    std::vector<std::uint8_t> directory_bytes_;
    std::map<FileId, Encoded> encoded_;
    std::map<FileId, bool> verbatim_;
};

} // namespace mhp3rd::mods::p3rd
