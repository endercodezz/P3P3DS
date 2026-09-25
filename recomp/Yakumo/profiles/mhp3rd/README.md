# MHP3rd HD profile

This profile builds `Yakumo` for **Monster Hunter Portable 3rd HD Ver.** (`NPJB-40001`). The PS3 release ships an ordinary PSP UMD image; this profile recompiles that PSP executable and its code overlays into a native program. No executable, game data or code generated from them is part of the repository — you supply your own copy of the game.

## Status

The game boots, loads its overlays, creates a character or loads a save, walks the village, plays hunts alone or in ad hoc multiplayer and saves, with sound, music, movies and lighting. It can be played with fully rebindable keyboard and mouse controls or a gamepad. The simulation remains at the PSP's 30 frames per second, with optional presentation at 45, 60, 90, 120 or the display's refresh rate through frame interpolation.

| Area | State |
| --- | --- |
| Code | The whole executable (362 478 instructions, 89 units) and all 355 code overlays are recompiled ahead of time; an interpreter covers anything they miss |
| Kernel | Threads with a deterministic virtual clock, semaphores, event flags, mutexes, callbacks, VTimers, partition memory, VBlank interrupts, file I/O straight from the disc image |
| Imports | 244 of 296 implemented; the rest are logging stubs that return 0 |
| Graphics | Vulkan: textures (palettes, DXT, swizzle), skinning, per-vertex lighting (four directional, point or spot lights and the full material model) and fog, blending, depth and alpha test, sprites, per-framebuffer render targets, arbitrary window shapes and frame interpolation; PPSSPP-compatible HD texture packs are supported |
| Audio | `sceSasCore` voice mixing, `sceAudio` output and ATRAC3 music through `sceAtrac3plus` |
| Movies | PSMF playback through `sceMpeg` and `sceJpegCsc`: H.264 video and ATRAC3plus sound |
| Input | Fully rebindable keyboard and mouse controls; SDL3 gamepads with an analog camera and proportional bow and bowgun aim on the second stick, plus optional trigger profiles |
| Text | `sceLibFont` glyphs rasterized from a host TrueType font |
| Saves | The save-data utility, with saves in the PSP's own format: a save copied from a PSP loads, and one made here can be copied back; the menu imports, exports and backs up saves |
| Interface | Gamepad, keyboard and mouse driven first-run setup, file browser, in-game settings, performance statistics and on-screen keyboard |
| Multiplayer | Ad hoc play through PSP ad hoc servers: two instances have met in a gathering hall and started a quest together; play with PPSSPP and on public servers is still to be tested. See [Multiplayer](#multiplayer-ad-hoc) |

Not done yet:

- **Curved surfaces** (Bézier and spline patches).
- **Infrastructure networking** (`sceHttp`, `sceNetInet`): the game's download mode. Ad hoc multiplayer works.
- **Dialog screens.** The save-data and message dialogs work but draw nothing; each answers as if the player confirmed it ([#33](https://github.com/TeamGDB/Yakumo/issues/33)).

Tested on macOS (Apple Silicon, Vulkan through MoltenVK), on a Steam Deck in Game Mode, built with GCC in a Debian 13 container and running on native Vulkan, and on Windows 11 with MSVC; see [the compatibility table](../../docs/COMPATIBILITY.md).

## Supported executable

| Item | Value |
| --- | --- |
| Module | `MonsterHunterPortable3rd` 1.1 |
| Encrypted `SYSDIR/EBOOT.BIN` SHA-256 | `79e25f3512d56e0f7bf5c48351d7d0d255269675ffc8811ac5599322bb66945e` |
| Decrypted `EBOOT.ELF` SHA-256 | `55c0598436c0753b04331f8e95d406f832d9217806e3a896fed0e88b33637d8c` |
| Load image | `0x08804000`–`0x0A285200` (needs 64 MiB RAM) |
| Entry / `gp` | `0x0882170C` / `0x08A2A680` |
| Imports | 296 across 35 libraries |

`config/mhp3rd_npjb40001.toml` records the same identity plus the overlay slot layout.

## Requirements

- CMake 3.20 or newer, Ninja and a C++20 compiler
- Optional: `ccache`, which the build uses automatically when it is installed
- Python 3
- SDL3, Vulkan (the loader and headers; MoltenVK on macOS) and `glslangValidator`
- `make` and a C compiler on macOS and Linux, to build FFmpeg (see below)

If SDL3, Vulkan or `glslangValidator` is missing, configuration still succeeds but builds the game **without a window**: CMake prints `mhp3rd: renderer disabled` and the program runs headless. Check for `mhp3rd: Vulkan renderer enabled` in the configure output.

The streamed music (ATRAC3) and the movies (H.264 with ATRAC3plus sound) are decoded by FFmpeg's shared `libavcodec` and `libavutil`. FFmpeg is part of the normal build; `MHP3RD_FFMPEG` chooses where it comes from (`cmake/FFmpeg.cmake` holds the pins):

- `bundled`, the default. The first configure of a build directory downloads FFmpeg 7.1.5, checks its SHA-256 and builds it with only the ATRAC3, ATRAC3plus and H.264 decoders, as LGPL-2.1-or-later shared libraries (configure stops if the result is not LGPL only). This takes a few minutes, once per build directory; a new version or configuration rebuilds it. The libraries and FFmpeg's licence go to `out/mhp3rd/bin/lib/`, which the executable finds through its rpath, so the game needs no FFmpeg on the system. On Windows, where FFmpeg's `configure` does not run with MSVC, it downloads a pinned, checksum-verified prebuilt LGPL shared FFmpeg 7.1.5 and puts its DLLs next to `Yakumo.exe` (see [BUILDING.md](../../docs/BUILDING.md#ffmpeg)). Configuration reports `mhp3rd: bundled FFmpeg 7.1.5 …; music and movies enabled`. To build offline, put the archive in `out/mhp3rd/_deps/downloads/` (`MHP3RD_FFMPEG_DOWNLOAD_DIR`) first.
- `system` uses the FFmpeg that `pkg-config` finds, for example from `brew install ffmpeg` on macOS or `apt install libavcodec-dev libavutil-dev` on Debian and Ubuntu, and stops if there is none.
- `OFF` builds without FFmpeg. This is possible but not recommended: the game then has no music and skips its movies, and configure prints a warning saying so.

Expect a full build to need several gigabytes of memory and some time: the generated code is large. With Ninja, the build compiles at most `PSPRECOMP_GENERATED_JOBS` generated units at once, whatever `-j` you pass; the default is one per 4 GiB of memory, so 2 on an 8 GB machine. Set it when configuring, for example `-DPSPRECOMP_GENERATED_JOBS=1`.

## Quick start

```bash
# 1. Game data: link your disc image and decrypted executable into profiles/mhp3rd/game
#    (or let the program set itself up from the image; see "Game data" below)
profiles/mhp3rd/scripts/prepare_game.sh "/path/to/your.iso" /path/to/EBOOT.ELF

# 2. Recompile the executable
cmake -S . -B out/mhp3rd -G Ninja -DCMAKE_BUILD_TYPE=Release -DPSPRECOMP_PROFILE=mhp3rd
profiles/mhp3rd/scripts/generate.sh
cmake -S . -B out/mhp3rd                          # pick up the generated units
cmake --build out/mhp3rd --target Yakumo -j 2

# 3. Recompile the code overlays (about 40 minutes, resumable)
profiles/mhp3rd/scripts/build_overlays.sh

# 4. Play
out/mhp3rd/bin/Yakumo
```

Each step is described below. [`docs/BUILDING.md`](../../docs/BUILDING.md) covers the platforms, how long each stage takes, and working on the code without full rebuilds.

## Game data

The program needs two things from your own copy of the game: the disc image and the game's executable. It finds them in this order:

1. A directory given on the command line or in `MHP3RD_GAME_DIR`.
2. The **per-user data directory** that the installer fills.
3. `profiles/mhp3rd/game` in the checkout, set up by `prepare_game.sh`. A release build (`-DMHP3RD_RELEASE=ON`, see [Release builds](#release-builds)) has no checkout and skips this.

If neither the per-user directory nor `profiles/mhp3rd/game` holds game data, the program starts its installer instead of the game.

### Installer

The installer needs only your disc image. It runs as a few screens in the game's window, all of them usable with a gamepad alone, a keyboard or a mouse:

1. **Welcome**: what is needed and where the data goes.
2. **Choose the disc image** in Yakumo's own file browser. It starts in your home folder (later in the folder where you last found an image) and lists folders and `.iso` files with their sizes; *Showing .iso only* switches to all files. The row of places above the list holds Home, Downloads, Desktop and Documents, and every removable drive: SD cards and USB drives under `/run/media` and `/media` on Linux (a Steam Deck's SD card among them), volumes under `/Volumes` on macOS, drive letters on Windows. Confirm opens a folder or picks a file; back goes up a folder, and from the top back to the welcome screen. On a gamepad, △ (Y) switches between .iso files and all files. A file dropped onto the window is taken as well, on this screen and on the welcome screen. *System dialog…* opens the system's file dialog instead; it is not offered under gamescope (Steam Deck Game Mode), where that dialog does not appear.
3. **Checks**: the image must be `NPJB-40001` (the disc id in `PARAM.SFO` and the SHA-256 of the encrypted executable). A wrong release or region, a modified image, a PlayStation 3 disc image, a compressed (`.cso`) image or a file that is no disc image at all each get a screen that says so plainly, with *Choose another file*.
4. **Copy or use in place**: copying (the default) puts the image (about 1.3 GB) into the per-user directory, so the game keeps working after the original is moved or deleted; the screen shows the free space and refuses the copy when there is not enough. Using the image where it is saves the space; the program then checks on every start that the image is still there and says so if it is not, offering to run the setup again.
5. **Progress**: a progress bar for the copy and for preparing the game's executable from the image, which is then checked against the hash in the table above. The work runs off the window's thread, so the window stays responsive; *Cancel* (or back) stops it and removes what it wrote.

The game then starts. Later starts go straight to the game. The in-game menu's *Set up game data again…* runs the same setup: the game closes and the program starts again with `--install`.

The per-user directory is SDL's preference path for `Yakumo/MHP3rd`:

| System | Directory |
| --- | --- |
| macOS | `~/Library/Application Support/Yakumo/MHP3rd/` |
| Linux | `~/.local/share/Yakumo/MHP3rd/` (or under `$XDG_DATA_HOME`) |
| Windows | `%APPDATA%\Yakumo\MHP3rd\` |

It holds `EBOOT.ELF`, `disc.iso` when the image was copied, `settings.ini`, which records where the image is and keeps the settings of the [in-game menu](#in-game-menu), `ms0`, the memory stick with the [saves](#saving-and-loading), `textures/NPJB40001` when you install an [HD texture pack](#hd-texture-packs), `pipeline_cache.bin`, the graphics pipelines compiled in earlier runs, and `pipeline_keys.bin`, the list of them the next run makes in the background from the start (deleting either only makes the next run compile them again). `MHP3RD_DATA_DIR` points the program at another directory. The Flatpak keeps this directory inside its own data directory, `~/.var/app/io.github.teamgdb.Yakumo/data/Yakumo/MHP3rd/`.

Saves made before `ms0` moved here stay where they were, in `profiles/mhp3rd/game/ms0`: a developer build keeps using them, and says so at start, until the per-user directory has an `ms0` of its own. Move the folder there to switch.

The same setup runs without any screens from a terminal, for scripts and headless machines:

```bash
out/mhp3rd/bin/Yakumo --install "/path/to/your.iso"             # copy the image
out/mhp3rd/bin/Yakumo --install "/path/to/your.iso" --in-place  # use it where it is
out/mhp3rd/bin/Yakumo --install                                 # run the setup screens again, then play
```

`--install` with an image prepares everything and exits. A build without generated code can already run it, and the `EBOOT.ELF` it writes into the per-user directory is the executable `generate.sh` needs.

When the window cannot be created, for example without a working Vulkan driver, the installer falls back to SDL3 message boxes and the system file dialog (on Linux through the desktop portal or `zenity`). A build without SDL shows neither and prints the `--install` command instead.

### Checkout directory

For development, `profiles/mhp3rd/game` works as before, with an executable decrypted by an external tool:

1. Decrypt `PSP_GAME/SYSDIR/EBOOT.BIN` from your own image with an external tool, or take the `EBOOT.ELF` that `--install` wrote into the per-user directory. `tools/extract_iso.py <iso> <dir> /PSP_GAME/SYSDIR/EBOOT.BIN` extracts the encrypted file; the table above gives the hashes to check both files against.
2. Populate `profiles/mhp3rd/game` (ignored by Git):

   ```bash
   profiles/mhp3rd/scripts/prepare_game.sh "/path/to/your.iso" /path/to/EBOOT.ELF
   ```

The image itself is not unpacked: `game/disc.iso` links to it and the host reads files, and raw `sce_lbn` sectors, from it directly. `game/ms0` backs `ms0:` for save data. If you move the image later, rerun the script — the links point at absolute paths.

When the per-user directory also holds an installation, it takes precedence; start with `profiles/mhp3rd/game` as an argument, or set `MHP3RD_GAME_DIR`, to use the checkout.

## Build

```bash
cmake -S . -B out/mhp3rd -G Ninja -DCMAKE_BUILD_TYPE=Release -DPSPRECOMP_PROFILE=mhp3rd
profiles/mhp3rd/scripts/generate.sh          # writes analysis/ and generated/
cmake -S . -B out/mhp3rd                     # pick up the generated units
cmake --build out/mhp3rd --target Yakumo -j 2
```

`generate.sh` analyzes the executable and writes the recompiled C++ into `generated/`. That corpus is derived from your copy of the game, so it stays local and is never committed.

Rerunning `generate.sh` rewrites only the units whose code changed, so the build after it recompiles only those.

The build protects its incremental state on macOS and Linux:

- **One build at a time.** With Ninja, CMake runs builds through a wrapper, `out/mhp3rd/ninja-locked`, that locks the build directory. A second `cmake --build` of the same directory, from a script or by hand, prints `another build is running` and waits. The lock belongs to the running Ninja, so interrupting `cmake --build` or a script around it does not let a new build start next to the Ninja that is still running. Running `ninja` directly bypasses the lock. `-DPSPRECOMP_BUILD_LOCK=OFF` turns it off. On Windows there is no lock yet: do not start two builds of one directory.
- **Damaged dependency log.** Before each build the wrapper checks `.ninja_deps` and repairs a damaged one with `ninja -t recompact`, which keeps every intact record. Do not delete `.ninja_deps` or `.ninja_log`: either costs a full rebuild.
- **Compiler cache.** If `ccache` is installed, every compile goes through it, so a rebuild of unchanged code takes seconds instead of minutes, also across checkouts at different paths. `-DPSPRECOMP_CCACHE=OFF` turns it off.

CMake prints a warning for Ninja 1.13.2, which cannot recover from a damaged dependency log by itself (upstream issue [#2703](https://github.com/ninja-build/ninja/issues/2703)). Building through `cmake --build` works around it; the fix is due in Ninja 1.14. [`docs/BUILD_SYSTEM.md`](../../docs/BUILD_SYSTEM.md) explains why incremental state gets lost and what the build does about it.

## Release builds

Players can use a prebuilt release instead of building: it contains the program with the recompiled code and all overlay libraries, but no game data, and sets the game up from the player's disc image on first start. `-DMHP3RD_RELEASE=ON` builds the executable for that: it reads nothing from the checkout, and on Linux it loads the libraries it ships from `lib/` next to itself and links the C++ runtime statically. [`docs/RELEASING.md`](../../docs/RELEASING.md) describes how a release is built and published; `scripts/release_linux.sh` builds the Linux artifacts, and `packaging/` holds their manifests, launcher and third-party notices.

## Code overlays

Beyond the main executable, the game loads 355 code overlays (`*.ovl`) from `USRDIR/DATA.BIN` into 12 fixed slots at run time: mode tasks (`game_task`, `lobby_task`, …), maps (`P_m*`, `P_v*`), monsters (`em*m0`–`m3`) and weapons (`we*player00`–`03`). The ELF section table lists them as zero-sized sections, so their code is not part of `EBOOT.ELF`.

Build them all once:

```bash
profiles/mhp3rd/scripts/build_overlays.sh [build_dir] [jobs]
```

It extracts every overlay from `DATA.BIN` into `analysis/overlays`, recompiles each one that has no library yet into `overlays/`, and then builds all their shared libraries into `bin/overlays` in a single `cmake --build` with `jobs` parallel jobs (default 2). Libraries that already exist are skipped and recompiled overlays are not recompiled again, so an interrupted run resumes. The game is playable before this finishes: an overlay with no library runs through the interpreter, which works but is roughly twenty times slower.

### How the host picks an overlay

Many overlays share one address, so each has its own corpus and the host must use the one matching what is loaded right now. An overlay image starts with `MWo3` and a 64-byte header: id, load address, code size, data size, bss size, two end-of-image addresses and a 32-byte name. A corpus is identified by its slot base plus an FNV-1a hash of that header and the code after it — the part the game never writes to, unlike the data section a loaded overlay keeps modifying.

On a dispatch miss inside a slot, the host hashes those bytes in guest memory, unregisters whatever was installed there and registers the matching corpus. Every frame it compares each installed slot's header against the one it installed, which is enough to notice a swap without rehashing the code. An overlay may be larger than the gap to the next slot's base; the slot list records where images load, not how much room they have.

Each library resolves its framework symbols from the executable that loads it, so it only matches a host built from the same sources; `cmake --build out/mhp3rd` rebuilds all of them. `MHP3RD_OVERLAY_DIR` points the host at another library directory.

To build a single overlay by hand — for example one dumped from memory with `MHP3RD_DUMP_OVERLAYS`:

```bash
profiles/mhp3rd/tools/add_overlay.py out/mhp3rd /path/to/overlay_0A05E600.bin 0x0A05E600
```

It builds with 2 parallel jobs; `-j N` changes that. `--no-build` stops after recompiling and prints the target name, for building many overlays in one run. No reconfigure is needed: CMake notices the new overlay directory by itself.

## Running

```bash
out/mhp3rd/bin/Yakumo [game_dir]       # see "Game data" for where it looks without game_dir
```

The window renders at twice the PSP resolution by default (960×544). Esc, or L3+R3 on a gamepad, opens the [in-game menu](#in-game-menu); quit from there, or close the window (Cmd+Q on macOS, Alt+F4 on most Linux desktops). When the game asks for a name, Yakumo's [on-screen keyboard](#on-screen-keyboard) opens; the menu can switch to giving a fixed name at once instead.

### Keyboard and mouse

The game can be played with a keyboard and a mouse alone. Every control can be rebound in the menu (Controls → *Keyboard and mouse*); these are the defaults:

| Key or button | PSP | In the game |
| --- | --- | --- |
| W / A / S / D | Analog stick | Move |
| Mouse | The HD release's second stick | Camera; see below |
| Left mouse button | △ | Draw, attack |
| Right mouse button, F | ○ (confirm) | Attack, talk, confirm |
| Space | ✕ (back) | Roll, back |
| E | □ | Use an item, gather, sheathe |
| Q / Left Shift | L / R | Camera behind the hunter / guard, run, aim |
| Enter, Tab | START | |
| Backspace | SELECT | |
| Arrow keys | D-pad | |
| I / J / K / L | The second stick, fully | Camera without the mouse |
| Esc | In-game menu | Frees the pointer |
| F3 | Performance overlay on or off | |

A bow aims with Left Shift held and shoots with the left button; a bowgun fires with the right one.

*Use the classic keyboard layout* in the same section brings back the keys of earlier versions, for play without a mouse: I / J / K / L move, X ○, Z ✕, A □, S △, Q / W L / R, Enter START, Right Shift or Backspace SELECT, and the arrow keys the D-pad. *Restore control defaults* returns to the table above.

To rebind, activate a control's row and press a key or a mouse button: it is added (a control takes two), or removed if the control has it already; Esc or a gamepad button cancels. A key taken from another control leaves that one. Keys are bound by their place on the keyboard, so W A S D stay under the same fingers on an AZERTY or a Dvorak layout; the menu shows their US names. They are kept in `settings.ini` as `input.bind.<control>` (for example `input.bind.circle=Mouse Right / F`; controls `stick_up`, `stick_left`, `stick_down`, `stick_right`, `triangle`, `circle`, `cross`, `square`, `l`, `r`, `start`, `select`, `dpad_up`, `dpad_left`, `dpad_down`, `dpad_right`, `camera_up`, `camera_left`, `camera_down`, `camera_right`; an empty value leaves a control unbound).

**The pointer.** While the game runs and the window has focus, Yakumo captures the mouse: the pointer is hidden and its motion and buttons go to the game. It is given back whenever Yakumo's menu, the on-screen keyboard or a setup screen is up, and when the window loses focus (switching to another window, Cmd+Tab or Alt+Tab, minimising). Buttons and keys still held when it is captured again reach the game only after they are released, and motion made while it was free is never replayed. *Mouse* in the menu (`input.mouse`, `MHP3RD_MOUSE=0`) turns all of this off, leaving the pointer alone.

**The mouse camera** feeds the same camera layer as the right stick, so it works where the [analog camera](#analog-camera) does: in a quest's ordinary camera, moving the mouse sideways turns the camera and moving it forward and back tilts it, by *Mouse sensitivity* degrees for each count of motion (0.10 by default), and it stops where the mouse stops. While a bow or a bowgun aims, and in a bowgun's scope, the mouse moves the aim the way the right stick does, slowed as *Aim speed* is to *Camera speed*; the game still decides when the aim may move. Where the port does not drive the camera (with *Analog camera* off, in the village or in a camera mode without a driver) the game's own camera turns at one fixed speed or not at all, so the mouse can only switch that turn on: moving it sideways turns the camera for as long as it moves, like holding the right stick, and vertical motion does nothing there (the second stick's up and down are the game's recentring commands). With *Right stick* set to D-pad or off, the mouse does not turn the game's own camera, since D-pad presses would also move cursors in the game's menus.

The keyboard and a gamepad can be used together or in turns: both are read every frame and add up, so nothing is left pressed by switching.

### Gamepad

Any controller SDL3 recognises works, and it can be connected before or after the game starts.

| Gamepad | PSP |
| --- | --- |
| South / East / West / North face buttons | ✕ / ○ / □ / △ |
| LB, RB | L, R |
| LT / RT (L2 / R2) past their threshold | L, R with the **Standard** trigger profile (the default); R, △ with **Bows**; R, ○ with **Bowguns** |
| Start / Back | START / SELECT |
| D-pad | D-pad |
| Left stick | Analog stick |
| Right stick | The HD release's second stick (camera) |
| L3 + R3 (both sticks pressed) | In-game menu |

The trigger profiles are for shooting: R, held to aim, moves onto L2, and the weapon's attack goes onto R2. They only add copies: RB, △ and ○ keep working, LB stays L, and the keyboard and Yakumo's own menu are unchanged. Choose one in the menu (*Trigger profile*) or with `MHP3RD_PAD_TRIGGERS`.

The face buttons are positional, so on a PlayStation pad circle is circle and confirms, exactly as the game's prompts say. The menu's *Confirm button* setting (or `MHP3RD_PAD_FACE=xbox`) moves confirm to the bottom button for pads labelled the other way round.

## On-screen keyboard

When the game asks for text (the hunter's name at character creation), Yakumo opens its own keyboard over the game. It works with a gamepad alone, and a physical keyboard types into it at the same time; the mouse can click its keys.

| Gamepad | Keyboard | Action |
| --- | --- | --- |
| D-pad, left stick | | Move over the keys |
| Confirm (○, or the bottom button with *Confirm button* set to it) | typing | Type the key |
| Back | Backspace | Delete the character before the cursor |
| □ (X) | | Shift: once, again for caps, again off |
| △ (Y) | Space | Space |
| Select / View | | Letters or symbols |
| L1 / R1 | ← / →, Home, End | Move the cursor |
| Start | Enter | OK |
| the *Cancel* key | Esc | Cancel |

The keys *Shift*, *#+=*, *Space*, *Delete*, *Cancel* and *OK* sit in the bottom row. A counter shows the length against the most the game takes (12 characters for a hunter name); it turns red when a key cannot be typed. Characters the game cannot take are dimmed. A hunter name may hold Latin letters, digits, space and `! # $ & ' ( ) + , - . / : ; = ? @ _ ~`. The game with the English patch draws every printable ASCII character in a name, but the asterisk comes out as a bullet, and the double quote, percent sign, asterisk, angle and square brackets, braces, backslash, caret, backquote and vertical bar are left out because text formatting may claim them.

The game keeps running behind the keyboard, as it does behind the PSP's: it keeps polling the keyboard and playing sound, and its animations go on. It reads a neutral pad until the keyboard closes, and buttons still held then reach it only after they are released. The game blanks its screen while the PSP's keyboard would cover it, so the window keeps the frame from just before the keyboard opened, dimmed. The in-game menu does not open over the keyboard.

Where SDL reports a system on-screen keyboard (such as Steam's in Big Picture or Game Mode), a *Steam* key appears in the bottom row and asks for it; what it types goes into the field like a physical keyboard. The built-in keyboard always works without it.

OK hands the text to the game as the PSP's keyboard would: UTF-16 in the field's output buffer, the field result *changed*, and the dialog status moving from visible to quit. Cancel leaves the buffer alone and reports *cancelled*; the game then keeps the name it had.

The menu's text fields (*Hunter name*, *Server*, *Nickname*) open the same keyboard when a gamepad activates them; with a keyboard or the mouse they are edited in place.

## In-game menu

Esc, or L3+R3 on a gamepad, opens Yakumo's menu over the game; the same again, back at its top level or Start closes it. Esc never quits the game: Steam's desktop controller layout on a Steam Deck sends Esc with the B button, so an Esc that arrives together with a gamepad button is ignored.

By default the game is paused while the menu is open: no guest code runs, emulated time stands still, the audio device stops, and the last frame stays behind the menu, dimmed. On resume the kernel's clock picks up from real time again, so the game neither races to make up the pause nor counts it in the `[perf]` statistics.

Two settings in the System section change that. With *Pause the game when the menu opens* off, the game keeps running behind the menu: it keeps drawing frames at its own pace, the sound keeps playing and its clock keeps running, and the menu is drawn over each frame. During ad hoc play (in a gathering hall, joining one, or hosting a session) the game keeps running behind the menu unless *Pause during multiplayer* is on, whatever the first setting says, because a paused game stops answering the other players and can drop a quest; this one is off by default. The menu's header says which applies: *Paused* or *Running*. Either way, input goes to the menu only, so moving through it never moves the hunter, and buttons still held when it closes reach the game only after they are released.

The menu follows the game's confirm convention: with the default layout the right face button (○ on a PlayStation pad, B on a Steam Deck) selects and the bottom one goes back, as in the game; with *Confirm button* set to the bottom button, both swap. The footer shows the buttons of the pad in use (PlayStation shapes or letters) or the keys, and a line explaining the focused setting. L1/R1 (LB/RB), or Q/W on the keyboard, switch between the sections. Left and right change a value; confirm steps it forward.

Every change applies at once and is saved to `settings.ini` in the per-user directory, next to the installer's `disc_image`. A setting whose environment variable is set is decided by that variable for the run: the menu shows it greyed with *Set by MHP3RD_…* and leaves the file's value alone. So the order is: environment variable, then `settings.ini`, then the default.

The Android app starts from other defaults where a phone differs, with the same keys and values: `video.aspect` is `fill` (a phone is wider than the PSP), `video.fullscreen` is on (there is no window) and `input.mouse` is off (a phone has no mouse to capture). Everything else starts as in the table. On Android, *Analog camera* alone decides whether a finger drag turns the camera: *Right stick* is about a physical stick there.

| Section | Setting | Key in `settings.ini` | Variable | Values |
| --- | --- | --- | --- | --- |
| Video | Resolution | `video.internal_scale` | `MHP3RD_INTERNAL_SCALE` | Auto (`auto` or `0`: the window's own size, followed as it changes, at most 1632 lines) or ×1–×6 of 480×272 (the variable allows up to ×8); default ×2. See [Picture shape and size](#picture-shape-and-size) |
| Video | Display | `video.fullscreen` | | Window or fullscreen |
| Video | Window size | `video.window_scale` | | ×1–×4 of 480×272; default ×2 |
| Video | Aspect ratio | `video.aspect` | | `original` (the PSP's shape, black bars; the default), `stretch` (stretched to the window) or `fill` (the game's view takes the window's shape). Older versions wrote `video.keep_aspect`, which is still read and written |
| Video | Scaling filter | `video.sharp_screen` | | Smooth or sharp scaling of the finished picture to the window |
| Video | Texture filter | `video.sharp_textures` | | Smooth (bilinear) or sharp (nearest) texture sampling |
| Video | Texture pack | `video.texture_pack` | `MHP3RD_TEXTURE_PACK` | On (default) or off: draw an installed [HD texture pack](#hd-texture-packs) instead of the game's textures. The footer shows how many textures the pack has and how many are on the GPU, or where the pack was looked for |
| Video | Import texture pack… | `video.texture_pack_folder` | `MHP3RD_TEXTURE_PACK` (a folder) | Empty (default): the pack in `textures/NPJB40001`. A folder: the pack [imported to be used where it is](#importing-a-texture-pack). *Stop using the pack folder* empties it |
| Video | Vsync | `video.present_mode` | | On (FIFO), or off through mailbox or immediate presentation where the driver offers them |
| Video | Frame rate | `video.frame_rate` | `MHP3RD_FRAME_RATE` | `30` (default: the game's own frames, as they are), `45`, `60`, `90`, `120` or `display` (the display's refresh rate): frames in between the game's, with blended movement. See [Frame rate](#frame-rate) |
| Video | Lower when behind | `video.frame_rate_auto` | `MHP3RD_FRAME_RATE_AUTO` | On (default): the frame rate steps down by itself rather than slow the game. Off: the chosen rate stays, and the game may run below full speed |
| Video | Game speed | `video.unthrottled` | `MHP3RD_UNTHROTTLED` | Normal (held to real time) or unlimited |
| Video | Fast loading | `video.fast_loading` | `MHP3RD_FAST_LOADING` | On (default) or off: while the game loads, and only then, it runs ahead of real time. See [Fast loading](#fast-loading) |
| Video | Performance | `video.performance` | `MHP3RD_PERF` | Off, overlay, overlay and log, log only |
| Video | Font | `text.font` | `MHP3RD_FONT` | Default (a Japanese system font), or an installed font; see [Game text](#game-text) |
| Video | Weight | `text.weight` | | Regular, bold (default) or heavy: thickens the game's text by 0–2 pixel columns |
| Audio | Volume | `audio.volume` | | 0–100% |
| Audio | Mute | `audio.mute` | | |
| Controls | Confirm button | `input.confirm` | `MHP3RD_PAD_FACE` | Right (○, Japanese) or bottom (Western) |
| Controls | Stick dead zone | `input.dead_zone` | `MHP3RD_PAD_DEADZONE` | 0–50% |
| Controls | Trigger point | `input.trigger` | `MHP3RD_PAD_TRIGGER` | 5–100% |
| Controls | Trigger profile | `input.trigger_profile` | `MHP3RD_PAD_TRIGGERS` | `standard` (L / R, the default), `bows` (R / △), `bowguns` (R / ○) |
| Controls | Right stick | `input.right_stick` | `MHP3RD_PAD_RSTICK_DPAD` | Camera, D-pad or off |
| Controls | Analog camera | `input.analog_camera` | `MHP3RD_ANALOG_CAMERA` | Proportional turn and continuous tilt in the ordinary quest camera, and proportional bow and bowgun aim; on by default |
| Controls | Aim speed | `input.aim_speed` | `MHP3RD_AIM_SPEED` | Degrees a second at full deflection while a bow or a bowgun aims, 10 to 360; default 90 |
| Controls | Camera speed | `input.camera_speed` | `MHP3RD_CAMERA_SPEED` | 20–720 degrees per second at full deflection; default 190 |
| Controls | Invert camera horizontally / vertically | `input.invert_camera_x`, `input.invert_camera_y` | | For the right-stick camera |
| Controls | Right stick D-pad point | `input.right_stick_zone` | `MHP3RD_PAD_RSTICK_ZONE` | 10–100%, for the D-pad mode |
| Controls | Mouse | `input.mouse` | `MHP3RD_MOUSE` | On (default): the window captures the pointer while the game runs, and the mouse turns the camera and presses its bound buttons; off: the pointer is left alone |
| Controls | Mouse sensitivity | `input.mouse_sensitivity` | `MHP3RD_MOUSE_SENSITIVITY` | Degrees of camera turn per count of mouse motion, 0.01 to 0.99; default 0.10 |
| Controls | Invert mouse horizontally / vertically | `input.invert_mouse_x`, `input.invert_mouse_y` | | For the mouse camera and aim |
| Controls | On-screen controls | `input.touch_controls` | | On (default): a touch screen shows the on-screen pad once it is touched |
| Controls | D-pad | `input.touch_dpad` | | On (default): the on-screen controls have a D-pad at the left edge, for the game's menus; off gives its place to the stick |
| Controls | Controls opacity | `input.touch_opacity` | | 10–100%; default 50% |
| Controls | Controls size | `input.touch_size` | | 60–160% of the default size; default 100% |
| Controls | Touch camera speed | `input.touch_camera_speed` | | Degrees the camera turns for a drag across the screen's height, 30 to 720; default 180 |
| Controls | A row per control (Move forward … Camera right) | `input.bind.<control>` | | Up to two keys or mouse buttons, see [Keyboard and mouse](#keyboard-and-mouse) |
| Controls | When the game asks for a name | `input.name_entry` | `MHP3RD_OSK_MODE` | `keyboard` (default): the on-screen keyboard; `fixed`: the name below at once |
| Controls | Hunter name | `input.name` | `MHP3RD_OSK_TEXT` | Default `Hunter`; up to 12 characters. Setting the variable also answers at once unless `MHP3RD_OSK_MODE` says otherwise |
| Mods | Use mods | `[general] enabled` in `mods.ini` | `MHP3RD_NO_MODS` | On (default) or off: every mod off, the game's own files only. See [Mods](#mods) |
| Mods | A row per mod: On, Priority | `[mod <folder>] enabled`, `rank` in `mods.ini` | | Off (default) or on; a higher rank wins where two mods replace the same file |
| System | Pause the game when the menu opens | `ui.menu_pause` | `MHP3RD_MENU_PAUSE` | On (default) or off: the game keeps running behind the menu |
| System | Pause during multiplayer | `ui.menu_pause_multiplayer` | `MHP3RD_MENU_PAUSE_MULTIPLAYER` | Off (default): during ad hoc play the game keeps running behind the menu; on: the setting above decides |
| System | Add a timestamp to the backup name | `saves.backup_timestamp` | | On (default): each backup from *Back up saves…* is a new folder named by its time; off: plain folder names, replaced after asking |
| Network | Ad hoc play | `network.adhoc` | `MHP3RD_ADHOC` | Off (default) or on; off, the game reports the wireless switch as off |
| Network | Server | `network.server` | `MHP3RD_ADHOC_SERVER` | Host name or address of a PSP ad hoc server, optionally `host:port`; empty by default |
| Network | Nickname | `network.nickname` | `MHP3RD_ADHOC_NICKNAME` | The name other players see; empty uses the hunter name |

Everything applies without a restart, apart from mods that change a file's size (see [Mods](#mods)); the name settings take effect the next time the game asks for a name. The Controls section also has *Use the classic keyboard layout*, and the System section has *Resume*, *Open the data folder*, *Set up game data again…* and *Quit game* (both of the last two ask first), the *Saves* rows described under [Saving and loading](#importing-a-save-from-a-psp), and the build version, the data and saves folders and the GPU. Each section but Mods has a button that restores its defaults.

The Network section also shows the connection and has the troubleshooting tools described under [Multiplayer](#multiplayer-ad-hoc). The file also keeps `network.mac`, the address other players know you by (made up the first time you go on line; `MHP3RD_ADHOC_MAC` overrides it), `ui.menu_hint_seen`, set once the menu has been opened (until then a hint at the bottom of the screen says how to open it during the first seconds of play), and `ui.last_folder`, where the setup's file browser opens.

The interface is drawn with [Dear ImGui](third_party/imgui/README.md). Its text uses a system font: San Francisco or Helvetica on macOS, Noto Sans, DejaVu Sans or Liberation Sans on Linux, Segoe UI on Windows, with a Japanese font merged in for file names; `MHP3RD_UI_FONT` names another `.ttf`. It scales with the window: about 27-pixel text on a Steam Deck's 1280×800 screen.

## Game text

The game draws its text with the PSP's system font, which lives in the console's flash and is not on the disc, so Yakumo draws those glyphs from a font on your computer. *Font* in the menu's Video page lists the installed fonts that have every Latin letter, digit and punctuation mark, marked *Japanese* when they also have the kana and kanji the game still shows. Characters a font lacks come from the default font: Hiragino Sans on macOS, Noto Sans CJK on Linux and the Steam Deck (the `fonts-noto-cjk` package or its equivalent; a release falls back to the copy it ships), MS Gothic or Meiryo on Windows. To use a font that is not installed, put its `.ttf`, `.otf`, `.ttc` or `.otc` file into the `fonts` folder of the per-user directory (*Open the fonts folder* in the same section); those are listed first. A preview line under the setting shows the choice the way the game draws it.

A change applies at once: Yakumo makes the game draw every character again the next time it shows it, so text already on screen changes within a frame or two.

How the text is laid out, as traced with `MHP3RD_TRACE_FONT=1`: the game sizes a glyph cell in a texture atlas from the font's maximum glyph size, renders each glyph into a 20×20 buffer and copies that whole buffer into the cell, and draws text as one sprite per cell, half a character wide for Latin letters and full width for Japanese ones. Yakumo reports a 20×20 maximum so cells and buffer match, and fits every glyph inside its cell with a pixel of margin, shifting it and, when it is too large, scaling it down, so no font can spill into a neighbour or lose its edges. The size of the text is therefore fixed by the game; *Weight* is the adjustment that fits within it.

## HD texture packs

Yakumo loads HD texture packs made for PPSSPP, in its `textures.ini` format, without conversion. None is included or downloaded: install one from the menu (below), or copy the pack's folder, the one that holds `textures.ini`, to `textures/NPJB40001` in the [per-user directory](#installer) yourself:

| System | Pack folder |
| --- | --- |
| macOS | `~/Library/Application Support/Yakumo/MHP3rd/textures/NPJB40001/` |
| Linux, Steam Deck | `~/.local/share/Yakumo/MHP3rd/textures/NPJB40001/` |
| Flatpak | `~/.var/app/io.github.teamgdb.Yakumo/data/Yakumo/MHP3rd/textures/NPJB40001/` (a copy, not a link to a folder outside the sandbox) |
| Windows | `%APPDATA%\Yakumo\MHP3rd\textures\NPJB40001\` |

*Open the textures folder* in the menu's Video section opens `textures`. A pack made for the PSP release (`ULJM05800`) works only if its `textures.ini` lists `NPJB40001` under `[games]`; the import installs such a pack as `NPJB40001` by itself. *Texture pack* in the Video section turns the pack on and off while the game runs; off draws exactly the game's own textures again. The footer under that setting shows how many textures the pack lists, *No pack in …* with the folder Yakumo looked in, or *Pack folder missing: …* when a pack used in place has moved.

### Importing a texture pack

The in-game menu does it (Esc, or L3+R3 on a gamepad; Video section, under *Texture pack*), with a gamepad alone or with the keyboard and mouse:

1. Unpack the pack if it came as a `.zip`: zipped packs are not read.
2. Choose *Import texture pack…*. The file browser lists folders and starts in Downloads. Opening a folder that holds `textures.ini` chooses it; *Import from this folder* chooses the folder shown. Dropping the folder on the window chooses it too. The pack is found in any of these layouts:
   - the pack folder itself, the one holding `textures.ini`;
   - a folder holding `textures/NPJB40001/` (a copy of another Yakumo data folder) or `NPJB40001/` (a `TEXTURES` folder);
   - PPSSPP's memory stick, or its `PSP` folder: `PSP/TEXTURES/NPJB40001/`;
   - a pack folder named for another release, such as `ULJM05800`, whose `textures.ini` lists `NPJB40001` under `[games]`, chosen itself or found in any of the places above. It is installed as `NPJB40001`.
3. The pack is checked, then shown beside the pack in use now. Nothing is copied until you choose. The review shows the pack's folder, its number of texture keys and hash, the number of image files and the size of the whole folder, and how many images `textures.ini` names that are not in the folder (those textures keep the game's own look). A pack is refused, with the reason, when its `textures.ini` cannot be read, names no hash or uses the old `quick` hash, when it is `textures.zip`, or when it is named for another release and does not list `NPJB40001`.
4. Choose how to install it:
   - **Copy into Yakumo's data folder** (*Copy and replace* when a pack is installed): copies the pack to `textures/NPJB40001`, leaving hidden files such as `.DS_Store` out. It needs the pack's size plus 64 MB free in the data folder and is refused otherwise. The copy runs in the background with a progress bar; the game keeps running (or stays paused) and the menu stays open until it ends. *Cancel* stops it at once.
   - **Use it where it is**: copies nothing and reads the pack from its folder from then on (`video.texture_pack_folder` in `settings.ini`), which saves the space of a large pack. The folder must stay where it is; if it moves, the Texture pack row says *Pack folder missing* and no pack is drawn. *Stop using the pack folder* goes back to the pack in the data folder.
5. The pack is turned on and applies at once: textures already on screen change within a frame or two, and the Texture pack row shows the new count.

**Nothing is deleted.** A copy goes to a hidden `textures/.incomplete-<date>_<time>` folder first; the pack in use stays in place until the copy is complete, so a cancelled or failed copy leaves it as it was (and removes only its own partial copy). Once the copy is complete, the pack it replaces is moved to `textures/.backup/<date>_<time>/NPJB40001/`, for example `.backup/2026-09-23_19-05-12/NPJB40001/`; import that folder to go back to it. A pack used in place is never moved or changed. `MHP3RD_TEXTURE_PACK` set to a folder still decides which pack is read for the run; the review says so.

How it works:

- **Keys.** Each texture is hashed once, when the renderer first uploads it, never per draw: `xxh64` or `xxh32` over its bytes in guest memory, with the dimensions and, for paletted textures, the palette's hash in the key, as the pack's `[options]` choose. `ignoreAddress`, `reduceHash`, `[hashranges]`, `[reducehashranges]`, `[filtering]`, `[games]`, the wildcard keys that leave the address, palette or data hash out, empty entries that keep a texture as it is, and images in the pack's top folder named by their key are all honoured. Packs that use the old `quick` hash, zipped packs (`textures.zip`), and DDS, KTX2 and ZIM images are not supported; PNG is.
- **Loading.** Images are read and decoded on background threads; the original texture is drawn until its replacement is on the GPU, usually a frame or two after it first appears. Each image is uploaded with a full set of mip levels, so a large image stays smooth at a distance. Its size does not matter: texture coordinates address the whole texture, so a 4× image covers exactly what the original covered, 2D screens included. A file name a pack spells in another case than the file on disk is still found, which matters on Linux.
- **Memory.** Images on the GPU are kept within a budget, 1 GiB by default (`MHP3RD_TEXTURE_PACK_MEMORY`); when a new scene needs more, the images drawn least recently are dropped and loaded again when they are next drawn. The village with a full pack needs about 250 MB.

`MHP3RD_TRACE_TEXTURE_PACK=1` logs each texture's key and what the pack does with it, each image decoded and uploaded, and once a second how many draws used a replacement. To make a pack, `MHP3RD_TEXTURE_DUMP=<folder>` writes every texture the game uploads, once, as a PNG named by its key; the folder gets a `textures.ini` that makes it a pack as it is, and edited images in it replace the game's.

## Mods

Yakumo reads mods in the format the game's community already uses with the mhp3reload mod loader and its mod manager, so a mod made for them works without changes. None is included, downloaded or linked here: install mods you downloaded yourself. Each mod is a folder in `mods` in the [per-user directory](#installer) (`~/Library/Application Support/Yakumo/MHP3rd/mods` on macOS, `~/.local/share/Yakumo/MHP3rd/mods` on Linux, `%APPDATA%\Yakumo\MHP3rd\mods` on Windows):

```
mods/
  my_armour/        a mod in the mod manager's format
    mod.ini
    preview.png     optional, shown in the menu
    armour.pac
  some_files/       files named by the file id they replace, as in mhp3reload's files folder
    0601
    0602P           a patch for file 0602
```

A `mod.ini` looks like this:

```ini
[MOD INFO]
Name="My armour"
Author="Me"
Type="File"
Version="HD"
Files="armour.pac;helm.pac"
Target="0601;05D2"
Description="First line.\Second line."
```

A **file id** is the index of a file in the game's `DATA.BIN`, in four hex digits ([DATA_BIN.md](../../docs/DATA_BIN.md#file-ids-and-mods)). What each `Type` does here:

| Type | Here |
| --- | --- |
| `File` | Replaces each `Target` file with the matching one of `Files`. A replacement may be larger than the original |
| `Patch` | Applies each of `Files` to its `Target` file as the game loads it; see below |
| `Pack` | Turns the mods named in `ModList` (their folder names) on and off together |
| `PseudoPack` | The `File` and `Patch` parts in its `SubModList` sections, as one mod |
| `Equip<type>`, `EquipSET`, `EquipCATSET` | A model you point at a piece of equipment: its screen in the menu asks for the file id each file replaces |
| `Code` | Listed, but cannot be turned on: code mods run their own PSP code inside the game, which recompiled code does not allow yet (#81) |

`Version` says which release a mod was made for: `HD`, `BOTH` (then `FilesHD` and `TargetHD` are used where given) or `NOHD`. A mod without `Version` counts as made for the PSP release, as the mod manager has it; its file ids are the PSP release's, so it cannot be turned on. `Depends` names mods that are turned on with it. `Priority`, `Animation`, `Audio` and `Script` are read but do nothing here; the mod's screen says so.

**Patches.** A patch file is a list of blocks, each a 32-bit address, a 32-bit length and that many bytes, ending with the address `FFFFFFFF` (the format's last 8 bytes, `FFFFFFFF00000000`, which the mod manager strips when it installs a patch; either form works). A block for an address inside a code overlay changes the overlay's bytes as the game loads it, which is the same as changing them in memory right after the load: the game copies an overlay to its load address unchanged. A block for other memory is written right after that overlay has loaded, at the instruction cache flush the game makes after copying code. A block whose address is smaller than the file is an offset into the file, for files that load at no fixed address. A block marked to run as code (the top bit of its length) is skipped, and a patch that changes an overlay's code makes that overlay run in the interpreter; both are reported.

**Several mods on one file.** Where two mods that are on replace the same file, the one higher in the list wins. Patches apply on top of the file the game gets, the winning replacement included, lowest priority first. The mod loader refuses a file that is both replaced and patched; Yakumo applies both, because patches are small edits at known places and a replacement usually keeps them where they were, and it lists the combination under *Conflicts* so it is easy to see which mod to turn off if it is not.

**The Mods page** of the menu lists the mods, highest priority first, with whether each is on. A mod's own screen shows its preview, author, type, the release it was made for, what it changes and its description, and has its switch, its place in the list (left and right move it) and, for equipment mods, the file id each file replaces. *Import mod…* copies a mod you downloaded and unpacked into the mods folder, turned off: choose its folder, the one with `mod.ini`, or a folder that holds several mods, in the file browser, or drop the folder on the window. A mod already installed under the same folder name moves to `mods/.backup` first; nothing is deleted. Unpack `.zip`, `.rar` and `.7z` archives before importing. *Open the mods folder*, *Read the folder again* (after adding or removing folders by hand) and *Use mods*, which turns every mod off for a clean comparison, are on the same page.

**When a change applies.** The game reads `DATA.BIN`'s directory once, at start. A change that keeps every file's size as the directory has it applies the next time the game loads each file: usually the next area, menu or piece of equipment shown; files the game loads once at start, such as the menus' textures, need a restart. A change that makes a file larger, or smaller where the directory has its exact size, applies at the next start; the page says so and offers *Restart now*.

The choices are kept in `mods.ini` next to `settings.ini`: `[general] enabled`, and per mod `enabled`, `rank` (higher wins) and the chosen targets of an equipment mod.

How it works: every file of the game comes from `DATA.BIN`, and the file I/O reads it for the game. The game reads its directory once, a table of where each file starts and a table of exact sizes, and then reads each file from its first block to exactly the size given, trusting the tables alone. With mods on, the file I/O serves an archive with the mods' files in it: a mod's file is encrypted for its place in the archive (the obfuscation is keyed by the block a file starts at), a file that no longer fits its blocks grows, every file after it moves up and is re-keyed on the way, and the directory says the same. With no mod on, not a byte changes. `host/mods/` holds the machinery, which knows no game (the mods folder, `mod.ini`, choices, conflicts, import, a file's bytes with mods), and the `mhp3rd_*` files this game's archive and format.

`MHP3RD_TRACE_MODS=1` logs what the mods change, each read they serve (`[mods] read 0FEE +0 131072 of 628736 bytes: replaced by …`) and each write made after an overlay loads; problems with a mod are logged whether it is set or not. `MHP3RD_TRACE_DATA_BIN=1` logs the id of every file the game reads while a mod is on: change the equipment on screen and the new ids are the files to target.

## Saving and loading

The game saves through the PSP's save-data utility, which the host implements. Saves live where a PSP keeps them, under the directory that backs `ms0:`: `ms0` in the [per-user data directory](#installer) for an installation, or `ms0` in the game directory when the game runs from one (`profiles/mhp3rd/game`, `MHP3RD_GAME_DIR` or the `game_dir` argument):

```text
game/ms0/PSP/SAVEDATA/ULJM05800/
    PARAM.SFO      titles, the file list and the hashes that protect the save
    MHP3RD.BIN     the game data, encrypted
    ICON0.PNG      icon shown in the PSP's save list
    PIC1.PNG       background shown in the PSP's save list
```

`MHP3RD.BIN` is encrypted and `PARAM.SFO` hashed exactly as the PSP's save-data utility does it, with the key the game supplies, so a folder can move between this port and a PSP's memory stick unchanged. The one field that cannot be reproduced is a hash made with a key unique to each PSP; the port writes a placeholder there. Copying a save made here onto a real PSP has not been tried yet.

Nothing is drawn for the save-data or message dialogs yet ([#33](https://github.com/TeamGDB/Yakumo/issues/33)): the game's own screens ask where to save and show the result, and the system dialogs answer as if the player confirmed them. The log shows each request and each message, for example `[savedata] AUTOSAVE (1) game="ULJM05800" …` followed by `[savedata] saved 1183744 bytes to …`.

### Importing a save from a PSP

The in-game menu does it (Esc, or L3+R3 on a gamepad; System section, *Saves*):

1. Copy the save to this machine, or connect the memory stick: on a PSP's memory stick the folder of *Monster Hunter Portable 3rd* is `PSP/SAVEDATA/ULJM05800`, with downloaded quests in `ULJM05800QST`. PPSSPP keeps the same folders in its `memstick/PSP/SAVEDATA`.
2. Choose *Import save…*. The file browser lists folders. Opening a save folder chooses it; *Import from this folder* chooses the folder shown, and every save in it, in its `SAVEDATA` or in its `PSP/SAVEDATA` is found, so a memory stick's root or a `SAVEDATA` folder holding several games works too.
3. Each save found is checked, then shown with its date and size beside the save it would replace. Nothing is copied until you choose *Import* (or *Replace and import*).
4. The game reads its saves at the title screen. *Restart now* closes the game and starts it again there; progress since your last save is lost. If you keep playing instead, do not save before restarting, or the game writes its own data over the imported save.

**Checks.** A folder is imported only when its `PARAM.SFO` names one of this game's folders (`ULJM05800`, `ULJM05800QST`, `ULJM05800DAT`), its own hashes match, and every data file it lists is present, matches its hash and decrypts with the key the game passes to the save-data utility. Other games' saves are left out (and counted), and a damaged one is refused with the reason. The key is taken from the game's own first save-data request, at boot; until then, saves cannot be checked and the menu says to try again at the title screen. A save stored without encryption, as early PPSSPP versions wrote them, has nothing to check and is imported as it is.

**Nothing is deleted.** A save that an import replaces is moved to `ms0/PSP/SAVEDATA/.backup/<date>_<time>/<folder>/`, for example `.backup/2026-09-19_19-05-12/ULJM05800/`; import that folder to go back to it. `.zip` files are not read: unpack them first.

By hand, the same works with the game closed: copy the folder into `ms0/PSP/SAVEDATA/` in the directory above (for example `~/.local/share/Yakumo/MHP3rd/ms0/PSP/SAVEDATA/`), after keeping a copy of any folder of the same name, which holds all three character slots. *Open the saves folder* in the menu shows that folder.

### Exporting and backing up

- **Export save…** copies the game data and the downloaded quests to a folder you choose, as `MHP3rd saves <date>_<time>/PSP/SAVEDATA/ULJM05800…`, the layout of a memory stick: copy its `PSP` folder to the root of a PSP's memory stick, or import it on another machine. It always makes a new folder.
- **Back up saves…** copies every save folder of the game, the install data included, to the backups folder, `save-backups` in the [per-user data directory](#installer), or to a folder you choose. With *Add a timestamp to the backup name* (on by default; `saves.backup_timestamp` in `settings.ini`) each backup is a new folder named by its time, holding the save folders: `save-backups/2026-09-19_19-05-12/ULJM05800/`. With it off, the save folders go straight into the chosen folder (`save-backups/ULJM05800/`), and an earlier backup there is replaced only after you confirm. *Open the backups folder* shows it. To restore a backup, import it.

### Downloadable content

The game keeps downloaded quests and equipment in the `ULJM05800QST` save folder and reads it through the save-data utility, like an ordinary save (AUTOLOAD of `ULJM05800QST` / `MHP3RD.BIN`). The download servers are long gone, so the in-game download mode's network side stays unimplemented. To use DLC you already have, import your `ULJM05800QST` folder from the menu (or copy it into `ms0/PSP/SAVEDATA/`), then open the game's download menu to install the quests. The project does not host, bundle or link to DLC files.

This release (`NPJB-40001`) asks for the original PSP release's folder names (`ULJM05800`), and the key it passes is the PSP release's: a downloaded-quest folder written by a PSP running `ULJM-05800` passes every check with it and decrypts. The two releases therefore share one save format, and saves should move between them in both directions; a save from this release has not yet been loaded on a PSP. When there is no save of its own, the game also looks for saves of *Monster Hunter Portable 2nd G* (`ULJM05500`) and *Monster Hunter Diary: Poka Poka Airu Village* (`ULJM05710`); those would be read from `ms0` the same way, which has not been tried.

To check a save folder without starting the game — for example one that the game reports as corrupted — run the save-data test program on it:

```bash
out/mhp3rd/bin/mhp3rd_savedata_tests --check profiles/mhp3rd/game/ms0/PSP/SAVEDATA/ULJM05800 MHP3RD.BIN <key>
```

`<key>` is the 32-digit key the game passes to the save-data utility; a run with `MHP3RD_TRACE_SAVEDATA=1` prints it as `key=` on every request for the game's own save. The program reports whether the hashes in `PARAM.SFO` and the data file's hash match and whether the file decrypts. Without arguments it runs the self-tests, which need no game data.

## Multiplayer (ad hoc)

The PSP game plays together through ad hoc wireless: up to four consoles in the same room. Yakumo carries that over a network through a **PSP ad hoc server**. One player can host a session from the game itself, with the server built in (see [Play together locally](#play-together-locally)), or everyone can use a public server that PSP and PPSSPP players use, so you can hunt with other Yakumo players and, as the protocol is the same, with players on PPSSPP (not tested yet). Nothing has to be forwarded on your router for a public server: all game traffic goes through the server.

### Play together locally

On one home network, or over a VPN such as Tailscale or ZeroTier, one player hosts and the others join; nothing else needs to be installed and nothing needs to be forwarded.

1. **Host.** Open the menu (Esc, or L3+R3), go to **Network** and choose **Host a session**. The server starts inside the game, and the screen lists the addresses the others can use, one per network (for example `192.168.1.20  Local network (en0)` and `100.101.102.103  Tailscale`). Selecting one copies it. Below them are the players connected to your session and the hall each is in; **Stop hosting** ends the session for everyone.
2. **Join.** The others open **Network** too. Under **Join a session** they see the sessions hosted on their network, with the host's name and player count, and choose **Join**. On a VPN that carries no broadcast, such as Tailscale or most WireGuard setups, the host's session is not listed by itself: type the host's VPN address into **Address** and press Enter. Joined addresses are remembered under the same heading.
3. **Play.** Everyone closes the menu, goes up the stairs in the village to the gathering hall entrance, chooses **Online Guild Hall** and picks the same hall, as with any server.

Hosting and joining turn **Ad hoc play** on and fill in **Server** (the host's own game uses its server on `127.0.0.1`). Joining another session, or hosting, while you are in a hall takes you out of it, as a dropped connection would. While you play together, opening the menu does not pause the game unless *Pause during multiplayer* is on (System section), so the others keep hearing from you.

**Ports.** The built-in server listens on every network interface on TCP **27312** (matchmaking) and **27313** (relay); a host answers address checks on UDP 27312 and announces itself to UDP **27314** on the local network, by broadcast and on the multicast group 239.255.27.14. On a LAN or a VPN nothing needs to be forwarded, though a firewall on the host may ask to let Yakumo accept connections: allow it (on Windows, for private networks). To host over the plain internet, forward TCP 27312 and 27313 on the host's router to the host, and give the others your public address; a VPN is usually simpler. `network.host_port` in `settings.ini` (or `MHP3RD_ADHOC_HOST_PORT`) moves the server to another port pair, for example when another server already uses 27312; others then join `address:port`.

**PPSSPP players** should be able to join a session hosted in Yakumo, since the server speaks the same protocols (not tested yet): in PPSSPP, set the ad hoc server to the host's address and use the relay (*AemuPostoffice*) data mode.

**A server without the game.** `Yakumo --adhoc-server [port]` runs the same server alone in a terminal, announced on the local network, and prints the addresses to join; Ctrl+C stops it. It needs neither game data nor a window.

A server has two parts, both over TCP: the matchmaking service on port **27312**, which knows who is in which gathering hall, and a relay on port **27313**, which carries the game's own traffic between the players. Yakumo needs both, so pick a server that runs the relay (servers list it as *AemuPostoffice* data mode).

### Setting it up

1. Open the menu (Esc, or L3+R3) and go to **Network**.
2. Turn **Ad hoc play** on.
3. Enter the **Server**: a host name or IP address, `host:port` if its matchmaking port is not 27312. There is no default; see [Choosing a server](#choosing-a-server).
4. Optionally set a **Nickname**; otherwise the other players see your hunter name.
5. Close the menu. In the village, go up the stairs to the gathering hall entrance and choose **Online Guild Hall** (✕), then a hall. Everyone who picks the same hall number on the same server meets there.

Server and nickname changes apply the next time the game goes on line: leave the hall and enter it again. Turning ad hoc play off while in a hall takes you out of it, as if the connection dropped. The address other players know you by (`network.mac` in `settings.ini`) is made up once and kept.

Everyone in a hall must play the same game: this release and the PSP's *Monster Hunter Portable 3rd* (`ULJM-05800`) are the same game on the server, so players of the PSP version on PPSSPP can join.

### Choosing a server

PPSSPP's list of public ad hoc servers is in its [`assets/adhoc-servers.json`](https://github.com/hrydgard/ppsspp/blob/master/assets/adhoc-servers.json); its entries say which games each server's community plays and which data mode it runs. Choose one that runs the relay and whose players play Monster Hunter, near you if you can, and agree on it with the people you want to play with. Each server has a status page (usually on port 8888) that shows who is on line in which game.

Public servers are run by volunteers. Yakumo keeps one connection to the matchmaking service and one to the relay per game socket, and pings the matchmaking service every two seconds, like the other clients.

### Running your own server

The simplest is **Host a session** in the game, or `Yakumo --adhoc-server` (see [Play together locally](#play-together-locally)). You can also run [aemu_postoffice](https://github.com/Kethen/aemu_postoffice), the server most public servers use. It is a separate program under its own licence; nothing of it is part of Yakumo.

Natively, on macOS or Linux (a C++20 compiler is all it needs):

```bash
git clone --recursive https://github.com/Kethen/aemu_postoffice
cd aemu_postoffice/server_cpp
bash build_linux.sh
./aemu_postoffice          # config.json and game_db.json must be next to it
```

In a container (Docker or Podman), from the same `aemu_postoffice` checkout:

```bash
docker run --rm -it -p 27312:27312 -p 27313:27313 -p 8888:8888 \
  -v "$PWD":/src -w /src/server_cpp debian:stable \
  bash -c 'apt-get update && apt-get install -y g++ && bash build_linux.sh && ./aemu_postoffice'
```

Then use `127.0.0.1` as the server on the same machine, or the machine's LAN address on the others. Open TCP 27312 and 27313 in its firewall for other machines. The server log shows every login, group join and relay session, and `http://<server>:8888/` lists who is on line.

### Two instances on one machine

Each instance needs its own settings (for its own address and nickname) and its own copy of the save:

```bash
# once: a game directory per instance with its own save
mkdir -p ~/yakumo-b/ms0/PSP/SAVEDATA
ln -s /path/to/disc.iso ~/yakumo-b/disc.iso
ln -s /path/to/EBOOT.ELF ~/yakumo-b/EBOOT.ELF
cp -R profiles/mhp3rd/game/ms0/PSP/SAVEDATA/ULJM05800 ~/yakumo-b/ms0/PSP/SAVEDATA/

# each instance: its own data directory, window title, server and nickname
MHP3RD_DATA_DIR=~/yakumo-a-data MHP3RD_WINDOW_TITLE="Yakumo A" MHP3RD_ADHOC=1 \
  MHP3RD_ADHOC_SERVER=127.0.0.1 MHP3RD_ADHOC_NICKNAME=HunterA out/mhp3rd/bin/Yakumo profiles/mhp3rd/game
MHP3RD_DATA_DIR=~/yakumo-b-data MHP3RD_WINDOW_TITLE="Yakumo B" MHP3RD_ADHOC=1 \
  MHP3RD_ADHOC_SERVER=127.0.0.1 MHP3RD_ADHOC_NICKNAME=HunterB out/mhp3rd/bin/Yakumo ~/yakumo-b
```

Two characters from one save are fine in one hall, since the game tells players apart by their address, and each instance makes up its own.

### When something goes wrong

The menu's **Network** section shows what the connection is doing, updated live:

| Row | Shows |
| --- | --- |
| Connection | Off line, connecting, reconnecting (with the attempt and the last error), or on line with how long and the server connection's round trip |
| Server | While you host: its ports, uptime, players, halls, relay connections and streams, and what it has relayed and dropped |
| Discovery | Whether this instance listens for hosted sessions on UDP 27314 and how many it hears, and while you host, on how many interfaces it announces yours and how many address checks it answered |
| One row per host | Each session heard on the network: its address, players, game and when it was last heard |
| You | Your address and nickname |
| Group | The hall's group (`MHP3Q000` is Hall 01) and how many players are in it, or that it is being rejoined after a dropped connection |
| One row per player | Their address and how long ago their last packet arrived |
| One row per socket | Each ad hoc socket the game has open: its kind (PDP datagrams, PTP streams), port, state and peer |
| Relay links | How many of the sockets' relay connections are up |
| Per second, Since start | Packets and bytes in and out |
| Problems | Datagrams dropped, calls that timed out, reconnections |

Below it:

- **Network overlay** shows the connection, the group, its players and the traffic in the top-right corner while you play. `MHP3RD_ADHOC_OVERLAY=1` turns it on at start.
- **Log every call and packet** is the same as `MHP3RD_TRACE_ADHOC=1`: every ad hoc call the game makes, with its arguments and result, and every packet header goes to the console and to the network log.
- **Save network log** writes the recent network log and the section's state to `logs/adhoc-<date>-<time>.log` in the data folder (menu: System, *Open the data folder*). Attach it to a problem report, ideally with the log turned on before the problem happens.
- **Reconnect now** drops the server connection and connects again; the hall is joined again. **Disconnect** leaves the hall as if the other players were lost. The game reacts to both as to a real dropped connection.

Common problems:

- *The game says the wireless switch is off*: ad hoc play is off in the menu.
- *Join a session lists nothing*: the host is on a VPN without broadcast (type its address), on another network, or a firewall blocks UDP 27314 on your side. **Discovery** says whether this instance listens.
- *Host a session says the port is in use*: another server runs on this machine; stop it, or set `network.host_port`.
- *Connecting never finishes*: the server name is wrong, the server is down, or a firewall blocks TCP 27312. The console says `cannot reach the ad hoc server` or `cannot resolve`.
- *You are in a hall but see nobody*: the other player is on another server, in another hall, or playing a game the server does not group with this one. The server's status page shows where everyone is.
- *Players see each other but a quest cannot be joined or the hall drops*: the server has no relay (TCP 27313), or it is blocked. **Relay links** stays below its total.
- *The connection drops in a quest*: when the server connection comes back within ten seconds, the hall is rejoined; the quest itself usually ends, as it would on a PSP. After ten seconds the game is told the connection is lost.

### How it works

The game uses the PSP's ad hoc libraries (`sceNetAdhocctl`, `sceNetAdhoc`, and the network configuration dialog `sceUtilityNetconf`). Entering the Online Guild Hall, it scans for halls, then asks the network dialog to join the hall's group (`MHP3Q000` for Hall 01); Yakumo joins it on the server and the dialog finishes when the server confirms. In the hall every console broadcasts its state over PDP (datagrams on port 10000), which Yakumo sends through the relay to each player in the group. A quest is a PTP stream: the host listens on port 20001 and each joining player connects to it, also through the relay.

One network thread owns every connection to the server, so the game never waits for the network except where a PSP call itself blocks, and then no longer than the call's own timeout. During ad hoc play the game keeps running behind the menu by default (see [In-game menu](#in-game-menu)); if it is paused, the connection stays up, but the other players stop hearing from the game until the menu closes.

## Configuration

The settings a player needs are in the [in-game menu](#in-game-menu). Environment variables remain for everything else, and override the menu's settings for the run where they overlap.

### Game and paths

| Variable | Default | Effect |
| --- | --- | --- |
| `MHP3RD_GAME_DIR` | unset | Directory holding `EBOOT.ELF`, `disc.iso` and `ms0/` (the saves); skips the per-user directory |
| `MHP3RD_DATA_DIR` | SDL's preference path | Per-user data directory the installer fills, with the saves in its `ms0/` |
| `MHP3RD_OVERLAY_DIR` | `overlays/` next to the executable (`Contents/Frameworks/overlays` in the macOS app) | Directory of overlay libraries |
| `MHP3RD_MODS_DIR` | `mods/` in the data directory | The [mods](#mods) folder |
| `MHP3RD_NO_MODS` | off | `1`: no mod applies this run, whatever `mods.ini` says; the menu still lists them |
| `MHP3RD_FONT` | a system CJK font | Font to draw the game's text with: a `.ttf`, `.otf`, `.ttc` or `.otc` file, with `#N` after the path for the Nth face of a collection. Glyphs it lacks come from the default, a Japanese system font (Hiragino on macOS, Noto Sans CJK on Linux, MS Gothic or Meiryo on Windows; inside a Flatpak, the host's Noto Sans CJK under `/run/host/fonts`), and last the font a release ships in `fonts/` next to the executable |
| `MHP3RD_UI_FONT` | a system font | TrueType font for Yakumo's menu and setup screens |

### Video

| Variable | Default | Effect |
| --- | --- | --- |
| `MHP3RD_INTERNAL_SCALE` | `2` | Render resolution as a multiple of 480×272, or `auto` for the window's size (menu: Resolution) |
| `MHP3RD_NO_RENDER` | off | Run without a window; the installer shows no dialogs either. Emulated time is not held to real time |
| `MHP3RD_WINDOW_TITLE` | `Yakumo` | Title of the game window, to tell instances apart |
| `MHP3RD_UNTHROTTLED` | off | Let emulated time run ahead of real time, so the game runs as fast as it can be drawn (menu: Game speed) |
| `MHP3RD_FAST_LOADING` | on | `0` keeps loads at the PSP's pace (menu: Fast loading). See [Fast loading](#fast-loading) |
| `MHP3RD_FRAME_RATE` | `30` | `45`, `60`, `90`, `120` or `display`: present frames in between the game's 30 (menu: Frame rate). See [Frame rate](#frame-rate) |
| `MHP3RD_FRAME_RATE_AUTO` | on | `0` keeps the chosen frame rate even when the game falls behind (menu: Lower when behind) |
| `MHP3RD_NO_MATERIAL_COLOR` | off | Leave unlit geometry without vertex colours white instead of taking the material colour |
| `MHP3RD_NO_LIGHTING` | off | Draw lit geometry with the flat white stand-in used before lighting existed, and without fog, to compare a scene with and without them |
| `MHP3RD_NO_FOG` | off | Turn fog off and keep lighting |
| `MHP3RD_NO_FB_TEXTURES` | off | Decode every texture from guest memory, as before, instead of sampling the render target when the game textures from a framebuffer it drew, and stop writing framebuffers back to guest memory for the shown frame and for GE block transfers |
| `MHP3RD_TEXTURE_PACK` | unset | `0` turns the [HD texture pack](#hd-texture-packs) off, `1` on; a folder path loads the pack from that folder instead, ahead of an imported one (menu: Texture pack) |
| `MHP3RD_TEXTURE_PACK_MEMORY` | `1024` | Megabytes of GPU memory for texture pack images; the least recently drawn are dropped above it |
| `MHP3RD_TEXTURE_DUMP` | unset | Write every texture the game uploads, once, as a PNG named by its texture pack key into this folder, to start a pack from |
| `MHP3RD_NO_SPRITE_CLAMP` | off | Let 2D tiles sample outside their own texels, as before; above ×1 this shows faint lines along the tile edges of 2D screens |
| `MHP3RD_SCREENSHOT_DIR` | unset | Write BMP frames into this directory |
| `MHP3RD_SCREENSHOT_EVERY` | `60` | Frames between screenshots |
| `MHP3RD_PERF` | off | `1` shows the performance overlay and logs frame statistics once per second; `log` only logs them (menu: Performance). See [Performance statistics](#performance-statistics) |
| `MHP3RD_NO_CACHED_READBACK` | off | Keep the frame written back to guest memory in the first host-visible memory type, as before, instead of a host-cached one; on a Steam Deck the CPU copy out of it then takes ~3 ms a frame instead of a fraction of one |
| `MHP3RD_NO_DIRECT_VERTICES` | off | Expand every draw into a plain triangle list on the CPU, as before, instead of writing a transformed draw's decoded vertices once with an index list |
| `MHP3RD_NO_LOOKUP_CACHE` | off | Look every draw's pipeline and texture up in the renderer's caches, as before, instead of reusing the previous draw's and what the display list already resolved |
| `MHP3RD_NO_BUFFER_REUSE` | off | Allocate the texture decoder's working buffers, the staging buffer and command buffer of each texture upload, and the pixels of a framebuffer read back for a block transfer every time, as before, instead of keeping them for the next use |
| `MHP3RD_NO_DRAW_MERGE` | off | Record every draw with all of its state, as before, instead of setting only the state that changed and merging consecutive transformed draws with identical state into one draw call |
| `MHP3RD_NO_GPU_TIMESTAMPS` | off | Do not time the GPU with timestamp queries; the perf line reads `gpu n/a` |
| `MHP3RD_FRAMES_IN_FLIGHT` | `2` | `1` waits for the GPU to finish each frame before the next is recorded, as before, instead of recording a frame while the GPU draws the one before. The frame written back to guest memory reaches it at the same flip either way |
| `MHP3RD_SYNC_UPLOADS` | off | Copy each new texture with a command buffer of its own and wait for the queue to go idle, and wait again before destroying an evicted one, as before, instead of copying it ahead of the frame's commands in the same submission and destroying evicted ones once their frames have finished |
| `MHP3RD_NO_PIPELINE_CACHE` | off | Create every pipeline without the cache kept in `pipeline_cache.bin` |
| `MHP3RD_NO_ALPHA_VARIANTS` | off | Use the one fragment shader that tests alpha for every draw, as before, instead of pipelines without the test (and so without `discard`) for draws that have none |
| `MHP3RD_NO_CLEAR_LOAD` | off | Load the target's colour and depth at the start of every render pass, as before, even when the pass begins with a clear that writes all of them |
| `MHP3RD_NO_FAST_DECODE` | off | Decode every vertex with the general loop, as before, instead of a loop specialised for the run's formats. `MHP3RD_CHECK_DECODE` runs both and reports runs whose vertices differ |
| `MHP3RD_GPU_DECODE` | off | `1` hands the guest's own vertex bytes of every transformed draw to the vertex shader, which decodes and skins them, instead of decoding and skinning them on the CPU. Off by default: on the Steam Deck (radv) it hung the GPU (`ring gfx timeout`) at the character select screen. Through-mode draws, sprites and morphing vertices are decoded on the CPU either way |
| `MHP3RD_CHECK_GPU_DECODE` | off | Turns GPU vertex decode on and has the vertex shader write what it decoded to a buffer, decodes every checked draw on the CPU too, and compares them after the frame; a `[gpu-decode-check]` line every 300 frames counts vertices equal bit for bit, within rounding (skinned positions, whose bone terms the GPU sums with its own rounding) and different. Needs `vertexPipelineStoresAndAtomics` |
| `MHP3RD_NO_ROBUST_BUFFERS` | off | Turns off `robustBufferAccess`, which the renderer enables when the device offers it, so that a shader reading past a buffer gets zeros instead of whatever memory lies there |
| `MHP3RD_GPU_BREADCRUMBS` | off | For a GPU hang (`VK_ERROR_DEVICE_LOST`, or `amdgpu ... ring gfx timeout` in the kernel log): writes numbered markers into the command stream with `VK_AMD_buffer_marker`, and when the device is lost prints the last marker the GPU finished, the last it reached and the last queued, with what each one was (frame begin, render pass, uploads, replay, copy to the window). Needs a device with that extension (radv, AMD on Windows) |
| `MHP3RD_NO_TIGHT_MERGE` | off | Starts every draw's vertices on a 16-byte boundary, as before, which keeps about half the draws that could merge from merging |
| `MHP3RD_SYNC_TEXTURE_DECODE` | off | Decodes each new texture on the spot, as before, instead of copying its bytes and palette when it is drawn and decoding it on background threads until the frame is submitted |
| `MHP3RD_NO_FAST_TEXTURE_DECODE` | off | Decodes textures texel by texel with every palette entry read from guest memory again, as before. `MHP3RD_CHECK_TEXTURE_DECODE` decodes every texture both ways and compares them |
| `MHP3RD_NO_PIPELINE_PREWARM` | off | Makes each pipeline when a draw first needs it, instead of making the last run's pipelines (`pipeline_keys.bin`) on a background thread from the start |
| `MHP3RD_TRACE_RENDER` | off | A `[render-split]` line each second: the render thread's milliseconds per game frame running display lists (parsing, vertex decode, the renderer's handling of each draw with its texture and command recording) and on interpolation, replays, presents and the write-back. Timed with the CPU's own counter, so the frame barely changes |
| `MHP3RD_NO_FAST_STORE` | off | Convert the frame written back to guest memory pixel by pixel, as before, instead of a row at a time |
| `MHP3RD_TEXTURE_CACHE_LIMIT` | `1024` | Keep at most this many decoded textures on the GPU; a small number tests eviction |
| `MHP3RD_MOLTENVK_ASYNC_SUBMITS` | off | macOS: `1` lets MoltenVK turn each submitted frame into Metal commands on a thread of its own instead of the game's, saving 1–2 ms of the game's thread a frame. Off by default: it crashed after minutes of play in v0.6.0-alpha.4. MoltenVK's own `MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS=0` does the same |

### Picture shape and size

**Aspect ratio** (Video) decides how the game's picture meets the window:

- **Original** keeps the PSP's 480×272 shape and adds black bars. This is the picture as it always was.
- **Stretch** stretches that picture over the whole window.
- **Fill** gives the game's 3D view the window's shape, with no bars and no stretching: the vertical field of view stays the game's and the horizontal one widens (16:9, 21:9, 32:9) or, in a narrower window such as the Steam Deck's 16:10, narrows a little. The 2D interface keeps the PSP's proportions, centred: at 21:9 it sits in the middle of the screen with the 3D view on both sides. Draws that cover the screen's width (fades, backdrops) and draws that sample the rendered picture (blur, the quest-reward background) spread with the 3D view. Movies keep the PSP's shape with black beside them.

**Resolution** Auto draws the game at the window's size in pixels (HiDPI included, at most 1632 lines) and follows the window: resizing it, fullscreen on or off, a move to another display. A new size is taken once the window has held it for a moment, so dragging an edge does not rebuild the picture on every frame. Under Original and Stretch, Auto picks the smallest multiple of 480×272 that covers the picture. ×1–×6 draw 272 lines per step; under Fill the width follows the window's shape.

Every change applies at the next frame; Original with a fixed resolution is exactly the picture of earlier versions.

How Fill works: the game keeps the projection's parameters in its camera object (the pointer at `0x08A2F958`): near and far plane, aspect ratio and vertical field of view at `+0x0` to `+0xC`. The camera's set-up (`0x0882D5A4`) copies the aspect ratio from a constant, 480/272 at `0x08969F74`; the projection (`0x0882CF0C`, which calls the perspective builder `0x0882CDCC`) and the culling planes (`0x0882BF1C`) are built from those fields, and the camera's update builds them again only when the field of view differs from the one kept at `+0x10`. At each flip `host/camera/game_aspect.cpp` writes the target's shape into the constant (for cameras set up later) and into the live camera, and makes the next update rebuild by changing `+0x10`. The culling planes cover the view up to an aspect ratio of about 3; beyond that the port also shrinks their depth factor, the constant `-1.5` at `0x08969ED4`, which only `0x0882BF74` reads. Back at Original or Stretch the port writes the game's own values back, bit for bit, and then nothing more. Eleven instructions and the two constants are checked at start-up (listed in `game_aspect.cpp`); if any differs, the view keeps the PSP's shape. The renderer keeps the game's 480×272 coordinates everywhere (viewport, scissor, framebuffer textures, the frame written back to guest memory) and only spreads them over a target of the window's shape; the interface's through-mode draws into the shown framebuffer are pulled in about the centre, their scissor with them.

The community's widescreen cheat for this release (NPJB-40001) patches the same constant; the port does it live, for any shape, and keeps the interface undistorted.

### Frame rate

The game makes 30 frames a second, and its logic is tied to that rate: animation, physics, attack windows, timers and the monsters' behaviour all advance by a fixed step per frame. **Frame rate** (Video) presents more frames than that without touching the game: between two of its frames the renderer draws the older one again with every moving object's transforms blended towards the newer one (frame interpolation). The game keeps its speed and its timing exactly; only the picture is smoother.

- **30** shows the game's frames as they are, when the game flips them. This is the picture of earlier versions.
- **45**, **60**, **90** and **120** present that many frames a second: 1.5, 2, 3 and 4 per game frame. 45 is half of a 90 Hz screen (the Steam Deck's) and 90 all of it; 120 is for desktop monitors of 120 Hz or more.
- **Match display** presents at the display's refresh rate.
- With **Vsync** on, the rate is never above the display's refresh: on a 60 Hz screen 90 and 120 run at 60. The row says so (*now 60*).
- If presenting that often would slow the game down, it steps down by itself to the fastest rate that fits (120, 90, 60, 45, 30), and tries a faster one again later once there is time to spare. It also steps down when the display has no image free for the presents (a screen that refreshes slower than the rate, such as a Steam Deck that gamescope switched to 60 Hz): a present waits at most 3 ms for one and is dropped otherwise. The row then shows *90 (running at 60)*. Loading screens and stalls while the game still has idle time do not count against it. **Lower when behind** (Video, on by default) turns this off: the chosen rate stays, capped only by Vsync, and the game may then run below full speed.
- The picture comes later than at 30: a frame is shown as it is one game frame after its own moment less the first in-between step, 16.7 ms later than at 30 at 60, 22.2 ms at 45 and 90, 25 ms at 120. The camera is still updated once per game frame. The pad is read when the game asks for it, at every Frame rate: the keyboard, mouse buttons and gamepad as they are then, not as they were at the last flip (issue #8), which takes up to a game frame off the input's own delay. `MHP3RD_PAD_AT_FLIP=1` reads it at the flip, as before.
- The presents keep the game's own clock: 29.97 frames a second, from the PSP's 59.94 Hz. A display refreshes at exactly 60 or 90 Hz, so with Vsync the two slip one refresh against each other about every 17 s at 60 and every 11 s at 90, which can show as a short stutter; at 30 the same happens every 33 s. Presenting on the display's own clock is left for later.

What is blended: 3D draws into the displayed picture that appear in both frames, recognised by where their vertices, indices and texture come from, their transforms, skinned characters' vertices (so bones move smoothly) and scrolling textures. The camera's motion between two frames is taken as one rigid motion of eye space (view times world), measured on the matched draw with the median turn: a turn about an axis through a centre, and a slide along it. In-between frames follow that motion along its arcs, and each draw's own motion is blended in the older frame's eye space, so scenery swings on the arc of a turn and the character the camera follows stays where it is on screen. What is not: the 2D interface and render-to-texture passes, shown as the older frame drew them; 3D draws in only one of the two frames (effects regenerated each frame, objects appearing or leaving), which move with the camera and nothing else; and flipbook animations such as the braziers' fire, whose texture offset jumps to the next cell of an atlas (a tenth of the texture or more in one game frame) and is held rather than swept across the cells between, so they animate at the game's own 30 frames a second. A pair whose draw would move more than 120 units in one game frame beyond what the camera's motion explains is not blended either: it is almost always two different objects, such as instances of one mesh drawn in a different order, or a buffer the game reused for other scenery, and blending them showed an object halfway to another for one present. Such a draw also moves with the camera only. `MHP3RD_INTERPOLATION_NO_MOTION_GUARD=1` and `MHP3RD_INTERPOLATION_NO_FLIPBOOK_GUARD=1` turn these two guards off. Two frames are not blended across a camera cut, a scene change or a loading screen: fewer than half of the draws match, the camera turned more than 30° or its eye moved more than 200 units in one game frame (the eye itself: a 7° orbit at 400 units moves it 49, whatever distant scenery does), or nothing 3D was drawn. A turn or a move that continues the previous frame's (the analog camera at up to 720° a second, 24° a game frame in yaw alone) may go past those limits, up to 75° and 800 units. Movies stay at 30.

How it is drawn. While the rate is above 30, each game frame is recorded as the renderer draws it: its draw calls (merged as usual) with their state, and a summary of every draw for matching it in the next frame. The vertex and index buffers keep three frames, the one being drawn and the two being blended, so drawing a frame again copies no vertices: an in-between frame records the older frame's draw calls again with blended matrices in their push constants. Only skinned draws get new vertices, blended on the CPU from copies of both frames' vertices, and only lit draws whose world matrix moved get a new lighting block. At each flip the frame is submitted and its picture copied; the presents follow a grid of evenly spaced moments aligned with the game's frames, timed by the real time of the vblank each frame started from (kept on a grid of two vblanks, since the game's code often runs past the vblank between two frames on a slower machine), and are made while the kernel waits for real time and, when due, while the game's code runs, up to half a game frame of the latter per frame. The delay allows for the game's code to take as long as the ninth longest of the last 60 frames; a slower frame holds the newest picture for a present or two. The flip no longer presents. The code is in `host/gpu/frame_interpolation.*` (matching, cuts, blending), `host/gpu/frame_pacing.*` (when to present, what each present shows, the rate governor) and `host/gpu/vulkan_renderer.cpp`.

Cost, Apple M1 at ×5 (2400×1360), in the village, vsync off (`MHP3RD_FRAME_RATE_CYCLE`, one run):

| Rate | fps | Speed | CPU per game frame (`render`) | GPU per game frame | One in-between frame: recording / whole present / GPU |
| --- | --- | --- | --- | --- | --- |
| 30 | 30.0 | 100% | 3.8 ms | 7.1 ms | |
| 45 | 45.0 | 100% | 5.4 ms | 12.7 ms | 0.54 / 1.5 / 5.9 ms |
| 60 | 59.8 | 100% | 5.5 ms | 11.3 ms | 0.55 / 1.7 / 5.3 ms |
| 90 | 89.9 | 100% | 6.5 ms | 15.2 ms | 0.59 / 1.6 / 4.7 ms |
| 120 | 119.8 | 100% | 7.1 ms | 18.1 ms | 0.56 / 3.0 / 4.3 ms |

An in-between frame records about 3,300 draw calls; the whole present includes acquiring, submitting and presenting the swapchain image, where MoltenVK waits for a drawable. No present was skipped at any rate.

Steam Deck (LCD, 90 Hz, Game Mode, ×6 under Fill, 2611×1632), village, Vsync on:

| Rate | fps | Speed | CPU per game frame (`render`) | GPU per game frame | One in-between frame: recording / whole present / GPU |
| --- | --- | --- | --- | --- | --- |
| 30 | 30.0 | 100% | 9.5 ms | 5.9 ms | |
| 45 | 45.0 | 100% | 13.8 ms | 11.1 ms | 2.7 / 3.1 / 4.8 ms |
| 60 | 59.9 | 100% | 14.1 ms | 11.2 ms | 2.9 / 3.4 / 4.8 ms |
| 90 | 89.9 | 100% | 16.5-17.0 ms | 16.0 ms | 2.7 / 3.0 / 4.9 ms |
| 120 | 89.3 (Vsync caps it at 90) | 100% | 16.7 ms | 16.1 ms | 2.7 / 3.1 / 4.9 ms |

At 90 the kernel still waits about 12 ms of every game frame, so the rate has room; 60 blended and 30 plain presents a second, none dropped, a delay of 26 ms (the game's code about 4 ms).

A game-side 60 fps patch was not used. The one community code for this game (Saramagrean's CWCheat database, NPJB-40001 and ULJM-05800, "60 FPS Beta") takes the vblank handler at `0x0887669C` from starting a frame every other vblank to every one, then halves a quest timer, one counter and a few animation speeds back. The game has no time step to scale: its timers count frames and its animations take fixed steps (about 280 self-doubling `add.s` in the executable and the quest overlays), so everything else — monsters, their attacks, stamina, hit windows — runs at double speed, as its author and testers say. The overlay half of the NPJB code also targets the PSP release's addresses: our overlays load 0x800000 higher. 120 would be four times the game's own rate. Presenting in between keeps every one of those timings.

### Fast loading

Reading the disc costs nothing here, yet a load takes as long as on a PSP: the game's loader threads read, check and unpack its data a piece at a time and wait on the emulated clock in between, and the kernel holds that clock to real time. Measured with `MHP3RD_TRACE_LOAD`, the host sits idle for 80–95% of a load. **Fast loading** (Video, on by default) lets the clock run ahead while the game loads, up to 16 times real time, so a load takes as long as the work itself.

A load is recognised from what the game does rather than from a timer: it has read from the disc within the last half second of game time, and for a quarter of a second everything it has handed to `sceAudio` has been exact silence once the channel's volume is applied (a loading screen's sound is nothing but zeros). Anything else keeps real time, and ends a fast stretch at once:

- any sound, however quiet: the buffer that carries it plays in full, and so does everything after it. Only the buffers of zeros a fast stretch hands over are dropped, so no sound is ever cut or sped up, and the audio stays in step afterwards;
- a button or a D-pad direction held (moving a stick is fine), so a press never lasts longer in the game than on the pad;
- a movie, the in-game menu, and ad hoc play: once the game has started its ad hoc networking, or a session is going, time stays real for the other players;
- Game speed set to Unlimited, which already runs everything unpaced, and runs without a window.

On a Mac with an M1, from the button press to the new scene's first sound (for the Guild Hall and the farm, *before* is the game time the load took, which normal speed plays in real time):

| Load | Before | After |
| --- | --- | --- |
| Boot to the first logo | 1.0 s | 0.3 s |
| Title to character select (*Now Loading*) | 4.7 s | 2.5 s |
| Character select to the village | 9.2 s | 5.0 s |
| Village to the Guild Hall (offline) | 3.9 s | 0.7 s |
| Guild Hall to the farm | 5.6 s | 1.0 s |
| Village to a quest's base camp | 6.9 s | 1.7 s |
| Quest end to the village | 6.0 s | 1.4 s |

What is left is the game's own music fading out and its animations, which play at real time, the game's check of what it read (`sha1Thread`) and unpacking, and installing a code overlay. While a load runs fast, a flip reaches the window at most about 30 times a second and the others are drawn and not shown, so Vsync never holds it back; frame interpolation pauses and picks up again after it. Each fast stretch logs one line: `[load] fast 4922 ms of game time in 434 ms real, 4488 ms saved (sound; 18.4 s saved so far)`, with what ended it. `MHP3RD_FAST_LOADING=0` or the menu's Off keeps every load at the PSP's pace.

### Audio

| Variable | Default | Effect |
| --- | --- | --- |
| `MHP3RD_NO_AUDIO` | off | Do not open a playback device; the game's audio timing is unchanged |
| `MHP3RD_AUDIO_DUMP` | unset | Write the mixed output to a 44100 Hz stereo WAV file |

### Input

| Variable | Default | Effect |
| --- | --- | --- |
| `MHP3RD_PAD_FACE` | positional | `xbox` puts confirm (○) on the south button (menu: Confirm button) |
| `MHP3RD_PAD_DEADZONE` | `0.15` | Left-stick dead zone, as a fraction of travel (menu: Stick dead zone) |
| `MHP3RD_PAD_TRIGGER` | `0.25` | How far LT/RT travel before they press anything (menu: Trigger point) |
| `MHP3RD_PAD_TRIGGERS` | `standard` | What LT/RT (L2/R2) press: `standard` L and R, `bows` R and △, `bowguns` R and ○ (menu: Trigger profile) |
| `MHP3RD_ANALOG_CAMERA` | on | Proportional yaw and continuous tilt in the ordinary quest camera, the tilt limited to −60°…70° before collision correction. Stick deflection controls speed; release holds the angle. The physical D-pad and recentre return control to the game. Uses the camera update directly, without memory searches or renderer tracing. Off restores stock input and stops camera writes immediately (menu: Analog camera) |
| `MHP3RD_CAMERA_SPEED` | `190` | Degrees a second at full deflection, 20 to 720 (menu: Camera speed) |
| `MHP3RD_AIM_SPEED` | `90` | Degrees a second at full deflection while a bow or a bowgun aims, 10 to 360 (menu: Aim speed) |
| `MHP3RD_MOUSE` | on | `0` leaves the pointer alone: no capture, no mouse camera and no mouse buttons (menu: Mouse) |
| `MHP3RD_MOUSE_SENSITIVITY` | `0.10` | Degrees of camera turn per count of mouse motion, 0.01 to 0.99 (menu: Mouse sensitivity) |
| `MHP3RD_PAD_RSTICK_DPAD` | off | Press D-pad bits from the right stick instead of feeding the HD release's second stick; enabling both would turn the camera twice (menu: Right stick) |
| `MHP3RD_PAD_RSTICK_ZONE` | `0.5` | Right-stick threshold for that (menu: Right stick D-pad point) |
| `MHP3RD_OSK_TEXT` | `Hunter` | Fixed name given when the game asks for one, at once and without the on-screen keyboard unless `MHP3RD_OSK_MODE=keyboard` (menu: Hunter name) |
| `MHP3RD_OSK_MODE` | `keyboard` | `keyboard` opens the on-screen keyboard; `fixed` gives the fixed name at once (menu: When the game asks for a name) |
| `MHP3RD_AUTO_CONFIRM` | off | Press ○ every N frames, to walk through menus unattended |

### Network

| Variable | Default | Effect |
| --- | --- | --- |
| `MHP3RD_ADHOC` | off | `1` turns ad hoc play on (menu: Ad hoc play) |
| `MHP3RD_ADHOC_SERVER` | none | PSP ad hoc server, `host` or `host:port` (menu: Server) |
| `MHP3RD_ADHOC_NICKNAME` | the hunter name | Name other players see (menu: Nickname) |
| `MHP3RD_ADHOC_MAC` | made up once | The address other players know you by, `xx:xx:xx:xx:xx:xx` |
| `MHP3RD_ADHOC_OVERLAY` | off | `1` shows the network overlay from the start |
| `MHP3RD_ADHOC_HOST_PORT` | `27312` | TCP port of the built-in server's matchmaking service; the relay uses the next one (`network.host_port`) |

### Analog camera

**Analog camera** (Controls) is on by default and drives both axes in a quest. Small right-stick deflections turn and tilt slowly; full deflection uses **Camera speed**. The input dead zone and inversion settings apply to both axes. The game retains terrain and wall collision handling. While a bow or a bowgun aims, the right stick moves the aim in proportion to how far it is pushed, at **Aim speed** (the game's own aim moves at one speed of about 100° a second, and only past half the stick's travel), and the game's camera follows the aim as it always does; the analog camera takes over again when the aim ends. A bowgun's scope (a short press of R) counts as an aim: the right stick and the mouse move the scope instead of turning the camera behind it. The game still decides whether the aim may move and which way: nothing moves while rolling, an axis the game locks (vertical aim while walking) stays locked, the scope's left-stick aim is untouched, and the game's Quick Aim Controls option applies. The game's own **Quick Aim Camera** option (START → Options) decides whether the camera turns behind the hunter while aiming: with Type 2 it stays where it is. Fixed village cameras and other special camera modes remain stock; this feature does not unlock them.

The option takes effect at run time, with no regeneration or rebuild. Turning it off stops all analog-camera writes, restores the game's vertical presets and passes the right stick through unchanged. Camera speed is in degrees per second of real time, measured between the game's flips, so it does not change when the game slows down.

The code is in two layers under `host/camera/`:

- `camera_input` is what the player asks for, independent of device and game. Rate sources (the stick, and the camera keys, which push it) hold a fraction of Camera speed; motion sources (the mouse, later a touch drag) add degrees. Every source adds together. New input devices only feed this layer.
- `game_camera` drives the game's camera from that input. The supported executable's ordinary camera calls a rotation helper at `0x088E6264`. The host wraps that helper and recognises this caller, taking the camera address directly from its context. It adjusts yaw and the temporary eye offset before the game applies collision handling. Manual height changes also advance the current eye height to avoid the game's 1/8 smoothing causing a long coast. Each camera mode needs its own driver: only the ordinary follow camera (mode 0) has one, and every other mode keeps the stock camera and stick. Aiming stays in mode 0: each update the camera asks the weapon's code whether it aims and keeps the answer at camera `+0x91` (-1 when not), and while it is not negative the game turns the camera after the aim, so for exactly that time the driver leaves the camera alone and sizes the aim instead. The weapon's aim code (in `game_task`) reads the stick as on/off commands and, in the states where the aim may move, steps the hunter's facing (followed object `+0x188`, copied by the game into `+0x74`) by 512 or 624 and one of three vertical aims (`+0xC22` or `+0x1457`, signed bytes limited to ±100, or `+0xC24`, a halfword limited to ±8192) by a fixed amount. While aiming, the stick reaches the game stretched to full length so the aim code steps at any push, and the driver replaces each step it finds since the previous update with one in proportion to the stick, in the game's direction. No step from the game means no movement. A mouse has no stick, so while the right stick is idle and the mouse has moved, the second stick shows the game the mouse's direction at full length, and a step the game makes then is sized by the mouse's degrees instead; degrees the game has not stepped for wait up to three updates (the game may step an update after it saw the push) and are then dropped, and a step made after they are spent is taken back.

Safeguards: CMake finds the generated unit that holds the rotation helper and fails the configure if none does, so a new partition of the corpus cannot call the wrong code. At start-up the driver compares twenty-two instructions and constants of the game (listed in `game_camera.cpp`) with what it expects and stays out, saying which differs, if any does. The wrapper is installed only when the option is on (from the first frame, by default): a player who turns it off before starting keeps the helper's generated unit on its direct calls, and the feature costs nothing. No shared preset table or generated code is patched, and guest RAM is never scanned.

### Diagnostics

| Variable | Effect |
| --- | --- |
| `MHP3RD_STRICT_HLE=1` | Do not bind logging stubs; stop at the first unimplemented import |
| `MHP3RD_TRACE_KERNEL=1`, `MHP3RD_TRACE_IO=1` | Trace thread and file activity |
| `MHP3RD_TRACE_LOAD=1` | Four `[loadtrace]` lines a second: real and emulated time, flips, disc and memory stick reads, the loudest audio sample, time spent holding the game to real time, and which guest threads had the CPU. What [Fast loading](#fast-loading) was measured with |
| `MHP3RD_TRACE_MODS=1` | What the [mods](#mods) change at start and after each change, every `DATA.BIN` read they serve, and each write made after an overlay loads |
| `MHP3RD_TRACE_DATA_BIN=1` | While a mod is on, the id of every `DATA.BIN` file the game reads (`[mods] data 034B (32768 bytes)`): how to find the file behind a model on screen |
| `MHP3RD_TRACE_SAVEDATA=1` | Log every field of each save-data request and each status poll |
| `MHP3RD_TRACE_SYNC=1` | Trace semaphores, event flags and mutexes; `MHP3RD_TRACE_SYNC_LIMIT` caps the lines (default 4000) |
| `MHP3RD_STARVATION_INTERVAL` | Dispatches between virtual-clock advances in code that never calls an import |
| `MHP3RD_TRACE_GE=1` | Log the first draws of the run with their state |
| `MHP3RD_CHECK_DIRECT_VERTICES=1` | Expand each transformed draw as well and compare it, vertex by vertex and byte for byte, with what its index list names; prints `[direct-check] N draws compared, M differed` every 300 frames. Slow |
| `MHP3RD_TRACE_STALLS=1` | Where the render thread waits, once a second and for every slow frame; `MHP3RD_TRACE_STALLS_MS` sets what is slow (default 40). See [Where the render thread waits](#where-the-render-thread-waits) |
| `MHP3RD_TRACE_INTERPOLATION=1` | With a frame rate above 30, an `[interp]` line a second: the rate running and the one chosen, how many draws matched, the camera's largest turn and the eye's largest move, cuts by reason, presents, the time of a blended present and of recording its draw calls, the draw calls and GPU time of one in-between frame, the plain presents, skipped ones, the latest one, and the delay from a frame's moment to its present. A second line says why presents showed a frame as it is (`at the newest frame`: one per game frame at 60, 90 and 120, as it should be; `at the older`, `not blended (cut)`, `textures dropped`) and which were not made (`display busy`: no swapchain image within 3 ms; `over budget`: presents during the game's code had taken half a frame). It also says when the rate steps down or up, and why. `frames` adds a line per game frame, `presents` a line per present with its blend factor |
| `MHP3RD_PAD_AT_FLIP=1` | Read the pad at each flip, as before issue #8, instead of when the game reads it |
| `MHP3RD_INTERPOLATION_NO_MOTION_GUARD=1` | Blend every matched pair however far it moves, and leave 3D draws without a partner where the older frame drew them, as before the guard. The `[interp] guards:` line counts what the guard does: pairs given up and how many had the same mesh drawn more than once, the largest own motion kept, draw calls moved with the camera only |
| `MHP3RD_INTERPOLATION_NO_FLIPBOOK_GUARD=1` | Blend texture offsets that jump up to half the texture, as before, instead of holding a flipbook's step; `[interp] guards:` counts the offsets held |
| `MHP3RD_CHECK_REPLAY=1` | Every 150 game frames, draw the older frame again from its recording without blending and compare it with its own picture pixel by pixel: `[interp] replay check … 0 of N pixels differ`. With `MHP3RD_SCREENSHOT_DIR` it also writes the older frame, the frame drawn again, the frame halfway to the newer one and the newer one as `replay_<frame>_*.bmp` |
| `MHP3RD_FRAME_RATE_CYCLE=30,60,90` | Switch the frame rate to the next one listed every `MHP3RD_FRAME_RATE_CYCLE_SECONDS` (default 10), to compare rates on one scene in one run |
| `MHP3RD_INTERPOLATION_EXTRA_MS=N` | Add N ms of busy CPU time to every blended present, to see the frame rate step down on a fast machine as it would on a slow one |
| `MHP3RD_TRACE_3D=1` | Per-frame counts of transformed draws, their targets and screen-space bounds |
| `MHP3RD_DEBUG_MENU=1` | Developer builds only: show the menu's **Debug** page, with cheats for testing (money, any item or equipment piece into the boxes, infinite health and stamina, a frozen quest clock, monsters at 1 health). Release builds (`-DMHP3RD_RELEASE=ON`) do not contain it, and it writes nothing during ad hoc play. See [docs/DEBUG_MENU.md](../../docs/DEBUG_MENU.md) |
| `MHP3RD_DEBUG_COMMANDS=path` | With `MHP3RD_DEBUG_MENU=1`: a file read while the game runs; each line appended to it is a command run at the next flip (memory search and dumps, `give`, `money`, `giveequip`, `quest` and more), answered with `[debug]` lines. The commands are listed in [docs/DEBUG_MENU.md](../../docs/DEBUG_MENU.md) |
| `MHP3RD_FIND_CAMERA=1` | Hunt guest memory for the words the camera is kept in, by what they do: one hunt against the yaw the view matrix reports and one against its pitch, trying every word as a float, a 32-bit and a 16-bit number, and as an angle, a rate, or a rate read a frame early. `MHP3RD_FIND_CAMERA_OUT` names a file the surviving list is written to |
| `MHP3RD_FIND_STEP=N` | Keep the 16-bit fields that move by exactly N between turning frames. The camera's own yaw moves by 1150, which is 1150/65536 of a turn |
| `MHP3RD_FIND_FLOAT=V`, `MHP3RD_FIND_INT32=N` | List every place in guest memory holding that value. `MHP3RD_FIND_INT16_WIDE=1` searches 16-bit fields instead of 32-bit |
| `MHP3RD_POKE_FOUND=V`, `MHP3RD_POKE_INT32=N` | Write a different value into what those found, once a frame. With more than one match, `MHP3RD_POKE_WHICH` must name an index or say `all`, since writing every place that held a number also writes whatever else held it |
| `MHP3RD_POKE_FLOAT=0xADDRESS:V[,...]` | Write floats into guest memory once a frame, to turn a guess about a constant into a measurement |
| `MHP3RD_TRACE_CAMERA_STATE=path.csv` | Trace ordinary camera updates: frame, guest camera address, enabled state, stick axes, yaw target/current, stock eye offset and target height, owned pitch, recentre/D-pad flags, adjusted offset and previous observed pitch. Does not enable memory searches or renderer tracing |
| `MHP3RD_TRACE_CAMERA_MODES=path.csv` | Every call of the camera's rotation helper from the camera update, and one line per game flip, with the camera mode, the aim the weapon reports, the stick, the yaw fields and the whole camera structure in hex. For finding what a camera mode keeps where before it has a driver. Needs Analog camera turned on once in the session, which installs the hook the trace runs in |
| `MHP3RD_TRACE_AIM=path.csv` | While a bow or a bowgun aims or a bowgun's scope is up: one line per camera update with the scope flag, the stick's and the mouse's degrees, the mouse degrees carried, the game's own step (yaw:pitch), the step taken off in advance and the step the driver made; and one line per frame the mouse shows the game a direction. For telling the game's steps from the driver's |
| `MHP3RD_TRACE_CAMERA=1` | One line per frame for the camera the game itself set: the second stick's offset from centre, the yaw and pitch read out of the frame's busiest view matrix, the turn since the previous frame, and the camera's world position. Reads the game's own camera, so it tells a stepped turn from a continuous one |
| `MHP3RD_TRACE_WHITE_TEXTURES=1` | Each texture the decoder cannot decode, once, with its address, size, format, swizzle and palette: those draws are made with a white texture instead, so this is the first thing to check when something draws white |
| `MHP3RD_TRACE_FB_TEXTURES=1` | Each distinct texture that lies in a framebuffer the renderer drew (with both layouts), each large texture, `sceDmacMemcpy` copies into or out of VRAM, GE block transfers, new render targets, and the GE commands the renderer ignores. Add `PSPRECOMP_TRACE_VRAM_READS=1` to log game code reading VRAM with the CPU, per 64 KiB block and at most once a second |
| `MHP3RD_TRACE_SPRITES=N` | Every through-mode draw of presented frame N: sprites one by one, other primitives by their bounds, with positions, texture coordinates and texture state |
| `MHP3RD_TRACE_MATERIAL=1` | Every distinct value the game writes to the GE material registers |
| `MHP3RD_TRACE_LIGHTING=1` | Every distinct value the game writes to the GE light and fog registers, and one line per distinct register state a lit draw is made with |
| `MHP3RD_TRACE_TEXTURE_PACK=1` | Each texture's texture pack key and what the pack does with it (its image, kept as it is, or not listed), each image decoded and uploaded, and once a second the draws that used a replacement and the images and megabytes on the GPU |
| `MHP3RD_SAMPLED_TEXTURE_KEYS=1` | Recognise changed textures of up to 64 KiB by one word in every 256 bytes, as for larger ones, instead of by all of their contents. Glyphs the game adds to its text atlas are then often missed, and text shows stale or missing characters |
| `MHP3RD_NO_CULL=1`, `MHP3RD_NO_DEPTH=1` | Disable face culling or the depth test, to bisect missing geometry |
| `MHP3RD_TRACE_AUDIO=1` | One line per second of output: frames, peak, RMS, silence and drops |
| `MHP3RD_TRACE_ATRAC=1` | Every `sceAtrac3plus` call with its arguments, result and decode position |
| `MHP3RD_TRACE_FONT=1` | Every `sceLibFont` call with its arguments: the font the game asks for, the font info and character metrics returned, and each glyph image's buffer and 26.6 position, with the caller's return address |
| `MHP3RD_TRACE_MPEG=1` | Every `sceMpeg` and `sceJpegCsc` call, and each call the ring buffer makes to the game's read callback |
| `MHP3RD_SAS_NO_ENV=1` | Hold every SAS voice at full envelope, to separate an envelope bug from a decoding one |
| `MHP3RD_TRACE_PAD=1` | Log the pad state whenever it changes |
| `MHP3RD_TRACE_OSK=1` | Every keyboard utility call with the status it returns, and the words of the parameter block, its first field and the strings they point to |
| `MHP3RD_TRACE_ADHOC=1` | Every ad hoc, network dialog and wireless call with its arguments and result, and every packet header sent to or received from the ad hoc server (menu: Network, *Log every call and packet*) |
| `MHP3RD_INPUT_SCRIPT` | Scripted keys, mouse motion and buttons, virtual-gamepad buttons and axes, dropped files and window captures, for testing the menu, the setup, the on-screen keyboard and the game's controls without a person at the controls; the syntax is in `host/ui/input_script.hpp`. Its virtual gamepad also becomes the game's pad, in place of a real one that is connected; its keys reach the game through the bindings as well as the interface; with mouse steps the pointer counts as captured without taking the real one. Example: `300:key Escape;330:shot menu;360:pad leftstick+rightstick;400:key W 30;430:mouse 50 0` |
| `MHP3RD_INPUT_LIVE` | A file read while the game runs; each line appended to it is an input-script step timed from when it is read, to drive two instances side by side |
| `MHP3RD_DUMP_OVERLAYS` | Directory to dump an overlay that has no library into |
| `PSPRECOMP_NO_INTERPRETER=1` | Stop at uncompiled code instead of interpreting it |
| `PSPRECOMP_MAX_DISPATCHES` | Stop after this many dispatches |
| `PSPRECOMP_HLE_HISTOGRAM=1` | Print import call counts on exit |

### Performance statistics

With `MHP3RD_PERF=1` the game draws a small overlay into the top-left corner of the presented image, so it appears in window and Steam screenshots and in `MHP3RD_SCREENSHOT_DIR` captures, and prints one line per second to stdout, flushed as it is written:

```text
[perf] fps 30.0 game 30.0 speed 100% | frame avg 33.4 max 34.7 ms | guest 4.1 render 9.8 wait 19.5 ms | lists 60/s draws 7712/2310 | FIFO 1440x816 90Hz | gpu 7.6 max 10.6 ms | overlay 0.05 ms
```

`MHP3RD_PERF=log` prints the line without the overlay. F3 shows or hides the overlay at any time, with or without the variable; there is deliberately no gamepad combination for it. The menu's *Performance* setting chooses the same modes, plus the overlay without the log. The statistics are collected all the time, so turning them on changes nothing else.

A frame runs from one guest flip (`sceDisplaySetFrameBuf`, where the renderer presents) to the next. With a [frame rate](#frame-rate) above 30 the renderer presents between flips instead: `fps`, `frame avg` and `max` and the overlay's graph then follow the presents, while `game`, `guest`, `render`, `wait` and `gpu` stay per game frame and include the in-between frames' work.

| Field | Meaning |
| --- | --- |
| `fps` | Frames presented per second of real time |
| `game` | Frames the game flips per second of *emulated* time: its own frame rate, 30 when it keeps up with its target |
| `speed` | Emulated time per real time: 100% when the game runs at PSP speed. The kernel holds its clock to real time, so it stays at 100% unless frames take longer than the game's frame time; below 100% the game runs slow |
| `frame avg`, `max` | Real time between presents over the last second |
| `guest` | The rest of the frame: recompiled code, HLE, the kernel, input and audio |
| `render` | CPU time turning display lists into Vulkan commands and recording the present, without the GPU waits inside it. The kernel's hold to real time happens outside it and does not reduce it |
| `wait` | Time blocked on the GPU: the frame fence, swapchain acquire, queue submit and present, the queue idle waits of texture uploads and evictions, and framebuffer read-backs for GE block transfers. With FIFO presentation, pacing to the display shows up here, and so does the time the kernel waits to hold the game to real time |
| `lists` | Display lists enqueued per second of real time |
| `draws` | Draws the GE made per frame / draw calls the renderer recorded for them; the second is smaller when consecutive draws with the same state are merged |
| `FIFO 1440x816 90Hz` | Present mode, swapchain size and the display's refresh rate as SDL reports it |
| `gpu`, `max` | GPU time per frame, averaged over the second, and the longest: from the first command of the frame to its last draw, measured with Vulkan timestamp queries and read back after the frame's fence, so it lags the frame by one. The copy to the window is not included. `gpu n/a` when the graphics queue has no timestamps (`timestampValidBits` 0) or `MHP3RD_NO_GPU_TIMESTAMPS` is set; the log says which at start-up |
| `overlay` | CPU time spent drawing the overlay, when it is shown |
| `interpolation` | The frame rate presented, when it is above 30; `interpolation 60 of 90` when it stepped down from the one chosen |
| `space` | The most of the vertex and index buffers one frame took in the second, in MiB, against 32 and 4 MiB a frame. What does not fit is not drawn (the interface goes first, as it is drawn last), and the log then says `[render] a frame ran out of vertex space` |
| `passes`, `cleared`, `copies` | Render passes a frame begins, presents between flips included; of those, the ones begun without loading what their first draw, a clear, overwrites; and full-size copies or blits of a render target (for sampling it as a texture, for frame interpolation's pictures, for the write-back to guest memory, to the window). On a phone's tiled GPU each pass reads and writes its target, and each copy moves a whole one |

The overlay shows the same numbers (GPU time on the second line, when there is one) and a graph of the last 192 frame times, from 0 to 50 ms, with guides at 16.7 and 33.3 ms: green up to 34 ms, yellow up to 50 ms, red beyond.

#### Comparing the old and new renderer paths

The renderer changes made for speed each have an off switch (see [Video](#video)). `MHP3RD_PERF_ALTERNATE=name[,name...]` instead turns the named ones off every other second, so a single run compares them under the same scene and load; each `[perf]` line then ends in `alt on` or `alt off`. Stand still in one spot for a minute and compare the `render` (CPU) and `gpu` numbers of the `on` and `off` lines. Names: `direct` (`MHP3RD_NO_DIRECT_VERTICES`), `lookup` (`MHP3RD_NO_LOOKUP_CACHE`), `reuse` (`MHP3RD_NO_BUFFER_REUSE`), `merge` (`MHP3RD_NO_DRAW_MERGE`), `store` (`MHP3RD_NO_FAST_STORE`), `decode` (`MHP3RD_NO_FAST_DECODE`), `alpha` (`MHP3RD_NO_ALPHA_VARIANTS`), `uploads` (`MHP3RD_SYNC_UPLOADS`), `clearload` (`MHP3RD_NO_CLEAR_LOAD`), `gpudecode` (GPU vertex decode, when `MHP3RD_GPU_DECODE=1` turns it on) and `texturedecode` (`MHP3RD_SYNC_TEXTURE_DECODE`).

#### Where the render thread waits

`MHP3RD_TRACE_STALLS=1` adds a `[stalls]` line after each `[perf]` line, and a `[slow-frame]` line for every frame longer than `MHP3RD_TRACE_STALLS_MS` milliseconds (default 40), so a spike can be matched to its cause:

```text
[stalls] ms per frame over 30 frames: fence 0.14 max 1.88 x30 acquire 0.08 max 0.10 x30 submit 3.15 max 3.37 x30 present 0.02 max 0.03 x30 pacing 21.68 max 7.48 x196 copy 0.02 max 0.02 x30 store 0.43 max 0.46 x30
[slow-frame] 714 152.0 ms | guest 2.5 render 81.6 wait 67.9 ms | gpu(prev) 2.0 ms | fence 2.02 x1 acquire 0.09 x1 submit 6.88 x1 present 0.03 x1 upload 43.93 x80 pacing 14.91 x4 copy 0.02 x1 store 0.51 x1
```

On Android, where no variable can be set, the `[stalls]` line comes with the `[perf]` line whenever Performance logs, so a log saved from the menu has it. Each kind that happened is listed with its time per frame averaged over the second, its longest single stall and how many there were; a `[slow-frame]` line gives that frame's totals and the GPU time of the frame before it, which is what a fence wait at the start of the frame waits for.

| Kind | Where |
| --- | --- |
| `fence` | A frame's fence: before a frame slot is recorded again (the frame two back, with two frames in flight), and before the previous frame's picture is written back to guest memory at the next flip. The GPU is still busy with that frame |
| `acquire` | `vkAcquireNextImageKHR` |
| `submit` | The frame's `vkQueueSubmit`; MoltenVK waits for the next drawable here |
| `present` | `vkQueuePresentKHR` |
| `upload` | A texture upload that waits for the queue to go idle, and so for the frames before it: with `MHP3RD_SYNC_UPLOADS`, or one made outside a frame |
| `evict` | The queue idle wait before a cached texture is destroyed to make room, with `MHP3RD_SYNC_UPLOADS` |
| `readback` | A framebuffer read back for a GE block transfer (submits the frame so far and waits for it) |
| `idle` | Other device idle waits, such as a movie frame changing size |
| `pacing` | The kernel holding the game to real time (not a GPU wait, but part of `wait`) |
| `copy` | CPU time copying the written-back frame out of mapped memory, which may be uncached (part of `render`) |
| `decode` | Waiting at the frame's submit for textures decoded in the background |
| `pipeline` | CPU time creating graphics pipelines the frame needed (part of `render`). A burst of them, as in a new area, is also reported once it settles: `[render] 4 new pipelines in 12.3 ms, 57 so far; pipeline cache saved` |
| `store` | CPU time converting that frame into guest memory, `store_frame` (part of `render`) |

#### A report from a phone

On Android, set Performance to *Log* in the menu, play to the place that is slow for a minute, then use *Save the log…*. The start of the log names the Vulkan driver (`[render] Vulkan …, driver …`), the memory the vertex buffer lives in, the swapchain's images and present mode, the internal resolution (`Renderer: … target WxH`) and how many frames are in flight. Each second then has a `[perf]` and a `[stalls]` line. Where to look:

- `gpu` close to or above the frame time (33 ms at 30 fps, 16.7 ms at 60): the GPU is the limit. Lower Resolution first, then Frame rate.
- `render` plus `guest` close to the frame time: the CPU is the limit; Frame rate's in-between frames add to `render`.
- `fence`, `acquire` or `present` large in `[stalls]`: waiting for the GPU or the display.
- `pipeline` and `[render] N new pipelines in X ms` lines: shader compiles. They should appear once per new area and not again in a later run, which loads them from `pipeline_cache.bin`.
- `upload` in `[stalls]`: texture uploads that waited for the GPU, which should not happen during play any more.

## Host layout

```text
host/main.cpp                    Entry point: finding the game data, executable check, startup
host/app_paths.{hpp,cpp}         The executable's own location, and what a release ships next to it
host/install/                    First-run installer: per-user directory, image checks, executable preparation
host/settings/                   Player settings: settings.ini, environment overrides, defaults
host/camera/                     Camera input from every device, the driver for the game's own camera, and its view's shape
host/input/                      Keyboard and mouse bindings: names, settings.ini spelling, what held keys press
host/ui/                         Yakumo's own interface (Dear ImGui): in-game menu, setup screens, file browser, on-screen keyboard
host/debug/                      Developer tools (not in release builds): the game's money, boxes and quest state, cheats, command file
host/overlays.{hpp,cpp}          Overlay library loading and run-time installation
host/kernel/kernel.{hpp,cpp}     Scheduler, waits, virtual clock, interrupts, memory
host/kernel/iso_image.{hpp,cpp}  Read-only ISO 9660 view of the disc image
host/hle/hle_threadman.cpp       ThreadManForUser, Kernel_Library
host/hle/hle_sysmem.cpp          SysMemUserForUser, sceSuspendForUser, sceDmac
host/hle/hle_io.cpp              IoFileMgrForUser, sceUmdUser
host/hle/hle_system.cpp          Utils, LoadExec, Stdio, ModuleMgr, interrupts, power, RTC
host/hle/hle_media.cpp           sceDisplay, sceCtrl, sceGe_user, sceAudio, sceSasCore
host/hle/hle_atrac.cpp           sceAtrac3plus: ATRAC3 music decoded frame by frame, loops, positions
host/hle/hle_mpeg.cpp            sceMpeg and sceJpegCsc: the movie player's ring buffer, access units and decoding
host/hle/hle_font.cpp            sceLibFont over a host font
host/fonts/game_font.*           The game's text font: loading, fitting glyphs into the game's cells, fallback, installed fonts
host/hle/hle_utility.cpp         sceUtility on-screen keyboard and message dialog
host/hle/hle_savedata.cpp        sceUtility save-data dialog
host/hle/hle_adhoc.cpp           sceNet, sceNetAdhoc, sceNetAdhocctl, sceNetAdhocDiscover, sceWlanDrv, sceUtilityNetconf
host/adhoc/                      Client for PSP ad hoc servers: wire formats, network thread, diagnostics
host/hle/utility_dialog.hpp      Status life cycle shared by the dialogs
host/save_data/                  AES-128, PARAM.SFO, the save-data encryption and hashes, save folders
host/gpu/ge_state.{hpp,cpp}      GE command state machine: display lists to draw calls
host/gpu/vulkan_renderer.*       Vulkan backend, window and input
host/gpu/texture_pack.*          HD texture packs: textures.ini, texture keys, background image decoding, texture dumps
host/gpu/texture_pack_import.*   Texture pack import: finding a pack in a folder, its checks, the copy, swap and backup
host/gpu/replacement_textures.*  Texture pack images on the GPU: uploads with mip levels, memory budget
host/gpu/frame_interpolation.*   Frame rate above 30: matching draws between frames, cuts, blending transforms
host/gpu/frame_pacing.*          Frame rate above 30: when to present and what, the rate governor
host/gpu/shaders/                GLSL, compiled to SPIR-V and embedded at build time
host/perf/frame_stats.*          Frame timing, the per-second summary and the [perf] log line
host/perf/perf_overlay.*         Performance overlay drawn on the CPU with a built-in 5x7 font
host/audio/audio_sink.*          SDL3 playback device and the mixing ring buffer
host/audio/sas_core.*            Software SAS: VAG decoding, pitch, envelopes, 32 voices
host/audio/atrac_decoder.*       ATRAC3 and ATRAC3plus frames to PCM through FFmpeg's libavcodec
host/movie/psmf_demuxer.*        PSMF program stream packs to H.264 pictures and ATRAC3plus frames
host/movie/avc_decoder.*         H.264 pictures to planar YCbCr through FFmpeg's libavcodec
```

Every import runs at the outer dispatch level, so a blocking import saves the caller's context with `pc = $ra` and loads another thread's context; the runtime's thread identity check keeps generated code from resuming in the wrong thread.

## Directory layout

```text
config/       Executable identity and overlay slot map
host/         Bootstrap, kernel, HLE, graphics, audio
scripts/      prepare_game.sh, generate.sh, build_overlays.sh, bootstrap_overlays.sh, release_linux.sh
packaging/    Release packaging: third-party notices; linux/ holds the Flatpak manifest, launcher and SDK build
tools/        ISO and DATA.BIN extraction, overlay wrapping, shader and NID table embedding
tests/        Save-data self-tests and save checker (mhp3rd_savedata_tests)
third_party/  stb_truetype, tiny-AES-c (installer and saves), Dear ImGui (menu and setup screens)
game/         Local game data: EBOOT.ELF, disc.iso, ms0/ (ignored)
analysis/     Analyzer output and extracted overlays (ignored)
generated/    Recompiled executable (ignored)
overlays/     One directory per recompiled overlay (ignored)
```
