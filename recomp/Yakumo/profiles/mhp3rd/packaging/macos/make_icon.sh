#!/usr/bin/env bash
# Makes Yakumo.icns, the app icon of the macOS release, from the Yakumo emblem
# (docs/images/emblem.svg): the emblem on a rounded square of its own night
# sky, sized to Apple's icon grid (an 824-point square in a 1024 canvas).
#
#   make_icon.sh [OUTPUT.icns]    default: Yakumo.icns next to this script
#
# Needs rsvg-convert (librsvg) and iconutil. The result is committed, like the
# Windows emblem.ico, so a release build does not need librsvg.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
repo_dir="$(cd "$here/../../../.." && pwd)"
output="${1:-$here/Yakumo.icns}"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

# The emblem nested in the icon canvas: its root element becomes an inner
# <svg> placed and scaled by its viewBox.
{
    cat <<'SVG'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1024 1024" width="1024" height="1024">
<defs>
  <linearGradient id="yakumo-icon-night" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#3a2856"/><stop offset="1" stop-color="#1a1029"/>
  </linearGradient>
</defs>
<rect x="100" y="100" width="824" height="824" rx="185" fill="url(#yakumo-icon-night)"/>
SVG
    sed -e '/^<?xml/d' \
        -e 's#^<svg [^>]*>#<svg x="152" y="152" width="720" height="720" viewBox="0 0 512 512">#' \
        "$repo_dir/docs/images/emblem.svg"
    echo '</svg>'
} > "$work/icon.svg"

iconset="$work/Yakumo.iconset"
mkdir "$iconset"
for size in 16 32 128 256 512; do
    rsvg-convert -w "$size" -h "$size" "$work/icon.svg" -o "$iconset/icon_${size}x${size}.png"
    rsvg-convert -w $((size * 2)) -h $((size * 2)) "$work/icon.svg" -o "$iconset/icon_${size}x${size}@2x.png"
done
iconutil -c icns "$iconset" -o "$output"
echo "wrote $output"
