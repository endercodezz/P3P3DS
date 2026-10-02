# P3P3DS — Next Steps

Updated 2026-10-02. What is done and measured lives in [`CURRENT_STATE.md`](CURRENT_STATE.md); this file only lists what comes next, in the maintainer's priority order. Every step names how it will be verified (CLAUDE.md section 3: measure first).

## Where the project stands

- Whole `.text` statically recompiled (237 units), HLE kernel, VFS with the mod chain, GE executor, deterministic PC runner with scripted input.
- New 3DS: PICA200 renderer, real-time pacing, save/load menu on the bottom screen; on the maintainer's New 3DS 40 minutes of play at 87 % of real time, no sound.
- P3P3DS Builder: the user's ISO becomes `p3p3ds.3dsx` on their PC in about 5 minutes.

## 1. Saves (priority 1)

Implemented (`core/src/hle/savedata.cpp`), checked by a unit round trip and by LOAD GAME without saves on PC. Still open:

1. **In-game save and load on PC.** Re-record `profiles/p3p/input/first_day.txt` with the post-ctrl-fix timings and extend it to the first save point; check that P3P's save calls succeed (modes, files under `ms0:/PSP/SAVEDATA/ULUS10512DATAxx/`), then LOAD GAME restores the same scene. Verify: frames before saving and after loading, deterministic replay.
2. **Hardware check** by the maintainer: bottom-screen menu, save, quit, load.
3. Later: compatibility with PSP/PPSSPP saves (the data file is encrypted on the PSP with the key P3P passes; ours is stored plain).

## 2. Stable 100 % speed (priority 2)

Hardware split (87 % speed): AOT + dispatch 55 %, HLE 42 % (GE rendering 10 %).

1. **Do not wait for the GPU every frame**: double-buffer vertices/command lists so the CPU builds frame N+1 while the PICA200 draws frame N. Verify: `present` share and speed in the bottom-screen report.
2. **Cheap path for the hottest HLE calls** (interrupt suspend/resume about 11,600 calls/s each in menus): no `std::function`, no scheduler hook. Verify: HLE share, CTest, deterministic replay.
3. **Profile hot guest functions on ARM** and compile those units with full optimization (profile-guided split of `-Os` / `-O2`). Verify: speed on hardware.
4. Report "speed over the last 10 s" next to the run average, so slow places are visible.

## 3. Sound (priority 3)

The PC mixer already produces correct game audio (`sceAudio`, `sceSasCore`, maintainer-verified WAV). Missing: an `ndsp` output on the 3DS fed from the virtual-clock mixer, on a separate thread/core. Verify: audio on hardware without drops; the bottom-screen report counts late/dropped buffers.

## Later

- **Lighting** in 3D scenes (12,684 lit draws in the first day; the GPU renderer has no lights yet), points/lines.
- **Opening movie**: H.264 + ATRAC3plus decoding (plan below); until then the runner skips it with START.
- **`.cia` packaging** (larger memory mode, home-menu icon) from the Builder.
- **Mods**: texture replacement and a Russian translation (feasibility in `3DS_PLATFORM.md` section 9 and `P3P_RESEARCH.md`); a 60 fps patch only as a separate mod (the game paces itself with `WaitVblankStartMultiCB(2)` and its logic is per frame [INFERRED]).
- Builder: version resource, detection of devkitPro installs in other places.

## Planned, but not soon: Old 3DS / 2DS

Requested by many players; it needs a different approach, not tuning, so it comes after the New 3DS version is playable. Today about 112 MB of application memory are needed (44 MB code, 32 MB PSP RAM), while the Old 3DS offers at most 96 MB and a 268 MHz CPU without L2 cache (New 3DS: 804 MHz with L2, and it reaches 87 % of real time). Directions to measure first: much smaller code (compile only hot code ahead of time, interpret the rest), less host memory per PSP byte, and whether any frame rate is reachable at 268 MHz at all.

---

### Render correctness reference

The maintainer compared the first-day screens (logos, title, main menu, protagonist selection, Dark Hour lobby, name entry, Mitsuru introduction, own room, next morning, school lobby) with the original game and found them matching, without graphical artifacts. Emulator reference captures are therefore not planned; new renderer work is checked against these screens and the deterministic frame hashes of the input scripts in `profiles/p3p/input/`. On the 3DS, frame dumps (`sdmc:/p3p3ds/dump_every.txt`) are compared with the PC frames.

### Movie and audio decoding plan (sceMpeg / ATRAC3plus)

Interface in place: `core/include/p3p3ds/hle/mpeg.hpp` `MpegDecoder` receives each demultiplexed AU (`decode_video` writes a picture into guest memory, `decode_audio` writes 2048 stereo S16 samples). `BlankMpegDecoder` (black / silence) is the default. Container behaviour is verified against `pspautotests/tests/video/mpeg/basic.expected`. P3P's only movie is `USRDIR/sound/pmsf/P3OPMV_P3P.pmsf` (`_P3PB` variant): H.264 2,997 AUs at 29.97 fps, ATRAC3plus 2,156 frames of 752 bytes.

1. PC: an H.264 Baseline/Main decoder + ATRAC3plus decoder behind `MpegDecoder`. Candidates must be license-compatible and vendored offline (none present in the workspace); PPSSPP uses FFmpeg — reference only, no code copy. Verify: decoded frame hashes against a reference decode of the same AUs; audio PCM against a reference decoder.
2. 3DS: the New 3DS has an MVD hardware H.264 service (`mvd:STD` in libctru) [UNVERIFIED for this stream profile]; ATRAC3plus needs a software decoder on ARM11 [UNVERIFIED cost]. Measure before choosing; an alternative is converting the user's movie to a 3DS-native format in the Builder.
3. The game's own `sceAtrac3plus` use for BGM (not yet reached) shares the ATRAC3plus decoder.
