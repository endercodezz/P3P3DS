//! HLE call pattern scanner: discovers function entry points passed as arguments
//! to PSP kernel APIs like sceKernelCreateThread. These entries are invisible to
//! Ghidra static analysis and must be found by scanning JAL call sites.

use anyhow::{Context, Result};
use psp_parser::analysis_json::JsonFunction;
use serde::Serialize;
use std::collections::{HashMap, HashSet};
use std::path::Path;

/// HLE functions that accept function pointer arguments.
/// Each tuple: (function_name, target_arg_register, description).
const HLE_FUNCPTR_APIS: &[(&str, u8, &str)] = &[
    ("sceKernelCreateThread", 5, "a1 = thread entry function"),
    ("sceKernelCreateCallback", 5, "a1 = callback function"),
    ("sceKernelSetAlarm", 5, "a1 = alarm handler"),
    ("sceKernelSetSysClockAlarm", 5, "a1 = alarm handler"),
    ("sceKernelSetVTimerHandler", 6, "a2 = timer handler"),
    ("sceKernelSetVTimerHandlerWide", 6, "a2 = timer handler"),
    (
        "sceKernelRegisterThreadEventHandler",
        7,
        "a3 = event handler",
    ),
];

/// MIPS opcodes used for backward scanning (raw u32 bit extraction).
const OP_LUI: u32 = 0x0F;
const OP_ADDIU: u32 = 0x09;
const OP_ORI: u32 = 0x0D;
const OP_JAL: u32 = 0x03;

/// Number of instructions to walk backward from a JAL call site.
const BACKWARD_WINDOW: usize = 32;

/// Valid PSP .text address range for discovered entries.
const TEXT_RANGE_START: u32 = 0x08800000;
const TEXT_RANGE_END: u32 = 0x0A000000;

/// A single HLE-discovered entry point.
#[derive(Debug, Clone, Serialize)]
pub struct HleDiscovery {
    pub address: u32,
    pub source_hle_call: String,
    pub call_site: u32,
    pub arg_register: String,
    pub was_known: bool,
    pub action: String,
    pub estimated_size: u64,
}

/// A function size correction record.
#[derive(Debug, Clone, Serialize)]
pub struct SizeCorrection {
    pub function: String,
    pub address: String,
    pub old_size: u64,
    pub new_size: u64,
    pub reason: String,
}

/// Result of scanning for HLE entry points.
#[derive(Debug, Clone, Serialize)]
pub struct HleDiscoveryResult {
    pub call_sites_scanned: usize,
    pub discoveries: Vec<HleDiscovery>,
    pub new_entries: Vec<HleDiscovery>,
    pub size_corrections: Vec<SizeCorrection>,
    pub functions_before: usize,
}

/// Build HLE stub map from already-parsed PRX import stubs.
///
/// For PRX binaries where analysis.json has import data, we can directly
/// match stub names against HLE_FUNCPTR_APIS without re-parsing the binary.
pub fn build_stub_map_from_imports(
    import_stubs: &[psp_parser::types::ImportStub],
) -> HashMap<u32, (String, u8)> {
    let api_regs: HashMap<&str, u8> = HLE_FUNCPTR_APIS
        .iter()
        .map(|&(name, reg, _)| (name, reg))
        .collect();

    let mut result = HashMap::new();
    for stub in import_stubs {
        if let Some(&reg) = api_regs.get(stub.name.as_str()) {
            result.insert(
                stub.stub_addr,
                (stub.name.clone(), reg),
            );
        }
    }
    result
}

/// Discover import stub addresses from a binary's `.lib.stub` table.
///
/// Thin shim over the single libstub walker
/// (`psp_parser::imports::parse_import_stubs` — the duplicate walker that
/// used to live here is gone, issue #52 consolidation): builds a
/// `LoadedImage` over the already-rebased segments (base 0 extra offset for
/// ELF), locates SceModuleInfo, walks the imports, and keeps only HLE APIs
/// that take function-pointer arguments. Failures degrade to an empty map
/// with a loud error — the HLE scan is best-effort by design.
pub fn discover_import_stubs_for_elf(
    elf: &goblin::elf::Elf,
    seg_bases: &[u32],
    seg_datas: &[Vec<u8>],
    nid_db: &HashMap<u32, String>,
) -> HashMap<u32, (String, u8)> {
    let api_regs: HashMap<&str, u8> = HLE_FUNCPTR_APIS
        .iter()
        .map(|&(name, reg, _)| (name, reg))
        .collect();

    let walk = || -> Result<Vec<psp_parser::types::ImportStub>, psp_parser::errors::ParseError> {
        let image = psp_parser::image::LoadedImage::new(seg_bases, seg_datas);
        let va = psp_parser::prx::locate_module_info_va(elf, 0)?;
        let mi = psp_parser::prx::parse_module_info(&image, va)?;
        tracing::info!(
            "SceModuleInfo '{}': libstub range 0x{:08X}-0x{:08X}",
            mi.name,
            mi.libstub,
            mi.libstub_end
        );
        psp_parser::imports::parse_import_stubs(&image, &mi, nid_db)
    };
    let stubs = match walk() {
        Ok(s) => s,
        Err(e) => {
            tracing::error!("Failed to parse .lib.stub for HLE stub discovery: {e}");
            return HashMap::new();
        }
    };

    let mut result = HashMap::new();
    for stub in &stubs {
        if let Some(&reg) = api_regs.get(stub.name.as_str()) {
            result.insert(stub.stub_addr, (stub.name.clone(), reg));
        }
    }

    tracing::info!(
        "Discovered {} import stubs, {} match HLE funcptr APIs",
        stubs.len(),
        result.len()
    );
    result
}

