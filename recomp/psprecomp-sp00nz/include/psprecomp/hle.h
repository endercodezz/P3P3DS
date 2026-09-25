/* psprecomp — high-level emulation of the PSP firmware.
 *
 * A PSP game does not touch hardware. It calls firmware entry points through
 * an import table, and every one of those calls is identified by a **NID** —
 * the first four bytes of SHA-1(function name), little-endian. That is a
 * verifiable fact rather than a convention, and `tests/test_hle.c` checks it
 * for every function registered here: a mistyped NID or a wrong name cannot
 * survive the test.
 *
 * Because the surface is a library rather than hardware, it is *implemented*,
 * not emulated. What a game needs is exactly its import table and nothing
 * else, which `allegrexrecomp funcs` reports — so the work is bounded and
 * knowable in advance.
 *
 * Calling convention is MIPS o32: arguments in $a0-$a3 then the stack, return
 * value in $v0. Handlers take no C arguments and use psp_arg()/psp_ret().
 */
#ifndef PSPRECOMP_HLE_H
#define PSPRECOMP_HLE_H

#include "cpu.h"
#include "mem.h"

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*psp_hle_fn)(void);

/* Register one firmware function. `name` is kept for diagnostics and is what
 * the NID is verified against. */
void psp_hle_register(uint32_t nid, const char *lib, const char *name, psp_hle_fn fn);
void psp_hle_register_unnamed(uint32_t nid, const char *lib, psp_hle_fn fn);
int psp_hle_is_named(int index);

/* Call a firmware function by NID. An unregistered NID reports itself by name
 * where possible and by number otherwise, rather than failing silently. */
void psp_hle_call(uint32_t nid);

/* Look up what is registered, for reporting. Returns NULL if absent. */
const char *psp_hle_name(uint32_t nid);
int         psp_hle_count(void);

/* Every registered entry, for the coverage report a game repo wants: which of
 * a module's imports actually exist yet. */
typedef struct {
    uint32_t    nid;
    const char *lib;
    const char *name;
} psp_hle_entry;

const psp_hle_entry *psp_hle_entries(int *count);

/* Print the recent firmware calls that returned zero. Zero is what a game most
 * often mistakes for an address, so this is the first thing to consult when a
 * wild pointer shows up far from its cause. */
void psp_hle_dump_recent(FILE *out);

/* Register everything the toolkit implements. Call once at startup. */
void psp_hle_init(void);

/* ---- o32 argument access ------------------------------------------------- */

/* Arguments 0-3 arrive in $a0-$a3; 4 and beyond are on the stack, at $sp+16
 * onward. The stack slots for the register arguments exist but are not
 * written by the caller, which is why the split is at 4 and not at 0. */
/* Firmware calls take arguments 5-8 in $t0-$t3, not on the stack.
 *
 * Plain o32 spills the fifth argument onward to `sp+16`, and that is what this
 * used to read. PSP firmware stubs do not: they load $t0-$t3 and branch, which
 * is visible in the delay slot of every such call --
 *
 *     000002EC  addu  $a3, $s1, $zero
 *     000002F0  jal   0x00091F14        ; sceKernelAllocPartitionMemory
 *     000002F4  addiu $t0, $zero, 4096  ; argument 5: the alignment
 *
 * Reading `sp+16` instead returned whatever happened to be on the stack. For
 * that call it produced 0x3AC85C where a power-of-two alignment belonged, so
 * the allocator rejected a 15.9 MB request with ILLEGAL_ATTR -- and every
 * failure this bring-up chased descended from it.
 *
 * Beyond eight, arguments do go on the stack. */
static inline uint32_t psp_arg(int n) {
    if (n < 4) return psp_cpu.r[PSP_REG_A0 + n];
    if (n < 8) return psp_cpu.r[PSP_REG_T0 + (n - 4)];
    return psp_read32(psp_cpu.r[PSP_REG_SP] + (uint32_t)n * 4);
}

static inline void psp_ret(uint32_t v) { psp_cpu.r[PSP_REG_V0] = v; }

/* Read a NUL-terminated string out of guest memory into a host buffer.
 * Always terminates; returns `dst`. */
const char *psp_str(uint32_t addr, char *dst, size_t cap);

/* ---- error codes --------------------------------------------------------- */
/* Only the ones the implemented functions can actually return. Games branch on
 * these, so returning a plausible-looking wrong value is worse than failing. */
#define SCE_KERNEL_ERROR_OK              0
#define SCE_KERNEL_ERROR_ERROR           0x80020001
#define SCE_KERNEL_ERROR_NOTIMPLEMENTED  0x80020002
#define SCE_KERNEL_ERROR_ILLEGAL_ADDR    0x80020005
#define SCE_KERNEL_ERROR_NO_MEMORY       0x80020190
#define SCE_KERNEL_ERROR_ILLEGAL_ATTR    0x80020191
#define SCE_KERNEL_ERROR_UNKNOWN_UID     0x800201A2
#define SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCK 0x800201A9
#define SCE_KERNEL_ERROR_ILLEGAL_THID    0x80020197
#define SCE_KERNEL_ERROR_WAIT_TIMEOUT    0x800201A8

/* ---- the subsystems ------------------------------------------------------ */

void psp_sysmem_init(void);
void psp_sysmem_register(void);
void psp_sysmem_reset(void);

void psp_display_init(void);
void psp_display_register(void);
void psp_display_reset(void);
int      psp_display_capture(const char *path);
uint64_t psp_display_vblanks(void);
uint32_t psp_display_framebuffer(void);

void psp_ge_init(void);
void psp_ge_register(void);
void psp_ge_reset(void);
void psp_ge_dump_stats(FILE *out);
uint64_t psp_ge_command_count(void);
uint64_t psp_ge_vertex_count(void);
uint64_t psp_ge_pixels(void);

void psp_sas_init(void);
void psp_sas_register(void);
void psp_sas_reset(void);
uint64_t psp_sas_frames(void);
uint64_t psp_sas_nonzero(void);

void psp_io_init(void);
void psp_io_register(void);
void psp_io_reset(void);
void psp_io_set_root(const char *root);
uint64_t psp_io_bytes_read(void);

void psp_misc_init(void);
void psp_misc_register(void);
void psp_misc_reset(void);
int  psp_exit_requested(void);
void psp_ctrl_set(uint32_t buttons, uint8_t ax, uint8_t ay);
uint64_t psp_audio_blocks(void);

void psp_threadman_init(void);
void psp_threadman_register(void);
void psp_threadman_reset(void);

/* Bytes of user memory still available — the cheapest end-to-end check that
 * the allocator is behaving. */
uint32_t psp_sysmem_free(void);

/* Raw allocation for use by other HLE subsystems (thread stacks, mostly).
 * Returns 0 on failure. These bypass the UID table because nothing in the
 * guest ever refers to them. */
uint32_t psp_sysmem_alloc(uint32_t size, int from_high);
void     psp_sysmem_release(uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif /* PSPRECOMP_HLE_H */
