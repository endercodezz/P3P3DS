# P3P3DS current execution state

This is the current source of truth. Historical audits describe their stated base commits, not today's runtime. Evidence below concerns ULUS-10512 on the PC host; no 3DS hardware result is claimed.

## Current Phase 2 result

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

Immediate blocker: missing UMD callback registration HLE (category 2: missing HLE service).

Next smallest step: implement and test `sceUmdRegisterUMDCallBack` with callback UID validation and registration state, using `references/ppsspp/Core/HLE/sceUmd.cpp::sceUmdRegisterUMDCallBack` as the reference; do not substitute unconditional success.

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
