# P3P3DS — Technical Roadmap & First Experimental Steps

This roadmap outlines the first 15 concrete, incremental engineering steps to establish the **P3P3DS** runtime. The strategy follows an iterative approach: **verify on PC first, achieve clean code generation and HLE logging, and only then port the native runtime to New 3DS.**

---

## 15-Step Experimental Implementation Plan

```text
Phase 1: Recompiler Toolchain & Analysis
Phase 2: Code Generation & PC Entry Point Execution
Phase 3: Subsystem HLE (I/O, VFS, Memory, Video/Audio)
Phase 4: Nintendo 3DS Backend Integration
```

### Phase 1: Recompiler Toolchain & Analysis

1. **Step 1: Build & Verify PSPRecomp Core Toolchain**
   - Build `psprecomp_core`, `psp_analyze`, `psp_recomp`, and `dump_function` on PC.
   - Run existing regression tests (`ctest`) to confirm decoder and analysis passes pass cleanly.
   - *Tools:* CMake, MSVC / GCC / Clang.

2. **Step 2: Study GTA VCS Profile Reference**
   - Inspect `profiles/vcs/host/vcs_profile.cpp` and `vcs_codegen_main.cpp`.
   - Document how thread contexts, memory arenas, register fast paths, and `register_hle` calls are registered.

3. **Step 3: Establish the P3P Profile Skeleton**
   - Create profile directory: `recomp/PSPRecomp/profiles/p3p/`.
   - Setup `profiles/p3p/CMakeLists.txt`, `profiles/p3p/config/`, `profiles/p3p/host/`, and `profiles/p3p/data/`.
   - Register `-DPSPRECOMP_PROFILE=p3p` in root `CMakeLists.txt`.

4. **Step 4: Prepare & Decrypt Legally Sourced P3P Executable**
   - From user's legal UMD copy (ULUS-10512), decrypt `EBOOT.BIN` to clean `EBOOT.ELF`.
   - Place in ignored directory `profiles/p3p/game/eboot.elf`.
   - Verify ELF header, 32-bit MIPS architecture, and segment virtual addresses (target base: `0x08804000`).

5. **Step 5: Run Static Analysis (`psp_analyze`)**
   - Execute `psp_analyze` against `eboot.elf`.
   - Discover entry points, function boundaries, jump tables, and call graph.
   - Output function map: `profiles/p3p/config/p3p_functions.csv`.

6. **Step 6: Extract & Audit Imported NIDs**
   - Parse all `.lib.stub` / `.rodata.sceNid` imports in the ELF using `psp/prxtool` or `tools/imports.py`.
   - Produce complete list of required PSP HLE APIs (estimated 60–90 distinct NIDs).
   - Match against existing PPSSPP and PSPRecomp HLE functions.

---

### Phase 2: Code Generation & PC Harness

7. **Step 7: Generate Initial AOT C++ Translation Units**
   - Run `psp_recomp` with function map and partition settings (e.g. 256 KiB chunks).
   - Verify generated units: `profiles/p3p/generated/unit_00.cpp`, `unit_01.cpp`, etc.
   - Verify syntax cleanliness, jump table handling, and VFPU instructions lowering.

8. **Step 8: Construct Minimal PC Host Runner (`p3p_host_pc`)**
   - Create entry point `profiles/p3p/host/main_pc.cpp`.
   - Initialize `GuestMemory` arena (32 MiB RAM + 2 MiB VRAM + 16 KiB scratchpad).
   - Map ELF data and BSS sections into guest RAM.
   - Set up initial CPU registers ($sp, $ra, $gp) and thread context for `module_start`.

9. **Step 9: Execute Entry Point & Log First HLE Calls**
   - Run the PC runner and begin stepping through recompiled code.
   - Catch the first imported NID calls via a generic fallback:
     ```cpp
     void unhandled_hle(psprecomp::Runtime &rt, const char *mod, uint32_t nid) {
         printf("[HLE UNHANDLED] %s:0x%08X at PC=0x%08X\n", mod, nid, rt.cpu().pc);
     }
     ```
   - Incrementally implement stubs for `SysMemUserForUser`, `ThreadManForUser`, and `UtilsForUser`.

---

### Phase 3: Core HLE Subsystems

10. **Step 10: Implement Virtual File System (VFS) with Modding Support**
    - Implement `IoFileMgrForUser` (`sceIoOpen`, `sceIoRead`, `sceIoLseek`, `sceIoClose`).
    - Integrate multi-tier fallback pipeline:
      `SD:/p3p3ds/mods/bind/` -> `mod.cpk` -> `mod1.cpk` -> `data.cpk`.
    - Verify with `CriFsV2Lib` that P3P loads its initial archives without error.

11. **Step 11: Implement Display & Frame Timing**
    - Implement `sceDisplay` (`sceDisplaySetMode`, `sceDisplaySetFrameBuf`, `sceDisplayWaitVblankStart`).
    - Connect guest framebuffer in VRAM (`0x04000000`) to an SDL3/OpenGL debug window on PC.
    - Confirm the initial Atlus boot screen / legal disclaimer renders.

12. **Step 12: Implement Controller Input & Event Flags**
    - Hook `sceCtrl` (`sceCtrlReadBufferPositive`, `sceCtrlPeekBufferPositive`).
    - Map PC gamepad/keyboard to PSP buttons.
    - Implement semaphores and event flags needed for game state transitions.

---

### Phase 4: Nintendo 3DS Backend Integration

13. **Step 13: Build Minimal 3DS Native Harness**
    - Create `3ds/p3p3ds/` CMake project using devkitARM toolchain.
    - Test New 3DS speedup initialization: `osSetSpeedupEnable(true)` and `APT_SetAppCpuTimeLimit(80)`.
    - Verify `citro3d` clear screen and basic textured quad display on New 3DS top screen.

14. **Step 14: Cross-Compile Recompiled P3P Units for ARM11**
    - Link generated `unit_*.cpp` into the 3DS homebrew target (.3dsx).
    - Compile with `-mcpu=mpcore -mfloat-abi=hard -mfpu=vfpv2 -O2`.
    - Verify memory consumption fits within the 124–178 MB application heap.

15. **Step 15: Connect Citro3D GE Renderer & NDSP Audio**
    - Replace the PC debug renderer with the Citro3D PICA200 command translator on Core 2.
    - Connect `sceAudio` / `sceSasCore` to 3DS NDSP hardware channels.
    - Verify full in-game execution on hardware / Citra emulator!
