#!/usr/bin/env python3
"""Turn an overlay dump into a shared library Yakumo loads at run time.

    add_overlay.py [-j N] [--no-build] <build_dir> <overlay.bin> <base_address>

Steps: identify the dump from its header (name, sizes, FNV-1a hash of the header
and the code after it), wrap it in a minimal ELF, run psp_recomp with a
per-overlay symbol prefix, then build the one CMake target for that overlay with
N parallel jobs (default 2). The executable is not relinked. --no-build stops
after psp_recomp and prints the target name, so a caller can build many
overlays in one Ninja run.
"""

import argparse
import os
import re
import struct
import subprocess
import sys

PROFILE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OVERLAY_DIR = os.path.join(PROFILE_DIR, "overlays")
HEADER_BYTES = 64


def fnv1a64(data):
    value = 0xCBF29CE484222325
    for byte in data:
        value = ((value ^ byte) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return value


def parse_header(data, base):
    """Name, image size and code size from the 64-byte overlay header."""
    if data[:4] != b"MWo3":
        raise SystemExit("not an overlay image: expected the MWo3 magic")
    _id, load, code_size, data_size = struct.unpack("<4I", data[4:20])
    if load != base:
        raise SystemExit(f"image loads at {load:#010x}, not at {base:#010x}")
    name = data[32:HEADER_BYTES].split(b"\0")[0].decode("ascii", "replace")
    name = re.sub(r"\W", "_", os.path.splitext(name)[0])
    return name, HEADER_BYTES + code_size + data_size, code_size


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0],
                                     usage=__doc__.strip().splitlines()[2].strip())
    parser.add_argument("-j", "--jobs", type=int, default=2, help="parallel build jobs (default 2)")
    parser.add_argument("--no-build", action="store_true", help="recompile only; print the target")
    parser.add_argument("build_dir")
    parser.add_argument("dump_path")
    parser.add_argument("base_text")
    options = parser.parse_args(argv[1:])
    build_dir, dump_path, base_text = options.build_dir, options.dump_path, options.base_text
    base = int(base_text, 0)
    data = open(dump_path, "rb").read()
    name, image_size, code_size = parse_header(data, base)
    # Only the header and the code identify the image: the game writes into the
    # data section of a loaded overlay.
    digest = fnv1a64(data[:HEADER_BYTES + code_size])
    prefix = f"ovl{base:08X}_{name}_{digest:016X}"
    target = os.path.join(OVERLAY_DIR, prefix)
    os.makedirs(target, exist_ok=True)

    elf_path = os.path.join(target, "overlay.elf")
    subprocess.run([sys.executable, os.path.join(PROFILE_DIR, "tools", "wrap_overlay.py"),
                    dump_path, base_text, elf_path], check=True)
    subprocess.run([os.path.join(build_dir, "psp_recomp"), elf_path, "--auto", target,
                    base_text, "--prefix", prefix], check=True)
    # The metadata the host identifies the corpus by. CMake reads it to configure
    # the library entry point. The build does not need an explicit reconfigure:
    # the CONFIGURE_DEPENDS glob over overlays/*/meta.txt notices the new
    # directory and reruns CMake on its own.
    with open(os.path.join(target, "meta.txt"), "w") as out:
        out.write(f"base=0x{base:08X}\nname={name}\nsize={image_size}\ncode_size={code_size}\n"
                  f"hash=0x{digest:016X}\nsource={os.path.basename(dump_path)}\n")

    if options.no_build:
        print(f"target: overlay_{prefix}")
        return 0
    subprocess.run(["cmake", "--build", build_dir, "--target", f"overlay_{prefix}",
                    "-j", str(options.jobs)], check=True)
    print(f"overlay {name}: {code_size} bytes of code at {base:#010x}")
    print(f"library: {os.path.join(build_dir, 'bin', 'overlays')}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
