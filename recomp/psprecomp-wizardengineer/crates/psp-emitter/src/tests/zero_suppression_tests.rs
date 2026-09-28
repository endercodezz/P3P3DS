//! TDD tests for $zero register suppression (EMIT-12).
//! Phase 2 success criterion 2.
use crate::{TestGenerator, Generator};
use psp_ir::Reg;

#[test]
fn zero_write_produces_no_output() {
    let mut gen = TestGenerator::new();
    gen.emit_gpr_write(Reg::Zero, "ctx->r4 + 1");
    assert!(gen.output.is_empty(),
        "writing to $zero must produce empty output, got: {:?}", gen.output);
}

#[test]
fn zero_read_returns_literal_zero() {
    let gen = TestGenerator::new();
    assert_eq!(gen.emit_gpr_read(Reg::Zero), "0",
        "reading $zero must return literal string '0'");
}

#[test]
fn non_zero_write_produces_output() {
    let mut gen = TestGenerator::new();
    gen.emit_gpr_write(Reg::Gpr(8), "42");
    assert_eq!(gen.output.len(), 1,
        "writing to $t0 (r8) must produce exactly one output line");
}

#[test]
fn non_zero_read_returns_register_expression() {
    let gen = TestGenerator::new();
    let s = gen.emit_gpr_read(Reg::Gpr(4));
    assert!(!s.is_empty() && s != "0",
        "$a0 read must return a non-empty, non-zero expression, got: {:?}", s);
}

#[test]
fn sequence_with_zero_writes_filters_correctly() {
    let mut gen = TestGenerator::new();
    gen.emit_gpr_write(Reg::Zero, "1");        // suppressed
    gen.emit_gpr_write(Reg::Gpr(8), "ctx->r9 + 1");  // kept
    gen.emit_gpr_write(Reg::Zero, "2");        // suppressed
    assert_eq!(gen.output.len(), 1,
        "only non-zero writes should appear in output");
}
