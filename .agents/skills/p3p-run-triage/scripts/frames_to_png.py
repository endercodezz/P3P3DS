#!/usr/bin/env python3
"""Convert frame BMPs written by --frames-dir to PNG (viewable with the Read
tool) and list where the picture changes.

Usage: frames_to_png.py <frames_dir> [frame numbers...]
  no numbers: print the first file of every run of identical frames
  numbers:    write frame_NNNNN.png next to each frame_NNNNN.bmp
Standard library only.
"""
import hashlib
import os
import struct
import sys
import zlib


def bmp_to_png(src: str, dst: str) -> None:
    b = open(src, "rb").read()
    w, h = struct.unpack("<ii", b[18:26])
    row = (w * 3 + 3) & ~3
    raw = bytearray()
    for y in range(h):
        r = b[54 + (h - 1 - y) * row: 54 + (h - 1 - y) * row + w * 3]
        raw.append(0)
        for i in range(0, len(r), 3):
            raw += bytes((r[i + 2], r[i + 1], r[i]))

    def chunk(tag: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data))

    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(bytes(raw))) + chunk(b"IEND", b""))
    open(dst, "wb").write(png)


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    folder = sys.argv[1]
    if len(sys.argv) > 2:
        for n in sys.argv[2:]:
            src = os.path.join(folder, f"frame_{int(n):05d}.bmp")
            dst = src[:-4] + ".png"
            bmp_to_png(src, dst)
            print(dst)
        return 0
    prev = None
    for name in sorted(f for f in os.listdir(folder) if f.endswith(".bmp")):
        digest = hashlib.md5(open(os.path.join(folder, name), "rb").read()).hexdigest()[:8]
        if digest != prev:
            print(name, digest)
        prev = digest
    return 0


if __name__ == "__main__":
    sys.exit(main())
