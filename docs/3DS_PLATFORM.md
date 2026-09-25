# P3P3DS — Nintendo 3DS Platform Architecture & Runtime Integration Guide

This document details the hardware architecture, OS services, constraints, and integration requirements for porting the **P3P3DS** runtime to the **New Nintendo 3DS / New 2DS XL** family of systems.

---

## 1. Target Hardware Specifications

| Specification | Old 3DS / 2DS (CTR) | New 3DS / New 3DS XL / New 2DS XL (SNAKE) | P3P3DS Target Note |
| :--- | :--- | :--- | :--- |
| **CPU** | Dual-core ARM11 MPCore @ 268 MHz | **Quad-core ARM11 MPCore @ 804 MHz** | **Mandatory.** 3x clock speed + extra user core. Old 3DS is intentionally unsupported. |
| **FPU** | VFPv2 (single precision hardware) | **VFPv2 (single precision hardware)** | VFPU vector ops lower to VFPv2 scalar/vector loops. |
| **Total FCRAM** | 128 MB | **256 MB** | Double total system memory. |
| **App Usable RAM** | 64 MB (or ~80 MB extended) | **124 MB to 178 MB** (via New 3DS memory modes) | Ample room for 34 MB guest arena + recompiled binary. |
| **VRAM** | 6 MB embedded VRAM | **6 MB embedded VRAM** | Used for display framebuffers and depth/stencil buffers. |
| **GPU** | DMP PICA200 @ 268 MHz | **DMP PICA200 @ 268 MHz** | Programmable vertex pipeline + 6 Tev combiner stages. |
| **Top Screen** | 400x240 (or 800x240 3D) | **400x240 (or 800x240 3D)** | PSP is 480x272. Top screen scales to 400x227 (aspect ratio preserved). |
| **Bottom Screen** | 320x240 Resistive Touch | **320x240 Resistive Touch** | Ideal for status HUD, minimap, inventory, or touch shortcuts. |
| **Controls** | Circle Pad, D-Pad, ABXY, L/R, Select/Start | **+ C-Stick, ZL, ZR** | Extra inputs map to camera rotation and shortcuts. |

---

## 2. Core Horizon OS Services & `libctru` Integration

### 2.1. System Initialization & Clock Boost
To unlock the full 804 MHz CPU frequency and L2 cache on New 3DS, the runtime must initialize Horizon OS services with speedup enabled:

```c
#include <3ds.h>

void platform_init(void) {
    // 1. Initialize core OS services
    gfxInitDefault();
    romfsInit();
    archiveMountSdmc();
    
    // 2. Enable New 3DS CPU clock boost (804 MHz + L2 cache)
    osSetSpeedupEnable(true);
    
    // 3. Request CPU time on Core 2 (Worker core) for GE / audio tasks
    // Allows user code to use up to 80% of Core 2
    APT_SetAppCpuTimeLimit(80);
}
```

### 2.2. Memory Layout & Allocation Strategy
New 3DS provides separate memory allocators in `libctru`:
1. **Application Heap (`malloc` / `new`):**
   - Managed by the system heap.
   - Used for the Recompiled Executable's `.data` / `.bss`, and the 32 MiB PSP Guest RAM arena (`uint8_t guest_ram[32 * 1024 * 1024]`).
2. **Linear Memory (`linearAlloc`):**
   - Physically contiguous memory required for GPU DMA transfers (textures, vertex buffers, and display transfer targets).
   - Used for the PICA200 command buffer, dynamic vertex buffers, and swizzled texture cache.
3. **VRAM (`vramAlloc`):**
   - High-speed 6 MB internal memory.
   - Used for PICA200 color render targets (`C3D_RenderTarget`) and depth buffers.

```text
┌────────────────────────────────────────────────────────┐
│               New 3DS Physical FCRAM (256 MB)          │
├───────────────────────────────┬────────────────────────┤
│ OS / System Reserved (78 MB)  │ Available App (178 MB) │
├───────────────────────────────┴────────────────────────┤
│ Application Memory Breakdown:                          │
│ ├── Statically Recompiled Code (.text): ~35 MB         │
│ ├── Guest RAM Arena (32 MB):             32 MB         │
│ ├── Guest VRAM Mirror (2 MB):             2 MB         │
│ ├── Linear Memory (Textures / Meshes):   30 MB         │
│ ├── Citro3D Command Buffers & Buffers:    8 MB         │
│ └── Free Heap for VFS & CPK Cache:      ~71 MB         │
└────────────────────────────────────────────────────────┘
```

---

## 3. Multithreading & Core Allocation

Horizon OS on New 3DS exposes two distinct cores for user applications:
- **Core 0 (App Core):** Runs the main game loop, recompiled Allegrex MIPS code, and Atlus `.bf` script VM.
- **Core 2 (System/User Core):** Runs background tasks:
  1. **GE Command Processor:** Parses PSP display lists and dispatches Citro3D rendering commands.
  2. **Audio Worker Thread:** Streams ATRAC3+ / ADPCM audio buffers to NDSP.
  3. **File I/O Streamer:** Background reading of `.cpk` chunks from SDMC.

Thread creation using `libctru`:
```c
Thread ge_thread;
s32 prio = 0x30;
ge_thread = threadCreate(ge_worker_entry, NULL, 0x8000, prio, 2 /* Core 2 */, false);
```

---

## 4. GPU & Graphics Backend: Citro3D / PICA200

