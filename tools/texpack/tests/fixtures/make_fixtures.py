#!/usr/bin/env python3
"""Regenerates the tex3ds reference fixtures for p3p-texpack (synthetic images only).

Usage: python tools/texpack/tests/fixtures/make_fixtures.py <tex3ds executable>

Writes <name>.png and, for every uncompressed format, <name>.<format>.tex:
the output of `tex3ds -r -z none -f <format>` without its 4-byte header.
Standard library only.
"""
import struct
import subprocess
import sys
import zlib
from pathlib import Path

HERE = Path(__file__).resolve().parent
FORMATS = ["rgba8", "rgb8", "rgb565", "rgba5551", "rgba4"]


def write_png(path, width, height, pixel, color_type):
    channels = {0: 1, 2: 3, 4: 2, 6: 4}[color_type]
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            raw.extend(pixel(x, y)[:channels])

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", width, height, 8, color_type, 0, 0, 0)
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b""))


# Every 8-bit value appears in every channel (16x16 = 256 texels).
def all_values(x, y):
    v = y * 16 + x
    return (v, 255 - v, (v * 7) & 255, (v * 13 + 5) & 255)


def pattern(x, y):
    return ((x * 16) & 255, (y * 32) & 255, (x * y * 7) & 255, 255 if (x + y) % 3 else (x * 20) & 255)


IMAGES = {
    "all_values_16x16": (16, 16, all_values, 6),
    "pattern_16x8": (16, 8, pattern, 6),
    "pattern_8x32": (8, 32, pattern, 6),
    "pattern_24x10": (24, 10, pattern, 6),
    "pattern_5x3": (5, 3, pattern, 6),
    "rgb_16x16": (16, 16, all_values, 2),
    "gray_alpha_16x8": (16, 8, lambda x, y: ((x * 17) & 255, (y * 36) & 255), 4),
    "gray_8x8": (8, 8, lambda x, y: ((x * 32 + y * 4) & 255,), 0),
}


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    tex3ds = sys.argv[1]
    for name, (w, h, pixel, color) in IMAGES.items():
        png = HERE / f"{name}.png"
        write_png(png, w, h, pixel, color)
        for fmt in FORMATS:
            out = HERE / f"{name}.{fmt}.tex"
            subprocess.run([tex3ds, "-r", "-z", "none", "-f", fmt, "-o", str(out), str(png)],
                           check=True, capture_output=True)
            data = out.read_bytes()
            if data[0] != 0 or int.from_bytes(data[1:4], "little") != len(data) - 4:
                sys.exit(f"{out}: unexpected tex3ds header {data[:4].hex()}")
            out.write_bytes(data[4:])
            print(f"{out.name}: {len(data) - 4} bytes")


if __name__ == "__main__":
    main()
