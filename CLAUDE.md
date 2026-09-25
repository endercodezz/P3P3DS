# P3P3DS — Engineering Rules & Working Notes

Working instructions for all contributors and coding agents working in this repository. Read this file before performing any task.

---

## 1. What is P3P3DS

**Objective:** Run *Persona 3 Portable* (`ULUS-10512`, North American release) on the **New Nintendo 3DS / New 3DS XL / New 2DS XL** family of consoles.

**Working Hypothesis (Architecture C: Hybrid Static Recompilation):**
- **P3P Allegrex code:** Ahead-of-Time (AOT) offline static recompilation into C/C++ translation units, compiled to native ARM11 machine code via devkitARM GCC.
- **PSP OS & Kernel services:** Lightweight, modular High-Level Emulation (HLE) runtime (C/C++).
- **PSP Graphics Engine (GE):** Native `citro3d` / DMP PICA200 hardware renderer backend.
- **PSP Audio:** Native Nintendo 3DS DSP (`ndsp`) hardware-accelerated audio backend.
- **PSP Filesystem:** Multi-tier Virtual File System (VFS) with runtime mod and asset redirection on SDMC (`sdmc:/p3p3ds/`).

*The architecture is not a dogma:* If empirical experiments or profiling demonstrate that an aspect (e.g. interpreter fallback for unresolved indirect calls, or an alternative renderer pipeline) is superior, document the proof with benchmarks and propose the revision.

---

## 2. Workspace Containment

All temporary files, caches, build output, and generated data produced by the project must reside strictly inside the root of the P3P3DS workspace:
- **Never use external directories** such as `C:\`, user home (`~`), `%TEMP%`, `%APPDATA%`, `%LOCALAPPDATA%`, Documents, Desktop, or other host paths without explicit user permission.
- **Allowed workspace directories:**
  - `.tmp/` — Temporary test files, intermediate script scratchpads.
  - `.cache/` — Build and tool caches.
  - `build/` — Intermediate build artifacts.
  - `out/` — Final binary artifacts.
- **Environment redirection:** Where possible before running tests or compilers, redirect `TEMP`, `TMP`, and `TMPDIR` to `.tmp/` within the workspace.
- **Host environment integrity:** Never install global dependencies, modify system PATH, touch the Windows registry, or edit global user configs without explicit user authorization.

---

## 3. Core Engineering Principle: MEASURE FIRST

Never make unverified technical claims. Do not write phrases such as:
- *"will run at 60 FPS"*
- *"zero cost abstraction"*
- *"native speed"*
- *"1:1 hardware mapping"*
- *"JIT is impossible"*
- *"this API is completely unused"*

without providing concrete measurements, source code citations, disassembly excerpts, or reproducible benchmarks.

**Claim Status Markings:**
Every non-trivial architectural or technical assertion in documentation and reports must be tagged:
- `[VERIFIED]`: Confirmed against real game executable, hardware test, or reference code (must cite exact repository, file, and line/symbol).
- `[INFERRED]`: Strongly suggested by architecture or patterns, but not yet directly measured on target hardware.
- `[UNVERIFIED]`: Working hypothesis or theoretical assumption requiring empirical testing.
- `[WRONG]`: Previously believed claim refuted by empirical test or source inspection (keep in log to prevent regressions).

---

## 4. Source Hierarchy

When technical sources or observations conflict, resolve in this strict order:
1. **Real P3P Executable (`ULUS-10512`) / Live Runtime Observations**
2. **PSP Hardware Tests / `references/pspautotests`**
3. **PPSSPP (`references/ppsspp`) & uOFW (`references/uofw`) Implementations**
4. **Official Open-Source PSP SDK (`psp/pspsdk`) Documentation**
5. **PSPRecomp (`recomp/PSPRecomp`) & Yakumo (`recomp/Yakumo`) Observed Behaviors**
6. **Existing P3P Community Patches & Reverse Engineering (`p3p/p3p-patches`, Mod Menu)**
7. **General Internet / Forum Documentation**
8. **Hypotheses & Assumptions**

*Never present an assumption as a verified fact.*

---

## 5. Repository Boundaries & Third-Party Code

The following directories contain upstream/reference projects and must **never** undergo mass refactoring or formatting sweeps:
- `recomp/` (PSPRecomp, Yakumo, sal063-recomp, psprecomp, N64Recomp)
- `references/` (ppsspp, pspautotests, uofw, DaedalusX64-3DS)
- `psp/` (pspsdk, vfpu-docs, prxtool, ghidra-allegrex)
- `3ds/` (libctru, citro3d, citro2d, 3ds-examples)
- `p3p/` (p3p-patches, Persona-3-Portable-Mod-Menu)
- `tools/` (Atlus-Script-Tools, AemulusModManager, Amicitia, AtlusFileSystemLibrary, CriFsV2Lib, CriPakTools)

**Our Code Boundaries:**
All P3P3DS-specific code lives in:
- `core/` (Target-agnostic P3P runtime, HLE definitions, memory map)
- `platform/pc/` (Development/debugging PC host runner)
- `platform/3ds/` (New 3DS `libctru`/`citro3d`/`ndsp` native implementation)
- `profiles/p3p/` (P3P-specific static recompilation profile, configs, generated units)
- `experiments/` (Isolated standalone microtests)
- `docs/` (Architecture, hardware, and reverse engineering documentation)

If an experiment requires modifying an upstream file in a third-party directory, make the **minimal viable patch**, clearly comment the reason, and do not commit unrelated cleanups.

---

## 6. Development Order (Small Verifiable Steps)

Do not attempt to fix Allegrex decoding, write the HLE kernel, port the graphics renderer, and compile for 3DS simultaneously.

Follow the verified sequence:
```text
Static Analysis (EBOOT.ELF)
  ↓
