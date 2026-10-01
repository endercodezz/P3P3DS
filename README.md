# P3P3DS

**P3P3DS** is an independent research and engineering project working toward running *Shin Megami Tensei: Persona 3 Portable* (`ULUS-10512`, North American release) natively on the **New Nintendo 3DS / New 3DS XL / New 2DS XL** family of consoles.

---

## Status: NOT PLAYABLE (boots on New 3DS at 1–2 fps; reaches the first school day on PC)

> **IMPORTANT:** The game is **not playable** on PC or Nintendo 3DS. On the PC development runner it boots, shows its logos, plays the opening movie (as black frames: there is no video decoder yet), reaches the title screen with audio and, driven by a scripted controller input, starts a New Game and plays through name entry, the first night in the dorm and the walk to school, where the navigation cursor responds to input. Input comes from deterministic scripts (or an XInput gamepad); there is no window yet, so frames are written as images. On **New 3DS** the `.3dsx` build starts and runs the opening — ATLUS and CRIWARE logos on the top screen, a live debug report on the bottom screen — but only at **1–2 fps** (about 5 % of full speed), with no sound: graphics are still drawn by the CPU.

On New 3DS (details in [`docs/3DS_PLATFORM.md`](docs/3DS_PLATFORM.md) sections 8.3 and 8.5):

| Milestone | Result |
|---|---|
| `.3dsx` build | devkitARM, 44 MB of recompiled ARM code (cut from 77 MB), clean build in about 9 minutes |
| Boot | runs the logos; the opening movie is black, as on PC (no video decoder yet) |
| Speed | 1–2 fps, about 5 % of real time; 91 % of the time goes to the CPU software renderer, the game's own code takes a few percent |
| Memory | 59 MB of a 79 MB heap in use during the opening |
| Screens | top: the game; bottom: authorship and debug data (fps, speed, time split, memory), also saved to `sdmc:/p3p3ds/report.txt` |

Measured on the PC runner (ULUS-10512, details and evidence in [`docs/CURRENT_STATE.md`](docs/CURRENT_STATE.md)):

| Milestone | Result |
|---|---|
| Whole `.text` statically recompiled | 237 C++ units, 751,453 instruction PCs; leftovers run through an interpreter fallback (0 mismatches vs AOT on 1,200 differential cases) |
| Boot | ThreadMan, SysMem, IoFileMgr (UMD ISO + memory stick + the community mod chain), ModuleMgr, UMD, Display services drive the game through CRI middleware startup |
| First rendered frames | ATLUS and CRIWARE logos drawn by the GE display-list executor + software renderer |
| Opening movie | 100 s PSMF movie demultiplexed by `sceMpeg` (on the pspautotests sample movie, container behaviour matches PSP hardware output line for line); picture and movie audio are placeholders (black / silence) |
| Title screen | "PRESS ANY BUTTON" at frame 3600 (~133 s virtual time), title music recorded to WAV |
| Main menu and New Game | START opens NEW GAME / LOAD GAME / CONFIG / DATA INSTALL; NEW GAME loads the protagonist selection scene ("Welcome to the world of P3P.") |
| First game day | Dark Hour dorm lobby (3D), name entry, Mitsuru introduction, own room with the navigation cursor, next morning with Yukari, school main lobby; 38 virtual minutes without a blocker |
| Stability | 38,314 frames (24 min virtual) of the attract loop without a blocker; runs with the same input script are bit-for-bit deterministic |
| Tests | CTest suites for HLE contracts, renderer, input and AOT/interpreter differential, several replaying pspautotests hardware transcripts |

---

## Progress Checklist

