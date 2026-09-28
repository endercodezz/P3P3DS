# Adding a Game

This is the end-to-end path for bringing up a third title (the first two are
`patapon` and `dothack`). It covers analyzing the binary, writing the per-game
manifest, adding runtime hooks, building with the right game selected, and the
staged bring-up with the verification gates.

The seam is deliberately thin: per-game data is split between a TOML manifest
(`games/<id>/game.toml`, consumed by the recompiler) and an optional runtime
module (`games/<id>/runtime/`, compiled into the C++ runtime). The runtime core
stays game-agnostic — see [ARCHITECTURE.md](../ARCHITECTURE.md) for the overall
data flow.

Start from the skeleton in [`games/TEMPLATE/`](../games/TEMPLATE/): copy it to
`games/<id>/`, rename `TEMPLATE` to your id in both `game.toml` and
`runtime/hooks_main.cpp`, and fill in only what bring-up demands.

## 0. The golden rule

A well-behaved game should recompile and boot with **no manifest and no runtime
module at all** (the pure generic build). The manifest and the runtime hooks
exist for *curation* — forced entries the discovery passes missed, an asset
archive layer, address-keyed workarounds — never as bring-up table stakes. Add
each entry only after a concrete failure in the runtime points at it. Do not
copy Patapon's or .hack's addresses; they are specific to those binaries.

## 1. Analyze the binary

Produce `analysis.json` from the game's PRX/ELF. Ghidra supplies function
boundaries; psp-parser supplies the canonical relocated image.

```bash
cargo build --release
cargo run --release -- analyze \
    --ghidra-dir <ghidra-install>/libexec \
    --output data/<id>/analysis.json \
    path/to/BOOT.BIN
# macOS Homebrew: --ghidra-dir $(brew --prefix ghidra)/libexec
```

Notes:
- PRX modules rebase to `PSP_USER_MODULE_BASE` `0x08804000`; pass
  `--load-base <hex>` to override. ET_EXEC binaries load where they are linked
  (`--load-base` is ignored for them).
- `--ghidra-dir` can also come from the `GHIDRA_INSTALL_DIR` env var.
- The module start function is emitted as `entry` (not `FUN_<addr>`).

## 2. Write `game.toml` — facts vs choices

Copy `games/TEMPLATE/game.toml` to `games/<id>/game.toml`. The schema lives in
`crates/psp-cli/src/config.rs`; every key is optional. The manifest splits into
two kinds of content:

**Binary facts** — properties of the game image, true regardless of how you run
it. Most facts are read straight from `analysis.json` and need no manifest entry.
The manifest only records facts the analysis passes got *wrong* or *missed*:
- `[recompile] force_entries` — valid function starts Ghidra merged into a
  larger function. Each must be confirmed a real start (see §3).
- `[recompile] force_mid_entries` — `{entry, parent}` pairs for mid-entries
  Ghidra missed, observed as repeated `LOOKUP_MISS` in the runtime.
- `[recompile] force_entries_cross_mid` — like `force_entries` but only active
  under the `PSPRECOMP_CROSS_MID=1` recompile env (experimental graft baselines).

**Choices** — how *you* decide to run this title:
- `[game] id` / `title` — `id` must match `-DPSPRECOMP_GAME=<id>` and the
  directory name; `title` is informational.
- `[boot] boot_path` — guest exec path (default `disc0:/PSP_GAME/SYSDIR/BOOT.BIN`;
  EBOOT-only titles override).
- `[module] heap_base` — pin the heap base, overriding the align-to-16MB
  `RECOMP_HEAP_BASE` policy. Rare; leave unset unless a title needs its
  historical heap layout reproduced exactly.
- `[runtime] asset_layer` — `none` (default) or `bnd`. Only set `bnd` if you
  have a matching asset-layer hook and archive parser in `games/<id>/runtime/`.

The `[game]`/`[boot]`/`[module]`/`[runtime]` tables are emitted into
`output/include/recomp_game_config.h` (`RECOMP_GAME_ID`, `RECOMP_BOOT_PATH`,
`RECOMP_HEAP_OVERRIDE`, `RECOMP_ASSET_LAYER_BND`). The runtime **never parses
TOML** — it reads that generated header. `[recompile]` is consumed directly by
the recompiler's force-entry passes.

