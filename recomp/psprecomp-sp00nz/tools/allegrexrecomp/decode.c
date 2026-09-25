/* allegrexrecomp — Allegrex instruction decoder. See decode.h for the contract. */

#include "decode.h"

#include <stdio.h>
#include <string.h>

/* ---- field extraction ---------------------------------------------------- */

#define OPCODE(w)  ((w) >> 26)
#define RS_F(w)    (((w) >> 21) & 0x1F)
#define RT_F(w)    (((w) >> 16) & 0x1F)
#define RD_F(w)    (((w) >> 11) & 0x1F)
#define SA_F(w)    (((w) >>  6) & 0x1F)
#define FUNCT(w)   ((w) & 0x3F)
#define IMM16(w)   ((w) & 0xFFFF)
#define SIMM16(w)  ((int32_t)(int16_t)((w) & 0xFFFF))
#define IMM26(w)   ((w) & 0x03FFFFFF)

/* VFPU register fields are 7 bits and overlap the integer fields. */
#define VD_F(w)    ((w) & 0x7F)
#define VS_F(w)    (((w) >> 8) & 0x7F)
#define VT_F(w)    (((w) >> 16) & 0x7F)
/* Vector width is split across two non-adjacent bits: 0..3 -> 1..4 lanes. */
#define VSIZE(w)   (((((w) >> 7) & 1) | (((w) >> 14) & 2)) + 1)

/* ---- mnemonic / format table --------------------------------------------- */
/* Designated initialisers so this table cannot drift out of sync with the enum
 * if someone inserts an op in the middle. */

typedef struct { const char *name; a_fmt fmt; } a_opinfo;

