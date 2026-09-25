# The recompiler

How `allegrexrecomp` works, and what the generated C is supposed to look like.

## One decoder, four consumers

The decoder (`tools/allegrexrecomp/decode.c`) is used by the disassembler, the
analyzer that discovers function boundaries, the C emitter, and — from phase 5 —
the interpreter that acts as the bring-up oracle.

They all call `a_decode()`. This is a deliberate constraint, not an accident of
structure: if the emitter and the oracle disagree about a game, the bug is in
exactly one of them, and never in two divergent copies of the decode logic.
The moment a project grows a second decoder, every divergence becomes a
three-way question.

`a_decode()` fills an `a_insn` with the opcode, the operand fields, and a set of
**analysis flags** the later passes need without re-deriving them:

| Flag | Meaning |
|---|---|
| `is_branch` | conditional; falls through if not taken |
| `is_jump` | unconditional |
| `is_call` | writes `$ra` (`jal`, `jalr`, the `*al` REGIMM forms) |
| `is_indirect` | target comes from a register |
| `is_return` | **`jr $ra` specifically** — the function-boundary signal |
| `is_likely` | `*L` form; delay slot nullified when not taken |
| `has_delay_slot` | the next instruction executes first |
| `ends_block` | control leaves here unconditionally |

`is_return` being distinct from `is_indirect` matters more than it looks:
`jr $ra` ends a function, `jr $v0` is a computed jump in the middle of one.
Treating them alike either truncates every function at its first jump table or
never closes one at all.

## Phase 1: what exists

```
allegrexrecomp info    <file>    identify and describe any layer of the stack
allegrexrecomp ls      <file>    list a disc image
allegrexrecomp extract <file> <match> <dir>
allegrexrecomp dis     <file> [addr] [count]
allegrexrecomp cover   <file>    decode-coverage report
```

`info` recurses one level, so a game-sharing PBP reports its inner module's
name, size and key tag in a single invocation. `dis` and `cover` accept an ELF
(decoding its `PF_X` segment), a PBP (reaching through to `DATA.PSP`), or a raw
binary (defaulting the load base to `0x08804000`).

### `cover` is a correctness instrument

`cover` reports decoded / VFPU-unnamed / unknown percentages plus an opcode
histogram. It is not a status readout; it is how three different questions get
answered:

- **Is the decoder complete?** A high `unknown` on a known-good module is a
  decoder gap.
- **Did decryption work?** Real compiled MIPS has a characteristic histogram —
  `lw`, `sw`, `addiu`, `nop` dominant. Random bytes decode as MIPS at a
  surprisingly high rate, but the histogram comes out *flat*. This distinguishes
  "decrypted correctly" from "decrypted to plausible garbage" better than a
  checksum does.
- **How expensive is this title?** The VFPU percentage is the honest cost
  estimate for a given game (see [`VFPU.md`](VFPU.md)).

## Phase 3: the emitter

One C function per discovered routine, named `psp_func_<addr>`, with every line
carrying its address and disassembly as a comment — the same convention
`lynxrecomp` and `tirecomp` use, and for the same reason: the output is meant
to be *read*, not just compiled. Recomp projects live or die on whether a human
can open the generated file and understand what the original was doing.

Intra-function control flow becomes labels and `goto`. `jal` to a known address
becomes a direct C call. Indirect jumps go through a dispatch table. Anything
whose target cannot be resolved becomes a runtime hook rather than a guess.

### The delay slot problem

This is the hard part of MIPS recompilation and it deserves naming up front.

Every branch and jump executes the instruction *after* it before control
transfers. So this:

```
    beq   $a0, $a1, target
    addiu $v0, $v0, 1        # executes whether or not the branch is taken
```

does not translate to `if (a0 == a1) goto target;` followed by the `addiu`. The
`addiu` happens first — or rather, it happens in both paths. And for a *likely*
branch (`beql`), it happens only when the branch **is** taken.

Three cases, three treatments:

| Form | Delay slot behaviour | Emitted as |
|---|---|---|
| ordinary branch | always executes | slot hoisted above the `if` |
| likely branch | executes only if taken | slot duplicated inside the taken path |
| jump / call | always executes | slot hoisted above the transfer |

The nasty sub-case is a delay slot that *writes a register the branch reads*:

```
    beq   $a0, $zero, target
    addiu $a0, $a0, 1        # the branch already read the OLD $a0
```

Hoisting naively changes the comparison. The emitter reads the branch condition
into a temporary before the hoisted slot, so the comparison uses the value the
hardware would have used. This gets its own test suite, because it is silent
when wrong and it is the first thing to suspect when a recompiled game takes
the wrong branch once in a thousand iterations.

## Readability goals

Borrowed wholesale from the sibling projects, because they are what makes a
recomp repo useful to anyone but its author:

- Every emitted line carries its address and original disassembly.
- Hardware-adjacent constants are named, not left as hex.
- Non-obvious semantics live in `recomp_rt.h` helpers with real names
  (`psp_lwl`, `psp_bitrev`) rather than being inlined as bit tricks.
- Generated files carry a header explaining the conventions.
- The generated C is committed to the game repo — it is the project's source of
  truth, the same as other recomp projects.
