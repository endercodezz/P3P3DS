#!/usr/bin/env bash
set -euo pipefail

# Reproduce P3P executable analysis pipeline
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "=== 1. Building PSPRecomp Framework and Tools ==="
cmake -S recomp/PSPRecomp -B recomp/PSPRecomp/out/framework -G Ninja -DCMAKE_BUILD_TYPE=Release -DPSPRECOMP_PROFILE="" -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++"
cmake --build recomp/PSPRecomp/out/framework

echo "=== 2. Running Framework Regression Tests ==="
ctest --test-dir recomp/PSPRecomp/out/framework --output-on-failure

echo "=== 3. Running PSPRecomp Program Analyzer ==="
if [ ! -f "profiles/p3p/game/eboot.elf" ]; then
    echo "Error: profiles/p3p/game/eboot.elf not found. Please provide decrypted ELF."
    exit 1
fi
./recomp/PSPRecomp/out/framework/psp_analyze.exe profiles/p3p/game/eboot.elf experiments/p3p-analysis/p3p_report.json 0x08804000

echo "=== 4. Running Independent ELF/PRX Analyzer ==="
python experiments/p3p-analysis/analyze_elf.py profiles/p3p/game/eboot.elf experiments/p3p-analysis/independent_analysis.json

echo "=== 5. Running Minimal Codegen Verification ==="
./recomp/PSPRecomp/out/framework/psp_recomp.exe profiles/p3p/game/eboot.elf experiments/p3p-analysis/test_functions.csv experiments/p3p-analysis/test_generated.cpp
g++ -std=c++20 -O2 -c -Irecomp/PSPRecomp/include experiments/p3p-analysis/test_generated.cpp -o experiments/p3p-analysis/test_generated.o
rm -f experiments/p3p-analysis/test_generated.o

echo "=== Analysis and Verification Completed Successfully ==="
