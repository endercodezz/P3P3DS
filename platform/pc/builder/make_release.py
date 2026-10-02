#!/usr/bin/env python3
"""Assemble the P3P3DS Builder release package (out/P3P3DS-Builder-<version>/ + .zip).

Prerequisites (maintainer machine): the host build in build/ (P3P3DS-Builder.exe,
psp_recomp.exe) and the New 3DS build in build/3ds/ (runtime objects). Nothing
derived from the game goes into the package: sdk/ holds only the prebuilt
P3P3DS runtime for the 3DS, the code generator, its headers and the CWCheat
patch list, which are all project code or community data.

Usage: python platform/pc/builder/make_release.py [--version 0.1.0]
"""
import argparse
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / "build"
BUILD3DS = BUILD / "3ds"

RUNTIME_OBJECTS = {
    "main.o": BUILD3DS / "CMakeFiles/p3p3ds.dir/main.o",
    "gpu_renderer.o": BUILD3DS / "CMakeFiles/p3p3ds.dir/gpu_renderer.o",
    "ge_shbin.o": BUILD3DS / "CMakeFiles/p3p_ge_shader_bin.dir/.dkp-generated/p3p_ge_shader_bin/ge_shbin.o",
    "libp3p3ds_core.a": BUILD3DS / "libp3p3ds_core.a",
    "libpsprecomp_runtime.a": BUILD3DS / "libpsprecomp_runtime.a",
}

README = """P3P3DS Builder {version}
==========================

Builds P3P3DS (Persona 3 Portable for New Nintendo 3DS) from YOUR OWN copy of
the game. Nothing is downloaded and no game data is included here.

You need
  * Windows 10 or 11 (64-bit)
  * your Persona 3 Portable ISO, North American release ULUS-10512
  * devkitPro with the "3DS Development" component
    (https://devkitpro.org/wiki/Getting_Started - Windows installer)

How to use
  1. Run P3P3DS-Builder.exe.
  2. Choose your ISO and the devkitPro folder (found automatically when possible).
  3. Press Build. It takes about 5 to 15 minutes, depending on your CPU.
     An interrupted build continues where it stopped.
  4. Put on the SD card of your New 3DS (the builder can copy them for you):
       \\3ds\\p3p3ds.3dsx
       \\p3p3ds\\<your game>.iso
     and start P3P3DS from the Homebrew Launcher.

The resulting p3p3ds.3dsx contains code made from your copy of the game:
keep it to yourself, do not share it.

State of the port: see README.md on GitHub (saves work, no sound yet).

P3P3DS by enderlit aka endercodezz - MIT License (see LICENSE.txt).
Persona 3 Portable is (c) ATLUS. This project is not affiliated with ATLUS or SEGA.
"""


def git_version() -> str:
    try:
        return subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "--short", "HEAD"], text=True).strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", default="0.1.0")
    parser.add_argument("--devkitpro", default="", help="devkitPro folder: strips debug info from the runtime objects")
    args = parser.parse_args()

    required = {"P3P3DS-Builder.exe": BUILD / "P3P3DS-Builder.exe",
                "psp_recomp.exe": BUILD / "recomp/PSPRecomp/psp_recomp.exe",
                "patches.txt": BUILD3DS / "generated/patches.txt", **RUNTIME_OBJECTS}
    missing = [name for name, path in required.items() if not path.exists()]
    if missing:
        print("missing build outputs: " + ", ".join(missing), file=sys.stderr)
        return 1

    name = f"P3P3DS-Builder-{args.version}"
    out = ROOT / "out" / name
    if out.exists():
        shutil.rmtree(out)
    sdk = out / "sdk"
    (sdk / "bin").mkdir(parents=True)
    (sdk / "lib").mkdir()
    shutil.copy2(required["P3P3DS-Builder.exe"], out / "P3P3DS-Builder.exe")
    shutil.copy2(required["psp_recomp.exe"], sdk / "bin" / "psp_recomp.exe")
    shutil.copy2(required["patches.txt"], sdk / "patches.txt")
    strip = Path(args.devkitpro) / "devkitARM/bin/arm-none-eabi-strip.exe" if args.devkitpro else None
    for obj, path in RUNTIME_OBJECTS.items():
        shutil.copy2(path, sdk / "lib" / obj)
        if strip and strip.exists():  # debug info is 95 % of the archives
            subprocess.check_call([str(strip), "--strip-debug", str(sdk / "lib" / obj)])
    shutil.copytree(ROOT / "recomp/PSPRecomp/include/psprecomp", sdk / "include" / "psprecomp")
    (sdk / "VERSION").write_text(f"{args.version} {git_version()}\n", encoding="utf-8", newline="\n")
    shutil.copy2(ROOT / "LICENSE", out / "LICENSE.txt")
    (out / "README.txt").write_text(README.format(version=args.version), encoding="utf-8", newline="\r\n")

    archive = ROOT / "out" / f"{name}.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for f in sorted(out.rglob("*")):
            if f.is_file():
                z.write(f, Path(name) / f.relative_to(out))
    size = sum(f.stat().st_size for f in out.rglob("*") if f.is_file())
    print(f"{out} ({size / 1e6:.1f} MB), {archive} ({archive.stat().st_size / 1e6:.1f} MB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
