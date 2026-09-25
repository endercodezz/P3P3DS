# PSP hardware

What a recompiler has to model, and how the work splits between "translate to
C" and "emulate as a peripheral".

## The split

The same split every recomp project makes: **the CPU is translated, everything
else is implemented.**

| Component | Treatment |
|---|---|
| Allegrex CPU (integer, FPU) | **Recompiled** to native C |
| VFPU | **Recompiled**, but see [`VFPU.md`](VFPU.md) |
| `sceKernel` / `sceIo` / `sceCtrl` / `sceAudio` | **HLE** — implemented as C library functions |
| GE (the GPU) | **Emulated** as a peripheral — a display-list processor |
| Memory | A flat host allocation behind a masking address decoder |

The PSP is unusual in how *far* the HLE line can be pushed. Games do not poke
hardware registers; they call firmware entry points through a documented module
import table. That surface is a library, and a library can be reimplemented.
A complete MIT-licensed reverse engineering of it already exists
([uofw](https://github.com/uofw/uofw)), which is what makes phase 4 a writing
job rather than a research job.

## Allegrex

A MIPS32r2 core at 222 MHz (333 MHz on later firmware). What matters:

- **32 general-purpose registers**, `$zero` hardwired. No 64-bit integer ops —
  the MIPS III/IV doubleword instructions are absent, so a decoder can treat
  their encodings as invalid rather than as something to implement.
- **HI/LO** for multiply and divide, with `madd`/`msub` accumulate forms.
- **Branch delay slots.** Every branch and jump executes the following
  instruction before control transfers. "Likely" branches (`beql`, `bnel`, …)
  nullify their delay slot when *not* taken. This is the single most
  error-prone thing in MIPS recompilation.
- **COP1**: an FPU with 32 registers, **single precision only**. No `.d` format.
- **COP2**: the VFPU.
- **No TLB.** Addresses decode by their top bits into fixed regions.

### Allegrex divergences from stock MIPS32

A decoder written from the generic MIPS32 manual gets all of these wrong, and
each one is pinned by a unit test in `tests/test_decode.c`:

| Instruction | Stock MIPS32 | Allegrex |
|---|---|---|
| `clz` / `clo` | SPECIAL2 | **SPECIAL** `0x16` / `0x17` |
| `madd` / `maddu` | SPECIAL2 | **SPECIAL** `0x1C` / `0x1D` |
| `msub` / `msubu` | SPECIAL2 | **SPECIAL** `0x2E` / `0x2F` |
| `max` / `min` | — | **SPECIAL** `0x2C` / `0x2D` (Allegrex-only) |
| `bitrev` | — | **SPECIAL3** BSHFL, `sa = 0x14` |
| `wsbw` | — | **SPECIAL3** BSHFL, `sa = 0x03` |
| `mfic` / `mtic` | — | **COP0** `rs = 0x0B` |

Two more that are easy to miss because they are not new opcodes, just
overloaded ones:

- **`rotr` is `srl` with `rs == 1`**, and `rotrv` is `srlv` with `sa == 1`.
  Miss it and every rotate silently becomes a shift — a bug that only surfaces
  in hash and checksum code, long after you stop looking for it.
- **The BSHFL group writes `rd` from `rt`**, not from `rs`. `rs` is unused.

## Memory map

No TLB, so this is the whole of address translation:

| Range | Size | Contents |
|---|---|---|
| `0x00010000`–`0x00013FFF` | 16 KB | Scratchpad (fast on-chip) |
| `0x04000000`–`0x041FFFFF` | 2 MB | VRAM |
| `0x08000000`–`0x09FFFFFF` | 32 MB | Main RAM |

Each region is mirrored at three segment bases that differ only in cache
behaviour — `0x0…` cached, `0x4…` uncached, `0x8…` kernel-cached. Since we do
not model the cache, all three fold onto one backing store by masking with
`0x3FFFFFFF`. A game that writes through the uncached mirror and reads back
through the cached one gets the right answer for free, which on real hardware
it would not.

On a PSP-1000 only the first 24 MB of main RAM is present, and user memory
starts at `0x08800000` (the kernel occupies `0x08000000`–`0x087FFFFF`). We
allocate the full 32 MB because the later models have it and a game that never
touches the top 8 MB costs nothing to over-allocate.

`psp_mem_ptr()` rejects an access that straddles the end of a region rather
than reading past the allocation, and unmapped accesses are **counted** in
`psp_mem_bad_access` rather than silently returning zero. A recompiled game
should reach a steady state with that counter flat; when it climbs, the
analysis missed something.

## The GE (Graphics Engine)

A fixed-function GPU driven by a **display list** — a command stream of 32-bit
words, each an 8-bit command and 24 bits of argument, built by the `sceGu`
library in user code and submitted to the GE.

This is the same shape as the Lynx's Suzy or the N64's RDP: not something to
recompile, something to implement as a peripheral that consumes a documented
command stream. The `sceGu` calls that *build* the list are ordinary user code
and get recompiled like anything else; only the list *execution* is emulated.

Notable for a recompiler: the GE reads directly from main RAM and VRAM, so
display-list and texture memory must be coherent with whatever the recompiled
code wrote. With one flat backing store and no cache model, they are.

## Audio

Two paths: `sceAudio` for raw PCM output, and `sceSas` — a hardware voice mixer
with 32 channels, ADPCM decoding, ADSR envelopes and a reverb unit. `sceSas` is
HLE'd as a mixer; the ATRAC3+ codec behind `libatrac3plus.prx` is a separate
problem and, for most titles, only affects music.
