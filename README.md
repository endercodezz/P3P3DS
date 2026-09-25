# P3P3DS

**P3P3DS** is an engineering project working toward running *Shin Megami Tensei: Persona 3 Portable* (`ULUS-10512`, North American release) natively on the **New Nintendo 3DS / New 3DS XL / New 2DS XL** family of consoles.

---

## Status

**Early Research & PC Execution Bootstrap.**

We have achieved the first live native execution milestone on PC:
1. The decrypted P3P executable is analyzed and loaded into a PSP user RAM arena.
2. All 178,513 PRX relocations are applied without unsupported or invalid types.
3. The game's entry point (`module_start`, `0x08804108`) has been statically recompiled into native C++ via an AOT pipeline.
4. The recompiled code executes natively on PC through a lightweight PSP runtime harness, advances guest control flow, and dispatches to the first PSP import stub (`SysMemUserForUser::0x35669D4C` / `sceKernelSetCompiledSdkVersion600_602`).

> **Note:** The game does **not** yet run on Nintendo 3DS, nor is it playable on PC yet. Current work is focused on expanding recompiled function coverage and implementing the necessary PSP HLE kernel services on PC before bringing up the native 3DS hardware backend.

---

## Progress Checklist

- [x] Analyze P3P executable structure (ELF32 PRX, segments, sections)
- [x] Full PRX relocation and library import table extraction (178,513 relocations, 221 import stubs)
- [x] Independent relocation-aware validation tooling matching PSPRecomp 100%
- [x] Minimal Ahead-of-Time (AOT) MIPS-to-C++ code generation
- [x] Execute recompiled P3P `module_start` on PC
- [x] First guest execution to PSP HLE import (`SysMemUserForUser::0x35669D4C`)
- [ ] Core PSP kernel and memory services (`SysMemUserForUser`, `ThreadManForUser`, `UtilsForUser`)
- [ ] Virtual File System (VFS) with CRI CPK streaming and mod overlay support
- [ ] Graphics display pipeline (PSP GE display list translation)
- [ ] Audio backend (NDSP audio streaming and `sceSasCore` software synth)
- [ ] New Nintendo 3DS native homebrew build (`.3dsx` / `.cia`)
- [ ] Playable game on New Nintendo 3DS hardware

---

## Goal

The ultimate goal of this project is a smooth, high-fidelity experience of Persona 3 Portable running natively on New Nintendo 3DS hardware, with full support for user-supplied community mods and arbitrary fan translations.

We are **not** building a general-purpose PSP emulator, nor an all-encompassing PPSSPP replacement. The scope is specifically tailored to Persona 3 Portable.

---

## Why P3P?

*Persona 3 Portable* is uniquely well-suited for a static recompilation port to the New Nintendo 3DS:
- **Visual Novel Navigation:** The 2D exploration and dialogue format of P3P maps naturally to dual-screen handheld hardware.
- **Relatively Low VFPU Density:** Only ~0.31% of the instruction stream uses PSP VFPU vector instructions (compared to heavily math-intensive titles like Monster Hunter or racing games).
- **Atlus Script Engine:** Story flow and cutscene logic run via an internal bytecode engine (`.bf`), meaning gameplay scripting is isolated from low-level MIPS machine code.
- **No Dynamic Self-Modifying Code:** Static analysis of the executable confirms standard relocatable code with clean function boundaries.

---

## Current Architecture

The project employs **Hybrid Static Recompilation (AOT)**:

```text
Decrypted P3P Executable (Allegrex MIPS ELF)
                    │
                    ▼
     Offline Static Recompiler (AOT)
  (Lowers Allegrex MIPS to native C++ functions)
                    │
                    ▼
       Native Host C++ Compilation
     (GCC / Clang on PC  ──►  devkitARM GCC on 3DS)
                    │
                    ▼
          P3P3DS Runtime Harness
  ├── Lightweight PSP HLE Kernel (SysMem, ThreadMan, IoFileMgr)
  ├── Multi-tier Virtual File System (SDMC asset & mod redirection)
  └── Platform Backend:
       ├── PC Runner (Debugging & rapid development)
       └── 3DS Native (Citro3D / PICA200 GPU + NDSP Audio)
```

1. **Game Machine Code:** Recompiled offline ahead-of-time into native C++ translation units. The resulting code compiles directly to native ARM machine code with compiler optimization.
2. **PSP Kernel & Services:** Handled by a modular, lightweight High-Level Emulation (HLE) runtime implemented in C/C++.
3. **Graphics Engine (GE):** Interprets PSP display lists and maps primitives directly to the 3DS PICA200 GPU via `citro3d`.
4. **Audio:** Decoded and streamed via the 3DS hardware DSP (`ndsp`).

---

## New 3DS Target Constraints

This project targets the **New Nintendo 3DS / New 3DS XL / New 2DS XL** exclusively (Old 3DS / 2DS is intentionally unsupported):
- **CPU:** Quad-core ARM11 MPCore @ 804 MHz with L2 cache enabled via `osSetSpeedupEnable(true)` (vs 268 MHz on Old 3DS).
- **RAM:** 256 MB FCRAM with 124–178 MB application heap (vs 64 MB on Old 3DS), providing plenty of room for the 34 MB guest memory arena and recompiled code.
- **Worker Core:** Core 2 is available for multithreaded worker tasks (display list processing, audio mixing, or file streaming).

---

## Dual-Screen Presentation (Future Design)

While keeping classic 1:1 PSP presentation intact, the architecture is designed to support enhanced dual-screen modes:
- **Top Screen (400x240):** 3D Tartarus exploration, battle scenes, and animated event sequences.
- **Bottom Screen (320x240):** Dialogue boxes, choice selections, minimaps, party status, and touch-screen shortcuts.

