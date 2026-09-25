# Roadmap

Phased plan. Phase 1 is done; phase 2 is the gate everything else waits behind.

## Phase 1 — the container stack and the decoder ✅

Getting from a disc dump to *something you can point a decoder at*, and a
decoder correct enough to trust when you do.

- [x] Two-repo split: `psprecomp` (toolkit) + `wtf-psp-recomp` (reference game,
      submodules the toolkit).
- [x] **ISO9660 walker** — full recursive directory tree, version-suffix
      stripping, sector-padding handling, extraction by substring match with
      path flattening (so `SYSDIR/EBOOT.BIN` and `SYSDIR/UPDATE/EBOOT.BIN` do
      not collide).
- [x] **PBP parser** — the 8-segment container, sizes derived from adjacent
      offsets, one-level recursion into `DATA.PSP`.
- [x] **`~PSP` and `~SCE` header parsers** — module name, segment table, entry
      point, decrypted-ELF size, decrypt mode, and the key tag.
- [x] **ELF32/PRX parser** — program headers, `PT_LOAD` extraction, `.text`
      located by `PF_X`, `e_type == 0xFFA0` recognised as a relocatable PRX.
- [x] **PARAM.SFO parser** — the disc/module identity (`DISC_ID`, `TITLE`,
      `PSP_SYSTEM_VER`).
- [x] **Complete Allegrex decoder** — the MIPS32r2 integer set, REGIMM,
      SPECIAL2/SPECIAL3, the single-precision COP1 FPU, and every Allegrex
      divergence from stock MIPS32 (`clz`/`clo` in SPECIAL 0x16/0x17,
      `max`/`min` at 0x2C/0x2D, `madd`/`msub` in SPECIAL, `bitrev`/`wsbw` in
      BSHFL, `rotr` as `srl` with `rs=1`, `mfic`/`mtic` in COP0).
- [x] **Analysis flags on every instruction** — branch vs jump vs call,
      indirect, `jr $ra` as a return specifically, likely-branch nullification,
      delay slots, block termination. This is what phase 3's discovery pass
      consumes.
- [x] **VFPU recognised as a distinct category** rather than silently invalid,
      with the load/store and vadd/vmul/vcmp families named.
- [x] **Runtime semantic helpers** (`recomp_rt.h`) — division edge cases, the
      unaligned load/store pairs in little-endian form, `ext`/`ins`, `clz`,
      `bitrev`, `wsbh`/`wsbw`, shift-amount masking.
- [x] **Memory map** — scratchpad/VRAM/RAM with cache mirrors folded, straddling
      accesses rejected, unmapped accesses counted.
- [x] **`info` / `ls` / `extract` / `dis` / `cover` CLI.**
- [x] **`ctest` suites** — decoder, runtime, containers. All synthetic.
- [x] **Validated against a retail disc** — *WTF: Work Time Fun* (ULUS10172):
      90 entries, correct SFO, the full boot chain, and all five game-sharing
      modules with their per-module key tags.
- [x] Build system (CMake, zero external dependencies), docs, MIT license.

## Phase 2a — the crypto core and KIRK CMD1 ✅

Every executable module on a retail disc is a `~PSP`-wrapped, KIRK-encrypted
PRX, and `BOOT.BIN` is zeroed. See [`docs/DECRYPT.md`](docs/DECRYPT.md).

- [x] **AES-128/192/256** (`crypto/aes.c`) — self-contained, no dependency. The
      S-box is derived from its algebraic definition rather than transcribed,
      so it is either completely right or obviously wrong.
- [x] **AES-CMAC** (`crypto/cmac.c`), RFC 4493.
- [x] **Validated against the primary standards** before touching a game
      module: FIPS-197 Appendix C (all three key sizes, plus the inverse
      cipher), NIST SP 800-38A F.2 (CBC, including in-place operation and a
      zero IV), RFC 4493 (all four CMAC examples, covering both padding
      branches). `ctest` target `crypto`.
- [x] **KIRK CMD1** (`crypto/kirk.c`) — key unwrapping, both CMAC checks, body
      decryption. Handles nonzero header/body padding and unaligned data
      lengths, and bounds-checks every size field so a corrupt module cannot
      read out of bounds.
