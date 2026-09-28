//! FPU (COP1) single-precision instruction decoder for the PSP Allegrex.
//!
//! Covers MFC1, MTC1, BC1T, BC1F, and all single-precision arithmetic operations.
//! Reference: PPSSPP MIPSInt.cpp Int_FPU2op and Int_FPUComp.

use psp_ir::{FpReg, MipsOp};
use crate::{DecodeError, rt as reg_rt, branch_target};

/// Decode a COP1 (FPU, opcode=0x11) instruction word.
pub(crate) fn decode_cop1(word: u32, vaddr: u32) -> Result<MipsOp, DecodeError> {
    let rs_field = (word >> 21) & 0x1F;
    let rt = reg_rt(word);
    let ft = FpReg(((word >> 16) & 0x1F) as u8);
    let fs = FpReg(((word >> 11) & 0x1F) as u8);
    let fd = FpReg(((word >> 6) & 0x1F) as u8);

    match rs_field {
        0x00 => Ok(MipsOp::Mfc1 { rt, fs }),
        0x04 => Ok(MipsOp::Mtc1 { rt, fs }),
        0x08 => {
            // BC1: branch on FPU condition code
            let rt_field = (word >> 16) & 0x1F;
            let offset = (word & 0xFFFF) as i16;
            let target = branch_target(vaddr, offset);
            match rt_field {
                0 => Ok(MipsOp::Bc1f { target, likely: false }),
                1 => Ok(MipsOp::Bc1t { target, likely: false }),
                2 => Ok(MipsOp::Bc1f { target, likely: true }),
                3 => Ok(MipsOp::Bc1t { target, likely: true }),
                _ => Err(DecodeError::Unknown(word, vaddr)),
            }
        }
        0x10 => {
            // Single-precision (S) operations: match func field (bits 5:0)
            let func = word & 0x3F;
            match func {
                0x00 => Ok(MipsOp::AddS { fd, fs, ft }),
                0x01 => Ok(MipsOp::SubS { fd, fs, ft }),
                0x02 => Ok(MipsOp::MulS { fd, fs, ft }),
                0x03 => Ok(MipsOp::DivS { fd, fs, ft }),
                0x04 => Ok(MipsOp::SqrtS { fd, fs }),
                0x05 => Ok(MipsOp::AbsS { fd, fs }),
                0x06 => Ok(MipsOp::MovS { fd, fs }),
                0x07 => Ok(MipsOp::NegS { fd, fs }),
                0x0D => Ok(MipsOp::TruncWS { fd, fs }),
                0x20 => Ok(MipsOp::CvtSW { fd, fs }),
                0x24 => Ok(MipsOp::CvtWS { fd, fs }),
                // c.cond.s: func 0x30–0x3F; cond = lower 4 bits
                0x30..=0x3F => {
                    let cond = (func & 0x0F) as u8;
                    Ok(MipsOp::CCond { cond, fs, ft })
                }
                _ => Err(DecodeError::Unknown(word, vaddr)),
            }
        }
        0x14 => {
            // W-format: integer operations (cvt.s.w uses this path)
            let func = word & 0x3F;
            match func {
                0x20 => Ok(MipsOp::CvtSW { fd, fs }), // cvt.s.w from W format
                _ => Err(DecodeError::Unknown(word, vaddr)),
            }
        }
        _ => Err(DecodeError::Unknown(word, vaddr)),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use psp_ir::FpReg;

    /// Encode a COP1 S-format instruction word.
    /// opcode=0x11, rs=0x10 (S format), ft, fs, fd, func.
    fn encode_cop1_s(ft: u8, fs: u8, fd: u8, func: u8) -> u32 {
        (0x11u32 << 26) | (0x10u32 << 21) | ((ft as u32) << 16)
            | ((fs as u32) << 11) | ((fd as u32) << 6) | (func as u32)
    }

    /// Encode a COP1 BC1 instruction word.
    fn encode_bc1(rt_sub: u8, offset: i16) -> u32 {
        (0x11u32 << 26) | (0x08u32 << 21) | ((rt_sub as u32) << 16)
            | ((offset as u16) as u32)
    }

    #[test]
    fn test_decode_add_s() {
        // add.s fd=4, fs=6, ft=8
        let word = encode_cop1_s(8, 6, 4, 0x00);
        let op = decode_cop1(word, 0x08800000).unwrap();
        match op {
            MipsOp::AddS { fd, fs, ft } => {
                assert_eq!(fd, FpReg(4));
                assert_eq!(fs, FpReg(6));
                assert_eq!(ft, FpReg(8));
            }
            _ => panic!("Expected AddS, got {op:?}"),
        }
    }

    #[test]
    fn test_decode_cvt_w_s() {
        // cvt.w.s fd=2, fs=4
        let word = encode_cop1_s(0, 4, 2, 0x24);
        let op = decode_cop1(word, 0x08800000).unwrap();
        assert!(matches!(op, MipsOp::CvtWS { fd: FpReg(2), fs: FpReg(4) }),
            "Expected CvtWS{{fd:FpReg(2), fs:FpReg(4)}}, got {op:?}");
    }

    #[test]
    fn test_decode_bc1t() {
        // bc1t with offset=4 (rt_sub=1 means bc1t, not likely)
        // target = 0x08800000 + 4 + 4*4 = 0x08800014
        let word = encode_bc1(1, 4);
        let op = decode_cop1(word, 0x08800000).unwrap();
        match op {
            MipsOp::Bc1t { target, likely } => {
                assert_eq!(target, 0x08800014, "branch target = vaddr+4+offset*4");
                assert!(!likely, "bc1t should have likely=false");
            }
            _ => panic!("Expected Bc1t, got {op:?}"),
        }
    }

    #[test]
    fn test_decode_c_eq_s() {
        // c.eq.s: func = 0x30 + cond=2 = 0x32; eq is cond=2 per MIPS FPU spec
        let word = encode_cop1_s(8, 6, 0, 0x32);
        let op = decode_cop1(word, 0x08800000).unwrap();
        match op {
            MipsOp::CCond { cond, fs, ft } => {
                assert_eq!(cond, 2, "c.eq.s cond should be 2");
                assert_eq!(fs, FpReg(6));
                assert_eq!(ft, FpReg(8));
            }
            _ => panic!("Expected CCond, got {op:?}"),
        }
    }
}
