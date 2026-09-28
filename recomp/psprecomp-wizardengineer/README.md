# psprecomp

A PSP static recompiler: a Rust pipeline translates Allegrex MIPS32 binaries into C++17 source,
and a C++17 runtime (SDL2 + OpenGL 3.3) executes the result with HLE implementations of the PSP
OS services. The target binary is **Patapon (USA) BOOT.BIN**.

> [!WARNING]
> **One game verified, not one game hardcoded.** The runtime core is game-agnostic — all
> Patapon-specific code lives in `games/patapon/` behind compile-time seams. But exactly one
> binary, **Patapon (USA)**, has been driven all the way to rendered graphics. HLE coverage,
> renderer fallbacks, and asset handling are exercised only as far as that title needs them, so
> other PSP games will most likely need additional work. See [Supported games](#supported-games).
>
> **Vibe coded.** The codebase is largely AI-generated (Claude). Every change is verified by
> running the game and diffing behavior against PPSSPP, but the code has not had a traditional
> human review pass — expect rough edges.

See [ARCHITECTURE.md](ARCHITECTURE.md) for the structural reference (crates, runtime subsystems,
data flow, invariants), and [docs/GRAPHICS.md](docs/GRAPHICS.md) for how the runtime translates
the PSP Graphics Engine to OpenGL 3.3.

## Table of Contents

- [Overview](#overview)
- [Pipeline](#pipeline)
- [Why Ghidra?](#why-ghidra)
- [Quickstart](#quickstart)
- [Supported games](#supported-games)
- [Current Status](#current-status)
- [Roadmap](#roadmap)
- [Building and Running](#building-and-running)
- [Verification Methodology](#verification-methodology)
- [Limitations](#limitations)
- [Documentation](#documentation)
- [Reference Projects](#reference-projects)
- [License](#license)

## Overview

The recompiler works in two stages. `psprecomp analyze` runs Ghidra headless analysis on the PSP
ELF and merges it with a Rust ELF parser into `analysis.json` (functions, imports, relocations,
mid-function entry points, xrefs, data sections). Both PSP executable formats are handled:
ET_EXEC binaries (Patapon's BOOT.BIN) load where linked, and relocatable PRX modules
(decrypted EBOOT.BIN, `e_type 0xFFA0`) are rebased to the PSP user-module base 0x08804000 with
their Type-A relocation tables applied and their imports parsed from SceModuleInfo.
`psprecomp recompile` decodes every instruction
into a typed IR and emits C++17: one C++ function per guest function, an address-to-function
dispatch table, and the data sections. The generated code compiles together with the runtime in
`runtime/`, which provides guest memory, a cooperative thread scheduler, HLE stubs for the
imported firmware NIDs, and a PSP Graphics Engine (GE) to OpenGL 3.3 translation layer.

For Patapon BOOT.BIN the current pipeline recompiles ~14,104 functions with 2,022 mid-function
entry points and HLEs 237 imported firmware NIDs. The NID-to-stub binding table is generated
per game into `output/syscall_table.cpp` from the analysis; the runtime carries no per-game
stub addresses.

## Pipeline

```mermaid
flowchart LR
    A[BOOT.BIN] -->|psprecomp analyze<br/>Ghidra headless + ELF parser| B[analysis.json]
    B -->|psprecomp recompile| C[output/<br/>C++17 source]
    C -->|cmake build| D[psprecomp_runtime]
    E[runtime/<br/>memory, scheduler, HLE, GE] --> D
    F[disc0/<br/>extracted ISO content] --> D
    D -->|SDL2 + OpenGL 3.3| G[Window]
```

## Why Ghidra?

A static recompiler must know where every function begins and ends **before** it decodes
anything — and a retail PSP binary doesn't say. BOOT.BIN is stripped: no symbol table, no
function boundaries, just a flat region of MIPS instructions and data. Simply decoding from the
entry point and following calls is not enough, because much of the code is only reachable
indirectly (C++ vtables, function-pointer tables, thread entry points, callbacks registered with
the OS).

Ghidra solves exactly that one problem. The `analyze` step runs Ghidra's headless auto-analysis
— a mature, battle-tested function-discovery engine — over the binary, and the
[`analysis/ExtractAnalysis.java`](analysis/ExtractAnalysis.java) script exports the results to
`analysis.json`: function entry points and sizes, mid-function entry points, cross-references,
and jump-table hints. For Patapon that census is ~9,700 functions.

Two things follow from this design:

- **Ghidra is needed once per binary.** After `analysis.json` exists, the `recompile` step and
  the runtime never touch Ghidra again. It is an analysis-time tool, not a runtime dependency —
  everything downstream (the decoder, the C++ emitter, the entire runtime) is this project's own
  code.
- **Ghidra's census is treated as a starting point, not ground truth.** Auto-analysis misses
  functions (no inbound xrefs, data-driven dispatch), so the pipeline supplements it: forced
  mid-entry injection and cross-function mid-jump discovery recover targets that surface at
  runtime as dispatch-table misses.

The PSP's CPU (Allegrex) is a MIPS32 variant with custom instructions and a vector unit (VFPU)
that stock Ghidra does not understand, so the
[ghidra-allegrex](https://github.com/kotcrab/ghidra-allegrex) processor extension is required —
see [Prerequisites](#prerequisites).

## Quickstart

This is the exact sequence that produces the verified Patapon title screen on macOS. It assumes
you have installed the [prerequisites](#prerequisites) (including the pinned Ghidra +
ghidra-allegrex combo) and have your own `BOOT.BIN` and extracted `disc0/` (no game data ships
with this repo).

```bash
# 0. Fetch the PSP NID database (once per clone; NOT committed — issue #53).
#    Maps firmware NIDs to SDK names so analyze can resolve import stubs.
./scripts/fetch-niddb.sh

# 1. Build the Rust pipeline.
cargo build --release

# 2. Analyze BOOT.BIN -> analysis.json (Ghidra headless; once per binary).
#    On a Homebrew Ghidra, --ghidra-dir is the install's libexec/.
cargo run --release -- analyze \
    --ghidra-dir "$(brew --prefix ghidra)/libexec" BOOT.BIN

# 3. Generate C++17 from analysis.json. PSPRECOMP_CROSS_MID=1 is required;
#    --config selects Patapon's manifest (force entries + per-game choices);
#    the --expect-* flags fail the build if the census drifts from the
#    verified Patapon baseline.
PSPRECOMP_CROSS_MID=1 cargo run --release -- recompile analysis.json \
    --config games/patapon/game.toml -o output \
    --expect-functions 14104 --expect-mid-entries 2022

# 4. Build the runtime (PSPRECOMP_GAME defaults to "patapon").
cmake -B runtime/build -S runtime \
    && cmake --build runtime/build -j$(sysctl -n hw.ncpu)

# 5. Run (the configuration the title screen was verified under).
PSPRECOMP_CROSS_MID=1 PSPRECOMP_CLEANROOM=1 ./runtime/build/psprecomp_runtime
```

The `--expect-functions`/`--expect-mid-entries` values are the Patapon baseline (14,104 / 2,022).
For a different binary, drop them or set them to that binary's expected census. Full prerequisite
and flag detail is in [Building and Running](#building-and-running); every environment variable is
catalogued in [docs/ENV_FLAGS.md](docs/ENV_FLAGS.md).

## Supported games

The recompiler core (decoder, emitter, runtime) is game-agnostic; per-game code is isolated under
`games/<id>/`. "Verified" below means actually driven to the stated behavior and checked against
PPSSPP — not "should work."

| Game | Status | Notes |
|------|--------|-------|
| **Patapon (USA)** — `BOOT.BIN`, UCUS-98643 | Boots + renders title screen (verified, macOS) | The reference title. Recompiles to 14,104 functions / 2,022 mid-entries; reaches the PATAPON logo + NEW GAME/CONTINUE menu. Gameplay beyond the title screen is unexplored. |
| **.hack//Link** — ULJS-00266 | Boots + draws its loading screen (in progress) | A second commercial binary, recompiled against the generic runtime (`-DPSPRECOMP_GAME=none`) with no per-game runtime code. Boots through `module_start`, runs its main thread, and reaches the GE render loop — drawing its in-game loading panel (shown below). The real menu/scene is still gated behind a loading-progression wall (#63), and boots are not yet deterministic. Config-only so far (`games/dothack/game.toml`). |

![.hack//Link (ULJS-00266) loading screen rendered by psprecomp_runtime](docs/dothack-render.png)

*Second game in the generic runtime: `.hack//Link` boots and reaches its render loop, drawing its loading panel — no Patapon-specific code involved. Full menu/scene rendering is still in progress ([#63](https://github.com/wizardengineer/psprecomp/issues/63)).*

The single platform is macOS — see [Limitations](#limitations) and
[docs/PLATFORMS.md](docs/PLATFORMS.md). To bring up another title, see
[docs/ADDING_A_GAME.md](docs/ADDING_A_GAME.md).

## Current Status

**The runtime boots Patapon and renders its title screen** — the PATAPON logo, the NEW
GAME/CONTINUE menu, and copyright text, visually matching PPSSPP. This is the public baseline: it
builds reproducibly and reaches the title **deterministically** (fresh recompile +
`PSPRECOMP_CLEANROOM=1`), with no per-game hacks in the runtime core.

![PATAPON title screen rendered by psprecomp_runtime](docs/title-screen.png)

How it got here:

- **Function discovery + faithful HLE** — the Ghidra census is supplemented with mid-entry and
  cross-jump recovery; IO HLE matches PPSSPP (path rejection, fd cap, `BADF`), the `f[]`/`fi[]`
  FPU register views alias correctly, and GE SIGNAL flow control runs the sub-lists Patapon keeps
  all of its real geometry in.
- **Correct geometry** — GE present on guest page-flip with an address-keyed FBO pool (#59), PSP
  viewport/depth mapping (#23), and the VFPU decode-rotation + per-op matrix-prefix fixes
  (#27/#67) that make the transform matrices correct.
- **Reproducible + onboardable** — byte-reproducible recompiles guarded by a build fingerprint, a
  pinned Ghidra + ghidra-allegrex toolchain, Rust-workspace CI, and the [docs](#documentation)
  below.

See the [Roadmap](#roadmap) for where this is going next.

## Roadmap

**Goal:** drive Patapon — and then a second title — to *playable* using only generic fixes (no
per-game hacks in the runtime core), verified against PPSSPP, while keeping the public baseline
stable and the project easy to build and extend. Progress is tracked in the
[issue tracker](https://github.com/wizardengineer/psprecomp/issues); `area:` and `severity:`
labels mark scope and priority.

**Done**

- [x] Boot Patapon to a rendered, deterministic title screen (the public baseline).
- [x] Correct GE present + VFPU transform matrices (#59, #23, #27/#67).
- [x] Byte-reproducible recompiles with a build-staleness fingerprint.
- [x] Onboarding/DX — pinned Ghidra toolchain + fixed install check (#75); env-flag,
      troubleshooting, platform, and adding-a-game docs; a `games/TEMPLATE/` skeleton; and
      Rust-workspace CI.

**Next — make Patapon playable** (tracked in #73; the through-line is scheduler faithfulness, #66)

- [ ] Activate and present the title menu: hand the cooperative scheduler's run-token to the asset
      loader so the menu state machine advances (#66), then present the drawn back-buffer without
      stalling the render queue.
- [ ] Test and wire menu input — cursor navigation and committing NEW GAME.
- [ ] Reach the new-game load, past the gates that currently block it: the SAS audio handshake
      (#29) and the PSMF intro stubs (#32).
- [ ] Remove the temporary degenerate-matrix fallback and render through the real 3D transform
      (#27/#67); the fallback is 2D-only and hardcodes Patapon's viewport.

**Beyond the title — broader goals**

- [ ] Audio: real ATRAC/SAS decode and a periodic audio IRQ / buffer drain (#29).
- [ ] GE/render fidelity: block transfers (#21), texture sampling (#22), the per-fragment pipeline
      (#24), 3D features — lighting / skinning / patches (#25), and display-list gaps (#26).
- [ ] Faithful kernel callback delivery (#11) and the event-flag wait-mode fix (#64).
- [ ] Second game: drive `.hack//Link` past its loading wall (#63) and generalize the runtime for
      more titles.
- [ ] Cross-platform: build and verify on Linux and Windows (#70 — see
      [docs/PLATFORMS.md](docs/PLATFORMS.md)).

## Building and Running

### Prerequisites

Build toolchain (macOS / Homebrew):

```bash
brew install cmake sdl2 pkg-config
```

- **Rust** (stable) and a **C++17 compiler**; CMake 3.16+, SDL2 (found via pkg-config), OpenGL 3.3.

#### Ghidra (for the `analyze` step only)

`analyze` runs Ghidra headless once per binary; `recompile` and the runtime never touch Ghidra.
Ghidra extensions are version-locked to a specific Ghidra patch, so use this exact, tested
combination:

| Component | Pinned version |
|-----------|----------------|
| Ghidra | **12.0.2 PUBLIC** (requires JDK 21) |
| [ghidra-allegrex](https://github.com/kotcrab/ghidra-allegrex) extension | **v21.3**, asset `ghidra_12.0.2_PUBLIC_20260310_ghidra-allegrex.zip` |

The ghidra-allegrex extension supplies the PSP Allegrex/VFPU instruction semantics and the
`PspElfLoader` the pipeline depends on (stock Ghidra understands neither). The extension
version-locks to the exact Ghidra patch — **match the asset name to your Ghidra
`application.version` 1:1**. v21.3 ships per-patch builds (`12.0`, `12.0.1`, `12.0.2`, `12.0.3`,
`12.0.4`, `12.1`); v21.2 and earlier do **not** support any Ghidra 12.x.

> [!WARNING]
> `brew install ghidra` currently lands a **newer** Ghidra (12.1.2 at time of writing), not the
> pinned 12.0.2. If you take the newer one, you must use the matching v21.3 `12.1` extension asset
> and re-verify. To stay on the tested combo, install Ghidra 12.0.2 directly (below).

Install Ghidra 12.0.2 and the matching extension:

```bash
# 1. Install Ghidra 12.0.2 PUBLIC.
#    macOS: download ghidra_12.0.2_PUBLIC from the official releases page and unzip, OR
#    `brew install ghidra` (then confirm the version below — brew may give 12.1.x).
#    Linux: download + unzip ghidra_12.0.2_PUBLIC, set GHIDRA_INSTALL_DIR.
#      https://github.com/NationalSecurityAgency/ghidra/releases
GHIDRA_DIR="$(brew --prefix ghidra)/libexec"   # Homebrew; on a manual unzip use the unzip dir
grep application.version "$GHIDRA_DIR/Ghidra/application.properties"   # confirm 12.0.2

# 2. Download the matching ghidra-allegrex extension (v21.3, 12.0.2 asset).
#    If step 1 gave a different 12.0.x/12.1, swap the asset name to match.
curl -L -o /tmp/allegrex.zip \
  https://github.com/kotcrab/ghidra-allegrex/releases/download/v21.3/ghidra_12.0.2_PUBLIC_20260310_ghidra-allegrex.zip

# 3. Install it: Ghidra GUI -> File -> Install Extensions -> "+" -> select the zip -> restart.
#    (Or unzip into "$GHIDRA_DIR/Ghidra/Extensions/" and restart.)
```

The analyze step checks for the ghidra-allegrex extension and **warns** if it can't locate it
(detection is heuristic, so it does not hard-block — see
[docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md)). Pass the install to `analyze` with
`--ghidra-dir` (Homebrew: `"$(brew --prefix ghidra)/libexec"`; manual unzip: the directory
containing `support/analyzeHeadless`, with no `libexec`).

You must also provide, from your own copy of the game (no game data is included in or
distributed with this repository):

- `BOOT.BIN` — the game executable, from `PSP_GAME/SYSDIR/` on the disc.
- `disc0/` — the extracted ISO contents (assets the game loads at runtime).

The `analyze` step also needs the PSP NID database (`data/niddb/ppsspp_niddb.xml`), which
maps firmware function NIDs to names. It is not committed (PPSSPP-derived; `data/` is
gitignored — issue #53). Fetch it once after cloning:

```bash
./scripts/fetch-niddb.sh
```

It downloads from [pspdev/psp-ghidra-scripts](https://github.com/pspdev/psp-ghidra-scripts)
(NID names derived from PPSSPP, GPL-2.0-or-later), verifies the checksum, and is idempotent.
If `analyze` is run without it, it stops with a message pointing at this script.

### Full pipeline

```bash
# 1. Rust pipeline
cargo build --release
cargo test

# 1b. NID database (once per clone; needed by analyze)
./scripts/fetch-niddb.sh

# 2. Analyze the binary -> analysis.json (requires Ghidra + ghidra-allegrex; once per binary)
cargo run --release -- analyze --ghidra-dir "$(brew --prefix ghidra)/libexec" BOOT.BIN

# 3. Generate C++ source from analysis.json (PSPRECOMP_CROSS_MID=1 is required;
#    --config selects the per-game manifest — Patapon's carries its force entries;
#    --expect-* fail the build if the census drifts from the Patapon baseline)
PSPRECOMP_CROSS_MID=1 cargo run --release -- recompile analysis.json \
    --config games/patapon/game.toml -o output \
    --expect-functions 14104 --expect-mid-entries 2022

# 4. Build the runtime (PSPRECOMP_GAME selects the games/<id>/ hook module;
#    defaults to "patapon". -DPSPRECOMP_GAME=none builds a pure generic
#    runtime with zero game-specific hooks — see ARCHITECTURE.md "Per-Game Layer")
cmake -B runtime/build -S runtime && cmake --build runtime/build -j$(sysctl -n hw.ncpu)

# 5. Run (the configuration the current status was verified under)
PSPRECOMP_CROSS_MID=1 PSPRECOMP_CLEANROOM=1 ./runtime/build/psprecomp_runtime
```

The most commonly used environment variables:

| Variable | Effect |
|----------|--------|
| `PSPRECOMP_CROSS_MID=1` | Cross-function mid-jump support; set at both recompile and run in the standard workflow |
| `PSPRECOMP_CLEANROOM=1` | Faithful HLE pass-throughs instead of legacy debug wrappers; the standard verification configuration |
| `PSPRECOMP_DISC0=/path` | Extracted ISO content directory (default `./disc0`) |
| `PSPRECOMP_STRICT=1` | Abort on dispatch-table miss (debugging) |

The full set of environment variables is catalogued in [docs/ENV_FLAGS.md](docs/ENV_FLAGS.md).
If a step fails (missing NID database, Ghidra/extension mismatch, stale `output/`, a black
window), see [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md).

Inspect the emitted C++ for a single function:

```bash
cargo run --release -- dump analysis.json 0xADDRESS
```

## Verification Methodology

PPSSPP is used as a scriptable behavioral oracle (breakpoints, memory reads, and input injection
through its debugger API) to capture ground truth and find the first divergence from our runtime.
The runtime exposes a TCP debug socket on port 9999 for live memory inspection, and changes are
checked with adversarial sub-agent verification before they are banked.

## Limitations

- **Temporary renderer fallbacks.** A known open guest-side bug remains: recompiled FPU/VFPU code
  computes broken view/projection matrices (all-zero view, NaN projection). A renderer fallback
  compensates: degenerate matrices route to a game-installable NDC mapping
  (`games/patapon/runtime/hooks_ge.cpp` provides Patapon's ortho; generic builds pass world
  space through with a warning). This is adequate for the 2D title/menu screens but must be
  fixed before 3D gameplay: the mapping hardcodes Patapon's viewport and has no depth ordering.
- **No audio.** ATRAC and SAS are crude stubs (0 samples decoded, instant end-of-stream).
- **GE gaps.** SIGNAL relative/offset variants (0x13–0x18), lighting, texture matrix, bone/morph
  skinning, bezier surfaces, and block transfers (TRANSFERSTART) are unimplemented.
- **Beyond the title screen is unexplored.** Title-screen interactivity (menu input advancing the
  game's state machine) has not yet been exercised in our runtime.
- **A rare race** in the game's IO worker (a phantom job, roughly 1 in 20 boots) is tripwired but
  not fixed.
- **One game playable-bound, single platform.** Patapon is the only title driven to a complete
  rendered screen. The runtime core itself is now game-agnostic — all Patapon-specific code lives
  in `games/patapon/` behind compile-time seams, enforced by `runtime/tools/purity_gate.sh`
  (no game literals or symbols in core objects) — and a second commercial binary recompiles,
  links against the generic runtime (`-DPSPRECOMP_GAME=none`), boots through `module_start`, runs
  its main thread, and reaches the GE render loop drawing its loading screen, but has not rendered
  its real menu/scene yet (bring-up is ongoing — see [Supported games](#supported-games)).
  Type-B (0x700000A1) packed relocations are detected and rejected with an explicit error.
  Developed and tested on macOS only; Linux and Windows have never been tried (the build
  assumes SDL2 via pkg-config and OpenGL 3.3, and the render-queue threading model was
  designed around macOS constraints). Testing and supporting other operating systems is a
  to-do — see [docs/PLATFORMS.md](docs/PLATFORMS.md). Optimizer passes are disabled by design
  until a later phase. Per-game render coverage is summarized in
  [Supported games](#supported-games).

## Documentation

| Document | Covers |
|----------|--------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Structural reference: crates, generated output layout, per-game layer, runtime subsystems, data flow, invariants |
| [DEBUGGING.md](DEBUGGING.md) | Debugging methodology: triage by symptom, lldb recipes, oracle-driven differential method, verification gates, known failure patterns |
| [docs/GRAPHICS.md](docs/GRAPHICS.md) | How the runtime translates the PSP Graphics Engine to OpenGL 3.3 |
| [docs/SCHEDULER-DESIGN.md](docs/SCHEDULER-DESIGN.md) | Cooperative thread scheduler design |
| [docs/ENV_FLAGS.md](docs/ENV_FLAGS.md) | Complete catalogue of environment variables |
| [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) | Common setup and runtime errors and their fixes |
| [docs/ADDING_A_GAME.md](docs/ADDING_A_GAME.md) | Bringing up a new title against the generic runtime |
| [docs/PLATFORMS.md](docs/PLATFORMS.md) | Platform support status and porting notes |

## Reference Projects

- [N64Recomp](https://github.com/N64Recomp/N64Recomp) — N64 static recompiler with a similar
  dispatch-table/context-struct architecture
- [PPSSPP](https://github.com/hrydgard/ppsspp) — PSP emulator, used as a behavioral oracle
  (observable behavior only)

## License

GPL-2.0-or-later. See [LICENSE](LICENSE).

Portions of the runtime (VFPU register-file semantics, GE command constants, HLE error codes
and kernel-object semantics) are derived from [PPSSPP](https://github.com/hrydgard/ppsspp),
which is licensed GPL-2.0-or-later.

Bundled third-party loaders (`runtime/src/glad.c`, `runtime/include/glad/`,
`runtime/include/KHR/khrplatform.h`) carry their own permissive licenses, noted in their
file headers.
