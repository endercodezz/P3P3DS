#!/usr/bin/env bash
# Package a macOS release (Apple Silicon) from a finished build directory:
# Yakumo.app in a disk image, and optionally in a zip archive.
#
#   release_macos.sh [--version VERSION] [--executable FILE] [--jobs N]
#                    [--zip] [--no-dmg] BUILD_DIR
#
# BUILD_DIR is a build of this checkout configured with -DMHP3RD_RELEASE=ON
# whose Yakumo and overlay libraries are built (bin/Yakumo, bin/overlays,
# bin/lib with the bundled FFmpeg). Nothing in it is rebuilt or changed.
#
# The script builds SDL3 and the Vulkan loader from pinned sources for the
# deployment target in packaging/macos/sources.sh, takes MoltenVK from its
# pinned Khronos release, and assembles Yakumo.app with all of them, the
# overlay libraries, the fallback font, the icon and the licenses. Every
# library resolves inside the bundle, the binaries are marked for the
# deployment target once their system imports are checked against that
# macOS's SDK, and everything is signed ad hoc (no Developer ID, no
# notarization, no hardened runtime, so the overlay libraries load). It then
# packs the disk image (and the zip), checks that no artifact contains game
# data or a path of this machine, and prints their SHA-256 checksums. Everything lands in
# out/release-macos; the artifacts in out/release-macos/dist.
#
# Needs Xcode or the Command Line Tools with the SDK of the deployment target
# (e.g. MacOSX13.1.sdk in /Library/Developer/CommandLineTools/SDKs), CMake,
# Ninja and curl.
#
#   --version VERSION  name the artifacts after VERSION instead of git describe
#   --executable FILE  package FILE, a Yakumo built with -DMHP3RD_RELEASE=ON,
#                      instead of BUILD_DIR/bin/Yakumo (for a build directory
#                      that was switched back to a developer build)
#   --jobs N           parallel compile jobs for SDL3 and the loader (default 4)
#   --zip              also pack Yakumo.app as a zip archive
#   --no-dmg           do not build the disk image
set -euo pipefail
# Byte-wise text tools: the checks read binary files.
export LC_ALL=C

profile_dir="$(cd "$(dirname "$0")/.." && pwd)"
repo_dir="$(cd "$profile_dir/../.." && pwd)"
packaging="$profile_dir/packaging/macos"
# shellcheck source=../packaging/macos/sources.sh
source "$packaging/sources.sh"

work="${YAKUMO_WORK:-$repo_dir/out/release-macos}"
sources="$work/sources"
deps="$work/deps"
deps_build="$work/deps-build"
stage="$work/stage"
dist="$work/dist"
app="$stage/Yakumo.app"

version=""
executable=""
jobs=4
make_dmg=1
make_zip=0
build_dir=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --version) version="${2:?--version needs a value}"; shift 2 ;;
        --executable) executable="${2:?--executable needs a value}"; shift 2 ;;
        --jobs) jobs="${2:?--jobs needs a value}"; shift 2 ;;
        --no-dmg) make_dmg=0; shift ;;
        --zip) make_zip=1; shift ;;
        -h|--help) sed -n '2,/^set -euo/p' "$0" | sed '$d; s/^# \{0,1\}//'; exit 0 ;;
        -*) echo "unknown option $1" >&2; exit 2 ;;
        *) [[ -z "$build_dir" ]] || { echo "only one build directory" >&2; exit 2; }; build_dir="$1"; shift ;;
    esac
done
[[ -n "$build_dir" ]] || { echo "usage: $0 [options] BUILD_DIR (see --help)" >&2; exit 2; }
build_dir="$(cd "$build_dir" && pwd)"
[[ -n "$executable" ]] || executable="$build_dir/bin/Yakumo"
if [[ -z "$version" ]]; then
    version="$(git -C "$repo_dir" describe --tags --always --dirty)"
    version="${version#v}"
fi
name="yakumo-$version-macos-arm64"
target="$MACOS_DEPLOYMENT_TARGET"

step() { printf '\n=== %s\n' "$*"; }
fail() { echo "error: $*" >&2; exit 1; }
sha256() { shasum -a 256 "$1" | cut -d' ' -f1; }
lower() { printf '%s' "$1" | tr '[:upper:]' '[:lower:]'; }

