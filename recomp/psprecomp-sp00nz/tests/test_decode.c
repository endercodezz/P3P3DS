/* Decoder tests. Every encoding here was written by hand from the MIPS32r2
 * manual and the Allegrex extensions — no game data, no ROM, no disc image.
 *
 * These are the instructions that make up the overwhelming majority of any
 * real PSP module (the prologue/epilogue pattern, the branch forms), plus one
 * case for each thing the decoder could plausibly get wrong: the Allegrex
 * opcodes that sit in different slots than stock MIPS32, the branch/jump
 * target arithmetic, and the ops whose operand order is not rd/rs/rt.
 */

#include "decode.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond, ...)                                       \
    do {                                                       \
        if (!(cond)) {                                         \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);        \
            printf(__VA_ARGS__);                               \
            printf("\n");                                      \
            failures++;                                        \
        }                                                      \
    } while (0)

/* Decode one word at `addr` and assert the opcode. Returns the decoded insn
 * so individual tests can go on to check fields. */
static a_insn dec(uint32_t word, uint32_t addr, a_op expect, const char *what) {
    a_insn in;
    a_decode(word, addr, &in);
    CHECK(in.op == expect, "%s: 0x%08X decoded as %s, expected %s",
          what, word, a_mnemonic(in.op), a_mnemonic(expect));
    return in;
}

static void test_core_integer(void) {
    /* The function prologue/epilogue every MIPS compiler emits. If these four
     * are wrong nothing else matters. */
    a_insn i;

    i = dec(0x27BDFFE0, 0x08900000, A_ADDIU, "addiu $sp,$sp,-32");
    CHECK(i.rt == 29 && i.rs == 29, "addiu registers");
    CHECK(i.imm == -32, "addiu immediate sign-extended: got %d", i.imm);

    i = dec(0xAFBF001C, 0x08900004, A_SW, "sw $ra,28($sp)");
    CHECK(i.rt == 31 && i.rs == 29 && i.imm == 28, "sw operands");

    i = dec(0x8FBF001C, 0x08900008, A_LW, "lw $ra,28($sp)");
    CHECK(i.rt == 31 && i.rs == 29 && i.imm == 28, "lw operands");

    i = dec(0x03E00008, 0x0890000C, A_JR, "jr $ra");
    CHECK(i.rs == 31, "jr register");
    CHECK(i.is_return, "jr $ra must be flagged as a return");
    CHECK(i.ends_block, "jr ends the block");
    CHECK(i.has_delay_slot, "jr has a delay slot");

    /* jr through any other register is an indirect jump, NOT a return — the
     * analyzer relies on this distinction to avoid closing a function early. */
    i = dec(0x00400008, 0x08900010, A_JR, "jr $v0");
    CHECK(!i.is_return, "jr $v0 must not be flagged as a return");
    CHECK(i.is_indirect, "jr $v0 is indirect");

    dec(0x00000000, 0x08900000, A_NOP, "nop");
    i = dec(0x00851021, 0x08900000, A_ADDU, "addu $v0,$a0,$a1");
    CHECK(i.rd == 2 && i.rs == 4 && i.rt == 5, "addu operands");

    i = dec(0x3C018880, 0x08900000, A_LUI, "lui $at,0x8880");
    CHECK((uint32_t)i.imm == 0x8880, "lui immediate must be zero-extended: got 0x%X",
          (unsigned)i.imm);

    /* andi/ori/xori zero-extend; addi/slti sign-extend. Getting this backwards
     * silently corrupts constants, so both directions are pinned. */
    i = dec(0x3084FFFF, 0x08900000, A_ANDI, "andi $a0,$a0,0xFFFF");
    CHECK((uint32_t)i.imm == 0xFFFF, "andi immediate zero-extended: got 0x%X",
          (unsigned)i.imm);
    i = dec(0x2084FFFF, 0x08900000, A_ADDI, "addi $a0,$a0,-1");
    CHECK(i.imm == -1, "addi immediate sign-extended: got %d", i.imm);
}

