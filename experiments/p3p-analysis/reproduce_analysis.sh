#!/usr/bin/env bash
set -euo pipefail

# Reproduce P3P executable analysis & PC bootstrap execution milestone
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "=== 1. Building P3P3DS PC Bootstrap & Tools ==="
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target p3p_pc_bootstrap

echo "=== 2. Running PSPRecomp Program Analyzer ==="
if [ ! -f "profiles/p3p/game/eboot.elf" ]; then
    echo "Error: profiles/p3p/game/eboot.elf not found. Please provide decrypted ELF."
    exit 1
fi
./build/recomp/PSPRecomp/psp_analyze.exe profiles/p3p/game/eboot.elf experiments/p3p-analysis/p3p_report.json 0x08804000

echo "=== 3. Running Independent Relocation-Aware ELF/PRX Analyzer ==="
python experiments/p3p-analysis/analyze_elf.py profiles/p3p/game/eboot.elf experiments/p3p-analysis/independent_analysis.json

echo "=== 4. Executing Recompiled P3P module_start on PC ==="
./build/p3p_pc_bootstrap.exe

echo "=== Milestone Verified: Recompiled P3P Execution Succeeded ==="
