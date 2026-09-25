/* psprecomp — ThreadManForUser.
 *
 * Threads, semaphores and event flags. This is the largest single dependency
 * any PSP game has: `module_start` typically does nothing but create a thread,
 * start it, and return — so until this works, a recompiled module runs about
 * forty instructions and stops.
 *
 * ## The execution model, and its ceiling
 *
 * Recompiled functions are ordinary C functions sharing one global register
 * file (`psp_cpu`). That makes true concurrent threads a much bigger change
 * than it looks: each PSP thread would need its own register context, and the
 * host would need to switch between them.
 *
 * So this implements the model that covers the overwhelmingly common case
 * exactly, and is honest about the rest:
 *
 *   - `sceKernelStartThread` **runs the thread to completion inline**, with the
 *     caller's register state saved and restored around it. For the standard
 *     `module_start` → create → start → return shape, this is not an
 *     approximation: it is what happens.
 *   - `sceKernelExitThread` unwinds to the matching start via longjmp, which is
 *     how a thread ends without returning normally.
 *   - A wait that *would block* returns a timeout instead of hanging, and says
 *     so once. With no preemption there is nothing to wait for, and a silent
 *     hang is the worst possible failure during bring-up.
 *
 * ponytail: single-threaded, run-to-completion. Real scheduling needs
 * per-thread register contexts (a `_Thread_local psp_cpu` plus a handoff lock,
 * since the PSP is single-core and never runs two threads at once anyway) —
 * worth doing when a game actually needs concurrent threads, and the thread
 * objects here are already shaped for it.
 */

#include "psprecomp/hle.h"
#include "psprecomp/dispatch.h"

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#define MAX_THREADS 128
#define MAX_SEMAS   128
#define MAX_FLAGS   128
#define MAX_CBS     64
#define UID_BASE    0x00040000u

enum { TH_DORMANT = 0, TH_READY, TH_RUNNING, TH_SUSPENDED };

typedef struct {
    uint32_t uid;
    char     name[32];
    uint32_t entry;
    uint32_t priority;
    uint32_t stack_size;
    uint32_t stack_base;   /* low address of the allocation */
    uint32_t attr;
    int      state;
    uint32_t exit_status;
    int      used;
    jmp_buf  unwind;       /* where sceKernelExitThread returns to */
    int      unwind_set;
} psp_thread;

typedef struct {
    uint32_t uid;
    char     name[32];
    int32_t  count;
    int32_t  max_count;
    int      used;
} psp_sema;

typedef struct {
    uint32_t uid;
    char     name[32];
    uint32_t pattern;
    int      used;
} psp_evflag;

typedef struct {
    uint32_t uid;
    char     name[32];
    uint32_t func;
    uint32_t arg;
    int      used;
} psp_callback;

static psp_thread   g_thread[MAX_THREADS];
static psp_sema     g_sema[MAX_SEMAS];
static psp_evflag   g_flag[MAX_FLAGS];
static psp_callback g_cb[MAX_CBS];
static uint32_t     g_next_uid;
static psp_thread  *g_current;
static int          g_warned_block;

void psp_threadman_reset(void) {
    memset(g_thread, 0, sizeof g_thread);
    memset(g_sema, 0, sizeof g_sema);
    memset(g_flag, 0, sizeof g_flag);
    memset(g_cb, 0, sizeof g_cb);
    g_next_uid = UID_BASE;
    g_current = NULL;
    g_warned_block = 0;
}

void psp_threadman_init(void) { psp_threadman_reset(); }

/* Typed lookups rather than one generic macro. A macro taking a parameter
 * named `uid` also rewrites every `.uid` member access it expands around,
 * which is exactly the kind of subtlety not worth inviting to save twelve
 * lines. */
static psp_thread *find_thread(uint32_t id) {
    for (int i = 0; i < MAX_THREADS; i++)
        if (g_thread[i].used && g_thread[i].uid == id) return &g_thread[i];
    return NULL;
}
static psp_sema *find_sema(uint32_t id) {
    for (int i = 0; i < MAX_SEMAS; i++)
        if (g_sema[i].used && g_sema[i].uid == id) return &g_sema[i];
    return NULL;
}
static psp_evflag *find_flag(uint32_t id) {
    for (int i = 0; i < MAX_FLAGS; i++)
        if (g_flag[i].used && g_flag[i].uid == id) return &g_flag[i];
    return NULL;
}

/* Report a would-block exactly once. Repeating it for every frame of a game
 * that polls a semaphore drowns out everything else in the log. */
static void warn_block(const char *what) {
    if (g_warned_block) return;
    g_warned_block = 1;
    fprintf(stderr,
        "psprecomp: %s would block, returning timeout.\n"
        "  The thread model runs one thread to completion (see src/hle/threadman.c);\n"
        "  there is no other thread to yield to. Further blocks are not reported.\n",
        what);
}

