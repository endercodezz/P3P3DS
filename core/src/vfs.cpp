#include "p3p3ds/vfs.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>

namespace p3p3ds::vfs {
namespace {
constexpr std::uint32_t kSector = 2048u;

std::string upper(std::string_view text) {
    std::string out(text);
    for (auto &c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}

std::uint32_t le32(const std::uint8_t *p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8u) |
           (static_cast<std::uint32_t>(p[2]) << 16u) | (static_cast<std::uint32_t>(p[3]) << 24u);
}

class IsoSource final : public Source {
public:
    IsoSource(std::shared_ptr<std::ifstream> stream, std::shared_ptr<std::mutex> lock, std::uint64_t offset, std::uint64_t size)
        : stream_(std::move(stream)), lock_(std::move(lock)), offset_(offset), size_(size) {}
    [[nodiscard]] std::uint64_t size() const override { return size_; }
    std::size_t read(std::uint64_t offset, void *destination, std::size_t length) const override {
        if (offset >= size_) return 0u;
        length = static_cast<std::size_t>(std::min<std::uint64_t>(length, size_ - offset));
        std::lock_guard guard(*lock_);
        stream_->clear();
        stream_->seekg(static_cast<std::streamoff>(offset_ + offset));
        stream_->read(static_cast<char *>(destination), static_cast<std::streamsize>(length));
        return static_cast<std::size_t>(stream_->gcount());
    }

private:
    std::shared_ptr<std::ifstream> stream_;
    std::shared_ptr<std::mutex> lock_;
    std::uint64_t offset_, size_;
};

class HostSource final : public Source {
public:
    explicit HostSource(const std::filesystem::path &path)
        : stream_(path, std::ios::binary), size_(std::filesystem::file_size(path)) {}
    [[nodiscard]] std::uint64_t size() const override { return size_; }
    std::size_t read(std::uint64_t offset, void *destination, std::size_t length) const override {
        if (offset >= size_) return 0u;
        length = static_cast<std::size_t>(std::min<std::uint64_t>(length, size_ - offset));
        std::lock_guard guard(lock_);
        stream_.clear();
        stream_.seekg(static_cast<std::streamoff>(offset));
        stream_.read(static_cast<char *>(destination), static_cast<std::streamsize>(length));
        return static_cast<std::size_t>(stream_.gcount());
    }

private:
    mutable std::ifstream stream_;
    mutable std::mutex lock_;
    std::uint64_t size_;
};
} // namespace

std::optional<std::string> normalize(std::string_view path) {
    std::vector<std::string> parts;
    std::string current;
    auto flush = [&]() -> bool {
        if (current.empty() || current == ".") { current.clear(); return true; }
        if (current == "..") {
            if (parts.empty()) return false;
            parts.pop_back();
        } else {
            parts.push_back(current);
        }
        current.clear();
        return true;
    };
    for (const char c : path) {
        if (c == '/' || c == '\\') { if (!flush()) return std::nullopt; }
        else current += c;
    }
    if (!flush()) return std::nullopt;
    std::string out;
    for (const auto &part : parts) { if (!out.empty()) out += '/'; out += part; }
    return out;
}

std::size_t MemorySource::read(std::uint64_t offset, void *destination, std::size_t length) const {
    if (offset >= bytes_.size()) return 0u;
    length = static_cast<std::size_t>(std::min<std::uint64_t>(length, bytes_.size() - offset));
    std::memcpy(destination, bytes_.data() + offset, length);
    return length;
}

// ---------------------------------------------------------------------------
// ISO9660: primary volume descriptor at sector 16, root record at byte 156;
// directory records: +2 extent LBA, +10 data length, +25 flags (bit 1 dir),
// +32 name length, +33 name (";1" version suffix stripped).
// ---------------------------------------------------------------------------

IsoFileSystem::IsoFileSystem(std::filesystem::path image)
    : stream_(std::make_shared<std::ifstream>(image, std::ios::binary)), lock_(std::make_shared<std::mutex>()) {
    if (!*stream_) throw std::runtime_error("cannot open UMD image: " + image.string());
    std::uint8_t pvd[kSector];
    stream_->seekg(16 * kSector);
    stream_->read(reinterpret_cast<char *>(pvd), kSector);
    if (stream_->gcount() != kSector || std::memcmp(pvd + 1, "CD001", 5) != 0)
        throw std::runtime_error("not an ISO9660 image: " + image.string());
    const std::uint8_t *root = pvd + 156;
    nodes_[""] = Node{"", true, le32(root + 2), le32(root + 10), {}};
    load_directory("", le32(root + 2), le32(root + 10), 0);
}

void IsoFileSystem::load_directory(const std::string &key, std::uint32_t lba, std::uint32_t size, int depth) {
    if (depth > 32) return;
    std::vector<std::uint8_t> data(size);
    stream_->clear();
    stream_->seekg(static_cast<std::streamoff>(lba) * kSector);
    stream_->read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(size));
    std::uint32_t offset = 0;
    while (offset < data.size()) {
        const std::uint8_t length = data[offset];
        if (length == 0u) { offset = (offset / kSector + 1u) * kSector; continue; }
        if (offset + length > data.size() || length < 34u) break;
        const std::uint8_t *record = data.data() + offset;
        offset += length;
        const std::uint8_t name_length = record[32];
        if (name_length == 1u && (record[33] == 0u || record[33] == 1u)) continue;
        std::string name(reinterpret_cast<const char *>(record + 33), name_length);
        if (auto semicolon = name.find(';'); semicolon != std::string::npos) name.resize(semicolon);
        const bool directory = (record[25] & 2u) != 0u;
        const std::string child = key.empty() ? upper(name) : key + "/" + upper(name);
        nodes_[key].children.push_back(child);
        nodes_[child] = Node{name, directory, le32(record + 2), le32(record + 10), {}};
        if (directory) load_directory(child, le32(record + 2), le32(record + 10), depth + 1);
        else ++files_;
    }
}

