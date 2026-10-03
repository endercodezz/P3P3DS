#!/usr/bin/env python3
"""Clone the upstream reference repositories into references/, psp/, 3ds/ and p3p/.

These directories are not part of the P3P3DS repository (.gitignore): they are
other projects' sources, read for research and partly used by tests
(references/pspautotests: tests/test_mpeg.cpp, tests/test_sascore.cpp,
tools/run_autotests.py; those skip when it is absent). The build does not
need them: the 3DS libraries come from devkitPro.

usage: python tools/fetch_references.py [name ...]   (default: all; existing directories are kept)
Shallow clones of each default branch; the maintainer's copies may be older,
so the autotest baseline can differ slightly. Inventory and roles:
docs/REPOSITORIES.md.
"""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPOS = {
    "references/pspautotests": "https://github.com/hrydgard/pspautotests",
    "references/ppsspp": "https://github.com/hrydgard/ppsspp",
    "references/uofw": "https://github.com/uofw/uofw",
    "references/jpcsp": "https://github.com/jpcsp/jpcsp",
    "references/azahar": "https://github.com/azahar-emu/azahar",
    "references/DaedalusX64-3DS": "https://github.com/MasterFeizz/DaedalusX64-3DS",
    "references/GLASS": "https://github.com/kynex7510/GLASS",
    "references/persona-3-dual": "https://github.com/p3d-project/persona-3-dual",
    "references/static-recomp/XenonRecomp": "https://github.com/hedge-dev/XenonRecomp",
    "references/static-recomp/rexglue-sdk": "https://github.com/rexglue/rexglue-sdk",
    "references/static-recomp/xenia": "https://github.com/xenia-project/xenia",
    "references/static-recomp/N64ModernRuntime": "https://github.com/N64Recomp/N64ModernRuntime",
    "references/static-recomp/Zelda64Recomp": "https://github.com/Zelda64Recomp/Zelda64Recomp",
    "references/static-recomp/UnleashedRecomp": "https://github.com/hedge-dev/UnleashedRecomp",
    "psp/pspsdk": "https://github.com/pspdev/pspsdk",
    "psp/vfpu-docs": "https://github.com/pspdev/vfpu-docs",
    "psp/prxtool": "https://github.com/pspdev/prxtool",
    "psp/ghidra-allegrex": "https://github.com/kotcrab/ghidra-allegrex",
    "psp/psp-ghidra-scripts": "https://github.com/pspdev/psp-ghidra-scripts",
    "3ds/libctru": "https://github.com/devkitPro/libctru",
    "3ds/citro3d": "https://github.com/devkitPro/citro3d",
    "3ds/citro2d": "https://github.com/devkitPro/citro2d",
    "3ds/3ds-examples": "https://github.com/devkitPro/3ds-examples",
    "3ds/picasso": "https://github.com/devkitPro/picasso",
    "3ds/nihstro": "https://github.com/neobrain/nihstro",
    "p3p/Persona-3-Portable-Mod-Menu": "https://github.com/DniweTamp/Persona-3-Portable-Mod-Menu",
}


def main() -> int:
    wanted = sys.argv[1:]
    failed = 0
    for path, url in REPOS.items():
        if wanted and not any(w in path for w in wanted):
            continue
        target = ROOT / path
        if target.exists() and any(target.iterdir()):
            print(f"keep   {path}")
            continue
        print(f"clone  {path} <- {url}")
        if subprocess.call(["git", "clone", "--depth", "1", url, str(target)]) != 0:
            failed += 1
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
