//! Typed IR instruction set for the PSP Allegrex MIPS32 recompiler.
//!
//! `MipsOp` is a flat enum with one variant per Allegrex/FPU instruction.
//! All variants use named fields (never positional tuple variants).

use crate::reg::{FpReg, Reg};

/// VFPU unary operation sub-op discriminator for opcode 0x34 (VFPU4Jump).
///
/// Each variant maps to a unique VFPU unary instruction. The `imm` field
/// on `VfpuUnary` carries extra data for ops that need it (vf2in, vcmov,
/// vcst, vi2f, vwbn).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VfpuUnaryOp {
    Mov,
    Abs,
    Neg,
    Idt,
    Sat0,
    Sat1,
    Vzero,
    Vone,
    Rcp,
    Rsq,
    Sin,
    Cos,
    Exp2,
    Log2,
    Sqrt,
    Asin,
    Nrcp,
    Nsin,
    Rexp2,
    Cmov0,
    Cmov1,
    Wbn,
    Vf2in,
    Vf2iz,
    Vf2iu,
    Vf2id,
    Vi2f,
    Vi2uc,
    Vi2c,
    Vi2us,
    Vi2s,
    Vsrt1,
    Vsrt2,
    Vsrt3,
    Vsrt4,
    Vbfy1,
    Vbfy2,
    Vsocp,
    Vfad,
    Vavg,
    Vf2h,
    Vh2f,
    Vuc2i,
    Vc2i,
    Vus2i,
    Vs2i,
    Vrnds,
    Vrndi,
    Vrndf1,
    Vrndf2,
    Vcst,
}

/// VFPU matrix unary operation sub-op discriminator for opcode 0x3C (VFPU6).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VfpuMatOp {
    Mmov,
    Midt,
    Mzero,
    Mone,
    Vrot,
}

/// Flat typed instruction IR — one variant per PSP Allegrex instruction.
///
/// All field names match MIPS register conventions: `rd` (destination), `rs`/`rt` (sources),
/// `sa` (shift amount), `imm` (immediate).
///
/// Branch instructions store `target: u32` (absolute virtual address), not a raw `i16` offset.
/// The decoder resolves `target = branch_pc + 4 + sign_extend(offset)*4`.
#[derive(Debug, Clone)]
pub enum MipsOp {
    // -------------------------------------------------------------------------
    // MIPS32 Base — Shift (DECODE-01)
    // -------------------------------------------------------------------------
    /// Shift left logical: rd = rt << sa
    Sll { rd: Reg, rt: Reg, sa: u8 },
    /// Shift right logical: rd = rt >> sa (unsigned)
    Srl { rd: Reg, rt: Reg, sa: u8 },
    /// Shift right arithmetic: rd = rt >> sa (signed)
    Sra { rd: Reg, rt: Reg, sa: u8 },
    /// Shift left logical variable: rd = rt << (rs & 31)
    Sllv { rd: Reg, rt: Reg, rs: Reg },
    /// Shift right logical variable: rd = rt >> (rs & 31) (unsigned)
    Srlv { rd: Reg, rt: Reg, rs: Reg },
    /// Shift right arithmetic variable: rd = rt >> (rs & 31) (signed)
    Srav { rd: Reg, rt: Reg, rs: Reg },

    // -------------------------------------------------------------------------
    // MIPS32 Base — Jumps (DECODE-01)
    // -------------------------------------------------------------------------
    /// Jump register (return when rs == $ra).
    Jr { rs: Reg },
    /// Jump and link register: rd = PC+8; PC = rs
    Jalr { rd: Reg, rs: Reg },

    // -------------------------------------------------------------------------
    // MIPS32 Base — Traps (DECODE-01)
    // -------------------------------------------------------------------------
    /// System call with 20-bit code.
    Syscall { code: u32 },
    /// Break trap with 20-bit code.
    Break_ { code: u32 },

    // -------------------------------------------------------------------------
    // MIPS32 Base — HI/LO moves (DECODE-01)
    // -------------------------------------------------------------------------
    /// Move from HI accumulator: rd = HI
    Mfhi { rd: Reg },
    /// Move to HI accumulator: HI = rs
    Mthi { rs: Reg },
    /// Move from LO accumulator: rd = LO
    Mflo { rd: Reg },
    /// Move to LO accumulator: LO = rs
    Mtlo { rs: Reg },

