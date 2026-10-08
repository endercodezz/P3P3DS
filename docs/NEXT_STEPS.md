# P3P3DS — Next Steps

Updated 2026-10-06. What is done and measured lives in [`CURRENT_STATE.md`](CURRENT_STATE.md); this file only lists what comes next, in the maintainer's priority order. Every step names how it will be verified (CLAUDE.md section 3: measure first).

## Where the project stands (0.2.8)

- Whole `.text` statically recompiled, HLE kernel, VFS with the mod chain, GE executor, deterministic PC runner with scripted input; the PC route plays through the first battle (maintainer's save, local route).
- New 3DS: PICA200 renderer with a vertex shader (transform, skinning, lighting), real-time pacing, save/load menu on the bottom screen. Field 21-30 fps at about 90 % speed; the first battle renders correctly; no sound.
- P3P3DS Builder for Windows and Linux; releases built by GitHub Actions.

## 1. Battle speed (priority)

9.9 fps / 33 % speed in Azahar after the per-draw work was cut (0.2.8, `CURRENT_STATE.md`); 0.2.7 measured 5-7.5 fps on hardware. Goal: 20 fps first, then the game's full 30 (in battle the game shows 30 frames per game second).

1. **Hardware report of 0.2.8** in battle (bottom-screen `draw:` and `merge` lines).
2. **Cache unpacked model vertices across frames**: vertex unpack is 26 % of battle time. Verify: `vtx` share and fps; frames unchanged on the PC renderer.
3. **The game's own cost**: aot + hle are about 48 % of the time at 33 % speed, which alone caps the battle near 70 % speed. Profile a battle window on the PC production build (`--sample`, `tools/profile_symbols.py`) and on hardware (`profile_seconds.txt`); candidates: hot guest functions compiled with full optimisation, cheaper paths for the hottest HLE calls.
4. **Do not wait for the GPU every frame** if GPU wait becomes visible on hardware (Azahar shows none).
5. Candidates from the code review of 2026-10-08 (`CURRENT_STATE.md`, "Code review"), each to be measured before it is kept: profiling timers on raw ticks instead of `steady_clock`; raw PSP vertex bytes as PICA200 attributes (alternative to step 2); a local copy of the AOT memory view and `PSPRECOMP_AOT_ASSUME_NO_WRITE_WATCH`; `-O2` with instruction scheduling for the hottest units; a Core 2 worker (first check that a `.3dsx` can create a thread there).

## 2. Field at a stable 100 %

Field draws are through mode (2D) and still take the CPU path; dorm floors run at 21-26 fps, 89-91 % speed. Verify: hardware report in the dorm. At 90 % speed 30 fps would show as 27, so frames are also lost in game time: test first whether the UMD-speed read latency applied to every file (`IoManager::transfer_us`) causes it (dorm route on the PC runner with near-zero latency).

## 3. Sound

The PC mixer already produces correct game audio (`sceAudio`, `sceSasCore`, maintainer-verified WAV). Missing: an `ndsp` output on the 3DS fed from the virtual-clock mixer, on a separate thread/core. Verify: audio on hardware without drops; the bottom-screen report counts late/dropped buffers. Before that (the skipped retry of `sceAudioOutputBlocking` after a preemption is fixed), decide what the output does when the game runs below 100 % speed (the virtual clock then runs slower than the DSP).

## Later

- **Saves**: compatibility with PSP/PPSSPP saves (the data file is encrypted on the PSP with the key P3P passes; ours is stored plain).
- **Opening movie**: H.264 + ATRAC3plus decoding (plan below); until then the runner skips it with START.
- **`.cia` packaging** (larger memory mode, home-menu icon) from the Builder.
- **Mods**: texture replacement and a Russian translation (feasibility in `3DS_PLATFORM.md` section 9 and `P3P_RESEARCH.md`); a 60 fps patch only as a separate mod (the game paces itself with `WaitVblankStartMultiCB(2)` and its logic is per frame [INFERRED]).
- Builder: detection of devkitPro installs in other places.

## Planned, but not soon: Old 3DS / 2DS

Requested by many players; it needs a different approach, not tuning, so it comes after the New 3DS version is playable. Today about 112 MB of application memory are needed (44 MB code, 32 MB PSP RAM), while the Old 3DS offers at most 96 MB and a 268 MHz CPU without L2 cache (New 3DS: 804 MHz with L2). Directions to measure first: much smaller code (compile only hot code ahead of time, interpret the rest), less host memory per PSP byte, and whether any frame rate is reachable at 268 MHz at all.

---

### Render correctness reference

The maintainer compared the first-day screens (logos, title, main menu, protagonist selection, Dark Hour lobby, name entry, Mitsuru introduction, own room, next morning, school lobby) and the first battle with the original game. New renderer work is checked against these screens, the PC renderer's frames of the same route and the deterministic frame hashes of the input scripts in `profiles/p3p/input/`. On the 3DS, frame dumps (`sdmc:/p3p3ds/dump_every.txt`) are compared with the PC frames.

### Movie and audio decoding plan (sceMpeg / ATRAC3plus)

Interface in place: `core/include/p3p3ds/hle/mpeg.hpp` `MpegDecoder` receives each demultiplexed AU (`decode_video` writes a picture into guest memory, `decode_audio` writes 2048 stereo S16 samples). `BlankMpegDecoder` (black / silence) is the default. Container behaviour is verified against `pspautotests/tests/video/mpeg/basic.expected`. P3P's only movie is `USRDIR/sound/pmsf/P3OPMV_P3P.pmsf` (`_P3PB` variant): H.264 2,997 AUs at 29.97 fps, ATRAC3plus 2,156 frames of 752 bytes.

1. PC: an H.264 Baseline/Main decoder + ATRAC3plus decoder behind `MpegDecoder`. Candidates must be license-compatible and fetched as references, not vendored; PPSSPP uses FFmpeg — reference only, no code copy. Verify: decoded frame hashes against a reference decode of the same AUs; audio PCM against a reference decoder.
2. 3DS: the New 3DS has an MVD hardware H.264 service (`mvd:STD` in libctru) [UNVERIFIED for this stream profile]; ATRAC3plus needs a software decoder on ARM11 [UNVERIFIED cost]. Measure before choosing; an alternative is converting the user's movie to a 3DS-native format in the Builder.
3. The game's own `sceAtrac3plus` use for BGM (not yet reached) shares the ATRAC3plus decoder.
