//! Per-function C++ emission for the PSP recompiler.
//!
//! `emit_function` orchestrates emission by iterating `func.blocks` and dispatching
//! each `MipsOp` variant to the appropriate `Generator::emit_*` method.

use std::collections::HashMap;

use psp_ir::{DecodedFunction, MipsOp, Reg};

use crate::cpp_generator::CppGenerator;
use crate::generator::Generator;

/// Maps HLE stub virtual address → stub function name.
///
/// Plan 02-06 wires real import data. For Phase 2 this is a placeholder.
pub type ImportMap = HashMap<u32, String>;

/// Returns `true` if the instruction is a branch-likely variant.
///
/// Branch-likely instructions execute the delay slot ONLY when
/// the branch is taken. The next instruction in the IR stream
/// will be `MipsOp::DelaySlot { .. }`.
fn is_likely(op: &MipsOp) -> bool {
    matches!(
        op,
        MipsOp::Beq { likely: true, .. }
            | MipsOp::Bne { likely: true, .. }
            | MipsOp::Blez { likely: true, .. }
            | MipsOp::Bgtz { likely: true, .. }
            | MipsOp::Bltz { likely: true, .. }
            | MipsOp::Bgez { likely: true, .. }
            | MipsOp::Bltzal { likely: true, .. }
            | MipsOp::Bgezal { likely: true, .. }
            | MipsOp::Bc1t { likely: true, .. }
            | MipsOp::Bc1f { likely: true, .. }
            | MipsOp::VfpuBvf { likely: true, .. }
            | MipsOp::VfpuBvt { likely: true, .. }
    )
}

/// Capture the C++ code for a single MipsOp into a string.
///
/// Uses a temporary `CppGenerator` to render the instruction,
/// then returns the trimmed output. Used to obtain delay-slot
/// code for branch-likely emission.
fn capture_op_cpp(
    op: &MipsOp,
    imports: &ImportMap,
    func_start: u32,
    func_end: u32,
) -> String {
    let mut tmp = CppGenerator::new();
    emit_op(op, &mut tmp, imports, func_start, func_end);
    tmp.take_output().trim().to_string()
}

// =========================================================================
// Option-A intra-function LINK/RA model (coalesced functions only).
//
// A coalesced owner is ONE emitted C++ function that absorbed Ghidra-over-split
// shared-frame siblings. Inside it, an internal `jal`/`bltzal`/`bgezal` is a
// real subroutine call that SHARES the frame; the emitter must model the link
// register so the subroutine's `jr ra` resumes at the correct internal label
// instead of tearing down a frame it never allocated (the +896 leak class).
//
// - internal `jal X`  -> `ctx->r[31] = (int32_t)0xRET; goto L_X;`
// - internal `bltzal/bgezal X` -> `ctx->r[31] = 0xRET; if (cond) goto L_X;`
//   (the link write is UNCONDITIONAL on real Allegrex; only the branch is cond.)
// - EXIT `jr ra`      -> `return;`  (returns to the external C++ caller)
// - INTERNAL `jr ra`  -> `switch ((uint32_t)ctx->r[31]) { case RET: goto L_RET;
//                          ... default: return; }`
//
// The `default: return;` (NOT RECOMP_LOOKUP) is load-bearing: a stale
// `ctx->r[31]` left by a different caller returns harmlessly instead of MISSing.
// =========================================================================

/// Per-function link/return context for a coalesced owner.
struct RaCtx {
    /// All internal link-return labels (the `pos+4` resume address of each
    /// internal `jal`/`bltzal`/`bgezal`). Used to build the INTERNAL `jr ra`
    /// switch — the FULL set, not a per-`jr` subset (see doc-28 §5).
    link_returns: Vec<u32>,
}

/// True if `r` is the stack pointer ($sp == GPR 29).
fn ra_is_sp(r: Reg) -> bool {
    r == Reg::Gpr(29)
}

/// True if `r` is the return-address register ($ra == GPR 31).
fn ra_is_ra(r: Reg) -> bool {
    r == Reg::Gpr(31)
}

/// True if `op` is a frame-teardown `addiu sp, sp, +N` (the reordered delay slot
/// that sits immediately before the epilogue `jr ra`).
fn is_frame_teardown(op: &MipsOp) -> bool {
    matches!(op, MipsOp::Addiu { rt, rs, imm } if ra_is_sp(*rt) && ra_is_sp(*rs) && *imm > 0)
}

/// True if `op` reloads `ra` from the stack (`lw ra, N(sp)`) — the epilogue
/// restore of the saved outer return address.
fn is_ra_reload(op: &MipsOp) -> bool {
    matches!(op, MipsOp::Lw { rt, rs, .. } if ra_is_ra(*rt) && ra_is_sp(*rs))
}

/// True if `reg` is a callee-saved register the MIPS ABI requires a function to
/// preserve across calls: `$s0..$s7` (16–23), `$gp` (28), `$fp` (30), `$ra` (31).
fn is_callee_saved(reg: u8) -> bool {
    matches!(reg, 16..=23 | 28 | 30 | 31)
}

/// Reconstruct the missing epilogue for a no-`jr ra` function whose terminal op
/// is an EXTERNAL `jal` (a call-then-fall-through into the next block whose
/// downstream chain holds the real frame teardown — see `emit_function`).
///
/// Scans the whole sequence for the prologue `addiu sp, sp, -M` (frame size) and
/// every callee-saved spill `sw $r, off(sp)` (`$r` in `is_callee_saved`). If a
/// well-formed frame is found (negative sp-adjust + at least the `$ra` spill),
/// emits the matching `lw $r, off(sp)` restores followed by `addiu sp, sp, +M`,
/// so the function is transparent to its C++ caller's callee-saved registers and
/// stack pointer. Returns `true` when an epilogue was emitted.
///
/// This closes the recompiler LINK/RA faithfulness gap where such a function
/// returns to its C++ caller with its frame and `$s0..$s7` UNrestored, silently
/// corrupting the caller's callee-saved registers (e.g. the boot-app store base).
fn emit_reconstructed_epilogue(seq: &[MipsOp], gen: &mut dyn Generator) -> bool {
    let mut frame_size: Option<i16> = None;
    // Saved (reg, offset) pairs, in spill order. A reg may be spilled once.
    let mut saves: Vec<(u8, i16)> = Vec::new();
    for op in seq {
        match op {
            // Prologue frame allocation: first negative `addiu sp, sp, -M`.
            MipsOp::Addiu { rt, rs, imm }
                if ra_is_sp(*rt) && ra_is_sp(*rs) && *imm < 0 && frame_size.is_none() =>
            {
                frame_size = Some(-*imm);
            }
            // Callee-saved spill into the frame.
            MipsOp::Sw { rt, rs, offset } if ra_is_sp(*rs) => {
                if let Reg::Gpr(n) = *rt {
                    if is_callee_saved(n) && !saves.iter().any(|(r, _)| *r == n) {
                        saves.push((n, *offset));
                    }
                }
            }
            _ => {}
        }
    }
    // Require a real frame and at least the saved return address — otherwise this
    // is not a standard prologue and we must not synthesize a teardown.
    let Some(m) = frame_size else { return false };
    if !saves.iter().any(|(r, _)| *r == 31) {
        return false;
    }
    gen.emit_raw(
        "/* reconstructed epilogue: restore callee-saved + pop frame (terminal-jal */",
    );
    gen.emit_raw("/*    fall-through whose teardown lives in the downstream chain) */");
    for (reg, off) in &saves {
        gen.emit_raw(&format!(
            "ctx->r[{reg}] = (int32_t)(MEM_W(rdram, ctx->r[29] + {off}));"
        ));
    }
    gen.emit_raw(&format!("ctx->r[29] = (int32_t)(ctx->r[29] + {m});"));
    true
}

/// True if `op` ends a basic block / cannot be inside an epilogue prefix.
/// The backward walk in `jr_ra_is_exit` stops here so it never crosses a
/// prior function's epilogue (the boundary-stop guarantee, doc-28 §3).
fn is_control_transfer(op: &MipsOp) -> bool {
    matches!(
        op,
        MipsOp::Jr { .. }
            | MipsOp::Jalr { .. }
            | MipsOp::J { .. }
            | MipsOp::Jal { .. }
            | MipsOp::Beq { .. }
            | MipsOp::Bne { .. }
            | MipsOp::Blez { .. }
            | MipsOp::Bgtz { .. }
            | MipsOp::Bltz { .. }
            | MipsOp::Bgez { .. }
            | MipsOp::Bltzal { .. }
            | MipsOp::Bgezal { .. }
            | MipsOp::Bc1t { .. }
            | MipsOp::Bc1f { .. }
            | MipsOp::VfpuBvf { .. }
            | MipsOp::VfpuBvt { .. }
            | MipsOp::JumpTable { .. }
            | MipsOp::DelaySlot { .. }
            | MipsOp::BranchHazardDelay { .. }
    )
}

/// Classify the `jr ra` at `seq[i]` (must be `Jr { rs: Gpr(31) }`).
///
/// Returns `true` => EXIT (emit plain `return;`); `false` => INTERNAL (emit the
/// internal RA switch). Operates on the POST-reorder IR sequence: it walks
/// straight-line backward from the `Jr`, returning EXIT on the first ra-reload
/// or frame-teardown, and INTERNAL the moment it crosses a control-transfer
/// boundary or reaches the top with no epilogue marker. Verbatim doc-28 §2.
fn jr_ra_is_exit(seq: &[MipsOp], i: usize) -> bool {
    debug_assert!(matches!(&seq[i], MipsOp::Jr { rs } if ra_is_ra(*rs)));
    let mut k = i;
    while k > 0 {
        k -= 1;
        let op = &seq[k];
        if is_ra_reload(op) {
            return true;
        }
        if is_frame_teardown(op) {
            return true;
        }
        if is_control_transfer(op) {
            return false;
        }
        // plain straight-line op (lw s4, move, nop, ...) — keep walking
    }
    false
}

/// True if a coalesced function is self-contained: every call/link op stays
/// inside the owner range, so `ctx->r[31]` can only ever hold an internal
/// link-return. Disqualifiers: any `Jalr` (register-indirect external call),
/// any `Jal` to an out-of-range target, or any out-of-range linking branch
/// (`Bltzal`/`Bgezal`). When false, the RA model is NOT applied (baseline
/// lowering preserves correctness for owners that make external nested calls).
fn is_self_contained(func: &DecodedFunction, func_end: u32) -> bool {
    let in_range = |t: u32| t >= func.vaddr && t < func_end;
    for block in &func.blocks {
        for op in &block.instrs {
            let inner = match op {
                MipsOp::DelaySlot { instr } => instr.as_ref(),
                MipsOp::BranchHazardDelay { branch, .. } => branch.as_ref(),
                other => other,
            };
            match inner {
                MipsOp::Jalr { .. } => return false,
                MipsOp::Jal { target } if !in_range(*target) => return false,
                MipsOp::Bltzal { target, .. } | MipsOp::Bgezal { target, .. }
                    if !in_range(*target) =>
                {
                    return false
                }
                _ => {}
            }
        }
    }
    true
}

/// Build the link/return context for a coalesced function.
///
/// For every internal `jal`/`bltzal`/`bgezal` (target within `[start, end)`),
/// records the link-return label = positional `instr_vaddr + 4` (the decoder
/// reorders the standard call's delay slot BEFORE the op, so the resume label is
/// `pos+4`). The result is the FULL internal-link-return set the INTERNAL
/// `jr ra` switch dispatches over.
fn build_ra_ctx(seq: &[MipsOp], func_start: u32, func_end: u32) -> RaCtx {
    let mut link_returns: Vec<u32> = Vec::new();
    for (idx, op) in seq.iter().enumerate() {
        // (target, resume offset from this op's positional vaddr)
        let (target, resume_off) = match op {
            MipsOp::Jal { target } => (*target, 4),
            MipsOp::Bltzal { target, .. } | MipsOp::Bgezal { target, .. } => (*target, 4),
            // Hazard-fused link branch: the fused node sits at the BRANCH's own
            // word position (its delay slot lives inside it, padded by a Nop),
            // so the resume label is branch+8 — the word after the delay slot.
            MipsOp::BranchHazardDelay { branch, .. } => match branch.as_ref() {
                MipsOp::Bltzal { target, .. } | MipsOp::Bgezal { target, .. } => (*target, 8),
                _ => continue,
            },
            _ => continue,
        };
        if target >= func_start && target < func_end {
            // positional vaddr of this op = func_start + idx*4
            let pos = func_start.saturating_add(idx as u32 * 4);
            let ret = pos.saturating_add(resume_off);
            if !link_returns.contains(&ret) {
                link_returns.push(ret);
            }
        }
    }
    RaCtx { link_returns }
}

