// See texture_replacement.hpp.
#include "p3p3ds/ge/texture_replacement.hpp"
#include "p3p3ds/ge/xxhash.hpp"

#include "psprecomp/guest_memory.hpp"

#include <algorithm>
#include <cstring>

namespace p3p3ds::ge {
namespace {

constexpr std::uint32_t kClutSeed = 0xC0108888u;
constexpr std::uint64_t kDataSeed = 0xBACD7814u;

std::uint32_t r24(const GeRegisters &regs, std::uint32_t op) { return regs.reg[op] & 0xFFFFFFu; }

// Bits per texel of GE texture formats 0-10 (5650, 5551, 4444, 8888, CLUT4,
// CLUT8, CLUT16, CLUT32, DXT1, DXT3, DXT5).
constexpr std::uint32_t kBitsPerTexel[16] = {16, 16, 16, 32, 4, 8, 16, 32, 4, 8, 8, 0, 0, 0, 0, 0};

std::uint32_t buffer_width(const GeRegisters &regs, std::uint32_t format) {
    const std::uint32_t bpp = kBitsPerTexel[format];
    if (bpp == 0) return 0;
    // Buffer width aligned to 16 bytes (PPSSPP textureAlignMask16).
    const std::uint32_t mask = 0x7FFu & ~((8u * 16u) / bpp - 1u);
    std::uint32_t bufw = r24(regs, 0xA8) & mask;
    if (bufw == 0 && format <= 10) bufw = (8u * 16u) / bpp;
    return bufw;
}

} // namespace

void PpssppClut::load(const std::uint8_t *bytes, std::uint32_t size) {
    if (size == 0) return;
    size = std::min<std::uint32_t>(size, static_cast<std::uint32_t>(raw.size()));
    if (bytes) std::memcpy(raw.data(), bytes, size);
    else std::memset(raw.data(), 0, size);
    last_bytes = size;
    max_bytes = std::max(max_bytes, size);
}

std::optional<ReplacementKey> parse_replacement_key(std::string_view text) {
    if (text.size() != 24) return std::nullopt;
    ReplacementKey key;
    for (std::size_t i = 0; i < 24; ++i) {
        const char c = text[i];
        std::uint32_t v;
        if (c >= '0' && c <= '9') v = static_cast<std::uint32_t>(c - '0');
        else if (c >= 'a' && c <= 'f') v = static_cast<std::uint32_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v = static_cast<std::uint32_t>(c - 'A' + 10);
        else return std::nullopt;
        if (i < 16) key.cachekey = key.cachekey << 4 | v;
        else key.hash = key.hash << 4 | v;
    }
    return key;
}

std::uint32_t replacement_texture_address(const GeRegisters &regs) {
    return (r24(regs, 0xA0) & 0xFFFFF0u) | ((r24(regs, 0xA8) << 8) & 0x0F000000u);
}

std::uint32_t replacement_clut_hash(const GeRegisters &regs, const PpssppClut &clut) {
    const std::uint32_t format = r24(regs, 0xC3) & 0xFu;
    if ((format & 4u) == 0u || format > 7u) return 0;
    const std::uint32_t clutformat = r24(regs, 0xC5);
    const std::uint32_t start_index = ((clutformat >> 16) & 0x1Fu) << 4;
    const std::uint32_t start_bytes = start_index * ((clutformat & 3u) == 3u ? 4u : 2u);
    const std::uint32_t bytes = std::min(clut.last_bytes + start_bytes, clut.max_bytes);
    // PPSSPP xors the whole CLUTFORMAT command word, command byte 0xC5 included.
    return xxh32(clut.raw.data(), bytes, kClutSeed) ^ (0xC5000000u | clutformat);
}

std::uint64_t replacement_cachekey(const GeRegisters &regs, std::uint32_t clut_hash) {
    const std::uint32_t format = r24(regs, 0xC3) & 0xFu;
    std::uint64_t key = static_cast<std::uint64_t>(replacement_texture_address(regs) & 0x3FFFFFFFu) << 32 | (r24(regs, 0xB8) & 0xF0Fu);
    if ((format & 4u) != 0u) key ^= clut_hash;
    return key;
}

std::uint32_t replacement_data_hash(const psprecomp::GuestMemory &memory, const GeRegisters &regs, std::uint16_t max_seen_v) {
    const std::uint32_t format = r24(regs, 0xC3) & 0xFu;
    const std::uint32_t bpp = kBitsPerTexel[format];
    if (bpp == 0) return 0;
    const std::uint32_t dim = r24(regs, 0xB8);
    const std::uint32_t w = 1u << (dim & 0xFu);
    std::uint32_t h = 1u << ((dim >> 8) & 0xFu);
    const bool swizzled = (r24(regs, 0xC2) & 1u) != 0u;
    // A swizzled 512-tall CLUT4 atlas may be filled below the drawn rows: hash all of it.
    const std::uint16_t hash_max_v = (h == 512u && swizzled && format == 4u) ? 512 : max_seen_v;
    if (h == 512u && hash_max_v < 512 && hash_max_v != 0) h = hash_max_v;
    const std::uint32_t address = replacement_texture_address(regs);
    const std::uint32_t bufw = buffer_width(regs, format);
    if (bufw <= w) {
        const std::uint32_t pixels = bufw * h + (w - bufw);
        const std::uint32_t size = bpp * pixels / 8u;
        const std::uint8_t *data = memory.raw_pointer(address, size);
        if (!data) return 0;
        return static_cast<std::uint32_t>(xxh64(data, size, kDataSeed));
    }
    const std::uint32_t line = bpp * w / 8u, stride = bpp * bufw / 8u;
    const std::uint32_t span = h > 0 ? (h - 1u) * stride + line : 0u;
    const std::uint8_t *data = memory.raw_pointer(address, span);
    if (!data) return 0;
    std::uint32_t result = 0;
    for (std::uint32_t y = 0; y < h; ++y) result = (result * 11u) ^ static_cast<std::uint32_t>(xxh64(data + y * stride, line, kDataSeed));
    return result;
}

std::optional<ReplacementKey> find_replacement(const ReplacementKeySet &pack, ReplacementKey key, bool ignore_address) {
    return find_replacement([&pack](const ReplacementKey &k) { return pack.contains(k); }, key, ignore_address);
}

std::optional<ReplacementKey> find_replacement(const std::function<bool(const ReplacementKey &)> &contains, ReplacementKey key,
                                               bool ignore_address) {
    if (ignore_address) key.cachekey &= 0xFFFFFFFFull;
    const std::uint64_t ck = key.cachekey;
    const std::uint32_t hash = key.hash;
    const std::uint64_t low = ck & 0xFFFFFFFFull, high = ck & ~0xFFFFFFFFull;
    ReplacementKey tries[] = {
        {ck, hash},
        {low, 0},     // CLUT hash only
        {ck, 0},      // no data hash (address kept)
        {low, hash},  // no address
        {high, hash}, // address without CLUT hash
        {high, 0},    // address only
        {0, hash},    // data hash only
    };
    const bool needs_address[] = {false, false, true, false, true, true, false};
    for (std::size_t i = 0; i < std::size(tries); ++i) {
        if (needs_address[i] && ignore_address) continue;
        if (contains(tries[i])) return tries[i];
    }
    return std::nullopt;
}

std::uint16_t TextureKeys::update_max_seen_v(std::uint64_t cachekey, std::uint32_t clut_hash, bool through, std::uint16_t draw_max_v) {
    auto &entry = max_seen_v_[cachekey];
    const std::uint64_t first = cachekey & ~0xFFFFFFFFull, last = first + (1ull << 32);
    // A new CLUT variant of a texture takes the height already seen at that address.
    if (clut_hash != 0 && entry == 0) {
        for (auto it = max_seen_v_.lower_bound(first); it != max_seen_v_.end() && it->first < last; ++it)
            if (it->second != 0) { entry = it->second; break; }
    }
    if (through) {
        if (entry == 0 && draw_max_v > 0) entry = std::max<std::uint16_t>(272, draw_max_v);
        else if (draw_max_v > entry) entry = 512;
    } else {
        entry = 512;
    }
    const std::uint16_t value = entry;
    if (clut_hash != 0)
        for (auto it = max_seen_v_.lower_bound(first); it != max_seen_v_.end() && it->first < last; ++it) it->second = value;
    return value;
}

ReplacementKey TextureKeys::key(const psprecomp::GuestMemory &memory, const GeRegisters &regs, const PpssppClut &clut, bool through,
                                std::uint16_t draw_max_v) {
    const std::uint32_t clut_hash = replacement_clut_hash(regs, clut);
    const std::uint64_t cachekey = replacement_cachekey(regs, clut_hash);
    std::uint16_t max_seen_v = 0;
    if ((r24(regs, 0xB8) & 0xF0Fu) >= 0x900u) max_seen_v = update_max_seen_v(cachekey, clut_hash, through, draw_max_v);
    return {cachekey, replacement_data_hash(memory, regs, max_seen_v)};
}

} // namespace p3p3ds::ge
