#pragma once

#include "p3p3ds/kernel_state.hpp"
#include <cstdint>

namespace psprecomp {
class Runtime;
}

namespace p3p3ds::hle {

// Real PSP API semantics (uOFW SysMem, pspautotests misc/sdkver.cpp, PPSSPP):
// int sceKernelSetCompiledSdkVersion600_602(int sdkVersion);
// Pure registration function. Records compiled SDK version in kernel state.
// Real API returns 0 (SCE_KERNEL_ERROR_OK) in $v0.
void sceKernelSetCompiledSdkVersion600_602(KernelState &kernel, std::uint32_t version) noexcept;

// SysMemUserForUser module handler / registration
void register_sysmem_user_for_user(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
