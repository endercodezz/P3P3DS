#include "hle/psp_hle.h"
#include "psp_runtime.h"
#include "psp_scheduler.h"
#include "psp_memory.h"
#include "recomp.h"

#include <cstdio>
#include <chrono>

// ---- SceCtrlData layout (PSP SDK) ----
// offset 0: uint32_t timestamp
// offset 4: uint32_t buttons
// offset 8: uint8_t  analog_x  (128 = center)
// offset 9: uint8_t  analog_y  (128 = center)
// offset 10-15: reserved
// Total: 16 bytes per sample

static void write_ctrl_data(
    uint8_t* rdram, uint32_t ptr
) {
    // Timestamp: microseconds since boot
    auto now = std::chrono::steady_clock::now();
    auto us = std::chrono::duration_cast<
        std::chrono::microseconds>(
            now.time_since_epoch()).count();
    psp_mem_write<uint32_t>(rdram, ptr,
        static_cast<uint32_t>(us & 0xFFFFFFFF));

    // Buttons: host keyboard mask OR debug-socket injected mask
    // (injected mask only while its deadline has not expired).
    uint32_t buttons =
        g_host_buttons.load(std::memory_order_relaxed);
    auto now_ms = std::chrono::duration_cast<
        std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    if (now_ms < g_injected_buttons_deadline_ms.load(
            std::memory_order_relaxed)) {
        buttons |= g_injected_buttons.load(
            std::memory_order_relaxed);
    }
    psp_mem_write<uint32_t>(rdram, ptr + 4, buttons);

    // Analog stick: center (128, 128)
    psp_mem_write<uint8_t>(rdram, ptr + 8, 128);
    psp_mem_write<uint8_t>(rdram, ptr + 9, 128);

    // Reserved bytes: zero
    for (uint32_t i = 10; i < 16; i++) {
        psp_mem_write<uint8_t>(rdram, ptr + i, 0);
    }
}

// ---- HLE Functions ----

static void hle_sceCtrlPeekBufferPositive(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t data_ptr = static_cast<uint32_t>(ctx->r[4]);
    int32_t count = ctx->r[5];

    if (data_ptr != 0 && count > 0) {
        // Write count samples (16 bytes each)
        for (int32_t i = 0; i < count; i++) {
            write_ctrl_data(rdram, data_ptr + i * 16);
        }
    }

    ctx->r[2] = count;
}

static void hle_sceCtrlReadBufferPositive(
    uint8_t* rdram, recomp_context* ctx
) {
    sched_yield_point();
    hle_sceCtrlPeekBufferPositive(rdram, ctx);
}

static void hle_sceCtrlSetSamplingCycle(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceCtrlSetSamplingMode(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceCtrlSetIdleCancelThreshold(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

// ---- Registration ----

void psp_hle_register_ctrl() {
    psp_hle_register("sceCtrlPeekBufferPositive",
                      hle_sceCtrlPeekBufferPositive);
    psp_hle_register("sceCtrlReadBufferPositive",
                      hle_sceCtrlReadBufferPositive);
    psp_hle_register("sceCtrlSetSamplingCycle",
                      hle_sceCtrlSetSamplingCycle);
    psp_hle_register("sceCtrlSetSamplingMode",
                      hle_sceCtrlSetSamplingMode);
    psp_hle_register("sceCtrlSetIdleCancelThreshold",
                      hle_sceCtrlSetIdleCancelThreshold);
}
