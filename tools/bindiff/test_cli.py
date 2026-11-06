#!/usr/bin/env python3
"""Synthetic, game-independent end-to-end fixtures for p3p-bindiff."""
import argparse
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
EXE = None


def elf_fixture(prx=True):
    """One PT_LOAD and NULL/.text/.shstrtab/.bss; .bss has no file payload."""
    b = bytearray(304)
    b[:16] = b"\x7fELF\x01\x01\x01" + bytes(9)
    struct.pack_into("<HHIIIIIHHHHHH", b, 16, 0xFFA0 if prx else 2, 8, 1,
                     0x1000, 52, 144, 0, 52, 32, 1, 40, 4, 2)
    struct.pack_into("<8I", b, 52, 1, 96, 0x1000, 0x1000, 4, 8, 5, 4)
    b[96:100] = b"ABCD"
    names = b"\x00.text\x00.shstrtab\x00.bss\x00"
    b[112:112 + len(names)] = names
    struct.pack_into("<10I", b, 184, 1, 1, 6, 0x1000, 96, 4, 0, 0, 4, 0)
    struct.pack_into("<10I", b, 224, 7, 3, 0, 0, 112, len(names), 0, 0, 1, 0)
    struct.pack_into("<10I", b, 264, 17, 8, 3, 0x1004, 0xFFFFFFF0, 4, 0, 0, 4, 0)
    return b


def many_sections_elf(address, count):
    """A section header table only: NULL, .shstrtab and `count` NOBITS sections."""
    names = b"\x00.s\x00"
    table = (52 + len(names) + 3) & ~3
    b = bytearray(table + (count + 2) * 40)
    b[:16] = b"\x7fELF\x01\x01\x01" + bytes(9)
    struct.pack_into("<HHIIIIIHHHHHH", b, 16, 2, 8, 1, 0x1000, 0, table, 0, 52, 32, 0, 40, count + 2, 1)
    b[52:52 + len(names)] = names
    struct.pack_into("<10I", b, table + 40, 1, 3, 0, 0, 52, len(names), 0, 0, 1, 0)
    for index in range(count):
        struct.pack_into("<10I", b, table + (index + 2) * 40, 1, 8, 3, address + index * 4, 0x100, 4, 0, 0, 4, 0)
    return b


