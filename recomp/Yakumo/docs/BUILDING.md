# Building Yakumo

This page covers building the port from source on every platform. It explains where the time goes and how to work on the code without paying for a full build each time. The settings and environment variables of the running game are described in the [profile README](../profiles/mhp3rd/README.md).

You need your own copy of *Monster Hunter Portable 3rd HD Ver.* (`NPJB-40001`) as a disc image. Nothing else from the game is needed, and nothing from it is ever committed.

## How the build works

The game's executable and its 355 code overlays are MIPS code for the PSP. The build turns them into C++ and compiles that for your machine, in five stages:

| Stage | What happens | Command |
| --- | --- | --- |
| 1. Bootstrap | Build a small `Yakumo` that contains no game code yet | `cmake --build out/mhp3rd --target Yakumo` |
| 2. Prepare the executable | The bootstrap checks your disc image and prepares the decrypted `EBOOT.ELF` from it | `Yakumo --install image.iso` |
| 3. Generate | Analyse `EBOOT.ELF` and write the recompiled executable as 89 C++ units in `profiles/mhp3rd/generated/` | `profiles/mhp3rd/scripts/generate.sh` |
| 4. Compile the executable | Compile the 89 units together with the host (kernel, HLE, renderer, audio, interface) | `cmake --build out/mhp3rd --target Yakumo` |
| 5. Overlays | Extract the 355 code overlays from the disc image's `DATA.BIN`, recompile each one, and build a shared library per overlay | `profiles/mhp3rd/scripts/build_overlays.sh` |

The installer accepts only `NPJB-40001`: the disc id in `PARAM.SFO` must match, and so must the SHA-256 of the encrypted `EBOOT.BIN`. Fan translation patches that change only `USRDIR/DATA.BIN` keep that executable and are accepted. The English patch v6.1.0 has been checked: its `EBOOT.BIN` and all 355 code overlays are identical to the original's.

The generated code and the overlays are derived from your copy of the game. They stay on your machine: `generated/`, `overlays/`, `analysis/` and `game/` are ignored by Git.

### Where the time goes

Measured on real machines, from a fresh clone with an empty compiler cache:

| Stage | Apple M1, 8 GB (clang) | Steam Deck (gcc, 4 cores) |
| --- | --- | --- |
| 1–3: bootstrap, prepare, generate | minutes | minutes |
| 4: the 89 generated units | about 12–14 min | about 30–40 min |
| 5: the 355 overlays | about 40 min | several hours at `-j3` |
| Everything, fresh clone | about 1 h 50 min | about 5–6 h |

The generated units are very large, so compiling them needs a lot of memory. The build limits how many compile at once to one per 4 GiB of memory, whatever `-j` you pass (`PSPRECOMP_GENERATED_JOBS`). That is 2 on an 8 GB machine.

**It is slow only once.** With `ccache` installed, which the build uses automatically, a later full rebuild of unchanged code takes seconds. Changes to the port's own code rebuild only what they touch (see [Working on the code](#working-on-the-code)).

## macOS

Tested on Apple Silicon.

```bash
brew install cmake ninja python ccache pkg-config sdl3 molten-vk vulkan-loader vulkan-headers glslang
```

