#!/usr/bin/env bash
set -euo pipefail

# Reproduce P3P executable analysis & PC bootstrap execution milestone
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "=== 1. Building P3P3DS PC Bootstrap & Tools ==="
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

echo "=== 2. Running PSPRecomp Program Analyzer ==="
if [ ! -f "profiles/p3p/game/eboot.elf" ]; then
    echo "Error: profiles/p3p/game/eboot.elf not found. Please provide decrypted ELF."
    exit 1
fi
./build/recomp/PSPRecomp/psp_analyze.exe profiles/p3p/game/eboot.elf experiments/p3p-analysis/p3p_report.json 0x08804000

echo "=== 3. Running Independent Relocation-Aware ELF/PRX Analyzer ==="
python experiments/p3p-analysis/analyze_elf.py profiles/p3p/game/eboot.elf experiments/p3p-analysis/independent_analysis.json

echo "=== 4. Automated Deterministic Cross-Verification of Analyzers ==="
python experiments/p3p-analysis/compare_analysis.py experiments/p3p-analysis/p3p_report.json experiments/p3p-analysis/independent_analysis.json experiments/p3p-analysis/p3p_report_imports.csv

echo "=== 5. Running Decoder Compatibility & Consistency Test ==="
./build/verify_p3p_decoder.exe profiles/p3p/game/eboot.elf

echo "=== 6. Executing Recompiled P3P module_start Milestone Verification ==="
./build/p3p_pc_bootstrap.exe --verify-milestone

echo "=== 7. Running Full CTest Suite ==="
ctest --test-dir build --output-on-failure

echo "=== All Verification Steps Passed Successfully ==="
