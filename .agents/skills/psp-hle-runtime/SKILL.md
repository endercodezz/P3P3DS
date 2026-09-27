---
name: psp-hle-runtime
description: Guide implementation of High-Level Emulation (HLE) modules for Sony PSP system calls, kernel threading, memory management, and hardware interfaces needed by P3P.
---

# PSP HLE Runtime Implementation Workflow

Use this skill when:
- Resolving PSP module imports (NIDs) encountered during P3P execution;
- Implementing kernel primitives (threads, mutexes, semaphores, event flags);
- Managing guest memory partitions and allocations (`SysMemUserForUser`);
- Handling file system I/O calls (`IoFileMgrForUser`);
- Implementing controller inputs (`sceCtrl`), display timing (`sceDisplay`), or audio (`sceAudio`, `sceSasCore`, `sceAtrac3plus`).

---

## 1. Core Rule: DO NOT IMPLEMENT ALL OF PSP

Implement **only the specific APIs imported and called by Persona 3 Portable**.
Do not copy hundreds of unused stubs from general-purpose emulators.

---

## 2. Key References

- `references/ppsspp/Core/HLE/` — Primary reference for exact functional semantics and return codes.
  - `sceKernelThread.cpp`, `sceKernelMemory.cpp`, `sceIo.cpp`, `sceDisplay.cpp`, `sceCtrl.cpp`, `sceSas.cpp`.
- `references/uofw/src/` — Reverse-engineered Sony kernel sources for low-level edge cases.
- `psp/pspsdk/include/` — Official C function prototypes, structs, and NID tables.
- `references/pspautotests/tests/` — Test cases for validating edge behavior.
- `recomp/PSPRecomp/profiles/vcs/host/vcs_profile.cpp` — Proven minimal HLE host implementation.
- `recomp/PSP-recompilation-project/src/rt/hle.c` — Clean, lightweight pure C HLE dispatch table.

---

## 3. Implementation Workflow per NID

When a missing NID call is logged by the runtime:

1. **Verify Usage:** Confirm that P3P actually calls this NID (check call-site PC and arguments in registers $a0–$a3).
2. **Lookup Prototype:** Find function signature and parameter types in `psp/pspsdk/include/`.
3. **Inspect Semantics:** Read the corresponding function in `references/ppsspp/Core/HLE/` and check error return codes.
4. **Locate Autotests:** Check if a test exists in `references/pspautotests/tests/` for this subsystem.
5. **Implement Minimal Valid Behavior:**
   - Read arguments from guest memory / registers.
   - Perform the required state change.
   - Return 0 (`SCE_KERNEL_ERROR_OK`) on success or specific negative error code on failure.
   - Set return value into register $v0.
6. **Classify Implementation State:**
   - `[STUB]`: Returns a fixed dummy value (e.g. 0) with a warning log. *A stub is never counted as implemented.*
   - `[PARTIAL]`: Implements common code paths but ignores complex flags or edge cases.
   - `[IMPLEMENTED]`: Full functional implementation matching PSP specifications.
   - `[VERIFIED]`: Passed deterministic unit tests or hardware autotests.
7. **Add Diagnostic Logging:** Always include thread UID, calling PC, and arguments in debug traces.

---

## 4. Critical Subsystem Checklists

- **ThreadMan (`sceKernelCreateThread`, `sceKernelStartThread`, `sceKernelDelayThread`):**
  - Thread priorities are inverted (0 = highest, 127 = lowest; default user thread is 32).
  - Delay units are in microseconds (convert accurately to host clock).
- **Event Flags (`sceKernelCreateEventFlag`, `sceKernelWaitEventFlag`, `sceKernelSetEventFlag`):**
  - Verify wait modes: `PSP_EVENT_WAITAND` (0x00), `PSP_EVENT_WAITOR` (0x01), clear on exit (`PSP_EVENT_WAITCLEAR` 0x20).
- **File I/O (`sceIoOpen`, `sceIoRead`, `sceIoClose`):**
  - Route through the P3P3DS Virtual File System (VFS).
  - Return distinct integer file descriptors (UIDs), tracking open offsets and file sizes.
- **Audio Synthesizer (`sceSasCore`):**
  - Used for P3P sound effects and menu clicks. Must process ADPCM voices and render 16-bit PCM blocks.

---

## 5. Expected Output Format

When implementing or documenting an HLE function:

```text
Module:         <e.g. IoFileMgrForUser | ThreadManForUser>
Function:       <Function Name> (NID: 0xXXXXXXXX)
Call Site:      PC=0x088xxxxx ($ra=0x088xxxxx)
Implementation: [STUB | PARTIAL | IMPLEMENTED | VERIFIED]
Reference:      references/ppsspp/Core/HLE/<file.cpp:line>
Behavior:       <Summary of parameters handled and return value set>
```
