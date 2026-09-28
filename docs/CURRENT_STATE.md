# P3P3DS current execution state

This is the current source of truth. Historical audits describe their stated base commits, not today's runtime. Evidence below concerns ULUS-10512 on the PC host; no 3DS hardware result is claimed.

## Current micro-sprint: observed UMD drive wait

Source base: `92e67180fc84c7df04c8e61d8c22ea47bca7dcbf`.

- [VERIFIED] Pre-change `build/p3p_pc_bootstrap.exe --run-until-blocker --dump-events .tmp/umd-wait-before.json` reproduced missing `sceUmdUser::sceUmdWaitDriveStat` (NID `0x8EF08FCE`) at stub `0x08B8010C`, caller `0x08AA1608`, return PC `0x08AA1610`, `user_main` / UID 2. The executed transfer word is `0x0E2E0043`. `build/generated/p3p_generated.cpp:L_08AA1608` sets `A0=0x20` in the call delay slot, with no intervening write; the single-argument API has no pointed-to argument. `L_08AA1610` does not branch on `V0`: it writes three zero values and returns. The result is not consumed on this executed path.
- [VERIFIED] uOFW `references/uofw/include/mediaman.h:55-67` names bit `0x20` `SCE_UMD_READABLE`; `references/uofw/src/kd/mediaman/mediaman.c:646-663` validates supported wait bits and waits on the matching event flag with OR semantics, returning the wait status. PSPSDK `psp/pspsdk/src/umd/pspumd.h:46-53` labels `0x20` `PSP_UMD_READY`, a naming disagreement. `references/pspautotests/tests/umd/wait/wait.c:72,79-81` and `wait.expected:8,18-20` show zero after an activated medium for the tested waits; this hardware fixture was inspected, not run here. PPSSPP `references/ppsspp/Core/HLE/sceUmd.cpp::__KernelUmdGetState/sceUmdWaitDriveStat` is a secondary behavioral reference: present plus activated yields the readable bit, and matching waits return zero.
- [VERIFIED] The host implements only P3P's observed `0x20` path: it returns zero when the existing per-kernel `medium_present` and `activation_requested` flags are both true. Any other mask or an unsatisfied condition stops explicitly instead of reporting success. Focused `p3p_hle_umd` tests cover success, both missing prerequisites, unsupported mask, telemetry and unchanged state. No mount, drive-ready state, wait queue, callback delivery or filesystem service was added. [INFERRED] The two existing flags are sufficient as a narrow readable-wait proxy for this configured PC game launch; they do not prove an actual PSP filesystem mount.
- [VERIFIED] Release build, full CTest (**11/11**), stable `--verify-bootstrap`, and `git diff --check` pass. `--run-until-blocker --dump-events .tmp/umd-wait-final-{1,2}.json` produced identical event files, SHA-256 **`E9CBBB420D3549174813B41B762171D4C78E5F0392C6EBA5135386A758044368`**. `umd_wait_drive_stat` records mask 32, result zero, presence and activation true; subsequent `guest_enter` at `0x08AA1610` proves guest continuation. Both runs stop at the next genuine frontier: `missing_guest_function` at **`0x08A9B934`**, direct caller **`0x08AB29BC`**, word **`0x0E2A6E4D`**, thread **UID 2**. No seed was added.
- [VERIFIED] Graphics milestone: **NO**. No GE writer, color or depth write was observed. Four unclassified-resource VRAM operations still touch 16 bytes (10 changed); the two setup GE lists still complete with 212/29 commands.

Immediate blocker: missing guest function at `0x08A9B934`.

Next smallest step: prove the executed call and independent function boundary at `0x08A9B934` in a separate sprint before adding one managed seed.

## UMD activation request (historical)

Source base: `cfb9d61599befd7942b0ac0f5be85874432d9691`.

