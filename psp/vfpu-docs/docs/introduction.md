
# Introduction

This document describes how the PSP VFPU instruction set operates. We attempted
to collect all the knowledge available in the community and put it toghether in
a document that can be used as a reference for developers and enthusiasts.

The goal is to describe the behaviour of the hardware unit with as much detail
as possible in a way that every statement can be verified. For this reason,
every functional detail described in the docs must have a test that
validates it. Of course some things are harder to validate (like hardware bugs)
so there's some statements that won't have tests for them at this time.

## MIPS allegrex CPU

The Allegrex CPU is a MIPS CPU based on the `MIPS II` architecture. This is a
32 bit CPU and architecture that has many similarities with other CPUs of the
same architecture. However, if we only focus on the instruction set, the main
differences with other CPUs in the `MIPS II` family would be:

 - Lack of 64 bit FPU support (only single float support).
 - Lack of MMU/TLB (only certain memory protections are available).
 - Some extra MIPS32r2 instructions (mostly arithmetic and bit manipulation).
 - Other COP0 instructions, some borrowed from MIPS32
 - Lack of _Coprocessor 3_ and custom _Coprocessor 2_ (VFPU)

Most of the extra instructions that are present in the CPU are identical to
their MIPS32 counterparts. In some cases though, the encoding is slightly
different.


## VFPU unit

The PSP VFPU is a coprocessor unit that can perform vector/matrix float and
integer operations on a set of 128 bit registers. It features dedicated units
to perform the most usual operations that 3D videogames require.

### Register set

The CPU features 128 registers, each of them 32 bit wide. Most of the time
they are interpreted as IEEE-754 compliant floating point registers, although
some instructions will interpret them as integers (or other formats such as
8/16 bit packed integers). The registers can be addressed individually but also
in a more powerful way by grouping them as vectors or matrices.

Registers will usually be represented in their matrix layout. The VFPU has 8
matrices, each of them containing 16 elements (4 rows by 4 columns). For each
of the 8 total available matrices, the elements are arranged in the following
fashion (`X` represents the matrix number, 0 to 7):


                         Single 32 bit elements
                       ---------------------------
                      | SX00 | SX10 | SX20 | SX30 |
                      |------|------|------|------|
                      | SX01 | SX11 | SX21 | SX31 |
                      |------|------|------|------|
                      | SX02 | SX12 | SX22 | SX32 |
                      |------|------|------|------|
                      | SX03 | SX13 | SX23 | SX33 |
                       ---------------------------

When the registers are referenced as vectors, they are grouped as rows and
columns of a given matrix. This is important since it means that a vector
is composed of elements from a single matrix and cannot access elements across
multiple matrices. There's 2D, 3D and 4D vectors, usually called pair, trio and
quad respectively. Single elements can be viewed as 1D vectors, and most
instructions are available in all four possible vector sizes (which makes the
instruction set very uniform). Not all access patterns are possible: pair and
trio registers have 128 possible addressing modes while quad has only 64.
The available patterns are described as follows:

                             2D vector rows
                       ---------------------------
                      | RX00        | RX20        |
                      |------+------|------+------|
                      | RX01        | RX21        |
                      |------+------|------+------|
                      | RX02        | RX22        |
                      |------+------|------+------|
                      | RX03        | RX23        |
                       ---------------------------

                             3D vector rows
                       ---------------------------
                      |        RX00        |      |
                      |------+------+------|------|
                      |        RX01        |      |
                      |------+------+------|------|
                      |        RX02        |      |
                      |------+------+------|------|
                      |        RX03        |      |
                       ---------------------------

                             3D vector rows
                       ---------------------------
                      |      |        RX10        |
                      |------|------+------+------|
                      |      |        RX11        |
                      |------|------+------+------|
                      |      |        RX12        |
                      |------|------+------+------|
                      |      |        RX13        |
                       ---------------------------

                             4D vector rows
                       ---------------------------
                      |        RX00               |
                      |------+------+------+------|
                      |        RX01               |
                      |------+------+------+------|
                      |        RX02               |
                      |------+------+------+------|
                      |        RX03               |
                       ---------------------------

                             2D vector cols
                       ---------------------------
                      | CX00 | CX10 | CX20 | CX30 |
                      |      |      |      |      |
                      |------|------|------|------|
                      | CX02 | CX12 | CX22 | CX32 |
                      |      |      |      |      |
                       ---------------------------

                             3D vector cols
                       ---------------------------
                      |      |      |      |      |
                      | CX00 | CX10 | CX20 | CX30 |
                      |      |      |      |      |
                      |------|------|------|------|
                      |      |      |      |      |
                       ---------------------------

                             3D vector cols
                       ---------------------------
                      |      |      |      |      |
                      |------|------|------|------|
                      |      |      |      |      |
                      | CX01 | CX11 | CX21 | CX31 |
                      |      |      |      |      |
                       ---------------------------

                             4D vector cols
                       ---------------------------
                      |      |      |      |      |
                      | CX00 | CX10 | CX20 | CX30 |
                      |      |      |      |      |
                      |      |      |      |      |
                       ---------------------------


