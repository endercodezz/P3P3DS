/* psprecomp — PSP memory map. See include/psprecomp/mem.h. */

#include "psprecomp/mem.h"
#include "psprecomp/dispatch.h"
#include "psprecomp/cpu.h"

#include <stdio.h>
#include <stdlib.h>

psp_memory psp_mem;
uint64_t   psp_mem_bad_access;

/* Watch writes to one address. "Which code writes this word" is a question
 * that came up repeatedly and could only be answered by guessing; the write
 * path is the one place that can answer it directly. */
static uint32_t g_wwatch;
static int g_whits;
void psp_mem_watch_write(uint32_t addr) { g_wwatch = addr; g_whits = 0; }
int psp_mem_watch_hits(void) { return g_whits; }

static void note_write(uint32_t addr, uint32_t width) {
    if (!g_wwatch || addr + width <= g_wwatch || addr > g_wwatch) return;
    if (g_whits++ < 8)
        fprintf(stderr, "write%u to 0x%08X (watch 0x%08X) from fn 0x%08X\n",
                width * 8, addr, g_wwatch, psp_trace_last());
}

/* Bad accesses were only ever counted, which says an initialiser went wrong
 * without saying which one. The addresses are what identify it, and there are
 * few enough of them (28 in the current run) to simply print. */
static void bad_access(uint32_t addr, int write, int width) {
    /* The first one matters most: everything after it may be cascade from a
     * register that was already wrong. Dump the whole file once. */
    if (psp_mem_bad_access == 0) {
        static const char *N[32] = {
            "zero","at","v0","v1","a0","a1","a2","a3","t0","t1","t2","t3",
            "t4","t5","t6","t7","s0","s1","s2","s3","s4","s5","s6","s7",
            "t8","t9","k0","k1","gp","sp","fp","ra" };
        fprintf(stderr, "\n--- first bad access: %s%d at 0x%08X (last fn 0x%08X) ---\n",
                write ? "write" : "read", width * 8, addr, psp_trace_last());
        for (int i = 0; i < 32; i += 4)
            fprintf(stderr, "  %-3s=0x%08X  %-3s=0x%08X  %-3s=0x%08X  %-3s=0x%08X\n",
                    N[i], psp_cpu.r[i], N[i+1], psp_cpu.r[i+1],
                    N[i+2], psp_cpu.r[i+2], N[i+3], psp_cpu.r[i+3]);
    }
    if (psp_mem_bad_access < 32)
        fprintf(stderr, "psprecomp: bad %s%d at 0x%08X (last fn 0x%08X)\n",
                write ? "write" : "read", width * 8, addr, psp_trace_last());
    psp_mem_bad_access++;
}

int psp_mem_init(void) {
    psp_mem.ram     = (uint8_t *)calloc(1, PSP_RAM_SIZE);
    psp_mem.vram    = (uint8_t *)calloc(1, PSP_VRAM_SIZE);
    psp_mem.scratch = (uint8_t *)calloc(1, PSP_SCRATCH_SIZE);
    if (!psp_mem.ram || !psp_mem.vram || !psp_mem.scratch) {
        psp_mem_free();
        return -1;
    }
    psp_mem_bad_access = 0;
    return 0;
}

void psp_mem_free(void) {
    free(psp_mem.ram);
    free(psp_mem.vram);
    free(psp_mem.scratch);
    psp_mem.ram = psp_mem.vram = psp_mem.scratch = NULL;
}

/* The loaded module image, for a PRX linked outside the RAM window. */
static uint8_t *g_module;
static uint32_t g_module_base, g_module_size;

int psp_mem_map_module(uint32_t base, uint32_t size) {
    free(g_module);
    g_module = (uint8_t *)calloc(1, size ? size : 1);
    if (!g_module) { g_module_size = 0; return -1; }
    g_module_base = base;
    g_module_size = size;
    return 0;
}

void *psp_mem_ptr(uint32_t addr, uint32_t size) {
    /* Collapse the three cache-behaviour mirrors onto one backing store. */
    const uint32_t a = addr & PSP_ADDR_MASK;

    /* Checked first: a module linked at 0 would otherwise fall through every
     * region test and read as unmapped. */
    if (g_module_size && a >= g_module_base && a < g_module_base + g_module_size) {
        uint32_t off = a - g_module_base;
        if (off + size > g_module_size) return NULL;
        return g_module + off;
    }

    if (a >= PSP_RAM_BASE && a < PSP_RAM_BASE + PSP_RAM_SIZE) {
        uint32_t off = a - PSP_RAM_BASE;
        if (off + size > PSP_RAM_SIZE) return NULL;   /* straddles the end */
        return psp_mem.ram + off;
    }
    if (a >= PSP_VRAM_BASE && a < PSP_VRAM_BASE + PSP_VRAM_SIZE) {
        uint32_t off = a - PSP_VRAM_BASE;
        if (off + size > PSP_VRAM_SIZE) return NULL;
        return psp_mem.vram + off;
    }
    if (a >= PSP_SCRATCH_BASE && a < PSP_SCRATCH_BASE + PSP_SCRATCH_SIZE) {
        uint32_t off = a - PSP_SCRATCH_BASE;
        if (off + size > PSP_SCRATCH_SIZE) return NULL;
        return psp_mem.scratch + off;
    }
    return NULL;
}

/* Reads of unmapped memory return 0 and are counted. Silently returning 0 is
 * what hardware roughly does, but a recompiled game should never be doing it
 * in a steady state — the counter is how you notice. */
#define READ_BODY(TYPE)                              \
    void *p = psp_mem_ptr(addr, (uint32_t)sizeof(TYPE)); \
    if (!p) { bad_access(addr, 0, (int)sizeof(TYPE)); return 0; }      \
    TYPE v;                                          \
    memcpy(&v, p, sizeof v);                         \
    return v;

uint8_t  psp_read8 (uint32_t addr) { READ_BODY(uint8_t)  }
uint16_t psp_read16(uint32_t addr) { READ_BODY(uint16_t) }
uint32_t psp_read32(uint32_t addr) { READ_BODY(uint32_t) }
float    psp_read_f32(uint32_t addr) { READ_BODY(float)  }

#define WRITE_BODY(TYPE)                             \
    void *p = psp_mem_ptr(addr, (uint32_t)sizeof(TYPE)); \
    if (!p) { bad_access(addr, 1, (int)sizeof(TYPE)); return; }         \
    note_write(addr, (uint32_t)sizeof(TYPE));                    \
    memcpy(p, &val, sizeof(TYPE));

void psp_write8 (uint32_t addr, uint8_t  val) { WRITE_BODY(uint8_t)  }
void psp_write16(uint32_t addr, uint16_t val) { WRITE_BODY(uint16_t) }
void psp_write32(uint32_t addr, uint32_t val) { WRITE_BODY(uint32_t) }
void psp_write_f32(uint32_t addr, float  val) { WRITE_BODY(float)    }

int psp_mem_write_block(uint32_t addr, const void *src, uint32_t len) {
    void *p = psp_mem_ptr(addr, len);
    if (!p) return -1;
    memcpy(p, src, len);
    return 0;
}

int psp_mem_read_block(void *dst, uint32_t addr, uint32_t len) {
    void *p = psp_mem_ptr(addr, len);
    if (!p) return -1;
    memcpy(dst, p, len);
    return 0;
}