std::shared_ptr<const Source> IsoFileSystem::open(std::string_view path) const {
    const auto it = nodes_.find(upper(path));
    if (it == nodes_.end() || it->second.directory) return nullptr;
    return std::make_shared<IsoSource>(stream_, lock_, static_cast<std::uint64_t>(it->second.lba) * kSector, it->second.size);
}

std::optional<Entry> IsoFileSystem::stat(std::string_view path) const {
    const auto it = nodes_.find(upper(path));
    if (it == nodes_.end()) return std::nullopt;
    return Entry{it->second.name, it->second.directory, it->second.directory ? 0u : it->second.size};
}

std::optional<std::vector<Entry>> IsoFileSystem::list(std::string_view path) const {
    const auto it = nodes_.find(upper(path));
    if (it == nodes_.end() || !it->second.directory) return std::nullopt;
    std::vector<Entry> entries;
    for (const auto &child : it->second.children) {
        const auto &node = nodes_.at(child);
        entries.push_back(Entry{node.name, node.directory, node.directory ? 0u : node.size});
    }
    return entries;
}

// ---------------------------------------------------------------------------
// Host directory
// ---------------------------------------------------------------------------

HostFileSystem::HostFileSystem(std::filesystem::path root) : root_(std::move(root)) {}

std::optional<std::filesystem::path> HostFileSystem::resolve(std::string_view path) const {
    const auto clean = normalize(path);
    if (!clean) return std::nullopt;
    // Case-insensitive component match, as on the PSP.
    std::filesystem::path current = root_;
    std::size_t start = 0;
    while (start < clean->size()) {
        const auto end = clean->find('/', start);
        const auto part = clean->substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::error_code ec;
        if (std::filesystem::exists(current / part, ec)) {
            current /= part;
        } else {
            bool found = false;
            if (std::filesystem::is_directory(current, ec)) {
                for (const auto &entry : std::filesystem::directory_iterator(current, ec)) {
                    if (upper(entry.path().filename().string()) == upper(part)) { current = entry.path(); found = true; break; }
                }
            }
            if (!found) return std::nullopt;
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return current;
}

std::shared_ptr<const Source> HostFileSystem::open(std::string_view path) const {
    const auto resolved = resolve(path);
    std::error_code ec;
    if (!resolved || !std::filesystem::is_regular_file(*resolved, ec)) return nullptr;
    return std::make_shared<HostSource>(*resolved);
}

std::optional<Entry> HostFileSystem::stat(std::string_view path) const {
    const auto resolved = resolve(path);
    std::error_code ec;
    if (!resolved || !std::filesystem::exists(*resolved, ec)) return std::nullopt;
    const bool directory = std::filesystem::is_directory(*resolved, ec);
    return Entry{resolved->filename().string(), directory, directory ? 0u : std::filesystem::file_size(*resolved, ec)};
}

std::optional<std::vector<Entry>> HostFileSystem::list(std::string_view path) const {
    const auto resolved = resolve(path);
    std::error_code ec;
    if (!resolved || !std::filesystem::is_directory(*resolved, ec)) return std::nullopt;
    std::vector<Entry> entries;
    for (const auto &entry : std::filesystem::directory_iterator(*resolved, ec)) {
        const bool directory = entry.is_directory(ec);
        entries.push_back(Entry{entry.path().filename().string(), directory, directory ? 0u : entry.file_size(ec)});
    }
    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) { return a.name < b.name; });
    return entries;
}

} // namespace p3p3ds::vfs