Matrix addressing is similar to vectors: registers can be read vertically or
horizontally. That means matrices can be accessed in a row major and column
major mode (ie. by accessing them as a set of rows or columns). Similarly
there's three possible sizes: 2x2, 3x3 and 4x4, containing 4, 9 and 16
registers respectively. Again not all addressing patterns are available, having
64 possible addressing modes for 2x2 and 3x3 matrices, but only 16 for 4x4
matrices. These are:

                                2D matrix
                       ---------------------------
                      | MX00        | MX20        |
                      |             |             |
                      |------+------|------+------|
                      | MX02        | MX22        |
                      |             |             |
                       ---------------------------

                                3D matrix
                       ---------------------------
                      |                    |      |
                      |        MX00        |      |
                      |                    |      |
                      |------+------+------+------|
                      |                    |      |
                       ---------------------------

                                3D matrix
                       ---------------------------
                      |      |                    |
                      |      |        MX10        |
                      |      |                    |
                      |------+------+------+------|
                      |      |                    |
                       ---------------------------

                                3D matrix
                       ---------------------------
                      |                    |      |
                      |------+------+------+------|
                      |                    |      |
                      |        MX01        |      |
                      |                    |      |
                       ---------------------------

                                3D matrix
                       ---------------------------
                      |      |                    |
                      |------+------+------+------|
                      |      |                    |
                      |      |        MX11        |
                      |      |                    |
                       ---------------------------

                                4D matrix
                       ---------------------------
                      |                           |
                      |        MX00               |
                      |                           |
                      |                           |
                       ---------------------------

There's also a small set of eight "control" registers that are used for a
variety of things, such as prefix state, comparison flag bits, etc. These
registers are defined as follow:

 - Reg 128 (VFPU_PFXS): holds the rs prefix value.
 - Reg 129 (VFPU_PFXT): holds the rt prefix value.
 - Reg 130 (VFPU_PFXD):  holds the rd prefix value.
 - Reg 131 (VFPU_CC): holds the condition code value.
 - Reg 135 (VFPU_REV): read only register with VFPU revision information.
 - Regs 136 to 147 (VFPU\_RCX0 to VFPU\_RCX7): Pseudorandom generator state.

Some of these registers are never accessed directly but rather using some
VFPU instructions (ie. prefixes, condition code, etc). However these can be
read and written in some useful cases, for instance thread context saving and
restoration (so that the VFPU state is preserved across thread rescheduling).

### Register hazards

Most CPUs have what's called "hazard detection logic", which tracks register
reads and writes so that things happen in the right order and results actually
make sense. In the VFPU this is also the case, however some operations are
quite complex and can be complex to track.

Control registers seem to have some hazards, for instance "mfvc" instruction
has a one cycle hazard with any previous vcmp instruction. That means a vnop
or some other VFPU instruction should be inserted between a vcmp and mfvc
instruction pair to get the right VFPU_CC value.

Some VFPU instructions (mostly dealing with matrices and transformations)
require that the input and output registers do not overlap. This has to do with
how the hardware performs the operations internally: the VFPU can perform most
vector-vector operations in a native way, but matrix operations seem to be
decomposed into series of vector-vector operations (ie. a vmmul seems to be
a sequence of vtfm operations). Since the results are only partial, the inputs
are overwritten before the CPU can even read them, causing incorrect results
for the operation.