    // -------------------------------------------------------------------------
    // MIPS32 Base — Multiply/Divide (DECODE-01)
    // -------------------------------------------------------------------------
    /// Signed multiply: HI:LO = rs * rt
    Mult { rs: Reg, rt: Reg },
    /// Unsigned multiply: HI:LO = rs * rt
    Multu { rs: Reg, rt: Reg },
    /// Signed divide: LO = rs / rt; HI = rs % rt
    Div { rs: Reg, rt: Reg },
    /// Unsigned divide: LO = rs / rt; HI = rs % rt
    Divu { rs: Reg, rt: Reg },

    // -------------------------------------------------------------------------
    // MIPS32 Base — R-type ALU (DECODE-01)
    // -------------------------------------------------------------------------
    /// Add (signed, trap on overflow): rd = rs + rt
    Add { rd: Reg, rs: Reg, rt: Reg },
    /// Add unsigned (no overflow trap): rd = rs + rt
    Addu { rd: Reg, rs: Reg, rt: Reg },
    /// Subtract (signed, trap on overflow): rd = rs - rt
    Sub { rd: Reg, rs: Reg, rt: Reg },
    /// Subtract unsigned (no overflow trap): rd = rs - rt
    Subu { rd: Reg, rs: Reg, rt: Reg },
    /// Bitwise AND: rd = rs & rt
    And { rd: Reg, rs: Reg, rt: Reg },
    /// Bitwise OR: rd = rs | rt
    Or { rd: Reg, rs: Reg, rt: Reg },
    /// Bitwise XOR: rd = rs ^ rt
    Xor { rd: Reg, rs: Reg, rt: Reg },
    /// Bitwise NOR: rd = ~(rs | rt)
    Nor { rd: Reg, rs: Reg, rt: Reg },
    /// Set if less than (signed): rd = (rs < rt) ? 1 : 0
    Slt { rd: Reg, rs: Reg, rt: Reg },
    /// Set if less than unsigned: rd = (rs < rt) ? 1 : 0
    Sltu { rd: Reg, rs: Reg, rt: Reg },
    /// Conditional move if zero: rd = rs if rt == 0
    Movz { rd: Reg, rs: Reg, rt: Reg },
    /// Conditional move if not zero: rd = rs if rt != 0
    Movn { rd: Reg, rs: Reg, rt: Reg },
    /// Multiply to register (no HI/LO): rd = rs * rt (low 32 bits)
    Mul { rd: Reg, rs: Reg, rt: Reg },
    /// Double-word add unsigned (treated as 32-bit addu on Allegrex)
    Daddu { rd: Reg, rs: Reg, rt: Reg },
    /// Double-word add (treated as 32-bit add on Allegrex)
    Dadd { rd: Reg, rs: Reg, rt: Reg },
    /// Double-word subtract (treated as 32-bit sub on Allegrex)
    Dsub { rd: Reg, rs: Reg, rt: Reg },
    /// Double-word shift right logical variable (treated as 32-bit srlv on Allegrex)
    Dsrlv { rd: Reg, rt: Reg, rs: Reg },

    // -------------------------------------------------------------------------
    // MIPS32 Base — Branches (DECODE-01, DECODE-04, DECODE-05)
    // -------------------------------------------------------------------------
    /// Branch if rs < 0; `likely` controls delay slot semantics.
    Bltz { rs: Reg, target: u32, likely: bool },
    /// Branch if rs >= 0; `likely` controls delay slot semantics.
    Bgez { rs: Reg, target: u32, likely: bool },
    /// Branch if rs < 0 and link; `likely` controls delay slot semantics.
    Bltzal { rs: Reg, target: u32, likely: bool },
    /// Branch if rs >= 0 and link; `likely` controls delay slot semantics.
    Bgezal { rs: Reg, target: u32, likely: bool },

    /// Unconditional jump (J-type, 26-bit absolute target).
    J { target: u32 },
    /// Jump and link (J-type, 26-bit absolute target).
    Jal { target: u32 },

