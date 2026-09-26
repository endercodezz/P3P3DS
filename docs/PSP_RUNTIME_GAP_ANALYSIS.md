# PSP runtime gap analysis: first genuine VRAM write

Base inspected: `881fc223d7f7e875bbc9df914a3ccaeaafb3e9b1`; parent `3765e9d0f1bec55fd11c068f2516b7a65eaa1e9e`. Only documentation changes belong to this task. Source keys refer to the exact paths/symbols in [PSP_RUNTIME_REFERENCE.md](PSP_RUNTIME_REFERENCE.md). Classifications apply to the stated behavior, not an entire subsystem.

## Evidence actually examined

**P3P3DS OBSERVATION [VERIFIED]:** Existing ignored `logs/p3p_bootstrap_latest.log` reports PC `0x08B19890`, RA `0x08B19BE0`, missing recompiled function, successful frontier checks and 0 nonzero bytes across all 2,097,152 VRAM bytes. This was read, not regenerated. SHA256: `542925C4B097E11F5C19B161629E1CD75034D4921BA80F3201EC576C1CDA7EA3`. Provenance of that run is the existing local log, not a fresh hardware observation.

**P3P3DS OBSERVATION [VERIFIED]:** Local `profiles/p3p/game/eboot.elf` SHA256 is `BE2ABBD43A4AE7CE5AA2F1F146083FFE0D924FC2EB1874E6FCDC21CBA40D49DB`. A bounded read parsed ELF32 program headers and inspected only the static GE array at runtime `0x08BB40D4`: PT_LOAD file offset `0xA0`, vaddr 0, so array file offset is `0xA0+(0x08BB40D4-0x08804000)=0x003B0174`. Reading little-endian words through END yields **212 words**, every argument zero; words 210/211 are FINISH/END. There are no PRIM, BEZIER, SPLINE, BOUNDINGBOX, JUMP, BJUMP, CALL, RET, SIGNAL or TRANSFERSTART commands in that array. It resets state, including zero-count palette-load/texture maintenance state; it cannot rasterize. No asset extraction or executable modification was performed.

**P3P3DS OBSERVATION [VERIFIED]:** The same ELF headers establish PT_LOAD 1 vaddr `0x003B1900`, file size `0x00085154`, memory size `0x0030B67C` (3,192,444 bytes). Its exclusive relocated end is `0x08EC0F7C`. Existing `docs/P3P_EXECUTABLE_ANALYSIS.md` states end `0x08EC0FC3`: that end address is a factual documentation discrepancy (the decimal memory size is correct), reported here without editing that file.

**P3P3DS OBSERVATION [VERIFIED, existing log + P-GE]:** Dynamic list is enqueued at `0x48D14600` with the same address as its initial stall, cb=1. Stall is later advanced to `0x48D14674`. After canonicalization, the 29-word interval is `[0x08D14600,0x08D14674)`; FINISH is at `+0x6C`, END at `+0x70`. The log's second dump starting at `+0x30` overlaps this list; it is not a third submission.

| Word indices | Observed words / decoded effect [VERIFIED, log + P-GE] |
|---|---|
| 0–3 | `E2001D0C E300F3E2 E4000C1D E500E2F3`: dither matrix. |
| 4–8 | `36001010 53000007 5B3F8000 483F8000 493F8000`: patch division 16/16, material-update mask 7, specular coefficient 1, UV scales 1/1. |
| 9–11 | `D2000001 9D0001E0 9C000000`: temporary 5551 target state, stride 480, offset 0. |
| 12–16 | `D2000003 9D000200 9C000000 9F000200 9E110000`: final 8888 color target offset 0, stride 512; depth offset `0x110000`, stride 512. |
| 17–18 | `4C007100 4D007780`: screen offsets 1808/1912. |
| 19–24 | `42437000 43C30800 45450000 46450000 44C69C40 4746EA60`: viewport X/Y scales 240/-136, centers 2048/2048, Z scale -20000, Z center 30000. |
| 25–28 | `D6002710 D700C350 0F000000 0C000000`: depth bounds 10000/50000, FINISH, END. |

