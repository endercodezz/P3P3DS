/* psprecomp — the VFPU.
 *
 * 128 single-precision registers, addressed as 8 matrices of 4x4. A single
 * 7-bit register field can name a scalar, a 2/3/4-element row, a column, or a
 * whole matrix, depending on the instruction's width bits and a transpose bit.
 *
 * ## Register layout
 *
 * The file is indexed `v[matrix*4 + column*32 + row]`. That looks arbitrary
 * and is not: it is the layout that makes a row and a column of the same
 * matrix alias the same storage the way the hardware does. A game that writes
 * a matrix by columns and reads it by rows -- which is exactly what a
 * transpose does -- only works if this matches.
 *
 * ## What is implemented, and what deliberately is not
 *
 * Measured against a real module (WTF's Lumberjack, 1276 VFPU-space words in
 * .text), the distribution is heavily skewed: quad load/store is 48% of it,
 * and vscl/vmul/vadd/vdot another 18%. Those, plus the compare/min/max family,
 * are implemented here and unit-tested.
 *
 * The **prefix** instructions are not. `vpfxs`/`vpfxt`/`vpfxd` do not compute
 * anything -- they set a register that rewrites the *operands of the next
 * instruction*, swizzling lanes, negating, forcing constants, masking writes.
 * An arithmetic op executed while a prefix is pending computes something
 * different from the same op without one.
 *
 * So a pending prefix makes the next arithmetic op **trap** rather than
 * compute. Implementing the arithmetic while ignoring the prefixes would be
 * worse than not implementing it at all: it would produce numbers that are
 * silently wrong instead of an error that says so. Loud beats plausible.
 */
#ifndef PSPRECOMP_VFPU_H
#define PSPRECOMP_VFPU_H

#include "cpu.h"
#include "mem.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Resolve a 7-bit register field and width into up to four indices into
 * psp_cpu.v[]. Returns the number of lanes. */
int psp_vfpu_regs(uint32_t vreg, int size, int out[4]);

/* Load/store. Quad forms are 16-byte aligned on hardware; the address is
 * masked rather than faulting, which is what the hardware does. */
void psp_lv_s(uint32_t vt, uint32_t addr);
void psp_lv_q(uint32_t vt, uint32_t addr);
void psp_sv_s(uint32_t vt, uint32_t addr);
void psp_sv_q(uint32_t vt, uint32_t addr);

/* Element-wise arithmetic across `size` lanes. */
void psp_vadd(uint32_t vd, uint32_t vs, uint32_t vt, int size);
void psp_vsub(uint32_t vd, uint32_t vs, uint32_t vt, int size);
void psp_vmul(uint32_t vd, uint32_t vs, uint32_t vt, int size);
void psp_vdiv(uint32_t vd, uint32_t vs, uint32_t vt, int size);
void psp_vmin(uint32_t vd, uint32_t vs, uint32_t vt, int size);
void psp_vmax(uint32_t vd, uint32_t vs, uint32_t vt, int size);

/* Reductions. vdot writes one lane; vscl scales a vector by a scalar. */
void psp_vdot(uint32_t vd, uint32_t vs, uint32_t vt, int size);
void psp_vscl(uint32_t vd, uint32_t vs, uint32_t vt, int size);

/* Comparison, writing the VFPU condition codes. */
void psp_vcmp(uint32_t cond, uint32_t vs, uint32_t vt, int size);

/* Unary element-wise ops (VFPU4). One entry point rather than eighteen, since
 * they differ only in the scalar function applied per lane. */
enum {
    PSP_VU_MOV = 0, PSP_VU_ABS, PSP_VU_NEG, PSP_VU_ZERO, PSP_VU_ONE,
    PSP_VU_RCP, PSP_VU_RSQ, PSP_VU_SQRT, PSP_VU_SIN, PSP_VU_COS,
    PSP_VU_EXP2, PSP_VU_LOG2, PSP_VU_SAT0, PSP_VU_SAT1,
    PSP_VU_NRCP, PSP_VU_NSIN, PSP_VU_REXP2, PSP_VU_ASIN,
    PSP_VU_F2IZ, PSP_VU_I2F
};
void psp_vunary(int op, uint32_t vd, uint32_t vs, int size);

/* Matrix ops that need no multiply: identity, zero, one, and copy. `size` is
 * the matrix order (2, 3 or 4). */
void psp_vmidt(uint32_t vd, int size);
void psp_vidt(uint32_t vd, int size);
void psp_vimm(uint32_t vd, float value);
void psp_vcst(uint32_t vd, uint32_t which, int size);
void psp_vmzero(uint32_t vd, int size);
void psp_vmone(uint32_t vd, int size);
void psp_vmmov(uint32_t vd, uint32_t vs, int size);

/* Matrix multiply, transform and scale. See the note in vfpu.c: the operand
 * orientation of vmmul is not independently verified. */
void psp_vmscl(uint32_t vd, uint32_t vs, uint32_t vt, int size);
void psp_vtfm(uint32_t vd, uint32_t vs, uint32_t vt, int size);
void psp_vmmul(uint32_t vd, uint32_t vs, uint32_t vt, int size);

/* Prefix state. Set by vpfxs/vpfxt/vpfxd; consumed (and cleared) by the next
 * arithmetic instruction. While any is pending, arithmetic traps. */
void psp_vfpu_set_prefix(int which, uint32_t value);
int  psp_vfpu_prefix_pending(void);
void psp_vfpu_reset(void);

/* Reports an instruction the VFPU cannot yet execute, by address and name. */
void psp_vfpu_unimplemented(uint32_t addr, const char *what);
uint64_t psp_vfpu_trap_count(void);

#ifdef __cplusplus
}
#endif

#endif /* PSPRECOMP_VFPU_H */
