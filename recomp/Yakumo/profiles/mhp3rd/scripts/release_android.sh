#!/usr/bin/env bash
# Package an Android release: a signed APK for arm64-v8a phones.
#
#   release_android.sh <build-dir> [--version VERSION] [--jobs N]
#
# From an Android build directory whose overlays are already built (the NDK
# toolchain, -DMHP3RD_ANDROID_APP=ON; docs/RELEASING.md has the commands),
# this switches it to a release build (MHP3RD_RELEASE, the pinned SDL3,
# Android 10 as the oldest system), rebuilds libmain.so if that changes it,
# copies the 355 overlay libraries as they are, packs and signs the APK with
# the release key, checks that it holds no game data and that its libraries
# import nothing Android 10 lacks, and prints its size and SHA-256. Everything lands in
# out/release-android; the artifacts in out/release-android/dist.
#
# Needs ANDROID_HOME (the SDK, with build-tools and platform 35), the NDK the
# build directory was configured with, CMake, Ninja and curl. The release key comes from KEYSTORE, KEYSTORE_PASS
# and KEY_ALIAS, or from the file KEY_ENV (default: keys/key.env in the
# checkout) that sets them. It is never in the repository; see RELEASING.md.
#
#   --version VERSION  name the APK after VERSION instead of git describe
#   --jobs N           parallel compile jobs for libmain.so (default 2)
set -euo pipefail

profile_dir="$(cd "$(dirname "$0")/.." && pwd)"
repo_dir="$(cd "$profile_dir/../.." && pwd)"
# The pinned SDL3 and font, shared with the Linux release.
# shellcheck source=../packaging/linux/sources.sh
source "$profile_dir/packaging/linux/sources.sh"

build_dir=""
version=""
jobs=2
while [[ $# -gt 0 ]]; do
    case "$1" in
        --version) version="${2:?--version needs a value}"; shift 2 ;;
        --jobs) jobs="${2:?--jobs needs a value}"; shift 2 ;;
        -h|--help) sed -n '2,/^set -euo/p' "$0" | sed '$d; s/^# \{0,1\}//'; exit 0 ;;
        -*) echo "unknown option $1" >&2; exit 2 ;;
        *) build_dir="$1"; shift ;;
    esac
done
[[ -n "$build_dir" ]] || { echo "usage: release_android.sh <build-dir> [--version VERSION]" >&2; exit 2; }
build_dir="$(cd "$build_dir" && pwd)"
cache="$build_dir/CMakeCache.txt"
cached() { sed -n "s/^$1:[A-Z]*=//p" "$cache" | head -1 || true; }
[[ -f "$cache" ]] || { echo "error: $build_dir is not a CMake build directory" >&2; exit 1; }
[[ "$(cached MHP3RD_ANDROID_APP)" == ON ]] ||
    { echo "error: $build_dir is not an Android app build (-DMHP3RD_ANDROID_APP=ON)" >&2; exit 1; }
[[ "$(cached ANDROID_ABI)" == arm64-v8a ]] || { echo "error: the build is not for arm64-v8a" >&2; exit 1; }
: "${ANDROID_HOME:?set ANDROID_HOME to the Android SDK}"
toolchain="$(cached CMAKE_TOOLCHAIN_FILE)"
ndk="$(cd "$(dirname "$toolchain")/../.." && pwd)"
# The oldest Android the APK installs on (build_apk.sh's minSdkVersion):
# libmain.so and SDL3 are compiled for it, so the NDK leaves out what newer
# systems added. Overlay libraries compiled for a newer platform are kept;
# they call only the host and plain libc, which the import check confirms.
min_api=29
platform="android-$min_api"

key_env="${KEY_ENV:-$repo_dir/keys/key.env}"
if [[ -z "${KEYSTORE:-}" && -f "$key_env" ]]; then
    set -a
    # shellcheck disable=SC1090
    source "$key_env"
    set +a
fi
[[ -n "${KEYSTORE:-}" && -f "$KEYSTORE" ]] ||
    { echo "error: no release key: set KEYSTORE, KEYSTORE_PASS and KEY_ALIAS, or KEY_ENV" >&2; exit 1; }

if [[ -z "$version" ]]; then
    version="$(git -C "$repo_dir" describe --tags --always --dirty)"
    version="${version#v}"
fi
# The code Android compares: the number of commits, which only grows along
# the history releases are made from.
version_code="$(git -C "$repo_dir" rev-list --count HEAD)"
name="yakumo-$version-android-arm64"

work="${YAKUMO_WORK:-$repo_dir/out/release-android}"
dist="$work/dist"
sources="$work/sources"
deps="$work/deps"
mkdir -p "$dist" "$sources" "$deps"
step() { printf '\n== %s\n' "$*"; }
sha256() { if command -v sha256sum > /dev/null; then sha256sum "$1"; else shasum -a 256 "$1"; fi | cut -d' ' -f1; }

fetch() {
    local target="$sources/$1"
    if [[ -f "$target" && "$(sha256 "$target")" == "$3" ]]; then return; fi
    curl -fsSL -o "$target.part" "$2"
    [[ "$(sha256 "$target.part")" == "$3" ]] || { echo "error: $2 does not match its pinned SHA-256" >&2; exit 1; }
    mv "$target.part" "$target"
}

