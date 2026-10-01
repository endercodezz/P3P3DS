# P3P3DS — Persona 3 Portable Technical Research & Reverse Engineering Notes

This document consolidates all technical reverse engineering data, executable structures, known memory addresses, file formats, and mod loading hooks for **Persona 3 Portable (P3P)** relevant to building the **P3P3DS** runtime.

---

## 1. Game Identification & Versions

| Region | Product Code / Title ID | Internal Disc ID | Notes |
| :--- | :--- | :--- | :--- |
| **North America (US)** | **ULUS-10512** | `ULUS-10512` | **Primary target for P3P3DS.** All existing modding tools (Aemulus), mod menus, English patches, and Russian fan translations (e.g. by The Miracle / DniweTamp) anchor to this release. |
| **Japan (JP)** | **ULJM-05500** | `ULJM-05500` | Original release (Nov 1, 2009). Different EBOOT layout and symbol offsets. |
| **Europe (EU)** | **ULES-01523** | `ULES-01523` | European English release. Similar to US, but memory addresses shift by several kilobytes. |

*Recommendation:* Target **ULUS-10512** exclusively for the initial static recompilation profile (`profiles/p3p_ulus10512`).

---

## 2. Executable Structure (EBOOT)

### 2.1. Extraction & Decryption
Retail PSP UMDs store the game executable in encrypted form at `PSP_GAME/SYSDIR/EBOOT.BIN`.
- Decryption yields a standard relocatable Sony PRX (MIPS ELF32) format.
- Relocatable PRX files can be rebased to the standard PSP user memory base:
  **Virtual Base Address:** `0x08804000`
- The executable contains standard ELF sections:
  - `.text` (Allegrex MIPS machine code)
  - `.rodata` (constants, strings, jump tables, Atlus lookup structures)
  - `.rodata.sceResident` / `.lib.ent` (PSP export stubs)
  - `.lib.stub` / `.rodata.sceNid` (PSP library import stubs: NIDs)
  - `.data` (initialized global variables)
  - `.bss` (uninitialized memory cleared to zero at startup)

### 2.2. Memory Layout
```text
0x00010000 - 0x00013FFF : PSP Scratchpad (16 KiB fast internal RAM)
0x04000000 - 0x041FFFFF : PSP VRAM (2 MiB framebuffers, textures, depth)
0x08800000 - 0x08803FFF : Reserved by PSP Kernel / OS stubs
0x08804000 - 0x08Axxxxx : P3P Executable Image (.text, .rodata, .data, .bss)
0x08Axxxxx - 0x09F00000 : Dynamic Game Heap (User Arena - grows upward)
0x09F00000 - 0x09FFFFFF : Thread Stacks (User stacks grow downward from top)
0x0A000000             : End of 32 MiB User Memory Space
```

---

## 3. Atlus Game Engine Architecture

Persona 3 Portable is built upon Atlus's proprietary PS2 RPG engine (used in *Shin Megami Tensei: Persona 3*, *Persona 3 FES*, and *Persona 4*), heavily adapted for PSP:
1. **Script Engine (Atlus Flow / `.bf`):**
   - The game does **not** compile story events into MIPS code.
   - All cutscenes, social links, NPC interactions, and dungeon events are written in Atlus Script and compiled into `.bf` (Binary Flow) bytecode.
   - The EBOOT contains a virtual machine that interprets `.bf` bytecode.
   - Dialogues are stored in companion `.bmd` (Binary Message Data) files.
2. **Graphics Architecture:**
   - 2D Navigation (Visual Novel style): Background images + 2D sprite portraits (`.spr` / `.tm2` / `.gim`) rendered via 2D textured quad primitives (`PRIM_RECTANGLES`).
   - 3D Tartarus & Battles: 3D character models and environments rendered via standard 3D meshes with bone skinning matrices and vertex lighting.
3. **Sound Architecture:**
   - Music (BGM): Streamed using ATRAC3+ or CRI ADX.
   - Sound Effects (SFX) & Voices: Triggered through `sceSasCore` (Sony software synthesizer) and CRI audio tracks.