Import & Function Boundary Discovery
  ↓
AOT Translation Unit Generation
  ↓
PC Host Runner Execution
  ↓
Core HLE (SysMem, ThreadMan, IoFileMgr)
  ↓
Game Boot Milestone (Titles / Disclaimers on PC)
  ↓
Graphics (GE display lists) & Audio (NDSP)
  ↓
Nintendo 3DS Platform Integration
```

---

## 7. Testing & Verification

Every functional change must include a verifiable test method:
- Unit test for isolated logic;
- Comparison against `pspautotests` for instruction or OS behavior;
- Behavioral diff against PPSSPP headless/log output;
- Deterministic trace log (e.g. instruction count, registered thread UIDs, opened file paths);
- Known P3P boot milestone (e.g. `module_start` reached, CRI initialization, first `sceIoOpen`).

*"Compiled successfully" is never proof of behavioral correctness.*

---

## 8. Persona 3 Portable Target Scope

- Initial target: **ULUS-10512** (US release).
- Do not introduce EU (`ULES-01523`) or JP (`ULJM-05500`) support until the US build boots and functions correctly.
- All game-specific addresses, hook offsets, and structures must reside in `profiles/p3p/config/` with clear attribution and verification tags.
- **Never scatter hardcoded magic addresses across generic runtime code.**

---

## 9. Modding & Localization Architecture

Generic modding support and arbitrary localization packages are first-class architectural requirements:
- Asset modifications must not require recompiling the runtime executable.
- The runtime must remain language-neutral: no specific translation or language may be hardcoded into the core engine.
- Implement the verified fallback resolution chain:
  ```text
  sdmc:/p3p3ds/mods/bind/<relative_path>
    ↓
  sdmc:/p3p3ds/mods/mod.cpk
    ↓
  sdmc:/p3p3ds/mods/mod1.cpk
    ↓
  sdmc:/p3p3ds/data/data.cpk (original game archive)
  ```
- Support arbitrary community mod packages and fan translations equally (e.g. Russian, German, French, Spanish, custom balancing, UI overhauls).
- Inspect whether community patches require binary hook addresses (e.g. CWCheat patches or EBOOT hooks) and implement them cleanly in the profile loader.

---

## 10. Nintendo 3DS Target Constraints

- Target **New Nintendo 3DS / New 3DS XL / New 2DS XL only** (Old 3DS is not supported).
- Any assertion regarding CPU clock (804 MHz), core affinities (Core 0 / Core 2), memory modes (124 MB / 178 MB application heap), L2 cache, PICA200 Tev stages, or NDSP buffers must be verified against `3ds/libctru` sources and empirical hardware/Citra tests.

---

## 11. Maintainer-Local Requirements

If `.claude/LOCAL.md` exists, the agent is **obligated to read it immediately after `CLAUDE.md`**.

- `LOCAL.md` is personal to the local maintainer and contains local development preferences, personal project goals, specific translation targets, UX preferences, and experimental priorities.
- **Precedence Rule:** `LOCAL.md` **cannot override** correctness, verification rules, repository containment, licensing rules, or architectural boundaries defined in `CLAUDE.md`.

---

## 12. Documentation Maintenance

Maintain established documentation files in `docs/` instead of proliferating ad-hoc markdown files:
- `docs/REPOSITORIES.md` — Inventory and roles of repositories.
- `docs/ARCHITECTURE_OPTIONS.md` — Comparative analysis of architectural paths.
- `docs/P3P_RESEARCH.md` — Formats, addresses, patches, and asset pipelines.
- `docs/3DS_PLATFORM.md` — Hardware specifications, 3DS services, and backend design.
- `docs/NEXT_STEPS.md` — Actionable technical roadmap and experiment milestones.
- `docs/VERIFICATION.md` — Registry of technical claims and their verification statuses.
- `docs/P3P_EXECUTABLE_ANALYSIS.md` — Verified technical analysis of the P3P executable.

---

## 13. Scope Discipline

- If asked to research: **do not implement code.**
- If asked to create a prototype: **do not over-engineer a production framework.**
- If asked to solve a localized issue: **do not refactor unrelated subsystems.**

---

## 14. Engineering Reporting Format

At the conclusion of each engineering turn, provide a concise structured summary:

```text
Changed:          <Files created, modified, or reconfigured>
Verified:         <Exact command, log milestone, or source reference used>
Tests:            <Status of regression or unit tests>
Remaining blocker: <Single immediate technical obstacle>
Next smallest step: <Actionable next milestone>
```

---

## 15. Git Authorship

Never add AI attribution or authorship metadata to commits.

Forbidden examples:
- `Co-Authored-By: Claude`
- `Co-Authored-By: Anthropic`
- `Generated-By`
- `Assisted-By`
- similar AI attribution trailers

Commits must represent the repository maintainer/developer only unless the user explicitly requests another human co-author.

Do not modify git `author.name` or `author.email`.
Do not add yourself as contributor, co-author, author, committer, reviewer, or maintainer.
