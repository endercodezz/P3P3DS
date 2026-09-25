/* psprecomp — PSP memory map.
 *
 * The Allegrex has no TLB. Addresses are decoded by their top bits into a
 * handful of fixed regions, which makes the recompiled memory path cheap:
 * mask off the segment bits, index a flat host allocation.
 *
 *   0x00010000-0x00013FFF   scratchpad  (16 KB, fast on-chip)
 *   0x04000000-0x041FFFFF   VRAM        (2 MB)
 *   0x08000000-0x09FFFFFF   main RAM    (32 MB on PSP-2000+, 24 MB usable
 *                                        on PSP-1000 where it ends at 0x09FFFFFF
 *                                        but only 0x087FFFFF is user memory)
 *
 * Each region is mirrored at three segment bases, which differ only in cache
 * behaviour — irrelevant to us, so we fold them together:
 *   0x0.......  kernel, cached
 *   0x4.......  uncached mirror
 *   0x8.......  kernel-only, cached
 * We mask with PSP_ADDR_MASK to collapse all three onto one backing store.
 */
#ifndef PSPRECOMP_MEM_H
#define PSPRECOMP_MEM_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_ADDR_MASK       0x3FFFFFFFu  /* collapse the cache-mirror segments */

#define PSP_SCRATCH_BASE    0x00010000u
#define PSP_SCRATCH_SIZE    0x00004000u  /* 16 KB */
#define PSP_VRAM_BASE       0x04000000u
#define PSP_VRAM_SIZE       0x00200000u  /* 2 MB */
#define PSP_RAM_BASE        0x08000000u
#define PSP_RAM_SIZE        0x02000000u  /* 32 MB */

typedef struct {
    uint8_t *ram;        /* PSP_RAM_SIZE bytes */
    uint8_t *vram;       /* PSP_VRAM_SIZE bytes */
    uint8_t *scratch;    /* PSP_SCRATCH_SIZE bytes */
} psp_memory;

extern psp_memory psp_mem;

int  psp_mem_init(void);
void psp_mem_free(void);

/* Map the loaded module's image.
 *
 * A relocatable PRX links at address 0, and the recompiled C has those
 * addresses baked in as literals — `lui $v1, 0x9` becomes `0x00090000`. So the
 * module's own code and data live *outside* the console's RAM window
 * (0x08000000+) and need their own mapping at wherever the module was linked.
 * A statically linked module already sits inside RAM and needs none of this.
 *
 * Call once, before loading segments. Returns 0 on success. */
int psp_mem_map_module(uint32_t base, uint32_t size);

/* Resolve a guest address to a host pointer, or NULL if unmapped.
 * `size` is the access width; a read straddling the end of a region is
 * rejected rather than silently truncated. */
void *psp_mem_ptr(uint32_t addr, uint32_t size);

/* The PSP is little-endian and so is every host we target, so these are plain
 * loads once the address is resolved. Unmapped accesses return 0 / are dropped
 * and bump psp_mem_bad_access — a recompiled game that starts faulting here is
 * telling you the analysis missed something, so it is counted, not ignored. */
extern uint64_t psp_mem_bad_access;
void psp_mem_watch_write(uint32_t addr);
int psp_mem_watch_hits(void);

uint8_t  psp_read8 (uint32_t addr);
uint16_t psp_read16(uint32_t addr);
uint32_t psp_read32(uint32_t addr);
float    psp_read_f32(uint32_t addr);

void psp_write8 (uint32_t addr, uint8_t  val);
void psp_write16(uint32_t addr, uint16_t val);
void psp_write32(uint32_t addr, uint32_t val);
void psp_write_f32(uint32_t addr, float val);

/* Bulk copy into guest memory — used by the loader to place PRX segments. */
int psp_mem_write_block(uint32_t addr, const void *src, uint32_t len);
int psp_mem_read_block(void *dst, uint32_t addr, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* PSPRECOMP_MEM_H */
