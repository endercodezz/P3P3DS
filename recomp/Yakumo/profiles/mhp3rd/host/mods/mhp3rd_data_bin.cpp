#include "mods/mhp3rd_data_bin.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace mhp3rd::mods::p3rd {
namespace {

// The byte substitution applied after the XOR step when encrypting, as
// recovered for profiles/mhp3rd/tools/databin.py (docs/DATA_BIN.md).
constexpr std::array<std::uint8_t, 256> kEncode = {
    0xc0, 0xa8, 0xca, 0x07, 0x4b, 0x6e, 0x48, 0x6f, 0xd6, 0x92, 0x31, 0x2c, 0x9d, 0xfb, 0xe1, 0x50,
    0x61, 0xc6, 0xe4, 0x52, 0x3e, 0x12, 0xad, 0x33, 0xae, 0xeb, 0xf3, 0x2f, 0x6b, 0x69, 0x7b, 0x53,
    0x96, 0xc4, 0xb1, 0x9c, 0x1c, 0xc5, 0x20, 0x86, 0x19, 0x13, 0xe9, 0x6a, 0x26, 0x75, 0x78, 0x8c,
    0x43, 0xed, 0x7a, 0x66, 0x5d, 0x18, 0x1d, 0xe8, 0x70, 0xa5, 0x5e, 0xf2, 0x5f, 0x58, 0x05, 0x46,
    0x0d, 0x97, 0x9e, 0x7c, 0xea, 0x65, 0xdd, 0x24, 0x8f, 0x49, 0x42, 0xaf, 0xf4, 0x25, 0xb8, 0x2b,
    0x08, 0x72, 0x17, 0xd9, 0xa4, 0xd3, 0x93, 0x71, 0x5b, 0x40, 0xb2, 0x2e, 0x0b, 0x7e, 0x4c, 0x04,
    0xf7, 0x11, 0xc1, 0x37, 0x79, 0xa7, 0x29, 0xbc, 0x1b, 0x56, 0x8b, 0xfa, 0x8d, 0x36, 0x3b, 0x6d,
    0xd4, 0x57, 0x83, 0xbd, 0x1f, 0xd7, 0x62, 0x84, 0xf5, 0xda, 0xd5, 0xab, 0xcc, 0xa2, 0x47, 0x88,
    0x9a, 0x2d, 0xc7, 0xdf, 0xcb, 0x02, 0x28, 0x41, 0xa9, 0x3d, 0xd8, 0xa1, 0x23, 0x3c, 0x81, 0x6c,
    0x5c, 0xd0, 0x68, 0xc9, 0xbf, 0x99, 0x01, 0xbe, 0xf9, 0xfc, 0xec, 0xb7, 0x0a, 0x82, 0x89, 0xdc,
    0x91, 0xef, 0x14, 0xcf, 0x34, 0x4a, 0x03, 0xd1, 0xba, 0x35, 0x8a, 0x06, 0xff, 0x38, 0xa0, 0xf0,
    0xce, 0x7d, 0x0c, 0x76, 0xc2, 0xb3, 0xac, 0x09, 0x94, 0x55, 0x54, 0x80, 0xa3, 0x95, 0xbb, 0xa6,
    0x30, 0x2a, 0xf6, 0x67, 0x1e, 0xfe, 0x77, 0x63, 0x64, 0x87, 0x60, 0x00, 0xb0, 0x98, 0x44, 0xee,
    0x4d, 0xe5, 0xc3, 0xcd, 0x51, 0x22, 0x73, 0x9b, 0xe0, 0x1a, 0x74, 0xc8, 0x5a, 0x3f, 0x4e, 0xe6,
    0xaa, 0x7f, 0x21, 0xf1, 0x59, 0x9f, 0xb9, 0x90, 0x4f, 0xe2, 0xfd, 0xb4, 0x16, 0xe3, 0xf8, 0x0e,
    0xe7, 0x15, 0x85, 0x39, 0x3a, 0xde, 0x0f, 0xd2, 0xb6, 0x8e, 0x27, 0xdb, 0xb5, 0x32, 0x45, 0x10,
};

constexpr std::array<std::uint8_t, 256> make_decode() {
    std::array<std::uint8_t, 256> table{};
    for (std::size_t plain = 0; plain < 256u; ++plain) table[kEncode[plain]] = static_cast<std::uint8_t>(plain);
    return table;
}
constexpr std::array<std::uint8_t, 256> kDecode = make_decode();

constexpr std::uint32_t kMultiplier[2] = {0x2345u, 0x7F8Du};
constexpr std::uint32_t kModulus[2] = {0xFFD9u, 0xFFF1u};
constexpr std::uint32_t kDefaultSeed[2] = {0x2345u, 0x7F8Du};

std::uint64_t power_mod(std::uint64_t base, std::uint64_t exponent, std::uint64_t modulus) {
    std::uint64_t result = 1u % modulus;
    base %= modulus;
    while (exponent > 0u) {
        if ((exponent & 1u) != 0u) result = result * base % modulus;
        base = base * base % modulus;
        exponent >>= 1u;
    }
    return result;
}

// Two 16-bit Lehmer generators, one per halfword of each little-endian word,
// each stepped before use. Started at any word of an entry.
class Keystream {
public:
    Keystream(std::uint32_t block, std::uint64_t word) {
        const std::uint32_t seeds[2] = {block >> 16u, block & 0xFFFFu};
        for (int i = 0; i < 2; ++i) {
            const std::uint64_t seed = seeds[i] != 0u ? seeds[i] : kDefaultSeed[i];
            state_[i] = seed * power_mod(kMultiplier[i], word, kModulus[i]) % kModulus[i];
        }
    }
    std::uint32_t next() {
        for (int i = 0; i < 2; ++i) state_[i] = state_[i] * kMultiplier[i] % kModulus[i];
        return static_cast<std::uint32_t>(state_[0] << 16u | state_[1]);
    }

private:
    std::uint64_t state_[2]{};
};

// Calls fn(byte, key byte) over data, whose first byte is at `offset` into
// an entry keyed by `block`.
template <typename Fn>
void each_byte(std::span<std::uint8_t> data, std::uint32_t block, std::uint64_t offset, Fn fn) {
    if (data.empty()) return;
    Keystream keys(block, offset / 4u);
    unsigned lane = static_cast<unsigned>(offset % 4u);
    std::uint32_t key = keys.next();
    for (std::uint8_t &byte : data) {
        byte = fn(byte, static_cast<std::uint8_t>(key >> (8u * lane)));
        if (++lane == 4u) {
            lane = 0u;
            key = keys.next();
        }
    }
}

std::uint32_t load32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) | static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u |
           static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u | static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u;
}

