# PSPrecomp V2 - Project Instructions

## 1. Project Overview

PSP static recompiler: Rust analysis/decode/emit pipeline produces C++17 output; C++17 runtime executes it with SDL2 + OpenGL 3.3.

**Core value:** Produce correct, compilable C++ that faithfully represents the PSP binary so that when paired with a runtime, the game boots and renders frames.
**Target binary:** Patapon BOOT.BIN (9,713 Ghidra-detected functions, 237 imported NIDs, 804 mid-entry points)

## 2. Architecture and Data Flow

```
psprecomp analyze --ghidra-dir <ghidra-install>/libexec BOOT.BIN -> analysis.json
psprecomp recompile analysis.json --config games/<id>/game.toml -o output -> output/ (C++17: batch_*.cpp, dispatch.cpp, data_sections.cpp, etc.)
cmake -B runtime/build -S runtime && cmake --build runtime/build -> psprecomp_runtime
```

### Rust Crates (`crates/`)

| Crate | Purpose |
|-------|---------|
| `psp-parser` | ELF/PRX parsing, NID database, relocations (goblin 0.9.3) |
| `psp-ir` | MipsOp enum, DecodedFunction, typed IR for all Allegrex + FPU instructions |
| `psp-decoder` | MIPS32 + Allegrex + FPU instruction decoding, delay slot reordering (two-pass) |
| `psp-optimizer` | Peephole passes (all disabled until Phase 7 -- `OptimizerConfig::default()` all false) |
| `psp-emitter` | C++ code generation, batch emission (rayon), dispatch table, CMakeLists |
| `psp-cli` | CLI entry point with `analyze`, `recompile`, `dump` subcommands |

### Key Output Files (`output/`)

`generated/batch_*.cpp` (per-batch translations), `dispatch.cpp` (address-to-function table), `data_sections.cpp` (.data/.rodata/.bss with 0x07FFFFFFU masking), `init_array.cpp`, `mid_entries.cpp`, `syscall_table.cpp` (generated NID import binding table -- the runtime binds HLE handlers by name at its stub addresses, issue #40), `funcs.h` (forward decls), `include/recomp.h` (recomp_context, register aliases, memory macros), `CMakeLists.txt` (globs `batch_*.cpp` only)

### Key Runtime Files (`runtime/`)

`src/main.cpp` (boot sequence, SDL2 event loop), `src/psp_memory.cpp` (128MB rdram), `src/psp_dispatch.cpp` (RECOMP_LOOKUP, miss handler), `src/psp_scheduler.cpp` (cooperative threading), `src/psp_render_queue.cpp` (condvar render protocol), `src/psp_event_loop.cpp` (SDL2 pump), `include/psp_runtime.h`

### Analysis: `analysis/ExtractAnalysis.java` -- Ghidra headless script (function export, xref mid-entries, callback scan)

### Live Memory Inspection

An external live-memory inspection tool can be pointed at either PPSSPP's debugger API (WebSocket; PPSSPP's debugger auto-listens — find the port with `lsof -i -P | grep -i PPSSPP | grep LISTEN`) or this runtime's debug socket to read structs, set breakpoints/watchpoints, and diff memory between the two.

**Runtime debug socket:** The runtime starts a TCP debug socket on port 9999 automatically (see `runtime/src/psp_debug_socket.cpp`). No setup needed — just run the runtime.

## 3. Key Conventions

These decisions are accumulated from 18+ completed plans. Violating them causes real bugs.

### Addresses and Data
- All addresses in analysis.json are hex strings (never u32/u64 in JSON)
- Function size = maxAddress - entry + 1 (not address-count methods)
- PSP memory addresses mask with `0x07FFFFFFU` (128MB address space)
- PRX modules rebase to `PSP_USER_MODULE_BASE` 0x08804000 (`--load-base` overrides); ET_EXEC loads where linked. Canonical image = psp-parser's relocated `segments[]`; Ghidra is byte-gate-verified, never a byte source (DEBUGGING.md #52)

### Runtime Architecture
- `rdram` is separate from `recomp_context` -- passed as separate function parameter for thread-safety
- Memory access via `psp_mem_read`/`psp_mem_write` with `0x07FFFFFFU` mask
- GL calls MUST go through render request queue (never from game threads -- macOS requirement)
- `thread_local PspThread* g_current` -- each OS thread knows its PspThread without shared lookup
- NO game-specific data in runtime core: per-game code lives in `games/<id>/runtime/` (selected by `-DPSPRECOMP_GAME`, default `patapon`), per-game choices in `games/<id>/game.toml`, binary facts in generated headers (`recomp_module.h`, `recomp_game_config.h`). A fix that only works for one title belongs in its game module, never in `runtime/src`

### Emitter/IR
- Mid-entry dispatch: wrapper sets `ctx->entry_point`, parent switch dispatches to label, clears before goto
- Branch/jump IR stores absolute u32 target addresses (not raw offsets)
- Cross-function branches use `RECOMP_LOOKUP` (not goto -- C++ goto cannot cross function boundaries)
- Decode errors produce empty stubs with error comments (not propagate)
- Deduplicated function names use `_ADDR` hex suffix for ODR safety
- JAL fallback uses `FUN_` prefix (Ghidra convention); module start is named "entry" (not FUN_089ACCD0)
- `ctx->f[N]` array notation for float registers (not `ctx->fN.fl`)

