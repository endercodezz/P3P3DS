/* allegrexrecomp — Allegrex (MIPS32r2) instruction decoder.
 *
 * One decoder serves the whole toolkit: the disassembler, the analyzer that
 * discovers function boundaries, the C emitter, and the interpreter that acts
 * as the bring-up oracle. They must agree about what an instruction *is*, so
 * they all call a_decode() and branch on the same a_op enum. When the emitter
 * and the oracle disagree about a game, the bug is in exactly one of them —
 * never in two divergent copies of the decode logic.
 *
 * Allegrex is MIPS32r2 minus the 64-bit integer ops, plus:
 *   - CLZ/CLO at SPECIAL 0x16/0x17 (not SPECIAL2, where MIPS32 puts them)
 *   - MAX/MIN at SPECIAL 0x2C/0x2D
 *   - MADD/MADDU/MSUB/MSUBU at SPECIAL 0x1C/0x1D/0x2E/0x2F
 *   - BITREV and WSBW in the SPECIAL3 BSHFL group
 *   - the VFPU occupying COP2 and opcodes 0x18/0x19/0x1B/0x1C, plus its own
 *     load/store opcodes (lv.s 0x32, lv.q 0x36, sv.s 0x3A, sv.q 0x3E)
 */
#ifndef ALLEGREX_DECODE_H
#define ALLEGREX_DECODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* $ra — the return-address register. `jr $ra` is how the analyzer recognises a
 * function return, which is the single most important boundary signal we have. */
#define PSP_RA_INDEX 31

typedef enum {
    A_INVALID = 0,

    /* SPECIAL — shifts */
    A_SLL, A_SRL, A_SRA, A_SLLV, A_SRLV, A_SRAV, A_ROTR, A_ROTRV,
    /* SPECIAL — jumps / moves / traps */
    A_JR, A_JALR, A_MOVZ, A_MOVN, A_SYSCALL, A_BREAK, A_SYNC,
    A_MFHI, A_MTHI, A_MFLO, A_MTLO,
    /* SPECIAL — Allegrex bit ops */
    A_CLZ, A_CLO,
    /* SPECIAL — multiply / divide */
    A_MULT, A_MULTU, A_DIV, A_DIVU, A_MADD, A_MADDU, A_MSUB, A_MSUBU,
    /* SPECIAL — ALU */
    A_ADD, A_ADDU, A_SUB, A_SUBU, A_AND, A_OR, A_XOR, A_NOR,
    A_SLT, A_SLTU, A_MAX, A_MIN,

    /* REGIMM */
    A_BLTZ, A_BGEZ, A_BLTZL, A_BGEZL, A_BLTZAL, A_BGEZAL, A_BLTZALL, A_BGEZALL,

    /* Jumps and branches */
    A_J, A_JAL,
    A_BEQ, A_BNE, A_BLEZ, A_BGTZ,
    A_BEQL, A_BNEL, A_BLEZL, A_BGTZL,

    /* Immediate ALU */
    A_ADDI, A_ADDIU, A_SLTI, A_SLTIU, A_ANDI, A_ORI, A_XORI, A_LUI,

    /* SPECIAL3 */
    A_EXT, A_INS, A_WSBH, A_WSBW, A_SEB, A_SEH, A_BITREV,

    /* Loads / stores */
    A_LB, A_LH, A_LWL, A_LW, A_LBU, A_LHU, A_LWR,
    A_SB, A_SH, A_SWL, A_SW, A_SWR,
    A_LL, A_SC, A_CACHE, A_PREF,

    /* COP0 */
    A_MFC0, A_MTC0, A_CFC0, A_CTC0, A_MFIC, A_MTIC, A_ERET,

    /* COP1 — single precision only */
    A_MFC1, A_CFC1, A_MTC1, A_CTC1,
    A_BC1F, A_BC1T, A_BC1FL, A_BC1TL,
    A_ADD_S, A_SUB_S, A_MUL_S, A_DIV_S, A_SQRT_S, A_ABS_S, A_MOV_S, A_NEG_S,
    A_ROUND_W_S, A_TRUNC_W_S, A_CEIL_W_S, A_FLOOR_W_S,
    A_CVT_W_S, A_CVT_S_W, A_C_COND_S,
    A_LWC1, A_SWC1,

    /* VFPU — control transfer and register moves */
    A_MFV, A_MTV, A_MFVC, A_MTVC, A_BVF, A_BVT, A_BVFL, A_BVTL,
    /* VFPU — load / store */
    A_LV_S, A_LV_Q, A_SV_S, A_SV_Q, A_LVL_Q, A_LVR_Q, A_SVL_Q, A_SVR_Q,
    /* VFPU — arithmetic (the families we decode today; see docs/VFPU.md) */
    A_VADD, A_VSUB, A_VDIV, A_VMUL, A_VDOT, A_VSCL, A_VHDP, A_VCRS, A_VDET,
    A_VCMP, A_VMIN, A_VMAX, A_VSCMP, A_VSGE, A_VSLT,
    A_VMOV, A_VABS, A_VNEG, A_VZERO, A_VONE, A_VRCP, A_VRSQ, A_VSQRT,
    A_VSIN, A_VCOS, A_VEXP2, A_VLOG2, A_VI2F, A_VF2I,
    /* Prefix instructions. These compute nothing: they set a register that
     * rewrites the operands of the *next* instruction. Decoding them is not
     * optional — if they are missed, the following arithmetic silently
     * computes the unprefixed answer. */
    A_VPFXS, A_VPFXT, A_VPFXD,
    /* VFPU4 (opcode 0x34, rs=0): unary element-wise ops. */
    A_VIDT, A_VSAT0, A_VSAT1, A_VASIN, A_VNRCP, A_VNSIN, A_VREXP2,
    A_VCST, A_VF2IN, A_VF2IZ, A_VF2IU, A_VF2ID, A_VCMOV, A_VWBN,
    /* VFPU6 (opcode 0x3C): the matrix unit. */
    A_VMMUL, A_VTFM2, A_VTFM3, A_VTFM4, A_VMSCL, A_VCRSP, A_VROT,
    A_VMMOV, A_VMIDT, A_VMZERO, A_VMONE,
    /* Any VFPU encoding we recognise as VFPU but do not yet name. Emitting
     * these is a hard error rather than a silent wrong answer. */
    A_VIIM,
    A_VFIM,
    A_VFPU_UNKNOWN,

    A_NOP,          /* canonical: sll $zero, $zero, 0 */
    A_OP_COUNT
} a_op;

