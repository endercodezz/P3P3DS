#!/usr/bin/env python3
"""
next_suffix.py — Increment a lowercase letter suffix for build versioning.

Usage:
    python3 tools/next_suffix.py <suffix>

Examples:
    python3 tools/next_suffix.py a   →  b
    python3 tools/next_suffix.py z   →  aa
    python3 tools/next_suffix.py az  →  ba
    python3 tools/next_suffix.py zz  →  aaa
"""

import sys


def next_suffix(s: str) -> str:
    chars = list(s)
    i = len(chars) - 1
    while i >= 0:
        if chars[i] != "z":
            chars[i] = chr(ord(chars[i]) + 1)
            break
        else:
            chars[i] = "a"
            i -= 1
    else:
        chars.insert(0, "a")
    return "".join(chars)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <suffix>", file=sys.stderr)
        sys.exit(1)
    print(next_suffix(sys.argv[1]))
