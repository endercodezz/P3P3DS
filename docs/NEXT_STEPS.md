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

1. **Step 1: Build & Verify PSPRecomp Core Toolchain** `[COMPLETED]`
   - Built `psprecomp_core`, `psp_analyze`, `psp_recomp`, and `dump_function` on PC.
   - Verified static linking flags and Ninja builds.

2. **Step 2: Study GTA VCS Profile Reference** `[COMPLETED]`
   - Inspected `profiles/vcs/host/vcs_profile.cpp` and `vcs_codegen_main.cpp`.
   - Extracted minimal bootstrap patterns (module stack, $gp from module info, entry point setup).

3. **Step 3: Establish the P3P Profile Skeleton** `[COMPLETED]`
   - Configured `profiles/p3p/config/p3p_ulus10512.toml` and `profiles/p3p/config/p3p_functions.csv`.
   - Established canonical layout matching the project's root build workflow.

4. **Step 4: Prepare & Decrypt Legally Sourced P3P Executable** `[COMPLETED]`
   - Prepared clean decrypted `profiles/p3p/game/eboot.elf` (ULUS-10512).
   - Verified ELF header, 32-bit Allegrex architecture, and virtual load base (`0x08804000`).

5. **Step 5: Run Static Analysis & Independent Analyzer** `[COMPLETED]`
   - Executed `psp_analyze` against `eboot.elf`, producing `experiments/p3p-analysis/p3p_report.json`.
   - Fixed `experiments/p3p-analysis/analyze_elf.py` to parse relocated PRX memory.
   - Validated 178,513 relocations and entry point `0x08804108`.

6. **Step 6: Extract & Audit Imported NIDs** `[COMPLETED]`
   - Cataloged all 221 import stubs across 22 libraries in `independent_analysis.json` and `p3p_report_imports.csv`.
   - Verified 100% agreement between independent parser and PSPRecomp.

---

### Phase 2: Code Generation & PC Execution Milestone

7. **Step 7: Generate Initial AOT C++ Translation Units** `[COMPLETED]`
   - Ran `psp_recomp` to lower `module_start` and predecessor functions to C++.
   - Generated import wrappers and registered function table in `build/generated/p3p_generated.cpp`.

8. **Step 8: Construct Minimal PC Host Runner (`p3p_pc_bootstrap`)** `[COMPLETED]`
   - Implemented `platform/pc/main.cpp` using `psprecomp::Runtime`.
   - Initialized 32 MiB guest RAM arena, relocated ELF, loaded module info, set $gp (`0x08C42A50`), and prepared 64 KiB stack at `0x09FFFF00`.

9. **Step 9: Execute Entry Point & Log First HLE Call** `[COMPLETED]`
   - Ran `p3p_pc_bootstrap.exe` on PC.
   - Executed recompiled `module_start`, advancing through real guest basic blocks.
   - Reached first PSP import wrapper at `0x08B7FC0C` ($ra=`0x0880413C`), deterministically stopping on:
     `Missing HLE import SysMemUserForUser::0x35669D4C`.

---

### Phase 3: Incremental HLE & Boot Milestone (Next Phase)

10. **Step 10: Implement Initial Kernel & Memory Stubs**
    - Implement `SysMemUserForUser` (`0x35669D4C` / `sceKernelSetCompilerVersion`, `sceKernelSetCompiledSdkVersion`).
    - Continue stepping execution to next imported calls in `ThreadManForUser` and `UtilsForUser`.

11. **Step 11: Implement Virtual File System (VFS) with Modding Support**
    - Implement `IoFileMgrForUser` (`sceIoOpen`, `sceIoRead`, `sceIoLseek`, `sceIoClose`).
    - Integrate multi-tier fallback pipeline:
      `SD:/p3p3ds/mods/bind/` -> `mod.cpk` -> `mod1.cpk` -> `data.cpk`.
    - Verify with `CriFsV2Lib` that P3P loads its initial archives without error.

12. **Step 12: Implement Display & Frame Timing**
    - Implement `sceDisplay` (`sceDisplaySetMode`, `sceDisplaySetFrameBuf`, `sceDisplayWaitVblankStart`).
    - Connect guest framebuffer in VRAM (`0x04000000`) to an SDL3/OpenGL debug window on PC.
    - Confirm the initial Atlus boot screen / legal disclaimer renders.

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
