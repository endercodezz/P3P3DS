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

Research PSP API semantics in this order when available:

1. `references/uofw/src/` and `references/uofw/include/` — kernel behavior and contracts.
2. `psp/pspsdk/src/` — public prototypes, structs and NID tables.
3. `references/pspautotests/tests/` — focused cases and recorded expected behavior.
4. `references/ppsspp/Core/HLE/` — supplementary behavioral reference only.

Do not copy or line-by-line translate PPSSPP implementation code. Record source disagreements and resolve them against observed guest/hardware evidence, not convenience.

Other implementation examples (not replacements for PSP contract research):

- `recomp/PSPRecomp/profiles/vcs/host/vcs_profile.cpp` — Proven minimal HLE host implementation.
- `recomp/PSP-recompilation-project/src/rt/hle.c` — Clean, lightweight pure C HLE dispatch table.

---

## 3. Implementation Workflow per NID

When a missing NID call is logged by the runtime:

1. **Prove the observed call:** Record guest/stub PC, executed caller, RA/return PC, relevant argument registers and pointed-to data, current thread/UID and state. Inspect the surrounding guest/AOT instructions to establish how the caller uses the return value. Do not implement from the API name alone.
2. **Establish the contract:** Use the ordered references above to identify only the arguments, return value, relevant errors and externally observable state changes needed by this call.
3. **Locate focused tests:** Compare applicable pspautotests cases and expected results; distinguish source inspection from actually running hardware tests.
4. **Bound the implementation:** Document unsupported behavior explicitly. Do not prebuild a complete subsystem or use unconditional success to bypass missing semantics.
5. **Implement Minimal Valid Behavior:**
   - Read arguments from guest memory / registers.
   - Perform the required state change.
   - Return the contract's actual result (status, UID, count, boolean or error); success is not universally zero.
   - Set return value into register $v0.
6. **Classify Implementation State:**
   - `[STUB]`: Returns a fixed dummy value (e.g. 0) with a warning log. *A stub is never counted as implemented.*
   - `[PARTIAL]`: Implements common code paths but ignores complex flags or edge cases.
   - `[IMPLEMENTED]`: Full functional implementation matching PSP specifications.
   - `[VERIFIED]`: Passed deterministic unit tests or hardware autotests.
7. **Verify and trace:** Add focused regression tests and bounded diagnostics with thread UID, caller, arguments, result and return PC. Confirm actual guest continuation, not just entry into the HLE handler. Follow the repository's verification/commit/stop workflow in `AGENTS.md`, rather than expanding the next blocker.

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
Reference:      <contract/test source path and line or symbol>
Behavior:       <Summary of parameters handled and return value set>
```