class CliTests(unittest.TestCase):
    def setUp(self):
        root = ROOT / ".tmp/bindiff-tests"
        root.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=root)
        self.addCleanup(self.temp.cleanup)
        self.dir = Path(self.temp.name)

    def run_cli(self, *args, code=0):
        result = subprocess.run([str(EXE), *map(str, args)], cwd=self.dir, capture_output=True)
        self.assertEqual(result.returncode, code, result.stderr.decode(errors="replace"))
        if code != 2:
            self.assertEqual(result.stderr, b"")
        else:
            self.assertEqual(result.stdout, b"")
            self.assertIn(b"p3p-bindiff:", result.stderr)
        return result

    def pair(self, left, right, elf=False):
        (self.dir / "left").write_bytes(left)
        (self.dir / "right").write_bytes(right)
        args = ["--json", "left", "right"]
        if elf:
            args.insert(0, "--elf")
        result = self.run_cli(*args, code=int(left != right))
        # Verify byte-identical JSON for the same inputs/configuration.
        self.assertEqual(result.stdout, self.run_cli(*args, code=int(left != right)).stdout)
        return json.loads(result.stdout)

    def test_help_version_and_usage(self):
        self.assertIn(b"Usage:", self.run_cli("--help").stdout)
        self.assertIn(b"Zig 0.17.1", self.run_cli("--version").stdout)
        for args in ([], ["x"], ["a", "b", "c"], ["--wat"],
                     ["--json", "--json", "a", "b"], ["--elf", "--elf", "a", "b"]):
            self.run_cli(*args, code=2)

    def test_empty_equal_and_tails(self):
        self.assertTrue(self.pair(b"", b"")["equal"])
        self.assertTrue(self.pair(b"same", b"same")["equal"])
        d = self.pair(b"abc", b"aX")["bytes"]
        self.assertEqual((d["changed_bytes"], d["removed_bytes"], d["added_bytes"]), (1, 1, 0))
        self.assertEqual(d["ranges"], [{"start": 1, "end": 3}])
        d = self.pair(b"", b"abc")["bytes"]
        self.assertEqual(d["added_bytes"], 3)

    def test_randomized_positional_oracle(self):
        rng = random.Random(10512)
        for _ in range(40):
            left = bytes(rng.randrange(4) for _ in range(rng.randrange(64)))
            right = bytes(rng.randrange(4) for _ in range(rng.randrange(64)))
            d = self.pair(left, right)["bytes"]
            self.assertEqual(d["changed_bytes"], sum(a != b for a, b in zip(left, right)))
            self.assertEqual(d["added_bytes"], max(0, len(right) - len(left)))
            self.assertEqual(d["removed_bytes"], max(0, len(left) - len(right)))
            expected = {i for i in range(max(len(left), len(right)))
                        if i >= min(len(left), len(right)) or left[i] != right[i]}
            actual = {i for r in d["ranges"] for i in range(r["start"], r["end"])}
            self.assertEqual(actual, expected)

    def test_bounded_ranges(self):
        d = self.pair(bytes(600), b"\x00\x01" * 300)["bytes"]
        self.assertEqual(d["range_count"], 300)
        self.assertEqual(d["changed_bytes"], 300)
        self.assertEqual(len(d["ranges"]), 256)
        self.assertTrue(d["ranges_truncated"])

    def test_unicode_paths_and_option_terminator(self):
        name = '-данные "quote".dat' if os.name != "nt" else "-данные space.dat"
        (self.dir / name).write_bytes(b"same")
        result = self.run_cli("--json", "--", name, name)
        self.assertEqual(json.loads(result.stdout)["left"], name)

    def test_missing_file_and_directory(self):
        self.run_cli("missing", "missing", code=2)
        self.run_cli(".", ".", code=2)

    def test_input_limit(self):
        # Sparse/extended file: exercise rejection without reading 256 MiB.
        with (self.dir / "large").open("wb") as f:
            f.truncate(256 * 1024 * 1024 + 1)
        self.run_cli("large", "large", code=2)

    def test_elf_and_prx_equal(self):
        for prx in (True, False):
            b = elf_fixture(prx)
            d = self.pair(b, b, elf=True)
            self.assertEqual(d["elf"]["section_changes"], [])
            self.assertEqual(d["elf"]["segment_changes"], [])
            self.assertEqual(d["elf"]["header_changes"], [])
            self.assertEqual(d["elf"]["left_header"]["elf_type"], 0xFFA0 if prx else 2)

    def test_content_and_metadata_changes(self):
        a = elf_fixture()
        b = bytearray(a)
        b[97] = ord("!")
        struct.pack_into("<I", b, 24, 0x1010)
        struct.pack_into("<I", b, 52 + 24, 7)
        d = self.pair(a, b, elf=True)["elf"]
        self.assertEqual(d["header_changes"], [{"field": "entry", "left": 0x1000, "right": 0x1010}])
        self.assertFalse(d["segment_changes"][0]["content_equal"])
        self.assertFalse(d["section_changes"][0]["content_equal"])
        self.assertEqual(d["section_changes"][0]["left"]["name"], ".text")
        self.assertEqual(d["section_changes"][0]["left"]["file_offset"], 96)
        self.assertEqual(d["section_changes"][0]["left"]["virtual_address"], 0x1000)
        text = self.run_cli("--elf", "left", "right", code=1).stdout
        self.assertIn(b"ELF entry:", text)
        self.assertIn(b"ELF section[1] modified", text)

    def test_added_removed_tables_and_nobits(self):
        a = elf_fixture()
        b = bytearray(a)
        struct.pack_into("<H", b, 44, 0)
        struct.pack_into("<H", b, 48, 3)
        d = self.pair(a, b, elf=True)["elf"]
        self.assertEqual(d["segment_changes"][0]["status"], "removed")
        self.assertEqual(d["section_changes"][0]["status"], "removed")
        self.assertIsNone(d["section_changes"][0]["content_equal"])
        d = self.pair(b, a, elf=True)["elf"]
        self.assertEqual(d["segment_changes"][0]["status"], "added")
        b = bytearray(a)
        struct.pack_into("<I", b, 264 + 20, 8)
        d = self.pair(a, b, elf=True)["elf"]
        self.assertIsNone(d["section_changes"][0]["content_equal"])

    def test_stripped_elf(self):
        a = elf_fixture()[:100]
        struct.pack_into("<I", a, 32, 0)
        struct.pack_into("<H", a, 48, 0)
        struct.pack_into("<H", a, 50, 0)
        self.assertTrue(self.pair(a, a, elf=True)["equal"])

    def test_section_names_are_compared_by_value(self):
        a = elf_fixture()
        b = bytearray(a)
        b[113:118] = b'.code'
        d = self.pair(a, b, elf=True)["elf"]
        self.assertEqual(d["section_changes"][0]["right"]["name"], ".code")
        # Non-UTF8 ELF names remain lossless JSON byte arrays.
        b[113] = 0xFF
        d = self.pair(a, b, elf=True)["elf"]
        self.assertEqual(d["section_changes"][0]["right"]["name"][0], 255)

    def test_malformed_elf(self):
        good = elf_fixture()
        (self.dir / "right").write_bytes(good)
        cases = [b"~PSP", b"\x00PBP", b"garbage", good[:51], good[:200]]
        for offset, fmt, value in (
            (4, "B", 2), (5, "B", 2), (6, "B", 0), (18, "H", 3),
            (20, "I", 0), (40, "H", 0), (42, "H", 0), (46, "H", 0),
            (28, "I", 0xFFFFFFF0), (32, "I", 0xFFFFFFF0),
            (28, "I", 0), (32, "I", 0),
            (44, "H", 0xFFFF), (50, "H", 0xFFFF), (50, "H", 20),
            (52 + 4, "I", 0xFFFFFFF0), (52 + 20, "I", 1),
            (52 + 8, "I", 0xFFFFFFFE), (184, "I", 500),
            (224 + 4, "I", 8), (184 + 16, "I", 0xFFFFFFF0),
        ):
            bad = bytearray(good)
            struct.pack_into("<" + fmt, bad, offset, value)
            cases.append(bad)
        bad = bytearray(good)
        bad[112:134] = b"X" * 22  # No terminating NUL in .shstrtab.
        cases.append(bad)
        for bad in cases:
            (self.dir / "left").write_bytes(bad)
            self.run_cli("--elf", "--json", "left", "right", code=2)
        # A malformed right input must also fail, before emitting any JSON.
        (self.dir / "left").write_bytes(good)
        (self.dir / "right").write_bytes(b"~PSP")
        self.run_cli("--elf", "--json", "left", "right", code=2)

    @unittest.skipUnless(os.name == "nt", "Windows pipes")
    def test_output_to_a_full_asynchronous_pipe(self):
        # Git Bash gives programs pipes like this one; the pipes of subprocess are synchronous.
        import ctypes
        from ctypes import wintypes
        import msvcrt

        (self.dir / "left").write_bytes(many_sections_elf(0x1000, 3000))
        (self.dir / "right").write_bytes(many_sections_elf(0x9000, 3000))
        expected = self.run_cli("--elf", "left", "right", code=1).stdout
        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.CreateNamedPipeW.restype = wintypes.HANDLE
        kernel32.CreateFileW.restype = wintypes.HANDLE
        name = rf"\\.\pipe\p3p-bindiff-test-{os.getpid()}"
        inbound, overlapped, generic_write, open_existing = 1, 0x40000000, 0x40000000, 3
        server = kernel32.CreateNamedPipeW(name, inbound, 0, 1, 4096, 4096, 0, None)
        client = kernel32.CreateFileW(name, generic_write, 0, None, open_existing, overlapped, None)
        self.assertNotIn(wintypes.HANDLE(-1).value, (server, client))
        descriptor = msvcrt.open_osfhandle(client, 0)
        process = subprocess.Popen([str(EXE), "--elf", "left", "right"], cwd=self.dir, stdout=descriptor, stderr=subprocess.PIPE)
        os.close(descriptor)
        received = bytearray()
        chunk = ctypes.create_string_buffer(65536)
        count = wintypes.DWORD()
        while kernel32.ReadFile(wintypes.HANDLE(server), chunk, len(chunk), ctypes.byref(count), None):
            received += chunk.raw[:count.value]
        kernel32.CloseHandle(wintypes.HANDLE(server))
        stderr = process.communicate()[1]
        self.assertEqual(process.returncode, 1, stderr[-400:])
        self.assertEqual(bytes(received), expected)

    def test_output_error(self):
        (self.dir / "left").write_bytes(bytes(600))
        (self.dir / "right").write_bytes(b"\x00\x01" * 300)
        process = subprocess.Popen([str(EXE), "--json", "left", "right"], cwd=self.dir,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        process.stdout.close()
        stderr = process.stderr.read()
        process.stderr.close()
        self.assertEqual(process.wait(), 2, stderr)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True, type=Path)
    args, rest = parser.parse_known_args()
    EXE = args.exe.resolve()
    unittest.main(argv=[__file__, *rest], verbosity=2)
