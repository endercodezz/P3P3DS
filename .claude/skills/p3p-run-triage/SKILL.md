---
name: p3p-run-triage
description: Build and run the P3P PC runner, read why it stopped, look at rendered frames and recorded audio, and prove a change kept execution deterministic. Use after any runtime/HLE/GE/codegen change, when asked "how far does the game get", or before committing a functional change.
---

# P3P run → triage → determinism loop

This is the loop used for every frontier step: build, run until the game stops, classify the stop, inspect frames/audio, fix, and re-prove determinism. Scripts live in `.claude/skills/p3p-run-triage/scripts/` (standard library Python / bash, run from the repository root).

## 1. Build

```bash
export TEMP=$PWD/.tmp TMP=$PWD/.tmp TMPDIR=$PWD/.tmp
cmake --build build -j10 2>&1 | grep -E " error|warning:" -A3 | head -20
```

- A change to `recomp/PSPRecomp/tools/codegen_main.cpp` regenerates all 237 units; ninja restat recompiles only changed ones. A clean AOT build is ~10-11 min / 4.8 GiB peak: run it with `run_in_background` and poll, never block on it.
- After `--target clean`, rebuild **all** targets before CTest, otherwise tests fail as "not found".
- Never run two heavy builds at once when timing them.

## 2. Run until the game stops

```bash
build/p3p_pc_bootstrap.exe --run-until-blocker --max-dispatches 20000000 --ms0 .tmp/ms0 \
    --frames-dir .tmp/frames --frame-every 60 --wav .tmp/run.wav --dump-events .tmp/run.json > .tmp/run.log 2>&1
grep -aE "Stop Reason|Blocker type|\[AUDIO\]" .tmp/run.log
python .claude/skills/p3p-run-triage/scripts/summarize_events.py .tmp/run.json --last 12
```

Reference points (ULUS-10512, i5-12400F): 20M dispatches ≈ 44 s wall ≈ 3,860 frames ≈ 146 s virtual and reaches the title screen at frame 3600; 200M ≈ 7 min. Unbounded runs (`2000000000`) take many minutes: background them.

`--verify-bootstrap --max-dispatches 100000` must still print `Stable bootstrap: PASS` (CTest `p3p_pc_bootstrap_smoke`).

## 3. Classify the stop

| Stop reason / blocker type | Meaning | Next step |
|---|---|---|
| `Missing HLE import <lib>::<name or NID>` / `missing_hle` | an import with no handler | `psp-hle-runtime` skill: name the NID, find the contract, implement + test |
| `GE ...` / `graphics` | GE command/feature not implemented or a callback cap | `core/src/hle/ge.cpp`, `core/src/ge/software_renderer.cpp` |
| `No recompiled function ...` | PC without AOT entry and interpreter off | the interpreter fallback normally covers this |
| `Deadlock: ...` | every thread waits with no deadline | find who should wake the named waiters |
| `Dispatch limit reached` (typed `runtime_semantics`, a known telemetry quirk) | budget only, **not** a blocker | look at frames/audio progress instead |
| `Diagnostic event budget exceeded` | an event type is uncapped | add it to the caps in `platform/pc/main.cpp` |

Event dumps keep only the first occurrences of capped types (`guest_transfer`, `hle_hit`, ...); `event_type_counts` are complete. The `events.back()` observer in `main.cpp` is order-sensitive: anything that emits events from inside the observer must run after `checkpoint.observe`.

## 4. Look at the picture and the sound

```bash
python .claude/skills/p3p-run-triage/scripts/frames_to_png.py .tmp/frames          # where the picture changes
python .claude/skills/p3p-run-triage/scripts/frames_to_png.py .tmp/frames 3600     # → frame_03600.png
```

Then open the PNG with the Read tool. Known milestones: ATLUS logo ≈ frames 70-140, CRIWARE ≈ 150-220, opening movie (black: no H.264 decoder) until ≈ 3240, title "PRESS ANY BUTTON" ≈ 3300-3840, then the attract loop repeats.

Audio: `[AUDIO] buffers= frames= nonzero= peak= late= clipped=`. `late` or `clipped` > 0 is a bug signal. Silence before ≈ 115 s is expected (movie ATRAC3plus has no decoder). Claims that audio "sounds right" stay `[UNVERIFIED]` until a human listens.

## 5. Prove determinism

```bash
bash .claude/skills/p3p-run-triage/scripts/replay_check.sh 20000000
```

Must print `REPLAY IDENTICAL`. Record the event-dump SHA-256 in `docs/CURRENT_STATE.md`. A change that should not alter guest behaviour must reproduce the previous SHA exactly; an intended change gets a new SHA with the reason.

## 6. Guest data hygiene

Copies of game files (movies, CPK extracts) go only to `.tmp/` and are deleted when no longer needed; never stage them. `experiments/cpk-check` lists/extracts CPK contents: `build/cpk-check/CpkCheck.exe <iso> --list PSP_GAME/USRDIR/umd0.cpk [substring] [outdir]` (build it offline with `DOTNET_CLI_HOME=$PWD/.cache/dotnet NUGET_PACKAGES=$PWD/.cache/nuget dotnet build -c Release -o ../../build/cpk-check`).