void store32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned i = 0; i < 4u; ++i) bytes[offset + i] = static_cast<std::uint8_t>(value >> (8u * i));
}

} // namespace

void encrypt(std::span<std::uint8_t> data, std::uint32_t block, std::uint64_t offset) {
    each_byte(data, block, offset, [](std::uint8_t plain, std::uint8_t key) { return kEncode[plain ^ key]; });
}

void decrypt(std::span<std::uint8_t> data, std::uint32_t block, std::uint64_t offset) {
    each_byte(data, block, offset, [](std::uint8_t stored, std::uint8_t key) {
        return static_cast<std::uint8_t>(kDecode[stored] ^ key);
    });
}

void transcode(std::span<std::uint8_t> data, std::uint32_t from_block, std::uint32_t to_block, std::uint64_t offset) {
    if (from_block == to_block || data.empty()) return;
    Keystream from(from_block, offset / 4u);
    Keystream to(to_block, offset / 4u);
    unsigned lane = static_cast<unsigned>(offset % 4u);
    std::uint32_t key = from.next() ^ to.next();
    for (std::uint8_t &byte : data) {
        byte = kEncode[kDecode[byte] ^ static_cast<std::uint8_t>(key >> (8u * lane))];
        if (++lane == 4u) {
            lane = 0u;
            key = from.next() ^ to.next();
        }
    }
}

bool verbatim_magic(std::span<const std::uint8_t> head) {
    if (head.size() < 4u) return false;
    return std::memcmp(head.data(), "~SCE", 4u) == 0 || std::memcmp(head.data(), "PSMF", 4u) == 0;
}

std::uint64_t Directory::size(std::uint32_t entry) const {
    const auto found = std::lower_bound(sizes.begin(), sizes.end(), std::make_pair(entry, 0u));
    if (found != sizes.end() && found->first == entry) return found->second;
    return span(entry);
}

bool Directory::has_exact_size(std::uint32_t entry) const {
    const auto found = std::lower_bound(sizes.begin(), sizes.end(), std::make_pair(entry, 0u));
    return found != sizes.end() && found->first == entry;
}