/* How to print / how many operands. The emitter switches on a_op, not this;
 * this exists for the disassembler and the annotated comments in generated C. */
typedef enum {
    F_NONE, F_RD_RS_RT, F_RT_RS_IMM, F_RT_RS_UIMM, F_RD_RT_SA, F_RD_RT_RS,
    F_RS_RT_OFF, F_RS_OFF, F_TARGET, F_RS, F_RD_RS, F_RD_RT, F_RD, F_RS_RT,
    F_RT_IMM, F_RT_OFF_BASE, F_FT_OFF_BASE, F_RT_FS, F_FD_FS_FT, F_FD_FS,
    F_FS_FT, F_OFF, F_CODE, F_RT_RS_POS_SZ, F_RD_RS_RT_SA,
    F_VD_VS_VT, F_VD_VS, F_VT_OFF_BASE, F_RT_VD, F_UNKNOWN
} a_fmt;

typedef struct {
    uint32_t raw;        /* the original 32-bit word */
    uint32_t addr;       /* the address it was decoded at */
    a_op     op;
    a_fmt    fmt;

    uint8_t  rs, rt, rd, sa;
    int32_t  imm;        /* sign-extended for signed forms, zero-extended otherwise */
    uint32_t target;     /* absolute branch/jump target (valid if has_target) */

    uint8_t  fs, ft, fd; /* COP1 register fields */
    uint8_t  fcond;      /* c.cond.s condition code */

    uint8_t  vs, vt, vd; /* VFPU register fields (7-bit) */
    uint8_t  vsize;      /* 1=single 2=pair 3=triple 4=quad */

    /* Analysis flags — what the discovery pass and the emitter need to know
     * without re-deriving it from the opcode. */
    unsigned has_target     : 1;
    unsigned is_branch      : 1;  /* conditional, falls through if not taken */
    unsigned is_jump        : 1;  /* unconditional */
    unsigned is_call        : 1;  /* jal / jalr / *al — writes $ra */
    unsigned is_indirect    : 1;  /* jr / jalr — target comes from a register */
    unsigned is_return      : 1;  /* jr $ra specifically */
    unsigned is_likely      : 1;  /* *L form: delay slot nullified if not taken */
    unsigned has_delay_slot : 1;
    unsigned ends_block     : 1;  /* control leaves here unconditionally */
} a_insn;

/* Decode one little-endian word. Always fills `out` (op == A_INVALID for
 * encodings we do not recognise) and returns 1 for a recognised instruction,
 * 0 otherwise — so a caller can count decode coverage over a whole image. */
int a_decode(uint32_t word, uint32_t addr, a_insn *out);

/* Canonical mnemonic, e.g. "addiu". Never NULL. */
const char *a_mnemonic(a_op op);

/* Format one decoded instruction into `buf` as text, e.g.
 * "addiu     $sp, $sp, -32". Returns the number of bytes written. */
int a_format(const a_insn *in, char *buf, int buflen);

/* Register name helpers, shared with the emitter's annotated comments. */
const char *a_reg_name(unsigned idx);
const char *a_freg_name(unsigned idx);

#ifdef __cplusplus
}
#endif

#endif /* ALLEGREX_DECODE_H */
