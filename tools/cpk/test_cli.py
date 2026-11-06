#!/usr/bin/env python3
"""Synthetic, game-independent end-to-end fixtures for p3p-cpk."""
import argparse
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile
import unittest

EXE = None
U16, U32, U64, STRING = 2, 4, 6, 10
ROW = 0x50
DEFAULT = 0x30


def utf_table(columns, rows):
    """columns: (name, kind, storage[, default]); rows: value per ROW column."""
    strings = bytearray(b"<NULL>\0")

    def text(value):
        offset = len(strings)
        strings.extend(value.encode("latin-1") + b"\0")
        return offset

    def packed(kind, value):
        if kind == STRING:
            return struct.pack(">I", text(value))
        return struct.pack({U16: ">H", U32: ">I", U64: ">Q"}[kind], value)

    schema = bytearray()
    row_columns = []
    for column in columns:
        name, kind, storage = column[:3]
        schema.append(storage | kind)
        schema += struct.pack(">I", text(name))
        if storage == DEFAULT:
            schema += packed(kind, column[3])
        else:
            row_columns.append(kind)
    data = b"".join(packed(kind, value) for row in rows for kind, value in zip(row_columns, row))
    row_size = len(data) // len(rows) if rows else 0
    rows_offset = 0x20 + len(schema)
    strings_offset = rows_offset + len(data)
    size = strings_offset + len(strings)
    header = b"@UTF" + struct.pack(">IHHIIIHHI", size - 8, 1, rows_offset - 8, strings_offset - 8, size - 8, 0, len(columns), row_size, len(rows))
    return header + bytes(schema) + data + bytes(strings)


def chunk(magic, table):
    return magic + b"\xFF\0\0\0" + struct.pack("<Q", len(table)) + table


def mask(table):
    out = bytearray(table)
    value = 0x5F
    for index, byte in enumerate(out):
        out[index] = byte ^ value
        value = value * 0x15 & 0xFF
    return bytes(out)