static void test_branch_targets(void) {
    /* beq $a0,$a1,+2 words => target is addr + 4 + 8. */
    a_insn i = dec(0x10850002, 0x08900000, A_BEQ, "beq $a0,$a1,+2");
    CHECK(i.has_target && i.target == 0x0890000C,
          "beq target: got 0x%08X, expected 0x0890000C", i.target);
    CHECK(i.is_branch && !i.is_jump, "beq is a conditional branch");
    CHECK(!i.is_likely, "beq is not a likely branch");

    /* A negative displacement — the backward branch that closes a loop. */
    i = dec(0x1085FFFE, 0x08900010, A_BEQ, "beq $a0,$a1,-2");
    CHECK(i.target == 0x0890000C,
          "backward beq target: got 0x%08X, expected 0x0890000C", i.target);

    /* Likely branches nullify their delay slot when not taken; the emitter
     * must know which is which. */
    i = dec(0x50850002, 0x08900000, A_BEQL, "beql");
    CHECK(i.is_likely, "beql must be flagged likely");

    /* j/jal take the top 4 bits from the DELAY SLOT's address, not their own.
     * The difference only shows up at a 256 MB boundary, which is exactly
     * where it would be missed. */
    i = dec(0x0E240040, 0x08900000, A_JAL, "jal 0x08900100");
    CHECK(i.target == 0x08900100, "jal target: got 0x%08X", i.target);
    CHECK(i.is_call, "jal is a call");
    CHECK(!i.ends_block, "jal returns, so it does not end the block");

    i = dec(0x0A240040, 0x08900000, A_J, "j 0x08900100");
    CHECK(i.ends_block, "j ends the block");
    CHECK(!i.is_call, "j is not a call");

    /* jalr writes $ra and is both indirect and a call. */
    i = dec(0x0040F809, 0x08900000, A_JALR, "jalr $v0");
    CHECK(i.is_call && i.is_indirect, "jalr is an indirect call");

    /* REGIMM: bltz vs bgez differ only in the rt field. */
    dec(0x04800002, 0x08900000, A_BLTZ, "bltz $a0");
    dec(0x04810002, 0x08900000, A_BGEZ, "bgez $a0");
    i = dec(0x04910002, 0x08900000, A_BGEZAL, "bgezal $a0");
    CHECK(i.is_call, "bgezal is a call");
}

