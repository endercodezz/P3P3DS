//! Branch-into-delay-slot (BIDS) decode tests (issue #56).
//!
//! When a branch/delay pair's delay-slot ADDRESS is itself an in-function
//! branch target, the decoder must fuse the pair into `BranchHazardDelay` at
//! the branch's position and place a `DelaySlotRejoin` (duplicated delay
//! instruction) at the delay slot's own position, so back-edges that enter AT
//! the delay-slot address execute the instruction. Found via .hack//Link's SCE
//! libc VFPU memset tail (FUN_08BDF1A8 @ 0x08BDF22C), where the previous
//! `Nop` padding left the loop label empty and the count register was never
//! decremented (infinite memset).

use crate::{decode_function, DecodeError};
use psp_ir::{MipsOp, Reg};

/// Build a little-endian byte sequence from a slice of u32 words.
fn words_to_bytes(words: &[u32]) -> Vec<u8> {
    words.iter().flat_map(|w| w.to_le_bytes()).collect()
}

// The exact .hack//Link memset tail (raw words from the relocated segment):
//   0x08BDF22C: 0x10C00004  beq  a2, zero, 0x08BDF240
//   0x08BDF230: 0x24C6FFFF  addiu a2, a2, -1   <- delay slot AND bne target
//   0x08BDF234: 0xA0850000  sb   a1, 0(a0)
//   0x08BDF238: 0x14C0FFFD  bne  a2, zero, 0x08BDF230
//   0x08BDF23C: 0x24840001  addiu a0, a0, 1
//   0x08BDF240:             jr ra / nop (function exit)
const MEMSET_TAIL_BASE: u32 = 0x08BDF22C;
const MEMSET_TAIL: [u32; 7] = [
    0x10C0_0004, // beq a2, zero, +0x14
    0x24C6_FFFF, // addiu a2, a2, -1
    0xA085_0000, // sb a1, 0(a0)
    0x14C0_FFFD, // bne a2, zero, -0xC (-> 0x08BDF230)
    0x2484_0001, // addiu a0, a0, 1
    0x03E0_0008, // jr ra
    0x0000_0000, // nop
];

#[test]
fn memset_tail_bids_produces_rejoin() {
    let bytes = words_to_bytes(&MEMSET_TAIL);
    let ops = decode_function(&bytes, MEMSET_TAIL_BASE, &[]).unwrap();

    assert_eq!(ops.len(), 7, "positional word-to-IR mapping must be preserved");

    // Position 0 (0x08BDF22C): fused beq + addiu (the pair is ALSO a hazard:
    // beq reads a2, addiu writes a2).
    match &ops[0] {
        MipsOp::BranchHazardDelay { branch, delay } => {
            assert!(
                matches!(**branch, MipsOp::Beq { rs: Reg::Gpr(6), rt: Reg::Zero, target: 0x08BDF240, likely: false }),
                "fused branch must be the beq: got {branch:?}"
            );
            assert!(
                matches!(**delay, MipsOp::Addiu { rt: Reg::Gpr(6), rs: Reg::Gpr(6), imm: -1 }),
                "fused delay must be the addiu a2,-1: got {delay:?}"
            );
        }
        other => panic!("expected BranchHazardDelay at position 0, got {other:?}"),
    }

    // Position 1 (0x08BDF230 — the bne's target): the duplicated delay
    // instruction, NOT a Nop pad (the Nop is what made the loop infinite).
    match &ops[1] {
        MipsOp::DelaySlotRejoin { instr } => {
            assert!(
                matches!(**instr, MipsOp::Addiu { rt: Reg::Gpr(6), rs: Reg::Gpr(6), imm: -1 }),
                "rejoin must duplicate the addiu a2,-1: got {instr:?}"
            );
        }
        other => panic!("expected DelaySlotRejoin at position 1 (branch target), got {other:?}"),
    }

    // Position 2: the sb (loop body).
    assert!(matches!(ops[2], MipsOp::Sb { .. }), "sb expected, got {:?}", ops[2]);

    // Positions 3/4: bne/addiu-a0 pair is NOT a BIDS site (0x08BDF23C is not
    // a branch target) and not a hazard — standard swap applies.
    assert!(
        matches!(ops[3], MipsOp::Addiu { rt: Reg::Gpr(4), rs: Reg::Gpr(4), imm: 1 }),
        "standard swap: delay before branch, got {:?}",
        ops[3]
    );
    assert!(
        matches!(ops[4], MipsOp::Bne { target: 0x08BDF230, .. }),
        "bne back-edge expected, got {:?}",
        ops[4]
    );
}

