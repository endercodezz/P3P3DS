# P3P3DS — Next Steps

Updated 2026-11-05. What is done and measured lives in [`CURRENT_STATE.md`](CURRENT_STATE.md); this file only lists what comes next, in the maintainer's priority order. Every step names how it will be verified (CLAUDE.md section 3: measure first). Each step is tracked as a [GitHub issue](https://github.com/endercodezz/P3P3DS/issues) and lands through a pull request that closes it (CLAUDE.md section 16).

## Where the project stands (0.3.3)

- Whole `.text` statically recompiled, HLE kernel, VFS with the mod chain, GE executor, deterministic PC runner with scripted input; the PC route plays through the first battle (maintainer's save, local route).
- New 3DS (0.3.0/0.3.1, New 2DS XL): PICA200 renderer with a vertex shader (transform, skinning, lighting), GE back end on core 2, real-time pacing, save/load menu on the bottom screen. Dorm 28-30 fps, dialogue mostly 30, battle 16-25 fps; a user's 0.3.1 run at 97-99 % speed; no sound, no opening movie.
- 0.3.2 includes the startup logo fix (4d198db, verified in Azahar and reported working by the maintainer on console in [#5](https://github.com/endercodezz/P3P3DS/issues/5)).
- 0.3.3: texture packs made for PPSSPP convert with `tools/texpack` and replace textures on the PC ([#62](https://github.com/endercodezz/P3P3DS/pull/62)); the New 3DS path (`sdmc:/p3p3ds/textures`) is in the build but not yet tested on Azahar or hardware ([#23](https://github.com/endercodezz/P3P3DS/issues/23)). The Builder shows when a newer release exists ([#67](https://github.com/endercodezz/P3P3DS/issues/67)).
- P3P3DS Builder for Windows and Linux; releases built by GitHub Actions.

## 0. Check fades and transitions on hardware

Check whether the 0.3.2 texture rebind fix also changes fades or black frames during scene transitions ([#3](https://github.com/endercodezz/P3P3DS/issues/3)).

## 1. Battle speed (priority) — [#16](https://github.com/endercodezz/P3P3DS/issues/16)

16-25 fps / 63-86 % of real time on hardware (0.3.0); 0.2.7 measured 5-7.5 fps. Goal: the game's full 30 (in battle the game shows 30 frames per game second). With the GE on core 2 the game's thread rarely waits for the worker (`wait` 0.0-4.3 %) and `aot` is 63-70 %: the battle is bound by the game's own code [VERIFIED, `CURRENT_STATE.md` 2026-10-11].

1. Profile a battle window on the PC production build (`--sample`, `tools/profile_symbols.py`) and on hardware (`sdmc:/p3p3ds/profile_seconds.txt`).
2. Candidates, each measured before it is kept: hot guest units with `-O2` and instruction scheduling (AOT units build with `-Os -fno-schedule-insns2`, only size was measured); a cheaper path for the ~360 `sceKernelCpuSuspendIntr`/`ResumeIntr` pairs per frame; free heap first if larger code needs it ([#18](https://github.com/endercodezz/P3P3DS/issues/18)).
3. Do not wait for the GPU every frame if GPU wait becomes visible on hardware.

## 2. Field at a stable 30 fps — [#17](https://github.com/endercodezz/P3P3DS/issues/17)

Dorm 28-30 fps on hardware (0.3.0). Test first whether the UMD-speed read latency applied to every file (`IoManager::transfer_us`) loses frames in game time: dorm route on the PC runner with near-zero latency.

## 3. Sound — [#14](https://github.com/endercodezz/P3P3DS/issues/14)

The PC mixer already produces correct game audio (`sceAudio`, `sceSasCore`, maintainer-verified WAV). Missing: an `ndsp` output on the 3DS fed from the virtual-clock mixer, on a separate thread/core. Verify: audio on hardware without drops; the bottom-screen report counts late/dropped buffers. First decide what the output does when the game runs below 100 % speed (the virtual clock then runs slower than the DSP; [#15](https://github.com/endercodezz/P3P3DS/issues/15)).

## Correctness defects (code review of 2026-10-08)

Reproduced in unit tests or confirmed in code, not yet seen to change P3P's behaviour; fix one per pull request, each with a regression test and an unchanged replay:

- HLE: thread stacks vs heap and memory reuse ([#7](https://github.com/endercodezz/P3P3DS/issues/7)), mutexes on owner exit ([#8](https://github.com/endercodezz/P3P3DS/issues/8)), event-flag `outBits` ([#9](https://github.com/endercodezz/P3P3DS/issues/9)), `sceIoOpenAsync` errors ([#10](https://github.com/endercodezz/P3P3DS/issues/10)), save-name validation ([#13](https://github.com/endercodezz/P3P3DS/issues/13)).
- GE: culling rules ([#6](https://github.com/endercodezz/P3P3DS/issues/6)), callback and signal timing ([#11](https://github.com/endercodezz/P3P3DS/issues/11)); portrait fringe ([#4](https://github.com/endercodezz/P3P3DS/issues/4)).
- Recompiler: FPU/VFPU edge cases ([#12](https://github.com/endercodezz/P3P3DS/issues/12)).
- CI and tests: release without tests ([#25](https://github.com/endercodezz/P3P3DS/issues/25)), partial skips reported as passed ([#26](https://github.com/endercodezz/P3P3DS/issues/26)), game-specific values outside the profile ([#27](https://github.com/endercodezz/P3P3DS/issues/27)).

## Later

- **Saves**: compatibility with PSP/PPSSPP saves ([#20](https://github.com/endercodezz/P3P3DS/issues/20)).
- **Opening movie**: H.264 + ATRAC3plus decoding ([#19](https://github.com/endercodezz/P3P3DS/issues/19), plan below); until then the movie is skipped.
- **`.cia` packaging** from the Builder ([#21](https://github.com/endercodezz/P3P3DS/issues/21)).
- **Mods**: mods from `p3p-cpk bind` run in Azahar; still open are a hardware test, `.spr` files, EBOOT patches in the Builder and texture replacement ([#23](https://github.com/endercodezz/P3P3DS/issues/23)); a 60 fps patch only as a separate mod (the game paces itself with `WaitVblankStartMultiCB(2)` and its logic is per frame [INFERRED]).
- Builder: detection of devkitPro installs in other places ([#22](https://github.com/endercodezz/P3P3DS/issues/22)).

## Planned, but not soon: Old 3DS / 2DS — [#24](https://github.com/endercodezz/P3P3DS/issues/24)

Requested by many players; it needs a different approach, not tuning, so it comes after the New 3DS version is playable. [VERIFIED] 0.2.9 stops cleanly at start on an Old 3DS: it cannot allocate the 32 MiB PSP RAM (`CURRENT_STATE.md`, 2026-10-11). About 112 MB of application memory are needed (about 41 MB code, 32 MiB PSP RAM, 10-15 MB function tables), while the Old 3DS offers at most 96 MB and a 268 MHz CPU without L2 cache (New 3DS: 804 MHz with L2). Directions to measure first: much smaller code (compile only hot code ahead of time, interpret the rest), less host memory per PSP byte, and whether any frame rate is reachable at 268 MHz at all.

---

### Render correctness reference

The maintainer compared the first-day screens (logos, title, main menu, protagonist selection, Dark Hour lobby, name entry, Mitsuru introduction, own room, next morning, school lobby) and the first battle with the original game. New renderer work is checked against these screens, the PC renderer's frames of the same route and the deterministic frame hashes of the input scripts in `profiles/p3p/input/`. On the 3DS, frame dumps (`sdmc:/p3p3ds/dump_every.txt`) are compared with the PC frames.

### Movie and audio decoding plan (sceMpeg / ATRAC3plus)

Interface in place: `core/include/p3p3ds/hle/mpeg.hpp` `MpegDecoder` receives each demultiplexed AU (`decode_video` writes a picture into guest memory, `decode_audio` writes 2048 stereo S16 samples). `BlankMpegDecoder` (black / silence) is the default. Container behaviour is verified against `pspautotests/tests/video/mpeg/basic.expected`. P3P's only movie is `USRDIR/sound/pmsf/P3OPMV_P3P.pmsf` (`_P3PB` variant): H.264 2,997 AUs at 29.97 fps, ATRAC3plus 2,156 frames of 752 bytes.

1. PC: an H.264 Baseline/Main decoder + ATRAC3plus decoder behind `MpegDecoder`. Candidates must be license-compatible and fetched as references, not vendored; PPSSPP uses FFmpeg — reference only, no code copy. Verify: decoded frame hashes against a reference decode of the same AUs; audio PCM against a reference decoder.
2. 3DS: the New 3DS has an MVD hardware H.264 service (`mvd:STD` in libctru) [UNVERIFIED for this stream profile]; ATRAC3plus needs a software decoder on ARM11 [UNVERIFIED cost]. Measure before choosing; an alternative is converting the user's movie to a 3DS-native format in the Builder.
3. The game's own `sceAtrac3plus` use for BGM (not yet reached) shares the ATRAC3plus decoder.