/// Emit the Option-A lowering for a LINK/RA op in a coalesced function.
///
/// Returns `true` if the op was handled here (caller skips `emit_op`), `false`
/// if it should fall through to the normal `emit_op` path (e.g. an external
/// `jal`, or a `jr` on a non-`ra` register).
#[allow(clippy::too_many_arguments)]
fn emit_coalesced_link_ra(
    op: &MipsOp,
    gen: &mut dyn Generator,
    ra_ctx: &RaCtx,
    seq: &[MipsOp],
    i: usize,
    instr_vaddr: u32,
    func_start: u32,
    func_end: u32,
    imports: &ImportMap,
) -> bool {
    match op {
        MipsOp::Jr { rs } if ra_is_ra(*rs) => {
            if jr_ra_is_exit(seq, i) {
                // EXIT: restore the caller's link register, then return.
                gen.emit_raw("ctx->r[31] = _ra_saved;");
                gen.emit_return();
            } else {
                emit_internal_ra_switch(gen, &ra_ctx.link_returns);
            }
            true
        }
        MipsOp::Jal { target } if *target >= func_start && *target < func_end => {
            let ret = instr_vaddr.saturating_add(4);
            gen.emit_raw(&format!("ctx->r[31] = (int32_t)0x{ret:08X};"));
            gen.emit_raw("_ra_active = true;");
            gen.emit_goto(&format!("L_{target:08X}"));
            true
        }
        MipsOp::Bltzal { rs, target, likely: false }
            if *target >= func_start && *target < func_end =>
        {
            let ret = instr_vaddr.saturating_add(4);
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_raw(&format!("ctx->r[31] = (int32_t)0x{ret:08X};"));
            gen.emit_raw("_ra_active = true;");
            gen.emit_raw(&format!(
                "if ((int32_t)({rs_s}) < 0) goto L_{target:08X};"
            ));
            true
        }
        MipsOp::Bgezal { rs, target, likely: false }
            if *target >= func_start && *target < func_end =>
        {
            let ret = instr_vaddr.saturating_add(4);
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_raw(&format!("ctx->r[31] = (int32_t)0x{ret:08X};"));
            gen.emit_raw("_ra_active = true;");
            gen.emit_raw(&format!(
                "if ((int32_t)({rs_s}) >= 0) goto L_{target:08X};"
            ));
            true
        }
        // Hazard-fused INTERNAL link branch: snapshot the condition register
        // BEFORE the delay slot runs (real MIPS order), run the delay slot,
        // write the link (resume = branch+8: the fused node spans branch +
        // delay), then branch on the snapshot.
        MipsOp::BranchHazardDelay { branch, delay } => {
            let (rs, target, cmp) = match branch.as_ref() {
                MipsOp::Bltzal { rs, target, likely: false } => (*rs, *target, "<"),
                MipsOp::Bgezal { rs, target, likely: false } => (*rs, *target, ">="),
                _ => return false,
            };
            if target < func_start || target >= func_end {
                return false;
            }
            let ret = instr_vaddr.saturating_add(8);
            let rs_s = gen.emit_gpr_read(rs);
            gen.emit_raw("{ /* delay-slot hazard: link-branch condition snapshotted pre-delay */");
            gen.emit_raw(&format!("const uint32_t _bh0 = (uint32_t)({rs_s});"));
            emit_op(delay, gen, imports, func_start, func_end);
            gen.emit_raw(&format!("ctx->r[31] = (int32_t)0x{ret:08X};"));
            gen.emit_raw("_ra_active = true;");
            gen.emit_raw(&format!("if ((int32_t)_bh0 {cmp} 0) goto L_{target:08X};"));
            gen.emit_raw("}");
            true
        }
        _ => false,
    }
}

/// Emit the INTERNAL `jr ra` switch: dispatch on `ctx->r[31]` to the internal
/// link-return label, `default: return;` (harmless on a stale value).
fn emit_internal_ra_switch(gen: &mut dyn Generator, link_returns: &[u32]) {
    // `_ra_active` is false until an INTERNAL link op runs. If this `jr ra` is
    // reached without one (external entry that never took an internal link), it
    // is an exit to the C++ caller: restore the saved link and return — never
    // dispatch on a coincidental incoming `ctx->r[31]`.
    gen.emit_raw("if (!_ra_active) { ctx->r[31] = _ra_saved; return; }");
    gen.emit_raw("switch ((uint32_t)ctx->r[31]) {");
    for &ret in link_returns {
        gen.emit_raw(&format!("    case 0x{ret:08X}U: goto L_{ret:08X};"));
    }
    // default: link not one of ours (shouldn't happen once active) — exit safely.
    gen.emit_raw("    default: ctx->r[31] = _ra_saved; return;");
    gen.emit_raw("}");
}

/// Emit a single decoded function as C++17 text.
///
/// Iterates all basic blocks in `func`, emitting a label for each block entry
/// and dispatching each `MipsOp` to the appropriate `gen.emit_*` method.
/// The `imports` map is used to resolve HLE stub calls (Plan 02-06 wires real data).
///
/// Branch-likely instructions are paired with their following
/// `MipsOp::DelaySlot` so the delay-slot code is emitted **inside**
/// the conditional block (executed only when the branch is taken).
///
/// # Arguments
///
/// * `func`    - Decoded function with basic blocks and metadata.
/// * `gen`     - Generator to receive emission calls.
/// * `imports` - HLE import map: vaddr → stub name.
pub fn emit_function(func: &DecodedFunction, gen: &mut dyn Generator, imports: &ImportMap) {
    gen.emit_function_start(&func.cpp_name, func.vaddr);
    let func_end = func.vaddr.saturating_add(func.size);

    // Decide the Option-A LINK/RA model up front (before any label/goto), so the
    // `_ra_saved`/`_ra_active` POD declarations can be placed before the
    // mid-entry dispatch. A coalesced owner is RA-modeled iff it is SELF-CONTAINED
    // (every call/link stays internal — see `is_self_contained`) AND has >=2
    // internal links (the genuine shared-frame multi-subroutine pattern that
    // produces the +896 epilogue leak, e.g. the decompressor's 8 links / 6
    // internal jr-ra switches). Single-internal-link owners gain nothing from the
    // model and only add cross-function RA interaction risk, so they keep
    // baseline lowering. Non-self-contained owners (e.g. the frame-tick, 42
    // external jals) likewise keep baseline `jal`/`jr ra` lowering; their
    // structural coalesce (cross-piece branches -> gotos) is still applied.
    let ra_ctx: Option<RaCtx> = if func.coalesced && is_self_contained(func, func_end) {
        let seq = func.blocks.first().map(|b| b.instrs.as_slice()).unwrap_or(&[]);
        let ctx = build_ra_ctx(seq, func.vaddr, func_end);
        if ctx.link_returns.len() >= 2 { Some(ctx) } else { None }
    } else {
        None
    };
    if ra_ctx.is_some() {
        // `_ra_saved`: incoming caller link, restored at EXIT (transparency).
        // `_ra_active`: set true only after an INTERNAL link op runs, so an
        // INTERNAL `jr ra` reached on an external-entry path that never took an
        // internal link dispatches `default` (return) instead of matching a
        // coincidental incoming `ctx->r[31]`. Both are POD decls before any
        // label so the mid-entry / internal gotos may legally cross them.
        gen.emit_raw("uint32_t _ra_saved;");
        gen.emit_raw("bool _ra_active;");
    }

    // Mid-entry dispatch (faithful, re-entrant — see 19H).
    //
    // `ctx->entry_point` is a single shared global. A mid-entry `_entry` wrapper
    // sets it immediately before calling its specific parent; the parent's
    // prologue (below) is supposed to consume it. The naive form
    // (`if (entry_point != 0) switch { case X: entry_point=0; goto X; }`) clears
    // the global ONLY on a matching case. With hundreds of mid-entry parents that
    // is unsafe: a non-zero value that does NOT match this function's cases falls
    // straight through into the normal body with the global still set, and then
    // leaks into the next nested call — whose prologue may match it by accident
    // and jump to the wrong label (the cross-contamination of 19G §3).
    //
    // The fix snapshots the global into a local and CLEARS the global
    // unconditionally before any body control flow or nested call. Invariant:
    // `ctx->entry_point` is 0 during all body execution and all nested calls of
    // every function, so a stale value can never propagate past the first parent
    // that observes it (it is absorbed in at most one hop). A matched case still
    // jumps to its label; an unmatched value (including 0) falls through to the
    // normal entry exactly as a clean normal call would. This is correct for any
    // number of mid-entry parents and benefits all code.
    if !func.mid_entry_addrs.is_empty() {
        gen.emit_raw("{");
        gen.emit_raw("    uint32_t _ep = ctx->entry_point;");
        gen.emit_raw("    ctx->entry_point = 0;");
        gen.emit_raw("    switch (_ep) {");
        for &addr in &func.mid_entry_addrs {
            gen.emit_raw(&format!(
                "        case 0x{addr:08X}U: goto L_{addr:08X};",
            ));
        }
        gen.emit_raw("        default: break;");
        gen.emit_raw("    }");
        gen.emit_raw("}");
    }

    // For a self-contained RA-modeled function, save the incoming `ctx->r[31]`
    // and restore it at every EXIT `jr ra`. This makes the function transparent
    // to its external C++ caller's link register: it never leaks its own internal
    // link-return to the outside, so a later RA-modeled function cannot observe a
    // stale internal link from THIS one and mis-dispatch its INTERNAL `jr ra`
    // switch by a coincidental case match. This assignment (separate from the
    // declaration above) may be legally crossed by a mid-entry / internal goto;
    // EXIT restore is emitted by `emit_coalesced_link_ra` / the switch default.
    if ra_ctx.is_some() {
        gen.emit_raw("_ra_saved = (uint32_t)ctx->r[31];");
        gen.emit_raw("_ra_active = false;");
    }

    // A function with NO `jr ra` whose terminal op is an EXTERNAL `jal` is a
    // call-then-fall-through: its own frame teardown lives downstream in the
    // tail chain (see `emit_reconstructed_epilogue`). When such a function is
    // RA-modeled (coalesced + self-contained), its internal `jr ra` switch
    // already handles exits, so this reconstruction is for the BASELINE path
    // only. We synthesize the missing epilogue after the terminal call so the
    // function is transparent to its C++ caller's callee-saved registers / sp.
    let reconstruct_terminal_epilogue = ra_ctx.is_none() && !func_has_jr_ra(func);
    // Set when `emit_reconstructed_epilogue` actually emitted a teardown —
    // MUTUALLY EXCLUSIVE with the fall-through tail below (see there).
    let mut epilogue_reconstructed = false;

    for block in &func.blocks {
        let instrs = &block.instrs;
        let is_last_block = std::ptr::eq(block, func.blocks.last().unwrap());
        let mut i = 0;
        while i < instrs.len() {
            let instr_vaddr = block.vaddr.saturating_add(i as u32 * 4);
            gen.emit_label(&format!("L_{instr_vaddr:08X}"));

            // Instruction-budget preemption point (#66, design approach (a)):
            // if this op is an in-function loop back-edge (static target at or
            // before this address), emit the budget decrement + sched_preempt
            // check BEFORE the branch, so a syscall-free busy-poll loop (the
            // .hack CRI-ring consumer) reaches a reschedule point on a bounded
            // budget. DEFAULT-OFF: sched_preempt is a no-op unless
            // PSPRECOMP_PREEMPT=1, so this leaves guest behavior identical.
            // Covers all three lowering paths below (BIDS, likely, plain) since
            // each emits the op at `instrs[i]`.
            if backward_branch_target(&instrs[i], instr_vaddr, func.vaddr).is_some() {
                emit_preempt_point(gen);
            }

            // BIDS pairing (issue #56): the next node is a DelaySlotRejoin —
            // this branch's delay-slot ADDRESS is itself a branch target.
            // Emit the branch/delay block at this position, an unconditional
            // skip (fall-through must not re-execute the duplicate), then the
            // delay-slot label holding a duplicated copy of the instruction
            // so back-edges INTO the slot get hardware semantics.
            if let Some(MipsOp::DelaySlotRejoin { instr: dup }) = instrs.get(i + 1) {
                emit_bids_pair(
                    &instrs[i], dup, gen, &ra_ctx, instrs, i, instr_vaddr, func.vaddr,
                    func_end, imports,
                );
                i += 2;
                continue;
            }

            // Option-A LINK/RA lowering for coalesced owners (intercepts
            // internal jal/bltzal/bgezal and jr ra; external/other ops fall
            // through to the normal path below).
            if let Some(ref rc) = ra_ctx {
                if emit_coalesced_link_ra(
                    &instrs[i], gen, rc, instrs, i, instr_vaddr, func.vaddr, func_end, imports,
                ) {
                    i += 1;
                    continue;
                }
            }

            // Branch-likely + DelaySlot pairing: when the current
            // instruction is a branch-likely, peek at the next
            // instruction. If it is a DelaySlot, capture its C++
            // code and pass it into emit_branch_or_tail so the
            // delay-slot code is emitted inside the if-block.
            if is_likely(&instrs[i]) {
                if let Some(MipsOp::DelaySlot { instr }) =
                    instrs.get(i + 1)
                {
                    let ds_cpp = capture_op_cpp(
                        instr,
                        imports,
                        func.vaddr,
                        func_end,
                    );
                    emit_op_with_ds(
                        &instrs[i],
                        gen,
                        imports,
                        func.vaddr,
                        func_end,
                        &ds_cpp,
                    );
                    // Emit label for the delay-slot address
                    // (other branches may target it) but skip
                    // re-emitting its code.
                    let ds_vaddr =
                        block.vaddr.saturating_add((i + 1) as u32 * 4);
                    gen.emit_label(&format!("L_{ds_vaddr:08X}"));
                    i += 2;
                    continue;
                }
            }

            emit_op(&instrs[i], gen, imports, func.vaddr, func_end);

            // Terminal-`jal` epilogue reconstruction (recompiler LINK/RA class):
            // if this is the function's last op and an EXTERNAL `jal`, and the
            // function has no `jr ra` of its own, restore the callee-saved regs
            // it spilled and pop its frame — the teardown the real code only
            // reaches via the downstream tail chain.
            if reconstruct_terminal_epilogue
                && is_last_block
                && is_terminal_external_jal(instrs, i, func.vaddr, func_end)
            {
                epilogue_reconstructed = emit_reconstructed_epilogue(instrs, gen);
            }
            i += 1;
        }
    }

    // Fall-through tail (dothack L5, the `FUN_08851A24` class): if the body's
    // end is reachable, hardware falls through into the code at `func_end`,
    // but the C++ body would run off its closing brace and silently return
    // with stale state (e.g. a caller-preset error v0). Dispatch to the next
    // address instead — full guest state lives in `ctx`, and the nested host
    // frames unwind when the downstream `jr ra` finally returns. Targets
    // absent from the dispatch table become LOUD LOOKUP_MISSes and are
    // recorded by the #37 dispatch audit via `emit_call_lookup`.
    //
    // MUTUALLY EXCLUSIVE with the reconstructed terminal-jal epilogue above:
    // that path already synthesized the downstream chain's frame teardown —
    // also dispatching into the chain would pop the frame a second time.
    //
    // `func_end > func.vaddr` guards the degenerate size-0 shape against
    // self-dispatch recursion.
    if !epilogue_reconstructed
        && func_end > func.vaddr
        && body_can_fall_off_end(func, ra_ctx.is_some(), func_end)
    {
        gen.emit_raw("/* fall-through past function end (no terminal control flow) */");
        gen.emit_call_lookup(func_end);
        gen.emit_return();
    }

    gen.emit_function_end();
}

