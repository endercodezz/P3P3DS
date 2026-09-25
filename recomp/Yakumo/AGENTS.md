# AGENTS.md

Working notes for anyone changing this repository: people and coding agents alike. Read this first. It is short on purpose and links to the longer documents instead of repeating them.

Yakumo is a native port of *Monster Hunter Portable 3rd HD Ver.* (`NPJB-40001`). It recompiles the game's PSP (MIPS) code to C++ ahead of time and supplies the PSP system around it: the kernel, HLE modules, a Vulkan GE renderer, audio, input, save data, ad hoc networking and an ImGui interface. The C++ lives in `profiles/mhp3rd/host/` (the port) and `include/psprecomp/` plus `src/` (the reusable runtime and recompiler).

## Rules

- **English only** in everything committed: code, comments, docs, commit messages, pull requests. The only exceptions are the translations `README.ru.md` and `README.es.md`, and they change in the same pull request as `README.md`.
- **Branch and pull request.** Never push to `main`. Pull requests are merged by rebase; keep a clean, reviewable commit series.
- **No game data, ever.** Disc images, `EBOOT`/`DATA.BIN` contents, the generated code (`profiles/mhp3rd/generated/`), overlay corpora, and saves stay local; they are ignored by Git. The same goes for anything personal: home paths, user names, machine names, addresses.
- **Write it yourself.** Read public documentation and other projects to understand the PSP, file formats and protocols. Never copy, paste or line-by-line translate code from a project whose licence is incompatible with this repository's MIT licence. Constants, offsets and format facts are fine. Record where intentionally included third-party code comes from; see [SOURCE_PROVENANCE.md](docs/SOURCE_PROVENANCE.md).
- **Say what you did not verify.** A pull request lists what was tested, on which platform, and what was not.

## Building without waiting hours

[docs/BUILDING.md](docs/BUILDING.md) is the full guide. A full build from a fresh clone takes about two hours on an M1 and longer on weaker machines. Most of that can be skipped:

- **Generated code.** Copy `profiles/mhp3rd/generated/` from a checkout that already has it with a plain `cp -R`, not a copy that keeps old timestamps.
- **Overlays.** Don't rebuild them per checkout: `MHP3RD_OVERLAY_DIR=/path/to/out/mhp3rd/bin/overlays`. This is safe while `include/psprecomp/` is unchanged.
- **ccache.** Install it; the build uses it automatically, across checkouts.
- **Game data.** Set `MHP3RD_GAME_DIR` to a game directory instead of running `prepare_game.sh` in every clone.
- **Host-only changes rebuild in seconds.** Changes under `include/psprecomp/` rebuild everything, including all 355 overlays, so avoid them unless they are the point.
- **Build commands.** Build with `cmake --build`, never `ninja` directly: `cmake --build` holds the per-directory lock. Run one build per build directory. Keep parallelism low (`-j2`), because generated units need gigabytes of memory each. Never delete `.ninja_deps` or `.ninja_log`.

## Running and testing

- **Bound every run.** `timeout 60 out/mhp3rd/bin/Yakumo`. Never leave a game running, and never drive it with an open-ended input loop, such as pressing confirm forever: it does not converge, and someone may be watching the screen.
- **Quick boot checks.** `MHP3RD_NO_RENDER=1 MHP3RD_NO_AUDIO=1 timeout 40 …`, then look for the function count and `[overlay] installed` in the output.
- **Scripted input and captures.**
  - `MHP3RD_INPUT_SCRIPT` sends keys, virtual gamepad input and dropped files, and captures the window. The syntax is in `profiles/mhp3rd/host/ui/input_script.hpp`.
  - `MHP3RD_SCREENSHOT_DIR` captures the game's own frames.
  - Look at the captures; don't assume.
- **Several instances.** For multiplayer or before/after comparisons, give each instance its own `MHP3RD_DATA_DIR`, its own saves, and an `MHP3RD_WINDOW_TITLE`.
- **Numbers.**
  - `MHP3RD_PERF=log` prints one line per second: fps, the game's own frame rate, emulation speed, and guest/render/wait time.
  - Speed must stay at 100%; the game runs at 30 frames per emulated second.
- **Tracing.**
  - `MHP3RD_TRACE_*` variables log one subsystem each: GE, material and lighting registers, save data, fonts, pad, audio, ATRAC, MPEG, ad hoc, I/O, kernel.
  - All the variables are listed under *Diagnostics* in the [profile README](profiles/mhp3rd/README.md#diagnostics).
- **Unit tests.** `cmake --build out/mhp3rd --target psprecomp_tests mhp3rd_savedata_tests && ctest --test-dir out/mhp3rd`.
- **Manual smoke test.** [TESTING.md](docs/TESTING.md), about fifteen minutes. [COMPATIBILITY.md](docs/COMPATIBILITY.md) records results per platform, always with the commit that was tested.

## Lessons that cost real time

- **Trace the hardware; don't recall it.** GE register numbers, PSP struct layouts and HLE semantics taken from memory have been wrong, and each wrong guess cost a debugging session. Add or use a `MHP3RD_TRACE_*` switch and read what the game actually does.
- **Measure before changing.** Find the cause with a trace, a profile or a number, then change code. Successive guesses at a rendering bug from screenshots failed four times in a row.
- **Compare like with like.**
  - Compare a known-good build and yours at the same frame of the same scripted run.
  - A model that isn't on screen yet looks the same in a broken and a working build.
  - The guest clock is seeded from the wall clock, and a connected gamepad changes input, so pin both when you need pixel-identical runs.
- **Give new behaviour an off switch.** An environment variable that restores the old behaviour lets anyone compare both on the same screen.
- **Logs.** stdout redirected to a file is block-buffered. A truncated last line means the buffer hasn't been flushed, not that the program has stopped.
- **The game's view differs from the host's.** A value the game reads every frame (a buffer count, a status word) must mean exactly what it means on a PSP. A wrong one breaks things far from where it is read. Ad hoc quests dropped because a byte count grew instead of reporting what was waiting.
- **Performance problems in the renderer are usually memory reads.** Resolve guest memory once per draw, not per element. Profile with `perf` (Linux) or Instruments (macOS) before optimising.
- **Long builds on a handheld.** Keep it on its charger and block suspend while the build runs: a suspend during a large build can hang the machine. Start long jobs detached (`systemd-run --user …`), so a dropped SSH session doesn't kill them.

## Pull requests

- Open a draft pull request early and push after every commit that builds. Work that exists only locally is lost when a session ends.
- The description says what changed and why, how it was verified (platform, commit, what you looked at), what was not verified, and which issues it closes or references.
- Update the docs that the change makes wrong: the profile README for settings and variables, `docs/BUILDING.md` for the build, and `docs/COMPATIBILITY.md` for verified results.
