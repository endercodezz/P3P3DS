# Persona 3 Portable (ULUS-10512) Profile for P3P3DS

This profile defines configuration, function maps, NID bindings, and HLE integration for *Shin Megami Tensei: Persona 3 Portable* (`ULUS-10512`).

## Directory Structure

```text
profiles/p3p/
  README.md          Profile overview
  config/
    p3p_ulus10512.toml Profile configuration
    p3p_functions.csv  Function map and code ranges for AOT codegen
  game/              User-supplied game data (gitignored)
    eboot.elf        Decrypted executable (ULUS-10512)
    USRDIR/          Game data directory (contains data.cpk)
```

## Function Map Terminology (`p3p_functions.csv`)

- **`module_start` (at `0x08804108`):** Verified executable runtime entry point (`[VERIFIED]`).
- **Code Range (`size = 0x108`):** Represents the smoke-test recompilation code range covering initialization, SDK version parameters, and the dispatch branch. The exact terminal function boundary is currently `[INFERRED]`.
- **Heuristic Seeds vs. Function Boundaries:** The automated analyzer finds ~21,965 seeds in the executable, but seed addresses are not verified function boundaries. P3P3DS employs an execution-driven discovery approach, adding code ranges as they are verified through runtime control flow.

## Security & Provenance

No commercial executables, decrypted EBOOTs, disc images, or copyrighted assets are stored in this repository. Executables must be supplied by the user from their legally owned copy.

Place the decrypted `EBOOT.BIN` as `profiles/p3p/game/eboot.elf`.

## Canonical Build & Execution Workflow

To generate recompiled units and run the PC bootstrap harness:

```bash
# Configure and build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target p3p_pc_bootstrap

# Run execution milestone verification test
./build/p3p_pc_bootstrap.exe --verify-milestone
```
