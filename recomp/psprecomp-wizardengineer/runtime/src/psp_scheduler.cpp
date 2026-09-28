#include "psp_scheduler.h"
#include "psp_debug_socket.h"  // PspDebugThreadInfo ([#35] I command)
#include "psp_vfpu.h"  // vfpu_init_context — VFPU prefix reset default
#include "hle/psp_hle.h"  // SCE_KERNEL_ERROR_WAIT_TIMEOUT
#include <cstdio>
#include <cstdlib>
#include <cstring>

/// Global scheduler lock — held during ALL status transitions and cv ops.
static std::mutex g_sched_mutex;

/// Thread pool — 64 slots matching PSP kernel limits.
static PspThread g_threads[MAX_THREADS];

/// Thread-local pointer to this OS thread's PspThread.
/// Each OS thread knows its own PspThread without shared state lookup.
static thread_local PspThread* g_current = nullptr;

// ---------------------------------------------------------------------------
// psp_scheduler_init — zero all 64 slots
// ---------------------------------------------------------------------------
void psp_scheduler_init() {
    std::unique_lock<std::mutex> lock(g_sched_mutex);
    for (int i = 0; i < MAX_THREADS; i++) {
        g_threads[i].id = i;
        g_threads[i].in_use = false;
        g_threads[i].priority = 32;
        g_threads[i].status = DORMANT;
        g_threads[i].rdram = nullptr;
        g_threads[i].entry_addr = 0;
        g_threads[i].arg_value = 0;
        g_threads[i].stack_top = 0;
        g_threads[i].wakeup_count = 0;
        std::memset(g_threads[i].name, 0, sizeof(g_threads[i].name));
        std::memset(g_threads[i].wait_reason, 0,
                    sizeof(g_threads[i].wait_reason));
        std::memset(&g_threads[i].ctx, 0, sizeof(recomp_context));
        vfpu_init_context(&g_threads[i].ctx);
    }
}

// ---------------------------------------------------------------------------
// psp_thread_create — allocate slot, set DORMANT
// ---------------------------------------------------------------------------
int psp_thread_create(
    const char* name,
    uint32_t entry,
    int priority,
    uint32_t stack_top,
    uint32_t arg
) {
    std::unique_lock<std::mutex> lock(g_sched_mutex);
    for (int i = 0; i < MAX_THREADS; i++) {
        if (!g_threads[i].in_use) {
            PspThread& t = g_threads[i];
            t.in_use = true;
            t.priority = priority;
            t.status = DORMANT;
            t.entry_addr = entry;
            t.arg_value = arg;
            t.stack_top = stack_top;
            t.wakeup_count = 0;
            std::memset(t.name, 0, sizeof(t.name));
            std::memset(t.wait_reason, 0, sizeof(t.wait_reason));
            if (name) {
                std::strncpy(t.name, name, sizeof(t.name) - 1);
            }
            std::memset(&t.ctx, 0, sizeof(recomp_context));
            vfpu_init_context(&t.ctx);
            return i;
        }
    }
    return -1; // All slots full
}

// ---------------------------------------------------------------------------
// Forward declaration — thread entry wrapper
// ---------------------------------------------------------------------------
static void thread_entry_wrapper(PspThread* t);

// ---------------------------------------------------------------------------
// psp_thread_start — set READY, launch OS thread
// ---------------------------------------------------------------------------
int psp_thread_start(int thid) {
    std::unique_lock<std::mutex> lock(g_sched_mutex);
    if (thid < 0 || thid >= MAX_THREADS) {
        return -1;
    }
    PspThread& t = g_threads[thid];
    if (!t.in_use || t.status != DORMANT) {
        return -1;
    }
    t.status = READY;
    g_alive_threads.fetch_add(1);
    t.host_thread = std::thread(thread_entry_wrapper, &t);
    return 0;
}