The affected instructions are divided in two groups, a group that does not
allow any sort of overlap, and another group that allows some limited overlap.
Instructions vmmul, vtfm2/3/4, vhtfm2/3/4, vqmul and vcrsp do not allow any
sort of overlap between input and output registers. These instructions perform
operations by repeating a dot product operation multiple times, which results
in partial updates of the output register. This partial updates overwrite
the input register causing the result to be incorrect.

Instructions that allow partial overlaps are vsin, vcos, vasin, vnsin, vexp2,
vrexp2, vlog2, vsqrt, vrsq, vrcp, vnrcp, vdiv, vmscl and vmmov. Single versions
(.s) are not affected by this restriction. These instructions are also
internally decomposed into a bunch of smaller operations (for instance
trigonometric operations are decomposed into a series of single (.s)
operations). The registers are allowed to overlap as long as they are
compatible in terms of element count and access "direction" (ie. a matrix must
be read using the same mode).


*Examples*

```
  vmscl.p M000, M022, S100    # No overlap, always OK
  vmscl.p M000, M000, S100    # M000 overlaps with itself, OK
  vmscl.p M000, E000, S100    # Invalid overlap, matrix order is different
  vmscl.t M000, M011, S100    # Overlapping registers are not identical
```

```
  vcos.q R000, C000           # Invalid overlap (one element only)
  vcos.q R000, R000           # Identical overlap, OK
```

### Floating point format

Although the FPU seems IEEE-754 compliant, it has a couple of non-standard
features that break this compatibility. Its rounding mode is hardwired to
"round to nearest" mode, so that users cannot choose another rounding mode.
It also lacks support for denormal numbers (also called subnormals): when an
operation produces a subnormal number, it rounds it to zero. If the input of
an operation is a denormal number, it will also be treated as zero.

See the `ieee754-fun.c` file for tests.


## Instruction execution


The VFPU is a pipelined CPU with an issue width of one. That means that
instructions take multiple cycles to execute, since they execute partially
during each cycle, and a maximum of one new instruction begins execution
each cycle. Instructions that block the pipeline for more than one cycle
can be identified by having a throughput different than one. These block the
pipeline for a certain number of cycles before a new instruction can enter
it.

An instruction usually begins executing whenever its input registers are
`ready`, that is, any previous instruction writing those registers have fully
completed their execution. For this reason it is important to closely observe
the instruction latency, measured in cycles, since an instruction might have
to wait for its inputs to become available, reducing efficiency. A common
strategy is to interleave non-dependant instructions to `hide` latency and
avoid wasting CPU cycles.

The pipeline structure looks more or less as follows:

 - Register read
 - Input prefix operations
 - VFPU operation (arithmetic, logic)
 - Output prefix operation
 - Register write

Prefix operations allow to perform certain operations on the inputs before
the actual instruction operation and some other operations on the output.


### Prefix operations

VFPU operations can operate on one or two inputs (`rs` and `rt`) and one
output (`rd`). The input values can be pre-processed by using the `VFPU_PFXS`
and `VFPU_PFXT` registers (and therefore `vpfxs` and `vpfxt` instructions).
The result of the operation being written to `rd` can be post-processed by
using the `VFPU_PFXD` register (`vpfxd` instruction).

Valid operations for input registers are:

 - Sign change (negation)
 - Absolute value
 - Swizzle (rearranging elments in a row/col)
 - Override element with constant value.

Operations available to the output register post-processing are:

 - Value clamping (to ranges 0..1 or -1..1)
 - Write masking (disable register write)

There's some restrictions on their usage. The assembler will signal an error
should you violate any of the restrictions.

 - Constant values can only be 0, 1, 2, 3, 1/2, 1/3, 1/4, 1/6 or any of their
   negative counterparts
 - Swizzle cannot extend beyond the operand size (ie. you cannot use .z with a
   an instruction that uses single or pair elements).

A few examples to showcase input prefixes:

