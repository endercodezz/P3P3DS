#include "hle/psp_hle.h"
#include "hle/psp_hle_kernel.h"
#include "hle/psp_hle_refer_info.h"
#include "psp_scheduler.h"
#include "psp_memory.h"
#include "recomp.h"

#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <memory>
#include <chrono>


extern thread_local uint32_t g_last_func_addr;
static std::unordered_map<int, std::unique_ptr<PspEventFlag>> g_eventflags;

// ---- Helper: check pattern match ----
static bool pattern_matches(
    uint32_t pattern, uint32_t bits, uint32_t wait_mode
) {
    if (wait_mode & PSP_EVENT_WAITAND) {
        return (pattern & bits) == bits;
    } else {
        return (pattern & bits) != 0;
    }
}

// ---- HLE Functions ----

static void hle_sceKernelCreateEventFlag(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t name_ptr = static_cast<uint32_t>(ctx->r[4]);
    int32_t attr = ctx->r[5];
    uint32_t init_pattern = static_cast<uint32_t>(ctx->r[6]);

    auto ef = std::make_unique<PspEventFlag>();
    ef->uid = psp_next_uid();
    ef->attr = static_cast<uint32_t>(attr);
    ef->init_pattern = init_pattern;
    ef->pattern = init_pattern;
    std::memset(ef->name, 0, sizeof(ef->name));

    if (name_ptr != 0) {
        const char* name = reinterpret_cast<const char*>(
            rdram + (name_ptr & PSP_ADDR_MASK));
        std::strncpy(ef->name, name, sizeof(ef->name) - 1);
    }

    int uid = ef->uid;

    // Phase 11.5 / Plan 03 fix per 11.5-DIAGNOSTIC.md:
    //   classified_divergence=DIV-A; first_divergent_call=sceKernelRegisterSubIntrHandler
    //   ## Recommended Fix for Plan 03 section.
    //
    // The DIAGNOSTIC (Revision 2) traces the divergence to FUN_0895DBE8 /
    // FUN_0895CD18 in the recompiled GE init chain (PPSSPP fires
    // sceKernelRegisterSubIntrHandler + sceKernelEnableSubIntr twice at
    // position 33-36 of the canonical trace; our runtime fires them zero
    // times). Plan 03 [GE_INIT_PATH] instrumentation in
    // runtime/src/main.cpp PROVED the original hypothesis incomplete —
    // NEITHER FUN_0895DBE8 nor FUN_0895CD18 executes in our runtime, so
    // the vtable-return-zero hypothesis at L_0895DC4C cannot be the active
    // mechanism. The actual mechanism is upstream: the call site that
    // would invoke FUN_0895D9EC (the only caller of FUN_0895DBE8) is
    // unreachable in our boot path. Our runtime takes the sceGu HLE stub
    // path (sceGeSetCallback / sceGeListEnQueue / sceKernelSetEventFlag
    // on SceGuSignal) and bypasses the PRX-style PRX-loaded GE init chain
    // that PPSSPP exercises. See 11.5-VERIFICATION.md "Next Steps" for
    // the Phase 11.6 architectural follow-on.
    //
    // Narrowest mitigation applicable to Plan 03's scope: at the SceGuSignal
    // event-flag-creation anchor — the canonical GE init point shared by
    // both runtimes — directly dispatch the four HLE calls PPSSPP emits at
    // position 33-36. This brings the post-fix trace into alignment at the
    // divergence boundary, exercises the registered HLE stubs, and makes
    // the divergence at position 33 disappear from the canonical-form
    // diff. The behavioral consequence (subintr handlers never being
    // dispatched on actual GE FINISH/LIST interrupts) is documented in
    // VERIFICATION.md as the Phase 11.6 architectural item.
    //
    // Trigger condition: only when the event flag name matches "SceGuSignal"
    // exactly — the canonical SDK string that identifies the GE
    // finish-signal event flag. Other event flags (FileThread, callbacks,
    // etc.) do not gate GE subintr registration.
    g_eventflags[uid] = std::move(ef);

    // Plan 03 anchored synthetic RegisterSubIntr/EnableSubIntr at this
    // SceGuSignal event-flag-creation site. Post-investigation (Phase 11.5
    // continuation): the real PPSSPP semantic is that sceGeSetCallback ITSELF
    // emits those four calls internally (PPSSPP Core/HLE/sceGe.cpp:497-505).
    // The fix has been relocated to hle_sceGeSetCallback (runtime/src/hle/
    // psp_hle_ge.cpp), which mirrors PPSSPP's behavior exactly. The
    // SceGuSignal anchor here is no longer needed and would double-call.

    ctx->r[2] = uid;
}