# fetch <file name> <url> <sha256>
fetch() {
    local file="$sources/$1"
    if [[ -f "$file" && "$(sha256 "$file")" == "$3" ]]; then return; fi
    curl -fsSL -o "$file.part" "$2"
    [[ "$(sha256 "$file.part")" == "$3" ]] || fail "$2 does not match its pinned SHA-256"
    mv "$file.part" "$file"
}

# A dependency is rebuilt when its version or configuration changes.
stamp_matches() { [[ -f "$deps/.$1.stamp" && "$(cat "$deps/.$1.stamp")" == "$2" ]]; }

# The Mach-O files in a tree.
macho_files() {
    find "$1" -type f -print0 | while IFS= read -r -d '' file; do
        [[ "$(head -c 4 "$file" | od -An -tx1 | tr -d ' \n')" == cffaedfe ]] && echo "$file"
    done
}

# The FFmpeg the build bundles, as pinned in cmake/FFmpeg.cmake.
ffmpeg_cmake="$profile_dir/cmake/FFmpeg.cmake"
cmake_value() { sed -n "s/^set($1 \(.*\))\$/\1/p" "$ffmpeg_cmake" | head -1; }
FFMPEG_VERSION="$(cmake_value MHP3RD_FFMPEG_VERSION)"
FFMPEG_URL="https://ffmpeg.org/releases/ffmpeg-$FFMPEG_VERSION.tar.xz"
FFMPEG_SHA256="$(cmake_value MHP3RD_FFMPEG_SHA256)"
FFMPEG_FLAGS="$(sed -n '/^set(MHP3RD_FFMPEG_CONFIGURE_FLAGS/,/)/p' "$ffmpeg_cmake" |
    sed 's/^set(MHP3RD_FFMPEG_CONFIGURE_FLAGS//; s/)$//' | tr -s ' \n' ' ' | sed 's/^ //; s/ $//')"
[[ -n "$FFMPEG_VERSION" && -n "$FFMPEG_FLAGS" ]] || fail "cannot read the FFmpeg pins from $ffmpeg_cmake"

# The notices must describe exactly what is bundled.
notices="$profile_dir/packaging/THIRD_PARTY_NOTICES.md"
for pinned in "SDL3 $SDL3_VERSION" "FFmpeg $FFMPEG_VERSION" "$FFMPEG_URL" "$SDL3_URL" \
              "./configure --prefix=<prefix> $FFMPEG_FLAGS" \
              "MoltenVK $MOLTENVK_VERSION" "$MOLTENVK_URL" "$MOLTENVK_SHA256" \
              "$VULKAN_LOADER_URL" "$VULKAN_LOADER_SHA256"; do
    grep -qF -- "$pinned" "$notices" || fail "THIRD_PARTY_NOTICES.md does not mention: $pinned"
done

# ---------------------------------------------------------------------------
# The game data never leaves this machine. Every artifact is checked by file
# name and by content before it is published.
# ---------------------------------------------------------------------------
check_no_game_data() {
    local root="$1" what="$2" bad=() file base head
    while IFS= read -r -d '' file; do
        base="$(lower "$(basename "$file")")"
        case "$base" in
            eboot*|*.iso|*.cso|*.pbp|*.prx|*.elf|*.ovl|*.bin|data.bin|param.sfo|umd_data*|ms0|savedata|ulj*|npjb*)
                bad+=("$file (name)"); continue ;;
        esac
        [[ -f "$file" && ! -L "$file" ]] || continue
        head="$(head -c 20 "$file" | od -An -tx1 | tr -d ' \n')"
        case "$head" in
            7f454c46*)
                # ELF: e_machine at offset 18, little endian. 8 is MIPS, the PSP.
                [[ "${head:36:4}" == "0800" ]] && bad+=("$file (PSP executable)") ;;
            00504250*) bad+=("$file (PBP)") ;;
            7e505350*) bad+=("$file (encrypted PSP module)") ;;
            00505346*) bad+=("$file (PARAM.SFO)") ;;
        esac
        if [[ "$(stat -f %z "$file")" -gt 32774 ]] &&
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

