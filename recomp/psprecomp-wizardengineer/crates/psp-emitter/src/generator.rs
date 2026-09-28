//! Generator trait and TestGenerator for C++ code emission.
//!
//! `Generator` is the central abstraction enabling unit testing of code
//! emission without actual C++ compilation. `TestGenerator` captures all
//! emission calls as strings. `CppGenerator` writes real C++17.

use psp_ir::Reg;

/// Central emission interface — one method per C++ construct.
///
/// All `emit_gpr_write` implementations MUST be no-ops when `rd == Reg::Zero`.
/// All `emit_gpr_read` implementations MUST return `"0"` when `rs == Reg::Zero`.
pub trait Generator {
    /// Start a new function. Emits RECOMP_FUNC signature line + opening brace.
    fn emit_function_start(&mut self, cpp_name: &str, vaddr: u32);
    /// Close the function body.
    fn emit_function_end(&mut self);
    /// Write to GPR `rd`. MUST be a no-op if rd == Reg::Zero.
    fn emit_gpr_write(&mut self, rd: Reg, expr: &str);
    /// Read GPR `rs`. MUST return "0" if rs == Reg::Zero.
    fn emit_gpr_read(&self, rs: Reg) -> String;
    /// Write to FPU register (float field .fl).
    fn emit_fpr_write(&mut self, fd: psp_ir::FpReg, expr: &str);
    /// Read FPU register.
    fn emit_fpr_read(&self, fs: psp_ir::FpReg) -> String;
    /// Emit a C++ label for a branch target.
    fn emit_label(&mut self, label: &str);
    /// Emit goto for unconditional jump.
    fn emit_goto(&mut self, label: &str);
    /// Emit standard branch: condition + goto. Delay slot already emitted before this.
    fn emit_branch(&mut self, cond: &str, target_label: &str);
    /// Emit branch-likely: delay slot inside if-block.
    fn emit_branch_likely(&mut self, cond: &str, target_label: &str, delay_slot_cpp: &str);
    /// Emit a C++ switch for a jump table (DECODE-06).
    fn emit_switch(&mut self, index_expr: &str, cases: &[(u32, String)]);
    /// Direct call to a statically-known function.
    fn emit_call_direct(&mut self, cpp_name: &str);
    /// Indirect call via RECOMP_LOOKUP dispatch table.
    fn emit_call_lookup(&mut self, vaddr: u32);
    /// Indirect call via RECOMP_LOOKUP using a register value at runtime.
    fn emit_call_lookup_reg(&mut self, reg_expr: &str);
    /// Call to a NID import stub: RECOMP_LOOKUP at the stub address (the
    /// runtime binds the HLE handler there at psp_hle_init — issue #40);
    /// `stub_name` is emitted as a trailing comment only.
    fn emit_call_hle(&mut self, stub_addr: u32, stub_name: &str);
    /// Return from function.
    fn emit_return(&mut self);
    /// Emit a raw pre-formatted C++ statement (used for complex expressions).
    fn emit_raw(&mut self, cpp: &str);
    /// Update HI/LO registers (used by mult/div).
    fn emit_hilo_write(&mut self, hi_expr: &str, lo_expr: &str);
    /// Set FPU condition code.
    fn emit_fpu_cc_write(&mut self, expr: &str);
    /// Read FPU condition code.
    fn emit_fpu_cc_read(&self) -> String;
    /// Record a statically-known cross-function `RECOMP_LOOKUP` target.
    ///
    /// Called by emission paths that format `RECOMP_LOOKUP(0xADDR)` text via
    /// `emit_raw` (cross-function branch tails), so dispatch-coverage auditing
    /// sees every static target. `emit_call_lookup` implementations record
    /// their own target. Default is a no-op.
    fn note_static_lookup(&mut self, _vaddr: u32) {}
}

/// Test double for `Generator` — captures all emission calls as strings.
///
/// Use in unit tests to verify code generation without compiling C++.
#[derive(Debug, Default)]
pub struct TestGenerator {
    /// All emitted lines, in emission order.
    pub output: Vec<String>,
}

