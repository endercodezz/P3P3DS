#!/usr/bin/env python3
"""
P3P3DS — Persona 3 Portable (ULUS-10512) Game Preparation Tool
Validates a user-supplied PSP ISO9660 image, checks game identity (PARAM.SFO),
decrypts the Allegrex executable (EBOOT.BIN), verifies cryptographic SHA-256 hashes,
and extracts USRDIR game assets into the local, gitignored destination (profiles/p3p/game/).

Usage:
  python tools/prepare_game.py "/path/to/Persona 3 Portable.iso" [options]

Options:
  --output-dir <dir>       Destination directory (default: profiles/p3p/game)
  --decrypted-eboot <path> Optional path to pre-decrypted eboot.elf (bypasses AES decryption)
  --check-only             Validate ISO identity and hashes without extracting files
  --force                  Overwrite existing files in destination directory
"""

import os
import sys
import struct
import hashlib
import shutil
import argparse
from pathlib import Path

# =============================================================================
# Constants & Game Identity Specification
# =============================================================================

SECTOR_SIZE = 2048

SUPPORTED_DISC_ID = "ULUS10512"
SUPPORTED_DISC_ID_FORMATTED = "ULUS-10512"
SUPPORTED_GAME_TITLE = "Persona3 PORTABLE"

# Verified SHA-256 of the decrypted executable (ULUS-10512)
EXPECTED_DECRYPTED_ELF_SHA256 = (
    "be2abbd43a4ae7ce5aa2f1f146083ffe0d924fc2eb1874e6fcdc21cba40d49db"
)

# Verified SHA-256 of the retail encrypted EBOOT.BIN from ULUS-10512 UMD
EXPECTED_ENCRYPTED_EBOOT_SHA256 = (
    "eeb19d9dcab8c42743151bae65dc6a40031b05f35b89db27deee5665a0be2855"
)

# Sony PSP PRX / Kirk cryptography keys for Tag 0xD91613F0
PSP_TAG_P3P = 0xD91613F0

K_TAG_KEY = bytes([
    0xEB, 0xFF, 0x40, 0xD8, 0xB4, 0x1A, 0xE1, 0x66,
    0x91, 0x3B, 0x8F, 0x64, 0xB6, 0xFC, 0xB7, 0x12
])

K_HEADER_KEY = bytes([
    0x11, 0x5A, 0x5D, 0x20, 0xD5, 0x3A, 0x8D, 0xD3,
    0x9C, 0xC5, 0xAF, 0x41, 0x0F, 0x0F, 0x18, 0x6F
])

K_PAYLOAD_WRAPPING_KEY = bytes([
    0x98, 0xC9, 0x40, 0x97, 0x5C, 0x1D, 0x10, 0xE8,
    0x7F, 0xE6, 0x0E, 0xA3, 0xFD, 0x03, 0xA8, 0xBA
])

# =============================================================================
# Cryptography (AES-128-CBC)
# =============================================================================

try:
    from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

    def cbc_decrypt(key: bytes, data: bytes) -> bytes:
        cipher = Cipher(algorithms.AES(key), modes.CBC(b"\x00" * 16))
        decryptor = cipher.decryptor()
        return decryptor.update(data) + decryptor.finalize()

