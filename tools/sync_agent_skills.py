#!/usr/bin/env python3
"""Mirror project skills from .claude/skills (source of truth) to .agents/skills.

Usage:
  python tools/sync_agent_skills.py          copy every file, delete stale mirror files
  python tools/sync_agent_skills.py --check  exit 1 and list differences, change nothing

Files are compared and copied as bytes, so line endings are preserved.
"""
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / ".claude" / "skills"
MIRROR = ROOT / ".agents" / "skills"


def files(base: Path) -> dict:
    return {p.relative_to(base).as_posix(): p for p in base.rglob("*") if p.is_file()} if base.exists() else {}


def main() -> int:
    check = "--check" in sys.argv[1:]
    src, dst = files(SOURCE), files(MIRROR)
    differences = []
    for rel, path in sorted(src.items()):
        if rel not in dst:
            differences.append(f"missing in mirror: {rel}")
        elif path.read_bytes() != dst[rel].read_bytes():
            differences.append(f"differs: {rel}")
    for rel in sorted(set(dst) - set(src)):
        differences.append(f"only in mirror: {rel}")
    if check:
        for d in differences:
            print(d)
        print("skills mirror OK" if not differences else f"{len(differences)} difference(s); run tools/sync_agent_skills.py")
        return 1 if differences else 0
    for rel, path in src.items():
        target = MIRROR / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, target)
    for rel in set(dst) - set(src):
        (MIRROR / rel).unlink()
    for d in differences:
        print("synced:", d)
    print(f"{len(src)} skill files mirrored to {MIRROR.relative_to(ROOT).as_posix()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