std::int64_t Directory::entry_at(std::uint32_t block) const {
    if (blocks.empty() || block < blocks.front()) return -1;
    // The last entry whose first block is at or before `block`. Empty entries
    // share a first block with the next one and are skipped by upper_bound.
    const auto found = std::upper_bound(blocks.begin(), blocks.end(), block);
    return static_cast<std::int64_t>(found - blocks.begin()) - 1;
}

std::optional<Directory> Directory::parse(std::span<const std::uint8_t> encrypted, std::uint64_t archive_bytes) {
    if (encrypted.size() < kBlock) return std::nullopt;
    std::vector<std::uint8_t> plain(encrypted.begin(), encrypted.end());
    decrypt(plain, 0u, 0u);
    Directory d;
    d.directory_blocks = load32(plain, 0u);
    const std::uint64_t directory_bytes = static_cast<std::uint64_t>(d.directory_blocks) * kBlock;
    if (d.directory_blocks == 0u || directory_bytes > plain.size()) return std::nullopt;
    const std::size_t words = static_cast<std::size_t>(directory_bytes / 4u);
    const auto end_block = static_cast<std::uint32_t>(archive_bytes / kBlock);
    // Block table: non-decreasing, ended by the archive's size in blocks.
    std::size_t count = 1u;
    d.blocks.push_back(d.directory_blocks);
    while (count < words) {
        const std::uint32_t value = load32(plain, count * 4u);
        if (value < d.blocks.back()) return std::nullopt;
        d.blocks.push_back(value);
        ++count;
        if (value == end_block) break;
    }
    if (d.blocks.back() != end_block) return std::nullopt;
    // Exact sizes: (entry, bytes) pairs by entry, while they make sense.
    std::size_t word = count;
    const std::size_t entries = d.blocks.size() - 1u;
    while (word + 1u < words) {
        const std::uint32_t entry = load32(plain, word * 4u);
        const std::uint32_t bytes = load32(plain, word * 4u + 4u);
        if (entry >= entries || bytes == 0u || bytes > d.span(entry)) break;
        if (!d.sizes.empty() && entry <= d.sizes.back().first) break;
        d.sizes.emplace_back(entry, bytes);
        word += 2u;
    }
    d.trailer.assign(encrypted.begin() + static_cast<std::ptrdiff_t>(word * 4u),
                     encrypted.begin() + static_cast<std::ptrdiff_t>(directory_bytes));
    return d;
}

std::vector<std::uint8_t> Directory::encode() const {
    const std::size_t tables = (blocks.size() + 2u * sizes.size()) * 4u;
    std::vector<std::uint8_t> bytes(tables, 0u);
    std::size_t at = 0u;
    for (const std::uint32_t block : blocks) {
        store32(bytes, at, block);
        at += 4u;
    }
    for (const auto &[entry, size] : sizes) {
        store32(bytes, at, entry);
        store32(bytes, at + 4u, size);
        at += 8u;
    }
    encrypt(bytes, 0u, 0u);
    bytes.insert(bytes.end(), trailer.begin(), trailer.end());
    return bytes;
}

Layout Layout::build(const Directory &disc, const std::map<FileId, std::uint64_t> &sizes) {
    Layout layout;
    layout.directory = disc;
    std::uint32_t shift = 0u;
    for (std::uint32_t entry = 0; entry < disc.entries(); ++entry) {
        layout.directory.blocks[entry] = disc.blocks[entry] + shift;
        const auto found = sizes.find(entry);
        if (found == sizes.end()) continue;
        const std::uint64_t span_blocks = disc.blocks[entry + 1u] - disc.blocks[entry];
        const std::uint64_t needed = (found->second + kBlock - 1u) / kBlock;
        if (needed > span_blocks) shift += static_cast<std::uint32_t>(needed - span_blocks);
    }
    layout.directory.blocks.back() = disc.blocks.back() + shift;
    // The game reads a fixed number of size rows, so an entry keeps or lacks
    // its row as on the disc.
    for (auto &[entry, bytes] : layout.directory.sizes) {
        if (const auto found = sizes.find(entry); found != sizes.end())
            bytes = static_cast<std::uint32_t>(found->second);
    }
    for (const auto &[entry, bytes] : sizes) {
        if (entry >= disc.entries() || layout.directory.has_exact_size(entry)) continue;
        if (bytes != layout.directory.span(entry)) layout.padded[entry] = bytes;
    }
    return layout;
}

void ArchiveView::set(std::shared_ptr<const Layout> layout, FileOverlay *overlay) {
    layout_ = std::move(layout);
    overlay_ = overlay;
    encoded_.clear();
    directory_bytes_ = layout_ ? layout_->directory.encode() : std::vector<std::uint8_t>{};
}

