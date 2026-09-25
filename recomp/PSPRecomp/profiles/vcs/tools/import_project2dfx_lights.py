#!/usr/bin/env python3
"""Generate VCSProject2DFX_Lights.bin from a compatible MIT-licensed lodl.c table.

Usage:
    python import_project2dfx_lights.py path/to/lodl.c [output.bin]

The source file is supplied explicitly so the repository does not depend on a
particular upstream directory name or network fetch. The output format is the
portable little-endian VCS2DFX1 table consumed by the VCS profile.
"""
from __future__ import annotations
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_OUT = ROOT / "data" / "VCSProject2DFX_Lights.bin"
ENTRY = struct.Struct("<6f6i")
NUMBER = re.compile(r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?[fF]?")


def parse(text: str):
    marker = text.find("aLodLights[]")
    if marker < 0:
        raise SystemExit("ERROR: aLodLights[] not found")
    start = text.find("{", marker)
    end = text.find("\n};", start)
    if start < 0 or end < 0:
        raise SystemExit("ERROR: aLodLights[] initializer is incomplete")
    rows = []
    for line in text[start + 1:end].splitlines():
        line = re.sub(r"/\*.*?\*/", "", line)
        if "{" not in line or "}" not in line:
            continue
        body = line[line.find("{") + 1:line.find("}")]
        values = NUMBER.findall(body)
        if len(values) != 12:
            continue
        floats = [float(x.rstrip("fF")) for x in values[:6]]
        integers = [int(float(x.rstrip("fF"))) for x in values[6:]]
        rows.append((*floats, *integers))
    if len(rows) < 1000:
        raise SystemExit(f"ERROR: only {len(rows)} lights parsed; incompatible table")
    return rows


def main() -> None:
    if len(sys.argv) < 2:
        raise SystemExit("Usage: import_project2dfx_lights.py path/to/lodl.c [output.bin]")
    source = pathlib.Path(sys.argv[1])
    output = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_OUT
    lights = parse(source.read_text(encoding="utf-8", errors="replace"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("wb") as file:
        file.write(b"VCS2DFX1")
        file.write(struct.pack("<I", len(lights)))
        for row in lights:
            file.write(ENTRY.pack(*row))
    print(f"Source: {source}")
    print(f"Lights: {len(lights)}")
    print(f"Output: {output} ({output.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
