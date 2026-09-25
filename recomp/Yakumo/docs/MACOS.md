# Playing on macOS

This guide is for players using a prebuilt release. To build Yakumo yourself instead, follow [`profiles/mhp3rd/README.md`](../profiles/mhp3rd/README.md); building from source stays fully supported.

A release does not include any game assets or original game files: no disc image, no copy of the game's executable or data, and no textures, models, audio or video from the game. You must provide the files from your own legally obtained copy of **Monster Hunter Portable 3rd HD Ver.** (`NPJB-40001`): its disc image, as an uncompressed `.iso` file. The first start checks that copy, accepts only the original release, and sets the game up from it.

Each [release](https://github.com/TeamGDB/Yakumo/releases) has the macOS app as a disk image, `yakumo-<version>-macos-arm64.dmg`.

It needs a Mac with Apple Silicon (M1 or newer) and macOS 13 Ventura or newer. Intel Macs are not supported. It is tested on macOS 27; macOS 13 to 26 should work but are untested, so please [report](https://github.com/TeamGDB/Yakumo/issues/new/choose) how it runs on them. Vulkan (MoltenVK), SDL3, FFmpeg and a Japanese font come with it; nothing else needs to be installed.

`SHA256SUMS` on the same page lists the checksum of each file; `shasum -a 256 -c SHA256SUMS --ignore-missing` in the download folder checks them.

## Install

1. Open the `.dmg`.
2. Drag **Yakumo** into **Applications**.

## First start

Yakumo is signed only ad hoc, not by an Apple Developer ID, and not notarized by Apple. So the first time you open it, macOS says it cannot verify that Yakumo is free of malware, or that it "cannot be opened", and offers only *Done* and *Move to Trash*. Choose *Done*, then allow it once:

1. Open **System Settings → Privacy & Security**.
2. Scroll down to *Security*. There is a line saying Yakumo was blocked; click **Open Anyway**.
3. Confirm with your password or Touch ID, and choose **Open Anyway** in the dialog that follows.

macOS remembers the choice, and later starts open straight away. You have to allow it again after replacing it with a new version.

The same from Terminal, instead of the steps above:

```bash
xattr -dr com.apple.quarantine /Applications/Yakumo.app
```

This removes the "downloaded from the internet" mark that makes macOS check the app.

Yakumo then opens its first-run setup in its own window, usable with a gamepad, the keyboard or the mouse:

1. The welcome screen says what is needed. Confirm to go on.
2. Pick your disc image in Yakumo's file browser. The row of places at the top holds Home, Downloads, Desktop, Documents and every mounted volume under `/Volumes`. *System dialog…* opens the macOS file dialog instead, and an `.iso` dropped onto the window is taken as well. macOS may ask whether Yakumo may access the folder the image is in, such as Downloads or a removable volume; allow it.
3. Yakumo checks the image. A different release or region, a modified image or a compressed `.cso` gets a screen that says so.
4. Choose whether to **copy** the image (the default, about 1.3 GB) or **use it where it is**. Copying keeps the game working if the original is moved or its drive is disconnected; using it in place saves the space, but the image has to be there every time you play.
5. Yakumo prepares the game from the image and starts the game.

Later starts go straight to the game. Esc, or L3+R3 on a gamepad, opens Yakumo's menu with the settings; *Set up game data again…* there repeats the setup.

## Where your data lives

```text
~/Library/Application Support/Yakumo/MHP3rd/
    EBOOT.ELF      the game's executable, prepared from your disc image
    disc.iso       the copy of your disc image (absent when you use it in place)
    settings.ini   where the image is, and the settings from Yakumo's menu
    ms0/           the memory stick
        PSP/SAVEDATA/ULJM05800/   your saves
```

In Finder, choose *Go → Go to Folder…* and paste the path. The folder is not inside the app, so it survives updates and is only removed if you delete it. To keep your saves safe, use *Back up saves…* in Yakumo's menu (System section), or back up `ms0` yourself. Saves from a PSP or PPSSPP import with *System → Import save…*, as the [profile README](../profiles/mhp3rd/README.md#importing-a-save-from-a-psp) describes.

## Update

Replace `Yakumo.app` in Applications with the new one, then allow it once more as in [First start](#first-start). Your data, settings and saves stay.

## Uninstall

Move `Yakumo.app` to the Trash. This keeps your data. To remove it as well, **including your saves and the copied disc image**, delete `~/Library/Application Support/Yakumo`.

## Multiplayer

Open **Network** in Yakumo's menu to host a session or join one, on your home network, over a VPN, or through an ad hoc server; the [profile README](../profiles/mhp3rd/README.md#multiplayer-ad-hoc) explains how. macOS asks once whether Yakumo may find devices on your local network; allow it for sessions on your network. When you host with the macOS firewall on, allow Yakumo's incoming connections when asked. Because the app is signed ad hoc, the firewall may ask again after each update.

## Troubleshooting

- **Messages from the game.** Start it from Terminal to see what it prints: `/Applications/Yakumo.app/Contents/MacOS/Yakumo`.
- **"Yakumo is damaged and can't be opened"**: the quarantine mark is still there and the app was changed, or the download is incomplete. Check the download against `SHA256SUMS`, copy the app again, and run the `xattr` command above.
- **Set up from Terminal** instead of the setup screens:

  ```bash
  /Applications/Yakumo.app/Contents/MacOS/Yakumo --install /path/to/your.iso              # copy the image
  /Applications/Yakumo.app/Contents/MacOS/Yakumo --install /path/to/your.iso --in-place   # use it where it is
  ```

- Every other setting is described in the [profile README](../profiles/mhp3rd/README.md#configuration).

Report problems with the [test report form](https://github.com/TeamGDB/Yakumo/issues/new/choose); [`TESTING.md`](TESTING.md) describes what to check.