- [VERIFIED] The pre-change replay still stopped at `sceUmdActivate` (`0x08B80124`), SHA-256 `33505F22B89F16D42B3EBCA31A2C808A550BA56DC68F0D33E55202BAD9861566`. The executed AOT path at `build/generated/p3p_generated.cpp:L_08AA15EC/L_08AA15F4` sets `A0=1`, `A1=0x08BA7EB8`, and `RA=0x08AA1600`; the relocated ELF segment at file offset `0x3A3F58` contains `disc0:\0`. The post-change `umd_activate` event confirms these arguments at the HLE boundary, `V0_before=1`, `A2=0`, `A3=0x08FBB1F8`, `GP=0x08C42A50`, `SP=0x09FEF940`, `user_main` / UID 2, and medium present. `L_08AA1600` branches to the error path on negative `V0`, otherwise continues to `0x08AA1608`; the exact nonnegative value matters to that branch only.
- [VERIFIED] `sceUmdUser::sceUmdActivate` NID `0xC6183D47` now accepts modes 1/2 and the exact readable NUL-terminated `disc0:` alias; invalid mode, alias or guest pointer returns `0x80010016`. A valid request returns zero and sets a per-kernel `activation_requested` flag. `sceUmdCheckMedium` presence and the registered callback remain separate and unchanged. This flag records acceptance of the call; it does **not** claim drive readiness, filesystem assignment or a successful `sceIoAssign`. Source: `references/uofw/src/kd/mediaman/mediaman.c:124-151,526-546`, `references/uofw/include/mediaman.h:45,75-77`, `psp/pspsdk/src/umd/pspumd.h:85-107`, `references/pspautotests/tests/umd/callbacks/umd.expected:2` and `references/pspautotests/tests/umd/wait/wait.expected:7`. uOFW stores the internal mount result as error status while returning zero from `sceUmdActivate`; no mount operation was implemented here.
- [VERIFIED] Release build, full CTest (**11/11**), stable `--verify-bootstrap`, and `git diff --check` pass. Focused UMD tests cover observed mode 1, documented mode 2, invalid mode/alias/pointer, pre/post request state, medium/callback preservation and independent kernels. No generated AOT or profile-seed changes.
- [VERIFIED] Two `--run-until-blocker --dump-events .tmp/umd-activate-final-{1,2}.json` runs are byte-identical, SHA-256 **`5101F5EF183480891A464AA2C25E7F0E7BCE80CED86B9D89E2B3C04672732D3E`**. `umd_activate` records result **0**; subsequent `guest_enter` at **`0x08AA1600`** proves return to P3P. Both runs stop at the next genuine blocker: missing HLE `sceUmdUser::sceUmdWaitDriveStat`, NID **`0x8EF08FCE`**, stub PC **`0x08B8010C`**, caller **`0x08AA1608`**, thread **user_main / UID 2**. No implementation of that service belongs to this sprint.
- [VERIFIED] Graphics milestone: **NO**. Two setup GE lists still complete with 212/29 commands, no GE writer. Four unclassified-resource VRAM writes touch 16 bytes (10 changed); CPU color/depth writes remain zero. There is no rendered pixel or frame evidence.

Immediate blocker: missing `sceUmdWaitDriveStat` HLE.

Next smallest step: prove the observed wait mask and post-call use at `0x08AA1608`, then research only the required wait semantics before a separate implementation sprint.

## UMD medium presence (historical)

Source base: `8cf9b70caf5af42be7663a674c5b9424701b93a8`.

- [VERIFIED] `UmdState` defaults to medium absent; setter/getter expose per-kernel presence. `sceUmdCheckMedium` reads it and returns 0/1 without drive-state, activation or callback effects. `platform/pc/main.cpp` explicitly configures presence for the game-launch environment; this is not proof of filesystem mounting or drive readiness. References: `references/uofw/include/mediaman_user.h:74-79`, `references/uofw/src/kd/mediaman/mediaman.c::sceUmdCheckMedium/sub_000004DC`, `psp/pspsdk/src/umd/pspumd.h:69-73`; PPSSPP `sceUmd.cpp::sceUmdCheckMedium` used only as behavioral reference, no source copied.
- [VERIFIED] Call `0x08AA15E4 -> 0x08B800EC` returns **1** with configured presence; subsequent `guest_enter` at **`0x08AA15EC`** proves continuation. SDK_UMD registration still succeeds with UID 4. Next blocker is **missing HLE `sceUmdUser::sceUmdActivate`**, NID `0xC6183D47`, PC **`0x08B80124`**, caller **`0x08AA15F8`**, user_main / UID 2. No implementation or further frontier work beyond this blocker.
- [VERIFIED] VRAM: four unclassified-resource writes, 16 bytes touched, 10 changed; CPU color/depth writes zero. GE setup lists remain 212/29 commands, one completion each, no writer. **Graphics milestone: NO**.
- [VERIFIED] `cmake --build build --config Release --parallel 2`, `ctest --test-dir build --output-on-failure` (**11/11**), `build/p3p_pc_bootstrap.exe --verify-bootstrap` and `git diff --check` pass. Focused UMD tests cover default/absent/present/removal through the actual HLE, preserved callback registration and independent kernel presence states.
- [VERIFIED] Two `--run-until-blocker --dump-events .tmp/medium-replay-{1,2}.json` runs match SHA-256 **`33505F22B89F16D42B3EBCA31A2C808A550BA56DC68F0D33E55202BAD9861566`**. Evidence: `umd_check_medium` followed by `guest_enter`, next `missing_hle`, and VRAM/GE counters. No generated AOT/profile-seed changes.

