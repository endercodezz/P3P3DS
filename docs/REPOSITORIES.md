# P3P3DS — Repository Registry & Classification

This document contains a structured inventory of all repositories currently integrated into the **P3P3DS** research/development workspace, as well as an evaluation of evaluated and rejected candidate projects.

Since 2026-10-03 the research sources under `references/`, `psp/` and `3ds/` are not tracked in git (`.gitignore`); `python tools/fetch_references.py` clones them (shallow, default branch) into the paths below. `recomp/`, `p3p/` and `tools/` stay vendored: the build uses PSPRecomp and the CWCheat patch list.

---

## 1. Active Repositories in Workspace

| Repository | Local Path | Roles | Priority | License | Why Useful | Important Directories / Files | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| [jessicanataliagta/PSPRecomp](https://github.com/jessicanataliagta/PSPRecomp) | `recomp/PSPRecomp` | `PRIMARY`, `STATIC RECOMPILER`, `CPU / ALLEGREX`, `VFPU`, `PSP HLE` | **CRITICAL** | MIT | Core C++20 static recompilation framework. Provides modular profile architecture (`profiles/`), Allegrex decoder, CFG analyzer, and battle-tested GTA VCS profile. | `include/psprecomp/`, `src/program_analysis.cpp`, `tools/codegen_main.cpp`, `profiles/vcs/` | Cloned & Analyzed |
| [TeamGDB/Yakumo](https://github.com/TeamGDB/Yakumo) | `recomp/Yakumo` | `PRIMARY`, `STATIC RECOMPILER`, `PSP HLE`, `VFPU`, `GPU / GE`, `AUDIO` | **CRITICAL** | MIT | Working, playable native static recompilation port of *Monster Hunter Portable 3rd HD Ver.* Proven capability compiling 355 code overlays, ARM compatibility, runtime logging, and HLE subsystems. | `src/`, `profiles/mhp3rd/host/`, `profiles/mhp3rd/scripts/`, `AGENTS.md` | Cloned & Analyzed |
| [sal063/PSP-recompilation-project](https://github.com/sal063/PSP-recompilation-project) | `recomp/PSP-recompilation-project` | `STATIC RECOMPILER`, `PSP HLE`, `REFERENCE`, `CPU / ALLEGREX`, `VFPU` | **HIGH** | GPL-2.0+ | Original PSP static recompiler toolkit with offline Python analyzer (`analyze.py`, `codegen.py`) and a pure **C** runtime (`src/rt/`). Extremely relevant for lightweight C emission compatible with devkitARM GCC. | `tools/codegen.py`, `tools/analyze.py`, `src/rt/recomp.c`, `src/rt/hle.c`, `src/rt/ge.c`, `src/rt/sched.c` | Cloned & Analyzed |
| [sp00nznet/psprecomp](https://github.com/sp00nznet/psprecomp) | `recomp/psprecomp-sp00nz` | `STATIC RECOMPILER`, `REFERENCE`, `CPU / ALLEGREX` | **MEDIUM** | MIT | Clean-room, permissively licensed rewrite effort targeting standalone MIPS-to-C recompilation inspired by N64Recomp and sal063. Excellent design notes on oracle-based verification. | `README.md`, `ROADMAP.md`, `docs/ORACLE.md`, `src/` | Cloned (renamed to avoid NTFS case conflict) |
| [wizardengineer/psprecomp](https://github.com/wizardengineer/psprecomp) | `recomp/psprecomp-wizardengineer` | `STATIC RECOMPILER`, `REFERENCE`, `HLE`, `GE`, `VFPU`, `ORACLE` | **HIGH** | GPL-2.0-or-later | [VERIFIED] Ghidra analysis JSON feeds a Rust AOT emitter and C++ runtime; Patapon-specific work and scheduler limits make it a useful comparison, not a drop-in P3P runtime. | `crates/psp-parser/src/analysis_json.rs`, `runtime/src/hle/`, `runtime/src/psp_scheduler.cpp`, `tools/ppsspp_trace/` | Vendored & examined |
| [N64Recomp/N64Recomp](https://github.com/N64Recomp/N64Recomp) | `recomp/N64Recomp` | `STATIC RECOMPILER`, `REFERENCE`, `CPU / ALLEGREX` | **HIGH** | MIT | Industry standard in MIPS static recompilation (Zelda 64 Recomp). State-of-the-art algorithms for basic block discovery, indirect jump/jump table resolution, and relocations. | `src/recompiler/`, `src/analysis/`, `include/` | Cloned & Evaluated |
| [hrydgard/ppsspp](https://github.com/hrydgard/ppsspp) | `references/ppsspp` | `REFERENCE`, `PSP HLE`, `GPU / GE`, `VFPU`, `AUDIO`, `FILESYSTEM` | **CRITICAL** | GPL-2.0+ | Gold-standard PSP emulator. Serves as ground-truth oracle for all `sce*` HLE APIs, GE display list command interpretation, VFPU bit-exact math, and ATRAC3+ audio decoding. | `Core/HLE/`, `GPU/GPUCommon.cpp`, `GPU/GPUState.h`, `Core/MIPS/`, `GPU/Software/` | Cloned (Shallow depth 1) |
| [jpcsp/jpcsp](https://github.com/jpcsp/jpcsp) | `references/jpcsp` | `REFERENCE`, `PSP HLE`, `VFPU`, `GE` | **MEDIUM** | GPL-3.0-or-later | [VERIFIED] Independent Java emulator with UMD HLE and Allegrex opcode tables. Compare behavior where uOFW, PSPSDK, pspautotests and PPSSPP leave questions; do not treat emulator behavior as firmware proof. | `src/jpcsp/HLE/modules/sceUmdUser.java`, `src/jpcsp/AllegrexOpcodes.java` | Vendored & examined |
| [azahar-emu/azahar](https://github.com/azahar-emu/azahar) | `references/azahar` | `3DS PLATFORM`, `PICA200`, `GPU DEBUGGING` | **MEDIUM** | GPL-2.0-or-later | [VERIFIED] 3DS emulator GPU and shader tests can validate future target output; it does not execute PSP guest code. | `src/video_core/gpu.cpp`, `src/tests/video_core/shader.cpp` | Vendored & examined |
| [hrydgard/pspautotests](https://github.com/hrydgard/pspautotests) | `references/pspautotests` | `TESTS`, `REFERENCE` | **HIGH** | BSD / Public Domain | Comprehensive test suite for PSP hardware behaviors: CPU instructions, VFPU math edges, GE commands, thread scheduling, synchronization primitives, and memory timing. | `tests/cpu/vfpu/`, `tests/gpu/`, `tests/threads/` | Cloned (Shallow depth 1) |
| [uofw/uofw](https://github.com/uofw/uofw) | `references/uofw` | `REVERSE ENGINEERING`, `PSP HLE`, `REFERENCE` | **HIGH** | Reverse engineered / GPL-3.0 | Complete C reverse engineering of original Sony PSP firmware modules. Invaluable for edge cases in ThreadMan, SysMem, IoFileMgr, and ModuleMgr. | `src/kernel/`, `src/sysmem/`, `src/threadman/`, `src/iofilemgr/` | Cloned & Evaluated |
| [MasterFeizz/DaedalusX64-3DS](https://github.com/MasterFeizz/DaedalusX64-3DS) | `references/DaedalusX64-3DS` | `3DS PLATFORM`, `DYNAMIC RECOMPILER`, `GPU / GE`, `AUDIO`, `REFERENCE` | **CRITICAL** | GPL-2.0 | Proven implementation of MIPS execution on 3DS ARM11, hardware-accelerated 3D rendering using `citro3d` / PICA200, and NDSP audio streaming. | `Source/SysCTR/Graphics/`, `Source/SysCTR/DynaRec/arm/`, `Source/SysCTR/HLEAudio/` | Cloned & Evaluated |
| [pspdev/pspsdk](https://github.com/pspdev/pspsdk) | `psp/pspsdk` | `PSP HLE`, `REFERENCE`, `REVERSE ENGINEERING` | **HIGH** | BSD-like | Official open-source PSP SDK headers, function prototypes, structures, and NID tables for all PSP OS libraries. Essential for writing type-accurate HLE wrappers. | `src/user/`, `src/kernel/`, `include/` | Cloned & Evaluated |
| [pspdev/vfpu-docs](https://github.com/pspdev/vfpu-docs) | `psp/vfpu-docs` | `VFPU`, `REFERENCE` | **HIGH** | Creative Commons / Public | Complete reference documentation of Sony PSP Vector Floating Point Unit (VFPU) matrices, registers, rotation opcodes, and prefix mechanics. | `README.md`, instruction tables | Cloned & Evaluated |
| [pspdev/prxtool](https://github.com/pspdev/prxtool) | `psp/prxtool` | `REVERSE ENGINEERING`, `ASSET TOOLS`, `CPU / ALLEGREX` | **HIGH** | Academic / Open | PSP PRX/ELF disassembler, relocation table parser, and NID symbol resolver. Useful for inspecting decrypted EBOOT sections and export/import tables. | `src/` | Cloned & Evaluated |
| [kotcrab/ghidra-allegrex](https://github.com/kotcrab/ghidra-allegrex) | `psp/ghidra-allegrex` | `REVERSE ENGINEERING`, `CPU / ALLEGREX`, `VFPU` | **HIGH** | Apache-2.0 | Ghidra processor definition for MIPS Allegrex (PSP) including full VFPU instruction decoding. Essential for decompilation and cross-verifying game functions. | `data/languages/allegrex.slaspec` | Cloned & Evaluated |
| [pspdev/psp-ghidra-scripts](https://github.com/pspdev/psp-ghidra-scripts) | `psp/psp-ghidra-scripts` | `REVERSE ENGINEERING`, `NID`, `GHIDRA` | **MEDIUM** | Apache-2.0 | [VERIFIED] PSP NID name resolution and hardware register annotation to speed import/caller review. | `SonyPSPResolveNIDs.py`, `SonyPSPMapHWRegisters.py` | Vendored & examined |
| [devkitPro/libctru](https://github.com/devkitPro/libctru) | `3ds/libctru` | `3DS PLATFORM`, `PRIMARY` | **CRITICAL** | Zlib / devkitPro | Core runtime library for Nintendo 3DS homebrew. Direct interface to Horizon OS sys-calls: memory allocation, multi-threading, New 3DS speedup mode, NDSP audio, and SDMC. | `include/3ds/`, `source/` | Cloned & Evaluated |
| [devkitPro/citro3d](https://github.com/devkitPro/citro3d) | `3ds/citro3d` | `3DS PLATFORM`, `GPU / GE`, `PRIMARY` | **CRITICAL** | Zlib / devkitPro | 3D graphics library for Nintendo 3DS PICA200 GPU. Manages command buffers, texture upload, render targets, vertex shaders, and texture combiners (Tev). | `include/c3d/`, `source/` | Cloned & Evaluated |
| [devkitPro/citro2d](https://github.com/devkitPro/citro2d) | `3ds/citro2d` | `3DS PLATFORM`, `GPU / GE`, `OPTIONAL` | **MEDIUM** | Zlib / devkitPro | 2D hardware-accelerated drawing library on top of citro3d. Can be used for HUD, touch-screen overlays, or text dialogues. | `include/c2d/`, `source/` | Cloned & Evaluated |
| [devkitPro/3ds-examples](https://github.com/devkitPro/3ds-examples) | `3ds/3ds-examples` | `3DS PLATFORM`, `REFERENCE` | **MEDIUM** | CC0 / Zlib | Official sample code demonstrating citro3d shaders, texture sampling, New 3DS clock speedup, multi-core threading, and NDSP audio streaming. | `graphics/gpu/`, `audio/streaming/` | Cloned & Evaluated |
| [devkitPro/picasso](https://github.com/devkitPro/picasso) | `3ds/picasso` | `3DS PLATFORM`, `SHADER TOOL` | **HIGH** | MIT | [VERIFIED] PICA200 shader assembler used to inspect or build target shader programs after a measured GE workload exists. | `source/picasso_assembler.cpp`, `Manual.md` | Vendored & examined |
| [neobrain/nihstro](https://github.com/neobrain/nihstro) | `3ds/nihstro` | `3DS PLATFORM`, `SHADER TOOL` | **MEDIUM** | BSD-3-Clause | [VERIFIED] Independent shader assembler/disassembler to cross-check PICA200 shader encoding. | `src/assembler.cpp`, `src/disassembler.cpp` | Vendored & examined |
| [kynex7510/GLASS](https://github.com/kynex7510/GLASS) | `references/GLASS` | `3DS PLATFORM`, `PICA200`, `GRAPHICS API` | **LOW** | MPL-2.0 header; no root license file | [VERIFIED] Work-in-progress OpenGL ES 2 interface and PICA extensions for platform research; no demonstrated P3P3DS performance benefit. | `Include/GLASS.h`, `Include/GLASS/Defs.h`, `Docs/MISC.md` | Vendored & examined |
| [zarroboogs/p3p-patches](https://github.com/zarroboogs/p3p-patches) | `p3p/p3p-patches` | `P3P-SPECIFIC`, `MODDING`, `REVERSE ENGINEERING` | **CRITICAL** | Unspecified / Public | CWCheat and xdelta patches for Persona 3 Portable (ULUS-10512). Documents key addresses for intro skipping and the canonical `ms0:/PSP/GAME/P3P/` mod loader hook. | `ULUS10512.ini` | Cloned & Analyzed |
| [DniweTamp/Persona-3-Portable-Mod-Menu](https://github.com/DniweTamp/Persona-3-Portable-Mod-Menu) | `p3p/Persona-3-Portable-Mod-Menu` | `P3P-SPECIFIC`, `MODDING`, `REVERSE ENGINEERING` | **HIGH** | Public Domain / Open | Mod menu written in Atlus Flow script (`.flow`) hooking into original event scripts (`.bf`). Maps all in-game field event script IDs (`h06_01` to `h37_02`). | `ModMenu.flow`, `hook/`, `Utilities.flow` | Cloned & Analyzed |
| [tge-was-taken/Atlus-Script-Tools](https://github.com/tge-was-taken/Atlus-Script-Tools) | `tools/Atlus-Script-Tools` | `ASSET TOOLS`, `P3P-SPECIFIC`, `MODDING` | **HIGH** | MIT | AtlusScriptCompiler suite for compiling and decompiling Atlus `.flow` / `.msg` into binary `.bf` / `.bmd` script formats. Necessary for custom event scripting and dialogue patching. | `Source/AtlusScriptCompiler/` | Cloned & Evaluated |
| [TekkaGB/AemulusModManager](https://github.com/TekkaGB/AemulusModManager) | `tools/AemulusModManager` | `MODDING`, `ASSET TOOLS`, `P3P-SPECIFIC` | **MEDIUM** | GPL-3.0 | Modern mod manager for Persona games. Handles multi-mod merging, CPK virtual file overriding, and package prioritization. | `AemulusModManager/` | Cloned & Evaluated |
| [tge-was-taken/Amicitia](https://github.com/tge-was-taken/Amicitia) | `tools/Amicitia` | `ASSET TOOLS`, `P3P-SPECIFIC`, `REVERSE ENGINEERING` | **HIGH** | MIT | GUI editor for Atlus container and model formats: PAC, BIN, BMD, BF, GIM, TM2, SPR. Essential for inspecting and extracting raw game assets. | `Amicitia/` | Cloned & Evaluated |
| [tge-was-taken/AtlusFileSystemLibrary](https://github.com/tge-was-taken/AtlusFileSystemLibrary) | `tools/AtlusFileSystemLibrary` | `ASSET TOOLS`, `P3P-SPECIFIC`, `FILESYSTEM` | **HIGH** | MIT | Low-level C# library providing programmatic read/write access to Atlus archive formats (CRI CPK, PAC/BIN, SPR, TMX/GIM). Ideal foundation for automated asset pipelines. | `AtlusFileSystemLibrary/` | Cloned & Evaluated |
| [Sewer56/CriFsV2Lib](https://github.com/Sewer56/CriFsV2Lib) | `tools/CriFsV2Lib` | `FILESYSTEM`, `ASSET TOOLS`, `MODDING` | **HIGH** | MIT | Ultra-fast modern library for parsing CriWare CPK archives and handling CRI file table virtualization with in-memory decompression. | `CriFsV2Lib/` | Cloned & Evaluated |
| [esperknight/CriPakTools](https://github.com/esperknight/CriPakTools) | `tools/CriPakTools` | `ASSET TOOLS`, `FILESYSTEM` | **MEDIUM** | Public Domain / Open | Command-line tool for unpacking and packing CriWare CPK archives with CRILAYLA compression algorithms. | `CriPakTools/` | Cloned & Evaluated |
| [hedge-dev/XenonRecomp](https://github.com/hedge-dev/XenonRecomp) | `references/static-recomp/XenonRecomp` | `STATIC RECOMPILER`, `REFERENCE`, `CPU / POWERPC`, `CFG / CODEGEN` | **HIGH** | MIT | State-of-the-art Xbox 360 PPC-to-C++ static recompilation architecture, recursive CFG analysis, and jump table lowering reference. | `XenonRecomp/`, `XenonRecomp.Analysis/` | Vendored & Analyzed |
| [rexglue/rexglue-sdk](https://github.com/rexglue/rexglue-sdk) | `references/static-recomp/rexglue-sdk` | `RUNTIME SDK`, `REFERENCE`, `KERNEL HLE`, `MEMORY / DISPATCHER` | **HIGH** | BSD-3-Clause | Clean-room modular runtime SDK for recompiled titles, featuring clean interface injection (`RuntimeConfig`), guest memory arena, and function thunking. | `include/rexglue/`, `src/` | Vendored & Analyzed |
| [xenia-project/xenia](https://github.com/xenia-project/xenia) | `references/static-recomp/xenia` | `EMULATOR`, `REFERENCE`, `KERNEL HLE`, `MEMORY / GPU` | **MEDIUM** | BSD-3-Clause | Research emulator providing mature reference implementations for kernel object managers, graphics translation state machines, and guest memory models. | `src/xenia/kernel/`, `src/xenia/gpu/`, `src/xenia/memory/` | Vendored & Analyzed |
| [N64Recomp/N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime) | `references/static-recomp/N64ModernRuntime` | `RUNTIME SDK`, `REFERENCE`, `DYNAMIC OVERLAYS`, `PATCH SYSTEM` | **MEDIUM** | GPL-3.0-or-later | Reference for dynamic overlay handling, binary function re-mapping, and modular C-based patch injection in static recompilations. | `src/` | Vendored & Evaluated |
| [Zelda64Recomp/Zelda64Recomp](https://github.com/Zelda64Recomp/Zelda64Recomp) | `references/static-recomp/Zelda64Recomp` | `STATIC RECOMPILER`, `REFERENCE`, `RUNTIME`, `GRAPHICS BRIDGE` | **HIGH** | GPL-3.0-or-later | Production recompiled game implementation, demonstrating RT64 graphics bridge, C patches, and asset loading. | `src/`, `patches/` | Vendored & Evaluated |
| [hedge-dev/UnleashedRecomp](https://github.com/hedge-dev/UnleashedRecomp) | `references/static-recomp/UnleashedRecomp` | `STATIC RECOMPILER`, `REFERENCE`, `RUNTIME`, `MEMORY / GPU` | **HIGH** | GPL-3.0-or-later | Complete production static recompilation project utilizing XenonRecomp codegen, host runtime dispatch, and memory virtualization. | `src/` | Vendored & Evaluated |
| [p3d-project/persona-3-dual](https://github.com/p3d-project/persona-3-dual) | `references/persona-3-dual` | `REFERENCE`, `PERSONA-SPECIFIC`, `DUAL-SCREEN DESIGN`, `UI / PRESENTATION`, `HANDHELD RENDERING` | **HIGH** | CC-BY-NC-SA-4.0 | Technical reference for Nintendo dual-screen Persona UI layouts (3D top screen, 2D bottom dialogue/menus), character portrait lifecycles, and handheld memory budgeting. | `source/views/EnvironmentView.cpp`, `source/components/screens/DialogueScreen.cpp`, `source/systems/UISystem.cpp`, `source/managers/RenderManager.cpp`, `source/controllers/AnimationController.cpp`, `tools/converters/` | Vendored & Analyzed |

---

## 2. wizardengineer/psprecomp comparison and decisions

The observations below concern the imported revision in `docs/UPSTREAMS.md`. They are research findings, not P3P execution measurements. The project is GPL-2.0-or-later (`recomp/psprecomp-wizardengineer/README.md:406-412`); implementation code from it must not be copied into our `core/`.

| Area | Observed reference design and evidence | P3P3DS consequence |
| :--- | :--- | :--- |
| Static recompilation | [VERIFIED] Headless Ghidra census plus ELF parser produce `analysis.json`; mid-entry and cross-jump data are represented (`README.md:40-42,78-92`, `crates/psp-parser/src/analysis_json.rs:70-126`). | [INFERRED] Compare its recovered targets with our conservative `profiles/p3p/config/frontier_seeds.csv` only when a missing guest-function frontier appears. Keep our CFG validator; an upstream target is not proof of a P3P boundary. |
| HLE and ThreadMan | [VERIFIED] NID/import table generated from analysis binds named HLE handlers (`ARCHITECTURE.md:59,149`); thread, semaphore, mutex and eventflag handlers live in separate `runtime/src/hle/psp_hle_kernel_*.cpp` files. Our handler registration is in `core/src/hle/hle_modules.cpp`, with thread ownership/priority in `core/src/hle/threadman.cpp`. | [INFERRED] For the present `sceUmdActivate` call, the useful pattern is import identity plus call-site evidence, followed by uOFW/PSPSDK/pspautotests semantics. No broad ThreadMan transplant is justified. |
| Scheduler | [VERIFIED] Upstream uses host threads with cooperative `sched_yield_point()` at chosen HLE calls (`runtime/src/psp_scheduler.cpp:160-216`, `docs/SCHEDULER-DESIGN.md:21-71`); its own design records busy-poll and timing limits. Our priority selection is in `core/src/hle/threadman.cpp:216-260`. | [INFERRED] Do not adopt HLE-only yields as a general PSP scheduling solution. When P3P exposes a scheduling divergence, capture thread states and the first differing event before changing policy. |
| GE | [VERIFIED] Upstream queues lists without blocking (`runtime/src/psp_render_queue.cpp:60-89`), but its `sceGeListEnQueueHead` ignores priority and `sceGeListSync` uses all-list sync (`runtime/src/hle/psp_hle_ge.cpp:34-59`). Our GE implementation explicitly lacks scheduler wait integration (`core/src/hle/ge.cpp:81`). | [INFERRED] A bounded list UID/stall trace could help classify a future real game-generated GE workload; the two current setup lists and VRAM metadata writes do not warrant renderer work. |
| VFPU | [VERIFIED] Decoder has prefix and matrix instruction handling plus regression cases (`crates/psp-decoder/src/vfpu.rs:516-545,1086-1263`). Our current decoder/lowering gap is recorded in `docs/CURRENT_STATE.md` as 62 generic VFPU words. | [INFERRED] Use opcode-level differential microtests only when a P3P-executed word reaches that gap. Upstream's matrix tests are a candidate test design, not a license to import code. |
| PPSSPP oracle | [VERIFIED] `tools/ppsspp_trace/` contains WebSocket capture and breakpoint tooling; `DEBUGGING.md:119-155` warns about dropped resume replies, build-specific breakpoint crashes, and same-snapshot comparison. | [INFERRED] Start with one read-only observation at the current UMD call and a separate P3P3DS event capture; identify exact version, game hash, PC, thread and register state before comparing outcomes. |

### Minimal differential oracle plan

1. [VERIFIED] PPSSPP's JSON WebSocket subprotocol is `debugger.ppsspp.org`; request/response events and optional `ticket` are documented in `references/ppsspp/Core/Debugger/WebSocket.cpp:30-50`. Existing handlers expose `cpu.breakpoint.add`, `cpu.status`, `cpu.getAllRegs`, `memory.read`, `hle.thread.list`, `cpu.stepping` and `cpu.resume` (`Core/Debugger/WebSocket/{BreakpointSubscriber,CPUCoreSubscriber,MemorySubscriber,HLESubscriber}.cpp`).
2. [UNVERIFIED] Launch a pinned PPSSPP build with the user's own ULUS-10512 image, record its hash and debugger endpoint, send `version`, then stop at guest call site `0x08AA15F8` or import `0x08B80124`. Capture PC, RA, SP, GP, A0-A3, V0-V1, thread UID, selected pointed-to bytes and emulated time. Test breakpoint support first: the upstream project documents a crash in one of its builds (`DEBUGGING.md:130-136`); no PPSSPP session was run for this import.
3. [VERIFIED] Our runner already emits structured events with `--run-until-blocker --dump-events` (`platform/pc/main.cpp:19,60-90`, `platform/pc/telemetry.hpp:59-86`). Capture two runs with identical inputs, compare event bytes and final blocker. Add an explicit boundary-register event only when the corresponding oracle snapshot exists; do not infer values from an API name.
4. [UNVERIFIED] Normalize only declared nonsemantic fields (host timestamp, absolute host paths). Compare ordered guest PC/call/return, thread UID and HLE result, then selected memory bytes from the *same* boot phase. Report first divergence with both raw snapshots and provenance. Do not silently normalize guest addresses or change runtime behavior to match PPSSPP alone.

Current oracle status: [VERIFIED] API and existing trace paths inspected statically; [UNVERIFIED] no running PPSSPP WebSocket connection or original P3P capture was available in this research sprint.

## 3. Repositories Evaluated and Rejected / Excluded

The following repositories were examined during technical research but intentionally excluded from the main workspace:

| Repository / Project | Stated Role | Reason for Rejection / Exclusion |
| :--- | :--- | :--- |
| **sp00nznet/ps3recomp** | Static Recompiler | Architecture mismatch: targets PlayStation 3 Cell Broadband Engine (PowerPC PPE + SPU swarm). Irrelevant to MIPS Allegrex and 3DS ARM11. |
| **PCSX-ReARMed (raw fork)** | PS1 Emulator | Focuses on MIPS-I (R3000A) without COP1 FPU or VFPU. DaedalusX64-3DS already provides a superior, actively optimized MIPS-to-ARM11 dynarec and citro3d renderer. |
| **dcherednik/extracpk** | CPK Extractor | Repository deleted / 404 on GitHub. Replaced by `CriFsV2Lib` and `CriPakTools`. |
| **Legacy 3DS PSP experiments (2015-2016)** | Prototype Emulator | Ancient, abandoned attempts to compile PPSSPP on 3DS without dynarec or PICA200 backends (achieved <1 fps). Non-functional code superseded by modern static recompilation approaches. |
| **Warez / Pirated ISO dumps** | Commercial assets | Strictly forbidden. All commercial ROMs, ISOs, decrypted EBOOTs, and copyrighted audio/dialogue assets are excluded per clean-room development rules. |

---

## 4. Directory Mapping Summary

```text
P3P3DS/
├── 3ds/
│   ├── 3ds-examples/
│   ├── citro2d/
│   ├── citro3d/
│   ├── nihstro/
│   ├── picasso/
│   └── libctru/
├── core/                       # Reserved for P3P3DS runtime & bridge code
├── docs/                       # Architectural analysis and development guides
├── experiments/                # Sandboxed test harnesses
├── p3p/
│   ├── Persona-3-Portable-Mod-Menu/
│   └── p3p-patches/
├── psp/
│   ├── ghidra-allegrex/
│   ├── psp-ghidra-scripts/
│   ├── prxtool/
│   ├── pspsdk/
│   └── vfpu-docs/
├── recomp/
│   ├── N64Recomp/
│   ├── PSP-recompilation-project/
│   ├── PSPRecomp/
│   ├── psprecomp-sp00nz/
│   ├── psprecomp-wizardengineer/
│   └── Yakumo/
├── references/
│   ├── DaedalusX64-3DS/
│   ├── GLASS/
│   ├── azahar/
│   ├── jpcsp/
│   ├── persona-3-dual/
│   ├── ppsspp/
│   ├── pspautotests/
│   ├── static-recomp/
│   │   ├── N64ModernRuntime/
│   │   ├── rexglue-sdk/
│   │   ├── UnleashedRecomp/
│   │   ├── xenia/
│   │   ├── XenonRecomp/
│   │   └── Zelda64Recomp/
│   └── uofw/
└── tools/
    ├── AemulusModManager/
    ├── Amicitia/
    ├── Atlus-Script-Tools/
    ├── AtlusFileSystemLibrary/
    ├── CriFsV2Lib/
    └── CriPakTools/
```