/// True if any op in the function is an EXIT `jr ra` (`jr $ra`). Functions that
/// already end with their own `jr ra` epilogue need no reconstruction.
fn func_has_jr_ra(func: &DecodedFunction) -> bool {
    func.blocks.iter().flat_map(|b| b.instrs.iter()).any(|op| {
        let inner = match op {
            MipsOp::DelaySlot { instr } => instr.as_ref(),
            other => other,
        };
        matches!(inner, MipsOp::Jr { rs } if ra_is_ra(*rs))
    })
}

/// True if `seq[i]` is the function's terminal EXTERNAL `jal target` — the last
/// meaningful op (only a trailing reordered `DelaySlot` may follow) whose target
/// is outside `[func_start, func_end)`. This is the call-then-fall-through
/// pattern whose frame teardown lives in the downstream tail chain.
fn is_terminal_external_jal(seq: &[MipsOp], i: usize, func_start: u32, func_end: u32) -> bool {
    let MipsOp::Jal { target } = &seq[i] else {
        return false;
    };
    if *target >= func_start && *target < func_end {
        return false; // internal jal — handled by goto / RA lowering
    }
    i + 1 == seq.len()
        || (i + 2 == seq.len() && matches!(seq[i + 1], MipsOp::DelaySlot { .. }))
}

/// True if `r` is the hardwired-zero register.
fn is_zero_reg(r: Reg) -> bool {
    matches!(r, Reg::Zero | Reg::Gpr(0))
}

/// Detect an in-function loop back-edge: a control op whose static target lies
/// at-or-before its own positional address (`target <= instr_vaddr`) and within
/// the function (`target >= func_start`). This is the structural signature of a
/// loop's bottom branch — including a degenerate `b .` busy-poll spin (the .hack
/// CRI-ring consumer, #66).
///
/// Returns the back-edge `target` when `op` is such a branch, else `None`.
/// Conditional branches (beq/bne/bgez/.../bc1t/bc1f) and unconditional jumps
/// (`j`) are considered; `jal`/`bltzal`/`bgezal` (calls/links) and register-
/// indirect jumps (`jr`/`jalr`, no static target) are NOT — a call is forward
/// progress, and an indirect jump has no statically-known back-edge to gate on.
/// A `BranchHazardDelay` is unwrapped to its inner branch.
///
/// # Arguments
///
/// * `op` - the candidate control op at `instr_vaddr`.
/// * `instr_vaddr` - positional virtual address of `op`.
/// * `func_start` - function entry vaddr (lower bound for an in-function target).
fn backward_branch_target(op: &MipsOp, instr_vaddr: u32, func_start: u32) -> Option<u32> {
    let target = match op {
        MipsOp::Beq { target, .. }
        | MipsOp::Bne { target, .. }
        | MipsOp::Bgez { target, .. }
        | MipsOp::Bgtz { target, .. }
        | MipsOp::Blez { target, .. }
        | MipsOp::Bltz { target, .. }
        | MipsOp::Bc1t { target, .. }
        | MipsOp::Bc1f { target, .. }
        | MipsOp::J { target } => *target,
        MipsOp::BranchHazardDelay { branch, .. } => {
            return backward_branch_target(branch, instr_vaddr, func_start);
        }
        _ => return None,
    };
    if target >= func_start && target <= instr_vaddr {
        Some(target)
    } else {
        None
    }
}

/// Emit the instruction-budget preemption point (#66, design approach (a)) for a
/// loop back-edge. The decrement is a dead-effect counter and `sched_preempt`
/// is a no-op unless `PSPRECOMP_PREEMPT=1` (see runtime `psp_scheduler.cpp`), so
/// the FLAG-OFF default leaves guest behavior identical. Placed before the
/// back-edge branch so every loop-bottom pass counts one iteration.
fn emit_preempt_point(gen: &mut dyn Generator) {
    gen.emit_raw("if (--ctx->preempt_budget <= 0) sched_preempt(ctx);");
}

/// True if the emitted lowering of `op` unconditionally ends guest control
/// flow — execution cannot fall past it into the next instruction slot.
///
/// Used by the fall-through tail decision (see `emit_function`): a function
/// whose last meaningful op does NOT end flow would run off the C++ body and
/// silently return with stale state (the dothack CRI-FS `FUN_08851A24` class,
/// the function-granularity sibling of BIDS #56).
fn op_ends_guest_flow(op: &MipsOp, ra_modeled: bool, func_start: u32, func_end: u32) -> bool {
    match op {
        // `jr ra` lowers to `return;` (or the internal RA switch, whose every
        // arm gotos or returns); `jr reg` lowers to dispatch + return.
        MipsOp::Jr { .. } => true,
        // `j` lowers to an unconditional goto (internal target) or to
        // dispatch + return (external tail jump).
        MipsOp::J { .. } => true,
        // Jump table: every case gotos; the default dispatches + returns.
        MipsOp::JumpTable { .. } => true,
        // RA-modeled INTERNAL `jal` lowers to a link write + unconditional
        // goto (see `emit_coalesced_link_ra`). The baseline internal/external
        // `jal` is a call that falls through and must NOT match here.
        MipsOp::Jal { target } if ra_modeled && *target >= func_start && *target < func_end => {
            true
        }
        // Always-taken non-linking branches: `b` (beq r,r), `bgez zero`,
        // `blez zero` — the not-taken fall-through path is statically dead.
        MipsOp::Beq { rs, rt, .. } if rs == rt => true,
        MipsOp::Bgez { rs, .. } | MipsOp::Blez { rs, .. } if is_zero_reg(*rs) => true,
        // Hazard-fused control: ends flow iff the fused op itself does
        // (`jr` → dispatch + return; jump table → switch). A fused `jalr` is
        // a call and fused conditional branches fall through.
        MipsOp::BranchHazardDelay { branch, .. } => matches!(
            branch.as_ref(),
            MipsOp::Jr { .. } | MipsOp::JumpTable { .. }
        ),
        _ => false,
    }
}

/// True if the emitted body's end is reachable: the last meaningful op of the
/// final block (looking through a trailing `DelaySlot` to the control op that
/// owns it) does not unconditionally end guest flow, so hardware would fall
/// through into the code at `func_end` while the C++ body silently returns.
fn body_can_fall_off_end(func: &DecodedFunction, ra_modeled: bool, func_end: u32) -> bool {
    let Some(seq) = func.blocks.last().map(|b| b.instrs.as_slice()) else {
        return true;
    };
    let last = match seq.last() {
        None => return true,
        // Trailing `DelaySlot` (a likely branch's slot kept after the branch,
        // or a terminal call's slot): the owning control op decides.
        Some(MipsOp::DelaySlot { .. }) if seq.len() >= 2 => &seq[seq.len() - 2],
        Some(op) => op,
    };
    !op_ends_guest_flow(last, ra_modeled, func.vaddr, func_end)
}

/// Emit a fused BIDS pair (issue #56): a branch whose delay-slot ADDRESS is
/// itself an in-function branch target, followed by its `DelaySlotRejoin`
/// duplicate. Lowering:
///
/// ```text
/// L_A:  { branch/delay block (hazard snapshot or likely taken-path copy) }
///       goto L_A8;     // fall-through must NOT re-execute the duplicate
/// L_A4: <delay instr>  // back-edges into the slot get hardware semantics
/// L_A8: ...
/// ```
///
/// `op` is the decoder-produced node at the branch position: a
/// `BranchHazardDelay` (non-likely forms) or a bare branch-likely. Coalesced
/// RA-modeled link branches are routed through `emit_coalesced_link_ra` first
/// so internal `bltzal`/`bgezal` BIDS sites keep the link write.
#[allow(clippy::too_many_arguments)]
fn emit_bids_pair(
    op: &MipsOp,
    dup: &MipsOp,
    gen: &mut dyn Generator,
    ra_ctx: &Option<RaCtx>,
    seq: &[MipsOp],
    i: usize,
    instr_vaddr: u32,
    func_start: u32,
    func_end: u32,
    imports: &ImportMap,
) {
    let ra_handled = match ra_ctx {
        Some(rc) => emit_coalesced_link_ra(
            op, gen, rc, seq, i, instr_vaddr, func_start, func_end, imports,
        ),
        None => false,
    };
    if !ra_handled {
        if is_likely(op) {
            // Likely branch: the conditional delay-slot copy runs inside the
            // taken path only; the duplicate below serves direct entries.
            let ds_cpp = capture_op_cpp(dup, imports, func_start, func_end);
            emit_op_with_ds(op, gen, imports, func_start, func_end, &ds_cpp);
        } else {
            emit_op(op, gen, imports, func_start, func_end);
        }
    }

    // Unconditional skip over the duplicate, then the delay-slot label.
    let ds_vaddr = instr_vaddr.wrapping_add(4);
    let rejoin = instr_vaddr.wrapping_add(8);
    if rejoin < func_end {
        gen.emit_goto(&format!("L_{rejoin:08X}"));
    } else {
        // Pair sits at the very end of the function: the not-taken path
        // continues at the rejoin point (== func_end) — a fall-through past
        // the function end, same class as the end-of-body tail in
        // `emit_function`. Dispatch there; never silently return.
        gen.emit_raw("/* BIDS skip: rejoin beyond function end — fall through */");
        gen.emit_call_lookup(rejoin);
        gen.emit_return();
    }
    gen.emit_label(&format!("L_{ds_vaddr:08X}"));
    emit_op(dup, gen, imports, func_start, func_end);
}

/// Emit a branch instruction, or a conditional tail call if the target is outside
/// the current function's address range.
///
/// In MIPS code, it's legal to branch to an address inside a different function
/// (tail-call to a mid-label). Since C++ does not allow cross-function `goto`,
/// out-of-bounds targets are emitted as `RECOMP_LOOKUP(addr)(rdram, ctx); return;`.
#[allow(clippy::too_many_arguments)]
pub(crate) fn emit_branch_or_tail(
    gen: &mut dyn Generator,
    cond: &str,
    target: u32,
    likely: bool,
    ds_comment: &str,
    func_start: u32,
    func_end: u32,
) {
    if target >= func_start && target < func_end {
        // In-function branch: use C++ goto label
        let target_pc = format!("L_{target:08X}");
        if likely {
            gen.emit_branch_likely(cond, &target_pc, ds_comment);
        } else {
            gen.emit_branch(cond, &target_pc);
        }
    } else {
        // Cross-function branch: conditional tail call via dispatch table
        gen.note_static_lookup(target);
        gen.emit_raw(&format!(
            "if ({cond}) {{ RECOMP_LOOKUP(0x{target:08X}u)(rdram, ctx); return; }}"
        ));
    }
}

/// Emit a branch-likely instruction with its captured delay-slot C++ code.
///
/// This is only called for branch-likely instructions whose delay slot
/// has been captured via `capture_op_cpp`. The delay-slot code is placed
/// inside the conditional block so it executes only when the branch is
/// taken (correct MIPS branch-likely semantics).
#[allow(clippy::too_many_arguments)]
fn emit_op_with_ds(
    op: &MipsOp,
    gen: &mut dyn Generator,
    _imports: &ImportMap,
    func_start: u32,
    func_end: u32,
    ds_cpp: &str,
) {
    match op {
        MipsOp::Beq { rs, rt, target, .. } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            let cond = format!("{rs_s} == {rt_s}");
            emit_branch_or_tail(gen, &cond, *target, true, ds_cpp, func_start, func_end);
        }
        MipsOp::Bne { rs, rt, target, .. } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            let cond = format!("{rs_s} != {rt_s}");
            emit_branch_or_tail(gen, &cond, *target, true, ds_cpp, func_start, func_end);
        }
        MipsOp::Blez { rs, target, .. } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let cond = format!("(int32_t)({rs_s}) <= 0");
            emit_branch_or_tail(gen, &cond, *target, true, ds_cpp, func_start, func_end);
        }
        MipsOp::Bgtz { rs, target, .. } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let cond = format!("(int32_t)({rs_s}) > 0");
            emit_branch_or_tail(gen, &cond, *target, true, ds_cpp, func_start, func_end);
        }
        MipsOp::Bltz { rs, target, .. } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let cond = format!("(int32_t)({rs_s}) < 0");
            emit_branch_or_tail(gen, &cond, *target, true, ds_cpp, func_start, func_end);
        }
        MipsOp::Bgez { rs, target, .. } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let cond = format!("(int32_t)({rs_s}) >= 0");
            emit_branch_or_tail(gen, &cond, *target, true, ds_cpp, func_start, func_end);
        }
        MipsOp::Bltzal { rs, target, .. } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let cond = format!("(int32_t)({rs_s}) < 0");
            emit_branch_or_tail(gen, &cond, *target, true, ds_cpp, func_start, func_end);
        }
        MipsOp::Bgezal { rs, target, .. } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let cond = format!("(int32_t)({rs_s}) >= 0");
            emit_branch_or_tail(gen, &cond, *target, true, ds_cpp, func_start, func_end);
        }
        MipsOp::Bc1t { target, .. } => {
            let cc = gen.emit_fpu_cc_read();
            let cond = format!("{cc} != 0");
            emit_branch_or_tail(gen, &cond, *target, true, ds_cpp, func_start, func_end);
        }
        MipsOp::Bc1f { target, .. } => {
            let cc = gen.emit_fpu_cc_read();
            let cond = format!("{cc} == 0");
            emit_branch_or_tail(gen, &cond, *target, true, ds_cpp, func_start, func_end);
        }
        _ => {
            // Should not happen: only branch-likely ops reach here
            emit_op(op, gen, _imports, func_start, func_end);
        }
    }
}