- [x] Analyze P3P executable structure (ELF32 PRX, segments, sections)
- [x] Full PRX relocation and library import table extraction (178,513 relocations, 221 import stubs)
- [x] Independent relocation-aware validation tooling matching PSPRecomp 100%
- [x] Whole-`.text` Ahead-of-Time (AOT) MIPS-to-C++ generation in fixed 16 KiB units, with an interpreter fallback
- [x] Core PSP kernel services (SysMem, ThreadMan with waits/callbacks/virtual time, UtilsForUser, ModuleMgr)
- [x] Virtual File System: UMD ISO9660, memory stick, CWCheat "Mod Support" chain (`bind/` → `mod.cpk` → `mod1-3.cpk` → original CPKs)
- [x] GE display-list execution with a reference software renderer (first visible frames, title screen)
- [x] `sceAudio`, `sceSasCore` and PCM output to WAV on PC
- [x] `sceMpeg` container/ringbuffer behaviour (PSMF demux)
- [ ] H.264 / ATRAC3plus decoding for the opening movie
- [x] Controller input: scripted (deterministic) and XInput gamepad
- [ ] Live window and audio device on PC
- [x] Main menu and New Game start
- [x] Name entry, introduction and first controllable section (scripted input)
- [ ] Complete renderer (filtering, lines, lighting/skinning as the game needs) and movie decoding
- [x] New Nintendo 3DS homebrew build (`.3dsx`) that boots (1–2 fps)
- [ ] PICA200 (citro3d) renderer for playable speed on New 3DS
- [ ] ndsp audio, live controls tested and `.cia` for New 3DS
- [ ] Playable game on New Nintendo 3DS hardware

---

## Goal

The long-term goal of this project is a smooth, high-fidelity experience of Persona 3 Portable running natively on New Nintendo 3DS hardware, with support for user-supplied community mods and fan translations.

We are **not** building a general-purpose PSP emulator, nor an all-encompassing PPSSPP replacement. The scope is specifically tailored to Persona 3 Portable.

---

## Why P3P?

*Persona 3 Portable* is uniquely well-suited for a static recompilation port to the New Nintendo 3DS:
- **Visual Novel Navigation:** The 2D exploration and dialogue format of P3P maps naturally to dual-screen handheld hardware.
- **Relatively Low VFPU Density:** Only ~0.31% of the instruction stream uses PSP VFPU vector instructions (compared to heavily math-intensive titles like Monster Hunter or racing games).
- **Atlus Script Engine:** Story flow and cutscene logic run via an internal bytecode engine (`.bf`), meaning gameplay scripting is isolated from low-level MIPS machine code.
- **No Evidence of Self-Modifying Code Observed So Far `[UNVERIFIED]`:** Static analysis shows standard relocatable code with clean function prologues/epilogues; runtime verification across deeper gameplay loops remains ongoing.

---

## Current Architecture

The project employs **Hybrid Static Recompilation (AOT)**:

```text
Decrypted P3P Executable (Allegrex MIPS ELF) + community CWCheat patches
                    │
                    ▼
     Offline Static Recompiler (recomp/PSPRecomp, psp_recomp --auto)
  (lowers all of .text to 237 C++ translation units of 16 KiB each)
                    │
                    ▼
       Native Host C++ Compilation
     (GCC on PC, devkitARM GCC on New 3DS)
                    │
                    ▼
          P3P3DS Runtime (core/)
  ├── Interpreter fallback for PCs without an AOT entry
  ├── PSP HLE kernel: SysMem, ThreadMan (deterministic virtual clock),
  │   IoFileMgr, ModuleMgr, UMD, Display, Ctrl, Audio, SasCore, Mpeg, ...
  ├── Virtual File System (UMD ISO9660 / host directories, mod overlay)
  ├── GE display-list executor → GeRenderer interface
  │     └── SoftwareRenderer (reference backend, draws into guest VRAM)
  └── Platform backends:
       ├── platform/pc  — development runner: frame dumps, WAV output, event traces
       └── platform/3ds — New 3DS runner (.3dsx): top screen game, bottom screen
                          debug report; Citro3D renderer + NDSP audio planned
```

1. **Game Machine Code:** Recompiled offline into native C++ translation units; PCs the static analysis missed are executed by an interpreter whose semantics mirror the code generator (checked by a differential test).
2. **PSP Kernel & Services:** A lightweight High-Level Emulation (HLE) runtime implementing only what P3P imports, with contracts taken from uOFW, PSPSDK and pspautotests hardware output.
3. **Graphics Engine (GE):** Display lists are executed in `core/`; a renderer interface lets the PC reference rasterizer and a future PICA200 (`citro3d`) backend share the same command processing.
4. **Audio:** Guest PCM (`sceAudio`, `sceSasCore`, CRI middleware) is mixed on the virtual clock; on 3DS it is intended for the hardware DSP (`ndsp`).

---

## New 3DS Target Constraints

