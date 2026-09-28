//! CppGenerator — implements Generator for real C++17 text output.
//!
//! Writes to an internal `String` buffer; callers retrieve the accumulated
//! text via `take_output()` and write it to disk.

use std::fmt::Write as FmtWrite;
use psp_ir::{FpReg, Reg};
use crate::generator::Generator;

/// C++17 text generator that writes to an internal `String` buffer.
///
/// Callers retrieve accumulated source text with `take_output()`.
pub struct CppGenerator {
    buf: String,
    indent: usize,
    /// Statically-known cross-function `RECOMP_LOOKUP` targets emitted so far
    /// (jump/call tails and `note_static_lookup` callers). Consumed by the
    /// recompile report's dispatch-coverage audit via
    /// [`CppGenerator::take_static_lookup_targets`].
    static_lookup_targets: Vec<u32>,
}

impl CppGenerator {
    /// Create a new CppGenerator with an empty buffer.
    pub fn new() -> Self {
        Self {
            buf: String::with_capacity(65536),
            indent: 0,
            static_lookup_targets: Vec::new(),
        }
    }

    /// Return accumulated C++ source text and reset the buffer.
    pub fn take_output(&mut self) -> String {
        std::mem::take(&mut self.buf)
    }

    /// Return (and reset) the statically-known RECOMP_LOOKUP targets emitted
    /// since the last call. Register-indirect lookups are never recorded.
    pub fn take_static_lookup_targets(&mut self) -> Vec<u32> {
        std::mem::take(&mut self.static_lookup_targets)
    }

    /// Emit the standard per-file header (`#include "recomp.h"` etc.).
    pub fn emit_file_header(&mut self, _includes: &[&str]) {
        self.buf.push_str("#include \"recomp.h\"\n");
        self.buf.push_str("#include \"funcs.h\"\n");
        self.buf.push('\n');
    }

