// psp_hle_kernel_lwmutex.cpp — sceKernel*LwMutex HLE family.
//
// PPSSPP-faithful semantics (Core/HLE/sceKernelMutex.cpp): an LwMutex's
// state lives in a GUEST-memory "workarea" (NativeLwMutexWorkarea) — the
// lock count, owner thread UID, attributes and kernel-object uid are words
// the game itself reads — while a host-side kernel object exists only for
// contention (waiter blocking/wakeup). Every state decision here reads and
// writes the workarea so guest-visible state stays authoritative.
//
// Workarea layout (32 bytes):
//   +0  lockLevel   (s32)  0 = unlocked; else recursive lock count
//   +4  lockThread  (s32)  owner thread UID; 0 = none; -1 = cleared/deleted
//   +8  attr        (u32)  PSP_MUTEX_ATTR_* flags
//   +12 numWaitThreads (s32, not kept live — PPSSPP refreshes it lazily)
//   +16 uid         (s32)  kernel object id; -1 = invalid/deleted
//   +20 pad[3]
//
// PPSSPP serializes all transitions on its single MIPS core; we serialize
// on one host mutex (g_lw_mtx) instead, which all six entry points take.
// Blocking integrates with the cooperative scheduler exactly like the
// sema/eventflag code: sched_yield_point() at entry, psp_thread_note_wait
// tagging, cv waits with a 5 s safety valve and a g_should_exit escape.

