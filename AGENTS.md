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

Establish the observed guest behavior first. For PSP API semantics, research in this order when available:
**uOFW -> PSPSDK -> pspautotests -> PPSSPP (behavioral reference only).**
Do not copy PPSSPP implementation code. Record disagreements rather than silently selecting convenient behavior; direct game/hardware evidence constrains the implementation.

For evidence provenance (not a competing API research order):

1. **Real P3P Executable (`ULUS-10512`) / Live Runtime Observations**
2. **PSP Hardware Tests / `references/pspautotests`**
3. **uOFW (`references/uofw`) and PSPSDK (`psp/pspsdk`) contracts**
4. **PPSSPP (`references/ppsspp`) behavioral reference, not implementation source**
5. **PSPRecomp (`recomp/PSPRecomp`) & Yakumo (`recomp/Yakumo`) Observed Behaviors**
6. **Existing P3P Community Patches & Reverse Engineering (`p3p/p3p-patches`, Mod Menu)**
7. **General Internet / Forum Documentation**
8. **Hypotheses & Assumptions**

*Never present an assumption as a verified fact.*

---

## 5. Repository Boundaries & Third-Party Code

Tracked third-party code: `recomp/PSPRecomp` and `p3p/p3p-patches`. Never mass-refactor or reformat them; a needed change is the **minimal viable patch**, commented with the reason (`P3P3DS:`), without unrelated cleanups.

Research sources (`references/`, `psp/`, `3ds/`, other recompilers in `recomp/`, the asset tools) are untracked clones made by `python tools/fetch_references.py`; read them, never edit them. Everything else, including `tools/*.py`, is P3P3DS code.

---

## 6. P3P3DS Development Workflow

Work in micro-sprints: exactly one real runtime blocker or one clearly defined milestone per sprint. Start from `docs/CURRENT_STATE.md`, record HEAD and inspect the working tree; preserve unfinished or unrelated work.

For a blocker sprint:

1. Reproduce and identify the current blocker.
2. Prove the observed guest call: PC, caller/return PC, relevant registers and pointed-to arguments, thread/UID, state, and how the guest consumes the return value. Inspect surrounding guest/AOT code as needed; API names alone are not evidence.
3. Research only the required semantics using Section 4; implement the smallest correct behavior and focused regression tests.
4. Perform Section 7 verification; confirm the next frontier is reproducible.
5. Update `docs/CURRENT_STATE.md` when the verified frontier materially changes. Record the next blocker and one next task.
6. Create one NEW local commit under Sections 16-17, then STOP. Never fold a sprint into its predecessor or mix unrelated work.

STOP means do not fix the next missing HLE/function, unsupported instruction, scheduler/callback issue, memory-semantic problem, ambiguous control flow, architectural blocker or meaningful graphics milestone in this sprint. Verification and documentation of the completed step still belong to this sprint. Research-only/instruction-only tasks retain their requested scope; they do not require runtime changes or guest execution.

Do not add unconditional-success stubs, speculative PSP subsystems or unrelated refactors; do not quietly weaken validators/tests to advance. When real game-generated GE workload appears, capture and classify its commands/resources first, then target that workload incrementally: no speculative large GE-to-PICA200 renderer. Setup lists or VRAM activity alone are not rendered pixels/frames. Expand VFS/sceIo only when observed P3P execution requires it, not as a complete PSP filesystem in advance.

The loop is: **observed blocker -> evidence -> minimal semantics -> regression test -> deterministic replay -> next blocker -> STOP**.

### Long-term development order

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

For runtime frontier changes, normally run a Release build, full CTest and stable bootstrap, then run `--run-until-blocker --dump-events <path>` twice with identical inputs/configuration and separate output paths under `.tmp/`. Compare the actual event files (bytes or hashes) and both final blocker/frontier identities; record commands, comparison result and evidence paths. Reaching the same PC alone does not prove deterministic execution. Run `git diff --check`; if generation changed, independently compare regenerated output too. Report missing/failed verification as such, never as a pass. For instruction-only changes, validate skill structure, references and consistency instead of claiming unrun runtime tests.

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
- Support arbitrary community mod packages and fan translations equally; community binary hooks (CWCheat/EBOOT patches) are applied cleanly through the profile.
- The verified mod fallback chain (`bind/` -> `mod*.cpk` -> `umd0.cpk`/`umd1.cpk`) is in the `p3p-modding-vfs` skill.