**Conclusion [VERIFIED, bounded evidence above]:** Neither inspected list contains an EDRAM-producing draw/clear/transfer trigger. Dynamic setup contains **no D3 clear command, no PRIM and no scissor command**. The old trace's broad wording “viewport, scissor” is not supported by these 29 words; scissor registers are zeroed in the static reset. `platform/pc/main.cpp` also labels `15` as DRAWBOUNDINGBOX and `16` as VSCX; actual meanings are REGION1/REGION2 (P-GE/W-GE). No endian fault was found or re-investigated.

## Memory and buffer relationships

**P3P3DS OBSERVATION [VERIFIED, E + GuestMemory symbols below]:** All three observed addresses resolve to distinct offsets in one 2 MiB VRAM allocation:

| Address | Role in observed initialization | Extent, exclusive end |
|---|---|---|
| `0x04000000` | GE color draw target, RGBA8888/512 stride | `[0x04000000,0x04088000)` for 272 rows |
| `0x04088000` | Selected next-vsync display buffer | `[0x04088000,0x04110000)` for 272 rows |
| `0x04110000` | GE depth target, 16-bit/512 stride | `[0x04110000,0x04154000)` **if** used for 272 rows |

**[INFERRED]:** These contiguous allocations support double-buffered color plus depth. They are not three aliases and the depth target is not a third color buffer. Buffer 0 is the draw target in this capture; labeling it “front” is unwarranted while buffer 1 is selected for display. A write only to depth proves a VRAM write, not a visible frame.

In the following tables, implementation observations and comparisons are **[VERIFIED]** by the cited functions/source keys. “Impact” is **[INFERRED]**, bounded to first-write progress. Required fixes are recommendations, not changes made here.

| Subsystem / behavior | PSP semantics | Current P3P3DS behavior | Classification | Impact on FIRST VRAM WRITE / required fix | Source |
|---|---|---|---|---|---|
| Primary VRAM backing | 2 MiB baseline at `0x04000000` | Correct-sized zero-initialized vector; observed ranges fit | CORRECT | No mapping rewrite needed for these targets | `recomp/PSPRecomp/src/guest_memory.cpp:72`, constructor, `resolve`, `raw_pointer`; P-MEM/T-GE |
| Observed cached/uncached aliases | Same underlying RAM/VRAM | `canonical(a)=a&0x1FFFFFFF`; `48D14600 -> 08D14600`, `44088000 -> 04088000` | CORRECT | Preserve alias identity | `recomp/PSPRecomp/include/psprecomp/guest_memory.hpp`, `canonical`, `AotFastView`; P-MEM |
| Full mirror/cache/protection model | Swizzled mirrors and permissions/cache effects require distinct treatment | Four flat VRAM mirrors; broad mask; coherent host RAM; no privilege/cache model | INCOMPLETE | Not implicated by primary target addresses; test before using swizzled aliases | GuestMemory `vram_offset`, `contains`; P-MEM comments at `MemMap.cpp:67` |
| Scratchpad | 16 KiB separate region | Only RAM/VRAM regions exist | UNIMPLEMENTED | No observed scratchpad blocker; add only when used | GuestMemory `Region`, `contains`; P-MEM |

## GeManager: metadata is not GE state execution

**[VERIFIED]:** `core/include/p3p3ds/hle/ge.hpp`, `GeManager::enqueue_list`, stores only the latest `{list_address,stall_address,callback_id,opt_param}`, increments a counter and returns 1. There is no queue, execution PC, return stack, GE register file, vertex decoder, primitive dispatch, or VRAM writer. `core/src/hle/ge.cpp::register_ge_module` reads commands only to print them. It never interprets their effects. Display state is tracked separately by DisplayManager; that is not GE target state.