// ---------------------------------------------------------------------------
// thread_entry_wrapper — OS thread entry point
// ---------------------------------------------------------------------------
static void thread_entry_wrapper(PspThread* t) {
    g_current = t;

    // Wait until this thread is scheduled to RUNNING (or exit requested)
    {
        std::unique_lock<std::mutex> lock(g_sched_mutex);
        t->cv.wait_for(
            lock,
            std::chrono::milliseconds(SCHED_TIMEOUT_MS),
            [&] {
                return t->status == RUNNING ||
                       g_should_exit.load();
            }
        );
        // Safety valve: if timed out and still READY, force-run
        if (t->status != RUNNING && !g_should_exit.load()) {
            t->status = RUNNING;
        }
    }

    // Exit early if shutdown requested
    if (g_should_exit.load()) {
        g_alive_threads.fetch_sub(1);
        std::unique_lock<std::mutex> lock(g_sched_mutex);
        t->status = DEAD;
        t->cv.notify_all();
        return;
    }

    // Register context (SP, k0, args) set by hle_sceKernelStartThread
    // before psp_thread_start() was called. Do not reinitialize here.

    // Call the recompiled PSP entry function via dispatch table.
    // Catch PspThreadExitException -- thrown by sceKernelExitThread
    // to implement "never returns" semantics. On the real PSP,
    // ExitThread terminates the thread immediately; we use C++
    // exception unwinding to achieve the same effect.
    try {
        FuncPtr entry = RECOMP_LOOKUP(t->entry_addr);
        if (entry) {
            entry(t->rdram, &t->ctx);
        }
    } catch (const PspThreadExitException& ex) {
        std::fprintf(stderr,
            "[SCHED] Thread %d (\"%s\") exited via "
            "sceKernelExitThread(status=%d)\n",
            t->id, t->name, ex.status);
    }

    // Thread function returned (or was exited) — mark dead
    std::fprintf(stderr,
        "[SCHED] Thread %d (\"%s\") entry function returned, marking dead "
        "(alive=%d)\n",
        t->id, t->name, g_alive_threads.load());
    psp_thread_exit_current();
}

// ---------------------------------------------------------------------------
// sched_yield_point — cooperative yield (RUNTIME-05, RUNTIME-06)
// ---------------------------------------------------------------------------
void sched_yield_point() {
    if (g_should_exit.load()) {
        return;
    }
    if (!g_current) {
        return;
    }

    std::unique_lock<std::mutex> lock(g_sched_mutex);

    // Find highest-priority READY thread (lowest priority number)
    PspThread* best = nullptr;
    for (int i = 0; i < MAX_THREADS; i++) {
        PspThread& t = g_threads[i];
        if (!t.in_use || t.status != READY) {
            continue;
        }
        if (&t == g_current) {
            continue;
        }
        if (!best || t.priority < best->priority) {
            best = &t;
        }
    }

    // No switch needed if no READY thread or current is higher priority
    if (!best || g_current->priority <= best->priority) {
        return;
    }

    // Context switch: yield to best, suspend current
    PspThread* current = g_current;
    if (current->status == RUNNING) {
        current->status = READY;
    }
    best->status = RUNNING;

    // Wake the best thread
    best->cv.notify_one();

    // Wait until we are re-scheduled (50ms safety valve)
    current->cv.wait_for(
        lock,
        std::chrono::milliseconds(SCHED_TIMEOUT_MS),
        [&] {
            return current->status == RUNNING ||
                   current->status == DEAD ||
                   g_should_exit.load();
        }
    );

    // Safety valve: if timed out and still READY, force-resume
    if (current->status == READY) {
        current->status = RUNNING;
    }
}

// ---------------------------------------------------------------------------
// sched_preempt — instruction-budget preemption hook (#66, approach (a))
//
// Called from emitted code at loop back-edges once ctx->preempt_budget hits
// <= 0. DEFAULT-OFF: with PSPRECOMP_PREEMPT unset/"0" this is a no-op beyond
// reloading the budget — the back-edge decrement becomes a dead effect and
// guest behavior is byte-identical to the pre-#66 scheduler (the gate this unit
// must pass on Patapon). When PSPRECOMP_PREEMPT=1 it additionally takes a fair
// cooperative yield so a syscall-free busy-poll reaches a reschedule point;
// step 3 (#66) replaces this with the real preemptive yield and validates it
// against the .hack freeze-rate harness. The flag is read once and cached.
// ---------------------------------------------------------------------------
static bool sched_preempt_enabled() {
    static const bool enabled = [] {
        const char* v = std::getenv("PSPRECOMP_PREEMPT");
        return v && v[0] == '1' && v[1] == '\0';
    }();
    return enabled;
}

void sched_preempt(recomp_context* ctx) {
    // Always reload first: a syscall-free spin must not call back every
    // iteration, and this self-primes the memset-zero initial budget.
    if (ctx) {
        ctx->preempt_budget = SCHED_PREEMPT_BUDGET;
    }
    if (!sched_preempt_enabled()) {
        return;  // DEFAULT-OFF: dead-effect counter, no yield.
    }
    // Flag ON (not enabled by default in this unit): take a fair cooperative
    // yield. Step 3 swaps this for the real preemptive yield + memory fence.
    sched_yield_point();
}

// ---------------------------------------------------------------------------
// psp_thread_exit_current — mark DEAD, decrement counter, wake next
// ---------------------------------------------------------------------------
void psp_thread_exit_current() {
    std::unique_lock<std::mutex> lock(g_sched_mutex);
    if (!g_current) {
        return;
    }

    g_current->status = DEAD;
    g_alive_threads.fetch_sub(1);

    // Find next READY thread and wake it
    for (int i = 0; i < MAX_THREADS; i++) {
        PspThread& t = g_threads[i];
        if (t.in_use && t.status == READY) {
            t.status = RUNNING;
            t.cv.notify_one();
            break;
        }
    }

    // Notify anyone waiting on this thread (e.g. sceKernelWaitThreadEnd)
    g_current->cv.notify_all();
}

