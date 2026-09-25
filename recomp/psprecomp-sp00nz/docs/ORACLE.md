# The oracle

Bring-up is a debugging problem: the recompiled game does something, it is
wrong, and the question is *where* it first diverged from correct. An oracle is
whatever can answer that question. This document is about which oracles we use
and — because it determines the whole licensing posture of the project — how
we use them.

## The rule

**The oracle is something you diff against, never something you link.**

There is no MIT-licensed PSP emulator. PPSSPP is GPLv2+, JPCSP is GPLv3.
Linking either into this toolkit would make the toolkit GPL, and that is not
the license this project wants. It is also unnecessary, because an oracle does
not need to be *inside* the program to be useful — it needs to produce a trace
you can compare against.

This is exactly the arrangement `lynxrecomp` has with Handy and Mednafen, and
`ps3recomp` has with RPCS3: cited in the credits, used for behavioural
reference and cross-validation, with no code copied, vendored, or linked.

## Three oracles, in order of use

### 1. The built-in interpreter (phase 5) — the primary one

An Allegrex interpreter that shares **this repo's decoder and this repo's
runtime**, differing from the recompiled path only in how it sequences
instructions.

This is the strongest oracle available and the one bring-up will lean on,
because when the interpreter and the recompiled C disagree, the bug is
necessarily in one of exactly two places: the emitter, or the interpreter's
sequencing. Everything else — the decode, the memory model, the semantic
helpers, the HLE — is *shared*, so it cannot be the difference. That property
is worth more than fidelity.

A trace is `(pc, register file, memory writes)` per instruction; the first
mismatching entry localizes the bug to one instruction.

### 2. PPSSPP — the external reference

PPSSPP is the most accurate public description of PSP behaviour that exists,
and it is well-suited to this role:

- **Headless mode** runs a module without a window and produces deterministic
  output.
- **A stepping disassembler and register view** for comparing state at a
  specific address.
- **The GE debugger** dumps the display list and framebuffer, which is how
  graphics divergence gets localized.

It answers questions the interpreter cannot, because the interpreter shares our
assumptions and therefore shares our bugs. When both the recompiled path *and*
the interpreter agree on something wrong, the error is in the shared layer —
the decoder, the memory model, or the HLE — and PPSSPP is the third opinion
that catches it.

Used strictly as a separate process. No code from PPSSPP is copied into this
repository.

### 3. pspautotests — ground truth

[pspautotests](https://github.com/hrydgard/pspautotests) is a suite of PSP
programs whose expected output was captured **on real hardware**. That makes it
the only oracle here that is not another program's opinion.

It is how the runtime and HLE layers get validated: the CPU semantics, the
`sceKernel` behaviours, and — most valuably — the VFPU, including the prefix
behaviour that is otherwise nearly impossible to get right by reading
documentation.

## Cross-validation of function discovery

A separate problem from execution: did discovery find all the functions?

The answer comes from running IDA Pro and/or Ghidra headless over the same
decrypted module and comparing function sets. The useful metric is **recall** —
functions the disassembler found that we missed — since those are concrete
discovery bugs. Functions we found that it did not are often *correct* (we
follow jump tables it gives up on) and get reported separately rather than
counted as errors.

Where both agree and we do not, that is a high-confidence gap, and it feeds
back as discovery seeds.

## What none of this is

It is worth being explicit, because the distinction is the entire point of the
project: **an oracle is not a dependency.** The shipped artifact is a native
executable containing recompiled C. It does not contain an emulator, it does
not call one, and it does not need one installed. The emulators are development
instruments — the equivalent of a logic analyzer, not a component.