### Confirming a forced entry is real

Before adding any `force_entries` / `force_mid_entries`, confirm the address is
a genuine function start, not an arbitrary offset:
- Raw-word decode the bytes there (a leaf typically begins with a prologue, or
  for prologue-less leaves ends in `jr ra` with a sane body).
- Cross-check behavior against the PPSSPP oracle (see
  [DEBUGGING.md](../DEBUGGING.md) §4) — does forcing the entry make the runtime
  match PPSSPP at that call site?

`games/dothack/game.toml` is a minimal real example: a single `force_entries`
address (a prologue-less CRI-FS leaf in a Ghidra gap) with a comment recording
how it was confirmed.

## 3. Recompile with the manifest

```bash
cargo run --release -- recompile \
    data/<id>/analysis.json \
    --config games/<id>/game.toml \
    -o output
```

This writes the C++17 project into `output/` (including
`output/include/recomp_game_config.h` with your `[game] id`). `output/` is
gitignored — regenerate after any manifest or emitter change before rebuilding
the runtime.

## 4. Add the runtime module (`games/<id>/runtime/`)

Most titles can run with **no runtime module** — build with
`-DPSPRECOMP_GAME=none` and add a module only when you need an address-keyed
workaround, an asset layer, or a boot/thread context tweak.

The seam is `PspGameModule` in
[`runtime/include/psp_game_module.h`](../runtime/include/psp_game_module.h):

```c
struct PspGameModule {
    const char* id;                                              // must equal RECOMP_GAME_ID
    void (*register_hooks)(uint8_t* rdram);                      // after psp_hle_init(), before data sections
    void (*on_boot_context)(uint8_t* rdram, recomp_context* ctx);// just before entry()
    void (*on_thread_start)(uint8_t* rdram, uint32_t k0_addr);   // per PSP thread
};
const PspGameModule* psp_game_module();   // EXACTLY ONE strong definition per build
```

Rules:
- Every function pointer is **non-null** — use no-op bodies, never `nullptr`,
  so the generic call sites stay unconditional.
- Exactly one translation unit provides the strong `psp_game_module()` — your
  selected module, or `runtime/src/psp_game_default.cpp` when
  `-DPSPRECOMP_GAME=none`.
- CMake globs every `*.cpp` under `games/<id>/runtime/` (with
  `CONFIGURE_DEPENDS`, so new files are picked up without a manual reconfigure)
  and adds that directory to the include path. Put cross-TU declarations in a
  game-private header (mirror `games/patapon/runtime/patapon_hooks.h`); the
  purity gate (`runtime/tools/purity_gate.sh`) forbids any generic object file
  from referencing those symbols.

Where each hook fires (from `runtime/src/main.cpp` and
`runtime/src/hle/psp_hle_kernel_thread.cpp`):
- `register_hooks(rdram)` — after `psp_hle_init()` (import stubs bound, dispatch
  table pristine), before data sections load. This is where you call
  `psp_dispatch_register(<guest addr>, <wrapper>)` to wrap a guest function, or
  install asset/io/GE seam hooks.
- `on_boot_context(rdram, ctx)` — immediately before `entry()` (module_start);
  poke boot-time guest state (e.g. a CRT heap descriptor).
- `on_thread_start(rdram, k0_addr)` — after each PSP thread's k0 block is built.

`games/TEMPLATE/runtime/hooks_main.cpp` is the minimal module: it registers zero
hooks, so it behaves exactly like the generic build while still keying the module
to your id. `games/patapon/runtime/` is the worked-out example (dispatch
wrappers, BND asset layer, io policy, GE fallback split across `hooks_*.cpp`).

## 5. Build with the game selected

```bash
cmake -B runtime/build -S runtime -DPSPRECOMP_GAME=<id>
cmake --build runtime/build -j$(sysctl -n hw.ncpu)
```

`-DPSPRECOMP_GAME` is a CMake cache var (default `patapon`). `none` builds the
pure generic runtime (`psp_game_default.cpp`, zero hooks). An unknown id whose
`games/<id>/runtime/` glob is empty is a hard `FATAL_ERROR` at configure time.