// ---------------------------------------------------------------------------
// psp_scheduler_shutdown — signal exit, wake all, join with timeout
// ---------------------------------------------------------------------------
void psp_scheduler_shutdown() {
    g_should_exit.store(true);

    // Wake all threads so they can observe the exit flag
    {
        std::unique_lock<std::mutex> lock(g_sched_mutex);
        for (int i = 0; i < MAX_THREADS; i++) {
            if (g_threads[i].in_use) {
                g_threads[i].cv.notify_all();
            }
        }
    }

    // Join all threads with 2-second timeout (detach as fallback)
    for (int i = 0; i < MAX_THREADS; i++) {
        PspThread& t = g_threads[i];
        if (t.in_use && t.host_thread.joinable()) {
            // Use a helper thread to implement join-with-timeout
            std::atomic<bool> joined{false};
            std::thread joiner([&] {
                t.host_thread.join();
                joined.store(true);
            });

            // Wait up to 2 seconds
            auto deadline = std::chrono::steady_clock::now()
                          + std::chrono::seconds(2);
            while (!joined.load() &&
                   std::chrono::steady_clock::now() < deadline) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(10)
                );
            }

            if (joined.load()) {
                joiner.join();
            } else {
                // Thread did not join in time — detach (process is exiting)
                std::fprintf(
                    stderr,
                    "SCHED: thread %d (%s) did not join in 2s, "
                    "detaching\n",
                    t.id, t.name
                );
                joiner.detach();
                t.host_thread.detach();
            }

            t.status = DEAD;
            t.in_use = false;
        }
    }
}

// ---------------------------------------------------------------------------
// psp_get_current_thread — thread-local accessor
// ---------------------------------------------------------------------------
PspThread* psp_get_current_thread() {
    return g_current;
}

// ---------------------------------------------------------------------------
// psp_get_thread — by-id accessor (no mutex, caller ensures DORMANT)
// ---------------------------------------------------------------------------
PspThread* psp_get_thread(int thid) {
    if (thid < 0 || thid >= MAX_THREADS) {
        return nullptr;
    }
    PspThread& t = g_threads[thid];
    if (!t.in_use) {
        return nullptr;
    }
    return &t;
}

// ---------------------------------------------------------------------------
// psp_thread_sleep_current — PPSSPP wakeupCount sleep semantics
// ---------------------------------------------------------------------------
int psp_thread_sleep_current() {
    if (!g_current) {
        return -1;
    }

    std::unique_lock<std::mutex> lock(g_sched_mutex);

    // If wakeup_count > 0, return immediately (pre-woken)
    if (g_current->wakeup_count > 0) {
        g_current->wakeup_count--;
        return 0;
    }

    // Enter WAIT_SLEEP state
    g_current->status = WAIT_SLEEP;
    std::strncpy(g_current->wait_reason, "sleep",
                 sizeof(g_current->wait_reason) - 1);

    // Wake next READY thread before sleeping (prevents deadlock)
    for (int i = 0; i < MAX_THREADS; i++) {
        PspThread& t = g_threads[i];
        if (t.in_use && t.status == READY && &t != g_current) {
            t.status = RUNNING;
            t.cv.notify_one();
            break;
        }
    }

    // Block until woken (5s safety valve)
    PspThread* self = g_current;
    self->cv.wait_for(
        lock,
        std::chrono::seconds(5),
        [&] {
            return self->status != WAIT_SLEEP ||
                   g_should_exit.load();
        }
    );

    // Safety valve: if still WAIT_SLEEP after timeout, force-resume
    if (self->status == WAIT_SLEEP) {
        self->status = RUNNING;
        std::fprintf(stderr,
            "SCHED: thread %d (%s) sleep safety valve "
            "triggered\n", self->id, self->name);
    }
    self->wait_reason[0] = '\0';

    return 0;
}

// ---------------------------------------------------------------------------
// psp_thread_wakeup — wake sleeping thread or pre-increment counter
// ---------------------------------------------------------------------------
int psp_thread_wakeup(int thid) {
    std::unique_lock<std::mutex> lock(g_sched_mutex);

    if (thid < 0 || thid >= MAX_THREADS) {
        return -1;
    }
    PspThread& t = g_threads[thid];
    if (!t.in_use) {
        return -1;
    }

    if (t.status == WAIT_SLEEP) {
        // Thread is sleeping -- wake it
        t.status = READY;
        t.cv.notify_one();
    } else {
        // Not sleeping -- pre-increment for future SleepThread call
        t.wakeup_count++;
    }

    return 0;
}