/// Scan text segment bytes for JAL instructions targeting HLE stubs,
/// then walk backward to extract function pointer arguments.
///
/// Returns a `HleDiscoveryResult` with all discoveries and metadata.
pub fn scan_hle_entries(
    text_bytes: &[u8],
    text_base: u32,
    stub_map: &HashMap<u32, (String, u8)>,
    existing_functions: &[JsonFunction],
) -> HleDiscoveryResult {
    use std::collections::HashSet;

    let func_addrs: HashSet<u32> = existing_functions
        .iter()
        .filter_map(|f| parse_hex_addr(&f.address))
        .collect();

    let mut all_discoveries = Vec::new();
    let mut call_sites_scanned = 0;

    // Scan every instruction in the text segment for JAL to HLE stubs
    let word_count = text_bytes.len() / 4;
    for i in 0..word_count {
        let off = i * 4;
        let word = u32::from_le_bytes(
            text_bytes[off..off + 4].try_into().unwrap(),
        );
        let opcode = (word >> 26) & 0x3F;
        if opcode != OP_JAL {
            continue;
        }

        // JAL target: bits 25:0 << 2, upper 4 bits from PC
        let pc = text_base + off as u32;
        let target = (pc & 0xF0000000) | ((word & 0x03FFFFFF) << 2);

        if let Some((hle_name, arg_reg)) = stub_map.get(&target) {
            call_sites_scanned += 1;

            let found = scan_backward_for_funcptr(
                text_bytes, off, *arg_reg, text_base, stub_map,
            );

            for addr in found {
                let was_known = func_addrs.contains(&addr);
                all_discoveries.push(HleDiscovery {
                    address: addr,
                    source_hle_call: hle_name.clone(),
                    call_site: pc,
                    arg_register: format!("a{}", arg_reg - 4),
                    was_known,
                    action: if was_known {
                        "already_known".into()
                    } else {
                        "added_as_new_function".into()
                    },
                    estimated_size: 0, // filled in later
                });
            }
        }
    }

    // Deduplicate by address (same function may be passed to multiple calls)
    let mut seen = HashSet::new();
    all_discoveries.retain(|d| seen.insert(d.address));

    // Separate new from known
    let new_entries: Vec<HleDiscovery> = all_discoveries
        .iter()
        .filter(|d| !d.was_known)
        .cloned()
        .collect();

    // Estimate sizes for new entries based on next known function
    let mut all_addrs: Vec<u32> = func_addrs.iter().copied().collect();
    for e in &new_entries {
        all_addrs.push(e.address);
    }
    all_addrs.sort();
    all_addrs.dedup();

    let new_entries_with_sizes: Vec<HleDiscovery> = new_entries
        .into_iter()
        .map(|mut e| {
            let pos = all_addrs
                .binary_search(&e.address)
                .unwrap_or_else(|p| p);
            if pos + 1 < all_addrs.len() {
                e.estimated_size =
                    (all_addrs[pos + 1] - e.address) as u64;
            } else {
                e.estimated_size = 256; // fallback
            }
            e
        })
        .collect();

    // Update all_discoveries with estimated sizes for audit trail
    let size_map: HashMap<u32, u64> = new_entries_with_sizes
        .iter()
        .map(|e| (e.address, e.estimated_size))
        .collect();
    for disc in &mut all_discoveries {
        if let Some(&size) = size_map.get(&disc.address) {
            disc.estimated_size = size;
        }
    }

    let functions_before = existing_functions.len();

    HleDiscoveryResult {
        call_sites_scanned,
        discoveries: all_discoveries,
        new_entries: new_entries_with_sizes,
        size_corrections: Vec::new(),
        functions_before,
    }
}

