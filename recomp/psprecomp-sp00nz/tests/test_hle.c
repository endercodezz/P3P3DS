/* HLE tests — the firmware layer, with no game data involved.
 *
 * The headline check is the first one: **every registered NID is verified
 * against the SHA-1 of its own declared name.** A PSP NID is defined as the
 * first four bytes of SHA-1(name), so this is not a convention we are choosing
 * to follow — it is the identity the hardware uses, and it makes the whole
 * table self-verifying. A mistyped NID or a wrong function name cannot get
 * past this, which matters because such a mistake produces a game that runs
 * and misbehaves in a way indistinguishable from a codegen bug.
 */

#include "psprecomp/hle.h"
#include "psprecomp/dispatch.h"
#include "crypto/sha1.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond, ...)                                       \
    do {                                                       \
        if (!(cond)) {                                         \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);        \
            printf(__VA_ARGS__);                               \
            printf("\n");                                      \
            failures++;                                        \
        }                                                      \
    } while (0)

/* Invoke a registered firmware call with o32 arguments. */
static uint32_t call(uint32_t nid, uint32_t a0, uint32_t a1, uint32_t a2,
                     uint32_t a3) {
    psp_cpu.r[PSP_REG_A0] = a0;
    psp_cpu.r[PSP_REG_A1] = a1;
    psp_cpu.r[PSP_REG_A2] = a2;
    psp_cpu.r[PSP_REG_A3] = a3;
    psp_hle_call(nid);
    return psp_cpu.r[PSP_REG_V0];
}

/* Stack arguments live at $sp+16 onward. */
static uint32_t call5(uint32_t nid, uint32_t a0, uint32_t a1, uint32_t a2,
                      uint32_t a3, uint32_t a4) {
    psp_write32(psp_cpu.r[PSP_REG_SP] + 16, a4);
    return call(nid, a0, a1, a2, a3);
}

static void test_sha1_vectors(void) {
    /* FIPS 180-4 examples, so the hash itself is trusted before anything is
     * built on it. */
    uint8_t d[20];
    char hex[41];

    sha1("abc", 3, d);
    for (int i = 0; i < 20; i++) sprintf(hex + i * 2, "%02x", d[i]);
    CHECK(strcmp(hex, "a9993e364706816aba3e25717850c26c9cd0d89d") == 0,
          "SHA-1(\"abc\"): got %s", hex);

    sha1("", 0, d);
    for (int i = 0; i < 20; i++) sprintf(hex + i * 2, "%02x", d[i]);
    CHECK(strcmp(hex, "da39a3ee5e6b4b0d3255bfef95601890afd80709") == 0,
          "SHA-1(\"\"): got %s", hex);

    sha1("The quick brown fox jumps over the lazy dog", 43, d);
    for (int i = 0; i < 20; i++) sprintf(hex + i * 2, "%02x", d[i]);
    CHECK(strcmp(hex, "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12") == 0,
          "SHA-1(\"The quick brown fox...\"): got %s", hex);

    /* 56 bytes is the boundary case: the 0x80 terminator fits in the first
     * block but the 8-byte length does not, so padding spills into a second
     * block. An implementation that gets this wrong passes every shorter
     * vector. */
    const char *m = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    CHECK(strlen(m) == 56, "the two-block padding case is 56 bytes");
    sha1(m, 56, d);
    for (int i = 0; i < 20; i++) sprintf(hex + i * 2, "%02x", d[i]);
    CHECK(strcmp(hex, "c2db330f6083854c99d4b5bfb6e8f29f201be699") == 0,
          "SHA-1 of 56 'a's (two-block padding): got %s", hex);
}

/* THE important test: the registered table describes itself correctly. */
static void test_nids_match_names(void) {
    int n = 0;
    const psp_hle_entry *e = psp_hle_entries(&n);
    CHECK(n > 0, "something is registered");

    int checked = 0, unnamed = 0;
    for (int i = 0; i < n; i++) {
        /* A NID may be observed without being identified: the game calls it,
         * the NID is exact, and no plausible name hashes to it. Those are
         * registered unnamed rather than under an invented name, and are
         * skipped here by construction. Inventing a name to satisfy this check
         * would defeat the only thing that makes the table trustworthy. */
        if (!e[i].name) { unnamed++; continue; }        uint32_t want = psp_nid(e[i].name);
        if (want != e[i].nid) {
            printf("FAIL %s::%s\n  registered 0x%08X but SHA-1(name) gives 0x%08X\n",
                   e[i].lib, e[i].name, e[i].nid, want);
            failures++;
        }
        checked++;
    }
    printf("  verified %d NIDs against SHA-1 of their names\n", checked);
}

