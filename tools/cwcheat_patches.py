"""Extract selected CWCheat codes into a psp_recomp patch list.

Usage: cwcheat_patches.py <cheats.ini> <selection.txt> <output.txt>
selection.txt: one code title per line ("_C" name), '#' comments.
Only constant writes are supported: 0x0 (8-bit), 0x1 (16-bit), 0x2 (32-bit);
addresses are relative to 0x08800000 as in CWCheat. Output lines:
"<address> <value> <width>" in hex. Standard library only.
"""
import sys
from pathlib import Path


def parse(ini_text):
    codes, current = {}, None
    for raw in ini_text.splitlines():
        line = raw.strip()
        if line.startswith("_C"):
            current = line[2:].strip()
            if current[:1].isdigit():
                current = current[1:].strip()
            codes[current] = []
        elif line.startswith("_L") and current is not None:
            a, v = line[2:].split()[:2]
            codes[current].append((int(a, 16), int(v, 16)))
    return codes


def main(argv):
    if len(argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    codes = parse(Path(argv[0]).read_text(encoding="utf-8", errors="replace"))
    wanted = [l.split("#")[0].strip() for l in Path(argv[1]).read_text(encoding="utf-8").splitlines()]
    out = []
    for name in filter(None, wanted):
        if name not in codes:
            print(f"unknown CWCheat code: {name}", file=sys.stderr)
            return 1
        for address, value in codes[name]:
            kind = address >> 28
            width = {0: 1, 1: 2, 2: 4}.get(kind)
            if width is None:
                print(f"unsupported CWCheat code type 0x{kind:X} in {name}", file=sys.stderr)
                return 1
            out.append(f"0x{0x08800000 + (address & 0x0FFFFFFF):08X} 0x{value & ((1 << (8 * width)) - 1):08X} {width}")
    Path(argv[2]).write_text("".join(l + "\n" for l in out), encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
