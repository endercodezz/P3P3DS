/* psprecomp — Allegrex CPU state.
 *
 * The PSP's main CPU is a MIPS32r2 core ("Allegrex", 222/333 MHz) with:
 *   - 32 general-purpose registers, no 64-bit integer ops
 *   - HI/LO for multiply/divide
 *   - COP1: a single-precision-only FPU (32 registers)
 *   - COP2: the VFPU, a 128-register vector unit (see vfpu.h)
 *   - no TLB — a fixed, simple memory map (see mem.h)
 *
 * Recompiled code operates on this struct directly. Generated functions are
 * plain C: they read and write psp_cpu.r[] and call the helpers in recomp_rt.h
 * so that flag/overflow/edge-case semantics live in exactly one place.
 */
#ifndef PSPRECOMP_CPU_H
#define PSPRECOMP_CPU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Register indices, by ABI name. Generated code uses these so the emitted C
 * reads like the original assembly: psp_cpu.r[PSP_REG_A0] not psp_cpu.r[4]. */
enum {
    PSP_REG_ZERO = 0, PSP_REG_AT,
    PSP_REG_V0, PSP_REG_V1,
    PSP_REG_A0, PSP_REG_A1, PSP_REG_A2, PSP_REG_A3,
    PSP_REG_T0, PSP_REG_T1, PSP_REG_T2, PSP_REG_T3,
    PSP_REG_T4, PSP_REG_T5, PSP_REG_T6, PSP_REG_T7,
    PSP_REG_S0, PSP_REG_S1, PSP_REG_S2, PSP_REG_S3,
    PSP_REG_S4, PSP_REG_S5, PSP_REG_S6, PSP_REG_S7,
    PSP_REG_T8, PSP_REG_T9,
    PSP_REG_K0, PSP_REG_K1,
    PSP_REG_GP, PSP_REG_SP, PSP_REG_FP, PSP_REG_RA,
    PSP_NUM_GPR = 32
};

/* Canonical ABI register names, indexed by the enum above. */
extern const char *const psp_reg_names[PSP_NUM_GPR];

typedef struct {
    uint32_t r[PSP_NUM_GPR];   /* r[0] is hardwired zero; see psp_set_reg() */
    uint32_t hi, lo;
    uint32_t pc;               /* only meaningful at dispatch boundaries */

    float    f[32];            /* COP1 — single precision only */
    uint32_t fcr31;            /* FPU control/status; bit 23 is the C flag */

    /* VFPU register file: 8 matrices x 4 rows x 4 columns = 128 floats.
     * Indexed linearly here; vfpu.h provides the matrix/row/column views. */
    float    v[128];
    uint32_t vfpu_cc;          /* VFPU condition codes (vcmp results) */
} psp_cpu_state;

extern psp_cpu_state psp_cpu;

/* Write a GPR, honouring the hardwired-zero rule for $zero. Generated code
 * calls this rather than assigning r[] directly, so a stray write to $zero
 * can never corrupt state. */
static inline void psp_set_reg(uint32_t idx, uint32_t val) {
    if (idx != PSP_REG_ZERO) psp_cpu.r[idx] = val;
}

void psp_cpu_reset(void);

/* FPU condition flag (fcr31 bit 23) — set by c.cond.s, tested by bc1t/bc1f. */
#define PSP_FCR31_C (1u << 23)

static inline int  psp_fpu_cond(void)      { return (psp_cpu.fcr31 & PSP_FCR31_C) != 0; }
static inline void psp_fpu_set_cond(int c) {
    psp_cpu.fcr31 = c ? (psp_cpu.fcr31 | PSP_FCR31_C) : (psp_cpu.fcr31 & ~PSP_FCR31_C);
}

#ifdef __cplusplus
}
#endif

#endif /* PSPRECOMP_CPU_H */