/// Emit pre-delay snapshot declarations for the values `branch` reads.
///
/// Declares `const uint32_t _bh0` (and `_bh1` for two-source compares) holding
/// the register / condition-flag values BEFORE the delay slot executes.
/// Returns the snapshot expression names (second string empty when unused).
fn emit_hazard_snapshots(branch: &MipsOp, gen: &mut dyn Generator) -> (String, String) {
    match branch {
        MipsOp::Beq { rs, rt, .. } | MipsOp::Bne { rs, rt, .. } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!("const uint32_t _bh0 = (uint32_t)({rs_s});"));
            gen.emit_raw(&format!("const uint32_t _bh1 = (uint32_t)({rt_s});"));
            ("_bh0".into(), "_bh1".into())
        }
        // `jr $ra` is excluded: it lowers to a plain `return;` (the register
        // VALUE is never consumed), so a BIDS-fused `jr ra` (issue #56) must
        // take the no-snapshot fallback `{ delay; return; }` — dispatching on
        // a snapshotted r31 would break the C++-native return model.
        MipsOp::Jr { rs } if *rs != Reg::Gpr(31) => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_raw(&format!("const uint32_t _bh0 = (uint32_t)({rs_s});"));
            ("_bh0".into(), String::new())
        }
        MipsOp::Blez { rs, .. }
        | MipsOp::Bgtz { rs, .. }
        | MipsOp::Bltz { rs, .. }
        | MipsOp::Bgez { rs, .. }
        | MipsOp::Bltzal { rs, .. }
        | MipsOp::Bgezal { rs, .. }
        | MipsOp::Jalr { rs, .. } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_raw(&format!("const uint32_t _bh0 = (uint32_t)({rs_s});"));
            ("_bh0".into(), String::new())
        }
        MipsOp::JumpTable { index_reg, .. } => {
            let idx_s = gen.emit_gpr_read(*index_reg);
            gen.emit_raw(&format!("const uint32_t _bh0 = (uint32_t)({idx_s});"));
            ("_bh0".into(), String::new())
        }
        MipsOp::Bc1t { .. } | MipsOp::Bc1f { .. } => {
            let cc = gen.emit_fpu_cc_read();
            gen.emit_raw(&format!("const uint32_t _bh0 = (uint32_t)({cc});"));
            ("_bh0".into(), String::new())
        }
        MipsOp::VfpuBvf { .. } | MipsOp::VfpuBvt { .. } => {
            gen.emit_raw(&format!(
                "const uint32_t _bh0 = (uint32_t)(ctx->vfpu_ctrl[{}]);",
                crate::vfpu::VFPU_CTRL_CC
            ));
            ("_bh0".into(), String::new())
        }
        _ => (String::new(), String::new()),
    }
}

/// Build the snapshot-based condition + target for a hazard-fused conditional
/// branch. Returns `None` for non-conditional control ops (`jr`/`jalr`/
/// `JumpTable`), which dispatch on the snapshot instead of testing it.
fn hazard_cond_target(branch: &MipsOp, s0: &str, s1: &str) -> Option<(String, u32)> {
    match branch {
        MipsOp::Beq { target, .. } => Some((format!("{s0} == {s1}"), *target)),
        MipsOp::Bne { target, .. } => Some((format!("{s0} != {s1}"), *target)),
        MipsOp::Blez { target, .. } => Some((format!("(int32_t)({s0}) <= 0"), *target)),
        MipsOp::Bgtz { target, .. } => Some((format!("(int32_t)({s0}) > 0"), *target)),
        MipsOp::Bltz { target, .. } | MipsOp::Bltzal { target, .. } => {
            Some((format!("(int32_t)({s0}) < 0"), *target))
        }
        MipsOp::Bgez { target, .. } | MipsOp::Bgezal { target, .. } => {
            Some((format!("(int32_t)({s0}) >= 0"), *target))
        }
        MipsOp::Bc1t { target, .. } => Some((format!("{s0} != 0"), *target)),
        MipsOp::Bc1f { target, .. } => Some((format!("{s0} == 0"), *target)),
        MipsOp::VfpuBvf { cc, target, .. } => {
            Some((format!("!({s0} & (1u << {cc}))"), *target))
        }
        MipsOp::VfpuBvt { cc, target, .. } => {
            Some((format!("({s0} & (1u << {cc}))"), *target))
        }
        _ => None,
    }
}

/// Emit a hazard-fused branch + delay-slot pair (`MipsOp::BranchHazardDelay`).
///
/// The delay slot writes a register (or FPU/VFPU condition flag) the branch
/// reads. Real MIPS evaluates the condition / captures the jump target BEFORE
/// the delay slot executes, so the emitted code snapshots the pre-delay values
/// into block-local consts, runs the delay slot exactly once (it executes on
/// both the taken and fall-through paths), then branches on the snapshot.
fn emit_branch_hazard_delay(
    branch: &MipsOp,
    delay: &MipsOp,
    gen: &mut dyn Generator,
    imports: &ImportMap,
    func_start: u32,
    func_end: u32,
) {
    gen.emit_raw("{ /* delay-slot hazard: condition/target uses pre-delay snapshot */");
    let (s0, s1) = emit_hazard_snapshots(branch, gen);
    if s0.is_empty() {
        // Not a hazard-capable control op (defensive): fall back to plain swap.
        emit_op(delay, gen, imports, func_start, func_end);
        emit_op(branch, gen, imports, func_start, func_end);
        gen.emit_raw("}");
        return;
    }
    emit_op(delay, gen, imports, func_start, func_end);
    match branch {
        MipsOp::Jr { .. } => {
            gen.emit_call_lookup_reg(&s0);
            gen.emit_return();
        }
        MipsOp::Jalr { .. } => {
            gen.emit_call_lookup_reg(&s0);
        }
        MipsOp::JumpTable { cases, .. } => {
            let case_pairs: Vec<(u32, String)> = cases
                .iter()
                .map(|&addr| (addr, format!("L_{addr:08X}")))
                .collect();
            gen.emit_switch(&s0, &case_pairs);
        }
        _ => {
            if let Some((cond, target)) = hazard_cond_target(branch, &s0, &s1) {
                emit_branch_or_tail(gen, &cond, target, false, "", func_start, func_end);
            }
        }
    }
    gen.emit_raw("}");
}

