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

- `recomp/PSPRecomp/profiles/vcs/host/vcs_profile.cpp` — another game's HLE host in the upstream PSPRecomp tree; not used by the P3P3DS build. The project's HLE is `core/src/hle/`.
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

## 4. P3P3DS Implementation Mechanics

**Name the NID first:** `bash .claude/skills/psp-hle-runtime/scripts/nid_lookup.sh 0xNNNNNNNN` prints the PSPSDK name, uOFW export/source files, pspautotests sources with `.expected` hardware output, and whether `core/` already registers it. NIDs absent from PSPSDK (e.g. `sceMpegAvcDecodeFlush` 0x4571CC64) are named from the pspautotests sources' `extern` declarations.

**Where code goes:** one file per module in `core/src/hle/<module>.cpp` with `register_<module>_module(Runtime&, KernelState&)`, called from `core/src/hle/hle_modules.cpp`; module state lives in `KernelState` (`kernel_state.hpp`, accessor like `kernel.sas()` / `kernel.mpeg()`). Small singletons go in `core/src/hle/utils.cpp`. Handler shape:

```cpp
runtime.register_hle("sceFoo", 0x12345678u, [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
    const auto arg0 = ctx.gpr[4];                  // a0..a3 = gpr[4..7], then t0.. = gpr[8..]
    if (!rt.memory().contains(arg0, 4u)) { ctx.set_gpr(2, kErrorCode); return; }
    rt.event("foo_call", {{"arg", arg0}});         // bounded diagnostics
    ctx.set_gpr(2, result);                         // $v0
});
```

**Blocking services:** never spin or fake completion. Use `kernel.threads().block_current(rt, ctx, WaitInfo{...})` (wait queues, deadlines on the virtual clock, +1 us per HLE call); a wait that must re-run the service when woken sets `retry` and resumes at the import stub. Callbacks run only at CB waits / `sceKernelCheckCallback`.

**Calling guest code from a service** (ringbuffer/read callbacks): `kernel.threads().call_guest(rt, ctx, resume, function, {a0, a1, a2}, gp, then)` runs the guest function on the calling thread (it may block, e.g. in `sceIoRead`) and invokes `then(rt, ctx, v0)` on return, which sets the result or chains another call (see `put_step` in `core/src/hle/mpeg.cpp`). Interrupt-context handlers (GE finish callbacks) cannot block and run isolated via `rt.invoke_isolated_aot`.

**Tests:** one `tests/test_<module>.cpp` per module, registered in `CMakeLists.txt` (`SKIP_RETURN_CODE 77` when it needs UMD/assets that may be absent). Harness pattern (copy from `tests/test_misc_hle.cpp`): `register_all_hle_modules`, `init_root_thread`, then call the import with `rt.invoke_import(lib, nid, ctx)` after setting `gpr[4..]`, `gpr[31]`=return PC and `pc`=fake stub; when the service starts a guest call, dispatch with `rt.invoke_isolated_aot(ctx.pc, ctx)` until `pc` returns (guest callbacks can be host functions registered with `rt.register_function`).

**Strongest evidence available: replay the hardware transcript.** When pspautotests has a `.c` + `.expected` pair, reimplement the test's calls in C++ and print lines in the exact `checkpoint` format, then compare line by line with the `.expected` file (strip the `[x] `/`[r] ` prefix and CRLF; normalize heap addresses that differ). `tests/test_mpeg.cpp` matches all 2,905 lines of `video/mpeg/basic.expected` this way; `tests/test_sascore.cpp` matches 53 ADSR traces. Mismatching lines are research leads (the AVC attribute turned out to be the slice `nal_ref_idc`).

After implementing, rerun the game with the `p3p-run-triage` skill and confirm the guest continued past the call.

## 5. Critical Subsystem Checklists

- **ThreadMan (`sceKernelCreateThread`, `sceKernelStartThread`, `sceKernelDelayThread`):**
  - Thread priorities are inverted (0 = highest, 127 = lowest; default user thread is 32).
  - Delay units are microseconds on the deterministic virtual clock (`ThreadManager::now()`), never the host clock.
- **Event Flags (`sceKernelCreateEventFlag`, `sceKernelWaitEventFlag`, `sceKernelSetEventFlag`):**
  - Verify wait modes: `PSP_EVENT_WAITAND` (0x00), `PSP_EVENT_WAITOR` (0x01), clear on exit (`PSP_EVENT_WAITCLEAR` 0x20).
- **File I/O (`sceIoOpen`, `sceIoRead`, `sceIoClose`):**
  - Route through the P3P3DS Virtual File System (VFS).
  - Return distinct integer file descriptors (UIDs), tracking open offsets and file sizes.
- **Audio Synthesizer (`sceSasCore`):**
  - Used for P3P sound effects and menu clicks. Must process ADPCM voices and render 16-bit PCM blocks.
- **Implemented so far** (see `docs/CURRENT_STATE.md` for status and evidence): SysMem, ThreadMan (threads, semaphores, event flags, mutexes, lwmutexes, callbacks, delays, vblank waits), IoFileMgr over ISO9660/host VFS with the CWCheat mod chain, ModuleMgr (unencrypted PRX), sceAudio, sceSasCore, sceCtrl, sceUmd, sceDisplay, sceGe (+ software renderer), sceMpeg (demux; decoding stubbed behind `MpegDecoder`), UtilsForUser, sceDmac, sceSuspendForUser.

---

## 6. Expected Output Format

When implementing or documenting an HLE function:

```text
Module:         <e.g. IoFileMgrForUser | ThreadManForUser>
Function:       <Function Name> (NID: 0xXXXXXXXX)
Call Site:      PC=0x088xxxxx ($ra=0x088xxxxx)
Implementation: [STUB | PARTIAL | IMPLEMENTED | VERIFIED]
Reference:      <contract/test source path and line or symbol>
Behavior:       <Summary of parameters handled and return value set>
```
