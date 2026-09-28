# Troubleshooting — first-run errors

A symptom → cause → fix guide for the failures most people hit the first time they
run the pipeline end to end. Each entry quotes the actual log/error so you can match
it by searching. For the full build/run flow see [README.md](../README.md); for every
environment variable see [docs/ENV_FLAGS.md](ENV_FLAGS.md); for deep debugging see
[DEBUGGING.md](../DEBUGGING.md).

The pipeline has three stages, and first-run errors cluster by stage:

- **analyze** (`psprecomp analyze`) — needs the ghidra-allegrex extension and the NID DB.
- **recompile** (`psprecomp recompile`) — refuses an incomplete `analysis.json`.
- **build/run** (CMake + the C++ runtime) — needs a fresh `output/`, SDL2/OpenGL, and a
  matching game module.

---

## 1. analyze: ghidra-allegrex extension not detected

This is the highest-impact correctness issue: stock Ghidra 12.x bundles a
`Ghidra/Processors/Allegrex` SLEIGH module, so a naive "is Allegrex present?" check passes
even though the kotcrab **ghidra-allegrex extension** — which supplies `PspElfLoader` and the
VFPU-complete language — is absent. The analyze step pins `-loader PspElfLoader` and relies on
that VFPU-complete language; without the extension you get wrong/garbage analysis output.