/// Walk backward from a JAL call site to find LUI+ADDIU/ORI pairs
/// that load the target argument register with a code address.
fn scan_backward_for_funcptr(
    bytes: &[u8],
    call_offset: usize,
    target_reg: u8,
    _base_vaddr: u32,
    stub_map: &HashMap<u32, (String, u8)>,
) -> Vec<u32> {
    let mut lui_values: HashMap<u8, u16> = HashMap::new();
    let window_start = call_offset.saturating_sub(BACKWARD_WINDOW * 4);
    let mut results = Vec::new();

    let mut off = window_start;
    while off < call_offset {
        if off + 4 > bytes.len() {
            break;
        }
        let word =
            u32::from_le_bytes(bytes[off..off + 4].try_into().unwrap());
        let opcode = (word >> 26) & 0x3F;
        let rs = ((word >> 21) & 0x1F) as u8;
        let rt = ((word >> 16) & 0x1F) as u8;
        let imm = (word & 0xFFFF) as u16;

        match opcode {
            OP_LUI => {
                lui_values.insert(rt, imm);
            }
            OP_ADDIU if rt == target_reg => {
                if let Some(&hi) = lui_values.get(&rs) {
                    let lo = imm as i16;
                    let addr = ((hi as u32) << 16)
                        .wrapping_add(lo as i32 as u32);
                    if is_valid_text_addr(addr, stub_map) {
                        results.push(addr);
                    }
                }
            }
            OP_ORI if rt == target_reg => {
                if let Some(&hi) = lui_values.get(&rs) {
                    let addr = ((hi as u32) << 16) | (imm as u32);
                    if is_valid_text_addr(addr, stub_map) {
                        results.push(addr);
                    }
                }
            }
            _ => {}
        }
        off += 4;
    }

    results
}

/// Validate that a discovered address is in .text range and not a stub.
fn is_valid_text_addr(
    addr: u32,
    stub_map: &HashMap<u32, (String, u8)>,
) -> bool {
    addr >= TEXT_RANGE_START
        && addr < TEXT_RANGE_END
        && !stub_map.contains_key(&addr)
}

/// Correct giant function sizes by clamping to the next function boundary.
///
/// Sorts functions by address and ensures no function extends past the
/// start of the next function. Returns a Vec of corrections applied.
pub fn correct_function_sizes(
    functions: &mut Vec<JsonFunction>,
) -> Vec<SizeCorrection> {
    functions.sort_by_key(|f| {
        parse_hex_addr(&f.address).unwrap_or(0)
    });

    let mut corrections = Vec::new();
    let len = functions.len();

    for i in 0..len.saturating_sub(1) {
        let this_addr =
            parse_hex_addr(&functions[i].address).unwrap_or(0);
        let next_addr =
            parse_hex_addr(&functions[i + 1].address).unwrap_or(0);

        if next_addr <= this_addr {
            continue;
        }

        let max_size = (next_addr - this_addr) as u64;
        if functions[i].size > max_size {
            let old_size = functions[i].size;
            functions[i].size = max_size;
            tracing::info!(
                "Clamped {} from {} to {} bytes (next func at 0x{:08X})",
                functions[i].name,
                old_size,
                max_size,
                next_addr
            );
            corrections.push(SizeCorrection {
                function: functions[i].name.clone(),
                address: functions[i].address.clone(),
                old_size,
                new_size: max_size,
                reason: "clamped_to_next_function".into(),
            });
        }
    }

    corrections
}

/// Read a little-endian 32-bit instruction word at a virtual address from the
/// decoded segment bytes. Returns None if the address is outside every segment.
fn read_word_at(va: u32, segment_bytes: &[(u32, Vec<u8>)]) -> Option<u32> {
    for (seg_va, bytes) in segment_bytes {
        if va >= *seg_va {
            let off = (va - *seg_va) as usize;
            if off + 4 <= bytes.len() {
                return Some(u32::from_le_bytes(
                    bytes[off..off + 4].try_into().unwrap(),
                ));
            }
        }
    }
    None
}

/// Read a stack-frame prologue size from a function entry.
///
/// Returns `Some(N)` when one of the first two instructions is
/// `ADDIU $sp, $sp, -N` (the standard MIPS frame setup), else `None`
/// (a frameless leaf/thunk that saves no callee-saved registers).
fn frame_prologue_size(
    entry: u32,
    segment_bytes: &[(u32, Vec<u8>)],
) -> Option<u32> {
    for k in 0..2u32 {
        let w = read_word_at(entry + 4 * k, segment_bytes)?;
        // ADDIU $sp,$sp,imm => top half 0x27BD; negative imm => bit15 set.
        if (w >> 16) == 0x27BD && (w & 0xFFFF) >= 0x8000 {
            return Some(0x1_0000 - (w & 0xFFFF));
        }
    }
    None
}