- [x] **Verified end to end with synthetic blobs** (`ctest` target `kirk`) —
      round trip, padding, unaligned lengths, wrong key caught at the *header*
      CMAC, tampered body caught at the *data* CMAC, and every malformed-input
      path. No PSP key material is needed for any of it, because CMD1's
      structure does not depend on which key it is given.
- [x] **External key loading** (`keys.c`) — `--keys`, `$PSPRECOMP_KEYS`, or
      `./keys/psp_keys.txt`. No key material is bundled; a missing or
      wrong-length key produces a named diagnostic.
- [x] `allegrexrecomp kirk1` (decrypt a raw CMD1 blob) and
      `allegrexrecomp decrypt` (identify a module and probe it).

## Phase 2b — the `~PSP` tag layer

Above CMD1 sits a per-tag transform that *builds* the CMD1 header.

Established empirically rather than assumed: `decrypt` scans for an embedded
CMD1 metadata block using a ~100-bit structural signature, and finds none in
any module on the WTF disc. So the CMD1 header is constructed by the tag layer,
not sitting in the `~PSP` header at a fixed offset. Guessing that offset would
have produced silently-wrong output.

Also established: WTF's main executable is **decrypt mode 9** while all five
game-sharing modules are **mode 10**, so at least two variants are in play.

- [ ] The tag → key-set table, from the external key file.
- [ ] The mode 9 and mode 10 transforms that build a CMD1 header from a `~PSP`
      header. Needs the reference algorithm — approximating it would produce
      plausible garbage that the CMACs would reject without saying which step
      was wrong.
- [ ] **`~PSP` → ELF** — reassemble decrypted segments into a valid ELF32 using
      the header's segment table.
- [ ] **Cross-check**: output must be exactly `elf_size` bytes, parse with the
      phase-1 ELF parser, have its entry point land in a `PF_X` segment, and
      show a code-shaped opcode histogram under `cover` rather than a flat one.
- [ ] `info`/`dis`/`cover` decrypting transparently, so the rest of the tool
      stops caring about the wrapper.