static const a_opinfo OPINFO[A_OP_COUNT] = {
    [A_INVALID]  = { "?",        F_UNKNOWN },
    [A_NOP]      = { "nop",      F_NONE },

    [A_SLL]      = { "sll",      F_RD_RT_SA },
    [A_SRL]      = { "srl",      F_RD_RT_SA },
    [A_SRA]      = { "sra",      F_RD_RT_SA },
    [A_ROTR]     = { "rotr",     F_RD_RT_SA },
    [A_SLLV]     = { "sllv",     F_RD_RT_RS },
    [A_SRLV]     = { "srlv",     F_RD_RT_RS },
    [A_SRAV]     = { "srav",     F_RD_RT_RS },
    [A_ROTRV]    = { "rotrv",    F_RD_RT_RS },

    [A_JR]       = { "jr",       F_RS },
    [A_JALR]     = { "jalr",     F_RD_RS },
    [A_MOVZ]     = { "movz",     F_RD_RS_RT },
    [A_MOVN]     = { "movn",     F_RD_RS_RT },
    [A_SYSCALL]  = { "syscall",  F_CODE },
    [A_BREAK]    = { "break",    F_CODE },
    [A_SYNC]     = { "sync",     F_NONE },
    [A_MFHI]     = { "mfhi",     F_RD },
    [A_MTHI]     = { "mthi",     F_RS },
    [A_MFLO]     = { "mflo",     F_RD },
    [A_MTLO]     = { "mtlo",     F_RS },
    [A_CLZ]      = { "clz",      F_RD_RS },
    [A_CLO]      = { "clo",      F_RD_RS },

    [A_MULT]     = { "mult",     F_RS_RT },
    [A_MULTU]    = { "multu",    F_RS_RT },
    [A_DIV]      = { "div",      F_RS_RT },
    [A_DIVU]     = { "divu",     F_RS_RT },
    [A_MADD]     = { "madd",     F_RS_RT },
    [A_MADDU]    = { "maddu",    F_RS_RT },
    [A_MSUB]     = { "msub",     F_RS_RT },
    [A_MSUBU]    = { "msubu",    F_RS_RT },

    [A_ADD]      = { "add",      F_RD_RS_RT },
    [A_ADDU]     = { "addu",     F_RD_RS_RT },
    [A_SUB]      = { "sub",      F_RD_RS_RT },
    [A_SUBU]     = { "subu",     F_RD_RS_RT },
    [A_AND]      = { "and",      F_RD_RS_RT },
    [A_OR]       = { "or",       F_RD_RS_RT },
    [A_XOR]      = { "xor",      F_RD_RS_RT },
    [A_NOR]      = { "nor",      F_RD_RS_RT },
    [A_SLT]      = { "slt",      F_RD_RS_RT },
    [A_SLTU]     = { "sltu",     F_RD_RS_RT },
    [A_MAX]      = { "max",      F_RD_RS_RT },
    [A_MIN]      = { "min",      F_RD_RS_RT },

    [A_BLTZ]     = { "bltz",     F_RS_OFF },
    [A_BGEZ]     = { "bgez",     F_RS_OFF },
    [A_BLTZL]    = { "bltzl",    F_RS_OFF },
    [A_BGEZL]    = { "bgezl",    F_RS_OFF },
    [A_BLTZAL]   = { "bltzal",   F_RS_OFF },
    [A_BGEZAL]   = { "bgezal",   F_RS_OFF },
    [A_BLTZALL]  = { "bltzall",  F_RS_OFF },
    [A_BGEZALL]  = { "bgezall",  F_RS_OFF },

    [A_J]        = { "j",        F_TARGET },
    [A_JAL]      = { "jal",      F_TARGET },
    [A_BEQ]      = { "beq",      F_RS_RT_OFF },
    [A_BNE]      = { "bne",      F_RS_RT_OFF },
    [A_BLEZ]     = { "blez",     F_RS_OFF },
    [A_BGTZ]     = { "bgtz",     F_RS_OFF },
    [A_BEQL]     = { "beql",     F_RS_RT_OFF },
    [A_BNEL]     = { "bnel",     F_RS_RT_OFF },
    [A_BLEZL]    = { "blezl",    F_RS_OFF },
    [A_BGTZL]    = { "bgtzl",    F_RS_OFF },

    [A_ADDI]     = { "addi",     F_RT_RS_IMM },
    [A_ADDIU]    = { "addiu",    F_RT_RS_IMM },
    [A_SLTI]     = { "slti",     F_RT_RS_IMM },
    [A_SLTIU]    = { "sltiu",    F_RT_RS_IMM },
    [A_ANDI]     = { "andi",     F_RT_RS_UIMM },
    [A_ORI]      = { "ori",      F_RT_RS_UIMM },
    [A_XORI]     = { "xori",     F_RT_RS_UIMM },
    [A_LUI]      = { "lui",      F_RT_IMM },

    [A_EXT]      = { "ext",      F_RT_RS_POS_SZ },
    [A_INS]      = { "ins",      F_RT_RS_POS_SZ },
    /* The BSHFL group reads rt and writes rd — not rs, which is unused. */
    [A_WSBH]     = { "wsbh",     F_RD_RT },
    [A_WSBW]     = { "wsbw",     F_RD_RT },
    [A_SEB]      = { "seb",      F_RD_RT },
    [A_SEH]      = { "seh",      F_RD_RT },
    [A_BITREV]   = { "bitrev",   F_RD_RT },

    [A_LB]       = { "lb",       F_RT_OFF_BASE },
    [A_LH]       = { "lh",       F_RT_OFF_BASE },
    [A_LWL]      = { "lwl",      F_RT_OFF_BASE },
    [A_LW]       = { "lw",       F_RT_OFF_BASE },
    [A_LBU]      = { "lbu",      F_RT_OFF_BASE },
    [A_LHU]      = { "lhu",      F_RT_OFF_BASE },
    [A_LWR]      = { "lwr",      F_RT_OFF_BASE },
    [A_SB]       = { "sb",       F_RT_OFF_BASE },
    [A_SH]       = { "sh",       F_RT_OFF_BASE },
    [A_SWL]      = { "swl",      F_RT_OFF_BASE },
    [A_SW]       = { "sw",       F_RT_OFF_BASE },
    [A_SWR]      = { "swr",      F_RT_OFF_BASE },
    [A_LL]       = { "ll",       F_RT_OFF_BASE },
    [A_SC]       = { "sc",       F_RT_OFF_BASE },
    [A_CACHE]    = { "cache",    F_RT_OFF_BASE },
    [A_PREF]     = { "pref",     F_RT_OFF_BASE },

    [A_MFC0]     = { "mfc0",     F_RT_FS },
    [A_MTC0]     = { "mtc0",     F_RT_FS },
    [A_CFC0]     = { "cfc0",     F_RT_FS },
    [A_CTC0]     = { "ctc0",     F_RT_FS },
    [A_MFIC]     = { "mfic",     F_RT_FS },
    [A_MTIC]     = { "mtic",     F_RT_FS },
    [A_ERET]     = { "eret",     F_NONE },

    [A_MFC1]     = { "mfc1",     F_RT_FS },
    [A_CFC1]     = { "cfc1",     F_RT_FS },
    [A_MTC1]     = { "mtc1",     F_RT_FS },
    [A_CTC1]     = { "ctc1",     F_RT_FS },
    [A_BC1F]     = { "bc1f",     F_OFF },
    [A_BC1T]     = { "bc1t",     F_OFF },
    [A_BC1FL]    = { "bc1fl",    F_OFF },
    [A_BC1TL]    = { "bc1tl",    F_OFF },
    [A_ADD_S]    = { "add.s",    F_FD_FS_FT },
    [A_SUB_S]    = { "sub.s",    F_FD_FS_FT },
    [A_MUL_S]    = { "mul.s",    F_FD_FS_FT },
    [A_DIV_S]    = { "div.s",    F_FD_FS_FT },
    [A_SQRT_S]   = { "sqrt.s",   F_FD_FS },
    [A_ABS_S]    = { "abs.s",    F_FD_FS },
    [A_MOV_S]    = { "mov.s",    F_FD_FS },
    [A_NEG_S]    = { "neg.s",    F_FD_FS },
    [A_ROUND_W_S]= { "round.w.s",F_FD_FS },
    [A_TRUNC_W_S]= { "trunc.w.s",F_FD_FS },
    [A_CEIL_W_S] = { "ceil.w.s", F_FD_FS },
    [A_FLOOR_W_S]= { "floor.w.s",F_FD_FS },
    [A_CVT_W_S]  = { "cvt.w.s",  F_FD_FS },
    [A_CVT_S_W]  = { "cvt.s.w",  F_FD_FS },
    [A_C_COND_S] = { "c.cond.s", F_FS_FT },
    [A_LWC1]     = { "lwc1",     F_FT_OFF_BASE },
    [A_SWC1]     = { "swc1",     F_FT_OFF_BASE },

    [A_MFV]      = { "mfv",      F_RT_VD },
    [A_MTV]      = { "mtv",      F_RT_VD },
    [A_MFVC]     = { "mfvc",     F_RT_VD },
    [A_MTVC]     = { "mtvc",     F_RT_VD },
    [A_BVF]      = { "bvf",      F_OFF },
    [A_BVT]      = { "bvt",      F_OFF },
    [A_BVFL]     = { "bvfl",     F_OFF },
    [A_BVTL]     = { "bvtl",     F_OFF },
    [A_LV_S]     = { "lv.s",     F_VT_OFF_BASE },
    [A_LV_Q]     = { "lv.q",     F_VT_OFF_BASE },
    [A_SV_S]     = { "sv.s",     F_VT_OFF_BASE },
    [A_SV_Q]     = { "sv.q",     F_VT_OFF_BASE },
    [A_LVL_Q]    = { "lvl.q",    F_VT_OFF_BASE },
    [A_LVR_Q]    = { "lvr.q",    F_VT_OFF_BASE },
    [A_SVL_Q]    = { "svl.q",    F_VT_OFF_BASE },
    [A_SVR_Q]    = { "svr.q",    F_VT_OFF_BASE },
    [A_VADD]     = { "vadd",     F_VD_VS_VT },
    [A_VSUB]     = { "vsub",     F_VD_VS_VT },
    [A_VDIV]     = { "vdiv",     F_VD_VS_VT },
    [A_VMUL]     = { "vmul",     F_VD_VS_VT },
    [A_VDOT]     = { "vdot",     F_VD_VS_VT },
    [A_VSCL]     = { "vscl",     F_VD_VS_VT },
    [A_VHDP]     = { "vhdp",     F_VD_VS_VT },
    [A_VCRS]     = { "vcrs",     F_VD_VS_VT },
    [A_VDET]     = { "vdet",     F_VD_VS_VT },
    [A_VCMP]     = { "vcmp",     F_VD_VS_VT },
    [A_VMIN]     = { "vmin",     F_VD_VS_VT },
    [A_VMAX]     = { "vmax",     F_VD_VS_VT },
    [A_VSCMP]    = { "vscmp",    F_VD_VS_VT },
    [A_VSGE]     = { "vsge",     F_VD_VS_VT },
    [A_VSLT]     = { "vslt",     F_VD_VS_VT },
    [A_VMOV]     = { "vmov",     F_VD_VS },
    [A_VABS]     = { "vabs",     F_VD_VS },
    [A_VNEG]     = { "vneg",     F_VD_VS },
    [A_VZERO]    = { "vzero",    F_VD_VS },
    [A_VONE]     = { "vone",     F_VD_VS },
    [A_VRCP]     = { "vrcp",     F_VD_VS },
    [A_VRSQ]     = { "vrsq",     F_VD_VS },
    [A_VSQRT]    = { "vsqrt",    F_VD_VS },
    [A_VSIN]     = { "vsin",     F_VD_VS },
    [A_VCOS]     = { "vcos",     F_VD_VS },
    [A_VEXP2]    = { "vexp2",    F_VD_VS },
    [A_VLOG2]    = { "vlog2",    F_VD_VS },
    [A_VI2F]     = { "vi2f",     F_VD_VS },
    [A_VF2I]     = { "vf2i",     F_VD_VS },
    [A_VIDT]     = { "vidt",     F_VD_VS },
    [A_VSAT0]    = { "vsat0",    F_VD_VS },
    [A_VSAT1]    = { "vsat1",    F_VD_VS },
    [A_VASIN]    = { "vasin",    F_VD_VS },
    [A_VNRCP]    = { "vnrcp",    F_VD_VS },
    [A_VNSIN]    = { "vnsin",    F_VD_VS },
    [A_VREXP2]   = { "vrexp2",   F_VD_VS },
    [A_VCST]     = { "vcst",     F_VD_VS },
    [A_VF2IN]    = { "vf2in",    F_VD_VS },
    [A_VF2IZ]    = { "vf2iz",    F_VD_VS },
    [A_VF2IU]    = { "vf2iu",    F_VD_VS },
    [A_VF2ID]    = { "vf2id",    F_VD_VS },
    [A_VCMOV]    = { "vcmov",    F_VD_VS },
    [A_VWBN]     = { "vwbn",     F_VD_VS },
    [A_VMMUL]    = { "vmmul",    F_VD_VS_VT },
    [A_VTFM2]    = { "vtfm2",    F_VD_VS_VT },
    [A_VTFM3]    = { "vtfm3",    F_VD_VS_VT },
    [A_VTFM4]    = { "vtfm4",    F_VD_VS_VT },
    [A_VMSCL]    = { "vmscl",    F_VD_VS_VT },
    [A_VCRSP]    = { "vcrsp",    F_VD_VS_VT },
    [A_VROT]     = { "vrot",     F_VD_VS },
    [A_VMMOV]    = { "vmmov",    F_VD_VS },
    [A_VMIDT]    = { "vmidt",    F_VD_VS },
    [A_VMZERO]   = { "vmzero",   F_VD_VS },
    [A_VMONE]    = { "vmone",    F_VD_VS },
    [A_VPFXS]    = { "vpfxs",    F_UNKNOWN },
    [A_VPFXT]    = { "vpfxt",    F_UNKNOWN },
    [A_VPFXD]    = { "vpfxd",    F_UNKNOWN },
        [A_VIIM]     = { "viim",     F_UNKNOWN },
    [A_VFIM]     = { "vfim",     F_UNKNOWN },
    [A_VFPU_UNKNOWN] = { "vfpu?", F_UNKNOWN },
};

