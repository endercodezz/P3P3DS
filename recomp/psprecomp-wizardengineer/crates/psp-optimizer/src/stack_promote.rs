//! Stack variable promotion pass.
//!
//! Converts sp-relative MEM_W/MEM_B store+load pairs on known offsets
//! into C++ local variable reads/writes.
//!
//! **DISABLED by default** — only called when `OptimizerConfig::stack_promote = true`.
//! V1 equivalent: `optimize_sp_locals()` in psprecomp/src/emitter/optimizer.py.
//!
//! Phase 2: pass-through stub. Full implementation deferred to Phase 7.
use psp_ir::MipsOp;

/// Run the stack variable promotion pass.
///
/// Phase 2: returns ops unchanged (stub for future implementation).
pub fn pass(ops: Vec<MipsOp>) -> Vec<MipsOp> {
    // Phase 7: implement sp-relative store/load pair detection here.
    ops
}
