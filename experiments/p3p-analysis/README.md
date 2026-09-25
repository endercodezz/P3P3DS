# Experiment 1 — Persona 3 Portable Executable Analysis

This experiment verifies that the PSP static recompilation toolchain (`PSPRecomp`) builds and accurately analyzes the retail *Persona 3 Portable* (`ULUS-10512`) executable.

---

## Artifacts in this Directory

| File | Description |
| :--- | :--- |
| `reproduce_analysis.sh` | Bash script to rebuild tools, run regression tests, and reproduce both analysis passes. |
| `analyze_elf.py` | Independent Python ELF/PRX parser verifying segments, relocations, imports, and `.text` opcodes. |
| `p3p_report.json` | Full analysis report produced by `psp_analyze` (sections, relocations, seeds, imports). |
| `p3p_report_imports.csv` | List of all 221 imported library NID stubs and resolved functions. |
| `p3p_report_function_seeds.csv` | List of all 21,965 discovered function entry points. |
| `p3p_report_functions_auto.csv` | Metrics for all analyzed functions (instructions, basic blocks, indirect call sites). |
| `independent_analysis.json` | Cross-verification results produced by `analyze_elf.py`. |
| `test_functions.csv` | Minimal 3-function test map used to verify AOT C++ code generation. |
| `test_generated.cpp` | Generated C++ source output for the 3 test functions and 221 import wrappers. |

---

## How to Reproduce

```bash
# Ensure local decrypted ELF is in profiles/p3p/game/eboot.elf (ignored by git)
bash experiments/p3p-analysis/reproduce_analysis.sh
```

---

## Security & Provenance Note

All game code, copyrighted assets, and raw binary dumps remain in local gitignored paths (`profiles/p3p/game/`). This directory contains only structural metadata, function metrics, reproduction scripts, and generated wrappers.