static const char *const REG_NAMES[32] = {
    "$zero","$at","$v0","$v1","$a0","$a1","$a2","$a3",
    "$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7",
    "$s0","$s1","$s2","$s3","$s4","$s5","$s6","$s7",
    "$t8","$t9","$k0","$k1","$gp","$sp","$fp","$ra"
};

static const char *const FREG_NAMES[32] = {
    "$f0","$f1","$f2","$f3","$f4","$f5","$f6","$f7",
    "$f8","$f9","$f10","$f11","$f12","$f13","$f14","$f15",
    "$f16","$f17","$f18","$f19","$f20","$f21","$f22","$f23",
    "$f24","$f25","$f26","$f27","$f28","$f29","$f30","$f31"
};

const char *a_reg_name(unsigned idx)  { return REG_NAMES[idx & 31]; }
const char *a_freg_name(unsigned idx) { return FREG_NAMES[idx & 31]; }

const char *a_mnemonic(a_op op) {
    if (op <= A_INVALID || op >= A_OP_COUNT || !OPINFO[op].name) return "?";
    return OPINFO[op].name;
}

/* ---- helpers ------------------------------------------------------------- */

static void set_branch(a_insn *in, uint32_t addr, int likely) {
    in->target        = addr + 4 + (uint32_t)(SIMM16(in->raw) << 2);
    in->has_target    = 1;
    in->is_branch     = 1;
    in->has_delay_slot= 1;
    in->is_likely     = likely ? 1 : 0;
}