std::uint64_t ArchiveView::size() const {
    return layout_ ? layout_->directory.archive_bytes() : disc_->archive_bytes();
}

bool ArchiveView::disc_verbatim(FileId entry) {
    if (const auto found = verbatim_.find(entry); found != verbatim_.end()) return found->second;
    std::array<std::uint8_t, 4> head{};
    const bool verbatim = raw_(static_cast<std::uint64_t>(disc_->blocks[entry]) * kBlock, head) == head.size() &&
                          verbatim_magic(head);
    verbatim_[entry] = verbatim;
    return verbatim;
}

std::vector<std::uint8_t> ArchiveView::original(FileId entry) {
    if (entry >= disc_->entries()) return {};
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(disc_->size(entry)));
    bytes.resize(raw_(static_cast<std::uint64_t>(disc_->blocks[entry]) * kBlock, bytes));
    if (!verbatim_magic(bytes)) decrypt(bytes, disc_->blocks[entry], 0u);
    return bytes;
}

const ArchiveView::Encoded &ArchiveView::encoded(FileId entry) {
    if (const auto found = encoded_.find(entry); found != encoded_.end()) return found->second;
    Encoded encoded;
    encoded.content = overlay_ != nullptr ? overlay_->content(entry) : nullptr;
    if (encoded.content) {
        const Directory &d = layout_->directory;
        encoded.bytes.assign(static_cast<std::size_t>(d.span(entry)), 0u);
        const std::size_t count = std::min(encoded.bytes.size(), encoded.content->bytes.size());
        std::copy_n(encoded.content->bytes.begin(), count, encoded.bytes.begin());
        encoded.verbatim = verbatim_magic(encoded.bytes);
        if (!encoded.verbatim) encrypt(encoded.bytes, d.blocks[entry], 0u);
    }
    return encoded_.emplace(entry, std::move(encoded)).first->second;
}

std::size_t ArchiveView::read(std::uint64_t offset, std::span<std::uint8_t> out, std::vector<FileId> *touched) {
    if (!layout_) return raw_(offset, out);
    const Directory &d = layout_->directory;
    const std::uint64_t end = std::min<std::uint64_t>(offset + out.size(), d.archive_bytes());
    std::uint64_t at = offset;
    std::size_t done = 0u;
    while (at < end) {
        std::span<std::uint8_t> piece;
        if (at < directory_bytes_.size()) {
            const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(end, directory_bytes_.size()) - at);
            std::copy_n(directory_bytes_.begin() + static_cast<std::ptrdiff_t>(at), count, out.subspan(done).begin());
            at += count;
            done += count;
            continue;
        }
        const std::int64_t found = d.entry_at(static_cast<std::uint32_t>(at / kBlock));
        if (found < 0) {
            // Between the tables and the first entry: nothing the game reads.
            const auto count = static_cast<std::size_t>(
                std::min<std::uint64_t>(end, static_cast<std::uint64_t>(d.blocks.front()) * kBlock) - at);
            std::fill_n(out.subspan(done).begin(), count, 0u);
            at += count;
            done += count;
            continue;
        }
        const auto entry = static_cast<FileId>(found);
        const std::uint64_t start = static_cast<std::uint64_t>(d.blocks[entry]) * kBlock;
        const std::uint64_t within = at - start;
        const auto count =
            static_cast<std::size_t>(std::min<std::uint64_t>(end, start + d.span(entry)) - at);
        piece = out.subspan(done, count);
        if (overlay_ != nullptr && overlay_->touches(entry)) {
            const Encoded &e = encoded(entry);
            if (e.content) {
                std::copy_n(e.bytes.begin() + static_cast<std::ptrdiff_t>(within), count, piece.begin());
                if (touched != nullptr) touched->push_back(entry);
                at += count;
                done += count;
                continue;
            }
        }
        // An entry the mods leave alone keeps its blocks' worth of bytes.
        const std::uint64_t disc_start = static_cast<std::uint64_t>(disc_->blocks[entry]) * kBlock;
        const std::size_t got = raw_(disc_start + within, piece);
        if (got < count) std::fill(piece.begin() + static_cast<std::ptrdiff_t>(got), piece.end(), 0u);
        if (d.blocks[entry] != disc_->blocks[entry] && !disc_verbatim(entry))
            transcode(piece, disc_->blocks[entry], d.blocks[entry], within);
        at += count;
        done += count;
    }
    return done;
}

} // namespace mhp3rd::mods::p3rd
