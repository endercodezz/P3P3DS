# P3P3DS — Dirty First Frame Execution Trace

Iterative blocker chasing log advancing Persona 3 Portable initialization toward the first visible visual output.

---

### Stop #1
- **PC:** `0x08B4E6A0`
- **Type:** DIRECT GUEST FUNCTION / SYSMEM ALLOCATION
- **Cause:** Atlus CRT heap initialization function `UserSbrk` calling `sceKernelAllocPartitionMemory` (type 3: `PSP_SMEM_LowAligned`, size 16 MiB, align 4096) and `sceKernelGetBlockHeadAddr`.
- **Fix:** Implemented `SysMemManager` partition memory model (`PSP_SMEM_LowAligned`), added `sub_08B4E6A0` to `p3p_functions.csv`, registered HLE wrappers for `SysMemUserForUser::0x237DBD4F`, `0x9D9A5BA1`, `0xB6D61D02`, `0x13A5ABEF`, `0xF9100EB9`, `0xA291F107`.
- **New Stop:** `0x08B74100`

---

### Stop #2
- **PC:** `0x088042EC`
- **Type:** CPU ARCHITECTURAL STATE ($k0)
- **Cause:** Instruction `sw s5, 4(k0)` faulted accessing address `0x00000004` because `$k0` (GPR 26, thread context block pointer in PSP OS convention) was uninitialized (0).
- **Fix:** Initialized `$k0` to `stack_top - 256` across `init_root_thread`, `create_thread`, `start_thread`, and `platform/pc/main.cpp`. Added `sub_08B74100`, `sub_08B73CF0`, `sub_08B73F1C`, `sub_08B72DF8`, `sub_08804538`, `sub_08B72F04`, and `sub_08804000` to `p3p_functions.csv`. Registered HLE for `ModuleMgrForUser::0xD8B73127`, `Kernel_Library` (suspend/resume intr, get thread id, memset, lwmutex), and `ThreadManForUser::0xAA73C935`.
- **New Stop:** `0x08B73E2C`

---

### Stop #3
- **PC:** `0x08B73E2C`
- **Type:** DIRECT GUEST FUNCTION
- **Cause:** Thread context inspection function in C runtime, succeeded by module cleanup calls.
- **Fix:** Added `sub_08B73E2C`, `sub_088040A8`, `sub_08B4F714` to `p3p_functions.csv`. Registered HLE for `ThreadManForUser::0xE81CAF8F` (`sceKernelCreateCallback`), `LoadExecForUser::0x4AC57943` (`sceKernelRegisterExitCallback`), `sceImpose::0x36AA6E91` (`sceImposeSetLanguageMode`), `sceUtility::0x2A2B3DE0` (`sceUtilityLoadNetModule`), `ThreadManForUser::0x68DA9E36` (`sceKernelDelayThread`), `ThreadManForUser::0x369ED59D` (`sceKernelGetSystemTimeLow`).
- **New Stop:** `0x08AB304C` (called by `sub_08804538` at `0x088045C8`)

---

### Stop #4
- **PC:** `0x08B7FC5C`
- **Type:** HLE IMPORT
- **Cause:** Missing `ThreadManForUser::0xEA748E31` (`sceKernelChangeCurrentThreadAttr`).
- **Fix:** Implemented `change_current_thread_attr` in `ThreadManager` and registered HLE handler. Fixed `local_pc` and `entry_id` declaration in `codegen_main.cpp`.
- **New Stop:** `0x08B6029C` (called by `sub_08B1477C` at `0x08B147C8`)

---

