#include "hle/psp_hle.h"
#include "hle/psp_hle_kernel.h"
#include "hle/psp_hle_refer_info.h"
#include "psp_scheduler.h"
#include "psp_memory.h"
#include "psp_game_module.h"
#include "recomp.h"

#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>
#include <unordered_map>

// ---- newlib _reent Initialization ----
// PSP newlib __getreent() reads the _reent pointer from k0+0x00.
// If k0+0x00 is NULL, newlib prints "no reent structure found"
// and calls sceKernelExitThread(1), killing the thread.
//
// On the real PSP, the kernel allocates a _reent struct for each
// thread and stores its address at k0+0x00. Our runtime must do
// the same. The _reent struct is 1024 bytes, zero-initialized.
//
// The _reent minimum fields (newlib struct _reent):
//   +0x00: _errno (int32_t)
//   +0x04: _stdin (FILE*, can be NULL)
//   +0x08: _stdout (FILE*, can be NULL)
//   +0x0C: _stderr (FILE*, can be NULL)
// Zero initialization is safe -- errno=0, all pointers NULL.
static constexpr uint32_t PSP_REENT_SIZE = 1024;

// ---- Thread UID Tracking ----
static std::unordered_map<int, PspThreadInfo> g_thread_by_uid;
static std::unordered_map<int, int> g_thid_to_uid;

// ---- Callback Table ----
static std::unordered_map<int, PspCallback> g_callbacks;

// Game-module callback-dispatch observer (#47 P5 seam): invoked right
// before sceKernelCheckCallback dispatches a pending callback. Lets a game
// module attach address-keyed diagnostics (e.g. Patapon's IoAsyncCallback
// arg-struct dump) without a special case in the generic dispatcher.
static PspCallbackDispatchObserver g_callback_dispatch_observer = nullptr;

void psp_kernel_set_callback_dispatch_observer(
    PspCallbackDispatchObserver fn
) {
    g_callback_dispatch_observer = fn;
}

// Forward declaration (used by CB variants before definition)
static void hle_sceKernelCheckCallback(
    uint8_t* rdram, recomp_context* ctx);

// ---- UID Generator (shared across all kernel objects) ----
static int g_uid_counter = 0x100;

int psp_next_uid() {
    return g_uid_counter++;
}

// ---- Stack Allocator ----
// Grows downward from top of PSP user memory
static uint32_t g_stack_top = PSP_USER_MEM_END - 0x1000;

uint32_t psp_alloc_stack(uint8_t* rdram, uint32_t size) {
    (void)rdram;
    // Align to 256 bytes
    size = (size + 0xFF) & ~0xFFU;
    g_stack_top -= size;
    return g_stack_top;
}

// ---- System Time Base ----
static auto g_time_base = std::chrono::steady_clock::now();

static uint64_t get_system_time_us() {
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            now - g_time_base).count());
}

// ---- HLE Functions ----

static void hle_sceKernelCreateThread(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t name_ptr = static_cast<uint32_t>(ctx->r[4]);
    uint32_t entry    = static_cast<uint32_t>(ctx->r[5]);
    int32_t  priority = ctx->r[6];
    uint32_t stack_sz = static_cast<uint32_t>(ctx->r[7]);
    uint32_t attr     = static_cast<uint32_t>(ctx->r[8]);  // t0 = arg 5

    const char* name = reinterpret_cast<const char*>(
        rdram + (name_ptr & PSP_ADDR_MASK));

    // Allocate guest stack
    uint32_t stack_top = psp_alloc_stack(rdram, stack_sz);

    // Create thread via Phase 3 scheduler
    int thid = psp_thread_create(name, entry, priority, stack_top, 0);
    if (thid < 0) {
        std::fprintf(stderr,
            "[HLE] sceKernelCreateThread(\"%s\") FAILED: no slots\n",
            name);
        ctx->r[2] = SCE_KERNEL_ERROR_NO_MEMORY;
        return;
    }

    int uid = psp_next_uid();
    PspThreadInfo info{};
    info.uid = uid;
    info.thid = thid;
    info.entry_addr = entry;
    info.stack_base = stack_top;
    info.stack_size = stack_sz;
    info.priority = priority;
    // PSP firmware ORs 0xFF into the attr on create (PPSSPP
    // sceKernelThread.cpp:1745); guest-visible via ReferThreadStatus.
    info.attr = attr | 0xFF;
    std::strncpy(info.name, name, sizeof(info.name) - 1);

    g_thread_by_uid[uid] = info;
    g_thid_to_uid[thid] = uid;

    std::fprintf(stderr,
        "[HLE] sceKernelCreateThread(\"%s\", 0x%08X, pri=%d, "
        "stk=%u) -> uid=%d\n",
        name, entry, priority, stack_sz, uid);

    ctx->r[2] = uid;
}

