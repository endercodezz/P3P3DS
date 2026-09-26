# PSP runtime reference for the current P3P frontier

Scope: ULUS-10512, P3P3DS base `881fc223d7f7e875bbc9df914a3ccaeaafb3e9b1`. Research only; no runtime changes or new execution run. The companion [gap analysis](PSP_RUNTIME_GAP_ANALYSIS.md) applies these semantics to the current implementation.

## Evidence conventions and source index

`[VERIFIED]` means inspected source or local evidence, **not** a new PSP hardware measurement. Each paragraph/table row inherits its stated evidence category: **DOCUMENTED PSP BEHAVIOR**, **REFERENCE IMPLEMENTATION BEHAVIOR**, **P3P3DS ASSUMPTION**, or **UNKNOWN / NOT YET VERIFIED**. `[INFERRED]` and `[UNVERIFIED]` retain that distinction. Paths below are repository-relative; symbols identify the relevant implementation. Snapshot provenance is in [UPSTREAMS.md](UPSTREAMS.md).

- **W-GE:** [psdevwiki Graphics Engine](https://www.psdevwiki.com/psp/Graphics_Engine), command table. Used for command format and opcode cross-checks; detailed masks below come from source, not extrapolation from the wiki.
- **W-MEM:** [psdevwiki Tachyon, Memory mapping](https://www.psdevwiki.com/psp/Tachyon#Memory_mapping). The separate `/psp/Memory_map` URL was inaccessible; no claims depend on that page.
- **SDK-GE:** PSPSDK `psp/pspsdk/src/ge/pspge.h`, `PspGeListArgs`, `PspGeCallbackData`, enqueue/sync declarations.
- **SDK-D:** PSPSDK `psp/pspsdk/src/display/pspdisplay.h`, display enums/API declarations; `sceDisplay.S`, import NIDs.
- **SDK-GU:** PSPSDK `psp/pspsdk/src/gu/`, `sceGuDrawBuffer.c`, `sceGuDepthBuffer.c`, `sceGuClear.c`, functions of the same names.
- **P-MEM:** PPSSPP `references/ppsspp/Core/MemMap.cpp:67-113`, memory views; `Core/MemMap.h`, `PSP_GetKernelMemoryEnd`, `PSP_GetVolatileMemoryStart`, `PSP_GetUserMemoryBase`, size constants.
- **P-GE:** PPSSPP `references/ppsspp/GPU/ge_constants.h`, `GECommand` and `GE_VTYPE_*`; `GPU/GPUState.h:212-238`, buffer/clear getters; `GPU/GPUState.h:684`, `GPUgstateCache::getRelativeAddress`; other named state getters in the same file.
- **P-EXEC:** PPSSPP `references/ppsspp/GPU/GPUCommon.cpp`, `EnqueueList`, `UpdateStall`, `ListSync`, `DrawSync`, `Execute_End`, `Execute_OffsetAddr`, `Execute_Vaddr`, `Execute_Iaddr`, `Execute_Jump`, `Execute_BJump`, `DoExecuteCall`, `Execute_Ret`, `Execute_BoundingBox`. `GPU/Software/SoftGpu.cpp`, `SoftGPU::Execute_Prim`; `GPU/Software/Rasterizer.cpp`, depth/color rasterization paths.
- **U-GE:** uOFW `references/uofw/src/kd/ge/ge.c:2343`, `sceGeListSync`; `sceGeDrawSync`, `_sceGeListEnQueue`, `sceGeBreak`, `sceGeContinue`, `sceGeSetCallback`, `sceGeUnsetCallback`; `src/kd/ge/stall.S`, `sceGeListUpdateStallAddr`.
- **T-GE:** pspautotests `references/pspautotests/tests/gpu/ge/`: `queue.cpp/.expected`, `break.cpp/.expected`, `edram.cpp/.expected`; `tests/gpu/displaylist/state.c`, `testGeCallbacks`. Expected results were inspected, not rerun on hardware.
- **T-D / P-D:** pspautotests `references/pspautotests/tests/display/setframebuf.cpp/.expected`, named sync/stride/address/latch cases; PPSSPP `references/ppsspp/Core/HLE/sceDisplay.cpp`, `sceDisplaySetFramebuf`, `__DisplaySetFramebuf`, `sceDisplayGetFramebuf`, latched framebuffer application in vblank handling.
- **U-S / P-S / T-S:** uOFW `references/uofw/src/kd/sysmem/partition.c:521`, `sceKernelAllocPartitionMemory`, `sceKernelAllocPartitionMemoryForUser`; `memory.c:494`, `_allocSysMemory`, and free/get-head functions. PPSSPP `references/ppsspp/Core/HLE/sceKernelMemory.cpp:814-930`, `PartitionMemoryBlock`, allocation/free/query APIs; `Core/Util/BlockAllocator.cpp`, `AllocAligned`, `AllocAt`, `Free`. pspautotests `references/pspautotests/tests/sysmem/partition.c/.expected`, named partition/type/alignment/position/free cases.
- **K:** PSPSDK `psp/pspsdk/src/user/pspthreadman.h`, encountered APIs; PPSSPP `references/ppsspp/Core/HLE/sceKernelEventFlag.cpp`, `__KernelCheckEventFlagMatches`, `__KernelApplyEventFlagMatch`, `sceKernelClearEventFlag`; `sceKernelThread.cpp`, `sceKernelChangeCurrentThreadAttr`, `sceKernelDelayThread`, `sceKernelCreateCallback`; `sceKernelMutex.cpp`, `sceKernelLockLwMutex`, `sceKernelTryLockLwMutex`, `sceKernelUnlockLwMutex`; `sceKernelTime.cpp:81`, `sceKernelGetSystemTimeLow`; uOFW `references/uofw/src/kd/sysmem/intr.S`, `suspendIntr`, `resumeIntr`.
- **E:** local executable and existing trace evidence, with fingerprints and bounded reproduction procedure in the gap analysis, section “Evidence actually examined”.

## A — PSP memory

**DOCUMENTED PSP BEHAVIOR [VERIFIED, W-MEM]; REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, P-MEM]:** The normal 32 MiB main-RAM window is `[0x08000000,0x0A000000)`. Its ordinary user portion begins at `0x08800000` (24 MiB). The wiki labels the preceding 8 MiB “kernel”; PPSSPP subdivides it into kernel `[0x08000000,0x08400000)` and volatile `[0x08400000,0x08800000)`. These descriptions differ in granularity: the volatile region is not automatically ordinary user heap. Scratchpad is 16 KiB at `[0x00010000,0x00014000)`. EDRAM is separate, 2 MiB at `[0x04000000,0x04200000)`.

**REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, P-MEM]:** PPSSPP supports 32/64 MiB model-dependent RAM backing; extra physical RAM does not establish that this P3P execution needs an extended user partition. **P3P3DS ASSUMPTION [INFERRED, E]:** Retain the current 32 MiB configuration for this milestone. No observed allocation requires a larger model. **UNKNOWN [UNVERIFIED]:** No claim here of exhaustive P3P model compatibility. SDK-GE documents a larger selectable EDRAM mode on newer models; T-GE's baseline reports `0x00200000`. Do not expand the observed EDRAM mapping on that basis.

**REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, P-MEM]:** Cached user RAM `0x08xxxxxx` and uncached user RAM `0x48xxxxxx` share storage; PPSSPP also maps kernel RAM aliases `0x88xxxxxx` and `0xC8xxxxxx`. `0x40000000` is an alias-selection bit, not a new RAM allocation or a valid buffer by itself. Observed `0x48D14600` corresponds to `0x08D14600`; VRAM `0x44088000` corresponds to `0x04088000`. Address translation and cache coherency are separate: shared host bytes do not model dirty CPU cache lines, writeback, or GE visibility barriers.

**REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, P-MEM]:** VRAM has additional windows at `0x04200000`, `0x04400000`, `0x04600000` and uncached counterparts. PPSSPP explicitly warns that its flat mirrors do not correctly model swizzled mirror behavior. Its memory helpers also distinguish VRAM from kernel-flagged RAM. Consequently, `address & 0x1FFFFFFF` plus modulo-2-MiB backing is adequate for the observed primary/uncached addresses, not a universal PSP MPU/cache/VRAM translation specification.

**P3P3DS OBSERVATION [VERIFIED, E]:** `0x08800000` is the ordinary user-memory lower bound; `0x08804000` is this executable's relocated load base, not a hardware memory-region boundary. ELF PT_LOAD ranges, exclusive end, are `[0x08804000,0x08BB58F4)` and `[0x08BB5900,0x08EC0F7C)`; entry is `0x08804108`. Application heap and thread stacks must avoid loaded code/data/BSS and each other. `0x08000000` backing existing in GuestMemory is not permission for user SysMem to allocate the kernel partition.

