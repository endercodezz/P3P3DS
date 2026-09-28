//! VFPU instruction decoder for the PSP Allegrex.
//!
//! Decodes all 14 VFPU top-level opcodes (0x12, 0x18, 0x19, 0x1B,
//! 0x32, 0x34-0x37, 0x3A, 0x3C-0x3F) into typed `MipsOp` variants.
//!
//! Reference: PPSSPP Core/MIPS/MIPSTables.cpp opcode dispatch tree,
//! vfpu-docs YAML encoding specification.

use psp_ir::{MipsOp, Reg, VfpuMatOp, VfpuUnaryOp};

use crate::{branch_target, DecodeError};

// -------------------------------------------------------------------------
// Field extraction helpers
// -------------------------------------------------------------------------

/// Extract VFPU VD register field (bits 6:0).
#[inline]
fn vd(word: u32) -> u8 {
    (word & 0x7F) as u8
}

/// Extract VFPU VS register field (bits 14:8).
#[inline]
fn vs(word: u32) -> u8 {
    ((word >> 8) & 0x7F) as u8
}

/// Extract VFPU VT register field (bits 22:16).
#[inline]
fn vt(word: u32) -> u8 {
    ((word >> 16) & 0x7F) as u8
}

/// Extract vector size from instruction encoding.
///
/// Size is encoded across bits 7 and 15:
/// `size = 1 + bit7 + (bit15 << 1)` giving 1=single, 2=pair,
/// 3=triple, 4=quad.
#[inline]
fn vec_size(word: u32) -> u8 {
    let b7 = (word >> 7) & 1;
    let b15 = (word >> 15) & 1;
    (1 + b7 + (b15 << 1)) as u8
}

/// Extract standard MIPS RT register field (bits 20:16).
#[inline]
fn mips_rt(word: u32) -> Reg {
    Reg::from_u8(((word >> 16) & 0x1F) as u8)
}

/// Extract standard MIPS RS register field (bits 25:21).
#[inline]
fn mips_rs(word: u32) -> Reg {
    Reg::from_u8(((word >> 21) & 0x1F) as u8)
}

// -------------------------------------------------------------------------
// COP2 (opcode 0x12)
// -------------------------------------------------------------------------

/// Decode a COP2 (VFPU, opcode=0x12) instruction word.
///
/// Dispatches on bits 25:21 (rs field) for control/move and branch
/// instructions.
pub(crate) fn decode_cop2(
    word: u32,
    vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let rs_field = (word >> 21) & 0x1F;

    match rs_field {
        0x00 => {
            // MFV: move from VFPU to GPR
            let rt = mips_rt(word);
            let vd_field = ((word >> 11) & 0x7F) as u8;
            Ok(MipsOp::VfpuMfv { rt, vd: vd_field })
        }
        0x04 => {
            // MTV: move to VFPU from GPR
            let rt = mips_rt(word);
            let vd_field = ((word >> 11) & 0x7F) as u8;
            Ok(MipsOp::VfpuMtv { rt, vd: vd_field })
        }
        0x03 => {
            // MFVC: move from VFPU control
            let rt = mips_rt(word);
            let imm = ((word >> 8) & 0xFF) as u8;
            Ok(MipsOp::VfpuMfvc { rt, imm })
        }
        0x07 => {
            // MTVC: move to VFPU control
            let rt = mips_rt(word);
            let imm = ((word >> 8) & 0xFF) as u8;
            Ok(MipsOp::VfpuMtvc { rt, imm })
        }
        0x08 => {
            // VFPU branch: bit 16 selects bvf(0)/bvt(1),
            // bit 17 selects likely
            let cc = ((word >> 18) & 7) as u8;
            let offset = (word & 0xFFFF) as i16;
            let target = branch_target(vaddr, offset);
            let is_true = (word >> 16) & 1 != 0;
            let likely = (word >> 17) & 1 != 0;
            if is_true {
                Ok(MipsOp::VfpuBvt { cc, target, likely })
            } else {
                Ok(MipsOp::VfpuBvf { cc, target, likely })
            }
        }
        _ => {
            // COP2 sub-ops with bit 24 set (single-precision VFPU ops
            // inside COP2) -- rare. Decode as unary if bit 25 is set,
            // otherwise unknown.
            if rs_field >= 0x10 {
                decode_cop2_vfpu_single(word, vaddr)
            } else {
                Ok(MipsOp::VfpuUnknown {
                    opcode: word,
                    pc: vaddr,
                })
            }
        }
    }
}

/// Decode COP2 single-precision VFPU operations (rs >= 0x10).
///
/// These are VFPU operations encoded inside the COP2 space.
fn decode_cop2_vfpu_single(
    word: u32,
    vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    // Most of these map to VFPU4-style unary ops.
    // For now, route to VfpuUnknown to be filled as needed.
    Ok(MipsOp::VfpuUnknown {
        opcode: word,
        pc: vaddr,
    })
}

// -------------------------------------------------------------------------
// VFPU0 (opcode 0x18): vadd, vsub, vsbn, vdiv
// -------------------------------------------------------------------------

