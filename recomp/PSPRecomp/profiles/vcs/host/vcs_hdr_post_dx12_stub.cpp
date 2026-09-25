#include "vcs_hdr_post.hpp"

namespace vcs {

// SimulateHDR is intentionally disabled in the Stage 44.6 DirectX 12 RC.
// The important part here is that the Windows binary no longer imports or even
// includes the Vulkan API just to satisfy this optional module. A native D3D12
// post chain can be added after the core renderer is physically validated.
void hdr_post_configure(const std::filesystem::path &) noexcept {}
bool hdr_post_enabled() noexcept { return false; }

} // namespace vcs
