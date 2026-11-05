// See texture_pack.hpp.
#include "p3p3ds/ge/texture_pack.hpp"
#include "p3p3ds/ge/geometry.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>

namespace p3p3ds::ge {
namespace {

constexpr char kMagic[8] = {'P', '3', 'T', 'X', 'P', 'A', 'C', 'K'};
constexpr std::uint32_t kVersion = 1;
constexpr std::size_t kHeaderSize = 24, kEntrySize = 32;

std::uint16_t le16(const std::uint8_t *p) { return static_cast<std::uint16_t>(p[0] | p[1] << 8); }
std::uint32_t le32(const std::uint8_t *p) { return static_cast<std::uint32_t>(le16(p)) | static_cast<std::uint32_t>(le16(p + 2)) << 16; }
std::uint64_t le64(const std::uint8_t *p) { return static_cast<std::uint64_t>(le32(p)) | static_cast<std::uint64_t>(le32(p + 4)) << 32; }

bool key_less(const PackEntry &e, const ReplacementKey &k) {
    return e.key.cachekey < k.cachekey || (e.key.cachekey == k.cachekey && e.key.hash < k.hash);
}

std::uint32_t bytes_per_texel(std::uint8_t format) {
    switch (format) {
    case 0: return 4;
    case 1: return 3;
    case 2: case 3: case 4: return 2;
    default: return 0;
    }
}

// Same tiling as platform/3ds/gpu_renderer.cpp tiled_index.
std::uint32_t tiled_index(std::uint32_t x, std::uint32_t y, std::uint32_t w) {
    const std::uint32_t m = (x & 1u) | ((y & 1u) << 1) | ((x & 2u) << 1) | ((y & 2u) << 2) | ((x & 4u) << 2) | ((y & 4u) << 3);
    return ((y >> 3) * (w >> 3) + (x >> 3)) * 64u + m;
}

} // namespace

std::unique_ptr<TexturePack> TexturePack::open(const std::filesystem::path &dir, std::string &error) {
    std::ifstream in(dir / "index.bin", std::ios::binary);
    if (!in) {
        error = "cannot open " + (dir / "index.bin").string();
        return nullptr;
    }
    const std::vector<std::uint8_t> index((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (index.size() < kHeaderSize || std::memcmp(index.data(), kMagic, 8) != 0) {
        error = "index.bin is not a P3P3DS texture pack";
        return nullptr;
    }
    if (le32(&index[8]) != kVersion) {
        error = "unsupported texture pack version " + std::to_string(le32(&index[8]));
        return nullptr;
    }
    const std::uint32_t count = le32(&index[16]);
    if (index.size() != kHeaderSize + static_cast<std::size_t>(count) * kEntrySize) {
        error = "index.bin size does not match its entry count";
        return nullptr;
    }
    std::unique_ptr<TexturePack> pack(new TexturePack());
    pack->ignore_address_ = (le32(&index[12]) & 1u) != 0u;
    pack->entries_.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::uint8_t *p = &index[kHeaderSize + i * kEntrySize];
        PackEntry e;
        e.key = {le64(p), le32(p + 8)};
        e.offset = le32(p + 12);
        e.size = le32(p + 16);
        e.width = le16(p + 20);
        e.height = le16(p + 22);
        e.format = p[24];
        if (e.width != 0) {
            const bool pow2 = (e.width & (e.width - 1u)) == 0u && (e.height & (e.height - 1u)) == 0u;
            if (!pow2 || e.width < 8u || e.height < 8u || e.width > 1024u || e.height > 1024u || bytes_per_texel(e.format) == 0 ||
                e.size != static_cast<std::uint32_t>(e.width) * e.height * bytes_per_texel(e.format)) {
                error = "invalid entry " + std::to_string(i) + " in index.bin";
                return nullptr;
            }
        }
        if (!pack->entries_.empty() && !key_less(pack->entries_.back(), e.key)) {
            error = "index.bin entries are not sorted";
            return nullptr;
        }
        pack->entries_.push_back(e);
    }
    pack->data_ = std::fopen((dir / "textures.bin").string().c_str(), "rb");
    if (!pack->data_) {
        error = "cannot open " + (dir / "textures.bin").string();
        return nullptr;
    }
    return pack;
}

TexturePack::~TexturePack() {
    if (data_) std::fclose(data_);
}

const PackEntry *TexturePack::exact(const ReplacementKey &key) const {
    const auto it = std::lower_bound(entries_.begin(), entries_.end(), key, key_less);
    return it != entries_.end() && it->key == key ? &*it : nullptr;
}

const PackEntry *TexturePack::find(const ReplacementKey &key) const {
    const auto found = find_replacement([this](const ReplacementKey &k) { return exact(k) != nullptr; }, key, ignore_address_);
    return found ? exact(*found) : nullptr;
}

bool TexturePack::read(const PackEntry &entry, std::vector<std::uint8_t> &out) const {
    if (entry.width == 0) return false;
    out.resize(entry.size);
    return std::fseek(data_, static_cast<long>(entry.offset), SEEK_SET) == 0 && std::fread(out.data(), 1, out.size(), data_) == out.size();
}

bool TexturePack::read_into(const PackEntry &entry, void *out) const {
    if (entry.width == 0) return false;
    return std::fseek(data_, static_cast<long>(entry.offset), SEEK_SET) == 0 && std::fread(out, 1, entry.size, data_) == entry.size;
}

const PackEntry *TextureReplacer::find(psprecomp::GuestMemory &memory, const GeRegisters &regs, bool through, std::uint16_t max_v) {
    // The CLUT cache changes only on LOADCLUT, which updates clut_hash.
    if (regs.clut_hash != clut_seen_) {
        clut_seen_ = regs.clut_hash;
        clut_.load(reinterpret_cast<const std::uint8_t *>(regs.clut.data()), regs.clut_words * 4u);
    }
    const PackEntry *entry = pack_->find(keys_.key(memory, regs, clut_, through, max_v));
    return entry && entry->width != 0 ? entry : nullptr;
}

std::uint16_t through_max_v(psprecomp::GuestMemory &memory, const GeRegisters &regs, std::uint32_t count, std::uint32_t vertex_address,
                            std::uint32_t index_address) {
    const VertexLayout layout = vertex_layout(regs.reg[0x12] & 0xFFFFFFu);
    if (!layout.through) return 0;
    std::uint16_t max_v = 0;
    ModelVertex v;
    for (std::uint32_t i = 0; i < count; ++i)
        if (decode_model_vertex(memory, regs, layout, vertex_address, vertex_index(memory, layout, index_address, i), v))
            max_v = std::max<std::uint16_t>(max_v, static_cast<std::uint16_t>(std::clamp(v.uv[1], 0.0f, 65535.0f)));
    return max_v;
}

void decode_pack_texture(const PackEntry &entry, const std::vector<std::uint8_t> &data, std::vector<std::uint32_t> &out) {
    const std::uint32_t w = entry.width, h = entry.height, bpp = bytes_per_texel(entry.format);
    out.assign(static_cast<std::size_t>(w) * h, 0u);
    if (bpp == 0 || data.size() < static_cast<std::size_t>(w) * h * bpp) return;
    auto expand = [](std::uint32_t v, int bits) { return (v << (8 - bits)) | (v >> (2 * bits - 8 > 0 ? 2 * bits - 8 : 0)); };
    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) {
            const std::uint8_t *t = &data[static_cast<std::size_t>(tiled_index(x, h - 1u - y, w)) * bpp];
            std::uint32_t r, g, b, a = 255;
            switch (entry.format) {
            case 0: a = t[0]; b = t[1]; g = t[2]; r = t[3]; break;
            case 1: b = t[0]; g = t[1]; r = t[2]; break;
            default: {
                const std::uint32_t v = le16(t);
                if (entry.format == 3) {
                    r = expand(v >> 11, 5); g = expand((v >> 5) & 63u, 6); b = expand(v & 31u, 5);
                } else if (entry.format == 2) {
                    r = expand(v >> 11, 5); g = expand((v >> 6) & 31u, 5); b = expand((v >> 1) & 31u, 5); a = (v & 1u) ? 255u : 0u;
                } else {
                    r = (v >> 12) * 17u; g = ((v >> 8) & 15u) * 17u; b = ((v >> 4) & 15u) * 17u; a = (v & 15u) * 17u;
                }
            }
            }
            out[static_cast<std::size_t>(y) * w + x] = r | g << 8 | b << 16 | a << 24;
        }
    }
}

} // namespace p3p3ds::ge
