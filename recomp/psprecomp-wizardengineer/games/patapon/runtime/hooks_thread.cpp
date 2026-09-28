// games/patapon/runtime/hooks_thread.cpp — Patapon thread/CRT overrides
// (issues #46 / #47 Phase 5).
//
// Moved verbatim from runtime/src/hle/psp_hle_kernel_thread.cpp: the newlib
// no-reent assertion override at Patapon's FUN_088133EC and the
// IoAsyncCallback (FUN_088629CC) arg-struct diagnostic that used to be a
// special case inside the generic sceKernelCheckCallback dispatcher (now
// reached through the psp_kernel_set_callback_dispatch_observer seam).
// Compiled only under -DPSPRECOMP_GAME=patapon.

#include "recomp.h"
#include "psp_memory.h"
#include "hle/psp_hle.h"
#include "hle/psp_hle_kernel.h"
#include "patapon_hooks.h"

#include <cstdio>

// ---- CRT Assertion Override ----
// FUN_088133EC is the game's CRT libc assertion handler that prints
// "libc:%s: no reent structure found" via sceKernelPrintf and then
// calls sceKernelExitThread(1). On the real PSP, the kernel provides
// a per-thread _reent structure; in our runtime, the reent may not
// be fully initialized. Override this function to log a warning and
// return gracefully instead of killing the thread.

static void hle_crt_no_reent_handler(
    uint8_t* rdram, recomp_context* ctx
) {
    static int s_warn_count = 0;
    if (s_warn_count < 3) {
        std::fprintf(stderr,
            "[HLE] CRT no-reent assertion suppressed "
            "(returning gracefully)\n");
        ++s_warn_count;
    }
    // Return 0 (success) instead of calling ExitThread
    ctx->r[2] = 0;
    (void)rdram;
}

/// Initialize CRT assertion override.
/// Must be called after psp_init_dispatch_table().
void psp_crt_assertion_override_init() {
    // Override FUN_088133EC (CRT no-reent assertion handler)
    psp_dispatch_register(0x088133ECU,
        reinterpret_cast<FuncPtr>(hle_crt_no_reent_handler));

    std::fprintf(stderr,
        "[RT] CRT assertion override registered "
        "(no_reent=0x088133EC)\n");
}

// ---- IoAsyncCallback dispatch diagnostic ----
// Debug: for IoAsyncCallback (FUN_088629CC), show the function pointer at
// notify_arg+12 that determines whether the callback does useful work.
// Fires through the generic callback-dispatch observer seam right before
// sceKernelCheckCallback invokes the callback.
static void patapon_callback_dispatch_observer(
    uint8_t* rdram, uint32_t func_addr, int notify_arg
) {
    if (func_addr == 0x088629CCU && notify_arg != 0) {
        uint32_t na = static_cast<uint32_t>(notify_arg);
        uint32_t na_off = na & 0x07FFFFFFU;
        if (na_off + 16 <= 0x08000000U) {
            uint32_t fptr = psp_mem_read<uint32_t>(
                rdram, na + 12);
            // Also log offset 0, 4, 8 for full struct context
            uint32_t f0 = psp_mem_read<uint32_t>(rdram, na + 0);
            uint32_t f4 = psp_mem_read<uint32_t>(rdram, na + 4);
            uint32_t f8 = psp_mem_read<uint32_t>(rdram, na + 8);
            std::fprintf(stderr,
                "[HLE] IoAsyncCB: notify_arg=0x%08X "
                "*(+0)=0x%08X *(+4)=0x%08X *(+8)=0x%08X *(+12)=0x%08X\n",
                na, f0, f4, f8, fptr);
        }
    }
}

void patapon_install_callback_observer() {
    psp_kernel_set_callback_dispatch_observer(
        patapon_callback_dispatch_observer);
}
