#!/usr/bin/env bash
# Analyze the decrypted executable and regenerate the AOT corpus.
#
#   generate.sh [build_dir]
#
# Requires a framework build (psp_analyze/psp_recomp) in build_dir, default
# out/mhp3rd. Output: profiles/mhp3rd/analysis and profiles/mhp3rd/generated.
set -euo pipefail

profile_dir="$(cd "$(dirname "$0")/.." && pwd)"
repo_dir="$(cd "$profile_dir/../.." && pwd)"
build_dir="${1:-$repo_dir/out/mhp3rd}"
elf="$profile_dir/game/EBOOT.ELF"

if [[ ! -f "$elf" ]]; then
    echo "missing $elf; run scripts/prepare_game.sh first" >&2
    exit 1
fi

cmake --build "$build_dir" --target psp_analyze psp_recomp
mkdir -p "$profile_dir/analysis"
"$build_dir/psp_analyze" "$elf" "$profile_dir/analysis/report.json"
# Regenerate in place: psp_recomp rewrites only units whose text changed and
# removes units that no longer exist, so unchanged units keep their timestamps
# and are not recompiled.
"$build_dir/psp_recomp" "$elf" --auto "$profile_dir/generated"
