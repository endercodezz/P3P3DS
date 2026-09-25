/* psprecomp — semantic helpers for recompiled code.
 *
 * Most MIPS instructions lower to one line of obvious C, and the emitter
 * writes those inline so the generated file stays readable. The ones that do
 * NOT — trapping arithmetic, division edge cases, the unaligned load/store
 * pairs, the bitfield ops — live here, in exactly one place. If a game
 * diverges from the oracle on one of these, there is one function to fix, not
 * one occurrence per call site.
 *
 * Everything here is a static inline in the header on purpose: the generated
 * C is enormous, and these want to vanish into the surrounding code.
 */
#ifndef PSPRECOMP_RECOMP_RT_H
#define PSPRECOMP_RECOMP_RT_H

#include "cpu.h"
#include "mem.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- arithmetic ---------------------------------------------------------- */

/* ADD/SUB/ADDI trap on signed overflow on real hardware. Games essentially
 * never rely on the trap — compilers emit addu/subu for C arithmetic — but the
 * wraparound result must still be exact, so we compute in unsigned. */
static inline uint32_t psp_add(uint32_t a, uint32_t b) { return a + b; }
static inline uint32_t psp_sub(uint32_t a, uint32_t b) { return a - b; }

static inline uint32_t psp_slt (uint32_t a, uint32_t b) { return (int32_t)a < (int32_t)b; }
static inline uint32_t psp_sltu(uint32_t a, uint32_t b) { return a < b; }
static inline uint32_t psp_max (uint32_t a, uint32_t b) { return (int32_t)a > (int32_t)b ? a : b; }
static inline uint32_t psp_min (uint32_t a, uint32_t b) { return (int32_t)a < (int32_t)b ? a : b; }

/* Shifts: C leaves shift-by->=32 undefined, MIPS masks the amount to 5 bits. */
static inline uint32_t psp_sll(uint32_t v, uint32_t s) { return v << (s & 31); }
static inline uint32_t psp_srl(uint32_t v, uint32_t s) { return v >> (s & 31); }
static inline uint32_t psp_sra(uint32_t v, uint32_t s) { return (uint32_t)((int32_t)v >> (s & 31)); }
static inline uint32_t psp_rotr(uint32_t v, uint32_t s) {
    s &= 31;
    return s ? ((v >> s) | (v << (32 - s))) : v;
}

/* ---- multiply / divide --------------------------------------------------- */

static inline void psp_mult(uint32_t a, uint32_t b) {
    int64_t r = (int64_t)(int32_t)a * (int64_t)(int32_t)b;
    psp_cpu.lo = (uint32_t)r;
    psp_cpu.hi = (uint32_t)((uint64_t)r >> 32);
}

static inline void psp_multu(uint32_t a, uint32_t b) {
    uint64_t r = (uint64_t)a * (uint64_t)b;
    psp_cpu.lo = (uint32_t)r;
    psp_cpu.hi = (uint32_t)(r >> 32);
}

/* Division by zero does not trap on MIPS — HI/LO are simply unpredictable.
 * We define them (quotient -1/0, remainder = dividend) so a recompiled game
 * that hits this is deterministic and diffable against the oracle instead of
 * depending on host UB. INT_MIN / -1 also overflows in C, so it is special-cased. */
static inline void psp_div(uint32_t a, uint32_t b) {
    int32_t sa = (int32_t)a, sb = (int32_t)b;
    if (sb == 0) {
        psp_cpu.lo = sa < 0 ? 1u : 0xFFFFFFFFu;
        psp_cpu.hi = a;
    } else if (sa == (int32_t)0x80000000 && sb == -1) {
        psp_cpu.lo = 0x80000000u;
        psp_cpu.hi = 0;
    } else {
        psp_cpu.lo = (uint32_t)(sa / sb);
        psp_cpu.hi = (uint32_t)(sa % sb);
    }
}

static inline void psp_divu(uint32_t a, uint32_t b) {
    if (b == 0) {
        psp_cpu.lo = 0xFFFFFFFFu;
        psp_cpu.hi = a;
    } else {
        psp_cpu.lo = a / b;
        psp_cpu.hi = a % b;
    }
}

static inline void psp_madd(uint32_t a, uint32_t b) {
    int64_t acc = (int64_t)(((uint64_t)psp_cpu.hi << 32) | psp_cpu.lo);
    acc += (int64_t)(int32_t)a * (int64_t)(int32_t)b;
    psp_cpu.lo = (uint32_t)acc;
    psp_cpu.hi = (uint32_t)((uint64_t)acc >> 32);
}