## B — GE command processor (priority)

**DOCUMENTED PSP BEHAVIOR [VERIFIED, W-GE; corroborated P-GE]:** A little-endian 32-bit command word has opcode `word >> 24` and argument `word & 0x00FFFFFF`. Most commands change persistent GE state; a command named “framebuffer” is not itself a drawing operation.

### Address and flow commands

The following rows are **REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, P-GE/P-EXEC]**, with opcode names cross-checked against W-GE. No row writes EDRAM merely by executing, except the explicitly marked draw operations.

| Opcode | Argument/state and effect | BASE/OFFSET and flow |
|---|---|---|
| `00` NOP | No operation. | Advance one word. |
| `10` BASE | Relevant address high nibble is argument bits 16–19. | `basePart=(arg & 0x000F0000)<<8`. Does not retroactively change a resolved VADDR. |
| `13` OFFSETADDR | Offset register becomes `arg<<8` (32-bit arithmetic). | Added after BASE/low-address composition. |
| `14` ORIGIN | Offset becomes this command's list PC. | Argument is not a pointer. Not present in the captured dynamic list. |
| `01` VADDR / `02` IADDR | Resolve and store vertex/index address. | `((basePart | arg)+offset)&0x0FFFFFFF`. No vertex fetch implies a draw by itself. |
| `08` JUMP / `09` BJUMP | Branch address uses `arg & 0x00FFFFFC`; BJUMP branches when bounding-box test fails. | Same relative reconstruction; change list PC. |
| `0A` CALL / `0B` RET | CALL pushes return PC and offset; RET restores them. | Ordinary CALL does **not** save/restore BASE in PPSSPP. RET is not FINISH. |
| `07` BOUNDINGBOX | Low 16 bits give vertex count; evaluate visibility for BJUMP, no color/depth rasterization. | Uses vertex type/address; count zero resets test false in PPSSPP. |
| `0F` FINISH / `0C` END | FINISH marks drawing completion/interrupt token; END stops command reading. | Normal list terminator is FINISH followed by END. See section C; neither creates pixels. |
| `0E` SIGNAL | Behavior in argument bits 16–23, token in low 16 bits. | SIGNAL/END can invoke a handler and resume; END is not unconditionally queue completion. Not observed in the dynamic list. |
| `04` PRIM | Count = bits 0–15; primitive type = bits 16–18 (0 points, 1 lines, 2 line strip, 3 triangles, 4 strip, 5 fan, 6 rectangles/sprites). | Consumes vertex/index streams and **can write color/depth**. |
| `05` BEZIER / `06` SPLINE | U/V control-point counts in low/next byte; SPLINE also has edge-type bits at 16–19. | Uses vertex and patch state; **can rasterize**. Neither occurs in inspected lists; defer tessellation. |

**REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, P-GE `GE_VTYPE_*`]:** `12` VERTEXTYPE: texture-coordinate type bits 0–1; color 2–4; normal 5–6; position 7–8; weights 9–10; index type 11–12; weight-count-minus-one 14–16; morph-count-minus-one 18–20; through/2D bit 23. Field sizes and alignment determine vertex stride; never assume a C struct until the first real draw's type is captured. VADDR/IADDR plus PRIM count control stream consumption, not a fixed framebuffer-sized memory copy. P-EXEC `SoftGPU::Execute_Prim` advances the internal vertex address for non-indexed draws or index address for indexed draws without changing the visible VADDR/IADDR registers.

