# The VFPU

The PSP's vector unit is the reason people give for why the PSP has never had a
static recompiler. It is genuinely awkward. It is also, importantly,
**localized** — and this document exists partly to make that claim measurable
rather than rhetorical.

## What it is

COP2 on the Allegrex: a 128-register single-precision vector unit. The register
file is 128 floats, but it is *addressed* as 8 matrices of 4×4, and a single
7-bit register field can name a scalar, a 2/3/4-element row, a 2/3/4-element
column, or a whole matrix depending on bits elsewhere in the instruction.

Three things make it hard to decode:

1. **Split width encoding.** The number of lanes an instruction operates on is
   `((op >> 7) & 1) | ((op >> 14) & 2)`, giving 0–3 for 1–4 lanes. Two
   non-adjacent bits, neither adjacent to the opcode. Decode this wrong and a
   quad operation writes one lane instead of four.
2. **Prefix instructions.** `vpfxs` / `vpfxt` / `vpfxd` do not compute anything;
   they set a register that *modifies the operands of the next instruction* —
   swizzling lanes, negating, forcing constants, masking writes. An instruction's
   meaning depends on what came before it, which is exactly what a
   one-instruction-at-a-time decoder is bad at.
3. **A sprawling opcode space.** The VFPU occupies most of opcodes `0x18`,
   `0x19`, `0x1B`–`0x1E`, `0x32`–`0x3F`, with sub-opcodes in several different
   bit positions depending on the family.

## What we decode today

Named and decoded:

- **Load/store** — `lv.s` (`0x32`), `lv.q` (`0x36`), `sv.s` (`0x3A`),
  `sv.q` (`0x3E`)
- **COP2 moves and branches** — `mfv`, `mtv`, `mfvc`, `mtvc`, `bvf`, `bvt`,
  `bvfl`, `bvtl`
- **VFPU0** (`0x18`) — `vadd`, `vsub`, `vdiv`
- **VFPU1** (`0x19`) — `vmul`, `vdot`, `vscl`, `vhdp`, `vcrs`, `vdet`
- **VFPU3** (`0x1B`) — `vcmp`, `vmin`, `vmax`, `vscmp`, `vsge`, `vslt`
- **Vector width**, for all four widths, unit-tested

Everything else in the VFPU encoding space decodes to `A_VFPU_UNKNOWN`.

## Why "unknown" is a distinct result

`A_VFPU_UNKNOWN` is deliberately *not* `A_INVALID`. The difference matters:

- `A_INVALID` means "this is not an instruction" — probably data misread as
  code, and the analyzer should stop following this path.
- `A_VFPU_UNKNOWN` means "this **is** a VFPU instruction and we know it, but we
  cannot yet say which one." The emitter must refuse to emit rather than
  guessing, and the analyzer should keep going.

Conflating them would mean a game's VFPU-heavy maths library looks like data,
function discovery stops at its first vector instruction, and the resulting
coverage number looks *better* than reality because the code was never counted.

## Measuring before committing

`allegrexrecomp cover <module>` reports three numbers over a module's `.text`:
decoded, VFPU-recognised-but-unnamed, and unknown. That third number is decoder
bugs or data; the second is the honest size of the VFPU problem *for that
specific title*.

This is the point of the whole approach. "The PSP is hard because of the VFPU"
is an assumption. A 2D microgame that never builds a projection matrix may use
almost none of it, while a 3D engine will use a great deal. Running `cover`
across a corpus turns the question into a sorted list of which titles are cheap
targets — which is phase 6, and which is why `cover` exists in phase 1 rather
than being deferred until the emitter needs it.

## Plan

VFPU completion is deliberately **demand-driven**, not front-loaded. Rather
than implementing 300 vector instructions speculatively, the order is:

1. Decrypt a corpus (phase 2).
2. Run `cover` across it; rank titles by VFPU density.
3. Implement the families that the chosen title actually uses.
4. Validate each against [pspautotests](https://github.com/hrydgard/pspautotests),
   which has hardware-verified VFPU coverage including the prefix behaviour.

The prefix instructions are the part that will need real design work, because
they break the one-instruction-one-statement model the rest of the emitter
uses. The likely shape is emitting prefix state as explicit local variables
that the following instruction's generated C reads — which keeps the output
readable and keeps the weirdness visible rather than hidden in a helper.