---

## 4. File Formats & Asset Containers

| Format / Extension | Description | Tools for Inspection & Extraction |
| :--- | :--- | :--- |
| **`.cpk`** | **CriWare Archive Container.** Main archives on disc: `data.cpk`, `data_install.cpk`. Contains all models, textures, sounds, and scripts. | `tools/CriFsV2Lib`, `tools/CriPakTools` |
| **CRILAYLA** | Proprietary compression algorithm used inside CPK archives for compressed data chunks. | Supported by `CriFsV2Lib` and `CriPakTools` |
| **`.pac` / `.bin`** | Atlus package archives containing sub-assets grouped by dungeon floor, event, or character. | `tools/AtlusFileSystemLibrary`, `tools/Amicitia` |
| **`.bf`** | Compiled binary event flow scripts. Can be decompiled and edited with AtlusScriptCompiler. | `tools/Atlus-Script-Tools` |
| **`.bmd`** | Compiled binary message data (dialogue text, choices, battle prompts). | `tools/Atlus-Script-Tools` |
| **`.tm2` / `.gim`** | Texture formats: TIM2 (PlayStation heritage) and GIM (Sony PSP native texture format). | `tools/Amicitia`, `tools/AtlusFileSystemLibrary` |
| **`.spr`** | Atlus sprite container bundling multiple 2D texture clips with coordinate layout tables. | `tools/Amicitia` |
| **`.adx` / `.at3`** | Audio formats: CRI ADX audio stream or Sony ATRAC3+ audio stream. | FFmpeg (`references/ppsspp/Core/HLE/sceAtrac.cpp`) |
| **`.pmf`** | Sony PSMF movie format (cutscenes/openings) containing MPEG-2 / AVC video and ATRAC3 audio. | FFmpeg / `references/ppsspp/Core/HLE/sceMpeg.cpp` |

---

## 5. Verified Hooks & Memory Addresses (ULUS-10512)

Analysis of `p3p/p3p-patches/ULUS10512.ini` and `p3p/Persona-3-Portable-Mod-Menu` reveals key engine anchor addresses in the US release:

### 5.1. Intro Movies Skip Patch
- Address: `0x08A594A4` (Virtual: `0x002594A4` offset from base `0x08800000`)
- Original MIPS: Checks video playback state machine.
- Patch: `0x24020006` (`li $v0, 6`) — forces engine state immediately into main title screen.

### 5.2. Canonical File Redirection / Mod Loader Hook
In retail P3P, file open calls resolve paths against UMD `disc0:/PSP_GAME/USRDIR/data.cpk`.
The standard P3P Mod Support patch (by *zarroboogs* / modding community) hooks into the CRI file resolver:
- **Base String Hook Address:** `0x08B95EE4` (replaces path prefix with `ms0:/PSP/GAME/P3P/%s`)
- **Mod Directory Search Sequence (Injection at `0x08806DF0` – `0x08807C58`):**
  1. `bind/` (Loose files directory)
  2. `mod.cpk` (Primary user mod archive)
  3. `mod1.cpk` (Secondary mod archive)
  4. `mod2.cpk` (Tertiary mod archive)
  5. `mod3.cpk` (Quaternary mod archive)
  6. Original UMD game data (`data.cpk`)

### 5.3. In-Game Event Script Hooks (Mod Menu)
As demonstrated in `p3p/Persona-3-Portable-Mod-Menu`:
- Mod menus hook into field shortcut triggers (`h06_01.flow` through `h37_02.flow`).
- When the player presses the menu key (Square on PSP / X or Y on 3DS), the event engine intercepts `CallOriginalSquareMenu()` and invokes `ModMenuInit()`.
- This proves that script-level mods function entirely at the asset level without requiring binary modifications!

---

## 6. Architecture for Mods & Russification (Русификатор)