**Not on the critical path.** Phase 3 needs *plaintext*, not necessarily *our*
decryptor — [`pspdecrypt`](https://github.com/John-K/pspdecrypt) (GPL-3.0) run
as an external process produces it today, exactly like PPSSPP-as-oracle. 2b is
about being self-contained, which is a goal but not a gate.

## Phase 3 — function discovery + the C emitter

- [x] **ELF section parsing** — `.text` gives the true code extent. A PSP PRX
      has one rwx `PT_LOAD` covering code *and* data, so decoding the segment
      measures 3.3 MB of data as instructions. Using `.text` moved decode
      coverage on Lumberjack from 70.8% to **99.81%, with 0% unknown**.
- [x] **Module info + export table** — layout corrected against a real module
      (`attribute` is a u16 and `name` starts at 0x04; the widely-circulated
      struct shifts every field by two). Verified by cross-checking the parsed
      export/import ranges against the `.lib.ent` / `.lib.stub` section
      addresses.
- [x] **Recursive-descent discovery** — seeds from the entry point and exports,
      follows calls and branches, carves at `jr $ra`, classifies calls into
      `.sceStub.text` as HLE imports rather than functions.
- [x] **`jal` harvesting by linear scan** — required, not optional: PSP
      `module_start` passes the real entry point to a thread as a *pointer*, so
      recursive descent alone finds 47 instructions. 1 function -> 2206.
- [x] **Tail-call disambiguation** — a full entry map built before walking, so
      a forward `j` is recognised as a tail call and a walk stops when it runs
      into the next function's entry. Without it one trace swallowed 121 KB.
- [x] **Honest metrics** — code size (instructions actually visited) reported
      separately from extent (entry to furthest address), with divergence
      counted. This is what made the tail-call bug visible.
- [x] **Validated across six real modules** — 2206-3410 functions each, ~75% of
      `.text` reached, **zero invalid instructions**, VFPU measured at 0.14-0.20%.
- [x] **Relocation mining** — R_MIPS_32 relocations name every word holding an
      address, which is exactly the set of stored function pointers (thread
      entries, callbacks, vtables) that no control-flow scan can see. This is
      enumeration, not guesswork: the linker recorded them because they *are*
      addresses. **75% -> 89% of `.text`** on the microgames, 2633 pointer
      seeds on Lumberjack alone.
      A PSP PRX tags these sections `SHT_PRXRELOC` (`0x700000A0`) rather than
      `SHT_REL` (9); checking only for the generic type finds nothing at all.
- [x] **Data-pointer scanning for static modules** — a statically linked
      module (`ET_EXEC`, which is what `EBOOT.BIN` is) has *empty* relocation
      sections, because absolute addresses need no patching. The fallback is to
      recognise pointers by shape: in the code extent, instruction-aligned, and
      pointing at something that decodes. Deliberately kept separate and named
      as a **heuristic** rather than enumeration. **70% -> 88%** on `hell2k`.
- [ ] **Jump-table resolution** — the `lui`/`addiu`/`sll`/`addu`/`lw`/`jr`
      idiom that MIPS compilers emit for `switch`. 38 unresolved sites in
      Lumberjack. Unresolved tables become dispatch-table lookups rather than
      analysis failures.
- [x] **The delay-slot problem.** Ordinary branches capture their condition
      into a temporary before the slot runs (so a slot that writes a compared
      register cannot flip the branch); likely branches duplicate the slot into
      the taken path; jumps, calls and returns hoist it. Asserted by
      `tests/test_emit.c`.
- [x] **The C emitter** — one `psp_func_<addr>` per routine, every line carrying
      its address and disassembly, lowered to `recomp_rt.h` helpers, flow as
      labels + `goto`, `jal` as a direct call, imports as named stubs, indirect
      jumps through the dispatch table. Untranslated instructions emit named
      run-time traps rather than silence.
- [x] **Dispatch table** (`src/dispatch.c`) — open-addressed `addr -> psp_fn_t`,
      with a miss handler that names the address instead of crashing.
- [x] **Proven end to end on a real game.** Lumberjack emits 2206 functions /
      256,566 lines, which compile with MSVC, link against the runtime, and run:
      2212 functions registered and `module_start` resolves through the table.
      Two edge cases the real module forced, each producing exactly one compile
      error in a quarter-million lines: a delay slot that is also a branch
      target (must be emitted twice), and a function owning instructions
      *below* its entry point (so functions carry a `start` as well as an `end`).
- [ ] **Hints file** — per-title force-code/force-data, function names, and
      HLE overrides, so a title's hard-won analysis is data rather than a patch.

## Phase 4 — the HLE library

Games do not touch hardware directly; they call the firmware. That surface is a
library, which means it is implemented, not emulated.

- [ ] **`sceKernel`** — threads, semaphores, event flags, mutexes, callbacks,
      the memory partitions, timers. Modelled on the MIT-licensed `uofw`
      reference implementation.
- [ ] **`sceIo`** — the file API over a host directory standing in for the UMD.
- [ ] **`sceCtrl`** — the pad, mapped to host input.
- [ ] **`sceDisplay` + `sceGe`** — the display list processor. The GE is a
      fixed-function GPU with a documented command stream; it is emulated as a
      peripheral, the same split as the Lynx's Suzy/Mikey.
- [ ] **`sceAudio` / `sceSas`** — PCM out and the hardware voice mixer.
- [ ] **Module import resolution** — a PRX's import stubs bound to HLE
      implementations at load time, with an unimplemented-call trap that names
      the missing function instead of crashing.

## Phase 5 — running the recompiled C

- [ ] **Dispatch table** — `addr -> psp_fn_t`, for indirect calls and jump
      tables. Static `jal` stays a direct C call.
- [ ] **The interpreter oracle** — an Allegrex interpreter sharing this repo's
      decoder and runtime, so the recompiled path can be diffed against it
      instruction-for-instruction. When they disagree, the bug is in exactly
      one of them. This is the bring-up tool that everything else leans on.
- [ ] **PPSSPP cross-check** — trace comparison against the external oracle at
      the syscall and frame level. See [`docs/ORACLE.md`](docs/ORACLE.md).
- [ ] **First pixels** — a recompiled module reaching a rendered frame.

## Phase 6 — corpus and player-facing layer

- [ ] `scripts/sweep` over a large PSP corpus as a correctness harness:
      decrypt-all, decode-all, report coverage. Every module that fails is a
      concrete decoder or container bug, and the aggregate VFPU percentage
      tells us which titles are cheap targets.
- [ ] VFPU completion, driven by what the corpus says is actually used.
- [ ] Save states, an SDL2 + Dear ImGui frontend, controller remapping —
      built on the runtime, kept behind an opt-in CMake option so the core
      stays dependency-free.
