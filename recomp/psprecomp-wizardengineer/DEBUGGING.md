# DEBUGGING.md — psprecomp debugging methodology

The single home for project-specific debugging practices, workflows, commands, failure
patterns, and lessons learned. CLAUDE.md points here; new debugging knowledge goes here,
not into CLAUDE.md. Distilled from ~48 recorded debugging sessions
(`.planning/handoff/next-session/`) and the 2026-06 infrastructure research
(`.planning/research/debug-infrastructure-research.md`).

Audience: humans **and** AI agents. Every workflow below is scriptable without window focus
or GUI interaction unless explicitly noted.

---

## 1. Core principles (violating these has cost real sessions)

1. **No fix without a measured store/branch/value.** Guessing the subsystem ("it's probably
   the X system") produced zero pixels across four phases once. Find the instruction or
   write that diverges first.
2. **Same-snapshot only.** Never track an absolute heap address across boot phases —
   addresses get reused; only within-snapshot diffs are trustworthy. Re-derive pointer
   chains per boot (double-deref!).
3. **CLEANROOM for verification.** `PSPRECOMP_CLEANROOM=1` disables register-mutating shims.
   Band-aids fabricate phantom blockers; always reproduce in the clean room before believing
   a divergence is real.
4. **A probe that doesn't replicate the game's own access conditions will lie.** Verify a
   surprising result a second, independent way before acting on it.
5. **Don't conclude "never" from one run.** Boot is nondeterministic; use ≥120 s windows and
   multiple runs.
6. **Anti-cascade.** If a fix only reveals the next layer, characterize it, file/comment the
   issue, and stop — use the oracle to find the *first* divergence instead of grinding.
7. **Define gates operationally** — by the exact log line and code line, never by a human
   label ("has 3D geometry" once meant something else than its label).
8. **Logs are radioactive.** Trace-enabled runs reach multi-GB. Always `timeout`, grep,
   delete. Never `cat` a runtime log.
9. **One dynamic resource, one agent.** One SDL window, one debug-socket port, one build
   tree. Parallel agents must use worktrees (symlink `disc0/` and `analysis.json`) and kill
   only their own binary path.
10. **Distrust prior conclusions without evidence links.** Six sessions were misled by stale
    handoff framings. Check `.planning/handoff/next-session/<highest>-START-HERE.md` §0
    corrections and §8 don't-re-chase before re-opening any closed lead.

## 2. Build & verification commands

```bash
# Rust pipeline
cargo build --release && cargo test --release
PSPRECOMP_CROSS_MID=1 cargo run --release -- recompile analysis.json \
    --config games/patapon/game.toml -o output

# Runtime (Release for verification, Debug for lldb). PSPRECOMP_GAME defaults
# to "patapon" (compiles in games/patapon/runtime/); -DPSPRECOMP_GAME=none
# builds the pure generic runtime — zero game hooks, used for quarantine runs
# (a generic-run log must contain NO Patapon hook/override/BND lines).
cmake -B runtime/build -S runtime && cmake --build runtime/build -j$(sysctl -n hw.ncpu)
cmake -B runtime/build-debug -S runtime -DCMAKE_BUILD_TYPE=Debug
cmake --build runtime/build-debug -j$(sysctl -n hw.ncpu)

# Unit gates
./runtime/build/test_vfpu                                              # 89/89
# test_asset_bnd lives with the BND layer in games/patapon/ (#47 P5); the
# target exists only in PSPRECOMP_GAME=patapon builds (the default).
DATA_CMN_BND_PATH=disc0/PSP_GAME/USRDIR/DATA_CMN.BND ./runtime/build/test_asset_bnd  # 33/33

# Purity gate (#46/#47 P5): proves the runtime core is game-free. Checks
# (1) source literals: every 0x08xxxxxx/0x09xxxxxx under runtime/src +
#     runtime/include is one of five allowlisted class-(b) PSP constants;
# (2) nm over the build's core object files (games/ objects excluded):
#     no defined OR undefined FUN_0*/patapon/BND symbol.
# Canonical run is against a generic build; the everyday patapon build is
# also valid input (continuous check).
cmake -B runtime/build-none -S runtime -DPSPRECOMP_GAME=none
cmake --build runtime/build-none -j$(sysctl -n hw.ncpu)
./runtime/tools/purity_gate.sh runtime/build-none   # prints PASS/FAIL per check
./runtime/tools/purity_gate.sh runtime/build        # patapon build, same invariant

# CLEANROOM verification run (the canonical gate)
PSPRECOMP_CROSS_MID=1 PSPRECOMP_CLEANROOM=1 timeout 120 \
  ./runtime/build/psprecomp_runtime > /tmp/run.log 2>&1
# Gates: exit 124; sentinel-only LOOKUP_MISS; 0 "bus error"/"marking dead";
# [GE_GEOM_HEARTBEAT] monotonic; real_nonsprite in expected band (13.8k-15.7k @120s;
#   observed across sessions — boot nondeterminism moves it within this band);
# "proj matrix degenerate" ABSENT; exactly ONE degenerate-matrix fallback notice
#   ("view matrix all-zero" OR "composed MVP collapses prim", title-tuned ortho
#   fallback — see below). DELETE /tmp/run.log after grepping.

# VISIBLE-OUTPUT gate (mandatory — the prim-count gate above CANNOT see a black
# screen: a broken transform can submit thousands of real_nonsprite prims that
# all collapse to a sub-pixel sliver, leaving the frame black while
# real_nonsprite stays in band. This regressed dev once — #27 turned the Patapon
# title black with real_nonsprite untouched; see
# .planning/research/patapon-visible-output.md). After a verification run, grab a
# frame at the title timepoint and assert it is actually VISIBLE:
PSPRECOMP_DISC0=./disc0 ./runtime/build/psprecomp_runtime > /tmp/run.log 2>&1 &
sleep 22 && printf 'S /tmp/title.tga\n' | nc -w 15 127.0.0.1 9999 && kill %1
python3 runtime/tools/visible_output_gate.py /tmp/title.tga
# PASS == distinct colors >= 8 AND non-black >= 1% (defaults). Master/oracle
# baseline for the Patapon title is ~100 distinct, ~14% non-black (~13k white px
# = the "PATAPON" logo). A black grab (1 color, 0%) FAILS the gate.
# DELETE /tmp/run.log and /tmp/title.tga after.
```

**Stale-output trap:** `output/` is gitignored. After ANY emitter change you MUST
regenerate (`recompile`) before building the runtime, and `PSPRECOMP_CROSS_MID=1` must be
set at recompile time. This check is now **mechanical** (issue #36): the runtime's CMake
configure step diffs `output/fingerprint.json` against the live emitter sources and fails
with the regeneration command on mismatch; every run log starts with the
`[RT] output fingerprint:` line including the cross_mid flag (see "#36 — build
fingerprinting" below). Symptom of slipping past it (`-DPSPRECOMP_ALLOW_STALE=ON`, or a
build dir configured before the staleness appeared): measurements that contradict the code
you just changed — re-run a bare `cmake -B runtime/build -S runtime` to re-check.

## 3. Triage by symptom

| Symptom | First moves |
|---|---|
| **LOOKUP_MISS** | Function not in dispatch table. Check analysis.json; `[LOOKUP_MISS_CTX]` dumps args + 32-entry func ring; `PSPRECOMP_STRICT=1` aborts at first miss for a backtrace. |
| **SIGSEGV** | Address-masking issue. Check the access is within PSP_MEM_SIZE (128MB, mask `0x07FFFFFFU`). lldb on the Debug build. |
| **Hang/stall** | `lldb` → `process interrupt` → `bt`; `psp_threads` (custom command) for PSP thread wait states. Mid-entry dispatch: check `ctx->entry_point` set/cleared. Sema STUCK warns appear rate-limited. |
| **Wrong branch / wrong data** | Oracle-driven differential debugging (§4). Hardware watchpoint on "the value that should change but doesn't". |
| **No render / wrong render** | `PSPRECOMP_GE_TRACE=1`; geometry sentinels (`[GE_GEOM_VERDICT]`, heartbeat); matrix fallback notices; `PSPRECOMP_SCREENSHOT=/tmp/f.tga`; compare vs PPSSPP frame. |
| **Build failure in `output/generated/`** | Fix the Rust emitter and regenerate — NEVER edit generated files. |
| **Linker errors** | funcs.h vs batch names; `_ADDR` dedup suffixes; CMake glob must be `batch_*.cpp` only. |

## 4. Oracle-driven differential debugging (the method for hard data/render bugs)

PPSSPP is a scriptable behavioral oracle, not a passive reference:

1. **Drive it**: launch GUI PPSSPP with the game image (debugger WebSocket auto-listens —
   find the port: `lsof -i -P | grep -i PPSSPP | grep LISTEN`). `game.reset` arms
   breakpoints before one-time boot events; `input.buttons.press` advances menus. Some
   builds drop the `cpu.resume` reply — poll `cpu.status`.
2. **Capture ground truth**: exec-bp a function to read args; one-shot-bp the caller's `ra`
   for return values; memory-write-bp a field to find its writer.
3. **CAVEAT**: the current PPSSPP build **crashes under exec breakpoints/stepping**
   (CoreAudio). Poll-only: memory reads, `press`, memory-write `watch`. Relaunch with the
   ISO to replay boot.
4. **Diff same-snapshot only** (principle 2).
5. **psp-reclass** (`~/Projects/PersonalWork/psp-reclass`): dual-target memory bridge
   (PPSSPP WebSocket + our debug socket): `read`, `read-struct`, `follow` (pointer chains),
   `diff`, `watch`, `press`; `--json` on everything for agents.
6. **A/B with git stash** settles "what did this change really do" disputes decisively.
7. **Differential unit harness**: when an operation's semantics are in doubt, build a tiny
   host harness linking our implementation against PPSSPP's source semantics (the VFPU bug
   was pinned this way without any emulator run).

**Tooling verdict (2026-06)** — full rationale in
`.planning/research/ppsspp-rpc-mcp-research.md`:

- No new PPSSPP RPC bridge and no self-built MCP server. PPSSPP's WebSocket debugger is
  the daemon; psp-reclass is the agent-facing client/bridge over it (and over our debug
  socket, §6).
- psp-reclass's runtime-socket client read the v2 framed protocol (§6) as unframed —
  header bytes spliced into every payload, silent corruption. The fix lives on its branch
  `tooling/framed-runtime-client`.
- Third-party `dmang-dev/mcp-ppsspp` MAY be used as a convenience for PPSSPP-side
  poll-safe ops, under hard conditions on this crash-prone build: NEVER use its pause /
  step / breakpoint_* / screenshot tools (screenshot silently sends `cpu.stepping` — the
  documented crash path, item 3 above). It cannot reach the runtime debug socket, so it
  does not replace psp-reclass for differential/`diff` workflows.
- PPSSPP `gpu.buffer.*` events (including screenshot capture) hard-require
  `CORE_STEPPING_CPU` — the same crash path on this build. `gpu.record.dump` is the safe
  frame-capture route.

## 5. lldb recipes

Launch, don't attach (macOS denies attach). `runtime/.lldbinit` auto-loads
`runtime/tools/psp_lldb.py` (`psp_thread`, `psp_threads`, `psp_module`, `psp_sema`,
`psp_heap`). `g_rdram` symbol is `__ZL7g_rdram`.

```bash
lldb ./runtime/build-debug/psprecomp_runtime
(lldb) b FUN_0881E7A8                                   # recompiled function
(lldb) b hle_sceKernelCreateThread                      # HLE stub
(lldb) p/x ctx->r[4]                                    # a0 (r[2]=v0, r[29]=sp, r[31]=ra)
(lldb) memory read rdram+(0x089F69B4&0x07FFFFFF) -c 32  # PSP memory (masked)
(lldb) b FUN_0881E558 -c 'ctx->r[4] == 0x09000000'      # conditional
(lldb) watchpoint set expression -- rdram+(0x089F69B4&0x07FFFFFF)  # write watch
(lldb) process interrupt                                 # catch a stall
```

Landmines: counting breakpoint *hits* must grep `frame #0:` lines only (command echoes were
once counted as hits); a `/tmp/dis.py` shadowing a stdlib module once corrupted lldb reads;
`PC_TRACE`'s atexit dump won't fire on SIGKILL (heartbeat sentinels are fflushed and do
survive).

**Tripwire pattern** for rare, lldb-suppressed races: plant a one-shot in-process state dump
at the symptom site and let normal runs capture it.

## 6. Runtime debug socket (TCP 127.0.0.1:9999)

Started automatically (`runtime/src/psp_debug_socket.cpp`). One command per line via `nc`;
multiple concurrent clients supported. **v2 framing:** every command replies
`OK <len>\n` + `<len>` payload bytes, or `ERR <reason>\n` — malformed input is never
silently dropped. Full protocol details in the issue #35 section below.

| Command | Effect | Reply |
|---|---|---|
| `I` | Runtime info (uptime, GE counters, LOOKUP_MISS, recent funcs, threads) | `OK <len>` + one JSON line |
| `R <hexaddr> <decsize>` | Read masked PSP memory (≤65536 bytes; out-of-range zero-filled) | `OK <size>` + raw bytes |
| `RAW <hexaddr> <decsize>` | Legacy v1 read — unframed | raw bytes only |
| `W <hexaddr> <hexbytes>` | Write bytes into rdram (masked; whole range must be in bounds) | `OK 0` |
| `B <hexmask> <decms>` | Inject button mask for duration (UP=10, CROSS=4000, START=8; clamped 60 s) | `OK 0` |
| `S <path>` | Screenshot: render thread writes 480x272 TGA to `<path>` | `OK 0` (≤10 s) or `ERR timeout` |

```bash
printf 'B 10 250\n'   | nc 127.0.0.1 9999   # UP
printf 'B 4000 250\n' | nc 127.0.0.1 9999   # CROSS
printf 'I\n' | nc -w 3 127.0.0.1 9999 | tail -n +2 | jq .   # info as JSON
```

Keyboard (window focus required): arrows=D-pad, X/Enter=CROSS, Z=CIRCLE, A=SQUARE,
S=TRIANGLE, Q/W=L/R, Space=START, Tab=SELECT. Buttons auto-release on focus loss — prefer
the socket for scripted/agent input.

## 7. Trace environment flags

| Flag | Effect |
|---|---|
| `PSPRECOMP_STRICT=1` | Abort on LOOKUP_MISS / unimplemented NID |
| `PSPRECOMP_CLEANROOM=1` | Disable all behavioral shims (verification mode) |
| `PSPRECOMP_HLE_TRACE=1` | Per-call HLE arg/ret logging (`[HLE-TRACE]`) |
| `PSPRECOMP_PC_TRACE=1` | Per-function frequency counters + atexit top-20 |
| `PSPRECOMP_GE_TRACE=1` | GE command-stream logging |
| `PSPRECOMP_SEMA_TRACE=1` | Per-call sema logging (off by default — floods ~1 GB/min) |
| `PSPRECOMP_SCREENSHOT=path` | One-shot TGA after first prims + shutdown capture |
| `PSPRECOMP_CROSS_MID=1` | (recompile-time) cross-function mid-entry emission — must match workflow |

~30 more narrow investigation probes exist (LK_*, GK_*, BND_*, D1_TRACE, CTX_PROBE,
PUSH_TRACE, FE90_OBJ, ...); they are one-off band-aids/probes from past bugs — since
#47 Phases 4–5 they live with their hooks in `games/patapon/runtime/*.cpp` (only
compiled under `PSPRECOMP_GAME=patapon`; the generic core reads none of them).
The Patapon-tuned `PSPRECOMP_SPLEAK` shadow-stack detector was deleted in #47 P5
(re-creatable from the game module via `psp_trace_checkpoint`-style instrumentation).
Discover via `rg 'getenv\("PSPRECOMP_' runtime games crates`, and prefer not to rely on
them (issue #43 tracks a proper registry/channel system).

## 8. Static-side oracles

- **pyghidra-mcp** (BOOT.BIN in a Ghidra project): `decompile_function`,
  `list_cross_references`, `search_symbols_by_name`, `gen_callgraph`, `search_code`.
  A Ghidra RPC daemon alongside it was evaluated and rejected (2026-06): its
  differentiators (write-back, patching) conflict with "Ghidra is byte-gate-verified,
  never a byte source" (#52) and have no consumer here — see
  `.planning/research/ppsspp-rpc-mcp-research.md` §7.
- `cargo run --release -- dump analysis.json <mode>` for analysis.json inspection
  (see issue #38 section for single-function C++ dumps).

## 9. Known failure patterns (don't re-discover)

- **PPSSPP crashes when paused/stepping** — poll-only oracle work (§4).
- **Stale `output/`** after emitter changes — §2 trap; regenerate first.
- **`g_last_func_addr` in HLE logs is NOT a return address** — it's the last dispatched
  function; misread twice historically.
- **Band-aid stacks fabricate phantom blockers** — verify in CLEANROOM.
- **lv.q/VFPU register-file class bugs live in the runtime register file**, not the
  emitter/decoder (verified faithful against raw BOOT.BIN words).
- **The freq-que/at3 audio workers parked on their reqsemas is NORMAL idle state.**
- **`verify_geometry.sh`-style oracles need a self-test path** (a synthesized known-good
  draw) to prove they're not constant-false.
- **A loop that never advances inside ONE function (frozen `recent_funcs` ring, one frozen
  register) was the BIDS class** (#56, fixed): a branch into another branch's delay slot
  used to land on an empty label / the swapped branch. The decoder now duplicates the
  delay instruction at the slot's label (`DelaySlotRejoin`); a control transfer in a
  branched-into slot is a loud `BranchInDelaySlot` decode error (stub) — 3 known in
  .hack//Link, all GE-list data misdetected as code.

---

# Per-issue infrastructure notes

Sections below are added by the issue that introduced the infrastructure. Each section:
what was added, how to use it, how it was verified, and any new failure modes discovered.

## #35 — Debug socket v2 (ACK framing, info JSON, write, screenshot, multi-client)

**What was added** (`runtime/src/psp_debug_socket.cpp`, hooks wired in `runtime/src/main.cpp`):

- **ACK framing.** Every command replies `OK <len>\n` followed by exactly `<len>` payload
  bytes, or `ERR <reason>\n` with no payload. Malformed/unknown input always gets an ERR
  (v1 dropped it silently, which made scripted probing indistinguishable from a hang).
- **Multi-client.** Thread-per-client accept loop; concurrent `nc` sessions are independent
  (v1 served one client at a time — a second connect hung until the first closed).
- **Legacy compat choice:** v2 frames `R`; the v1 unframed read survives verbatim as the
  `RAW <hexaddr> <decsize>` alias (success replies are raw bytes with no header; malformed
  RAW still gets ERR). Old clients switch `R`→`RAW`, new clients use framed `R`.

### Command reference

| Command | Args | Success reply | ERR reasons |
|---|---|---|---|
| `I` | — | `OK <len>` + one newline-terminated JSON line | — |
| `R` | `<hexaddr> <decsize>` (1..65536) | `OK <size>` + raw bytes; masked `0x07FFFFFF`; out-of-range tail zero-filled | `bad-addr`, `bad-size`, `size-out-of-range` |
| `RAW` | same as `R` | raw bytes, **no header** (v1 behavior) | same as `R` |
| `W` | `<hexaddr> <hexbytes>` (even-length hex, no spaces) | `OK 0` | `bad-addr`, `bad-bytes`, `out-of-range` (whole range must fit — no partial writes) |
| `B` | `<hexmask> <decms>` (clamped 60 s) | `OK 0` | `bad-mask`, `bad-duration` |
| `S` | `<path>` (rest of line, spaces allowed) | `OK 0` after the TGA is on disk | `bad-path`, `unsupported`, `timeout` (10 s; also covers pre-GL boot and a concurrent capture in flight) |
| anything else | | | `unknown-command`, `empty`, `line-too-long` (>4095 chars; connection stays usable) |

### `I` JSON schema (one line, jq-able)

```json
{"uptime_sec": 70.9,
 "ge": {"frames": 1667, "prims": 12696, "real_nonsprite": 11030,
        "sprite_nonclear": 0, "clears": 1666},
 "lookup_miss": {"unique": 1, "total": 1},
 "recent_funcs": ["0x089B440C", "..."],
 "threads": [{"id": 0, "name": "user_main", "status": "RUNNING", "wait": ""},
             {"id": 2, "name": "sgx-psp-freq-thr", "status": "RUNNING", "wait": "sema:278"}]}
```

- `ge.*` mirrors the `[GE_GEOM_*]` sentinel counters (`real_nonsprite > 0` == GRAPHICS).
- `recent_funcs` is a shared 64-entry ring fed by every recompiled-function entry,
  oldest first — entries from all game threads interleave, so cross-thread ordering is
  approximate. (The per-thread PC-TRACE ring is `thread_local` and not readable here.)
- `threads[].status` ∈ DORMANT/READY/RUNNING/WAIT/DEAD/WAIT_SLEEP. `wait` is `sema:<uid>` /
  `semacb:<uid>` / `sleep` / `thread_end:<id>` / `""`. **Caveat:** sema waits block on the
  sema's own condvar without changing scheduler status, so a sema-blocked thread shows
  `status:"RUNNING"` with a non-empty `wait` — trust `wait` over `status` for "is it stuck".
- All reads are deliberately racy snapshots (documented in code); never assert on exact counts.

```bash
printf 'I\n' | nc -w 3 127.0.0.1 9999 | tail -n +2 \
  | jq '{frames: .ge.frames, waits: [.threads[] | select(.wait != "") | {name, wait}]}'
printf 'W 9FFFF00 DEADBEEF\n' | nc -w 3 127.0.0.1 9999       # OK 0
printf 'R 9FFFF00 4\n' | nc -w 3 127.0.0.1 9999 | xxd        # OK 4 + dead beef
printf 'S /tmp/frame.tga\n' | nc -w 12 127.0.0.1 9999        # OK 0 once written
```

**Screenshot path obeys the GL rule:** the socket thread only posts a request; the main
(GL) thread polls `ge_draw_service_screenshot_request()` each event-loop pass next to
`render_queue_process()` and does the FBO readback there. Give `nc` a `-w` ≥ the service
latency (capture is usually <50 ms once GL is up; the 10 s bound covers pre-GL boot).

**Failure modes discovered during verification:**
- `S` before `ge_draw_init` (first ~1 s of boot) fails fast → `ERR timeout`; retry later.
- v1's `strtoul` parsing accepted garbage as address 0 — v2 rejects empty/garbage tokens
  explicitly (`bad-addr`/`bad-size`), so don't rely on `R zz 4` reading address 0 anymore.
- `W` refuses partially-out-of-range writes entirely (no partial write), unlike `R` which
  zero-fills — asymmetry is intentional.

**How verified:** `runtime/build/test_debug_socket` (59 checks: framing, ERR paths, W/R
round-trip, masking, JSON shape + hostile-name escaping, two concurrent clients, oversized
line) plus a live run against Patapon — `I` during boot and at title, `W`+`R` round-trip at
`0x09FFFF00`, `S` produced the actual 480x272 title-screen TGA mid-run, `B 4000 250` → `OK 0`,
malformed commands → ERR, two simultaneous `nc` sessions served concurrently.
## #36 — build fingerprinting (stale-output detection)

Every `psprecomp recompile` writes `<output>/fingerprint.json` and
`<output>/include/recomp_fingerprint.h`. The runtime's CMake configure step verifies the
fingerprint; the runtime prints it at boot. Together they make the stale-output trap (§2)
mechanical instead of procedural.

### What the fingerprint covers

- **Emitter sources** (`emitter_sources_hash`): content hash over every `*.rs` under
  `crates/psp-emitter/src`, `crates/psp-decoder/src`, `crates/psp-ir/src`,
  `crates/psp-optimizer/src`, plus the psp-cli files that shape emission
  (`recompile.rs`, `config.rs`, `hle_entry_scanner.rs`). Content hashing catches
  **uncommitted** edits — a git commit hash cannot.
- **analysis.json**: SHA-256 of the input file's bytes (re-analyze without recompile is
  also stale).
- **Flags**: `cross_mid` — whether `PSPRECOMP_CROSS_MID=1` was set at recompile time
  (previously invisible after the fact).
- **Context** (not hashed): ISO-8601 timestamp, git commit + dirty flag, headline counts
  (functions / mid-entries / batch files).

**NOT covered:** `runtime/` sources (CMake rebuilds those itself), the NID database, game
config TOMLs passed via `--config`, and Cargo dependency versions (`cargo update` that
changes codegen is invisible). A stale **build dir** configured before the staleness
appeared also escapes until the next configure — `cmake --build` alone does not re-check.

### Hash recipe (v1) and why content hashes are listed per file

Per-file SHA-256 of raw bytes; combined hash = SHA-256 over `"{path}\n{hash}\n"` sorted by
repo-root-relative path. Deterministic: same trees → same hash; timestamps and git state do
not feed it. fingerprint.json embeds the **full per-file list**, and the configure check
recomputes hashes for exactly those files — so the Rust and Python sides can never disagree
about the file set. The recipe roots are embedded too, so files *added* after the recompile
are still detected. Authoritative recipe doc: module header of
`crates/psp-cli/src/fingerprint.rs`; mirror: `runtime/cmake/check_fingerprint.py`.

### Boot line (grep-stable, first line of every run log)

```
[RT] output fingerprint: <sha256> (cross_mid=1, recompiled 2026-06-12T06:18:21Z)
```

`unavailable (output/ predates issue #36)` appears for pre-fingerprint output dirs.

### Configure-time check, failure message, override

On mismatch, `cmake -B runtime/build -S runtime` fails with:

```
output/ is stale relative to the emitter sources — run:
    PSPRECOMP_CROSS_MID=1 cargo run --release -- recompile analysis.json --config games/<id>/game.toml -o output
STALE: emitter sources changed since output/ was recompiled (recorded <hash>..., actual <hash>...)
  changed: crates/psp-emitter/src/lib.rs
```

Override with `-DPSPRECOMP_ALLOW_STALE=ON` → configures with a loud warning instead.
**The option is cached** — pass `-DPSPRECOMP_ALLOW_STALE=OFF` to re-arm an existing build
dir. Missing `fingerprint.json` (pre-#36 output dirs) and unverifiable states (sources not
on disk at recompile time, missing Python3) are warnings, never errors.

### Verified (2026-06-12, Patapon BOOT.BIN)

Fresh recompile → configure printed `output/ fingerprint OK`, build + boot printed the
banner. Appending a comment to `crates/psp-emitter/src/lib.rs` without recompiling →
configure exited 1 with the message above naming the file; `-DPSPRECOMP_ALLOW_STALE=ON` →
exit 0 with the warning; revert + recompile → `fingerprint OK` with the **identical**
combined hash (determinism). Removing fingerprint.json → warning, configure + build OK.


## #47 P2 — recomp_module.h (generated module facts)

Every `psprecomp recompile` also writes `<output>/include/recomp_module.h` from the
analysis.json `module{}` block (plus `segments[0]`, `heap_base`, `constructors[]`):
`RECOMP_MODULE_NAME/ENTRY/GP/TEXT_START/TEXT_SIZE`, `RECOMP_SEG0_VADDR/MEMSZ`,
`RECOMP_HEAP_BASE`, `RECOMP_CTOR_COUNT/FIRST_CTOR`. The runtime boot path (NativeModule
block, boot GP, bump-heap base, boot probes, `sceKernelGetModuleIdByAddress` text range)
hard-includes it — per-game constants no longer live in runtime sources. It regenerates
with the output dir; staleness is covered by the #36 fingerprint (analysis.json hash).

`module{}` is produced by `analyze` for **both** ET_EXEC and PRX inputs from the same
SceModuleInfo parser path (`crates/psp-cli/src/prx_load.rs::build_module_facts`).
Text extent follows PPSSPP `ElfReader` semantics (`psp-parser elf::text_extent`).

### Differential note: text_size (investigated 2026-06-12)

The pre-P2 runtime hardcoded Patapon text_size `0x244D30` — that is the exec phdr's
`p_filesz`, which PPSSPP (`ElfReader::GetTotalTextSizeFromSeg`) only uses for
**sectionless** inputs. Patapon has section headers, so PPSSPP computes
`GetTotalTextSize()` = sum of SHF_ALLOC, non-WRITE, non-STRINGS section sizes =
`0x001D4E04`, and that is what a game would read via `sceKernelQueryModuleInfo` under
PPSSPP. Verdict: the old hardcoding was the bug; the generated fact is PPSSPP-faithful.
(All other Patapon facts equal the old constants exactly: name "Labo" — matches the
in-binary SceModuleInfo at 0x089D7EEC — entry 0x089ACCD0, GP 0x08A50D20, text_start
0x08804000, seg0 0x08804000+0x2D8400, heap_base 0x08AE0000 → 16MB-aligned 0x09000000.)

The old ctor boot probe address `0x08804B58` was not `constructors[0]` (it sits at
index 12 — `constructors[]` is unsorted; the first entry is `0x08804CE8`), and its
`RECOMP_LOOKUP(...) == nullptr` check could never fire (non-STRICT lookup returns a noop
stub). The probe now uses `RECOMP_FIRST_CTOR` + `psp_dispatch_probe_lookup` (nullable),
i.e. it actually checks.

### Upgrading an analysis.json baseline (#47 Phase 2)

`recompile` **hard-errors** on an analysis.json without `module{}` (anything analyzed
before Phase 2) and on an empty `imports[]` (anything analyzed before the ELF import
walker, issue #40 — the runtime would boot with zero HLE bindings). For a normal
baseline, just re-run analyze:

```bash
cargo run --release -- analyze --ghidra-dir $(brew --prefix ghidra)/libexec \
    disc0/PSP_GAME/SYSDIR/BOOT.BIN -o analysis.json
```

**The augmented Patapon baseline is the exception.** It carries 524 hand-curated
`source:"vtable_miss"` functions a fresh analyze does not produce. Measured empirically
(2026-06-12): fresh analyze = 9,719 functions (baseline 10,243 = fresh + exactly 524);
after recompile-stage discovery the fresh path reaches 14,098 functions / 2,017
mid-entries vs the baseline's 14,104 / 2,022 — recovery is **incomplete**. 13 dispatch
entries exist only in the baseline path: vtable_miss roots `0x08827FA8`, `0x08858D0C`,
`0x0887E598`, plus 10 entries derived from vtable_miss seeds (incl. the coalesced
`0x08827F7C/0x08827F9C` family and `0x0887E690/98`). Verdict: do NOT regenerate the
augmented baseline wholesale — **graft** the module block from a fresh analyze into it:

```bash
cargo run --release -- analyze --ghidra-dir $(brew --prefix ghidra)/libexec \
    disc0/PSP_GAME/SYSDIR/BOOT.BIN -o /tmp/fresh-analysis.json
jq --slurpfile fresh /tmp/fresh-analysis.json \
    '.module = $fresh[0].module | .imports = $fresh[0].imports' \
    analysis.json > analysis-upgraded.json && mv analysis-upgraded.json analysis.json
PSPRECOMP_CROSS_MID=1 cargo run --release -- recompile analysis.json \
    --config games/patapon/game.toml -o output \
    --expect-functions 14104 --expect-mid-entries 2022
```

(The manifest `force_entries` mechanism (#47 Phase 4) can carry the 524 vtable_miss
addresses as curated per-game data, but the grafted baseline remains the documented
Patapon path. The fresh-analyze build break (issue #54/#65) is **fixed** — see below.)

**The graft is mandatory for boot correctness, not for the build.** The build-blocking
half (issue #54/#65) is **resolved** by the `inject_force_mid_entries` function-entry
collision guard (`crates/psp-cli/src/recompile.rs`, `fix/baseline-stabilize`): on the
*fresh* export, enhanced discovery promotes `0x08827EA0/EB0` to standalone functions and
moves the `0x08827F44/F9C` bodies into `FUN_08827EB0`, so the old `game.toml`
force-mid-entries (authored for the coalesced augmented layout where `FUN_08827E7C` owns
them all) would emit cross-function `goto`s (`error: use of undeclared label`, CLAUDE.md
#9). The guard resolves each force-mid-entry's *actual* owning function: it skips entries
that are themselves a discovered standalone function (EA0/EB0 — redundant, reached via
`RECOMP_LOOKUP`) and re-points entries whose declared parent is stale to their real owner
(F44/F9C → `FUN_08827EB0`). On the augmented baseline the guard is a **no-op** (the
declared parents already match the coalesced owner), so `14104/2022/283` is unchanged.

The fresh path now **builds green** (verified `fix/baseline-stabilize`). It still **boots
short** of the baseline because the 524 vtable_miss functions + their 13 dispatch roots
(`0x08858D0C` et al.) are absent — without them boot stalls in a `sceIoOpenAsync`
fd-table loop and never reaches GE geometry (`real_nonsprite` stays 0). So the graft above
remains mandatory to reach the `~15618 real_nonsprite` baseline; the guard just removes the
compile barrier that previously blocked even *testing* the fresh path.


## #46/#47 P4 — per-game manifest + game module (PSPRECOMP_GAME)

A game's curated layer lives under `games/<id>/` (ARCHITECTURE.md "Per-Game Layer"):
`game.toml` feeds `recompile --config` (force entries; `[game]/[boot]/[module]/[runtime]`
choices → generated `recomp_game_config.h`), and `games/<id>/runtime/*.cpp` holds the
address-keyed hooks, compiled in via the CMake cache var `PSPRECOMP_GAME` (default
`patapon`; `none` = pure generic build).

Debugging handles:

- **Boot banner**: `[RT] game module: <id>` is the second log line; a
  `game-module mismatch` WARNING means the compiled-in module id differs from the
  output dir's manifest id (`RECOMP_GAME_ID`) — wrong-game hooks silently corrupt, so
  always resolve the warning before trusting a run.
- **Quarantine gate** (generic-build sanity): a `-DPSPRECOMP_GAME=none` run against ANY
  output must contain zero Patapon hook/override/BND lines —
  `grep -E '\[BND|\[GK|\[WRAP|\[SM_|dlmalloc overrides|installed for FUN' run.log` → 0.
  Patapon-specific misses (e.g. `0x08816F9C`, the GE finish label the Patapon module
  stubs) surface as loud `[LOOKUP_MISS]` lines instead — expected and informational.
- **Hook archaeology**: the migrated hook bodies keep their original main.cpp block
  numbering (4a..4z) in `games/patapon/runtime/hooks_main.cpp`, so older handoffs and
  planning docs still cross-reference. Behavioral hooks carry `REMOVAL CRITERION`
  comments (issue #46 acceptance).
- **Forgot the manifest?** Recompiling Patapon WITHOUT `--config games/patapon/game.toml`
  silently drops its 3 force entries + 4 force mid-entries — counts shift from
  14104/2022 and `--expect-*` flags catch it. The mismatch warning also fires
  (output id `""` vs module `patapon`).

## #37 — recompile_report.json (silent-path audit)

Every `psprecomp recompile` run writes `<output>/recompile_report.json` and prints a
one-paragraph summary. The report turns the pipeline's four historically *silent* failure
paths into named, counted artifacts — read it first when triaging a new game or an
unexplained runtime divergence.

### The four silent paths it exposes

| Path | What used to happen | Report field |
|------|--------------------|--------------|
| Unhandled relocation types | `reloc.rs` left the word untouched for anything other than R_MIPS_32/26/HI16/LO16 — unrelocated pointers with no trace | `unhandled_relocations` (r_type → count); also warned once per type at analyze time |
| Unresolved NIDs | a database miss silently became a `NID_0x%08X` stub name | `unresolved_nids[]` (nid, stub_addr, fallback_name) |
| Decode errors | function silently emitted as an empty stub with a comment | `decode_errors[]` (address, name, error) |
| Unbacked static dispatch targets | emitter generated `RECOMP_LOOKUP(0xADDR)` for a target absent from the dispatch table — a guaranteed LOOKUP_MISS if reached | `dispatch_audit.missing_targets[]` |

### Schema (schema_version 1)

Top-level keys: `schema_version`, `generated_at` (ISO-8601 UTC), `counts`,
`decode_errors[]`, `unresolved_nids[]`, `unhandled_relocations{}`, `dispatch_audit{}`,
`dedup_renames[]`. All addresses are hex strings (`"0x%08X"`, analysis.json style);
`unhandled_relocations` keys are decimal r_type values as strings.

`counts` carries the self-check numbers: `functions_total`, `functions_by_source`
(analysis.json `source` field), `discovery` (recompile-stage pass breakdown:
force/raw_scan/prologue/gap_start/gap_rescued), `mid_entries`, `batch_files`,
`imports_total`, plus a total per silent-path category. `dedup_renames[]` lists every
function the emitter renamed with the `_ADDR` suffix for ODR safety (the dispatch/funcs.h
name to grep for is `unique_name`, not the Ghidra name).

Authoritative schema doc: module header of `crates/psp-cli/src/report.rs`.

### Count assertions (--expect flags)

```bash
PSPRECOMP_CROSS_MID=1 cargo run --release -- recompile analysis.json \
    --config games/patapon/game.toml -o output \
    --expect-functions 14104 --expect-mid-entries 2022   # Patapon baseline
```

On mismatch, recompile exits nonzero with a clear error — *after* writing the report, so
the counts breakdown is available for triage. Both flags are optional; use them in scripts
and verification gates to turn the documented baseline into an assertion instead of a
doc-comment that drifts.

### Triaging a new game with the report

1. Recompile without expect flags; read the printed summary line.
2. `unhandled_relocations` non-empty (PRX games) → relocation support gap; affected code
   reads unrelocated pointers. Fix in `psp-parser/src/reloc.rs` before chasing runtime bugs.
3. `unresolved_nids` non-empty → update the NID database; each listed stub is an import the
   runtime can only no-op. (ELF games like Patapon have an empty analysis.json `imports`
   array — their stubs are discovered from `.lib.stub` at analyze time, so this list covers
   PRX-style imports.)
4. `decode_errors`: cluster the addresses. A contiguous run of entries with ASCII-looking
   words (e.g. `0x44555453` = "STUD") is data misdetected as code by heuristic discovery —
   harmless stubs. A decode error in a *Ghidra-sourced* function is a real decoder gap.
5. `dispatch_audit.missing_targets`: targets **inside** the loaded segment range are real —
   each is a guaranteed LOOKUP_MISS if reached (cross-check against runtime LOOKUP_MISS
   logs). Targets outside the segment range are artifacts of data words misdecoded as `j`
   inside heuristically recovered functions (same root cause as the decode-error cluster).

### Verified (2026-06-12, Patapon BOOT.BIN analysis.json)

Full recompile with `PSPRECOMP_CROSS_MID=1`: counts matched the documented baseline
(14,104 functions / 2,022 mid-entries / 283 batch files) and the expect flags passed;
a deliberate `--expect-functions 99` run exited 1 with the mismatch error. The report
contained the 4 known `FUN_089DBC*` decode errors among 1,830 total (1,829 from
recompile-stage heuristic discovery decoding data, 1 from hle_scan). The dispatch audit
found 346 missing targets — **all** at `0x02xxxxxx`, none inside the loaded segment
(0x08804000–0x08ADC400): garbage `j`-targets decoded from data words, not real misses.
Zero relocations (Patapon is a relocation-free ELF; the path is exercised by unit
tests instead). Since issue #40, analyze parses ET_EXEC imports too: Patapon's
`counts.imports_total` baseline is **237** (was 0) and `unresolved_nids` is **1**
(NID 0xEBD177D6, scePower — missing from data/niddb; see the #40 section).

## #40 — generated syscall table (NID import bindings)

Every `psprecomp recompile` writes `<output>/syscall_table.cpp`: one row per
analysis.json `imports[]` entry (stub address, NID, resolved name, library), sorted by
stub address, header stamped with the analysis.json sha256. The runtime's
`psp_hle_init()` walks `recomp_nid_stubs` (decls: `runtime/include/hle/psp_hle_imports.h`)
and binds HLE handlers **by name** at the stub addresses — the hand-maintained
`runtime/include/hle/psp_hle_syscall_table.h` is gone, and the runtime contains no
per-game stub addresses. Runtime code that needs a stub keys on the NID (a universal PSP
API constant) via `psp_hle_stub_addr_for_nid(nid)` — e.g. the GE sub-intr replay in
`psp_hle_ge.cpp`.

Pieces to know when debugging:

- **Boot line** (unchanged format): `[HLE] Import stubs: X/Y implemented, Z unimplemented`.
  Patapon baseline: `237/237 implemented, 0 unimplemented`. A sudden drop means HLE
  registration names diverged from NID-db names — diff the generated table's `func_name`
  column against `psp_hle_register` calls.
- **Unimplemented imports are loud, never silent — and return a deterministic v0=0**:
  each unbound stub is registered with a per-stub handler that logs
  `[HLE] UNIMPLEMENTED import <name> (NID 0x..., module ..., stub 0x...)` once on first
  call and sets `v0 = 0` (SCE_OK). It originally mirrored the raw stub (`jr $ra; nop` —
  no register effects), which made every consumer of the result branch on leftover r2
  garbage (.hack//Link L5: `sceKernelGetThreadCurrentPriority` garbage fed the main wait
  loop's branch). If a game needs a specific non-zero return, that is the signal to
  implement the NID, not to special-case the stub. Unresolved NIDs carry the walker's
  canonical fallback name `NID_0x%08X`; an HLE handler for such an import registers under
  exactly that name (example: `psp_hle_power.cpp` registers `NID_0xEBD177D6`).
- **Call sites**: the emitter lowers `jal <import stub>` to
  `RECOMP_LOOKUP(0xADDR)(rdram, ctx); /* sceName */` — through the dispatch table, never a
  direct named call (the by-name HLE symbols do not exist; psp_hle_init's registration is
  what makes the lookup hit the handler). The #37 dispatch audit therefore counts import
  stub addresses as registered.
- **Binding-diff gate (2026-06-12, hand-table deletion evidence)**: fresh Patapon analyze
  vs the 237-entry hand table — 237/237 stub addresses identical, 237/237 NIDs identical,
  236/237 names byte-identical (every previously hand-corrected name agrees with
  data/niddb). The single discrepancy: NID 0xEBD177D6 at 0x089D7A60 had the hand-invented
  placeholder `unknown_EBD177D6`; the walker emits `NID_0xEBD177D6` (niddb miss). PPSSPP
  `Core/HLE/scePower.cpp` identifies 0xEBD177D6 as scePowerSetClockFrequency (exported
  alias "scePower_EBD177D6"); the runtime handler keeps returning SCE_OK and now registers
  under the fallback name. The same NID shows up in .hack//Link's imports — the rename
  generalizes.
- **recompile hard-errors on an empty `imports[]`** (analysis.json predating the ELF
  import walker): the runtime would otherwise boot with zero HLE bindings. Fix: re-analyze
  (or graft `.imports`+`.module` for the augmented baseline, see the #47 P2 section).

## #38 — `dump <analysis.json> 0xADDR` (single-function C++ emission)

Emits exactly one function's generated C++ to stdout — no full recompile, no grepping
batch files, no writes under `output/`. The list modes are unchanged
(`dump analysis.json functions|imports|relocations|segments|mid_entries`, positional or
`--what`; default `functions`).

```bash
# Module start (named "entry" in emitter output)
PSPRECOMP_CROSS_MID=1 cargo run --release -- dump analysis.json 0x089ACCD0

# Ordinary function; 0x prefix optional, hex case-insensitive
PSPRECOMP_CROSS_MID=1 cargo run --release -- dump analysis.json 0881e7a8 > /tmp/f.cpp

# Mid-entry address -> actionable error on stderr, exit 1:
#   Error: 0x08804CE8 is a mid-entry inside FUN_08804a4c (entry 0x08804A4C); ...
#   dump the parent: psprecomp dump <analysis.json> 0x08804A4C
cargo run --release -- dump analysis.json 0x08804CE8
```

Address resolution is against the **post-pipeline** function list (after discovery,
force-injection, and the CROSS_MID passes). Non-entry addresses never silently emit the
wrong thing: a registered mid-entry names its parent; an address strictly inside a body
names the containing function; an unknown address lists the nearest entries below/above;
bad hex is a usage error. Errors and all tracing go to stderr — stdout carries only C++,
so it pipes/redirects cleanly.

**Fidelity / CROSS_MID note:** the dump goes through the same `prepare_emission` +
`emit_one_function` path as the batch emitter (same IR, same global `_ADDR` name dedup,
same mid-entry dispatch), so its output is **byte-identical** to the function's text in
its `batch_*.cpp` for the same flag set. That means `PSPRECOMP_CROSS_MID` (and
`PSPRECOMP_NO_COALESCE`/`PSPRECOMP_NO_RA_MODEL`, and `--config`) must match whatever the
`output/` you are comparing against was generated with — CROSS_MID changes coalescing and
mid-entries, which changes bodies. Pipeline prep takes a few seconds (it re-runs discovery,
and under CROSS_MID the coalesce/recovery decode passes) before the single function emits.

**How verified:** unit tests in `crates/psp-cli/src/dump.rs` compare the dump path against
in-memory batch emission for the real analysis.json (entry, an ordinary `FUN_`, a mid-entry
parent, an `_ADDR`-deduplicated thunk — all byte-identical substrings), plus end-to-end CLI
runs with `PSPRECOMP_CROSS_MID=1` on both sides: dumps of `entry` (0x089ACCD0, 4052 bytes),
mid-entry parent `FUN_08804a4c` (11637 bytes), dedup `thunk_FUN_08864f98_0887f1c4`, and
`FUN_0881e7a8` were each exact byte substrings of their `output/generated/batch_*.cpp`,
with the recompile baseline (14104/2022/283) unchanged.

## #52 — PRX bring-up (rebase, relocations, imports, byte-equality gate)

Relocatable PSP modules (`e_type 0xFFA0` — e.g. a decrypted `EBOOT.BIN` of a game whose
`BOOT.BIN` is a dummy) take a different analyze path than ET_EXEC binaries like Patapon:
psp-parser rebases all segments to `PSP_USER_MODULE_BASE` (0x08804000, override with
`analyze --load-base <hex>`), applies the Type-A relocation tables, parses SceModuleInfo +
the `.lib.stub` import table from the *relocated* image, and pins Ghidra's loader
(`-loader PspElfLoader -loader-imagebase <hex>`) so both engines see the same image.

### Diagnosing a base-0 / no-imports analysis (the original #52 symptom)

Any of these in an analyze run of a PRX means the rebase path did not engage — suspect a
stale cache (below) or a non-0xFFA0 e_type:

- functions named `FUN_000xxxxx`, `entry` missing, or a stray `FUN_00000000`
- `heap_base` ≈ `0x005xxxxx` instead of `0x08Dxxxxx`
- `Applied 0 relocations` / `Resolved 0 import stubs` in the log (import failures on a
  PRX are now hard errors, never silent empty arrays)
- 0 RAW_SCAN xrefs and 0 constructors (the Java script's PSP-range gates see base-0 addresses)

### "byte-equality gate FAILED for block ..."

`ExtractAnalysis.java` exports a SHA-256 per initialized memory block; analyze recomputes
each hash over its own relocated `segments[]` slice and hard-fails on mismatch. A failure
means the two relocation engines (psp-parser `reloc.rs` vs ghidra-allegrex) diverged —
**do not** weaken the gate; one of the engines is wrong. To find which: dump the named
block's first differing bytes from Ghidra (re-import the binary in the GUI with the same
imagebase) and compare against the analysis.json segment slice at the same VA
(`jq -r '.segments[0].data_b64' | base64 -d | xxd`). Plan
`.planning/plans/52-prx-support-plan.md` §1 D1 records the fallback strategy (pre-relocated
temp ELF) if a divergence cannot be fixed in `reloc.rs`. A *warning* that the gate was not
enforced means the `blocks` key is absent — legacy `ghidra_raw.json` or a `--ghidra-dir`-less
run.

### `.ghidra_raw.meta.json` cache rule

The `<output>.ghidra_raw.json` cache is reused only when its sidecar
`<output>.ghidra_raw.meta.json` matches the run (binary sha256, loader, imagebase) — a
pre-#52 base-0 cache can therefore never silently poison a rebased re-run. Mismatch or
missing meta: with `--ghidra-dir`, Ghidra re-runs (one-time cost for legacy caches);
without it, analyze errors and tells you to re-run with `--ghidra-dir` or delete the cache.

### Type-B relocations

`0x700000A1` packed relocation tables (stripped kernel-style PRX) are detected but
deliberately unimplemented: analyze fails loudly naming the table. Spec for a future
implementation: `.planning/research/52-prx-format-spec.md` §5.

### Verified (2026-06-12, .hack//Link BOOT_DEC.BIN + Patapon regression)

.hack//Link (6,217,020 bytes, decrypted): 188,284 Type-A relocations applied from 7 section
tables (0 skipped / 0 unhandled), module `hacklink`, gp 0x08C89BD0, 243 import stubs across
28 libraries (`sceDmacMemcpy` 0x617F3FE6 resolves; 9 game-specific NIDs surface in
`unresolved_nids`), entry renamed to `entry` at 0x08BC3D98, heap 0x08D30000, byte gate
12/12 blocks. Recompile: 22,170 functions / 3,439 mid-entries / 444 batches; **all** batch
files pass `clang++ -std=c++17 -fsyntax-only`. Patapon: fresh analyze output byte-identical
to a master-built run (modulo `binary_path`); gate 12/12; recompile baseline
(14,104/2,022/283 under `PSPRECOMP_CROSS_MID=1`) unchanged.

## #57 — byte-reproducible recompile output (emitter-change byte-diff gate)

Two recompiles of the same analysis.json with the same binary produce a byte-identical
output directory — `diff -r out1 out2` shows ONLY the three timestamp lines
(`generated_at` in fingerprint.json + recompile_report.json, `RECOMP_FINGERPRINT_TIMESTAMP`
in include/recomp_fingerprint.h). This holds under `PSPRECOMP_CROSS_MID=1` too: the
coalesce pass registers absorbed-sibling mid-entries sorted by address (it previously
iterated a HashMap, scrambling the wrapper order in mid_entries.cpp / funcs.h /
dispatch.cpp on every run). Byte-diff against a master-built output is therefore a valid
emitter regression gate with no sort-normalization needed; any other residual diff is a
real behavior change. Regression test:
`coalesce_mid_entry_registration_is_deterministic_and_sorted` (crates/psp-cli/src/recompile.rs).

## LwMutex HLE (runtime/src/hle/psp_hle_kernel_lwmutex.cpp)

`sceKernelCreate/Delete/Lock/LockCB/TryLock/UnlockLwMutex`, PPSSPP-faithful
(`Core/HLE/sceKernelMutex.cpp`). The part that bites when debugging: **the LwMutex's
state is GUEST-visible** — a 32-byte workarea the game allocated (`+0` lockLevel,
`+4` lockThread = the `sceKernelGetThreadId` UID of the owner, `+8` attr,
`+16` kernel-object uid; `lockThread` 0 = free, −1 = deleted; `uid` −1 = invalid).
Read it live over the debug socket to see lock state — there is no host-side mirror to
trust; every HLE decision derives from those words. The host object exists only to block
waiters (one global host mutex serializes all transitions; per-object condvar; 5 s
safety valve + `g_should_exit` escape; `wait` tag `lwmutex:<uid>` in the socket `I`
thread dump). `Create` returns **0**, not the uid (the uid lives in the workarea — a
game comparing v0 against the uid is misreading the API, not hitting a runtime bug).
Recursion requires attr `0x200`; a second lock without it returns
`SCE_LWMUTEX_ERROR_ALREADY_LOCKED` (0x800201CF), exactly like PPSSPP. Patapon imports
no LwMutex NIDs (0/237 — binding is a no-op there); .hack//Link imports
Create/Delete/Lock/Unlock and exercises them at ~430 calls/s from the CRI mixer.
