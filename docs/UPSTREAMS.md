# P3P3DS — Upstream Vendored Repositories & Provenance Registry

This registry records the exact provenance, upstream source URLs, imported commit revisions, branches, licenses, and architectural purposes of all 25 vendored research and reference repositories included in the **P3P3DS** repository tree.

All original `LICENSE`, `COPYING`, `NOTICE`, and copyright headers inside each vendored directory are preserved unchanged.

---

## Vendored Repositories Summary

| Local Path | Upstream Repository URL | Imported Revision (Commit SHA) | Branch | License | Stated Purpose in P3P3DS |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `3ds/3ds-examples` | `https://github.com/devkitPro/3ds-examples` | `be2001fee08cf8ec7d3366e095e83f6f75da8942` | `master` | CC0-1.0 / Public Domain | Nintendo 3DS homebrew sample code (GPU rendering, threading, audio streaming) |
| `3ds/citro2d` | `https://github.com/devkitPro/citro2d` | `147b02aae021da61b1f620446ad2892ecc45411e` | `master` | Zlib | 2D graphics hardware-accelerated drawing library on Nintendo 3DS |
| `3ds/citro3d` | `https://github.com/devkitPro/citro3d` | `9f21cf7b380ce6f9e01a0420f19f0763e5443ca7` | `master` | Zlib | 3D graphics library for Nintendo 3DS DMP PICA200 GPU |
| `3ds/libctru` | `https://github.com/devkitPro/libctru` | `36fe1ada5b7ebe53ba4decda36d764a55f8fefb6` | `master` | Zlib | Core Nintendo 3DS userland OS, hardware primitives, threading, and NDSP library |
| `p3p/p3p-patches` | `https://github.com/zarroboogs/p3p-patches` | `e21f8d711fd4c89be1ded6f347d0a17df1db941f` | `master` | Unspecified / Public | Persona 3 Portable CWCheat patches, intro-skip, and mod loader memory hooks |
| `p3p/Persona-3-Portable-Mod-Menu` | `https://github.com/DniweTamp/Persona-3-Portable-Mod-Menu` | `d8e9d55d8240214c6529e839f3021b8c6b0b79a9` | `master` | Public Domain / Open | Persona 3 Portable event script mod menu & field hook mappings (`.flow`/`.bf`) |
| `psp/ghidra-allegrex` | `https://github.com/kotcrab/ghidra-allegrex` | `8d5ca58e609ad7e732d550036126b6b0f99bd663` | `master` | Apache-2.0 | Ghidra processor definition for MIPS Allegrex and VFPU vector instructions |
| `psp/prxtool` | `https://github.com/pspdev/prxtool` | `08849d87fa8692f2ba29238ec1710f2706e3e021` | `master` | AFL-2.0 | PSP PRX/ELF disassembler, symbol resolver, and relocation analyzer |
| `psp/pspsdk` | `https://github.com/pspdev/pspsdk` | `c1d856c1ff21b2098a4ea2aab9894d890b321040` | `master` | BSD-3-Clause | Official open-source Sony PSP SDK headers, structs, and NID tables |
| `psp/vfpu-docs` | `https://github.com/pspdev/vfpu-docs` | `f283bbb8d2ed39b6fa3414bd9f62632143b63e06` | `master` | GPL-3.0 | Sony PSP Vector Floating Point Unit (VFPU) reference documentation |
| `recomp/N64Recomp` | `https://github.com/N64Recomp/N64Recomp` | `ffb39cdad1da5de07eaaa48bd1db4a89a7986771` | `main` | MIT | MIPS-III static recompiler framework and basic-block analysis reference |
| `recomp/PSP-recompilation-project` | `https://github.com/sal063/PSP-recompilation-project` | `da17b0e1db209206a407d097d132201e516e3855` | `main` | GPL-2.0+ | Standalone PSP static recompiler with lightweight pure C runtime (sal063) |
| `recomp/PSPRecomp` | `https://github.com/jessicanataliagta/PSPRecomp` | `f6e7d415c7f447b934cc3865a31eb725f353d659` | `main` | MIT | PSP static recompilation framework with GTA VCS profile architecture |
| `recomp/psprecomp-sp00nz` | `https://github.com/sp00nznet/psprecomp` | `caca7595251410ae7887aa209ba56397a835d0e1` | `main` | MIT | Clean-room static recompilation toolkit and verification oracle design |
| `recomp/Yakumo` | `https://github.com/TeamGDB/Yakumo` | `87bc4d9f5ad61829652ffb07bc8e477d39f706d5` | `main` | MIT | Production static recompilation port of Monster Hunter Portable 3rd HD |
| `references/DaedalusX64-3DS` | `https://github.com/MasterFeizz/DaedalusX64-3DS` | `31c5e560d4cbd11d6e5b50e5e42c25b9598a68a9` | `master` | GPL-2.0 | Reference for MIPS dynarec on ARM11, citro3d PICA200 renderer, and NDSP audio |
| `references/ppsspp` | `https://github.com/hrydgard/ppsspp` | `a50fb6071f4b5ee71f3cd04b65d1cf4cbba22518` | `master` | GPL-2.0+ | Gold-standard PSP emulator: ground truth oracle for HLE, GE, VFPU, audio |
| `references/pspautotests` | `https://github.com/hrydgard/pspautotests` | `6f03ee6457804144add92bfc47563cf60c443e4f` | `master` | BSD-3-Clause | Comprehensive test suite for PSP CPU, VFPU, GE, and kernel timing |
| `references/uofw` | `https://github.com/uofw/uofw` | `5e192e75a83d043d5a65db21128d62758e5741f5` | `master` | GPL-3.0 | Reverse-engineered Sony PSP firmware modules in C (ThreadMan, SysMem, Io) |
| `tools/AemulusModManager` | `https://github.com/TekkaGB/AemulusModManager` | `fe5ec67cfa45b6f0de10c2121652c2eaa332b0ac` | `master` | GPL-3.0 | Package manager for Persona 3/4/5 mod merging and priority virtualization |
| `tools/Amicitia` | `https://github.com/tge-was-taken/Amicitia` | `6b6312af992d27cc8b45122480d5bb1b23aa4202` | `master` | MIT | GUI asset editor for Atlus container and model formats (PAC, BIN, GIM, SPR) |
| `tools/Atlus-Script-Tools` | `https://github.com/tge-was-taken/Atlus-Script-Tools` | `1c557a092dd7610829b7215be0f0f5101acf1d72` | `master` | GPL-3.0 | AtlusScriptCompiler suite for compiling and decompiling `.flow`/`.msg` scripts |
| `tools/AtlusFileSystemLibrary` | `https://github.com/tge-was-taken/AtlusFileSystemLibrary` | `d70da989be2d82179b58b81c104b46778f409c91` | `master` | GPL-2.0 | Programmatic C# library for reading/writing Atlus CPK, PAC, BIN, GIM formats |
| `tools/CriFsV2Lib` | `https://github.com/Sewer56/CriFsV2Lib` | `169b001c748dfffc28c9fc14fcec269dd45e6eec` | `master` | LGPL-3.0+ | High-speed CriWare CPK archive parser and virtual filesystem library |
| `tools/CriPakTools` | `https://github.com/esperknight/CriPakTools` | `0dbd2229f7c3b519c67d65e89055fd4e610d6dca` | `master` | Public Domain | CLI utility for extracting and repacking CPK archives with CRILAYLA compression |
