# P3P3DS — Architecture Comparison & Technical Evaluation

This document provides a technical evaluation of the three architectural options for executing **Persona 3 Portable** on the **New Nintendo 3DS / New 2DS XL** platform.

---

## 1. Overview of the Three Architectures

```text
┌──────────────────────────────────────────────────────────────────────────────────┐
│                               ARCHITECTURAL OPTIONS                              │
└──────────────────────────────────────────────────────────────────────────────────┘

Option A: Pure Static Recompilation (AOT)
  P3P Allegrex EBOOT
       │
       ▼ [Offline Recompiler: PSPRecomp / Yakumo / sal063]
  Generated C/C++ Source Files (All functions -> C++ code)
       │
       ▼ [devkitARM gcc / g++]
  Native ARM11 Executable (.3dsx / .cia)
       │
  Links directly against minimal static runtime (HLE + 3DS Native Backends)

───────────────────────────────────────────────────────────────────────────────────

Option B: Specialized Lightweight Emulator
  P3P EBOOT (decrypted raw binary in memory)
       │
       ▼
  Specialized 3DS Host Application
  ├── Minimal Allegrex MIPS Interpreter / ARM11 JIT (DaedalusX64 style)
  ├── Minimal VFPU emulation (VFPv2 / Soft-float)
  ├── Selective HLE (only the ~50-80 APIs invoked by P3P)
  └── Citro3D / PICA200 GE Renderer

───────────────────────────────────────────────────────────────────────────────────

Option C: Hybrid Architecture (Recommended Implementation)
  1. Main Game Executable:
     Statically Recompiled Ahead-of-Time (AOT) into native C/C++ functions.
  2. PSP Subsystem Services (OS & Kernel):
     High-Level Emulation (HLE) runtime implemented in C/C++:
     - ThreadMan (mapped to 3DS cooperative/preemptive threads)
     - IoFileMgr (redirected to SDMC with priority mod overlay)
     - sceAudio / sceSasCore (streamed to 3DS NDSP hardware channels)
  3. Dynamic / Fallback Safety Net:
     Small interpreter / lookup dispatcher for unresolved indirect calls
     or dynamically patched hooks.
  4. Native 3DS Hardware Backends:
     - GE Display List Parser -> Citro3D / PICA200 commands
     - Audio -> 3DS DSP (NDSP)
     - Virtual File System -> SDMC with bind/ and mod.cpk hooks
```

---

## 2. In-Depth Comparative Matrix

| Evaluation Criteria | Option A: Pure Static Recompilation | Option B: Specialized Emulator | Option C: Hybrid Architecture (Recommended) |
| :--- | :--- | :--- | :--- |
| **Architectural Model** | Offline MIPS→C++ AOT + static runtime | Dynamic binary interpretation / ARM11 JIT + HLE | MIPS→C++ AOT + Modular HLE + Native 3DS Drivers |
| **CPU Performance on New 3DS (ARM11 @ 804MHz)** | **Maximum (95-100% native speed)**. No JIT overhead, compiler optimizes register allocation directly to ARM registers. | **Poor to Mediocre (20-45% speed)**. ARM11 has small L1/L2 caches and in-order execution; JIT compilation overhead on 3DS is severe. | **Maximum (90-100% native speed)**. All game code runs at native ARM speed; only OS calls execute via lightweight C dispatch. |
| **Executable Memory / JIT Restrictions** | **None**. Binary is fully signed/packaged ahead of time. Works on homebrew (.3dsx) and installed CIA without special JIT service permissions. | **High Risk**. Requires executable heap memory (`svcControlMemory` with `MEMOP_PROT` or CSND service access). | **None**. Pure native code execution; no dynamic executable page generation required. |
| **Binary & RAM Footprint** | **Large Binary / Low RAM Overhead**. Recompiled C++ code can result in a 25–45 MB ELF executable. Fits in 256 MB New 3DS FCRAM easily. | **Small Binary / High JIT RAM**. Small loader executable, but requires dedicated JIT translation cache (16–32 MB) + guest RAM. | **Balanced**. Recompiled code split into translation units + ~34 MB guest memory arena (32 MB RAM + 2 MB VRAM). |
| **GPU / PICA200 Translation** | Requires mapping PSP GE display list processor to PICA200 via `citro3d`. Must run asynchronously on secondary core. | Requires synchronous or buffered GE interpretation within emulator frame loop. High CPU overhead. | Hardware-matched: GE display list interpreted by a dedicated worker thread (Core 2) submitting commands to `citro3d`. |
| **VFPU Handling** | Ahead-of-Time translation of vector operations into optimized C/ARM VFPv2 code or inline routines. | Emulated via software function calls or complex ARM11 dynarec blocks with heavy pipeline stalls. | AOT vectorized math where possible; inline fast-paths for matrix multiply / transformations. |
| **Handling of Indirect Jumps & Jump Tables** | Requires exhaustive static jump table analysis (via CFG analysis and Ghidra/N64Recomp techniques) or runtime switch fallback. | Trivial for emulator (reads address from register and looks up block in JIT hash map). | Resolved statically for ~98% of tables; unresolved targets route through a central `dispatch_indirect(target_pc)` table. |
| **Handling Code Overlays (PRX)** | Must analyze and recompile each PRX overlay ahead-of-time (as proven by Yakumo with 355 overlays). | Handled dynamically by loading PRX into guest memory and executing it. | Offline recompiled overlay modules registered in an overlay table; resolved at runtime upon `sceKernelLoadModule`. |
| **Modding & File Redirection** | **Excellent**. Assets remain external (`sdmc:/p3p3ds/`). Mod loader works at the HLE `sceIo*` level without touching code. | **Excellent**. Easily hooked in emulator HLE layer. | **Maximum Flexibility**. HLE filesystem checks loose files (`bind/`), then `mod.cpk`, then base `data.cpk`. |
| **Development Complexity** | High recompiler tooling setup, but low runtime debugging once code generates cleanly. | Extremely High: writing an Allegrex+VFPU JIT for ARM11 requires months of low-level assembly work. | **Pragmatic & Iterative**: leverages existing recompiler tools (`PSPRecomp` / `sal063`), existing HLE logic, and `citro3d`. |
| **Overall Feasibility for P3P** | **High** | **Low (Too slow for 3DS ARM11)** | **Highest (Best balance of performance and control)** |