static void test_sysmem(void) {
    psp_sysmem_reset();

    const uint32_t NID_ALLOC = psp_nid("sceKernelAllocPartitionMemory");
    const uint32_t NID_FREE  = psp_nid("sceKernelFreePartitionMemory");
    const uint32_t NID_HEAD  = psp_nid("sceKernelGetBlockHeadAddr");

    uint32_t before = psp_sysmem_free();

    /* Low placement. */
    uint32_t uid = call5(NID_ALLOC, 2, 0, 0 /*Low*/, 0x1000, 0);
    CHECK(uid >= 0x00010000u, "low alloc returns a UID, got 0x%08X", uid);
    uint32_t lo = call(NID_HEAD, uid, 0, 0, 0);
    CHECK(lo != 0, "block has an address");

    /* High placement must land above the low one -- games depend on the
     * distinction, so it is not enough that both merely succeed. */
    uint32_t uid2 = call5(NID_ALLOC, 2, 0, 1 /*High*/, 0x1000, 0);
    uint32_t hi = call(NID_HEAD, uid2, 0, 0, 0);
    CHECK(hi > lo, "high allocation sits above the low one (lo=0x%08X hi=0x%08X)", lo, hi);

    /* Blocks must not overlap. */
    CHECK(lo + 0x1000 <= hi, "blocks do not overlap");

    /* Sizes round up to the hardware's 256-byte granule. */
    uint32_t uid3 = call5(NID_ALLOC, 2, 0, 0, 100, 0);
    CHECK(uid3 >= 0x00010000u, "a 100-byte request succeeds");
    CHECK(psp_sysmem_free() % 0x100 == 0, "allocations are granule-rounded");

    /* Freeing returns the memory. */
    uint32_t mid = psp_sysmem_free();
    CHECK(call(NID_FREE, uid, 0, 0, 0) == 0, "free succeeds");
    CHECK(psp_sysmem_free() > mid, "freeing returns memory");

    CHECK(call(NID_FREE, 0xDEADBEEF, 0, 0, 0) == SCE_KERNEL_ERROR_UNKNOWN_UID,
          "freeing an unknown UID is refused");
    CHECK(call(NID_HEAD, 0xDEADBEEF, 0, 0, 0) == 0,
          "an unknown UID has no address");

    /* An impossible request fails rather than returning a bogus block. */
    uint32_t huge = call5(NID_ALLOC, 2, 0, 0, 0x7F000000u, 0);
    CHECK(huge == SCE_KERNEL_ERROR_NO_MEMORY, "an oversized request fails, got 0x%08X", huge);

    call(NID_FREE, uid2, 0, 0, 0);
    call(NID_FREE, uid3, 0, 0, 0);
    CHECK(psp_sysmem_free() == before, "everything freed restores the heap");
}

static void test_semaphores(void) {
    psp_threadman_reset();

    const uint32_t CREATE = psp_nid("sceKernelCreateSema");
    const uint32_t WAIT   = psp_nid("sceKernelWaitSema");
    const uint32_t SIGNAL = psp_nid("sceKernelSignalSema");
    const uint32_t DELETE = psp_nid("sceKernelDeleteSema");

    uint32_t sem = call5(CREATE, 0, 0, 2 /*init*/, 4 /*max*/, 0);
    CHECK(sem != 0, "semaphore created");

    CHECK(call(WAIT, sem, 1, 0, 0) == 0, "wait succeeds while the count allows");
    CHECK(call(WAIT, sem, 1, 0, 0) == 0, "and again, down to zero");
    CHECK(call(WAIT, sem, 1, 0, 0) == SCE_KERNEL_ERROR_WAIT_TIMEOUT,
          "a wait that would block reports a timeout rather than hanging");

    CHECK(call(SIGNAL, sem, 1, 0, 0) == 0, "signal succeeds");
    CHECK(call(WAIT, sem, 1, 0, 0) == 0, "and the signalled count is available");

    /* The count must saturate at max, not run away. */
    call(SIGNAL, sem, 100, 0, 0);
    CHECK(call(WAIT, sem, 4, 0, 0) == 0, "count saturates at max (4 available)");
    CHECK(call(WAIT, sem, 1, 0, 0) == SCE_KERNEL_ERROR_WAIT_TIMEOUT,
          "and no more than max");

    CHECK(call(DELETE, sem, 0, 0, 0) == 0, "delete succeeds");
    CHECK(call(WAIT, sem, 1, 0, 0) == SCE_KERNEL_ERROR_UNKNOWN_UID,
          "a deleted semaphore is gone");
}

