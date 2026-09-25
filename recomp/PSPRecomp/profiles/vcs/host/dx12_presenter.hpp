#pragma once

#include "vcs_config.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace vcs {

struct Dx12PresenterStatus {
    bool initialized{};
    bool hardware_adapter{};
    bool tearing_supported{};
    std::uint32_t frames_in_flight{};
    std::string adapter_name;
    std::string last_error;
};

// Windows-only Direct3D 12 presentation backend. It owns a flip-model DXGI
// swapchain, one upload arena/source texture per frame in flight, and performs
// scaling/aspect correction on the GPU with a fullscreen triangle. Non-Windows
// builds expose harmless stubs so the rest of the source remains portable.
[[nodiscard]] bool dx12_presenter_initialize(void *native_window, std::string &error) noexcept;
[[nodiscard]] bool dx12_presenter_present_rgba(
    std::span<const std::byte> rgba,
    std::uint32_t source_width,
    std::uint32_t source_height,
    const DisplayConfiguration &display,
    bool force_preserve_aspect,
    std::string &error) noexcept;
void dx12_presenter_shutdown() noexcept;
[[nodiscard]] bool dx12_presenter_active() noexcept;
[[nodiscard]] Dx12PresenterStatus dx12_presenter_status();

} // namespace vcs
