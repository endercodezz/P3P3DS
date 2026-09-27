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
2. **Consult the Source Hierarchy:**
   - Level 1: Real P3P executable (`ULUS-10512`) / live runtime traces.
   - Level 2: `references/pspautotests/` hardware test results.
   - Level 3: `references/ppsspp/Core/` or `references/uofw/` implementations.
   - Level 4: `psp/pspsdk/include/` headers and prototypes.
   - Level 5: `recomp/PSPRecomp/` and `recomp/Yakumo/` tested behaviors.
   - Level 6: `p3p/p3p-patches/` and `p3p/Persona-3-Portable-Mod-Menu/`.
   - Level 7: General internet/wiki documentation.
   - Level 8: Unverified hypotheses.
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
