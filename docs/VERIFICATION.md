# P3P3DS — Technical Claim Verification Registry

This registry tracks the verification status of all technical claims, hardware parameters, memory addresses, and architectural assumptions in the P3P3DS project, adhering to the `MEASURE FIRST` engineering principle.

---

## Status Classification
- `[VERIFIED]`: Confirmed against primary source code, executable disassembly, or empirical hardware/runtime test.
- `[INFERRED]`: Supported by architectural evidence, but not yet directly measured on target hardware.
- `[UNVERIFIED]`: Hypothesis requiring experimental testing.
- `[WRONG]`: Refuted claim (kept for historical record to prevent regressions).

---

## 1. Game & Executable (Persona 3 Portable `ULUS-10512`)

| Claim | Status | Primary Source / Citation | Notes |
| :--- | :--- | :--- | :--- |
| P3P US Product Code is `ULUS-10512` | `[VERIFIED]` | `p3p/p3p-patches/ULUS10512.ini:1` | Confirmed as primary target. |
| PSP User PRX Virtual Base is `0x08804000` | `[VERIFIED]` | `psp/pspsdk/include/pspkerneltypes.h`, `recomp/PSP-recompilation-project/README.md:55` | Standard base for decrypted game ELFs. |
| Runtime Entry Point is `0x08804108` (`module_start`) | `[VERIFIED]` | `profiles/p3p/game/eboot.elf` (header `e_entry` + base `0x08804000`), `experiments/p3p-analysis/independent_analysis.json` | Exact initial execution address. |
| Total relocations in executable: 178,513 | `[VERIFIED]` | `recomp/PSPRecomp/src/elf32.cpp:206-296`, `experiments/p3p-analysis/analyze_elf.py` | 81,398 R_26, 45,148 R_LO16, 37,395 R_HI16, 14,572 R_32 (0 invalid, 0 unsupported). |
| Total library import stubs: 221 across 22 libraries | `[VERIFIED]` | `experiments/p3p-analysis/compare_analysis.py` | Exact multiset match (Counter / canonical sorted tuples) between independent parser and PSPRecomp for all 221 import tuples. |
| Recompiled P3P `module_start` executes on PC | `[VERIFIED]` | `platform/pc/main.cpp`, runtime execution test `build/p3p_pc_bootstrap.exe` | Recompiled C++ code executes guest blocks and branches natively. |
| Deterministic execution milestone reached | `[VERIFIED]` | `build/p3p_pc_bootstrap.exe --verify-milestone` | Starts at `0x08804108`, executes `SysMemUserForUser::0x35669D4C` (`sceKernelSetCompiledSdkVersion600_602`, SDK `0x06020010`), returns to `0x0880413C`, advances and halts on `Missing HLE import SysMemUserForUser::0xF77D77CB` (`sceKernelSetCompilerVersion`) at PC `0x08B7FC04`, $ra=`0x08804148`. |
| Control-flow target breakdown: J vs JAL | `[VERIFIED]` | `experiments/p3p-analysis/analyze_elf.py` | 21,845 `j` sites (14,362 targets), 59,553 `jal` sites (7,698 targets), combined 81,398 sites (21,470 targets). |
| All .text instructions recognized by baseline decoder | `[WRONG]` | Empirical decoder test (`psprecomp::decode_allegrex`) | 912,337 instructions (99.9209%) recognized; 722 instructions (0.0791%) currently unsupported in baseline (`madd`, `break`, `msub`). |
| Baseline decoder recognizes 99.92% of .text (912,337 / 913,059) | `[VERIFIED]` | Empirical decoder pass over `.text` using `verify_p3p_decoder` | Exactly 722 instructions unsupported: 468 `madd` (fn=0x1C), 251 `break` (fn=0x0D), 3 `msub` (fn=0x2E). Sum: 468 + 251 + 3 = 722. |
| Codegen lowering covers 99.91% of .text (912,275 / 913,059) | `[VERIFIED]` | Audit of `codegen_main.cpp` lowering rules in `verify_p3p_decoder` | 912,275 instructions directly lowerable; 62 decoded instructions (10 `vfpu1`, 52 `vfpu4`) hit unlowered generic VFPU stub. |
| Intro Skip Hook address is `0x002594A4` (`li $v0, 6`) | `[VERIFIED]` | `p3p/p3p-patches/ULUS10512.ini:4` | Forces title screen state. |
| Mod Loader path prefix is `ms0:/PSP/GAME/P3P/%s` at `0x00395EE4` | `[VERIFIED]` | `p3p/p3p-patches/ULUS10512.ini:6-9` | Memory string replacement for loose file redirection. |
| Mod Loader search sequence is `bind/` -> `mod.cpk` -> `mod1.cpk` -> `mod2.cpk` -> `mod3.cpk` | `[VERIFIED]` | `p3p/p3p-patches/ULUS10512.ini:10-24` | Verified from community CWCheat table. |
| P3P event scripts are compiled `.bf` bytecode, not native MIPS | `[VERIFIED]` | `tools/Atlus-Script-Tools/Source/AtlusScriptCompiler/`, `p3p/Persona-3-Portable-Mod-Menu/ModMenu.flow` | Game engine uses an internal bytecode interpreter. |
| P3P uses CriWare CPK archives (`data.cpk`) | `[VERIFIED]` | `tools/CriFsV2Lib`, `tools/CriPakTools`, UMD file structure | Main asset container. |
| P3P has exactly 21,965 individual functions | `[INFERRED]` | `experiments/p3p-analysis/p3p_report.json` | 21,965 are heuristic analyzer *seeds* (prologue signatures / call targets); actual verified function boundaries require execution-driven discovery. |
| P3P has no self-modifying code | `[UNVERIFIED]` | Static inspection of `.text` | No evidence observed so far; runtime proof across all gameplay states requires live execution tracking. |
| P3P is 100% viable for static recompilation | `[INFERRED]` | Clean relocation handling + entry point execution milestone | Bootstrap and entry point succeed; viability of remaining complex subsystems (VFPU, overlays, indirect jumps) requires incremental verification. |
| Russian fan translation is 100% asset-only with no EBOOT patches | `[UNVERIFIED]` | Community packaging analysis | While scripts (`.bmd`) and fonts are asset-level, potential EBOOT localization patches (character spacing, glyph width) need empirical testing. |