At boot the runtime prints the compiled-in module id and cross-checks it against
`RECOMP_GAME_ID` from the output dir's `recomp_game_config.h`. A mismatch is a
loud warning (not fatal) — keep `[game] id`, `-DPSPRECOMP_GAME=<id>`, and the
directory name in sync so hooks built for one title never silently run against
another.

## 6. Staged bring-up

Bring a new title up in stages; do not chase pixels before the earlier stages
are green. Provide game data via `PSPRECOMP_DISC0=/path/to/extracted/iso`
(default `./disc0`).

1. **Boots** — the runtime reaches `entry()` and runs without crashing. Watch
   `stderr` for the `[RT]` boot lines (game module, memory, dispatch table, HLE)
   and for `LOOKUP_MISS`. Each repeated `LOOKUP_MISS` is a candidate
   `force_entries` / `force_mid_entries` once confirmed (§2). Run with
   `PSPRECOMP_STRICT=1` to abort on the first miss while triaging.
2. **Main thread** — the game's main thread starts and makes progress (asset
   loads, kernel calls succeed). Inspect live state over the debug socket (§7).
   Missing/garbage guest state here usually points at an unforced mid-entry or a
   missing asset/io policy.
3. **First pixels** — the GE submits real geometry and a frame becomes visible.
   A non-black prim count is necessary but **not sufficient** — verify the frame
   is actually visible (§7), since a broken transform can submit thousands of
   prims that all collapse to a sliver.

## 7. Verify — debug socket + CLEANROOM gate

Full methodology is in [DEBUGGING.md](../DEBUGGING.md); the two tools you need
for bring-up:

**Runtime debug socket** (TCP `127.0.0.1:9999`, started automatically). One
command per line via `nc` — read/write guest memory, query subsystem info, and
capture a screenshot:

```bash
printf 'I\n' | nc -w 3 127.0.0.1 9999 | tail -n +2 | jq .   # subsystem info as JSON
printf 'R 9FFFF00 4\n' | nc -w 3 127.0.0.1 9999 | xxd       # read 4 bytes
printf 'S /tmp/frame.tga\n' | nc -w 12 127.0.0.1 9999       # screenshot
```

**CLEANROOM gate** — the canonical verification run. `PSPRECOMP_CLEANROOM=1`
disables behavioral shims so you measure real progress, not band-aids:

```bash
PSPRECOMP_CLEANROOM=1 timeout 120 \
  ./runtime/build/psprecomp_runtime > /tmp/run.log 2>&1
# Then grep /tmp/run.log for LOOKUP_MISS, crashes, and your stage's progress
# markers. Logs are multi-GB — always timeout, grep, then delete.
```

**Visible-output gate** — for stage 3, grab a frame at your title's timepoint
and assert it is actually visible (not just non-zero prim count):

```bash
PSPRECOMP_DISC0=./disc0 ./runtime/build/psprecomp_runtime > /tmp/run.log 2>&1 &
sleep 22 && printf 'S /tmp/title.tga\n' | nc -w 15 127.0.0.1 9999 && kill %1
python3 runtime/tools/visible_output_gate.py /tmp/title.tga
# Delete /tmp/run.log and /tmp/title.tga after.
```

The exact CLEANROOM pass/fail thresholds in DEBUGGING.md are Patapon-tuned;
derive your title's own band from a PPSSPP oracle baseline (DEBUGGING.md §4).

## Checklist

- [ ] `data/<id>/analysis.json` produced by `analyze`.
- [ ] `games/<id>/game.toml` from `games/TEMPLATE/`, `[game] id` set, forced
      entries confirmed (or none).
- [ ] `recompile --config games/<id>/game.toml -o output` regenerated `output/`.
- [ ] `games/<id>/runtime/` present only if hooks are needed (else
      `-DPSPRECOMP_GAME=none`); module id matches `[game] id`.
- [ ] Built with `-DPSPRECOMP_GAME=<id>`; no boot-time mismatch warning.
- [ ] Staged bring-up: boots -> main thread -> first pixels.
- [ ] CLEANROOM + visible-output gates pass for your title's baseline.