### 6.1. Multi-Tiered Virtual File System (VFS)
In **P3P3DS**, the HLE `IoFileMgr` subsystem will intercept all game file requests (`sceIoOpen`) and implement our priority fallback pipeline natively:

```text
               Game requests: "data/field/pack/f006.bin"
                                 │
                                 ▼
         Does SD:/p3p3ds/mods/bind/data/field/pack/f006.bin exist?
                         ├── YES ──► Open loose file from SD
                         └── NO
                                 │
                                 ▼
             Does f006.bin exist inside SD:/p3p3ds/mods/mod.cpk?
                         ├── YES ──► Stream file from mod.cpk
                         └── NO
                                 │
                                 ▼
             Does f006.bin exist inside SD:/p3p3ds/mods/mod1.cpk?
                         ├── YES ──► Stream file from mod1.cpk
                         └── NO
                                 │
                                 ▼
        Fallback: original archives disc0:/PSP_GAME/USRDIR/umd0.cpk, umd1.cpk
        (ULUS-10512 has no data.cpk; the chain itself is the CWCheat "Mod Support" patch)
```

### 6.2. Russification & Font Handling
Russian fan translations for Persona 3 Portable (such as the translation by The Miracle / DniweTamp):
1. **Text Encoding:** `[UNVERIFIED]` expected to map Cyrillic onto a custom 1-byte table used by the replaced font; the real translation package has not been examined.
2. **Font Textures:** `[INFERRED]` modifies the game font; `umd0.cpk` contains exactly one `.fnt` file (CPK listing via `experiments/cpk-check`).
3. **Dialogue Scripts:** `[INFERRED]` modified `.bf` (1,272 in `umd0.cpk`, flow scripts with embedded messages) and `.bmd` files.
4. **Potential Executable Patches:** May require EBOOT-level binary hooks for glyph spacing, line wrapping, or proportional font widths (`[UNVERIFIED]` - requires empirical test against translated assets).

**Architectural Requirement for P3P3DS:**
The runtime must remain language-agnostic. While dialogue assets and textures resolve cleanly through the VFS (`SD:/p3p3ds/mods/mod.cpk` or `SD:/p3p3ds/mods/bind/`), any necessary executable-level font or spacing patches must be modularly supported via profile hook tables in `profiles/p3p/config/` without hardcoding language-specific logic into the core engine.

### 6.3. Measured: where P3P's visible text lives (2026-10-01)
Search of the decrypted EBOOT for strings seen on the first-day screens:
- **In the EBOOT (`.rodata`):** UI and location labels — "Enter your last name", "Your Room", "School Entranceway", "Main Lobby", "Faculty Office", "Dark Hour", "Early Morning", "Evening", "NEW GAME"; 4,038 printable ASCII strings of 12+ characters in total. Translating them needs EBOOT data patches. `[VERIFIED]`
- **Not in the EBOOT:** dialogue such as "You are tired today..." and "Welcome to the world of P3P." (archive scripts/messages, i.e. a CPK/`bind/` mod). `[VERIFIED]`
- **Neither:** "PRESS ANY BUTTON", "LOAD GAME" — drawn from textures, so a translation replaces images. `[INFERRED]`

Consequence for P3P3DS: assets of a translation work unchanged through the mod chain (`--mods`, VFS verified with a CPK). EBOOT changes split in two kinds: pure data (string bytes, tables) can be applied to guest memory at load because AOT code reads data at run time; any change to instructions (including `lui`/`addiu` pairs that build string addresses, which is how MIPS code references data) must reach the code generator. Today `profiles/p3p/config/cwcheat_patches.txt` patches are applied before AOT generation (`apply_generated_patches`), so a code-patching translation means a rebuild per patch set — unless patched code ranges are routed to the interpreter fallback at run time (interpreter: ~29 M instructions/s on the PC host, measured with `p3p_autotest`; 3DS speed `[UNVERIFIED]`). Which kind the Russian translation needs is `[UNVERIFIED]` until its package is examined.
