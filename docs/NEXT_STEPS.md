# P3P3DS — Next Steps

Updated 2026-10-02. What is done and measured lives in [`CURRENT_STATE.md`](CURRENT_STATE.md); this file only lists what comes next, in the maintainer's priority order. Every step names how it will be verified (CLAUDE.md section 3: measure first).

## Where the project stands

- Whole `.text` statically recompiled (237 units), HLE kernel, VFS with the mod chain, GE executor, deterministic PC runner with scripted input.
- New 3DS: PICA200 renderer, real-time pacing, save/load menu on the bottom screen; on the maintainer's New 3DS 40 minutes of play at 87 % of real time, no sound.
- P3P3DS Builder: the user's ISO becomes `p3p3ds.3dsx` on their PC in about 5 minutes.

## 1. Saves (priority 1)

Working in the maintainer's Azahar session (save at the dorm desk, load from the title and in game). Still open:

1. **Hardware check** on the New 2DS XL with the numbered save menu.
2. A deterministic PC test of an in-game save: the scripted route reaches the faculty office on 4/7 (`.tmp` work, not yet in `profiles/p3p/input/`); the first save point is the dorm desk that evening. Use `--render-from` to keep iterations short.
3. Later: compatibility with PSP/PPSSPP saves (the data file is encrypted on the PSP with the key P3P passes; ours is stored plain).

## 2. Stable 100 % speed (priority 2)

Hardware split (87 % speed, build f6c9611): AOT + dispatch 55 %, HLE 42 % (GE rendering 10 %). Azahar (build bfa6474, after the sceCtrl fix raised menus and scenes from 20 to 30 frames/s): 56 % average, 6.5 game fps on the Dark Hour rooftop, HLE 60 % (rendering 16 %). First get a hardware report of the current build.

0. **Battle**: renders correctly on hardware since 72bd103 (0.2.7). Speed: 9.9 fps in Azahar after the per-draw work was cut (65d261b, `CURRENT_STATE.md`); confirm on hardware. Next: (a) cache unpacked model vertices across frames (vertex unpack is 26 % of the time); (b) the game code itself (aot + hle about 48 % at 33 % speed) caps the battle near 70 % speed: profile the AOT and HLE share of a battle frame (goal 20 fps at 100 %).
   Earlier: **Battle rendering and speed** (build 06818ae, `CURRENT_STATE.md`): the first battle runs but is black except the gun and the Shadow, at 5-10 fps with GE 49 %. In order: (a) PC census and frames of a battle to confirm lighting/skinning/morph use; (b) lighting and skinning in the vertex path, preferably in the PICA200 vertex shader (also removes the CPU vertex cost) - done, waiting for the hardware check (`CURRENT_STATE.md`); (c) bottom-screen report redraw less often (ui 8 %; done: every 5 s, written in place without clearing, which also removes the flicker the maintainer saw). **Target set by the maintainer: at least 20 game frames/s in battle at 100 % of real time.**
   Earlier: **hardware report past the first Shadow** (build 472599b: about 30 fps in the field, 15 at the Shadow, then a command-buffer crash, now fixed); then the **GE cost at the Shadow** (49 % of the time, not hashing): `cpu->rtt` and `flushes` in the report. Earlier: **hardware report of the speed-work build** (bottom-screen split over the last 10 s: aot / hle / ge / present / idle, texture hash, GPU wait) in the dorm, school and Dark Hour; it decides between the steps below. Host-side costs outside the game code were halved on the PC (`CURRENT_STATE.md`).
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