### Render targets: do not use BASE

**DOCUMENTED PSP BEHAVIOR [VERIFIED, SDK-GU/W-GE]:** `9C` FRAMEBUFPTR/`9D` FRAMEBUFWIDTH and `9E` ZBUFPTR/`9F` ZBUFWIDTH are pointer/width pairs. SDK-GU sends low pointer bits in the pointer command and pointer high byte in width argument bits 16–23. The nominal split-pointer composition is `(ptrArg & 0xFFFFFF) | ((widthArg & 0xFF0000)<<8)`. This is **not** VADDR's BASE/OFFSET mechanism.

**REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, P-GE `getFrameBufRawAddress`, `getDepthBufRawAddress`]:** The inspected PPSSPP deliberately ignores framebuffer/depth high pointer bits and constrains rendering to EDRAM: raw offset `ptrArg & 0x1FFFF0`, stride `widthArg & 0x7FC`. It returns `0x44000000 | offset` for color and `0x44600000 | offset` for depth (a mirror in its own memory model). **Source limitation:** the wiki's upper-byte description alone does not specify effective render-target routing; do not infer that setting those bits permits rendering into RAM. Exact mirror/swizzle behavior remains outside this verified first-write subset.

**P3P3DS OBSERVATION [VERIFIED, E + P-GE]:** Both observed width arguments are `0x000200`; color pointer argument is zero and depth pointer argument is `0x110000`. Thus the primary backing locations are color `0x04000000`, depth `0x04110000`; **adding BASE would be wrong**. FRAMEBUFPTR is a target offset in this observed encoding, not a complete absolute CPU address. Format `D2` uses low two bits: 0 RGB565, 1 RGBA5551, 2 RGBA4444, 3 RGBA8888. Depth samples are 16-bit (P-EXEC software depth stores).

### State necessary to understand the captured setup and first draw

All rows are **REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, P-GE named getters and enums; SDK-GU `sceGuClear`]**. These commands alone do **not** write color/depth EDRAM and normally advance the list PC.

| Commands | Argument interpretation / dependency |
|---|---|
| `42–44` viewport scale, `45–47` center | Float24: reinterpret `arg<<8` as IEEE float32. X/Y/Z transformation state. |
| `4C/4D` screen offset | Low 16 bits, 12.4 fixed point; subtract from screen coordinates. Distinct from OFFSETADDR. |
| `15/16` REGION1/2, `D4/D5` SCISSOR1/2 | X in low 10 bits, Y in next 10; scissor upper bound is inclusive. REGION1 is not simply another raster scissor origin: PPSSPP `getRegionRateX/Y` describes nonzero values as affecting drawing rate, while `getRegionX1/Y1` supplies bounding-box bounds. Keep its observed zero reset. `15` is not BOUNDINGBOX (`07`), and `16` is not VSCX (`F0`). |
| `D6/D7` MINZ/MAXZ | Low 16-bit depth range bounds; no clearing. |
| `D3` CLEARMODE | Bit 0 enables clear drawing; bits 8/9/10 select RGB/stencil-alpha/depth writes. Needs a primitive. |
| Clear color / clear depth | No standalone GE register that fills the target with a clear constant. SDK-GU stores CPU-side clear values then emits colored vertices with Z, CLEARMODE, PRIM sprites, and CLEARMODE off. |
| `21`, `DF`, `E0/E1` blending | Enable bit 0; source factor bits 0–3, destination 4–7, equation 8–10; fixed RGB factors in low 24 bits. Clear uses its special write semantics. |
| `23`, `DE`, `E7`, `E8/E9` | Depth-test enable, low-three-bit depth comparison, bit-0 depth-write disable, RGB/alpha write masks (set mask bits inhibit normal writes). |
| `1E` texture enable; `A0–A7` texture pointers; `A8–AF` texture widths | Texture pointers use `(ptr&0xFFFFF0)|((width<<8)&0x0F000000)`, **not BASE/OFFSET**. Width supplies row layout; interpretation also depends on format. |
| `B8–BF` texture sizes; `C2/C3` mode/format | Width/height are `1<<(arg&15)` and `1<<((arg>>8)&15)`; mode includes swizzle bit 0 and maximum mip level bits 16–18; format low nibble: 0–3 direct color, 4–7 indexed, 8–10 compressed DXT. Indexed sampling needs CLUT state. |
| `C6/C7/C9`, `48–4B` | Filter min bits 0–2/mag bit 8; wrap S bit 0/T bit 8; texture function bits 0–2, alpha bit 8, doubling bit 16; float24 UV scale/offset. |
| `36`, `53`, `5B`, `E2–E5` | Patch U/V division low 7 bits of each byte; material color-update mask low 3 bits; float24 specular coefficient; four rows of signed 4-bit dither coefficients. Observed dynamic setup only changes state. |

**REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED]:** Texture row width is not the logical power-of-two image width. PPSSPP `references/ppsspp/GPU/Common/TextureDecoder.cpp::GetTextureBufw` masks the width argument to 11 bits and aligns uncompressed rows down to 16-byte units according to bits per pixel (8888 mask `0x7FC`, 16-bit mask `0x7F8`, CLUT8 mask `0x7F0`, CLUT4 mask `0x7E0`); a zero result receives a minimum 16-byte row. DXT uses mask `0x7FF`. This is a reference implementation rule, not independently measured here; the function's special emulator-atlas exception is irrelevant to P3P and must not be copied.

**REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, P-GE `GE_CMD_TRANSFER*`]:** A transfer trigger (`EA` TRANSFERSTART) can write memory without triangle rasterization, using source/destination pointer/stride (`B2–B5`), positions (`EB/EC`) and size (`EE`). No `EA` is present in either inspected P3P list. CPU stores are another independent VRAM-write source. Defer transfer implementation unless it is the first actual writer encountered.

## C — Display-list execution and synchronization

**DOCUMENTED PSP BEHAVIOR [VERIFIED, SDK-GE]; REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, U-GE/P-EXEC/T-GE]:** Enqueue returns a list handle, not a completion status. Preserve list PC, stall address, queue order, callback association, return stack, lifecycle and optional context. EnQueue appends; EnQueueHead inserts before existing work but is not an arbitrary preempt-running-list operation: the inspected implementation requires an existing current list to be paused, and the inserted head is paused until continued. T-GE `queue.expected` confirms rejection of head insertion over a stalled, unpaused list.

**REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, P-EXEC `ProcessDLQueue`/`UpdateStall`, U-GE `stall.S`]:** Stall stops **before fetching/executing the word at that address**. A null stall disables the stop. A CPU producer may write as-yet-unconsumed list storage while GE is stopped and advance the stall after making that storage visible. Do not execute the zero-filled tail or read through the stall merely because RAM exists there. Updating a live list changes its stall and can resume it; updating an invalid/completed handle must report an error. No blanket guarantee exists for modifying already-consumed commands or in-flight vertex data. Cache publication requirements remain separate from pointer aliases.

**REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, U-GE `sceGeListSync`/`sceGeDrawSync`, P-EXEC]:** Sync mode 0 waits for completion; mode 1 polls without waiting. Status values are 0 completed, 1 queued, 2 drawing, 3 stalled, 4 paused. SDK-GE names 2 “DRAWING_DONE” and 4 “CANCEL_DONE”; these names conflict with the actual state-machine meanings in uOFW/PPSSPP. Invalid modes/handles and forbidden wait contexts are errors. DrawSync concerns queued drawing as a whole; ListSync concerns one handle. Success is permissible after genuinely completing synchronous work, not as a replacement for executing it.

**REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, P-EXEC `Execute_End`, U-GE, T-GE `state.c`]:** The usual FINISH (`0F`) / END (`0C`) pair completes a list and delivers its finish event/callback in order. SIGNAL/END can instead pause/call/resume; treating every END as full completion loses that distinction. FINISH is not a framebuffer fill, swap, or PRIM. A backend may flush outstanding draws when finishing, but those earlier draws caused the writes. The firmware's interrupt/queue bookkeeping and callback completion also matter before advancing subsequent work.