---

## 3. Deep Dive into Hardware Constraints on New 3DS

### 3.1. CPU: ARM11 MPCore @ 804 MHz vs. Allegrex MIPS @ 333 MHz
The New Nintendo 3DS features a quad-core ARM11 MPCore processor:
- **Core 0 (App Core):** Available for the main game thread.
- **Core 1 (System Core):** Reserved for Nintendo 3DS Horizon OS services.
- **Core 2 (Worker / Extra Core):** Available for user applications when unlocked via `APT_SetAppCpuTimeLimit(30)` or higher (up to 80%).
- **Core 3 (System Core):** Reserved for OS background operations.

**Why JIT (Option B) Fails on 3DS:**
The ARM1176JZF-S is an ARMv6 architecture processor with an in-order, 8-stage pipeline. It possesses very limited branch prediction and small L1 caches (16KB instruction / 16KB data per core) and no L2 cache on 3DS. In-flight dynamic recompilation (JIT) incurs severe instruction-cache invalidation penalties (`svcFlushProcessDataCache`), pipeline stalls, and constant context switching. DaedalusX64-3DS achieves playable framerates only for select N64 titles with aggressive assembly hand-tuning. Emulating the PSP's 333 MHz Allegrex (with its 128-bit VFPU vector pipe) through a JIT on an 804 MHz ARM11 would struggle to sustain even 15 frames per second.

**Why Static Recompilation (Option A/C) Succeeds:**
Ahead-of-Time compilation lets GCC 14+ (via devkitARM) optimize basic blocks globally:
- General-Purpose Registers (GPRs $r0-$r31) can be mapped directly into ARM11 registers (r0-r12, lr).
- Dead code is eliminated at compile time.
- Function delay slots are eliminated or reordered safely.
- Code generation produces straight-line native machine code that executes with zero runtime translation overhead.

### 3.2. Memory (RAM): 256 MB New 3DS FCRAM
- **PSP Requirements:**
  - Standard PSP RAM: 32 MiB
  - VRAM: 2 MiB (0x04000000 – 0x041FFFFF)
  - Scratchpad: 16 KiB (0x00010000 – 0x00013FFF)
  - Total Guest Memory footprint: **~34.02 MiB**.
- **New 3DS Available Memory:**
  - With `SYSTEM_MODE_EXT_124MB` or New 3DS mode, user applications have **124 MB to 178 MB** of contiguous linear RAM.
  - Recompiled game binary (.text + .rodata): **~30–45 MB**.
  - Guest Memory arena: **34 MB**.
  - Citro3D render targets & texture cache: **~20 MB**.
  - Citro3D display lists & audio stream buffers: **~10 MB**.
  - **Total estimated footprint:** ~95–110 MB, well within the 124–178 MB New 3DS ceiling!