# Checks that every symbol the Mach-O files import from the system's C and
# C++ libraries and CoreFoundation exists in the SDK of the deployment target,
# so the binaries, built for a newer macOS, still load there. Weak imports are
# optional anyway.
check_system_imports() {
    local sdk="$1"; shift
    local exported="$work/sdk-exports.txt" imports="$work/imports.txt"
    find "$sdk/usr/lib" -maxdepth 1 -name 'libc++*.tbd' -o -maxdepth 1 -name 'libSystem.B.tbd' |
        cat - <(find "$sdk/usr/lib/system" -name '*.tbd') \
              <(find "$sdk/System/Library/Frameworks/CoreFoundation.framework" -name '*.tbd') |
        xargs cat | grep -oE "[A-Za-z_][A-Za-z0-9_\$.]*" | sort -u > "$exported"
    : > "$imports"
    local file
    for file in "$@"; do
        dyld_info -imports "$file" |
            awk -v f="$(basename "$file")" '
                /\(from (libc\+\+|libc\+\+abi|libSystem|CoreFoundation)\)/ && !/weak-import/ {
                    # The .tbd files list Objective-C classes by their bare names.
                    symbol = $2
                    sub(/^_OBJC_(CLASS|METACLASS|EHTYPE)_\$_/, "", symbol)
                    print symbol, f
                }' >> "$imports"
    done
    local missing
    missing="$(sort -u "$imports" | awk 'NR == FNR { have[$1] = 1; next } !($1 in have)' "$exported" - || true)"
    if [[ -n "$missing" ]]; then
        echo "error: symbols missing from macOS $target (symbol, first binary):" >&2
        echo "$missing" | sort -u -k1,1 | head -50 >&2
        exit 1
    fi
    echo "all $(cut -d' ' -f1 "$imports" | sort -u | wc -l | tr -d ' ') system imports exist in $(basename "$sdk")"
}

# ---------------------------------------------------------------------------
step "Yakumo $version for macOS (arm64, macOS $target or newer)"
if [[ -n "$(git -C "$repo_dir" status --porcelain --untracked-files=no)" ]]; then
    echo "warning: the checkout has uncommitted changes; the version says -dirty" >&2
fi
[[ "$(uname -s)" == Darwin ]] || fail "run this on macOS"

[[ -x "$executable" ]] || fail "no Yakumo at $executable"
lipo -archs "$executable" | grep -qx arm64 || fail "$executable is not an arm64 executable"
# A developer build falls back to its checkout's game directory; a release
# must not depend on the machine it was built on.
if strings -a "$executable" | grep -q '/profiles/mhp3rd/game$'; then
    fail "$executable is a developer build; configure with -DMHP3RD_RELEASE=ON, build Yakumo and pass it with --executable"
fi
overlay_count="$(find "$build_dir/bin/overlays" -name '*.dylib' | wc -l | tr -d ' ')"
[[ "$overlay_count" -eq 355 ]] || fail "expected 355 overlay libraries in $build_dir/bin/overlays, found $overlay_count"
for file in "libavcodec.61.dylib" "libavutil.59.dylib" "FFmpeg-COPYING.LGPLv2.1.txt" "FFmpeg-SOURCE.txt"; do
    [[ -e "$build_dir/bin/lib/$file" ]] || fail "$build_dir/bin/lib/$file is missing; configure with -DMHP3RD_FFMPEG=bundled"
done
grep -qF -- "$FFMPEG_FLAGS" "$build_dir/bin/lib/FFmpeg-SOURCE.txt" ||
    fail "the build's FFmpeg was not configured as cmake/FFmpeg.cmake pins it"
# Overlays newer than the executable may have been linked against another one.
newer="$(find "$build_dir/bin/overlays" -name '*.dylib' -newer "$executable" | head -1)"
[[ -z "$newer" ]] || echo "note: overlays such as $(basename "$newer") are newer than the executable"

sdk_dir="${YAKUMO_CHECK_SDK:-}"
if [[ -z "$sdk_dir" ]]; then
    for candidate in /Library/Developer/CommandLineTools/SDKs/MacOSX"${target%%.*}".*.sdk \
                     "$(xcode-select -p)"/Platforms/MacOSX.platform/Developer/SDKs/MacOSX"${target%%.*}".*.sdk; do
        [[ -d "$candidate" ]] && sdk_dir="$candidate"
    done
fi
[[ -d "$sdk_dir" ]] ||
    fail "no macOS ${target%%.*} SDK to check the imports against; install one or set YAKUMO_CHECK_SDK"

mkdir -p "$sources" "$deps" "$deps_build"

