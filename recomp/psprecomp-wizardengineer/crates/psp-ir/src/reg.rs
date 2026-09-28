//! Register types for the PSP Allegrex MIPS32 IR.
//!
//! `Reg` distinguishes `Zero` from numbered GPRs, allowing exhaustive matching
//! without integer comparisons. `FpReg` and `VfpuReg` wrap FPU and VFPU registers.

use std::fmt;

/// General-purpose register for Allegrex MIPS32.
///
/// `Zero` is a distinct variant (not `Gpr(0)`) to enable suppression of writes
/// and substitution of literal `0` for reads without integer comparison.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Reg {
    /// The hardwired-zero register ($zero, GPR 0).
    Zero,
    /// Numbered general-purpose register (values 1–31).
    Gpr(u8),
    /// HI accumulator register.
    HI,
    /// LO accumulator register.
    LO,
}

impl Reg {
    /// Returns the register number (0 for Zero, inner value for Gpr).
    ///
    /// # Panics
    ///
    /// Panics if called on `HI` or `LO` (accumulator registers have no GPR number).
    pub fn num(self) -> u8 {
        match self {
            Reg::Zero => 0,
            Reg::Gpr(n) => n,
            Reg::HI | Reg::LO => panic!("accumulator registers have no GPR number"),
        }
    }

    /// Constructs a `Reg` from a raw register number.
    ///
    /// `n == 0` returns `Reg::Zero`; all other values return `Reg::Gpr(n)`.
    pub fn from_u8(n: u8) -> Reg {
        if n == 0 { Reg::Zero } else { Reg::Gpr(n) }
    }
}

impl fmt::Display for Reg {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Reg::Zero => write!(f, "0"),
            Reg::Gpr(n) => write!(f, "ctx->r[{n}]"),
            Reg::HI => write!(f, "ctx->hi"),
            Reg::LO => write!(f, "ctx->lo"),
        }
    }
}

/// FPU register ($f0–$f31).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct FpReg(pub u8);

impl fmt::Display for FpReg {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "ctx->f[{}]", self.0)
    }
}

/// VFPU register (0–127).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct VfpuReg(pub u8);

impl fmt::Display for VfpuReg {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "ctx->vf[{}]", self.0)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_zero_is_distinct() {
        assert_eq!(Reg::from_u8(0), Reg::Zero);
        assert_ne!(Reg::from_u8(0), Reg::Gpr(0));
    }

    #[test]
    fn test_from_u8_nonzero() {
        assert_eq!(Reg::from_u8(1), Reg::Gpr(1));
        assert_eq!(Reg::from_u8(31), Reg::Gpr(31));
    }

    #[test]
    fn test_display_zero() {
        assert_eq!(format!("{}", Reg::Zero), "0");
    }

    #[test]
    fn test_display_gpr() {
        assert_eq!(format!("{}", Reg::Gpr(4)), "ctx->r[4]");
    }

    #[test]
    fn test_display_fpreg() {
        assert_eq!(format!("{}", FpReg(12)), "ctx->f[12]");
    }

    #[test]
    fn test_display_vfpureg() {
        assert_eq!(format!("{}", VfpuReg(3)), "ctx->vf[3]");
    }
}
