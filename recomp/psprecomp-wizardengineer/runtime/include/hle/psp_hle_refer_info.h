#pragma once
// PSP-faithful Refer*Status guest-struct layouts and fill protocols.
// Oracle: PPSSPP Core/HLE/sceKernelThread.{h,cpp} (NativeThread,
// sceKernelReferThreadStatus:1125) and Core/HLE/sceKernelEventFlag.cpp
// (NativeEventFlag, sceKernelReferEventFlagStatus). Pure layout/encoding
// logic shared by psp_hle_kernel_thread.cpp / psp_hle_kernel_eventflag.cpp;
// unit-tested standalone by tests/test_refer_info.cpp (no SDL/GL deps).

#include <cstdint>
#include <cstring>
#include <cstddef>
#include <algorithm>

#include "recomp.h"  // psp_mem_read/psp_mem_write (inline, masked)

// ---- PSP thread status encodings (PPSSPP sceKernelThread.h:201) ----
constexpr uint32_t PSP_THREADSTATUS_RUNNING = 1;
constexpr uint32_t PSP_THREADSTATUS_READY   = 2;
constexpr uint32_t PSP_THREADSTATUS_WAIT    = 4;
constexpr uint32_t PSP_THREADSTATUS_SUSPEND = 8;
constexpr uint32_t PSP_THREADSTATUS_DORMANT = 16;
constexpr uint32_t PSP_THREADSTATUS_DEAD    = 32;

// ---- PSP wait type encodings (PPSSPP sceKernelThread.h:92) ----
constexpr uint32_t PSP_WAITTYPE_NONE      = 0;
constexpr uint32_t PSP_WAITTYPE_SLEEP     = 1;
constexpr uint32_t PSP_WAITTYPE_DELAY     = 2;
constexpr uint32_t PSP_WAITTYPE_SEMA      = 3;
constexpr uint32_t PSP_WAITTYPE_EVENTFLAG = 4;

// ---- PSP exit-status sentinels (PPSSPP Core/HLE/ErrorCodes.h) ----
constexpr int32_t PSP_ERROR_DORMANT     = (int32_t)0x800201A2U;
constexpr int32_t PSP_ERROR_NOT_DORMANT = (int32_t)0x800201A4U;

/// PSP encoding of one thread's (status, waitType) pair.
struct PspThreadStatusEncoding {
    uint32_t status;
    uint32_t wait_type;
};

/// Explicit map: internal scheduler ThreadStatus -> PSP encodings.
///
/// GUARD G1 (dothack-L6 verification): the internal enum (psp_scheduler.h:
/// DORMANT=0 READY=1 RUNNING=2 WAIT=3 DEAD=4 WAIT_SLEEP=5) is nearly the
/// inverse of the PSP encodings, and internal DEAD(4) collides with PSP
/// WAIT(4) — a cast-through would be silently wrong. Internal DEAD maps to
/// PSP DORMANT(16): PSP "dormant" = exited-but-not-deleted, which is what
/// game-side worker pumps treat as "worker exited".
///
/// waitType: only WAIT_SLEEP is distinguishable today (the scheduler has a
/// dedicated status for it — GUARD G6, no scheduler changes). A generic
/// internal WAIT reports WAITTYPE_NONE because the blocking primitive kind
/// is not tracked (sema/eventflag wait-id tracking is a later leg).
inline PspThreadStatusEncoding psp_encode_thread_status(int internal) {
    switch (internal) {
        case 0:  return {PSP_THREADSTATUS_DORMANT, PSP_WAITTYPE_NONE};
        case 1:  return {PSP_THREADSTATUS_READY,   PSP_WAITTYPE_NONE};
        case 2:  return {PSP_THREADSTATUS_RUNNING, PSP_WAITTYPE_NONE};
        case 3:  return {PSP_THREADSTATUS_WAIT,    PSP_WAITTYPE_NONE};
        case 4:  return {PSP_THREADSTATUS_DORMANT, PSP_WAITTYPE_NONE};
        case 5:  return {PSP_THREADSTATUS_WAIT,    PSP_WAITTYPE_SLEEP};
        default: return {PSP_THREADSTATUS_RUNNING, PSP_WAITTYPE_NONE};
    }
}

/// Host-side image of the guest SceKernelThreadInfo / NativeThread struct
/// (PPSSPP sceKernelThread.h:154). sizeof == 104; every offset pinned below.
struct PspNativeThreadImage {
    uint32_t size;                  // +0   (104 or 108 per SDK protocol)
    char     name[32];              // +4   NUL-terminated
    uint32_t attr;                  // +36
    uint32_t status;                // +40  PSP encoding
    uint32_t entry;                 // +44
    uint32_t stack;                 // +48
    uint32_t stack_size;            // +52
    uint32_t gp;                    // +56
    int32_t  init_priority;         // +60
    int32_t  current_priority;      // +64
    uint32_t wait_type;             // +68  PSP encoding
    int32_t  wait_id;               // +72
    int32_t  wakeup_count;          // +76
    int32_t  exit_status;           // +80
    uint32_t run_clocks_lo;         // +84
    uint32_t run_clocks_hi;         // +88
    int32_t  intr_preempt_count;    // +92
    int32_t  thread_preempt_count;  // +96
    int32_t  release_count;         // +100
};
static_assert(sizeof(PspNativeThreadImage) == 104, "NativeThread is 104B");
static_assert(offsetof(PspNativeThreadImage, name) == 4, "name at +4");
static_assert(offsetof(PspNativeThreadImage, attr) == 36, "attr at +36");
static_assert(offsetof(PspNativeThreadImage, status) == 40, "status at +40");
static_assert(offsetof(PspNativeThreadImage, entry) == 44, "entry at +44");
static_assert(offsetof(PspNativeThreadImage, stack) == 48, "stack at +48");
static_assert(offsetof(PspNativeThreadImage, current_priority) == 64,
              "currentPriority at +64");