/// Decode VFPU0 group (opcode 0x18).
pub(crate) fn decode_vfpu0(
    word: u32,
    vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let sub = (word >> 23) & 7;
    let d = vd(word);
    let s = vs(word);
    let t = vt(word);
    let sz = vec_size(word);

    match sub {
        0 => Ok(MipsOp::VfpuAdd {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        1 => Ok(MipsOp::VfpuSub {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        // 6 => vsbn (subtract-negate, uncommon) -- treat as unknown
        7 => Ok(MipsOp::VfpuDiv {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        _ => Ok(MipsOp::VfpuUnknown {
            opcode: word,
            pc: vaddr,
        }),
    }
}

// -------------------------------------------------------------------------
// VFPU1 (opcode 0x19): vmul, vdot, vscl, vhdp, vcrs, vdet
// -------------------------------------------------------------------------

/// Decode VFPU1 group (opcode 0x19).
pub(crate) fn decode_vfpu1(
    word: u32,
    vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let sub = (word >> 23) & 7;
    let d = vd(word);
    let s = vs(word);
    let t = vt(word);
    let sz = vec_size(word);

    match sub {
        0 => Ok(MipsOp::VfpuMul {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        1 => Ok(MipsOp::VfpuDot {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        2 => Ok(MipsOp::VfpuScl {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        3 => Ok(MipsOp::VfpuHdp {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        5 => Ok(MipsOp::VfpuCrs {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        6 => Ok(MipsOp::VfpuDet {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        _ => Ok(MipsOp::VfpuUnknown {
            opcode: word,
            pc: vaddr,
        }),
    }
}

// -------------------------------------------------------------------------
// VFPU3 (opcode 0x1B): vcmp, vmin, vmax, vscmp, vsge, vslt
// -------------------------------------------------------------------------

/// Decode VFPU3 group (opcode 0x1B).
pub(crate) fn decode_vfpu3(
    word: u32,
    vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let sub = (word >> 23) & 7;
    let d = vd(word);
    let s = vs(word);
    let t = vt(word);
    let sz = vec_size(word);

    match sub {
        0 => {
            let cond = (word & 0xF) as u8;
            Ok(MipsOp::VfpuCmp {
                vs: s,
                vt: t,
                cond,
                size: sz,
            })
        }
        1 => Ok(MipsOp::VfpuVmin {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        2 => Ok(MipsOp::VfpuVmax {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        3 => Ok(MipsOp::VfpuScmp {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        4 => Ok(MipsOp::VfpuSge {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        5 => Ok(MipsOp::VfpuSlt {
            vd: d,
            vs: s,
            vt: t,
            size: sz,
        }),
        _ => Ok(MipsOp::VfpuUnknown {
            opcode: word,
            pc: vaddr,
        }),
    }
}

// -------------------------------------------------------------------------
// VFPU4Jump (opcode 0x34): unary/trig ops -- complex sub-dispatch
// -------------------------------------------------------------------------

/// Decode VFPU4 group (opcode 0x34).
///
/// Dispatch matches PPSSPP `tableVFPU4Jump`, indexed by the 5-bit field
/// `bits[25:21]` (`EncodingBitsInfo(21, 5)` in MIPSTables.cpp):
///
/// | idx   | group                                                  |
/// |-------|--------------------------------------------------------|
/// | 0     | VFPU4 unary (mov/abs/.../rcp/rsq/sin/.../sqrt/asin)     |
/// | 1     | VFPU7 (vrnd*, vf2h/vh2f, v*2i / vi2*)                   |
/// | 2     | VFPU9 (vsrt*, vbfy*, vocp/vsocp, vfad, vavg, ...)       |
/// | 3     | vcst (constant load)                                   |
/// | 16-19 | vf2in / vf2iz / vf2iu / vf2id                          |
/// | 20    | vi2f                                                   |
/// | 21    | vcmov                                                  |
/// | 24-31 | vwbn                                                   |
///
/// The previous implementation indexed on the 3-bit `bits[25:23]` field,
/// which collapsed idx 0-3 onto one handler and mis-split idx 16-31 — so
/// vf2i*, vi2f, vcmov, vcst, vocp, vfad, vavg, vsrt*, vbfy* and vwbn all
/// decoded to the wrong op (e.g. vcmov decoded as vf2iz, silently
/// re-zeroing the rsqrt scale in sceGumLookAt's safe-normalize idiom).
pub(crate) fn decode_vfpu4(
    word: u32,
    vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let d = vd(word);
    let s = vs(word);
    let sz = vec_size(word);
    let idx = (word >> 21) & 0x1F;
    let imm5 = ((word >> 16) & 0x1F) as u8;

    let unknown = || {
        Ok(MipsOp::VfpuUnknown {
            opcode: word,
            pc: vaddr,
        })
    };
    let unary_imm = |op| {
        Ok(MipsOp::VfpuUnary {
            vd: d,
            vs: s,
            op,
            size: sz,
            imm: imm5,
        })
    };

    match idx {
        0 => decode_vfpu4_unary(word, d, s, sz, vaddr),
        1 => decode_vfpu7(word, d, s, sz, vaddr),
        2 => decode_vfpu9(word, d, s, sz, vaddr),
        3 => unary_imm(VfpuUnaryOp::Vcst),
        16 => unary_imm(VfpuUnaryOp::Vf2in),
        17 => unary_imm(VfpuUnaryOp::Vf2iz),
        18 => unary_imm(VfpuUnaryOp::Vf2iu),
        19 => unary_imm(VfpuUnaryOp::Vf2id),
        20 => unary_imm(VfpuUnaryOp::Vi2f),
        21 => {
            // vcmov: pack imm3 = (word>>16)&7 with tf = (word>>19)&1
            // into a single byte (tf in bit 3) so the runtime can honor
            // the true/false sense exactly like PPSSPP Int_Vcmov.
            let imm3 = ((word >> 16) & 7) as u8;
            let tf = ((word >> 19) & 1) as u8;
            Ok(MipsOp::VfpuUnary {
                vd: d,
                vs: s,
                op: VfpuUnaryOp::Cmov0,
                size: sz,
                imm: imm3 | (tf << 3),
            })
        }
        24..=31 => unary_imm(VfpuUnaryOp::Wbn),
        _ => unknown(),
    }
}

/// VFPU4 idx 0 — single-vector unary (PPSSPP `tableVFPU4`, sub = bits 20:16).
///
/// Indices 0-7 are move/init ops; 16-28 are trig/math ops.
fn decode_vfpu4_unary(
    word: u32,
    d: u8,
    s: u8,
    sz: u8,
    vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let sub = (word >> 16) & 0x1F;
    let op = match sub {
        0 => VfpuUnaryOp::Mov,
        1 => VfpuUnaryOp::Abs,
        2 => VfpuUnaryOp::Neg,
        3 => VfpuUnaryOp::Idt,
        4 => VfpuUnaryOp::Sat0,
        5 => VfpuUnaryOp::Sat1,
        6 => VfpuUnaryOp::Vzero,
        7 => VfpuUnaryOp::Vone,
        // indices 8-15 are INVALID in PPSSPP
        16 => VfpuUnaryOp::Rcp,
        17 => VfpuUnaryOp::Rsq,
        18 => VfpuUnaryOp::Sin,
        19 => VfpuUnaryOp::Cos,
        20 => VfpuUnaryOp::Exp2,
        21 => VfpuUnaryOp::Log2,
        22 => VfpuUnaryOp::Sqrt,
        23 => VfpuUnaryOp::Asin,
        24 => VfpuUnaryOp::Nrcp,
        26 => VfpuUnaryOp::Nsin,
        28 => VfpuUnaryOp::Rexp2,
        _ => {
            return Ok(MipsOp::VfpuUnknown {
                opcode: word,
                pc: vaddr,
            });
        }
    };
    Ok(MipsOp::VfpuUnary {
        vd: d,
        vs: s,
        op,
        size: sz,
        imm: 0,
    })
}

/// VFPU7 (idx 1) — vrnd*, half-float and packed integer conversions
/// (PPSSPP `tableVFPU7`, sub = bits 20:16).
fn decode_vfpu7(
    word: u32,
    d: u8,
    s: u8,
    sz: u8,
    vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let sub = (word >> 16) & 0x1F;
    let op = match sub {
        0 => VfpuUnaryOp::Vrnds,
        1 => VfpuUnaryOp::Vrndi,
        2 => VfpuUnaryOp::Vrndf1,
        3 => VfpuUnaryOp::Vrndf2,
        // 4 => vsbz (uncommon), 5 => vlgb (uncommon)
        8 => VfpuUnaryOp::Vf2h,
        9 => VfpuUnaryOp::Vh2f,
        12 => VfpuUnaryOp::Vuc2i,
        13 => VfpuUnaryOp::Vc2i,
        14 => VfpuUnaryOp::Vus2i,
        15 => VfpuUnaryOp::Vs2i,
        16 => VfpuUnaryOp::Vi2uc,
        17 => VfpuUnaryOp::Vi2c,
        18 => VfpuUnaryOp::Vi2us,
        19 => VfpuUnaryOp::Vi2s,
        _ => {
            return Ok(MipsOp::VfpuUnknown {
                opcode: word,
                pc: vaddr,
            });
        }
    };
    Ok(MipsOp::VfpuUnary {
        vd: d,
        vs: s,
        op,
        size: sz,
        imm: 0,
    })
}

/// VFPU9 (idx 2) — vsrt*, vbfy*, vocp/vsocp, vfad, vavg
/// (PPSSPP `tableVFPU9`, sub = bits 20:16).
fn decode_vfpu9(
    word: u32,
    d: u8,
    s: u8,
    sz: u8,
    vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let sub = (word >> 16) & 0x1F;
    let op = match sub {
        0 => VfpuUnaryOp::Vsrt1,
        1 => VfpuUnaryOp::Vsrt2,
        2 => VfpuUnaryOp::Vbfy1,
        3 => VfpuUnaryOp::Vbfy2,
        // 4 => vocp (1-x, same size) is NOT modelled and must NOT be
        // aliased to vsocp (which doubles the vector) -- emit a stub.
        5 => VfpuUnaryOp::Vsocp,
        6 => VfpuUnaryOp::Vfad,
        7 => VfpuUnaryOp::Vavg,
        8 => VfpuUnaryOp::Vsrt3,
        9 => VfpuUnaryOp::Vsrt4,
        // 16/17 vmfvc/vmtvc, 24-26 vt4444/vt5551/vt5650 not yet modelled
        _ => {
            return Ok(MipsOp::VfpuUnknown {
                opcode: word,
                pc: vaddr,
            });
        }
    };
    Ok(MipsOp::VfpuUnary {
        vd: d,
        vs: s,
        op,
        size: sz,
        imm: 0,
    })
}

// -------------------------------------------------------------------------
// VFPU5 (opcode 0x37): prefix, viim, vfim
// -------------------------------------------------------------------------

/// Decode VFPU5 group (opcode 0x37).
///
/// PPSSPP tableVFPU5 dispatches on bits 25:23 (3 bits, 8 entries):
///   0,1 -> vpfxs, 2,3 -> vpfxt, 4,5 -> vpfxd, 6 -> viim.s, 7 -> vfim.s
pub(crate) fn decode_vfpu5(
    word: u32,
    _vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let sub = (word >> 23) & 7;

    match sub {
        0 | 1 => {
            // vpfxs (S prefix)
            Ok(MipsOp::VfpuPrefix {
                reg_idx: 0,
                data: word & 0x000F_FFFF,
            })
        }
        2 | 3 => {
            // vpfxt (T prefix)
            Ok(MipsOp::VfpuPrefix {
                reg_idx: 1,
                data: word & 0x000F_FFFF,
            })
        }
        4 | 5 => {
            // vpfxd (D prefix, only 12 bits)
            Ok(MipsOp::VfpuPrefix {
                reg_idx: 2,
                data: word & 0x0000_0FFF,
            })
        }
        6 => {
            // viim.s
            let t = vt(word);
            let imm = (word & 0xFFFF) as i16;
            Ok(MipsOp::VfpuViim { vt: t, imm })
        }
        7 => {
            // vfim.s
            let t = vt(word);
            let imm = (word & 0xFFFF) as u16;
            Ok(MipsOp::VfpuVfim { vt: t, imm })
        }
        _ => unreachable!("3-bit field always 0-7"),
    }
}

// -------------------------------------------------------------------------
// VFPU6 (opcode 0x3C): matrix ops
// -------------------------------------------------------------------------

/// Decode a VFPU6 transform op (`vtfm`N / `vhtfm`N) by size relationship.
///
/// `ins` is the table group `(word>>23)&7` (1=>tfm2, 2=>tfm3, 3=>tfm4) and
/// `n` is the operand vector size from the size bits. PPSSPP `Dis_Vtfm`
/// chooses the homogeneous form (`vhtfm`N) when `n == ins` and the plain
/// form (`vtfm`N) when `n == ins+1`; any other relationship is invalid.
/// The emitted size is always `n` (the printed `vtfm`N / `vhtfm`N digit).
///
/// Args:
///   word: The full 32-bit instruction word (for the unknown fallback).
///   vaddr: The instruction's virtual address (for the unknown fallback).
///   ins: The transform table group `(word>>23)&7`, one of 1, 2 or 3.
///   d: Decoded VD register field.
///   s: Decoded VS register field.
///   t: Decoded VT register field.
///
/// Returns:
///   `VfpuTfm` / `VfpuHtfm` with `size = n`, or `VfpuUnknown` for a bad
///   size relationship.
fn decode_tfm(
    word: u32,
    vaddr: u32,
    ins: u32,
    d: u8,
    s: u8,
    t: u8,
) -> Result<MipsOp, DecodeError> {
    let n = vec_size(word) as u32;
    if n == ins {
        Ok(MipsOp::VfpuHtfm {
            vd: d,
            vs: s,
            vt: t,
            size: n as u8,
        })
    } else if n == ins + 1 {
        Ok(MipsOp::VfpuTfm {
            vd: d,
            vs: s,
            vt: t,
            size: n as u8,
        })
    } else {
        Ok(MipsOp::VfpuUnknown {
            opcode: word,
            pc: vaddr,
        })
    }
}

/// Decode VFPU6 group (opcode 0x3C).
///
/// Complex sub-dispatch via bits 25:21 for matrix operations.
pub(crate) fn decode_vfpu6(
    word: u32,
    vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let d = vd(word);
    let s = vs(word);
    let t = vt(word);
    let sz = vec_size(word);
    let sub = (word >> 23) & 7;

    match sub {
        0 => {
            // vmmul
            Ok(MipsOp::VfpuMmul {
                vd: d,
                vs: s,
                vt: t,
                size: sz,
            })
        }
        // PPSSPP tableVFPU6 indices 4-15 (sub=1,2,3): v(h)tfm2/3/4.
        // The vtfm/vmscl block had been rotated by one slot vs PPSSPP
        // `tableVFPU6` (sub=1 mis-mapped to vmscl, etc.); the correct
        // layout puts the transform groups at sub=1,2,3 and vmscl at
        // sub=4. decode_tfm resolves vtfm-vs-vhtfm from the size bits,
        // not the rt field (mirrors PPSSPP `Dis_Vtfm`).
        1..=3 => decode_tfm(word, vaddr, sub, d, s, t),
        4 => {
            // PPSSPP tableVFPU6 indices 16-19: vmscl (matrix scale).
            Ok(MipsOp::VfpuMscl {
                vd: d,
                vs: s,
                vt: t,
                size: sz,
            })
        }
        5 => {
            // PPSSPP tableVFPU6 indices 20-23: vcrsp.t/vqmul.q
            // Dispatches on bits 22:21 but all 4 entries map to the
            // same cross-product/quaternion-multiply operation;
            // vector size distinguishes triple (vcrsp.t) from quad
            // (vqmul.q).
            Ok(MipsOp::VfpuCrsp {
                vd: d,
                vs: s,
                vt: t,
                size: sz,
            })
        }
        6 => {
            // PPSSPP tableVFPU6 indices 24-27: INVALID
            Ok(MipsOp::VfpuUnknown {
                opcode: word,
                pc: vaddr,
            })
        }
        7 => {
            // Matrix unary: vmmov, vmidt, vmzero, vmone, vrot
            let sub21 = (word >> 21) & 3;
            let sub16 = (word >> 16) & 0x1F;
            match sub21 {
                0 => {
                    let mat_op = match sub16 {
                        0 => VfpuMatOp::Mmov,
                        3 => VfpuMatOp::Midt,
                        6 => VfpuMatOp::Mzero,
                        7 => VfpuMatOp::Mone,
                        _ => {
                            return Ok(MipsOp::VfpuUnknown {
                                opcode: word,
                                pc: vaddr,
                            });
                        }
                    };
                    Ok(MipsOp::VfpuMatUnary {
                        vd: d,
                        vs: s,
                        op: mat_op,
                        size: sz,
                        imm: 0,
                    })
                }
                1 => {
                    // vrot: imm5 = lower 5 bits of vt field (rotation control)
                    let rot_imm = ((word >> 16) & 0x1F) as u8;
                    Ok(MipsOp::VfpuMatUnary {
                        vd: d,
                        vs: s,
                        op: VfpuMatOp::Vrot,
                        size: sz,
                        imm: rot_imm,
                    })
                }
                _ => Ok(MipsOp::VfpuUnknown {
                    opcode: word,
                    pc: vaddr,
                }),
            }
        }
        _ => Ok(MipsOp::VfpuUnknown {
            opcode: word,
            pc: vaddr,
        }),
    }
}

// -------------------------------------------------------------------------
// Memory operations
// -------------------------------------------------------------------------

/// Decode lv.s (opcode 0x32): load VFPU single.
///
/// Non-standard register field: `vt = ((op>>16)&0x1F) | ((op&3)<<5)`.
/// Offset: sign-extend of bits 15:2 shifted left by 2.
pub(crate) fn decode_lv_s(
    word: u32,
    _vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let vt_field =
        (((word >> 16) & 0x1F) | ((word & 3) << 5)) as u8;
    let rs = mips_rs(word);
    let offset = (word & 0xFFFC) as i16;
    Ok(MipsOp::VfpuLvS {
        vt: vt_field,
        rs,
        offset,
    })
}

/// Decode sv.s (opcode 0x3A): store VFPU single.
pub(crate) fn decode_sv_s(
    word: u32,
    _vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let vt_field =
        (((word >> 16) & 0x1F) | ((word & 3) << 5)) as u8;
    let rs = mips_rs(word);
    let offset = (word & 0xFFFC) as i16;
    Ok(MipsOp::VfpuSvS {
        vt: vt_field,
        rs,
        offset,
    })
}

/// Decode lv.q (opcode 0x36): load VFPU quad.
///
/// Register field: `vt = ((op>>16)&0x1F) | ((op&1)<<5)`.
pub(crate) fn decode_lv_q(
    word: u32,
    _vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let vt_field =
        (((word >> 16) & 0x1F) | ((word & 1) << 5)) as u8;
    let rs = mips_rs(word);
    let offset = (word & 0xFFFC) as i16;
    Ok(MipsOp::VfpuLvQ {
        vt: vt_field,
        rs,
        offset,
    })
}

/// Decode sv.q (opcode 0x3E): store VFPU quad.
pub(crate) fn decode_sv_q(
    word: u32,
    _vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let vt_field =
        (((word >> 16) & 0x1F) | ((word & 1) << 5)) as u8;
    let rs = mips_rs(word);
    let offset = (word & 0xFFFC) as i16;
    Ok(MipsOp::VfpuSvQ {
        vt: vt_field,
        rs,
        offset,
    })
}

/// Decode lvl.q/lvr.q (opcode 0x35): unaligned quad load.
///
/// Bit 1 distinguishes left(0) vs right(1).
pub(crate) fn decode_lvlr_q(
    word: u32,
    _vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let vt_field =
        (((word >> 16) & 0x1F) | ((word & 1) << 5)) as u8;
    let rs = mips_rs(word);
    let offset = (word & 0xFFFC) as i16;
    let is_right = (word >> 1) & 1 != 0;
    if is_right {
        Ok(MipsOp::VfpuLvrQ {
            vt: vt_field,
            rs,
            offset,
        })
    } else {
        Ok(MipsOp::VfpuLvlQ {
            vt: vt_field,
            rs,
            offset,
        })
    }
}

/// Decode svl.q/svr.q (opcode 0x3D): unaligned quad store.
///
/// Bit 1 distinguishes left(0) vs right(1).
pub(crate) fn decode_svlr_q(
    word: u32,
    _vaddr: u32,
) -> Result<MipsOp, DecodeError> {
    let vt_field =
        (((word >> 16) & 0x1F) | ((word & 1) << 5)) as u8;
    let rs = mips_rs(word);
    let offset = (word & 0xFFFC) as i16;
    let is_right = (word >> 1) & 1 != 0;
    if is_right {
        Ok(MipsOp::VfpuSvrQ {
            vt: vt_field,
            rs,
            offset,
        })
    } else {
        Ok(MipsOp::VfpuSvlQ {
            vt: vt_field,
            rs,
            offset,
        })
    }
}

// -------------------------------------------------------------------------
// Tests
// -------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;
    use psp_ir::MipsOp;

    /// Encode a VFPU0 instruction (opcode 0x18).
    fn encode_vfpu0(sub: u8, vd: u8, vs: u8, vt: u8, sz: u8) -> u32 {
        let (b7, b15) = size_bits(sz);
        (0x18u32 << 26)
            | ((sub as u32) << 23)
            | ((vt as u32 & 0x7F) << 16)
            | (b15 << 15)
            | ((vs as u32 & 0x7F) << 8)
            | (b7 << 7)
            | (vd as u32 & 0x7F)
    }

    /// Encode a VFPU5 instruction (opcode 0x37).
    fn encode_vfpu5(sub: u8, data: u32) -> u32 {
        (0x37u32 << 26) | ((sub as u32) << 23) | (data & 0x007F_FFFF)
    }

    /// Encode a lv.s instruction (opcode 0x32).
    fn encode_lv_s(vt: u8, rs: u8, offset: i16) -> u32 {
        let vt_low5 = (vt & 0x1F) as u32;
        let vt_hi2 = ((vt >> 5) & 3) as u32;
        (0x32u32 << 26)
            | ((rs as u32) << 21)
            | (vt_low5 << 16)
            | ((offset as u16 as u32) & 0xFFFC)
            | vt_hi2
    }

    /// Encode a COP2 MFV instruction (opcode 0x12, rs=0x00).
    fn encode_cop2_mfv(rt: u8, vd: u8) -> u32 {
        (0x12u32 << 26)
            | (0x00u32 << 21)
            | ((rt as u32) << 16)
            | ((vd as u32 & 0x7F) << 11)
    }

    /// Encode a VFPU3 vcmp instruction (opcode 0x1B).
    fn encode_vfpu3_vcmp(
        vs: u8,
        vt: u8,
        cond: u8,
        sz: u8,
    ) -> u32 {
        let (b7, b15) = size_bits(sz);
        (0x1Bu32 << 26)
            | (0u32 << 23) // sub=0 for vcmp
            | ((vt as u32 & 0x7F) << 16)
            | (b15 << 15)
            | ((vs as u32 & 0x7F) << 8)
            | (b7 << 7)
            | (cond as u32 & 0xF)
    }

    /// Convert vector size (1-4) to (bit7, bit15) values.
    fn size_bits(sz: u8) -> (u32, u32) {
        // size = 1 + b7 + (b15 << 1)
        let s = (sz - 1) as u32;
        let b7 = s & 1;
        let b15 = (s >> 1) & 1;
        (b7, b15)
    }

    #[test]
    fn test_vec_size_helper() {
        // size=1: b7=0, b15=0
        assert_eq!(vec_size(0x0000_0000), 1);
        // size=2: b7=1, b15=0 => bit 7 set
        assert_eq!(vec_size(0x0000_0080), 2);
        // size=3: b7=0, b15=1 => bit 15 set
        assert_eq!(vec_size(0x0000_8000), 3);
        // size=4: b7=1, b15=1 => both set
        assert_eq!(vec_size(0x0000_8080), 4);
    }

    #[test]
    fn test_decode_vfpu0_vadd_q() {
        // vadd.q with sub=0, vd=0x10, vs=0x20, vt=0x30, size=4
        let word = encode_vfpu0(0, 0x10, 0x20, 0x30, 4);
        let op = decode_vfpu0(word, 0x08800000).unwrap();
        match op {
            MipsOp::VfpuAdd {
                vd, vs, vt, size, ..
            } => {
                assert_eq!(vd, 0x10);
                assert_eq!(vs, 0x20);
                assert_eq!(vt, 0x30);
                assert_eq!(size, 4);
            }
            _ => panic!("Expected VfpuAdd, got {op:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu5_vpfxs() {
        // vpfxs with data=0x000F0E4
        let word = encode_vfpu5(0, 0x000F_00E4);
        let op = decode_vfpu5(word, 0x08800000).unwrap();
        match op {
            MipsOp::VfpuPrefix { reg_idx, data } => {
                assert_eq!(reg_idx, 0, "vpfxs = reg_idx 0");
                assert_eq!(data, 0x000F_00E4 & 0x000F_FFFF);
            }
            _ => panic!("Expected VfpuPrefix, got {op:?}"),
        }
    }

    #[test]
    fn test_decode_lv_s_field_extraction() {
        // lv.s vt=0x25 (5 low bits=0x05, high 2 bits=0x01),
        // rs=4, offset=0x0040
        let word = encode_lv_s(0x25, 4, 0x0040);
        let op = decode_lv_s(word, 0x08800000).unwrap();
        match op {
            MipsOp::VfpuLvS { vt, rs, offset } => {
                assert_eq!(vt, 0x25, "vt should be 0x25");
                assert_eq!(rs, Reg::Gpr(4));
                assert_eq!(offset, 0x0040);
            }
            _ => panic!("Expected VfpuLvS, got {op:?}"),
        }
    }

    #[test]
    fn test_decode_cop2_mfv() {
        // mfv rt=8, vd=3
        let word = encode_cop2_mfv(8, 3);
        let op = decode_cop2(word, 0x08800000).unwrap();
        match op {
            MipsOp::VfpuMfv { rt, vd } => {
                assert_eq!(rt, Reg::Gpr(8));
                assert_eq!(vd, 3);
            }
            _ => panic!("Expected VfpuMfv, got {op:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu3_vcmp() {
        // vcmp with cond=2 (EQ), vs=0x10, vt=0x20, size=4
        let word =
            encode_vfpu3_vcmp(0x10, 0x20, 2, 4);
        let op = decode_vfpu3(word, 0x08800000).unwrap();
        match op {
            MipsOp::VfpuCmp {
                vs, vt, cond, size,
            } => {
                assert_eq!(vs, 0x10);
                assert_eq!(vt, 0x20);
                assert_eq!(cond, 2);
                assert_eq!(size, 4);
            }
            _ => panic!("Expected VfpuCmp, got {op:?}"),
        }
    }

    #[test]
    fn test_vfpu_branch_likely() {
        // COP2 branch: bvt with likely=true
        // opcode=0x12, rs=0x08, bit16=1(true), bit17=1(likely),
        // cc=2
        let word = (0x12u32 << 26)
            | (0x08u32 << 21)
            | (2u32 << 18)      // cc=2
            | (1u32 << 17)      // likely
            | (1u32 << 16)      // true branch
            | (4u16 as u32);    // offset=4
        let op = decode_cop2(word, 0x08800000).unwrap();
        match op {
            MipsOp::VfpuBvt {
                cc,
                target,
                likely,
            } => {
                assert_eq!(cc, 2);
                assert!(likely, "should be likely");
                // target = 0x08800000 + 4 + 4*4 = 0x08800014
                assert_eq!(target, 0x08800014);
            }
            _ => panic!("Expected VfpuBvt, got {op:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu4_type0_vrcp() {
        // vrcp: VFPU4Jump dispatch=0, tableVFPU4[16]
        // 0xD0102020: bits25:21=0, bits20:16=0x10(16)=vrcp
        let op = decode_vfpu4(0xD0102020, 0x08857CBC).unwrap();
        match op {
            MipsOp::VfpuUnary { op, .. } => {
                assert_eq!(op, VfpuUnaryOp::Rcp, "sub=16 should be vrcp");
            }
            _ => panic!("Expected VfpuUnary(Rcp), got {op:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu4_type0_vrsq() {
        // vrsq: VFPU4Jump dispatch=0, tableVFPU4[17]
        // 0xD0110808: bits25:21=0, bits20:16=0x11(17)=vrsq
        let op = decode_vfpu4(0xD0110808, 0x08857E90).unwrap();
        match op {
            MipsOp::VfpuUnary { op, .. } => {
                assert_eq!(op, VfpuUnaryOp::Rsq, "sub=17 should be vrsq");
            }
            _ => panic!("Expected VfpuUnary(Rsq), got {op:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu6_vcrsp() {
        // vcrsp/vqmul: VFPU6 bits25:23=5
        // 0xF2A68224: top6=0x3C, bits25:21=0x15, primary=5
        let op = decode_vfpu6(0xF2A68224, 0x08857EA0).unwrap();
        match op {
            MipsOp::VfpuCrsp { .. } => {}
            _ => panic!("Expected VfpuCrsp, got {op:?}"),
        }
    }

    // ---------------------------------------------------------------------
    // VFPU6 vtfm/vmscl slot-rotation + vtfm/vhtfm size-split regression
    // (issue #27/#67). The vtfm/vmscl dispatch block was rotated by one
    // slot vs PPSSPP `tableVFPU6` (sub=1->vmscl, 2->vtfm2, 3->vtfm3,
    // 4->vtfm4) so the engine matrix builder emitted ZERO plain vtfm and
    // ZERO vmscl game-wide -- every real vtfm decoded as vhtfm. Words are
    // captured from the live Patapon image (relocated segment in
    // analysis.json); see `.planning/research/pin-verification.md`.
    // ---------------------------------------------------------------------

    #[test]
    fn test_decode_vfpu6_vmmul_unchanged() {
        // 0xF02488A0 @ 0x088337A8 (sub=0). Brackets the rotated block.
        match decode_vfpu6(0xF02488A0, 0x088337A8).unwrap() {
            MipsOp::VfpuMmul { vd, vs, vt, size } => {
                assert_eq!((vd, vs, vt, size), (0x20, 0x08, 0x24, 4));
            }
            other => panic!("expected vmmul, got {other:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu6_vtfm2() {
        // 0xF0914991 @ 0x089FA424 (sub=1, n=2, n==ins+1 -> vtfm2).
        // Pre-fix this decoded as vmscl (the rotation).
        match decode_vfpu6(0xF0914991, 0x089FA424).unwrap() {
            MipsOp::VfpuTfm { vd, vs, vt, size } => {
                assert_eq!((vd, vs, vt, size), (0x11, 0x49, 0x11, 2));
            }
            other => panic!("expected vtfm2, got {other:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu6_vtfm3() {
        // 0xF108A400 @ 0x08855468 (sub=2, n=3, n==ins+1 -> vtfm3).
        // Pre-fix this decoded as vhtfm2.
        match decode_vfpu6(0xF108A400, 0x08855468).unwrap() {
            MipsOp::VfpuTfm { vd, vs, vt, size } => {
                assert_eq!((vd, vs, vt, size), (0x00, 0x24, 0x08, 3));
            }
            other => panic!("expected vtfm3, got {other:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu6_vtfm4() {
        // 0xF18BA483 @ 0x08834144 (sub=3, n=4, n==ins+1 -> vtfm4).
        // Pre-fix this decoded as vhtfm3 -- the exact builder mis-route
        // behind the WORLD -90 deg fingerprint.
        match decode_vfpu6(0xF18BA483, 0x08834144).unwrap() {
            MipsOp::VfpuTfm { vd, vs, vt, size } => {
                assert_eq!((vd, vs, vt, size), (0x03, 0x24, 0x0B, 4));
            }
            other => panic!("expected vtfm4, got {other:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu6_vhtfm3_size_split() {
        // 0xF180AC01 @ 0x08857B8C (sub=3, n=3, n==ins -> vhtfm3). A
        // GENUINE homogeneous transform: the size-split keeps it vhtfm
        // while its sibling 0xF18BA483 (n=4) becomes vtfm4. This is the
        // real builder word that fired the live "vhtfm3" emit.
        match decode_vfpu6(0xF180AC01, 0x08857B8C).unwrap() {
            MipsOp::VfpuHtfm { vd, vs, vt, size } => {
                assert_eq!((vd, vs, vt, size), (0x01, 0x2C, 0x00, 3));
            }
            other => panic!("expected vhtfm3, got {other:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu6_vmscl() {
        // 0xF208A4A0 @ 0x088554FC (sub=4 -> vmscl). Pre-fix this decoded
        // as vhtfm4; zero vmscl were emitted game-wide before the fix.
        match decode_vfpu6(0xF208A4A0, 0x088554FC).unwrap() {
            MipsOp::VfpuMscl { vd, vs, vt, size } => {
                assert_eq!((vd, vs, vt, size), (0x20, 0x24, 0x08, 4));
            }
            other => panic!("expected vmscl, got {other:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu5_vfim() {
        // vfim.s: VFPU5 bits25:23=7
        // 0xDFE6BC00: top6=0x37, sub=7
        let op = decode_vfpu5(0xDFE6BC00, 0x08857CEC).unwrap();
        match op {
            MipsOp::VfpuVfim { vt, imm } => {
                assert_eq!(imm, 0xBC00);
                // vt = bits 22:16 = 0x66
                assert_eq!(vt, 0x66);
            }
            _ => panic!("Expected VfpuVfim, got {op:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu5_vpfxt_correct_sub() {
        // vpfxt: PPSSPP sub=2, should map to reg_idx=1
        let word = encode_vfpu5(2, 0x000F_00E4);
        let op = decode_vfpu5(word, 0x08800000).unwrap();
        match op {
            MipsOp::VfpuPrefix { reg_idx, .. } => {
                assert_eq!(reg_idx, 1, "sub=2 should be vpfxt (reg_idx=1)");
            }
            _ => panic!("Expected VfpuPrefix, got {op:?}"),
        }
    }

    #[test]
    fn test_decode_vfpu5_vpfxd_correct_sub() {
        // vpfxd: PPSSPP sub=4, should map to reg_idx=2
        let word = encode_vfpu5(4, 0x0000_0ABC);
        let op = decode_vfpu5(word, 0x08800000).unwrap();
        match op {
            MipsOp::VfpuPrefix { reg_idx, data } => {
                assert_eq!(reg_idx, 2, "sub=4 should be vpfxd (reg_idx=2)");
                assert_eq!(data, 0x0ABC & 0x0FFF);
            }
            _ => panic!("Expected VfpuPrefix, got {op:?}"),
        }
    }

    #[test]
    fn test_decode_all_8_unknown_opcodes() {
        // Verify all 8 previously-unknown opcodes now decode
        let unknowns = [
            (0xD0102020u32, 0x08857CBCu32),
            (0xD0102222, 0x08857CD0),
            (0xD0100101, 0x08857CD4),
            (0xD0110808, 0x08857E90),
            (0xD0110888, 0x08857EB4),
            (0xF2A68224, 0x08857EA0),
            (0xF2A4A625, 0x08857EA4),
            (0xDFE6BC00, 0x08857CEC),
        ];
        for (word, pc) in unknowns {
            let result =
                crate::decode_word(word, pc);
            match result {
                Ok(MipsOp::VfpuUnknown { .. }) => {
                    panic!(
                        "Opcode 0x{word:08X} at 0x{pc:08X} still decodes as VfpuUnknown"
                    );
                }
                Ok(_) => {} // good, decoded to something
                Err(e) => {
                    panic!(
                        "Opcode 0x{word:08X} at 0x{pc:08X} produced decode error: {e:?}"
                    );
                }
            }
        }
    }

    // ---------------------------------------------------------------------
    // VFPU4Jump 5-bit dispatch regression tests (issue #27 view-matrix fix).
    //
    // The old 3-bit `bits[25:23]` dispatch collapsed VFPU4Jump idx 0-3 and
    // mis-split idx 16-31, so vcmov/vf2i*/vi2f/vcst/vsrt/vbfy decoded to the
    // wrong op. These pin the real Patapon sceGumLookAt words.
    // ---------------------------------------------------------------------

    fn unary_op(word: u32, pc: u32) -> VfpuUnaryOp {
        match decode_vfpu4(word, pc).unwrap() {
            MipsOp::VfpuUnary { op, .. } => op,
            other => panic!("0x{word:08X}: expected VfpuUnary, got {other:?}"),
        }
    }

    #[test]
    fn test_vfpu4_vcmov_not_vf2iz() {
        // 0xD2A06808 @ 0x08857E94 — the LookAt safe-normalize guard.
        // VFPU4Jump idx = bits[25:21] = 21 = vcmov (NOT vf2iz, which is
        // idx 17). The old decoder produced vf2iz here, unconditionally
        // re-zeroing the rsqrt scale and collapsing the view matrix.
        match decode_vfpu4(0xD2A06808, 0x08857E94).unwrap() {
            MipsOp::VfpuUnary { op, vd, vs, imm, .. } => {
                assert_eq!(op, VfpuUnaryOp::Cmov0, "must be vcmov");
                assert_eq!(vd, 0x08);
                assert_eq!(vs, 0x68);
                // imm packs imm3=(op>>16)&7=0 and tf=(op>>19)&1=0.
                assert_eq!(imm, 0, "imm3=0, tf=0");
            }
            other => panic!("expected vcmov, got {other:?}"),
        }
        // Second LookAt cmov: 0xD2A16828 @ 0x08857EBC -> vd=0x28, imm3=1.
        match decode_vfpu4(0xD2A16828, 0x08857EBC).unwrap() {
            MipsOp::VfpuUnary { op, vd, vs, imm, .. } => {
                assert_eq!(op, VfpuUnaryOp::Cmov0);
                assert_eq!(vd, 0x28);
                assert_eq!(vs, 0x68);
                assert_eq!(imm, 1, "imm3=1, tf=0");
            }
            other => panic!("expected vcmov, got {other:?}"),
        }
    }

    #[test]
    fn test_vfpu4_vf2i_family() {
        // Real Patapon words: idx 16/17/18/19 = vf2in/vf2iz/vf2iu/vf2id.
        assert_eq!(unary_op(0xD2000080, 0x08853F40), VfpuUnaryOp::Vf2in);
        assert_eq!(unary_op(0xD2378080, 0x08833680), VfpuUnaryOp::Vf2iz);
        assert_eq!(unary_op(0xD2400080, 0x08853F60), VfpuUnaryOp::Vf2iu);
        assert_eq!(unary_op(0xD2600080, 0x08853F00), VfpuUnaryOp::Vf2id);
    }

    #[test]
    fn test_vfpu4_vi2f_not_vf2in() {
        // 0xD29F8080 @ 0x088339C4 — idx 20 = vi2f (old decoder: vf2in).
        match decode_vfpu4(0xD29F8080, 0x088339C4).unwrap() {
            MipsOp::VfpuUnary { op, imm, .. } => {
                assert_eq!(op, VfpuUnaryOp::Vi2f);
                // imm5 = bits[20:16] = 0x1F (the int->float scale).
                assert_eq!(imm, 0x1F);
            }
            other => panic!("expected vi2f, got {other:?}"),
        }
    }

    #[test]
    fn test_vfpu4_vcst_not_vsat1() {
        // 0xD0650001 @ 0x088551C4 — idx 3 = vcst (old decoder: vsat1).
        match decode_vfpu4(0xD0650001, 0x088551C4).unwrap() {
            MipsOp::VfpuUnary { op, imm, .. } => {
                assert_eq!(op, VfpuUnaryOp::Vcst);
                // imm5 = bits[20:16] = constant index 5.
                assert_eq!(imm, 5);
            }
            other => panic!("expected vcst, got {other:?}"),
        }
    }

    #[test]
    fn test_vfpu4_vfpu9_group() {
        // idx 2 = VFPU9 sub-table on bits[20:16].
        // 0xD0468000 @ 0x088548E4 sub=6 -> vfad; 0xD0478000 sub=7 -> vavg;
        // 0xD0428080 @ 0x08857D64 sub=2 -> vbfy1.
        assert_eq!(unary_op(0xD0468000, 0x088548E4), VfpuUnaryOp::Vfad);
        assert_eq!(unary_op(0xD0478000, 0x08854908), VfpuUnaryOp::Vavg);
        assert_eq!(unary_op(0xD0428080, 0x08857D64), VfpuUnaryOp::Vbfy1);
        // vsrt3/vsrt4 (sub 8/9).
        assert_eq!(unary_op(0xD0488181, 0x0885681C), VfpuUnaryOp::Vsrt3);
        assert_eq!(unary_op(0xD0498081, 0x08856814), VfpuUnaryOp::Vsrt4);
    }

    #[test]
    fn test_vfpu4_vrsq_still_correct() {
        // Regression guard: idx 0 unary path unchanged.
        // 0xD0110808 -> vrsq (the rsqrt in the LookAt normalize).
        assert_eq!(unary_op(0xD0110808, 0x08857E90), VfpuUnaryOp::Rsq);
        // vrcp (sub 16) and vzero (sub 6).
        assert_eq!(unary_op(0xD0102020, 0x08857CBC), VfpuUnaryOp::Rcp);
        assert_eq!(unary_op(0xD0060068, 0x08857E88), VfpuUnaryOp::Vzero);
    }

    #[test]
    fn test_vfpu4_vocp_not_aliased_to_vsocp() {
        // idx 2 sub 4 = vocp (1-x, same size). We do not model vocp and
        // must NOT alias it to vsocp (which doubles the vector). It must
        // decode to VfpuUnknown rather than silently wrong math.
        // 0xD0442303 @ 0x088338FC: idx=2, sub=4.
        match decode_vfpu4(0xD0442303, 0x088338FC).unwrap() {
            MipsOp::VfpuUnknown { .. } => {}
            other => panic!("vocp must be VfpuUnknown, got {other:?}"),
        }
    }
}