step "Fetching the pinned SDL3 and font"
fetch "SDL3-$SDL3_VERSION.tar.gz" "$SDL3_URL" "$SDL3_SHA256"
fetch NotoSansCJKjp-Regular.otf "$NOTO_CJK_URL" "$NOTO_CJK_SHA256"
fetch NotoSansCJK-LICENSE.txt "$NOTO_CJK_LICENSE_URL" "$NOTO_CJK_LICENSE_SHA256"

step "SDL3 $SDL3_VERSION for Android"
sdl_source="$deps/SDL3-$SDL3_VERSION"
sdl_install="$deps/sdl3-$SDL3_VERSION-android"
stamp="$SDL3_VERSION $platform $ndk"
if [[ ! -f "$sdl_install/.stamp" || "$(cat "$sdl_install/.stamp")" != "$stamp" ]]; then
    rm -rf "$sdl_source" "$deps/sdl3-build" "$sdl_install"
    tar -xzf "$sources/SDL3-$SDL3_VERSION.tar.gz" -C "$deps"
    cmake -S "$sdl_source" -B "$deps/sdl3-build" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
        -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM="$platform" -DCMAKE_BUILD_TYPE=Release \
        -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF \
        -DCMAKE_INSTALL_PREFIX="$sdl_install" > "$deps/sdl3-configure.log"
    cmake --build "$deps/sdl3-build" -j "$jobs" > "$deps/sdl3-build.log"
    cmake --install "$deps/sdl3-build" > /dev/null
    echo "$stamp" > "$sdl_install/.stamp"
fi

step "libmain.so for release"
cmake -S "$repo_dir" -B "$build_dir" -DMHP3RD_RELEASE=ON -DANDROID_PLATFORM="$platform" \
    -DSDL3_DIR="$sdl_install/lib/cmake/SDL3" -DCMAKE_FIND_ROOT_PATH="$sdl_install" > "$work/configure.log"
cmake --build "$build_dir" --target Yakumo -j "$jobs"

step "Checking the overlay libraries"
# They are copied as they are, so they must all be there and newer than the
# framework headers they were compiled against.
expected="$(ninja -C "$build_dir" -t targets all | grep -c '^overlay_[^:]*: phony')"
built="$(find "$build_dir/bin/overlays" -name 'libovl*.so' | wc -l | tr -d ' ')"
if [[ "$built" -ne "$expected" ]]; then
    echo "error: $built of $expected overlay libraries in $build_dir/bin/overlays; build them first" \
        "(cmake --build $build_dir)" >&2
    exit 1
fi
# (head closes the pipe early, which pipefail would take for a failure.)
oldest="$(ls -tr "$build_dir"/bin/overlays/libovl*.so | head -1 || true)"
newer_header="$(find "$repo_dir/include/psprecomp" -type f -newer "$oldest" | head -1 || true)"
if [[ -n "$newer_header" ]]; then
    echo "error: $newer_header changed after the overlay libraries were built; rebuild them" >&2
    exit 1
fi
echo "$built overlay libraries"

step "Packing $name.apk"
apk="$dist/$name.apk"
rm -f "$apk" "$apk.idsig"
SDL_LIB="$sdl_install/lib/libSDL3.so"
VERSION_NAME="$version" VERSION_CODE="$version_code" FONT_DIR="$sources" ANDROID_NDK="$ndk" \
    "$profile_dir/packaging/android/build_apk.sh" "$build_dir" "$sdl_source" "$SDL_LIB" "$apk"
rm -f "$apk.idsig"

step "Checking the APK"
build_tools="$(ls -d "$ANDROID_HOME"/build-tools/* | sort -V | tail -1)"
"$build_tools/apksigner" verify "$apk"
if unzip -l "$apk" | grep -Ei '\.(iso|cso|elf|pbp|prx)$|EBOOT|DATA\.BIN|PARAM\.SFO|SAVEDATA'; then
    echo "error: the APK holds game data" >&2
    exit 1
fi
"$build_tools/aapt2" dump badging "$apk" | grep -E "^package:|^minSdkVersion|^application-label:|^native-code"
# Every function a packed library imports must be in Android $min_api's system
# libraries or in another packed one; a missing one stops the app loading
# there. Weak imports may be missing: their callers check them first.
llvm="$ndk/toolchains/llvm/prebuilt/$(ls "$ndk/toolchains/llvm/prebuilt" | head -1)"
stubs="$llvm/sysroot/usr/lib/aarch64-linux-android/$min_api"
packed="$build_dir/apk/lib/arm64-v8a"
exports="$work/exports.txt"
{
    for system in libc libm libdl liblog libandroid libvulkan libOpenSLES libGLESv1_CM libGLESv2 libEGL; do
        "$llvm/bin/llvm-nm" -D --defined-only "$stubs/$system.so"
    done
    "$llvm/bin/llvm-nm" -D --defined-only "$packed"/*.so
} 2> /dev/null | awk 'NF >= 3 { sub(/@.*/, "", $3); print $3 }' | sort -u > "$exports"
missing="$("$llvm/bin/llvm-nm" -D --undefined-only "$packed"/*.so | awk '$1 == "U" { sub(/@.*/, "", $2); print $2 }' |
    sort -u | comm -23 - "$exports")"
if [[ -n "$missing" ]]; then
    echo "error: the libraries import what Android $min_api lacks:" $missing >&2
    exit 1
fi
echo "every import is in Android $min_api"
(cd "$dist" && for f in *.apk; do echo "$(sha256 "$f")  $f"; done > SHA256SUMS)
size="$(wc -c < "$apk" | tr -d ' ')"
echo
echo "$apk"
echo "  $size bytes, SHA-256 $(sha256 "$apk")"