    /// Branch if rs == rt; `likely` controls delay slot semantics.
    Beq { rs: Reg, rt: Reg, target: u32, likely: bool },
    /// Branch if rs != rt; `likely` controls delay slot semantics.
    Bne { rs: Reg, rt: Reg, target: u32, likely: bool },
    /// Branch if rs <= 0; `likely` controls delay slot semantics.
    Blez { rs: Reg, target: u32, likely: bool },
    /// Branch if rs > 0; `likely` controls delay slot semantics.
    Bgtz { rs: Reg, target: u32, likely: bool },

    // -------------------------------------------------------------------------
    // MIPS32 Base — Immediate ALU (DECODE-01)
    // -------------------------------------------------------------------------
    /// Add immediate (signed, trap on overflow): rt = rs + imm
    Addi { rt: Reg, rs: Reg, imm: i16 },
    /// Add immediate unsigned (no overflow trap): rt = rs + imm
    Addiu { rt: Reg, rs: Reg, imm: i16 },
    /// Set if less than immediate (signed): rt = (rs < imm) ? 1 : 0
    Slti { rt: Reg, rs: Reg, imm: i16 },
    /// Set if less than immediate unsigned: rt = (rs < imm) ? 1 : 0
    Sltiu { rt: Reg, rs: Reg, imm: u16 },
    /// Bitwise AND immediate: rt = rs & imm
    Andi { rt: Reg, rs: Reg, imm: u16 },
    /// Bitwise OR immediate: rt = rs | imm
    Ori { rt: Reg, rs: Reg, imm: u16 },
    /// Bitwise XOR immediate: rt = rs ^ imm
    Xori { rt: Reg, rs: Reg, imm: u16 },
    /// Load upper immediate: rt = imm << 16
    Lui { rt: Reg, imm: u16 },

    // -------------------------------------------------------------------------
    // MIPS32 Base — Loads (DECODE-01)
    // -------------------------------------------------------------------------
    /// Load byte (signed): rt = MEM_B(rs + offset)
    Lb { rt: Reg, rs: Reg, offset: i16 },
    /// Load halfword (signed): rt = MEM_H(rs + offset)
    Lh { rt: Reg, rs: Reg, offset: i16 },
    /// Load word left (unaligned): rt = LWL(rs + offset)
    Lwl { rt: Reg, rs: Reg, offset: i16 },
    /// Load word: rt = MEM_W(rs + offset)
    Lw { rt: Reg, rs: Reg, offset: i16 },
    /// Load byte unsigned: rt = MEM_BU(rs + offset)
    Lbu { rt: Reg, rs: Reg, offset: i16 },
    /// Load halfword unsigned: rt = MEM_HU(rs + offset)
    Lhu { rt: Reg, rs: Reg, offset: i16 },
    /// Load word right (unaligned): rt = LWR(rs + offset)
    Lwr { rt: Reg, rs: Reg, offset: i16 },

    // -------------------------------------------------------------------------
    // MIPS32 Base — Stores (DECODE-01)
    // -------------------------------------------------------------------------
    /// Store byte: MEM_B(rs + offset) = rt
    Sb { rt: Reg, rs: Reg, offset: i16 },
    /// Store halfword: MEM_H(rs + offset) = rt
    Sh { rt: Reg, rs: Reg, offset: i16 },
    /// Store word left (unaligned): SWL(rs + offset) = rt
    Swl { rt: Reg, rs: Reg, offset: i16 },
    /// Store word: MEM_W(rs + offset) = rt
    Sw { rt: Reg, rs: Reg, offset: i16 },
    /// Store word right (unaligned): SWR(rs + offset) = rt
    Swr { rt: Reg, rs: Reg, offset: i16 },

    // -------------------------------------------------------------------------
    // MIPS32 Base — Misc (DECODE-01)
    // -------------------------------------------------------------------------
    /// Cache operation hint.
    Cache { op: u8, rs: Reg, offset: i16 },
    /// Sync (memory barrier).
    Sync {},
    /// No-operation (canonical encoding: sll $zero, $zero, 0).
    Nop {},

