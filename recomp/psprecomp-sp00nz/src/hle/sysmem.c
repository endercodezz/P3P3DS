/* psprecomp — SysMemUserForUser.
 *
 * The PSP's user-memory allocator. A game asks for a block by size and
 * placement policy, gets back a UID, and turns that UID into an address with
 * sceKernelGetBlockHeadAddr. Nothing runs before this works: it is the first
 * thing almost every module does after start-up.
 *
 * Placement matters and is not a detail. A game that asks for PSP_SMEM_High
 * expects its block at the *top* of user memory, and engines rely on that to
 * keep long-lived allocations away from a low-end scratch heap. Treating every
 * request as "any free block" appears to work and then fragments in a way that
 * only shows up hours in.
 */

#include "psprecomp/hle.h"

#include <stdio.h>
#include <string.h>

/* Placement policies, as passed in the `type` argument. */
#define PSP_SMEM_Low          0
#define PSP_SMEM_High         1
#define PSP_SMEM_Addr         2
#define PSP_SMEM_LowAligned   3
#define PSP_SMEM_HighAligned  4

#define MAX_BLOCKS 512
#define UID_BASE   0x00010000u

typedef struct {
    uint32_t uid;
    uint32_t addr;
    uint32_t size;
    char     name[32];
    int      used;
} mem_block;

static mem_block g_block[MAX_BLOCKS];
static uint32_t  g_next_uid;
static uint32_t  g_heap_lo, g_heap_hi;

/* Default user heap. Modules load at 0x08800000 and are a few megabytes, so
 * starting above them keeps the allocator from handing out memory the module
 * itself occupies. A host that knows its real layout should narrow this. */
#define DEFAULT_HEAP_LO 0x08C00000u
#define DEFAULT_HEAP_HI (PSP_RAM_BASE + PSP_RAM_SIZE)

void psp_sysmem_reset(void) {
    memset(g_block, 0, sizeof g_block);
    g_next_uid = UID_BASE;
    g_heap_lo = DEFAULT_HEAP_LO;
    g_heap_hi = DEFAULT_HEAP_HI;
}

void psp_sysmem_init(void) { psp_sysmem_reset(); }

static mem_block *find_uid(uint32_t uid) {
    for (int i = 0; i < MAX_BLOCKS; i++)
        if (g_block[i].used && g_block[i].uid == uid) return &g_block[i];
    return NULL;
}

static mem_block *alloc_slot(void) {
    for (int i = 0; i < MAX_BLOCKS; i++)
        if (!g_block[i].used) return &g_block[i];
    return NULL;
}

/* Does [addr, addr+size) overlap anything already handed out? */
static int overlaps(uint32_t addr, uint32_t size) {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (!g_block[i].used) continue;
        uint32_t b = g_block[i].addr, e = b + g_block[i].size;
        if (addr < e && b < addr + size) return 1;
    }
    return 0;
}

/* First fit walking up from the bottom of the heap. Blocks are few (a few
 * hundred at most) so a linear probe per candidate is fine and keeps the
 * bookkeeping to one array. */
static uint32_t place_low(uint32_t size, uint32_t align) {
    uint32_t a = (g_heap_lo + align - 1) & ~(align - 1);
    while (a + size <= g_heap_hi) {
        if (!overlaps(a, size)) return a;
        /* Skip past whatever is in the way rather than stepping by `align`,
         * which on a full heap would be O(heap/align) probes per allocation. */
        uint32_t next = g_heap_hi;
        for (int i = 0; i < MAX_BLOCKS; i++) {
            if (!g_block[i].used) continue;
            uint32_t b = g_block[i].addr, e = b + g_block[i].size;
            if (a < e && b < a + size && e < next) next = e;
        }
        a = (next + align - 1) & ~(align - 1);
    }
    return 0;
}