# ---------------------------------------------------------------------------
step "Fetching pinned sources"
fetch "SDL3-$SDL3_VERSION.tar.gz" "$SDL3_URL" "$SDL3_SHA256"
fetch "Vulkan-Headers-$VULKAN_SDK_TAG.tar.gz" "$VULKAN_HEADERS_URL" "$VULKAN_HEADERS_SHA256"
fetch "Vulkan-Loader-$VULKAN_SDK_TAG.tar.gz" "$VULKAN_LOADER_URL" "$VULKAN_LOADER_SHA256"
fetch "MoltenVK-macos-$MOLTENVK_VERSION.tar" "$MOLTENVK_URL" "$MOLTENVK_SHA256"
fetch NotoSansCJKjp-Regular.otf "$NOTO_CJK_URL" "$NOTO_CJK_SHA256"
fetch NotoSansCJK-LICENSE.txt "$NOTO_CJK_LICENSE_URL" "$NOTO_CJK_LICENSE_SHA256"
fetch "ffmpeg-$FFMPEG_VERSION.tar.xz" "$FFMPEG_URL" "$FFMPEG_SHA256"

cmake_common=(-G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$deps" -DCMAKE_INSTALL_LIBDIR=lib
              -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET="$target"
              -DCMAKE_INSTALL_NAME_DIR=@rpath)

sdl_stamp="$SDL3_VERSION $SDL3_SHA256 $target"
if ! stamp_matches sdl3 "$sdl_stamp"; then
    step "Building SDL3 $SDL3_VERSION"
    rm -rf "$deps_build/SDL3-$SDL3_VERSION" "$deps_build/sdl3-build"
    tar -xzf "$sources/SDL3-$SDL3_VERSION.tar.gz" -C "$deps_build"
    cmake -S "$deps_build/SDL3-$SDL3_VERSION" -B "$deps_build/sdl3-build" "${cmake_common[@]}" \
        -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
    cmake --build "$deps_build/sdl3-build" -j "$jobs"
    cmake --install "$deps_build/sdl3-build"
    echo "$sdl_stamp" > "$deps/.sdl3.stamp"
fi

loader_stamp="$VULKAN_SDK_TAG $VULKAN_LOADER_SHA256 $VULKAN_HEADERS_SHA256 $target sysconfdir=/etc"
if ! stamp_matches vulkan-loader "$loader_stamp"; then
    step "Building the Vulkan loader ($VULKAN_SDK_TAG)"
    rm -rf "$deps_build/Vulkan-Headers-$VULKAN_SDK_TAG" "$deps_build/Vulkan-Loader-$VULKAN_SDK_TAG" \
        "$deps_build/vulkan-headers-build" "$deps_build/vulkan-loader-build"
    tar -xzf "$sources/Vulkan-Headers-$VULKAN_SDK_TAG.tar.gz" -C "$deps_build"
    tar -xzf "$sources/Vulkan-Loader-$VULKAN_SDK_TAG.tar.gz" -C "$deps_build"
    cmake -S "$deps_build/Vulkan-Headers-$VULKAN_SDK_TAG" -B "$deps_build/vulkan-headers-build" "${cmake_common[@]}"
    cmake --install "$deps_build/vulkan-headers-build"
    # SYSCONFDIR: search /etc like any system loader, not this work directory.
    cmake -S "$deps_build/Vulkan-Loader-$VULKAN_SDK_TAG" -B "$deps_build/vulkan-loader-build" "${cmake_common[@]}" \
        -DVULKAN_HEADERS_INSTALL_DIR="$deps" -DBUILD_TESTS=OFF -DUPDATE_DEPS=OFF -DSYSCONFDIR=/etc
    cmake --build "$deps_build/vulkan-loader-build" -j "$jobs"
    cmake --install "$deps_build/vulkan-loader-build"
    echo "$loader_stamp" > "$deps/.vulkan-loader.stamp"
fi

