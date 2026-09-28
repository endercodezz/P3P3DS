#include "hle/psp_hle.h"
#include "hle/psp_hle_kernel.h"
#include "psp_scheduler.h"
#include "psp_memory.h"
#include "recomp.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <chrono>
#include <thread>

static std::unordered_map<int, std::unique_ptr<PspSemaphore>> g_semaphores;

// ---- Debug trace gating ----
// The per-call WaitSema/Signal logs produced ~1GB/min during stalls.
// Gate them behind PSPRECOMP_SEMA_TRACE=1 (getenv cached once, matching
// the PSPRECOMP_HLE_TRACE pattern in psp_hle_dispatch.cpp).
static bool sema_trace_enabled() {
    static const bool enabled = [] {
        const char* e = std::getenv("PSPRECOMP_SEMA_TRACE");
        return e != nullptr && e[0] == '1';
    }();
    return enabled;
}

// ---- FIFO waiter handoff helpers (issue #29) ----
// SignalSema transfers the count directly to queued waiters in FIFO
// order (PPSSPP-equivalent direct handoff; FIFO is correct for the
// attr=0 semaphores Patapon creates). Without this, the polling main
// thread steals the count every time and the PCM thread starves
// forever on 'sgx-psp-sas-attrsema'.

/// Drain the FIFO wait queue: transfer count to waiters in order while
/// it suffices, then wake everyone. Caller must hold s.mtx.
static void sema_grant_waiters(PspSemaphore& s) {
    while (!s.waiters.empty()
           && s.current_count >= s.waiters.front()->need) {
        SemaWaiter* w = s.waiters.front();
        s.current_count -= w->need;
        w->granted = true;
        s.waiters.pop_front();
    }
    s.cv.notify_all();
}

/// Remove a (non-granted) waiter from the queue, e.g. on timeout or
/// shutdown. Caller must hold s.mtx.
static void sema_remove_waiter(PspSemaphore& s, SemaWaiter* w) {
    for (auto it = s.waiters.begin(); it != s.waiters.end(); ++it) {
        if (*it == w) {
            s.waiters.erase(it);
            return;
        }
    }
}

/// STUCK warning, rate-limited to once per uid per 30s.
/// Caller must hold s.mtx.
static void sema_warn_stuck(PspSemaphore& s, int uid, int32_t signal) {
    auto now = std::chrono::steady_clock::now();
    if (now - s.last_stuck_warn < std::chrono::seconds(30)) {
        return;
    }
    s.last_stuck_warn = now;
    std::fprintf(stderr,
        "[HLE] sceKernelWaitSema STUCK: uid=%d '%s' "
        "count=%d signal=%d (still waiting)\n",
        uid, s.name, s.current_count, signal);
}

// ---- HLE Functions ----

static void hle_sceKernelCreateSema(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t name_ptr = static_cast<uint32_t>(ctx->r[4]);
    int32_t attr = ctx->r[5];
    int32_t init_count = ctx->r[6];
    int32_t max_count = ctx->r[7];

    (void)attr;

    auto sema = std::make_unique<PspSemaphore>();
    sema->uid = psp_next_uid();
    sema->current_count = init_count;
    sema->init_count = init_count;
    sema->max_count = max_count;
    std::memset(sema->name, 0, sizeof(sema->name));

    if (name_ptr != 0) {
        const char* name = reinterpret_cast<const char*>(
            rdram + (name_ptr & PSP_ADDR_MASK));
        std::strncpy(sema->name, name, sizeof(sema->name) - 1);
    }

    int uid = sema->uid;
    g_semaphores[uid] = std::move(sema);

    ctx->r[2] = uid;
}