/* First fit walking down from the top. */
static uint32_t place_high(uint32_t size, uint32_t align) {
    if (size > g_heap_hi - g_heap_lo) return 0;
    uint32_t a = (g_heap_hi - size) & ~(align - 1);
    while (a >= g_heap_lo) {
        if (!overlaps(a, size)) return a;
        uint32_t prev = g_heap_lo;
        for (int i = 0; i < MAX_BLOCKS; i++) {
            if (!g_block[i].used) continue;
            uint32_t b = g_block[i].addr, e = b + g_block[i].size;
            if (a < e && b < a + size && b > prev) prev = b;
        }
        if (prev <= g_heap_lo || prev < size) return 0;
        a = (prev - size) & ~(align - 1);
    }
    return 0;
}

uint32_t psp_sysmem_free(void) {
    uint32_t used = 0;
    for (int i = 0; i < MAX_BLOCKS; i++)
        if (g_block[i].used) used += g_block[i].size;
    return (g_heap_hi - g_heap_lo) - used;
}

uint32_t psp_sysmem_alloc(uint32_t size, int from_high) {
    uint32_t rounded = (size + 0xFF) & ~0xFFu;
    if (!rounded) rounded = 0x100;

    uint32_t addr = from_high ? place_high(rounded, 0x100) : place_low(rounded, 0x100);
    if (!addr) return 0;

    mem_block *b = alloc_slot();
    if (!b) return 0;
    b->uid = g_next_uid++;
    b->addr = addr;
    b->size = rounded;
    fprintf(stderr, "AllocPartition -> uid 0x%X at 0x%08X size %u (ends 0x%08X)\n",
            b->uid, b->addr, b->size, b->addr + b->size);
    b->used = 1;
    snprintf(b->name, sizeof b->name, "internal");
    return addr;
}

void psp_sysmem_release(uint32_t addr) {
    for (int i = 0; i < MAX_BLOCKS; i++)
        if (g_block[i].used && g_block[i].addr == addr) { g_block[i].used = 0; return; }
}

/* ---- the calls ----------------------------------------------------------- */

static void hle_AllocPartitionMemory(void) {

    /* (partitionid, name, type, size, addr) */
    uint32_t name_ptr = psp_arg(1);
    uint32_t type     = psp_arg(2);
    uint32_t size     = psp_arg(3);
    uint32_t want     = psp_arg(4);

    /* The hardware allocator works in 256-byte granules. Rounding up matters:
     * a game that allocates 100 bytes and then writes 256 is relying on it. */
    uint32_t rounded = (size + 0xFF) & ~0xFFu;
    if (rounded == 0) rounded = 0x100;

    uint32_t align = 0x100;
    if (type == PSP_SMEM_LowAligned || type == PSP_SMEM_HighAligned) {
        align = want ? want : 0x100;
        if (align & (align - 1)) { psp_ret(SCE_KERNEL_ERROR_ILLEGAL_ATTR); return; }
        if (align < 0x100) align = 0x100;
    }

    uint32_t addr = 0;
    switch (type) {
    case PSP_SMEM_Low:
    case PSP_SMEM_LowAligned:  addr = place_low(rounded, align);  break;
    case PSP_SMEM_High:
    case PSP_SMEM_HighAligned: addr = place_high(rounded, align); break;
    case PSP_SMEM_Addr:
        if (want >= g_heap_lo && want + rounded <= g_heap_hi && !overlaps(want, rounded))
            addr = want;
        break;
    default:
        psp_ret(SCE_KERNEL_ERROR_ILLEGAL_ATTR);
        return;
    }

    if (!addr) { psp_ret(SCE_KERNEL_ERROR_NO_MEMORY); return; }

    mem_block *b = alloc_slot();
    if (!b) { psp_ret(SCE_KERNEL_ERROR_NO_MEMORY); return; }

    b->uid = g_next_uid++;
    b->addr = addr;
    b->size = rounded;
    fprintf(stderr, "AllocPartition -> uid 0x%X at 0x%08X size %u (ends 0x%08X)\n",
            b->uid, b->addr, b->size, b->addr + b->size);
    b->used = 1;
    psp_str(name_ptr, b->name, sizeof b->name);

    psp_ret(b->uid);
}