#[test]
fn standard_nonhazard_bids_is_fused_with_rejoin() {
    // Non-hazard BIDS variant: the delay writes a register the branch does
    // NOT read — previously the "standard swap" placed the BRANCH at the
    // delay slot's label (jump to it executed a branch hardware never takes).
    //
    //   0x00: beq a0, a1, +0x10 (-> 0x14)
    //   0x04: addiu t0, t0, 1    <- delay slot (no hazard) AND branch target
    //   0x08: nop
    //   0x0C: bne a0, zero, -0xC (-> 0x04)
    //   0x10: nop
    //   0x14: jr ra
    //   0x18: nop
    let base = 0x0880_0000u32;
    let words = [
        (0x04u32 << 26) | (4 << 21) | (5 << 16) | 4,            // beq a0,a1,+0x14
        (0x09u32 << 26) | (8 << 21) | (8 << 16) | 1,            // addiu t0,t0,1
        0,                                                       // nop
        (0x05u32 << 26) | (4 << 21) | (0xFFFD & 0xFFFF),        // bne a0,zero,-0xC
        0,                                                       // nop
        (31 << 21) | 0x08,                                       // jr ra
        0,                                                       // nop
    ];
    let ops = decode_function(&words_to_bytes(&words), base, &[]).unwrap();

    match &ops[0] {
        MipsOp::BranchHazardDelay { branch, delay } => {
            assert!(matches!(**branch, MipsOp::Beq { .. }), "got {branch:?}");
            assert!(
                matches!(**delay, MipsOp::Addiu { rt: Reg::Gpr(8), .. }),
                "got {delay:?}"
            );
        }
        other => panic!("non-hazard BIDS must still fuse: got {other:?}"),
    }
    assert!(
        matches!(&ops[1], MipsOp::DelaySlotRejoin { instr }
            if matches!(**instr, MipsOp::Addiu { rt: Reg::Gpr(8), .. })),
        "rejoin duplicate expected at the targeted delay slot, got {:?}",
        ops[1]
    );
}

#[test]
fn branch_into_own_delay_slot_is_fused_with_rejoin() {
    // Degenerate: a branch targeting its OWN delay slot (offset 0).
    //   0x00: bne a0, zero, +4 (-> 0x04, its own delay slot)
    //   0x04: addiu t0, t0, 1
    //   0x08: jr ra
    //   0x0C: nop
    let base = 0x0880_0000u32;
    let words = [
        (0x05u32 << 26) | (4 << 21), // bne a0, zero, offset 0 -> pc+4
        (0x09u32 << 26) | (8 << 21) | (8 << 16) | 1, // addiu t0,t0,1
        (31 << 21) | 0x08,           // jr ra
        0,                           // nop
    ];
    let ops = decode_function(&words_to_bytes(&words), base, &[]).unwrap();

    assert!(
        matches!(ops[0], MipsOp::BranchHazardDelay { .. }),
        "self-targeting branch must fuse, got {:?}",
        ops[0]
    );
    assert!(
        matches!(&ops[1], MipsOp::DelaySlotRejoin { instr }
            if matches!(**instr, MipsOp::Addiu { rt: Reg::Gpr(8), .. })),
        "own delay slot needs the duplicate (taken path executes it twice, \
         exactly like hardware), got {:?}",
        ops[1]
    );
}

#[test]
fn multiple_branches_into_same_delay_slot_one_rejoin() {
    // Two distinct back-edges into the same delay slot: exactly one rejoin
    // duplicate at the slot's position; both branches goto the same label.
    //   0x00: beq a2, zero, +0x18 (-> 0x1C)
    //   0x04: addiu a2, a2, -1    <- target of BOTH bne ops below
    //   0x08: nop
    //   0x0C: bne a2, zero, -0xC  (-> 0x04)
    //   0x10: nop
    //   0x14: bne a3, zero, -0x14 (-> 0x04)
    //   0x18: nop
    //   0x1C: jr ra
    //   0x20: nop
    let base = 0x0880_0000u32;
    let words = [
        (0x04u32 << 26) | (6 << 21) | 6,                  // beq a2,zero,+0x1C
        0x24C6_FFFF,                                      // addiu a2,a2,-1
        0,                                                // nop
        (0x05u32 << 26) | (6 << 21) | (0xFFFD & 0xFFFF),  // bne a2,zero -> 0x04
        0,                                                // nop
        (0x05u32 << 26) | (7 << 21) | (0xFFFB & 0xFFFF),  // bne a3,zero -> 0x04
        0,                                                // nop
        (31 << 21) | 0x08,                                // jr ra
        0,                                                // nop
    ];
    let ops = decode_function(&words_to_bytes(&words), base, &[]).unwrap();

    assert_eq!(ops.len(), 9, "positional mapping preserved");
    assert!(matches!(ops[0], MipsOp::BranchHazardDelay { .. }));
    let rejoin_count = ops
        .iter()
        .filter(|op| matches!(op, MipsOp::DelaySlotRejoin { .. }))
        .count();
    assert_eq!(rejoin_count, 1, "exactly one duplicate at the shared slot");
    assert!(
        matches!(&ops[1], MipsOp::DelaySlotRejoin { instr }
            if matches!(**instr, MipsOp::Addiu { rt: Reg::Gpr(6), rs: Reg::Gpr(6), imm: -1 })),
        "got {:?}",
        ops[1]
    );
}