static void test_event_flags(void) {
    psp_threadman_reset();

    const uint32_t CREATE = psp_nid("sceKernelCreateEventFlag");
    const uint32_t SET    = psp_nid("sceKernelSetEventFlag");
    const uint32_t CLEAR  = psp_nid("sceKernelClearEventFlag");
    const uint32_t WAIT   = psp_nid("sceKernelWaitEventFlag");

    uint32_t ef = call(CREATE, 0, 0, 0x0000, 0);
    CHECK(ef != 0, "event flag created");

    call(SET, ef, 0x0005, 0, 0);

    /* WAITOR is satisfied by any bit; WAITAND needs all of them. */
    CHECK(call5(WAIT, ef, 0x0004, 0x01 /*OR*/, 0, 0) == 0, "OR wait on a set bit");
    CHECK(call5(WAIT, ef, 0x0003, 0x00 /*AND*/, 0, 0) == SCE_KERNEL_ERROR_WAIT_TIMEOUT,
          "AND wait fails when only some bits are set");
    CHECK(call5(WAIT, ef, 0x0005, 0x00 /*AND*/, 0, 0) == 0,
          "AND wait succeeds when all bits are set");

    /* clear takes a mask of bits to KEEP. Getting that backwards leaves a game
     * waiting on a flag that never clears, so it is pinned explicitly. */
    call(CLEAR, ef, ~0x0004u, 0, 0);
    CHECK(call5(WAIT, ef, 0x0004, 0x01, 0, 0) == SCE_KERNEL_ERROR_WAIT_TIMEOUT,
          "the cleared bit is gone");
    CHECK(call5(WAIT, ef, 0x0001, 0x01, 0, 0) == 0,
          "the kept bit survives");
}

/* A recompiled thread entry: writes a marker so the test can prove it ran on
 * the stack the thread manager gave it. */
static uint32_t g_thread_sp;
static uint32_t g_thread_arg;
static int      g_thread_ran;

static void fake_thread_entry(void) {
    g_thread_ran++;
    g_thread_sp  = psp_cpu.r[PSP_REG_SP];
    g_thread_arg = psp_cpu.r[PSP_REG_A0];
    psp_cpu.r[PSP_REG_V0] = 0x1234;      /* exit status */
}

static void test_threads(void) {
    psp_sysmem_reset();
    psp_threadman_reset();
    psp_dispatch_reset();

    const uint32_t CREATE = psp_nid("sceKernelCreateThread");
    const uint32_t START  = psp_nid("sceKernelStartThread");
    const uint32_t WAITEND= psp_nid("sceKernelWaitThreadEnd");
    const uint32_t DELETE = psp_nid("sceKernelDeleteThread");
    const uint32_t GETID  = psp_nid("sceKernelGetThreadId");

    const uint32_t ENTRY = 0x08801000u;
    psp_register(ENTRY, fake_thread_entry);

    uint32_t thid = call5(CREATE, 0 /*name*/, ENTRY, 32 /*prio*/, 0x4000 /*stack*/, 0);
    CHECK(thid != 0, "thread created, got 0x%08X", thid);

    /* The caller's context must survive the thread running. */
    psp_cpu.r[PSP_REG_S0] = 0xC0FFEE;
    uint32_t caller_sp = psp_cpu.r[PSP_REG_SP];

    g_thread_ran = 0;
    uint32_t rc = call(START, thid, 7 /*arglen*/, 0xAAAA, 0);
    CHECK(rc == 0, "start succeeds");
    CHECK(g_thread_ran == 1, "the thread entry actually ran");
    CHECK(g_thread_arg == 7, "argument reached the thread, got %u", g_thread_arg);
    CHECK(g_thread_sp != caller_sp && g_thread_sp != 0,
          "the thread ran on its own stack (0x%08X vs caller 0x%08X)",
          g_thread_sp, caller_sp);
    CHECK((g_thread_sp & 15) == 0, "the thread stack pointer is 16-byte aligned");

    CHECK(psp_cpu.r[PSP_REG_S0] == 0xC0FFEE, "caller's registers restored");
    CHECK(psp_cpu.r[PSP_REG_SP] == caller_sp, "caller's stack pointer restored");

    /* The exit status the thread returned is what a waiter sees. */
    uint32_t out = 0x08802000u;
    CHECK(call(WAITEND, thid, out, 0, 0) == 0, "wait-for-end succeeds");
    CHECK(psp_read32(out) == 0x1234, "exit status recorded, got 0x%X", psp_read32(out));

    /* Deleting frees the stack. */
    uint32_t before_delete = psp_sysmem_free();
    CHECK(call(DELETE, thid, 0, 0, 0) == 0, "delete succeeds");
    CHECK(psp_sysmem_free() > before_delete, "deleting a thread frees its stack");

    CHECK(call(START, 0xDEADBEEF, 0, 0, 0) == SCE_KERNEL_ERROR_ILLEGAL_THID,
          "starting an unknown thread is refused");
    CHECK(call(GETID, 0, 0, 0, 0) == 0, "no current thread outside one");
}