// ---------------------------------------------------------------------------
// psp_thread_wait_end — block until target thread reaches DEAD
// ---------------------------------------------------------------------------
int psp_thread_wait_end(int thid, int timeout_us) {
    if (!g_current) {
        return -1;
    }

    std::unique_lock<std::mutex> lock(g_sched_mutex);

    if (thid < 0 || thid >= MAX_THREADS) {
        return SCE_KERNEL_ERROR_NOT_FOUND_THREAD;
    }
    PspThread& t = g_threads[thid];

    // Already dead or freed -- return success immediately
    if (t.status == DEAD || !t.in_use) {
        return 0;
    }

    // Block current thread, wake next READY thread
    PspThread* self = g_current;
    self->status = WAIT;
    std::snprintf(self->wait_reason, sizeof(self->wait_reason),
                  "thread_end:%d", thid);

    for (int i = 0; i < MAX_THREADS; i++) {
        PspThread& other = g_threads[i];
        if (other.in_use && other.status == READY &&
            &other != self) {
            other.status = RUNNING;
            other.cv.notify_one();
            break;
        }
    }

    // Wait for target thread to die
    auto deadline_dur = (timeout_us > 0)
        ? std::chrono::microseconds(timeout_us)
        : std::chrono::microseconds(5000000);  // 5s safety valve

    bool done = t.cv.wait_for(
        lock,
        deadline_dur,
        [&] {
            return t.status == DEAD || !t.in_use ||
                   g_should_exit.load();
        }
    );

    self->status = RUNNING;
    self->wait_reason[0] = '\0';

    if (!done && t.status != DEAD && t.in_use) {
        return SCE_KERNEL_ERROR_WAIT_TIMEOUT;
    }

    return 0;
}

// ---------------------------------------------------------------------------
// psp_thread_wait — set thread to WAIT status
// ---------------------------------------------------------------------------
void psp_thread_wait(int thid) {
    std::unique_lock<std::mutex> lock(g_sched_mutex);
    if (thid < 0 || thid >= MAX_THREADS) {
        return;
    }
    PspThread& t = g_threads[thid];
    if (!t.in_use) {
        return;
    }
    if (t.status == RUNNING || t.status == READY) {
        t.status = WAIT;
        if (t.wait_reason[0] == '\0') {
            std::strncpy(t.wait_reason, "wait", sizeof(t.wait_reason) - 1);
        }
    }
}

// ---------------------------------------------------------------------------
// psp_thread_resume — set thread to READY and notify
// ---------------------------------------------------------------------------
void psp_thread_resume(int thid) {
    std::unique_lock<std::mutex> lock(g_sched_mutex);
    if (thid < 0 || thid >= MAX_THREADS) {
        return;
    }
    PspThread& t = g_threads[thid];
    if (!t.in_use) {
        return;
    }
    if (t.status == WAIT) {
        t.status = READY;
        t.wait_reason[0] = '\0';
        t.cv.notify_one();
    }
}

// ---------------------------------------------------------------------------
// [#35] Debug socket support — wait-reason notes + thread snapshot
// ---------------------------------------------------------------------------

void psp_thread_note_wait(const char* reason) {
    if (!g_current || !reason) {
        return;
    }
    std::strncpy(g_current->wait_reason, reason,
                 sizeof(g_current->wait_reason) - 1);
    g_current->wait_reason[sizeof(g_current->wait_reason) - 1] = '\0';
}

void psp_thread_clear_wait() {
    if (!g_current) {
        return;
    }
    g_current->wait_reason[0] = '\0';
}

int psp_scheduler_snapshot(PspDebugThreadInfo* out, int max) {
    static const char* kStatusNames[] = {
        "DORMANT", "READY", "RUNNING", "WAIT", "DEAD", "WAIT_SLEEP"
    };
    std::unique_lock<std::mutex> lock(g_sched_mutex);
    int n = 0;
    for (int i = 0; i < MAX_THREADS && n < max; i++) {
        PspThread& t = g_threads[i];
        if (!t.in_use) {
            continue;
        }
        PspDebugThreadInfo& info = out[n++];
        info.id = t.id;
        std::strncpy(info.name, t.name, sizeof(info.name) - 1);
        info.name[sizeof(info.name) - 1] = '\0';
        int s = static_cast<int>(t.status);
        const char* status = (s >= 0 && s <= 5) ? kStatusNames[s] : "?";
        std::strncpy(info.status, status, sizeof(info.status) - 1);
        info.status[sizeof(info.status) - 1] = '\0';
        std::strncpy(info.wait_reason, t.wait_reason,
                     sizeof(info.wait_reason) - 1);
        info.wait_reason[sizeof(info.wait_reason) - 1] = '\0';
    }
    return n;
}