MoltenVK provides Vulkan on top of Metal. FFmpeg is not needed: the build makes its own; see [FFmpeg](#ffmpeg).

## Linux

Tested on Debian 13 and on the Steam Deck. Any distribution with SDL3 packages works the same way. On Debian 13 (trixie) or newer, and on distributions of similar age:

```bash
sudo apt install build-essential cmake ninja-build python3 ccache pkg-config \
    libsdl3-dev libvulkan-dev glslang-tools
```

Older releases, for example Ubuntu 24.04, have no SDL3 package. Build SDL3 from source there, or build in a Debian 13 container as described for the Steam Deck below.

The recompiled code nests deeply, so the main thread needs a 64 MiB stack. Start the game with a larger stack limit:

```bash
ulimit -s 65536 && out/mhp3rd/bin/Yakumo
```

### Steam Deck

SteamOS keeps its system read-only, so build inside a container and run the result on SteamOS itself. SteamOS 3.8 has glibc 2.41, the same as Debian 13, so a binary built in a Debian 13 container runs directly on the host, in Game Mode too.

```bash
# In Desktop Mode, in a terminal:
distrobox create --name yakumo --image debian:trixie
distrobox enter yakumo -- sudo apt update
distrobox enter yakumo -- sudo apt install -y build-essential cmake ninja-build python3 ccache pkg-config \
    libsdl3-dev libvulkan-dev glslang-tools
distrobox enter yakumo      # then follow the steps below inside the container
```

For Game Mode, add a small launch script to Steam as a non-Steam game. The script changes to the checkout, sets `ulimit -s 65536`, and runs `out/mhp3rd/bin/Yakumo`. Keep the Deck on its charger during long builds, and stop it from sleeping while one runs: a suspend in the middle of a large build can hang the Deck.

## Windows

> **Status:** the game builds with MSVC, including all 355 overlay DLLs, and plays. Creating a character, saving several times, restarting the game and loading the save have been tested on Windows 11. Please report problems with a **Test report** issue.

| Tool | Notes |
| --- | --- |
| MSVC: Visual Studio 2022 or newer, or just the free *Build Tools for Visual Studio* | The *Desktop development with C++* workload (MSVC and the Windows SDK). The IDE itself is not needed. `clang-cl` works on top of the same workload. MinGW has not been tried; the build files are written for MSVC |
| CMake 3.20 or newer and Ninja | Visual Studio's own copies work, and so do standalone ones on `PATH` |
| Python 3 | On `PATH` as `python3`. The `python3` that Windows ships by default only opens the Microsoft Store and does not work, so install Python and make sure its `python3` comes first on `PATH` |
| Git for Windows | Git Bash runs the `.sh` scripts in `profiles/mhp3rd/scripts/` |
| Vulkan SDK (LunarG) | The Vulkan loader and headers, plus `glslangValidator` for the shaders |
| SDL3 | For example the official `SDL3-devel-*-VC.zip`; pass its directory in `CMAKE_PREFIX_PATH` when configuring |
| FFmpeg | Nothing to install: the build downloads a pinned prebuilt FFmpeg and puts its DLLs next to the executable. See [FFmpeg](#ffmpeg) |

Plan for about 10 GB of free disk space and several GB of free memory.

Open the **x64 Native Tools Command Prompt** of your Visual Studio or Build Tools, so that the MSVC compiler is on `PATH`. From it, start Git Bash, so the scripts run with the same environment:

```bat
"C:\Program Files\Git\bin\bash.exe"
```

Then follow the [build steps](#build-steps) in that Bash, with these differences:
- **SDL3 location.** Add `-DCMAKE_PREFIX_PATH="C:/path/to/SDL3"` to the first `cmake` command.
- **Paths.** The executable is `out/mhp3rd/bin/Yakumo.exe`, and the per-user data directory is `$APPDATA/Yakumo/MHP3rd`.
- **DLLs.** Before playing, copy `SDL3.dll` next to `Yakumo.exe`, or put its directory on `PATH`. The FFmpeg DLLs are already there.

Windows specifics:
- **Data directory.** Step 2 writes `EBOOT.ELF` and `settings.ini` to the per-user data directory. It takes precedence over `profiles/mhp3rd/game`, and both hold the same data after step 3.
- **Symbolic links.** `prepare_game.sh` links the disc image into `profiles/mhp3rd/game`. Git Bash copies the file instead unless Windows Developer Mode is on and `MSYS=winsymlinks:nativestrict` is exported. A copy works too; it costs about 1.3 GB.
- **Overlay DLLs.** Each overlay links against the executable's import library. The runtime (`psprecomp_core`) is an object library, so its objects belong to the executable and `WINDOWS_EXPORT_ALL_SYMBOLS` exports them to the overlays.
- **No build lock.** Unlike macOS and Linux, the build does not lock its directory on Windows yet, so never run two builds of the same directory at once.
- **Out of memory** while compiling a generated unit means the parallelism is too high. Rerun the same `cmake --build` with `-j 1`; it continues where it stopped.

## Build steps

The same on every platform. On Windows, run them in Git Bash from a Visual Studio developer prompt.

```bash
git clone https://github.com/TeamGDB/Yakumo.git
cd Yakumo

# 1. Configure and build the bootstrap (no game code yet: a few minutes)
cmake -S . -B out/mhp3rd -G Ninja -DCMAKE_BUILD_TYPE=Release -DPSPRECOMP_PROFILE=mhp3rd
cmake --build out/mhp3rd --target Yakumo

# 2. Prepare EBOOT.ELF from your disc image. --in-place uses the image where it is;
#    without it the installer keeps its own copy (about 1.3 GB).
out/mhp3rd/bin/Yakumo --install /path/to/your.iso --in-place

# 3. Point the checkout at the image and the prepared executable.
#    The per-user data directory is ~/Library/Application Support/Yakumo/MHP3rd on macOS,
#    ~/.local/share/Yakumo/MHP3rd on Linux and %APPDATA%\Yakumo\MHP3rd on Windows.
profiles/mhp3rd/scripts/prepare_game.sh /path/to/your.iso "<per-user data directory>/EBOOT.ELF"

# 4. Generate the recompiled executable and compile it (the first long step)
profiles/mhp3rd/scripts/generate.sh
cmake -S . -B out/mhp3rd
cmake --build out/mhp3rd --target Yakumo

# 5. Recompile the code overlays (the longest step; resumable)
profiles/mhp3rd/scripts/build_overlays.sh

# 6. Play
out/mhp3rd/bin/Yakumo
```

Check the configure output for `mhp3rd: Vulkan renderer enabled`. Without SDL3, Vulkan or `glslangValidator`, configuration still succeeds but builds a program with no window.

Stage 5 is resumable. If it stops, run it again: overlays that are already built are skipped. You can play before it finishes. An overlay without its library runs in an interpreter, which works but is much slower, so hunts stutter until the overlays are built.

### FFmpeg

The streamed music (ATRAC3, ATRAC3plus) and the movies (H.264) are decoded by FFmpeg's `libavcodec`. FFmpeg is part of the normal build and needs no setup. `MHP3RD_FFMPEG` chooses where it comes from:

| `MHP3RD_FFMPEG` | What you get |
| --- | --- |
| `bundled` (default) | The first configure of a build directory downloads FFmpeg 7.1.5, checks its SHA-256 and builds a minimal LGPL configuration with just the three decoders the game uses. This takes a few minutes, once; later configures and builds reuse it. The libraries go to `out/mhp3rd/bin/lib/`, which the executable finds through its rpath, so the game needs no FFmpeg on the system. On Windows, see below |
| `system` | The `libavcodec` and `libavutil` that `pkg-config` finds, for example from `brew install ffmpeg` or `apt install libavcodec-dev libavutil-dev`. Configure stops if there are none |
| `OFF` | No FFmpeg. Not recommended: the game then has no music and skips its movies, and configure warns about it |

The bundled build needs `make` and a C compiler, which the tools above already include. Offline, put `ffmpeg-7.1.5.tar.xz` (or the Windows `.zip`) in `out/mhp3rd/_deps/downloads/` before configuring. `profiles/mhp3rd/cmake/FFmpeg.cmake` holds the pinned versions and checksums.

**On Windows** FFmpeg's `configure` needs a POSIX shell and `make`, which the MSVC toolchain lacks, so `bundled` downloads a pinned prebuilt instead: the LGPL shared build of FFmpeg 7.1.5 from [BtbN/FFmpeg-Builds](https://github.com/BtbN/FFmpeg-Builds), checked against its SHA-256. It needs no extra setup. Configure copies `avcodec-61.dll`, `avutil-59.dll` and `swresample-5.dll`, with FFmpeg's licence, next to `Yakumo.exe`, and reports `mhp3rd: bundled FFmpeg 7.1.5 (prebuilt, LGPL); music and movies enabled`.

## Working on the code

### What a change costs

The generated code is its own object library, and every compile goes through `ccache`. So the cost of a change depends on what it touches:

| You change | What rebuilds | Cost |
| --- | --- | --- |
| Host code: kernel, HLE, renderer, audio, interface, settings | The files you changed, then a relink | Seconds |
| A shader | The embedded shaders and the renderer | Seconds |
| The runtime's sources in `src/` | Those files, then every tool and executable that links the runtime | Seconds to a minute |
| `profiles/mhp3rd/CMakeLists.txt` (flags, sources) | Host files only; the generated units have their own flags | Seconds to a minute |
| Headers in `include/psprecomp/` (the runtime the generated code uses) | All 89 generated units **and** all 355 overlays | The full build again, unless `ccache` already holds that exact version |
| The recompiler, then `generate.sh` | Only the units whose generated text changed | Depends on the change |

Keep `include/psprecomp/` stable when you can. It is the one place where a small edit costs hours.

### Several checkouts

Work on separate features in separate clones without paying the long stages again:

- **Generated code:** copy `profiles/mhp3rd/generated/` from a checkout that already has it. A plain `cp -R` is fine. Don't use copies that preserve old timestamps (`cp -p`, `rsync -a`, `tar`), because Ninja may then think the objects are newer than the sources.
- **Overlays:** don't rebuild them per clone. Point the game at one built set with `MHP3RD_OVERLAY_DIR=/path/to/out/mhp3rd/bin/overlays`. It is safe as long as `include/psprecomp/` is the same in both checkouts.
- **Compiler cache:** `ccache` is shared by every checkout on the machine. A second clone at another path compiles almost entirely from the cache.
- **Game data:** `MHP3RD_GAME_DIR=/path/to/game` points a build at a game directory (disc image, `EBOOT.ELF`, `ms0/` saves), so clones need no `prepare_game.sh` of their own.

### Running several instances

For multiplayer tests or before/after comparisons, give each instance its own settings and saves, and a window title so you can tell the windows apart:

```bash
MHP3RD_DATA_DIR=~/yakumo-a MHP3RD_GAME_DIR=~/game-a MHP3RD_WINDOW_TITLE="Yakumo A" out/mhp3rd/bin/Yakumo
```

### Rules the build enforces

- **One build per build directory at a time.** On macOS and Linux, `cmake --build` takes a lock, so a second build waits for the first. Run builds through `cmake --build`, not `ninja` directly, because running `ninja` directly bypasses the lock. Windows has no lock yet: don't start two builds of one directory there.
- **Keep `.ninja_deps` and `.ninja_log`.** Deleting them forces a full rebuild. A damaged dependency log is repaired automatically before each build.

[BUILD_SYSTEM.md](BUILD_SYSTEM.md) explains why these rules exist.

### Tests and diagnostics

- `cmake --build out/mhp3rd --target psprecomp_tests mhp3rd_savedata_tests`, then `ctest --test-dir out/mhp3rd`, builds and runs the unit tests of the recompiler framework and of the save-data format. Neither needs game data.
- [TESTING.md](TESTING.md) is the manual smoke test: about fifteen minutes through every part of the game that works.
- `MHP3RD_PERF=1`, or F3 in the game, shows the performance overlay. The `MHP3RD_TRACE_*` variables log individual subsystems. All of them are listed under *Diagnostics* in the [profile README](../profiles/mhp3rd/README.md#diagnostics).
- `MHP3RD_NO_RENDER=1` runs without a window, which is useful for quick boot checks in scripts. Bound such runs with `timeout`.

## Troubleshooting

| Symptom | Cause and fix |
| --- | --- |
| `mhp3rd: renderer disabled` | SDL3, Vulkan or `glslangValidator` was not found. Install them and configure again |
| The compiler is killed, or the machine swaps, while compiling generated units | Too many large units at once: configure with `-DPSPRECOMP_GENERATED_JOBS=1` |
| The game crashes right after starting on Linux | The main thread's stack is too small: start it after `ulimit -s 65536` |
| `another build is running` | A build of the same directory is still going. Wait, or find the leftover `ninja` process |
| No music and no movies | The build was configured with `-DMHP3RD_FFMPEG=OFF`, or the FFmpeg libraries next to the executable are missing; see [FFmpeg](#ffmpeg) |
| `No game data found` | Run step 2 (`--install`) or step 3 (`prepare_game.sh`), or set `MHP3RD_GAME_DIR` |
