#include "hle/psp_hle.h"
#include "psp_render_queue.h"
#include "psp_ge.h"
#include "psp_scheduler.h"
#include "psp_memory.h"
#include "recomp.h"

#include <cstdio>

// ---- State ----
static int g_ge_cb_uid = 0;
static constexpr uint32_t GE_EDRAM_ADDR = 0x04000000U;
static constexpr uint32_t GE_EDRAM_SIZE = 0x00200000U;  // 2MB

// ---- HLE Functions ----

static void hle_sceGeListEnQueue(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t list_addr = static_cast<uint32_t>(ctx->r[4]);
    uint32_t stall_addr = static_cast<uint32_t>(ctx->r[5]);

    std::fprintf(stderr,
        "[HLE] sceGeListEnQueue(list=0x%08X, stall=0x%08X)\n",
        list_addr, stall_addr);

    // Non-blocking: push to FIFO and return UID immediately.
    // The main thread will process the list via render_queue_process().
    int uid = render_queue_enqueue_ge_list(rdram, list_addr, stall_addr);

    ctx->r[2] = uid;
}

static void hle_sceGeListEnQueueHead(
    uint8_t* rdram, recomp_context* ctx
) {
    // EnQueueHead has priority semantics on real PSP, but we don't
    // implement priority -- treat the same as EnQueue.
    hle_sceGeListEnQueue(rdram, ctx);
}

static void hle_sceGeListSync(
    uint8_t* rdram, recomp_context* ctx
) {
    int32_t list_uid = static_cast<int32_t>(ctx->r[4]);
    int32_t sync_mode = static_cast<int32_t>(ctx->r[5]);

    static int sync_count = 0;
    sync_count++;
    if (sync_count <= 10) {
        std::fprintf(stderr,
            "[HLE] sceGeListSync(uid=%d, mode=%d)\n",
            list_uid, sync_mode);
    }

    // ListSync syncs a specific list, but we don't track per-list
    // completion yet. Treat as DrawSync (all-lists sync).
    sched_yield_point();
    ctx->r[2] = render_queue_draw_sync(sync_mode);
    (void)rdram;
}

static void hle_sceGeDrawSync(
    uint8_t* rdram, recomp_context* ctx
) {
    int32_t sync_mode = ctx->r[4];

    // Yield before potentially blocking so other threads can run
    sched_yield_point();

    // mode=0: block until all GE lists processed (returns 0)
    // mode=1: poll -- returns 0 if all done, 1 if still processing
    ctx->r[2] = render_queue_draw_sync(sync_mode);
    (void)rdram;
}