static void set_jump(a_insn *in, uint32_t addr) {
    /* The upper 4 bits come from the *delay slot's* address, not the jump's. */
    in->target        = ((addr + 4) & 0xF0000000u) | (IMM26(in->raw) << 2);
    in->has_target    = 1;
    in->is_jump       = 1;
    in->has_delay_slot= 1;
    in->ends_block    = 1;
}

/* ---- decode -------------------------------------------------------------- */

static a_op decode_special(uint32_t w, uint32_t addr, a_insn *in) {
    switch (FUNCT(w)) {
    case 0x00:
        /* sll $zero,$zero,0 is the canonical nop; keep it distinct so the
         * emitter can drop it instead of emitting a dead statement. */
        return (w == 0) ? A_NOP : A_SLL;
    case 0x02: return (RS_F(w) == 1) ? A_ROTR : A_SRL;
    case 0x03: return A_SRA;
    case 0x04: return A_SLLV;
    case 0x06: return (SA_F(w) == 1) ? A_ROTRV : A_SRLV;
    case 0x07: return A_SRAV;
    case 0x08:
        in->is_jump = in->is_indirect = in->has_delay_slot = in->ends_block = 1;
        if (RS_F(w) == PSP_RA_INDEX) in->is_return = 1;
        return A_JR;
    case 0x09:
        in->is_jump = in->is_indirect = in->is_call = in->has_delay_slot = 1;
        return A_JALR;
    case 0x0A: return A_MOVZ;
    case 0x0B: return A_MOVN;
    case 0x0C: return A_SYSCALL;
    case 0x0D: in->ends_block = 1; return A_BREAK;
    case 0x0F: return A_SYNC;
    case 0x10: return A_MFHI;
    case 0x11: return A_MTHI;
    case 0x12: return A_MFLO;
    case 0x13: return A_MTLO;
    case 0x16: return A_CLZ;   /* Allegrex puts these in SPECIAL, not SPECIAL2 */
    case 0x17: return A_CLO;
    case 0x18: return A_MULT;
    case 0x19: return A_MULTU;
    case 0x1A: return A_DIV;
    case 0x1B: return A_DIVU;
    case 0x1C: return A_MADD;
    case 0x1D: return A_MADDU;
    case 0x20: return A_ADD;
    case 0x21: return A_ADDU;
    case 0x22: return A_SUB;
    case 0x23: return A_SUBU;
    case 0x24: return A_AND;
    case 0x25: return A_OR;
    case 0x26: return A_XOR;
    case 0x27: return A_NOR;
    case 0x2A: return A_SLT;
    case 0x2B: return A_SLTU;
    case 0x2C: return A_MAX;   /* Allegrex extension */
    case 0x2D: return A_MIN;   /* Allegrex extension */
    case 0x2E: return A_MSUB;
    case 0x2F: return A_MSUBU;
    default:   return A_INVALID;
    }
}

