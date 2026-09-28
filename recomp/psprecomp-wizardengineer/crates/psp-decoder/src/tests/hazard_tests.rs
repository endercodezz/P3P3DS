//! Delay-slot hazard fusion tests (issue #12).
//!
//! When the delay-slot instruction WRITES a register the branch/jump READS,
//! the plain delay-slot-before-branch swap evaluates the condition on the
//! clobbered value. The decoder must instead fuse the pair into
//! `MipsOp::BranchHazardDelay` (padded with a `Nop` to preserve the positional
//! word-to-IR mapping) so the emitter can snapshot the pre-delay reads.
use crate::decode_function;
use psp_ir::{MipsOp, Reg};

/// Build a little-endian byte sequence from a slice of u32 words.
fn words_to_bytes(words: &[u32]) -> Vec<u8> {
    words.iter().flat_map(|w| w.to_le_bytes()).collect()
}

// beq $t0, $at, +12  (opcode=0x04, rs=8, rt=1, offset=3 words)
// — the 0x089D73E0 shape from the gzip-inflate hot path.
const BEQ_T0_AT: u32 = (0x04u32 << 26) | (8 << 21) | (1 << 16) | 3;
// addiu $t0, $v0, 3  (opcode=0x09, rs=2, rt=8) — writes $t0, which beq reads.
const ADDIU_T0_V0_3: u32 = (0x09u32 << 26) | (2 << 21) | (8 << 16) | 3;
// bne $t1, $zero, +12 (bnez $t1) — the 0x089D74D0 shape.
const BNEZ_T1: u32 = (0x05u32 << 26) | (9 << 21) | 3;
// nor $t1, $t0, $zero (SPECIAL func=0x27, rs=8, rt=0, rd=9) — writes $t1.
const NOR_T1_T0: u32 = (8 << 21) | (9 << 11) | 0x27;
// jr $t3 (SPECIAL func=0x08, rs=11)
const JR_T3: u32 = (11 << 21) | 0x08;
// lw $t3, 0($sp) (opcode=0x23, rs=29, rt=11) — clobbers the jump target reg.
const LW_T3_SP: u32 = (0x23u32 << 26) | (29 << 21) | (11 << 16);
// beq $a0, $a1, +12 with addu $t0, $t1, $t2 delay — NO hazard (writes $t0,
// branch reads $a0/$a1).
const BEQ_A0_A1: u32 = (0x04u32 << 26) | (4 << 21) | (5 << 16) | 3;
const ADDU_T0: u32 = (9 << 21) | (10 << 16) | (8 << 11) | 0x21;
const NOP: u32 = 0;

#[test]
fn beq_with_delay_writing_compared_reg_is_fused() {
    // 0x089D73E0 shape: `beq $t0,$at` / delay `addiu $t0,$v0,3`.
    let bytes = words_to_bytes(&[BEQ_T0_AT, ADDIU_T0_V0_3]);
    let ops = decode_function(&bytes, 0x08800000, &[]).unwrap();

    assert_eq!(ops.len(), 2, "fused node + Nop pad must preserve word count");
    match &ops[0] {
        MipsOp::BranchHazardDelay { branch, delay } => {
            assert!(
                matches!(**branch, MipsOp::Beq { rs: Reg::Gpr(8), rt: Reg::Gpr(1), .. }),
                "fused branch must be the beq: got {branch:?}"
            );
            assert!(
                matches!(**delay, MipsOp::Addiu { rt: Reg::Gpr(8), rs: Reg::Gpr(2), imm: 3 }),
                "fused delay must be the addiu: got {delay:?}"
            );
        }
        other => panic!("expected BranchHazardDelay, got {other:?}"),
    }
    assert!(matches!(ops[1], MipsOp::Nop {}), "Nop pad expected, got {:?}", ops[1]);
}

