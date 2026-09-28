//! Function and basic block types for the PSP recompiler IR.
//!
//! `DecodedFunction` is the primary output of `psp-decoder` and the primary
//! input to `psp-optimizer` and `psp-emitter`.

use crate::ops::MipsOp;

/// A contiguous block of instructions with a single entry.
#[derive(Debug, Clone)]
pub struct BasicBlock {
    /// Virtual address of the first instruction in this block.
    pub vaddr: u32,
    /// Decoded instructions in execution order.
    pub instrs: Vec<MipsOp>,
}

/// A decoded function ready for emission.
#[derive(Debug, Clone)]
pub struct DecodedFunction {
    /// Virtual address of the function entry point.
    pub vaddr: u32,
    /// Original name from analysis.json (may be auto-generated).
    pub name: String,
    /// C++ identifier (sanitized for valid C++ identifiers and keyword avoidance).
    pub cpp_name: String,
    /// Size of the function in bytes.
    pub size: u32,
    /// Basic blocks in the function body.
    pub blocks: Vec<BasicBlock>,
    /// True if this function has mid-function entry points that need wrapper stubs.
    pub is_mid_entry_parent: bool,
    /// Mid-entry addresses that fall within this function (for dispatch switch emission).
    /// Empty if this is not a mid-entry parent.
    pub mid_entry_addrs: Vec<u32>,
    /// True if this function is a coalesce *owner* — it absorbed one or more
    /// Ghidra-over-split shared-frame siblings into a single emitted C++ function.
    ///
    /// When true, the emitter applies the Option-A intra-function LINK/RA model
    /// (internal `jal`/`bltzal` write `ctx->r[31]` + `goto`; `jr ra` is classified
    /// EXIT vs INTERNAL via a boundary-aware backward walk). All other functions
    /// keep the baseline `jal`→nested-call / `jr ra`→`return;` lowering.
    pub coalesced: bool,
}
