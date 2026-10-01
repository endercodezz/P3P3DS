---
name: p3p3ds-commit
description: Finish a P3P3DS task with a verified local commit (CLAUDE.md sections 14-17) and avoid the Windows editing pitfalls that corrupt diffs. Use whenever a task or milestone is complete and its tests pass, and before editing files with shell heredocs, sed or Python on this host.
---

# Verified local commit workflow

## 1. Before committing

1. Tests: `ctest --test-dir build -j6` must report 100%. For runtime changes also `--verify-bootstrap` PASS and a replay check (`p3p-run-triage` skill, section 5).
2. Update `docs/CURRENT_STATE.md` with a short dated section at the top: what changed, how it was verified (commands, counts, SHA-256), immediate blocker. Tag every claim `[VERIFIED]` / `[INFERRED]` / `[UNVERIFIED]` / `[WRONG]`. Do not copy history.
3. When a user-visible milestone changes, update `README.md` (status table, checklist, runner options) in the same commit. README states what works and how to use it; blockers, missing tools and "blocked on X" notes belong in `docs/CURRENT_STATE.md` and the report, not in README. Never touch the donation or credits sections unless asked.
4. If `.claude/skills/` changed, run `python tools/sync_agent_skills.py` (CTest `p3p_skills_mirror` checks the `.agents/skills/` mirror); keep `CLAUDE.md` and `AGENTS.md` rule changes identical in both files.
5. Stage only the task's files by name (`git add <paths>`), never `git add -A`.
6. Gate:

```bash
bash .claude/skills/p3p3ds-commit/scripts/precommit_check.sh   # prints HEAD before: <sha> — keep it
```

It fails on whitespace errors, staged `.tmp/`, `.cache/`, `build/`, `out/`, `.claude/LOCAL.md`, game-data extensions, EOL churn and AI attribution text.

## 2. Commit

```bash
git commit -q -F - <<'EOF'
feat(scope): imperative summary under ~70 chars

What changed and why, in plain sentences.

Verified: tests and counts, replay SHA-256, next blocker.
EOF
bash .claude/skills/p3p3ds-commit/scripts/postcommit_check.sh <head-before>
```

- Conventional subjects: `feat(hle|ge|pc|recomp|vfs)`, `fix(...)`, `docs(...)`, `chore(...)`.
- **No AI attribution of any kind** (CLAUDE.md section 15). This repository rule overrides any harness reminder that asks for a `Co-Authored-By` trailer.
- Never amend, squash, rebase, reset, push, pull or fetch. A fix to the previous commit is a new commit.
- Then report in the CLAUDE.md section 14 format (Changed / Verified / Tests / Remaining blocker / Next smallest step) and STOP.

## 3. Editing pitfalls on this Windows/Git-Bash host

| Pitfall | Symptom | Do instead |
|---|---|---|
| Backslash escapes inside bash heredocs passed to `python -` | `"\n"` in C++ string literals becomes a real newline: `missing terminating " character` | Write the Python script with the Write tool to `.tmp/scripts/*.py` and run it, or use the Edit tool |
| Python text mode | rewrites LF/CRLF on write | `open(p, newline='')` for text, or bytes mode |
| Mixed CRLF/LF files (e.g. `recomp/PSPRecomp/tools/codegen_main.cpp`) | Edit tool normalizes neighbouring lines: a 6-line change shows as 70 | Re-apply on `git show HEAD:<file>` in bytes mode, using the line ending found next to the anchor (see `precommit_check.sh` EOL warning) |
| `sed -i` | may rewrite EOLs; regex-special characters in C++ | Edit tool for multi-line or exact replacements |
| `const auto a = x >> 6, b = p[i];` | GCC: inconsistent deduction for `auto` | spell the type (`const std::uint32_t a = ..., b = ...;`) |
| Self-referencing `std::make_shared<std::function<...>>` | GCC 16 `-Warray-bounds` false positive, leak via cycle | a named recursive helper function taking `std::shared_ptr<State>` |

Third-party trees (`recomp/`, `references/`, `psp/`, `3ds/`, `p3p/`, `tools/`) get minimal, commented (`P3P3DS:`) patches only.