#[test]
fn branch_as_bids_delay_instruction_is_loud_error() {
    // A control-transfer in a branched-into delay slot is invalid MIPS the
    // rejoin duplicate cannot represent — must fail loudly, not mis-emit.
    //   0x00: beq a0, a1, +8 (-> 0x0C... irrelevant)
    //   0x04: bne a0, zero, +8  <- a BRANCH in the delay slot, AND targeted
    //   0x08: nop
    //   0x0C: bne a1, zero, -0xC (-> 0x04)
    //   0x10: nop
    let base = 0x0880_0000u32;
    let words = [
        (0x04u32 << 26) | (4 << 21) | (5 << 16) | 2,      // beq a0,a1,+0xC
        (0x05u32 << 26) | (4 << 21) | 2,                  // bne a0,zero,+0xC
        0,                                                // nop
        (0x05u32 << 26) | (5 << 21) | (0xFFFD & 0xFFFF),  // bne a1,zero -> 0x04
        0,                                                // nop
    ];
    let err = decode_function(&words_to_bytes(&words), base, &[]).unwrap_err();
    assert!(
        matches!(err, DecodeError::BranchInDelaySlot(0x0880_0004)),
        "expected loud BranchInDelaySlot error, got {err:?}"
    );
}

#[test]
fn likely_branch_bids_target_gets_rejoin() {
    // Branch-likely whose delay slot is a branch target: the duplicate sits
    // at the slot's label; the conditional delay-slot copy stays inside the
    // taken path (likely semantics).
    //   0x00: beql a0, a1, +0x10 (-> 0x14)
    //   0x04: addiu t0, t0, 1   <- conditional delay slot AND branch target
    //   0x08: nop
    //   0x0C: bne a0, zero, -0xC (-> 0x04)
    //   0x10: nop
    //   0x14: jr ra
    //   0x18: nop
    let base = 0x0880_0000u32;
    let words = [
        (0x14u32 << 26) | (4 << 21) | (5 << 16) | 4,      // beql a0,a1,+0x14
        (0x09u32 << 26) | (8 << 21) | (8 << 16) | 1,      // addiu t0,t0,1
        0,                                                // nop
        (0x05u32 << 26) | (4 << 21) | (0xFFFD & 0xFFFF),  // bne a0,zero -> 0x04
        0,                                                // nop
        (31 << 21) | 0x08,                                // jr ra
        0,                                                // nop
    ];
    let ops = decode_function(&words_to_bytes(&words), base, &[]).unwrap();

    assert!(
        matches!(ops[0], MipsOp::Beq { likely: true, .. }),
        "likely branch stays at its position, got {:?}",
        ops[0]
    );
    assert!(
        matches!(&ops[1], MipsOp::DelaySlotRejoin { instr }
            if matches!(**instr, MipsOp::Addiu { rt: Reg::Gpr(8), .. })),
        "likely BIDS slot gets the duplicate (DelaySlotRejoin), got {:?}",
        ops[1]
    );
}

#[test]
fn non_bids_pairs_keep_existing_lowering() {
    // Scope guard (Patapon byte-identity): without any branch into the delay
    // slot, the hazard pair stays BranchHazardDelay + Nop and the standard
    // pair stays swapped — no DelaySlotRejoin anywhere.
    let base = 0x0880_0000u32;
    let words = [
        (0x04u32 << 26) | (6 << 21) | 2, // beq a2,zero,+0xC (hazard with addiu)
        0x24C6_FFFF,                     // addiu a2,a2,-1
        0,                               // nop
        (31 << 21) | 0x08,               // jr ra
        0,                               // nop
    ];
    let ops = decode_function(&words_to_bytes(&words), base, &[]).unwrap();
    assert!(matches!(ops[0], MipsOp::BranchHazardDelay { .. }));
    assert!(
        matches!(ops[1], MipsOp::Nop {}),
        "non-BIDS hazard pad must stay Nop, got {:?}",
        ops[1]
    );
    assert!(
        !ops.iter().any(|op| matches!(op, MipsOp::DelaySlotRejoin { .. })),
        "no rejoin without a branch into the slot"
    );
}