static inline void psp_maddu(uint32_t a, uint32_t b) {
    uint64_t acc = ((uint64_t)psp_cpu.hi << 32) | psp_cpu.lo;
    acc += (uint64_t)a * (uint64_t)b;
    psp_cpu.lo = (uint32_t)acc;
    psp_cpu.hi = (uint32_t)(acc >> 32);
}

static inline void psp_msub(uint32_t a, uint32_t b) {
    int64_t acc = (int64_t)(((uint64_t)psp_cpu.hi << 32) | psp_cpu.lo);
    acc -= (int64_t)(int32_t)a * (int64_t)(int32_t)b;
    psp_cpu.lo = (uint32_t)acc;
    psp_cpu.hi = (uint32_t)((uint64_t)acc >> 32);
}

static inline void psp_msubu(uint32_t a, uint32_t b) {
    uint64_t acc = ((uint64_t)psp_cpu.hi << 32) | psp_cpu.lo;
    acc -= (uint64_t)a * (uint64_t)b;
    psp_cpu.lo = (uint32_t)acc;
    psp_cpu.hi = (uint32_t)(acc >> 32);
}

/* ---- bit manipulation (MIPS32r2 + Allegrex) ------------------------------ */

static inline uint32_t psp_clz(uint32_t v) {
    if (v == 0) return 32;
    uint32_t n = 0;
    while (!(v & 0x80000000u)) { v <<= 1; n++; }
    return n;
}

static inline uint32_t psp_clo(uint32_t v) { return psp_clz(~v); }

/* ext rt, rs, pos, size — extract `size` bits starting at `pos`. */
static inline uint32_t psp_ext(uint32_t v, uint32_t pos, uint32_t size) {
    if (size == 0 || size > 32) return 0;
    if (size == 32) return v >> pos;
    return (v >> pos) & ((1u << size) - 1);
}

/* ins rt, rs, pos, size — splice `size` bits of `src` into `dst` at `pos`. */
static inline uint32_t psp_ins(uint32_t dst, uint32_t src, uint32_t pos, uint32_t size) {
    if (size == 0 || size > 32) return dst;
    uint32_t mask = (size == 32) ? 0xFFFFFFFFu : (((1u << size) - 1) << pos);
    return (dst & ~mask) | ((src << pos) & mask);
}

static inline uint32_t psp_seb(uint32_t v) { return (uint32_t)(int32_t)(int8_t)(v & 0xFF); }
static inline uint32_t psp_seh(uint32_t v) { return (uint32_t)(int32_t)(int16_t)(v & 0xFFFF); }

/* wsbh: swap bytes within each halfword. wsbw: reverse all four bytes. */
static inline uint32_t psp_wsbh(uint32_t v) {
    return ((v & 0x00FF00FFu) << 8) | ((v & 0xFF00FF00u) >> 8);
}
static inline uint32_t psp_wsbw(uint32_t v) {
    return (v << 24) | ((v & 0x0000FF00u) << 8) |
           ((v & 0x00FF0000u) >> 8) | (v >> 24);
}

/* bitrev: reverse the bit order of the whole word (Allegrex extension). */
static inline uint32_t psp_bitrev(uint32_t v) {
    v = ((v & 0x55555555u) << 1)  | ((v >> 1)  & 0x55555555u);
    v = ((v & 0x33333333u) << 2)  | ((v >> 2)  & 0x33333333u);
    v = ((v & 0x0F0F0F0Fu) << 4)  | ((v >> 4)  & 0x0F0F0F0Fu);
    v = ((v & 0x00FF00FFu) << 8)  | ((v >> 8)  & 0x00FF00FFu);
    return (v << 16) | (v >> 16);
}

/* ---- unaligned load / store ---------------------------------------------- */
/* lwl/lwr and swl/swr are how MIPS compilers implement unaligned 32-bit
 * access: a pair of instructions that each move the part of the word that
 * lies in one aligned container. The PSP is little-endian, which is the case
 * people get wrong — these follow the LE definitions. */

/* The four byte-offset cases are a lookup, not arithmetic — this is the form
 * the MIPS manual defines them in, and it avoids the shift-by-32 UB that the
 * arithmetic form walks into at offset 0. */