/// Dispatch a single MipsOp to the appropriate Generator emit method.
///
/// `func_start`/`func_end`: current function's virtual address range.
/// Branch targets outside `[func_start, func_end)` are emitted as tail calls
/// via `RECOMP_LOOKUP` to avoid cross-function C++ goto labels.
#[allow(clippy::too_many_lines)]
fn emit_op(
    op: &MipsOp,
    gen: &mut dyn Generator,
    imports: &ImportMap,
    func_start: u32,
    func_end: u32,
) {
    match op {
        // -----------------------------------------------------------------
        // Shifts
        // -----------------------------------------------------------------
        MipsOp::Sll { rd, rt, sa } => {
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("(uint32_t)({rt_s}) << {sa}"));
        }
        MipsOp::Srl { rd, rt, sa } => {
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("(uint32_t)({rt_s}) >> {sa}"));
        }
        MipsOp::Sra { rd, rt, sa } => {
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("(int32_t)({rt_s}) >> {sa}"));
        }
        MipsOp::Sllv { rd, rt, rs } => {
            let rt_s = gen.emit_gpr_read(*rt);
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rd, &format!("(uint32_t)({rt_s}) << ({rs_s} & 0x1F)"));
        }
        MipsOp::Srlv { rd, rt, rs } => {
            let rt_s = gen.emit_gpr_read(*rt);
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rd, &format!("(uint32_t)({rt_s}) >> ({rs_s} & 0x1F)"));
        }
        MipsOp::Srav { rd, rt, rs } => {
            let rt_s = gen.emit_gpr_read(*rt);
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rd, &format!("(int32_t)({rt_s}) >> ({rs_s} & 0x1F)"));
        }

        // -----------------------------------------------------------------
        // Allegrex Rotate
        // -----------------------------------------------------------------
        MipsOp::Rotr { rd, rt, sa } => {
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(
                *rd,
                &format!(
                    "((uint32_t)({rt_s}) >> {sa}) | ((uint32_t)({rt_s}) << (32 - {sa}))"
                ),
            );
        }
        MipsOp::Rotrv { rd, rt, rs } => {
            let rt_s = gen.emit_gpr_read(*rt);
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(
                *rd,
                &format!(
                    "((uint32_t)({rt_s}) >> ({rs_s} & 0x1F)) | \
                     ((uint32_t)({rt_s}) << (32 - ({rs_s} & 0x1F)))"
                ),
            );
        }

        // -----------------------------------------------------------------
        // Jumps / returns
        // -----------------------------------------------------------------
        MipsOp::Jr { rs } => {
            if *rs == Reg::Gpr(31) {
                // jr $ra — function return
                gen.emit_return();
            } else {
                // jr $reg — indirect jump (tail call via register)
                let rs_s = gen.emit_gpr_read(*rs);
                gen.emit_call_lookup_reg(&rs_s);
                gen.emit_return();
            }
        }
        MipsOp::Jalr { rd, rs } => {
            let rs_s = gen.emit_gpr_read(*rs);
            if *rd != Reg::Zero {
                // Return address (PC+8) -- in static recompilation this is a no-op;
                // the caller's return path is handled by the C++ function return.
            }
            gen.emit_call_lookup_reg(&rs_s);
        }

        // -----------------------------------------------------------------
        // Traps — emit as comments (semantics not needed for translation)
        // -----------------------------------------------------------------
        MipsOp::Syscall { code } => {
            gen.emit_raw(&format!("psp_hle_syscall(rdram, ctx, 0x{code:05X}U);"));
        }
        MipsOp::Break_ { code } => {
            gen.emit_raw(&format!("/* BREAK 0x{code:05X} */;"));
        }

        // -----------------------------------------------------------------
        // HI/LO moves
        // -----------------------------------------------------------------
        MipsOp::Mfhi { rd } => {
            gen.emit_gpr_write(*rd, "ctx->hi");
        }
        MipsOp::Mthi { rs } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_raw(&format!("ctx->hi = {rs_s};"));
        }
        MipsOp::Mflo { rd } => {
            gen.emit_gpr_write(*rd, "ctx->lo");
        }
        MipsOp::Mtlo { rs } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_raw(&format!("ctx->lo = {rs_s};"));
        }

        // -----------------------------------------------------------------
        // Multiply / divide
        // -----------------------------------------------------------------
        MipsOp::Mult { rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!(
                "{{ int64_t _prod = (int64_t)(int32_t)({rs_s}) * (int64_t)(int32_t)({rt_s}); \
                 ctx->hi = (uint32_t)(_prod >> 32); ctx->lo = (uint32_t)_prod; }}"
            ));
        }
        MipsOp::Multu { rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!(
                "{{ uint64_t _prod = (uint64_t)(uint32_t)({rs_s}) * \
                 (uint64_t)(uint32_t)({rt_s}); \
                 ctx->hi = (uint32_t)(_prod >> 32); ctx->lo = (uint32_t)_prod; }}"
            ));
        }
        MipsOp::Div { rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!(
                "if ((uint32_t)({rt_s}) != 0) {{ \
                 ctx->lo = (int32_t)({rs_s}) / (int32_t)({rt_s}); \
                 ctx->hi = (int32_t)({rs_s}) % (int32_t)({rt_s}); }}"
            ));
        }
        MipsOp::Divu { rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!(
                "if ((uint32_t)({rt_s}) != 0) {{ \
                 ctx->lo = (uint32_t)({rs_s}) / (uint32_t)({rt_s}); \
                 ctx->hi = (uint32_t)({rs_s}) % (uint32_t)({rt_s}); }}"
            ));
        }

        // -----------------------------------------------------------------
        // R-type ALU
        // -----------------------------------------------------------------
        MipsOp::Add { rd, rs, rt } | MipsOp::Addu { rd, rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("{rs_s} + {rt_s}"));
        }
        MipsOp::Sub { rd, rs, rt } | MipsOp::Subu { rd, rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("{rs_s} - {rt_s}"));
        }
        MipsOp::And { rd, rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("{rs_s} & {rt_s}"));
        }
        MipsOp::Or { rd, rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("{rs_s} | {rt_s}"));
        }
        MipsOp::Xor { rd, rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("{rs_s} ^ {rt_s}"));
        }
        MipsOp::Nor { rd, rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("~({rs_s} | {rt_s})"));
        }
        MipsOp::Slt { rd, rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(
                *rd,
                &format!("(int32_t)({rs_s}) < (int32_t)({rt_s}) ? 1 : 0"),
            );
        }
        MipsOp::Sltu { rd, rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(
                *rd,
                &format!("(uint32_t)({rs_s}) < (uint32_t)({rt_s}) ? 1 : 0"),
            );
        }
        MipsOp::Movz { rd, rs, rt } => {
            if *rd != Reg::Zero {
                let rs_s = gen.emit_gpr_read(*rs);
                let rt_s = gen.emit_gpr_read(*rt);
                let rd_s = gen.emit_gpr_read(*rd);
                gen.emit_raw(&format!("if ({rt_s} == 0) {rd_s} = {rs_s};"));
            }
        }
        MipsOp::Movn { rd, rs, rt } => {
            if *rd != Reg::Zero {
                let rs_s = gen.emit_gpr_read(*rs);
                let rt_s = gen.emit_gpr_read(*rt);
                let rd_s = gen.emit_gpr_read(*rd);
                gen.emit_raw(&format!("if ({rt_s} != 0) {rd_s} = {rs_s};"));
            }
        }
        MipsOp::Mul { rd, rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(
                *rd,
                &format!(
                    "(uint32_t)((int32_t)({rs_s}) * (int32_t)({rt_s}))"
                ),
            );
        }
        MipsOp::Daddu { rd, rs, rt } | MipsOp::Dadd { rd, rs, rt } => {
            // 64-bit add treated as 32-bit addu on Allegrex
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("{rs_s} + {rt_s}"));
        }
        MipsOp::Dsub { rd, rs, rt } => {
            // 64-bit sub treated as 32-bit sub on Allegrex
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("{rs_s} - {rt_s}"));
        }
        MipsOp::Dsrlv { rd, rt, rs } => {
            // 64-bit shift right logical variable treated as 32-bit srlv
            let rt_s = gen.emit_gpr_read(*rt);
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(
                *rd,
                &format!("(uint32_t)({rt_s}) >> ({rs_s} & 0x1F)"),
            );
        }

        // -----------------------------------------------------------------
        // Branches
        // -----------------------------------------------------------------
        MipsOp::J { target } => {
            if *target >= func_start && *target < func_end {
                // Intra-function jump: use C++ goto
                gen.emit_goto(&format!("L_{target:08X}"));
            } else {
                // Cross-function tail jump: emit as tail call via dispatch table
                gen.emit_call_lookup(*target);
                gen.emit_return();
            }
        }
        MipsOp::Jal { target } => {
            // Import-stub targets flow through the dispatch table like every
            // other cross-function call: the runtime's psp_hle_init() binds
            // HLE handlers at the generated-table stub addresses (issue #40),
            // so a direct named call would bypass the binding (and the
            // by-name symbols do not exist in the runtime). The resolved NID
            // name is kept as a comment for debuggability.
            if let Some(name) = imports.get(target) {
                gen.emit_call_hle(*target, name);
            } else {
                // Unknown target — use runtime dispatch table.
                // Direct-call resolution (func_map lookup) is wired in Phase 3+;
                // RECOMP_LOOKUP is correct for any address not in the import map.
                gen.emit_call_lookup(*target);
            }
        }
        MipsOp::Beq { rs, rt, target, likely } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            let cond = format!("{rs_s} == {rt_s}");
            emit_branch_or_tail(gen, &cond, *target, *likely, "/* beql */", func_start, func_end);
        }
        MipsOp::Bne { rs, rt, target, likely } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            let cond = format!("{rs_s} != {rt_s}");
            emit_branch_or_tail(gen, &cond, *target, *likely, "/* bnel */", func_start, func_end);
        }
        MipsOp::Blez { rs, target, likely } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let cond = format!("(int32_t)({rs_s}) <= 0");
            emit_branch_or_tail(gen, &cond, *target, *likely, "/* blezl */", func_start, func_end);
        }
        MipsOp::Bgtz { rs, target, likely } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let cond = format!("(int32_t)({rs_s}) > 0");
            emit_branch_or_tail(gen, &cond, *target, *likely, "/* bgtzl */", func_start, func_end);
        }
        MipsOp::Bltz { rs, target, likely } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let cond = format!("(int32_t)({rs_s}) < 0");
            emit_branch_or_tail(gen, &cond, *target, *likely, "/* bltzl */", func_start, func_end);
        }
        MipsOp::Bgez { rs, target, likely } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let cond = format!("(int32_t)({rs_s}) >= 0");
            emit_branch_or_tail(gen, &cond, *target, *likely, "/* bgezl */", func_start, func_end);
        }
        MipsOp::Bltzal { rs, target, likely } => {
            // link + branch if rs < 0 (link handled by stack, simplified here)
            let rs_s = gen.emit_gpr_read(*rs);
            let cond = format!("(int32_t)({rs_s}) < 0");
            emit_branch_or_tail(gen, &cond, *target, *likely, "/* bltzall */", func_start, func_end);
        }
        MipsOp::Bgezal { rs, target, likely } => {
            // link + branch if rs >= 0 (link handled by stack, simplified here)
            let rs_s = gen.emit_gpr_read(*rs);
            let cond = format!("(int32_t)({rs_s}) >= 0");
            emit_branch_or_tail(gen, &cond, *target, *likely, "/* bgezall */", func_start, func_end);
        }

        // -----------------------------------------------------------------
        // Immediate ALU
        // -----------------------------------------------------------------
        MipsOp::Addi { rt, rs, imm } | MipsOp::Addiu { rt, rs, imm } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rt, &format!("{rs_s} + {imm}"));
        }
        MipsOp::Slti { rt, rs, imm } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rt, &format!("(int32_t)({rs_s}) < {imm} ? 1 : 0"));
        }
        MipsOp::Sltiu { rt, rs, imm } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rt, &format!("(uint32_t)({rs_s}) < {imm}u ? 1 : 0"));
        }
        MipsOp::Andi { rt, rs, imm } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rt, &format!("{rs_s} & 0x{imm:04X}u"));
        }
        MipsOp::Ori { rt, rs, imm } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rt, &format!("{rs_s} | 0x{imm:04X}u"));
        }
        MipsOp::Xori { rt, rs, imm } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rt, &format!("{rs_s} ^ 0x{imm:04X}u"));
        }
        MipsOp::Lui { rt, imm } => {
            gen.emit_gpr_write(*rt, &format!("0x{:08X}u", (*imm as u32) << 16));
        }

        // -----------------------------------------------------------------
        // Memory loads
        // -----------------------------------------------------------------
        MipsOp::Lb { rt, rs, offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rt, &format!("MEM_B(rdram, {rs_s} + {offset})"));
        }
        MipsOp::Lh { rt, rs, offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rt, &format!("MEM_H(rdram, {rs_s} + {offset})"));
        }
        MipsOp::Lw { rt, rs, offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rt, &format!("MEM_W(rdram, {rs_s} + {offset})"));
        }
        MipsOp::Lbu { rt, rs, offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rt, &format!("MEM_BU(rdram, {rs_s} + {offset})"));
        }
        MipsOp::Lhu { rt, rs, offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(*rt, &format!("MEM_HU(rdram, {rs_s} + {offset})"));
        }
        MipsOp::Lwl { rt, rs, offset } => {
            if *rt == Reg::Zero {
                // LWL to $zero is a no-op
            } else {
                let rs_s = gen.emit_gpr_read(*rs);
                let rt_n = match rt {
                    Reg::Gpr(n) => n,
                    _ => unreachable!(),
                };
                gen.emit_raw(&format!(
                    "psp_lwl(rdram, ctx->r[{rt_n}], (uint32_t)({rs_s} + {offset}));"
                ));
            }
        }
        MipsOp::Lwr { rt, rs, offset } => {
            if *rt == Reg::Zero {
                // LWR to $zero is a no-op
            } else {
                let rs_s = gen.emit_gpr_read(*rs);
                let rt_n = match rt {
                    Reg::Gpr(n) => n,
                    _ => unreachable!(),
                };
                gen.emit_raw(&format!(
                    "psp_lwr(rdram, ctx->r[{rt_n}], (uint32_t)({rs_s} + {offset}));"
                ));
            }
        }

        // -----------------------------------------------------------------
        // Memory stores
        // -----------------------------------------------------------------
        MipsOp::Sb { rt, rs, offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!("MEM_B_WRITE(rdram, {rs_s} + {offset}, {rt_s});"));
        }
        MipsOp::Sh { rt, rs, offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!("MEM_H_WRITE(rdram, {rs_s} + {offset}, {rt_s});"));
        }
        MipsOp::Sw { rt, rs, offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!("MEM_W_WRITE(rdram, {rs_s} + {offset}, {rt_s});"));
        }
        MipsOp::Swl { rt, rs, offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!(
                "psp_swl(rdram, (uint32_t)({rt_s}), (uint32_t)({rs_s} + {offset}));"
            ));
        }
        MipsOp::Swr { rt, rs, offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!(
                "psp_swr(rdram, (uint32_t)({rt_s}), (uint32_t)({rs_s} + {offset}));"
            ));
        }

        // -----------------------------------------------------------------
        // Misc
        // -----------------------------------------------------------------
        MipsOp::Nop {} => {
            // No output — explicit no-op
        }
        MipsOp::Cache { .. } => {
            gen.emit_raw("/* CACHE ignored */;");
        }
        MipsOp::Sync {} => {
            gen.emit_raw("/* SYNC ignored */;");
        }

        // -----------------------------------------------------------------
        // Allegrex extensions
        // -----------------------------------------------------------------
        MipsOp::Ext { rt, rs, pos, size } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!(
                "{rt_s} = ({rs_s} >> {pos}) & ((1u << {size}) - 1);"
            ));
        }
        MipsOp::Ins { rt, rs, pos, size } => {
            let rt_s = gen.emit_gpr_read(*rt);
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_raw(&format!(
                "{rt_s} = ({rt_s} & ~(((1u << {size}) - 1) << {pos})) \
                 | (({rs_s} & ((1u << {size}) - 1)) << {pos});"
            ));
        }
        MipsOp::Seb { rd, rt } => {
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("(int32_t)(int8_t)({rt_s})"));
        }
        MipsOp::Seh { rd, rt } => {
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(*rd, &format!("(int32_t)(int16_t)({rt_s})"));
        }
        MipsOp::Clz { rd, rs } => {
            // EMIT-12: $zero destination is architecturally discarded; only
            // data decoded as code produces `clz $zero` (PRX binary_scan
            // regions, issue #52) and `0 = ...` would not compile.
            if *rd != Reg::Zero {
                let rs_s = gen.emit_gpr_read(*rs);
                let rd_s = gen.emit_gpr_read(*rd);
                gen.emit_raw(&format!(
                    "{rd_s} = ({rs_s} == 0) ? 32 : __builtin_clz((uint32_t)({rs_s}));"
                ));
            }
        }
        MipsOp::Clo { rd, rs } => {
            // EMIT-12: see Clz above.
            if *rd != Reg::Zero {
                let rs_s = gen.emit_gpr_read(*rs);
                let rd_s = gen.emit_gpr_read(*rd);
                gen.emit_raw(&format!(
                    "{rd_s} = (~({rs_s}) == 0) ? 32 : __builtin_clz((uint32_t)(~({rs_s})));"
                ));
            }
        }
        MipsOp::Min { rd, rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(
                *rd,
                &format!("((int32_t)({rs_s}) < (int32_t)({rt_s})) ? {rs_s} : {rt_s}"),
            );
        }
        MipsOp::Max { rd, rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_gpr_write(
                *rd,
                &format!("((int32_t)({rs_s}) > (int32_t)({rt_s})) ? {rs_s} : {rt_s}"),
            );
        }
        MipsOp::Bitrev { rd, rt } => {
            let rt_s = gen.emit_gpr_read(*rt);
            let rd_s = gen.emit_gpr_read(*rd);
            gen.emit_raw(&format!(
                "{{ uint32_t _v = (uint32_t)({rt_s}); \
                 _v = ((_v & 0xAAAAAAAAu) >> 1) | ((_v & 0x55555555u) << 1); \
                 _v = ((_v & 0xCCCCCCCCu) >> 2) | ((_v & 0x33333333u) << 2); \
                 _v = ((_v & 0xF0F0F0F0u) >> 4) | ((_v & 0x0F0F0F0Fu) << 4); \
                 _v = ((_v & 0xFF00FF00u) >> 8) | ((_v & 0x00FF00FFu) << 8); \
                 {rd_s} = (int32_t)((_v >> 16) | (_v << 16)); }}"
            ));
        }
        MipsOp::Wsbh { rd, rt } => {
            let rt_s = gen.emit_gpr_read(*rt);
            let rd_s = gen.emit_gpr_read(*rd);
            gen.emit_raw(&format!(
                "{{ uint32_t _v = (uint32_t)({rt_s}); \
                 {rd_s} = (int32_t)(((_v & 0xFF00FF00u) >> 8) | ((_v & 0x00FF00FFu) << 8)); }}"
            ));
        }
        MipsOp::Wsbw { rd, rt } => {
            let rt_s = gen.emit_gpr_read(*rt);
            let rd_s = gen.emit_gpr_read(*rd);
            gen.emit_raw(&format!(
                "{rd_s} = (int32_t)__builtin_bswap32((uint32_t)({rt_s}));"
            ));
        }
        MipsOp::Madd { rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!(
                "{{ int64_t _acc = ((int64_t)ctx->hi << 32) | (uint64_t)ctx->lo; \
                 _acc += (int64_t)(int32_t)({rs_s}) * (int64_t)(int32_t)({rt_s}); \
                 ctx->hi = (uint32_t)(_acc >> 32); ctx->lo = (uint32_t)_acc; }}"
            ));
        }
        MipsOp::Msub { rs, rt } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!(
                "{{ int64_t _acc = ((int64_t)ctx->hi << 32) | (uint64_t)ctx->lo; \
                 _acc -= (int64_t)(int32_t)({rs_s}) * (int64_t)(int32_t)({rt_s}); \
                 ctx->hi = (uint32_t)(_acc >> 32); ctx->lo = (uint32_t)_acc; }}"
            ));
        }
        MipsOp::Mfc0 { .. } => {
            gen.emit_raw("/* MFC0 ignored */;");
        }
        MipsOp::Mtc0 { .. } => {
            gen.emit_raw("/* MTC0 ignored */;");
        }

        // -----------------------------------------------------------------
        // FPU — COP1 single-precision
        // -----------------------------------------------------------------
        MipsOp::AddS { fd, fs, ft } => {
            let fs_s = gen.emit_fpr_read(*fs);
            let ft_s = gen.emit_fpr_read(*ft);
            gen.emit_fpr_write(*fd, &format!("{fs_s} + {ft_s}"));
        }
        MipsOp::SubS { fd, fs, ft } => {
            let fs_s = gen.emit_fpr_read(*fs);
            let ft_s = gen.emit_fpr_read(*ft);
            gen.emit_fpr_write(*fd, &format!("{fs_s} - {ft_s}"));
        }
        MipsOp::MulS { fd, fs, ft } => {
            let fs_s = gen.emit_fpr_read(*fs);
            let ft_s = gen.emit_fpr_read(*ft);
            gen.emit_fpr_write(*fd, &format!("{fs_s} * {ft_s}"));
        }
        MipsOp::DivS { fd, fs, ft } => {
            let fs_s = gen.emit_fpr_read(*fs);
            let ft_s = gen.emit_fpr_read(*ft);
            gen.emit_fpr_write(*fd, &format!("{fs_s} / {ft_s}"));
        }
        MipsOp::SqrtS { fd, fs } => {
            let fs_s = gen.emit_fpr_read(*fs);
            gen.emit_fpr_write(*fd, &format!("sqrtf({fs_s})"));
        }
        MipsOp::AbsS { fd, fs } => {
            let fs_s = gen.emit_fpr_read(*fs);
            gen.emit_fpr_write(*fd, &format!("fabsf({fs_s})"));
        }
        MipsOp::MovS { fd, fs } => {
            let fs_s = gen.emit_fpr_read(*fs);
            gen.emit_fpr_write(*fd, &fs_s);
        }
        MipsOp::NegS { fd, fs } => {
            let fs_s = gen.emit_fpr_read(*fs);
            gen.emit_fpr_write(*fd, &format!("-({fs_s})"));
        }
        MipsOp::CvtSW { fd, fs } => {
            // Convert int to float: fd = (float)(int32_t)fs_int
            let fs_n = fs.0;
            gen.emit_fpr_write(*fd, &format!("(float)(int32_t)ctx->fi[{fs_n}]"));
        }
        MipsOp::CvtWS { fd, fs } => {
            // Convert float to int (truncate): fd_int = (int32_t)fs
            let fd_n = fd.0;
            let fs_s = gen.emit_fpr_read(*fs);
            gen.emit_raw(&format!("ctx->fi[{fd_n}] = (int32_t)({fs_s});"));
        }
        MipsOp::TruncWS { fd, fs } => {
            // Truncate float to int (always toward zero)
            let fd_n = fd.0;
            let fs_s = gen.emit_fpr_read(*fs);
            gen.emit_raw(&format!(
                "ctx->fi[{fd_n}] = (int32_t)truncf({fs_s});"
            ));
        }
        MipsOp::CCond { cond, fs, ft } => {
            let fs_s = gen.emit_fpr_read(*fs);
            let ft_s = gen.emit_fpr_read(*ft);
            // FPU compare conditions (lower 4 bits of cond encode condition)
            let cond_expr = match cond & 0xF {
                0 => format!("false"),                       // f (false)
                1 => format!("false"),                       // un (unordered — simplified)
                2 => format!("{fs_s} == {ft_s}"),            // eq
                3 => format!("!({fs_s} == {ft_s})"),         // ueq (simplified)
                4 => format!("{fs_s} < {ft_s}"),             // olt
                5 => format!("!({fs_s} >= {ft_s})"),         // ult (simplified)
                6 => format!("{fs_s} <= {ft_s}"),            // ole
                7 => format!("!({fs_s} > {ft_s})"),          // ule (simplified)
                8 => format!("false"),                       // sf (false)
                9 => format!("false"),                       // ngle (simplified)
                10 => format!("{fs_s} == {ft_s}"),           // seq
                11 => format!("{fs_s} == {ft_s}"),           // ngl (simplified)
                12 => format!("{fs_s} < {ft_s}"),            // lt
                13 => format!("{fs_s} < {ft_s}"),            // nge (simplified)
                14 => format!("{fs_s} <= {ft_s}"),           // le
                _ => format!("{fs_s} <= {ft_s}"),            // ngt (simplified)
            };
            gen.emit_fpu_cc_write(&cond_expr);
        }
        MipsOp::Bc1t { target, likely } => {
            let cc = gen.emit_fpu_cc_read();
            emit_branch_or_tail(gen, &cc, *target, *likely, "/* bc1tl */", func_start, func_end);
        }
        MipsOp::Bc1f { target, likely } => {
            let cc = gen.emit_fpu_cc_read();
            let cond = format!("!({cc})");
            emit_branch_or_tail(gen, &cond, *target, *likely, "/* bc1fl */", func_start, func_end);
        }
        MipsOp::Mfc1 { rt, fs } => {
            // Move from FPU (int view): rt = fi[n]
            let fs_n = fs.0;
            gen.emit_gpr_write(*rt, &format!("(int32_t)ctx->fi[{fs_n}]"));
        }
        MipsOp::Mtc1 { rt, fs } => {
            // Move to FPU (int view): fi[n] = rt
            let fs_n = fs.0;
            let rt_s = gen.emit_gpr_read(*rt);
            gen.emit_raw(&format!("ctx->fi[{fs_n}] = (uint32_t)({rt_s});"));
        }
        MipsOp::Lwc1 { ft, rs, offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let ft_n = ft.0;
            gen.emit_raw(&format!(
                "ctx->fi[{ft_n}] = (uint32_t)MEM_WU(rdram, {rs_s} + {offset});"
            ));
        }
        MipsOp::Swc1 { ft, rs, offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            let ft_n = ft.0;
            gen.emit_raw(&format!(
                "MEM_W_WRITE(rdram, {rs_s} + {offset}, (int32_t)ctx->fi[{ft_n}]);"
            ));
        }

        // -----------------------------------------------------------------
        // VFPU -- dispatch to vfpu module for per-variant emission
        // -----------------------------------------------------------------
        MipsOp::VfpuMfv { .. }
        | MipsOp::VfpuMtv { .. }
        | MipsOp::VfpuBvf { .. }
        | MipsOp::VfpuBvt { .. }
        | MipsOp::VfpuMfvc { .. }
        | MipsOp::VfpuMtvc { .. }
        | MipsOp::VfpuPrefix { .. }
        | MipsOp::VfpuViim { .. }
        | MipsOp::VfpuVfim { .. }
        | MipsOp::VfpuAdd { .. }
        | MipsOp::VfpuSub { .. }
        | MipsOp::VfpuDiv { .. }
        | MipsOp::VfpuMul { .. }
        | MipsOp::VfpuDot { .. }
        | MipsOp::VfpuScl { .. }
        | MipsOp::VfpuHdp { .. }
        | MipsOp::VfpuCrs { .. }
        | MipsOp::VfpuDet { .. }
        | MipsOp::VfpuCmp { .. }
        | MipsOp::VfpuVmin { .. }
        | MipsOp::VfpuVmax { .. }
        | MipsOp::VfpuScmp { .. }
        | MipsOp::VfpuSge { .. }
        | MipsOp::VfpuSlt { .. }
        | MipsOp::VfpuUnary { .. }
        | MipsOp::VfpuMmul { .. }
        | MipsOp::VfpuMscl { .. }
        | MipsOp::VfpuTfm { .. }
        | MipsOp::VfpuHtfm { .. }
        | MipsOp::VfpuCrsp { .. }
        | MipsOp::VfpuMatUnary { .. }
        | MipsOp::VfpuLvS { .. }
        | MipsOp::VfpuSvS { .. }
        | MipsOp::VfpuLvQ { .. }
        | MipsOp::VfpuSvQ { .. }
        | MipsOp::VfpuLvlQ { .. }
        | MipsOp::VfpuLvrQ { .. }
        | MipsOp::VfpuSvlQ { .. }
        | MipsOp::VfpuSvrQ { .. }
        | MipsOp::VfpuFlush { .. }
        | MipsOp::VfpuUnknown { .. } => {
            crate::vfpu::emit_vfpu_op(op, gen, func_start, func_end);
        }

        // -----------------------------------------------------------------
        // Jump table (DECODE-06) — emit as C++ switch
        // -----------------------------------------------------------------
        MipsOp::JumpTable { index_reg, cases } => {
            let index_s = gen.emit_gpr_read(*index_reg);
            let case_pairs: Vec<(u32, String)> = cases
                .iter()
                .map(|&addr| (addr, format!("L_{addr:08X}")))
                .collect();
            gen.emit_switch(&index_s, &case_pairs);
        }

        // -----------------------------------------------------------------
        // Delay slot — should be unwrapped by decoder. Emit inner op.
        // -----------------------------------------------------------------
        MipsOp::DelaySlot { instr } => {
            emit_op(instr, gen, imports, func_start, func_end);
        }

        // -----------------------------------------------------------------
        // Hazard-fused branch + delay slot (issue #12): the delay slot writes
        // a register the branch reads — snapshot first, then run the delay.
        // -----------------------------------------------------------------
        MipsOp::BranchHazardDelay { branch, delay } => {
            emit_branch_hazard_delay(branch, delay, gen, imports, func_start, func_end);
        }

        // -----------------------------------------------------------------
        // BIDS duplicate (issue #56) — normally consumed by the pairing in
        // `emit_function` (which emits the skip-goto + label + duplicate).
        // Standalone fallback: the duplicate at its own label IS the
        // direct-entry (hardware) semantic of the delay-slot word.
        // -----------------------------------------------------------------
        MipsOp::DelaySlotRejoin { instr } => {
            emit_op(instr, gen, imports, func_start, func_end);
        }

        // -----------------------------------------------------------------
        // Relocation site markers (EMIT-10)
        // -----------------------------------------------------------------
        MipsOp::RelocHi16 { rt, section_idx, symbol_offset } => {
            gen.emit_gpr_write(
                *rt,
                &format!("RELOC_HI16(section_{section_idx}_base, {symbol_offset}u)"),
            );
        }
        MipsOp::RelocLo16 { rt, rs, section_idx, symbol_offset } => {
            let rs_s = gen.emit_gpr_read(*rs);
            gen.emit_gpr_write(
                *rt,
                &format!(
                    "{rs_s} + (int16_t)RELOC_LO16(section_{section_idx}_base, {symbol_offset}u)"
                ),
            );
        }
        MipsOp::RelocJ26 { target_section_idx, symbol_offset } => {
            // J-type relocation: call via lookup using relocated address
            gen.emit_raw(&format!(
                "/* RELOC_J26: section_{target_section_idx}_base + {symbol_offset} */ \
                 RECOMP_LOOKUP(RELOC_J26(section_{target_section_idx}_base, {symbol_offset}u) << 2)(rdram, ctx);"
            ));
        }
    }
}

