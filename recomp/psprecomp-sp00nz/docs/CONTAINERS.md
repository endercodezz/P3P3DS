# Containers — from a disc dump to a module

Getting from a PSP disc image to something a decoder can read means peeling
four layers. A real retail game uses all of them, and the tool's `info`
subcommand walks as far down as it can before it hits the encryption.

```
game.iso                            ISO9660, 2048-byte sectors
 └── PSP_GAME/
     ├── PARAM.SFO                  disc identity: DISC_ID, TITLE, firmware
     ├── SYSDIR/
     │   ├── BOOT.BIN               the plain ELF — zeroed on retail discs
     │   └── EBOOT.BIN              ~PSP-wrapped, encrypted: the real module
     └── USRDIR/
         ├── module/*.prx           ~SCE -> ~PSP: engine libraries
         └── gamesharing_en/*.dat   PBP -> DATA.PSP -> ~PSP: standalone modules
```

## ISO9660

Standard, and the PSP uses it plainly — no UDF, no exotic extensions. The
primary volume descriptor sits at sector 16 (`0x8000`) and is identified by
`CD001` at offset 1. Its root directory record is embedded at offset 156.

Two details that bite:

- **Directory records never straddle a sector.** A record length of zero means
  "skip to the next 2048-byte boundary", not "end of directory". Treating zero
  as a terminator truncates the listing at the first sector boundary — which
  on a real disc silently loses most of the tree.
- **Filenames carry a version suffix**, `EBOOT.BIN;1`. Strip at the semicolon.

`iso_extract` flattens the ISO path into the output filename
(`PSP_GAME_SYSDIR_EBOOT.BIN`) because a disc has both
`SYSDIR/EBOOT.BIN` and `SYSDIR/UPDATE/EBOOT.BIN` — extracting by basename
silently overwrites one with the other, and the one you lose is the one you
wanted.

## PBP

A flat container: a 40-byte header holding a magic (`\0PBP`), a version, and
eight absolute offsets. There are no explicit sizes — each segment runs to the
next segment's offset, and the last runs to end-of-file. Equal adjacent offsets
mean an empty segment.

| # | Segment | Contents |
|---|---|---|
| 0 | `PARAM.SFO` | identity and metadata |
| 1 | `ICON0.PNG` | the list icon |
| 2 | `ICON1.PMF` | animated icon |
| 3 | `PIC0.PNG` | background art |
| 4 | `PIC1.PNG` | background art |
| 5 | `SND0.AT3` | background music |
| 6 | **`DATA.PSP`** | **the executable module** |
| 7 | `DATA.PSAR` | packed game data (PSN/minis payload) |

Segment 6 is the one that matters. `allegrexrecomp info` recurses into it and
reports the inner module's format, name and key tag.

WTF's game-sharing modules are PBPs of exactly this shape — a small
`PARAM.SFO` naming the microgame, and a ~4 MB `DATA.PSP`:

```
format:   PBP container (version 0x10000)
  PARAM.SFO   offset 0x00000028  size        440
  DATA.PSP    offset 0x000001E0  size    4212688  [~PSP encrypted PRX]
  TITLE              Baseball Superstar
```

## PARAM.SFO

Two tables at header-declared offsets: a key table of NUL-terminated names, and
a data table of values. Each of the `count` index entries gives a key offset, a
format word (`0x0404` = `uint32`, `0x0204` = UTF-8), a length, a maximum
length, and a data offset.

The parser bounds-checks every offset against the buffer and refuses an absurd
entry count, because `PARAM.SFO` is the first thing read from an untrusted dump
and a corrupt one should produce an error rather than a walk off the end.

## `~SCE` and `~PSP`

`~SCE` is a thin 0x40-byte shim; whatever follows it is the real payload,
usually a `~PSP`. Most of the `.prx` files in `USRDIR/module/` are `~SCE`
wrapping `~PSP`.

`~PSP` is the encrypted-module header, 0x150 bytes:

| Offset | Field |
|---|---|
| `0x00` | magic `~PSP` |
| `0x04` | module attributes |
| `0x06` | compression attributes |
| `0x0A` | module name, 28 bytes — **not guaranteed NUL-terminated** |
| `0x27` | segment count |
| `0x28` | size of the decrypted ELF |
| `0x2C` | size of this `~PSP` file |
| `0x30` | boot entry point |
| `0x34` | module info offset |
| `0x38` | `.bss` size |
| `0x44` | segment addresses (4) |
| `0x54` | segment sizes (4) |
| `0x78` | devkit version |
| `0x7C` | decrypt mode |
| `0x130` | **key tag** — selects the KIRK key set |

The header is plaintext even though the body is not, which is what lets `info`
report a module's identity and decryption requirements before any crypto
exists. The key tag is per-module, not per-disc: WTF's five game-sharing
modules carry five different tags.

## ELF / PRX

Once decrypted, a module is an ordinary ELF32 MIPS little-endian file. Games
ship as `ET_EXEC` (2) or as a relocatable PRX with `e_type == 0xFFA0`.

The parser takes the first `PT_LOAD` segment with `PF_X` set as `.text` — the
extent handed to the decoder. A PRX additionally carries a
`.rodata.sceModuleInfo` section describing its exports and imports; resolving
those against the HLE library is phase 4 work.