---

## 2. PSP Architecture & Emulation

| Claim | Status | Primary Source / Citation | Notes |
| :--- | :--- | :--- | :--- |
| Standard PSP has 32 MiB User RAM + 2 MiB VRAM + 16 KiB Scratchpad | `[VERIFIED]` | `recomp/PSPRecomp/src/guest_memory.cpp:74-76`, `references/ppsspp/Core/MemMap.h:20` | Fixed physical layout; no TLB page walking. |
| PSP CPU Allegrex is MIPS32r2 with VFPU coprocessor | `[VERIFIED]` | `psp/vfpu-docs/README.md`, `references/ppsspp/Core/MIPS/` | VFPU instructions use custom opcodes and prefix registers. |
| Yakumo successfully statically recompiled all 355 MHP3rd HD overlays | `[VERIFIED]` | `recomp/Yakumo/README.ru.md:49`, `recomp/Yakumo/AGENTS.md:7` | Proves viability of large-scale PSP static recompilation. |
| sal063 static recompiler utilizes pure C runtime (`src/rt/`) | `[VERIFIED]` | `recomp/PSP-recompilation-project/src/rt/recomp.c` | Lightweight runtime reference without heavy C++ dependencies. |

---

## 3. Nintendo 3DS Hardware & Platform

| Claim | Status | Primary Source / Citation | Notes |
| :--- | :--- | :--- | :--- |
| New 3DS CPU clock boost to 804 MHz enabled via `osSetSpeedupEnable(true)` | `[VERIFIED]` | `3ds/libctru/include/3ds/os.h:58` | Triples CPU frequency and enables L2 cache. |
| New 3DS exposes Core 2 to user applications via `APT_SetAppCpuTimeLimit` | `[VERIFIED]` | `3ds/libctru/include/3ds/services/apt.h:47` | Allows requesting CPU allocation for thread processing. |
| Multi-threaded worker thread affinity (Core 0 vs Core 2) yields performance gain | `[UNVERIFIED]` | Needs hardware benchmark | Hypothesis that offloading GE display list / audio to Core 2 outperforms single-core execution with lower sync overhead. |
| New 3DS has 256 MB FCRAM with 124 MB application mode | `[VERIFIED]` | `3ds/libctru/include/3ds/os.h:40` | Ample headroom for 34 MB guest arena and recompiled code. |
| PICA200 texture sampling requires Morton 8x8 tiled layout | `[VERIFIED]` | `3ds/citro3d/source/texture.c:12`, `references/DaedalusX64-3DS/Source/SysCTR/Graphics/NativeTextureCTR.cpp:115` | Native GPU sampler layout requirement. |
| Optimal texture conversion pipeline (CPU swizzle vs C3D display transfer) | `[UNVERIFIED]` | Needs citro3d benchmark | Must profile texture cache overhead vs GPU display transfer on real hardware. |
| 3DS DSP (`ndsp`) supports 24 hardware-mixed audio channels | `[VERIFIED]` | `3ds/libctru/include/3ds/ndsp/ndsp.h:18` | Hardware mixing for music and sound effects. |
| Recompiled P3P code will fit in New 3DS memory | `[INFERRED]` | `docs/ARCHITECTURE_OPTIONS.md:3.2` | Estimated ~95–110 MB footprint vs 124–178 MB available; requires physical hardware measurement. |
| Recompiled P3P will run at 90-100% native speed on New 3DS | `[INFERRED]` | Architectural analysis | Inferred from native ARM machine code compilation vs in-flight JIT, but unmeasured without on-device profiling. |
| PICA200 can maintain 30 FPS for Tartarus 3D dungeons | `[UNVERIFIED]` | Needs citro3d benchmark | Working hypothesis based on geometry complexity vs DaedalusX64-3DS. |
