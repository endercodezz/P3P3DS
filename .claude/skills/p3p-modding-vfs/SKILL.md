---
name: p3p-modding-vfs
description: Guide Persona 3 Portable asset extraction, CriWare CPK parsing, Atlus script/message patching, runtime mod loading, and Russian fan-translation integration.
---

# P3P Modding & Virtual File System (VFS) Workflow

Use this skill when:
- Designing or modifying the Virtual File System (VFS) for asset loading;
- Implementing the multi-tier mod override pipeline on SDMC;
- Integrating the Russian fan translation (русификатор) or community mod packages;
- Handling Atlus-specific formats (CPK, PAC/BIN, BMD, BF, GIM, TM2, SPR);
- Applying game-specific EBOOT patches or event script hooks.

---

## 1. Mod Type Categorization

Do **not** assume all mods are purely asset replacements. For every modification, identify its exact mechanism:

1. **Loose Asset Override:** Raw replaced files (`.bin`, `.tm2`, `.gim`, `.bmd`).
2. **CPK Archive Override:** Packed archive containing replaced files (`mod.cpk`, `mod1.cpk`).
3. **Event Script Modification:** Compiled `.bf` bytecode hooking into flow logic (e.g. `p3p/Persona-3-Portable-Mod-Menu/hook/`).
4. **Dialogue / Text Patch:** Compiled `.bmd` message data with modified string tables.
5. **Font & Glyph Patch:** Replaced bitmap font textures (`font.fnt`, `.gim`) and character code tables.
6. **Binary / EBOOT Patch:** Code or data modifications applied to the executable (e.g. CWCheat patches in `p3p/p3p-patches/ULUS10512.ini`).

---

## 2. Key References

- `p3p/p3p-patches/ULUS10512.ini` — Verified memory offsets for intro skip and mod loader string pointers.
- `p3p/Persona-3-Portable-Mod-Menu/` — Field script hooks (`.flow` / `.bf`) and menu initialization.
- `tools/CriFsV2Lib/` — Modern C# library for reading and parsing CriWare CPK archives.
- `tools/CriPakTools/` — C# CLI tool for unpacking and repacking CPK files with CRILAYLA compression.
- `tools/AtlusFileSystemLibrary/` — Comprehensive library for Atlus PAC, BIN, GIM, and TMX containers.
- `tools/Atlus-Script-Tools/` — AtlusScriptCompiler for compiling `.flow` and `.msg` into `.bf` and `.bmd`.
- `tools/Amicitia/` — GUI asset inspector for 3D models, textures, and sprites.

---

## 3. VFS Priority Fallback Pipeline

The HLE `IoFileMgr` subsystem must resolve all game path requests through this strict priority order:

```text
Game calls: sceIoOpen("disc0:/PSP_GAME/USRDIR/data.cpk") or sub-file
    │
    ▼
1. Check: <root>/mods/bind/<relative_path>
    ├── Exists? ──► Open loose file directly
    └── Not found
    │
    ▼
2. Check: <root>/mods/mod.cpk
    ├── Contains file? ──► Stream file chunk from mod.cpk
    └── Not found
    │
    ▼
3. Check: <root>/mods/mod1.cpk ... modN.cpk (sequential priority)
    ├── Contains file? ──► Stream file chunk from modN.cpk
    └── Not found
    │
    ▼
4. Fallback: <root>/data/data.cpk (Original game archive)
    ├── Contains file? ──► Stream from base game archive
    └── Not found ──► Return SCE_KERNEL_ERROR_ERRNO_FILE_NOT_FOUND (-0x80010002)
```

**Filesystem Portability Rule:**
- *Never hardcode absolute paths* (e.g. `D:/...` or `sdmc:/...`) in generic VFS logic.
- Use a root prefix provider (e.g. `vfs_mount_root("./game_data")` on PC vs `vfs_mount_root("sdmc:/p3p3ds")` on 3DS).

---

## 4. Russification (Русификатор) Verification Checklist

When verifying or integrating the Russian fan translation:
- **Archive Check:** Is the translation packaged as a `mod.cpk` or as loose files?
- **Font Texture Format:** Verify if the Cyrillic font is mapped via a modified 1-byte character table (CP1251) or a custom Atlus glyph layout.
- **Dialogue Scripts:** Ensure dialogue files (`.bmd`) load without truncation or parsing errors.
- **EBOOT Patches:** Check if the translation requires any text-width or character-spacing code patches (document in `profiles/p3p/config/`).

---

## 5. Expected Output Format

When analyzing or integrating a mod:

```text
Mod Name:       <Name of mod or translation>
Target Files:   <List of affected files/archives>
Type:           [Asset Override | Script Hook | Font Replacement | EBOOT Patch]
VFS Layer:      <bind/ | mod.cpk | memory patch>
Tools Used:     <CriFsV2Lib | AtlusScriptCompiler | Amicitia>
Status:         [VERIFIED | UNVERIFIED]
```
