//! Delay slot semantic helpers for MIPS32 branch instructions.
//!
//! Provides predicates used by `decode_function` in `lib.rs` to determine whether
//! an instruction has a delay slot and whether that slot is conditional (branch-likely).

use psp_ir::{MipsOp, Reg};

/// Returns `true` if `op` is a branch-likely instruction.
///
/// Branch-likely instructions execute the delay slot only when the branch is taken.
/// The delay slot is wrapped in `MipsOp::DelaySlot{}` and placed after the branch
/// in the output sequence.
pub fn is_branch_likely(op: &MipsOp) -> bool {
    matches!(
        op,
        MipsOp::Beq { likely: true, .. }
            | MipsOp::Bne { likely: true, .. }
            | MipsOp::Blez { likely: true, .. }
            | MipsOp::Bgtz { likely: true, .. }
            | MipsOp::Bltz { likely: true, .. }
            | MipsOp::Bgez { likely: true, .. }
            | MipsOp::Bltzal { likely: true, .. }
            | MipsOp::Bgezal { likely: true, .. }
            | MipsOp::Bc1t { likely: true, .. }
            | MipsOp::Bc1f { likely: true, .. }
            | MipsOp::VfpuBvf { likely: true, .. }
            | MipsOp::VfpuBvt { likely: true, .. }
    )
}

/// Returns `true` if `op` is a branch or jump instruction (has a delay slot).
///
/// All branch/jump instructions in MIPS32 have a delay slot — the next instruction
/// always occupies the delay slot regardless of whether the branch is taken.
pub fn is_branch_or_jump(op: &MipsOp) -> bool {
    matches!(
        op,
        MipsOp::J { .. }
            | MipsOp::Jal { .. }
            | MipsOp::Jr { .. }
            | MipsOp::Jalr { .. }
            | MipsOp::JumpTable { .. }
            | MipsOp::Beq { .. }
            | MipsOp::Bne { .. }
            | MipsOp::Blez { .. }
            | MipsOp::Bgtz { .. }
            | MipsOp::Bltz { .. }
            | MipsOp::Bgez { .. }
            | MipsOp::Bltzal { .. }
            | MipsOp::Bgezal { .. }
            | MipsOp::Bc1t { .. }
            | MipsOp::Bc1f { .. }
            | MipsOp::VfpuBvf { .. }
            | MipsOp::VfpuBvt { .. }
    )
}

/// Returns the GPRs a branch/jump instruction READS to evaluate its condition
/// or capture its target.
///
/// Used for delay-slot hazard detection: real MIPS evaluates the condition /
/// captures the jump target BEFORE the delay slot executes. `jr $ra` is
/// intentionally excluded — it lowers to a plain C++ `return;` (the emitter
/// never consumes the register VALUE), and excluding it keeps the coalesced
/// LINK/RA model's `Jr { rs: $ra }` pattern matching intact.
pub fn branch_gpr_reads(op: &MipsOp) -> Vec<Reg> {
    match op {
        MipsOp::Beq { rs, rt, .. } | MipsOp::Bne { rs, rt, .. } => vec![*rs, *rt],
        MipsOp::Blez { rs, .. }
        | MipsOp::Bgtz { rs, .. }
        | MipsOp::Bltz { rs, .. }
        | MipsOp::Bgez { rs, .. }
        | MipsOp::Bltzal { rs, .. }
        | MipsOp::Bgezal { rs, .. }
        | MipsOp::Jalr { rs, .. } => vec![*rs],
        MipsOp::Jr { rs } if *rs != Reg::Gpr(31) => vec![*rs],
        MipsOp::JumpTable { index_reg, .. } => vec![*index_reg],
        _ => Vec::new(),
    }
}