/// Emit a function preamble comment for debugging.
///
/// Writes a C++ block comment identifying the function vaddr and original name.
pub fn emit_function_comment(func: &DecodedFunction, gen: &mut dyn Generator) {
    gen.emit_raw(&format!(
        "/* Function: {} @ 0x{:08X} (size: {} bytes) */",
        func.name, func.vaddr, func.size
    ));
}

#[cfg(test)]
mod tests {
    use super::*;
    use psp_ir::{BasicBlock, DecodedFunction, MipsOp, Reg};
    use crate::generator::TestGenerator;

    fn make_func(ops: Vec<MipsOp>) -> DecodedFunction {
        DecodedFunction {
            vaddr: 0x08804000,
            name: "test_func".to_string(),
            cpp_name: "test_func".to_string(),
            size: 8,
            blocks: vec![BasicBlock { vaddr: 0x08804000, instrs: ops }],
            is_mid_entry_parent: false,
            mid_entry_addrs: vec![],
            coalesced: false,
        }
    }

    #[test]
    fn nop_produces_no_output() {
        let mut gen = TestGenerator::new();
        let func = make_func(vec![MipsOp::Nop {}]);
        emit_function(&func, &mut gen, &ImportMap::new());
        // Only FUNC_START, LABEL, FUNC_END — no NOP output
        assert!(!gen.output.iter().any(|s| s.contains("nop")));
    }

    #[test]
    fn add_emits_plus() {
        let mut gen = TestGenerator::new();
        let func = make_func(vec![MipsOp::Addu {
            rd: Reg::Gpr(2),
            rs: Reg::Gpr(4),
            rt: Reg::Gpr(5),
        }]);
        emit_function(&func, &mut gen, &ImportMap::new());
        let joined = gen.output.join("\n");
        assert!(joined.contains('+'), "addu should emit '+' expression");
    }

    // --- Instruction-budget preemption points (#66, design approach (a)) -----

    /// A backward conditional branch (target <= its own address) is a loop
    /// back-edge: the emitter must inject the budget-decrement preemption point.
    #[test]
    fn backward_branch_emits_preempt_point() {
        let mut gen = TestGenerator::new();
        // size=8: op@vaddr (nop), op@vaddr+4 (beq back to vaddr) — a 2-instr loop.
        let mut func = make_func(vec![
            MipsOp::Nop {},
            MipsOp::Beq {
                rs: Reg::Gpr(4),
                rt: Reg::Gpr(5),
                target: 0x08804000, // back-edge to func start (<= 0x08804004)
                likely: false,
            },
        ]);
        func.size = 8;
        emit_function(&func, &mut gen, &ImportMap::new());
        let joined = gen.output.join("\n");
        assert!(
            joined.contains("--ctx->preempt_budget") && joined.contains("sched_preempt(ctx)"),
            "backward branch must emit a preemption point; got:\n{joined}"
        );
    }

    /// A degenerate self-loop (`b .`, target == own address) is the busy-poll
    /// spin shape (.hack CRI ring); it must still get a preemption point.
    #[test]
    fn self_loop_emits_preempt_point() {
        let mut gen = TestGenerator::new();
        let func = make_func(vec![MipsOp::Beq {
            rs: Reg::Zero,
            rt: Reg::Zero,
            target: 0x08804000, // == this op's own address
            likely: false,
        }]);
        emit_function(&func, &mut gen, &ImportMap::new());
        let joined = gen.output.join("\n");
        assert!(
            joined.contains("--ctx->preempt_budget"),
            "self-loop spin must emit a preemption point; got:\n{joined}"
        );
    }