moltenvk_stamp="$MOLTENVK_VERSION $MOLTENVK_SHA256"
if ! stamp_matches moltenvk "$moltenvk_stamp"; then
    step "Unpacking MoltenVK $MOLTENVK_VERSION"
    rm -rf "$deps_build/moltenvk"
    mkdir -p "$deps_build/moltenvk"
    tar -xf "$sources/MoltenVK-macos-$MOLTENVK_VERSION.tar" -C "$deps_build/moltenvk"
    dylib="$(find "$deps_build/moltenvk" -path '*dynamic*macOS*' -name libMoltenVK.dylib | head -1)"
    [[ -n "$dylib" ]] || dylib="$(find "$deps_build/moltenvk" -path '*macOS*' -name libMoltenVK.dylib | head -1)"
    [[ -n "$dylib" ]] || fail "no macOS libMoltenVK.dylib in the MoltenVK release"
    lipo "$dylib" -thin arm64 -output "$deps/lib/libMoltenVK.dylib" 2>/dev/null || cp "$dylib" "$deps/lib/libMoltenVK.dylib"
    cp "$(find "$deps_build/moltenvk" -name LICENSE | head -1)" "$deps/MoltenVK-LICENSE.txt"
    echo "$moltenvk_stamp" > "$deps/.moltenvk.stamp"
fi

# ---------------------------------------------------------------------------
step "Assembling Yakumo.app"
rm -rf "$stage" "$dist"
mkdir -p "$app/Contents/MacOS" "$app/Contents/Frameworks/overlays" \
    "$app/Contents/Resources/fonts" "$app/Contents/Resources/licenses" \
    "$app/Contents/Resources/vulkan/icd.d" "$dist"
contents="$app/Contents"
frameworks="$contents/Frameworks"
resources="$contents/Resources"

install -m 755 "$executable" "$contents/MacOS/Yakumo"
cp "$build_dir/bin/overlays/"*.dylib "$frameworks/overlays/"
for lib in libavcodec.61.dylib libavutil.59.dylib; do
    cp "$build_dir/bin/lib/$lib" "$frameworks/"
done
cp -L "$deps/lib/libSDL3.0.dylib" "$deps/lib/libvulkan.1.dylib" "$deps/lib/libMoltenVK.dylib" "$frameworks/"
chmod 644 "$frameworks/"*.dylib "$frameworks/overlays/"*.dylib

# The loader looks for driver manifests in the bundle's Resources first; the
# path in it is relative to the manifest.
cat > "$resources/vulkan/icd.d/MoltenVK_icd.json" <<'JSON'
{
    "file_format_version": "1.0.0",
    "ICD": {
        "library_path": "../../../Frameworks/libMoltenVK.dylib",
        "api_version": "1.4.0",
        "is_portability_driver": true
    }
}
JSON

short_version="$(printf '%s' "$version" | grep -oE '^[0-9]+(\.[0-9]+){0,2}' || true)"
[[ -n "$short_version" ]] || short_version=0.0.0
commits="$(printf '%s' "$version" | sed -nE 's/^[0-9.]+-([0-9]+)-g.*/\1/p')"
bundle_version="$short_version${commits:+.$commits}"
sed -e "s/@VERSION@/$version/g" -e "s/@SHORT_VERSION@/$short_version/" \
    -e "s/@BUNDLE_VERSION@/$bundle_version/" -e "s/@MINIMUM_SYSTEM_VERSION@/$target/" \
    "$packaging/Info.plist.in" > "$contents/Info.plist"
plutil -lint "$contents/Info.plist" > /dev/null
printf 'APPL????' > "$contents/PkgInfo"
cp "$packaging/Yakumo.icns" "$resources/Yakumo.icns"

cp "$sources/NotoSansCJKjp-Regular.otf" "$resources/fonts/"
licenses="$resources/licenses"
cp "$repo_dir/LICENSE" "$licenses/Yakumo-LICENSE.txt"
cp "$notices" "$licenses/THIRD_PARTY_NOTICES.md"
cp "$deps_build/SDL3-$SDL3_VERSION/LICENSE.txt" "$licenses/SDL3-LICENSE.txt"
cp "$deps_build/Vulkan-Loader-$VULKAN_SDK_TAG/LICENSE.txt" "$licenses/Vulkan-Loader-LICENSE.txt"
cp "$deps/MoltenVK-LICENSE.txt" "$licenses/MoltenVK-LICENSE.txt"
cp "$build_dir/bin/lib/FFmpeg-COPYING.LGPLv2.1.txt" "$build_dir/bin/lib/FFmpeg-SOURCE.txt" "$licenses/"
cp "$profile_dir/third_party/imgui/LICENSE.txt" "$licenses/DearImGui-LICENSE.txt"
cp "$profile_dir/third_party/tiny_aes/UNLICENSE" "$licenses/tiny-AES-c-UNLICENSE.txt"
cp "$profile_dir/third_party/xxhash/LICENSE" "$licenses/xxHash-LICENSE.txt"
cp "$sources/NotoSansCJK-LICENSE.txt" "$licenses/NotoSansCJK-OFL.txt"

