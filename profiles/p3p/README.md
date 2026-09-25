# Persona 3 Portable (ULUS-10512) Profile for P3P3DS

This profile defines configuration, function maps, NID bindings, and HLE integration for *Shin Megami Tensei: Persona 3 Portable* (`ULUS-10512`).

## Directory Structure

```text
profiles/p3p/
  README.md          Profile overview
  config/
    p3p_ulus10512.toml Profile configuration
    p3p_functions.csv  Verified function entry points and sizes for AOT codegen
  game/              User-supplied game data (gitignored)
    eboot.elf        Decrypted executable (ULUS-10512)
    USRDIR/          Game data directory (contains data.cpk)
```

## Security & Provenance

No commercial executables, decrypted EBOOTs, disc images, or copyrighted assets are stored in this repository. Executables must be supplied by the user from their legally owned copy.

Place the decrypted `EBOOT.BIN` as `profiles/p3p/game/eboot.elf`.

## Canonical Build & Execution Workflow

To generate recompiled units and run the PC bootstrap harness:

```bash
# Configure and build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target p3p_pc_bootstrap

# Run execution smoke test
./build/p3p_pc_bootstrap.exe
```