**DOCUMENTED PSP BEHAVIOR [VERIFIED, SDK-GE `PspGeCallbackData`]; REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, U-GE `sceGeSetCallback`, P-EXEC]:** GE callbacks register distinct signal/finish function pointers and arguments, then associate their returned ID with lists. GE interrupt callbacks are not ThreadMan callbacks serviced only by `*CB` waits. A hardcoded callback ID without a stored function cannot deliver completion to P3P.

**P3P3DS ASSUMPTION [INFERRED, above sources]:** A synchronous minimal executor can commit CPU-visible GuestMemory color/depth writes before reporting completion. It need not imitate PPSSPP's asynchronous GPU timing to produce the first write. It must retain a stalled/running distinction and must not claim a wait completed while the list remains stalled. Exact cache/timing fidelity is **UNKNOWN [UNVERIFIED]**, not an excuse to return success early.

## D — PSP display

**DOCUMENTED PSP BEHAVIOR [VERIFIED, SDK-D]; REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, T-D/P-D]:** SetMode chooses mode/visible dimensions; SetFrameBuf chooses the scanout address, row stride in pixels, format and latch mode. It neither executes GE nor copies/clears framebuffer memory. GetFrameBuf mode 0 queries current scanout, mode 1 the latched choice. Sync 0 is SDK “next hsync”, traditionally named immediate; sync 1 latches the address for next vsync. Setting the buffer is not itself a vblank wait. Vcount counts display periods, not calls to GetVcount. WaitVblank may return during a current vblank; WaitVblankStart waits for a start; CB variants additionally service ordinary callbacks.

**Source qualifications [VERIFIED, T-D/P-D versus SDK-D]:** The SDK comment says stride must be a power of two, but hardware expected cases accept 448 and 768 (multiples of 64). Do not enforce the comment as a complete rule. T-D accepts 16-byte-aligned framebuffer addresses in VRAM or RAM and uncached VRAM, rejects scratchpad/misalignment. It also shows format/stride changes via latched mode affecting current parameters immediately, while the address remains deferred. An implementation with two entire framebuffer structs latched identically would miss that nuance. Sync-0 format/stride changes have additional restrictions against the latched configuration.

**P3P3DS OBSERVATION [VERIFIED, E/SDK-D]:** `sceDisplaySetFrameBuf(0x04088000,512,3,1)` selects the second EDRAM color region for next-vsync scanout, stride 512, 32-bit RGBA8888. With 480×272 mode, visible pixels occupy the first 480 pixels of each row; full storage is `512*272*4=0x88000` bytes. Little-endian 8888 storage orders bytes R,G,B,A. Draw target and scanout target are independent; selecting buffer 1 does not redirect GE away from buffer 0. Double buffering requires the game to render one target and later select it for display.

## E — SysMem needed now

**DOCUMENTED PSP BEHAVIOR [VERIFIED, PSPSDK `src/user/pspsysmem.h`]; REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, U-S/P-S/T-S]:** Partition 2 is ordinary user allocation. User access to partition 1 is rejected in T-S; partition IDs are not interchangeable heap labels. T-S allows 5/6 in its test configuration, which does not establish their availability/size in every model or mode. Allocations return a UID, and GetBlockHeadAddr resolves it to the start address.

The placement rules below are **REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, U-S `_allocSysMemory`, P-S `AllocAligned`, T-S]**, on available free intervals `[L,H)` with 256-byte allocation granularity:

| Type | Placement |
|---|---|
| 0 Low | Lowest suitable free space, size rounded up to 256 bytes, base at allocation granularity. |
| 1 High | Highest suitable free space, placing the rounded allocation at the high end. |
| 2 Addr | Reserve the requested range at 256-byte granularity, only if available. uOFW rounds start down and expands coverage through the rounded-up requested end. |
| 3 LowAligned | Search upwards for `alignUp(L,max(256,alignment))`; rounded size must fit. |
| 4 HighAligned | Search downwards for `alignDown(H-roundedSize,max(256,alignment))`; it must remain in the free interval. |

**REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, U-S/P-S]:** Aligned types require a nonzero power of two; small valid alignments remain subject to allocation granularity. **Disagreement:** PPSSPP's PartitionMemoryBlock masks an Addr request down before `AllocAt`, whereas uOFW explicitly expands size to cover an unaligned original interval. Do not copy PPSSPP's exact-address shortcut as proven firmware behavior; test an unaligned request that crosses a 256-byte boundary if implementing it.

**REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, T-S named cases]:** Zero/oversized/unavailable allocations return `0x800200D9`; invalid type `0x800200D8`; invalid alignment `0x800200E4`; forbidden partition 1 `0x800200D6`; out-of-range user partition `0x800200D2`; null name `0x80020001`. Validation precedence is contextual, not inferred from this list. Free invalid/deleted UID returns `0x800200CB`; get-head invalid/deleted UID returns zero. Successful free must restore allocatable space; total-free sums free intervals and max-free measures the largest suitable contiguous extent (U-S/P-S).

**P3P3DS OBSERVATION [VERIFIED, E]:** Current request is partition 2, LowAligned, 16 MiB, 4096 alignment. **P3P3DS ASSUMPTION [INFERRED]:** Its successful placement suffices to continue the current bootstrap while monitoring future allocations/stacks; it does not justify claiming a correct general allocator.

## F — Encountered kernel primitives only

All items are **DOCUMENTED PSP BEHAVIOR / REFERENCE IMPLEMENTATION BEHAVIOR [VERIFIED, K]**, not measured scheduling fidelity:

- Event flags keep a UID, attributes and bit pattern. Set ORs bits; Clear performs `pattern &= bits` (not `&= ~bits`). AND waits require all requested bits; OR waits require any. Successful wait/poll returns the actual pattern before optional clear-matched (`0x20`) or clear-all (`0x10`). Unsatisfied poll fails; wait blocks with timeout/callback semantics. Always returning the requested pattern fabricates an event.
- ThreadMan callbacks retain name/function/user argument and notification state; creation alone does not execute them. Ordinary callback-aware waits and notification delivery differ from GE interrupt callbacks.
- LW mutexes maintain workarea UID/owner/count and recursive attributes. TryLock must fail if unavailable; Lock may wait; Unlock validates ownership/count and wakes eligible waiters. Success without workarea updates is not a lock even on a single host thread.
- Dispatch correction within the encountered Kernel_Library surface: `1839852A` is **sceKernelMemcpy**, not TryLock. It copies the requested bytes and returns the destination; treating it as lock success loses data. Sources: PPSSPP `references/ppsspp/Core/HLE/sceKernelInterrupt.cpp`, Kernel_Library registration and `sceKernelMemcpy`; uOFW `references/uofw/src/kd/usersystemlib/exports.exp:14`. PSPSDK `psp/pspsdk/src/user/Kernel_Library.S` identifies TryLock as `DC692EE3`.
- ChangeCurrentThreadAttr performs `(attr & ~clear)|set`; inspected PPSSPP restricts changes to the VFPU attribute and rejects other bits. PSPSDK's older declaration calls the first parameter “unknown”; use reference behavior, not that name, as the working interpretation.
- GetSystemTimeLow is low 32 bits of microsecond system time; DelayThread blocks the caller for a microsecond interval and allows scheduling. A clock advancing only when queried and a no-op delay cannot support elapsed-time waits.
- Interrupt suspend saves the prior enable state and disables interrupts; resume restores that state (uOFW assembly). Nested suspend cannot always return “previously enabled”. This must coordinate with GE interrupt delivery when callbacks are implemented.

**UNKNOWN / NOT YET VERIFIED [UNVERIFIED]:** Whether the next P3P writer depends on a finish callback, event flag, elapsed-time loop, or mutex handoff is not established at the missing-function frontier. Implement only the dependency actually encountered; do not replace the scheduler wholesale.
