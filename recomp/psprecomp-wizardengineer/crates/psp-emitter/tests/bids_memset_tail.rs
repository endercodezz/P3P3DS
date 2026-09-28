//! End-to-end BIDS acceptance test (issue #56): decode the exact .hack//Link
//! SCE libc VFPU memset tail sequence, emit C++, COMPILE it with the host C++
//! compiler, and RUN it. The emitted function must terminate and write exactly
//! N bytes (the pre-fix emission loops forever because the back-edge into the
//! `beq` delay slot lands on an empty label and never decrements the count).

use std::process::Command;

use psp_emitter::cpp_generator::CppGenerator;
use psp_emitter::function::{emit_function, ImportMap};
use psp_ir::{BasicBlock, DecodedFunction};

/// The exact raw words of FUN_08BDF1A8's tail loop (0x08BDF22C..0x08BDF244):
/// beq a2,zero,exit / addiu a2,a2,-1 (delay + bne target) / sb a1,0(a0) /
/// bne a2,zero,0x08BDF230 / addiu a0,a0,1 / jr ra / nop.
const MEMSET_TAIL_BASE: u32 = 0x08BDF22C;
const MEMSET_TAIL: [u32; 7] = [
    0x10C0_0004,
    0x24C6_FFFF,
    0xA085_0000,
    0x14C0_FFFD,
    0x2484_0001,
    0x03E0_0008,
    0x0000_0000,
];

/// Emit the decoded memset tail as C++ text via the real CppGenerator.
fn emit_memset_tail_cpp() -> String {
    let bytes: Vec<u8> = MEMSET_TAIL.iter().flat_map(|w| w.to_le_bytes()).collect();
    let ops = psp_decoder::decode_function(&bytes, MEMSET_TAIL_BASE, &[])
        .expect("memset tail must decode");
    let func = DecodedFunction {
        vaddr: MEMSET_TAIL_BASE,
        name: "memset_tail".to_string(),
        cpp_name: "memset_tail".to_string(),
        size: (MEMSET_TAIL.len() * 4) as u32,
        blocks: vec![BasicBlock { vaddr: MEMSET_TAIL_BASE, instrs: ops }],
        is_mid_entry_parent: false,
        mid_entry_addrs: vec![],
        coalesced: false,
    };
    let mut gen = CppGenerator::new();
    emit_function(&func, &mut gen, &ImportMap::new());
    gen.take_output()
}

/// Minimal host harness: real recomp_context layout subset, a write-counting
/// MEM_B_WRITE, and a runaway guard (an infinite tail loop trips it instead
/// of hanging the test). Runs the function with a0=dst, a1=fill, a2=argv[1]
/// and verifies exactly `count` fill bytes were written, then exits 0.
fn harness(emitted: &str) -> String {
    format!(
        r#"#include <cstdint>
#include <cstdio>
#include <cstdlib>
#define RECOMP_FUNC
struct recomp_context {{
    int32_t r[32];
    uint32_t hi, lo;
    bool fpu_cc;
    union {{ float f[32]; uint32_t fi[32]; }};
    float vfpu[128];
    uint32_t vfpu_ctrl[16];
    uint32_t pc;
    uint32_t entry_point;
    int32_t preempt_budget;  // #66 instruction-budget preemption
}};
static void psp_trace_checkpoint(uint32_t) {{}}
// #66: the emitted back-edge preemption point calls sched_preempt. In this
// standalone harness it is a no-op reload (mirrors the DEFAULT-OFF runtime),
// so the memset tail still terminates writing exactly N bytes.
static void sched_preempt(recomp_context* ctx) {{ if (ctx) ctx->preempt_budget = 100000; }}
static uint64_t g_writes = 0;
static uint8_t g_mem[0x10000];
static void mem_b_write(uint32_t addr, uint32_t val) {{
    if (++g_writes > 1000000ULL) {{ std::printf("RUNAWAY\n"); std::exit(2); }}
    g_mem[addr & 0xFFFFU] = (uint8_t)val;
}}
#define MEM_B_WRITE(rdram, addr, val) mem_b_write((uint32_t)(addr), (uint32_t)(val))
{emitted}
int main(int argc, char** argv) {{
    if (argc < 2) return 3;
    uint32_t count = (uint32_t)std::strtoul(argv[1], nullptr, 0);
    recomp_context ctx = {{}};
    uint8_t rdram = 0;
    ctx.r[4] = 0x1000;        // a0 = dst
    ctx.r[5] = 0xAB;          // a1 = fill
    ctx.r[6] = (int32_t)count; // a2 = count
    memset_tail(&rdram, &ctx);
    if (g_writes != count) {{ std::printf("WRITES=%llu\n", (unsigned long long)g_writes); return 1; }}
    for (uint32_t i = 0; i < count; ++i) {{
        if (g_mem[(0x1000 + i) & 0xFFFF] != 0xAB) {{ std::printf("BADBYTE=%u\n", i); return 1; }}
    }}
    std::printf("OK %u\n", count);
    return 0;
}}
"#
    )
}

#[test]
fn memset_tail_terminates_and_writes_exactly_n_bytes() {
    let cpp = harness(&emit_memset_tail_cpp());
    let dir = std::env::temp_dir().join(format!("psprecomp_bids_{}", std::process::id()));
    std::fs::create_dir_all(&dir).expect("mkdir");
    let src = dir.join("bids_memset_tail.cpp");
    let bin = dir.join("bids_memset_tail");
    std::fs::write(&src, &cpp).expect("write source");

    let cc = Command::new("c++")
        .args(["-std=c++17", "-O1", "-o"])
        .arg(&bin)
        .arg(&src)
        .output()
        .expect("host c++ compiler must be available");
    assert!(
        cc.status.success(),
        "emitted C++ must compile:\n{}\n--- source ---\n{cpp}",
        String::from_utf8_lossy(&cc.stderr)
    );

    // The measured .hack//Link case: tail entered with count 0x37 (r6 frozen
    // at 0x37 in the live spin) — must terminate writing exactly 55 bytes.
    for count in ["0x37", "0", "1", "2", "4096"] {
        let run = Command::new(&bin).arg(count).output().expect("run harness");
        let stdout = String::from_utf8_lossy(&run.stdout);
        assert!(
            run.status.code() == Some(0),
            "count={count}: emitted memset tail must terminate writing exactly \
             count bytes (exit={:?}, stdout={stdout})\n--- source ---\n{cpp}",
            run.status.code()
        );
    }

    let _ = std::fs::remove_dir_all(&dir);
}