| Behavior | PSP semantics | Current behavior | Classification | Impact / required fix | Source |
|---|---|---|---|---|---|
| EDRAM base/size queries | Baseline `04000000`/`00200000` | Returns those constants | CORRECT | Preserve | `GeManager::edram_base/edram_size`; T-GE |
| Enqueue/list handles/options | Queue distinct live lists with resumable state/context | Overwrites last-list record; all handles 1; options only stored | INCOMPLETE | Build minimal real list records; do not run through initial stall | `GeManager::enqueue_list`; SDK-GE/U-GE/P-EXEC |
| EnQueueHead | Head insertion with paused-state restrictions | Same metadata operation as tail enqueue | INCORRECT | Implement only if encountered; reject unsupported lifecycle rather than pretend | `register_ge_module` NID `1C0D95A6`; P-EXEC/T-GE |
| UpdateStallAddr | Validate handle, change stall, resume consumable prefix | Prints raw/canonical address; no state mutation | INCORRECT | Immediate blocker for consuming dynamic setup; update the actual list | NID `E0D68148`; U-GE `stall.S`, P-EXEC |
| ListSync/DrawSync | Mode 0 wait, mode 1 real status | Always return 0, ignoring ID/mode/work | INCORRECT | Fabricates completion; tie success to finished work | NIDs `03444EB4`/`B287BD61`; U-GE/P-EXEC |
| Break/Continue | Pause/reset/resume real queue, contextual errors/results | Always return 0 | INCORRECT | Required when actual queue control is used; not all successful Continue returns are wrong | NIDs `B448EC0D`/`4C06E472`; T-GE `break.expected` |
| SetCallback/UnsetCallback | Store function/argument pairs, allocate/release validated ID | Return 1/0; no storage/delivery | UNIMPLEMENTED | Dynamic list uses cb=1: inspect and deliver its finish callback if registered; do not fake it | NIDs `A4FC06A4`/`05DB22CE`; SDK-GE/U-GE |
| Debug traversal | Respect fetch bounds, flow and correct opcodes | Checks 32 bytes then can read 16 words (64 bytes), ignores stall; `0B/0C` break comment says FINISH/END | INCORRECT | Fix diagnostics first: per-word bounds, RET `0B`, FINISH `0F`, END `0C`; logging unbuilt tail is misleading | `register_ge_module`, enqueue dump; P-GE |
| State decode / flow | Execute commands in list order, retain target/vertex/viewport state | No state executor at all | UNIMPLEMENTED | Implement captured state/FINISH-END path first; stop on unsupported effect | `ge.hpp` and `ge.cpp`; P-GE/P-EXEC |
| Clear/primitive/transfer writes | Actual memory-producing command with required state/data | No execution or memory writes | UNIMPLEMENTED | Implement first observed writer only, after capturing it | Same local files; SDK-GU/P-EXEC |

**Answer: which success stubs are wrong? [VERIFIED]:** ListSync and DrawSync cannot report completed unexecuted work. UpdateStallAddr cannot report success without updating the list. Break/Continue cannot substitute 0 for their state transitions/errors; EnQueueHead cannot discard ordering/restrictions; UnsetCallback cannot ignore registration state. EnQueue returns a handle rather than success, but its constant-handle behavior still loses identity. Genuine synchronous completion could make some calls legitimately return immediately; that is not what this code does.

## Display gaps, including incorrect dispatch

**[VERIFIED, SDK-D `sceDisplay.S`, corroborated uOFW `references/uofw/src/kd/display/exports.exp`]:** Several runtime comments/NIDs name the wrong API. This is more severe than approximate vblank timing.

| Runtime NID | Actual API | Current handler |
|---|---|---|
| `EEDA2E54` | GetFrameBuf | Increments/returns fake vcount; does not fill output pointers |
| `46F186C3` | WaitVblankStartCB | Treats stale argument registers as three writable output pointers |
| `9C6EAAD7` | GetVcount | “WaitVblank” handler increments counter but returns zero |
| `984C27E7` | WaitVblankStart | Labeled CB, though neither handler schedules/calls callbacks |
| `DBA6C4C4` | GetFramePerSec | “WaitVblankCB”, returns integer zero instead of float API result |
| `77ED8B3A` | Not SetHoldMode in inspected SDK exports | Hold-mode stub; correct SetHoldMode NID is `7ED59BC4` |

**[VERIFIED]:** Actual WaitVblank (`36CDFADE`) and WaitVblankCB (`8EB9EC49`) are absent from this registration function. `46F186C3` can cause unintended writes or a memory exception if the guest really calls WaitVblankStartCB with leftover nonzero argument registers. These are source-proven hazards, not claims that the current log already triggered them.

