//! TDD tests for delay slot semantics (DECODE-04, DECODE-05).
//! Phase 2 success criterion 3.
use crate::decode_function;
use psp_ir::MipsOp;

/// Encode a MIPS word: beq $a0, $a1, +8 (offset=2 in words)
fn encode_beq(likely: bool) -> u32 {
    let opcode: u32 = if likely { 0x14 } else { 0x04 };
    let rs: u32 = 4;  // $a0
    let rt: u32 = 5;  // $a1
    let offset: u32 = 2u32; // +8 bytes = 2 words forward
    (opcode << 26) | (rs << 21) | (rt << 16) | (offset & 0xFFFF)
}

/// Encode a MIPS NOP (sll $zero, $zero, 0 = 0x00000000)
fn encode_nop() -> u32 { 0x00000000 }

/// Encode addu $t0, $t1, $t2
fn encode_addu() -> u32 {
    // SPECIAL func=0x21, rs=$t1=9, rt=$t2=10, rd=$t0=8
    (0x00u32 << 26) | (9 << 21) | (10 << 16) | (8 << 11) | 0x21
}

#[test]
fn standard_delay_slot_before_branch() {
    // beq $a0, $a1, +8   (standard: delay slot executes unconditionally)
    // nop                (delay slot instruction)
    let words = [encode_beq(false), encode_nop()];
    let bytes: Vec<u8> = words.iter().flat_map(|w| w.to_le_bytes()).collect();
    let ops = decode_function(&bytes, 0x08804000, &[]).unwrap();

    // Standard branch: delay slot (Nop) must appear BEFORE the Beq in output
    assert!(ops.len() >= 2, "must produce at least 2 ops");
    assert!(matches!(ops[0], MipsOp::Nop {}),
        "delay slot (NOP) must be first: got {:?}", ops[0]);
    assert!(matches!(ops[1], MipsOp::Beq { likely: false, .. }),
        "branch must be second: got {:?}", ops[1]);
}

#[test]
fn branch_likely_delay_slot_wrapped_after_branch() {
    // beql $a0, $a1, +8  (likely: delay slot executes ONLY if branch taken)
    // addu $t0, $t1, $t2 (delay slot instruction)
    let words = [encode_beq(true), encode_addu()];
    let bytes: Vec<u8> = words.iter().flat_map(|w| w.to_le_bytes()).collect();
    let ops = decode_function(&bytes, 0x08804000, &[]).unwrap();

    assert!(ops.len() >= 2, "must produce at least 2 ops");
    // Branch-likely: Beq{likely:true} must come FIRST
    assert!(matches!(ops[0], MipsOp::Beq { likely: true, .. }),
        "branch-likely must be first: got {:?}", ops[0]);
    // DelaySlot wrapper must come SECOND (not the raw Addu)
    assert!(matches!(ops[1], MipsOp::DelaySlot { .. }),
        "delay slot must be wrapped in DelaySlot{{}}: got {:?}", ops[1]);
}

#[test]
fn branch_likely_delay_slot_not_before_branch() {
    // Critical negative test: Pitfall 1 from RESEARCH.md
    // Verify the Addu delay slot does NOT appear BEFORE the branch
    let words = [encode_beq(true), encode_addu()];
    let bytes: Vec<u8> = words.iter().flat_map(|w| w.to_le_bytes()).collect();
    let ops = decode_function(&bytes, 0x08804000, &[]).unwrap();

    // The Addu must NOT be the first op (that would be the standard-branch mistake)
    assert!(!matches!(ops[0], MipsOp::Addu { .. }),
        "PITFALL: delay slot instruction must NOT appear before the branch-likely op");
}