    /// A forward-only function (no branch whose target precedes it) must NOT
    /// gain any preemption point — straight-line code stays cost-free.
    #[test]
    fn forward_only_emits_no_preempt_point() {
        let mut gen = TestGenerator::new();
        // forward beq (target after the branch) + an arithmetic op.
        let mut func = make_func(vec![
            MipsOp::Beq {
                rs: Reg::Gpr(4),
                rt: Reg::Gpr(5),
                target: 0x08804008, // forward (> 0x08804000)
                likely: false,
            },
            MipsOp::Addu { rd: Reg::Gpr(2), rs: Reg::Gpr(4), rt: Reg::Gpr(5) },
        ]);
        func.size = 8;
        emit_function(&func, &mut gen, &ImportMap::new());
        let joined = gen.output.join("\n");
        assert!(
            !joined.contains("preempt_budget"),
            "forward-only function must emit no preemption point; got:\n{joined}"
        );
    }

    /// Unit-level coverage of the back-edge predicate: backward/self in range
    /// hit; forward, out-of-range, and calls/links/indirect jumps do not.
    #[test]
    fn backward_branch_target_predicate() {
        let start = 0x08804000;
        let at = 0x08804010; // the branch's own positional address
        // Backward in-range -> Some.
        assert_eq!(
            backward_branch_target(
                &MipsOp::Beq { rs: Reg::Gpr(1), rt: Reg::Gpr(2), target: start, likely: false },
                at,
                start,
            ),
            Some(start)
        );
        // Self-loop (target == at) -> Some.
        assert_eq!(
            backward_branch_target(&MipsOp::J { target: at }, at, start),
            Some(at)
        );
        // Forward (target > at) -> None.
        assert_eq!(
            backward_branch_target(&MipsOp::J { target: 0x08804020 }, at, start),
            None
        );
        // Backward but BELOW func start (out of function) -> None.
        assert_eq!(
            backward_branch_target(&MipsOp::J { target: 0x08800000 }, at, start),
            None
        );
        // jal/bltzal/bgezal are calls/links, not loop back-edges -> None.
        assert_eq!(backward_branch_target(&MipsOp::Jal { target: start }, at, start), None);
        assert_eq!(
            backward_branch_target(
                &MipsOp::Bltzal { rs: Reg::Gpr(4), target: start, likely: false },
                at,
                start,
            ),
            None
        );
        // Register-indirect jump has no static target -> None.
        assert_eq!(
            backward_branch_target(&MipsOp::Jr { rs: Reg::Gpr(31) }, at, start),
            None
        );
        // Hazard-fused backward branch unwraps to its inner branch -> Some.
        assert_eq!(
            backward_branch_target(
                &MipsOp::BranchHazardDelay {
                    branch: Box::new(MipsOp::Bne {
                        rs: Reg::Gpr(1),
                        rt: Reg::Gpr(2),
                        target: start,
                        likely: false,
                    }),
                    delay: Box::new(MipsOp::Nop {}),
                },
                at,
                start,
            ),
            Some(start)
        );
    }

    #[test]
    fn jr_ra_emits_return() {
        let mut gen = TestGenerator::new();
        let func = make_func(vec![MipsOp::Jr { rs: Reg::Gpr(31) }]);
        emit_function(&func, &mut gen, &ImportMap::new());
        assert!(gen.output.contains(&"return;".to_string()));
    }

    #[test]
    fn zero_write_in_addu_suppressed() {
        let mut gen = TestGenerator::new();
        // addu $zero, $a0, $a1 — rd is $zero, write should be suppressed
        let func = make_func(vec![MipsOp::Addu {
            rd: Reg::Zero,
            rs: Reg::Gpr(4),
            rt: Reg::Gpr(5),
        }]);
        emit_function(&func, &mut gen, &ImportMap::new());
        let joined = gen.output.join("\n");
        // Should not contain an assignment (suppressed)
        assert!(!joined.contains("= ctx->r[4] + ctx->r[5]"));
    }

    // --- Option-A LINK/RA model -------------------------------------------

    #[test]
    fn jr_ra_is_exit_after_teardown() {
        // ... addiu sp,sp,+16 ; jr ra  => EXIT (reordered teardown before jr)
        let seq = vec![
            MipsOp::Addiu { rt: Reg::Gpr(29), rs: Reg::Gpr(29), imm: 16 },
            MipsOp::Jr { rs: Reg::Gpr(31) },
        ];
        assert!(jr_ra_is_exit(&seq, 1));
    }

    #[test]
    fn jr_ra_is_exit_after_ra_reload() {
        // lw ra,0x10(sp) ; lw s4,0xc(sp) ; jr ra => EXIT (ra reload before jr)
        let seq = vec![
            MipsOp::Lw { rt: Reg::Gpr(31), rs: Reg::Gpr(29), offset: 0x10 },
            MipsOp::Lw { rt: Reg::Gpr(20), rs: Reg::Gpr(29), offset: 0xc },
            MipsOp::Jr { rs: Reg::Gpr(31) },
        ];
        assert!(jr_ra_is_exit(&seq, 2));
    }

    #[test]
    fn jr_ra_internal_when_no_epilogue_marker() {
        // r-type ; jr ra  (subroutine return, no teardown/reload) => INTERNAL
        let seq = vec![
            MipsOp::Addu { rd: Reg::Gpr(2), rs: Reg::Gpr(4), rt: Reg::Gpr(5) },
            MipsOp::Jr { rs: Reg::Gpr(31) },
        ];
        assert!(!jr_ra_is_exit(&seq, 1));
    }

    #[test]
    fn jr_ra_internal_stops_at_prior_boundary() {
        // A prior function's `lw ra` must NOT leak across a control-transfer:
        // lw ra ; jr ra ; r-type ; jr ra  — the SECOND jr ra (a leaf) is INTERNAL.
        let seq = vec![
            MipsOp::Lw { rt: Reg::Gpr(31), rs: Reg::Gpr(29), offset: 0xc },
            MipsOp::Jr { rs: Reg::Gpr(31) }, // EXIT of prior fn
            MipsOp::Addu { rd: Reg::Gpr(2), rs: Reg::Gpr(4), rt: Reg::Gpr(5) },
            MipsOp::Jr { rs: Reg::Gpr(31) }, // leaf => INTERNAL
        ];
        assert!(jr_ra_is_exit(&seq, 1));
        assert!(!jr_ra_is_exit(&seq, 3));
    }

    #[test]
    fn build_ra_ctx_collects_internal_links_positionally() {
        // jal internal@+8 (op idx 0) ; nop ; bltzal internal (idx 2)
        let start = 0x08804000;
        let seq = vec![
            MipsOp::Jal { target: 0x08804010 }, // internal -> link-return = +4
            MipsOp::Nop {},
            MipsOp::Bltzal { rs: Reg::Gpr(4), target: 0x08804014, likely: false },
            MipsOp::Jal { target: 0x09000000 }, // EXTERNAL -> not a link
        ];
        let rc = build_ra_ctx(&seq, start, start + 0x40);
        // jal at idx 0 -> 0x08804004 ; bltzal at idx 2 -> 0x0880400C
        assert_eq!(rc.link_returns, vec![0x08804004, 0x0880400C]);
    }