This project targets the **New Nintendo 3DS / New 3DS XL / New 2DS XL** exclusively (Old 3DS / 2DS is intentionally unsupported):
- **CPU:** Quad-core ARM11 MPCore @ 804 MHz with L2 cache enabled via `osSetSpeedupEnable(true)` (vs 268 MHz on Old 3DS).
- **RAM:** 256 MB FCRAM (vs 128 MB on Old 3DS). The `.3dsx` currently holds 44 MB of recompiled code plus the 32 MB guest RAM and the runtime; a `.cia` can request a larger memory mode.
- **Worker Core:** Core 2 is available for multithreaded worker tasks (display list processing, audio mixing, or file streaming).

---

## Dual-Screen Presentation (Future Design Goal)

While keeping classic 1:1 PSP presentation intact, the architecture is designed to support enhanced dual-screen modes:
- **Top Screen (400x240):** 3D Tartarus exploration, battle scenes, and animated event sequences.
- **Bottom Screen (320x240):** Dialogue boxes, choice selections, minimaps, party status, and touch-screen shortcuts.

---

## Modding & Localization Architecture (Design Goals)

Modding support and fan translations are first-class architectural requirements:
- **Language-Agnostic Core:** No language strings, fonts, or specific localization hacks will be hardcoded into the runtime engine.
- **VFS Fallback Pipeline:** P3P on PSP loads mods through the community CWCheat "Mod Support" patch (`p3p/p3p-patches`), which the recompiler applies to the executable. The runtime serves the paths it opens, mapped to the SD card on 3DS:
  ```text
  sdmc:/p3p3ds/mods/bind/<relative_path>   (loose file overrides, ms0:/PSP/P3P/bind/)
    ↓
  sdmc:/p3p3ds/mods/mod.cpk                (user mod package)
    ↓
  sdmc:/p3p3ds/mods/mod1.cpk ... mod3.cpk  (additional mod archives)
    ↓
  disc0:/PSP_GAME/USRDIR/umd0.cpk, umd1.cpk (original game archives)
  ```
  On PC, `--mods <dir>` maps `ms0:/PSP/P3P` to a host directory.
- **Translation & Mod Support (Future Goal):** The architecture aims to support community translations (such as Russian, Spanish, German, French) and mods via VFS redirection. Note that translations are not guaranteed to be drop-in asset-only packages: individual localizations may require custom font sheets, character encoding tables, `.bmd`/`.bf` script handling, or runtime executable hooks (such as glyph-spacing/kerning adjustments).

---

## Repository Structure

```text
P3P3DS/
├── 3ds/                 # 3DS platform libraries (libctru, citro3d, citro2d)
├── core/                # Target-agnostic runtime: HLE kernel, VFS, interpreter, GE executor + software renderer
├── docs/                # Current state, architecture, verification registry, research notes
├── experiments/         # Standalone analyses (decoder audit, CPK checker, microtests)
├── platform/
│   ├── pc/              # PC development runner (frame dumps, WAV output, event traces)
│   └── 3ds/             # New 3DS runner (.3dsx), built with devkitARM
├── profiles/p3p/        # P3P profile: AOT layout, patches, addresses, game inputs (local only)
├── recomp/              # Recompilation engines and tools (PSPRecomp, Yakumo, N64Recomp)
├── references/          # Reference emulators and hardware autotests (PPSSPP, pspautotests, uOFW)
├── psp/                 # PSP SDK headers, VFPU documentation, and Ghidra definitions
├── tests/               # CTest suites (HLE contracts, renderer, differential AOT/interpreter)
└── tools/               # Build helpers and asset tools (CriFsV2Lib, AtlusScriptTools, Amicitia)
```

---

## Prerequisites

To build the PC runner and development tools:
- **CMake** (version 3.20 or newer)
- **Ninja**
- **C++20 Compiler** (measured with GCC 16.1 / MinGW-w64; other compilers are untested)
- **Python 3.10+** (code generation helpers, analysis and verification scripts)
- About **5 GB of RAM** and ~11 minutes on a 12-thread CPU for a clean build of the 237 generated units (`-j10`)
- Optional: .NET SDK for `experiments/cpk-check` (CPK inspection)

---

## Game Setup (One-Command Preparation)

This repository contains **no copyrighted game assets or proprietary executables**. You must supply files from your own legally owned copy of *Shin Megami Tensei: Persona 3 Portable* (`ULUS-10512`).

Run the automated game preparation tool pointing to your retail ISO image:

```bash
python tools/prepare_game.py "/path/to/Persona 3 Portable.iso"
```

