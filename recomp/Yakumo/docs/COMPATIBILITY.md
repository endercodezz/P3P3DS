# Compatibility

What works on each platform, as last checked by hand. Each row is a part of the game; each cell is its state on that platform, with the issue that tracks any problem.

| Status | Meaning |
| --- | --- |
| ✅ | Works |
| ⚠️ | Works, with a known problem |
| ❌ | Does not work yet |
| ❔ | Not checked on this platform |

## Game

| | macOS (Apple Silicon) | Linux | Steam Deck | Windows | Android (emulator only, device test pending [#127]) |
| --- | --- | --- | --- | --- | --- |
| Build | ✅ | ❔ [#14] | ✅ in a Debian 13 container [#14] | ✅ MSVC 19.44, all 355 overlay DLLs [#13] | ✅ NDK r28c, arm64-v8a; all 355 overlay libraries |
| Boot, title and menus | ✅ | ❔ | ✅ | ✅ | ✅ |
| Character creation | ✅ | ❔ | ✅ | ❔ | ✅ |
| Village | ✅ | ❔ | ✅ | ❔ | ❔ |
| Hunts | ✅ | ❔ | ✅ | ❔ | ✅ |
| Graphics | ✅ lighting, fog, the quest reward screen and tiled 2D screens | ❔ | ✅ lighting, fog, the quest reward screen and tiled 2D screens; ⚠️ lighting slows the busiest village spots slightly [#7] | ❔ | ❔ on a device |
| Sound effects | ❔ re-check [#4]: the game now runs at real time | ❔ | ❔ | ❔ | ❔ |
| Music | ✅ with FFmpeg | ❔ | ❔ | ❔ | ❔ |
| Cutscene movies | ✅ with FFmpeg; the opening movie checked | ❔ | ❔ | ❔ | ✅ the opening movie |
| Saving and loading | ✅ PSP-format saves; no dialog screens yet [#33] | ❔ | ✅ a save copied from a PSP loads | ✅ a new character saves and loads after restarting the game [#13] | ✅ saves load; import and export through Android's file picker |
| Downloadable content | ✅ a player's `ULJM05800QST` folder is read by the download menu | ❔ | ❔ | ❔ | ❔ |
| Multiplayer | ✅ with a Steam Deck through an ad hoc server: hall and a full quest [#2] | ❔ | ✅ with a Mac through an ad hoc server: hall and a full quest [#2] | ❔ | ❔ |

Rows marked ❌ on every platform are missing features rather than platform problems.

## Input

| | macOS (Apple Silicon) | Linux | Steam Deck | Windows | Android (emulator only, device test pending [#127]) |
| --- | --- | --- | --- | --- | --- |
| Keyboard | ✅ | ❔ | ✅ | ❔ | ⚠️ the emulator's own keyboard releases keys at once; scrcpy's keyboard is the workaround |
| DualSense | ✅ | ❔ | — | ❔ | ✅ through scrcpy on the emulator |
| DualShock 4 | ❔ | ❔ | — | ❔ | ❔ |
| Xbox controllers | ❔ | ❔ | — | ❔ | ❔ |
| Built-in controls | — | — | ✅ Game Mode (added to Steam as a non-Steam game); ⚠️ Desktop Mode sends mouse and Esc from Steam's desktop layout | — | ✅ on-screen touch controls |

## Tested hardware

| Platform | Machine | GPU and driver | Commit | Date |
| --- | --- | --- | --- | --- |
| macOS 27 | Apple M1, 8 GB | Apple M1, MoltenVK | `v0.1.0` | 2026-09-18 |
| macOS 27 | Apple M1, 8 GB | Apple M1, MoltenVK | `3480dc2` (saving and loading) | 2026-09-18 |
| SteamOS 3.8.16 | Steam Deck | AMD Custom GPU 0932, RADV (Mesa 26.0.0-devel) | `5e4b27c` | 2026-09-18 |
| SteamOS 3.8.16 | Steam Deck | AMD Custom GPU 0932, RADV (Mesa 26.0.0-devel) | `v0.3.0` | 2026-09-18 |
| macOS 27 | Apple M1, 8 GB | Apple M1, MoltenVK | `v0.3.0` | 2026-09-19 |
| Windows 11 Pro | x64 PC | AMD Radeon RX 6500 XT | `98e2468` | 2026-09-19 |
| Windows 11 Pro | x64 PC | AMD Radeon RX 6500 XT | `b67cd7a` (saving and loading) | 2026-09-20 |
| Android 15 emulator (API 35, arm64) | Apple M1, 8 GB | Apple M1 through the emulator's gfxstream | `7b87a57` | 2026-09-23 |

## Updating this page

Run through [the smoke test](TESTING.md) on the platform, then change the cells you checked in the same pull request as the fix, or in a pull request of their own. Add a row to *Tested hardware* with the commit you tested. A result without a commit cannot be compared with anything later, so it does not go in the table.

To report a result without editing the page, open a **Test report** issue.

[#1]: https://github.com/TeamGDB/Yakumo/issues/1
[#2]: https://github.com/TeamGDB/Yakumo/issues/2
[#3]: https://github.com/TeamGDB/Yakumo/issues/3
[#4]: https://github.com/TeamGDB/Yakumo/issues/4
[#5]: https://github.com/TeamGDB/Yakumo/issues/5
[#6]: https://github.com/TeamGDB/Yakumo/issues/6
[#7]: https://github.com/TeamGDB/Yakumo/issues/7
[#13]: https://github.com/TeamGDB/Yakumo/issues/13
[#14]: https://github.com/TeamGDB/Yakumo/issues/14
[#33]: https://github.com/TeamGDB/Yakumo/issues/33
[#127]: https://github.com/TeamGDB/Yakumo/issues/127
