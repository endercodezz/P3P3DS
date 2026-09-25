/* Function discovery — recursive descent over Allegrex code.
 *
 * The emitter needs to know where functions begin and end. Nothing in a
 * stripped PRX says so directly, so we recover it by following control flow
 * from a set of seeds until nothing new is reachable.
 *
 * The signals we lean on, in order of reliability:
 *
 *   1. `jal <target>` — a direct call. Its target is a function entry, full
 *      stop. This is by far the strongest signal and most of a module is
 *      discovered through it.
 *   2. `jr $ra` — a return, and therefore a function end. The decoder
 *      distinguishes this from `jr $rN` precisely so discovery can rely on it;
 *      conflating them either truncates every function at its first jump table
 *      or never closes one at all.
 *   3. The module's entry point and its exported function addresses, as seeds.
 *
 * What this deliberately does NOT do is scan linearly for function prologues.
 * That finds more, but it also finds data that happens to look like a
 * prologue, and a wrong function boundary produces C that compiles and
 * silently misbehaves. Recursive descent under-approximates, which is the
 * right direction to be wrong in: everything it reports is genuinely reachable
 * code, and what it misses shows up as unreached bytes we can go and account
 * for.
 */
#ifndef ALLEGREX_ANALYZE_H
#define ALLEGREX_ANALYZE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t addr;          /* function entry */
    /* The address range actually touched. `start` is not always `addr`: a
     * block can sit below the entry, and the emitter must iterate over
     * everything the function owns rather than assuming it all lies forward. */
    uint32_t start;
    uint32_t end;           /* one past the highest instruction reached */
    uint32_t insns;         /* instructions actually visited */
    unsigned has_return  : 1;   /* reaches a `jr $ra` */
    unsigned has_indirect: 1;   /* contains an unresolved computed jump */
    unsigned has_vfpu    : 1;   /* touches the VFPU */
} a_func;

typedef struct {
    /* Input */
    const uint8_t *code;
    uint32_t base;              /* virtual address of code[0] */
    uint32_t size;
    uint32_t stub_addr, stub_size;   /* import thunks; calls here are HLE */

    /* Also harvest call targets by linearly scanning the code for `jal`.
     *
     * Necessary on PSP, not optional. A module's `module_start` typically does
     * little more than create a thread, passing the real entry point as a
     * *pointer* to sceKernelCreateThread. That pointer is built with
     * lui/addiu, so the bulk of a game is never the target of a `jal` from
     * anything reachable — pure recursive descent from the entry point finds
     * the init stub and stops.
     *
     * `jal` carries an absolute target, so a linear scan recovers those
     * targets without needing to reach the instruction by control flow. The
     * cost is that data decoding as `jal` yields bogus seeds; over a real
     * `.text` (99.8% valid instructions) that is rare, and a bogus seed
     * produces a function that fails to reach a `jr $ra`, which the report
     * counts separately. */
    int scan_calls;

    /* The linear `jal` harvest runs over this sub-range only, normally `.text`.
     *
     * The walkable extent (`base`/`size` above) covers the whole loaded image,
     * because a module can and does place executable code outside `.text` --
     * C++ static initialisers in particular. But scanning all of `.data` for
     * `jal` patterns would manufacture thousands of false seeds out of data
     * that happens to decode. Walking widely and scanning narrowly gets both:
     * control flow may lead anywhere, while the brute-force harvest stays
     * where instructions are known to live. */
    uint32_t scan_base;
    uint32_t scan_size;

    /* The whole loaded module image, not just .text. Jump tables live in
     * .rodata or .data, so resolving a computed jump means reading outside the
     * code extent. Optional: without it, tables are simply not resolved. */
    const uint8_t *image;
    uint32_t       image_base;
    uint32_t       image_size;

    /* Output */
    a_func   *funcs;
    int       nfuncs;
    uint32_t *imports;          /* distinct import stubs called */
    int       nimports;
    uint32_t *indirects;        /* sites with an unresolved computed jump */
    int       nindirects;

    /* Jump tables recovered from the `jr $rN` sites. */
    int       ntables;          /* tables resolved */
    int       ntable_targets;   /* total entries across them */

    /* Statistics over *discovered* code only — the number that matters, as
     * opposed to coverage over a whole segment that is mostly data. */
    uint64_t insns;
    uint64_t vfpu;
    uint64_t invalid;
    uint32_t bytes_reached;
    /* Executable extent, when the container reports one. Distinct from size,
     * which spans .data and .bss too. */
    uint32_t text_size;

    /* Which function owns each word: the owning function's entry address, or
     * A_NO_OWNER. The emitter needs this because a function's instructions are
     * not necessarily contiguous — blocks get laid out apart from each other,
     * and the gaps belong either to another function or to data. Emitting the
     * whole [addr, end) range instead would pull in a neighbour's code. */
    uint32_t *owner;
    uint32_t  nwords;
} a_analysis;

#define A_NO_OWNER 0xFFFFFFFFu

/* Run discovery. `seeds` are function entry addresses to start from (the
 * module entry point and its exports). Returns 0 on success.
 * Call a_analysis_free() when done. */
int a_discover(a_analysis *an, const uint32_t *seeds, int nseeds);

/* Scan a data region for words that look like code addresses.
 *
 * This is the fallback for a **statically linked** module (ET_EXEC). A
 * relocatable PRX lists its function pointers in the relocation tables, which
 * is enumeration — the linker recorded them because they *are* addresses. A
 * static executable has those addresses baked in absolutely and its relocation
 * sections are empty, so there is nothing to enumerate and the only option is
 * to recognise them by shape.
 *
 * That makes this a **heuristic, not a fact**, and it is kept separate and
 * named accordingly so nobody later mistakes one for the other. Three filters
 * keep the false-positive rate down: the value must land inside the code
 * extent, be instruction-aligned, and point at something that actually decodes
 * as a valid instruction. A survivor that is nonetheless data shows up as a
 * function that never reaches `jr $ra`, which the report counts.
 *
 * `region` is the raw bytes to scan; `out` receives candidate addresses.
 * Returns the number found (may exceed `max`). */
int a_scan_data_pointers(const a_analysis *an,
                         const uint8_t *region, uint32_t region_len,
                         uint32_t *out, int max);

void a_analysis_free(a_analysis *an);

/* Is `addr` inside the discovered code extent? */
int a_in_range(const a_analysis *an, uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif /* ALLEGREX_ANALYZE_H */