/* ---- threads ------------------------------------------------------------- */

static void hle_CreateThread(void) {
    /* (name, entry, priority, stackSize, attr, option) */
    psp_thread *t = NULL;
    for (int i = 0; i < MAX_THREADS; i++) if (!g_thread[i].used) { t = &g_thread[i]; break; }
    if (!t) { psp_ret(SCE_KERNEL_ERROR_NO_MEMORY); return; }

    memset(t, 0, sizeof *t);
    psp_str(psp_arg(0), t->name, sizeof t->name);
    t->entry      = psp_arg(1);
    t->priority   = psp_arg(2);
    t->stack_size = psp_arg(3);
    t->attr       = psp_arg(4);

    if (t->stack_size < 0x1000) t->stack_size = 0x1000;
    /* Stacks grow down, so allocate from the top of the heap: a stack that
     * overflows then runs into free space rather than into another block. */
    t->stack_base = psp_sysmem_alloc(t->stack_size, 1);
    if (!t->stack_base) { psp_ret(SCE_KERNEL_ERROR_NO_MEMORY); return; }

    t->uid = g_next_uid++;
    t->state = TH_DORMANT;
    t->used = 1;
    psp_ret(t->uid);
}

static void hle_StartThread(void) {
    /* (thid, arglen, argp) */
    psp_thread *t = find_thread(psp_arg(0));
    if (!t) { psp_ret(SCE_KERNEL_ERROR_ILLEGAL_THID); return; }

    uint32_t arglen = psp_arg(1);
    uint32_t argp   = psp_arg(2);

    /* Save the caller's whole context. The thread runs on the same register
     * file, so this *is* the context switch. */
    psp_cpu_state saved = psp_cpu;
    psp_thread   *prev  = g_current;

    psp_cpu.r[PSP_REG_A0] = arglen;
    psp_cpu.r[PSP_REG_A1] = argp;
    /* Stack pointer starts at the top of the allocation, 16-byte aligned, with
     * a little headroom so a callee storing below $sp cannot run off the end. */
    psp_cpu.r[PSP_REG_SP] = (t->stack_base + t->stack_size - 64) & ~15u;
    psp_cpu.r[PSP_REG_RA] = 0;

    t->state = TH_RUNNING;
    g_current = t;

    if (setjmp(t->unwind) == 0) {
        t->unwind_set = 1;
        psp_dispatch(t->entry);          /* runs to completion */
        t->exit_status = psp_cpu.r[PSP_REG_V0];
    }
    /* Landing here with a nonzero setjmp result means sceKernelExitThread
     * unwound out of the thread; exit_status was recorded there. */
    t->unwind_set = 0;
    t->state = TH_DORMANT;

    g_current = prev;
    psp_cpu = saved;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_ExitThread(void) {
    uint32_t status = psp_arg(0);
    if (g_current && g_current->unwind_set) {
        g_current->exit_status = status;
        longjmp(g_current->unwind, 1);
    }
    /* Nothing to unwind to: the module called ExitThread from its entry rather
     * than from a started thread. Nothing further can run. */
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_DeleteThread(void) {
    psp_thread *t = find_thread(psp_arg(0));
    if (!t) { psp_ret(SCE_KERNEL_ERROR_ILLEGAL_THID); return; }
    if (t->stack_base) psp_sysmem_release(t->stack_base);
    t->used = 0;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

/* With one thread running to completion there is nothing to schedule during a
 * delay, so it returns immediately. Time still advances for anything reading
 * the clock. */
static void hle_DelayThread(void)   { psp_ret(SCE_KERNEL_ERROR_OK); }

static void hle_WaitThreadEnd(void) {
    /* The thread already ran to completion inside StartThread, so by the time
     * anyone waits on it, it has ended. */
    psp_thread *t = find_thread(psp_arg(0));
    if (!t) { psp_ret(SCE_KERNEL_ERROR_ILLEGAL_THID); return; }
    uint32_t out = psp_arg(1);
    if (out) psp_write32(out, t->exit_status);
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_GetThreadId(void) { psp_ret(g_current ? g_current->uid : 0); }

static void hle_SuspendThread(void) {
    psp_thread *t = find_thread(psp_arg(0));
    if (!t) { psp_ret(SCE_KERNEL_ERROR_ILLEGAL_THID); return; }
    t->state = TH_SUSPENDED;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_ResumeThread(void) {
    psp_thread *t = find_thread(psp_arg(0));
    if (!t) { psp_ret(SCE_KERNEL_ERROR_ILLEGAL_THID); return; }
    t->state = TH_READY;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_ChangeThreadPriority(void) {
    psp_thread *t = find_thread(psp_arg(0));
    if (!t) { psp_ret(SCE_KERNEL_ERROR_ILLEGAL_THID); return; }
    t->priority = psp_arg(1);
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_GetThreadCurrentPriority(void) {
    psp_ret(g_current ? g_current->priority : 0);
}

static void hle_ChangeCurrentThreadAttr(void) {
    if (g_current) g_current->attr = (g_current->attr & ~psp_arg(0)) | psp_arg(1);
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_GetThreadStackFreeSize(void) {
    /* Real firmware walks the stack looking for the fill pattern. We do not
     * paint one, so report the whole stack: it is used for "am I close to
     * overflowing", and claiming plenty of room is the safe direction. */
    psp_thread *t = find_thread(psp_arg(0));
    psp_ret(t ? t->stack_size : (g_current ? g_current->stack_size : 0));
}

/* ---- semaphores ---------------------------------------------------------- */

static void hle_CreateSema(void) {
    /* (name, attr, initVal, maxVal, option) */
    psp_sema *s = NULL;
    for (int i = 0; i < MAX_SEMAS; i++) if (!g_sema[i].used) { s = &g_sema[i]; break; }
    if (!s) { psp_ret(SCE_KERNEL_ERROR_NO_MEMORY); return; }

    memset(s, 0, sizeof *s);
    psp_str(psp_arg(0), s->name, sizeof s->name);
    s->count     = (int32_t)psp_arg(2);
    s->max_count = (int32_t)psp_arg(3);
    s->uid = g_next_uid++;
    s->used = 1;
    psp_ret(s->uid);
}

static void hle_DeleteSema(void) {
    psp_sema *s = find_sema(psp_arg(0));
    if (!s) { psp_ret(SCE_KERNEL_ERROR_UNKNOWN_UID); return; }
    s->used = 0;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_SignalSema(void) {
    psp_sema *s = find_sema(psp_arg(0));
    if (!s) { psp_ret(SCE_KERNEL_ERROR_UNKNOWN_UID); return; }
    s->count += (int32_t)psp_arg(1);
    if (s->max_count > 0 && s->count > s->max_count) s->count = s->max_count;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_WaitSema(void) {
    psp_sema *s = find_sema(psp_arg(0));
    if (!s) { psp_ret(SCE_KERNEL_ERROR_UNKNOWN_UID); return; }
    int32_t need = (int32_t)psp_arg(1);
    if (s->count >= need) {
        s->count -= need;
        psp_ret(SCE_KERNEL_ERROR_OK);
        return;
    }
    warn_block("sceKernelWaitSema");
    psp_ret(SCE_KERNEL_ERROR_WAIT_TIMEOUT);
}

/* ---- event flags --------------------------------------------------------- */

#define PSP_EVENT_WAITAND   0x00
#define PSP_EVENT_WAITOR    0x01
#define PSP_EVENT_WAITCLEAR 0x20

static void hle_CreateEventFlag(void) {
    /* (name, attr, bits, option) */
    psp_evflag *f = NULL;
    for (int i = 0; i < MAX_FLAGS; i++) if (!g_flag[i].used) { f = &g_flag[i]; break; }
    if (!f) { psp_ret(SCE_KERNEL_ERROR_NO_MEMORY); return; }

    memset(f, 0, sizeof *f);
    psp_str(psp_arg(0), f->name, sizeof f->name);
    f->pattern = psp_arg(2);
    f->uid = g_next_uid++;
    f->used = 1;
    psp_ret(f->uid);
}

static void hle_DeleteEventFlag(void) {
    psp_evflag *f = find_flag(psp_arg(0));
    if (!f) { psp_ret(SCE_KERNEL_ERROR_UNKNOWN_UID); return; }
    f->used = 0;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_SetEventFlag(void) {
    psp_evflag *f = find_flag(psp_arg(0));
    if (!f) { psp_ret(SCE_KERNEL_ERROR_UNKNOWN_UID); return; }
    f->pattern |= psp_arg(1);
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_ClearEventFlag(void) {
    psp_evflag *f = find_flag(psp_arg(0));
    if (!f) { psp_ret(SCE_KERNEL_ERROR_UNKNOWN_UID); return; }
    /* The argument is a mask of bits to KEEP, not bits to clear. Getting this
     * backwards leaves a game waiting on a flag that never clears. */
    f->pattern &= psp_arg(1);
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_WaitEventFlag(void) {
    /* (evfid, bits, wait mode, outBits, timeout) */
    psp_evflag *f = find_flag(psp_arg(0));
    if (!f) { psp_ret(SCE_KERNEL_ERROR_UNKNOWN_UID); return; }

    uint32_t bits = psp_arg(1);
    uint32_t mode = psp_arg(2);
    uint32_t out  = psp_arg(3);

    int satisfied = (mode & PSP_EVENT_WAITOR)
                  ? (f->pattern & bits) != 0
                  : (f->pattern & bits) == bits;

    if (out) psp_write32(out, f->pattern);

    if (!satisfied) {
        warn_block("sceKernelWaitEventFlag");
        psp_ret(SCE_KERNEL_ERROR_WAIT_TIMEOUT);
        return;
    }
    if (mode & PSP_EVENT_WAITCLEAR) f->pattern &= ~bits;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

/* ---- callbacks ----------------------------------------------------------- */

static void hle_CreateCallback(void) {
    psp_callback *c = NULL;
    for (int i = 0; i < MAX_CBS; i++) if (!g_cb[i].used) { c = &g_cb[i]; break; }
    if (!c) { psp_ret(SCE_KERNEL_ERROR_NO_MEMORY); return; }

    memset(c, 0, sizeof *c);
    psp_str(psp_arg(0), c->name, sizeof c->name);
    c->func = psp_arg(1);
    c->arg  = psp_arg(2);
    c->uid  = g_next_uid++;
    c->used = 1;
    /* Registered but never fired: callbacks are delivered from the scheduler,
     * which does not exist yet. A game that only registers them (the common
     * case -- exit and power callbacks) is unaffected. */
    psp_ret(c->uid);
}

void psp_threadman_register(void) {
    /* NIDs are SHA-1(name)[0:4] little-endian; tests/test_hle.c verifies every
     * pair below. */
    psp_hle_register(0x446D8DE6, "ThreadManForUser", "sceKernelCreateThread",            hle_CreateThread);
    psp_hle_register(0xF475845D, "ThreadManForUser", "sceKernelStartThread",             hle_StartThread);
    psp_hle_register(0xAA73C935, "ThreadManForUser", "sceKernelExitThread",              hle_ExitThread);
    psp_hle_register(0x9FA03CD3, "ThreadManForUser", "sceKernelDeleteThread",            hle_DeleteThread);
    psp_hle_register(0xCEADEB47, "ThreadManForUser", "sceKernelDelayThread",             hle_DelayThread);
    psp_hle_register(0x68DA9E36, "ThreadManForUser", "sceKernelDelayThreadCB",           hle_DelayThread);
    psp_hle_register(0x278C0DF5, "ThreadManForUser", "sceKernelWaitThreadEnd",           hle_WaitThreadEnd);
    psp_hle_register(0x293B45B8, "ThreadManForUser", "sceKernelGetThreadId",             hle_GetThreadId);
    psp_hle_register(0x9944F31F, "ThreadManForUser", "sceKernelSuspendThread",           hle_SuspendThread);
    psp_hle_register(0x75156E8F, "ThreadManForUser", "sceKernelResumeThread",            hle_ResumeThread);
    psp_hle_register(0x71BC9871, "ThreadManForUser", "sceKernelChangeThreadPriority",    hle_ChangeThreadPriority);
    psp_hle_register(0x94AA61EE, "ThreadManForUser", "sceKernelGetThreadCurrentPriority",hle_GetThreadCurrentPriority);
    psp_hle_register(0xEA748E31, "ThreadManForUser", "sceKernelChangeCurrentThreadAttr", hle_ChangeCurrentThreadAttr);
    psp_hle_register(0x52089CA1, "ThreadManForUser", "sceKernelGetThreadStackFreeSize",  hle_GetThreadStackFreeSize);

    psp_hle_register(0xD6DA4BA1, "ThreadManForUser", "sceKernelCreateSema",              hle_CreateSema);
    psp_hle_register(0x28B6489C, "ThreadManForUser", "sceKernelDeleteSema",              hle_DeleteSema);
    psp_hle_register(0x3F53E640, "ThreadManForUser", "sceKernelSignalSema",              hle_SignalSema);
    psp_hle_register(0x4E3A1105, "ThreadManForUser", "sceKernelWaitSema",                hle_WaitSema);

    psp_hle_register(0x55C20A00, "ThreadManForUser", "sceKernelCreateEventFlag",         hle_CreateEventFlag);
    psp_hle_register(0xEF9E4C70, "ThreadManForUser", "sceKernelDeleteEventFlag",         hle_DeleteEventFlag);
    psp_hle_register(0x1FB15A32, "ThreadManForUser", "sceKernelSetEventFlag",            hle_SetEventFlag);
    psp_hle_register(0x812346E4, "ThreadManForUser", "sceKernelClearEventFlag",          hle_ClearEventFlag);
    psp_hle_register(0x402FCF22, "ThreadManForUser", "sceKernelWaitEventFlag",           hle_WaitEventFlag);

    psp_hle_register(0xE81CAF8F, "ThreadManForUser", "sceKernelCreateCallback",          hle_CreateCallback);
}
