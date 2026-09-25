#!/usr/bin/env bash
# Build a Linux release: the Flatpak bundle and the portable tarball.
#
#   release_linux.sh [--version VERSION] [--jobs N] [--skip-build]
#                    [--no-flatpak] [--no-tarball]
#
# From a checkout and the game data a source build uses (EBOOT.ELF and
# disc.iso in profiles/mhp3rd/game, see prepare_game.sh), this builds the
# executable, all overlay libraries and the bundled SDL3 and FFmpeg inside the
# Steam Runtime 3 "sniper" SDK container (packaging/linux/build_in_sdk.sh),
# packs both artifacts, checks that neither contains any game data, and
# prints their SHA-256 checksums. Everything lands in out/release-linux; the
# artifacts in out/release-linux/dist.
#
# Needs podman (or docker), flatpak with the Flathub remote, curl, ostree and
# about 25 GB of free space. The first run takes an hour or more; later runs
# reuse the compiler cache, the dependencies and the finished overlays.
#
#   --version VERSION  name the artifacts after VERSION instead of git describe
#   --jobs N           parallel compile jobs (default 4)
#   --skip-build       package the staged build from a previous run as it is
#   --no-flatpak       do not build the Flatpak bundle
#   --no-tarball       do not build the tarball
set -euo pipefail

profile_dir="$(cd "$(dirname "$0")/.." && pwd)"
repo_dir="$(cd "$profile_dir/../.." && pwd)"
packaging="$profile_dir/packaging/linux"
# shellcheck source=../packaging/linux/sources.sh
source "$packaging/sources.sh"

app_id=io.github.teamgdb.Yakumo
work="${YAKUMO_WORK:-$repo_dir/out/release-linux}"
dist="$work/dist"
stage="$work/stage/yakumo"
expected_executable_sha256=55c0598436c0753b04331f8e95d406f832d9217806e3a896fed0e88b33637d8c

version=""
jobs=4
skip_build=0
make_flatpak=1
make_tarball=1
while [[ $# -gt 0 ]]; do
    case "$1" in
        --version) version="${2:?--version needs a value}"; shift 2 ;;
        --jobs) jobs="${2:?--jobs needs a value}"; shift 2 ;;
        --skip-build) skip_build=1; shift ;;
        --no-flatpak) make_flatpak=0; shift ;;
        --no-tarball) make_tarball=0; shift ;;
        -h|--help) sed -n '2,/^set -euo/p' "$0" | sed '$d; s/^# \{0,1\}//'; exit 0 ;;
        *) echo "unknown option $1" >&2; exit 2 ;;
    esac
done
if [[ -z "$version" ]]; then
    version="$(git -C "$repo_dir" describe --tags --always --dirty)"
    version="${version#v}"
fi
name="yakumo-$version-linux-x86_64"
export SOURCE_DATE_EPOCH="$(git -C "$repo_dir" log -1 --format=%ct)"

step() { printf '\n=== %s\n' "$*"; }
fail() { echo "error: $*" >&2; exit 1; }

sha256() { sha256sum "$1" | cut -d' ' -f1; }

# The FFmpeg the build bundles, as pinned in cmake/FFmpeg.cmake.
ffmpeg_cmake="$profile_dir/cmake/FFmpeg.cmake"
cmake_value() { sed -n "s/^set($1 \(.*\))\$/\1/p" "$ffmpeg_cmake" | head -1; }
FFMPEG_VERSION="$(cmake_value MHP3RD_FFMPEG_VERSION)"
FFMPEG_URL="https://ffmpeg.org/releases/ffmpeg-$FFMPEG_VERSION.tar.xz"
FFMPEG_FLAGS="$(sed -n '/^set(MHP3RD_FFMPEG_CONFIGURE_FLAGS/,/)/p' "$ffmpeg_cmake" |
    sed 's/^set(MHP3RD_FFMPEG_CONFIGURE_FLAGS//; s/)$//' | tr -s ' \n' ' ' | sed 's/^ //; s/ $//')"
[[ -n "$FFMPEG_VERSION" && -n "$FFMPEG_FLAGS" ]] || fail "cannot read the FFmpeg pins from $ffmpeg_cmake"
grep -qF "\"$FFMPEG_URL\"" "$ffmpeg_cmake" ||
    grep -qF 'https://ffmpeg.org/releases/ffmpeg-${MHP3RD_FFMPEG_VERSION}.tar.xz' "$ffmpeg_cmake" ||
    fail "unexpected FFmpeg source URL in $ffmpeg_cmake"