static void hle_sceGeSetCallback(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = g_ge_cb_uid++;
    uint32_t ptr = static_cast<uint32_t>(ctx->r[4]);

    std::fprintf(stderr,
        "[HLE] sceGeSetCallback(ptr=0x%08X) -> uid=%d\n", ptr, uid);

    // sceGeCallbackData layout (PSP SDK):
    //   +0  signal_func (uint32)  -- called when SIGNAL GE cmd is encountered
    //   +4  signal_arg  (uint32)
    //   +8  finish_func (uint32)  -- called when a display list completes
    //   +12 finish_arg  (uint32)

    if (ptr != 0) {
        uint32_t signal_fn  = psp_mem_read<uint32_t>(rdram, (ptr + 0)  & PSP_ADDR_MASK);
        uint32_t signal_arg = psp_mem_read<uint32_t>(rdram, (ptr + 4)  & PSP_ADDR_MASK);
        uint32_t finish_fn  = psp_mem_read<uint32_t>(rdram, (ptr + 8)  & PSP_ADDR_MASK);
        uint32_t finish_arg = psp_mem_read<uint32_t>(rdram, (ptr + 12) & PSP_ADDR_MASK);
        ge_set_signal_callback(rdram, signal_fn, signal_arg, uid);
        ge_set_finish_callback(finish_fn, finish_arg);

        // Match PPSSPP behavior (Core/HLE/sceGe.cpp:497-505): sceGeSetCallback
        // auto-registers + enables sub-intr handlers for PSP_GE_INTR (25) sub-
        // intrs FINISH (subIntrBase|0) and SIGNAL (subIntrBase|1). Patapon's
        // user code at FUN_0881389C calls sceGeSetCallback then expects the GE
        // interrupt path to deliver finish/signal callbacks. Without these
        // RegisterSubIntr/EnableSubIntr calls, the runtime's HLE trace at
        // position 33-36 doesn't align with PPSSPP (Phase 11.5 DIV-A finding).
        // The shims (hle_sceKernelRegisterSubIntrHandler/EnableSubIntr) are
        // no-ops semantically — our direct ge_set_signal_callback +
        // ge_set_finish_callback above handle the actual dispatch — but
        // calling them here surfaces them in the HLE trace and keeps the
        // observable boot sequence in lock-step with PPSSPP.
        constexpr int PSP_GE_INTR = 25;
        constexpr int FINISH_SUBINTR = 0;
        constexpr int SIGNAL_SUBINTR = 1;
        // Resolve the current game's stub addresses by NID (issue #40):
        // NIDs are PSP-API-universal constants; stub addresses are per-game.
        uint32_t reg_stub =
            psp_hle_stub_addr_for_nid(0xCA04A2B9U);  // sceKernelRegisterSubIntrHandler
        uint32_t en_stub =
            psp_hle_stub_addr_for_nid(0xFB8E22ECU);  // sceKernelEnableSubIntr
        if (reg_stub == 0 || en_stub == 0) {
            static bool warned = false;
            if (!warned) {
                warned = true;
                std::fprintf(stderr,
                    "[GE] sceGeSetCallback: this game does not import "
                    "RegisterSubIntrHandler/EnableSubIntr (reg=0x%08X en=0x%08X) "
                    "— skipping the PPSSPP-aligned sub-intr replay\n",
                    reg_stub, en_stub);
            }
        }
        FuncPtr reg = reg_stub ? RECOMP_LOOKUP(reg_stub) : nullptr;
        FuncPtr en  = en_stub  ? RECOMP_LOOKUP(en_stub)  : nullptr;
        auto call = [&](FuncPtr fn, int a0, int a1, uint32_t a2, uint32_t a3) {
            if (fn == nullptr) return;
            recomp_context sub_ctx{};
            sub_ctx.r[4] = a0;
            sub_ctx.r[5] = a1;
            sub_ctx.r[6] = static_cast<int32_t>(a2);
            sub_ctx.r[7] = static_cast<int32_t>(a3);
            fn(rdram, &sub_ctx);
        };
        int subIntrBase = (uid & 0x7F) * 2;  // PPSSPP convention
        if (finish_fn != 0) {
            call(reg, PSP_GE_INTR, subIntrBase | FINISH_SUBINTR, finish_fn, finish_arg);
            call(en,  PSP_GE_INTR, subIntrBase | FINISH_SUBINTR, 0, 0);
        }
        if (signal_fn != 0) {
            call(reg, PSP_GE_INTR, subIntrBase | SIGNAL_SUBINTR, signal_fn, signal_arg);
            call(en,  PSP_GE_INTR, subIntrBase | SIGNAL_SUBINTR, 0, 0);
        }
    }

    ctx->r[2] = uid;
}

static void hle_sceGeUnsetCallback(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceGeEdramGetAddr(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = static_cast<int32_t>(GE_EDRAM_ADDR);
    (void)rdram;
}

static void hle_sceGeEdramGetSize(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = static_cast<int32_t>(GE_EDRAM_SIZE);
    (void)rdram;
}

static void hle_sceGeGetCmd(
    uint8_t* rdram, recomp_context* ctx
) {
    // No GE state yet; Phase 5 will implement
    ctx->r[2] = 0;
    (void)rdram;
}

static void hle_sceGeContinue(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceGeBreak(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceGeListUpdateStallAddr(
    uint8_t* rdram, recomp_context* ctx
) {
    int32_t list_uid = ctx->r[4];
    uint32_t new_stall = static_cast<uint32_t>(ctx->r[5]);

    static int update_count = 0;
    update_count++;
    if (update_count <= 5) {
        std::fprintf(stderr,
            "[HLE] sceGeListUpdateStallAddr(uid=%d, "
            "stall=0x%08X)\n",
            list_uid, new_stall);
    }

    render_queue_update_stall(list_uid, new_stall);
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

// ---- Registration ----

void psp_hle_register_ge() {
    psp_hle_register("sceGeListEnQueue",
                      hle_sceGeListEnQueue);
    psp_hle_register("sceGeListEnQueueHead",
                      hle_sceGeListEnQueueHead);
    psp_hle_register("sceGeListSync",
                      hle_sceGeListSync);
    psp_hle_register("sceGeDrawSync",
                      hle_sceGeDrawSync);
    psp_hle_register("sceGeSetCallback",
                      hle_sceGeSetCallback);
    psp_hle_register("sceGeUnsetCallback",
                      hle_sceGeUnsetCallback);
    psp_hle_register("sceGeEdramGetAddr",
                      hle_sceGeEdramGetAddr);
    psp_hle_register("sceGeEdramGetSize",
                      hle_sceGeEdramGetSize);
    psp_hle_register("sceGeGetCmd",
                      hle_sceGeGetCmd);
    psp_hle_register("sceGeContinue",
                      hle_sceGeContinue);
    psp_hle_register("sceGeBreak",
                      hle_sceGeBreak);
    psp_hle_register("sceGeListUpdateStallAddr",
                      hle_sceGeListUpdateStallAddr);
}
