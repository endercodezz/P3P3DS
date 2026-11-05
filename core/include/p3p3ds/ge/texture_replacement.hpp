#pragma once
// Texture keys compatible with PPSSPP texture packs, which is how the existing
// Persona 3 Portable packs are published (textures.ini + images, #23/#56).
// A pack names a replacement by <cachekey:16 hex><hash:8 hex>. The rules below
// reproduce PPSSPP's observable behaviour (references/ppsspp/GPU/Common/
// TextureCacheCommon.cpp, TextureReplacer.cpp; behavioural reference only,
// AGENTS.md section 4). [UNVERIFIED] until the hit rate of a real pack on the
// PC runner is measured (--texture-keys).
//
//   cachekey = ((texaddr & 0x3FFFFFFF) << 32 | dim) ^ cluthash   (cluthash only for CLUT formats)
//   dim      = TEXSIZE0 & 0xF0F
//   cluthash = XXH32(clut bytes, n, 0xC0108888) ^ (0xC5000000 | CLUTFORMAT)
//              n = min(last load + start index bytes, largest load so far)
//   hash     = low 32 bits of XXH64(texture bytes, 0xBACD7814), rows hashed
//              separately (h = h * 11 ^ row) when the buffer is wider than the texture;
//              a 512-texel-tall texture hashes only maxSeenV rows (see TextureKeys).
#include "p3p3ds/ge/renderer.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string_view>
#include <unordered_set>

namespace psprecomp { class GuestMemory; }

namespace p3p3ds::ge {

struct ReplacementKey {
    std::uint64_t cachekey{};
    std::uint32_t hash{};
    bool operator==(const ReplacementKey &) const = default;
};

struct ReplacementKeyHash {
    std::size_t operator()(const ReplacementKey &k) const noexcept {
        return std::hash<std::uint64_t>{}(k.cachekey * 0x9E3779B97F4A7C15ull ^ k.hash);
    }
};

using ReplacementKeySet = std::unordered_set<ReplacementKey, ReplacementKeyHash>;

// CLUT bytes as PPSSPP keeps them for keys: a load overwrites the first bytes
// and leaves the rest of the 2 KiB buffer as it was.
struct PpssppClut {
    std::array<std::uint8_t, 2048> raw{};
    std::uint32_t last_bytes{}, max_bytes{};
    void load(const std::uint8_t *bytes, std::uint32_t size);
};

// "<16 hex><8 hex>" (a pack key) -> key; nullopt for anything else.
[[nodiscard]] std::optional<ReplacementKey> parse_replacement_key(std::string_view text);

// Texture address as PPSSPP masks it (TEXADDR0 | TEXBUFWIDTH0 bits 16-19 << 24).
[[nodiscard]] std::uint32_t replacement_texture_address(const GeRegisters &regs);
// 0 for formats without a CLUT.
[[nodiscard]] std::uint32_t replacement_clut_hash(const GeRegisters &regs, const PpssppClut &clut);
[[nodiscard]] std::uint64_t replacement_cachekey(const GeRegisters &regs, std::uint32_t clut_hash);
// Texture data hash; `max_seen_v` as tracked by TextureKeys (0 = unknown).
// 0 when the texture lies outside guest memory.
[[nodiscard]] std::uint32_t replacement_data_hash(const psprecomp::GuestMemory &memory, const GeRegisters &regs, std::uint16_t max_seen_v);

// PPSSPP's lookup order with fallbacks to keys with zeroed parts.
[[nodiscard]] std::optional<ReplacementKey> find_replacement(const std::function<bool(const ReplacementKey &)> &contains, ReplacementKey key,
                                                             bool ignore_address);
[[nodiscard]] std::optional<ReplacementKey> find_replacement(const ReplacementKeySet &pack, ReplacementKey key, bool ignore_address);

// Per-texture state PPSSPP keeps between draws (maxSeenV of 512-tall textures).
class TextureKeys {
public:
    // Key of the texture bound for a draw. `through` and `draw_max_v` (largest
    // V texel coordinate of the draw's vertices, through mode only) feed maxSeenV.
    ReplacementKey key(const psprecomp::GuestMemory &memory, const GeRegisters &regs, const PpssppClut &clut, bool through,
                       std::uint16_t draw_max_v);

private:
    std::uint16_t update_max_seen_v(std::uint64_t cachekey, std::uint32_t clut_hash, bool through, std::uint16_t draw_max_v);
    std::map<std::uint64_t, std::uint16_t> max_seen_v_; // full cachekey -> maxSeenV
};

} // namespace p3p3ds::ge
