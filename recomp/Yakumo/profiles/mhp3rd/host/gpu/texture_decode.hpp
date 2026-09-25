#pragma once

#include "ge_state.hpp"

#include <cstdint>
#include <vector>

namespace mhp3rd::gpu {

// Decodes a PSP texture into RGBA8888 (one std::uint32_t per texel, red in the
// low byte). Handles the direct colour formats, 4/8/16/32-bit palettes, DXT1/3/5
// and the swizzled layout. Returns false when the texture cannot be read.
bool decode_texture(const GuestMemory &memory, const TextureState &texture, std::vector<std::uint32_t> &out);

// What decoding a texture reads from guest memory, copied, so that it can be
// decoded on another thread while the game goes on changing memory.
struct TextureSnapshot {
    TextureState texture;
    std::uint32_t row_bytes{};
    std::vector<std::uint8_t> texels;
    std::vector<std::uint8_t> clut;  // the palette's first 512 entries, for the indexed formats
};
// Copies what decode_texture() would read; false for a texture it cannot copy
// that way (a block format, a run not contiguous in host memory), which is
// then decoded at once.
bool snapshot_texture(const GuestMemory &memory, const TextureState &texture, TextureSnapshot &snapshot);
// Decodes a snapshot to the texels decode_texture() gives. Safe on any thread;
// unswizzles the snapshot's texels in place.
bool decode_snapshot(TextureSnapshot &snapshot, std::vector<std::uint32_t> &out);

// Key that identifies the decoded contents of a texture for caching.
[[nodiscard]] std::uint64_t texture_key(const GuestMemory &memory, const TextureState &texture);

} // namespace mhp3rd::gpu