# The notices must describe exactly what is bundled.
notices="$profile_dir/packaging/THIRD_PARTY_NOTICES.md"
for pinned in "SDL3 $SDL3_VERSION" "FFmpeg $FFMPEG_VERSION" "$FFMPEG_URL" "$SDL3_URL" \
              "./configure --prefix=<prefix> $FFMPEG_FLAGS"; do
    grep -qF -- "$pinned" "$notices" || fail "THIRD_PARTY_NOTICES.md does not mention: $pinned"
done

grep -qF "runtime-version: '$FLATPAK_RUNTIME_VERSION'" "$packaging/$app_id.yml" ||
    fail "the Flatpak manifest does not use runtime $FLATPAK_RUNTIME_VERSION from sources.sh"

# ---------------------------------------------------------------------------
# The game data never leaves this machine. Every artifact is checked by file
# name and by content before it is published.
# ---------------------------------------------------------------------------
check_no_game_data() {
    local root="$1" what="$2" bad=()
    while IFS= read -r -d '' file; do
        local base lower
        base="$(basename "$file")"
        lower="${base,,}"
        case "$lower" in
            eboot*|*.iso|*.cso|*.pbp|*.prx|*.elf|*.ovl|*.bin|data.bin|param.sfo|umd_data*|ms0|savedata|ulj*|npjb*)
                bad+=("$file (name)"); continue ;;
        esac
        [[ -f "$file" && ! -L "$file" ]] || continue
        local head
        head="$(head -c 20 "$file" | od -An -tx1 | tr -d ' \n')"
        case "$head" in
            7f454c46*)
                # ELF: e_machine at offset 18, little endian. 8 is MIPS, the PSP.
                [[ "${head:36:4}" == "0800" ]] && bad+=("$file (PSP executable)") ;;
            00504250*) bad+=("$file (PBP)") ;;
            7e505350*) bad+=("$file (encrypted PSP module)") ;;
            00505346*) bad+=("$file (PARAM.SFO)") ;;
        esac
        if [[ "$(stat -c %s "$file")" -gt 32774 ]] &&
           [[ "$(dd if="$file" bs=1 skip=32769 count=5 2>/dev/null | tr -d '\0')" == "CD001" ]]; then
            bad+=("$file (disc image)")
        fi
    done < <(find "$root" -print0)
    if [[ ${#bad[@]} -gt 0 ]]; then
        printf 'error: %s contains game data:\n' "$what" >&2
        printf '  %s\n' "${bad[@]}" >&2
        exit 1
    fi
    echo "no game data in $what"
}

# ---------------------------------------------------------------------------
step "Yakumo $version for Linux"
if [[ -n "$(git -C "$repo_dir" status --porcelain --untracked-files=no)" ]]; then
    echo "warning: the checkout has uncommitted changes; the version says -dirty" >&2
fi

if [[ $skip_build -eq 0 ]]; then
    game_dir="$profile_dir/game"
    [[ -e "$game_dir/EBOOT.ELF" && -e "$game_dir/disc.iso" ]] ||
        fail "$game_dir needs EBOOT.ELF and disc.iso; run scripts/prepare_game.sh first"
    [[ "$(sha256 "$game_dir/EBOOT.ELF")" == "$expected_executable_sha256" ]] ||
        fail "$game_dir/EBOOT.ELF is not the supported executable"

    if command -v podman > /dev/null; then container=podman
    elif command -v docker > /dev/null; then container=docker
    else fail "podman or docker is needed to run the build SDK"; fi

    # The checkout and the work directory appear at the same paths inside the
    # container. Game files linked from elsewhere are mounted read-only where
    # their links point, so the links resolve there too.
    mounts=(-v "$repo_dir:$repo_dir")
    [[ "$work" == "$repo_dir"/* ]] || mounts+=(-v "$work:$work")
    for file in EBOOT.ELF disc.iso; do
        link="$game_dir/$file"
        while [[ -L "$link" ]]; do
            target="$(readlink "$link")"
            [[ "$target" == /* ]] || target="$(dirname "$link")/$target"
            mounts+=(-v "$(readlink -f "$target"):$target:ro")
            link="$target"
        done
    done
    user_args=()
    [[ "$container" == podman ]] && user_args=(--userns=keep-id) || user_args=(--user "$(id -u):$(id -g)")

    step "Building in $SDK_IMAGE"
    mkdir -p "$work/home"
    "$container" run --rm "${user_args[@]}" --security-opt label=disable \
        "${mounts[@]}" -w "$repo_dir" \
        -e HOME="$work/home" -e YAKUMO_WORK="$work" -e YAKUMO_JOBS="$jobs" \
        "$SDK_IMAGE" bash "$packaging/build_in_sdk.sh"
fi

[[ -x "$stage/Yakumo" ]] || fail "nothing staged in $stage; run without --skip-build"
overlay_count="$(find "$stage/overlays" -name '*.so' | wc -l)"
[[ "$overlay_count" -eq 355 ]] || fail "expected 355 overlay libraries, found $overlay_count"
check_no_game_data "$stage" "the staged build"
rm -rf "$dist"
mkdir -p "$dist"

# ---------------------------------------------------------------------------
if [[ $make_tarball -eq 1 ]]; then
    step "Tarball"
    tree="$work/tarball/$name"
    rm -rf "$work/tarball"
    mkdir -p "$tree"
    cp -a "$stage/." "$tree/"
    install -m 755 "$packaging/yakumo.sh" "$tree/yakumo"
    install -m 644 "$packaging/README.txt" "$tree/README.txt"
    tar --sort=name --owner=0 --group=0 --numeric-owner --mtime="@$SOURCE_DATE_EPOCH" \
        -C "$work/tarball" -cf - "$name" | gzip -n -9 > "$dist/$name.tar.gz"
    rm -rf "$work/tarball"

    # Check what was actually packed.
    check_dir="$work/check-tarball"
    rm -rf "$check_dir"
    mkdir -p "$check_dir"
    tar -xzf "$dist/$name.tar.gz" -C "$check_dir"
    check_no_game_data "$check_dir" "$name.tar.gz"
    rm -rf "$check_dir"
fi

# ---------------------------------------------------------------------------
if [[ $make_flatpak -eq 1 ]]; then
    step "Flatpak"
    flatpak --user remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
    flatpak --user install --noninteractive --or-update flathub org.flatpak.Builder \
        "org.freedesktop.Platform//$FLATPAK_RUNTIME_VERSION" "org.freedesktop.Sdk//$FLATPAK_RUNTIME_VERSION"

    source_dir="$work/flatpak/source"
    rm -rf "$source_dir"
    mkdir -p "$source_dir/files"
    cp -al "$stage" "$source_dir/yakumo"
    install -m 755 "$packaging/yakumo.sh" "$source_dir/files/yakumo"
    install -m 644 "$packaging/$app_id.desktop" "$source_dir/files/"
    install -m 644 "$repo_dir/docs/images/emblem.svg" "$source_dir/files/$app_id.svg"
    sed -e "s/@VERSION@/$version/" -e "s/@DATE@/$(date -u -d "@$SOURCE_DATE_EPOCH" +%Y-%m-%d)/" \
        "$packaging/$app_id.metainfo.xml" > "$source_dir/files/$app_id.metainfo.xml"
    cp "$packaging/$app_id.yml" "$source_dir/"

    rm -rf "$work/flatpak/repo"
    flatpak run --filesystem="$work" org.flatpak.Builder \
        --user --force-clean --disable-rofiles-fuse --default-branch=stable \
        --state-dir="$work/flatpak/state" --repo="$work/flatpak/repo" \
        "$work/flatpak/build" "$source_dir/$app_id.yml"
    flatpak build-bundle --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo \
        "$work/flatpak/repo" "$dist/$name.flatpak" "$app_id" stable

    # Check the committed tree, which is what the bundle carries.
    check_dir="$work/check-flatpak"
    rm -rf "$check_dir"
    ostree --repo="$work/flatpak/repo" checkout --user-mode "app/$app_id/x86_64/stable" "$check_dir"
    check_no_game_data "$check_dir" "$name.flatpak"
    rm -rf "$check_dir" "$work/flatpak/build"
fi

# ---------------------------------------------------------------------------
step "Checksums"
# The LGPL source of the FFmpeg the artifacts contain goes on the same release
# page (see THIRD_PARTY_NOTICES.md).
cp "$work/sources/ffmpeg-$FFMPEG_VERSION.tar.xz" "$dist/" ||
    fail "the FFmpeg source archive is not in $work/sources"
(
    cd "$dist"
    shopt -s nullglob
    sha256sum ./*.tar.gz ./*.flatpak ./*.tar.xz | sed 's# \./# #' > SHA256SUMS
    cat SHA256SUMS
)
{
    echo "Yakumo $version for Linux (x86-64)"
    echo "Built from: $(git -C "$repo_dir" rev-parse HEAD)"
    echo "Build environment: $SDK_IMAGE"
    echo "Flatpak runtime: org.freedesktop.Platform//$FLATPAK_RUNTIME_VERSION"
    echo "Needs glibc: $(sed 's/GLIBC_//' "$work/stage/glibc-floor.txt") or newer"
    echo "SDL3 $SDL3_VERSION, FFmpeg $FFMPEG_VERSION"
    echo "FFmpeg configured with: ./configure --prefix=<prefix> $FFMPEG_FLAGS"
} > "$dist/BUILDINFO.txt"
ls -l "$dist"
