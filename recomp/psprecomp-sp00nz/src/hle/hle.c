/* psprecomp — HLE dispatch. See include/psprecomp/hle.h. */

#include "psprecomp/hle.h"
#include "psprecomp/dispatch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HLE_MAX 512

static psp_hle_entry g_entry[HLE_MAX];
static psp_hle_fn    g_fn[HLE_MAX];
static int           g_count;

void psp_hle_register(uint32_t nid, const char *lib, const char *name, psp_hle_fn fn) {
    /* Re-registering replaces, so a game repo can override one function
     * without forking the table. */
    for (int i = 0; i < g_count; i++) {
        if (g_entry[i].nid == nid) { g_fn[i] = fn; g_entry[i].name = name; return; }
    }
    if (g_count >= HLE_MAX) return;
    g_entry[g_count].nid = nid;
    g_entry[g_count].lib = lib;
    g_entry[g_count].name = name;
    g_fn[g_count] = fn;
    g_count++;
}

/* Register a NID whose name is genuinely unknown.
 *
 * Every named entry is verified against SHA-1 of its own name, which is what
 * makes the table self-checking -- and which correctly refuses a made-up name.
 * But a firmware entry point can be *observed* without being *identified*: the
 * game calls it, the NID is exact, and no plausible name hashes to it.
 *
 * The wrong response is to invent a name so the entry can be registered. That
 * produces a table which runs, looks right, and lies about what it implements.
 * The right response is to make "unidentified" representable: registered and
 * callable, marked as unnamed, and skipped by the NID check by construction
 * rather than by exception. */
void psp_hle_register_unnamed(uint32_t nid, const char *lib, psp_hle_fn fn) {
    psp_hle_register(nid, lib, NULL, fn);
}

int psp_hle_is_named(int index) {
    return index >= 0 && index < g_count && g_entry[index].name != NULL;
}

const char *psp_hle_name(uint32_t nid) {
    for (int i = 0; i < g_count; i++)
        if (g_entry[i].nid == nid) return g_entry[i].name;
    return NULL;
}

int psp_hle_count(void) { return g_count; }

const psp_hle_entry *psp_hle_entries(int *count) {
    if (count) *count = g_count;
    return g_entry;
}

/* A short history of firmware calls that returned zero.
 *
 * Zero is the value a game most often treats as an address, so a stubbed or
 * failing call returning it tends to surface far away as a wild pointer --
 * typically a table walk starting near address 0. When that happens the useful
 * question is "which call handed the game a null?", and by then the call is
 * long gone. Keeping the last few makes the answer immediate instead of
 * requiring a second run with different instrumentation. */
#define ZERO_HISTORY 16
static struct { uint32_t nid; const char *name; } g_zero[ZERO_HISTORY];
static int g_zero_n;

static void note_zero(uint32_t nid, const char *name) {
    g_zero[g_zero_n % ZERO_HISTORY].nid = nid;
    g_zero[g_zero_n % ZERO_HISTORY].name = name;
    g_zero_n++;
}

void psp_hle_dump_recent(FILE *out) {
    if (!g_zero_n) {
        fprintf(out, "  (no firmware call returned zero)\n");
        return;
    }
    int n = g_zero_n < ZERO_HISTORY ? g_zero_n : ZERO_HISTORY;
    /* Read this list with care: SCE_KERNEL_ERROR_OK is also zero, so a
     * successful call that returns nothing appears here exactly like a
     * handle-returning call that failed. The entries worth suspecting are the
     * ones whose name implies an address or an id -- GetBlockHeadAddr,
     * GetModuleId, the Create/Alloc family. The rest are noise.
     *
     * That ambiguity is a limitation of this instrument, not of the runtime:
     * distinguishing the two needs per-function knowledge of what the return
     * value means, which the table does not currently carry. */
    fprintf(out, "  last %d firmware calls that returned zero, newest first\n"
                 "  (note: success is also zero -- see the comment in hle.c):\n", n);
    for (int i = 1; i <= n; i++) {
        int k = (g_zero_n - i) % ZERO_HISTORY;
        fprintf(out, "    0x%08X  %s\n", g_zero[k].nid,
                g_zero[k].name ? g_zero[k].name : "(unimplemented)");
    }
}

void psp_hle_call(uint32_t nid) {
    if (nid == 0x237DBD4Fu)
        fprintf(stderr, "psp_hle_call ENTERED for 0x237DBD4F (%d entries registered)\n", g_count);
    for (int i = 0; i < g_count; i++) {
        if (g_entry[i].nid == nid) {
            g_fn[i]();
            if (psp_cpu.r[PSP_REG_V0] == 0) note_zero(nid, g_entry[i].name);
            return;
        }
    }
    note_zero(nid, NULL);

    /* Unimplemented. Naming the function is the whole point — bringing a game
     * up is largely the process of watching this message stop appearing, and
     * "0x237DBD4F" is far less use than "sceKernelAllocPartitionMemory".
     *
     * Returning 0 rather than aborting is deliberate: many firmware calls are
     * advisory (version reporting, profiling hooks) and a game will run past
     * them happily. One that genuinely needed the result will fail visibly
     * soon after, with this line already in the log. */
    fprintf(stderr, "psprecomp: unimplemented firmware call 0x%08X\n", nid);
    psp_ret(0);
}

const char *psp_str(uint32_t addr, char *dst, size_t cap) {
    size_t n = 0;
    if (cap == 0) return dst;
    while (n + 1 < cap) {
        uint8_t c = psp_read8(addr + (uint32_t)n);
        if (!c) break;
        dst[n++] = (char)c;
    }
    dst[n] = '\0';
    return dst;
}

static void miss_context(void) { psp_trace_dump(); psp_hle_dump_recent(stderr); }

void psp_hle_init(void) {
    psp_set_miss_context(miss_context);
    psp_sysmem_init();
    psp_sysmem_register();
    psp_threadman_init();
    psp_threadman_register();
    psp_display_init();
    psp_display_register();
    psp_ge_init();
    psp_ge_register();
    psp_sas_init();
    psp_sas_register();
    psp_io_init();
    psp_io_register();
    psp_misc_init();
    psp_misc_register();
}
