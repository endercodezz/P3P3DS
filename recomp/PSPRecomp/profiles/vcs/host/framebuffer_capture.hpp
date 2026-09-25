#pragma once

#include "psprecomp/guest_memory.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace vcs {

struct FramebufferDescription {
    std::uint32_t address{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t stride{};
    std::uint32_t pixel_format{};
};

// Converts one PSP display framebuffer to tightly packed RGB888 pixels.
// Supported formats match sceDisplaySetFrameBuf: 5650, 5551, 4444 and 8888.
[[nodiscard]] std::vector<std::uint8_t> decode_framebuffer_rgb(
    const psprecomp::GuestMemory &memory,
    const FramebufferDescription &description);

// Same decode in RGBA8 form for native GPU presentation. Keeping this in the
// framebuffer decoder avoids the old RGB temporary + per-pixel channel shuffle
// before Direct3D 12 upload. Alpha is expanded from PSP formats when present.
[[nodiscard]] std::vector<std::byte> decode_framebuffer_rgba(
    const psprecomp::GuestMemory &memory,
    const FramebufferDescription &description);

void write_framebuffer_ppm(const std::filesystem::path &path,
                           const FramebufferDescription &description,
                           const std::vector<std::uint8_t> &rgb);

// Dumps changed frames when PSPRECOMP_FRAME_DUMP_DIR is set. This is a
// bring-up backend: it makes the first visible frame inspectable before a
// native window/render backend is introduced.
void capture_frame_if_requested(const psprecomp::GuestMemory &memory,
                                const FramebufferDescription &description);
void reset_frame_capture() noexcept;

} // namespace vcs