### 3.3. GPU: PICA200 vs. PSP Graphic Engine (GE)
- **PSP Graphic Engine (GE):**
  - Tile-based / immediate renderer with fixed-function vertex transformation and texturing.
  - Supported primitives: Points, Lines, Triangles, Triangle Strips, Triangle Fans, Sprites (2D rects).
  - Vertex formats: 8-bit/16-bit/32-bit floats, packed bone matrices, morph weights.
  - Drawing controlled via a FIFO display list of 32-bit command words (e.g. `CMD_VTYPE`, `CMD_TEXTURE`, `CMD_VERTEXTYPE`).
- **Nintendo 3DS PICA200:**
  - Tile-based deferred renderer (TBDR).
  - Programmable Vertex Shader pipeline (DVLE bytecode).
  - Configurable Fragment Combiner pipeline (6 Texture Environment / Tev stages).
  - Internal VRAM: 6 MB dedicated high-speed VRAM (used for depth/stencil buffers and color framebuffers).
- **Matching the Two:**
  P3P is predominantly a 2D VN (Visual Novel) / UI driven game with 3D dungeon exploration (Tartarus) and 3D turn-based battles.
  - 2D scenes (UI, dialogue portraits, backgrounds, menus) use simple textured quads (`PRIM_RECTANGLES`). These map 1:1 to PICA200 textured quads!
  - 3D scenes (Tartarus floors, character models, Personas, shadows) use standard textured triangle meshes with diffuse lighting. These map directly to a standard Citro3D vertex shader with Tev combiner stages.
  - Texture format swizzling: PSP textures are stored in linear, unswizzled or simple swizzled formats (CLUT4, CLUT8, RGBA4444, RGBA5551, RGBA8888). 3DS PICA200 textures **must be swizzled into morton (Z-order) tiling**. Conversion can either be done on the fly on CPU Core 2 or cached in RAM.

---

## 4. Assessment of Persona 3 Portable Code Characteristics

Does P3P have dynamic code generation or anti-recompilation quirks?
1. **Self-Modifying Code:** None. P3P is a retail UMD game authored in standard C/C++ compiled with SN Systems ProDG MIPS compiler.
2. **Dynamic JIT:** None. The game's scripting engine (Atlus Flow / `.bf`) is an interpreted bytecode VM operating on script data loaded from archives, not native machine code.
3. **Overlays:** P3P relies primarily on its monolithic `EBOOT.BIN` with standard libraries. All overlays (if any PRX modules exist) can be identified from the UMD directory (`USRDIR/module/`) and statically recompiled into separate translation units using the Yakumo pipeline.
4. **Imports:** Standard PSP user libraries:
   - `IoFileMgrForUser` (file reading)
   - `ThreadManForUser` (threads, semaphores, event flags)
   - `sceCtrl` (gamepad inputs)
   - `sceDisplay` (framebuffer presentation and vsync)
   - `sceGe_user` (display list queuing and sync)
   - `sceAudio` / `sceSasCore` (sound effects)
   - `sceAtrac3plus` (music playback)
   - `sceMpeg` (cutscenes / intro video)
   - `sceUtility` (save data dialogs)

---

## 5. Architectural Verdict & Selected Strategy

### Recommended Winner: Architecture C (Hybrid Static Recompilation)

**Why Architecture C is the decisive choice:**
1. **Performance:** Only Ahead-of-Time static recompilation guarantees 30/60 fps on the 3DS ARM11 processor. Emulation via JIT is doomed to unplayable framerates.
2. **Modding Independence:** Keeping the HLE runtime and asset loader separate from the recompiled game code ensures that mods, translations (русификатор), custom scripts (`.flow`), and modded CPKs (`mod.cpk`) can be swapped on the SD card at runtime without recompiling the executable!
3. **Multi-Core Exploitation:**
   - **Core 0:** Runs the recompiled P3P game logic and scripts.
   - **Core 2 (Worker Core):** Runs the GE display list parser, texture swizzling, and NDSP audio streaming concurrently.
4. **Development Phasing:** We can compile and verify the recompiled code on PC first (using SDL3/OpenGL or software rendering as in `PSPRecomp` / `sal063`), achieve 100% logic and HLE stability, and then compile against devkitARM and link `citro3d` / `libctru` for New 3DS!