    /// Return the full content of `recomp.h` to write once per output directory.
    ///
    /// Includes: RECOMP_FUNC macro, recomp_context struct, memory accessor templates,
    /// relocation macros, and VFPU stub declaration.
    pub fn emit_recomp_h() -> String {
        r#"#pragma once
#include <cstdint>
#include <cstring>
#include <unordered_map>

#ifdef __clang__
  #define RECOMP_FUNC __attribute__((noinline))
#elif defined(__GNUC__)
  #define RECOMP_FUNC __attribute__((noipa, noinline))
#else
  #define RECOMP_FUNC
#endif

struct recomp_context {
    int32_t r[32];   // GPRs (r[0] always reads as 0 — enforced by emitter)
    uint32_t hi, lo;
    bool fpu_cc;
    // FPU registers: float and raw-bit views MUST alias the same storage.
    // lwc1/swc1/mtc1/mfc1 go through fi[], float arithmetic uses f[] —
    // separate arrays break every lwc1-fed float computation.
    union {
        float f[32];     // FPU registers as float
        uint32_t fi[32]; // FPU registers as raw bits (same storage)
    };
    // VFPU registers -- appended in Phase 3 to prevent layout break in Phase 6
    float vfpu[128];        // 8 x 4x4 matrices (mtx0-mtx7)
    uint32_t vfpu_ctrl[16]; // VFPU control registers (SPREFIX, TPREFIX, DPREFIX, etc.)
    uint32_t pc;            // Program counter (for debugging/STRICT mode)
    uint32_t entry_point;   // Mid-entry dispatch: 0 = normal entry, nonzero = jump to label
    // Instruction-budget preemption (#66, design approach (a)). Decremented at
    // every emitted loop back-edge; when it reaches <= 0 the emitted code calls
    // sched_preempt(ctx), which reloads it. With PSPRECOMP_PREEMPT unset/0 this
    // is a dead-effect counter + a no-op call (default-off — Patapon-identical).
    int32_t preempt_budget;
};
// Alias helpers (emitted code uses ctx->r[N], ctx->f[N].fl, etc.)
// ctx->r[0] is always 0 — enforced by emitter suppression, not runtime check.

using FuncPtr = void(*)(uint8_t*, recomp_context*);
FuncPtr RECOMP_LOOKUP(uint32_t vaddr);

// Memory accessors — type-safe rdram access (EMIT-13)
// Bounds-checked: masked offset + sizeof(T) must fit within 128MB rdram.
// NULL-page guard (issue #29): checked on the UNMASKED virtual address —
// scratchpad starts at 0x00010000 and kernel mirrors (0x08000000+) have
// unmasked values >= 0x00010000, so only genuinely invalid low pointers hit it.
// PPSSPP-faithful invalid-access semantics: NULL-page reads return 0, writes
// are discarded — guest code (e.g. Patapon's named-node walks with NULL roots)
// depends on this.
template<typename T>
inline T psp_mem_read(uint8_t* rdram, uint32_t addr) {
    if (addr < 0x00010000U) {
        return T{};
    }
    uint32_t off = addr & 0x07FFFFFFU;
    if (off + sizeof(T) > 0x08000000U) {
        T zero{};
        return zero;
    }
    T val;
    std::memcpy(&val, rdram + off, sizeof(T));
    return val;
}
template<typename T>
inline void psp_mem_write(uint8_t* rdram, uint32_t addr, T val) {
    if (addr < 0x00010000U) {
        return;
    }
    uint32_t off = addr & 0x07FFFFFFU;
    if (off + sizeof(T) > 0x08000000U) {
        return;
    }
    std::memcpy(rdram + off, &val, sizeof(T));
}
#define MEM_B(rdram, addr)  psp_mem_read<int8_t>(rdram, addr)
#define MEM_BU(rdram, addr) psp_mem_read<uint8_t>(rdram, addr)
#define MEM_H(rdram, addr)  psp_mem_read<int16_t>(rdram, addr)
#define MEM_HU(rdram, addr) psp_mem_read<uint16_t>(rdram, addr)
#define MEM_W(rdram, addr)  psp_mem_read<int32_t>(rdram, addr)
#define MEM_WU(rdram, addr) psp_mem_read<uint32_t>(rdram, addr)
#define MEM_W_WRITE(rdram, addr, val) psp_mem_write<int32_t>(rdram, addr, val)
#define MEM_H_WRITE(rdram, addr, val) psp_mem_write<int16_t>(rdram, addr, val)
#define MEM_B_WRITE(rdram, addr, val) psp_mem_write<int8_t>(rdram, addr, val)

// Relocation macros (EMIT-10)
#define RELOC_HI16(base, off) ((uint32_t)((base) + (off)) >> 16)
#define RELOC_LO16(base, off) ((uint32_t)((base) + (off)) & 0xFFFF)
#define RELOC_J26(base, off)  (((base) + (off)) >> 2)

// VFPU runtime library (implemented in runtime/src/psp_vfpu_*.cpp)
extern void vfpu_set_prefix(recomp_context* ctx, int reg_idx, uint32_t data);
extern void vfpu_eat_prefixes(recomp_context* ctx);
extern void vfpu_mfv(recomp_context* ctx, int rt_idx, uint8_t vd);
extern void vfpu_mtv(recomp_context* ctx, int rt_idx, uint8_t vd);
extern void vfpu_mfvc(recomp_context* ctx, int rt_idx, int imm);
extern void vfpu_mtvc(recomp_context* ctx, int rt_idx, int imm);
extern void vfpu_viim(recomp_context* ctx, uint8_t vt, uint16_t imm);
extern void vfpu_vfim(recomp_context* ctx, uint8_t vt, uint16_t imm);
extern void vfpu_vadd(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vsub(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vmul(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vdiv(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vmin(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vmax(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vscmp(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vsge(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vslt(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vdot(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vscl(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vhdp(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vcrs(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vdet(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vcmp(recomp_context* ctx, uint8_t* rdram, uint8_t vs, uint8_t vt, uint8_t cond, uint8_t size);
extern void vfpu_vcmov(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t cc, uint8_t size);
extern void vfpu_vmov(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vabs(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vneg(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vrcp(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vrsq(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vsin(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vcos(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vexp2(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vlog2(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vsqrt(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vasin(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vnrcp(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vnsin(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vrexp2(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vidt(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t size);
extern void vfpu_vzero(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t size);
extern void vfpu_vone(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t size);
extern void vfpu_vsat0(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vsat1(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vcst(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t imm5, uint8_t size);
extern void vfpu_vf2in(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t imm5, uint8_t size);
extern void vfpu_vf2iz(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t imm5, uint8_t size);
extern void vfpu_vf2iu(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t imm5, uint8_t size);
extern void vfpu_vf2id(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t imm5, uint8_t size);
extern void vfpu_vi2f(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t imm5, uint8_t size);
extern void vfpu_vi2uc(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vi2c(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vi2us(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vi2s(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vuc2i(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vc2i(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vus2i(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vs2i(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vf2h(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vh2f(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vmmul(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vmscl(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt, uint8_t size);
extern void vfpu_vtfm2(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt);
extern void vfpu_vtfm3(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt);
extern void vfpu_vtfm4(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt);
extern void vfpu_vhtfm2(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt);
extern void vfpu_vhtfm3(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt);
extern void vfpu_vhtfm4(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt);
extern void vfpu_vcrsp(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt);
extern void vfpu_vqmul(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t vt);
extern void vfpu_vmmov(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vmidt(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t size);
extern void vfpu_vmzero(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t size);
extern void vfpu_vmone(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t size);
extern void vfpu_vrot(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint16_t imm5, uint8_t size);
extern void vfpu_vwbn(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t imm8, uint8_t size);
extern void vfpu_lv_s(recomp_context* ctx, uint8_t* rdram, uint8_t vt, uint8_t rs, int16_t offset);
extern void vfpu_sv_s(recomp_context* ctx, uint8_t* rdram, uint8_t vt, uint8_t rs, int16_t offset);
extern void vfpu_lv_q(recomp_context* ctx, uint8_t* rdram, uint8_t vt, uint8_t rs, int16_t offset);
extern void vfpu_sv_q(recomp_context* ctx, uint8_t* rdram, uint8_t vt, uint8_t rs, int16_t offset);
extern void vfpu_lvl_q(recomp_context* ctx, uint8_t* rdram, uint8_t vt, uint8_t rs, int16_t offset);
extern void vfpu_lvr_q(recomp_context* ctx, uint8_t* rdram, uint8_t vt, uint8_t rs, int16_t offset);
extern void vfpu_svl_q(recomp_context* ctx, uint8_t* rdram, uint8_t vt, uint8_t rs, int16_t offset);
extern void vfpu_svr_q(recomp_context* ctx, uint8_t* rdram, uint8_t vt, uint8_t rs, int16_t offset);
extern void vfpu_vsrt1(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vsrt2(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vsrt3(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vsrt4(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vbfy1(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vbfy2(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vsocp(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vfad(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vavg(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t vs, uint8_t size);
extern void vfpu_vrnds(recomp_context* ctx, uint8_t* rdram, uint8_t vs);
extern void vfpu_vrndi(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t size);
extern void vfpu_vrndf1(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t size);
extern void vfpu_vrndf2(recomp_context* ctx, uint8_t* rdram, uint8_t vd, uint8_t size);
extern void vfpu_unknown_stub(recomp_context* ctx, uint8_t* rdram, uint32_t opcode, uint32_t pc);

// Unaligned memory access helpers (LWL/LWR/SWL/SWR)
// Semantics match PPSSPP MIPSInt.cpp exactly (little-endian PSP).
// Uses memcpy for aligned word access to avoid strict-aliasing UB.
static inline void psp_lwl(uint8_t* rdram, int32_t& rt, uint32_t addr) {
    uint32_t masked = addr & 0x07FFFFFFU;
    uint32_t aligned = masked & ~3U;
    if (aligned + 4 > 0x08000000U) return;
    uint32_t mem;
    std::memcpy(&mem, rdram + aligned, 4);
    uint32_t shift = (masked & 3) * 8;
    rt = (int32_t)(((uint32_t)rt & (0x00FFFFFFu >> shift)) | (mem << (24 - shift)));
}

static inline void psp_lwr(uint8_t* rdram, int32_t& rt, uint32_t addr) {
    uint32_t masked = addr & 0x07FFFFFFU;
    uint32_t aligned = masked & ~3U;
    if (aligned + 4 > 0x08000000U) return;
    uint32_t mem;
    std::memcpy(&mem, rdram + aligned, 4);
    uint32_t shift = (masked & 3) * 8;
    rt = (int32_t)(((uint32_t)rt & (0xFFFFFF00u << (24 - shift))) | (mem >> shift));
}

static inline void psp_swl(uint8_t* rdram, uint32_t rt, uint32_t addr) {
    uint32_t masked = addr & 0x07FFFFFFU;
    uint32_t aligned = masked & ~3U;
    if (aligned + 4 > 0x08000000U) return;
    uint32_t mem;
    std::memcpy(&mem, rdram + aligned, 4);
    uint32_t shift = (masked & 3) * 8;
    mem = (rt >> (24 - shift)) | (mem & (0xFFFFFF00u << shift));
    std::memcpy(rdram + aligned, &mem, 4);
}

static inline void psp_swr(uint8_t* rdram, uint32_t rt, uint32_t addr) {
    uint32_t masked = addr & 0x07FFFFFFU;
    uint32_t aligned = masked & ~3U;
    if (aligned + 4 > 0x08000000U) return;
    uint32_t mem;
    std::memcpy(&mem, rdram + aligned, 4);
    uint32_t shift = (masked & 3) * 8;
    mem = (rt << shift) | (mem & (0x00FFFFFFu >> (24 - shift)));
    std::memcpy(rdram + aligned, &mem, 4);
}

// HLE syscall dispatcher (Phase 4 -- runtime provides implementation)
void psp_hle_syscall(uint8_t* rdram, recomp_context* ctx, uint32_t code);

// PC tracing checkpoint (Phase 06.1 -- runtime provides implementation)
// No-op when PSPRECOMP_PC_TRACE env var is not set to "1".
extern void psp_trace_checkpoint(uint32_t addr);

// Instruction-budget preemption point (#66, design approach (a) — runtime
// provides the implementation in psp_scheduler.cpp). Emitted at loop back-edges
// as `if (--ctx->preempt_budget <= 0) sched_preempt(ctx);`. DEFAULT-OFF: when
// the PSPRECOMP_PREEMPT env var is unset/0 this only reloads ctx->preempt_budget
// and returns (no yield); the real fair-yield path is gated behind the flag.
extern void sched_preempt(recomp_context* ctx);
"#
        .to_string()
    }

    fn write_indent(&mut self) {
        for _ in 0..self.indent {
            self.buf.push_str("    ");
        }
    }

    fn writeln(&mut self, s: &str) {
        self.write_indent();
        self.buf.push_str(s);
        self.buf.push('\n');
    }
}

impl Default for CppGenerator {
    fn default() -> Self {
        Self::new()
    }
}

impl Generator for CppGenerator {
    fn emit_function_start(&mut self, cpp_name: &str, vaddr: u32) {
        let _ = writeln!(
            self.buf,
            "RECOMP_FUNC void {cpp_name}(uint8_t* rdram, recomp_context* ctx) \
             {{ /* 0x{vaddr:08X} */\n    psp_trace_checkpoint(0x{vaddr:08X}U);"
        );
        self.indent = 1;
    }

    fn emit_function_end(&mut self) {
        self.indent = 0;
        self.buf.push_str("}\n\n");
    }

    fn emit_gpr_write(&mut self, rd: Reg, expr: &str) {
        if rd == Reg::Zero {
            return; // EMIT-12: suppress $zero writes
        }
        self.writeln(&format!("{rd} = (int32_t)({expr});"));
    }

    fn emit_gpr_read(&self, rs: Reg) -> String {
        if rs == Reg::Zero {
            return "0".to_string(); // EMIT-12: $zero reads return literal 0
        }
        format!("{rs}")
    }

    fn emit_fpr_write(&mut self, fd: FpReg, expr: &str) {
        self.writeln(&format!("{fd} = {expr};"));
    }

    fn emit_fpr_read(&self, fs: FpReg) -> String {
        format!("{fs}")
    }

    fn emit_label(&mut self, label: &str) {
        // Labels must not be indented for C++ goto compatibility
        self.buf.push_str(&format!("{label}:\n"));
    }

    fn emit_goto(&mut self, label: &str) {
        self.writeln(&format!("goto {label};"));
    }

    fn emit_branch(&mut self, cond: &str, target_label: &str) {
        self.writeln(&format!("if ({cond}) goto {target_label};"));
    }

    fn emit_branch_likely(&mut self, cond: &str, target_label: &str, delay_slot_cpp: &str) {
        self.writeln(&format!("if ({cond}) {{"));
        self.indent += 1;
        self.writeln(delay_slot_cpp);
        self.writeln(&format!("goto {target_label};"));
        self.indent -= 1;
        self.writeln("}");
    }

    fn emit_switch(&mut self, index_expr: &str, cases: &[(u32, String)]) {
        self.writeln(&format!("switch ({index_expr}) {{"));
        self.indent += 1;
        // Deduplicate cases by address to avoid C++ duplicate-case errors.
        // Multiple xrefs can point to the same target; only the first case matters.
        let mut seen = std::collections::HashSet::new();
        for (addr, _label) in cases {
            if seen.insert(addr) {
                self.writeln(&format!(
                    "case 0x{addr:08X}: goto L_{addr:08X}; break;"
                ));
            }
        }
        // Default: lookup via dispatch table.
        // NOT recorded in static_lookup_targets: last_addr is an in-function
        // jump-table label, by design absent from dispatch — recording it would
        // flood the dispatch audit with intentional non-entries.
        if let Some((last_addr, _)) = cases.last() {
            self.writeln(&format!(
                "default: RECOMP_LOOKUP(0x{last_addr:08X})(rdram, ctx); return;"
            ));
        }
        self.indent -= 1;
        self.writeln("}");
    }

    fn emit_call_direct(&mut self, cpp_name: &str) {
        self.writeln(&format!("{cpp_name}(rdram, ctx);"));
    }

    fn emit_call_lookup(&mut self, vaddr: u32) {
        self.static_lookup_targets.push(vaddr);
        self.writeln(&format!("RECOMP_LOOKUP(0x{vaddr:08X})(rdram, ctx);"));
    }

    fn emit_call_lookup_reg(&mut self, reg_expr: &str) {
        self.writeln(&format!(
            "RECOMP_LOOKUP((uint32_t){reg_expr})(rdram, ctx);"
        ));
    }

    fn emit_call_hle(&mut self, stub_addr: u32, stub_name: &str) {
        // Same dispatch path as emit_call_lookup — the stub address is a
        // statically-emitted lookup target (recorded for the #37 audit; the
        // runtime registers every generated-table stub at psp_hle_init).
        self.static_lookup_targets.push(stub_addr);
        self.writeln(&format!(
            "RECOMP_LOOKUP(0x{stub_addr:08X})(rdram, ctx); /* {stub_name} */"
        ));
    }

    fn emit_return(&mut self) {
        self.writeln("return;");
    }

    fn emit_raw(&mut self, cpp: &str) {
        self.writeln(cpp);
    }

    fn emit_hilo_write(&mut self, hi_expr: &str, lo_expr: &str) {
        self.writeln(&format!("ctx->hi = {hi_expr}; ctx->lo = {lo_expr};"));
    }

    fn emit_fpu_cc_write(&mut self, expr: &str) {
        self.writeln(&format!("ctx->fpu_cc = {expr};"));
    }

    fn emit_fpu_cc_read(&self) -> String {
        "ctx->fpu_cc".into()
    }

    fn note_static_lookup(&mut self, vaddr: u32) {
        self.static_lookup_targets.push(vaddr);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn emit_recomp_h_contains_macro() {
        let h = CppGenerator::emit_recomp_h();
        assert!(h.contains("RECOMP_FUNC"), "recomp.h must define RECOMP_FUNC macro");
    }

    #[test]
    fn emit_recomp_h_contains_mem_w() {
        let h = CppGenerator::emit_recomp_h();
        assert!(h.contains("MEM_W"), "recomp.h must define MEM_W accessor");
    }

    #[test]
    fn emit_recomp_h_contains_reloc_hi16() {
        let h = CppGenerator::emit_recomp_h();
        assert!(h.contains("RELOC_HI16"), "recomp.h must define RELOC_HI16 macro");
    }

    #[test]
    fn emit_recomp_h_contains_vfpu_fields() {
        let h = CppGenerator::emit_recomp_h();
        assert!(h.contains("float vfpu[128]"), "recomp.h must have vfpu[128]");
        assert!(h.contains("uint32_t vfpu_ctrl[16]"), "recomp.h must have vfpu_ctrl[16]");
        assert!(h.contains("uint32_t pc;"), "recomp.h must have pc field");
    }

    #[test]
    fn emit_recomp_h_fpu_views_alias_via_union() {
        // f[] (float view) and fi[] (raw-bit view) must share storage:
        // lwc1/swc1/mtc1/mfc1 emit through fi[], float arithmetic through f[].
        // Separate arrays silently break every lwc1-fed float computation.
        let h = CppGenerator::emit_recomp_h();
        let union_start = h.find("union {").expect("recomp.h must declare an anonymous union");
        let union_end = h[union_start..].find("};").expect("union must be closed") + union_start;
        let union_body = &h[union_start..union_end];
        assert!(union_body.contains("float f[32]"), "f[32] must be inside the union");
        assert!(union_body.contains("uint32_t fi[32]"), "fi[32] must be inside the union");
    }

    #[test]
    fn emit_recomp_h_null_page_guard_in_mem_accessors() {
        // PPSSPP-faithful invalid-access semantics (issue #29): reads from the
        // NULL page return 0, writes are discarded. Without this, guest code
        // that walks NULL-rooted trees (Patapon's named-node ctor) writes to
        // rdram[0], reads it back, and spins forever.
        let h = CppGenerator::emit_recomp_h();
        let read_start = h.find("inline T psp_mem_read").expect("psp_mem_read must exist");
        let read_end = h[read_start..].find("template").expect("read followed by write template")
            + read_start;
        let read_body = &h[read_start..read_end];
        assert!(
            read_body.contains("if (addr < 0x00010000U)"),
            "psp_mem_read must guard the unmasked NULL page"
        );
        let write_start = h.find("inline void psp_mem_write").expect("psp_mem_write must exist");
        let write_end =
            h[write_start..].find("#define MEM_B").expect("MEM_B macros follow write")
                + write_start;
        let write_body = &h[write_start..write_end];
        assert!(
            write_body.contains("if (addr < 0x00010000U)"),
            "psp_mem_write must guard the unmasked NULL page"
        );
    }

    #[test]
    fn emit_recomp_h_contains_vfpu_function_decls() {
        let h = CppGenerator::emit_recomp_h();
        assert!(!h.contains("psp_vfpu_stub"), "recomp.h must NOT contain old psp_vfpu_stub");
        assert!(h.contains("vfpu_set_prefix"), "recomp.h must declare vfpu_set_prefix");
        assert!(h.contains("vfpu_vadd"), "recomp.h must declare vfpu_vadd");
        assert!(h.contains("vfpu_vmov"), "recomp.h must declare vfpu_vmov");
        assert!(h.contains("vfpu_lv_q"), "recomp.h must declare vfpu_lv_q");
        assert!(h.contains("vfpu_unknown_stub"), "recomp.h must declare vfpu_unknown_stub");
        assert!(h.contains("vfpu_vrot"), "recomp.h must declare vfpu_vrot");
        assert!(h.contains("vfpu_mfv"), "recomp.h must declare vfpu_mfv");
    }

    #[test]
    fn cpp_generator_zero_write_suppressed() {
        let mut gen = CppGenerator::new();
        gen.emit_function_start("test_func", 0x08804000);
        gen.emit_gpr_write(Reg::Zero, "42");
        let out = gen.take_output();
        assert!(!out.contains("= (int32_t)(42)"), "$zero write must not appear in output");
    }

    #[test]
    fn cpp_generator_zero_read_returns_literal() {
        let gen = CppGenerator::new();
        assert_eq!(gen.emit_gpr_read(Reg::Zero), "0");
    }

    #[test]
    fn cpp_generator_function_start_contains_recomp_func() {
        let mut gen = CppGenerator::new();
        gen.emit_function_start("my_func", 0x08800000);
        let out = gen.take_output();
        assert!(out.contains("RECOMP_FUNC"), "function preamble must have RECOMP_FUNC");
        assert!(out.contains("my_func"), "function preamble must have function name");
    }

    #[test]
    fn cpp_generator_function_start_contains_trace_checkpoint() {
        let mut gen = CppGenerator::new();
        gen.emit_function_start("test_fn", 0x08804000);
        let out = gen.take_output();
        assert!(
            out.contains("psp_trace_checkpoint(0x08804000U)"),
            "function preamble must call psp_trace_checkpoint with address"
        );
    }

    #[test]
    fn emit_recomp_h_contains_trace_checkpoint_decl() {
        let h = CppGenerator::emit_recomp_h();
        assert!(
            h.contains("psp_trace_checkpoint"),
            "recomp.h must declare psp_trace_checkpoint"
        );
    }
}
