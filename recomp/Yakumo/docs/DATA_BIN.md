# MHP3rd `DATA.BIN` archive format

`/PSP_GAME/USRDIR/DATA.BIN` (1,208,858,624 bytes on NPJB-40001) is the read-only
archive that the game's `fakeRofsLoader` thread serves every asset and every code
overlay from. It is obfuscated, not cryptographically encrypted: the whole
archive, every code overlay included, can be read offline.

The layout below was recovered from the disc image itself, cross-checked against
the loader in the executable and against overlay images captured from guest
memory. The general shape of the cipher (an XOR keystream seeded from the 2 KiB
block address, followed by a byte substitution) is also documented by the
community in [svanheulen/mhef](https://github.com/svanheulen/mhef/wiki/DATA.BIN-Encryption).
The constants and the substitution table used here were derived independently by
a known-plaintext comparison, so no third-party source is involved.

## Obfuscation

Every entry is transformed with the same size-preserving two-stage scheme. The
keystream is seeded per entry from the **2 KiB block address at which the entry
starts**, which is why two entries whose block addresses share a high halfword
show a visibly correlated ciphertext.

Encryption, over an entry padded to a multiple of 4 bytes:

1. **XOR keystream.** Two independent 16-bit Lehmer generators, one per halfword
   of a little-endian 32-bit word. With `lba` the entry's block address:

   ```
   k0 = lba >> 16      (or 0x2345 when zero)
   k1 = lba & 0xFFFF   (or 0x7F8D when zero)

   per 32-bit word, stepped before use:
     k0 = (k0 * 0x2345) % 0xFFD9
     k1 = (k1 * 0x7F8D) % 0xFFF1
     word ^= (k0 << 16) | k1
   ```

   Both moduli are prime, so each generator is purely periodic and the keystream
   can be generated once per period and tiled.

2. **Byte substitution.** A fixed 256-entry bijection is applied to every byte of
   the XOR result. The table is `ENCODE_TABLE` in
   `profiles/mhp3rd/tools/databin.py`; decryption applies its inverse first and
   then the same XOR keystream.

The archive directory is obfuscated with block address 0, i.e. with the default
seeds.

Sixteen entries are stored verbatim with no obfuscation at all. On this disc
those are entries 17–20, plain `~SCE` PRX stubs, and entries 104–115, the
twelve `PSMF` movie streams. They are recognised by their magic rather than by
a hard-coded index list; deobfuscating them anyway turns them into noise.

## Directory

The directory occupies the first blocks of the archive and contains two tables.

| Region | Bytes on NPJB-40001 | Contents |
| --- | --- | --- |
| Block table | `0x0000` – `0x5E70` (24,176) | `uint32[6044]` block addresses |
| Size table | `0x5E70` – `0x86B8` (34,488) | `(uint32 index, uint32 size)[1289]` |
| Trailer | `0x86B8` – `0x8800` (34,816) | 328 unidentified high-entropy bytes |

* `word[0]` is the size of the whole directory **in 2 KiB blocks** (17 here, so
  34,816 bytes) and is simultaneously the block address of entry 0.
* The block table is monotonically non-decreasing. Entry *n* occupies blocks
  `table[n] .. table[n+1]`, so its block-aligned length is
  `(table[n+1] - table[n]) * 2048`. The table is terminated by the block count of
  the whole archive (590,263), which gives **6,043 entries**.
* The size table follows immediately and gives the exact byte length of the
  entries whose true length is not block aligned — 1,289 of them, sorted by
  index. Entries absent from it are exactly block sized.

This matches the loader's observed I/O exactly: it reads 24,176 bytes at offset 0
(the block table) and then 10,312 bytes at offset 24,176 (the size table). It
never reads the 328 bytes that follow, which do not decrypt to anything
recognisable; they are most likely the material the game's `sha1Thread` checks.
Nothing in extraction depends on them.

There are no file names in the directory. Entries are addressed by index; the
only names in the archive are the ones inside overlay headers.

By first four bytes, the 6,043 entries break down roughly as: 1,467 `03 00 00 00`,
838 `5B 03 C3 AA`, 703 all-zero, 699 `Head`, 454 `04 00 00 00`, **355 `MWo3`
(overlays)**, 255 `RIFF`, 101 `dbsT`, 79 `GIF8`, 12 `PSMF`, and a long tail.

The 255 `RIFF` entries are the streamed music and all carry `wFormatTag`
`0x270`: baseline ATRAC3, not ATRAC3+. The 12 `PSMF` entries are the movies,
which hold H.264 video with their own audio track.

## Overlay entries

An overlay is an archive entry beginning with the magic `MWo3` and a 64-byte
header:

| Offset | Type | Meaning |
| --- | --- | --- |
| `0x00` | char[4] | `MWo3` |
| `0x04` | uint32 | overlay id, a dense 1-based index (1…355) |
| `0x08` | uint32 | guest load address |
| `0x0C` | uint32 | code size |
| `0x10` | uint32 | data size |
| `0x14` | uint32 | bss size |
| `0x18` | uint32 | guest address at (or 128 bytes below) the end of the image |
| `0x1C` | uint32 | guest address, usually `[0x18] + 4` |
| `0x20` | char[32] | NUL-terminated name, e.g. `demo_task.ovl` |

The entry's byte length is exactly `64 + code + data`; this agrees with the
directory's size table for all 355 overlays. The whole entry, header included, is
loaded at the load address, so the header is visible in guest memory at the slot
address.

The disc contains 355 overlays spread over 12 distinct load addresses, which are
the 12 slots the host tracks:

| Slot | Overlays |
| --- | --- |
| `0x0A001780` | 2 |
| `0x0A055E80` | 154 |
| `0x0A05E600` | 11 |
| `0x0A1BB000` | 44 |
| `0x0A1EFE80` | 43 |
| `0x0A224D00` | 18 |
| `0x0A239780` | 18 |
| `0x0A24E200` | 11 |
| `0x0A25BA80` | 11 |
| `0x0A269300` | 11 |
| `0x0A276B80` | 12 |
| `0x0A284400` | 20 |

The names and load addresses match the 355 `*.ovl` sections in `EBOOT.ELF`
one-for-one, with no mismatches. (The executable also carries a `.rel<name>.ovl`
section per overlay; relocation data lives in the executable, not in the archive,
and nothing in the overlay image is patched at load time — see below.)

## Tool

`profiles/mhp3rd/tools/databin.py` implements the directory and the cipher. It
accepts either `DATA.BIN` itself or a PSP ISO, in which case it locates
`/PSP_GAME/USRDIR/DATA.BIN` inside the image and reads it in place — no 1.2 GB
extraction step and no need to hold the archive in memory.

```sh
ISO=game.iso

# every entry, with overlay names and load addresses decoded
python3 profiles/mhp3rd/tools/databin.py "$ISO" list
python3 profiles/mhp3rd/tools/databin.py "$ISO" list --overlays --csv

# all 355 overlays, named overlay_<LOADADDR>_<name>.bin (~15 MB, a few seconds)
python3 profiles/mhp3rd/tools/databin.py "$ISO" extract-overlays out/overlays

# individual entries, or one entry to stdout
python3 profiles/mhp3rd/tools/databin.py "$ISO" extract out/ 116 117 125
python3 profiles/mhp3rd/tools/databin.py "$ISO" cat 125 | xxd | head

# compare an entry against an overlay image captured from guest memory
python3 profiles/mhp3rd/tools/databin.py "$ISO" verify 117 dump/overlays/overlay_0A05E600.bin
```

`verify` splits the comparison at the end of the code section, because the data
section is written to at run time and a memory capture will differ there.

## Verification against memory captures

| Entry | Overlay | Result |
| --- | --- | --- |
| 117 | `edit_task.ovl` @ `0x0A05E600` | **0 differing bytes** over the whole 81,416-byte capture |
| 125 | `demo_sub.ovl` @ `0x0A001780` | header + code (`0x0` – `0x4CBC`) identical; 79 bytes differ in the data section |

So the archive image is the load image: nothing is decompressed, expanded or
relocated on the way into the slot. The only divergence from a memory capture is
in the data section, where the running game has written to its own variables.

Extracting all 355 overlays takes about 4 seconds and produces 15 MB.

## How the game reads it

Traced with `MHP3RD_TRACE_IO=1` (and the caller's return address, added for the trace) from start-up to the title screen:

- The game opens `disc0:/PSP_GAME/USRDIR/DATA.BIN` once and keeps it open. It reads the block table (24,176 bytes at offset 0) into `0x08B42A80` and the size table (10,312 bytes at offset 24,176) into `0x08B40200`, once, and asks `sceIoGetstat` for the archive's size.
- Every other read is a whole entry or a piece of one, never two entries at once. The `fakeRofsLoader` thread seeks to the entry's first block and reads it in pieces of at most 128 KiB into a bounce buffer at `0x08B48900` (the read returns to `0x08865514`), decrypts and copies each piece to the file's destination (`0x08863664`), and marks the request done by storing 1 through the request's done pointer (`0x088657FC`). Movies are streamed straight into their buffers in 64 KiB pieces (from `0x08863F6C`).
- A read ends exactly at the size the tables give (127,216 bytes, 24,432 bytes), and consecutive entries are read without a seek between them only when the next one starts where the last ended. The game trusts the two tables and nothing else about a file's size: an entry made larger in both tables is read in full and used.
- The raw sector file `disc0:/sce_lbn0x115fa_size0x50d0` the game opens at start is entry 17, a module stub inside `DATA.BIN` (`0x115FA` is block 15,162 of the archive plus the archive's first sector, 56,000).
- After copying a code overlay into its slot, the game flushes the instruction cache. That is the point at which an overlay has finished loading.

## File ids and mods

The game's mod community numbers the files of `DATA.BIN` by their entry index, written as four upper-case hex digits: `0601` is entry 1537. The community's file lists count the directory as file 0 and the entries from 1, so their file names (`1490.pak`) are one more than the id (`05D1`) in decimal. On NPJB-40001, entry `05D1` is indeed the first model archive after the weapon overlays, as those lists have it.

Do not trust those lists' HD ids for armour without checking. Traced on NPJB-40001 with a male hunter wearing nothing, the game reads head `0503` and waist `0409`, which the male armour list gives, but body `034B`, arms `03C8` and legs `04C2`, where the list gives `030F`, `038C` and `0486`. `MHP3RD_TRACE_DATA_BIN=1` (with any mod on) logs every file the game reads, which is how to find the right id: change the equipment on screen and note the new ids.

Yakumo serves mods' files through the file I/O (`profiles/mhp3rd/host/mods/`). A replaced or patched entry is encrypted for the block it starts at. An entry that no longer fits its blocks grows; every entry after it then starts that many blocks later, and its bytes are re-keyed from its block on the disc to its new block as they are read (decrypt with the one keystream, encrypt with the other; verbatim entries are served as they are). The block table and the size table the game reads say the same. The size table has a fixed number of rows (the game reads exactly 10,312 bytes), so an entry keeps or lacks its row as on the disc; a new size that is not a whole number of blocks for an entry without a row is padded with zeros to its blocks. The trailer after the tables is served unchanged.