/// Returns the GPR an instruction WRITES (its architectural destination).
///
/// `Reg::Zero` destinations return `None` — writes to `$zero` are no-ops, so
/// they can never clobber a value a branch reads. Conditional writers
/// (`movz`/`movn`) are treated as writers (conservative: fusing a non-hazard
/// is correct, missing a hazard is not).
pub fn gpr_write_of(op: &MipsOp) -> Option<Reg> {
    use MipsOp::*;
    let dest = match op {
        // rd-destination R-type / Allegrex ops
        Sll { rd, .. } | Srl { rd, .. } | Sra { rd, .. } | Sllv { rd, .. }
        | Srlv { rd, .. } | Srav { rd, .. } | Rotr { rd, .. } | Rotrv { rd, .. }
        | Mfhi { rd } | Mflo { rd } | Add { rd, .. } | Addu { rd, .. }
        | Sub { rd, .. } | Subu { rd, .. } | And { rd, .. } | Or { rd, .. }
        | Xor { rd, .. } | Nor { rd, .. } | Slt { rd, .. } | Sltu { rd, .. }
        | Movz { rd, .. } | Movn { rd, .. } | Mul { rd, .. } | Daddu { rd, .. }
        | Dadd { rd, .. } | Dsub { rd, .. } | Dsrlv { rd, .. } | Min { rd, .. }
        | Max { rd, .. } | Seb { rd, .. } | Seh { rd, .. } | Bitrev { rd, .. }
        | Wsbh { rd, .. } | Wsbw { rd, .. } | Clz { rd, .. } | Clo { rd, .. } => *rd,
        // rt-destination I-type ops (ALU-immediate, loads, bitfield, moves-from)
        Addi { rt, .. } | Addiu { rt, .. } | Slti { rt, .. } | Sltiu { rt, .. }
        | Andi { rt, .. } | Ori { rt, .. } | Xori { rt, .. } | Lui { rt, .. }
        | Lb { rt, .. } | Lh { rt, .. } | Lwl { rt, .. } | Lw { rt, .. }
        | Lbu { rt, .. } | Lhu { rt, .. } | Lwr { rt, .. } | Ext { rt, .. }
        | Ins { rt, .. } | Mfc0 { rt, .. } | Mfc1 { rt, .. } | VfpuMfv { rt, .. }
        | VfpuMfvc { rt, .. } | RelocHi16 { rt, .. } | RelocLo16 { rt, .. } => *rt,
        // Link writers (cannot legally sit in a delay slot, but be safe)
        Jalr { rd, .. } => *rd,
        Jal { .. } | Bltzal { .. } | Bgezal { .. } => Reg::Gpr(31),
        _ => return None,
    };
    if dest == Reg::Zero { None } else { Some(dest) }
}

/// Returns `true` when a NON-likely branch/jump and its delay-slot instruction
/// have a read-after-write hazard.
///
/// A hazard exists when the delay slot WRITES a register (or condition flag)
/// that the control instruction READS. The plain delay-slot-before-branch
/// reorder would evaluate the condition on the clobbered value; real MIPS
/// evaluates it on the pre-delay value. Confirmed miscompile sites: 0x089D73E0
/// (`beq $t0,$at` / delay `addiu $t0,$v0,3`) and 0x089D74D0 (`bnez $t1` /
/// delay `nor $t1,$t0,$zero`) on the gzip-inflate hot path (issue #12).
pub fn has_delay_slot_hazard(branch: &MipsOp, delay: &MipsOp) -> bool {
    if let Some(w) = gpr_write_of(delay) {
        if branch_gpr_reads(branch).contains(&w) {
            return true;
        }
    }
    // FPU condition flag: bc1t/bc1f read fpu_cc; c.cond.s writes it.
    if matches!(branch, MipsOp::Bc1t { .. } | MipsOp::Bc1f { .. })
        && matches!(delay, MipsOp::CCond { .. })
    {
        return true;
    }
    // VFPU condition flags: bvf/bvt read VFPU_CC; vcmp writes it.
    if matches!(branch, MipsOp::VfpuBvf { .. } | MipsOp::VfpuBvt { .. })
        && matches!(delay, MipsOp::VfpuCmp { .. })
    {
        return true;
    }
    false
}

