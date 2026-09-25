#pragma once

#include "kernel/kernel.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace mhp3rd {

using HleFunction = std::function<void(Runtime &, AllegrexContext &)>;

// PSP EABI argument registers: a0-a3 then t0-t3.
[[nodiscard]] inline std::uint32_t arg(const AllegrexContext &ctx, unsigned index) noexcept {
    return index < 4u ? ctx.gpr[4u + index] : ctx.gpr[8u + (index - 4u)];
}

// 64-bit arguments occupy an even-aligned register pair, low word first.
[[nodiscard]] inline std::uint64_t arg64(const AllegrexContext &ctx, unsigned low_index) noexcept {
    return static_cast<std::uint64_t>(arg(ctx, low_index)) |
           (static_cast<std::uint64_t>(arg(ctx, low_index + 1u)) << 32u);
}

[[nodiscard]] std::string read_cstring(const psprecomp::GuestMemory &memory, std::uint32_t address,
                                       std::size_t max_length = 512u);
void write_cstring(psprecomp::GuestMemory &memory, std::uint32_t address, std::string_view text,
                   std::size_t capacity);
void store64(psprecomp::GuestMemory &memory, std::uint32_t address, std::uint64_t value);

// Registers HLE handlers by function name, resolving NIDs from the runtime's
// NID registry, and remembers what was bound so the profile can stub the rest.
class HleRegistrar {
public:
    explicit HleRegistrar(Runtime &runtime);

    void add(std::string_view library, std::string_view name, HleFunction function);
    // Binds `function` if the name is known; returns false otherwise.
    bool try_add(std::string_view library, std::string_view name, HleFunction function);
    [[nodiscard]] bool bound(const std::string &library, std::uint32_t nid) const;
    [[nodiscard]] std::size_t count() const noexcept { return bound_.size(); }

private:
    Runtime &runtime_;
    std::map<std::pair<std::string, std::string>, std::uint32_t> nids_by_name_;
    std::set<std::pair<std::string, std::uint32_t>> bound_;
};

// Reads from an open guest file descriptor without moving its position.
// Returns the number of bytes read; 0 when the descriptor is unknown.
std::size_t read_open_file(std::uint32_t fd, std::uint64_t offset, std::uint8_t *output, std::size_t size);

// True when MHP3RD_TRACE_SYNC is set: logs kernel object activity.
[[nodiscard]] bool trace_sync();
void log_sync(const std::string &message);

// Prints a message the first time `key` is seen.
void log_once(const std::string &key, const std::string &message);

// A movie is being decoded: the game has a sceMpeg instance (hle_mpeg.cpp).
[[nodiscard]] bool mpeg_active();
// The game has ad hoc networking initialised (hle_adhoc.cpp).
[[nodiscard]] bool adhoc_networking_on();

void register_threadman(HleRegistrar &hle);
void register_sysmem(HleRegistrar &hle);
void register_io(HleRegistrar &hle, const std::filesystem::path &disc_image, const std::filesystem::path &memory_stick);
void register_system(HleRegistrar &hle);
void register_media(HleRegistrar &hle);
void register_atrac(HleRegistrar &hle);
void register_mpeg(HleRegistrar &hle);
void register_font(HleRegistrar &hle);
void register_utility(HleRegistrar &hle, const std::filesystem::path &memory_stick);
void register_savedata(HleRegistrar &hle, const std::filesystem::path &memory_stick);
void register_adhoc(HleRegistrar &hle);

#if defined(MHP3RD_HAS_RENDERER)
namespace gpu { class VulkanRenderer; }
// The renderer owns the window, so HLE that needs host input goes through it.
[[nodiscard]] gpu::VulkanRenderer *active_renderer();
// Creates the renderer, with the window and the interface on it, the first
// time it is needed: by the setup screens before the game starts, otherwise
// when the game registers its media imports. Null without a window.
gpu::VulkanRenderer *ensure_renderer();
#endif

} // namespace mhp3rd