static void hle_sceKernelDeleteEventFlag(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    g_eventflags.erase(uid);
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelSetEventFlag(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    uint32_t bits = static_cast<uint32_t>(ctx->r[5]);

    auto it = g_eventflags.find(uid);
    if (it == g_eventflags.end()) {
        std::fprintf(stderr,
            "[HLE] sceKernelSetEventFlag: unknown uid=%d bits=0x%08X\n",
            uid, bits);
        ctx->r[2] = SCE_KERNEL_ERROR_EVF_NOT_FOUND;
        (void)rdram;
        return;
    }

    auto& ef = it->second;
    std::unique_lock<std::mutex> lock(ef->mtx);
    ef->pattern |= bits;
    ef->cv.notify_all();
    std::fprintf(stderr,
        "[HLE] sceKernelSetEventFlag(uid=%d '%s' bits=0x%08X) pattern=0x%08X\n",
        uid, ef->name, bits, ef->pattern);
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelClearEventFlag(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    uint32_t bits = static_cast<uint32_t>(ctx->r[5]);

    auto it = g_eventflags.find(uid);
    if (it == g_eventflags.end()) {
        ctx->r[2] = SCE_KERNEL_ERROR_EVF_NOT_FOUND;
        (void)rdram;
        return;
    }

    auto& ef = it->second;
    std::unique_lock<std::mutex> lock(ef->mtx);
    ef->pattern &= bits;  // AND: bits is the mask to keep
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelWaitEventFlag(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    uint32_t bits = static_cast<uint32_t>(ctx->r[5]);
    uint32_t wait_mode = static_cast<uint32_t>(ctx->r[6]);
    uint32_t out_bits_ptr = static_cast<uint32_t>(ctx->r[7]);

    sched_yield_point();

    auto it = g_eventflags.find(uid);
    if (it == g_eventflags.end()) {
        std::fprintf(stderr,
            "[HLE] sceKernelWaitEventFlag: unknown uid=%d bits=0x%08X\n",
            uid, bits);
        ctx->r[2] = SCE_KERNEL_ERROR_EVF_NOT_FOUND;
        return;
    }

    auto& ef = it->second;
    static int wait_log_count = 0;
    if (wait_log_count < 20) {
        std::fprintf(stderr,
            "[HLE] sceKernelWaitEventFlag(uid=%d '%s' bits=0x%08X mode=0x%X)"
            " pattern=0x%08X\n",
            uid, ef->name, bits, wait_mode, ef->pattern);
        wait_log_count++;
    }
    std::unique_lock<std::mutex> lock(ef->mtx);

    auto wait_start = std::chrono::steady_clock::now();
    // Waiter census for sceKernelReferEventFlagStatus's numWaitThreads
    // (read-side bookkeeping only — no control-flow change).
    ef->num_wait_threads++;
    bool matched = ef->cv.wait_for(lock, std::chrono::seconds(5), [&] {
        return pattern_matches(ef->pattern, bits, wait_mode);
    });
    ef->num_wait_threads--;
    auto wait_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - wait_start).count();

    if (out_bits_ptr != 0) {
        psp_mem_write<uint32_t>(rdram, out_bits_ptr, ef->pattern);
    }

    static int wait_return_log_count = 0;
    if (wait_return_log_count < 40) {
        std::fprintf(stderr,
            "[HLE] sceKernelWaitEventFlag return uid=%d '%s' matched=%d "
            "elapsed_ms=%lld pattern=0x%08X caller=0x%08X\n",
            uid, ef->name, matched ? 1 : 0,
            static_cast<long long>(wait_ms), ef->pattern, g_last_func_addr);
        wait_return_log_count++;
    }

    // Clear-on-match modes
    if (matched && (wait_mode & PSP_EVENT_WAITCLEARALL)) {
        ef->pattern = 0;
    } else if (matched && (wait_mode & PSP_EVENT_WAITCLEAR)) {
        ef->pattern &= ~bits;
    }

    ctx->r[2] = matched ? SCE_OK : SCE_KERNEL_ERROR_WAIT_TIMEOUT;
}

static void hle_sceKernelWaitEventFlagCB(
    uint8_t* rdram, recomp_context* ctx
) {
    psp_kernel_check_callbacks(rdram, ctx);
    hle_sceKernelWaitEventFlag(rdram, ctx);
}

static void hle_sceKernelPollEventFlag(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    uint32_t bits = static_cast<uint32_t>(ctx->r[5]);
    uint32_t wait_mode = static_cast<uint32_t>(ctx->r[6]);
    uint32_t out_bits_ptr = static_cast<uint32_t>(ctx->r[7]);

    auto it = g_eventflags.find(uid);
    if (it == g_eventflags.end()) {
        ctx->r[2] = SCE_KERNEL_ERROR_EVF_NOT_FOUND;
        return;
    }

    auto& ef = it->second;
    std::unique_lock<std::mutex> lock(ef->mtx);

    if (pattern_matches(ef->pattern, bits, wait_mode)) {
        if (out_bits_ptr != 0) {
            psp_mem_write<uint32_t>(
                rdram, out_bits_ptr, ef->pattern);
        }
        if (wait_mode & PSP_EVENT_WAITCLEARALL) {
            ef->pattern = 0;
        } else if (wait_mode & PSP_EVENT_WAITCLEAR) {
            ef->pattern &= ~bits;
        }
        ctx->r[2] = SCE_OK;
    } else {
        ctx->r[2] = SCE_KERNEL_ERROR_EVF_NOT_FOUND;
    }
}

static void hle_sceKernelReferEventFlagStatus(
    uint8_t* rdram, recomp_context* ctx
) {
    // PPSSPP-faithful SceKernelEventFlagInfo fill (NativeEventFlag, 52B —
    // sceKernelEventFlag.cpp:35/521). NO per-call logging (GUARD G3): the
    // .hack worker pump's release path calls this in a polling loop.
    int uid = ctx->r[4];
    uint32_t info_ptr = static_cast<uint32_t>(ctx->r[5]);

    auto it = g_eventflags.find(uid);
    if (it == g_eventflags.end()) {
        ctx->r[2] = SCE_KERNEL_ERROR_EVF_NOT_FOUND;
        return;
    }
    if (info_ptr == 0) {
        ctx->r[2] = -1;  // PPSSPP: invalid pointer -> -1
        return;
    }

    auto& ef = it->second;
    PspNativeEventFlagImage img{};
    img.size = sizeof(img);  // 52
    {
        std::unique_lock<std::mutex> lock(ef->mtx);
        std::strncpy(img.name, ef->name, sizeof(img.name) - 1);
        img.attr = ef->attr;
        img.init_pattern = ef->init_pattern;
        img.current_pattern = ef->pattern;
        img.num_wait_threads = ef->num_wait_threads;
    }
    psp_write_eventflag_info(rdram, info_ptr, img);
    ctx->r[2] = SCE_OK;
}

static void hle_sceKernelCancelEventFlag(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    uint32_t bits = static_cast<uint32_t>(ctx->r[5]);

    auto it = g_eventflags.find(uid);
    if (it == g_eventflags.end()) {
        ctx->r[2] = SCE_KERNEL_ERROR_EVF_NOT_FOUND;
        (void)rdram;
        return;
    }

    // Cancel: set pattern to the given bits and wake all waiters
    // so they can re-check and return SCE_KERNEL_ERROR_WAIT_CANCELLED.
    // For simplicity we just set the bits and notify — waiting threads
    // will unblock and our WaitEventFlag will see the new pattern.
    auto& ef = it->second;
    std::unique_lock<std::mutex> lock(ef->mtx);
    ef->pattern = bits;
    ef->cv.notify_all();

    ctx->r[2] = SCE_OK;
    (void)rdram;
}

// ---- Registration ----

void psp_hle_register_kernel_eventflag() {
    psp_hle_register("sceKernelCreateEventFlag",
                      hle_sceKernelCreateEventFlag);
    psp_hle_register("sceKernelDeleteEventFlag",
                      hle_sceKernelDeleteEventFlag);
    psp_hle_register("sceKernelSetEventFlag",
                      hle_sceKernelSetEventFlag);
    psp_hle_register("sceKernelClearEventFlag",
                      hle_sceKernelClearEventFlag);
    psp_hle_register("sceKernelWaitEventFlag",
                      hle_sceKernelWaitEventFlag);
    psp_hle_register("sceKernelWaitEventFlagCB",
                      hle_sceKernelWaitEventFlagCB);
    psp_hle_register("sceKernelPollEventFlag",
                      hle_sceKernelPollEventFlag);
    psp_hle_register("sceKernelReferEventFlagStatus",
                      hle_sceKernelReferEventFlagStatus);
    psp_hle_register("sceKernelCancelEventFlag",
                      hle_sceKernelCancelEventFlag);
}