/// Decode a PC-relative branch target if `w` (at address `va`) is a MIPS
/// conditional branch (or `b`/branch-likely form). Returns the absolute
/// target address, else None. `j`/`jal` (absolute) are NOT included.
fn branch_target(w: u32, va: u32) -> Option<u32> {
    let op = w >> 26;
    let is_branch = match op {
        // REGIMM: bltz/bgez/bltzl/bgezl/bltzal/bgezal/bltzall/bgezall
        0x01 => matches!(
            (w >> 16) & 0x1F,
            0x00 | 0x01 | 0x02 | 0x03 | 0x10 | 0x11 | 0x12 | 0x13
        ),
        // beq/bne/blez/bgtz + likely variants
        0x04..=0x07 | 0x14..=0x17 => true,
        // COP1 bc1f/bc1t/bc1fl/bc1tl, COP2 (VFPU) bvf/bvt/bvfl/bvtl:
        // rs field == 0x08 (BC sub-op). Cold tails reached only via an FPU
        // branch (e.g. `bc1tl 0x08856BDC` in FUN_08856AEC) are missed
        // without these.
        0x11 | 0x12 => ((w >> 21) & 0x1F) == 0x08,
        _ => false,
    };
    if !is_branch {
        return None;
    }
    let imm = (w & 0xFFFF) as i16 as i64;
    Some((va as i64 + 4 + imm * 4) as u32)
}

/// True when `w` unconditionally transfers control away from the next
/// instruction: `j`, `jr` (any register), `b` (`beq rs,rs`), or
/// `bgez $zero` (branch-always idiom). `jal`/`jalr` return, so they are
/// NOT terminators.
fn is_unconditional_transfer(w: u32) -> bool {
    let op = w >> 26;
    match op {
        0x02 => true,                       // j
        0x00 => (w & 0x3F) == 0x08,         // SPECIAL/jr
        0x04 | 0x14 => ((w >> 21) & 0x1F) == ((w >> 16) & 0x1F), // beq rs,rs
        0x01 => {
            let rs = (w >> 21) & 0x1F;
            let rt = (w >> 16) & 0x1F;
            rs == 0 && (rt == 0x01 || rt == 0x03) // bgez/bgezl $zero
        }
        _ => false,
    }
}

/// Locate a framed function's real end by finding its matching epilogue.
///
/// Scans forward from `entry` for `jr $ra` (0x03E00008) adjacent to
/// `ADDIU $sp, $sp, +N` (the frame teardown matching the prologue's `-N`).
///
/// An epilogue is only accepted as the function end when no earlier
/// in-function branch jumps PAST it: compilers place cold blocks AFTER the
/// epilogue (the post-epilogue cold-tail pattern, issue #13 — `bnez` at
/// 0x0884326C in FUN_088431E8 targets 0x088432A4, past the `jr ra` at
/// 0x0884329C). Stopping at the first epilogue orphans those blocks; their
/// branch targets then LOOKUP_MISS to noop_stub, skipping register restores
/// and corrupting the caller. After the matching epilogue has been seen,
/// any unconditional control transfer (cold blocks end with `j` back into
/// the body, or their own `jr ra`) also ends the function once every
/// pending forward branch target is covered.
///
/// Bounded by `scan_cap` so a missing epilogue can't run away. Falls back
/// to the first-epilogue size when a forward target can't be resolved
/// within the cap (e.g. a cross-function conditional branch). Returns the
/// byte size, or None if no matching epilogue is found within the cap.
fn framed_function_size(
    entry: u32,
    frame_n: u32,
    scan_cap: u32,
    segment_bytes: &[(u32, Vec<u8>)],
) -> Option<u64> {
    let teardown = 0x27BD_0000 | frame_n; // ADDIU $sp,$sp,+N
    let end_cap = entry.saturating_add(scan_cap);
    let mut va = entry;
    let mut max_fwd_target: u32 = 0;
    let mut first_epilogue: Option<u64> = None;
    while va < end_cap {
        let Some(w) = read_word_at(va, segment_bytes) else { break };
        if let Some(t) = branch_target(w, va) {
            if t > va && t < end_cap {
                max_fwd_target = max_fwd_target.max(t);
            }
        }
        let is_epilogue = w == 0x03E0_0008 && {
            let delay = read_word_at(va + 4, segment_bytes);
            let prev = if va > entry {
                read_word_at(va - 4, segment_bytes)
            } else {
                None
            };
            delay == Some(teardown) || prev == Some(teardown)
        };
        let end = va.wrapping_add(8); // past the delay slot
        if is_epilogue {
            if first_epilogue.is_none() {
                first_epilogue = Some((end - entry) as u64);
            }
            if max_fwd_target < end {
                return Some((end - entry) as u64);
            }
        } else if first_epilogue.is_some()
            && is_unconditional_transfer(w)
            && max_fwd_target < end
        {
            // Post-epilogue cold block terminator; all forward branch
            // targets are covered, so the function ends here.
            return Some((end - entry) as u64);
        }
        va += 4;
    }
    first_epilogue
}