### 4.1. Rendering Architecture
The PSP Graphics Engine (GE) processes a command stream containing state changes and draw primitives:
- State commands: `CMD_TEXTURE`, `CMD_TEXFORMAT`, `CMD_BLEND`, `CMD_ALPHATEST`, `CMD_DEPTHFUNC`.
- Draw commands: `CMD_PRIM` (Points, Lines, Triangles, Triangle Strips, Sprites/Rectangles).

In P3P3DS, this maps to Citro3D:
```text
PSP GE Display List
       │
       ▼ [Parser in Core 2 Worker Thread]
┌───────────────────────────────────────────────────────┐
│ Citro3D Command Translation                           │
│ ├── 2D Sprites (Rectangles)                           │
│ │   └── Direct textured quad emission with ortho proj │
│ ├── 3D Geometry (Models/Dungeons)                     │
│ │   └── Citro3D vertex buffer + PICA vertex shader    │
│ └── Blend / Alpha States                              │
│     └── Citro3D Tev Combiner Stages                   │
└───────────────────────────────────────────────────────┘
       │
       ▼
PICA200 Hardware Rendering into 3DS Top Screen Render Target
```

### 4.2. Texture Swizzling (Morton Tiling)
- PSP textures are linear: pixel `(x, y)` is at `y * pitch + x`.
- 3DS PICA200 requires textures in **Morton (Z-order) 8x8 tiled format**.
- A texture upload routine converts linear PSP textures to tiled 3DS format upon texture bind, caching converted textures in a hash table keyed by PSP VRAM address.

### 4.3. Screen Scaling (PSP 480x272 -> 3DS 400x240)
- **Direct Fit:** 480x272 downscaled by factor 0.8333 yields **400x227**, centered on the 400x240 screen with a negligible 6-pixel letterbox top and bottom.
- **Display Transfer:** The PICA200 display transfer engine (`C3D_SyncDisplayTransfer`) performs hardware downscaling and color conversion automatically at zero CPU cost!

---

## 5. Audio Subsystem: NDSP Integration

### 5.1. Hardware DSP (Digital Signal Processor)
Nintendo 3DS features a dedicated Teak DSP capable of mixing up to 24 independent hardware channels with volume, panning, and surround filtering.
- Initialized via `ndspInit()`.
- Supports 16-bit PCM and DSP-ADPCM.

### 5.2. Audio Mapping
1. **BGM Music (`sceAtrac3plus`):**
   - Decoded via a lightweight ATRAC3+ decoder into a stereo 16-bit PCM circular buffer.
   - Streamed to NDSP channel 0 with double-buffering (`ndspChnWaveBufAdd`).
2. **Sound Effects (`sceSasCore` / `sceAudio`):**
   - Mapped to NDSP channels 1 through 16.
   - P3P's voice clips and battle sounds play natively with hardware mixing.

---

## 6. Storage & SDMC Virtual File System

- Access to Nintendo 3DS SD card is handled by `libctru` via `archiveMountSdmc()`.
- Standard SD card directory structure for P3P3DS:
  ```text
  sdmc:/p3p3ds/
  ├── config.ini
  ├── data/
  │   └── data.cpk         # Base game archive extracted from legal UMD
  ├── mods/
  │   ├── bind/            # Loose file overrides
  │   ├── mod.cpk          # Primary mod archive (e.g. Russian translation)
  │   └── mod1.cpk         # Optional secondary mod archive
  └── saves/               # PSP format save games
  ```
- File reads from SDMC are buffered in 64 KiB chunks to maintain 10–15 MB/s transfer speeds on Class 10 SD cards.

---

## 7. Controls & Button Mapping

The New 3DS controls map naturally to the PSP gamepad layout, with bonus inputs for enhanced gameplay:

| 3DS Input | PSP Button | In-Game Action |
| :--- | :--- | :--- |
| **Circle Pad** | Analog Stick | Character movement |
| **C-Stick (New 3DS)** | D-Pad Left/Right | Free 3D Camera Rotation |
| **D-Pad** | D-Pad | Menu navigation / Persona selection |
| **A** | Circle (East) | Confirm / Interact (US/JP selectable) |
| **B** | Cross (South) | Cancel / Back / Run |
| **X** | Square (West) | Shortcut Menu / Open Mod Menu |
| **Y** | Triangle (North) | Main Menu / Status / Persona |
| **L** | L-Trigger | Camera reset / Turn left |
| **R** | R-Trigger | Rush mode in battle / Turn right |
| **ZL / ZR (New 3DS)** | Mapped to Shortcuts | Fast-forward dialogue / Auto-advance |
| **Touch Screen** | Virtual buttons | Direct tap on dialogue choices & menu options |

---

## 8. Binary Size & Compiler Considerations

### 8.1. Preventing Linker & Compiler Out-of-Memory
A full PSP executable like Persona 3 Portable contains ~12,000 to 18,000 functions. Emitting all functions into a single giant C++ file causes GCC to crash with an Out-of-Memory (OOM) error.
- **Solution:** As established in `PSPRecomp` and `Yakumo`, generated code is partitioned into discrete translation units (e.g., `unit_00.cpp`, `unit_01.cpp`, each covering 256 KiB of code).
- **Optimization Flags for devkitARM GCC:**
  `-O2 -mcpu=mpcore -mfloat-abi=hard -mfpu=vfpv2 -fno-exceptions -fno-rtti`
  `-ffunction-sections -fdata-sections -Wl,--gc-sections` (strips unused symbols).
- Link-Time Optimization (`-flto`) should be disabled initially to avoid compiler timeouts on large translation units.