# ---------------------------------------------------------------------------
step "Linking inside the bundle"
exe="$contents/MacOS/Yakumo"
# Every library the executable loads by its bundled file name through
# @rpath, which is Contents/Frameworks and nothing else.
while read -r dependency; do
    base="$(basename "$dependency")"
    [[ -e "$frameworks/$base" ]] || continue
    [[ "$dependency" == "@rpath/$base" ]] || install_name_tool -change "$dependency" "@rpath/$base" "$exe" 2> /dev/null
done < <(otool -L "$exe" | tail -n +2 | awk '{ print $1 }')
while read -r rpath; do
    install_name_tool -delete_rpath "$rpath" "$exe" 2> /dev/null
done < <(otool -l "$exe" | awk '$1 == "cmd" && $2 == "LC_RPATH" { getline; getline; print $2 }')
install_name_tool -add_rpath @executable_path/../Frameworks "$exe" 2> /dev/null
for lib in "$frameworks/"*.dylib; do
    install_name_tool -id "@rpath/$(basename "$lib")" "$lib" 2> /dev/null
    while read -r rpath; do
        install_name_tool -delete_rpath "$rpath" "$lib" 2> /dev/null
    done < <(otool -l "$lib" | awk '$1 == "cmd" && $2 == "LC_RPATH" { getline; getline; print $2 }')
done

# Symbols the executable exports to the overlay libraries stay; only local
# symbols and debug information go.
strip -x "$exe" "$frameworks/"*.dylib "$frameworks/overlays/"*.dylib 2> /dev/null

# ---------------------------------------------------------------------------
step "Deployment target"
minos_of() { otool -l "$1" | awk '$1 == "minos" { print $2; exit }'; }
# 1 if version $1 is newer than the target.
newer_than_target() { [[ -n "$1" && "$(printf '%s\n%s\n' "$1" "$target" | sort -V | tail -1)" != "$target" ]]; }
machos=()
retarget=()
while IFS= read -r file; do
    machos+=("$file")
    lipo -archs "$file" | grep -qx arm64 || fail "$file is not arm64"
    newer_than_target "$(minos_of "$file")" && retarget+=("$file")
