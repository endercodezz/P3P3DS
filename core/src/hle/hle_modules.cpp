#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/hle/display.hpp"
#include "p3p3ds/hle/audio.hpp"
#include "p3p3ds/hle/ge.hpp"
#include "p3p3ds/hle/sysmem.hpp"
#include "p3p3ds/hle/threadman.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/runtime.hpp"

namespace p3p3ds::hle {

void register_all_hle_modules(psprecomp::Runtime &runtime, KernelState &kernel) {
    install_threadman_post_import_hook();
    register_sysmem_user_for_user(runtime, kernel);
    register_threadman_for_user(runtime, kernel);
    register_display_module(runtime, kernel);
    register_ge_module(runtime, kernel);
    register_umd_module(runtime, kernel);
    register_audio_module(runtime, kernel);

    // ModuleMgrForUser::0xD8B73127 - sceKernelGetModuleIdByAddress
    runtime.register_hle("ModuleMgrForUser", 0xD8B73127u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: return main game module UID 1
            ctx.set_gpr(2, 1u);
        });

    // Kernel_Library::0xA089ECA4 - sceKernelMemset
    runtime.register_hle("Kernel_Library", 0xA089ECA4u,
        [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t dest = ctx.gpr[4];
            const std::uint8_t val = static_cast<std::uint8_t>(ctx.gpr[5]);
            const std::uint32_t size = ctx.gpr[6];
            for (std::uint32_t i = 0; i < size; ++i) {
                rt.memory().store8(dest + i, val);
            }
            ctx.set_gpr(2, dest);
        });

    // Kernel_Library::sceKernelMemcpy (PSPSDK/uOFW NID, not TryLock).
    runtime.register_hle("Kernel_Library", 0x1839852Au,
        [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const auto dst = ctx.gpr[4], src = ctx.gpr[5], size = ctx.gpr[6];
            if (size && (!rt.memory().contains(dst, size) || !rt.memory().contains(src, size))) {
                rt.stop("sceKernelMemcpy invalid guest range"); return;
            }
            // Forward guest copy; do not use host pointers that bypass VRAM accounting.
            for (std::uint32_t i = 0; i < size; ++i)
                rt.memory().store8(dst + i, rt.memory().load8(src + i));
            ctx.set_gpr(2, dst);
        });

    // LoadExecForUser::0x4AC57943 - sceKernelRegisterExitCallback
    runtime.register_hle("LoadExecForUser", 0x4AC57943u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: exit callback registered
            ctx.set_gpr(2, 0u);
        });

    // sceImpose::0x36AA6E91 - sceImposeSetLanguageMode
    runtime.register_hle("sceImpose", 0x36AA6E91u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: language mode set
            ctx.set_gpr(2, 0u);
        });

    // sceUtility::0x2A2B3DE0 - sceUtilityLoadNetModule / sceUtilityLoadModule
    runtime.register_hle("sceUtility", 0x2A2B3DE0u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: utility module loaded
            ctx.set_gpr(2, 0u);
        });

}

} // namespace p3p3ds::hle