static void test_allegrex_extensions(void) {
    /* These are the opcodes where Allegrex differs from stock MIPS32. A
     * decoder written against the generic manual gets every one of them wrong,
     * so each is pinned explicitly. */
    a_insn i;

    /* CLZ/CLO live in SPECIAL (0x16/0x17), not SPECIAL2 as in MIPS32. */
    i = dec(0x00801016, 0x08900000, A_CLZ, "clz $v0,$a0");
    CHECK(i.rd == 2 && i.rs == 4, "clz operands");
    dec(0x00801017, 0x08900000, A_CLO, "clo $v0,$a0");

    /* MAX/MIN are Allegrex-only, at SPECIAL 0x2C/0x2D. */
    dec(0x0085102C, 0x08900000, A_MAX, "max $v0,$a0,$a1");
    dec(0x0085102D, 0x08900000, A_MIN, "min $v0,$a0,$a1");

    /* MADD/MSUB in SPECIAL, not SPECIAL2. */
    dec(0x0085001C, 0x08900000, A_MADD,  "madd $a0,$a1");
    dec(0x0085002E, 0x08900000, A_MSUB,  "msub $a0,$a1");

    /* SPECIAL3 BSHFL group — one funct (0x20) shared by five instructions,
     * discriminated only by the sa field. Build the word explicitly so the
     * discriminator is visible rather than baked into a magic constant. */
#define BSHFL(sa) (0x1Fu << 26 | 4u << 16 | 2u << 11 | (uint32_t)(sa) << 6 | 0x20u)
    i = dec(BSHFL(0x14), 0x08900000, A_BITREV, "bitrev $v0,$a0");
    CHECK(i.rd == 2 && i.rt == 4, "bitrev reads rt and writes rd");
    i = dec(BSHFL(0x10), 0x08900000, A_SEB, "seb $v0,$a0");
    CHECK(i.rd == 2 && i.rt == 4, "seb reads rt and writes rd");
    dec(BSHFL(0x18), 0x08900000, A_SEH,  "seh $v0,$a0");
    dec(BSHFL(0x02), 0x08900000, A_WSBH, "wsbh $v0,$a0");
    dec(BSHFL(0x03), 0x08900000, A_WSBW, "wsbw $v0,$a0");
#undef BSHFL

    /* rotr is srl with rs==1; rotrv is srlv with sa==1. Miss this and every
     * rotate silently becomes a shift — a bug that only shows up in hash and
     * checksum code, long after you stop looking. */
    dec(0x00221002, 0x08900000, A_ROTR, "rotr $v0,$v0,0 (rs=1)");
    dec(0x00021002, 0x08900000, A_SRL,  "srl  $v0,$v0,0 (rs=0)");

    /* EXT/INS field packing. */
    i = dec(0x7C82F800 | 0x00, 0x08900000, A_EXT, "ext");
    CHECK(i.op == A_EXT, "ext opcode");
    i = dec(0x7C82F800 | 0x04, 0x08900000, A_INS, "ins");
    CHECK(i.op == A_INS, "ins opcode");
}

static void test_fpu(void) {
    /* COP1 is single-precision only on Allegrex. */
    dec(0x46000000 | (2 << 6),  0x08900000, A_ADD_S, "add.s");
    dec(0x46000002 | (2 << 6),  0x08900000, A_MUL_S, "mul.s");
    dec(0x46000006 | (2 << 6),  0x08900000, A_MOV_S, "mov.s");
    dec(0x4600000D | (2 << 6),  0x08900000, A_TRUNC_W_S, "trunc.w.s");

    /* c.cond.s: funct 0x30..0x3F, condition in the low nibble. */
    a_insn i = dec(0x46000032, 0x08900000, A_C_COND_S, "c.eq.s");
    CHECK(i.fcond == 2, "c.eq.s condition code: got %u", i.fcond);
    i = dec(0x4600003C, 0x08900000, A_C_COND_S, "c.lt.s");
    CHECK(i.fcond == 12, "c.lt.s condition code: got %u", i.fcond);

    /* bc1t/bc1f are branches and must carry a target and a delay slot. */
    i = dec(0x45010002, 0x08900000, A_BC1T, "bc1t");
    CHECK(i.has_target && i.target == 0x0890000C, "bc1t target: 0x%08X", i.target);
    CHECK(i.has_delay_slot, "bc1t has a delay slot");
    dec(0x45000002, 0x08900000, A_BC1F, "bc1f");

    dec(0xC4820000, 0x08900000, A_LWC1, "lwc1");
    dec(0xE4820000, 0x08900000, A_SWC1, "swc1");
    dec(0x44020000, 0x08900000, A_MFC1, "mfc1");
    dec(0x44820000, 0x08900000, A_MTC1, "mtc1");
}

