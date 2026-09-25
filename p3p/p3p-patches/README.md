
# P3P Patches

![mod](https://cdn.discordapp.com/attachments/546718581572894730/976179261578809404/mod.gif)

- [Usage](#usage)
  - [PPSSPP CWCheat](#ppsspp-cwcheat)
  - [PSP CWCheat](#psp-cwcheat)
  - [Patching `EBOOT.BIN`](#patching-ebootbin)
- [Available Patches](#available-patches)

## Usage

### PPSSPP CWCheat

1. Place `ULUS10512.ini` in `ms0:/PSP/Cheats/`. If the file already exists, add the contents of the provided `ULUS10512.ini` file to it.
2. Enable the patches you'd like to use via the cheats menu.

### PSP CWCheat

1. Install CWCheat and add the contents of `ULUS10512.ini` to `ms0:/seplugins/cwcheat/cheat.db`.
2. Make sure to add `CHEAT ENABLE = 1` to `ms0:/seplugins/cwcheat/cwcheat.ini` so that patches may be applied on boot.
3. Enable the patches you'd like to use via the cheats menu.

### Patching `EBOOT.BIN`

If the CWCheat method doesn't work, you can unpack the game and directly patch `EBOOT.BIN` with the provided [xdelta][xdelta_url] patches:

```cmd
xdelta -vfn merge -m intro.xdelta mod.xdelta tmp.xdelta
xdelta -vfn -d -s EBOOT.BIN tmp.xdelta PATCHED_EBOOT.BIN
```

Overwrite the original `EBOOT.BIN` with the patched file, then repack and boot the game.

## Available Patches

For ULUS10512 (US) v1.00

- *Intro Skip* - Skips logos and intro movie when booting the game.

- *Mod Support* - File replacement via (listed from highest to lowest load priority):

  1. Loose files in `ms0:/PSP/P3P/bind/`
  2. `ms0:/PSP/P3P/mod.cpk`
  3. `ms0:/PSP/P3P/mod1.cpk`
  4. `ms0:/PSP/P3P/mod2.cpk`
  5. `ms0:/PSP/P3P/mod3.cpk`

  Make sure to **disable the Data Install setting** via the Config menu as files from installed data will override some game files (even if they were replaced via a mod).

  Loose file loading is done via a leftover debug function and might be unstable.

[xdelta_url]: https://github.com/jmacd/xdelta-gpl
