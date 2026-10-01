---
name: n3ds-platform
description: Guide native New Nintendo 3DS backend implementation using libctru, citro3d, PICA200 GPU translation, NDSP audio, and SDMC storage.
---

# New Nintendo 3DS Platform Integration Workflow

Use this skill when:
- Writing or maintaining the Nintendo 3DS hardware backend (`platform/3ds/`);
- Interfacing with `libctru`, `citro3d`, `citro2d`, and `ndsp`;
- Configuring New 3DS CPU clock boost (804 MHz) and multi-core threading;
- Implementing the PSP GE display list translator for the PICA200 GPU;
- Implementing texture conversion into PICA200 Morton (Z-order) format;
- Streaming audio buffers to the 3DS hardware DSP.

---

## 1. Core Rule: MAINTAIN PLATFORM ABSTRACTION

Never introduce PC-specific code (e.g. Win32, SDL3, Vulkan, Direct3D) into generic `core/` files.
Keep a clean architectural boundary:
- `core/` — Target-agnostic P3P recompiled logic, HLE definitions, and VFS interfaces.
- `platform/pc/` — Host development, unit tests, and debugging runner.
- `platform/3ds/` — Native `libctru` / `citro3d` / `ndsp` implementation.

---

## 2. Key References

- `3ds/libctru/` — Horizon OS system calls, memory allocators (`linearAlloc`, `vramAlloc`), threads, and events.
- `3ds/citro3d/` — PICA200 GPU state management, render targets, vertex shaders, and Tev combiners.
- `3ds/citro2d/` — 2D drawing primitives and text rendering.
- `3ds/3ds-examples/` — Working code samples for GPU rendering, multi-threading, and audio streaming.
- `references/DaedalusX64-3DS/Source/SysCTR/` — Complete, battle-tested 3DS backend for MIPS emulation:
  - `Graphics/GraphicsContextCTR.cpp` (PICA200 graphics pipeline)
  - `Graphics/NativeTextureCTR.cpp` (Texture tiling and format conversion)
  - `HLEAudio/` (NDSP streaming)

---

## 3. Subsystem Implementation Requirements

### 3.1. System & Clock Initialization
Always initialize New 3DS speedup mode at startup and measure thread scheduling options:
```c
gfxInitDefault();
archiveMountSdmc();
osSetSpeedupEnable(true);       // Boost CPU to 804 MHz and enable L2 cache
// Evaluate Core 2 CPU quota if using multi-threaded worker architecture:
// APT_SetAppCpuTimeLimit(percentage);
```

### 3.2. Memory Allocation Rules
- **Linear Memory (`linearAlloc`):** Use for buffers accessed by GPU DMA / display transfer (Citro3D command buffers, dynamic vertex arrays, texture surfaces).
- **VRAM (`vramAlloc`):** Use for high-bandwidth color render targets and depth/stencil buffers.
- **Application Heap (`malloc`):** Use for general allocations and the 32 MiB PSP guest RAM arena.

### 3.3. Threading Architecture
- Profile and test thread core affinity before binding threads.
- Test whether worker tasks (display list processing, audio, or I/O) benefit from running on Core 2 via `threadCreate(..., prio, coreId, ...)` or if cooperative scheduling on the main core has lower synchronization overhead.
- Always check return codes of `threadCreate()` and join threads cleanly upon exit.

### 3.4. PICA200 GPU Rendering & Texture Format
- Verify actual texture upload and swizzling requirements directly against `3ds/citro3d` (`C3D_TexUpload`, `C3D_SyncDisplayTransfer`) and `3ds/libctru`.
- Do not assume fixed Morton tiling without benchmarking GPU texture upload vs display transfer conversion.
- Top Screen scaling: verify aspect-correct presentation (PSP 480x272 onto 3DS 400x240) via hardware display transfer or render-to-texture blit.

