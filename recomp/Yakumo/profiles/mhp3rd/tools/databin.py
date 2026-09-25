#!/usr/bin/env python3
"""List and extract entries from the game's DATA.BIN archive.

DATA.BIN (`/PSP_GAME/USRDIR/DATA.BIN`) is the read-only archive the game's
`fakeRofsLoader` thread serves every asset and code overlay from. Entries are
obfuscated but not cryptographically encrypted: see docs/DATA_BIN.md for the
format. This tool implements the archive directory and the cipher so entries,
the code overlays among them, can be read offline.

    databin.py <archive> list [--overlays] [--csv]
    databin.py <archive> extract <output_dir> [index ...]
    databin.py <archive> extract-overlays <output_dir>
    databin.py <archive> cat <index>
    databin.py <archive> verify <index> <reference.bin>

`<archive>` is either DATA.BIN itself or a PSP ISO, in which case
/PSP_GAME/USRDIR/DATA.BIN is located inside it and read in place.

The cipher constants and the substitution table below were recovered from this
game's own data by a known-plaintext comparison between DATA.BIN and overlay
images captured from guest memory; no third-party source was used.
"""

import argparse
import os
import struct
import sys

BLOCK = 2048

# XOR keystream: two independent 16-bit Lehmer generators, one per halfword.
KEY_MULTIPLIER = (0x2345, 0x7F8D)
KEY_MODULUS = (0xFFD9, 0xFFF1)
KEY_DEFAULT = (0x2345, 0x7F8D)

# Byte substitution applied after the XOR step when encrypting.
ENCODE_TABLE = bytes.fromhex(
    "c0a8ca074b6e486fd692312c9dfbe15061c6e4523e12ad33aeebf32f6b697b53"
    "96c4b19c1cc520861913e96a2675788c43ed7a665d181de870a55ef25f580546"
    "0d979e7cea65dd248f4942aff425b82b087217d9a4d393715b40b22e0b7e4c04"
    "f711c13779a729bc1b568bfa8d363b6dd45783bd1fd76284f5dad5abcca24788"
    "9a2dc7dfcb022841a93dd8a1233c816c5cd068c9bf9901bef9fcecb70a8289dc"
    "91ef14cf344a03d1ba358a06ff38a0f0ce7d0c76c2b3ac0994555480a395bba6"
    "302af6671efe776364876000b09844ee4de5c3cd5122739be01a74c85a3f4ee6"
    "aa7f21f1599fb9904fe2fdb416e3f80ee71585393ade0fd2b68e27dbb5324510"
)

DECODE_TABLE = bytearray(256)
for _plain, _cipher in enumerate(ENCODE_TABLE):
    DECODE_TABLE[_cipher] = _plain
DECODE_TABLE = bytes(DECODE_TABLE)

VERBATIM_MAGICS = (b"~SCE", b"PSMF")
OVERLAY_MAGIC = b"MWo3"
OVERLAY_HEADER = 64


