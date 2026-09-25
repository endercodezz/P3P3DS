# psprecomp

### *Because the Allegrex deserves a native second life*

> Static recompilation toolkit for PlayStation Portable titles.
> Turn PSP binaries into native executables. No emulator required.

---

## What Is This?

**psprecomp** is an open-source toolkit providing the analysis tools, code
generator, and runtime libraries needed to **statically recompile PSP games
into native executables**.

Instead of interpreting or JIT-ing Allegrex MIPS at runtime (what PPSSPP does
brilliantly), we take the opposite approach: **translate everything ahead of
time** into C that compiles with any modern compiler on any platform.

This is the same philosophy behind:
- [N64Recomp](https://github.com/N64Recomp/N64Recomp) (N64 → native)
- [UnleashedRecomp](https://github.com/hedge-dev/UnleashedRecomp) (Xbox 360 → native)
- [PS2Recomp](https://github.com/ran-j/PS2Recomp) (PS2 → native)
- [ps3recomp](https://github.com/sp00nznet/ps3recomp) (PS3 → native)

...and for the PSP, where
[sal063/PSP-recompilation-project](https://github.com/sal063/PSP-recompilation-project)
got there first — same idea, offline MIPS-to-C plus a native runtime, with an
SDL3/Vulkan backend. Worth reading if you are interested in this problem.

The two differ mainly in licensing posture: that project ports GPL code from
PPSSPP for parts of its HLE, while psprecomp keeps a hard MIT boundary and uses
emulators only as an oracle to diff against (see
[docs/ORACLE.md](docs/ORACLE.md)). If you want a permissively-licensed base to
build on, that is the distinction that matters.

## Why PSP?

The PSP has excellent emulation and, until recently, essentially no static
recompilation work. It is a genuinely *good* recompilation target hiding behind
a reputation for being a hard one:

- **One documented CPU.** The Allegrex is MIPS32r2. No second processor, no SPU
  swarm, no exotic addressing. MIPS is the best-understood RISC there is, and
  it is what the original static-recomp work was built on.
- **No TLB.** The memory map is fixed and small — scratchpad, VRAM, main RAM,
  each mirrored at three cache-behaviour bases. Address translation is a mask
  and an index, not a page walk.
- **A documented OS boundary.** Games call `sceKernel*` / `sceGu*` / `sceCtrl*`.
  That is a *library* surface, not a hardware surface — it can be implemented as
  HLE C rather than emulated, and a complete MIT-licensed reverse engineering of
  that firmware already exists.

Plus the usual static-recomp payoffs: native performance, no runtime translation
overhead, moddable C, and ports that survive long after emulators stop being
maintained.

## The Challenge

| Component | What It Is | Why It's Hard |
|-----------|-----------|---------------|
| **Allegrex** | MIPS32r2 main CPU | Sony extensions (`bitrev`, `wsbw`, `max`/`min`), branch delay slots, likely-branch nullification |
| **VFPU** | 128-register vector unit on COP2 | Prefix instructions that rewrite the *next* instruction's operands; matrix-shaped register aliasing |
| **GE** | Display-list GPU | Not called — *fed*. A producer/consumer command stream with its own control flow |
| **Container stack** | ISO9660 → PBP → `~PSP` → PRX | Four nested formats before you reach an ELF, the innermost KIRK-encrypted |
| **Firmware** | `sceKernel*`, `sceGu*`, `sceCtrl*` | Hundreds of NID-addressed entry points, several with non-standard calling conventions |

We don't shy away from hard problems. We just break them into smaller ones.

## Architecture

```
psprecomp/
├── tools/allegrexrecomp/     # Analysis & recompilation pipeline
│   ├── container.c           # ISO9660 / PBP / ~PSP / ~SCE / ELF-PRX peeling
│   ├── keys.c                # KIRK AES-CBC decrypt, per-tag key selection
│   ├── decode.c              # Allegrex decoder: MIPS32r2 + COP1 + VFPU
│   ├── analyze.c             # Function discovery (fixpoint; jump tables, exports)
│   ├── emit.c                # Allegrex → readable C, one psp_func_<addr> each
│   └── main.c                # info / ls / extract / dis / cover / funcs / emit
│
├── src/                      # Runtime the generated C links against
│   ├── cpu.c                 # Register file, HI/LO, semantic helpers
│   ├── mem.c                 # Memory map, cache-mirror collapse, access auditing
│   ├── dispatch.c            # Address → function table, tracing, stack auditing
│   ├── vfpu.c                # Vector unit: arithmetic, matrix ops, prefixes
│   ├── ctors.c               # Static-constructor discovery and execution
│   └── hle/                  # Firmware, by module
│       ├── sysmem.c          # sceKernelAllocPartitionMemory, partitions, blocks
│       ├── threadman.c       # Threads, semaphores, event flags, callbacks
│       ├── ge.c              # Display-list execution + rasterizer
│       ├── display.c         # Framebuffer, vblank, capture
│       ├── sascore.c         # sceSasCore audio synthesis
│       ├── iofilemgr.c       # sceIo* file I/O
│       └── misc.c            # sceCtrl, ModuleMgr, LoadExec, Kernel_Library
│
├── include/psprecomp/        # Public API
│   ├── cpu.h  mem.h          # Execution context and memory
│   ├── dispatch.h            # Dispatch table + the bring-up instruments
│   ├── hle.h                 # NID registration, argument/return convention
│   ├── vfpu.h  ctors.h       # Vector unit, static constructors
│
├── tests/                    # 10 self-contained suites, no game data
└── docs/                     # See Documentation below
```

## How It Works

```
  game.iso ──► allegrexrecomp ──► EBOOT.BIN / DATA.PSP     peel the container
              (ISO9660 · PBP)      (a ~PSP-wrapped PRX)     stack down to a module
                  │
                  ▼
          ┌────────────────────┐   KIRK AES + the published per-tag key set;
          │  decrypt           │   ~PSP  ──►  a plain ELF32 MIPS PRX
          └────────────────────┘
                  │
                  ▼
          ┌────────────────────┐   decode Allegrex, discover functions
          │  allegrexrecomp    │   (jump tables + the module's export table),
          │  (tools/)          │   translate each to readable C
          └────────────────────┘
                  │  <prefix>_funcs.c
                  ▼
          ┌────────────────────┐   CPU state · memory map · semantic helpers ·
          │  psprecomp (lib)   │   sceKernel/sceGu HLE · dispatch · rasterizer
          │  (src/, include/)  │
          └────────────────────┘
                  │
                  ▼   (+ a per-game host: load module, register, run, present)
          native executable
```

Every generated function is named for its address and carries the original
disassembly as comments, so the output is readable and diffable against the
binary it came from:

```c
/* ---------------------------------------------------------------
 * psp_func_0000E06C  --  16 instructions, 64 bytes
 * ------------------------------------------------------------- */
static void psp_body_0000E06C(uint32_t _entry) {
    /* 0000E06C  addiu      $sp, $sp, -16 */
    r_sp = r_sp + -16;
    /* 0000E070  sw         $ra, 8($sp) */
    psp_write32(r_sp + 8, r_ra);
    ...
```

## Status

**A retail PSP game recompiles to C that compiles, links, and executes.**
It does not yet render. Being specific about which parts are real:

| Area | State | Detail |
|------|-------|--------|
| **Container stack** | ✅ Complete | ISO9660, PBP, `~PSP`/`~SCE`, ELF/PRX — `info` recurses the whole stack |
| **Decryption** | ✅ Complete | KIRK AES-CBC with two CMACs; key tag read from the right offset, verified against published tooling |
| **Allegrex decoder** | ✅ Complete | MIPS32r2 + Sony extensions + COP1 + VFPU, including the prefix instructions |
| **Function discovery** | ✅ Working | Fixpoint walk over exports, relocations and jump tables — 5,849 functions in the flagship module, 92% of `.text` |
| **C emitter** | ✅ Working | Every discovered function, every label individually dispatchable (13,682 entries) — computed jumps resolve by construction |
| **Runtime** | ✅ Working | Register file, memory map with cache mirrors, dispatch, static constructors |
| **VFPU** | 🔨 Partial | Arithmetic, matrix ops, constants, immediates, prefixes. 108 of 137,748 instructions still trap (0.08%) |
| **HLE firmware** | 🔨 Partial | 112 registrations across sysmem, threadman, GE, display, sascore, io, ctrl. Every NID verified against SHA-1 of its own name |
| **GE rasterizer** | 🔨 Partial | Triangles, strips, sprites in through-mode; six tests assert *where* pixels land. Transformed geometry counted, not drawn |
| **Threading** | ❌ Not started | Run-to-completion only; no scheduler |
| **A game on screen** | ❌ Not yet | The flagship reaches its allocator and stalls in start-up. See [BRINGUP.md](docs/BRINGUP.md) |

### Bring-up instruments

Static recompilation fails quietly: a wrong value is usually *plausible*, not
obviously broken. The toolkit therefore ships the instruments that catch that,
because each one found a bug that reasoning had missed:

| Instrument | Catches |
|---|---|
| Function-entry tracing | Where execution actually went, vs where you assumed |
| Loop back-edge recording | Spins inside a single function, invisible to any call-boundary counter |
| Label-level reachability | Whether a specific block ran — for any of the 13,682 labelled addresses |
| Memory write watch | *Which code* writes a given word |
| Stack-balance checking | A callee that consumes stack and never returns it — corrupts every callee-saved restore afterwards |
| Wall-clock watchdog | Reports statistics from a hung run instead of losing them |

Two real codegen bugs were found this way, both affecting *every* recompiled
program, and both producing plausible values rather than visible failures:

- **`$ra` was never assigned by `jal`/`jalr`** — 2 assignments existed across
  137,748 instructions where 9,814 belong. Every non-leaf function returned
  through a stale register.
- **`psp_arg` read arguments 5–8 from the stack.** PSP firmware passes them in
  `$t0`–`$t3`. The wrong read turned a valid 4096-byte alignment into garbage,
  so a correct 15.9 MB allocation was rejected — and every downstream symptom
  followed from that.

## Documentation

| Document | What It Covers |
|----------|---------------|
| **[Architecture](docs/ARCHITECTURE.md)** | Allegrex overview, pipeline stages, memory model |
| **[Containers](docs/CONTAINERS.md)** | ISO9660 → PBP → `~PSP` → PRX, and how to identify each |
| **[Decryption](docs/DECRYPT.md)** | KIRK, key tags, and supplying your own key material |
| **[Recompiler](docs/RECOMPILER.md)** | Discovery, emission, delay slots, jump tables |
| **[VFPU](docs/VFPU.md)** | Register aliasing, prefix instructions, matrix layout |
| **[Oracle](docs/ORACLE.md)** | How emulators are used as references without being linked |
| **[Bring-up log](docs/BRINGUP.md)** | The full investigation record — every finding, **and every retraction** |

`BRINGUP.md` is worth singling out. It records the wrong turns as prominently
as the results, because in this domain the wrong turns are the transferable
part: seven conclusions in one session were withdrawn after measurement
contradicted them, and every one came from an instrument adopted before it was
verified.

## Getting Started

Requires CMake and a C compiler (MSVC on Windows; gcc/clang elsewhere). The core
has **no external dependencies** — a fresh clone builds with nothing installed.

```bash
git clone https://github.com/sp00nznet/psprecomp
cd psprecomp
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
```

```
allegrexrecomp info    <file>                    identify any layer of the stack
allegrexrecomp ls      <file.iso>                list a disc image
allegrexrecomp extract <file.iso> <match> <dir>  pull files out of a disc image
allegrexrecomp dis     <file> [addr] [count]     disassemble Allegrex code
allegrexrecomp cover   <file>                    decode-coverage report
allegrexrecomp funcs   <file> [--list]           function discovery report
allegrexrecomp emit    <file> <outdir> [prefix]  generate C
allegrexrecomp decrypt <file> [--keys <path>]    peel a ~PSP module
```

`info` accepts anything in the stack — a disc image, a PBP, a `~PSP` or `~SCE`
module, a plain ELF/PRX — and recurses one level, so pointing it at a
game-sharing PBP reports the inner module's name, size and key tag in one go.

> **No game data here.** Disc images, `EBOOT.BIN`, PRX modules and PSP
> decryption keys are all `.gitignore`d. This repo is the recompiler, the
> runtime, and the docs — bring your own UMD or PSN dump.

## Game Ports Using psprecomp

The toolkit is separate from the games brought up on it; each game lives in its
own repo consuming this one as a submodule. That split is deliberate — the
toolkit is the thing you fork to recompile *your* PSP game.

- [**wtf-psp-recomp**](https://github.com/sp00nznet/wtf-psp-recomp) —
  *WTF: Work Time Fun*. An anthology of ~30 microgames, five of which ship as
  separately-bootable modules, which makes it an unusually good bring-up target:
  each module is a small, self-contained program.

## Relationship to Other Projects

There is no MIT-licensed PSP emulator. PPSSPP is GPLv2+, JPCSP is GPL-3.0.
Neither can be linked into an MIT toolkit, and neither needs to be:

| Project | License | How it is used |
|---|---|---|
| [uofw](https://github.com/uofw/uofw) | **MIT** | A complete reverse engineering of the PSP firmware in readable C. The reference for the HLE layer. |
| [pspsdk](https://github.com/pspdev/pspsdk) | **BSD** | Headers and the published ABI for `sceGu` / `sceKernel` / `sceAudio`. (`tools/PrxEncrypter` is GPL-3.0 and excluded.) |
| **PPSSPP** | GPLv2+ | **Oracle only** — run as a separate process and compared against. No code copied, linked, or vendored. |
| [pspautotests](https://github.com/hrydgard/pspautotests) | — | A hardware-validated behavioural corpus to check the runtime against. |

Same arrangement `ps3recomp` has with RPCS3 and `lynxrecomp` has with Handy: the
emulator is a thing you *diff against*, never a thing you link. See
[docs/ORACLE.md](docs/ORACLE.md).

## Legal

This project contains no Sony code, no firmware images, and no decryption keys.
It is a clean-room toolkit built from published documentation and MIT/BSD
reverse-engineering work. Recompiling a game requires a dump you made from
media you own.

## License

MIT. See [LICENSE](LICENSE).