impl TestGenerator {
    /// Create a new, empty TestGenerator.
    pub fn new() -> Self {
        Self::default()
    }
}

impl Generator for TestGenerator {
    fn emit_function_start(&mut self, cpp_name: &str, vaddr: u32) {
        self.output.push(format!("FUNC_START:{cpp_name}:0x{vaddr:08X}"));
    }

    fn emit_function_end(&mut self) {
        self.output.push("FUNC_END".into());
    }

    fn emit_gpr_write(&mut self, rd: Reg, expr: &str) {
        if rd == Reg::Zero {
            return; // EMIT-12: suppress $zero writes
        }
        self.output.push(format!("{rd} = {expr};"));
    }

    fn emit_gpr_read(&self, rs: Reg) -> String {
        if rs == Reg::Zero {
            return "0".to_string(); // EMIT-12: $zero reads always return literal 0
        }
        format!("{rs}")
    }

    fn emit_fpr_write(&mut self, fd: psp_ir::FpReg, expr: &str) {
        self.output.push(format!("{fd} = {expr};"));
    }

    fn emit_fpr_read(&self, fs: psp_ir::FpReg) -> String {
        format!("{fs}")
    }

    fn emit_label(&mut self, label: &str) {
        self.output.push(format!("LABEL:{label}"));
    }

    fn emit_goto(&mut self, label: &str) {
        self.output.push(format!("goto {label};"));
    }

    fn emit_branch(&mut self, cond: &str, target_label: &str) {
        self.output.push(format!("if ({cond}) goto {target_label};"));
    }

    fn emit_branch_likely(&mut self, cond: &str, target_label: &str, delay_slot_cpp: &str) {
        self.output.push(format!(
            "if ({cond}) {{ {delay_slot_cpp} goto {target_label}; }}"
        ));
    }

    fn emit_switch(&mut self, index_expr: &str, cases: &[(u32, String)]) {
        self.output
            .push(format!("SWITCH:{index_expr}:{}cases", cases.len()));
    }

    fn emit_call_direct(&mut self, cpp_name: &str) {
        self.output.push(format!("CALL_DIRECT:{cpp_name}"));
    }

    fn emit_call_lookup(&mut self, vaddr: u32) {
        self.output.push(format!("CALL_LOOKUP:0x{vaddr:08X}"));
    }

    fn emit_call_lookup_reg(&mut self, reg_expr: &str) {
        self.output.push(format!("CALL_LOOKUP_REG:{reg_expr}"));
    }

    fn emit_call_hle(&mut self, stub_addr: u32, stub_name: &str) {
        self.output
            .push(format!("CALL_HLE:0x{stub_addr:08X}:{stub_name}"));
    }

    fn emit_return(&mut self) {
        self.output.push("return;".into());
    }

    fn emit_raw(&mut self, cpp: &str) {
        self.output.push(cpp.to_string());
    }

    fn emit_hilo_write(&mut self, hi_expr: &str, lo_expr: &str) {
        self.output.push(format!("HI={hi_expr};LO={lo_expr};"));
    }

    fn emit_fpu_cc_write(&mut self, expr: &str) {
        self.output.push(format!("fpu_cc={expr};"));
    }

    fn emit_fpu_cc_read(&self) -> String {
        "ctx->fpu_cc".into()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use psp_ir::Reg;

    #[test]
    fn zero_write_suppressed() {
        let mut gen = TestGenerator::new();
        gen.emit_gpr_write(Reg::Zero, "ctx->r[4] + 1");
        assert!(gen.output.is_empty(), "write to $zero must produce no output");
    }

    #[test]
    fn zero_read_is_literal_zero() {
        let gen = TestGenerator::new();
        assert_eq!(gen.emit_gpr_read(Reg::Zero), "0");
    }

    #[test]
    fn gpr_write_non_zero() {
        let mut gen = TestGenerator::new();
        gen.emit_gpr_write(Reg::Gpr(4), "42");
        assert_eq!(gen.output.len(), 1);
        assert!(gen.output[0].contains("42"));
    }
}