static void hle_sceKernelDeleteSema(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    auto it = g_semaphores.find(uid);
    if (it == g_semaphores.end()) {
        ctx->r[2] = SCE_KERNEL_ERROR_SEMA_ZERO;
        (void)rdram;
        return;
    }
    it->second->deleted = true;
    std::fprintf(stderr,
        "[HLE] sceKernelDeleteSema(uid=%d) -> soft-delete\n",
        uid);
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

extern thread_local uint32_t g_last_func_addr;

static void hle_sceKernelSignalSema(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    int32_t signal = ctx->r[5];

    // Trace all direct Signal(259) calls to find the source
    if (uid == 259 && sema_trace_enabled()) {
        static int direct_count = 0;
        direct_count++;
        if (direct_count <= 30) {
            std::fprintf(stderr,
                "[SIGNAL259] direct call #%d caller=0x%08X\n",
                direct_count, g_last_func_addr);
        }
    }

    auto it = g_semaphores.find(uid);
    if (it == g_semaphores.end()) {
        // Tolerate garbage UIDs from uninitialized data structures.
        // The game reads semaphore UIDs from memory that was never
        // written by PRX init code. Returning an error causes the
        // game to enter an infinite stall loop.
        // Warn once per unique uid to reduce log spam.
        static std::unordered_set<int32_t> warned_signal_uids;
        if (warned_signal_uids.find(uid) == warned_signal_uids.end()) {
            uint32_t deref0 = 0;
            uint32_t deref4 = 0;
            uint32_t deref0_0 = 0;
            uint32_t deref0_4 = 0;
            uint32_t deref0_0_0 = 0;
            uint32_t deref0_0_4 = 0;
            if (static_cast<uint32_t>(uid) >= 0x08000000U
                    && static_cast<uint32_t>(uid) < 0x0C000000U) {
                deref0 = psp_mem_read<uint32_t>(
                    rdram, static_cast<uint32_t>(uid) + 0U);
                deref4 = psp_mem_read<uint32_t>(
                    rdram, static_cast<uint32_t>(uid) + 4U);
                if (deref0 >= 0x08000000U && deref0 < 0x0C000000U) {
                    deref0_0 = psp_mem_read<uint32_t>(rdram, deref0 + 0U);
                    deref0_4 = psp_mem_read<uint32_t>(rdram, deref0 + 4U);
                    if (deref0_0 >= 0x08000000U && deref0_0 < 0x0C000000U) {
                        deref0_0_0 = psp_mem_read<uint32_t>(rdram, deref0_0 + 0U);
                        deref0_0_4 = psp_mem_read<uint32_t>(rdram, deref0_0 + 4U);
                    }
                }
            }
            std::fprintf(stderr,
                "[HLE] sceKernelSignalSema: unknown uid=0x%08X "
                "caller=0x%08X a0=0x%08X *uid=0x%08X uid+4=0x%08X "
                "**uid=0x%08X **uid+4=0x%08X "
                "***uid=0x%08X ***uid+4=0x%08X (tolerating as no-op)\n",
                static_cast<uint32_t>(uid),
                g_last_func_addr,
                static_cast<uint32_t>(ctx->r[4]),
                deref0, deref4, deref0_0, deref0_4, deref0_0_0, deref0_0_4);
            warned_signal_uids.insert(uid);
        }
        ctx->r[2] = SCE_OK;
        (void)rdram;
        return;
    }

    auto& s = it->second;
    if (s->deleted) {
        static bool once = false;
        if (!once) {
            std::fprintf(stderr,
                "[HLE] sceKernelSignalSema: operating on "
                "soft-deleted uid=%d ('%s') -- kept alive "
                "for cross-thread safety\n",
                uid, s->name);
            once = true;
        }
    }
    std::unique_lock<std::mutex> lock(s->mtx);

    if (s->current_count + signal > s->max_count) {
        ctx->r[2] = SCE_KERNEL_ERROR_SEMA_OVERFLOW;
        (void)rdram;
        return;
    }

    s->current_count += signal;

    // Log sema 271 signals to trace completion pipelines.
    // (Before the handoff drain so the logged count is the post-signal,
    // pre-transfer value.)
    if (uid == 271 && sema_trace_enabled()) {
        std::fprintf(stderr,
            "[SEMA271] Signal count=%d->%d caller=0x%08X\n",
            s->current_count - signal, s->current_count,
            g_last_func_addr);
    }

    // Direct handoff: transfer the count to queued waiters (FIFO),
    // then wake everyone (issue #29).
    sema_grant_waiters(*s);

    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelWaitSema(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    int32_t signal = ctx->r[5];
    uint32_t timeout_ptr = static_cast<uint32_t>(ctx->r[6]);

    sched_yield_point();

    auto it = g_semaphores.find(uid);
    if (it == g_semaphores.end()) {
        // Tolerate garbage UIDs — return success immediately.
        // See SignalSema comment for rationale.
        // Warn once per unique uid to reduce log spam.
        static std::unordered_set<int32_t> warned_wait_uids;
        if (warned_wait_uids.find(uid) == warned_wait_uids.end()) {
            std::fprintf(stderr,
                "[HLE] sceKernelWaitSema: unknown uid=0x%08X "
                "(tolerating as no-op)\n",
                static_cast<uint32_t>(uid));
            warned_wait_uids.insert(uid);
        }
        ctx->r[2] = SCE_OK;
        return;
    }

    auto& s = it->second;
    if (s->deleted) {
        // Soft-deleted semaphore: use 100ms timeout to avoid permanent deadlock
        // if the Signal side was also deleted and never fires.
        static bool once = false;
        if (!once) {
            std::fprintf(stderr,
                "[HLE] sceKernelWaitSema: operating on "
                "soft-deleted uid=%d ('%s') -- using 100ms timeout\n",
                uid, s->name);
            once = true;
        }
        std::unique_lock<std::mutex> lock(s->mtx);
        s->wait_count++;
        bool got_it = s->cv.wait_for(lock,
            std::chrono::milliseconds(100),
            [&] { return s->current_count >= signal
                         && s->waiters.empty(); });
        if (got_it) {
            s->current_count -= signal;
        }
        s->wait_count--;
        ctx->r[2] = SCE_OK;
        return;
    }

    if (uid == 259 && sema_trace_enabled()) {
        static int wait259_count = 0;
        wait259_count++;
        if (wait259_count <= 30) {
            std::fprintf(stderr,
                "[WAIT259] #%d caller=0x%08X count=%d signal=%d\n",
                wait259_count, g_last_func_addr,
                s->current_count, signal);
        }
    }
    if (sema_trace_enabled()) {
        std::fprintf(stderr,
            "[HLE] sceKernelWaitSema(uid=%d '%s' count=%d signal=%d)\n",
            uid, s->name, s->current_count, signal);
    }

    std::unique_lock<std::mutex> lock(s->mtx);

    // Fast path: take immediately ONLY if no one is queued ahead of us
    // (no queue jumping -- preserves FIFO fairness, issue #29).
    if (s->current_count >= signal && s->waiters.empty()) {
        s->current_count -= signal;
        ctx->r[2] = SCE_OK;
        return;
    }

    // Slow path: enqueue a stack-allocated waiter. SignalSema transfers
    // the count to us directly (granted=true), so on the granted path we
    // must NOT decrement current_count again.
    SemaWaiter waiter{signal, false};
    s->waiters.push_back(&waiter);

    // [#35] Tag this thread's wait for the debug socket I command.
    {
        char wait_tag[24];
        std::snprintf(wait_tag, sizeof(wait_tag), "sema:%d", uid);
        psp_thread_note_wait(wait_tag);
    }

    s->wait_count++;
    if (timeout_ptr != 0) {
        uint32_t timeout_us = psp_mem_read<uint32_t>(
            rdram, timeout_ptr);
        auto timeout = std::chrono::microseconds(timeout_us);
        s->cv.wait_for(lock, timeout,
            [&] { return waiter.granted || g_should_exit.load(); });
        if (!waiter.granted) {
            // Timed out (or shutting down): remove our own waiter.
            // If granted flipped during the race, it was already popped
            // by the signaller and we fall through as acquired.
            sema_remove_waiter(*s, &waiter);
            s->wait_count--;
            psp_thread_clear_wait();
            ctx->r[2] = SCE_KERNEL_ERROR_WAIT_TIMEOUT;
            return;
        }
    } else {
        // Unbounded wait: 5-second safety valve + g_should_exit escape
        // so threads can be cleaned up on shutdown.
        while (!waiter.granted && !g_should_exit.load()) {
            s->cv.wait_for(lock, std::chrono::seconds(5),
                [&] { return waiter.granted || g_should_exit.load(); });
            if (!waiter.granted && !g_should_exit.load()) {
                sema_warn_stuck(*s, uid, signal);
            }
        }
        if (!waiter.granted) {
            // g_should_exit: bail without consuming the resource.
            sema_remove_waiter(*s, &waiter);
            s->wait_count--;
            psp_thread_clear_wait();
            ctx->r[2] = SCE_KERNEL_ERROR_WAIT_TIMEOUT;
            return;
        }
    }
    s->wait_count--;
    psp_thread_clear_wait();

    // Granted: the signaller already transferred the count to us --
    // do NOT decrement current_count here.
    if (uid == 271 && sema_trace_enabled()) {
        std::fprintf(stderr,
            "[SEMA271] WaitSema acquired count_after=%d caller=0x%08X\n",
            s->current_count, g_last_func_addr);
    }
    ctx->r[2] = SCE_OK;
}

static void hle_sceKernelWaitSemaCB(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    int32_t signal = ctx->r[5];

    // CB variant: process callbacks continuously while waiting.
    // On real PSP, WaitSemaCB checks for pending callbacks in a loop:
    //   1. Dispatch any pending callbacks
    //   2. Try to acquire the semaphore (non-blocking)
    //   3. If acquired, return SCE_OK
    //   4. If not, yield and loop
    // This is critical for the game's asset pipeline: I/O completion
    // callbacks fire during the wait and eventually signal the semaphore.

    auto it = g_semaphores.find(uid);
    if (it == g_semaphores.end()) {
        psp_kernel_check_callbacks(rdram, ctx);
        ctx->r[2] = SCE_OK;
        return;
    }

    auto& s = it->second;

    // WaitSemaCB: PSP semantics — dispatch pending callbacks while
    // waiting for the semaphore.  The key constraint is that callback
    // dispatch MUST happen (IO completion callbacks advance the file
    // pipeline and may eventually signal this semaphore).
    //
    // Implementation:
    //   1. Dispatch ONE pending callback (non-blocking)
    //   2. Try to acquire semaphore (non-blocking mutex lock)
    //   3. If acquired, return SCE_OK
    //   4. If not, wait on the cv with a short timeout so we wake
    //      immediately when SignalSema fires cv.notify_all()
    //   5. After cv wake (or timeout), go to step 1

    constexpr int MAX_CB_LOOPS = 10000;
    int loops = 0;

    // [#35] Tag this thread's wait for the debug socket I command (the CB
    // variant is a poll loop, but it is still semantically a sema wait).
    {
        char wait_tag[24];
        std::snprintf(wait_tag, sizeof(wait_tag), "semacb:%d", uid);
        psp_thread_note_wait(wait_tag);
    }

    while (!g_should_exit.load()) {
        // 1. Process pending callbacks
        psp_kernel_check_callbacks(rdram, ctx);
        // Restore a0/a1 since check_callbacks may clobber them
        ctx->r[4] = uid;
        ctx->r[5] = signal;

        // 2+3. Try to acquire under mutex; if not available, cv.wait_for
        {
            std::unique_lock<std::mutex> lock(s->mtx);
            // No queue jumping: only acquire if no waiter is queued
            // ahead of us (issue #29 FIFO handoff).
            if (s->current_count >= signal && s->waiters.empty()) {
                s->current_count -= signal;
                if (uid == 271 && sema_trace_enabled()) {
                    std::fprintf(stderr,
                        "[SEMA271] WaitSemaCB acquired count_after=%d "
                        "caller=0x%08X\n",
                        s->current_count, g_last_func_addr);
                }
                psp_thread_clear_wait();
                ctx->r[2] = SCE_OK;
                return;
            }
            // Yield the cooperative scheduler lock before blocking
            // on the cv, so other C++ threads can advance.
            // cv.wait_for wakes immediately on notify_all() from
            // SignalSema — much faster than sleep_for.
            s->wait_count++;
            s->cv.wait_for(lock, std::chrono::milliseconds(5),
                [&] { return (s->current_count >= signal
                              && s->waiters.empty())
                             || g_should_exit.load(); });
            s->wait_count--;
            // Do NOT decrement here — re-check at top of loop
        }

        loops++;
        if (loops >= MAX_CB_LOOPS) {
            std::fprintf(stderr,
                "[HLE] sceKernelWaitSemaCB: uid=%d '%s' "
                "gave up after %d callback loops (count=%d)\n",
                uid, s->name, loops, s->current_count);
            break;
        }
    }

    // Fallback: do a blocking wait (limited timeout)
    psp_thread_clear_wait();  // [#35] WaitSema below re-tags if it blocks
    hle_sceKernelWaitSema(rdram, ctx);
}

static void hle_sceKernelPollSema(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    int32_t signal = ctx->r[5];

    auto it = g_semaphores.find(uid);
    if (it == g_semaphores.end()) {
        ctx->r[2] = SCE_KERNEL_ERROR_SEMA_ZERO;
        (void)rdram;
        return;
    }

    auto& s = it->second;
    if (s->deleted) {
        static bool once = false;
        if (!once) {
            std::fprintf(stderr,
                "[HLE] sceKernelPollSema: operating on "
                "soft-deleted uid=%d ('%s') -- kept alive "
                "for cross-thread safety\n",
                uid, s->name);
            once = true;
        }
    }
    std::unique_lock<std::mutex> lock(s->mtx);

    // No queue jumping: a poller must not steal the count from queued
    // waiters (issue #29 FIFO handoff).
    if (s->current_count >= signal && s->waiters.empty()) {
        s->current_count -= signal;
        ctx->r[2] = SCE_OK;
    } else {
        ctx->r[2] = SCE_KERNEL_ERROR_SEMA_ZERO;
    }
    (void)rdram;
}

static void hle_sceKernelReferSemaStatus(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    uint32_t info_ptr = static_cast<uint32_t>(ctx->r[5]);

    auto it = g_semaphores.find(uid);
    if (it == g_semaphores.end()) {
        ctx->r[2] = SCE_KERNEL_ERROR_SEMA_ZERO;
        return;
    }

    auto& s = it->second;
    if (info_ptr != 0) {
        // Write minimal SceKernelSemaInfo
        psp_mem_write<int32_t>(rdram, info_ptr, 56);  // size
        // currentCount at offset 28
        psp_mem_write<int32_t>(rdram, info_ptr + 28,
                               s->current_count);
        // maxCount at offset 32
        psp_mem_write<int32_t>(rdram, info_ptr + 32,
                               s->max_count);
    }

    ctx->r[2] = SCE_OK;
}

// ---- Public API for workarounds ----

void psp_hle_signal_sema_by_uid(int uid, int count) {
    auto it = g_semaphores.find(uid);
    if (it == g_semaphores.end()) return;
    auto& s = it->second;
    std::unique_lock<std::mutex> lock(s->mtx);
    s->current_count += count;
    // Same direct handoff as SignalSema (issue #29).
    sema_grant_waiters(*s);
}

// ---- Registration ----

void psp_hle_register_kernel_sema() {
    psp_hle_register("sceKernelCreateSema",
                      hle_sceKernelCreateSema);
    psp_hle_register("sceKernelDeleteSema",
                      hle_sceKernelDeleteSema);
    psp_hle_register("sceKernelSignalSema",
                      hle_sceKernelSignalSema);
    psp_hle_register("sceKernelWaitSema",
                      hle_sceKernelWaitSema);
    psp_hle_register("sceKernelWaitSemaCB",
                      hle_sceKernelWaitSemaCB);
    psp_hle_register("sceKernelPollSema",
                      hle_sceKernelPollSema);
    psp_hle_register("sceKernelReferSemaStatus",
                      hle_sceKernelReferSemaStatus);
}