static void hle_sceKernelStartThread(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    int32_t arglen = ctx->r[5];
    uint32_t argp = static_cast<uint32_t>(ctx->r[6]);

    // 1. Look up thread info by UID
    auto it = g_thread_by_uid.find(uid);
    if (it == g_thread_by_uid.end()) {
        std::fprintf(stderr,
            "[HLE] sceKernelStartThread(uid=%d) NOT FOUND\n", uid);
        ctx->r[2] = SCE_KERNEL_ERROR_NOT_FOUND_THREAD;
        return;
    }

    int thid = it->second.thid;

    // 2. Get PspThread via psp_get_thread(thid)
    PspThread* pt = psp_get_thread(thid);
    if (!pt) {
        std::fprintf(stderr,
            "[HLE] sceKernelStartThread(uid=%d) thid=%d "
            "invalid slot\n", uid, thid);
        ctx->r[2] = SCE_KERNEL_ERROR_ILLEGAL_THREAD;
        return;
    }

    // 3. Wire rdram (Research Fix 1): set BEFORE thread starts
    pt->rdram = rdram;

    // 4. Set up k0 area (Research Fix 2):
    //    256 bytes at stack_top, k0 register points there,
    //    usable SP starts below the k0 area.
    //    PPSSPP sets k0+C0=UID, k0+C8=stack, k0+F8/FC=0xFFFFFFFF.
    //    k0+4 heap descriptor stays 0 here (unconfigured PPSSPP-style k0
    //    area, plan decision P12); title-specific values (e.g. Patapon's
    //    dlmalloc default-heap descriptor) come from the game module's
    //    on_thread_start hook below.
    uint32_t stack_top = pt->stack_top;
    std::memset(
        rdram + (stack_top & PSP_ADDR_MASK), 0, 0x100);
    psp_mem_write<int32_t>(
        rdram, stack_top + 0xC0, it->second.uid);
    psp_mem_write<uint32_t>(
        rdram, stack_top + 0xC8, stack_top);
    psp_mem_write<uint32_t>(
        rdram, stack_top + 0xF8, 0xFFFFFFFFU);
    psp_mem_write<uint32_t>(
        rdram, stack_top + 0xFC, 0xFFFFFFFFU);

    // 4a. Game-module per-thread hook (issues #46/#47 Phase 4): runs after
    //     the standard k0 block is built so the module can override fields.
    psp_game_module()->on_thread_start(rdram, stack_top);

    // 4b. Allocate newlib _reent structure for this thread.
    //     PSP __getreent() reads from k0+0x00. If NULL, newlib
    //     asserts "no reent structure found" and exits the thread.
    //     Allocate 1024 bytes from kernel memory, zero-init, and
    //     write the address to k0+0x00.
    uint32_t reent_addr =
        psp_alloc_kernel_memory(PSP_REENT_SIZE);
    if (reent_addr != 0) {
        std::memset(
            rdram + (reent_addr & PSP_ADDR_MASK),
            0, PSP_REENT_SIZE);
        psp_mem_write<uint32_t>(
            rdram, stack_top + 0x00, reent_addr);
    }

    pt->ctx.r[26] = static_cast<int32_t>(stack_top);

    // 5. Set usable SP below k0 area
    pt->ctx.r[29] = static_cast<int32_t>(stack_top - 0x100);

    // 6. Copy thread arguments (Research Fix 3):
    //    PSP convention: copy argPtr data onto thread's stack,
    //    set a0=argLen, a1=stack copy address.
    if (argp != 0 && arglen > 0) {
        uint32_t aligned_len =
            (static_cast<uint32_t>(arglen) + 0xFU) & ~0xFU;
        uint32_t new_sp =
            static_cast<uint32_t>(pt->ctx.r[29]) - aligned_len;
        std::memcpy(
            rdram + (new_sp & PSP_ADDR_MASK),
            rdram + (argp & PSP_ADDR_MASK),
            static_cast<size_t>(arglen));
        pt->ctx.r[4] = arglen;
        pt->ctx.r[5] = static_cast<int32_t>(new_sp);
        pt->ctx.r[29] = static_cast<int32_t>(new_sp);
    }

    // 7. Set GP register from boot module's gp_value
    // PPSSPP does this via __KernelGetModuleGP(module->GetUID())
    pt->ctx.r[28] = static_cast<int32_t>(psp_get_boot_module_gp());

    // 8. RA = 0 for clean return to thread_entry_wrapper
    pt->ctx.r[31] = 0;

    // 9. Start the thread via scheduler (LAST)
    int rc = psp_thread_start(thid);
    if (rc < 0) {
        std::fprintf(stderr,
            "[HLE] sceKernelStartThread(uid=%d) FAILED\n", uid);
        ctx->r[2] = SCE_KERNEL_ERROR_ILLEGAL_THREAD;
        return;
    }

    std::fprintf(stderr,
        "[HLE] sceKernelStartThread(uid=%d, \"%s\", "
        "arglen=%d, argp=0x%08X)\n",
        uid, it->second.name, arglen, argp);

    ctx->r[2] = SCE_OK;
}