done < <(macho_files "$app")
# What the build made on this Mac (the executable, the overlays, FFmpeg) is
# marked for this Mac's macOS. SDL3, the loader and MoltenVK are built for the
# target already.
if [[ ${#retarget[@]} -gt 0 ]]; then
    check_system_imports "$sdk_dir" "${retarget[@]}"
    for file in "${retarget[@]}"; do
        sdk="$(otool -l "$file" | awk '$1 == "sdk" { print $2; exit }')"
        vtool -set-build-version macos "$target" "$sdk" -replace -output "$file.vtool" "$file"
        mv "$file.vtool" "$file"
    done
    echo "marked ${#retarget[@]} binaries for macOS $target"
fi

# ---------------------------------------------------------------------------
step "Signing (ad hoc)"
# Inside out: every library, then the application, which seals Resources.
# No hardened runtime: it would refuse to load the ad hoc signed overlays.
sign() { codesign --force --sign - --timestamp=none "$@" 2>&1 | { grep -v ': replacing existing signature$' || true; }; }
while IFS= read -r -d '' lib; do sign "$lib"; done < <(find "$frameworks" -name '*.dylib' -print0)
sign "$app"

step "Checking the bundle"
codesign --verify --deep --strict "$app"
bad_links="$(for file in "${machos[@]}"; do
    otool -L "$file" | tail -n +2 | awk '{ print $1 }' |
        { grep -vE '^(/usr/lib/|/System/Library/|@rpath/|@executable_path/|@loader_path/)' || true; } |
        sed "s#^#$(basename "$file"): #"
    otool -l "$file" | awk '$1 == "cmd" && $2 == "LC_RPATH" { getline; getline; print $2 }' |
        { grep -vE '^@(executable|loader)_path' || true; } | sed "s#^#$(basename "$file") rpath: #"
done)"
[[ -z "$bad_links" ]] || { echo "error: libraries or rpaths outside the bundle:" >&2; echo "$bad_links" >&2; exit 1; }
for file in "${machos[@]}"; do
    ! newer_than_target "$(minos_of "$file")" || fail "$file needs macOS $(minos_of "$file")"
    [[ "$(lipo -archs "$file")" == arm64 ]] || fail "$file has more than the arm64 architecture"
done
# No path of this machine: the home directory, the user name, the checkout or
# the build directory (FFmpeg's configure line once carried its prefix).
leaks="$(grep -rlaF -e "$HOME" -e "/$(id -un)/" -e "$repo_dir" -e "$build_dir" -e "$work" "$app" || true)"
[[ -z "$leaks" ]] || { echo "error: files in Yakumo.app name paths of this machine:" >&2; echo "$leaks" >&2; exit 1; }
echo "no paths of this machine in Yakumo.app"
check_no_game_data "$app" "Yakumo.app"
echo "Yakumo.app: $(du -sh "$app" | cut -f1), ${#machos[@]} Mach-O files, $overlay_count overlays"

# ---------------------------------------------------------------------------
readme="$stage/Read Me.txt"
sed -e "s/@MINIMUM_SYSTEM_VERSION@/$target/" "$packaging/README.txt" > "$readme"

if [[ $make_zip -eq 1 ]]; then
    step "Zip"
    zip_root="$work/zip/$name"
    rm -rf "$work/zip"
    mkdir -p "$zip_root"
    ditto "$app" "$zip_root/Yakumo.app"
    cp "$readme" "$zip_root/"
    (cd "$work/zip" && ditto -c -k --sequesterRsrc --keepParent "$name" "$dist/$name.zip")
    rm -rf "$work/zip"

    check_dir="$work/check-zip"
    rm -rf "$check_dir"
    mkdir -p "$check_dir"
    ditto -x -k "$dist/$name.zip" "$check_dir"
    codesign --verify --deep --strict "$check_dir/$name/Yakumo.app"
    check_no_game_data "$check_dir" "$name.zip"
    rm -rf "$check_dir"
fi

if [[ $make_dmg -eq 1 ]]; then
    step "Disk image"
    dmg_root="$work/dmg/Yakumo"
    rm -rf "$work/dmg"
    mkdir -p "$dmg_root"
    ditto "$app" "$dmg_root/Yakumo.app"
    cp "$readme" "$dmg_root/"
    ln -s /Applications "$dmg_root/Applications"
    hdiutil create -quiet -volname "Yakumo $version" -srcfolder "$dmg_root" -fs APFS \
        -format ULMO -ov "$dist/$name.dmg"
    rm -rf "$work/dmg"

    mount_point="$work/check-dmg"
    rm -rf "$mount_point"
    mkdir -p "$mount_point"
    hdiutil attach -quiet -readonly -nobrowse -mountpoint "$mount_point" "$dist/$name.dmg"
    status=0
    { codesign --verify --deep --strict "$mount_point/Yakumo.app" &&
      check_no_game_data "$mount_point" "$name.dmg"; } || status=$?
    hdiutil detach -quiet "$mount_point"
    rmdir "$mount_point"
    [[ $status -eq 0 ]] || exit "$status"
fi

# ---------------------------------------------------------------------------
step "Checksums"
# The LGPL source of the FFmpeg the artifacts contain goes on the same release
# page (see THIRD_PARTY_NOTICES.md).
cp "$sources/ffmpeg-$FFMPEG_VERSION.tar.xz" "$dist/"
(
    cd "$dist"
    shopt -s nullglob
    shasum -a 256 ./*.dmg ./*.zip ./*.tar.xz | sed 's# \./# #' > SHA256SUMS
    cat SHA256SUMS
)
{
    echo "Yakumo $version for macOS (Apple Silicon)"
    echo "Built from: $(git -C "$repo_dir" rev-parse HEAD)"
    echo "Needs macOS $target or newer; signed ad hoc, not notarized"
    echo "Build environment: macOS $(sw_vers -productVersion), macOS SDK $(xcrun --show-sdk-version), $(clang --version | head -1)"
    echo "SDL3 $SDL3_VERSION, Vulkan loader $VULKAN_SDK_TAG, MoltenVK $MOLTENVK_VERSION, FFmpeg $FFMPEG_VERSION"
    echo "FFmpeg configured with: ./configure --prefix=<prefix> $FFMPEG_FLAGS"
} > "$dist/BUILDINFO.txt"
ls -l "$dist"