### Stop #5
- **PC:** `0x08B60B94` / `0x08B60640`
- **Type:** DIRECT GUEST FUNCTION / GE DISPLAY LIST / DISPLAY SETUP
- **Cause:** Graphics initialization pipeline executing `sceGeListEnQueue` (twice), setting up display lists, and calling `sceDisplaySetMode` (`480x272`).
- **Fix:** Implemented `DisplayManager` and `GeManager`, registered `sceDisplay` and `sceGe_user` HLE modules, registered `ThreadManForUser` event flags. Added GE setup functions (`sub_08B6029C`, `sub_08B63B84`, `sub_08B603D8`, `sub_08B60D58`, `sub_08B60F20`, `sub_08B60F4C`, `sub_08B60F74`, `sub_08B60FA0`, `sub_08B63C90`, `sub_08B60B94`, `sub_08B614E8`, `sub_08B61664`, `sub_08B61FF4`, `sub_08B61EFC`, `sub_08B61FA0`, `sub_08B610C8`, `sub_08B613F0`, `sub_08B5F300`, `sub_08B14550`, `sub_08B61228`, `sub_08B15DCC`, `sub_08B63564`, `sub_08B6360C`, `sub_08B636B8`, `sub_08B5F4FC`, `sub_08B175C0`).
- **New Stop:** `0x08B60640`

---

### Stop #6
- **PC:** `0x08B60640`
- **Type:** MIPS JUMP TABLE / DIRECT GUEST FUNCTIONS / DISPLAY & GE SUBMISSION
- **Cause:** GE state configuration function `sub_08B60640` dispatched through indirect jump table at `0x08BB4480` to `0x08B606A0`, which executed `sub_08B61478` calling `sceDisplaySetMode` (480x272) and `sceDisplaySetFrameBuf` (`topaddr=0x04088000`, `bufferwidth=512`, `pixelformat=3`, `sync=1`).
- **Fix:** Added jump table targets `sub_08B606A0`, `sub_08B60760`, `sub_08B607B4` and framebuffer helper `sub_08B61478` to `p3p_functions.csv`. Added GE sync functions `sub_08B6141C`, `sub_08B61430`, `sub_08B61438`, `sub_08B61450`.
- **New Stop:** `0x08B174E0`

---

### Stop #7
- **PC:** `0x08B174E0`
- **Type:** UNSUPPORTED ALLEGREX INSTRUCTION
- **Cause:** Instruction `0x0053001C` (`madd $v0, $s3`) faulted with `special? not lowered yet`.
- **Fix:** Added `Madd`, `Maddu`, `Msub`, `Msubu` opcode kinds to `decoder.hpp`, decoded function codes `0x1C` (`madd`), `0x1D` (`maddu`), `0x2E` (`msub`), `0x2F` (`msubu`) in `decoder.cpp`, and implemented lowering with 64-bit HI/LO accumulator math in `codegen_main.cpp`. Added `sub_08B177F0`, `sub_08B13400`, `sub_08AC4794`, `sub_08B15E58`, `sub_08B15AB0`, `sub_08B1B8AC`, `sub_08B1C214`, `sub_08B17AC0`, `sub_08B19928`, `sub_08B6094C` to `p3p_functions.csv`.
- **New Stop:** `0x08B19890` (Deterministic Frontier)

---

## Visual Probe Findings & VRAM State

### Display & Framebuffer Status
- **`sceDisplaySetMode` Reached:** `YES` (`mode = 0` [LCD 480x272], `width = 480` [0x1E0], `height = 272` [0x110]).
- **`sceDisplaySetFrameBuf` Reached:** `YES` (`topaddr = 0x04088000`, `bufferwidth = 512`, `pixelformat = 3` [PSP_DISPLAY_PIXEL_FORMAT_8888], `sync = 1` [PSP_DISPLAY_SETBUF_NEXTVSYNC]).
- **Buffer 0 (0x04000000):** Backed by PSP VRAM / EDRAM (`kVramPhysicalBase = 0x04000000`, 557,056 bytes = 0x88000). Valid raw pointer, 0 non-zero bytes (0.00% filled, all 0x00).
- **Buffer 1 (0x04088000):** Backed by PSP VRAM / EDRAM (`0x04088000`, 557,056 bytes = 0x88000). Valid raw pointer, 0 non-zero bytes (0.00% filled, all 0x00).
- **Entire 2 MiB PSP VRAM (0x04000000 - 0x04200000):** Exactly 0 non-zero bytes out of 2,097,152 bytes (0.00% filled).