static void hle_FreePartitionMemory(void) {
    mem_block *b = find_uid(psp_arg(0));
    if (!b) { psp_ret(SCE_KERNEL_ERROR_UNKNOWN_UID); return; }
    b->used = 0;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_GetBlockHeadAddr(void) {
    mem_block *b = find_uid(psp_arg(0));
    fprintf(stderr, "GetBlockHeadAddr(uid=0x%X) -> 0x%08X (size %u, ends 0x%08X)\n",
            psp_arg(0), b ? b->addr : 0, b ? b->size : 0,
            b ? b->addr + b->size : 0);
    psp_ret(b ? b->addr : 0);
}

/* Version-reporting calls. These are advisory: the firmware records the value
 * and games do not read it back, so accepting and ignoring is correct rather
 * than merely convenient. */
static void hle_SetCompiledSdkVersion(void) { psp_ret(SCE_KERNEL_ERROR_OK); }
static void hle_SetCompilerVersion(void)    { psp_ret(SCE_KERNEL_ERROR_OK); }

/* sceKernelPrintf is a game's own debug output, which makes it one of the most
 * valuable things to have working during bring-up -- it is the game telling you
 * what it thinks it is doing. Formatting is done here rather than passed to the
 * host printf because the arguments live in guest registers and, for %s, guest
 * memory. */
static void hle_Printf(void) {
    char fmt[256];
    psp_str(psp_arg(0), fmt, sizeof fmt);

    int argi = 1;
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') { fputc(*p, stderr); continue; }
        p++;
        if (!*p) break;
        /* Skip flags/width/precision; we do not reproduce padding. */
        while (*p && strchr("-+ #0123456789.lhz", *p)) p++;
        switch (*p) {
        case 'd': case 'i': fprintf(stderr, "%d", (int32_t)psp_arg(argi++)); break;
        case 'u':           fprintf(stderr, "%u", psp_arg(argi++)); break;
        case 'x':           fprintf(stderr, "%x", psp_arg(argi++)); break;
        case 'X':           fprintf(stderr, "%X", psp_arg(argi++)); break;
        case 'p':           fprintf(stderr, "0x%08X", psp_arg(argi++)); break;
        case 'c':           fputc((int)psp_arg(argi++), stderr); break;
        case 's': {
            char s[256];
            psp_str(psp_arg(argi++), s, sizeof s);
            fputs(s, stderr);
            break;
        }
        case '%': fputc('%', stderr); break;
        default:  fputc('%', stderr); fputc(*p, stderr); break;
        }
    }
    psp_ret(SCE_KERNEL_ERROR_OK);
}

void psp_sysmem_register(void) {
    /* NIDs are SHA-1(name)[0:4] little-endian; tests/test_hle.c verifies every
     * pair below, so a mistyped NID cannot survive. */
    psp_hle_register(0x237DBD4F, "SysMemUserForUser", "sceKernelAllocPartitionMemory", hle_AllocPartitionMemory);
    psp_hle_register(0xB6D61D02, "SysMemUserForUser", "sceKernelFreePartitionMemory",  hle_FreePartitionMemory);
    psp_hle_register(0x9D9A5BA1, "SysMemUserForUser", "sceKernelGetBlockHeadAddr",     hle_GetBlockHeadAddr);
    psp_hle_register(0x7591C7DB, "SysMemUserForUser", "sceKernelSetCompiledSdkVersion",hle_SetCompiledSdkVersion);
    psp_hle_register(0xF77D77CB, "SysMemUserForUser", "sceKernelSetCompilerVersion",   hle_SetCompilerVersion);
    psp_hle_register(0x13A5ABEF, "SysMemUserForUser", "sceKernelPrintf",               hle_Printf);
}
