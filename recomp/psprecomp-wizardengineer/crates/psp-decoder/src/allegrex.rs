//! PSP Allegrex extension instruction decoders.
//!
//! Covers SPECIAL3 (opcode=0x1F: ext, ins, seb, seh, bitrev) and
//! ALLEGREX_SPECIAL2 (opcode=0x1C: madd, msub, max, min, clz, clo, wsbw, wsbh).
//!
//! Encoding reference: PPSSPP MIPSInt.cpp Int_Allegrex/Int_Allegrex2/Int_Special3.

use psp_ir::MipsOp;
use crate::{DecodeError, rs as reg_rs, rt as reg_rt, rd as reg_rd};

/// Decode a SPECIAL3 (opcode=0x1F) instruction word.
///
/// Covers: ext, ins, seb, seh, bitrev (and rotr/rotrv which are in decode_special).
pub(crate) fn decode_special3(word: u32, vaddr: u32) -> Result<MipsOp, DecodeError> {
    let func = word & 0x3F;
    let rs = reg_rs(word);
    let rt = reg_rt(word);

    match func {
        0x00 => {
            // EXT: extract bit field
            let pos = ((word >> 6) & 0x1F) as u8;
            let size_minus_1 = ((word >> 11) & 0x1F) as u8;
            Ok(MipsOp::Ext { rt, rs, pos, size: size_minus_1 + 1 })
        }
        0x04 => {
            // INS: insert bit field
            let pos = ((word >> 6) & 0x1F) as u8;
            let msb = ((word >> 11) & 0x1F) as u8;
            let size = msb - pos + 1;
            Ok(MipsOp::Ins { rt, rs, pos, size })
        }
        0x20 => {
            // BSHFL group: sub-opcode in bits[10:6]
            let sub = (word >> 6) & 0x1F;
            let rd = reg_rd(word);
            match sub {
                16 => Ok(MipsOp::Seb { rd, rt }),   // bits[10:6] == 16
                24 => Ok(MipsOp::Seh { rd, rt }),   // bits[10:6] == 24
                20 => Ok(MipsOp::Bitrev { rd, rt }), // bits[10:6] == 20
                _ => Err(DecodeError::Unknown(word, vaddr)),
            }
        }
        _ => Err(DecodeError::Unknown(word, vaddr)),
    }
}

/// Decode an ALLEGREX_SPECIAL2 (opcode=0x1C) instruction word.
///
/// Covers: madd, msub, max, min, clz, clo, wsbw, wsbh.
pub(crate) fn decode_allegrex_special2(word: u32, vaddr: u32) -> Result<MipsOp, DecodeError> {
    let func = word & 0x3F;
    let rs = reg_rs(word);
    let rt = reg_rt(word);
    let rd = reg_rd(word);

    match func {
        0x00 => Ok(MipsOp::Madd { rs, rt }),
        0x04 => Ok(MipsOp::Msub { rs, rt }),
        0x0C => Ok(MipsOp::Max { rd, rs, rt }),
        0x0D => Ok(MipsOp::Min { rd, rs, rt }),
        0x20 => Ok(MipsOp::Clz { rd, rs }),
        0x21 => Ok(MipsOp::Clo { rd, rs }),
        _ => {
            // WSBW / WSBH: match bottom 10 bits (bits 9:0)
            let bottom10 = word & 0x3FF;
            match bottom10 {
                0x0E0 => Ok(MipsOp::Wsbw { rd, rt }),
                0x0A0 => Ok(MipsOp::Wsbh { rd, rt }),
                _ => Err(DecodeError::Unknown(word, vaddr)),
            }
        }
    }
}