static void test_vfpu(void) {
    /* We do not decode the whole VFPU yet. What matters for correctness today
     * is that VFPU encodings are never mistaken for something else and never
     * silently fall through as A_INVALID — the emitter must be able to refuse
     * them loudly. See docs/VFPU.md. */
    a_insn in;

    /* 0xD0000000 is opcode 0x34 with rs=0, rt=0 -- VFPU4's vmov. It used to
     * assert as "unknown" here, which was only true while 0x34 was unmapped;
     * the assertion outlived the gap it described. */
    dec(0xD0000000, 0x08900000, A_VMOV, "vmov (VFPU4 rs=0 rt=0)");

    /* Something still genuinely unmapped: opcode 0x35 (VFPU6's sibling table).
     * It must be recognised as VFPU rather than falling through to an integer
     * op, so the emitter refuses it instead of emitting nonsense. */
    a_decode(0xD4000000, 0x08900000, &in);
    CHECK(in.op == A_VFPU_UNKNOWN,
          "an unmapped VFPU encoding stays VFPU-unknown (got %s)", a_mnemonic(in.op));

    /* lv.q / sv.q are ordinary opcodes and we do decode those. */
    dec(0xD8000000, 0x08900000, A_LV_Q, "lv.q");
    dec(0xF8000000, 0x08900000, A_SV_Q, "sv.q");
    dec(0xC8000000, 0x08900000, A_LV_S, "lv.s");
    dec(0xE8000000, 0x08900000, A_SV_S, "sv.s");

    /* VFPU0 family: sub-opcode in bits 25..23. */
    dec(0x60000000, 0x08900000, A_VADD, "vadd");
    dec(0x60800000, 0x08900000, A_VSUB, "vsub");
    /* vdiv is VFPU0 sub-opcode 4, not 7. This encoding previously used 7,
     * matching a wrong decoder -- the test agreed with the bug rather than
     * catching it. Both are corrected against the published encoding. */
    dec(0x62000000, 0x08900000, A_VDIV, "vdiv (VFPU0 sub-opcode 4)");
    dec(0x64000000, 0x08900000, A_VMUL, "vmul");
    dec(0x64800000, 0x08900000, A_VDOT, "vdot");

    /* Vector width is split across two non-adjacent bits. All four widths must
     * round-trip, because a quad op decoded as single writes a quarter of the
     * lanes it should. */
    a_decode(0x60000000, 0, &in); CHECK(in.vsize == 1, "single: got %u", in.vsize);
    a_decode(0x60000080, 0, &in); CHECK(in.vsize == 2, "pair:   got %u", in.vsize);
    a_decode(0x60008000, 0, &in); CHECK(in.vsize == 3, "triple: got %u", in.vsize);
    a_decode(0x60008080, 0, &in); CHECK(in.vsize == 4, "quad:   got %u", in.vsize);
}

static void test_formatting(void) {
    /* The disassembly text ends up verbatim in the generated C as a comment on
     * every line, so it is part of the output contract, not a debug aid. */
    char buf[128];
    a_insn in;

    a_decode(0x27BDFFE0, 0x08900000, &in);
    a_format(&in, buf, sizeof buf);
    CHECK(strstr(buf, "addiu") && strstr(buf, "$sp") && strstr(buf, "-32"),
          "addiu formatting: \"%s\"", buf);

    a_decode(0x8FBF001C, 0x08900000, &in);
    a_format(&in, buf, sizeof buf);
    CHECK(strstr(buf, "28($sp)"), "lw offset formatting: \"%s\"", buf);

    a_decode(0x46000032, 0x08900000, &in);
    a_format(&in, buf, sizeof buf);
    CHECK(strstr(buf, "c.eq.s"), "c.cond.s formatting: \"%s\"", buf);

    /* An unrecognised word must format as data, never as a plausible-looking
     * instruction — that is what makes a bad disassembly obvious on sight. */
    a_decode(0xFFFFFFFF, 0x08900000, &in);
    a_format(&in, buf, sizeof buf);
    CHECK(strstr(buf, ".word"), "unknown word formatting: \"%s\"", buf);
}

int main(void) {
    test_core_integer();
    test_branch_targets();
    test_allegrex_extensions();
    test_fpu();
    test_vfpu();
    test_formatting();

    if (failures) {
        printf("\n%d check(s) failed\n", failures);
        return 1;
    }
    printf("all decoder checks passed\n");
    return 0;
}