static_assert(offsetof(PspNativeThreadImage, wait_type) == 68,
              "waitType at +68");
static_assert(offsetof(PspNativeThreadImage, wait_id) == 72, "waitID at +72");
static_assert(offsetof(PspNativeThreadImage, exit_status) == 80,
              "exitStatus at +80");
static_assert(offsetof(PspNativeThreadImage, run_clocks_lo) == 84,
              "runClocks at +84");
static_assert(offsetof(PspNativeThreadImage, release_count) == 100,
              "releaseCount at +100");

/// Block write into guest memory with recomp.h psp_mem_write semantics:
/// NULL-page (< 0x10000) and out-of-range writes are discarded.
inline void psp_mem_write_block(
    uint8_t* rdram, uint32_t addr, const void* src, uint32_t len
) {
    if (addr < 0x00010000U || len == 0) {
        return;
    }
    uint32_t off = addr & 0x07FFFFFFU;
    if (off + len > 0x08000000U) {
        return;
    }
    std::memcpy(rdram + off, src, len);
}

/// PPSSPP sceKernelReferThreadStatus copy protocol (sceKernelThread.cpp:1125).
/// Reads the caller's wantedSize from info_ptr+0 and size-gates the copy:
///   SDK > 2.60: wantedSize > 108 -> SCE_KERNEL_ERROR_ILLEGAL_SIZE (no write);
///     size field reads back 108; copy min(wantedSize, 104) bytes; zero-fill
///     bytes 104..wantedSize. wantedSize == 0 writes nothing.
///   SDK <= 2.60 (or never set): size field reads back 104; copy
///     min(wantedSize, 104) bytes; no tail fill, no size error.
/// The caller's size value is never stomped beyond this PPSSPP-faithful
/// write-back (GUARD G2). Returns 0 or SCE_KERNEL_ERROR_ILLEGAL_SIZE.
inline int32_t psp_write_thread_info(
    uint8_t* rdram, uint32_t info_ptr, bool sdk_after_260,
    PspNativeThreadImage img
) {
    constexpr int32_t kIllegalSize = (int32_t)0x800201BCU;
    if (info_ptr == 0) {
        return 0;  // PPSSPP: Read_U32(0) yields 0 -> nothing copied
    }
    uint32_t wanted = psp_mem_read<uint32_t>(rdram, info_ptr);
    if (sdk_after_260) {
        if (wanted > 108) {
            return kIllegalSize;
        }
        img.size = 108;
        psp_mem_write_block(rdram, info_ptr, &img,
                            std::min<uint32_t>(wanted, sizeof(img)));
        for (uint32_t off = sizeof(img); off < wanted; off++) {
            psp_mem_write<uint8_t>(rdram, info_ptr + off, 0);
        }
    } else {
        img.size = 104;
        psp_mem_write_block(rdram, info_ptr, &img,
                            std::min<uint32_t>(wanted, sizeof(img)));
    }
    return 0;
}

/// Host-side image of the guest SceKernelEventFlagInfo / NativeEventFlag
/// struct (PPSSPP sceKernelEventFlag.cpp:35). sizeof == 52.
struct PspNativeEventFlagImage {
    uint32_t size;              // +0   always 52
    char     name[32];          // +4
    uint32_t attr;              // +36
    uint32_t init_pattern;      // +40
    uint32_t current_pattern;   // +44
    int32_t  num_wait_threads;  // +48
};
static_assert(sizeof(PspNativeEventFlagImage) == 52,
              "NativeEventFlag is 52B");
static_assert(offsetof(PspNativeEventFlagImage, attr) == 36, "attr at +36");
static_assert(offsetof(PspNativeEventFlagImage, init_pattern) == 40,
              "initPattern at +40");
static_assert(offsetof(PspNativeEventFlagImage, current_pattern) == 44,
              "currentPattern at +44");
static_assert(offsetof(PspNativeEventFlagImage, num_wait_threads) == 48,
              "numWaitThreads at +48");

/// PPSSPP sceKernelReferEventFlagStatus copy protocol: the full 52-byte
/// NativeEventFlag is copied only when the caller's size field at info_ptr+0
/// is non-zero (sceKernelEventFlag.cpp:532); size reads back 52.
inline void psp_write_eventflag_info(
    uint8_t* rdram, uint32_t info_ptr, const PspNativeEventFlagImage& img
) {
    if (info_ptr == 0) {
        return;
    }
    if (psp_mem_read<uint32_t>(rdram, info_ptr) != 0) {
        psp_mem_write_block(rdram, info_ptr, &img, sizeof(img));
    }
}