---

## 10. Nintendo 3DS Target Constraints

- Target **New Nintendo 3DS / New 3DS XL / New 2DS XL only** (Old 3DS is not supported).
- Any assertion regarding CPU clock (804 MHz), core affinities (Core 0 / Core 2), memory modes (124 MB / 178 MB application heap), L2 cache, PICA200 Tev stages, or NDSP buffers must be verified against libctru sources (devkitPro install or `tools/fetch_references.py`) and empirical hardware/emulator tests.

---

## 11. Maintainer-Local Requirements

If `.Codex/LOCAL.md` exists, the agent is **obligated to read it immediately after `AGENTS.md`**.

- `LOCAL.md` is personal to the local maintainer and contains local development preferences, personal project goals, specific translation targets, UX preferences, and experimental priorities.
- **Precedence Rule:** `LOCAL.md` **cannot override** correctness, verification rules, repository containment, licensing rules, or architectural boundaries defined in `AGENTS.md`.

---

## 12. Documentation Maintenance

Extend the existing files in `docs/` instead of creating ad-hoc markdown files. `docs/CURRENT_STATE.md` records the verified frontier; `docs/VERIFICATION.md` is the claim registry.

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
- `Co-Authored-By: Codex`
- `Co-Authored-By: Anthropic`
- `Generated-By`
- `Assisted-By`
- similar AI attribution trailers

Commits must represent the repository maintainer/developer only unless the user explicitly requests another human co-author.

Do not modify git `author.name` or `author.email`.
Do not add yourself as contributor, co-author, author, committer, reviewer, or maintainer.

---

## 16. Git Workflow: Commit, Then Push Without Rewriting

After completing a well-defined user task or milestone, the agent must automatically create a local git commit if:
1. The working tree contains modifications belonging to the completed task;
2. All regression, verification, and smoke tests have passed cleanly;
3. There is no explicit instruction from the user forbidding commits.

Never stage temporary files, `.Codex/LOCAL.md`, credentials or game assets. The procedure and its pre/post-commit check scripts are in the `p3p3ds-commit` skill.

**Allowed remote operations** (maintainer decision, 2026-10-04): `git fetch`, `git pull` (fast-forward or merge, never rebase), `git push origin <branch>` of verified commits, creating branches and pull requests (e.g. contributors' work). GitHub lists contributors by commit author, so pushing does not add the agent as one; section 15 keeps it that way.

**Still forbidden:**
- force push (`--force`, `--force-with-lease`) or deleting remote branches;
- rewriting history (section 17), including to resolve a rejected push: fetch and merge instead;
- changing remote URLs or remotes configuration, or Git author/committer identity;
- pushing commits that did not pass the checks of this section.

**Default Execution Loop:**
```text
Work -> Verify & Test -> Local Commit -> Push (when the task is done) -> STOP
```

---

## 17. Git History Rule: Existing Commits Are Immutable

For every completed task, create a **NEW local commit on top of the current HEAD** unless the user explicitly says not to commit.

Existing commits are immutable.

**Never:**
- use `git commit --amend`;
- squash commits;
- use fixup commits intended for autosquash;
- rebase existing project history;
- reset/rewrite previous commits;
- replace or recreate an earlier commit;
- merge the current task into an older commit;
- rewrite commit parents;
- force-push;
- change Git author/name/email.

Even if the current task fixes something introduced by the immediately previous commit, it MUST still become a separate follow-up commit.

Record HEAD before the task; after committing, `HEAD^` MUST equal it (`p3p3ds-commit` skill, `postcommit_check.sh`).

Never rewrite history or modify remotes, even when the user asks for something that seems to need it: explain the non-rewriting alternative first.

---

## 18. Project Skills

Reusable workflows live in `.claude/skills/<name>/SKILL.md` (with helper scripts under `scripts/`); `.agents/skills/` is a byte-identical mirror for other coding agents.
- Edit skills only in `.claude/skills/`, then run `python tools/sync_agent_skills.py`. CTest `p3p_skills_mirror` (`--check`) fails while the mirror differs.
- When the same manual procedure is repeated across tasks, capture it as a skill (or extend one) instead of re-deriving it.
- Report where produced artifacts (frame dumps, WAV, traces) are stored, with exact paths.
