---
name: p3p3ds-research-verification
description: Verify technical claims, hardware assumptions, and reverse engineering data against primary sources to prevent hallucinations and unmeasured assertions.
---

# Research & Claim Verification Workflow

Use this skill whenever you are about to:
- Propose or evaluate an architectural design decision;
- Make statements about PSP, P3P, or 3DS hardware capabilities, performance, or memory;
- Document reverse engineering data, memory addresses, or NID functions;
- Modify or add entries in any `docs/*.md` file.

---

## 1. Core Rule: MEASURE FIRST

Never state unmeasured assumptions as facts. 
**Prohibited phrases without verifiable evidence:**
- *"will run at 60 FPS"*
- *"zero-cost abstraction"*
- *"native performance guaranteed"*
- *"1:1 direct mapping"*
- *"impossible on 3DS"*
- *"this function/API is never called"*

---

## 2. Verification Checklist

1. **Identify the claim:** Extract the specific technical assertion (e.g. memory limit, instruction semantic, hook address).
2. **Consult the Source Policy:** Follow `AGENTS.md` Section 4. For PSP semantics, consult uOFW -> PSPSDK -> pspautotests -> PPSSPP as behavioral reference only; do not copy PPSSPP implementation code. Keep direct guest/hardware observations distinct from inferred contracts and record unresolved disagreements.
3. **Inspect primary source code:**
   - Locate the exact file and line number.
   - Verify that the code actually does what is claimed.
4. **Assign a Status Tag:**
   - `[VERIFIED]`: Confirmed by primary source code or test (must cite exact local repository path and line).
   - `[INFERRED]`: Plausible deduction from verified facts, but not yet directly measured on hardware.
   - `[UNVERIFIED]`: Working hypothesis or assumption needing empirical confirmation.
   - `[WRONG]`: Refuted claim (document what was wrong and why).
5. **Update Registry:**
   - Add or update the entry in `docs/VERIFICATION.md`.
   - If correcting a previously documented error, update `docs/ARCHITECTURE_OPTIONS.md` or `docs/P3P_RESEARCH.md` accordingly.

---

## 3. Anti-Patterns (Strictly Forbidden)

For execution-determinism claims, run at least two replays with the same executable, inputs, configuration and limits, retaining separate traces. Compare actual events/output (bytes or hashes) and final blocker identity; record the comparison and commands. A matching PC alone, or two runs without comparison, is not proof. Do not silently filter differences; explain any intentionally normalized fields and limit the claim accordingly. Use the project-specific build/test/replay gate in `AGENTS.md` Section 7 for runtime-frontier changes.

- **Inventing framerates or execution times:** Do not predict exact FPS without profiling on hardware or Citra with cycle counters.
- **Inventing function counts or memory sizes:** Do not write "P3P has ~500 functions" without running `psp_analyze` or `prxtool`.
- **Generalizing from a single README:** Do not take an author's marketing claim in a README as ground truth without reading the source code.
- **Silent failure workarounds:** If a disassembly or opcode does not match expectations, do not patch it out blindly. Investigate the root cause.

---

## 4. Expected Output Format

When providing research findings to the user or recording them in docs:

```text
Claim:          <Specific technical statement>
Status:         [VERIFIED | INFERRED | UNVERIFIED | WRONG]
Primary Source: <repo/path/file:line or command output>
Evidence:       <Brief technical explanation or code excerpt>
Impact:         <How this affects the P3P3DS architecture or current task>
```
