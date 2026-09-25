Yakumo for Linux (x86-64)
=========================

A native port of Monster Hunter Portable 3rd HD Ver. Yakumo does not include
any game assets or original game files: no disc image, no copy of the game's
executable or data, and no textures, models, audio or video from the game.
You must provide the files from your own legally obtained copy of the game
(NPJB-40001): its disc image. The first start checks that copy and accepts
only the original release.

Start it with

    ./yakumo

The first start asks for your disc image, copies it into Yakumo's data
directory (~/.local/share/Yakumo/MHP3rd) and prepares the game from it.
After that the game starts straight away. Saves are kept in the same
directory, under ms0/PSP/SAVEDATA.

    ./yakumo --install /path/to/image.iso   set up from a terminal instead
    ./yakumo --help                         all options

Needs: an x86-64 Linux with glibc 2.29 or newer, a Vulkan driver (Mesa on
AMD and Intel, or NVIDIA's), and Wayland or X11. On a Steam Deck the
Flatpak is easier; see the guide below.

Guide for players, including the Steam Deck:
    https://github.com/TeamGDB/Yakumo/blob/main/docs/LINUX.md

Third-party software and its licenses: licenses/THIRD_PARTY_NOTICES.md