    // -------------------------------------------------------------------------
    // Allegrex Extensions (DECODE-02)
    // -------------------------------------------------------------------------
    /// Extract bit field: rt = rs[pos + size - 1 : pos]
    Ext { rt: Reg, rs: Reg, pos: u8, size: u8 },
    /// Insert bit field: rt[pos + size - 1 : pos] = rs[size - 1 : 0]
    Ins { rt: Reg, rs: Reg, pos: u8, size: u8 },
    /// Word swap bytes within halfwords: swap bytes in each 16-bit half of rt
    Wsbh { rd: Reg, rt: Reg },
    /// Word swap bytes within words: reverse byte order in rt
    Wsbw { rd: Reg, rt: Reg },
    /// Sign-extend byte: rd = sign_extend(rt[7:0])
    Seb { rd: Reg, rt: Reg },
    /// Sign-extend halfword: rd = sign_extend(rt[15:0])
    Seh { rd: Reg, rt: Reg },
    /// Bit reverse: rd = bit_reverse(rt)
    Bitrev { rd: Reg, rt: Reg },
    /// Count leading zeros: rd = clz(rs)
    Clz { rd: Reg, rs: Reg },
    /// Count leading ones: rd = clo(rs)
    Clo { rd: Reg, rs: Reg },
    /// Signed minimum: rd = min(rs, rt)
    Min { rd: Reg, rs: Reg, rt: Reg },
    /// Signed maximum: rd = max(rs, rt)
    Max { rd: Reg, rs: Reg, rt: Reg },
    /// Multiply-add (signed): HI:LO += rs * rt
    Madd { rs: Reg, rt: Reg },
    /// Multiply-subtract (signed): HI:LO -= rs * rt
    Msub { rs: Reg, rt: Reg },
    /// Rotate right: rd = rotr(rt, sa)
    Rotr { rd: Reg, rt: Reg, sa: u8 },
    /// Rotate right variable: rd = rotr(rt, rs & 31)
    Rotrv { rd: Reg, rt: Reg, rs: Reg },
    /// Move from COP0: rt = COP0[rd]
    Mfc0 { rt: Reg, rd: u8 },
    /// Move to COP0: COP0[rd] = rt
    Mtc0 { rt: Reg, rd: u8 },

    // -------------------------------------------------------------------------
    // FPU — COP1 Single-Precision (DECODE-03)
    // -------------------------------------------------------------------------
    /// FPU add: fd = fs + ft
    AddS { fd: FpReg, fs: FpReg, ft: FpReg },
    /// FPU subtract: fd = fs - ft
    SubS { fd: FpReg, fs: FpReg, ft: FpReg },
    /// FPU multiply: fd = fs * ft
    MulS { fd: FpReg, fs: FpReg, ft: FpReg },
    /// FPU divide: fd = fs / ft
    DivS { fd: FpReg, fs: FpReg, ft: FpReg },
    /// FPU square root: fd = sqrt(fs)
    SqrtS { fd: FpReg, fs: FpReg },
    /// FPU absolute value: fd = |fs|
    AbsS { fd: FpReg, fs: FpReg },
    /// FPU move: fd = fs
    MovS { fd: FpReg, fs: FpReg },
    /// FPU negate: fd = -fs
    NegS { fd: FpReg, fs: FpReg },
    /// Convert integer to float: fd = (float)fs
    CvtSW { fd: FpReg, fs: FpReg },
    /// Convert float to integer (truncate): fd = (int)fs
    CvtWS { fd: FpReg, fs: FpReg },
    /// Truncate float to integer: fd = trunc(fs) (always truncates toward zero)
    TruncWS { fd: FpReg, fs: FpReg },
    /// FPU compare condition: fpu_cc = compare(fs, ft) with given condition code.
    CCond { cond: u8, fs: FpReg, ft: FpReg },
    /// Branch if FPU condition code is true; `likely` controls delay slot.
    Bc1t { target: u32, likely: bool },
    /// Branch if FPU condition code is false; `likely` controls delay slot.
    Bc1f { target: u32, likely: bool },
    /// Move from FPU: rt = fs (integer view)
    Mfc1 { rt: Reg, fs: FpReg },
    /// Move to FPU: fs = rt (integer view)
    Mtc1 { rt: Reg, fs: FpReg },
    /// Load word to FPU: ft = MEM_W(rs + offset)
    Lwc1 { ft: FpReg, rs: Reg, offset: i16 },
    /// Store word from FPU: MEM_W(rs + offset) = ft
    Swc1 { ft: FpReg, rs: Reg, offset: i16 },