```
  # Sign change prefix
  vmul.p R000, R001, R002[-x,-y]     # Multiplies two rows negating one of the inputs
                                     # S000 = S001 * -S002;  S010 = S011 * -S012

  vfad.q R000, R001[x,-y,z,-w]       # Funnel-add all elements with some changed signs
                                     # S000 = S001 - S011 + S021 - S031

  # Absolute value prefix
  vdot.p S000, R001[|x|,|y|], R002   # Dot product with forced absolute value for R001
                                     # S000 = |S001| * S002 + |S011| * S012

  # Negative and absolute value prefixes
  vdot.p S000, R001[-|x|,-|y|], R002   # Dot product with forced negative values
                                       # S000 = -|S001| * S002 - |S011| * S012

  # Swizzle prefix
  vdot.q R000, R001, R002[x,y,x,y]   # Multiplies with repeating values
                                     # S000 = S001 * S002;  S010 = S011 * S012
                                     # S020 = S021 * S002;  S030 = S031 * S012

  # Constant value prefixes
  vdot.t R000, R001, R002[1,2,3]     # Second operand ignored, overrides to (1,2,3)
                                     # S000 = S001 + S011 * 2 + S021 * 3

  vdot.t R000, R001, R002[x,-2,-y]   # Mix swizzle and constant elements
                                     # S000 = S001 * S002 - S011 * 2 - S021 * S012


```

Some more examples for output prefixes.

```
  vmul.p R000[[-1:1],[-1:1]], R001, R002  # Multiplies with output saturation
                                          # S000 = min(1.0f, max(-1.0f, S001 * S002))
                                          # S010 = min(1.0f, max(-1.0f, S011 * S012))

```

Adding a prefix modifier to an operand will result in `vpfxs/t/d` instructions
being emitted before the actual instruction. This syntax exists just to make
assembly coding more comfortable to the user. When using the disassembler the
prefix instructions will be clearly visible.

```
  # The following operand-decorated instruction:
  vmul.q R000, R100[x,y,x,y], R200[-x,-y,z,w]

  # is actually encoded as a sequence of instructions:
  vpfxs [x,y,x,y]
  vpfxt [-x,-y,z,w]
  vmul.q R000, R100, R200
```

Prefix instructions consume one cycle and have no visible latency (the
"decorated" instruction doesn't have to wait any extra cycles). In some cases
it might be faster to not use prefixes and use other instructions (vcst, vabs,
vneg, vsat0/1 are some similar alternatives), particularly when optimizing for
throughput. The advantage of using prefixes is that latency is kept low (since
they have no latency and the extra operation is "included" in the instruction
pipeline).


# Allegrex Instructions

## Bit manipulation instructions

The following instructions exist in the Allegrex CPU and share the same MIPS32
encodings:

 - *seb*: Sign extend byte (byte to word signed extension)
 - *seh*: Sign extend half-word (half-word to word signed extension)
 - *ext*: Extract bit field (extract a bit field in a zeroed register)
 - *ins*: Insert bit field (insert lower bits into another register)
 - *wsbh*: Swap bytes within a half-word

Other instructions that are borrowed from MIPS32 but have a different encoding
are:

 - *clo*: Count leading ones (uses some unused `SPECIAL` encodings)
 - *clz*: Count leading zeros (uses some unused `SPECIAL` encodings)

The bit manipulation Allegrex specific instructions are:

 - *wsbw*: Swap bytes in word (uses `BSHFL` encoding adjacent to `wsbh`)
 - *bitrev*: Reverse bits in a word (uses unused `BSHFL` encoding)

## Arithmetic-Logical instructions

Allegrex features some instructions present in MIPS32 and MIPS32r2 with
identical encoding to these:

 - *rotr*: Rotate word right by a fixed amount
 - *rotrv*: Rotate word right by a variable amount
 - *movz*: Conditional register move on zero
 - *movn*: Conditional register move on non-zero

Other instructions that have some particular encoding are multiply-accumulate
instructions. Some overlap with `MIPS R4010` encodings and some others just use
unused encodings. They all use unused `SPECIAL` opcodes:

 - *madd*: Signed multiply-accumulate integer
 - *maddu*: Unsigned multiply-accumulate integer
 - *msub*: Signed multiply-subtract integer
 - *msubu*: Unsigned multiply-subtract integer

There's also two novel Allegrex instructions that are used to perform faster
compare-and-move operations. These use free `SPECIAL` opcodes as well:

 - *min*: Selects smallest (signed) value between two registers.
 - *max*: Selects greatest (signed) value between two registers.


