"""Resolve import NIDs to names from PSPSDK IMPORT_FUNC stubs (AGENTS.md section 4 order).

Usage: name_imports.py <imports.csv> [output.txt]   Standard library only.
"""
import csv
import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def pspsdk_names():
    names = {}
    for path in sorted((ROOT / "psp/pspsdk/src").rglob("*.S")):
        text = path.read_text(encoding="utf-8", errors="replace")
        for lib, nid, name in re.findall(r'IMPORT_FUNC\s+"(\w+)"\s*,\s*(0x[0-9A-Fa-f]+)\s*,\s*(\w+)', text):
            names.setdefault((lib, int(nid, 16)), (name, path.relative_to(ROOT).as_posix()))
            names.setdefault(("*", int(nid, 16)), (name, path.relative_to(ROOT).as_posix()))
    return names


def uofw_names():
    """uOFW exports.exp: explicit NIDs and SHA-1 hashed names (first 4 bytes, LE)."""
    names = {}
    for path in sorted((ROOT / "references/uofw/src").rglob("exports.exp")):
        text = path.read_text(encoding="utf-8", errors="replace")
        source = path.relative_to(ROOT).as_posix()
        for name, nid in re.findall(r"PSP_EXPORT_FUNC_NID\(\s*(\w+)\s*,\s*(0x[0-9A-Fa-f]+)\s*\)", text):
            names.setdefault(("*", int(nid, 16)), (name, source))
        for name in re.findall(r"PSP_EXPORT_FUNC_HASH\(\s*(\w+)\s*\)", text):
            nid = int.from_bytes(hashlib.sha1(name.encode()).digest()[:4], "little")
            names.setdefault(("*", nid), (name, source))
    return names


def main(argv):
    names = uofw_names()
    for key, value in pspsdk_names().items():
        names.setdefault(key, value)
    rows = list(csv.reader(open(argv[0], newline="", encoding="utf-8")))[1:]
    lines = []
    for row in rows:
        lib, nid = row[0], int(row[1], 16)
        name, source = names.get((lib, nid)) or names.get(("*", nid)) or ("?", "")
        lines.append(f"{lib},0x{nid:08X},{name},{source}")
    text = "library,nid,name,source\n" + "\n".join(lines) + "\n"
    if len(argv) > 1:
        Path(argv[1]).write_text(text, encoding="utf-8", newline="\n")
    else:
        sys.stdout.write(text)
    missing = sum(1 for l in lines if ",?," in l)
    print(f"resolved {len(lines) - missing}/{len(lines)}", file=sys.stderr)


if __name__ == "__main__":
    main(sys.argv[1:])