    // -------------------------------------------------------------------------
    // VFPU — COP2 control/move (opcode 0x12)
    // -------------------------------------------------------------------------
    /// Move from VFPU single register to GPR.
    VfpuMfv { rt: Reg, vd: u8 },
    /// Move to VFPU single register from GPR.
    VfpuMtv { rt: Reg, vd: u8 },
    /// Branch if VFPU condition false.
    VfpuBvf { cc: u8, target: u32, likely: bool },
    /// Branch if VFPU condition true.
    VfpuBvt { cc: u8, target: u32, likely: bool },
    /// Move from VFPU control register.
    VfpuMfvc { rt: Reg, imm: u8 },
    /// Move to VFPU control register.
    VfpuMtvc { rt: Reg, imm: u8 },

    // -------------------------------------------------------------------------
    // VFPU — Prefix (opcode 0x37 sub-ops 0-2)
    // -------------------------------------------------------------------------
    /// VFPU prefix instruction: vpfxs(0), vpfxt(1), vpfxd(2).
    VfpuPrefix { reg_idx: u8, data: u32 },

    // -------------------------------------------------------------------------
    // VFPU — Immediate loads (opcode 0x37 sub-ops 3-4)
    // -------------------------------------------------------------------------
    /// Load integer immediate to VFPU register.
    VfpuViim { vt: u8, imm: i16 },
    /// Load half-float immediate to VFPU register.
    VfpuVfim { vt: u8, imm: u16 },