Immediate blocker: missing `sceUmdActivate` HLE.

Next smallest step: establish the minimal `sceUmdActivate` activation contract for the observed call against uOFW/PSPSDK before implementing it.

## UMD callback registration (historical)

Source base: `0e6eca51292ce77ca40b78e9eeb6be094327e6a8`; no frontier expansion or generated-code changes in this sprint.

- [VERIFIED] `ThreadManager::create_callback/get_callback` now owns callback UID, name, guest function, common argument and owner thread. Threads and callbacks share the existing UID sequence with type-specific lookup. The former hardcoded UID 1 stub is removed. Creation is **PARTIAL**: ownership/identity only; no callback scheduling, notification queue, delivery, deletion or callback-aware waits. Invalid name pointers stop explicitly rather than pretending success.
- [VERIFIED] `UmdState::register_callback` rejects zero, invalid and non-callback UIDs with `0x80010016`; accepts valid and repeated valid registration with zero; preserves registration on failure and replaces it on another valid registration. References: `references/uofw/src/kd/mediaman/mediaman.c:590` (`sceUmdRegisterUMDCallBack`), `references/uofw/include/mediaman_user.h:54`, `references/pspautotests/tests/umd/register.expected:1`. No PPSSPP implementation code was copied.
- [VERIFIED] P3P creates callback `SDK_UMD`, UID **4**, owner **user_main / UID 2**, common argument zero. The recorded call `0x08AA15D4 -> 0x08B80114` registers UID 4 with result **0**; the following `guest_enter` at **`0x08AA15DC`** proves return to guest execution. The earlier `Persona3PSP` callback receives UID 3.
- [VERIFIED] Next genuine blocker: **missing HLE**, `sceUmdUser::sceUmdCheckMedium` (NID `0x46EBB729`), final PC **`0x08B800EC`**, caller **`0x08AA15E4`**, thread **user_main / UID 2**. Execution stops here; this service is not implemented by the sprint.
- [VERIFIED] VRAM activity remains four unclassified-resource writes, 16 bytes touched, 10 changed; CPU color/depth writes zero. GE lists 1/2 complete once with 212/29 commands; no GE writer. **Graphics milestone: NO.** Allocator metadata remains an inference, not graphics output.
- [VERIFIED] Release build and all **11/11 CTest** tests pass, including `p3p_hle_umd`, stable bootstrap and frontier regressions. The UMD test checks real callback identity/fields, thread UID rejection, invalid/zero UID, valid/repeat registration, state preservation/replacement and independent kernel state. Expected registration results match the limited pspautotests cases above; no hardware test run is claimed.
- [VERIFIED] `build/p3p_pc_bootstrap.exe --verify-bootstrap` passes. Two `--run-until-blocker --dump-events .tmp/umd-replay-{1,2}.json` runs are byte-identical, SHA-256 **`26DD4607202D819D5516386AD7F4D5E7D73FDB266ED4C469A103A3F40DDEADFB`**. `callback_create`, `umd_callback_register`, subsequent `guest_enter`, and `missing_hle` events provide the evidence. `git diff --check` is clean.

Previous blocker: missing `sceUmdCheckMedium` HLE, resolved above.


## Phase 2 result (historical, superseded above)

