#!/usr/bin/env bash
# Run the game, recompile whatever overlay it stops on, rebuild, repeat.
#
#   bootstrap_overlays.sh [iterations] [build_dir]
#
# Each round: Yakumo runs until it needs an overlay that has no corpus and
# dumps it; the dump is recompiled and linked in, and the next round gets
# further. Stops when a round adds no new overlay.
set -euo pipefail

profile_dir="$(cd "$(dirname "$0")/.." && pwd)"
repo_dir="$(cd "$profile_dir/../.." && pwd)"
iterations="${1:-10}"
build_dir="${2:-$repo_dir/out/mhp3rd}"
dump_dir="${MHP3RD_DUMP_OVERLAYS:-$profile_dir/analysis/overlays}"
log_dir="$profile_dir/analysis"
mkdir -p "$dump_dir" "$log_dir"

for round in $(seq 1 "$iterations"); do
    echo "=== round $round: running"
    set +e
    MHP3RD_DUMP_OVERLAYS="$dump_dir" "$build_dir/bin/Yakumo" > "$log_dir/run_round$round.log" 2>&1
    set -e
    tail -1 "$log_dir/run_round$round.log"

    dump=$(grep -o "recompile it with: .* [0-9a-fA-Fx]*$" "$log_dir/run_round$round.log" | tail -1 || true)
    if [[ -z "$dump" ]]; then
        echo "=== no new overlay; stopping"
        break
    fi
    path=$(echo "$dump" | awk '{print $(NF-1)}')
    base=$(echo "$dump" | awk '{print $NF}')
    echo "=== adding overlay $base from $path"
    python3 "$profile_dir/tools/add_overlay.py" "$build_dir" "$path" "$base" | tail -2

    cmake --build "$build_dir" --target Yakumo -j 3 > "$log_dir/build_round$round.log" 2>&1 ||
        { echo "build failed; see $log_dir/build_round$round.log"; exit 1; }
done