#include "hle/psp_hle.h"
#include "hle/psp_hle_kernel.h"
#include "psp_scheduler.h"
#include "psp_memory.h"
#include "recomp.h"

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace {

// Workarea field offsets (NativeLwMutexWorkarea).
constexpr uint32_t WA_LOCK_LEVEL  = 0;
constexpr uint32_t WA_LOCK_THREAD = 4;
constexpr uint32_t WA_ATTR        = 8;
constexpr uint32_t WA_UID         = 16;
constexpr uint32_t WA_SIZE        = 32;

constexpr uint32_t PSP_MUTEX_ATTR_RECURSIVE = 0x200;

/// Host kernel object: contention only (the state is in the workarea).
struct PspLwMutex {
    int uid = -1;
    char name[32] = {};
    uint32_t workarea = 0;
    bool deleted = false;
    std::condition_variable cv;  // waits on g_lw_mtx
};

// One mutex serializes ALL LwMutex state transitions (the host stand-in
// for PPSSPP's single-core atomicity). cv waits use this same mutex.
std::mutex g_lw_mtx;
std::unordered_map<int, std::shared_ptr<PspLwMutex>> g_lwmutexes;

std::shared_ptr<PspLwMutex> lw_find_locked(int uid) {
    auto it = g_lwmutexes.find(uid);
    return it == g_lwmutexes.end() ? nullptr : it->second;
}

/// PPSSPP's __KernelLockLwMutex fast path. Returns true when acquired
/// (workarea updated). On false: *err is an SCE error, or 0 = contended
/// (caller should block). Caller holds g_lw_mtx.
bool lw_fast_lock(uint8_t* rdram, uint32_t wa, int32_t count, int32_t* err) {
    int32_t lock_level  = psp_mem_read<int32_t>(rdram, wa + WA_LOCK_LEVEL);
    int32_t lock_thread = psp_mem_read<int32_t>(rdram, wa + WA_LOCK_THREAD);
    uint32_t attr = psp_mem_read<uint32_t>(rdram, wa + WA_ATTR);
    int32_t uid   = psp_mem_read<int32_t>(rdram, wa + WA_UID);

    *err = 0;
    if (count <= 0) {
        *err = SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    } else if (count > 1 && !(attr & PSP_MUTEX_ATTR_RECURSIVE)) {
        *err = SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    } else if (count + lock_level < 0) {
        *err = SCE_LWMUTEX_ERROR_LOCK_OVERFLOW;
    } else if (uid == -1) {
        *err = SCE_LWMUTEX_ERROR_NO_SUCH_LWMUTEX;
    }
    if (*err != 0) {
        return false;
    }

    int32_t cur = psp_current_thread_uid();
    if (lock_level == 0) {
        // PPSSPP validates the kernel object when a stale owner is recorded.
        if (lock_thread != 0 && lw_find_locked(uid) == nullptr) {
            *err = SCE_LWMUTEX_ERROR_NO_SUCH_LWMUTEX;
            return false;
        }
        psp_mem_write<int32_t>(rdram, wa + WA_LOCK_LEVEL, count);
        psp_mem_write<int32_t>(rdram, wa + WA_LOCK_THREAD, cur);
        return true;
    }
    if (lock_thread == cur) {
        if (attr & PSP_MUTEX_ATTR_RECURSIVE) {
            psp_mem_write<int32_t>(rdram, wa + WA_LOCK_LEVEL,
                                   lock_level + count);
            return true;
        }
        *err = SCE_LWMUTEX_ERROR_ALREADY_LOCKED;
        return false;
    }
    return false;  // contended
}

/// Blocking tail of Lock/LockCB: waits on the kernel object's cv until the
/// fast path succeeds, the LwMutex is deleted, the (optional) guest timeout
/// expires, or shutdown. Caller holds `lock` on g_lw_mtx.
void lw_lock_blocking(uint8_t* rdram, recomp_context* ctx,
                      std::unique_lock<std::mutex>& lock,
                      uint32_t wa, int32_t count, uint32_t timeout_ptr) {
    int32_t uid = psp_mem_read<int32_t>(rdram, wa + WA_UID);
    auto m = lw_find_locked(uid);
    if (!m) {
        ctx->r[2] = SCE_LWMUTEX_ERROR_NO_SUCH_LWMUTEX;
        return;
    }

    char tag[24];
    std::snprintf(tag, sizeof(tag), "lwmutex:%d", uid);
    psp_thread_note_wait(tag);

    const bool timed = (timeout_ptr != 0);

    // Tiny-timeout rounding: PPSSPP __KernelWaitLwMutex (sceKernelMutex.cpp
    // lines 857-863) rounds very short timeouts up before scheduling the wait
    // timer so the hardware-observable minimum granularity is preserved.
    uint32_t raw_us = timed ? psp_mem_read<uint32_t>(rdram, timeout_ptr) : 0;
    if (timed) {
        if (raw_us <= 3)
            raw_us = 25;
        else if (raw_us <= 249)
            raw_us = 250;
    }
    auto deadline = std::chrono::steady_clock::now() +
        std::chrono::microseconds(raw_us);

    int32_t err = 0;
    for (;;) {
        if (lw_fast_lock(rdram, wa, count, &err)) {
            // Write remaining time back through the timeout pointer on
            // successful acquisition — matches PPSSPP's
            // __KernelUnlockLwMutexForThread (sceKernelMutex.cpp lines
            // 726-731) which unconditionally writes cyclesLeft->us on
            // both the acquire and timeout completion paths.
            if (timed) {
                auto remaining = deadline - std::chrono::steady_clock::now();
                uint32_t us_left = (remaining.count() > 0)
                    ? static_cast<uint32_t>(
                          std::chrono::duration_cast<std::chrono::microseconds>(
                              remaining).count())
                    : 0u;
                psp_mem_write<uint32_t>(rdram, timeout_ptr, us_left);
            }
            ctx->r[2] = SCE_OK;
            break;
        }
        if (err == SCE_LWMUTEX_ERROR_NO_SUCH_LWMUTEX && m->deleted) {
            ctx->r[2] = SCE_KERNEL_ERROR_WAIT_DELETE;
            break;
        }
        if (err != 0) {
            ctx->r[2] = err;
            break;
        }
        auto now = std::chrono::steady_clock::now();
        if (g_should_exit.load() || (timed && now >= deadline)) {
            if (timed) {
                psp_mem_write<uint32_t>(rdram, timeout_ptr, 0);
            }
            ctx->r[2] = SCE_KERNEL_ERROR_WAIT_TIMEOUT;
            break;
        }
        // 5 s safety valve (sema/eventflag convention) so shutdown and
        // missed notifies cannot wedge the host thread forever.
        auto valve = std::chrono::seconds(5);
        if (timed && deadline - now < valve) {
            m->cv.wait_until(lock, deadline);
        } else {
            m->cv.wait_for(lock, valve);
        }
    }
    psp_thread_clear_wait();
}

// ---- HLE Functions ----

// sceKernelCreateLwMutex(workarea, name, attr, initialCount, optionsPtr).
// The 5th argument arrives in t0 (r8): PSP syscall stubs pass up to 8 args
// in $a0-$t3.
//
// optionsPtr handling: PPSSPP (sceKernelMutex.cpp line 701-705) reads the
// size word at optionsPtr and emits WARN_LOG_REPORT if size > 4; it does NOT
// read or act on any remaining option bytes. We match that behavior: if
// optionsPtr is non-zero we read the size word and ignore the options block
// entirely (no warning log in our HLE since we have no sceKernel log channel).
void hle_sceKernelCreateLwMutex(uint8_t* rdram, recomp_context* ctx) {
    uint32_t wa = static_cast<uint32_t>(ctx->r[4]);
    uint32_t name_ptr = static_cast<uint32_t>(ctx->r[5]);
    uint32_t attr = static_cast<uint32_t>(ctx->r[6]);
    int32_t init_count = ctx->r[7];
    uint32_t options_ptr = static_cast<uint32_t>(ctx->r[8]);  // $t0

    if (name_ptr == 0) {
        ctx->r[2] = SCE_KERNEL_ERROR_ERROR;
        return;
    }
    if (attr >= 0x400) {
        ctx->r[2] = SCE_KERNEL_ERROR_ILLEGAL_ATTR;
        return;
    }
    if (init_count < 0 ||
        ((attr & PSP_MUTEX_ATTR_RECURSIVE) == 0 && init_count > 1)) {
        ctx->r[2] = SCE_KERNEL_ERROR_ILLEGAL_COUNT;
        return;
    }

    // Read (and discard) the options size word — matches PPSSPP lines 701-705.
    if (options_ptr != 0) {
        (void)psp_mem_read<uint32_t>(rdram, options_ptr);
    }

    std::unique_lock<std::mutex> lock(g_lw_mtx);
    auto m = std::make_shared<PspLwMutex>();
    m->uid = psp_next_uid();
    m->workarea = wa;
    const char* name = reinterpret_cast<const char*>(
        rdram + (name_ptr & PSP_ADDR_MASK));
    std::strncpy(m->name, name, sizeof(m->name) - 1);

    for (uint32_t off = 0; off < WA_SIZE; off += 4) {
        psp_mem_write<uint32_t>(rdram, wa + off, 0);
    }
    psp_mem_write<int32_t>(rdram, wa + WA_LOCK_LEVEL, init_count);
    psp_mem_write<int32_t>(rdram, wa + WA_LOCK_THREAD,
        init_count == 0 ? 0 : psp_current_thread_uid());
    psp_mem_write<uint32_t>(rdram, wa + WA_ATTR, attr);
    psp_mem_write<int32_t>(rdram, wa + WA_UID, m->uid);

    g_lwmutexes[m->uid] = std::move(m);
    ctx->r[2] = SCE_OK;  // uid lives in the workarea, not in v0
}

void hle_sceKernelDeleteLwMutex(uint8_t* rdram, recomp_context* ctx) {
    uint32_t wa = static_cast<uint32_t>(ctx->r[4]);
    if (wa == 0) {
        ctx->r[2] = SCE_KERNEL_ERROR_ILLEGAL_ADDR;
        return;
    }

    std::unique_lock<std::mutex> lock(g_lw_mtx);
    int32_t uid = psp_mem_read<int32_t>(rdram, wa + WA_UID);
    auto it = g_lwmutexes.find(uid);
    if (it == g_lwmutexes.end()) {
        ctx->r[2] = SCE_LWMUTEX_ERROR_NO_SUCH_LWMUTEX;
        return;
    }

    // workarea->clear() + wake every waiter (they observe deleted=true and
    // return SCE_KERNEL_ERROR_WAIT_DELETE from the blocking loop).
    psp_mem_write<int32_t>(rdram, wa + WA_LOCK_LEVEL, 0);
    psp_mem_write<int32_t>(rdram, wa + WA_LOCK_THREAD, -1);
    psp_mem_write<int32_t>(rdram, wa + WA_UID, -1);
    it->second->deleted = true;
    it->second->cv.notify_all();
    g_lwmutexes.erase(it);
    ctx->r[2] = SCE_OK;
}

void hle_sceKernelLockLwMutex(uint8_t* rdram, recomp_context* ctx) {
    uint32_t wa = static_cast<uint32_t>(ctx->r[4]);
    int32_t count = ctx->r[5];
    uint32_t timeout_ptr = static_cast<uint32_t>(ctx->r[6]);

    sched_yield_point();

    if (wa == 0) {
        ctx->r[2] = SCE_KERNEL_ERROR_ACCESS_ERROR;
        return;
    }

    std::unique_lock<std::mutex> lock(g_lw_mtx);
    int32_t err = 0;
    if (lw_fast_lock(rdram, wa, count, &err)) {
        ctx->r[2] = SCE_OK;
        return;
    }
    if (err != 0) {
        ctx->r[2] = err;
        return;
    }
    lw_lock_blocking(rdram, ctx, lock, wa, count, timeout_ptr);
}

void hle_sceKernelLockLwMutexCB(uint8_t* rdram, recomp_context* ctx) {
    psp_kernel_check_callbacks(rdram, ctx);
    hle_sceKernelLockLwMutex(rdram, ctx);
}

void hle_sceKernelTryLockLwMutex(uint8_t* rdram, recomp_context* ctx) {
    uint32_t wa = static_cast<uint32_t>(ctx->r[4]);
    int32_t count = ctx->r[5];

    if (wa == 0) {
        ctx->r[2] = SCE_KERNEL_ERROR_ACCESS_ERROR;
        return;
    }

    std::unique_lock<std::mutex> lock(g_lw_mtx);
    int32_t err = 0;
    if (lw_fast_lock(rdram, wa, count, &err)) {
        ctx->r[2] = SCE_OK;
    } else {
        // Base (non-_600) variant returns TRYLOCK_FAILED for every failure.
        ctx->r[2] = SCE_MUTEX_ERROR_TRYLOCK_FAILED;
    }
}

void hle_sceKernelUnlockLwMutex(uint8_t* rdram, recomp_context* ctx) {
    uint32_t wa = static_cast<uint32_t>(ctx->r[4]);
    int32_t count = ctx->r[5];

    if (wa == 0) {
        ctx->r[2] = SCE_KERNEL_ERROR_ACCESS_ERROR;
        return;
    }

    std::unique_lock<std::mutex> lock(g_lw_mtx);
    int32_t lock_level  = psp_mem_read<int32_t>(rdram, wa + WA_LOCK_LEVEL);
    int32_t lock_thread = psp_mem_read<int32_t>(rdram, wa + WA_LOCK_THREAD);
    uint32_t attr = psp_mem_read<uint32_t>(rdram, wa + WA_ATTR);
    int32_t uid   = psp_mem_read<int32_t>(rdram, wa + WA_UID);

    if (uid == -1) {
        ctx->r[2] = SCE_LWMUTEX_ERROR_NO_SUCH_LWMUTEX;
        return;
    }
    if (count <= 0 ||
        ((attr & PSP_MUTEX_ATTR_RECURSIVE) == 0 && count > 1)) {
        ctx->r[2] = SCE_KERNEL_ERROR_ILLEGAL_COUNT;
        return;
    }
    if (lock_level == 0 || lock_thread != psp_current_thread_uid()) {
        ctx->r[2] = SCE_LWMUTEX_ERROR_NOT_LOCKED;
        return;
    }
    if (lock_level < count) {
        ctx->r[2] = SCE_LWMUTEX_ERROR_UNLOCK_UNDERFLOW;
        return;
    }

    lock_level -= count;
    psp_mem_write<int32_t>(rdram, wa + WA_LOCK_LEVEL, lock_level);
    if (lock_level == 0) {
        psp_mem_write<int32_t>(rdram, wa + WA_LOCK_THREAD, 0);
        if (auto m = lw_find_locked(uid)) {
            m->cv.notify_all();
        }
    }
    ctx->r[2] = SCE_OK;
}

}  // namespace

// ---- Registration ----

void psp_hle_register_kernel_lwmutex() {
    psp_hle_register("sceKernelCreateLwMutex",
                      hle_sceKernelCreateLwMutex);
    psp_hle_register("sceKernelDeleteLwMutex",
                      hle_sceKernelDeleteLwMutex);
    psp_hle_register("sceKernelLockLwMutex",
                      hle_sceKernelLockLwMutex);
    psp_hle_register("sceKernelLockLwMutexCB",
                      hle_sceKernelLockLwMutexCB);
    psp_hle_register("sceKernelTryLockLwMutex",
                      hle_sceKernelTryLockLwMutex);
    psp_hle_register("sceKernelUnlockLwMutex",
                      hle_sceKernelUnlockLwMutex);
}