/// Static control-flow target of `w` at `va`: a PC-relative conditional
/// branch target or an absolute `j`/`jal` target. `jal` is included
/// because the emitter lowers a call to an unknown target to
/// `RECOMP_LOOKUP(target)` too — a `jal` into an unclaimed gap (e.g.
/// `jal 0x8815ef8` at 0x0881480C) is a real function entry both Ghidra and
/// the prologue scan missed.
fn static_cf_target(w: u32, va: u32) -> Option<u32> {
    branch_target(w, va).or_else(|| {
        matches!(w >> 26, 0x02 | 0x03).then(|| {
            (va.wrapping_add(4) & 0xF000_0000) | ((w & 0x03FF_FFFF) << 2)
        })
    })
}

/// True when `t` is covered by any sorted, non-overlapping `(start, end)`
/// interval (a function start counts as covered).
fn interval_claims(intervals: &[(u32, u32)], t: u32) -> bool {
    match intervals.binary_search_by_key(&t, |&(s, _)| s) {
        Ok(_) => true, // exactly at a function start
        Err(0) => false,
        Err(i) => t < intervals[i - 1].1,
    }
}

/// Rescue static control-flow targets that land in unclaimed gaps.
///
/// After all sizing passes, scan every function body for conditional-branch
/// and `j` targets that fall OUTSIDE every known function interval. The
/// emitter turns such targets into `RECOMP_LOOKUP(target)`; with no owner
/// in the dispatch table they resolve to `noop_stub` and the control
/// transfer is silently dropped — skipping that code's register restores
/// and corrupting the caller (the issue-13 LOOKUP_MISS class). Each rescued
/// target becomes a `binary_scan` function entry with the standard
/// placeholder size; the caller must re-run
/// `resize_truncated_framed_functions` + `correct_function_sizes`.
/// Returns the rescued addresses.
pub fn rescue_gap_branch_targets(
    functions: &mut Vec<JsonFunction>,
    segment_bytes: &[(u32, Vec<u8>)],
    seg_start: u32,
    seg_end: u32,
) -> Vec<u32> {
    let mut intervals: Vec<(u32, u32)> = functions
        .iter()
        .filter_map(|f| {
            let s = parse_hex_addr(&f.address)?;
            Some((s, s.saturating_add(f.size as u32)))
        })
        .collect();
    intervals.sort_unstable();
    let mut rescued: Vec<u32> = Vec::new();
    let mut seen: HashSet<u32> = HashSet::new();
    for &(start, end) in &intervals {
        let mut va = start;
        while va < end {
            let Some(w) = read_word_at(va, segment_bytes) else { break };
            if let Some(t) = static_cf_target(w, va) {
                if t >= seg_start
                    && t < seg_end
                    && t % 4 == 0
                    && !interval_claims(&intervals, t)
                    && seen.insert(t)
                {
                    rescued.push(t);
                }
            }
            va += 4;
        }
    }
    for &addr in &rescued {
        functions.push(JsonFunction {
            name: format!("FUN_{addr:08X}"),
            address: format!("0x{addr:08X}"),
            size: 256, // placeholder; resized + clamped by the caller
            is_external: false,
            is_thunk: false,
            source: "binary_scan".to_string(),
        });
    }
    rescued
}

