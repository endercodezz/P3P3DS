# Decryption — the phase 2 gate

Every executable module on a retail PSP disc is encrypted. This is the one
thing standing between the phase-1 tooling and real Allegrex code, and it is
the direct analogue of the Lynx's encrypted boot block: a well-documented,
publicly reverse-engineered scheme that is a task rather than a research
project.

## What we are looking at

`BOOT.BIN` — nominally the plain ELF — is **zeroed** on retail discs. Confirmed
on *WTF: Work Time Fun* (ULUS10172): 1,224,764 bytes of `0x00`. The real module
is `EBOOT.BIN`, a `~PSP`-wrapped encrypted PRX.

This is not selective. On the WTF disc, **all 28 executable modules** are
encrypted:

- `SYSDIR/EBOOT.BIN` — `~PSP`, module `hell2k` (the game's dev codename)
- all 20 `USRDIR/module/*.prx` — `~SCE` wrapping `~PSP`
- all 8 `USRDIR/kmodule/*.prx` — `~PSP`
- the `DATA.PSP` inside each of the five game-sharing PBPs

## The header tells us what we need

The `~PSP` header is **plaintext** even though the body is not, which is why
phase 1 can report exactly what phase 2 has to do. From the five WTF
game-sharing modules:

| Module | Title | Decrypted size | Decrypt mode | Key tag |
|---|---|---:|---:|---|
| `EBOOT.BIN` | (main executable, `hell2k`) | 1,224,764 | 9 | `0xC0CB167C` |
| `b00_bootbin.dat` | Baseball Superstar | 4,212,342 | 10 | `0x09000000` |
| `b02_bootbin.dat` | Lumberjack | 4,104,742 | 10 | `0x09000000` |
| `b04_bootbin.dat` | Pendemonium | 4,393,310 | 10 | `0x09000000` |
| `b80_bootbin.dat` | Lumberjack Challenge | 4,092,606 | 10 | `0x09000000` |
| `b81_bootbin.dat` | Séance | 4,725,526 | 10 | `0x09000000` |

### A correction worth recording

An earlier revision of this document listed *five different* tags for the five
game-sharing modules, read from header offset `0x130`. **That was wrong**, and
the way it was wrong is instructive.

`0x130` lands inside the encrypted key material. Every module therefore yields
a different, perfectly plausible-looking 32-bit value there — which reads
exactly like a per-module tag and even supports a satisfying conclusion ("five
distinct tags, so the key table gets exercised immediately"). It was noise.

The tag is at **`0xD0`**. Cross-checked against an independent decryptor, `0xD0`
reproduces the tag it reports for both a mode 9 and a mode 10 module, and
`0x130` matches neither. The real picture is the opposite of the earlier one:
all five microgames share a single tag and differ only from the main
executable.

The lesson is the one this project keeps relearning — a plausible reading of a
binary is not a verified one. The check that caught it was running an
independent implementation and comparing, which is exactly what
[`ORACLE.md`](ORACLE.md) argues for.

## KIRK

Decryption runs through the PSP's KIRK crypto engine. For game modules the
relevant operation is **CMD1** (decrypt-and-verify).

The work splits cleanly into two halves, and the first is done.

### Phase 2a — the crypto core and KIRK CMD1 ✅

**AES-128/192/256** (`crypto/aes.c`), self-contained, no dependency. The S-box
is *derived* at init from its algebraic definition (multiplicative inverse in
GF(2⁸), then the affine transform) rather than pasted in as a 256-byte table —
a mistyped constant in a transcribed table produces an implementation that
fails only for certain inputs, which is exactly the kind of bug that survives
casual testing.

**AES-CMAC** (`crypto/cmac.c`), RFC 4493.

Both are validated against the primary standards before they ever see a game
module: FIPS-197 Appendix C for the block cipher at all three key sizes,
NIST SP 800-38A F.2 for CBC, RFC 4493 for CMAC. `ctest` target `crypto`.

**KIRK CMD1** (`crypto/kirk.c`) — the full decrypt-and-verify path:

1. AES-CBC-decrypt header bytes `0x00`–`0x1F` with the KIRK1 key, IV = 0. This
   is one 0x20-byte run, so the block at `0x10` chains against the *ciphertext*
   at `0x00`. Yields the body AES key and the CMAC key.
2. CMAC the 0x30-byte metadata block at `0x60`; compare against `0x20`.
3. CMAC the metadata + padding + still-encrypted body; compare against `0x30`.
   This authenticates the ciphertext, so it is computed *before* decryption.
4. AES-CBC-decrypt the body at `0x90 + padding`, IV = 0.

The CMACs are why this was worth building carefully. **They turn "did the key
work?" from a judgement call into a yes/no answer** — and they distinguish the
two failure modes: a wrong KIRK1 key fails at step 2, while a correct key with
a corrupt body fails at step 3. Both are separately tested.

The whole path is verified end to end by `ctest` target `kirk`, using synthetic
CMD1 blobs built with arbitrary test keys. **No PSP key material is needed for
this**, because CMD1's structure does not depend on which key you feed it — a
round trip under a test key proves the algorithm exactly as well as a round
trip under Sony's would. That property is the reason CMD1 was worth
implementing before the key question was settled.

### Phase 2b — the `~PSP` tag layer (remaining)

Above CMD1 sits a per-tag transform: the `~PSP` header's tag at offset `0x130`
selects a key set, which is used to construct the CMD1 header that CMD1 then
consumes.

**We measured rather than assumed.** `allegrexrecomp decrypt` scans a module
for an embedded CMD1 metadata block — a heavily constrained ~100-bit signature
(mode exactly 1, signature flag 0 or 1, a retail/devkit word that is 0 or
all-ones, 0x18 bytes of mandatory zeros, a plausible data size). Run against
every module on the WTF disc, it finds **nothing**:

```
$ allegrexrecomp decrypt b02_bootbin.dat
module:   boot_bin
tag:      0x4597CB4E   decrypt mode 10
sizes:    4105088 encrypted -> 4104742 decrypted

probing for a KIRK CMD1 metadata block...
  none found in the first 0x400 bytes
```

That is a useful negative: the CMD1 header is **not** sitting in the `~PSP`
header in plaintext waiting to be pointed at. It is *constructed* by the tag
layer, which means that layer does real cryptographic work (a key-derivation
step and a scramble) rather than being a memcpy at a fixed offset. Guessing an
offset would have produced silently-wrong output; the probe rules that out.

Also learned from the same run: **the main executable uses decrypt mode 9,
while all five game-sharing modules use mode 10.** The mode selects which
variant of the transform applies, so WTF exercises at least two of them.

What remains for 2b:

- [ ] The tag → key-set table, loaded from the external key file.
- [ ] The mode 9 and mode 10 transforms that build a CMD1 header from a `~PSP`
      header. This needs the reference algorithm; it is intricate enough that
      approximating it from memory would produce plausible garbage, and the
      CMACs would (correctly) reject it without telling us which step was wrong.
- [ ] Segment reassembly into a valid ELF32 using the header's segment table.

## Keys are never bundled

**No key material is distributed with this toolkit.** Keys load from an external
file — `--keys <path>`, then `$PSPRECOMP_KEYS`, then `./keys/psp_keys.txt`,
which is gitignored. See [`psp_keys.example.txt`](psp_keys.example.txt) for the
format.

The PSP's constants have been public since the console's active life and are
published facts, in the same sense that the Lynx boot ROM's RSA modulus is a
published public key — but they are not this project's to redistribute. This is
the same arrangement `lynxrecomp` has with the Lynx boot ROM.

Keeping keys as data rather than baked-in constants is also better engineering:
a wrong key produces a named diagnostic (`key 'kirk1' not found in
keys/psp_keys.txt`) instead of a mysterious failure, and a correction needs no
rebuild.

## The pragmatic unblock

Phase 2b is not on the critical path for *recompilation*. Phase 3 — function
discovery and the C emitter, where the actual recompilation value is — needs
**plaintext ELF**, not necessarily *our* decryptor.

[`pspdecrypt`](https://github.com/John-K/pspdecrypt) (GPL-3.0) produces
plaintext today. Used as a separate process it is an external tool, exactly
like PPSSPP-as-oracle or a headless Ghidra run — nothing is linked, nothing is
copied, and the toolkit stays MIT. That path unblocks phase 3 immediately while
2b is built properly.

Building our own decryptor is about being **self-contained**, which matters
eventually — a one-command pipeline from disc to native executable is the goal
— but it is not what gates progress.

## How we will know it worked

Decryption is the kind of thing that can appear to succeed and produce
plausible garbage, so the check is explicit and layered — each step would catch
a failure the previous one might not:

1. **Declared size.** The reassembled ELF must be exactly `elf_size` bytes, the
   value the plaintext header declares. A wrong key gives a wrong length.
2. **It parses.** The phase-1 ELF parser must accept it, with a sane
   `e_type` (2 or `0xFFA0`) and at least one `PT_LOAD`.
3. **The entry point lands in code.** `e_entry` must fall inside a `PF_X`
   segment.
4. **The opcode histogram is code-shaped.** `allegrexrecomp cover` over `.text`
   must show the distribution real MIPS text has — `lw`, `sw`, `addiu`, `nop`,
   `jr` dominant, and a high decode rate. Random bytes decode as MIPS at a
   surprisingly high rate, but they produce a *flat* histogram; real compiled
   code does not. This is the check that catches a subtly-wrong key.
5. **Cross-check.** The same module decrypted by an independent implementation
   must be byte-identical. `lynxrecomp` validated its RSA boot decrypt the same
   way, against an independent Python implementation.

## What this does not cover

`DATA.PSAR` — the packed payload in PSN and minis releases — uses a separate
(and for signed PSN content, differently-keyed) scheme. WTF's UMD and PSN
releases both put the game in `EBOOT.BIN`/`DATA.PSP`, so PSAR handling is not
on the critical path and is deferred until a title needs it.
