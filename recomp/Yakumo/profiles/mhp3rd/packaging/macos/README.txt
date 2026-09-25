Yakumo for macOS (Apple Silicon)
================================

A native port of Monster Hunter Portable 3rd HD Ver. Yakumo does not include
any game assets or original game files: no disc image, no copy of the game's
executable or data, and no textures, models, audio or video from the game.
You must provide the files from your own legally obtained copy of the game
(NPJB-40001): its disc image, as an uncompressed .iso file. The first start
checks that copy and accepts only the original release.

Needs: a Mac with Apple Silicon (M1 or newer) and macOS @MINIMUM_SYSTEM_VERSION@ or newer.
Tested on macOS 27. It is built to run on macOS @MINIMUM_SYSTEM_VERSION@ to 26 as well, but
that is untested: please report how it runs there (link below).


Install
-------

Drag Yakumo into the Applications folder.


First start
-----------

This build is not notarized by Apple, so the first time you open it macOS
says that it cannot verify the developer, or that Yakumo "cannot be opened",
and offers only Done or Move to Trash. Choose Done, then:

  1. Open System Settings > Privacy & Security.
  2. Scroll down to the message about Yakumo and click "Open Anyway".
  3. Confirm with your password or Touch ID, then "Open Anyway" once more.

macOS remembers this; later starts open straight away. The same works from
Terminal instead, if you prefer:

    xattr -dr com.apple.quarantine /Applications/Yakumo.app

The first start then asks for your disc image in Yakumo's own window: pick
the .iso file, let Yakumo check it, and choose whether to copy it (about
1.3 GB) or use it where it is. Yakumo prepares the game from it and starts
the game. Later starts go straight to the game. Esc, or L3+R3 on a gamepad,
opens Yakumo's menu with the settings.

If macOS asks whether Yakumo may access your Downloads, Documents or Desktop
folder, or a removable drive, allow it for the folder your disc image is in.


Where your data lives
---------------------

    ~/Library/Application Support/Yakumo/MHP3rd/
        EBOOT.ELF      the game's executable, prepared from your disc image
        disc.iso       the copy of your disc image (absent when used in place)
        settings.ini   where the image is, and the settings from the menu
        ms0/PSP/SAVEDATA/ULJM05800/   your saves

(In Finder: Go > Go to Folder..., then paste the path.) Updating or deleting
Yakumo.app leaves this folder alone; back up ms0 to keep your saves safe, or
use "Back up saves..." in Yakumo's menu.


Update and uninstall
--------------------

To update, replace Yakumo.app in Applications with the new one (and allow it
once more as above). To uninstall, move Yakumo.app to the Trash; delete the
folder above as well only if you no longer want your saves.


More: https://github.com/TeamGDB/Yakumo/blob/main/docs/MACOS.md
Report a problem: https://github.com/TeamGDB/Yakumo/issues/new/choose
Third-party software and its licenses: Yakumo.app/Contents/Resources/licenses/
