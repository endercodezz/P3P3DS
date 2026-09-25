#!/usr/bin/env bash
# Extract every code overlay from DATA.BIN and build a recompiled library for each.
#
#   build_overlays.sh [build_dir] [jobs]
#
# The game loads its overlays into shared slots at run time, so each one needs
# its own corpus. This builds all of them up front instead of waiting for the
# game to reach them. First every overlay without a library is recompiled to
# C++, then all of them are built in one Ninja run with `jobs` parallel jobs
# (default 2), so one scheduler and one limit govern the whole build. Overlays
# that already have a library are skipped, and recompiled overlays are not
# recompiled again, so an interrupted run picks up where it stopped. Expect
# roughly 40 minutes for the whole set.
set -euo pipefail

profile_dir="$(cd "$(dirname "$0")/.." && pwd)"
repo_dir="$(cd "$profile_dir/../.." && pwd)"
build_dir="${1:-$repo_dir/out/mhp3rd}"
jobs="${2:-2}"
iso="$profile_dir/game/disc.iso"
extract_dir="$profile_dir/analysis/overlays"
library_dir="$build_dir/bin/overlays"

if [[ ! -e "$iso" ]]; then
    echo "error: $iso not found; run scripts/prepare_game.sh first" >&2
    exit 1
fi
if [[ ! -x "$build_dir/bin/Yakumo" ]]; then
    echo "error: $build_dir/bin/Yakumo not found; build the profile first" >&2
    exit 1
fi

mkdir -p "$extract_dir"
echo "extracting overlays from DATA.BIN into $extract_dir"
python3 "$profile_dir/tools/databin.py" "$iso" extract-overlays "$extract_dir" > /dev/null

images=("$extract_dir"/overlay_*.bin)
total=${#images[@]}
targets=()
skipped=0
failed=()

# Step 1: recompile every overlay that has no library yet. A corpus whose
# meta.txt exists is complete (add_overlay.py writes it last) and only needs
# building.
for image in "${images[@]}"; do
    stem="$(basename "$image" .bin)"   # overlay_<BASE>_<name>
    rest="${stem#overlay_}"
    base="${rest%%_*}"
    name="${rest#*_}"
    if compgen -G "$library_dir/ovl${base}_${name}_*" > /dev/null; then
        skipped=$((skipped + 1))
        continue
    fi
    if ! compgen -G "$profile_dir/overlays/ovl${base}_${name}_*/meta.txt" > /dev/null; then
        echo "[$((${#targets[@]} + skipped + ${#failed[@]} + 1))/$total] recompiling $name at 0x$base"
        if ! python3 "$profile_dir/tools/add_overlay.py" --no-build "$build_dir" "$image" "0x$base" > /dev/null 2>&1; then
            failed+=("$name")
            continue
        fi
    fi
    for meta in "$profile_dir/overlays/ovl${base}_${name}_"*/meta.txt; do
        targets+=("overlay_$(basename "$(dirname "$meta")")")
    done
done

# Step 2: build them all in one invocation. -k 0 keeps going past a failed
# overlay; the libraries that are still missing afterwards are reported.
built=0
if [[ ${#targets[@]} -gt 0 ]]; then
    echo "building ${#targets[@]} overlay libraries with -j $jobs"
    cmake --build "$build_dir" -j "$jobs" --target "${targets[@]}" -- -k 0 || true
    for target in "${targets[@]}"; do
        if compgen -G "$library_dir/${target#overlay_}.*" > /dev/null; then
            built=$((built + 1))
        else
            failed+=("${target#overlay_}")
        fi
    done
fi

echo "built $built, already present $skipped, failed ${#failed[@]}, of $total"
if [[ ${#failed[@]} -gt 0 ]]; then
    printf '  failed: %s\n' "${failed[@]}"
    exit 1
fi
