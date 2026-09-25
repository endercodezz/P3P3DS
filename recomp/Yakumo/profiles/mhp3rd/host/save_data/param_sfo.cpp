#include "save_data/param_sfo.hpp"

#include <algorithm>
#include <cstring>

namespace mhp3rd::savedata {
namespace {

constexpr std::uint32_t kMagic = 0x46535000u;  // "\0PSF"
constexpr std::uint32_t kVersion = 0x00000101u;
constexpr std::size_t kHeaderSize = 20u;
constexpr std::size_t kIndexEntrySize = 16u;

std::uint32_t read32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
}

std::uint16_t read16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset] | (bytes[offset + 1] << 8u));
}

void write32(std::vector<std::uint8_t> &out, std::size_t offset, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
}

void write16(std::vector<std::uint8_t> &out, std::size_t offset, std::uint16_t value) {
    out[offset] = static_cast<std::uint8_t>(value);
    out[offset + 1] = static_cast<std::uint8_t>(value >> 8u);
}

std::size_t align4(std::size_t value) { return (value + 3u) & ~std::size_t{3u}; }

} // namespace

std::optional<ParamSfo> ParamSfo::parse(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < kHeaderSize || read32(bytes, 0) != kMagic) return std::nullopt;
    const std::uint32_t key_table = read32(bytes, 8);
    const std::uint32_t data_table = read32(bytes, 12);
    const std::uint32_t count = read32(bytes, 16);
    if (kHeaderSize + static_cast<std::size_t>(count) * kIndexEntrySize > bytes.size()) return std::nullopt;
    ParamSfo sfo;
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::size_t index = kHeaderSize + static_cast<std::size_t>(i) * kIndexEntrySize;
        const std::size_t key_offset = key_table + read16(bytes, index);
        Entry entry;
        entry.format = read16(bytes, index + 2);
        const std::uint32_t length = read32(bytes, index + 4);
        entry.max_length = read32(bytes, index + 8);
        const std::size_t data_offset = static_cast<std::size_t>(data_table) + read32(bytes, index + 12);
        if (key_offset >= bytes.size() || data_offset + length > bytes.size() || length > entry.max_length)
            return std::nullopt;
        std::string key;
        for (std::size_t p = key_offset; p < bytes.size() && bytes[p] != 0u; ++p) key.push_back(static_cast<char>(bytes[p]));
        entry.data.assign(bytes.begin() + static_cast<std::ptrdiff_t>(data_offset),
                          bytes.begin() + static_cast<std::ptrdiff_t>(data_offset + length));
        sfo.entries_[key] = std::move(entry);
    }
    return sfo;
}

std::vector<std::uint8_t> ParamSfo::serialize() const {
    std::size_t key_bytes = 0u;
    std::size_t data_bytes = 0u;
    for (const auto &[key, entry] : entries_) {
        key_bytes += key.size() + 1u;
        data_bytes += align4(entry.max_length);
    }
    const std::size_t key_table = kHeaderSize + entries_.size() * kIndexEntrySize;
    const std::size_t data_table = key_table + align4(key_bytes);
    std::vector<std::uint8_t> out(data_table + data_bytes, 0u);
    write32(out, 0, kMagic);
    write32(out, 4, kVersion);
    write32(out, 8, static_cast<std::uint32_t>(key_table));
    write32(out, 12, static_cast<std::uint32_t>(data_table));
    write32(out, 16, static_cast<std::uint32_t>(entries_.size()));
    std::size_t index = kHeaderSize;
    std::size_t key_offset = 0u;
    std::size_t data_offset = 0u;
    for (const auto &[key, entry] : entries_) {
        write16(out, index, static_cast<std::uint16_t>(key_offset));
        write16(out, index + 2, entry.format);
        write32(out, index + 4, static_cast<std::uint32_t>(entry.data.size()));
        write32(out, index + 8, entry.max_length);
        write32(out, index + 12, static_cast<std::uint32_t>(data_offset));
        std::memcpy(&out[key_table + key_offset], key.data(), key.size());
        std::copy(entry.data.begin(), entry.data.end(), out.begin() + static_cast<std::ptrdiff_t>(data_table + data_offset));
        key_offset += key.size() + 1u;
        data_offset += align4(entry.max_length);
        index += kIndexEntrySize;
    }
    return out;
}

std::optional<std::size_t> ParamSfo::data_offset(const std::string &key) const {
    std::size_t key_bytes = 0u;
    for (const auto &[name, entry] : entries_) key_bytes += name.size() + 1u;
    std::size_t offset = kHeaderSize + entries_.size() * kIndexEntrySize + align4(key_bytes);
    for (const auto &[name, entry] : entries_) {
        if (name == key) return offset;
        offset += align4(entry.max_length);
    }
    return std::nullopt;
}

void ParamSfo::set_string(const std::string &key, const std::string &value, std::uint32_t max_length) {
    Entry entry;
    entry.format = kString;
    entry.max_length = max_length;
    const std::size_t used = std::min<std::size_t>(value.size(), max_length - 1u);
    entry.data.assign(value.begin(), value.begin() + static_cast<std::ptrdiff_t>(used));
    entry.data.push_back(0u);
    entries_[key] = std::move(entry);
}

void ParamSfo::set_integer(const std::string &key, std::uint32_t value) {
    Entry entry;
    entry.format = kInteger;
    entry.max_length = 4u;
    entry.data = {static_cast<std::uint8_t>(value), static_cast<std::uint8_t>(value >> 8u),
                  static_cast<std::uint8_t>(value >> 16u), static_cast<std::uint8_t>(value >> 24u)};
    entries_[key] = std::move(entry);
}

void ParamSfo::set_binary(const std::string &key, std::vector<std::uint8_t> value, std::uint32_t max_length) {
    Entry entry;
    entry.format = kBinary;
    entry.max_length = max_length;
    value.resize(std::min<std::size_t>(value.size(), max_length));
    entry.data = std::move(value);
    entries_[key] = std::move(entry);
}

std::optional<std::string> ParamSfo::string(const std::string &key) const {
    const auto found = entries_.find(key);
    if (found == entries_.end() || found->second.format != kString) return std::nullopt;
    std::string text(found->second.data.begin(), found->second.data.end());
    if (const auto nul = text.find('\0'); nul != std::string::npos) text.resize(nul);
    return text;
}

std::optional<std::uint32_t> ParamSfo::integer(const std::string &key) const {
    const auto found = entries_.find(key);
    if (found == entries_.end() || found->second.format != kInteger || found->second.data.size() < 4u) return std::nullopt;
    return read32(found->second.data, 0);
}

const std::vector<std::uint8_t> *ParamSfo::binary(const std::string &key) const {
    const auto found = entries_.find(key);
    if (found == entries_.end() || found->second.format != kBinary) return nullptr;
    return &found->second.data;
}

} // namespace mhp3rd::savedata
