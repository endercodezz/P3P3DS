#!/usr/bin/env python3
"""Visible-output gate for the CLEANROOM / verification flow.

A prim-count gate (real_nonsprite > 0) cannot tell a rendered frame from a
black one: a broken transform can submit thousands of prims that all collapse
to a sub-pixel dot, leaving the screen black while real_nonsprite stays high
(see .planning/research/patapon-visible-output.md — #27 turned the Patapon
title black this exact way). This gate reads a runtime frame grab (the debug
socket `S <path>` TGA) and asserts the frame is actually VISIBLE: it counts
distinct pixel colors and the non-black pixel ratio and fails if either falls
below a threshold.

Master Patapon title baseline (t≈22s): ~100 distinct colors, ~19159/130560
(14.67%) non-black, ~13100 pure-white pixels. A black grab = 1 color, 0%.

Usage:
    visible_output_gate.py <tga> [--min-distinct N] [--min-nonblack-pct P]
                                 [--black-thresh T]

Exit 0 (PASS) if distinct_colors >= min-distinct AND non-black% >=
min-nonblack-pct; exit 1 (FAIL) otherwise. Prints a one-line summary either
way so it can be grepped from a verification log.
"""
from __future__ import annotations

import argparse
import sys


def read_tga_bgra(path: str) -> tuple[int, int, bytes]:
    """Read an uncompressed 32bpp BGRA TGA written by the runtime.

    Args:
        path: Path to the TGA file (runtime write_tga format: 18-byte header,
            true-color uncompressed, 32bpp BGRA).

    Returns:
        A (width, height, pixels) tuple where pixels is the raw BGRA byte run.

    Raises:
        ValueError: If the header is not the expected uncompressed 32bpp form.
    """
    data = open(path, "rb").read()
    if len(data) < 18:
        raise ValueError(f"{path}: too short to be a TGA")
    image_type = data[2]
    bpp = data[16]
    if image_type != 2 or bpp != 32:
        raise ValueError(
            f"{path}: expected uncompressed 32bpp TGA, got type={image_type} "
            f"bpp={bpp}"
        )
    width = data[12] | (data[13] << 8)
    height = data[14] | (data[15] << 8)
    return width, height, data[18:]


def analyze(pixels: bytes, black_thresh: int) -> tuple[int, int, int]:
    """Count distinct colors and non-black pixels in a BGRA pixel run.

    Args:
        pixels: Raw BGRA bytes (4 bytes per pixel).
        black_thresh: Max per-channel R/G/B value still counted as black.

    Returns:
        A (distinct_colors, non_black, total) tuple.
    """
    seen: set[int] = set()
    non_black = 0
    total = len(pixels) // 4
    for i in range(total):
        b, g, r = pixels[i * 4], pixels[i * 4 + 1], pixels[i * 4 + 2]
        seen.add((r << 16) | (g << 8) | b)
        if r > black_thresh or g > black_thresh or b > black_thresh:
            non_black += 1
    return len(seen), non_black, total


def main() -> int:
    """Parse args, analyze the grab, and return the gate exit code."""
    ap = argparse.ArgumentParser(description="Visible-output gate")
    ap.add_argument("tga")
    ap.add_argument("--min-distinct", type=int, default=8)
    ap.add_argument("--min-nonblack-pct", type=float, default=1.0)
    ap.add_argument("--black-thresh", type=int, default=8)
    args = ap.parse_args()

    width, height, pixels = read_tga_bgra(args.tga)
    distinct, non_black, total = analyze(pixels, args.black_thresh)
    pct = (100.0 * non_black / total) if total else 0.0

    ok = distinct >= args.min_distinct and pct >= args.min_nonblack_pct
    verdict = "PASS" if ok else "FAIL"
    print(
        f"[VISIBLE_GATE] {verdict} {width}x{height} distinct={distinct} "
        f"non_black={non_black}/{total} ({pct:.2f}%) "
        f"min_distinct={args.min_distinct} min_nonblack_pct="
        f"{args.min_nonblack_pct}",
        file=sys.stderr if not ok else sys.stdout,
    )
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
