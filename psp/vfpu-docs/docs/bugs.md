
# Known VFPU bugs/errata

The VFPU has some known bugs or errata. Some of the bugs have been fixed in
later hardware revisions, but some others have been _kept_ for compatibility
reasons.

## vhtfm output register written incorrectly

Instructions vhtfm2.p and vhtfm3.t feature a bug that affects all PSP models.
This bug is triggered whenever the output register `rd` is 64 or higher,
which is incidentally the reason why this bug doesn't affect vhtfm4.q.

Registers below 64 are left and top aligned registers, that is, registers in
the form of `RX0Y` or `CXY0`. However registers above 64 are in the form
`RX1Y`, `RX2Y`, `CXY1` or `CXY2`, that is, they are _shifted_ by one or two
columns or rows (not at the edge of the matrix). Whenever these registers are
used the instruction makes a mistake writing the register and shifts the row
or column by an incorrect number of elements, causing an unvoluntary
_corruption_ in the output value.

This writing error seems to be deterministic. For vhtfm2.p the result is
written in one element shifted to the left/up. For vhtfm3.t the shift happens
to the right/down, wrapping around the edges of the matrix. You can find the
tests and examples in `psp-tests/manual/vfpu-bugs.c`

## ulv.q (lvl.q / lvr.q) register corruption

In PSP 1000 devices, the lvl.q and lvr.q instructions (which are usually
expanded from ulv.q macros) present a bug (fixed in 2000 and later models).
When any of these instructions is executed, the CPU corrupts the FPU register
bank (that is, Coprocessor 1, which is unrelated to the VFPU).

The bug causes an unexpected write to an FPU register whenever the instruction
is executed. The value being written is apparently whatever value was left in
the  coprocessor bus (which in some incidental cases causing no corruption if
the previous and new values are identical). The bus seems to be used by
mfc1/mtc1 and some other COP1 instructions.

The corrupted register is deterministic, its value is derived from the
VFPU destination register. The lowest 5 bits of the register indicate the FPU
register that will be corrupted (ie. `C000` corrupts `$f0`, `C010` corrupts
`$f1` and so on). As a workaround in inline VFPU assembly, it is possible to
designate the register as _clobbered_, which will make gcc assume it was
corrupted/written, thus using other registers or saving and restoring it.


