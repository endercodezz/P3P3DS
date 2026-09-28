//! Tests for `jr $reg` -> `JumpTable` upgrade classification.
//!
//! A register-indirect *tail-jump* (e.g. a vtable dispatch) must NOT be upgraded to a
//! `JumpTable` just because many xrefs point at the function's own entry; doing so made
//! the emitter dispatch the function's own address and self-recurse forever
//! (`FUN_0886f32c`). Only ≥2 *distinct* targets strictly inside the body upgrade.
use crate::decode_function;
use psp_ir::{MipsOp, Reg};

/// Encode `jr $rs` (SPECIAL opcode 0, func 0x08).
fn encode_jr(rs: u32) -> u32 {
    (0x00u32 << 26) | (rs << 21) | 0x08
}

/// Encode a NOP (delay slot filler).
fn encode_nop() -> u32 {
    0x00000000
}

/// Build a small function body whose terminator is `jr $rs`.
///
/// Layout (4 words = 0x10 bytes from `base`): nop, nop, jr $rs, nop(delay slot).
/// The `jr` sits at `base + 8`; interior addresses are `base+4`, `base+8`, `base+0xc`.
fn jr_body(rs: u32) -> Vec<u8> {
    [encode_nop(), encode_nop(), encode_jr(rs), encode_nop()]
        .iter()
        .flat_map(|w| w.to_le_bytes())
        .collect()
}

#[test]
fn vtable_tailjump_xrefs_all_func_start_not_upgraded() {
    // The degenerate case (FUN_0886f32c): every xref `to` equals the function entry
    // (vtable slots holding `&fn`). These must NOT form a jump table.
    let base = 0x0886_F32C;
    let bytes = jr_body(25); // jr $t9
    let xrefs: Vec<(u32, u32)> = (0..34).map(|i| (0x08A4_0000 + i * 4, base)).collect();

    let ops = decode_function(&bytes, base, &xrefs).unwrap();

    assert!(
        ops.iter().any(|op| matches!(op, MipsOp::Jr { rs } if *rs == Reg::Gpr(25))),
        "tail-jump with only func_start xrefs must stay Jr, got {ops:?}"
    );
    assert!(
        !ops.iter().any(|op| matches!(op, MipsOp::JumpTable { .. })),
        "must NOT upgrade to JumpTable when all targets are func_start: {ops:?}"
    );
}

#[test]
fn two_distinct_interior_targets_upgraded_to_jump_table() {
    // A real jump table: ≥2 distinct targets strictly inside the body.
    let base = 0x0880_4000;
    let bytes = jr_body(25); // jr $t9
    // Interior targets: base+4 and base+8 (both > func_start, < func_end).
    // base+0xc is avoided: it is the jr's own DELAY SLOT, which would also
    // (correctly) trigger the BIDS rejoin path (issue #56) — this test is
    // about the JumpTable upgrade only.
    let xrefs = vec![(0x0890_0000, base + 4), (0x0890_0004, base + 8)];

    let ops = decode_function(&bytes, base, &xrefs).unwrap();

    assert!(
        ops.iter().any(|op| matches!(
            op,
            MipsOp::JumpTable { index_reg, cases }
                if *index_reg == Reg::Gpr(25) && cases.len() == 2
        )),
        "two distinct interior targets must upgrade to JumpTable(2 cases): {ops:?}"
    );
}

#[test]
fn single_distinct_interior_target_not_upgraded() {
    // Only one distinct interior target (even if repeated) is not a table.
    let base = 0x0880_4000;
    let bytes = jr_body(25);
    let xrefs = vec![(0x0890_0000, base + 4), (0x0890_0004, base + 4)];

    let ops = decode_function(&bytes, base, &xrefs).unwrap();

    assert!(
        ops.iter().any(|op| matches!(op, MipsOp::Jr { rs } if *rs == Reg::Gpr(25))),
        "single distinct interior target must stay Jr: {ops:?}"
    );
    assert!(
        !ops.iter().any(|op| matches!(op, MipsOp::JumpTable { .. })),
        "must NOT upgrade with only one distinct interior target: {ops:?}"
    );
}