/// Repair truncated heuristically-discovered functions by sizing them to their
/// real epilogue.
///
/// Functions found by binary scans (`vtable_miss`, `binary_scan`) are created
/// with a fixed placeholder size (256). `correct_function_sizes` only shrinks
/// oversized functions, so a heuristic function whose real body exceeds the
/// placeholder stays truncated -- its epilogue (callee-saved register restores
/// + `jr $ra`) is dropped, so the saved registers are NOT restored across the
/// call and the caller's values are corrupted.
///
/// This is exactly what breaks Patapon's C++ constructor walk: the 504-entry
/// `__libc_init_array` table holds code-in-`.data` constructors Ghidra misses
/// (found heuristically). The truncated constructor FUN_08A0D658 (real size
/// 1424, emitted as 256) never restores `s3`, which the walker (`FUN_088046B4`)
/// uses as its loop-end bound -- so the walk aborts after ~48/504 entries and
/// the asset-table builder (FUN_08A21CD4, entry #235) never runs. Result: empty
/// asset hash table, no geometry, clear-only frames.
///
/// Only *framed* functions (those with an `ADDIU $sp,$sp,-N` prologue) are
/// touched -- they are the ones that save callee-saved registers and thus can
/// corrupt the caller when truncated. Their exact size is recovered from the
/// matching `jr $ra` / `ADDIU $sp,$sp,+N` epilogue, never the next-function gap
/// (which would over-extend across legitimately-separate frameless functions in
/// the same region). Frameless thunks are left untouched.
///
/// Run BEFORE `correct_function_sizes` and before the discovery passes so the
/// corrected intervals stop the gap/prologue scans from injecting phantom
/// entries inside a (previously truncated) body.
pub fn resize_truncated_framed_functions(
    functions: &mut Vec<JsonFunction>,
    segment_bytes: &[(u32, Vec<u8>)],
) -> Vec<SizeCorrection> {
    const HEURISTIC_SOURCES: &[&str] = &["vtable_miss", "binary_scan"];
    // Generous upper bound on a single function's size; bounds runaway scans
    // when an epilogue can't be found (the function is then left unchanged).
    const SCAN_CAP: u32 = 0x4000; // 16 KiB

    functions.sort_by_key(|f| parse_hex_addr(&f.address).unwrap_or(0));

    let mut corrections = Vec::new();

    for f in functions.iter_mut() {
        if !HEURISTIC_SOURCES.contains(&f.source.as_str()) {
            continue;
        }
        let entry = match parse_hex_addr(&f.address) {
            Some(a) => a,
            None => continue,
        };
        let frame_n = match frame_prologue_size(entry, segment_bytes) {
            Some(n) => n,
            None => continue, // frameless: cannot corrupt callee-saved regs
        };
        let true_size = match framed_function_size(
            entry,
            frame_n,
            SCAN_CAP,
            segment_bytes,
        ) {
            Some(s) => s,
            None => continue, // no clear epilogue found: leave as-is
        };

        if f.size != true_size {
            let old_size = f.size;
            f.size = true_size;
            tracing::info!(
                "Resized framed {} from {} to {} bytes (epilogue scan)",
                f.name,
                old_size,
                true_size,
            );
            corrections.push(SizeCorrection {
                function: f.name.clone(),
                address: f.address.clone(),
                old_size,
                new_size: true_size,
                reason: "resized_to_epilogue".into(),
            });
        }
    }

    corrections
}

/// Write discovered_entries.json audit file alongside analysis output.
pub fn write_discovered_entries_json(
    result: &HleDiscoveryResult,
    corrections: &[SizeCorrection],
    output_path: &Path,
) -> Result<()> {
    #[derive(Serialize)]
    struct DiscoveredEntriesJson {
        scan_date: String,
        summary: Summary,
        discoveries: Vec<DiscoveryEntry>,
        size_corrections: Vec<SizeCorrection>,
    }

    #[derive(Serialize)]
    struct Summary {
        functions_before: usize,
        new_entries_added: usize,
        size_corrections: usize,
        hle_call_sites_scanned: usize,
    }

    #[derive(Serialize)]
    struct DiscoveryEntry {
        address: String,
        source_hle_call: String,
        call_site: String,
        arg_register: String,
        was_known: bool,
        action: String,
        estimated_size: u64,
    }

    let json = DiscoveredEntriesJson {
        scan_date: chrono_date_string(),
        summary: Summary {
            functions_before: result.functions_before,
            new_entries_added: result.new_entries.len(),
            size_corrections: corrections.len(),
            hle_call_sites_scanned: result.call_sites_scanned,
        },
        discoveries: result
            .discoveries
            .iter()
            .map(|d| DiscoveryEntry {
                address: format!("0x{:08X}", d.address),
                source_hle_call: d.source_hle_call.clone(),
                call_site: format!("0x{:08X}", d.call_site),
                arg_register: d.arg_register.clone(),
                was_known: d.was_known,
                action: d.action.clone(),
                estimated_size: d.estimated_size,
            })
            .collect(),
        size_corrections: corrections.to_vec(),
    };

    let json_str = serde_json::to_string_pretty(&json)
        .context("Failed to serialize discovered_entries.json")?;
    std::fs::write(output_path, json_str).with_context(|| {
        format!("Failed to write {}", output_path.display())
    })?;

    tracing::info!(
        "Wrote discovered_entries.json: {} discoveries, {} new entries",
        result.discoveries.len(),
        result.new_entries.len()
    );
    Ok(())
}

/// Parse a hex address string like "0x08804000" to u32.
fn parse_hex_addr(s: &str) -> Option<u32> {
    let trimmed = s
        .trim()
        .trim_start_matches("0x")
        .trim_start_matches("0X");
    u32::from_str_radix(trimmed, 16).ok()
}

