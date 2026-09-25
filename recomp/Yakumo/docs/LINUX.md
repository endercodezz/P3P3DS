# Playing on Linux and the Steam Deck

This guide is for players using a prebuilt release. To build Yakumo yourself instead, follow [`profiles/mhp3rd/README.md`](../profiles/mhp3rd/README.md); building from source stays fully supported.

A release does not include any game assets or original game files: no disc image, no copy of the game's executable or data, and no textures, models, audio or video from the game. You must provide the files from your own legally obtained copy of **Monster Hunter Portable 3rd HD Ver.** (`NPJB-40001`): its disc image, as an uncompressed `.iso` file. The first start checks that copy, accepts only the original release, and sets the game up from it.

Each [release](https://github.com/TeamGDB/Yakumo/releases) has two Linux downloads:

| File | For |
| --- | --- |
| `yakumo-<version>-linux-x86_64.flatpak` | The Steam Deck, and any distribution with Flatpak. Recommended. |
| `yakumo-<version>-linux-x86_64.tar.gz` | Any x86-64 distribution, without installing anything |

`SHA256SUMS` on the same page lists the checksum of each file; `sha256sum -c SHA256SUMS --ignore-missing` in the download folder checks them.

## Steam Deck and Flatpak

### Install

1. Switch to Desktop Mode (hold the power button, then *Switch to Desktop*).
2. Download the `.flatpak` file from the release page into Downloads.
3. Open Konsole and run:

   ```bash
   flatpak install --user ~/Downloads/yakumo-*-linux-x86_64.flatpak
   ```

   Answer `y`. Flatpak also downloads the Freedesktop runtime (`org.freedesktop.Platform` 25.08) from Flathub if it is not installed yet; SteamOS usually has it already.

Yakumo then appears in the application menu under *Games*.

### First start

Start Yakumo from the application menu, or with `flatpak run io.github.teamgdb.Yakumo`. The setup runs in the game's window and works with the Deck's controls alone. The buttons each screen uses are shown at its bottom; which face button confirms follows the button layout set in Yakumo's menu.

1. The welcome screen says what is needed. Confirm to go on.
2. Pick your disc image in Yakumo's file browser. The row of places at the top holds Home, Downloads and every SD card or USB drive (on the Deck, the SD card shows under its label). Confirm opens a folder or picks the file, back goes up a folder, and Ⓨ switches between `.iso` files and all files. *System dialog…* opens the desktop's own file dialog instead; it is not offered in Game Mode.
3. Yakumo checks the image. A different release or region, a modified image or a compressed `.cso` gets a screen that says so.
4. Choose whether to **copy** the image (the default, about 1.3 GB in your home folder) or **use it where it is**. Copying keeps the game working if the original is moved or its SD card is removed; using it in place saves the space, but the image has to be there every time you play.
5. Yakumo copies the image and prepares the game from it, then starts the game.

Later starts go straight to the game. Esc, or L3+R3 (both sticks pressed in), opens Yakumo's menu with the settings; *Set up game data again…* there repeats the setup.

### Game Mode

Add Yakumo to Steam once, in Desktop Mode:

1. Open Steam, then *Games* → *Add a Non-Steam Game to My Library…*
2. Tick **Yakumo** in the list and click *Add Selected Programs*. Steam launches it through Flatpak by itself.
3. Return to Game Mode. Yakumo is in the library under *Non-Steam*.

In Game Mode Steam presents the Deck's controls to the game as a gamepad; if a button does something unexpected, open the controller settings for Yakumo and choose the *Gamepad* layout. The first setup also works in Game Mode, through Yakumo's own file browser.

To change the name or artwork Steam shows, open the shortcut's *Properties* in Steam.

### Where your data lives

Everything Yakumo keeps is in the Flatpak's own data directory:

```text
~/.var/app/io.github.teamgdb.Yakumo/data/Yakumo/MHP3rd/
    EBOOT.ELF      the game's executable, prepared from your disc image
    disc.iso       the copy of your disc image (absent when you use it in place)
    settings.ini   where the image is, and the settings from Yakumo's menu
    ms0/           the memory stick
        PSP/SAVEDATA/ULJM05800/   your saves
```

It survives updates and is removed only if you ask for it (see [Uninstall](#uninstall)). To keep your saves safe, use *Back up saves…* in Yakumo's menu (System section), which copies them to `save-backups` in this directory or to a folder you choose, or back up `ms0` yourself.

### Import a save from a PSP

Saves use the PSP's own format, so a save from a PSP's memory stick or from PPSSPP works unchanged. Yakumo's menu imports it, with a gamepad:

1. Put the save where Yakumo can see it: connect the memory stick or SD card, or copy the folder into your home folder. On a PSP's memory stick the folder of *Monster Hunter Portable 3rd* is `PSP/SAVEDATA/ULJM05800`, with downloaded quests in `ULJM05800QST`.
2. In the game, open the menu (L3+R3, or Esc) and go to **System → Import save…**.
3. Open the save folder, or choose *Import from this folder* on a folder that holds several, such as the memory stick's `PSP/SAVEDATA`. Removable drives are in the row of places at the top.
4. Check what is shown, with the save it replaces, and confirm. The replaced save is kept in `ms0/PSP/SAVEDATA/.backup/`, not deleted.
5. Choose **Restart now**: the game reads saves at the title screen, which then leads to character select with the imported characters.

**System → Back up saves…** copies your saves to `save-backups` in the data directory above, and **Open the saves folder** and **Open the backups folder** show where they are. The Flatpak reads your home folder and removable drives, but it writes only to its own data directory and to your **Downloads** folder. So *Export save…* and backups work when you pick Downloads or the backups folder; any other folder fails. The [profile README](../profiles/mhp3rd/README.md#importing-a-save-from-a-psp) describes the checks and the backups.

By hand, with Yakumo closed, copy the folder into `~/.var/app/io.github.teamgdb.Yakumo/data/Yakumo/MHP3rd/ms0/PSP/SAVEDATA/`, keeping a copy of any `ULJM05800` folder already there first: it holds all three character slots.

```bash
mkdir -p ~/.var/app/io.github.teamgdb.Yakumo/data/Yakumo/MHP3rd/ms0/PSP/SAVEDATA
cp -r /path/to/memory-stick/PSP/SAVEDATA/ULJM05800 ~/.var/app/io.github.teamgdb.Yakumo/data/Yakumo/MHP3rd/ms0/PSP/SAVEDATA/
```

In Dolphin, the file manager, Ctrl+H shows hidden folders such as `.var` in your home folder.

### Update

Download the new `.flatpak` and install it the same way:

```bash
flatpak install --user ~/Downloads/yakumo-<new version>-linux-x86_64.flatpak
```

It replaces the old version. Your data, settings and saves stay, and the Steam shortcut keeps working.

### Uninstall

```bash
flatpak uninstall --user io.github.teamgdb.Yakumo
```

This keeps your data. To remove it as well, **including your saves and the copied disc image**, add `--delete-data`, or delete `~/.var/app/io.github.teamgdb.Yakumo`. `flatpak uninstall --unused` then removes a runtime nothing else uses. Remove the Steam shortcut from the library as with any game.

### Multiplayer

Ad hoc multiplayer works in the Flatpak as it does elsewhere: open **Network** in Yakumo's menu (Esc, or L3+R3) to host a session or join one, on your home network, over a VPN, or through an ad hoc server. The [profile README](../profiles/mhp3rd/README.md#multiplayer-ad-hoc) explains how. SteamOS has no firewall by default. On a desktop with one, allow TCP 27312 and 27313 to host, and UDP 27314 for sessions on your network to show up. On a VPN without broadcast, such as Tailscale, type the host's address instead of waiting for it to be listed.

### Permissions

The Flatpak asks for as little as the game needs:

| Permission | Why |
| --- | --- |
| Wayland, with X11 as a fallback; X11 shared memory | The game window |
| GPU (`dri`) | Vulkan rendering |
| Input devices | Gamepads, including the Deck's built-in controls and Steam Input in Game Mode |
| PulseAudio (PipeWire serves it on current systems) | Sound |
| Network | Ad hoc multiplayer: joining a server, hosting a session with the built-in server, and finding sessions on the local network |
| Home folder, read-only | Finding your disc image with the gamepad file browser. The desktop's file dialog needs no permission, but it is not available in Game Mode. |
| `/run/media` and `/media`, read-only | Disc images on SD cards and USB drives |
| Downloads folder, read and write | Where *Export save…* and *Back up saves…* can write outside the app's own data directory |

Yakumo writes only to its own data directory. It uses the network only for multiplayer, when you host or join a session. If your image is somewhere else, for example on a second drive mounted under `/mnt`, give Yakumo read access to it:

```bash
flatpak override --user --filesystem=/mnt/games:ro io.github.teamgdb.Yakumo
```

## Other distributions: the tarball

The tarball runs without installing anything. It needs:

- x86-64 Linux with glibc 2.29 or newer: Ubuntu 20.04, Debian 11, Fedora 31, SteamOS 3 or anything newer
- A Vulkan driver and the Vulkan loader (`libvulkan.so.1`): Mesa on AMD and Intel, or NVIDIA's driver
- Wayland or X11, and PipeWire, PulseAudio or ALSA for sound

SDL3, FFmpeg and a Japanese font come with it.

```bash
tar -xzf yakumo-<version>-linux-x86_64.tar.gz
cd yakumo-<version>-linux-x86_64
./yakumo
```

Always start it through `./yakumo`, which gives the game the 64 MiB stack it needs. The setup is the same as in the Flatpak. Its data, settings and saves are in `~/.local/share/Yakumo/MHP3rd/` (under `$XDG_DATA_HOME` if you set it), with the saves in `ms0/PSP/SAVEDATA/`; import a PSP save there as described above.

To set up from a terminal instead of the setup screens:

```bash
./yakumo --install /path/to/your.iso              # copy the image
./yakumo --install /path/to/your.iso --in-place   # use it where it is
```

**Update:** unpack the new version and delete the old folder. Your data stays in `~/.local/share/Yakumo`.
**Uninstall:** delete the folder, and `~/.local/share/Yakumo` to remove your data and saves too.

To add the tarball to Steam, choose *Browse…* in *Add a Non-Steam Game* and pick the `yakumo` script.

## Troubleshooting

- **Messages from the game.** Start it from a terminal (`flatpak run io.github.teamgdb.Yakumo`, or `./yakumo`) to see what it prints.
- **No gamepad in the Flatpak** on a system with Flatpak older than 1.15.6, which does not know the input-device permission: `flatpak override --user --device=all io.github.teamgdb.Yakumo`.
- **The game's text is missing.** Yakumo draws it with a Japanese font from the system and falls back to the one it ships. `MHP3RD_FONT=/path/to/font.ttf` picks another; in the Flatpak, set it with `flatpak override --user --env=MHP3RD_FONT=... io.github.teamgdb.Yakumo`.
- Every other setting is described in the [profile README](../profiles/mhp3rd/README.md#configuration). In the Flatpak, pass environment variables with `flatpak run --env=NAME=value io.github.teamgdb.Yakumo`.

Report problems with the [test report form](https://github.com/TeamGDB/Yakumo/issues/new/choose); [`TESTING.md`](TESTING.md) describes what to check.
