#!/usr/bin/env python3
"""Wrap a raw overlay dump in a minimal ELF32 MIPS executable.

The recompiler works on ELF images, while overlays are raw code the game copies
into a slot at run time. This builds the smallest ELF that carries the same
bytes at the same address, with one PT_LOAD and one executable .text section so
the analyzer treats the whole blob as code.

Usage: wrap_overlay.py <overlay.bin> <base_address> <output.elf>
"""

import struct
import sys

EHDR_SIZE = 52
PHDR_SIZE = 32
SHDR_SIZE = 40
SHSTRTAB = b"\0.text\0.shstrtab\0"


def main(argv):
    if len(argv) != 4:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    data = open(argv[1], "rb").read()
    base = int(argv[2], 0)
    if base % 4:
        print("base address must be 4-byte aligned", file=sys.stderr)
        return 1

    data_offset = EHDR_SIZE + PHDR_SIZE
    shstrtab_offset = data_offset + len(data)
    shdr_offset = shstrtab_offset + len(SHSTRTAB)

    ehdr = struct.pack(
        "<16sHHIIIIIHHHHHH",
        b"\x7fELF\x01\x01\x01" + b"\0" * 9,
        2,            # ET_EXEC
        8,            # EM_MIPS
        1,            # version
        base,         # entry
        EHDR_SIZE,    # phoff
        shdr_offset,  # shoff
        0x10A23001,   # flags, as produced by the PSP toolchain
        EHDR_SIZE, PHDR_SIZE, 1, SHDR_SIZE, 3, 2,
    )
    phdr = struct.pack("<8I", 1, data_offset, base, base, len(data), len(data), 5, 0x10)
    null_shdr = b"\0" * SHDR_SIZE
    text_shdr = struct.pack(
        "<10I", 1, 1, 0x6, base, data_offset, len(data), 0, 0, 0x10, 0)  # PROGBITS, ALLOC|EXECINSTR
    shstrtab_shdr = struct.pack(
        "<10I", 7, 3, 0, 0, shstrtab_offset, len(SHSTRTAB), 0, 0, 1, 0)  # STRTAB

    with open(argv[3], "wb") as out:
        out.write(ehdr + phdr + data + SHSTRTAB + null_shdr + text_shdr + shstrtab_shdr)
    print(f"wrote {argv[3]}: {len(data)} bytes at {base:#010x}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
