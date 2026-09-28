#pragma once
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <exception>

#include "recomp.h"
#include "psp_runtime.h"

/// Exception thrown by sceKernelExitThread to unwind the call stack.
/// On the real PSP, sceKernelExitThread never returns -- it terminates
/// the calling thread immediately. We use a C++ exception to achieve
/// the same "never returns" semantics in recompiled code: the HLE stub
/// throws PspThreadExitException, which unwinds through recompiled C++
/// back to thread_entry_wrapper where it is caught.
struct PspThreadExitException : public std::exception {
    int32_t status;
    explicit PspThreadExitException(int32_t s) : status(s) {}
    const char* what() const noexcept override {
        return "PSP thread exit";
    }
};

/// Thread status enum — models PSP kernel thread states.
/// DORMANT: created but not started; READY: runnable; RUNNING: executing;
/// WAIT: blocked on semaphore/event; WAIT_SLEEP: blocked via SleepThread
/// (distinct from WAIT so WakeupThread can identify sleep-blocked threads);
/// DEAD: exited.
enum ThreadStatus {
    DORMANT    = 0,
    READY      = 1,
    RUNNING    = 2,
    WAIT       = 3,
    DEAD       = 4,
    WAIT_SLEEP = 5
};

/// Maximum concurrent PSP threads (PSP kernel supports ~64).
static constexpr int MAX_THREADS = 64;

/// Safety valve timeout in milliseconds — prevents deadlock when a thread
/// enters a tight compute loop with no yield calls (RUNTIME-06).
static constexpr int SCHED_TIMEOUT_MS = 50;

/// Instruction-budget reload value for emitted loop-back-edge preemption
/// points (#66, design approach (a)). The emitter decrements
/// `ctx->preempt_budget` once per loop back-edge pass; when it reaches <= 0 the
/// generated code calls `sched_preempt()`, which reloads it to this value. This
/// is a generic, title-agnostic constant (purity gate): ~100k back-edge passes
/// between reschedule checks keeps the dead-effect counter overhead negligible
/// on straight-line-dominated code while bounding how long a syscall-free spin
/// runs before reaching a preemption point once the flag is enabled.
static constexpr int32_t SCHED_PREEMPT_BUDGET = 100000;

/// Per-thread state for PSP cooperative scheduler.
/// Each thread gets its own recomp_context (RUNTIME-01) so register state
/// is naturally isolated without save/restore.
struct PspThread {
    int id;                     ///< Slot index (0-63)
    bool in_use;                ///< Slot occupied
    int priority;               ///< PSP priority (lower = higher, default 32)
    ThreadStatus status;        ///< Current thread state
    std::condition_variable cv; ///< Per-thread condvar for sleep/wake
    std::thread host_thread;    ///< Actual OS thread
    recomp_context ctx;         ///< Per-thread register state (RUNTIME-01)
    uint8_t* rdram;             ///< Pointer to shared rdram (set at creation)
    uint32_t entry_addr;        ///< PSP entry point address
    uint32_t arg_value;         ///< Argument passed to thread entry (a0/r4)
    uint32_t stack_top;         ///< Initial SP value
    int wakeup_count;           ///< Sleep/wakeup counter (PPSSPP semantics)
    char name[32];              ///< Thread name for debugging
    char wait_reason[24];       ///< Why the thread is blocked (e.g. "sema:259",
                                ///< "sleep"); diagnostics only, racy reads OK
};

/// Initialize scheduler — zero all 64 thread slots.
void psp_scheduler_init();

/// Create a new PSP thread in DORMANT state.
/// Returns slot id (0-63) or -1 if all slots are full.
int psp_thread_create(
    const char* name,
    uint32_t entry,
    int priority,
    uint32_t stack_top,
    uint32_t arg
);

/// Start a DORMANT thread — sets READY, launches OS thread.
/// Returns 0 on success, -1 on invalid thid or wrong state.
int psp_thread_start(int thid);

/// Cooperative yield point — called by HLE stubs at blocking boundaries.
/// Finds highest-priority READY thread and context-switches to it.
/// Uses 50ms timedwait safety valve (RUNTIME-06).
void sched_yield_point();

/// Instruction-budget preemption hook (#66, design approach (a)) — called from
/// emitted code at loop back-edges when `ctx->preempt_budget` hits <= 0.
/// DEFAULT-OFF: with the `PSPRECOMP_PREEMPT` env var unset or "0" this only
/// reloads `ctx->preempt_budget` to SCHED_PREEMPT_BUDGET and returns (no yield),
/// so the decrement is a dead effect and Patapon behavior is unchanged. When
/// `PSPRECOMP_PREEMPT=1` it reloads the budget and additionally takes a fair
/// cooperative yield (step 3 wires the real preemptive yield; today the enabled
/// path reuses `sched_yield_point()`). Always resets the budget so the spin loop
/// does not call back every iteration.
void sched_preempt(recomp_context* ctx);

/// Mark current thread DEAD, decrement g_alive_threads, wake next thread.
void psp_thread_exit_current();

/// Shutdown scheduler — set exit flag, wake all threads, join with timeout.
void psp_scheduler_shutdown();

/// Returns the PspThread* for the calling OS thread (thread-local).
PspThread* psp_get_current_thread();

/// Returns PspThread* by slot id, or nullptr if invalid/unused.
/// Does NOT take the scheduler mutex -- callers access the thread struct
/// while the thread is DORMANT (no race).
PspThread* psp_get_thread(int thid);

/// Sleep current thread using wakeupCount counter semantics (PPSSPP model).
/// If wakeup_count > 0, decrements and returns immediately.
/// Otherwise blocks in WAIT_SLEEP until woken by psp_thread_wakeup().
/// Returns 0 on success.
int psp_thread_sleep_current();

/// Wake a sleeping thread or pre-increment its wakeup_count.
/// If target is WAIT_SLEEP, sets READY and notifies.
/// Otherwise increments wakeup_count for future SleepThread call.
/// Returns 0 on success.
int psp_thread_wakeup(int thid);

/// Block current thread until target thread reaches DEAD status.
/// timeout_us > 0: wait up to timeout_us microseconds.
/// timeout_us <= 0: wait up to 5 seconds (safety valve).
/// Returns 0 on success, SCE_KERNEL_ERROR_WAIT_TIMEOUT on timeout.
int psp_thread_wait_end(int thid, int timeout_us);

/// Set thread to WAIT status (called by HLE stubs for semaphore/event waits).
void psp_thread_wait(int thid);

/// Set thread to READY status and notify (called by HLE stubs to unblock).
void psp_thread_resume(int thid);

/// [#35] Record/clear why the CURRENT thread is about to block (e.g.
/// "sema:259", "sleep"). Pure diagnostics for the debug socket I command;
/// no locking (single writer = the thread itself; readers tolerate races).
void psp_thread_note_wait(const char* reason);
void psp_thread_clear_wait();

/// [#35] Copy id/name/status/wait_reason of every in-use thread slot into
/// `out` (up to `max` entries) for the debug socket I command. Takes the
/// scheduler mutex briefly. Returns the number of entries written.
struct PspDebugThreadInfo;
int psp_scheduler_snapshot(PspDebugThreadInfo* out, int max);