| Behavior | PSP semantics | Current behavior | Classification | Impact / required fix | Source |
|---|---|---|---|---|---|
| Encountered valid SetMode/SetFrameBuf data | Store 480×272, address/512/8888 | Parameters retained, no pixels generated | APPROXIMATELY CORRECT FOR DIRTY_FIRST_FRAME | Enough to describe a capture target; preserve distinction from rendering | `core/include/p3p3ds/hle/display.hpp`, setters; SDK-D |
| Display dispatch | NIDs select correct ABI/API | Mappings above are wrong | INCORRECT | Correct this small dispatch defect before pursuing display/callback waits | `core/src/hle/display.cpp::register_display_module`; SDK-D/uOFW exports |
| Latch/query/validation | Current vs next-vsync address; valid args; nuanced stride/format updates | One struct updated immediately; GetFrameBuf body ignores sync; all inputs accepted | INCOMPLETE | Add current/pending scanout for a genuine displayed frame; not needed just to write color RAM | DisplayManager setters, GetFrameBuf body; T-D/P-D |
| Vblank/time | Independent display time and waits | Counter increments on query/wait; waits never block | INCORRECT | Can break frame-paced progress; fix only required timing dependency, not arbitrary renderer delays | `DisplayManager::vcount`, HLE wait handlers; SDK-D/P-D |

## SysMem and kernel dependencies

| Behavior | PSP semantics | Current behavior | Classification | Impact / required fix | Source |
|---|---|---|---|---|---|
| Observed 16 MiB LowAligned allocation | Partition 2; 4096-aligned free range | Successful ascending allocation from `08ED0000` | APPROXIMATELY CORRECT FOR DIRTY_FIRST_FRAME | Continue; no allocator rewrite justified yet | `core/src/hle/sysmem.cpp::alloc_partition_memory`, defaults in `sysmem.hpp`; E/U-S |
| High/HighAligned | Place from upper free end | Both use ascending bump allocation | INCORRECT | Fix when requested, not preemptively for current LowAligned path | Same function; U-S/P-S |
| Addr / allocator reservation | Reserve non-overlapping rounded range | Accepts range without checking live blocks or advancing bump pointer | INCORRECT | Potential future overlap; implement range reservation when exercised | Same function; U-S/P-S |
| Free/reuse/free queries | Reclaim/coalesce; total vs max differ under fragmentation | Erases UID only; bump never retreats; both queries return tail | INCOMPLETE | May exhaust prematurely; not why GE writes nothing | SysMemManager `free_partition_memory`, `total_free_memory`, `max_free_memory`; U-S/P-S |
| Validation/granularity | Partition permissions, reject invalid types/alignment, round size | Partition 1 uses user heap, invalid type becomes Low, bad alignment becomes 4096, null name becomes empty; raw size retained | INCORRECT | Preserve current valid path but correct before relying on error handling/free size | SysMemManager allocation/HLE wrapper; T-S |
| UID get/free failure | Get invalid returns 0; free invalid returns `800200CB` | Matches those results | CORRECT | Do not “fix” GetBlockHeadAddr to a speculative negative error | SysMemManager get/free; T-S tail cases |
| Stack/heap ownership | Allocations must not overlap | Independent stack bump descends from `09FF0000`; heap grows below same limit | INCOMPLETE | Monitor reservations before adding threads; shared address-space backing alone does not prevent collision | `core/src/hle/threadman.cpp::create_thread`, SysMem defaults, `kernel_state.hpp` independent managers |
| Event flags | Persistent pattern + genuine wait/poll result | All objects UID 1; set/clear/delete no-op; waits/poll return requested bits and success | UNIMPLEMENTED | Likely dependency if GE callback signals a wait; implement actual observed pattern/transition | `core/src/hle/hle_modules.cpp::register_all_hle_modules`; K |
| Ordinary callbacks / exit registration | Retain callable identity and registration | UID 1 / success only | UNIMPLEMENTED | Exit callback not established as first-write blocker; GE callback is separate and higher priority | Same local function; K |
| LW mutexes | Workarea ownership/count + contention | Lock/unlock/try always success, no workarea updates | UNIMPLEMENTED | Potential producer synchronization blocker; verify use before implementing contention | Same local function; K |
| Kernel_Library memcpy dispatch | `1839852A` copies bytes and returns destination | Misregistered as TryLock; returns 0 without copying | INCORRECT | Can silently lose guest data before graphics output; correct dispatch/copy semantics in next small fix | `hle_modules.cpp` NID `1839852A`; PPSSPP `Core/HLE/sceKernelInterrupt.cpp::sceKernelMemcpy`/registration, uOFW `src/kd/usersystemlib/exports.exp:14` |
| Thread attribute update | Clear/set update with legal-bit validation | Formula correct, no legal-bit validation | APPROXIMATELY CORRECT FOR DIRTY_FIRST_FRAME | Retain for observed valid request, add validation when needed | `ThreadManager::change_current_thread_attr`; K |
| System time / delay | Elapsed microseconds / scheduled wait | Time +=1000 per query; delay no-op | INCORRECT | Can spin or mis-sequence producer work; not a demonstrated current stop cause | `hle_modules.cpp`, matching registrations; K |
| Interrupt suspend/resume | Save/restore state, including nesting | Always “previously enabled”, resume no-op | INCOMPLETE | Must integrate before delivering GE interrupt callbacks safely | Same local function; K/uOFW `intr.S` |
| Module lookup / language / utility / printf shortcuts | Real module/state semantics | Hardcoded UID/success | APPROXIMATELY CORRECT FOR DIRTY_FIRST_FRAME | Existing trace passes these; no evidence to prioritize ahead of GE/AOT frontier | Same local function, SysMem printf handler; E (bootstrap sufficiency only) |

