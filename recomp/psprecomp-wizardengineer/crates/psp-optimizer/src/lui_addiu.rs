//! lui+addiu collapse pass.
//!
//! Collapses `lui Rn, HI; addiu Rn, Rn, LO` pairs into a single
//! `addiu Rn, $zero, val` encoding a 32-bit immediate.
//!
//! **DISABLED by default** — only called when `OptimizerConfig::lui_addiu_collapse = true`.
//! V1 equivalent: `collapse_lui_ori()` in psprecomp/src/emitter/optimizer.py.
use psp_ir::{MipsOp, Reg};

/// Run the lui+addiu collapse pass.
pub fn pass(ops: Vec<MipsOp>) -> Vec<MipsOp> {
    let mut out = Vec::with_capacity(ops.len());
    let mut i = 0;
    while i < ops.len() {
        if let Some(fused) = try_collapse(&ops, i) {
            out.push(fused);
            i += 2;
        } else {
            out.push(ops[i].clone());
            i += 1;
        }
    }
    out
}

fn try_collapse(ops: &[MipsOp], i: usize) -> Option<MipsOp> {
    if i + 1 >= ops.len() { return None; }
    if let MipsOp::Lui { rt: lui_rt, imm: hi } = ops[i] {
        if let MipsOp::Addiu { rt: add_rt, rs: add_rs, imm: lo } = ops[i + 1] {
            if lui_rt == add_rt && add_rs == lui_rt {
                let val = (((hi as u32) << 16) as i32).wrapping_add(lo as i32);
                return Some(MipsOp::Addiu {
                    rt: add_rt,
                    rs: Reg::Zero,
                    imm: val as i16,
                });
            }
        }
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn collapses_lui_addiu_pair() {
        let ops = vec![
            MipsOp::Lui { rt: Reg::Gpr(4), imm: 0x0880 },
            MipsOp::Addiu { rt: Reg::Gpr(4), rs: Reg::Gpr(4), imm: 0x1000_u16 as i16 },
        ];
        let result = pass(ops);
        // Should collapse to 1 op
        assert_eq!(result.len(), 1);
    }

    #[test]
    fn does_not_collapse_mismatched_registers() {
        let ops = vec![
            MipsOp::Lui { rt: Reg::Gpr(4), imm: 0x0880 },
            MipsOp::Addiu { rt: Reg::Gpr(5), rs: Reg::Gpr(4), imm: 0 },
        ];
        let result = pass(ops);
        assert_eq!(result.len(), 2, "different rt registers must not collapse");
    }
}