    #[test]
    fn coalesced_self_contained_emits_ra_model() {
        // A self-contained coalesced owner with >=2 internal links is RA-modeled:
        // internal jal -> link write + goto; epilogue jr ra -> restore + return.
        let ops = vec![
            MipsOp::Jal { target: 0x08804010 },          // 0x4000 internal
            MipsOp::Jal { target: 0x08804014 },          // 0x4004 internal
            MipsOp::Lw { rt: Reg::Gpr(31), rs: Reg::Gpr(29), offset: 0x10 }, // 0x4008
            MipsOp::Addiu { rt: Reg::Gpr(29), rs: Reg::Gpr(29), imm: 16 },   // 0x400C
            MipsOp::Jr { rs: Reg::Gpr(31) },             // 0x4010 EXIT
            MipsOp::Jr { rs: Reg::Gpr(31) },             // 0x4014 INTERNAL
        ];
        let mut func = make_func(ops);
        func.size = 0x18;
        func.coalesced = true;
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());
        let out = gen.output.join("\n");
        assert!(out.contains("uint32_t _ra_saved;"), "saves caller link");
        assert!(out.contains("_ra_active = true;"), "marks active on internal jal");
        assert!(out.contains("ctx->r[31] = (int32_t)0x08804004"), "internal link write");
        assert!(out.contains("switch ((uint32_t)ctx->r[31])"), "internal jr-ra switch");
        assert!(out.contains("ctx->r[31] = _ra_saved;"), "EXIT restores caller link");
    }

    #[test]
    fn coalesced_single_link_not_ra_modeled() {
        // Only one internal link -> NOT RA-modeled (keeps baseline lowering).
        let ops = vec![
            MipsOp::Jal { target: 0x08804008 }, // single internal link
            MipsOp::Nop {},
            MipsOp::Jr { rs: Reg::Gpr(31) },
        ];
        let mut func = make_func(ops);
        func.size = 0xc;
        func.coalesced = true;
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());
        let out = gen.output.join("\n");
        assert!(!out.contains("_ra_active"), "single-link owner is not RA-modeled");
    }

    // --- Terminal-`jal` epilogue reconstruction --------------------------

    /// Build a function mirroring the `FUN_0881c7e4` shape: a real prologue
    /// (frame + callee-saved spills) and a body that ends in an EXTERNAL `jal`
    /// with NO `jr ra` of its own.
    fn terminal_jal_func() -> DecodedFunction {
        let ops = vec![
            MipsOp::Addiu { rt: Reg::Gpr(29), rs: Reg::Gpr(29), imm: -0x30 }, // sp -= 0x30
            MipsOp::Sw { rt: Reg::Gpr(16), rs: Reg::Gpr(29), offset: 0 },     // s0
            MipsOp::Sw { rt: Reg::Gpr(23), rs: Reg::Gpr(29), offset: 0x1c },  // s7
            MipsOp::Sw { rt: Reg::Gpr(30), rs: Reg::Gpr(29), offset: 0x20 },  // fp
            MipsOp::Sw { rt: Reg::Gpr(31), rs: Reg::Gpr(29), offset: 0x24 },  // ra
            MipsOp::Jal { target: 0x08813000 },                              // EXTERNAL tail jal
        ];
        let len = ops.len() as u32;
        let mut f = make_func(ops);
        f.size = len * 4;
        f
    }

    #[test]
    fn terminal_external_jal_reconstructs_epilogue() {
        let mut gen = TestGenerator::new();
        emit_function(&terminal_jal_func(), &mut gen, &ImportMap::new());
        let out = gen.output.join("\n");
        // Restores for every spilled callee-saved reg + frame pop are emitted.
        assert!(out.contains("ctx->r[16] = (int32_t)(MEM_W(rdram, ctx->r[29] + 0))"));
        assert!(out.contains("ctx->r[23] = (int32_t)(MEM_W(rdram, ctx->r[29] + 28))"));
        assert!(out.contains("ctx->r[30] = (int32_t)(MEM_W(rdram, ctx->r[29] + 32))"));
        assert!(out.contains("ctx->r[31] = (int32_t)(MEM_W(rdram, ctx->r[29] + 36))"));
        assert!(out.contains("ctx->r[29] = (int32_t)(ctx->r[29] + 48)"));
    }

    #[test]
    fn function_with_jr_ra_not_reconstructed() {
        // A normal function ending in `jr ra` must NOT get a reconstructed
        // epilogue (it already restores its own frame).
        let ops = vec![
            MipsOp::Addiu { rt: Reg::Gpr(29), rs: Reg::Gpr(29), imm: -16 },
            MipsOp::Sw { rt: Reg::Gpr(31), rs: Reg::Gpr(29), offset: 12 },
            MipsOp::Lw { rt: Reg::Gpr(31), rs: Reg::Gpr(29), offset: 12 },
            MipsOp::Addiu { rt: Reg::Gpr(29), rs: Reg::Gpr(29), imm: 16 },
            MipsOp::Jr { rs: Reg::Gpr(31) },
        ];
        let mut func = make_func(ops);
        func.size = 0x14;
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());
        let out = gen.output.join("\n");
        assert!(!out.contains("reconstructed epilogue"), "jr-ra func needs no reconstruction");
    }

    // --- Hazard-fused branch + delay slot (issue #12) ---------------------

    #[test]
    fn hazard_beq_snapshots_before_delay() {
        // beq $t0,$at,L / delay addiu $t0,$v0,3 — the compare must use the
        // PRE-delay $t0 snapshot, and the delay must execute before the branch.
        let ops = vec![
            MipsOp::BranchHazardDelay {
                branch: Box::new(MipsOp::Beq {
                    rs: Reg::Gpr(8),
                    rt: Reg::Gpr(1),
                    target: 0x0880400C,
                    likely: false,
                }),
                delay: Box::new(MipsOp::Addiu { rt: Reg::Gpr(8), rs: Reg::Gpr(2), imm: 3 }),
            },
            MipsOp::Nop {},
            MipsOp::Nop {},
            MipsOp::Jr { rs: Reg::Gpr(31) },
        ];
        let mut func = make_func(ops);
        func.size = 0x10;
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());

        let snap0 = gen.output.iter().position(|s| s.contains("_bh0 = (uint32_t)(ctx->r[8])"))
            .expect("missing rs snapshot");
        let snap1 = gen.output.iter().position(|s| s.contains("_bh1 = (uint32_t)(ctx->r[1])"))
            .expect("missing rt snapshot");
        let delay = gen.output.iter().position(|s| s.contains("ctx->r[8] = ctx->r[2] + 3"))
            .expect("missing delay-slot addiu");
        let branch = gen.output.iter().position(|s| s.contains("if (_bh0 == _bh1) goto L_0880400C"))
            .expect("branch must compare the snapshots");
        assert!(snap0 < delay && snap1 < delay, "snapshots must precede the delay slot");
        assert!(delay < branch, "delay slot must precede the branch");
    }

    #[test]
    fn hazard_bnez_nor_tests_pre_delay_value() {
        // bnez $t1,L / delay nor $t1,$t0,$zero — condition uses the pre-nor $t1.
        let ops = vec![
            MipsOp::BranchHazardDelay {
                branch: Box::new(MipsOp::Bne {
                    rs: Reg::Gpr(9),
                    rt: Reg::Zero,
                    target: 0x0880400C,
                    likely: false,
                }),
                delay: Box::new(MipsOp::Nor { rd: Reg::Gpr(9), rs: Reg::Gpr(8), rt: Reg::Zero }),
            },
            MipsOp::Nop {},
            MipsOp::Nop {},
            MipsOp::Jr { rs: Reg::Gpr(31) },
        ];
        let mut func = make_func(ops);
        func.size = 0x10;
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());
        let out = gen.output.join("\n");
        assert!(out.contains("const uint32_t _bh0 = (uint32_t)(ctx->r[9]);"));
        assert!(out.contains("ctx->r[9] = ~(ctx->r[8] | 0);"), "nor delay emitted: {out}");
        assert!(out.contains("if (_bh0 != _bh1) goto L_0880400C;"),
            "condition must test the snapshot, not the clobbered reg: {out}");
    }

    #[test]
    fn hazard_jr_dispatches_on_pre_delay_target() {
        // jr $t3 / delay lw $t3,0($sp) — dispatch must use the snapshot.
        let ops = vec![
            MipsOp::BranchHazardDelay {
                branch: Box::new(MipsOp::Jr { rs: Reg::Gpr(11) }),
                delay: Box::new(MipsOp::Lw { rt: Reg::Gpr(11), rs: Reg::Gpr(29), offset: 0 }),
            },
            MipsOp::Nop {},
        ];
        let mut func = make_func(ops);
        func.size = 0x8;
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());

        let snap = gen.output.iter().position(|s| s.contains("_bh0 = (uint32_t)(ctx->r[11])"))
            .expect("missing target snapshot");
        let delay = gen.output.iter().position(|s| s.contains("ctx->r[11] = MEM_W(rdram, ctx->r[29] + 0)"))
            .expect("missing delay-slot lw");
        let dispatch = gen.output.iter().position(|s| s == "CALL_LOOKUP_REG:_bh0")
            .expect("jr must dispatch on the snapshot");
        assert!(snap < delay && delay < dispatch, "snapshot -> delay -> dispatch order");
    }

    // --- BIDS rejoin pairing (issue #56) -----------------------------------

    /// The memset-tail shape: BranchHazardDelay{beq a2,zero / addiu a2,-1} at
    /// 0x08804000 whose delay slot 0x08804004 is a branch target (the bne at
    /// 0x0880400C loops back into it).
    fn bids_memset_tail_ops() -> Vec<MipsOp> {
        vec![
            MipsOp::BranchHazardDelay {
                branch: Box::new(MipsOp::Beq {
                    rs: Reg::Gpr(6),
                    rt: Reg::Zero,
                    target: 0x08804014,
                    likely: false,
                }),
                delay: Box::new(MipsOp::Addiu { rt: Reg::Gpr(6), rs: Reg::Gpr(6), imm: -1 }),
            },
            MipsOp::DelaySlotRejoin {
                instr: Box::new(MipsOp::Addiu { rt: Reg::Gpr(6), rs: Reg::Gpr(6), imm: -1 }),
            },
            MipsOp::Sb { rt: Reg::Gpr(5), rs: Reg::Gpr(4), offset: 0 },
            MipsOp::Addiu { rt: Reg::Gpr(4), rs: Reg::Gpr(4), imm: 1 },
            MipsOp::Bne {
                rs: Reg::Gpr(6),
                rt: Reg::Zero,
                target: 0x08804004,
                likely: false,
            },
            MipsOp::Jr { rs: Reg::Gpr(31) },
        ]
    }

    #[test]
    fn bids_rejoin_emits_skip_and_duplicate_at_slot_label() {
        let mut func = make_func(bids_memset_tail_ops());
        func.size = 0x18;
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());

        // Order: hazard delay (in-block) -> skip goto L_..08 -> label L_..04
        // -> duplicate delay instr.
        let block_delay = gen
            .output
            .iter()
            .position(|s| s.contains("ctx->r[6] = ctx->r[6] + -1"))
            .expect("hazard block must run the delay once");
        let skip = gen
            .output
            .iter()
            .position(|s| s == "goto L_08804008;")
            .expect("fall-through must skip the duplicate: {gen.output:?}");
        let slot_label = gen
            .output
            .iter()
            .position(|s| s == "LABEL:L_08804004")
            .expect("delay-slot address keeps its label");
        let dup = gen.output[slot_label + 1..]
            .iter()
            .position(|s| s.contains("ctx->r[6] = ctx->r[6] + -1"))
            .map(|p| p + slot_label + 1)
            .expect("duplicate delay instr must live AT the slot label");
        assert!(block_delay < skip && skip < slot_label && slot_label < dup,
            "hazard-block -> skip -> label -> duplicate, got {:?}", gen.output);

        // The back-edge target label is no longer empty: the very next emitted
        // item after L_08804004 is the duplicate (not another label).
        assert!(
            gen.output[slot_label + 1].contains("ctx->r[6] = ctx->r[6] + -1"),
            "label must be immediately followed by the duplicate, got {:?}",
            &gen.output[slot_label..slot_label + 2]
        );
    }

    #[test]
    fn bids_pair_at_function_end_dispatches_fall_through() {
        // Degenerate: the BIDS pair is the last word pair — no L_A8 exists.
        // The skip must NOT goto a missing label, and (fall-through class)
        // must dispatch to func_end instead of silently returning. The
        // duplicate's own fall-off end gets the same tail.
        let ops = vec![
            MipsOp::BranchHazardDelay {
                branch: Box::new(MipsOp::Bne {
                    rs: Reg::Gpr(6),
                    rt: Reg::Zero,
                    target: 0x08804004,
                    likely: false,
                }),
                delay: Box::new(MipsOp::Addiu { rt: Reg::Gpr(6), rs: Reg::Gpr(6), imm: -1 }),
            },
            MipsOp::DelaySlotRejoin {
                instr: Box::new(MipsOp::Addiu { rt: Reg::Gpr(6), rs: Reg::Gpr(6), imm: -1 }),
            },
        ];
        let mut func = make_func(ops);
        func.size = 0x8;
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());
        let out = gen.output.join("\n");
        assert!(
            !out.contains("goto L_08804008"),
            "no goto to a label outside the function: {out}"
        );
        let skip = gen.output.iter().position(|s| s == "CALL_LOOKUP:0x08804008")
            .expect("BIDS end-of-body skip must dispatch to func_end");
        assert_eq!(gen.output[skip + 1], "return;", "skip dispatch must return: {out}");
        let slot_label = gen.output.iter().position(|s| s == "LABEL:L_08804004")
            .expect("slot label");
        assert!(skip < slot_label, "skip must precede the duplicate's label");
        assert!(
            gen.output[slot_label + 1..].contains(&"CALL_LOOKUP:0x08804008".to_string()),
            "duplicate's fall-off end must also dispatch to func_end: {out}"
        );
    }

    // --- Fall-through tail (function end reachable) ------------------------

    #[test]
    fn fall_off_end_emits_tail_dispatch() {
        // The dothack L5 shape (FUN_08851A24): a single plain store, no
        // terminal control flow — the body must dispatch to func_end (the
        // next function's entry), never silently return with stale v0.
        let ops = vec![MipsOp::Sw { rt: Reg::Zero, rs: Reg::Gpr(7), offset: 0 }];
        let mut func = make_func(ops);
        func.size = 4;
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());
        let lookup = gen.output.iter().position(|s| s == "CALL_LOOKUP:0x08804004")
            .expect("fall-off end must dispatch to func_end");
        assert_eq!(gen.output[lookup + 1], "return;", "tail must return after dispatch");
    }

    #[test]
    fn jr_ra_end_emits_no_tail() {
        let mut gen = TestGenerator::new();
        let func = make_func(vec![MipsOp::Nop {}, MipsOp::Jr { rs: Reg::Gpr(31) }]);
        emit_function(&func, &mut gen, &ImportMap::new());
        assert!(
            !gen.output.contains(&"CALL_LOOKUP:0x08804008".to_string()),
            "jr-ra end must not get a fall-through tail: {:?}",
            gen.output
        );
    }

    #[test]
    fn external_j_end_emits_no_tail() {
        // Tail jump out of the function: dispatch + return already ends flow.
        let func = make_func(vec![MipsOp::Nop {}, MipsOp::J { target: 0x08813000 }]);
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());
        assert!(
            !gen.output.contains(&"CALL_LOOKUP:0x08804008".to_string()),
            "external j end must not get a fall-through tail: {:?}",
            gen.output
        );
    }

    #[test]
    fn conditional_branch_end_emits_tail() {
        // Terminal conditional branch: the not-taken path runs off the end.
        let ops = vec![
            MipsOp::Nop {},
            MipsOp::Bne { rs: Reg::Gpr(4), rt: Reg::Zero, target: 0x08804000, likely: false },
        ];
        let mut gen = TestGenerator::new();
        emit_function(&make_func(ops), &mut gen, &ImportMap::new());
        assert!(
            gen.output.contains(&"CALL_LOOKUP:0x08804008".to_string()),
            "not-taken fall-off must dispatch to func_end: {:?}",
            gen.output
        );
    }

    #[test]
    fn always_taken_branch_end_emits_no_tail() {
        // `b` (beq r,r): the not-taken path is statically dead — no tail.
        let ops = vec![
            MipsOp::Nop {},
            MipsOp::Beq { rs: Reg::Gpr(4), rt: Reg::Gpr(4), target: 0x08804000, likely: false },
        ];
        let mut gen = TestGenerator::new();
        emit_function(&make_func(ops), &mut gen, &ImportMap::new());
        assert!(
            !gen.output.contains(&"CALL_LOOKUP:0x08804008".to_string()),
            "always-taken branch end must not get a tail: {:?}",
            gen.output
        );
    }

    #[test]
    fn reconstructed_epilogue_suppresses_tail() {
        // Mutual exclusion: the terminal-jal reconstructed epilogue already
        // synthesizes the downstream chain's frame teardown — also emitting
        // the fall-through tail would pop the frame a second time.
        let func = terminal_jal_func();
        let func_end = func.vaddr + func.size;
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());
        let out = gen.output.join("\n");
        assert!(out.contains("reconstructed epilogue"), "epilogue expected: {out}");
        assert!(
            !gen.output.contains(&format!("CALL_LOOKUP:0x{func_end:08X}")),
            "tail must be suppressed when the epilogue was reconstructed: {out}"
        );
    }

    #[test]
    fn terminal_external_jal_without_frame_emits_tail() {
        // Terminal external jal with NO recoverable frame: the epilogue
        // reconstruction declines, so the call-then-fall-through gets the
        // dispatch tail (hardware resumes at func_end after the call).
        let ops = vec![MipsOp::Nop {}, MipsOp::Jal { target: 0x08813000 }];
        let mut gen = TestGenerator::new();
        emit_function(&make_func(ops), &mut gen, &ImportMap::new());
        let out = gen.output.join("\n");
        assert!(!out.contains("reconstructed epilogue"), "no frame to reconstruct: {out}");
        assert!(
            gen.output.contains(&"CALL_LOOKUP:0x08804008".to_string()),
            "frameless terminal jal must get the fall-through tail: {out}"
        );
    }

    #[test]
    fn bids_likely_branch_keeps_taken_path_copy_and_duplicate() {
        // Branch-likely BIDS: delay-slot copy inside the taken path, skip
        // goto, duplicate at the slot label.
        let ops = vec![
            MipsOp::Beq {
                rs: Reg::Gpr(4),
                rt: Reg::Gpr(5),
                target: 0x0880400C,
                likely: true,
            },
            MipsOp::DelaySlotRejoin {
                instr: Box::new(MipsOp::Addiu { rt: Reg::Gpr(8), rs: Reg::Gpr(8), imm: 1 }),
            },
            MipsOp::Nop {},
            MipsOp::Jr { rs: Reg::Gpr(31) },
        ];
        let mut func = make_func(ops);
        func.size = 0x10;
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());
        let out = gen.output.join("\n");
        // The taken path holds the delay-slot copy (rendered by the real
        // CppGenerator via capture_op_cpp, hence the (int32_t) cast).
        assert!(
            gen.output.iter().any(|s| s.contains("goto L_0880400C")
                && s.contains("ctx->r[8] = (int32_t)(ctx->r[8] + 1);")),
            "taken path must execute the delay-slot copy: {out}"
        );
        assert!(out.contains("goto L_08804008;"), "skip over the duplicate: {out}");
        let slot_label = gen
            .output
            .iter()
            .position(|s| s == "LABEL:L_08804004")
            .expect("slot label");
        assert!(
            gen.output[slot_label + 1].contains("ctx->r[8] = ctx->r[8] + 1"),
            "duplicate at the slot label: {:?}",
            &gen.output[slot_label..slot_label + 2]
        );
    }

    #[test]
    fn internal_terminal_jal_not_reconstructed() {
        // A terminal jal whose target is INTERNAL is a goto, not a fall-through
        // tail call — no epilogue reconstruction.
        let ops = vec![
            MipsOp::Addiu { rt: Reg::Gpr(29), rs: Reg::Gpr(29), imm: -16 },
            MipsOp::Sw { rt: Reg::Gpr(31), rs: Reg::Gpr(29), offset: 12 },
            MipsOp::Jal { target: 0x08804000 }, // internal (== func.vaddr)
        ];
        let mut func = make_func(ops);
        func.size = 0xc;
        let mut gen = TestGenerator::new();
        emit_function(&func, &mut gen, &ImportMap::new());
        let out = gen.output.join("\n");
        assert!(!out.contains("reconstructed epilogue"), "internal jal is not a tail fall-through");
    }
}