/// Simple date string without chrono dependency.
fn chrono_date_string() -> String {
    // Use a fixed format since we don't have chrono
    "2026-02-20".to_string()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_hex_addr() {
        assert_eq!(parse_hex_addr("0x089ACDE4"), Some(0x089ACDE4));
        assert_eq!(parse_hex_addr("0x08804000"), Some(0x08804000));
        assert_eq!(parse_hex_addr("0X08804000"), Some(0x08804000));
    }

    #[test]
    fn test_is_valid_text_addr() {
        let stub_map = HashMap::new();
        assert!(is_valid_text_addr(0x089ACDE4, &stub_map));
        assert!(!is_valid_text_addr(0x00000000, &stub_map));
        assert!(!is_valid_text_addr(0x0A100000, &stub_map));
    }

    #[test]
    fn test_backward_scan_lui_addiu() {
        // Simulate: LUI r5, 0x089B; ADDIU r5, r5, 0xCDE4
        // -> address 0x089BCDE4
        let mut bytes = vec![0u8; 32 * 4];
        // LUI r5, 0x089B at offset 0
        let lui = (OP_LUI << 26) | (5 << 16) | 0x089B;
        bytes[0..4].copy_from_slice(&lui.to_le_bytes());
        // ADDIU r5, r5, 0xCDE4 at offset 4 (signed: -0x321C = 0xCDE4)
        let addiu = (OP_ADDIU << 26) | (5 << 21) | (5 << 16) | 0xCDE4;
        bytes[4..8].copy_from_slice(&addiu.to_le_bytes());
        // JAL (dummy) at offset 8
        let jal = OP_JAL << 26;
        bytes[8..12].copy_from_slice(&jal.to_le_bytes());

        let stub_map = HashMap::new();
        let results = scan_backward_for_funcptr(
            &bytes, 8, 5, 0x08800000, &stub_map,
        );
        // 0x089B << 16 = 0x089B0000
        // 0xCDE4 sign-extended = -0x321C = -12828
        // 0x089B0000 + (-12828) = 0x089ACDE4
        assert_eq!(results, vec![0x089ACDE4]);
    }

    #[test]
    fn test_backward_scan_lui_ori() {
        // LUI r5, 0x089B; ORI r5, r5, 0x0100 -> 0x089B0100
        let mut bytes = vec![0u8; 32 * 4];
        let lui = (OP_LUI << 26) | (5 << 16) | 0x089B;
        bytes[0..4].copy_from_slice(&lui.to_le_bytes());
        let ori = (OP_ORI << 26) | (5 << 21) | (5 << 16) | 0x0100;
        bytes[4..8].copy_from_slice(&ori.to_le_bytes());
        let jal = OP_JAL << 26;
        bytes[8..12].copy_from_slice(&jal.to_le_bytes());

        let stub_map = HashMap::new();
        let results = scan_backward_for_funcptr(
            &bytes, 8, 5, 0x08800000, &stub_map,
        );
        assert_eq!(results, vec![0x089B0100]);
    }

    #[test]
    fn test_backward_scan_different_source_reg() {
        // LUI r17, 0x089B; ADDIU r5, r17, 0xCDE4
        // -> 0x089ACDE4 (source reg differs from dest)
        let mut bytes = vec![0u8; 32 * 4];
        let lui = (OP_LUI << 26) | (17 << 16) | 0x089B;
        bytes[0..4].copy_from_slice(&lui.to_le_bytes());
        let addiu =
            (OP_ADDIU << 26) | (17 << 21) | (5 << 16) | 0xCDE4;
        bytes[4..8].copy_from_slice(&addiu.to_le_bytes());
        let jal = OP_JAL << 26;
        bytes[8..12].copy_from_slice(&jal.to_le_bytes());

        let stub_map = HashMap::new();
        let results = scan_backward_for_funcptr(
            &bytes, 8, 5, 0x08800000, &stub_map,
        );
        assert_eq!(results, vec![0x089ACDE4]);
    }

    /// Build a single-segment byte buffer at `base` from instruction words.
    fn seg(base: u32, words: &[u32]) -> Vec<(u32, Vec<u8>)> {
        let mut bytes = Vec::with_capacity(words.len() * 4);
        for w in words {
            bytes.extend_from_slice(&w.to_le_bytes());
        }
        vec![(base, bytes)]
    }

    const NOP: u32 = 0;
    const JR_RA: u32 = 0x03E0_0008;

    /// `bne $v0, $zero, <target>` at word index `i` targeting word index `t`.
    fn bnez_v0(i: u32, t: u32) -> u32 {
        let imm = (t as i32 - (i as i32 + 1)) as u32 & 0xFFFF;
        (0x05 << 26) | (2 << 21) | imm
    }

    #[test]
    fn test_framed_size_simple_first_epilogue() {
        // No branch past the epilogue: first epilogue is the end.
        let words = [
            0x27BD_FFF0, // addiu sp,sp,-0x10
            NOP,
            JR_RA,
            0x27BD_0010, // addiu sp,sp,+0x10 (teardown in delay slot)
            NOP,
        ];
        let s = seg(0x08800000, &words);
        assert_eq!(framed_function_size(0x08800000, 0x10, 0x100, &s), Some(16));
    }

    #[test]
    fn test_framed_size_post_epilogue_cold_tail() {
        // The issue-13 shape (FUN_088431E8): a bnez jumps PAST the epilogue
        // into a cold block that ends with `j` back into the body. The
        // function must extend over the cold tail, not stop at the epilogue.
        let words = [
            0x27BD_FFD0,    // 0x00 addiu sp,sp,-0x30
            NOP,            // 0x04
            bnez_v0(2, 8),  // 0x08 bnez v0 -> 0x20 (cold tail)
            NOP,            // 0x0C (delay)
            NOP,            // 0x10
            JR_RA,          // 0x14 jr ra
            0x27BD_0030,    // 0x18 addiu sp,sp,+0x30
            NOP,            // 0x1C
            NOP,            // 0x20 cold tail
            0x0A20_0004,    // 0x24 j 0x08800010 (back into the body)
            NOP,            // 0x28 (delay)
            0x27BD_FFE0,    // 0x2C next function's prologue
        ];
        let s = seg(0x08800000, &words);
        // End = after the cold tail's `j` delay slot: 0x2C bytes.
        assert_eq!(framed_function_size(0x08800000, 0x30, 0x100, &s), Some(0x2C));
    }

    #[test]
    fn test_branch_target_cop1() {
        // bc1tl 0x08856BDC at 0x08856B58: 0x45030020
        assert_eq!(branch_target(0x4503_0020, 0x08856B58), Some(0x08856BDC));
        // Non-BC COP1 (add.s) must not decode as a branch.
        assert_eq!(branch_target(0x4606_3300, 0x08856B5C), None);
    }

    #[test]
    fn test_framed_size_unresolvable_forward_target_falls_back() {
        // A forward branch target that never gets covered by a terminator:
        // fall back to the first-epilogue size (previous behavior).
        let mut words = vec![
            0x27BD_FFF0,     // 0x00 addiu sp,sp,-0x10
            bnez_v0(1, 0x40), // 0x04 bnez v0 -> word 0x40 (0x100 bytes in)
            NOP,             // 0x08 (delay)
            JR_RA,           // 0x0C jr ra
            0x27BD_0010,     // 0x10 addiu sp,sp,+0x10
        ];
        words.resize(0x80, NOP); // nops only; no terminator covers 0x100
        let s = seg(0x08800000, &words);
        assert_eq!(framed_function_size(0x08800000, 0x10, 0x200, &s), Some(0x14));
    }

    #[test]
    fn test_rescue_gap_branch_targets() {
        fn jf(addr: u32, size: u64) -> JsonFunction {
            JsonFunction {
                name: format!("FUN_{addr:08X}"),
                address: format!("0x{addr:08X}"),
                size,
                is_external: false,
                is_thunk: false,
                source: "binary_scan".into(),
            }
        }
        // A: 0x08800000..0x08800020 with a bnez into the unclaimed gap
        // (0x08800028) and one into B (0x08800044, claimed -> no rescue).
        // B: 0x08800040..0x08800050. Gap: 0x08800020..0x08800040.
        let mut words = vec![NOP; 0x40];
        words[2] = bnez_v0(2, 0x0A); // at 0x08 -> 0x28 (gap)
        words[4] = bnez_v0(4, 0x11); // at 0x10 -> 0x44 (inside B)
        let s = seg(0x08800000, &words);
        let mut funcs = vec![jf(0x08800000, 0x20), jf(0x08800040, 0x10)];
        let rescued = rescue_gap_branch_targets(
            &mut funcs, &s, 0x08800000, 0x08800100,
        );
        assert_eq!(rescued, vec![0x08800028]);
        assert_eq!(funcs.len(), 3);
        assert_eq!(funcs[2].address, "0x08800028");
        assert_eq!(funcs[2].source, "binary_scan");
    }

    #[test]
    fn test_correct_function_sizes() {
        let mut funcs = vec![
            JsonFunction {
                name: "FUN_0897EF4C".into(),
                address: "0x0897EF4C".into(),
                size: 204412,
                is_external: false,
                is_thunk: false,
                source: "ghidra".into(),
            },
            JsonFunction {
                name: "FUN_0897EF54".into(),
                address: "0x0897EF54".into(),
                size: 100,
                is_external: false,
                is_thunk: false,
                source: "ghidra".into(),
            },
        ];

        let corrections = correct_function_sizes(&mut funcs);
        assert_eq!(funcs[0].size, 8); // 0x0897EF54 - 0x0897EF4C = 8
        assert_eq!(corrections.len(), 1);
        assert_eq!(corrections[0].old_size, 204412);
        assert_eq!(corrections[0].new_size, 8);
    }
}
