//! MIPS32 base instruction decoders for the PSP Allegrex.
//!
//! Covers all SPECIAL and REGIMM encodings. Invoked from `lib.rs` `decode_word`.

use psp_ir::{MipsOp, Reg};
use crate::{DecodeError, rs as reg_rs, rt as reg_rt, rd as reg_rd, branch_target};

/// Decode a SPECIAL (opcode=0x00) instruction word.
///
/// Dispatches on the `func` field (bits 5:0).
pub(crate) fn decode_special(word: u32, vaddr: u32) -> Result<MipsOp, DecodeError> {
    let func = word & 0x3F;
    let rs = reg_rs(word);
    let rt = reg_rt(word);
    let rd = reg_rd(word);
    let sa = ((word >> 6) & 0x1F) as u8;

    match func {
        0x00 => {
            // SLL — but entire word == 0 is canonical NOP
            if word == 0 {
                Ok(MipsOp::Nop {})
            } else {
                Ok(MipsOp::Sll { rd, rt, sa })
            }
        }
        0x02 => {
            // SRL or ROTR — rs field == 1 indicates ROTR
            if ((word >> 21) & 0x1F) == 1 {
                Ok(MipsOp::Rotr { rd, rt, sa })
            } else {
                Ok(MipsOp::Srl { rd, rt, sa })
            }
        }
        0x03 => Ok(MipsOp::Sra { rd, rt, sa }),
        0x04 => Ok(MipsOp::Sllv { rd, rt, rs }),
        0x06 => {
            // SRLV or ROTRV — sa field == 1 indicates ROTRV
            if sa == 1 {
                Ok(MipsOp::Rotrv { rd, rt, rs })
            } else {
                Ok(MipsOp::Srlv { rd, rt, rs })
            }
        }
        0x07 => Ok(MipsOp::Srav { rd, rt, rs }),
        0x08 => Ok(MipsOp::Jr { rs }),
        0x09 => {
            // JALR: rd defaults to $ra (31) if rd field is 0
            let link_reg = if matches!(rd, Reg::Zero) { Reg::Gpr(31) } else { rd };
            Ok(MipsOp::Jalr { rd: link_reg, rs })
        }
        0x0C => Ok(MipsOp::Syscall { code: (word >> 6) & 0xFFFFF }),
        0x0D => Ok(MipsOp::Break_ { code: (word >> 6) & 0xFFFFF }),
        0x0F => Ok(MipsOp::Sync {}),
        0x10 => Ok(MipsOp::Mfhi { rd }),
        0x11 => Ok(MipsOp::Mthi { rs }),
        0x12 => Ok(MipsOp::Mflo { rd }),
        0x13 => Ok(MipsOp::Mtlo { rs }),
        0x1C => Ok(MipsOp::Mul { rd, rs, rt }),
        0x18 => Ok(MipsOp::Mult { rs, rt }),
        0x19 => Ok(MipsOp::Multu { rs, rt }),
        0x1A => Ok(MipsOp::Div { rs, rt }),
        0x1B => Ok(MipsOp::Divu { rs, rt }),
        0x20 => Ok(MipsOp::Add { rd, rs, rt }),
        0x21 => Ok(MipsOp::Addu { rd, rs, rt }),
        0x22 => Ok(MipsOp::Sub { rd, rs, rt }),
        0x23 => Ok(MipsOp::Subu { rd, rs, rt }),
        0x24 => Ok(MipsOp::And { rd, rs, rt }),
        0x25 => Ok(MipsOp::Or { rd, rs, rt }),
        0x26 => Ok(MipsOp::Xor { rd, rs, rt }),
        0x27 => Ok(MipsOp::Nor { rd, rs, rt }),
        0x0A => Ok(MipsOp::Movz { rd, rs, rt }),
        0x0B => Ok(MipsOp::Movn { rd, rs, rt }),
        // Allegrex encodes CLZ/CLO in the SPECIAL group (op=0), NOT MIPS32r2's
        // SPECIAL2 (op=0x1C). func 0x16 = CLZ, func 0x17 = CLO, with the result in
        // `rd` and the source in `rs` (the 32-bit Allegrex has no 64-bit DSRLV).
        0x16 => Ok(MipsOp::Clz { rd, rs }),
        0x17 => Ok(MipsOp::Clo { rd, rs }),
        0x2A => Ok(MipsOp::Slt { rd, rs, rt }),
        0x2B => Ok(MipsOp::Sltu { rd, rs, rt }),
        // Allegrex reuses the MIPS-III 64-bit doubleword slots for its own
        // 32-bit ops (the Allegrex has no DADD/DADDU/DSUB):
        //   func 0x2C = MAX (rd = max(rs, rt))
        //   func 0x2D = MIN (rd = min(rs, rt))
        //   func 0x2E = MSUB (HI:LO -= rs * rt)
        0x2C => Ok(MipsOp::Max { rd, rs, rt }),
        0x2D => Ok(MipsOp::Min { rd, rs, rt }),
        0x2E => Ok(MipsOp::Msub { rs, rt }),
        _ => Err(DecodeError::Unknown(word, vaddr)),
    }
}

