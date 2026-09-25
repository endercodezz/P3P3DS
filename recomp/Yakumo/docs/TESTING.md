# Testing

## Automated

- **Framework tests** run with `ctest --test-dir out/framework` and need no game data.
- **Builds on every platform** in CI — planned in [#15](https://github.com/TeamGDB/Yakumo/issues/15).
- **Regression tests on your own copy of the game**, replaying recorded input and comparing frames against reference images — planned in [#16](https://github.com/TeamGDB/Yakumo/issues/16).

Until those exist, changes are checked by playing, with the smoke test below.

## Smoke test

About fifteen minutes. It walks through every part of the game that currently works, so a regression anywhere shows up. Start from a fresh profile — rename `profiles/mhp3rd/game/ms0` aside — so earlier state cannot hide a problem.

Before you start, write down the commit you are testing: `git rev-parse --short HEAD`. A result is only useful with it.

A released build is tested the same way. Note its version and which download it is (Flatpak or tarball) instead of the commit, start it through its launcher (`flatpak run io.github.teamgdb.Yakumo` or `./yakumo`, from a terminal to see the console), and start from a fresh data directory: for the Flatpak, move `~/.var/app/io.github.teamgdb.Yakumo` aside; for the tarball, `~/.local/share/Yakumo`. The first start then runs the setup from your disc image, which is part of the test. [`LINUX.md`](LINUX.md) says where a release keeps its saves.

| # | Step | Expected |
| --- | --- | --- |
| 1 | Start `out/mhp3rd/bin/Yakumo` | A window opens; the console lists 355 overlay corpora, the renderer and the audio device |
| 2 | Wait through the logos | Movies are skipped (see #6) and the title screen appears; streamed music is silent (see #5) |
| 3 | Start a new game | Character creation appears |
| 4 | In character creation, change each option | The character model is whole and textured, animates, and changes with each option |
| 5 | Enter a name, confirm, and save to a slot when asked | The console logs `[savedata] saved … ULJM05800 (encrypted)` and the game moves on to the hot spring scene |
| 6 | Watch the hot spring scene | Water, steam and the waterfall draw correctly; characters have soft shadows, not white patches |
| 7 | Talk through the scene and walk out | The village loads; the marker over an NPC's head is red, characters are shaded, and distant geometry fades into the fog |
| 8 | Walk around the village | Everything draws; it runs slower than elsewhere (see #7) |
| 9 | Take a quest and depart | The quest map loads with the HUD, the minimap and the character's weapon |
| 10 | Hunt a small monster | Monsters appear and animate; attacks, hits and sound effects work |
| 11 | Stand still with no input for ten seconds | The character and the camera stay still |
| 12 | Move the camera with the right stick, if you have a gamepad, and with the mouse | The camera turns and stops when the stick is released or the mouse stops |
| 13 | Return to the village | The village loads again |
| 14 | Close the window and start the game again | The console logs `[savedata] loaded … (decrypted)`; after the title screen, character select lists the character from step 5 |
| 15 | Pick that character | The game continues from the save |

### What to watch for throughout

- Any line reading `[interpreter] no recompiled function at …` — that code runs about twenty times slower. Note the address.
- Missing, torn or flickering geometry, and black or white patches where effects should be.
- The console's last lines if the game stops or crashes.

### Analog camera regression (#106)

Build `mhp3rd_camera_tests` and run it through CTest. These checks require no game data and cover dispatch interception, proportional rates, fractional yaw, pitch limits, release without filter catch-up, Off passthrough, special modes, scene changes and the extent of guest writes.

For the manual check, start without `MHP3RD_TRACE_CAMERA` or `MHP3RD_FIND_CAMERA`. Keep **Controls → Analog camera** on (the default), then enter an ordinary quest. Test small and full stick deflections on both axes, release, reversal, movement near walls, a zone transition, L recentre, physical D-pad commands, and Off/On toggles. With a bow and a bowgun, aim (R, and the bowgun scope) and move the right stick, then the mouse: the aim must move in proportion on both axes without shaking when the motion starts, stops or reverses, stop at its vertical limits, and the camera follow it; the analog camera must take over again after the aim. Confirm that the village cameras retain their own behaviour. Watch for residual vertical coast and camera movement after input has stopped. Repeat on Steam Deck before marking that platform verified.

### Picture shape and size (#117)

Build `mhp3rd_aspect_tests` and run it through CTest. These checks need no game data and cover what is written to guest memory for a wider or narrower view, that the game's own shape writes nothing, that switching back restores every value bit for bit, and that different game code is left alone.

For the manual check, open **Video** in the menu and compare the three **Aspect ratio** values on the same screen, in a quest and in the village:

- **Original** with a fixed resolution looks exactly as before, bars included.
- **Fill** fills the window with no bars. Circles (the minimap, round icons) stay round; the 3D view is not stretched: a monster turning in place keeps its proportions. The HUD sits in a centred PSP-shaped area.
- In **Fill**, look for objects popping in or out at the left and right edges while the camera turns, especially at 21:9 or wider.
- Fades, the pause menu's darkening, the blur of the item and map menus, and the quest-reward screen cover the whole window.
- Switching the value back and forth takes effect at once; the console logs `[aspect] the game's view is … wide to 1 high`, and back at Original the game's `1.76471`.
- With **Resolution** on Auto, resize the window, toggle fullscreen and, where possible, move it to another display: after a moment the console logs `[render] internal resolution W×H` with the window's size.
- On a Steam Deck (1280×800, 16:10), Fill with Auto draws 1280×800 and should hold 30 fps in a quest.

### Keyboard and mouse (#94)

`mhp3rd_input_tests` (CTest) checks the bindings without SDL or game data: key and button names, how `settings.ini` spells them, the shipped layouts, the menu's rebinding rules and what held keys press. `mhp3rd_camera_tests` covers the mouse in the camera layer: its turn in the ordinary camera, sizing the game's aim steps from the mouse (including a step the game makes an update late, and one made after the mouse stopped), and switching the game's own turn where the port does not drive the camera.

For the manual check, unplug the gamepad (or leave it untouched) and play from the title screen with the keyboard and mouse only, on the default layout (see the profile README's *Keyboard and mouse*):

- While the game window has focus the pointer is hidden and captured. Esc opens the menu and the pointer comes back; closing the menu takes it again. Switching to another window (Cmd+Tab, Alt+Tab) gives it back; returning takes it again. The on-screen keyboard for the hunter's name and the setup screens never take it.
- Create or load a hunter, walk the village with W A S D, talk (F or the right button), take a quest from the menus (F or the right button confirms, Space goes back) and depart.
- In the quest, the mouse turns and tilts the camera smoothly and stops where it stops; *Mouse sensitivity* and both *Invert mouse* settings change it at once. With *Analog camera* off, moving the mouse sideways turns the game's own camera while it moves. Q puts the camera behind the hunter.
- Attack with the left button, roll with Space, use an item with E, guard or run with Left Shift. With a bow: hold Left Shift and move the mouse, the aim follows in proportion, then shoot with the left button; with a bowgun, fire with the right button. Rolling or walking while aiming must not move the aim more than the game allows.
- Hold a key or a mouse button, open the menu with Esc, release it, close the menu: nothing stays pressed. Pick the gamepad up mid-quest and put it down again: both work, and no camera motion is left over.
- In Controls, rebind a control (activate its row, press a key or a mouse button), check it in play and in `settings.ini`, try *Use the classic keyboard layout* and *Restore control defaults*.

With `MHP3RD_TRACE_PAD=1` the console shows `[pad] pointer captured` and `[pad] pointer free` as the pointer changes hands, and the mouse's motion in counts and degrees.

### Texture pack import (#49)

`mhp3rd_texture_pack_tests` (CTest) checks the import without game data or a window: finding a pack in each layout (the pack folder, `textures/NPJB40001`, `NPJB40001`, `PSP/TEXTURES/NPJB40001`, in any case), a pack named for `ULJM05800` taken only when its `[games]` lists `NPJB40001`, `quick` and hashless packs refused, zipped packs refused with "unpack it first", key, image, size and missing-image counts, the copy into a staging folder, the swap that moves the old pack to `textures/.backup/<date>_<time>/NPJB40001`, cancelling, and where the pack is read from with `MHP3RD_TEXTURE_PACK` and a pack used in place. `mhp3rd_texture_pack_tests --check <folder>` prints what the menu would find in a real folder and reads nothing else.

For the manual check, use a throwaway data folder (`MHP3RD_DATA_DIR`, with a copy of `settings.ini`, `EBOOT.ELF` and the disc image) and a real pack, then in **Video**:

- *Import texture pack…* with the gamepad only: browse to the pack, open its folder (it is chosen at once), and read the review: the key count, hash, images, size, missing images, free space and the pack in use now. Back returns to the folders; *Cancel* imports nothing.
- Choose the pack's parent folder, a folder holding `PSP/TEXTURES/NPJB40001`, and a copy renamed `ULJM05800` with and without `NPJB40001 = true` under `[games]`: the first three are found, the last is refused with its reason. A copy whose `textures.ini` says `hash = quick` is refused.
- *Copy into Yakumo's data folder*: the progress bar moves, the game keeps drawing (or stays paused) without stutter, Esc/Start do not close the menu. Cancel half way: the console logs `[texpack] copy cancelled`, the result says nothing changed, `textures/` holds no `.incomplete-…` folder and the old pack still draws.
- Copy again to the end: the Texture pack row shows the new count within a frame or two and the textures change on screen. Import a second time: the first pack is in `textures/.backup/<date>_<time>/NPJB40001`, whole.
- *Use it where it is*: nothing is copied, `settings.ini` has `video.texture_pack_folder`, the textures stay. Rename the pack folder and turn *Texture pack* off and on: the row says *Pack folder missing: …*. *Stop using the pack folder* goes back to the installed pack.
- With the keyboard and mouse: the same with clicks, and drag the pack folder from the file manager onto the window while the browser is open.

### Mods (#79, #80, #82)

`mhp3rd_mods_tests` (CTest) needs no game data: `mod.ini` reading (quoted values, lists, a quote left open, `Version` and the PSP default, `FilesHD`/`TargetHD`, packs, pseudo packs, equipment slots, code mods refused, missing files and bad targets), mhp3reload's files named by id, priority and conflicts, packs and dependencies, the saved choices, import and its backup, the `DATA.BIN` obfuscation from any byte, and a small archive served with a grown replacement, a smaller one, a patch and a moved verbatim entry, read whole and in pieces. `mhp3rd_mods_tests --check-disc <image.iso>` reads the real directory (only that) and checks that it encodes back to the disc's bytes.

For the manual check, never use downloaded mods for a regression you cannot undo: use a throwaway data folder (`MHP3RD_DATA_DIR`, with copies of `settings.ini`, the saves, `EBOOT.ELF` and the disc image) and `MHP3RD_MODS_DIR` pointing at a scratch folder, and make test mods from the game's own files with `profiles/mhp3rd/tools/databin.py … extract`: for example file `0FEE` (the game menu's textures) with part of each texture's pixels overwritten, in a folder with a `mod.ini` of `Type="File"`, `Version="HD"`, `Files`, `Target="0FEE"`. Run with `MHP3RD_TRACE_MODS=1`.

- With no mods folder, the log has no `[mods]` lines beyond the count, and the game is unchanged.
- Turn the mod on in **Mods** and restart: the log lists `0FEE <- …` and `[mods] read 0FEE …` lines, and the game menu (after the title) shows the change. Turn it off: *Applied* and, after a restart, the original textures. `MHP3RD_NO_MODS=1` gives the original too, with the mod still on in the menu.
- A copy of the same file 300 KiB larger (zeros appended): *Restart to apply* and *Restart now*; after the restart the log says `DATA.BIN grows from 1208858624 to …` and the title screen, its music and the intro movie, all read from entries after the grown one, are as before.
- A patch mod (`Type="Patch"`) for `0FEF` with blocks of `(offset, length, bytes)` into its textures and `FFFFFFFF00000000` at the end: stripes on the title screen. Together with the grown `0FEE`, which moves `0FEF`, the same.
- Two mods on the same file: *Conflicts* names the winner; *Priority* left and right changes it (after the next load or a restart).
- A `mod.ini` without `Version`, a `Type="Code"` mod and one with a missing file show as *Cannot be used* with the reason.
- *Import mod…* with the gamepad only, then with the mouse and by dropping a folder on the window: a single mod folder, a folder holding two, and a mod already installed (it moves to `mods/.backup/<name>-<time>`). The imported mods are off. A preview.png shows on the mod's screen.
- An equipment mod: type the target file id on the on-screen keyboard; the row shows it and `mods.ini` has `slot1=`.

### Renderer performance paths (#92)

Build `mhp3rd_render_tests` and run it through CTest: it checks, without a GPU or game data, that the index lists transformed draws are now drawn with name exactly the vertices the old expansion wrote, in the same order.

The speed changes each have an off switch that restores the old path: `MHP3RD_NO_DIRECT_VERTICES`, `MHP3RD_NO_LOOKUP_CACHE`, `MHP3RD_NO_BUFFER_REUSE` and `MHP3RD_NO_DRAW_MERGE`. When a frame looks wrong, run once with all four set: if the fault goes away, set them one at a time to find the change behind it, and report which. `MHP3RD_CHECK_DIRECT_VERTICES=1` compares every transformed draw with the old expansion while playing and prints `[direct-check] N draws compared, M differed`; M must stay 0.

To measure, run with `MHP3RD_PERF=log MHP3RD_TRACE_STALLS=1` and stand still in the village by the shop and the smithy passage for a minute. `MHP3RD_PERF_ALTERNATE=direct,lookup,reuse,merge` turns the new paths off every other second; compare the `render` and `gpu` numbers of the `alt on` and `alt off` lines. A spike shows up as a `[slow-frame]` line naming where the frame waited.

### Frame rate (#39)

Build `mhp3rd_interpolation_tests` and run it through CTest. It needs no GPU or game data and checks matching draws between two frames, the cut rules (a camera turn that keeps growing past 30 degrees blends, a sudden one does not), blending, when each present falls and what it shows at 45, 60, 90 and 120 (the blend factors, no picture going back, skipped presents never queued), and how the frame rate steps down and back up.

For the manual check, open **Video → Frame rate**:

- **30** looks and times exactly as before. The `[perf]` line has no `interpolation` field.
- At **60**, walk and turn in the village and in a quest: movement, the camera and characters' limbs are smooth, with no doubled or jumping objects. The interface and the minimap stay as they are. A camera cut in a cutscene, entering an area and a loading screen show no blended frame.
- In a quest with **Controls → Analog camera** on and *Camera speed* at 720, turn the camera at full deflection, alone and diagonally with the tilt: no stutter, and with `MHP3RD_TRACE_INTERPOLATION=1` the `[interp]` lines show no `camera turned` or `camera moved` cuts while turning (a turn measured 24.5-25.4 degrees and 174-180 units per game frame on the Mac).
- Switching the value while playing takes effect at once; the menu pausing the game and resuming it, changing *Resolution*, *Aspect ratio* or *Vsync*, and turning *Game speed* to Unlimited (which greys the row) do not break it. Switching to 120 on a 60 or 90 Hz screen with Vsync never stalls the game: the row shows *120 (running at 90)*.
- **Lower when behind**: with it off, the chosen rate stays even if `speed` drops; with it on (the default) a rate the machine cannot hold steps down within two seconds and the log says why. *Restore video defaults* sets it back to On.
- The pad: walking, attacking and turning respond as quickly at 30 as before, or quicker. With `MHP3RD_PAD_AT_FLIP=1` the old reading at the flip comes back, to compare.
- During a camera turn the `[interp] plain:` line must show only `at the newest frame` (30 a second), and no `camera moved` cuts; the hunter must not shake against the scenery.
- The braziers in front of the guild hall and in the gathering hall burn as fast as at 30. NPCs, villagers and single objects never appear displaced for one present while you walk or turn; the `[interp] guards:` line counts the pairs it gave up. For comparison, `MHP3RD_INTERPOLATION_NO_FLIPBOOK_GUARD=1` and `MHP3RD_INTERPOLATION_NO_MOTION_GUARD=1` bring the old behaviour back.
- `MHP3RD_CHECK_REPLAY=1` must print `0 of N pixels differ` for every check.

To measure, run with `MHP3RD_PERF=log MHP3RD_TRACE_INTERPOLATION=1` and stand still in the village by the shop and the smithy passage. `MHP3RD_FRAME_RATE_CYCLE=30,45,60,90` switches the rate every ten seconds; skip the first two `[perf]` lines after each `[interp] cycle:` line. For each rate read:

- `[perf]`: `fps` (45, 60 or 90 — on a Steam Deck with Vsync, 120 and *Match display* run at 90), `speed` (100%), `render` and `gpu` per game frame, and `frame … max` (no regular spikes above the present interval).
- `[interp]`: `presents` and `skipped` (0 or close to it), the ms of a blended present and of its `recording`, its `gpu`, `late up to` and the `delay` (at 90 on a Steam Deck about 26 ms, with the game's `code` about 4 ms). On the `plain:` line only `at the newest frame` should count, and `display busy` and `over budget` should stay at 0.
- No `[interp] frame rate … -> …` line while standing still; if one appears, it gives the speed, the spare time a frame and the cost of a blended present that made the rate step down.

`MHP3RD_INTERPOLATION_EXTRA_MS=16` with *Frame rate* 90 shows the step-down on a fast machine: within a few seconds `[interp] frame rate 90 -> 60: the game had no time to spare`, then 60 fps with nothing skipped.

## Reporting

Open a **Test report** issue with the platform, hardware, commit and the steps you reached. If a result changes a cell in [the compatibility table](COMPATIBILITY.md), update the table in a pull request as well.

Useful settings while testing — all described in [the profile README](../profiles/mhp3rd/README.md#configuration):

- `MHP3RD_SCREENSHOT_DIR` and `MHP3RD_SCREENSHOT_EVERY` capture frames to attach to a report.
- `MHP3RD_TRACE_PAD=1` shows whether input is reaching the game.
- `MHP3RD_TRACE_AUDIO=1` shows audio levels and dropped frames.
- `PSPRECOMP_HLE_HISTOGRAM=1` prints which system calls the game made.