    // -------------------------------------------------------------------------
    // VFPU — Arithmetic (VFPU0 group, opcode 0x18)
    // -------------------------------------------------------------------------
    /// VFPU vector add.
    VfpuAdd { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU vector subtract.
    VfpuSub { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU vector divide.
    VfpuDiv { vd: u8, vs: u8, vt: u8, size: u8 },

    // -------------------------------------------------------------------------
    // VFPU — Arithmetic (VFPU1 group, opcode 0x19)
    // -------------------------------------------------------------------------
    /// VFPU vector multiply.
    VfpuMul { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU vector dot product.
    VfpuDot { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU vector scale.
    VfpuScl { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU homogeneous dot product.
    VfpuHdp { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU cross product (triple only).
    VfpuCrs { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU determinant (pair only).
    VfpuDet { vd: u8, vs: u8, vt: u8, size: u8 },

    // -------------------------------------------------------------------------
    // VFPU — Compare/minmax (VFPU3 group, opcode 0x1B)
    // -------------------------------------------------------------------------
    /// VFPU vector compare.
    VfpuCmp { vs: u8, vt: u8, cond: u8, size: u8 },
    /// VFPU vector minimum.
    VfpuVmin { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU vector maximum.
    VfpuVmax { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU scalar compare.
    VfpuScmp { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU set if greater or equal.
    VfpuSge { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU set if less than.
    VfpuSlt { vd: u8, vs: u8, vt: u8, size: u8 },

    // -------------------------------------------------------------------------
    // VFPU — Unary/trig (VFPU4Jump, opcode 0x34)
    // -------------------------------------------------------------------------
    /// VFPU unary operation with sub-op discriminator.
    VfpuUnary { vd: u8, vs: u8, op: VfpuUnaryOp, size: u8, imm: u8 },

    // -------------------------------------------------------------------------
    // VFPU — Matrix (VFPU6 group, opcode 0x3C)
    // -------------------------------------------------------------------------
    /// VFPU matrix multiply.
    VfpuMmul { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU matrix scale.
    VfpuMscl { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU transform (vtfm2/3/4, size determines which).
    VfpuTfm { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU homogeneous transform (vhtfm2/3/4).
    VfpuHtfm { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU cross product / quaternion multiply.
    VfpuCrsp { vd: u8, vs: u8, vt: u8, size: u8 },
    /// VFPU matrix unary operation.
    /// `imm` carries extra data for vrot (rotation control from vt field).
    VfpuMatUnary { vd: u8, vs: u8, op: VfpuMatOp, size: u8, imm: u8 },

    // -------------------------------------------------------------------------
    // VFPU — Memory
    // -------------------------------------------------------------------------
    /// Load VFPU single: lv.s.
    VfpuLvS { vt: u8, rs: Reg, offset: i16 },
    /// Store VFPU single: sv.s.
    VfpuSvS { vt: u8, rs: Reg, offset: i16 },
    /// Load VFPU quad: lv.q.
    VfpuLvQ { vt: u8, rs: Reg, offset: i16 },
    /// Store VFPU quad: sv.q.
    VfpuSvQ { vt: u8, rs: Reg, offset: i16 },
    /// Load VFPU quad unaligned left: lvl.q.
    VfpuLvlQ { vt: u8, rs: Reg, offset: i16 },
    /// Load VFPU quad unaligned right: lvr.q.
    VfpuLvrQ { vt: u8, rs: Reg, offset: i16 },
    /// Store VFPU quad unaligned left: svl.q.
    VfpuSvlQ { vt: u8, rs: Reg, offset: i16 },
    /// Store VFPU quad unaligned right: svr.q.
    VfpuSvrQ { vt: u8, rs: Reg, offset: i16 },

    // -------------------------------------------------------------------------
    // VFPU — Misc
    // -------------------------------------------------------------------------
    /// VFPU flush / nop / sync (all no-ops).
    VfpuFlush {},
    /// Unknown/undocumented VFPU sub-op (emits log-once runtime warning).
    VfpuUnknown { opcode: u32, pc: u32 },

    // -------------------------------------------------------------------------
    // Jump table (DECODE-06) — emitter uses this for C++ switch emission
    // -------------------------------------------------------------------------
    /// Jump table dispatch: switch(index_reg) with resolved case targets.
    JumpTable { index_reg: Reg, cases: Vec<u32> },

    // -------------------------------------------------------------------------
    // Delay slot wrapper (DECODE-04, DECODE-05)
    // -------------------------------------------------------------------------
    /// Wrapper for a branch-likely delay slot instruction (executed only if branch taken).
    DelaySlot { instr: Box<MipsOp> },

    /// Fused NON-likely branch/jump + hazardous delay slot (issue #12).
    ///
    /// Created by the decoder when the delay-slot instruction WRITES a register
    /// (or FPU/VFPU condition flag) that the branch/jump READS. Real MIPS
    /// evaluates the branch condition / captures the jump target BEFORE the
    /// delay slot executes, so the standard delay-slot-before-branch reorder
    /// would test the clobbered value (e.g. 0x089D73E0 `beq $t0,$at,..` with
    /// delay `addiu $t0,$v0,3`). The emitter snapshots the registers the branch
    /// reads, emits the delay slot exactly once (it executes on both the taken
    /// and fall-through paths), then branches/dispatches on the snapshot.
    ///
    /// The decoder pads this node with a following `Nop` so the positional
    /// word-to-IR mapping (label addresses, mid-entry dispatch) is preserved.
    BranchHazardDelay { branch: Box<MipsOp>, delay: Box<MipsOp> },

    /// Duplicated delay-slot instruction at its own word position (issue #56).
    ///
    /// Created by the decoder when a branch/delay pair's delay-slot ADDRESS is
    /// itself an in-function branch target ("branch into delay slot", BIDS —
    /// e.g. the SCE libc VFPU memset tail loops back into a `beq`'s delay
    /// slot). The pair is fused into `BranchHazardDelay` at the branch's
    /// position (taken/fall-through semantics), and this node occupies the
    /// delay slot's position so a back-edge that enters AT the delay-slot
    /// address executes the instruction — hardware runs it there as a normal
    /// instruction. The emitter lowers the pair as:
    ///
    /// ```text
    /// L_A:  { hazard block: snapshot; delay; if (cond) goto target; }
    ///       goto L_A8;     // fall-through must NOT re-execute the duplicate
    /// L_A4: <instr>        // direct entries get hardware semantics
    /// L_A8: ...
    /// ```
    ///
    /// Positional word-to-IR mapping (labels, mid-entry dispatch) is
    /// preserved: 2 ops at 2 word positions.
    DelaySlotRejoin { instr: Box<MipsOp> },

    // -------------------------------------------------------------------------
    // Relocation site markers (EMIT-10)
    // -------------------------------------------------------------------------
    /// Relocatable LUI instruction (high 16 bits from relocation table).
    RelocHi16 { rt: Reg, section_idx: u8, symbol_offset: u32 },
    /// Relocatable ADDIU/ORI instruction (low 16 bits from relocation table).
    RelocLo16 { rt: Reg, rs: Reg, section_idx: u8, symbol_offset: u32 },
    /// Relocatable J/JAL instruction (26-bit target from relocation table).
    RelocJ26 { target_section_idx: u8, symbol_offset: u32 },
}
