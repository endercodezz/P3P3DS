//! Peephole IR optimization passes for the PSP recompiler.
//!
//! All passes are **disabled by default** (every flag in `OptimizerConfig`
//! defaults to `false`). Enable passes only after the game renders frames.
//!
//! Decision: passes disabled until Phase 7 to prevent optimizer bugs from
//! masking fundamental pipeline correctness issues.
mod lui_addiu;
mod stack_promote;

use psp_ir::MipsOp;

/// Configuration for the peephole optimizer.
///
/// All fields default to `false`. Call `OptimizerConfig::default()` for
/// the Phase 2 pass-through mode.
#[derive(Debug, Clone, Default)]
pub struct OptimizerConfig {
    /// Collapse `lui Rn, HI; addiu Rn, Rn, LO` pairs into a single
    /// 32-bit constant load. Disabled by default.
    pub lui_addiu_collapse: bool,
    /// Promote sp-relative MEM_W/MEM_B reads/writes to C++ local variables.
    /// Disabled by default.
    pub stack_promote: bool,
}

/// Run enabled peephole passes over a sequence of `MipsOp` IR values.
///
/// With default config (all flags false), returns `ops` unchanged.
/// Each enabled pass runs in order: lui_addiu → stack_promote.
pub fn optimize(ops: Vec<MipsOp>, cfg: &OptimizerConfig) -> Vec<MipsOp> {
    let ops = if cfg.lui_addiu_collapse { lui_addiu::pass(ops) } else { ops };
    let ops = if cfg.stack_promote { stack_promote::pass(ops) } else { ops };
    ops
}

#[cfg(test)]
mod tests {
    use super::*;
    use psp_ir::{MipsOp, Reg};

    #[test]
    fn default_config_passthrough() {
        let ops = vec![
            MipsOp::Nop {},
            MipsOp::Addu { rd: Reg::Gpr(4), rs: Reg::Gpr(5), rt: Reg::Gpr(6) },
        ];
        let cfg = OptimizerConfig::default();
        let result = optimize(ops.clone(), &cfg);
        assert_eq!(result.len(), ops.len(), "pass-through must not change op count");
    }

    #[test]
    fn lui_addiu_disabled_by_default() {
        let lui = MipsOp::Lui { rt: Reg::Gpr(4), imm: 0x0880 };
        let addiu = MipsOp::Addiu { rt: Reg::Gpr(4), rs: Reg::Gpr(4), imm: 0x1000_u16 as i16 };
        let ops = vec![lui, addiu];
        let cfg = OptimizerConfig::default();
        let result = optimize(ops.clone(), &cfg);
        assert_eq!(result.len(), 2, "lui+addiu must NOT be collapsed when pass disabled");
    }
}