def keystream(block_address, word_count):
    """Return `word_count` little-endian keystream words as raw bytes.

    Both halfword generators are purely multiplicative, so each cycles with a
    period of at most 65520. The cycle is generated once and tiled, which keeps
    even multi-megabyte entries cheap.
    """
    if word_count == 0:
        return b""
    seeds = (block_address >> 16, block_address & 0xFFFF)
    halves = []
    for index in range(2):
        state = seeds[index] or KEY_DEFAULT[index]
        multiplier = KEY_MULTIPLIER[index]
        modulus = KEY_MODULUS[index]
        # Both moduli are prime, so the sequence is purely periodic from the
        # first generated value onwards. Generate at most one period and tile it,
        # which keeps large entries cheap without penalising small ones.
        cycle = []
        while len(cycle) < word_count:
            state = (state * multiplier) % modulus
            if cycle and state == cycle[0]:
                break
            cycle.append(state)
        if len(cycle) < word_count:
            cycle = (cycle * -(-word_count // len(cycle)))[:word_count]
        halves.append(cycle)

    words = bytearray(word_count * 4)
    struct.pack_into(
        "<%dH" % (word_count * 2),
        words,
        0,
        *[value for pair in zip(halves[1], halves[0]) for value in pair],
    )
    return bytes(words)


def decrypt(data, block_address):
    """Decrypt `data`, which must start at `block_address` inside the archive."""
    padded = data + b"\0" * (-len(data) % 4)
    substituted = padded.translate(DECODE_TABLE)
    mask = keystream(block_address, len(padded) // 4)
    plain = int.from_bytes(substituted, "little") ^ int.from_bytes(mask, "little")
    return plain.to_bytes(len(padded), "little")[: len(data)]


def iso_find(image, wanted):
    """Return (offset, size) of `wanted` inside an ISO9660 image, or None."""
    image.seek(16 * BLOCK)
    primary = image.read(BLOCK)
    if primary[1:6] != b"CD001":
        return None
    found = []

    def walk(record, parent):
        lba, size = struct.unpack_from("<I", record, 2)[0], struct.unpack_from("<I", record, 10)[0]
        image.seek(lba * BLOCK)
        data = image.read(size)
        offset = 0
        while offset < len(data):
            length = data[offset]
            if length == 0:
                offset = (offset // BLOCK + 1) * BLOCK
                continue
            entry = data[offset : offset + length]
            offset += length
            name = entry[33 : 33 + entry[32]]
            if name in (b"\0", b"\1"):
                continue
            path = parent + "/" + name.decode("ascii", "replace").split(";")[0]
            if entry[25] & 2:
                walk(entry, path)
            elif path.upper() == wanted:
                found.append(
                    (
                        struct.unpack_from("<I", entry, 2)[0] * BLOCK,
                        struct.unpack_from("<I", entry, 10)[0],
                    )
                )

    walk(primary[156:190], "")
    return found[0] if found else None


class Archive:
    """The DATA.BIN directory: a block table plus an exact-size table."""

    def __init__(self, path):
        self.stream = open(path, "rb")
        self.base = 0
        self.size = os.path.getsize(path)
        located = iso_find(self.stream, "/PSP_GAME/USRDIR/DATA.BIN")
        if located:
            self.base, self.size = located
        self.blocks, self.sizes = self._read_directory()

    def _raw(self, offset, length):
        self.stream.seek(self.base + offset)
        return self.stream.read(length)

    def _read_directory(self):
        # The first word is the number of blocks the directory itself occupies.
        head = decrypt(self._raw(0, 4), 0)
        directory_size = struct.unpack("<I", head)[0] * BLOCK
        if not 0 < directory_size <= 1 << 20:
            raise ValueError("implausible directory size; not an MHP3rd DATA.BIN?")
        words = struct.unpack(
            "<%dI" % (directory_size // 4), decrypt(self._raw(0, directory_size), 0)
        )

        # Block table: entry n spans blocks[n] .. blocks[n + 1], terminated by
        # the block count of the whole archive.
        end = self.size // BLOCK
        count = 1
        while count < len(words) and words[count] >= words[count - 1]:
            if words[count] == end:
                break
            count += 1
        if count >= len(words) or words[count] != end:
            raise ValueError("block table is not terminated by the archive size")
        blocks = list(words[: count + 1])

        # Exact-size table: (entry index, byte size) pairs, sorted by index,
        # present only for entries whose true length is not block aligned.
        sizes = {}
        pairs = words[count + 1 :]
        for index in range(0, len(pairs) - 1, 2):
            entry, length = pairs[index], pairs[index + 1]
            if entry >= count or not 0 < length <= (blocks[entry + 1] - blocks[entry]) * BLOCK:
                break
            sizes[entry] = length
        return blocks, sizes

    def __len__(self):
        return len(self.blocks) - 1

    def entry_size(self, index):
        span = (self.blocks[index + 1] - self.blocks[index]) * BLOCK
        return self.sizes.get(index, span)

    def read(self, index, length=None):
        block = self.blocks[index]
        if length is None:
            length = self.entry_size(index)
        raw = self._raw(block * BLOCK, length)
        # A handful of entries are stored verbatim: the ~SCE PRX stubs and the
        # PSMF movies. Deobfuscating them turns them into noise.
        if raw[:4] in VERBATIM_MAGICS:
            return raw
        return decrypt(raw, block)

    def overlay(self, index):
        """Return the overlay header fields, or None if the entry is not one."""
        header = self.read(index, OVERLAY_HEADER)
        if header[:4] != OVERLAY_MAGIC:
            return None
        ident, load, text, data, bss, end, entry = struct.unpack_from("<7I", header, 4)
        return {
            "id": ident,
            "load": load,
            "text": text,
            "data": data,
            "bss": bss,
            "end": end,
            "entry": entry,
            "name": header[32:64].split(b"\0")[0].decode("ascii", "replace"),
            "size": OVERLAY_HEADER + text + data,
        }


def describe(archive, index):
    overlay = archive.overlay(index)
    if overlay:
        return "overlay %-24s load=%#010x text=%#x data=%#x bss=%#x" % (
            overlay["name"],
            overlay["load"],
            overlay["text"],
            overlay["data"],
            overlay["bss"],
        )
    magic = archive.read(index, 16)[:8]
    printable = "".join(chr(b) if 32 <= b < 127 else "." for b in magic[:4])
    return "%-8s %s" % (printable, magic.hex())


def command_list(archive, args):
    for index in range(len(archive)):
        overlay = archive.overlay(index)
        if args.overlays and not overlay:
            continue
        if args.csv:
            print(
                "%d,%d,%d,%s,%s"
                % (
                    index,
                    archive.blocks[index],
                    archive.entry_size(index),
                    overlay["name"] if overlay else "",
                    "%#x" % overlay["load"] if overlay else "",
                )
            )
        else:
            print(
                "%5d  lba=%-7d size=%-9d %s"
                % (index, archive.blocks[index], archive.entry_size(index), describe(archive, index))
            )


def write_entry(archive, index, directory, name=None):
    data = archive.read(index)
    if name is None:
        overlay = archive.overlay(index)
        name = "%05d_%s" % (index, overlay["name"]) if overlay else "%05d.bin" % index
    path = os.path.join(directory, name)
    with open(path, "wb") as out:
        out.write(data)
    return path, len(data)


def command_extract(archive, args):
    os.makedirs(args.output, exist_ok=True)
    indices = [int(value, 0) for value in args.indices] or range(len(archive))
    for index in indices:
        path, length = write_entry(archive, index, args.output)
        print("%9d  %s" % (length, path))


def command_extract_overlays(archive, args):
    os.makedirs(args.output, exist_ok=True)
    count = 0
    for index in range(len(archive)):
        overlay = archive.overlay(index)
        if not overlay:
            continue
        name = "overlay_%08X_%s.bin" % (overlay["load"], overlay["name"].rsplit(".", 1)[0])
        path, length = write_entry(archive, index, args.output, name)
        print("%9d  %s" % (length, path))
        count += 1
    print("%d overlays written" % count, file=sys.stderr)


def command_cat(archive, args):
    sys.stdout.buffer.write(archive.read(int(args.index, 0)))


def command_verify(archive, args):
    index = int(args.index, 0)
    data = archive.read(index)
    with open(args.reference, "rb") as handle:
        reference = handle.read()
    overlap = min(len(data), len(reference))
    differing = [i for i in range(overlap) if data[i] != reference[i]]
    overlay = archive.overlay(index)
    print("archive entry %d: %d bytes, reference: %d bytes" % (index, len(data), len(reference)))
    if overlay:
        code_end = OVERLAY_HEADER + overlay["text"]
        in_code = [i for i in differing if i < code_end]
        print("header+code (0..%#x): %d differing bytes" % (code_end, len(in_code)))
        print(
            "data (%#x..%#x): %d differing bytes"
            % (code_end, overlay["size"], len(differing) - len(in_code))
        )
    print("%d of %d compared bytes differ" % (len(differing), overlap))
    return 0 if not differing else 1


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("archive", help="DATA.BIN or a PSP ISO containing it")
    sub = parser.add_subparsers(dest="command", required=True)

    listing = sub.add_parser("list")
    listing.add_argument("--overlays", action="store_true", help="only .ovl entries")
    listing.add_argument("--csv", action="store_true")
    listing.set_defaults(handler=command_list)

    extract = sub.add_parser("extract")
    extract.add_argument("output")
    extract.add_argument("indices", nargs="*")
    extract.set_defaults(handler=command_extract)

    overlays = sub.add_parser("extract-overlays")
    overlays.add_argument("output")
    overlays.set_defaults(handler=command_extract_overlays)

    cat = sub.add_parser("cat")
    cat.add_argument("index")
    cat.set_defaults(handler=command_cat)

    verify = sub.add_parser("verify")
    verify.add_argument("index")
    verify.add_argument("reference")
    verify.set_defaults(handler=command_verify)

    args = parser.parse_args(argv[1:])
    try:
        archive = Archive(args.archive)
    except (ValueError, struct.error) as error:
        print("%s: %s" % (args.archive, error), file=sys.stderr)
        return 2
    try:
        return args.handler(archive, args) or 0
    except BrokenPipeError:  # piping into head and friends
        os.dup2(os.open(os.devnull, os.O_WRONLY), sys.stdout.fileno())
        return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