- Source base: `2d520e30f9cac063707cb9d3b26055288a9967eb`; checkpoint commit: `ea7374b5a63fa73c05af8fdc27a552a23e60f470`. The intervening commit contains only maintainer README/ignore changes.
- [VERIFIED] Execution now passes four non-render-target CPU VRAM writes and one further proven direct call: `0x08AA1564 -> 0x08AA0F8C`. The unchanged conservative validator accepts a two-instruction closed CFG. There are now 18 managed seeds, 126 manual function groups, 221 import wrappers and 1,487 registered entries (`frontier_seeds.csv`, generated output, canonical log).
- [VERIFIED] Final blocker is **missing HLE**: `sceUmdUser::sceUmdRegisterUMDCallBack`, NID `0xAEE7404D`, guest stub `0x08B80114`, caller `0x08AA15D4`, thread user_main / UID 2. No service stub was introduced and no further execution attempted beyond this stop rule.
- [VERIFIED] Four CPU VRAM stores touch 16 bytes and change 10 bytes, with zero observed color/depth-target writes and zero bound-texture writes. Their producer PCs are `0x08AB2B38`, `0x08AB2B3C`, `0x08AB2B48`, `0x08AB2B50`; telemetry labels them `unclassified_resource`. The allocator/resource-metadata interpretation remains [INFERRED], not a pixel/frame result.
- [VERIFIED] GE activity remains the same two completed setup lists (212 and 29 commands), no GE writer and no framebuffer output. **Graphics milestone: NO.** The legacy log banner saying FRAMEBUFFER SET records a configuration request, not rendered output.
- [VERIFIED] `core/include/p3p3ds/vram_activity.hpp::VramActivity` preserves guest continuation for non-target writes, logs at most 32 such records plus aggregate counters, and stops on a color/depth-target write. Membership uses mirrored VRAM intervals and configured target stride/format/display height. Pending display buffers are not mistaken for the active display buffer. This is diagnostic membership, not proof of visible coverage or resource ownership.
- [VERIFIED] Texture membership is deliberately limited to enabled, bound, unswizzled direct-color formats and configured mip levels. Layout references: `references/ppsspp/GPU/GPUState.h::getTextureAddress/getTextureHeight/getTextureMaxLevel`, `GPU/Common/TextureDecoder.cpp::GetTextureBufw`. Other texture layouts remain unclassified; there is no renderer or general resource tracker.
- [VERIFIED] New PSP/HLE semantics: **none**. Existing RAM/VRAM mappings and scheduler are unchanged. This milestone removes a diagnostic halt, not a PSP behavior workaround. `--stop-on-any-vram-write` retains the original first-write probe.
- [VERIFIED] Final Release build, full CTest (10/10, including frontier safety/workflow and stable bootstrap), and two identical event replays passed via `python -B tools/chase_frontier.py --check-only --output .tmp/frontier-vram-final`. New safety cases cover bounded continuation, mirror/wrap boundaries, active versus pending display targets, depth-target stop and direct-color texture stride.
- [VERIFIED] Current generated AOT SHA-256: `ED5466486F6173C6C97EE47C99E596BDAE90EA344A2E7D415F9BFB1D71888C3A`; independent regeneration is byte-identical. Current event SHA-256: `956565659C500780888190D405D36CA1E2B9CBFAC6AE07BA692C72DA4389C48D`. Evidence: `.tmp/frontier-vram-classified/report.json`, `.tmp/frontier-vram-final/report.json`, `logs/p3p_bootstrap_latest.log` and `profiles/p3p/config/frontier_checkpoint.json`.

Phase 2 blocker: missing UMD callback registration HLE (category 2: missing HLE service).

The registration blocker was resolved by the micro-sprint above.

## Phase 1 checkpoint provenance (historical)

- Baseline source commit: `25b014886934434ba6c524f5c77b7408724c9c80`; resulting Phase 1 checkpoint: `ea7374b5a63fa73c05af8fdc27a552a23e60f470`.
- ELF SHA-256: `BE2ABBD43A4AE7CE5AA2F1F146083FFE0D924FC2EB1874E6FCDC21CBA40D49DB`. The locally supplied ELF is not committed.
- [VERIFIED] Generated AOT SHA-256: `0DA7823454CC661548ECA6C0C42696B2D6A414C502494F2305D0C96FE2ED7840`. Independent regeneration is byte-identical.
- [VERIFIED] Two complete JSON event replays and the canonical replay are byte-identical: SHA-256 `F7DEBC6C97B1893FA94761C6A1DD6CC240656D53A34EBF3D0A5D5DE812CBA2F5`.

## Phase 1 verified execution, not a rendered frame