`analyze` checks for the *extension* (not just the SLEIGH module) and **warns** if it can't
locate it (issue #75). It does **not** hard-block: detection is heuristic — the extension can
live in the install tree or a per-user settings dir, and its loader is packaged inside a jar —
so a false block would be worse than a warning. (Note: the byte-equality gate guards
*relocation*, not instruction decode, so it does not catch a stock-Allegrex VFPU mis-decode —
installing the extension is the real fix; this warning is the safeguard.)

**Symptom** (printed to stderr; analysis still runs)

```
warning: kotcrab ghidra-allegrex extension not detected under <ghidra>/Ghidra/Extensions or your Ghidra user-settings Extensions dir.
  Stock Ghidra's bundled Allegrex lacks the PspElfLoader + VFPU-complete language this pipeline requires; analysis may be incorrect. Install ghidra-allegrex v21.3 ...
```

If you installed the extension via the Ghidra GUI, this check may not see it and you can
ignore the warning. If *no* Allegrex language is present at all, `analyze` hard-errors instead
— that means `--ghidra-dir` is not pointing at a valid Ghidra install (its `libexec` dir).

**Confirm**

Installed extensions live flat under `Ghidra/Extensions/<name>/` (install-tree unzip) or your
Ghidra user-settings dir `~/.ghidra/.ghidra_<ver>/Extensions/` (GUI install), where `<name>`
contains `allegrex`. Stock Ghidra's bundled `Processors/Allegrex` never satisfies this check.

```bash
ls "$(brew --prefix ghidra)/libexec/Ghidra/Extensions" | grep -i allegrex
ls ~/.ghidra/.ghidra_*/Extensions 2>/dev/null | grep -i allegrex
```

No `allegrex` entry in either location → the extension is not installed.

**Fix**

Install the kotcrab ghidra-allegrex extension matching your Ghidra version. For Ghidra
12.0.2 use **v21.3**, asset `ghidra_12.0.2_PUBLIC_20260310_ghidra-allegrex.zip`, from
<https://github.com/kotcrab/ghidra-allegrex/releases/tag/v21.3>:

- GUI: File → Install Extensions → `+` → select the zip → restart Ghidra; or
- unzip into `<ghidra>/Ghidra/Extensions/`.

Match the asset name to your Ghidra `application.version` exactly, then re-run `analyze`.

---

## 2. analyze: NID database missing

`data/niddb/ppsspp_niddb.xml` is required by `analyze` to resolve the binary's import
stubs to real SDK function names (e.g. NID `0x...` → `sceGeListEnQueue`). It is **not
committed** — the NID names are PPSSPP-derived (GPL-2.0-or-later) and `data/` is gitignored
(issue #53), so a fresh clone does not have it.

**Symptom**

```
NID database not found at data/niddb/ppsspp_niddb.xml.
It is not committed (it is PPSSPP-derived; data/ is gitignored).
Fetch it with: ./scripts/fetch-niddb.sh
Or pass an existing copy with --nid-db <path>.
```

**Fix**

Run the bootstrap script once per clone (idempotent; checksum-verified):

```bash
./scripts/fetch-niddb.sh
```

It downloads the pinned upstream copy from `pspdev/psp-ghidra-scripts` and verifies its
SHA-256 before writing `data/niddb/ppsspp_niddb.xml`. If you already have a copy elsewhere,
point `analyze` at it instead:

```bash
cargo run --release -- analyze --ghidra-dir <ghidra>/libexec BOOT.BIN --nid-db /abs/path/ppsspp_niddb.xml
```

---

## 3. recompile: analysis.json is incomplete

`recompile` hard-errors (rather than emitting a silently broken runtime) when the
`analysis.json` is missing data the runtime depends on. All three of these mean the same
thing: re-run `analyze` against the original binary to produce a complete file.

### 3a. Empty `imports[]`

An empty `imports[]` means the analysis predates the import walker — the runtime would boot
with zero HLE bindings (every `sce*` call a no-op), a guaranteed silent failure (issue #40).

**Symptom**

```
<path> has an empty imports[] — syscall_table.cpp (the runtime's NID→HLE binding table, issue #40) cannot be generated.
Re-run analyze against the original binary:
    cargo run --release -- analyze --ghidra-dir <ghidra>/libexec <BOOT.BIN> -o <path>
```

### 3b. Missing `module{}` block

A missing `module` facts block means the file predates issue #47 Phase 2, so the runtime's
boot-facts header (`recomp_module.h`) cannot be generated.

**Symptom**

```
<path> has no `module` facts block — it predates issue #47 Phase 2, so recomp_module.h (the runtime's boot facts) cannot be generated.
Re-run analyze against the original binary:
    cargo run --release -- analyze --ghidra-dir <ghidra>/libexec <BOOT.BIN> -o <path>
```

**Fix (3a and 3b)**

Regenerate `analysis.json` with a current `analyze`:

```bash
cargo run --release -- analyze --ghidra-dir "$(brew --prefix ghidra)/libexec" BOOT.BIN -o analysis.json
```

If you have an augmented baseline you cannot regenerate wholesale, graft the `.imports`
(and `.module`) blocks from a fresh analyze instead — see DEBUGGING.md "Upgrading an
analysis.json baseline".

### 3c. `--expect-*` count mismatch

The optional `--expect-functions` / `--expect-mid-entries` flags fail the run if the final
census drifts from the value you assert. For Patapon the verified baseline is **14,104
functions / 2,022 mid-entries**.

**Symptom**

```
--expect-functions mismatch: expected 14104, got <actual> (see recompile_report.json counts for the breakdown)
```

**Fix**

Either the drift is expected (you changed the binary, config, or emitter) — update the
expected values to match the new census — or it is unexpected, in which case inspect
`recompile_report.json` for what changed. For a different binary, drop the flags or set them
to that binary's census:

```bash
PSPRECOMP_CROSS_MID=1 cargo run --release -- recompile analysis.json \
    --config games/patapon/game.toml -o output \
    --expect-functions 14104 --expect-mid-entries 2022
```

---

## 4. CMake configure: stale output/

`output/` is gitignored. After you change the emitter sources (or re-`analyze`), the
generated C++ in `output/` no longer reflects the code. The runtime's CMake configure step
recomputes the emitter-source content hash and compares it with `output/fingerprint.json`
(issue #36), and **fails the configure** on a mismatch — so you cannot silently build against
stale output.

**Symptom**

```
output/ is stale relative to the emitter sources — run:
    PSPRECOMP_CROSS_MID=1 cargo run --release -- recompile analysis.json --config games/<id>/game.toml -o <output>
STALE: emitter sources changed since output/ was recompiled (recorded <hash>..., actual <hash>...)
  changed: crates/psp-emitter/src/<file>.rs
(override with -DPSPRECOMP_ALLOW_STALE=ON at your own risk)
```

(A related variant, exit 3, prints `output/fingerprint.json not found (output/ predates
issue #36)` — same fix.)

**Fix**

Recompile, then re-configure. `PSPRECOMP_CROSS_MID=1` is required and is recorded in the
fingerprint's `cross_mid` flag:

```bash
PSPRECOMP_CROSS_MID=1 cargo run --release -- recompile analysis.json \
    --config games/patapon/game.toml -o output
cmake -B runtime/build -S runtime
```

Do **not** edit `output/generated/*.cpp` by hand — fix the Rust emitter and regenerate. The
escape hatch `-DPSPRECOMP_ALLOW_STALE=ON` downgrades the failure to a warning but means the
runtime will not reflect the current emitter sources; use it only when you know the
divergence is irrelevant.

---

## 5. CMake configure: SDL2 not found (macOS)

The runtime locates SDL2 via `pkg-config` (`pkg_check_modules(SDL2 REQUIRED ...)` in
`runtime/CMakeLists.txt`), not CMake's `find_package(SDL2)`. On macOS this matches a Homebrew
`sdl2` install. If SDL2 (or `pkg-config` itself) is absent, configure fails.

**Symptom**

```
-- Checking for module 'sdl2'
--   No package 'sdl2' found
CMake Error ... pkg_check_modules ... REQUIRED ...
```

or, if `pkg-config` is missing entirely:

```
Could NOT find PkgConfig (missing: PKG_CONFIG_EXECUTABLE)
```

**Fix**

```bash
brew install cmake sdl2 pkg-config
```

Then re-configure. SDL2 also pulls in the OpenGL 3.3 context the runtime requires; see
[docs/PLATFORMS.md](PLATFORMS.md) for the platform rationale (and why Linux should resolve
the same way via `libsdl2-dev` + Mesa).

---

## 6. Runtime: SDL/GL initialization fails at launch

The runtime creates an SDL2 window and an **OpenGL 3.3 core** context on the main thread,
then loads GL function pointers with GLAD2. Each step prints a distinct `[RT]` line on
failure and the process exits.

**Symptoms**

```
[RT] SDL_Init failed: <SDL error>
[RT] SDL_CreateWindow failed: <SDL error>
[RT] SDL_GL_CreateContext failed: <SDL error>
[RT] gladLoadGL failed
```

**Causes and fixes**

- `SDL_Init` / `SDL_CreateWindow` failing usually means no display is available (headless
  session, no `$DISPLAY`, SSH without forwarding). Run on a real desktop session.
- `SDL_GL_CreateContext failed` means the system cannot provide an OpenGL 3.3 core profile
  context — the GPU/driver does not support 3.3, or you are inside a VM/remote session
  without GL acceleration. Run on hardware with OpenGL 3.3 support.
- `gladLoadGL failed` means GLAD could not load the GL entry points. The context creation
  must precede `gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)`; if the context line
  succeeded but this fails, the driver is not exposing a usable 3.3 core profile — same fix
  as above.

---

## 7. Runtime: LOOKUP_MISS

When the runtime jumps to an address that is not in the dispatch table, it logs a miss and
(by default) installs a no-op stub so execution continues. A few `[LOOKUP_MISS]` lines for
unbacked stubs are expected and informational; a flood usually means the analysis or
dispatch table is incomplete for the path you reached.

**Symptom**

```
[LOOKUP_MISS] addr=0x08XXXXXX (first hit)
```

and at exit:

```
[LOOKUP_MISS_SUMMARY] <n> unique addresses, ...
```

**Investigate**

To turn the first miss into a hard stop with a backtrace, run under strict mode:

```bash
PSPRECOMP_STRICT=1 ./runtime/build/psprecomp_runtime
```

It aborts at the first miss:

```
STRICT: aborting on LOOKUP_MISS 0x08XXXXXX
```

The `[LOOKUP_MISS_CTX]` line dumps the missed address, caller, args, and a func ring for
context. Cross-check the missed address against `analysis.json` and
`recompile_report.json`'s `dispatch_audit.missing_targets[]`. See DEBUGGING.md (LOOKUP_MISS
triage) for the full method. `PSPRECOMP_STRICT` also aborts on unimplemented NIDs — see
[docs/ENV_FLAGS.md](ENV_FLAGS.md).

---

## 8. Runtime: game-module mismatch warning

`PSPRECOMP_GAME` selects the compiled-in game module at build time (default `patapon`). At
boot the runtime cross-checks that module's id against the manifest id baked into the
`output/` you are running. A mismatch is not fatal — a generic boot may be intentional — but
it is loud, because hooks compiled for one title can silently corrupt another.

**Symptom**

```
[RT] game module: patapon
[RT] WARNING: game-module mismatch — runtime compiled with PSPRECOMP_GAME='patapon' but this output dir was recompiled with manifest id '<other>'. Game hooks may target the wrong binary.
```

**Fix**

Build the runtime with the `PSPRECOMP_GAME` that matches the output's manifest id (the
`[game] id` in the `--config` you recompiled with), or build a generic runtime with no game
hooks:

```bash
# Match the game module to the output:
cmake -B runtime/build -S runtime -DPSPRECOMP_GAME=<id>

# Or a pure generic build (zero game hooks):
cmake -B runtime/build -S runtime -DPSPRECOMP_GAME=none
```

See [docs/ADDING_A_GAME.md](ADDING_A_GAME.md) for how game modules and manifests fit
together.

---

## Still stuck?

- Environment variables that change behavior: [docs/ENV_FLAGS.md](ENV_FLAGS.md)
- Platform support and toolchain notes: [docs/PLATFORMS.md](PLATFORMS.md)
- Deep debugging (lldb recipes, the oracle method, trace flags, the debug socket):
  [DEBUGGING.md](../DEBUGGING.md)