The tool automatically:
1. Verifies ISO9660 disc integrity and `PARAM.SFO` metadata (`ULUS10512`).
2. Decrypts the Allegrex executable (`EBOOT.BIN` -> `profiles/p3p/game/eboot.elf`) via the PSP AES-128 engine.
3. Validates the decrypted binary against the verified reference SHA-256 (`be2abbd4...`).
4. Extracts game assets (`USRDIR/` containing CPK archives) into `profiles/p3p/game/USRDIR/`.

*(All extracted files in `profiles/p3p/game/` are strictly local and gitignored.)*

*(Optional fallback: If you already have a pre-decrypted ELF, pass `--decrypted-eboot /path/to/eboot.elf`.)*

The runner reads game data directly from the ISO at run time: keep the `.iso` in the repository root (it is gitignored) or pass `--umd <iso>`.

---

## Development Workflow

### Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j10
ctest --test-dir build -j6
```

`-DP3P_AOT_TRANSFER_RECORDS=OFF` generates code without per-branch diagnostics (smaller and ~17% faster on PC, but without the transfer trace used by the frontier tooling).

### Build for New 3DS

Needs [devkitPro](https://devkitpro.org/wiki/Getting_Started) with the 3DS packages and the host build above (it provides `psp_recomp`). devkitPro's CMake toolchain only works from its own msys2 shell:

```bash
export DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM
cmake -S platform/3ds -B build/3ds -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake -DCMAKE_BUILD_TYPE=Release
make -C build/3ds -j10
```

A clean build takes about 9 minutes; after a runtime change only a few files and the link are rebuilt. Copy `build/3ds/p3p3ds.3dsx` to the console or emulator, put your ULUS-10512 `.iso` in `sdmc:/p3p3ds/`, and start it (Azahar: enable New 3DS mode). The bottom screen shows the debug report, also saved to `sdmc:/p3p3ds/report.txt`; START+SELECT quits. The `.3dsx` embeds code generated from your game executable: it is for your own testing, never for distribution.

### Run the game on PC

```bash
# Boot until the title screen (~45 s wall), dumping every 60th frame and all audio
./build/p3p_pc_bootstrap.exe --run-until-blocker --max-dispatches 20000000 \
    --frames-dir .tmp/frames --frame-every 60 --wav .tmp/p3p.wav --dump-events .tmp/run.json
