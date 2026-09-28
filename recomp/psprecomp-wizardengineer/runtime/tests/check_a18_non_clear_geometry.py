#!/usr/bin/env python3
"""Phase 11.5 A18 gate: at least one [GE_PRIM_DETAIL] line with type != 6 AND normal > 0 AND clear == 0.

Parses [GE_PRIM_DETAIL] log lines emitted by runtime/src/psp_ge_draw.cpp's
atexit summary. Each line has the canonical form:
    [GE_PRIM_DETAIL] type=<N> normal=<N> clear=<N> total=<N>

PASS iff at least one line has type != 6 (i.e. NOT clear-mode PRIM-type-6
sprites) AND normal > 0 (non-clear-mode invocations exist) AND clear == 0
(none of the invocations of that type were in clear render mode). FAIL with
context dump if no line matches.

Usage: check_a18_non_clear_geometry.py [logfile]
Default logfile: /tmp/p112.log
"""

import re
import sys


def parse_kv(line: str) -> dict:
    """Parse `key=value` tokens from a [GE_PRIM_DETAIL] line.

    Args:
        line: A raw log line containing [GE_PRIM_DETAIL] markers.

    Returns:
        Dict mapping each key to its int value (0 default for missing).
    """
    out: dict = {}
    for tok in re.findall(r"(\w+)=(-?\d+)", line):
        out[tok[0]] = int(tok[1])
    return out


def main(argv: list) -> int:
    """Scan logfile for A18-passing [GE_PRIM_DETAIL] lines.

    Args:
        argv: argv[1] = optional logfile path (default /tmp/p112.log).

    Returns:
        0 on PASS, 1 on FAIL.
    """
    path = argv[1] if len(argv) > 1 else "/tmp/p112.log"
    matched_lines: list = []
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            for raw in f:
                if "[GE_PRIM_DETAIL]" not in raw:
                    continue
                if "[GE_PRIM_DETAIL_SUMMARY]" in raw:
                    continue
                matched_lines.append(raw.rstrip())
                kv = parse_kv(raw)
                t = kv.get("type", -1)
                n = kv.get("normal", 0)
                # accept either spelling
                c = kv.get("clear", kv.get("clear_mode", 0))
                if t != 6 and n > 0 and c == 0:
                    print(f"PASS A18: {raw.rstrip()}")
                    return 0
    except FileNotFoundError:
        print(f"FAIL A18: logfile not found: {path}", file=sys.stderr)
        return 1
    print(
        "FAIL A18: no [GE_PRIM_DETAIL] line with type != 6 AND normal > 0 "
        "AND clear == 0",
        file=sys.stderr,
    )
    for ln in matched_lines[:10]:
        print(f"  context: {ln}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
