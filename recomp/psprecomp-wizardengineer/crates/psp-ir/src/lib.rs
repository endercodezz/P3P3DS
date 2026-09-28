//! Typed IR for the PSP Allegrex MIPS32 recompiler.
//!
//! `MipsOp` is a flat enum with one variant per instruction.
//! `Reg` distinguishes `Zero` from numbered GPRs so downstream code
//! can match exhaustively without integer comparisons.
pub mod function;
pub mod ops;
pub mod reg;
pub use ops::{MipsOp, VfpuMatOp, VfpuUnaryOp};
pub use reg::{FpReg, Reg, VfpuReg};
pub use function::{BasicBlock, DecodedFunction};