static const uint32_t PSP_LWL_MASK [4] = { 0x00FFFFFFu, 0x0000FFFFu, 0x000000FFu, 0x00000000u };
static const uint32_t PSP_LWL_SHIFT[4] = { 24, 16, 8, 0 };
static const uint32_t PSP_LWR_MASK [4] = { 0x00000000u, 0xFF000000u, 0xFFFF0000u, 0xFFFFFF00u };
static const uint32_t PSP_LWR_SHIFT[4] = { 0, 8, 16, 24 };
static const uint32_t PSP_SWL_MASK [4] = { 0xFFFFFF00u, 0xFFFF0000u, 0xFF000000u, 0x00000000u };
static const uint32_t PSP_SWL_SHIFT[4] = { 24, 16, 8, 0 };
static const uint32_t PSP_SWR_MASK [4] = { 0x00000000u, 0x000000FFu, 0x0000FFFFu, 0x00FFFFFFu };
static const uint32_t PSP_SWR_SHIFT[4] = { 0, 8, 16, 24 };

static inline uint32_t psp_lwl(uint32_t old, uint32_t addr) {
    uint32_t b = addr & 3;
    uint32_t w = psp_read32(addr & ~3u);
    return (old & PSP_LWL_MASK[b]) | (w << PSP_LWL_SHIFT[b]);
}

static inline uint32_t psp_lwr(uint32_t old, uint32_t addr) {
    uint32_t b = addr & 3;
    uint32_t w = psp_read32(addr & ~3u);
    return (old & PSP_LWR_MASK[b]) | (w >> PSP_LWR_SHIFT[b]);
}

static inline void psp_swl(uint32_t val, uint32_t addr) {
    uint32_t b = addr & 3, base = addr & ~3u;
    psp_write32(base, (psp_read32(base) & PSP_SWL_MASK[b]) | (val >> PSP_SWL_SHIFT[b]));
}

static inline void psp_swr(uint32_t val, uint32_t addr) {
    uint32_t b = addr & 3, base = addr & ~3u;
    psp_write32(base, (psp_read32(base) & PSP_SWR_MASK[b]) | (val << PSP_SWR_SHIFT[b]));
}

/* ---- COP1, single precision --------------------------------------------- */

/* `mtc1`/`mfc1` move raw bits between the integer and FP register files — they
 * are not conversions. Going through a union keeps that explicit and avoids
 * the strict-aliasing violation a pointer cast would introduce. */
static inline float psp_bits_to_f32(uint32_t b) {
    union { uint32_t u; float f; } c;
    c.u = b;
    return c.f;
}

static inline uint32_t psp_f32_to_bits(float v) {
    union { uint32_t u; float f; } c;
    c.f = v;
    return c.u;
}

static inline float psp_fabs(float v)  { return v < 0.0f ? -v : v; }

/* Newton-Raphson would be faster but the host FPU is exact and this is not the
 * hot path in any recompiled game; correctness first. */
static inline float psp_fsqrt(float v) {
    if (v <= 0.0f) return 0.0f;
    float g = v;
    for (int i = 0; i < 24; i++) g = 0.5f * (g + v / g);
    return g;
}

/* `c.<cond>.s` condition codes. The distinction that matters is *ordered* vs
 * *unordered*: with a NaN operand the ordered forms are false and the
 * unordered forms are true. Comparisons involving NaN are false in C, so the
 * NaN case is tested explicitly rather than assumed. */
static inline int psp_fcmp(unsigned cond, float a, float b) {
    const int unordered = (a != a) || (b != b);   /* either is NaN */
    switch (cond & 0xF) {
    case 0x0: case 0x8: return 0;                              /* F, SF */
    case 0x1: case 0x9: return unordered;                      /* UN, NGLE */
    case 0x2: case 0xA: return !unordered && a == b;           /* EQ, SEQ */
    case 0x3: case 0xB: return unordered || a == b;            /* UEQ, NGL */
    case 0x4: case 0xC: return !unordered && a <  b;           /* OLT, LT */
    case 0x5: case 0xD: return unordered || a <  b;            /* ULT, NGE */
    case 0x6: case 0xE: return !unordered && a <= b;           /* OLE, LE */
    default:            return unordered || a <= b;            /* ULE, NGT */
    }
}

#ifdef __cplusplus
}
#endif

#endif /* PSPRECOMP_RECOMP_RT_H */