def layla(data):
    """CRILAYLA with literals, plus one copy block when the data repeats."""
    prefix, body = data[:0x100], data[0x100:]
    bits = []

    def put(value, width):
        bits.extend((value >> shift) & 1 for shift in range(width - 1, -1, -1))

    position = len(body)
    while position > 0:
        if position >= 8 and position + 3 <= len(body) and body[position - 3:position] == body[position:position + 3]:
            put(1, 1)
            put(0, 13)  # distance 3
            put(0, 2)  # length 3
            position -= 3
        else:
            position -= 1
            put(0, 1)
            put(body[position], 8)
    bits.extend([0] * (-len(bits) % 8))
    stream = bytearray(len(bits) // 8)
    for index, bit in enumerate(bits):
        stream[len(stream) - 1 - index // 8] |= bit << (7 - index % 8)
    return b"CRILAYLA" + struct.pack("<II", len(body), len(stream)) + bytes(stream) + prefix


def cpk(entries, encrypted=False):
    """entries: (dir, name, stored bytes, extract size). Layout: CPK, TOC, content."""
    columns = [("DirName", STRING, ROW), ("FileName", STRING, ROW), ("FileSize", U32, ROW), ("ExtractSize", U32, ROW), ("FileOffset", U64, ROW)]

    def build(content_offset, toc_offset):
        # With the TOC before the content, file offsets count from the TOC.
        position, rows = content_offset - toc_offset, []
        for directory, name, stored, extract in entries:
            rows.append((directory, name, len(stored), extract, position))
            position += len(stored)
        toc = utf_table(columns, rows)
        header = utf_table([("TocOffset", U64, ROW), ("ContentOffset", U64, ROW), ("Files", U32, DEFAULT, len(entries))], [(toc_offset, content_offset)])
        if encrypted:
            toc, header = mask(toc), mask(header)
        return chunk(b"CPK ", header), chunk(b"TOC ", toc)

    head, toc = build(0, 0)
    toc_offset = (len(head) + 0xFF) & ~0xFF
    content_offset = (toc_offset + len(toc) + 0xFF) & ~0xFF
    head, toc = build(content_offset, toc_offset)
    image = bytearray(content_offset)
    image[:len(head)] = head
    image[toc_offset:toc_offset + len(toc)] = toc
    return bytes(image) + b"".join(entry[2] for entry in entries)


def iso_record(name, sector, size, directory):
    body = bytearray(33 + len(name) + (len(name) + 1) % 2)
    body[0] = len(body)
    body[2:10] = struct.pack("<I", sector) + struct.pack(">I", sector)
    body[10:18] = struct.pack("<I", size) + struct.pack(">I", size)
    body[25] = 2 if directory else 0
    body[32] = len(name)
    body[33:33 + len(name)] = name
    return bytes(body)


def iso(cpk_bytes):
    """Image with /PSP_GAME/GAME.CPK;1."""
    sector = 2048
    root, game, data = 18, 19, 20
    image = bytearray(data * sector + cpk_bytes.__len__())
    descriptor = bytearray(sector)
    descriptor[0] = 1
    descriptor[1:6] = b"CD001"
    descriptor[156:156 + 34] = iso_record(b"\0", root, sector, True)
    image[16 * sector:17 * sector] = descriptor
    root_dir = iso_record(b"\0", root, sector, True) + iso_record(b"\1", root, sector, True) + iso_record(b"PSP_GAME", game, sector, True)
    game_dir = iso_record(b"\0", game, sector, True) + iso_record(b"\1", root, sector, True) + iso_record(b"GAME.CPK;1", data, len(cpk_bytes), False)
    image[root * sector:root * sector + len(root_dir)] = root_dir
    image[game * sector:game * sector + len(game_dir)] = game_dir
    image[data * sector:] = cpk_bytes
    return bytes(image)


def fnv(data):
    value = 0xCBF29CE484222325
    for byte in data:
        value = ((value ^ byte) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return value


def run(*args):
    # Names are printed as stored, and damaged ones are not valid text.
    return subprocess.run([EXE, *map(str, args)], capture_output=True, encoding="utf-8", errors="replace")


PLAIN = bytes(range(256)) * 3
ORIGINAL = bytes(random.Random(1).randrange(256) for _ in range(0x100)) + b"abcabcabcabc" * 40
COMPRESSED = layla(ORIGINAL)
ENTRIES = [("data/a", "plain.bin", PLAIN, len(PLAIN)), ("data/b", "packed.bin", COMPRESSED, len(ORIGINAL)), ("", "root.txt", b"hello", 5), ("data/a", "empty.bin", b"", 0)]


class Cli(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.dir = Path(self.temp.name)
        self.cpk = self.dir / "mod.cpk"
        self.cpk.write_bytes(cpk(ENTRIES))

    def assert_clean_error(self, result):
        self.assertEqual(result.returncode, 2, (result.returncode, result.stdout, result.stderr))
        self.assertTrue(result.stderr.startswith("p3p-cpk: "), result.stderr)

    def test_list_loose(self):
        result = run("list", self.cpk)
        self.assertEqual(result.returncode, 0, result.stderr)
        lines = result.stdout.splitlines()
        self.assertEqual(len(lines), 4)
        self.assertRegex(lines[0], r"^data/a/plain\.bin offset=0x[0-9A-F]+ size=0x300 extract=0x300 compressed=0$")
        self.assertRegex(lines[1], rf"^data/b/packed\.bin .* extract=0x{len(ORIGINAL):X} compressed=1$")
        self.assertTrue(lines[2].startswith("/root.txt "))

    def test_list_reads_no_file_data(self):
        data = bytearray(self.cpk.read_bytes())
        first = int(run("list", self.cpk, "plain").stdout.split("offset=0x")[1].split()[0], 16)
        data[first:] = bytes([0xEE]) * (len(data) - first)
        self.cpk.write_bytes(data)
        self.assertEqual(sum("compressed=1" in line for line in run("list", self.cpk).stdout.splitlines()), 1)

    def many_files(self):
        path = self.dir / "many.cpk"
        path.write_bytes(cpk([("some/long/directory/name", f"file_{index:05}.bin", b"x", 1) for index in range(3000)]))
        return path

    def test_output_larger_than_a_pipe_buffer(self):
        result = run("list", self.many_files())
        self.assertEqual(result.returncode, 0, result.stderr[-400:])
        self.assertEqual(len(result.stdout.splitlines()), 3000)

    @unittest.skipUnless(os.name == "nt", "Windows pipes")
    def test_output_to_a_full_asynchronous_pipe(self):
        # Git Bash gives programs pipes like this one; the pipes of subprocess are synchronous.
        import ctypes
        from ctypes import wintypes
        import msvcrt

        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.CreateNamedPipeW.restype = wintypes.HANDLE
        kernel32.CreateFileW.restype = wintypes.HANDLE
        name = rf"\\.\pipe\p3p-cpk-test-{os.getpid()}"
        inbound, overlapped, generic_write, open_existing = 1, 0x40000000, 0x40000000, 3
        server = kernel32.CreateNamedPipeW(name, inbound, 0, 1, 4096, 4096, 0, None)
        client = kernel32.CreateFileW(name, generic_write, 0, None, open_existing, overlapped, None)
        self.assertNotIn(wintypes.HANDLE(-1).value, (server, client))
        descriptor = msvcrt.open_osfhandle(client, 0)
        process = subprocess.Popen([EXE, "list", self.many_files()], stdout=descriptor, stderr=subprocess.PIPE, text=True)
        os.close(descriptor)
        received = bytearray()
        chunk_buffer = ctypes.create_string_buffer(65536)
        count = wintypes.DWORD()
        while kernel32.ReadFile(wintypes.HANDLE(server), chunk_buffer, len(chunk_buffer), ctypes.byref(count), None):
            received += chunk_buffer.raw[:count.value]
        kernel32.CloseHandle(wintypes.HANDLE(server))
        stderr = process.communicate()[1]
        self.assertEqual(process.returncode, 0, stderr[-400:])
        self.assertEqual(received.count(b"\n"), 3000)

    def test_filter_spans_directory_and_name(self):
        self.assertEqual(len(run("list", self.cpk, "A/PLAIN").stdout.splitlines()), 1)
        self.assertEqual(run("list", self.cpk, "missing").stdout, "")

    def test_extract_loose(self):
        out = self.dir / "out"
        result = run("extract", self.cpk, out)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((out / "data/a/plain.bin").read_bytes(), PLAIN)
        self.assertEqual((out / "data/b/packed.bin").read_bytes(), ORIGINAL)
        self.assertEqual((out / "root.txt").read_bytes(), b"hello")
        self.assertEqual((out / "data/a/empty.bin").read_bytes(), b"")

    def test_extract_filtered(self):
        out = self.dir / "out"
        self.assertEqual(run("extract", self.cpk, "packed", out).returncode, 0)
        self.assertEqual([p.name for p in out.rglob("*") if p.is_file()], ["packed.bin"])

    def test_masked_tables(self):
        path = self.dir / "masked.cpk"
        path.write_bytes(cpk(ENTRIES, encrypted=True))
        self.assertEqual(run("list", path).stdout, run("list", self.cpk).stdout)

    def test_cpk_inside_iso(self):
        image = self.dir / "game.iso"
        image.write_bytes(iso(cpk(ENTRIES)))
        result = run("list", image, "/psp_game/game.cpk", "packed")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, run("list", self.cpk, "packed").stdout)
        out = self.dir / "out"
        self.assertEqual(run("extract", image, "/PSP_GAME/GAME.CPK", out).returncode, 0)
        self.assertEqual((out / "data/b/packed.bin").read_bytes(), ORIGINAL)
        self.assert_clean_error(run("list", image, "/PSP_GAME/NOPE.CPK"))
        self.assert_clean_error(run("list", image, "/PSP_GAME"))
        self.assert_clean_error(run("list", image))

    def test_traversal_writes_nothing(self):
        for directory, name in [("..", "evil"), ("a/../..", "evil"), ("/abs", "evil"), ("C:", "evil"), ("a\\..\\..", "evil"), ("", "../evil"), ("", "NUL")]:
            with self.subTest(directory=directory, name=name):
                path = self.dir / "bad.cpk"
                path.write_bytes(cpk([("safe", "first.bin", b"x", 1), (directory, name, b"y", 1)]))
                out = self.dir / "sandbox" / "out"
                self.assert_clean_error(run("extract", path, out))
                self.assertFalse((self.dir / "sandbox").exists())
                self.assertFalse((self.dir / "evil").exists())

    def test_truncated_input_is_an_error(self):
        data = self.cpk.read_bytes()
        path = self.dir / "cut.cpk"
        for size in list(range(4, 0x60, 7)) + [len(data) // 2, len(data) - 1]:
            with self.subTest(size=size):
                path.write_bytes(data[:size])
                self.assert_clean_error(run("list", path))

    def test_mutated_input_never_crashes(self):
        data = self.cpk.read_bytes()
        rng = random.Random(65)
        path = self.dir / "mutated.cpk"
        out = self.dir / "out"
        for round_number in range(250):
            mutated = bytearray(data)
            for _ in range(rng.randrange(1, 9)):
                mutated[rng.randrange(4, len(mutated))] = rng.choice((0, 1, 0x7F, 0x80, 0xFF, rng.randrange(256)))
            path.write_bytes(mutated)
            for args in (("list", path), ("extract", path, out)):
                result = run(*args)
                self.assertIn(result.returncode, (0, 2), (round_number, args[0], result.stderr[-400:]))

    def test_corrupt_stream_is_an_error(self):
        stream = bytearray(COMPRESSED)
        stream[12:16] = struct.pack("<I", 0x7FFFFFF0)
        path = self.dir / "stream.cpk"
        path.write_bytes(cpk([("d", "f.bin", bytes(stream), 1)]))
        self.assert_clean_error(run("extract", path, self.dir / "out"))

    def test_garbage_and_usage(self):
        garbage = self.dir / "garbage.bin"
        garbage.write_bytes(b"\xAB" * 40000)
        self.assert_clean_error(run("list", garbage, "/x.cpk"))
        self.assert_clean_error(run("list"))
        self.assert_clean_error(run("nope"))
        self.assert_clean_error(run("list", self.dir / "missing.cpk"))
        self.assert_clean_error(run("list", self.cpk, "a", "b"))
        self.assertEqual(run("--help").returncode, 0)
        self.assertIn("0.1.0", run("--version").stdout)

    def trace(self, rows):
        path = self.dir / "trace.csv"
        path.write_text("path,offset,size,fnv1a64,time_us\n" + "".join(f"{p},{o},{s},0x{h:016X},0\n" for p, o, s, h in rows))
        return path

    def check_fixture(self):
        image = self.dir / "game.iso"
        data = cpk(ENTRIES)
        image.write_bytes(iso(data))
        first = int(run("list", self.cpk, "plain").stdout.split("offset=0x")[1].split()[0], 16)
        return image, data, first

    def test_check_trace_pass(self):
        image, data, first = self.check_fixture()
        path = "disc0:/PSP_GAME/GAME.CPK"
        trace = self.trace([
            (path, 0, 16, fnv(data[:16])),
            (path, first + 8, 100, fnv(data[first + 8:first + 108])),
            ("disc0:/PSP_GAME/OTHER.BIN", 0, 4, 0),
        ])
        result = run("check-trace", image, trace)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("cpk reads=2 header/toc=1 file_matched=1 mismatched=0 unclassified=0 non_cpk=1", result.stdout)
        self.assertIn("read 1x data/a/plain.bin", result.stdout)

    def test_check_trace_mismatch_and_unclassified(self):
        image, data, first = self.check_fixture()
        path = "disc0:/PSP_GAME/GAME.CPK"
        trace = self.trace([(path, first, 16, 1), (path, first - 4, 16, 1)])
        result = run("check-trace", image, trace)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("MISMATCH data/a/plain.bin", result.stdout)
        self.assertIn("UNCLASSIFIED", result.stdout)
        self.assertIn("file_matched=0 mismatched=1 unclassified=1", result.stdout)

    def test_check_trace_host_map(self):
        image, data, first = self.check_fixture()
        trace = self.trace([("ms0:/PSP/P3P/mod.cpk", first, 16, fnv(data[first:first + 16]))])
        result = run("check-trace", image, trace, f"ms0:/PSP/P3P={self.dir}")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("file_matched=1", result.stdout)

    def test_check_trace_bad_line(self):
        image, _, _ = self.check_fixture()
        path = self.dir / "trace.csv"
        path.write_text("path,offset,size,fnv1a64,time_us\ndisc0:/a.cpk,zero,1,0x0,0\n")
        self.assert_clean_error(run("check-trace", image, path))


def main():
    global EXE
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True)
    args, rest = parser.parse_known_args()
    EXE = os.fspath(Path(args.exe).resolve())
    unittest.main(argv=[sys.argv[0], *rest], verbosity=1)


if __name__ == "__main__":
    main()
