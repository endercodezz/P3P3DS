#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/hle/display.hpp"
#include "p3p3ds/hle/ge.hpp"
#include "p3p3ds/hle/sysmem.hpp"
#include "p3p3ds/hle/threadman.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/runtime.hpp"

namespace p3p3ds::hle {

void register_all_hle_modules(psprecomp::Runtime &runtime, KernelState &kernel) {
    register_sysmem_user_for_user(runtime, kernel);
    register_threadman_for_user(runtime, kernel);
    register_display_module(runtime, kernel);
    register_ge_module(runtime, kernel);

    // ModuleMgrForUser::0xD8B73127 - sceKernelGetModuleIdByAddress
    runtime.register_hle("ModuleMgrForUser", 0xD8B73127u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: return main game module UID 1
            ctx.set_gpr(2, 1u);
        });

    // Kernel_Library::0x092968F4 - sceKernelCpuSuspendIntr
    runtime.register_hle("Kernel_Library", 0x092968F4u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: return old interrupt state (1 = interrupts previously enabled)
            ctx.set_gpr(2, 1u);
        });

    // Kernel_Library::0x5F10D406 - sceKernelCpuResumeIntr
    runtime.register_hle("Kernel_Library", 0x5F10D406u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, 0u);
        });

    // Kernel_Library::0x293B45B8 - sceKernelGetThreadId
    runtime.register_hle("Kernel_Library", 0x293B45B8u,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, static_cast<std::uint32_t>(kernel.threads().current_thread_id()));
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

    // Kernel_Library::0xBEA46419 - sceKernelLockLwMutex
    runtime.register_hle("Kernel_Library", 0xBEA46419u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: single-threaded bootstrap lock success
            ctx.set_gpr(2, 0u);
        });

    // Kernel_Library::0x15B6446B - sceKernelUnlockLwMutex
    runtime.register_hle("Kernel_Library", 0x15B6446Bu,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: single-threaded bootstrap unlock success
            ctx.set_gpr(2, 0u);
        });

    // Kernel_Library::0x1839852A - sceKernelTryLockLwMutex
    runtime.register_hle("Kernel_Library", 0x1839852Au,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: single-threaded bootstrap try-lock success
            ctx.set_gpr(2, 0u);
        });

    // ThreadManForUser::0xAA73C935 - sceKernelExitThread
    runtime.register_hle("ThreadManForUser", 0xAA73C935u,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const std::int32_t status = static_cast<std::int32_t>(ctx.gpr[4]);
            kernel.threads().exit_current_thread(status, ctx, rt);
        });

    // ThreadManForUser::0xE81CAF8F - sceKernelCreateCallback
    runtime.register_hle("ThreadManForUser", 0xE81CAF8Fu,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: callback UID = 1
            ctx.set_gpr(2, 1u);
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

    // ThreadManForUser::0x68DA9E36 - sceKernelDelayThread
    runtime.register_hle("ThreadManForUser", 0x68DA9E36u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: delay thread no-op in bootstrap
            ctx.set_gpr(2, 0u);
        });

    // ThreadManForUser::0x369ED59D - sceKernelGetSystemTimeLow
    runtime.register_hle("ThreadManForUser", 0x369ED59Du,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            static std::uint32_t s_time = 1000u;
            s_time += 1000u;
            ctx.set_gpr(2, s_time);
        });

    // ThreadManForUser::0x55C20A00 - sceKernelCreateEventFlag
    runtime.register_hle("ThreadManForUser", 0x55C20A00u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: return event flag UID = 1
            ctx.set_gpr(2, 1u);
        });

    // ThreadManForUser::0xEF9E4C70 - sceKernelDeleteEventFlag
    runtime.register_hle("ThreadManForUser", 0xEF9E4C70u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, 0u);
        });

    // ThreadManForUser::0x1FB15A32 - sceKernelSetEventFlag
    runtime.register_hle("ThreadManForUser", 0x1FB15A32u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, 0u);
        });

    // ThreadManForUser::0x812346E4 - sceKernelClearEventFlag
    runtime.register_hle("ThreadManForUser", 0x812346E4u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, 0u);
        });

    // ThreadManForUser::0x402FCF22 - sceKernelWaitEventFlag
    runtime.register_hle("ThreadManForUser", 0x402FCF22u,
        [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t bits = ctx.gpr[5];
            const std::uint32_t out_bits_ptr = ctx.gpr[7];
            if (out_bits_ptr != 0u && rt.memory().contains(out_bits_ptr, 4u)) {
                rt.memory().store32(out_bits_ptr, bits);
            }
            ctx.set_gpr(2, 0u);
        });

    // ThreadManForUser::0x328C546A - sceKernelWaitEventFlagCB
    runtime.register_hle("ThreadManForUser", 0x328C546Au,
        [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t bits = ctx.gpr[5];
            const std::uint32_t out_bits_ptr = ctx.gpr[7];
            if (out_bits_ptr != 0u && rt.memory().contains(out_bits_ptr, 4u)) {
                rt.memory().store32(out_bits_ptr, bits);
            }
            ctx.set_gpr(2, 0u);
        });

    // ThreadManForUser::0x30FD7D3A - sceKernelPollEventFlag
    runtime.register_hle("ThreadManForUser", 0x30FD7D3Au,
        [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t bits = ctx.gpr[5];
            const std::uint32_t out_bits_ptr = ctx.gpr[7];
            if (out_bits_ptr != 0u && rt.memory().contains(out_bits_ptr, 4u)) {
                rt.memory().store32(out_bits_ptr, bits);
            }
            ctx.set_gpr(2, 0u);
        });
}

} // namespace p3p3ds::hle