#[cfg(test)]
mod tests {
    use super::*;
    use psp_ir::Reg;
    use crate::decode_function;

    /// Build a little-endian byte sequence from a slice of u32 words.
    fn words_to_bytes(words: &[u32]) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(words.len() * 4);
        for &w in words {
            bytes.extend_from_slice(&w.to_le_bytes());
        }
        bytes
    }

    // beq $a0, $a1, offset=8  (opcode=0x04, rs=4, rt=5, offset=2 words → 8 bytes)
    const BEQ_WORD: u32 = (0x04u32 << 26) | (4 << 21) | (5 << 16) | 2;
    // nop (all zeros)
    const NOP_WORD: u32 = 0x00000000;
    // addu $t0, $a0, $a1 (func=0x21, rs=4, rt=5, rd=8)
    const ADDU_WORD: u32 = (4 << 21) | (5 << 16) | (8 << 11) | 0x21;
    // beql $a0, $a1, offset=8 (branch-likely, opcode=0x14)
    const BEQL_WORD: u32 = (0x14u32 << 26) | (4 << 21) | (5 << 16) | 2;

    #[test]
    fn test_standard_delay_slot_order() {
        // Standard branch: beq + nop
        // Expected output: [nop, beq] — delay slot (nop) comes BEFORE the branch
        let bytes = words_to_bytes(&[BEQ_WORD, NOP_WORD]);
        let ops = decode_function(&bytes, 0x08800000, &[]).unwrap();

        assert_eq!(ops.len(), 2, "Expected 2 ops, got {}", ops.len());
        assert!(matches!(ops[0], MipsOp::Nop {}),
            "Expected Nop first (delay slot), got {:?}", ops[0]);
        assert!(matches!(ops[1], MipsOp::Beq { .. }),
            "Expected Beq second, got {:?}", ops[1]);
    }

    #[test]
    fn test_likely_delay_slot_wrapped() {
        // Branch-likely: beql + addu
        // Expected output: [beql, DelaySlot{addu}] — delay slot AFTER branch, wrapped
        let bytes = words_to_bytes(&[BEQL_WORD, ADDU_WORD]);
        let ops = decode_function(&bytes, 0x08800000, &[]).unwrap();

        assert_eq!(ops.len(), 2, "Expected 2 ops, got {}", ops.len());
        assert!(matches!(ops[0], MipsOp::Beq { likely: true, .. }),
            "Expected Beq(likely=true) first, got {:?}", ops[0]);
        match &ops[1] {
            MipsOp::DelaySlot { instr } => {
                assert!(matches!(**instr, MipsOp::Addu { .. }),
                    "Expected Addu inside DelaySlot, got {:?}", instr);
            }
            _ => panic!("Expected DelaySlot second, got {:?}", ops[1]),
        }
    }

    #[test]
    fn test_jr_ra_not_jump_table() {
        // jr $ra (rs=31) should stay as Jr{}, not JumpTable
        // jr $ra encoding: opcode=0x00, func=0x08, rs=31
        let jr_ra_word: u32 = (31 << 21) | 0x08;
        let nop_word: u32 = 0x00000000;
        let bytes = words_to_bytes(&[jr_ra_word, nop_word]);
        // Provide some xrefs (should be ignored for $ra)
        let xrefs = [(0x08800010u32, 0x08800020u32)];
        let ops = decode_function(&bytes, 0x08800000, &xrefs).unwrap();

        // After delay slot swap: [nop, Jr]
        assert!(ops.iter().any(|op| matches!(op, MipsOp::Jr { rs } if *rs == Reg::Gpr(31))),
            "Expected Jr{{rs: Gpr(31)}}, got {:?}", ops);
        assert!(!ops.iter().any(|op| matches!(op, MipsOp::JumpTable { .. })),
            "jr $ra should NOT become JumpTable");
    }
}
