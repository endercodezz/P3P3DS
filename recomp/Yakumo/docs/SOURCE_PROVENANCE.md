# Source provenance and license boundaries

PSPRecomp separates independently written framework/profile code from third-party components with their own licenses.

## Framework

The reusable PSPRecomp runtime, decoder, analyzer and code generator in the repository root are distributed under the MIT License. Game-specific addresses and implementations are not accepted in the reusable core.

The project may use public hardware documentation, observable program behavior and other implementations as technical references. Reference material is used to understand behavior and architecture; source code from incompatible copyleft projects is not imported into the MIT framework.

## Decryption

The framework contains no EBOOT/PRX decryption. The mhp3rd profile's installer (`profiles/mhp3rd/host/install`) prepares the game's executable from the player's own disc image: it accepts exactly one encrypted file, identified by its SHA-256, and checks its output against the SHA-256 of the executable the profile was generated from. It was written for this project from public descriptions of the file format and the crypto primitives; no code from other implementations was copied or adapted. Its AES implementation is tiny-AES-c (public domain), kept with its notice in `profiles/mhp3rd/third_party/tiny_aes`. Developers can still prepare the executable outside the project and supply it through `profiles/mhp3rd/game`.

## Save data

`profiles/mhp3rd/host/save_data` implements the PSP save-data format — the `PARAM.SFO` layout, the encryption of the data file and the hashes that protect a save — so that saves can be exchanged with a PSP. It was written for this project from public descriptions of the PSP save data format, with no code copied or adapted from other implementations, and checked against saves made by a PSP. Its AES-128 cipher is the same tiny-AES-c the installer uses; the self-tests check it against FIPS-197 and the CMAC built on it against RFC 4493. The fixed key values it uses are published technical constants. None of it decrypts executables.

## Ad hoc networking

`profiles/mhp3rd/host/adhoc` and `profiles/mhp3rd/host/hle/hle_adhoc.cpp` let the game's ad hoc play reach other players through the PSP ad hoc servers players already run. The client was written for this project from the protocols' documented and observed behaviour: the servers' published packet layouts, opcodes and ports, and the traffic between the game and a server. No code was copied or adapted from other implementations. The PSP library calls it serves follow their public API descriptions and the game's own calls, traced while it runs.

The built-in server (`server.cpp`, used by *Host a session* and `--adhoc-server`) was also written for this project from the same protocols' documented and observed behaviour: the packet layouts, opcodes, ports and the order of the messages a server sends, checked against the project's own client. No code, structure or text was copied or adapted from other server implementations. The local network discovery protocol (`discovery.cpp`) is the project's own design.

## Texture packs

`profiles/mhp3rd/host/gpu/texture_pack.cpp` and `replacement_textures.cpp` load HD texture packs made for PPSSPP, in its `textures.ini` format. They were written for this project. The format was learnt from PPSSPP's public documentation and pull requests about texture replacement, from reading PPSSPP's GPL-licensed source to find the format's facts (the layout of a key, the hash seeds, how the palette and the texture's dimensions enter the key, the order in which wildcard keys are tried, the ini sections and options), and from the keys a real community pack for this game uses, which the implementation reproduces. Only those facts and constants were taken; no code was copied, adapted or translated line by line. PPSSPP's `quick` hash is not implemented. Hashing uses xxHash, and PNG files are read and written with stb_image and stb_image_write, all unmodified (table below).

## Mods

`profiles/mhp3rd/host/mods/` reads the mod folders of this game's community, made for the mhp3reload mod loader and its mod manager, which are GPL-licensed. It was written for this project from their published documentation and from observed behaviour; no code of either was read for the implementation, copied, adapted or translated. The facts taken, and where from:

