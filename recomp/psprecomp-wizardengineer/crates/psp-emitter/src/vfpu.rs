//! VFPU emission module -- maps each VFPU MipsOp variant to a C++ runtime
//! library call.
//!
//! Every VFPU instruction emits a single function call to the runtime
//! library defined in `psp_vfpu.h`. No inline math -- all computation
//! is delegated to the C++ runtime functions.

use psp_ir::{MipsOp, Reg, VfpuMatOp, VfpuUnaryOp};

use crate::function::emit_branch_or_tail;
use crate::generator::Generator;

/// VFPU control register index for condition code.
pub(crate) const VFPU_CTRL_CC: u8 = 3;

/// Emit a single VFPU MipsOp as a C++ runtime library call.
///
/// Dispatches each VFPU variant to the corresponding runtime function
/// declared in `psp_vfpu.h`. Branch ops use `emit_branch_or_tail` for
/// correct intra/cross-function dispatch.
pub(crate) fn emit_vfpu_op(
    op: &MipsOp,
    gen: &mut dyn Generator,
    func_start: u32,
    func_end: u32,
) {
    match op {
        // -------------------------------------------------------------
        // Prefix
        // -------------------------------------------------------------
        MipsOp::VfpuPrefix { reg_idx, data } => {
            gen.emit_raw(&format!(
                "vfpu_set_prefix(ctx, {reg_idx}, 0x{data:08X}u);"
            ));
        }

        // -------------------------------------------------------------
        // Immediate loads
        // -------------------------------------------------------------
        MipsOp::VfpuViim { vt, imm } => {
            gen.emit_raw(&format!(
                "vfpu_viim(ctx, 0x{vt:02X}, {imm}u);"
            ));
        }
        MipsOp::VfpuVfim { vt, imm } => {
            gen.emit_raw(&format!(
                "vfpu_vfim(ctx, 0x{vt:02X}, 0x{imm:04X}u);"
            ));
        }

        // -------------------------------------------------------------
        // Binary arithmetic (VFPU0/VFPU1 groups)
        // -------------------------------------------------------------
        MipsOp::VfpuAdd { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vadd", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuSub { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vsub", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuMul { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vmul", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuDiv { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vdiv", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuDot { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vdot", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuScl { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vscl", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuHdp { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vhdp", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuCrs { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vcrs", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuDet { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vdet", *vd, *vs, *vt, *size);
        }

        // -------------------------------------------------------------
        // Compare/minmax (VFPU3 group)
        // -------------------------------------------------------------
        MipsOp::VfpuCmp { vs, vt, cond, size } => {
            gen.emit_raw(&format!(
                "vfpu_vcmp(ctx, rdram, 0x{vs:02X}, 0x{vt:02X}, \
                 {cond}, {size});"
            ));
        }
        MipsOp::VfpuVmin { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vmin", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuVmax { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vmax", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuScmp { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vscmp", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuSge { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vsge", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuSlt { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vslt", *vd, *vs, *vt, *size);
        }

        // -------------------------------------------------------------
        // Unary/trig (VFPU4Jump group)
        // -------------------------------------------------------------
        MipsOp::VfpuUnary { vd, vs, op, size, imm } => {
            emit_unary_op(gen, *vd, *vs, op, *size, *imm);
        }

        // -------------------------------------------------------------
        // Matrix ops (VFPU6 group)
        // -------------------------------------------------------------
        MipsOp::VfpuMmul { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vmmul", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuMscl { vd, vs, vt, size } => {
            emit_binary(gen, "vfpu_vmscl", *vd, *vs, *vt, *size);
        }
        MipsOp::VfpuTfm { vd, vs, vt, size } => {
            // Size-specific variants: vtfm2/3/4
            let fn_name = match size {
                2 => "vfpu_vtfm2",
                3 => "vfpu_vtfm3",
                _ => "vfpu_vtfm4",
            };
            gen.emit_raw(&format!(
                "{fn_name}(ctx, rdram, 0x{vd:02X}, 0x{vs:02X}, \
                 0x{vt:02X});"
            ));
        }
        MipsOp::VfpuHtfm { vd, vs, vt, size } => {
            // Size-specific variants: vhtfm2/3/4
            let fn_name = match size {
                2 => "vfpu_vhtfm2",
                3 => "vfpu_vhtfm3",
                _ => "vfpu_vhtfm4",
            };
            gen.emit_raw(&format!(
                "{fn_name}(ctx, rdram, 0x{vd:02X}, 0x{vs:02X}, \
                 0x{vt:02X});"
            ));
        }
        MipsOp::VfpuCrsp { vd, vs, vt, size } => {
            // size==3 => vcrsp (cross product),
            // size==4 => vqmul (quaternion multiply)
            let fn_name = if *size == 4 {
                "vfpu_vqmul"
            } else {
                "vfpu_vcrsp"
            };
            gen.emit_raw(&format!(
                "{fn_name}(ctx, rdram, 0x{vd:02X}, 0x{vs:02X}, \
                 0x{vt:02X});"
            ));
        }
        MipsOp::VfpuMatUnary {
            vd, vs, op: mat_op, size, imm,
        } => {
            emit_mat_unary_op(gen, *vd, *vs, mat_op, *size, *imm);
        }

        // -------------------------------------------------------------
        // Memory operations
        // -------------------------------------------------------------
        MipsOp::VfpuLvS { vt, rs, offset } => {
            emit_mem_op(gen, "vfpu_lv_s", *vt, *rs, *offset);
        }
        MipsOp::VfpuSvS { vt, rs, offset } => {
            emit_mem_op(gen, "vfpu_sv_s", *vt, *rs, *offset);
        }
        MipsOp::VfpuLvQ { vt, rs, offset } => {
            emit_mem_op(gen, "vfpu_lv_q", *vt, *rs, *offset);
        }
        MipsOp::VfpuSvQ { vt, rs, offset } => {
            emit_mem_op(gen, "vfpu_sv_q", *vt, *rs, *offset);
        }
        MipsOp::VfpuLvlQ { vt, rs, offset } => {
            emit_mem_op(gen, "vfpu_lvl_q", *vt, *rs, *offset);
        }
        MipsOp::VfpuLvrQ { vt, rs, offset } => {
            emit_mem_op(gen, "vfpu_lvr_q", *vt, *rs, *offset);
        }
        MipsOp::VfpuSvlQ { vt, rs, offset } => {
            emit_mem_op(gen, "vfpu_svl_q", *vt, *rs, *offset);
        }
        MipsOp::VfpuSvrQ { vt, rs, offset } => {
            emit_mem_op(gen, "vfpu_svr_q", *vt, *rs, *offset);
        }

        // -------------------------------------------------------------
        // Control register moves
        // -------------------------------------------------------------
        MipsOp::VfpuMfv { rt, vd } => {
            let rt_idx = rt.num();
            gen.emit_raw(&format!(
                "vfpu_mfv(ctx, {rt_idx}, 0x{vd:02X});"
            ));
        }
        MipsOp::VfpuMtv { rt, vd } => {
            let rt_idx = rt.num();
            gen.emit_raw(&format!(
                "vfpu_mtv(ctx, {rt_idx}, 0x{vd:02X});"
            ));
        }
        MipsOp::VfpuMfvc { rt, imm } => {
            let rt_idx = rt.num();
            gen.emit_raw(&format!(
                "vfpu_mfvc(ctx, {rt_idx}, {imm});"
            ));
        }
        MipsOp::VfpuMtvc { rt, imm } => {
            let rt_idx = rt.num();
            gen.emit_raw(&format!(
                "vfpu_mtvc(ctx, {rt_idx}, {imm});"
            ));
        }

        // -------------------------------------------------------------
        // Branches
        // -------------------------------------------------------------
        MipsOp::VfpuBvf { cc, target, likely } => {
            let cond = format!(
                "!(ctx->vfpu_ctrl[{VFPU_CTRL_CC}] & (1 << {cc}))"
            );
            emit_branch_or_tail(
                gen, &cond, *target, *likely, "/* bvfl */",
                func_start, func_end,
            );
        }
        MipsOp::VfpuBvt { cc, target, likely } => {
            let cond = format!(
                "(ctx->vfpu_ctrl[{VFPU_CTRL_CC}] & (1 << {cc}))"
            );
            emit_branch_or_tail(
                gen, &cond, *target, *likely, "/* bvtl */",
                func_start, func_end,
            );
        }

        // -------------------------------------------------------------
        // Flush (no-op)
        // -------------------------------------------------------------
        MipsOp::VfpuFlush {} => {
            gen.emit_raw("/* vfpu_vflush */;");
        }

        // -------------------------------------------------------------
        // Unknown VFPU sub-op (log-once runtime warning)
        // -------------------------------------------------------------
        MipsOp::VfpuUnknown { opcode, pc } => {
            gen.emit_raw(&format!(
                "vfpu_unknown_stub(ctx, rdram, \
                 0x{opcode:08X}u, 0x{pc:08X}u);"
            ));
        }

        // Non-VFPU ops should never reach here
        _ => unreachable!(
            "emit_vfpu_op called with non-VFPU op: {:?}", op
        ),
    }
}

/// Emit a standard binary VFPU op: fn(ctx, rdram, vd, vs, vt, size).
fn emit_binary(
    gen: &mut dyn Generator,
    fn_name: &str,
    vd: u8,
    vs: u8,
    vt: u8,
    size: u8,
) {
    gen.emit_raw(&format!(
        "{fn_name}(ctx, rdram, 0x{vd:02X}, 0x{vs:02X}, \
         0x{vt:02X}, {size});"
    ));
}

/// Emit a VFPU memory operation: fn(ctx, rdram, vt, rs_idx, offset).
///
/// Memory ops pass the GPR index directly -- the runtime function reads
/// `ctx->r[rs]` internally to get the base address.
fn emit_mem_op(
    gen: &mut dyn Generator,
    fn_name: &str,
    vt: u8,
    rs: Reg,
    offset: i16,
) {
    let rs_idx = rs.num();
    gen.emit_raw(&format!(
        "{fn_name}(ctx, rdram, 0x{vt:02X}, {rs_idx}, {offset});"
    ));
}

/// Emit a VFPU unary operation.
///
/// Dispatches to the correct runtime function based on the VfpuUnaryOp
/// discriminator. Some ops need an additional `imm` parameter.
fn emit_unary_op(
    gen: &mut dyn Generator,
    vd: u8,
    vs: u8,
    op: &VfpuUnaryOp,
    size: u8,
    imm: u8,
) {
    match op {
        // Standard unary: fn(ctx, rdram, vd, vs, size)
        VfpuUnaryOp::Mov => emit_unary(gen, "vfpu_vmov", vd, vs, size),
        VfpuUnaryOp::Abs => emit_unary(gen, "vfpu_vabs", vd, vs, size),
        VfpuUnaryOp::Neg => emit_unary(gen, "vfpu_vneg", vd, vs, size),
        VfpuUnaryOp::Rcp => emit_unary(gen, "vfpu_vrcp", vd, vs, size),
        VfpuUnaryOp::Rsq => emit_unary(gen, "vfpu_vrsq", vd, vs, size),
        VfpuUnaryOp::Sin => emit_unary(gen, "vfpu_vsin", vd, vs, size),
        VfpuUnaryOp::Cos => emit_unary(gen, "vfpu_vcos", vd, vs, size),
        VfpuUnaryOp::Exp2 => emit_unary(gen, "vfpu_vexp2", vd, vs, size),
        VfpuUnaryOp::Log2 => emit_unary(gen, "vfpu_vlog2", vd, vs, size),
        VfpuUnaryOp::Sqrt => emit_unary(gen, "vfpu_vsqrt", vd, vs, size),
        VfpuUnaryOp::Asin => emit_unary(gen, "vfpu_vasin", vd, vs, size),
        VfpuUnaryOp::Nrcp => emit_unary(gen, "vfpu_vnrcp", vd, vs, size),
        VfpuUnaryOp::Nsin => emit_unary(gen, "vfpu_vnsin", vd, vs, size),
        VfpuUnaryOp::Rexp2 => {
            emit_unary(gen, "vfpu_vrexp2", vd, vs, size);
        }
        VfpuUnaryOp::Sat0 => {
            emit_unary(gen, "vfpu_vsat0", vd, vs, size);
        }
        VfpuUnaryOp::Sat1 => {
            emit_unary(gen, "vfpu_vsat1", vd, vs, size);
        }

        // Sort/pack/misc unary: fn(ctx, rdram, vd, vs, size)
        VfpuUnaryOp::Vsrt1 => {
            emit_unary(gen, "vfpu_vsrt1", vd, vs, size);
        }
        VfpuUnaryOp::Vsrt2 => {
            emit_unary(gen, "vfpu_vsrt2", vd, vs, size);
        }
        VfpuUnaryOp::Vsrt3 => {
            emit_unary(gen, "vfpu_vsrt3", vd, vs, size);
        }
        VfpuUnaryOp::Vsrt4 => {
            emit_unary(gen, "vfpu_vsrt4", vd, vs, size);
        }
        VfpuUnaryOp::Vbfy1 => {
            emit_unary(gen, "vfpu_vbfy1", vd, vs, size);
        }
        VfpuUnaryOp::Vbfy2 => {
            emit_unary(gen, "vfpu_vbfy2", vd, vs, size);
        }
        VfpuUnaryOp::Vsocp => {
            emit_unary(gen, "vfpu_vsocp", vd, vs, size);
        }
        VfpuUnaryOp::Vfad => {
            emit_unary(gen, "vfpu_vfad", vd, vs, size);
        }
        VfpuUnaryOp::Vavg => {
            emit_unary(gen, "vfpu_vavg", vd, vs, size);
        }

        // Conversion ops with int-to-float: fn(ctx, rdram, vd, vs, size)
        VfpuUnaryOp::Vi2uc => {
            emit_unary(gen, "vfpu_vi2uc", vd, vs, size);
        }
        VfpuUnaryOp::Vi2c => {
            emit_unary(gen, "vfpu_vi2c", vd, vs, size);
        }
        VfpuUnaryOp::Vi2us => {
            emit_unary(gen, "vfpu_vi2us", vd, vs, size);
        }
        VfpuUnaryOp::Vi2s => {
            emit_unary(gen, "vfpu_vi2s", vd, vs, size);
        }
        VfpuUnaryOp::Vuc2i => {
            emit_unary(gen, "vfpu_vuc2i", vd, vs, size);
        }
        VfpuUnaryOp::Vc2i => {
            emit_unary(gen, "vfpu_vc2i", vd, vs, size);
        }
        VfpuUnaryOp::Vus2i => {
            emit_unary(gen, "vfpu_vus2i", vd, vs, size);
        }
        VfpuUnaryOp::Vs2i => {
            emit_unary(gen, "vfpu_vs2i", vd, vs, size);
        }
        VfpuUnaryOp::Vf2h => {
            emit_unary(gen, "vfpu_vf2h", vd, vs, size);
        }
        VfpuUnaryOp::Vh2f => {
            emit_unary(gen, "vfpu_vh2f", vd, vs, size);
        }

        // Identity/zero/one -- no vs source:
        // fn(ctx, rdram, vd, size)
        VfpuUnaryOp::Idt => {
            gen.emit_raw(&format!(
                "vfpu_vidt(ctx, rdram, 0x{vd:02X}, {size});"
            ));
        }
        VfpuUnaryOp::Vzero => {
            gen.emit_raw(&format!(
                "vfpu_vzero(ctx, rdram, 0x{vd:02X}, {size});"
            ));
        }
        VfpuUnaryOp::Vone => {
            gen.emit_raw(&format!(
                "vfpu_vone(ctx, rdram, 0x{vd:02X}, {size});"
            ));
        }

        // Random ops: vrnds takes only vs (seed), no vd/size
        VfpuUnaryOp::Vrnds => {
            gen.emit_raw(&format!(
                "vfpu_vrnds(ctx, rdram, 0x{vs:02X});"
            ));
        }
        VfpuUnaryOp::Vrndi => {
            gen.emit_raw(&format!(
                "vfpu_vrndi(ctx, rdram, 0x{vd:02X}, {size});"
            ));
        }
        VfpuUnaryOp::Vrndf1 => {
            gen.emit_raw(&format!(
                "vfpu_vrndf1(ctx, rdram, 0x{vd:02X}, {size});"
            ));
        }
        VfpuUnaryOp::Vrndf2 => {
            gen.emit_raw(&format!(
                "vfpu_vrndf2(ctx, rdram, 0x{vd:02X}, {size});"
            ));
        }

        // Constants: vcst(ctx, rdram, vd, imm5, size)
        VfpuUnaryOp::Vcst => {
            gen.emit_raw(&format!(
                "vfpu_vcst(ctx, rdram, 0x{vd:02X}, {imm}, {size});"
            ));
        }

        // Conversion with imm: fn(ctx, rdram, vd, vs, imm5, size)
        VfpuUnaryOp::Vf2in => {
            emit_unary_imm(gen, "vfpu_vf2in", vd, vs, imm, size);
        }
        VfpuUnaryOp::Vf2iz => {
            emit_unary_imm(gen, "vfpu_vf2iz", vd, vs, imm, size);
        }
        VfpuUnaryOp::Vf2iu => {
            emit_unary_imm(gen, "vfpu_vf2iu", vd, vs, imm, size);
        }
        VfpuUnaryOp::Vf2id => {
            emit_unary_imm(gen, "vfpu_vf2id", vd, vs, imm, size);
        }
        VfpuUnaryOp::Vi2f => {
            emit_unary_imm(gen, "vfpu_vi2f", vd, vs, imm, size);
        }

        // Conditional move: vcmov(ctx, rdram, vd, vs, cc, size)
        VfpuUnaryOp::Cmov0 | VfpuUnaryOp::Cmov1 => {
            gen.emit_raw(&format!(
                "vfpu_vcmov(ctx, rdram, 0x{vd:02X}, 0x{vs:02X}, \
                 {imm}, {size});"
            ));
        }

        // Wrap by negative: vwbn(ctx, rdram, vd, vs, imm8, size)
        VfpuUnaryOp::Wbn => {
            gen.emit_raw(&format!(
                "vfpu_vwbn(ctx, rdram, 0x{vd:02X}, 0x{vs:02X}, \
                 {imm}, {size});"
            ));
        }
    }
}

/// Emit standard unary: fn(ctx, rdram, vd, vs, size).
fn emit_unary(
    gen: &mut dyn Generator,
    fn_name: &str,
    vd: u8,
    vs: u8,
    size: u8,
) {
    gen.emit_raw(&format!(
        "{fn_name}(ctx, rdram, 0x{vd:02X}, 0x{vs:02X}, {size});"
    ));
}

/// Emit unary with imm: fn(ctx, rdram, vd, vs, imm5, size).
fn emit_unary_imm(
    gen: &mut dyn Generator,
    fn_name: &str,
    vd: u8,
    vs: u8,
    imm: u8,
    size: u8,
) {
    gen.emit_raw(&format!(
        "{fn_name}(ctx, rdram, 0x{vd:02X}, 0x{vs:02X}, \
         {imm}, {size});"
    ));
}

/// Emit a matrix unary operation.
fn emit_mat_unary_op(
    gen: &mut dyn Generator,
    vd: u8,
    vs: u8,
    op: &VfpuMatOp,
    size: u8,
    imm: u8,
) {
    match op {
        VfpuMatOp::Mmov => {
            gen.emit_raw(&format!(
                "vfpu_vmmov(ctx, rdram, 0x{vd:02X}, 0x{vs:02X}, \
                 {size});"
            ));
        }
        VfpuMatOp::Midt => {
            gen.emit_raw(&format!(
                "vfpu_vmidt(ctx, rdram, 0x{vd:02X}, {size});"
            ));
        }
        VfpuMatOp::Mzero => {
            gen.emit_raw(&format!(
                "vfpu_vmzero(ctx, rdram, 0x{vd:02X}, {size});"
            ));
        }
        VfpuMatOp::Mone => {
            gen.emit_raw(&format!(
                "vfpu_vmone(ctx, rdram, 0x{vd:02X}, {size});"
            ));
        }
        VfpuMatOp::Vrot => {
            gen.emit_raw(&format!(
                "vfpu_vrot(ctx, rdram, 0x{vd:02X}, 0x{vs:02X}, \
                 {imm}, {size});"
            ));
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::generator::TestGenerator;

    #[test]
    fn emit_prefix() {
        let mut gen = TestGenerator::new();
        let op = MipsOp::VfpuPrefix {
            reg_idx: 0,
            data: 0x000E4,
        };
        emit_vfpu_op(&op, &mut gen, 0, 0x1000);
        assert!(gen.output[0].contains("vfpu_set_prefix"));
        assert!(gen.output[0].contains("0x000000E4u"));
    }

    #[test]
    fn emit_vadd() {
        let mut gen = TestGenerator::new();
        let op = MipsOp::VfpuAdd {
            vd: 0x01,
            vs: 0x02,
            vt: 0x03,
            size: 4,
        };
        emit_vfpu_op(&op, &mut gen, 0, 0x1000);
        assert!(gen.output[0].contains("vfpu_vadd"));
        assert!(gen.output[0].contains("0x01"));
        assert!(gen.output[0].contains("0x02"));
        assert!(gen.output[0].contains("0x03"));
        assert!(gen.output[0].contains(", 4)"));
    }

    #[test]
    fn emit_lv_q() {
        let mut gen = TestGenerator::new();
        let op = MipsOp::VfpuLvQ {
            vt: 0x10,
            rs: Reg::Gpr(4),
            offset: 16,
        };
        emit_vfpu_op(&op, &mut gen, 0, 0x1000);
        assert!(gen.output[0].contains("vfpu_lv_q"));
        assert!(gen.output[0].contains("0x10"));
        assert!(gen.output[0].contains(", 4,"));
        assert!(gen.output[0].contains("16)"));
    }

    #[test]
    fn emit_bvt_in_function() {
        let mut gen = TestGenerator::new();
        let op = MipsOp::VfpuBvt {
            cc: 0,
            target: 0x0100,
            likely: false,
        };
        emit_vfpu_op(&op, &mut gen, 0, 0x1000);
        let joined = gen.output.join("\n");
        assert!(joined.contains("vfpu_ctrl[3]"));
    }

    #[test]
    fn emit_vcst() {
        let mut gen = TestGenerator::new();
        let op = MipsOp::VfpuUnary {
            vd: 0x05,
            vs: 0x00,
            op: VfpuUnaryOp::Vcst,
            size: 1,
            imm: 3,
        };
        emit_vfpu_op(&op, &mut gen, 0, 0x1000);
        assert!(gen.output[0].contains("vfpu_vcst"));
        assert!(gen.output[0].contains("0x05"));
        assert!(gen.output[0].contains(", 3,"));
    }

    #[test]
    fn emit_vrot() {
        let mut gen = TestGenerator::new();
        let op = MipsOp::VfpuMatUnary {
            vd: 0x01,
            vs: 0x02,
            op: VfpuMatOp::Vrot,
            size: 4,
            imm: 12,
        };
        emit_vfpu_op(&op, &mut gen, 0, 0x1000);
        assert!(gen.output[0].contains("vfpu_vrot"));
        assert!(gen.output[0].contains("12"));
    }

    #[test]
    fn emit_unknown_stub() {
        let mut gen = TestGenerator::new();
        let op = MipsOp::VfpuUnknown {
            opcode: 0xDEADBEEF,
            pc: 0x08804000,
        };
        emit_vfpu_op(&op, &mut gen, 0, 0x1000);
        assert!(gen.output[0].contains("vfpu_unknown_stub"));
        assert!(gen.output[0].contains("0xDEADBEEF"));
    }

    #[test]
    fn emit_mfv() {
        let mut gen = TestGenerator::new();
        let op = MipsOp::VfpuMfv {
            rt: Reg::Gpr(2),
            vd: 0x10,
        };
        emit_vfpu_op(&op, &mut gen, 0, 0x1000);
        assert!(gen.output[0].contains("vfpu_mfv"));
        assert!(gen.output[0].contains(", 2,"));
    }

    #[test]
    fn emit_flush() {
        let mut gen = TestGenerator::new();
        let op = MipsOp::VfpuFlush {};
        emit_vfpu_op(&op, &mut gen, 0, 0x1000);
        assert!(gen.output[0].contains("vfpu_vflush"));
    }
}