### Proven vs Unproven Distinctions
- **Framebuffer Setup Proven:** `YES` — P3P guest code deterministically configured display mode and registered VRAM buffer addresses via official PSP display APIs.
- **Display List Generation Proven:** `YES` — P3P submitted 2 GE display lists:
  1. Static setup list at `0x08BB40D4` (clearing/initializing GE state registers).
  2. Dynamic display list at `0x48D14600` (uncached alias of `0x08D14600`), completed up to `0x48D14674` (29 commands ending with `GE_CMD_FINISH` and `GE_CMD_END`), configuring render target buffer to `0x04000000`, Z-buffer to `0x04110000`, viewport, scissor, and depth range.
- **Actual Rendering to Framebuffer Proven:** `NO` — VRAM contains only zero bytes. The submitted display list has not been rasterized/drawn into EDRAM, and the game has not yet issued texture blit or 2D polygon primitive draw commands to populate pixel data.
- **First Frame Claim:** **NO FAKE CLAIM.** Framebuffer registration is verified, but actual rendered pixels do not exist yet.

---

## GE Command Decoder & Formatting Verification

- **Investigation:** In initial probe traces, `0xD2000003` was formatted as `ARG=0x300000` instead of `ARG=0x000003`, and `0x9D0001E0` as `ARG=0x1E0000` instead of `ARG=0x0001E0`.
- **Root Cause Analysis:** Verified that `GeManager` and `GuestMemory` operate in native little-endian MIPS format without byte-swapping bugs. The discrepancy was entirely caused by a stream formatting defect: `print_registers` set `std::left` on `std::cout` without resetting it to `std::right`. Consequently, `std::setw(6) << std::setfill('0') << 3` left-aligned the integer `3` and padded zeroes to the right (`300000`).
- **Correction Applied:** Reset `std::cout << std::right;` in `print_registers` and explicitly enforced `std::right` in `dump_ge_list`. The verified output now accurately renders:
  - `0xd2000003 -> OP=0xd2, ARG=0x000003 (FRAMEBUFPIXFORMAT: 3)`
  - `0x9d0001e0 -> OP=0x9d, ARG=0x0001e0 (FRAMEBUFWIDTH: 480)`
  - `0x9d000200 -> OP=0x9d, ARG=0x000200 (FRAMEBUFWIDTH: 512)`
  - `0xd6002710 -> OP=0xd6, ARG=0x002710 (MINZ)`
  - `0xd700c350 -> OP=0xd7, ARG=0x00c350 (MAXZ)`
  - `0x0f000000 -> OP=0x0f, ARG=0x000000 (FINISH)`
  - `0x0c000000 -> OP=0x0c, ARG=0x000000 (END)`

---

## Technical Debt & DIRTY_FIRST_FRAME Markers
- `DIRTY_FIRST_FRAME` markers in `core/src/hle/hle_modules.cpp`:
  - `ModuleMgrForUser::0xD8B73127` (`sceKernelGetModuleIdByAddress`): returns UID 1.
  - `Kernel_Library::0x092968F4` (`sceKernelCpuSuspendIntr`): returns 1.
  - `Kernel_Library::0xBEA46419` / `0x15B6446B` / `0x1839852A` (`sceKernel*LockLwMutex`): single-threaded locks.
  - `ThreadManForUser::0x55C20A00` / `0x402FCF22`: event flag UID and synchronous poll/wait.
  - `ThreadManForUser::0xE81CAF8F`: callback UID 1.
  - `LoadExecForUser::0x4AC57943`: exit callback registered.
  - `sceImpose::0x36AA6E91`: language mode stub.
  - `sceUtility::0x2A2B3DE0`: utility net module load stub.
- Removal Trigger: Implement complete multithreaded event flag wait lists, real kernel callback dispatcher, and modular LW-mutex synchronization once deeper gameplay threads execute.





