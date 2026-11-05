#pragma once
// New 3DS texture pack written by `p3p-texpack build` (tools/texpack/src/build.rs
// documents the format): index.bin (entries sorted by PPSSPP-compatible key)
// and textures.bin (PICA200 tiled data, rows bottom-up). The index is read
// once; texture data is read on demand.
#include "p3p3ds/ge/texture_replacement.hpp"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace p3p3ds::ge {

struct PackEntry {
    ReplacementKey key;
    std::uint32_t offset{}, size{};
    std::uint16_t width{}, height{}; // 0 x 0: keep the original texture
    std::uint8_t format{};           // citro3d GPU_TEXCOLOR (0 RGBA8, 1 RGB8, 2 RGBA5551, 3 RGB565, 4 RGBA4)
};

class TexturePack {
public:
    // nullptr with `error` set when the directory has no valid pack.
    static std::unique_ptr<TexturePack> open(const std::filesystem::path &dir, std::string &error);
    ~TexturePack();
    TexturePack(const TexturePack &) = delete;
    TexturePack &operator=(const TexturePack &) = delete;

    [[nodiscard]] bool ignore_address() const noexcept { return ignore_address_; }
    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    // The entry PPSSPP's lookup order finds for `key` (find_replacement), or nullptr.
    [[nodiscard]] const PackEntry *find(const ReplacementKey &key) const;
    // Texture data of an entry with a size; false on a read error.
    bool read(const PackEntry &entry, std::vector<std::uint8_t> &out) const;
    // The same into `out` (entry.size bytes, e.g. a C3D_Tex's data).
    bool read_into(const PackEntry &entry, void *out) const;

private:
    TexturePack() = default;
    [[nodiscard]] const PackEntry *exact(const ReplacementKey &key) const;
    std::vector<PackEntry> entries_;
    bool ignore_address_{};
    std::FILE *data_{};
};

// The replacement for the texture a draw binds: keeps the CLUT bytes and the
// per-texture maxSeenV PPSSPP keys depend on (texture_replacement.hpp).
// Used by the PC reference renderer and the New 3DS renderer.
class TextureReplacer {
public:
    explicit TextureReplacer(std::shared_ptr<const TexturePack> pack) : pack_(std::move(pack)) {}
    // nullptr when the pack has no replacement (or says "keep the original").
    // `through` / `max_v`: the draw's mode and largest V texel (through_max_v).
    const PackEntry *find(psprecomp::GuestMemory &memory, const GeRegisters &regs, bool through, std::uint16_t max_v);
    [[nodiscard]] const TexturePack &pack() const noexcept { return *pack_; }

private:
    std::shared_ptr<const TexturePack> pack_;
    TextureKeys keys_;
    PpssppClut clut_;
    std::uint64_t clut_seen_{};
};

// Largest V texel coordinate of a through-mode draw's vertices (0 otherwise).
[[nodiscard]] std::uint16_t through_max_v(psprecomp::GuestMemory &memory, const GeRegisters &regs, std::uint32_t count,
                                          std::uint32_t vertex_address, std::uint32_t index_address);

// Texture data of `entry` -> RGBA texels (r in the low byte), image row 0 first.
void decode_pack_texture(const PackEntry &entry, const std::vector<std::uint8_t> &data, std::vector<std::uint32_t> &out);

} // namespace p3p3ds::ge