### 3.5. Audio via NDSP
- Initialize via `ndspInit()`.
- Use Channel 0 for BGM (streamed stereo 16-bit PCM in 4 KiB double buffers).
- Use Channels 1–16 for sound effects (`sceSasCore`).

---

## 3a. Build, Run and Diagnose the .3dsx

- Configure and build only from devkitPro's msys2 bash (Windows-native cmake is rejected by `3DS.cmake`):
  `<devkitPro>/msys2/usr/bin/bash.exe -lc "export DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM; cd <repo> && make -C build/3ds -j10"`
  (first time: `cmake -S platform/3ds -B build/3ds -G 'Unix Makefiles' -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake -DCMAKE_BUILD_TYPE=Release`, plus `-DPython3_EXECUTABLE=<host python>` if msys2 has none).
- Code size is the constraint (docs/3DS_PLATFORM.md 8.5). Never add a define that changes `Runtime`'s layout to only some targets; a define that changes compile flags of `p3p_aot` rebuilds all 237 units (~9 min) — put runtime-only defines on `psprecomp_runtime`.
- Test in Azahar (New 3DS mode) with the ISO in its virtual `sdmc/p3p3ds/`. Read `sdmc/p3p3ds/report.txt`: `stage`, heap size/in use, and `failed alloc: <bytes> from <addr>`; resolve the address with `arm-none-eabi-addr2line -f -C -i -e build/3ds/p3p3ds.elf <addr>`. The maintainer may be running Azahar too: do not kill an instance you did not start, and do not trust a report without checking its build id and timestamp.
- Never write `sdmc` or emulator paths of the maintainer's machine into tracked files.

## 3b. P3P3DS Builder release

- Sources: `platform/pc/builder/` (`pipeline.cpp` = the build steps, `psp_eboot.cpp` = decryption, `builder_main.cpp` = Win32 wizard, `fun_facts.hpp`). Compile flags, unit count and load base in `pipeline.cpp` must match `platform/3ds/CMakeLists.txt` and `profiles/p3p/config/aot_layout.cmake`; runtime-layout defines must match the prebuilt objects.
- Release: build the host (`--target p3p3ds_builder`, psp_recomp) and `build/3ds`, then `python platform/pc/builder/make_release.py --version X --devkitpro <dir>` -> `out/P3P3DS-Builder-X/` + `.zip` (it deletes and recreates that folder, including any `work/` cache in it).
- Test without the window: `P3P3DS-Builder.exe --cli --iso <iso> [--jobs n]`, then run `output/p3p3ds.3dsx` in Azahar. Never put a `.3dsx`, `work/` or `output/` into a release: they contain code made from the game.
- Facts in `fun_facts.hpp` must be well-known public facts or P3P3DS measurements; no guesses.

## 4. Standalone Experiment Requirement

Before integrating any 3DS subsystem into the full P3P runtime, create and verify a standalone test in `experiments/`:
1. `3ds_hello` — Minimal `libctru` application testing 804 MHz clock boost and text console.
2. `3ds_mem` — Allocate 34 MB guest arena + 30 MB linear memory to confirm memory headroom.
3. `3ds_threads` — Test thread creation and communication across Core 0 and Core 2.
4. `3ds_quad` — Render a textured quad via Citro3D on the top screen.
5. `3ds_audio` — Stream a sine wave or PCM buffer through NDSP.
6. `3ds_sdmc` — Benchmark sequential read speed from `sdmc:/p3p3ds/`.

---

## 5. Expected Output Format

When implementing or reporting 3DS platform components:

```text
Component:       <e.g. Memory | Graphics / citro3d | Audio / NDSP | Threading>
Target Device:   New Nintendo 3DS (CTR/SNAKE)
Status:          [VERIFIED | INFERRED | UNVERIFIED]
Hardware Test:   <Citra emulator | Real Hardware (Luma3DS)>
Metrics:         <Memory usage, frame delta ms, or audio buffer latency>
Blocking Issues: <None or specific Horizon OS constraint>
```