static void test_guest_strings(void) {
    /* Names come out of guest memory, so the reader has to terminate and must
     * not run past the buffer. */
    const uint32_t at = 0x08803000u;
    const char *s = "sceThreadName";
    for (uint32_t i = 0; i <= strlen(s); i++) psp_write8(at + i, (uint8_t)s[i]);

    char buf[32];
    CHECK(strcmp(psp_str(at, buf, sizeof buf), s) == 0, "string round-trips");

    char small[6];
    psp_str(at, small, sizeof small);
    CHECK(strlen(small) == 5 && strncmp(small, s, 5) == 0,
          "an over-long string is truncated, not overflowed: \"%s\"", small);
}

/* Build a display list in guest memory and check the walk follows control flow
 * rather than merely counting words. The list deliberately contains a PRIM
 * that a JUMP skips over: if it gets counted, the walk is not following jumps. */
static void test_ge_display_list(void) {
    psp_ge_reset();

    const uint32_t LIST = 0x08820000u;
    uint32_t w[16], n = 0;

    /* GE addresses are 24-bit; BASE supplies the high byte. Setting it here
     * also exercises that path -- a list whose BASE is ignored jumps to the
     * wrong place and silently executes garbage. */
    w[n++] = (0x10u << 24) | 0x080000;              /* BASE  -> 0x08000000 */
    w[n++] = (0x12u << 24) | 0x000123;              /* VTYPE */
    w[n++] = (0x04u << 24) | (3u << 16) | 6;        /* PRIM triangles, 6 verts */
    w[n++] = (0x08u << 24) | 0x820020;              /* JUMP -> LIST + 0x20 */
    w[n++] = (0x04u << 24) | (0u << 16) | 99;       /* PRIM points -- SKIPPED */
    w[n++] = (0x00u << 24);
    w[n++] = (0x00u << 24);
    w[n++] = (0x00u << 24);
    /* word 8 == LIST + 0x20 */
    w[n++] = (0x04u << 24) | (6u << 16) | 2;        /* PRIM sprites, 2 verts */
    w[n++] = (0x0Fu << 24);                          /* FINISH */
    w[n++] = (0x0Cu << 24);                          /* END */

    for (uint32_t i = 0; i < n; i++) psp_write32(LIST + i * 4, w[i]);

    uint64_t before = psp_ge_command_count();
    /* sceGeListEnQueue(list, stall=0, cbid, arg) */
    uint32_t qid = call(psp_nid("sceGeListEnQueue"), LIST, 0, 0, 0);
    CHECK(qid != 0, "list enqueued, got 0x%08X", qid);

    uint64_t executed = psp_ge_command_count() - before;
    CHECK(executed == 6, "walked BASE,VTYPE,PRIM,JUMP,PRIM,FINISH = 6, got %llu",
          (unsigned long long)executed);
    CHECK(psp_ge_vertex_count() == 8,
          "counted 6+2 vertices and skipped the jumped-over PRIM, got %llu",
          (unsigned long long)psp_ge_vertex_count());

    CHECK(call(psp_nid("sceGeDrawSync"), 0, 0, 0, 0) == 0, "draw sync succeeds");
    CHECK(call(psp_nid("sceGeEdramGetAddr"), 0, 0, 0, 0) == PSP_VRAM_BASE,
          "eDRAM is the VRAM window");
    CHECK(call(psp_nid("sceGeEdramGetSize"), 0, 0, 0, 0) == PSP_VRAM_SIZE,
          "eDRAM is 2 MB");
}

/* A list that jumps to itself must terminate rather than hang the host -- this
 * is a normal transient state while the CPU is still writing the list. */