**SysMem sufficiency [INFERRED]:** Heap `[08ED0000,09FF0000)` is `0x1120000` bytes. The observed 16 MiB allocation leaves a tail `0x120000` bytes and ends at `09ED0000`. Thread stack allocator has no knowledge of that reservation and could descend into it. Existing execution reaches the stated frontier, so SysMem is sufficient for the observed bootstrap, **not proven sufficient through a first frame**. The immediate blocker is missing AOT code and GE execution, not a demonstrated allocation failure. Keep the heap/stack overlap risk visible without rewriting SysMem during this research task.

## Direct answers and limits of the evidence

1. **VRAM mapping? [VERIFIED]** Correct backing for the observed primary targets; full mirror/swizzle/protection behavior remains incomplete.
2. **Aliases? [VERIFIED]** Correct for observed `48D14600` and primary uncached VRAM; cache publication is not emulated.
3. **Three addresses? [VERIFIED/INFERRED]** Color draw buffer, separate selected scanout buffer, then depth storage, as quantified above.
4. **Dynamic draw/clear? [VERIFIED]** Neither; 29 setup/termination words only.
5. **What must run next? [INFERRED]** Consume real setup with correct stall/completion, deliver any associated completion dependency, advance AOT from `08B19890`, and capture the next guest-produced memory-writing command/data. Merely rasterizing the existing setup is not possible.
6. **False GE success? [VERIFIED]** ListSync, DrawSync, UpdateStallAddr, Break/Continue and callback removal/queue restrictions are detailed above.
7. **FINISH vs END? [VERIFIED]** Completion marker/event versus stop-reading; pair and prior SIGNAL context matter, neither draws.
8. **Opcodes? [VERIFIED]** FINISH=`0F`, END=`0C`, RET=`0B`.
9. **Buffer reconstruction? [VERIFIED]** Dedicated pointer/width state, EDRAM offsets for observed color/depth; not the VADDR relative-address path.
10. **Absolute FRAMEBUFPTR? [VERIFIED]** Observed zero is EDRAM offset zero. Do not add BASE; width high-byte encoding and effective render routing differ as explained in the reference.
11. **What writes EDRAM? [VERIFIED]** A real primitive/patch with enabled writes, a memory transfer into EDRAM, or CPU stores. CLEARMODE only changes how subsequent primitives write.
12. **Smallest likely first rasterizer? [INFERRED, SDK-GU]** Through-mode, untextured clear sprites/rectangles are a small plausible candidate. **[UNVERIFIED]** No captured P3P draw yet proves this is the first operation; neither its vertex type nor clear value is known.
13. **SysMem sufficient? [INFERRED]** Yes for the demonstrated bootstrap; conditional for continued execution, with stack/heap ownership risk.
14. **Likely pre-output stub blockers? [INFERRED]** First: ignored GE stall/completion/callbacks, incorrect display dispatch and memcpy misregistered as TryLock. Then event flags, timing/vblank and LW mutex workareas **if** the next producer path uses them. No evidence justifies replacing every DIRTY_FIRST_FRAME stub.
15. **Next implementation? [INFERRED]** A bounded, resumable GE setup executor with honest statuses, plus the verified small diagnostic/display dispatch corrections, before selecting the first real writer. Do not start a full renderer.