/// Decode a REGIMM (opcode=0x01) instruction word.
///
/// Dispatches on the `rt` field (bits 20:16) which acts as a sub-opcode.
pub(crate) fn decode_regimm(word: u32, vaddr: u32) -> Result<MipsOp, DecodeError> {
    let rt_field = (word >> 16) & 0x1F;
    let rs = reg_rs(word);
    let offset = (word & 0xFFFF) as i16;
    let target = branch_target(vaddr, offset);

    match rt_field {
        0x00 => Ok(MipsOp::Bltz { rs, target, likely: false }),
        0x01 => Ok(MipsOp::Bgez { rs, target, likely: false }),
        0x02 => Ok(MipsOp::Bltz { rs, target, likely: true }),
        0x03 => Ok(MipsOp::Bgez { rs, target, likely: true }),
        0x10 => Ok(MipsOp::Bltzal { rs, target, likely: false }),
        0x11 => Ok(MipsOp::Bgezal { rs, target, likely: false }),
        0x12 => Ok(MipsOp::Bltzal { rs, target, likely: true }),
        0x13 => Ok(MipsOp::Bgezal { rs, target, likely: true }),
        _ => Err(DecodeError::Unknown(word, vaddr)),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use psp_ir::Reg;

    /// Encode an R-type MIPS word: opcode=0, rs, rt, rd, sa, func.
    fn encode_r(rs: u8, rt: u8, rd: u8, sa: u8, func: u8) -> u32 {
        ((rs as u32) << 21) | ((rt as u32) << 16) | ((rd as u32) << 11)
            | ((sa as u32) << 6) | (func as u32)
    }

    /// Encode a REGIMM word: opcode=0x01, rs, rt_sub (sub-opcode), offset.
    fn encode_regimm(rs: u8, rt_sub: u8, offset: i16) -> u32 {
        (0x01u32 << 26) | ((rs as u32) << 21) | ((rt_sub as u32) << 16)
            | ((offset as u16) as u32)
    }

    #[test]
    fn test_decode_nop() {
        // Word == 0x00000000 is canonical NOP
        let op = decode_special(0x00000000, 0x08800000).unwrap();
        assert!(matches!(op, MipsOp::Nop {}));
    }

    #[test]
    fn test_decode_addu() {
        // addu $t0, $a0, $a1 -> rs=4, rt=5, rd=8, sa=0, func=0x21
        let word = encode_r(4, 5, 8, 0, 0x21);
        let op = decode_special(word, 0x08800000).unwrap();
        match op {
            MipsOp::Addu { rd, rs, rt } => {
                assert_eq!(rd, Reg::Gpr(8));
                assert_eq!(rs, Reg::Gpr(4));
                assert_eq!(rt, Reg::Gpr(5));
            }
            _ => panic!("Expected Addu, got {op:?}"),
        }
    }

    #[test]
    fn test_decode_srl_vs_rotr() {
        // SRL: func=0x02, rs=0 (rs field == 0 means srl)
        let srl_word = encode_r(0, 5, 8, 3, 0x02);
        let op = decode_special(srl_word, 0x08800000).unwrap();
        assert!(matches!(op, MipsOp::Srl { .. }), "Expected Srl, got {op:?}");

        // ROTR: func=0x02, rs=1 (rs field == 1 means rotr)
        let rotr_word = encode_r(1, 5, 8, 3, 0x02);
        let op = decode_special(rotr_word, 0x08800000).unwrap();
        assert!(matches!(op, MipsOp::Rotr { .. }), "Expected Rotr, got {op:?}");
    }

    #[test]
    fn test_decode_bltz_likely() {
        // REGIMM rt=0x02 → Bltz with likely=true (bltzl)
        // target = 0x08800000 + 4 + 100*4 = 0x08800194
        let word = encode_regimm(5, 0x02, 100);
        let op = decode_regimm(word, 0x08800000).unwrap();
        match op {
            MipsOp::Bltz { rs, target, likely } => {
                assert_eq!(rs, Reg::Gpr(5));
                assert_eq!(target, 0x08800194, "branch target = vaddr+4+offset*4");
                assert!(likely, "Expected likely=true for bltzl");
            }
            _ => panic!("Expected Bltz, got {op:?}"),
        }
    }
}
