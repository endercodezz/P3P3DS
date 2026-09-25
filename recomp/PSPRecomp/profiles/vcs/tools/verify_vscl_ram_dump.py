#!/usr/bin/env python3
"""Validate the deterministic VCS VSCL collision checkpoint from RAM dumps."""
from __future__ import annotations
import math
import struct
import sys
from pathlib import Path

RAM_BASE = 0x08000000
PLAYER_ENTITY = 0x098B0940
POSITION_OFFSET = 0x30
EXPECTED_MIN_Z = 14.5
MAX_Z_DRIFT = 0.01


def read_position(path: Path) -> tuple[float, float, float]:
    data = path.read_bytes()
    offset = PLAYER_ENTITY - RAM_BASE + POSITION_OFFSET
    if offset < 0 or offset + 12 > len(data):
        raise RuntimeError(f"player position lies outside {path}")
    return struct.unpack_from("<3f", data, offset)


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {Path(sys.argv[0]).name} RAM_DUMP_DIR", file=sys.stderr)
        return 2
    directory = Path(sys.argv[1])
    first = directory / "ram_vblank_001710.bin"
    second = directory / "ram_vblank_001740.bin"
    if not first.is_file() or not second.is_file():
        print("missing RAM dumps for vblank 1710/1740", file=sys.stderr)
        return 2
    p1710 = read_position(first)
    p1740 = read_position(second)
    drift = abs(p1740[2] - p1710[2])
    print(f"vblank 1710: X={p1710[0]:.6f} Y={p1710[1]:.6f} Z={p1710[2]:.9f}")
    print(f"vblank 1740: X={p1740[0]:.6f} Y={p1740[1]:.6f} Z={p1740[2]:.9f}")
    print(f"Z drift: {drift:.9f}")
    valid = all(math.isfinite(v) for v in (*p1710, *p1740)) and p1740[2] >= EXPECTED_MIN_Z and drift <= MAX_Z_DRIFT
    print("VSCL_COLLISION_VALIDATION=PASS" if valid else "VSCL_COLLISION_VALIDATION=FAIL")
    return 0 if valid else 1


if __name__ == "__main__":
    raise SystemExit(main())