static a_op decode_regimm(uint32_t w, uint32_t addr, a_insn *in) {
    switch (RT_F(w)) {
    case 0x00: set_branch(in, addr, 0); return A_BLTZ;
    case 0x01: set_branch(in, addr, 0); return A_BGEZ;
    case 0x02: set_branch(in, addr, 1); return A_BLTZL;
    case 0x03: set_branch(in, addr, 1); return A_BGEZL;
    case 0x10: set_branch(in, addr, 0); in->is_call = 1; return A_BLTZAL;
    case 0x11: set_branch(in, addr, 0); in->is_call = 1; return A_BGEZAL;
    case 0x12: set_branch(in, addr, 1); in->is_call = 1; return A_BLTZALL;
    case 0x13: set_branch(in, addr, 1); in->is_call = 1; return A_BGEZALL;
    default:   return A_INVALID;
    }
}

static a_op decode_special3(uint32_t w) {
    switch (FUNCT(w)) {
    case 0x00: return A_EXT;
    case 0x04: return A_INS;
    case 0x20:                       /* BSHFL group, selected by the sa field */
        switch (SA_F(w)) {
        case 0x02: return A_WSBH;
        case 0x03: return A_WSBW;    /* Allegrex extension */
        case 0x10: return A_SEB;
        case 0x14: return A_BITREV;  /* Allegrex extension */
        case 0x18: return A_SEH;
        default:   return A_INVALID;
        }
    default: return A_INVALID;
    }
}

static a_op decode_cop0(uint32_t w) {
    switch (RS_F(w)) {
    case 0x00: return A_MFC0;
    case 0x02: return A_CFC0;
    case 0x04: return A_MTC0;
    case 0x06: return A_CTC0;
    case 0x0B: return (RT_F(w) & 1) ? A_MTIC : A_MFIC;  /* Allegrex */
    case 0x10: return (FUNCT(w) == 0x18) ? A_ERET : A_INVALID;
    default:   return A_INVALID;
    }
}

static a_op decode_cop1(uint32_t w, uint32_t addr, a_insn *in) {
    switch (RS_F(w)) {
    case 0x00: return A_MFC1;
    case 0x02: return A_CFC1;
    case 0x04: return A_MTC1;
    case 0x06: return A_CTC1;
    case 0x08:                                  /* BC1 — branch on FP cond */
        switch (RT_F(w) & 3) {
        case 0: set_branch(in, addr, 0); return A_BC1F;
        case 1: set_branch(in, addr, 0); return A_BC1T;
        case 2: set_branch(in, addr, 1); return A_BC1FL;
        default:set_branch(in, addr, 1); return A_BC1TL;
        }
    case 0x10:                                  /* .s format */
        if ((FUNCT(w) & 0x30) == 0x30) {        /* c.cond.s */
            in->fcond = FUNCT(w) & 0xF;
            return A_C_COND_S;
        }
        switch (FUNCT(w)) {
        case 0x00: return A_ADD_S;
        case 0x01: return A_SUB_S;
        case 0x02: return A_MUL_S;
        case 0x03: return A_DIV_S;
        case 0x04: return A_SQRT_S;
        case 0x05: return A_ABS_S;
        case 0x06: return A_MOV_S;
        case 0x07: return A_NEG_S;
        case 0x0C: return A_ROUND_W_S;
        case 0x0D: return A_TRUNC_W_S;
        case 0x0E: return A_CEIL_W_S;
        case 0x0F: return A_FLOOR_W_S;
        case 0x24: return A_CVT_W_S;
        default:   return A_INVALID;
        }
    case 0x14:                                  /* .w format */
        return (FUNCT(w) == 0x20) ? A_CVT_S_W : A_INVALID;
    default:   return A_INVALID;
    }
}

/* COP2 on Allegrex is the VFPU control space: scalar moves between the integer
 * file and the vector file, plus branches on the VFPU condition codes. */
static a_op decode_cop2(uint32_t w, uint32_t addr, a_insn *in) {
    /* Sub-opcode is the rs field. An earlier revision had mfv at 0 and mtv at
     * 4, which are mfc2 and mtc2 — the *integer* moves. The vector moves are 3
     * and 7. Confirmed against this module: the only values that occur are 3,
     * 7 and 8 (mfv, mtv, and the branch group), which is exactly what a game
     * shuttling values between the integer and vector files would use. */
    switch (RS_F(w)) {
    case 0x00: return A_MFC1;    /* mfc2: integer move, no vector register */
    case 0x02: return A_CFC1;    /* cfc2 */
    case 0x03: return A_MFV;
    case 0x04: return A_MTC1;    /* mtc2 */
    case 0x06: return A_CTC1;    /* ctc2 */
    case 0x07: return A_MTV;
    case 0x08:
        switch (RT_F(w) & 3) {
        case 0: set_branch(in, addr, 0); return A_BVF;
        case 1: set_branch(in, addr, 0); return A_BVT;
        case 2: set_branch(in, addr, 1); return A_BVFL;
        default:set_branch(in, addr, 1); return A_BVTL;
        }
    default: return A_VFPU_UNKNOWN;
    }
}

