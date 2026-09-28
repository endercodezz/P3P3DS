//! Tests for static RECOMP_LOOKUP target tracking (issue #37 dispatch audit).
//!
//! The recompile report diffs the set of statically-emitted lookup targets
//! against the dispatch table; these tests pin down which emission paths
//! record targets (cross-function tails, direct lookup calls) and which do
//! not (in-function branches, register-indirect lookups, switch defaults).
use crate::function::emit_branch_or_tail;
use crate::{CppGenerator, Generator};

#[test]
fn emit_call_lookup_records_static_target() {
    let mut gen = CppGenerator::new();
    gen.emit_call_lookup(0x0881_E7A8);
    assert_eq!(gen.take_static_lookup_targets(), vec![0x0881_E7A8]);
}

#[test]
fn cross_function_branch_tail_records_target() {
    let mut gen = CppGenerator::new();
    // Target 0x08900000 lies outside [0x08804000, 0x08804100) — tail call path.
    emit_branch_or_tail(&mut gen, "a == b", 0x0890_0000, false, "", 0x0880_4000, 0x0880_4100);
    assert_eq!(gen.take_static_lookup_targets(), vec![0x0890_0000]);
    assert!(gen.take_output().contains("RECOMP_LOOKUP(0x08900000u)"));
}

#[test]
fn in_function_branch_records_nothing() {
    let mut gen = CppGenerator::new();
    emit_branch_or_tail(&mut gen, "a == b", 0x0880_4080, false, "", 0x0880_4000, 0x0880_4100);
    assert!(gen.take_static_lookup_targets().is_empty(),
        "in-function branches are gotos, never dispatch lookups");
}

#[test]
fn register_indirect_lookup_records_nothing() {
    let mut gen = CppGenerator::new();
    gen.emit_call_lookup_reg("ctx->r[25]");
    assert!(gen.take_static_lookup_targets().is_empty(),
        "register-indirect targets are not statically known");
}

#[test]
fn switch_default_records_nothing() {
    let mut gen = CppGenerator::new();
    gen.emit_switch("idx", &[(0x0880_4010, "L_08804010".into())]);
    assert!(gen.take_static_lookup_targets().is_empty(),
        "jump-table default fallback targets in-function labels by design");
}

#[test]
fn take_resets_the_recorded_set() {
    let mut gen = CppGenerator::new();
    gen.emit_call_lookup(0x0880_4000);
    let _ = gen.take_static_lookup_targets();
    assert!(gen.take_static_lookup_targets().is_empty());
}