static void hle_sceKernelExitThread(
    uint8_t* rdram, recomp_context* ctx
) {
    int32_t status = ctx->r[4];
    static int s_exit_log_count = 0;
    if (s_exit_log_count < 5) {
        std::fprintf(stderr,
            "[HLE] sceKernelExitThread(status=%d)\n", status);
        ++s_exit_log_count;
        if (s_exit_log_count == 5) {
            std::fprintf(stderr,
                "[HLE] (further ExitThread messages "
                "suppressed)\n");
        }
    }
    // On the real PSP, sceKernelExitThread never returns --
    // it terminates the calling thread immediately. We throw
    // PspThreadExitException to unwind through recompiled code
    // back to thread_entry_wrapper where it is caught.
    // psp_thread_exit_current() is called after the catch.
    (void)rdram;
    throw PspThreadExitException(status);
}

static void hle_sceKernelExitDeleteThread(
    uint8_t* rdram, recomp_context* ctx
) {
    int32_t status = ctx->r[4];
    std::fprintf(stderr,
        "[HLE] sceKernelExitDeleteThread(status=%d)\n",
        status);
    (void)rdram;
    throw PspThreadExitException(status);
}

static void hle_sceKernelDeleteThread(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    // Just remove from tracking
    g_thread_by_uid.erase(uid);
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelDelayThread(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t usec = static_cast<uint32_t>(ctx->r[4]);
    // Dispatch pending IO callbacks before yielding — this matches the PSP's
    // behaviour where async IO completions are delivered on the next scheduler
    // quantum.  Without this, IO state-machine callbacks (e.g. the FileThread's
    // fd callback uid=273) are never dispatched because the driving loop uses
    // plain sceKernelDelayThread (not the CB variant).
    psp_kernel_check_callbacks(rdram, ctx);
    sched_yield_point();
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
    ctx->r[2] = SCE_OK;
}

static void hle_sceKernelDelayThreadCB(
    uint8_t* rdram, recomp_context* ctx
) {
    psp_kernel_check_callbacks(rdram, ctx);
    hle_sceKernelDelayThread(rdram, ctx);
}

static void hle_sceKernelSleepThread(
    uint8_t* rdram, recomp_context* ctx
) {
    sched_yield_point();
    int rc = psp_thread_sleep_current();
    ctx->r[2] = (rc == 0) ? SCE_OK : rc;
    (void)rdram;
}

static void hle_sceKernelSleepThreadCB(
    uint8_t* rdram, recomp_context* ctx
) {
    psp_kernel_check_callbacks(rdram, ctx);
    hle_sceKernelSleepThread(rdram, ctx);
}

int psp_current_thread_uid() {
    PspThread* t = psp_get_current_thread();
    if (t) {
        auto it = g_thid_to_uid.find(t->id);
        if (it != g_thid_to_uid.end()) {
            return it->second;
        }
    }
    // Boot thread doesn't have a registered UID
    return 0x100;
}

static void hle_sceKernelGetThreadId(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = psp_current_thread_uid();
    (void)rdram;
}

// psp_encode_thread_status (psp_hle_refer_info.h) maps these internal
// values by number; pin them so silent enum edits cannot skew the map.
// GUARD G1: internal DEAD(4) == PSP THREADSTATUS_WAIT(4) — never cast
// through, always map.
static_assert(DORMANT == 0 && READY == 1 && RUNNING == 2 && WAIT == 3 &&
              DEAD == 4 && WAIT_SLEEP == 5,
              "update psp_encode_thread_status when ThreadStatus changes");

/// Build the guest-visible SceKernelThreadInfo image for a registered
/// thread. Genuinely tracked fields: name, attr, entry, stack base/size,
/// gp, initial/current priority, status + waitType (explicit map),
/// wakeupCount. Untracked fields and their PSP-plausible defaults:
///   waitID            -> 0 (sema/eventflag wait-id tracking is a later leg)
///   exitStatus        -> SCE_KERNEL_ERROR_DORMANT before start (PPSSPP
///                        creation value), 0 once exited (real exit values
///                        are not recorded), NOT_DORMANT while alive
///   runClocks,
///   preempt/release   -> 0 (PPSSPP initializes all of them to 0)
static PspNativeThreadImage build_thread_info_image(
    const PspThreadInfo& info
) {
    PspNativeThreadImage img{};
    std::strncpy(img.name, info.name, sizeof(img.name) - 1);
    img.attr = info.attr;
    img.entry = info.entry_addr;
    img.stack = info.stack_base;
    img.stack_size = info.stack_size;
    img.gp = psp_get_boot_module_gp();
    img.init_priority = info.priority;
    img.current_priority = info.priority;

    int internal = DORMANT;
    PspThread* pt = psp_get_thread(info.thid);
    if (pt) {
        internal = pt->status;       // racy read OK (diagnostic convention)
        img.current_priority = pt->priority;
        img.wakeup_count = pt->wakeup_count;
    }
    PspThreadStatusEncoding enc = psp_encode_thread_status(internal);
    img.status = enc.status;
    img.wait_type = enc.wait_type;
    if (internal == DORMANT) {
        img.exit_status = PSP_ERROR_DORMANT;
    } else if (internal == DEAD) {
        img.exit_status = 0;
    } else {
        img.exit_status = PSP_ERROR_NOT_DORMANT;
    }
    return img;
}

/// Image for the unregistered boot thread (no PspThreadInfo record): a
/// plausible RUNNING user thread.
static PspNativeThreadImage boot_thread_image() {
    PspNativeThreadImage img{};
    std::strncpy(img.name, "boot", sizeof(img.name) - 1);
    img.attr = 0x800000FFU;  // PSP_THREAD_ATTR_USER | firmware 0xFF
    img.gp = psp_get_boot_module_gp();
    img.init_priority = 32;
    img.current_priority = 32;
    img.status = PSP_THREADSTATUS_RUNNING;
    img.wait_type = PSP_WAITTYPE_NONE;
    img.exit_status = PSP_ERROR_NOT_DORMANT;
    return img;
}

static void hle_sceKernelReferThreadStatus(
    uint8_t* rdram, recomp_context* ctx
) {
    // HOT PATH (GUARD G3): .hack//Link's frame pump polls this ~250k/s
    // while a worker is busy. NO logging here, ever — call tracing is the
    // dispatch layer's PSPRECOMP_HLE_TRACE job.
    int uid = ctx->r[4];
    uint32_t info_ptr = static_cast<uint32_t>(ctx->r[5]);
    bool sdk_after_260 = psp_kernel_compiled_sdk_version() > 0x02060010U;

    const PspThreadInfo* info = nullptr;
    if (uid == 0) {
        // uid 0 = calling thread. GUARD G4: resolve via the current thread,
        // never via uid 0x100 (the unregistered boot thread also reports
        // 0x100, colliding with the first registered thread).
        PspThread* cur = psp_get_current_thread();
        if (cur) {
            auto u = g_thid_to_uid.find(cur->id);
            if (u != g_thid_to_uid.end()) {
                auto it = g_thread_by_uid.find(u->second);
                if (it != g_thread_by_uid.end()) {
                    info = &it->second;
                }
            }
        }
        if (!info) {
            ctx->r[2] = psp_write_thread_info(
                rdram, info_ptr, sdk_after_260, boot_thread_image());
            return;
        }
    } else {
        auto it = g_thread_by_uid.find(uid);
        if (it == g_thread_by_uid.end()) {
            // PPSSPP-faithful: unknown uid -> error, write nothing.
            ctx->r[2] = SCE_KERNEL_ERROR_NOT_FOUND_THREAD;
            return;
        }
        info = &it->second;
    }
    ctx->r[2] = psp_write_thread_info(
        rdram, info_ptr, sdk_after_260, build_thread_info_image(*info));
}

static void hle_sceKernelChangeThreadPriority(
    uint8_t* rdram, recomp_context* ctx
) {
    // No-op for now
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelChangeCurrentThreadAttr(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelGetSystemTimeLow(
    uint8_t* rdram, recomp_context* ctx
) {
    uint64_t us = get_system_time_us();
    ctx->r[2] = static_cast<int32_t>(us & 0xFFFFFFFFU);
    (void)rdram;
}

static void hle_sceKernelGetSystemTimeWide(
    uint8_t* rdram, recomp_context* ctx
) {
    // 64-bit return: low 32 in v0, high 32 in v1
    // PSP actually returns 64-bit in v0:v1 pair
    uint64_t us = get_system_time_us();
    ctx->r[2] = static_cast<int32_t>(us & 0xFFFFFFFFU);
    ctx->r[3] = static_cast<int32_t>((us >> 32) & 0xFFFFFFFFU);
    (void)rdram;
}

static void hle_sceKernelWaitThreadEnd(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    uint32_t timeout_ptr = static_cast<uint32_t>(ctx->r[5]);

    auto it = g_thread_by_uid.find(uid);
    if (it == g_thread_by_uid.end()) {
        ctx->r[2] = SCE_KERNEL_ERROR_NOT_FOUND_THREAD;
        (void)rdram;
        return;
    }

    int thid = it->second.thid;
    int timeout_us = 0;
    if (timeout_ptr != 0) {
        timeout_us = static_cast<int>(
            psp_mem_read<uint32_t>(rdram, timeout_ptr));
    }

    sched_yield_point();
    int rc = psp_thread_wait_end(thid, timeout_us);
    ctx->r[2] = rc;
}

static void hle_sceKernelWaitThreadEndCB(
    uint8_t* rdram, recomp_context* ctx
) {
    psp_kernel_check_callbacks(rdram, ctx);
    hle_sceKernelWaitThreadEnd(rdram, ctx);
}

static void hle_sceKernelGetThreadStackFreeSize(
    uint8_t* rdram, recomp_context* ctx
) {
    // Return a reasonable free stack size
    ctx->r[2] = 0x4000;  // 16KB free
    (void)rdram;
}

static void hle_sceKernelCreateCallback(
    uint8_t* rdram, recomp_context* ctx
) {
    uint32_t name_ptr = static_cast<uint32_t>(ctx->r[4]);
    uint32_t func_addr = static_cast<uint32_t>(ctx->r[5]);
    uint32_t user_arg = static_cast<uint32_t>(ctx->r[6]);

    const char* name = reinterpret_cast<const char*>(
        rdram + (name_ptr & PSP_ADDR_MASK));

    int uid = psp_next_uid();
    PspCallback cb{};
    cb.uid = uid;
    std::strncpy(cb.name, name, sizeof(cb.name) - 1);
    cb.func_addr = func_addr;
    cb.user_arg = user_arg;
    cb.pending = false;
    cb.notify_count = 0;
    cb.notify_arg = 0;
    g_callbacks[uid] = cb;

    std::fprintf(stderr,
        "[HLE] sceKernelCreateCallback(\"%s\", 0x%08X, "
        "arg=0x%08X) -> uid=%d\n",
        name, func_addr, user_arg, uid);

    ctx->r[2] = uid;
}

static void hle_sceKernelDeleteCallback(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];
    g_callbacks.erase(uid);
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelCheckCallback(
    uint8_t* rdram, recomp_context* ctx
) {
    // Iterate callbacks looking for pending dispatch
    for (auto& [id, cb] : g_callbacks) {
        if (!cb.pending) {
            continue;
        }
        cb.pending = false;

        FuncPtr fn = RECOMP_LOOKUP(cb.func_addr);
        if (!fn) {
            std::fprintf(stderr,
                "[HLE] CheckCallback: LOOKUP_MISS for "
                "cb uid=%d func=0x%08X\n",
                cb.uid, cb.func_addr);
            continue;
        }

        // Save registers that callback will clobber
        int32_t saved_a0 = ctx->r[4];
        int32_t saved_a1 = ctx->r[5];
        int32_t saved_a2 = ctx->r[6];
        int32_t saved_ra = ctx->r[31];

        // PSP callback ABI (PPSSPP HLE/sceKernelThread.cpp):
        // a0=notify_count, a1=notify_arg, a2=common_arg
        ctx->r[4] = cb.notify_count;
        ctx->r[5] = cb.notify_arg;
        ctx->r[6] = static_cast<int32_t>(cb.user_arg);
        ctx->r[31] = 0;  // ra=0 so callback returns cleanly

        std::fprintf(stderr,
            "[HLE] CheckCallback: DISPATCHING cb uid=%d "
            "func=0x%08X notify_count=%d notify_arg=%d "
            "user_arg=0x%08X\n",
            cb.uid, cb.func_addr, cb.notify_count,
            cb.notify_arg, cb.user_arg);

        // Game-module observer (#47 P5 seam): address-keyed callback
        // diagnostics live in the game module, not here.
        if (g_callback_dispatch_observer != nullptr) {
            g_callback_dispatch_observer(
                rdram, cb.func_addr, cb.notify_arg);
        }

        fn(rdram, ctx);

        std::fprintf(stderr,
            "[HLE] CheckCallback: RETURNED from cb uid=%d "
            "func=0x%08X v0=0x%08X\n",
            cb.uid, cb.func_addr,
            static_cast<uint32_t>(ctx->r[2]));

        // Restore caller registers
        ctx->r[4] = saved_a0;
        ctx->r[5] = saved_a1;
        ctx->r[6] = saved_a2;
        ctx->r[31] = saved_ra;

        // Return count=1 (one callback dispatched)
        ctx->r[2] = 1;
        return;
    }

    // No pending callbacks
    ctx->r[2] = 0;
}

// ---- Callback Check (called from *CB HLE variants) ----

void psp_kernel_check_callbacks(
    uint8_t* rdram, recomp_context* ctx
) {
    hle_sceKernelCheckCallback(rdram, ctx);
}

// ---- Callback Notification (called from IO subsystem) ----

void psp_kernel_notify_callback(int cbId, int notifyArg) {
    auto it = g_callbacks.find(cbId);
    if (it == g_callbacks.end()) {
        return;
    }
    static int notify_log_count = 0;
    if (notify_log_count < 200) {
        std::fprintf(stderr,
            "[HLE] notify_callback(cbId=%d, notifyArg=0x%08X) "
            "was_pending=%d old_arg=0x%08X\n",
            cbId, static_cast<uint32_t>(notifyArg),
            it->second.pending ? 1 : 0,
            static_cast<uint32_t>(it->second.notify_arg));
        notify_log_count++;
    }
    it->second.pending = true;
    it->second.notify_count++;
    it->second.notify_arg = notifyArg;
}

static void hle_sceKernelSuspendThread(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelResumeThread(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelWakeupThread(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];

    auto it = g_thread_by_uid.find(uid);
    if (it == g_thread_by_uid.end()) {
        std::fprintf(stderr,
            "[HLE] sceKernelWakeupThread(uid=%d) NOT FOUND\n",
            uid);
        ctx->r[2] = SCE_KERNEL_ERROR_NOT_FOUND_THREAD;
        (void)rdram;
        return;
    }

    int thid = it->second.thid;
    int rc = psp_thread_wakeup(thid);
    ctx->r[2] = (rc == 0) ? SCE_OK : rc;
    (void)rdram;
}

static void hle_sceKernelTerminateThread(
    uint8_t* rdram, recomp_context* ctx
) {
    int uid = ctx->r[4];

    auto it = g_thread_by_uid.find(uid);
    if (it == g_thread_by_uid.end()) {
        ctx->r[2] = SCE_KERNEL_ERROR_NOT_FOUND_THREAD;
        (void)rdram;
        return;
    }

    int thid = it->second.thid;
    PspThread* pt = psp_get_thread(thid);
    if (pt) {
        std::fprintf(stderr,
            "[HLE] sceKernelTerminateThread(uid=%d, \"%s\")\n",
            uid, it->second.name);
    }

    ctx->r[2] = SCE_OK;
    (void)rdram;
}

static void hle_sceKernelTerminateDeleteThread(
    uint8_t* rdram, recomp_context* ctx
) {
    hle_sceKernelTerminateThread(rdram, ctx);
    // Also remove from tracking
    int uid = ctx->r[4];
    g_thread_by_uid.erase(uid);
}

// ---- Registration ----

void psp_hle_register_kernel_thread() {
    psp_hle_register("sceKernelCreateThread",
                      hle_sceKernelCreateThread);
    psp_hle_register("sceKernelStartThread",
                      hle_sceKernelStartThread);
    psp_hle_register("sceKernelExitThread",
                      hle_sceKernelExitThread);
    psp_hle_register("sceKernelExitDeleteThread",
                      hle_sceKernelExitDeleteThread);
    psp_hle_register("sceKernelDeleteThread",
                      hle_sceKernelDeleteThread);
    psp_hle_register("sceKernelDelayThread",
                      hle_sceKernelDelayThread);
    psp_hle_register("sceKernelDelayThreadCB",
                      hle_sceKernelDelayThreadCB);
    psp_hle_register("sceKernelSleepThread",
                      hle_sceKernelSleepThread);
    psp_hle_register("sceKernelSleepThreadCB",
                      hle_sceKernelSleepThreadCB);
    psp_hle_register("sceKernelGetThreadId",
                      hle_sceKernelGetThreadId);
    psp_hle_register("sceKernelReferThreadStatus",
                      hle_sceKernelReferThreadStatus);
    psp_hle_register("sceKernelChangeThreadPriority",
                      hle_sceKernelChangeThreadPriority);
    psp_hle_register("sceKernelChangeCurrentThreadAttr",
                      hle_sceKernelChangeCurrentThreadAttr);
    psp_hle_register("sceKernelGetSystemTimeLow",
                      hle_sceKernelGetSystemTimeLow);
    psp_hle_register("sceKernelGetSystemTimeWide",
                      hle_sceKernelGetSystemTimeWide);
    psp_hle_register("sceKernelWaitThreadEnd",
                      hle_sceKernelWaitThreadEnd);
    psp_hle_register("sceKernelWaitThreadEndCB",
                      hle_sceKernelWaitThreadEndCB);
    psp_hle_register("sceKernelGetThreadStackFreeSize",
                      hle_sceKernelGetThreadStackFreeSize);
    psp_hle_register("sceKernelCreateCallback",
                      hle_sceKernelCreateCallback);
    psp_hle_register("sceKernelDeleteCallback",
                      hle_sceKernelDeleteCallback);
    psp_hle_register("sceKernelCheckCallback",
                      hle_sceKernelCheckCallback);
    psp_hle_register("sceKernelSuspendThread",
                      hle_sceKernelSuspendThread);
    psp_hle_register("sceKernelResumeThread",
                      hle_sceKernelResumeThread);
    psp_hle_register("sceKernelWakeupThread",
                      hle_sceKernelWakeupThread);
    psp_hle_register("sceKernelTerminateThread",
                      hle_sceKernelTerminateThread);
    psp_hle_register("sceKernelTerminateDeleteThread",
                      hle_sceKernelTerminateDeleteThread);
}