int a_decode(uint32_t word, uint32_t addr, a_insn *out) {
    a_insn in;
    memset(&in, 0, sizeof in);
    in.raw  = word;
    in.addr = addr;

    in.rs = (uint8_t)RS_F(word);
    in.rt = (uint8_t)RT_F(word);
    in.rd = (uint8_t)RD_F(word);
    in.sa = (uint8_t)SA_F(word);
    in.fs = (uint8_t)RD_F(word);   /* COP1 fs sits where rd does */
    in.ft = (uint8_t)RT_F(word);
    in.fd = (uint8_t)SA_F(word);
    in.vd = (uint8_t)VD_F(word);
    in.vs = (uint8_t)VS_F(word);
    in.vt = (uint8_t)VT_F(word);
    in.vsize = (uint8_t)VSIZE(word);

    const uint32_t opc = OPCODE(word);
    a_op op;

    switch (opc) {
    case 0x00: op = decode_special (word, addr, &in); break;
    case 0x01: op = decode_regimm  (word, addr, &in); break;

    case 0x02: op = A_J;   set_jump(&in, addr); break;
    case 0x03: op = A_JAL; set_jump(&in, addr); in.is_call = 1; in.ends_block = 0; break;

    case 0x04: op = A_BEQ;  set_branch(&in, addr, 0); break;
    case 0x05: op = A_BNE;  set_branch(&in, addr, 0); break;
    case 0x06: op = A_BLEZ; set_branch(&in, addr, 0); break;
    case 0x07: op = A_BGTZ; set_branch(&in, addr, 0); break;
    case 0x14: op = A_BEQL; set_branch(&in, addr, 1); break;
    case 0x15: op = A_BNEL; set_branch(&in, addr, 1); break;
    case 0x16: op = A_BLEZL;set_branch(&in, addr, 1); break;
    case 0x17: op = A_BGTZL;set_branch(&in, addr, 1); break;

    case 0x08: op = A_ADDI;  break;
    case 0x09: op = A_ADDIU; break;
    case 0x0A: op = A_SLTI;  break;
    case 0x0B: op = A_SLTIU; break;
    case 0x0C: op = A_ANDI;  break;
    case 0x0D: op = A_ORI;   break;
    case 0x0E: op = A_XORI;  break;
    case 0x0F: op = A_LUI;   break;

    case 0x10: op = decode_cop0(word);             break;
    case 0x11: op = decode_cop1(word, addr, &in);  break;
    case 0x12: op = decode_cop2(word, addr, &in);  break;
    case 0x1F: op = decode_special3(word);         break;

    case 0x20: op = A_LB;  break;
    case 0x21: op = A_LH;  break;
    case 0x22: op = A_LWL; break;
    case 0x23: op = A_LW;  break;
    case 0x24: op = A_LBU; break;
    case 0x25: op = A_LHU; break;
    case 0x26: op = A_LWR; break;
    case 0x28: op = A_SB;  break;
    case 0x29: op = A_SH;  break;
    case 0x2A: op = A_SWL; break;
    case 0x2B: op = A_SW;  break;
    case 0x2E: op = A_SWR; break;
    case 0x2F: op = A_CACHE; break;
    case 0x30: op = A_LL;  break;
    case 0x31: op = A_LWC1; break;
    case 0x33: op = A_PREF; break;
    case 0x38: op = A_SC;  break;
    case 0x39: op = A_SWC1; break;

    /* VFPU load/store — these use the 7-bit vt field and a 16-byte-aligned
     * offset for the quad forms. */
    case 0x32: op = A_LV_S; break;
    case 0x36: op = A_LV_Q; break;
    case 0x3A: op = A_SV_S; break;
    case 0x3E: op = A_SV_Q; break;

    /* VFPU arithmetic families. Sub-opcode lives in bits 25..23. */
    case 0x18:
        switch ((word >> 23) & 7) {
        case 0: op = A_VADD; break;
        case 1: op = A_VSUB; break;
        /* vdiv is sub-opcode 4, not 7. An earlier revision had it at 7, which
         * both mis-decoded two real instructions and left actual vdiv falling
         * through as unknown. Corrected against the published encoding. */
        case 4: op = A_VDIV; break;
        default: op = A_VFPU_UNKNOWN; break;
        }
        break;

    /* VFPU5: the operand-prefix instructions, plus the immediate loads.
     * Sub-opcode is bits 25..23; each prefix occupies two values. */
    case 0x37:
        switch ((word >> 23) & 7) {
        case 0: case 1: op = A_VPFXS; break;
        case 2: case 3: op = A_VPFXT; break;
        case 4: case 5: op = A_VPFXD; break;
        /* Immediate loads. Bit 23 picks the format: viim takes a signed 16-bit
         * integer, vfim a half-precision float. Both write a single lane. */
        case 6: op = A_VIIM; break;
        case 7: op = A_VFIM; break;
        default: op = A_VFPU_UNKNOWN; break;
        }
        out->vd = (word >> 16) & 0x7F;
        out->imm = (int32_t)(int16_t)(word & 0xFFFF);
        out->vsize = 1;
        break;
    case 0x19:
        switch ((word >> 23) & 7) {
        case 0: op = A_VMUL; break;
        case 1: op = A_VDOT; break;
        case 2: op = A_VSCL; break;
        case 4: op = A_VHDP; break;
        case 5: op = A_VCRS; break;
        case 6: op = A_VDET; break;
        default: op = A_VFPU_UNKNOWN; break;
        }
        break;
    case 0x1B:
        switch ((word >> 23) & 7) {
        case 0: op = A_VCMP; break;
        case 2: op = A_VMIN; break;
        case 3: op = A_VMAX; break;
        case 5: op = A_VSCMP; break;
        case 6: op = A_VSGE; break;
        case 7: op = A_VSLT; break;
        default: op = A_VFPU_UNKNOWN; break;
        }
        break;

    /* The rest of the VFPU encoding space. We recognise it *as* VFPU so the
     * emitter refuses loudly rather than emitting silently-wrong code, and so
     * coverage reports tell us how much of a game actually needs it. */
    /* The rest of the VFPU encoding space. 0x34 (VFPU4: vmov/vabs/vrcp/vsin/
     * vcos/vf2i/vi2f/vcmov and friends) and 0x3C (VFPU6: vmmul, vtfm2/3/4,
     * vmscl, vrot) are mapped but not yet implemented — see docs/VFPU.md.
     * Recognised as VFPU so they refuse loudly instead of decoding as
     * something else. */
    /* VFPU4 group. Two levels: rs (25..21) picks the sub-table, rt (20..16)
     * picks the operation within it. Confirmed against a real module -- every
     * rs value observed there falls inside this map with nothing left over,
     * which is what distinguishes the right field from a plausible one. */
    case 0x34:
        switch (RS_F(word)) {
        case 0x00:                                  /* VFPU4: unary ops */
            switch (RT_F(word)) {
            case 0x00: op = A_VMOV;   break;
            case 0x01: op = A_VABS;   break;
            case 0x02: op = A_VNEG;   break;
            case 0x03: op = A_VIDT;   break;
            case 0x04: op = A_VSAT0;  break;
            case 0x05: op = A_VSAT1;  break;
            case 0x06: op = A_VZERO;  break;
            case 0x07: op = A_VONE;   break;
            case 0x10: op = A_VRCP;   break;
            case 0x11: op = A_VRSQ;   break;
            case 0x12: op = A_VSIN;   break;
            case 0x13: op = A_VCOS;   break;
            case 0x14: op = A_VEXP2;  break;
            case 0x15: op = A_VLOG2;  break;
            case 0x16: op = A_VSQRT;  break;
            case 0x17: op = A_VASIN;  break;
            case 0x18: op = A_VNRCP;  break;
            case 0x1A: op = A_VNSIN;  break;
            case 0x1C: op = A_VREXP2; break;
            default:   op = A_VFPU_UNKNOWN; break;
            }
            break;
        case 0x03: op = A_VCST;  break;
        case 0x10: op = A_VF2IN; break;
        case 0x11: op = A_VF2IZ; break;
        case 0x12: op = A_VF2IU; break;
        case 0x13: op = A_VF2ID; break;
        case 0x14: op = A_VI2F;  break;
        case 0x15: op = A_VCMOV; break;
        /* VFPU7 (rs=1) and VFPU9 (rs=2) are the conversion and shuffle
         * tables; mapped but not broken out yet. */
        default:   op = A_VFPU_UNKNOWN; break;
        }
        break;

    /* VFPU6: the matrix unit. rs selects, and the multiply/transform entries
     * occupy four consecutive values each (the low two bits carry part of the
     * operand encoding). */
    case 0x3C:
        switch (RS_F(word) & 0x1C) {
        case 0x00: op = A_VMMUL; break;
        case 0x04: op = A_VTFM2; break;
        case 0x08: op = A_VTFM3; break;
        case 0x0C: op = A_VTFM4; break;
        case 0x10: op = A_VMSCL; break;
        case 0x14: op = A_VCRSP; break;
        case 0x1C:
            if (RS_F(word) == 0x1D) { op = A_VROT; break; }
            switch ((word >> 16) & 0xF) {   /* VFPUMatrix1 */
            case 0x0: op = A_VMMOV;  break;
            case 0x3: op = A_VMIDT;  break;
            case 0x6: op = A_VMZERO; break;
            case 0x7: op = A_VMONE;  break;
            default:  op = A_VFPU_UNKNOWN; break;
            }
            break;
        default: op = A_VFPU_UNKNOWN; break;
        }
        break;

    case 0x1C: case 0x1D: case 0x1E:
    case 0x35: case 0x3D: case 0x3F:
        op = A_VFPU_UNKNOWN;
        break;

    default:
        op = A_INVALID;
        break;
    }

    in.op  = op;
    in.fmt = (op > A_INVALID && op < A_OP_COUNT) ? OPINFO[op].fmt : F_UNKNOWN;

    /* Immediate: signed for the arithmetic forms and every memory offset,
     * zero-extended for the logical forms. */
    switch (op) {
    case A_ANDI: case A_ORI: case A_XORI: case A_LUI:
        in.imm = (int32_t)IMM16(word);
        break;
    default:
        in.imm = SIMM16(word);
        break;
    }

    *out = in;
    return op != A_INVALID;
}

