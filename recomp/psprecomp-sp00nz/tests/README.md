# Tests

Self-contained — **no game data**. There is no disc image, no `EBOOT.BIN`, and
no PRX module in this repository or in these tests. Every input is either an
instruction encoding written by hand from the MIPS32r2 manual, or a buffer the
test builds itself.

```powershell
ctest --test-dir build -C Release --output-on-failure
```

| Suite | Covers |
|---|---|
| `test_decode` | the Allegrex decoder |
| `test_runtime` | the semantic helpers and the memory map |
| `test_container` | the ISO9660 / PBP / `~PSP` / ELF / SFO parsers |
| `test_crypto` | AES and AES-CMAC, against published standard vectors |
| `test_kirk` | KIRK CMD1 decrypt-and-verify |

**No key material either.** `test_crypto` uses only published standard vectors
(FIPS-197, NIST SP 800-38A, RFC 4493), and `test_kirk` builds its own CMD1
blobs with arbitrary keys chosen in the test file. Neither needs a PSP key, and
neither would be more convincing with one — CMD1's structure does not depend on
which key it is given, so a round trip under a test key proves the algorithm
exactly as well as a round trip under Sony's would.

## What each suite is actually protecting

**`test_decode`** does not try to cover 256 opcodes. It covers the ones that
carry real risk:

- The function prologue/epilogue pattern every MIPS compiler emits — if
  `addiu $sp` / `sw $ra` / `lw $ra` / `jr $ra` are wrong, nothing else matters.
- **Branch and jump target arithmetic**, including a backward branch (the form
  that closes a loop) and the `j`/`jal` case where the top 4 bits come from the
  *delay slot's* address, not the instruction's. That difference only shows up
  at a 256 MB boundary, which is exactly where it would go unnoticed.
- **Every Allegrex divergence from stock MIPS32** — `clz`/`clo` in SPECIAL,
  `max`/`min`, `madd`/`msub`, the BSHFL group, `rotr` as `srl` with `rs == 1`.
  A decoder written from the generic manual gets all of them wrong.
- The operand-order cases: BSHFL writes `rd` from `rt`, not `rs`.
- Immediate sign- vs zero-extension in **both** directions, since getting it
  backwards silently corrupts constants.
- VFPU vector width across all four widths, and that VFPU encodings never
  decode as integer instructions.

**`test_runtime`** covers the instructions whose C translation is *not*
obvious — the set where a wrong answer hides for months:

- Divide by zero and `INT_MIN / -1` (both UB in C, both defined on MIPS).
- The `lwl`/`lwr` and `swl`/`swr` unaligned pairs, tested as the compiler
  actually emits them and checked byte-by-byte, including that they leave
  neighbouring bytes alone and degrade to a plain `lw` when aligned.
- Shift-amount masking (C leaves shift-by-32 undefined; MIPS masks to 5 bits).
- That `$zero` stays zero when written.
- That the three cache mirrors alias one backing store, and that an access
  straddling the end of a region is rejected rather than reading past the
  allocation.

**`test_container`** is mostly about *rejection*. These parsers are pointed at
untrusted dumps, so every one is checked for refusing bad magic, truncated
input, absurd counts, and — the case a corrupt dump actually hits — a
program-header offset pointing past the end of the file.

**`test_crypto`** checks against the primary sources rather than against
another implementation, so a shared misunderstanding cannot pass. The CMAC
cases deliberately include lengths 0, 16, 40 and 64: 0 and 40 take the
`0x80`-pad / K2 branch while 16 and 64 take the K1 branch, so getting the
branch condition backwards passes one pair and fails the other.

**`test_kirk`** pins the property that makes the whole decryption effort
tractable: the two failure modes stay distinguishable. A single wrong key *bit*
must fail at the **header** CMAC, and a correct key with one flipped body bit
must fail at the **data** CMAC. If those ever collapse into one indistinct
failure, "is my key right?" stops being answerable and decryption bring-up
turns into guesswork. It also covers nonzero header padding — real modules use
it and it participates in the body CMAC, so an implementation that assumes zero
passes every other test here and then fails on the first real module.