---

## Modding & Localization Architecture

Modding support and fan translations are first-class architectural requirements:
- **Language-Agnostic Core:** No language strings, fonts, or specific localization hacks are hardcoded into the runtime engine.
- **VFS Fallback Pipeline:** The file system intercepts `sceIoOpen` and resolves assets with priority fallback:
  ```text
  sdmc:/p3p3ds/mods/bind/<relative_path>   (Loose file overrides)
    ↓
  sdmc:/p3p3ds/mods/mod.cpk                (User mod package)
    ↓
  sdmc:/p3p3ds/mods/mod1.cpk ... modN.cpk  (Additional mod archives)
    ↓
  sdmc:/p3p3ds/data/data.cpk               (Original game archive)
  ```
- **Universal Translation Support:** Any community translation (Russian, Spanish, German, French, etc.) or gameplay overhaul can be loaded simply by dropping loose files or `.cpk` archives onto the SD card without rebuilding the executable.

---

## Repository Structure

```text
P3P3DS/
├── 3ds/                 # 3DS platform libraries (libctru, citro3d, citro2d)
├── core/                # Target-agnostic P3P runtime, HLE definitions, memory map
├── docs/                # Architecture, verification registry, and research papers
├── experiments/         # Standalone test harnesses and analysis scripts
├── platform/
│   ├── pc/              # PC host runner and bootstrap harness
│   └── 3ds/             # Native 3DS backend implementation
├── profiles/p3p/        # P3P profile configuration, function maps, and game inputs
├── recomp/              # Recompilation engines and tools (PSPRecomp, Yakumo, N64Recomp)
├── references/          # Reference emulators and hardware autotests (PPSSPP, pspautotests)
├── psp/                 # PSP SDK headers, VFPU documentation, and Ghidra definitions
└── tools/               # Asset tools (CriFsV2Lib, AtlusScriptTools, Amicitia)
```

---

## Prerequisites

To build the PC bootstrap harness and development tools:
- **CMake** (version 3.20 or newer)
- **Ninja** or MinGW Make
- **C++20 Compiler** (GCC 13+, Clang 16+, or MSVC 2022+)
- **Python 3.10+** (for analysis and verification scripts)

---

## How Local Game Files Are Supplied

This repository contains **no copyrighted game assets or proprietary executables**. You must supply files from your own legally owned copy of *Shin Megami Tensei: Persona 3 Portable* (`ULUS-10512`):

1. Decrypt `PSP_GAME/SYSDIR/EBOOT.BIN` from your retail UMD/ISO.
2. Place the decrypted ELF file at:
   ```text
   profiles/p3p/game/eboot.elf
   ```
3. Extract `PSP_GAME/USRDIR/` (containing `data.cpk`) to:
   ```text
   profiles/p3p/game/USRDIR/
   ```

*(These paths are gitignored.)*

---

## Development Workflow

### Build and Run the PC Bootstrap Runner

```bash
# 1. Configure the project
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 2. Build the recompiler and PC bootstrap executable
cmake --build build --target p3p_pc_bootstrap

# 3. Execute the recompiled module_start smoke test
./build/p3p_pc_bootstrap.exe
```

Expected output confirms the execution milestone:
```text
====================================================
   P3P3DS PC Bootstrap Execution Harness
====================================================
Target ELF:       profiles/p3p/game/eboot.elf
ELF Type:         65440 (PSP PRX relocatable)
Runtime Entry:    0x08804108
Relocations:      178513 total
Module Info:      p3p v1.1
Module GP:        0x08C42A50
Registered Funcs: 319

=== Execution Result ===
Stop Reason:      Missing HLE import SysMemUserForUser::0x35669D4C
Stopped:          yes
Final Guest PC:   0x08B7FC0C
```

### Reproduce Full Analysis Pipeline

```bash
bash experiments/p3p-analysis/reproduce_analysis.sh
```

---

## Legal & Clean-Room Notice

- This project is an independent clean-room engineering effort.
- It is not affiliated with, endorsed by, or connected to Atlus, SEGA, Sony Interactive Entertainment, or Nintendo.
- No proprietary game binaries, copyrighted assets, encrypted firmware keys, or commercial code are distributed in this repository.
- All reverse engineering data and HLE implementations are derived from clean-room analysis, open-source references (`pspsdk`, `uofw`), and publicly available community research.

---

## Upstream References & Credits

- [PSPRecomp](https://github.com/jessicanataliagta/PSPRecomp) by Jessica Natalia — C++20 static recompilation framework.
- [Yakumo](https://github.com/TeamGDB/Yakumo) by TeamGDB — Static recompilation port of *Monster Hunter Portable 3rd HD Ver.*
- [PPSSPP](https://github.com/hrydgard/ppsspp) by Henrik Rydgård & contributors — PSP emulation ground truth and HLE reference.
- [pspautotests](https://github.com/hrydgard/pspautotests) — Hardware behavioral test suite.
- [DaedalusX64-3DS](https://github.com/MasterFeizz/DaedalusX64-3DS) by MasterFeizz — Battle-tested Citro3D rendering and NDSP audio pipeline for MIPS on 3DS.
- [libctru](https://github.com/devkitPro/libctru) & [citro3d](https://github.com/devkitPro/citro3d) by devkitPro — Nintendo 3DS homebrew SDK and GPU libraries.
- [zarroboogs](https://github.com/zarroboogs/p3p-patches) & [DniweTamp](https://github.com/DniweTamp/Persona-3-Portable-Mod-Menu) — P3P community patches and reverse engineering research.