- [VERIFIED] P3P guest instructions execute as compiled native host C++ AOT functions. `CMakeLists.txt` links the generator output into `p3p_pc_bootstrap`; `Runtime::run` dispatches registered compiled functions. This does not establish hardware accuracy or native PSP performance.
- [VERIFIED] `module_start` at `0x08804108` reaches native `user_main` execution at `0x0880421C`, UID 2. SDK `0x06020010`, compiler `0x00030306`, thread creation/start, GE initialization, callback completion and display setup pass the stable checkpoint (`platform/pc/telemetry.hpp::BootstrapCheckpoint`).
- [VERIFIED] Execution-driven expansion accepted 17 additional functions in `profiles/p3p/config/frontier_seeds.csv`: the generator emits 125 manual function groups and 221 import wrappers. The running harness registers 1,486 entries including basic-block labels and the thread-return sentinel, not 1,486 distinct guest functions.
- [VERIFIED] Progress: missing function `0x08B1C594` -> first committed CPU VRAM store at **`0x08AB2B38`**, word **`0xAE220004`** (`sw v0,4(s1)`), address **`0x04154004`**, size **4 bytes**, thread **user_main / UID 2**. One operation touched 4 bytes and changed 3. Source: generated `L_08AB2AF4`, `GuestMemory::store32`, and the `cpu_vram_write` event in the reproduced report.
- [INFERRED] This appears to initialize allocator/resource metadata: surrounding ELF instructions calculate aligned bounds, write their difference at offset 4, and link pointers. That interpretation is not proof of resource ownership. It is **not a first pixel, first frame, or demonstrated graphics output**.
- [VERIFIED] No GE writer observed. Static list 1 executes 212 commands and dynamic setup list 2 executes 29, each completing once. GE color `0x04000000`, stride 512, RGBA8888; depth `0x04110000`, stride 512; display buffer `0x04088000`. Color/depth write counters remain zero. No framebuffer image was produced.
- [VERIFIED] The Phase 1 diagnostic policy stopped after the first CPU VRAM store. The exact instruction PC is in the structured event/final PC; the legacy `[VRAM FIRST STORE] pc` field denotes outer dispatch PC `0x08AB2AF4`.

## Workflow and verification

`tools/chase_frontier.py` runs the bootstrap twice per frontier and requires identical JSON events. Only a missing guest function with an actually recorded matching direct JAL can reach `p3p_frontier_check`. The checker uses the relocated ELF and the same `analyze_function` API as the generator: executable/aligned address, import exclusion, independent boundary, supported closed CFG, delay slots and existing coverage overlap are checked. Failure is reported, never bypassed.

The managed manifest is merged with the original profile at build time. Each accepted seed triggers generation, Release build, the full CTest suite and a byte-for-byte regeneration check. A failed build/test restores the prior manifest and reports that a rebuild is needed. Reports and command logs live under `.tmp/`. There is an explicit addition limit and per-command timeout; no all-game generation is used.

[VERIFIED] A long prologue with its RA save in an ordinary branch delay slot is supported and tested; an annulled branch-likely RA save is rejected (`tests/test_frontier.cpp`). The original short-window refusal at `0x08B1BF40` is preserved in `.tmp/frontier-sprint/report.json`.

Stable regression is `--verify-bootstrap` (also the compatibility alias `--verify-milestone`): it confirms bootstrap facts and actual continuation after the repaired sparse-dispatch return. It stops at that stable checkpoint. `--run-until-blocker` continues independently; `--expect-frontier <address>` is an optional progress assertion and is not part of CTest.

[VERIFIED] Commands and results on this checkpoint:

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build --output-on-failure                 # 10/10 passed
build/p3p_pc_bootstrap.exe --verify-bootstrap              # PASS
python -B tools/chase_frontier.py --check-only --output .tmp/frontier-checkpoint-final
build/recomp/PSPRecomp/psp_recomp.exe profiles/p3p/game/eboot.elf build/generated/frontier_functions.csv .tmp/checkpoint-generated.cpp
git diff --check                                          # clean (CRLF-aware repository attributes)
```

Use the configured Python interpreter from CMake. TEMP/TMP/TMPDIR are redirected to workspace `.tmp`. On this host Ninja hangs inside the execution sandbox; the same build graph succeeds outside it. No manual compile/link replacement was needed for this checkpoint.

## Scope and limitations

- [VERIFIED] Decoder recognizes 912,808 / 913,059 words; 251 BREAK words remain unsupported. Lowering coverage is 912,746; 62 generic VFPU words are decoded but not lowered (`verify_p3p_decoder`). These counts supersede older audit snapshots, not a whole-game correctness claim.
- [VERIFIED] Existing HLE modules are SysMem, ThreadMan, Display and GE setup, plus partial bootstrap wrappers (`core/src/hle/hle_modules.cpp::register_all_hle_modules`). Existing stub/debt annotations still apply. This checkpoint adds diagnostics, not full PSP service semantics.
- [VERIFIED] Memory currently backs the RAM arena at `0x08000000` (32 MiB for this runner) and 2 MiB VRAM mirrored through an 8 MiB window (`GuestMemory::contains/resolve`). Scratchpad and MMIO are not backed by this implementation. No observed access on this checkpoint requires them; the map has not been rewritten.
- [UNVERIFIED] No ARM build or physical New 3DS test was performed. Platform integration remains deferred while the PC path has unresolved blockers.

The Phase 1 diagnostic halt has been superseded by the Phase 2 missing-HLE blocker above.
