#include "p3p3ds/hle/sysmem.hpp"
#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/runtime.hpp"

namespace p3p3ds::hle {

void sceKernelSetCompiledSdkVersion600_602(KernelState &kernel, std::uint32_t version) noexcept {
    kernel.set_compiled_sdk_version(version);
}

void sceKernelSetCompilerVersion(KernelState &kernel, std::uint32_t version) noexcept {
    kernel.set_compiler_version(version);
}

void register_sysmem_user_for_user(psprecomp::Runtime &runtime, KernelState &kernel) {
    // SysMemUserForUser::0x35669D4C - sceKernelSetCompiledSdkVersion600_602
    runtime.register_hle("SysMemUserForUser", 0x35669D4Cu,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t version = ctx.gpr[4]; // $a0
            sceKernelSetCompiledSdkVersion600_602(kernel, version);
            ctx.set_gpr(2, 0u); // $v0 = 0 (SCE_KERNEL_ERROR_OK)
        });

    // SysMemUserForUser::0xF77D77CB - sceKernelSetCompilerVersion
    runtime.register_hle("SysMemUserForUser", 0xF77D77CBu,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t version = ctx.gpr[4]; // $a0
            sceKernelSetCompilerVersion(kernel, version);
            ctx.set_gpr(2, 0u); // $v0 = 0 (SCE_KERNEL_ERROR_OK)
        });
}

} // namespace p3p3ds::hle
