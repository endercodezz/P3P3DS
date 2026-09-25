#!/usr/bin/env python3
"""
Unit tests for tools/prepare_game.py
Tests PARAM.SFO parsing, ISO9660 walking, path traversal sanitization,
DISC_ID validation, and error handling without using proprietary files.
"""

import io
import struct
import unittest
import tempfile
from pathlib import Path

# Add project root to sys.path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.prepare_game import (
    parse_param_sfo,
    sanitize_iso_path,
    IsoReader,
    prepare_p3p_game,
    decrypt_eboot,
    SUPPORTED_DISC_ID
)

def create_synthetic_param_sfo(entries: dict) -> bytes:
    """Builds a binary PARAM.SFO buffer from a dictionary of string key-value pairs."""
    num_entries = len(entries)
    key_table = bytearray()
    data_table = bytearray()
    index_table = bytearray()

    for key, val in entries.items():
        k_off = len(key_table)
        key_table.extend(key.encode("utf-8") + b"\x00")

        d_off = len(data_table)
        encoded_val = str(val).encode("utf-8") + b"\x00"
        data_table.extend(encoded_val)

        # uint16 k_off, uint16 param_fmt (0x0204 utf8), uint32 param_len, uint32 param_max_len, uint32 d_off
        index_table.extend(struct.pack("<HHIII", k_off, 0x0204, len(encoded_val), len(encoded_val), d_off))

    header_len = 20
    index_len = len(index_table)
    key_table_off = header_len + index_len
    data_table_off = key_table_off + len(key_table)

    header = struct.pack("<4sIIII", b"\x00PSF", 0x0101, key_table_off, data_table_off, num_entries)
    return header + bytes(index_table) + bytes(key_table) + bytes(data_table)

def create_synthetic_iso(files: dict) -> bytes:
    """
    Creates a minimal synthetic ISO9660 byte stream.
    files: dict of { "PATH/IN/ISO": bytes_data }
    """
    SECTOR = 2048
    # Pre-allocate 32 sectors for system area and PVD
    iso = bytearray(32 * SECTOR)

    # Sector 16: Primary Volume Descriptor (PVD)
    pvd_offset = 16 * SECTOR
    iso[pvd_offset] = 1 # Type: PVD
    iso[pvd_offset + 1:pvd_offset + 6] = b"CD001"
    iso[pvd_offset + 6] = 1 # Version

    # Allocate sectors for root directory and files
    # Sector 20: Root directory
    root_lba = 20
    # Directory record in PVD at offset 156
    struct.pack_into("<B", iso, pvd_offset + 156, 34) # Root record length
    struct.pack_into("<I", iso, pvd_offset + 158, root_lba) # LBA
    struct.pack_into("<I", iso, pvd_offset + 166, SECTOR)   # Data length
    struct.pack_into("<B", iso, pvd_offset + 181, 2)        # Flags (directory)
    struct.pack_into("<B", iso, pvd_offset + 188, 1)        # Name len
    iso[pvd_offset + 189] = 0                               # Root dir name \0

    # Build directory entries in root sector
    current_lba = 21
    dir_entries = bytearray()

    # Self (.) and parent (..)
    self_rec = bytearray(34)
    self_rec[0] = 34
    struct.pack_into("<I", self_rec, 2, root_lba)
    struct.pack_into("<I", self_rec, 10, SECTOR)
    self_rec[25] = 2
    self_rec[32] = 1
    self_rec[33] = 0
    dir_entries.extend(self_rec)

    parent_rec = bytearray(34)
    parent_rec[0] = 34
    struct.pack_into("<I", parent_rec, 2, root_lba)
    struct.pack_into("<I", parent_rec, 10, SECTOR)
    parent_rec[25] = 2
    parent_rec[32] = 1
    parent_rec[33] = 1
    dir_entries.extend(parent_rec)

    file_payloads = []
    for iso_name, content in files.items():
        name_clean = iso_name.upper().strip("/").split(";")[0]
        name_bytes = name_clean.encode("ascii")
        rec_len = 33 + len(name_bytes)
        if rec_len % 2 != 0:
            rec_len += 1
        rec = bytearray(rec_len)
        rec[0] = rec_len
        struct.pack_into("<I", rec, 2, current_lba)
        struct.pack_into("<I", rec, 10, len(content))
        rec[25] = 0 # File flag
        rec[32] = len(name_bytes)
        rec[33:33 + len(name_bytes)] = name_bytes
        dir_entries.extend(rec)

        file_payloads.append((current_lba, content))
        sectors_needed = (len(content) + SECTOR - 1) // SECTOR
        if sectors_needed == 0:
            sectors_needed = 1
        current_lba += sectors_needed

    total_sectors = max(32, current_lba + 1)
    if len(iso) < total_sectors * SECTOR:
        iso.extend(bytearray(total_sectors * SECTOR - len(iso)))
    # Write directory sector
    iso[root_lba * SECTOR : root_lba * SECTOR + len(dir_entries)] = dir_entries

    # Write file payloads
    for lba, content in file_payloads:
        iso[lba * SECTOR : lba * SECTOR + len(content)] = content

    return bytes(iso)