#[test]
fn bnez_with_nor_delay_writing_condition_reg_is_fused() {
    // 0x089D74D0 shape: `bnez $t1` / delay `nor $t1,$t0,$zero`.
    let bytes = words_to_bytes(&[BNEZ_T1, NOR_T1_T0]);
    let ops = decode_function(&bytes, 0x08800000, &[]).unwrap();

    assert_eq!(ops.len(), 2);
    match &ops[0] {
        MipsOp::BranchHazardDelay { branch, delay } => {
            assert!(
                matches!(**branch, MipsOp::Bne { rs: Reg::Gpr(9), rt: Reg::Zero, .. }),
                "fused branch must be the bnez: got {branch:?}"
            );
            assert!(
                matches!(**delay, MipsOp::Nor { rd: Reg::Gpr(9), rs: Reg::Gpr(8), rt: Reg::Zero }),
                "fused delay must be the nor: got {delay:?}"
            );
        }
        other => panic!("expected BranchHazardDelay, got {other:?}"),
    }
}

#[test]
fn jr_with_delay_clobbering_target_reg_is_fused() {
    // `jr $t3` / delay `lw $t3, 0($sp)` — the dispatch target must be the
    // PRE-delay $t3.
    let bytes = words_to_bytes(&[JR_T3, LW_T3_SP]);
    let ops = decode_function(&bytes, 0x08800000, &[]).unwrap();

    assert_eq!(ops.len(), 2);
    match &ops[0] {
        MipsOp::BranchHazardDelay { branch, delay } => {
            assert!(
                matches!(**branch, MipsOp::Jr { rs: Reg::Gpr(11) }),
                "fused branch must be the jr: got {branch:?}"
            );
            assert!(
                matches!(**delay, MipsOp::Lw { rt: Reg::Gpr(11), rs: Reg::Gpr(29), offset: 0 }),
                "fused delay must be the lw: got {delay:?}"
            );
        }
        other => panic!("expected BranchHazardDelay, got {other:?}"),
    }
}

#[test]
fn non_hazard_branch_still_uses_plain_swap() {
    // Regression guard: `beq $a0,$a1` / delay `addu $t0,..` has no hazard —
    // the delay slot must still be reordered BEFORE the branch, unfused.
    let bytes = words_to_bytes(&[BEQ_A0_A1, ADDU_T0]);
    let ops = decode_function(&bytes, 0x08800000, &[]).unwrap();

    assert_eq!(ops.len(), 2);
    assert!(matches!(ops[0], MipsOp::Addu { .. }), "plain swap: delay first, got {:?}", ops[0]);
    assert!(matches!(ops[1], MipsOp::Beq { likely: false, .. }), "branch second");
    assert!(
        !ops.iter().any(|op| matches!(op, MipsOp::BranchHazardDelay { .. })),
        "non-hazard pair must NOT be fused"
    );
}

#[test]
fn jr_ra_with_delay_writing_ra_is_not_fused() {
    // `jr $ra` lowers to a plain C++ `return;` (register value never consumed)
    // and must stay a bare Jr so the coalesced LINK/RA model keeps matching it.
    let jr_ra: u32 = (31 << 21) | 0x08;
    let lw_ra_sp: u32 = (0x23u32 << 26) | (29 << 21) | (31 << 16) | 0x10;
    let bytes = words_to_bytes(&[jr_ra, lw_ra_sp]);
    let ops = decode_function(&bytes, 0x08800000, &[]).unwrap();

    assert!(
        !ops.iter().any(|op| matches!(op, MipsOp::BranchHazardDelay { .. })),
        "jr $ra must not be hazard-fused"
    );
    assert!(matches!(ops[1], MipsOp::Jr { rs: Reg::Gpr(31) }));
}

#[test]
fn hazard_fusion_preserves_positional_mapping() {
    // A hazard pair embedded in a longer stream must keep IR length == word
    // count so positional labels (L_{vaddr}) stay aligned.
    let bytes = words_to_bytes(&[NOP, BEQ_T0_AT, ADDIU_T0_V0_3, ADDU_T0, NOP]);
    let ops = decode_function(&bytes, 0x08800000, &[]).unwrap();
    assert_eq!(ops.len(), 5, "IR length must equal instruction word count");
    assert!(matches!(ops[1], MipsOp::BranchHazardDelay { .. }));
    assert!(matches!(ops[2], MipsOp::Nop {}));
    assert!(matches!(ops[3], MipsOp::Addu { .. }));
}