**Why VRAM is zero [VERIFIED + INFERRED]:** GeManager contains no writer, and the two inspected lists request no rasterization. The CPU run stops before any captured memory-producing graphics operation. Thus zero VRAM is expected from this evidence even if setup commands were implemented. Conversely, zero bytes alone would not prove that no write happened: a legitimate black/zero-depth clear could write zeros. Add explicit write accounting; do not equate a nonzero byte count with a visible frame.

**Uncertain semantics [UNVERIFIED]:** First actual P3P writer and its callback/event dependencies; exact mirror/swizzle and CPU cache visibility; complete SIGNAL/END corner cases and hardware timing; unaligned Addr allocation edge case. Source disagreements are retained in the reference: wiki versus effective target routing, SDK sync-status names, display stride wording, SysMem Addr coverage. No PSP hardware tests or regression binaries were executed for this documentation-only task.

# FIRST VRAM WRITE PLAN

1. **Correct observability and dispatch.** Fix RET/FINISH/END and REGION labels, bound each debug read and stop at stall. Correct the specific display NID/ABI mismatches above and the memcpy-as-TryLock registration; verify that a memcpy fixture copies bytes and returns its destination. Validate import dispatch with SDK-D/uOFW/PPSSPP rather than the existing comments. No guest progress or pixel claim follows from these fixes alone.
2. **Execute the captured setup honestly.** Keep per-live-list handle, PC, stall, lifecycle, callback association and required context. Initially support the observed straight-line state writes and FINISH/END; retain matrix/register reset state needed by the static list. Unsupported control/effect commands stop explicitly. UpdateStall must resume from the saved PC, not restart. Test enqueue-at-stall executes zero commands, advance through END completes exactly once, invalid handles/modes fail, and polls distinguish stalled/completed. Handle callback registration/delivery before claiming that cb=1 completion has reached the guest; integrate interrupt state or explicitly report the unsupported dependency.
3. **Verify targets without manufacturing pixels.** Decode captured D2/9C–9F, viewport/offset/depth and reset state. Assert final color `04000000`, stride 512/8888, depth `04110000`, stride 512; retain scanout `04088000` separately. Replay both observed setup lists and assert **zero raster writes**, not a synthetic clear. This is the first useful GE correctness milestone.
4. **Expose the first writer through AOT.** Continue boundary discovery narrowly from `08B19890`/RA `08B19BE0`; retain stop-on-missing behavior. Capture list PC/opcode, relevant state, vertex/index memory and completion dependency at the first PRIM/patch/transfer (or watched CPU VRAM store). Implement event-flag/time/LW-mutex behavior only if that path proves it is required. Do not guess a drawing command into the setup list.
5. **Implement that single observed operation.** If it is a clear rectangle, decode its actual VERTEXTYPE and vertices, honor clear channel/depth masks, clipping/scissor, strides and target bounds; store into GuestMemory. Compare a small synthetic fixture with the cited PSP/PPSSPP semantics, then replay the genuine command. If the first writer differs, implement that operation instead; do not build textures, tessellation or 3DS rendering speculatively.
6. **Prove the write, then the image.** Record command PC, target, pixel/depth store count and changed-byte count; also retain the whole-VRAM nonzero probe. Nonzero depth alone and a zero-valued clear are distinct milestones. Dump the actual color draw target locally using 512 stride, 480×272 crop and verified format; later capture the buffer actually latched for scanout. Keep dumps/logs ignored. Claim first P3P visual output only after a guest-driven color result is visible, never from framebuffer registration or a test-generated pattern.