class TestPrepareGame(unittest.TestCase):

    def test_parse_param_sfo(self):
        sfo_bytes = create_synthetic_param_sfo({
            "TITLE": "Persona3 PORTABLE",
            "DISC_ID": "ULUS10512",
            "DISC_VERSION": "1.00"
        })
        parsed = parse_param_sfo(sfo_bytes)
        self.assertEqual(parsed.get("TITLE"), "Persona3 PORTABLE")
        self.assertEqual(parsed.get("DISC_ID"), "ULUS10512")
        self.assertEqual(parsed.get("DISC_VERSION"), "1.00")

    def test_parse_param_sfo_invalid_magic(self):
        with self.assertRaises(ValueError):
            parse_param_sfo(b"INVALID_HEADER_DATA_12345")

    def test_sanitize_iso_path(self):
        self.assertEqual(sanitize_iso_path("/data/field/pack.bin"), "data/field/pack.bin")
        self.assertEqual(sanitize_iso_path("USRDIR\\sound\\pmsf\\"), "USRDIR/sound/pmsf")
        with self.assertRaises(ValueError):
            sanitize_iso_path("/USRDIR/../../etc/passwd")

    def test_iso_reader_not_iso(self):
        fake_data = io.BytesIO(b"\x00" * 40960)
        with self.assertRaises(ValueError) as ctx:
            IsoReader(fake_data)
        self.assertIn("Not a valid ISO9660", str(ctx.exception))

    def test_synthetic_iso_reader_walk(self):
        sfo_data = create_synthetic_param_sfo({"DISC_ID": "ULUS10512"})
        iso_bytes = create_synthetic_iso({
            "PARAM.SFO": sfo_data,
            "TEST.TXT": b"Hello P3P"
        })
        with IsoReader(io.BytesIO(iso_bytes)) as reader:
            files = dict((path.upper().strip("/"), (lba, size)) for path, lba, size in reader.walk_files())
            self.assertIn("PARAM.SFO", files)
            self.assertIn("TEST.TXT", files)
            read_test = reader.read_file(files["TEST.TXT"][0], files["TEST.TXT"][1])
            self.assertEqual(read_test, b"Hello P3P")

    def test_wrong_disc_id_rejection(self):
        wrong_sfo = create_synthetic_param_sfo({"DISC_ID": "ULES99999", "TITLE": "Other Game"})
        iso_bytes = create_synthetic_iso({
            "PSP_GAME/PARAM.SFO": wrong_sfo
        })
        with tempfile.NamedTemporaryFile(suffix=".iso", delete=False) as tmp:
            tmp.write(iso_bytes)
            tmp_path = tmp.name

        try:
            with self.assertRaises(ValueError) as ctx:
                prepare_p3p_game(tmp_path, check_only=True, verbose=False)
            self.assertIn("Unsupported game disc ID", str(ctx.exception))
        finally:
            Path(tmp_path).unlink(missing_ok=True)

    def test_missing_iso_raises(self):
        with self.assertRaises(FileNotFoundError):
            prepare_p3p_game("non_existent_disc_image_12345.iso", verbose=False)

    def test_decrypt_eboot_validation(self):
        with self.assertRaises(ValueError) as ctx:
            decrypt_eboot(b"SHORT")
        self.assertIn("truncated", str(ctx.exception))

        fake_header = bytearray(0x200)
        fake_header[:4] = b"BADM"
        with self.assertRaises(ValueError) as ctx:
            decrypt_eboot(fake_header)
        self.assertIn("header magic mismatch", str(ctx.exception))


if __name__ == "__main__":
    unittest.main()