```

The runner prints the stop reason and a summary; `--dump-events` writes a JSON trace of HLE calls, thread switches, GE lists, frames (with hashes) and audio statistics.

| Option | Purpose |
|---|---|
| `--run-until-blocker` | run until a missing service, a fault or the dispatch budget |
| `--max-dispatches <n>` | dispatch budget (20M ≈ title screen) |
| `--umd <iso>` | game image mounted as `disc0:` (default: the single `*.iso` in the working directory) |
| `--ms0 <dir>` / `--mods <dir>` | memory stick root (default `out/ms0`) / directory mapped to `ms0:/PSP/P3P` for mods |
| `--input <file>` | vblank-keyed controller script, e.g. `profiles/p3p/input/new_game.txt` (format in `core/include/p3p3ds/input.hpp`) |
| `--gamepad` | read XInput controller 0 |
| `--profile` | wall-time split between HLE, GE rendering and interpreter |
| `--frames-dir <dir>` `--frame-every <n>` | write displayed frames as BMP |
| `--wav <file>` | mix all game audio on the virtual clock into a 44.1 kHz stereo WAV |
| `--dump-events <json>` / `--io-trace <csv>` | execution trace / file reads (for `experiments/cpk-check`) |
| `--no-interpreter` | stop at PCs without AOT code instead of interpreting |
| `--verify-bootstrap` | fixed early-boot checkpoint used by CTest |

Execution is deterministic: two runs with the same options produce byte-identical traces, frames and audio.

### Reproduce Full Analysis Pipeline

```bash
bash experiments/p3p-analysis/reproduce_analysis.sh
```

### Documentation

- [`docs/CURRENT_STATE.md`](docs/CURRENT_STATE.md) — what works today, with evidence for every claim.
- [`docs/NEXT_STEPS.md`](docs/NEXT_STEPS.md) — roadmap and the movie/audio decoding plan.
- [`docs/VERIFICATION.md`](docs/VERIFICATION.md) — registry of technical claims and their status.
- [`docs/3DS_PLATFORM.md`](docs/3DS_PLATFORM.md) — New 3DS backend design and measurements.
- [`AGENTS.md`](AGENTS.md), [`CLAUDE.md`](CLAUDE.md) — engineering rules for contributors and coding agents; project skills live in `.claude/skills/`.

---

## License

The original P3P3DS code and documentation are released under the [MIT License](LICENSE). Third-party projects vendored in this repository (`recomp/`, `references/`, `psp/`, `3ds/`, `p3p/`, `tools/`) keep their own licenses. Game data is never part of the repository.

---

## Legal & Open-Source Research Notice

- This project is an independent reverse-engineering and open-source research effort.
- It is not affiliated with, endorsed by, or connected to Atlus, SEGA, Sony Interactive Entertainment, or Nintendo.
- No proprietary game binaries, copyrighted assets, encrypted firmware keys, or commercial code are distributed in this repository.
- All reverse engineering data and HLE implementations are derived from independent analysis, open-source references (`pspsdk`, `uofw`), and publicly available community research.

---
## Support development

P3P3DS is a personal research project developed in my spare time.

The project involves reverse engineering, static recompilation, runtime development, testing on real hardware, and maintaining development infrastructure.

If you find this project interesting and want to support its development, donations are appreciated.

Support helps cover:
- development tools and infrastructure
- AI/API usage for research and development
- testing hardware and related expenses

### Crypto

| Asset | Network | Address |
| --- | --- | --- |
| Bitcoin (BTC) | Bitcoin | `12bg49c1rUcuBkmmz1uDcrc1b53y1rKc6J` |
| Ethereum (ETH) | Ethereum | `0x10c5babc98271e427f65bdc11246d1abdcf4e86a` |
| USDC | Ethereum / ERC-20 | `0x10c5babc98271e427f65bdc11246d1abdcf4e86a` |
| USDT | BNB Smart Chain / BEP-20 | `0x10c5babc98271e427f65bdc11246d1abdcf4e86a` |
| USDT | TRON / TRC-20 | `TYXrXb76fPwfbKCD3C8FqERK2ybmN73Cbv` |

> Please verify the selected network before sending funds.
> Sending assets through an unsupported network may result in permanent loss.

Donations are completely optional and do not provide additional access, features, priority, or influence over development decisions.

## Upstream References & Credits

- [PSPRecomp](https://github.com/jessicanataliagta/PSPRecomp) by Jessica Natalia — C++20 static recompilation framework.
- [Yakumo](https://github.com/TeamGDB/Yakumo) by TeamGDB — Static recompilation port of *Monster Hunter Portable 3rd HD Ver.*
- [PPSSPP](https://github.com/hrydgard/ppsspp) by Henrik Rydgård & contributors — PSP emulation ground truth and HLE reference.
- [pspautotests](https://github.com/hrydgard/pspautotests) — Hardware behavioral test suite.
- [DaedalusX64-3DS](https://github.com/MasterFeizz/DaedalusX64-3DS) by MasterFeizz — Battle-tested Citro3D rendering and NDSP audio pipeline for MIPS on 3DS.
- [libctru](https://github.com/devkitPro/libctru) & [citro3d](https://github.com/devkitPro/citro3d) by devkitPro — Nintendo 3DS homebrew SDK and GPU libraries.
- [zarroboogs](https://github.com/zarroboogs/p3p-patches) & [DniweTamp](https://github.com/DniweTamp/Persona-3-Portable-Mod-Menu) — P3P community patches and reverse engineering research.
- [XenonRecomp](https://github.com/hedge-dev/XenonRecomp) by hedge-dev — Xbox 360 PPC-to-C++ static recompilation architecture/analysis reference.
- [ReXGlue](https://github.com/rexglue/rexglue-sdk) by Tom Clay & contributors — Xbox 360 AOT runtime and Xenia-derived platform architecture reference.
- [Xenia](https://github.com/xenia-project/xenia) by Ben Vanik & contributors — Kernel, memory, and GPU architecture reference for Xbox recomp ecosystem.
- [N64Recomp](https://github.com/N64Recomp/N64Recomp) by Mr-Wiseguy & contributors — MIPS static recompilation, indirect calls, relocations and jump-table reference.
- [Persona 3 Dual](https://github.com/p3d-project/persona-3-dual) by the p3d-project team — Nintendo dual-screen Persona UI/presentation and constrained handheld-rendering reference.