/* ---- formatting ---------------------------------------------------------- */

static const char *const FP_CONDS[16] = {
    "f","un","eq","ueq","olt","ult","ole","ule",
    "sf","ngle","seq","ngl","lt","nge","le","ngt"
};

int a_format(const a_insn *in, char *buf, int buflen) {
    char mnem[16];
    const char *name = a_mnemonic(in->op);

    if (in->op == A_C_COND_S) {
        snprintf(mnem, sizeof mnem, "c.%s.s", FP_CONDS[in->fcond & 15]);
        name = mnem;
    }

    switch (in->fmt) {
    case F_NONE:
        return snprintf(buf, buflen, "%s", name);
    case F_RD_RS_RT:
        return snprintf(buf, buflen, "%-10s %s, %s, %s", name,
                        a_reg_name(in->rd), a_reg_name(in->rs), a_reg_name(in->rt));
    case F_RT_RS_IMM:
        return snprintf(buf, buflen, "%-10s %s, %s, %d", name,
                        a_reg_name(in->rt), a_reg_name(in->rs), in->imm);
    case F_RT_RS_UIMM:
        return snprintf(buf, buflen, "%-10s %s, %s, 0x%X", name,
                        a_reg_name(in->rt), a_reg_name(in->rs), (unsigned)in->imm);
    case F_RD_RT_SA:
        return snprintf(buf, buflen, "%-10s %s, %s, %u", name,
                        a_reg_name(in->rd), a_reg_name(in->rt), in->sa);
    case F_RD_RT_RS:
        return snprintf(buf, buflen, "%-10s %s, %s, %s", name,
                        a_reg_name(in->rd), a_reg_name(in->rt), a_reg_name(in->rs));
    case F_RS_RT_OFF:
        return snprintf(buf, buflen, "%-10s %s, %s, 0x%08X", name,
                        a_reg_name(in->rs), a_reg_name(in->rt), in->target);
    case F_RS_OFF:
        return snprintf(buf, buflen, "%-10s %s, 0x%08X", name,
                        a_reg_name(in->rs), in->target);
    case F_TARGET:
        return snprintf(buf, buflen, "%-10s 0x%08X", name, in->target);
    case F_RS:
        return snprintf(buf, buflen, "%-10s %s", name, a_reg_name(in->rs));
    case F_RD_RS:
        return snprintf(buf, buflen, "%-10s %s, %s", name,
                        a_reg_name(in->rd), a_reg_name(in->rs));
    case F_RD_RT:
        return snprintf(buf, buflen, "%-10s %s, %s", name,
                        a_reg_name(in->rd), a_reg_name(in->rt));
    case F_RD:
        return snprintf(buf, buflen, "%-10s %s", name, a_reg_name(in->rd));
    case F_RS_RT:
        return snprintf(buf, buflen, "%-10s %s, %s", name,
                        a_reg_name(in->rs), a_reg_name(in->rt));
    case F_RT_IMM:
        return snprintf(buf, buflen, "%-10s %s, 0x%X", name,
                        a_reg_name(in->rt), (unsigned)in->imm);
    case F_RT_OFF_BASE:
        return snprintf(buf, buflen, "%-10s %s, %d(%s)", name,
                        a_reg_name(in->rt), in->imm, a_reg_name(in->rs));
    case F_FT_OFF_BASE:
        return snprintf(buf, buflen, "%-10s %s, %d(%s)", name,
                        a_freg_name(in->ft), in->imm, a_reg_name(in->rs));
    case F_RT_FS:
        return snprintf(buf, buflen, "%-10s %s, $%u", name,
                        a_reg_name(in->rt), in->fs);
    case F_FD_FS_FT:
        return snprintf(buf, buflen, "%-10s %s, %s, %s", name,
                        a_freg_name(in->fd), a_freg_name(in->fs), a_freg_name(in->ft));
    case F_FD_FS:
        return snprintf(buf, buflen, "%-10s %s, %s", name,
                        a_freg_name(in->fd), a_freg_name(in->fs));
    case F_FS_FT:
        return snprintf(buf, buflen, "%-10s %s, %s", name,
                        a_freg_name(in->fs), a_freg_name(in->ft));
    case F_OFF:
        return snprintf(buf, buflen, "%-10s 0x%08X", name, in->target);
    case F_CODE:
        return snprintf(buf, buflen, "%-10s 0x%05X", name, (in->raw >> 6) & 0xFFFFF);
    case F_RT_RS_POS_SZ: {
        unsigned pos = in->sa;
        unsigned msb = in->rd;
        unsigned size = (in->op == A_EXT) ? msb + 1 : msb - pos + 1;
        return snprintf(buf, buflen, "%-10s %s, %s, %u, %u", name,
                        a_reg_name(in->rt), a_reg_name(in->rs), pos, size);
    }
    case F_VD_VS_VT:
        return snprintf(buf, buflen, "%-10s v%u, v%u, v%u   ; %u lanes", name,
                        in->vd, in->vs, in->vt, in->vsize);
    case F_VD_VS:
        return snprintf(buf, buflen, "%-10s v%u, v%u   ; %u lanes", name,
                        in->vd, in->vs, in->vsize);
    case F_VT_OFF_BASE:
        return snprintf(buf, buflen, "%-10s v%u, %d(%s)", name,
                        in->vt, in->imm & ~3, a_reg_name(in->rs));
    case F_RT_VD:
        return snprintf(buf, buflen, "%-10s %s, v%u", name,
                        a_reg_name(in->rt), in->vd);
    case F_UNKNOWN:
    default:
        return snprintf(buf, buflen, ".word     0x%08X", in->raw);
    }
}