- The mod folder layout, the `mod.ini` keys and their meaning (`[MOD INFO]`; `Name`, `Type` with `File`, `Patch`, `Code`, `Pack`, `PseudoPack`, `Equip<type>`, `EquipSET`, `EquipCATSET` and the equipment type keys; `Files` and `Target` as `;` lists; `Version` with `NOHD`, `HD`, `BOTH` and `NOHD` as the default; `FilesHD`, `TargetHD`, `FilesOG`; `Description` with `\` for a new line; `Depends`; `Priority` 0–5, default 3; `ModList`, `SubModList`; `Animation`, `Audio`, `Script`; `Name_<language>`; `preview.png`, 166×166), and that the last 8 bytes of a patch, `FFFFFFFF00000000`, are skipped when it is installed: the mod manager's `README.md` (github.com/Kurogami2134/p3rdml_modman).
- Replacement files named by their file id, in capitals, and patches named by the file id and `P`; the mod file format as blocks of a load address and a length whose top bit means "run as it loads", ended by `FFFFFFFF00000000` in the HD loader; the hook blocks (`j` or `jal` written at an address); that a file cannot be both replaced and patched: the loaders' `Readme.md` files (github.com/Kurogami2134/mhp3reload and mhp3reload_hd).
- That a file id is the `DATA.BIN` entry index in hex, one less than the number in the community's extracted file names: the community file list (github.com/Kurogami2134/MHP3rd-Game-FIle-List, `weapons.md`), checked against this disc's own directory with `profiles/mhp3rd/tools/databin.py`.
- How the game reads `DATA.BIN`, its sizes and its load completion: this project's traces ([DATA_BIN.md](DATA_BIN.md#how-the-game-reads-it)).

`Author` is not a documented key; it is read when present. Equipment mods need the player to type the target file id, because the lists of which file belongs to which piece of equipment are the manager's data and were not copied. The obfuscation code in `mhp3rd_data_bin.cpp` is this project's own, from [DATA_BIN.md](DATA_BIN.md). Preview images are decoded with stb_image, as for texture packs. No mod, and no file of the game, is in the repository; the tests build their own from random data.

## Profile code

A profile owns its generated AOT corpus, address-specific lowering, HLE behavior and native fast paths. Those files remain isolated under `profiles/<id>` so they do not become hidden dependencies of the generic framework.

## Analog camera

`profiles/mhp3rd/host/camera/` was written from this project's own NPJB-40001 executable analysis and run-time traces. The camera caller, structure offsets, 16-bit yaw format and height-filter coefficient are observed facts. Continuous pitch uses an independently written spherical-orbit calculation. No third-party camera-mod code is included or adapted.

## Third-party components

Third-party source, binary dependencies, shader code and notices stay beside the profile that needs them. Their original copyright and license notices must be preserved.

| Component | Used by | License | How it is included |
| --- | --- | --- | --- |
| [Dear ImGui](https://github.com/ocornut/imgui) 1.92.9b | mhp3rd profile: in-game menu and setup screens | MIT | Unmodified copy in `profiles/mhp3rd/third_party/imgui` with its `LICENSE.txt` |
| [tiny-AES-c](https://github.com/kokke/tiny-AES-c) | mhp3rd profile: installer and save data | Unlicense (public domain) | Unmodified copy in `profiles/mhp3rd/third_party/tiny_aes` with its `UNLICENSE` |
| [stb_truetype](https://github.com/nothings/stb) 1.26 | mhp3rd profile: game text | MIT or public domain | Single header, `profiles/mhp3rd/third_party/stb_truetype.h`, notice at its end |
| [stb_image](https://github.com/nothings/stb) 2.30 and stb_image_write 1.16 | mhp3rd profile: texture pack images (PNG only) | MIT or public domain | Single headers from commit `2c980bb`, `profiles/mhp3rd/third_party/stb_image.h` and `stb_image_write.h`, notices at their ends |
| [xxHash](https://github.com/Cyan4973/xxHash) 0.8.3 | mhp3rd profile: texture pack keys | BSD-2-Clause | Unmodified `xxhash.h` of release v0.8.3 in `profiles/mhp3rd/third_party/xxhash` with its `LICENSE` |
| [SDL3](https://www.libsdl.org/) | mhp3rd profile: window, input, audio output | zlib | External dependency, linked dynamically; Linux releases ship an unmodified build in `lib/` |
| [FFmpeg](https://ffmpeg.org/) (`libavcodec`, `libavutil`) | mhp3rd profile: ATRAC3 music and H.264/ATRAC3plus movie decoding (`profiles/mhp3rd/host/audio/atrac_decoder.cpp`, `profiles/mhp3rd/host/movie/avc_decoder.cpp`) | LGPL-2.1-or-later (the Windows build: LGPL-3.0-or-later) | Linked dynamically; no FFmpeg source is in the repository. By default the build downloads the unmodified [FFmpeg 7.1.5 release](https://ffmpeg.org/releases/ffmpeg-7.1.5.tar.xz) (SHA-256 `de668509caf9e35e3cd162473441fdb29538c6d96ed080292b3cf9e6fc5d558f`) and builds it with `--enable-shared --disable-static --disable-programs --disable-doc --disable-avdevice --disable-avformat --disable-avfilter --disable-swscale --disable-swresample --disable-network --disable-autodetect --disable-everything --enable-decoder=atrac3,atrac3p,h264 --disable-x86asm --disable-debug` (`profiles/mhp3rd/cmake/FFmpeg.cmake`). On Windows it uses the unmodified prebuilt [LGPL shared build](https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2026-06-30-13-34/ffmpeg-n7.1.5-1-g7d0e842004-win64-lgpl-shared-7.1.zip) of FFmpeg 7.1.5 (commit `7d0e842004`) from [BtbN/FFmpeg-Builds](https://github.com/BtbN/FFmpeg-Builds). The licence text and a note with the source location and configuration are copied next to the libraries. `-DMHP3RD_FFMPEG=system` uses an FFmpeg found through `pkg-config` instead |
| [Noto Sans CJK JP](https://github.com/notofonts/noto-cjk) | mhp3rd releases: fallback font for Japanese text | SIL Open Font License 1.1 | Downloaded by the release build, shipped in `fonts/`; not in the repository |

Released builds carry these notices in `profiles/mhp3rd/packaging/THIRD_PARTY_NOTICES.md`, together with the license texts; [`RELEASING.md`](RELEASING.md) describes how they are built.

## Contribution rule

Do not paste or adapt source from a project whose license is incompatible with the destination file. Reimplement required behavior from specifications, observations or independently documented semantics, and record the source of third-party material when it is intentionally included under a compatible license.