### Build System
- CMake glob must be `batch_*.cpp` only (not `generated/*.cpp` -- stale duplicates cause linker errors)
- SDL2 found via pkg-config (not find_package) on macOS
- GLAD2 requires `gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)`
- Optimizer passes disabled until Phase 7; PSP OS does not process `.init_array` (game CRT handles it)

## 4. Build Commands

```bash
# Rust pipeline
cargo build --release
cargo run --release -- analyze --ghidra-dir <ghidra-install>/libexec BOOT.BIN
# e.g. on macOS with Homebrew: --ghidra-dir $(brew --prefix ghidra)/libexec
cargo run --release -- recompile analysis.json --config games/patapon/game.toml -o output
cargo test

# C++ runtime (Release; PSPRECOMP_GAME defaults to "patapon" — compiles in
# games/patapon/runtime/. Use -DPSPRECOMP_GAME=none for a pure generic build
# with zero game hooks.)
cmake -B runtime/build -S runtime && cmake --build runtime/build -j$(sysctl -n hw.ncpu)

# C++ runtime (Debug -- for lldb)
cmake -B runtime/build-debug -S runtime -DCMAKE_BUILD_TYPE=Debug
cmake --build runtime/build-debug -j$(sysctl -n hw.ncpu)

# Run / Debug
./runtime/build/psprecomp_runtime                                    # Normal
PSPRECOMP_STRICT=1 ./runtime/build/psprecomp_runtime                 # Abort on LOOKUP_MISS
lldb ./runtime/build-debug/psprecomp_runtime                         # Debug session

# Dump single function's emitted C++ (byte-identical to its batch file; set
# PSPRECOMP_CROSS_MID=1 to match an output/ generated with it — see DEBUGGING.md #38)
cargo run --release -- dump analysis.json 0xADDRESS
```

## 5. pyghidra-mcp Integration

Ghidra binary analysis from Claude Code sessions (via the pyghidra-mcp MCP server). Use for: investigating unknown functions, checking xrefs, understanding GE dispatch, verifying decompiled output. Requires BOOT.BIN in a Ghidra project.

```
decompile_function("BOOT.BIN", "0xADDRESS")         # Decompile a function
list_cross_references("BOOT.BIN", "func_name")       # Find callers
search_symbols_by_name("BOOT.BIN", "sceKernel")      # Find PSP SDK patterns
gen_callgraph("BOOT.BIN", "entry", "callees", 3)     # Call graph
search_code("BOOT.BIN", "sceGe", 20, "literal")      # Code search
```

## 6. Debugging

**All debugging methodology lives in [DEBUGGING.md](DEBUGGING.md)** — triage by symptom,
lldb recipes, the oracle-driven differential method, the debug socket protocol, trace env
flags, CLEANROOM verification gates, and known failure patterns. Read it before debugging
anything; add new debugging knowledge there, not here.

Quick anchors:
- Crashes/stalls/wrong values → lldb on `runtime/build-debug/` (DEBUGGING.md §3, §5)
- Wrong data/render, no crash → PPSSPP as scriptable oracle (DEBUGGING.md §4)
- Verification runs → CLEANROOM gate (DEBUGGING.md §2); logs are multi-GB — timeout, grep, delete
- Scripted input / memory reads → runtime debug socket on TCP 9999 (DEBUGGING.md §6)

## 7. Critical Rules

1. **NEVER** modify `output/generated/*.cpp` directly -- always fix the Rust emitter and regenerate
2. **ALWAYS** check CMake glob patterns when adding/removing files from output/
3. `rdram` is NOT a `recomp_context` field -- it is a separate function parameter
4. PSP memory addresses mask with `0x07FFFFFFU` (128MB address space)
5. Module start function is named "entry" in emitter output (not `FUN_089ACCD0`)
6. Optimizer passes stay disabled until Phase 7
7. GL calls MUST go through render request queue (never from game threads -- macOS requirement)
8. Generated CMakeLists.txt globs `batch_*.cpp` only -- never `generated/*.cpp`
9. Cross-function branches use `RECOMP_LOOKUP` dispatch -- C++ goto cannot cross function boundaries
10. **MANDATORY runtime verification:** After implementing any phase or feature, ALWAYS build and run against Patapon BOOT.BIN before claiming completion. Check logs, verify behavior, iterate on failures. Never mark complete based on static analysis alone.

## 8. Documentation Maintenance

`ARCHITECTURE.md` is the structural reference; `README.md` is the high-level overview. Keep both true in the same change that makes them false:

- If a change alters the structure ARCHITECTURE.md describes (new/removed crate or subsystem, moved responsibility, changed data flow, new invariant), update ARCHITECTURE.md in that same change, including the reasoning for the structural change.
- If a change alters what README.md represents (status, capabilities, build steps, limitations), update README.md in that same change.

Keep tone factual and technical; no hyperbole. README links to ARCHITECTURE.md — do not duplicate structure in README.