static void test_ge_infinite_list(void) {
    psp_ge_reset();
    const uint32_t LIST = 0x08830000u;
    psp_write32(LIST + 0, (0x10u << 24) | 0x080000);   /* BASE */
    psp_write32(LIST + 4, (0x08u << 24) | 0x830000);   /* JUMP to itself */

    uint32_t qid = call(psp_nid("sceGeListEnQueue"), LIST, 0, 0, 0);
    CHECK(qid != 0, "a self-jumping list still returns");
}

static void test_sas_adpcm(void) {
    psp_sas_reset();

    /* One VAG block: shift 8, filter 0, then 28 nibbles. With filter 0 there is
     * no prediction, so each output sample is just the sign-extended nibble
     * scaled -- which makes a decode bug obvious rather than merely quieter. */
    const uint32_t VAG = 0x08840000u;
    psp_write8(VAG + 0, 0x08);        /* shift 8, filter 0 */
    psp_write8(VAG + 1, 0x00);        /* flags: not the end */
    for (uint32_t i = 0; i < 14; i++)
        psp_write8(VAG + 2 + i, 0x7Fu);   /* nibbles 0xF and 0x7 */

    CHECK(call5(psp_nid("__sceSasInit"), 0, 64 /*grain*/, 32, 0, 44100) == 0,
          "SAS init");

    /* sceSasSetVoice(core, voice, addr, size, loop) */
    CHECK(call5(psp_nid("__sceSasSetVoice"), 0, 0, VAG, 16, 0) == 0, "voice set");
    CHECK(call(psp_nid("__sceSasSetVolume"), 0, 0, 0x1000, 0x1000) == 0, "volume set");
    CHECK(call(psp_nid("__sceSasSetPitch"), 0, 0, 0x1000, 0) == 0, "pitch set");

    /* Before key-on nothing should be playing. */
    const uint32_t OUT = 0x08850000u;
    for (uint32_t i = 0; i < 64 * 4; i += 4) psp_write32(OUT + i, 0);
    call(psp_nid("__sceSasCore"), 0, OUT, 0, 0);
    CHECK(psp_sas_nonzero() == 0, "silence before key-on");

    CHECK(call(psp_nid("__sceSasSetKeyOn"), 0, 0, 0, 0) == 0, "key on");
    call(psp_nid("__sceSasCore"), 0, OUT, 0, 0);

    CHECK(psp_sas_frames() == 2, "two frames rendered, got %llu",
          (unsigned long long)psp_sas_frames());
    CHECK(psp_sas_nonzero() > 0, "audio was actually produced after key-on");

    /* The end flag is how a game knows a sound finished; a voice that never
     * reports ended is a common way for audio to stall after the first sound. */
    uint32_t ended = call(psp_nid("__sceSasGetEndFlag"), 0, 0, 0, 0);
    CHECK((ended & ~1u) != 0, "unused voices report ended, got 0x%08X", ended);
}

static void test_display(void) {
    psp_display_reset();

    CHECK(call(psp_nid("sceDisplaySetMode"), 0, 480, 272, 0) == 0, "set mode");
    /* (topaddr, bufferwidth, pixelformat, sync) */
    CHECK(call(psp_nid("sceDisplaySetFrameBuf"), PSP_VRAM_BASE, 512, 3, 0) == 0,
          "set framebuffer");
    CHECK(psp_display_framebuffer() == PSP_VRAM_BASE, "framebuffer recorded");

    uint64_t v0 = psp_display_vblanks();
    call(psp_nid("sceDisplayWaitVblank"), 0, 0, 0, 0);
    call(psp_nid("sceDisplayWaitVblankStartCB"), 0, 0, 0, 0);
    CHECK(psp_display_vblanks() == v0 + 2,
          "vblank waits advance the frame counter (the bring-up heartbeat)");
}

int main(void) {
    CHECK(psp_mem_init() == 0, "memory init");
    psp_cpu_reset();
    psp_cpu.r[PSP_REG_SP] = 0x08810000u;   /* a stack for the "caller" */
    psp_hle_init();

    printf("registered %d firmware functions\n", psp_hle_count());

    test_sha1_vectors();
    test_nids_match_names();
    test_sysmem();
    test_semaphores();
    test_event_flags();
    test_threads();
    test_guest_strings();
    test_ge_display_list();
    test_ge_infinite_list();
    test_sas_adpcm();
    test_display();

    psp_mem_free();

    if (failures) {
        printf("\n%d check(s) failed\n", failures);
        return 1;
    }
    printf("all HLE checks passed\n");
    return 0;
}
