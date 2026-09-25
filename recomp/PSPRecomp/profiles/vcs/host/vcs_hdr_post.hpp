#pragma once

#include <filesystem>

namespace vcs {

// Stage 44.6 keeps SimulateHDR disabled until its post chain is ported to
// Direct3D 12. No Vulkan declarations are exposed by the active runtime.
void hdr_post_configure(const std::filesystem::path &ini_path) noexcept;
[[nodiscard]] bool hdr_post_enabled() noexcept;

} // namespace vcs