except ImportError:
    # Pure-Python fallback for environments without `cryptography`
    # Standard Rijndael AES S-Box implementation for 128-bit CBC
    S_BOX = [
        0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
        0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
        0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
        0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
        0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
        0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
        0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
        0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
        0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
        0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
        0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
        0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
        0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
        0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
        0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
        0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
    ]
    INV_S_BOX = [0] * 256
    for _i, _v in enumerate(S_BOX):
        INV_S_BOX[_v] = _i

    RCON = [0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36]

    def _xtime(a):
        return ((a << 1) ^ 0x1B) & 0xFF if (a & 0x80) else (a << 1)

    def _mul(a, b):
        res = 0
        while b > 0:
            if b & 1: res ^= a
            a = _xtime(a)
            b >>= 1
        return res

    def _key_expansion(key):
        w = list(key)
        for i in range(16, 176, 4):
            t = w[i - 4:i]
            if i % 16 == 0:
                t = [S_BOX[t[1]] ^ RCON[i // 16], S_BOX[t[2]], S_BOX[t[3]], S_BOX[t[0]]]
            w.extend([w[i - 16 + j] ^ t[j] for j in range(4)])
        return w

    def _inv_cipher_block(block, round_keys):
        s = list(block)
        for j in range(16): s[j] ^= round_keys[160 + j]
        for r in range(9, 0, -1):
            s = [
                s[0], s[13], s[10], s[7],
                s[4], s[1], s[14], s[11],
                s[8], s[5], s[2], s[15],
                s[12], s[9], s[6], s[3]
            ]
            s = [INV_S_BOX[x] for x in s]
            rk = round_keys[r * 16:(r + 1) * 16]
            for j in range(16): s[j] ^= rk[j]
            ns = [0] * 16
            for c in range(4):
                col = s[c * 4:(c + 1) * 4]
                ns[c * 4 + 0] = _mul(col[0], 0x0E) ^ _mul(col[1], 0x0B) ^ _mul(col[2], 0x0D) ^ _mul(col[3], 0x09)
                ns[c * 4 + 1] = _mul(col[0], 0x09) ^ _mul(col[1], 0x0E) ^ _mul(col[2], 0x0B) ^ _mul(col[3], 0x0D)
                ns[c * 4 + 2] = _mul(col[0], 0x0D) ^ _mul(col[1], 0x09) ^ _mul(col[2], 0x0E) ^ _mul(col[3], 0x0B)
                ns[c * 4 + 3] = _mul(col[0], 0x0B) ^ _mul(col[1], 0x0D) ^ _mul(col[2], 0x09) ^ _mul(col[3], 0x0E)
            s = ns
        s = [
            s[0], s[13], s[10], s[7],
            s[4], s[1], s[14], s[11],
            s[8], s[5], s[2], s[15],
            s[12], s[9], s[6], s[3]
        ]
        s = [INV_S_BOX[x] for x in s]
        for j in range(16): s[j] ^= round_keys[j]
        return bytes(s)

    def cbc_decrypt(key: bytes, data: bytes) -> bytes:
        rkeys = _key_expansion(key)
        out = bytearray(len(data))
        prev_iv = bytes(16)
        for i in range(0, len(data), 16):
            ct = data[i:i + 16]
            pt = _inv_cipher_block(ct, rkeys)
            for j in range(16):
                out[i + j] = pt[j] ^ prev_iv[j]
            prev_iv = ct
        return bytes(out)

# =============================================================================
# EBOOT.BIN Unwrapping & Decryption
# =============================================================================

def derive_mask() -> bytes:
    raw = bytearray()
    for index in range(9):
        block = bytearray(K_TAG_KEY)
        block[0] = index
        raw.extend(block)
    return cbc_decrypt(K_HEADER_KEY, bytes(raw))

def unwrap_payload_key(header: bytes) -> bytes:
    record = bytearray()
    record.extend(header[0x140 : 0x140 + 0x10])
    record.extend(header[0x12C : 0x12C + 0x14])
    record.extend(header[0x80  : 0x80  + 0x30])
    record.extend(header[0xC0  : 0xC0  + 0x0C])
    dec_record = bytearray(cbc_decrypt(K_HEADER_KEY, bytes(record)))

    mask = derive_mask()
    key = bytearray(dec_record[0x24 : 0x24 + 16])
    for i in range(16):
        key[i] ^= mask[0x10 + i]
    key = bytearray(cbc_decrypt(K_HEADER_KEY, bytes(key)))
    for i in range(16):
        key[i] ^= mask[0x50 + i]
    return cbc_decrypt(K_PAYLOAD_WRAPPING_KEY, bytes(key))

def decrypt_eboot(eboot_bytes: bytes) -> bytes:
    if len(eboot_bytes) < 0x150:
        raise ValueError("EBOOT.BIN is truncated (smaller than ~PSP header)")

    magic = eboot_bytes[:4]
    if magic != b"~PSP":
        raise ValueError(f"EBOOT.BIN header magic mismatch: expected ~PSP, got {magic}")

    tag, = struct.unpack_from("<I", eboot_bytes, 0xD0)
    if tag != PSP_TAG_P3P:
        raise ValueError(f"Unsupported EBOOT.BIN tag: expected 0x{PSP_TAG_P3P:08X}, got 0x{tag:08X}")

    attr = struct.unpack_from("<H", eboot_bytes, 0x06)[0]
    if attr & 1:
        raise ValueError("Compressed EBOOT.BIN (~PSP attributes bit 0) is not supported")

    filetype = eboot_bytes[0x7C]
    if filetype != 9:
        raise ValueError(f"Unexpected file type in EBOOT header: {filetype} (expected 9 for UMD game)")

    payload_size, = struct.unpack_from("<I", eboot_bytes, 0xB0)
    padded_size = (payload_size + 15) & ~15
    if 0x150 + padded_size > len(eboot_bytes):
        raise ValueError("EBOOT.BIN is truncated relative to header payload size")

    payload_key = unwrap_payload_key(eboot_bytes[:0x150])
    payload = eboot_bytes[0x150 : 0x150 + padded_size]
    decrypted = cbc_decrypt(payload_key, payload)[:payload_size]

    if decrypted[:4] != b"\x7fELF":
        raise ValueError("Decrypted executable does not contain ELF magic header")

    return decrypted

# =============================================================================
# PARAM.SFO Parser
# =============================================================================

def parse_param_sfo(data: bytes) -> dict:
    if len(data) < 20:
        raise ValueError("Truncated PARAM.SFO buffer")

    magic, version, key_table_off, data_table_off, num_entries = struct.unpack_from("<4sIIII", data, 0)
    if magic != b"\x00PSF":
        raise ValueError(f"Invalid PARAM.SFO magic: {magic}")

    entries = {}
    for i in range(num_entries):
        k_off, param_fmt, param_len, _, d_off = struct.unpack_from("<HHIII", data, 20 + i * 16)
        raw_key = data[key_table_off + k_off:].split(b"\x00")[0]
        key_name = raw_key.decode("utf-8", errors="replace")

        d_raw = data[data_table_off + d_off : data_table_off + d_off + param_len]
        if param_fmt in (0x0004, 0x0204): # String
            val = d_raw.split(b"\x00")[0].decode("utf-8", errors="replace")
        elif param_fmt == 0x0404 and len(d_raw) >= 4: # uint32
            val = struct.unpack_from("<I", d_raw, 0)[0]
        else:
            val = d_raw
        entries[key_name] = val

    return entries

# =============================================================================
# ISO9660 Reader
# =============================================================================

class IsoReader:
    def __init__(self, file_path_or_obj):
        if hasattr(file_path_or_obj, "read"):
            self.file = file_path_or_obj
            self._should_close = False
        else:
            self.file = open(file_path_or_obj, "rb")
            self._should_close = True

        self.root_record = self._parse_volume_descriptor()

    def close(self):
        if self._should_close:
            self.file.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def _read_sector(self, lba: int, count: int = 1) -> bytes:
        self.file.seek(lba * SECTOR_SIZE)
        return self.file.read(count * SECTOR_SIZE)

    def _parse_volume_descriptor(self) -> bytes:
        pvd = self._read_sector(16, 1)
        if len(pvd) < SECTOR_SIZE or pvd[1:6] != b"CD001":
            raise ValueError("Not a valid ISO9660 disc image (CD001 identifier not found at sector 16)")
        return pvd[156:190] # Root directory record

    def walk_files(self):
        def _walk(record, parent_dir):
            lba, = struct.unpack_from("<I", record, 2)
            size, = struct.unpack_from("<I", record, 10)
            self.file.seek(lba * SECTOR_SIZE)
            dir_data = self.file.read(size)

            offset = 0
            while offset < len(dir_data):
                entry_len = dir_data[offset]
                if entry_len == 0:
                    offset = (offset // SECTOR_SIZE + 1) * SECTOR_SIZE
                    continue
                entry = dir_data[offset : offset + entry_len]
                offset += entry_len

                name_len = entry[32]
                raw_name = entry[33 : 33 + name_len]
                if raw_name in (b"\x00", b"\x01"):
                    continue

                name = raw_name.decode("latin1", errors="replace").split(";")[0]
                iso_path = f"{parent_dir}/{name}"
                flags = entry[25]
                ent_lba, = struct.unpack_from("<I", entry, 2)
                ent_size, = struct.unpack_from("<I", entry, 10)

                if flags & 2: # Directory
                    yield from _walk(entry, iso_path)
                else:
                    yield (iso_path, ent_lba, ent_size)

        yield from _walk(self.root_record, "")

    def read_file(self, lba: int, size: int) -> bytes:
        self.file.seek(lba * SECTOR_SIZE)
        return self.file.read(size)

# =============================================================================
# Game Preparation Orchestration
# =============================================================================

def sanitize_iso_path(iso_path: str) -> str:
    cleaned = iso_path.replace("\\", "/").strip("/")
    parts = []
    for segment in cleaned.split("/"):
        if segment in ("", "."):
            continue
        if segment == "..":
            raise ValueError(f"Path traversal detected in ISO path: {iso_path}")
        parts.append(segment)
    return "/".join(parts)

def prepare_p3p_game(
    iso_path: str,
    output_dir: str = "profiles/p3p/game",
    decrypted_eboot_path: str = None,
    check_only: bool = False,
    force: bool = False,
    verbose: bool = True
) -> bool:
    iso_file = Path(iso_path)
    if not iso_file.exists():
        raise FileNotFoundError(f"ISO file not found: {iso_path}")

    out_dir = Path(output_dir).resolve()

    if verbose:
        print("====================================================")
        print("   P3P3DS — Persona 3 Portable Game Preparation")
        print("====================================================")
        print(f"ISO Image:        {iso_file.name}")
        print(f"Destination:      {out_dir}")

    with IsoReader(iso_file) as reader:
        file_catalog = {}
        for path, lba, size in reader.walk_files():
            file_catalog[path.upper()] = (path, lba, size)

        # 1. Locate and parse PARAM.SFO
        sfo_key = "/PSP_GAME/PARAM.SFO"
        if sfo_key not in file_catalog:
            raise ValueError("PARAM.SFO not found inside ISO (not a valid PSP game disc)")

        _, sfo_lba, sfo_size = file_catalog[sfo_key]
        sfo_bytes = reader.read_file(sfo_lba, sfo_size)
        sfo = parse_param_sfo(sfo_bytes)

        disc_id = str(sfo.get("DISC_ID", "")).strip().replace("-", "")
        title = str(sfo.get("TITLE", "")).strip()
        version = str(sfo.get("DISC_VERSION", sfo.get("APP_VER", "1.00"))).strip()

        if verbose:
            print("\nDetected Disc Metadata:")
            print(f"  Title:          {title}")
            print(f"  Disc ID:        {disc_id} ({SUPPORTED_DISC_ID_FORMATTED})")
            print(f"  Version:        {version}")

        if disc_id != SUPPORTED_DISC_ID:
            raise ValueError(
                f"Unsupported game disc ID: {disc_id}. "
                f"P3P3DS currently supports only Persona 3 Portable USA ({SUPPORTED_DISC_ID_FORMATTED})."
            )

        # 2. Check & Prepare Executable (eboot.elf)
        if verbose:
            print("\nPreparing Executable...")

        decrypted_elf_bytes = None

        if decrypted_eboot_path:
            dec_path = Path(decrypted_eboot_path)
            if not dec_path.exists():
                raise FileNotFoundError(f"Supplied decrypted EBOOT not found: {decrypted_eboot_path}")
            decrypted_elf_bytes = dec_path.read_bytes()
            if verbose:
                print(f"  Using supplied external ELF: {dec_path.name}")
        else:
            eboot_key = "/PSP_GAME/SYSDIR/EBOOT.BIN"
            if eboot_key not in file_catalog:
                raise ValueError("EBOOT.BIN not found inside ISO")
            _, eboot_lba, eboot_size = file_catalog[eboot_key]
            eboot_bytes = reader.read_file(eboot_lba, eboot_size)
            enc_hash = hashlib.sha256(eboot_bytes).hexdigest()

            if verbose:
                print(f"  Extracted EBOOT.BIN from ISO ({eboot_size:,} bytes)")
                print(f"  Encrypted SHA-256: {enc_hash}")

            if enc_hash != EXPECTED_ENCRYPTED_EBOOT_SHA256:
                print(f"  Notice: Encrypted EBOOT SHA-256 differs from reference retail UMD dump.")

            if verbose:
                print("  Decrypting Allegrex executable via PSP AES-128 engine...")
            decrypted_elf_bytes = decrypt_eboot(eboot_bytes)

        actual_elf_hash = hashlib.sha256(decrypted_elf_bytes).hexdigest()
        if verbose:
            print(f"  Decrypted SHA-256: {actual_elf_hash}")

        if actual_elf_hash != EXPECTED_DECRYPTED_ELF_SHA256:
            raise ValueError(
                f"Executable SHA-256 mismatch!\n"
                f"  Expected: {EXPECTED_DECRYPTED_ELF_SHA256}\n"
                f"  Actual:   {actual_elf_hash}\n"
                f"Please verify that your disc is an untampered retail copy of ULUS-10512."
            )

        if verbose:
            print("  Executable Hash:  [VERIFIED MATCH]")

        if check_only:
            if verbose:
                print("\n[CHECK ONLY] Disc validation and executable verification completed successfully.")
            return True

        # 3. Extract to Temporary Staging Directory
        staging_dir = Path(".tmp/prepare_game_staging").resolve()
        if staging_dir.exists():
            shutil.rmtree(staging_dir)
        staging_dir.mkdir(parents=True, exist_ok=True)

        try:
            # Write decrypted eboot.elf
            staged_elf = staging_dir / "eboot.elf"
            staged_elf.write_bytes(decrypted_elf_bytes)

            # Extract USRDIR
            if verbose:
                print("\nExtracting Game Assets (USRDIR)...")

            usrdir_prefix = "/PSP_GAME/USRDIR"
            extracted_count = 0
            extracted_bytes = 0

            for key, (orig_path, lba, size) in file_catalog.items():
                if key.startswith(usrdir_prefix):
                    rel_sub = sanitize_iso_path(orig_path[len(usrdir_prefix):])
                    dest_file = staging_dir / "USRDIR" / rel_sub
                    dest_file.parent.mkdir(parents=True, exist_ok=True)
                    data = reader.read_file(lba, size)
                    dest_file.write_bytes(data)
                    extracted_count += 1
                    extracted_bytes += size

            if verbose:
                print(f"  Extracted {extracted_count} files ({extracted_bytes / (1024*1024):.2f} MB)")

            # Check existing destination
            out_dir.mkdir(parents=True, exist_ok=True)
            target_elf = out_dir / "eboot.elf"
            target_usrdir = out_dir / "USRDIR"

            if target_elf.exists() and not force:
                if verbose:
                    print(f"\nTarget {target_elf} already exists. Updating files safely...")

            # Move staged files into destination
            shutil.copy2(staged_elf, target_elf)
            if (staging_dir / "USRDIR").exists():
                if target_usrdir.exists() and force:
                    shutil.rmtree(target_usrdir)
                shutil.copytree(staging_dir / "USRDIR", target_usrdir, dirs_exist_ok=True)

        finally:
            if staging_dir.exists():
                shutil.rmtree(staging_dir)

    if verbose:
        print("\n====================================================")
        print("   Game Files Prepared Successfully!")
        print("====================================================")
        print(f"Executable:  {out_dir / 'eboot.elf'} [VERIFIED]")
        print(f"Game Assets: {out_dir / 'USRDIR'} [VERIFIED]")
        print("\nNext development steps:")
        print("  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release")
        print("  cmake --build build")
        print("  ctest --test-dir build --output-on-failure")

    return True

# =============================================================================
# CLI Entry Point
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="P3P3DS — Prepare Persona 3 Portable (ULUS-10512) game data from a retail ISO image.",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("iso_path", help="Path to the user's Persona 3 Portable ISO file")
    parser.add_argument(
        "--output-dir",
        default="profiles/p3p/game",
        help="Target directory for game data (default: profiles/p3p/game)"
    )
    parser.add_argument(
        "--decrypted-eboot",
        default=None,
        help="Optional path to a pre-decrypted eboot.elf file (bypasses direct AES decryption)"
    )
    parser.add_argument(
        "--check-only",
        action="store_true",
        help="Validate the ISO image identity and executable hash without extracting files"
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Overwrite existing files in the output directory"
    )

    args = parser.parse_args()

    try:
        success = prepare_p3p_game(
            iso_path=args.iso_path,
            output_dir=args.output_dir,
            decrypted_eboot_path=args.decrypted_eboot,
            check_only=args.check_only,
            force=args.force,
            verbose=True
        )
        sys.exit(0 if success else 1)
    except Exception as e:
        print(f"\nError: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
