# Bring-up: where the recompiled game stops, and what has been ruled out

Lumberjack's recompiled C runs past `module_start` into the game's own startup,
prints its own debug message, and then walks a null pointer as a function table
and spins. This file records the investigation so far â€” mostly so that the
things already eliminated are not eliminated a second time.

## The symptom

```
libc:_getmodreent: no reent structure     <- the game's own libc, via sceKernelPrintf
dispatch miss: 0xFC2027BD not recompiled
dispatch miss: 0x000309FF not recompiled
... eight more
```

## What the garbage targets actually are

Searching `.text` for each miss value as a byte sequence finds eight of the ten,
all at **unaligned (+2)** offsets, at consecutive stride-4 addresses:

```
.text+0x0A, 0x0E, 0x12, 0x16, 0x1A, 0x1E, 0x22, 0x26
```

That is a loop walking a function-pointer table whose base is near address 8
and two bytes misaligned â€” the signature of a null pointer with a small offset
added, then indexed. It is not random corruption.

Crucially, **the libc message appears before the first miss.** The misses are
downstream of that failure, not independent of it.

## Ruled out

**Unresolved jump tables.** The computed-jump sites are now resolved (41 of 61
on Lumberjack, 70 of 102 on `hell2k`) and every one of those ~2100 targets was
already discovered by relocation mining. The garbage targets are not table
entries; they are wild register values.

**`sceKernelGetModuleIdByAddress` returning a fabricated id.** The theory was
that newlib's `_getmodreent` used that id, failed to match it against a
per-module table, and returned null. Changing it to return an error produced
**byte-identical output** â€” same message, same ten misses, same order. Not on
this path. (The error return was kept anyway: fabricating an id we cannot
honour is the sort of plausible lie that fails silently later.)

**A missing `$gp`.** Recompiled code addresses globals relative to `$gp`, and
the host never set it. Setting it from the module info's `gp_value`
(`0x003BC870` for Lumberjack) also produced **byte-identical output**. Setting
`$gp` is still correct and should be part of a real loader â€” it simply is not
what causes this.

**The `_getmodreent` message itself.** This turned out to be a red herring, and
the way it was eliminated is worth keeping: rather than looking for a reference
describing how newlib finds its per-module reent structure, the function is
*in the module* and can simply be read.

Locating the string `_getmodreent` (vaddr `0x0009446C`) and searching `.text`
for the `lui`/`addiu` pair that materialises it gives exactly one referencing
site, `0x0000F3C0`. The code around it:

```
0000F358  lw    $v0, 132($a1)      walk a linked list of module records
0000F35C  beq   $v0, $a3, 0xF3E4   match -> success path
0000F364  lw    $a2, 0($a2)        next node
0000F368  bne   $a2, $zero, 0xF358 loop while the list continues
          ...                      not found: print the warning
0000F3D0  lw    $v0, -14272($v0)   return *(0x003AC840)
```

So the function walks a module registry, fails to find this module â€” because
nothing ever registered it, which *is* a genuine loader gap â€” prints a warning,
and then returns a **fallback**. The word at `0x003AC840` in the image is
`0x003AC4C0`, a valid `.data` address: a global reent structure.

**It does not return null.** The warning is benign and the fallback works. The
message merely happens to be printed before the first dispatch miss; it does
not cause it. Two rounds of investigation were spent on the assumption that it
did.

The missing module registration remains real and should still be fixed by a
loader â€” it is simply not fatal, and not the thing to chase first.

## FOUND: there is executable code outside `.text`

A function-entry trace (build the generated code with `PSPRECOMP_TRACE`) names
the caller instead of only the bad target. The last function entered before the
first miss is `psp_func_000182F4`:

```
000182F4  lui   $a0, 0x3B
          addiu $a0, $a0, 12260     -> 0x003B2FE4
          addiu $a1, $a1, 12292     -> 0x003B3004
          addiu $a2, $a2, 18544     -> 0x003B4870
          j     0x000181B0          tail call, with $a3 also 0x003B4870
```

Two (start, end) pairs handed to a loop: the shape of a static-initializer
runner walking an init/fini array.

`0x003B2FE4..0x003B3004` holds eight pointers â€” to `0x003B2AF8`, `0x003B2B40`,
`0x003B2B7C`, and five more. And those addresses contain **code**:

```
0x003B2B40:  27BDFFF0   addiu $sp, $sp, -16     a function prologue
             AFBF000C   sw    $ra, 12($sp)
             0C013074   jal   ...
```

`.text` is `0x00000000..0x00091E54`. These constructors sit at `0x003B2AF8+`,
which is inside `.data`. **The toolkit analyses and emits `.text` only**, so
they were never discovered, never emitted and never registered â€” and the
runner calls straight into them.

That is the failure. It is not a wild pointer at all: the pointers are correct,
and the code they point at is real. It is simply code the recompiler never
looked at.

The earlier "garbage targets near address 8" reading was a coincidence of
byte-searching: those values also occur as unaligned spans inside `.text`, which
made them look like misaligned reads of code. They are better explained as
instruction words from the uncovered `.data` region being treated as addresses
once execution derails.

### What this needs

Discovery and emission must cover code wherever it lives, not just `.text`.
Concretely:

- Seed from the init/fini arrays, which are identifiable from the runner or
  from the `.rodata`/`.data` relocations that populate them.
- Allow the analysed extent to include regions outside `.text` rather than
  assuming one contiguous code range (`a_analysis` currently takes a single
  base/size).
- Keep the existing validation: a candidate region is only code if it decodes
  cleanly, so widening the range does not mean decoding all of `.data`.

This is a structural change to the analyser, not a stub or a table fix, and it
is on the critical path to running the game â€” the constructors must execute
before `main` does anything useful.

## What is actually still open

With the reent warning eliminated as the cause, the wild pointer walk has **no
current explanation**. What is known:

- It is a loop reading a function-pointer table at a base near address 8,
  misaligned by two, and calling each entry.
- It is not a jump table, not the reent fallback, not `$gp`, and not the
  ModuleMgr id.

The next move is to find which recompiled function performs that loop, rather
than guessing at causes. The dispatch miss handler knows the bad target but not
the caller; recording the caller â€” or simply recompiling with a breakpoint on
`psp_dispatch` and reading the C stack â€” would name it directly. That is a
better instrument than another hypothesis.

**Relocations are verified as no-ops.** All 27,269 entries in Lumberjack carry
`(offsetSegment, addendSegment) = (0, 0)` against a single segment at vaddr 0,
so applying them at load base 0 changes nothing. This was previously an
untested assumption; it holds, and relocation application is off the loader's
critical path.

**A loader is still wanted** for module registration and TLS/reent setup, and
to set `$gp` properly rather than from the host. None of that is on the
critical path for the current failure.

## Suggested next steps

1. Write a real loader in the runtime rather than the throwaway host: parse the
   ELF, place segments, apply relocations, set `$gp`, and record the module.
   Verify by checking that applying relocations at base 0 is genuinely a no-op
   before assuming it.
2. Give the host an **instruction budget**. Every run in this investigation had
   to be killed on a timer, which makes each experiment more expensive than it
   should be and makes a hang indistinguishable from slow progress.
3. Only then resume chasing `_getmodreent`.

## A note on method

Both falsifications above cost one build and one run each, and both killed a
plausible theory that would otherwise have absorbed real effort. The two
findings that *did* land â€” the encrypted-module key tag offset, and the
identification of these garbage values â€” both came from searching the binary
for a pattern rather than reasoning about what should be happening.

---

# The GE draws, and the game reaches it

## Where it stands

The game now submits display lists: **3 lists, 254 commands, 2 finishes**, and
sets a framebuffer at `0x04088000`. That is the first time anything has reached
the GPU. It gets there through libc startup, `sceCtrl` input setup, and into
what is almost certainly the render path.

It still does not draw. `vertices submitted: 0`, `vtype 0x000000` — the lists
seen so far are state setup, not geometry. 240 of the 254 commands are not
individually decoded, so some of that setup is being ignored rather than
applied.

## Every label is now dispatchable

A recompiled function has one C entry point. The original has as many as
something can jump to. Direct branches become `goto`, and discovery promotes
cross-function branches — but a **computed** jump resolves at runtime, and its
target is routinely a block in the middle of a function that discovery had no
static reason to make callable. Dispatch then missed on `0x0000E594`: an
address whose code was sitting right there in the generated file, under a label
nothing could reach.

Each function now takes an entry address and begins with a switch; each
interior label gets a thunk. 5849 functions, 7833 interior entries, 13682
dispatch entries. Jump tables, indirect calls and cross-function branches
resolve by construction.

This was the **fourth** bug in one family — a boundary change applied to
emission but not to reachability. The others: entry address, cross-function
targets, fall-through. Each was invisible in whatever metric was being watched
at the time. Making reachability total is what ends the pattern; the first
three were each fixed one crash at a time.

## Two ways to see a hang, because one was not enough

The host had no budget, so every run had to be killed on a timer and every
statistic was lost with it. Two mechanisms now:

- **A call budget** in `psp_dispatch`. Useful, but far narrower than it looks:
  direct calls compile to direct C calls, so it only counts *indirect* ones. A
  20-second run registered **38**.
- **A wall-clock watchdog** in the host. This is the one that works. It does not
  care about the shape of the loop, and a spin-wait compiled entirely into
  `goto`s inside a single function — exactly what bring-up gets stuck in — is
  invisible to any counter placed at a call boundary.

Note also that MSVC treats `_IOLBF` as full buffering. A killed run lost all of
its stdout, which is what made the first hang look like an instant silent crash.

## Where it hangs, and the standing hypothesis

Last function entered is `0x00046830`, but that and its neighbours are short
leaf helpers that return promptly — the loop is in a caller and contains **no
calls at all**: not dispatch (38 indirect calls total), not firmware (101
stderr lines, all first-call logs — only 11 `sceKernelGetGPI`).

A pure compute loop that never exits. Immediately before it:

```
unimplemented at 0x0002F898: vfpu?
unimplemented at 0x0002F89C: vcst
unimplemented at 0x0002F8A0: vcst
unimplemented at 0x0002F8B0: vidt
```

**Hypothesis: the unimplemented VFPU ops leave registers holding garbage, and a
loop that iterates until a value converges never converges.** `vcst` loads a
constant and `vidt` builds an identity vector — both matrix setup, both on the
render path, and both trapping to no-ops right now. A loop fed zeros or stale
values where it expects a unit basis is a good way to spin forever.

This is a hypothesis, not a finding. It is cheap to test: implement `vcst` and
`vidt`, identify the `vfpu?` at `0x0002F898`, and re-run. If the loop still
spins, the theory is dead and the next step is to find the loop head by address
rather than by trace.

## Next

1. Implement `vcst` / `vidt`; identify the unknown op at `0x0002F898`.
2. Decode the 240 unrecognised GE commands — some are certainly the vertex and
   texture state that would make `vtype` non-zero.
3. Transformed geometry. The rasterizer currently handles through-mode only and
   *counts* transformed vertices rather than drawing them, deliberately: wrong
   pixels are harder to diagnose than no pixels.

---

# Falsified: the VFPU-garbage theory

The standing hypothesis was that unimplemented VFPU ops left registers holding
garbage and a convergence loop therefore never converged. `vcst`, `vidt`,
`viim` and `vfim` are now implemented — `viim` needed a decoder fix too, since
opcode `0x37` sub-values 6 and 7 were falling through to "unknown" and the
`.word 0xDF3C00FF` at `0x0002F898` was in fact `viim.s v60, 255`.

**The output is byte-for-byte identical.** Same 3 lists, same 254 commands,
same hang. The theory is dead.

That is a clean result and it cost one build: the ops needed implementing
regardless, and the run says plainly that they were not what the loop was
waiting on. Do not revisit this.

## What the run actually points at

The spin is inside `0x000471F4`, which calls `0x00046830` (the last traced
entry) and then loops without calling anything else — which is why neither the
dispatch counter nor the firmware log can see it.

More promising, and visible in stderr just before the first null call:

```
libc:_getmodreent: no reent structure
dispatch miss: 0x00000000 not recompiled       (x17)
```

`_getmodreent` is back. Earlier work established that it returns a *working*
fallback rather than null, so this is not the old theory returning — but
something downstream is reading a function pointer out of a structure that was
never populated, getting zero, and calling it. 17 times, alongside 28 bad
memory accesses.

That is the thread to pull: find what writes the table those 17 null calls read
from. A null callback that is silently skipped is exactly the shape of a
program that then waits forever for something that callback was supposed to do.

## Next

1. Trace the 17 null dispatches to their call sites and find the unpopulated
   table.
2. Disassemble `0x000471F4` and read the loop condition directly — it will name
   the memory it is waiting on.
3. The 240 undecoded GE commands still hide whatever would set `vtype`.

---

# The hang, located exactly

## Read the trace in the right direction

`psp_trace_dump()` prints **newest first**. Reading the tail of a redirected
log therefore shows the *oldest* of the last 32 entries, and two rounds of
disassembly went into short leaf helpers that were never the problem. Running
the watchdog at 20s and 60s and diffing the two dumps is what caught it: both
stop at exactly **470 function entries**, so execution genuinely stalls rather
than crawling.

## The loop

`0x0004DD14`, reached through `0x0004DBB0` ? `0x0004DCF8`:

```
0004DD14  addu $t0, $a0, $a2          ; base + offset
0004DD18  lw   $a2, 4($t0)            ; compare key
0004DD1C  bne  $a2, $t1, 0x0004DD38
...
0004DD40  lhu  $v0, 16($t0)           ; next index
0004DD44  bnel $v0, $t2, 0x0004DD14   ; loop while != sentinel
0004DD48  sll  $a2, $v0, 4            ; 16-byte stride
```

A chain walk over 16-byte entries: read the next index from `+16`, stop when it
equals the sentinel `$t2` (loaded from `+24` of the header back at
`0x0004DA60`). It never stops.

## What that rules out

Host memory is `calloc`ed and `.bss` is therefore zero, so an **uninitialised
table would exit immediately** — `v0` would read 0 and the sentinel is 0. The
loop spinning means the table holds *garbage*: non-zero next-indices that never
reach the sentinel, most likely a chain that points back into itself.

The prime suspect is the **28 bad memory accesses**. A structure initialiser
whose writes landed outside mapped memory would leave exactly this: a header
that looks plausible and a chain that goes nowhere. The 17 null dispatches and
`libc:_getmodreent: no reent structure` are probably the same root cause seen
from a different angle.

## Next

1. Log the address of every bad memory access with the function that made it.
   That list is short (28) and one of them almost certainly writes this table.
2. Dump the 16-byte entries at `$a0` when the loop is entered — whether the
   chain self-references or runs off the end says which write went missing.
3. Only then go back to the GE. The 240 undecoded commands and `vtype == 0`
   are downstream of this: the game never gets far enough to submit geometry.

---

# Bad memory accesses: 28 to 3

Logging the *address* of each bad access, plus the function that made it, took
this from a bare count to a diagnosis in one run. The output clustered
immediately:

```
bad write32 at 0x00833FE0 (last fn 0x000123A4)   ... 18 accesses, one structure
bad write32 at 0x02336F68 (last fn 0x000473A4)   ... 7 accesses
bad read32  at 0x0C0247C5 (last fn 0x0000F3C0)   ... 3, unaligned
```

The first two clusters sit just past the host's `MODULE_SIZE` of `0x00500000`,
while the module's own `memsz` is only `0x004B0000`. That gap is the game's
**internal heap**, which starts after `.bss` and grows — straight out of the
mapping. Enlarging the module mapping to `0x03000000` drops bad accesses from
**28 to 3**.

The remaining three are a genuinely bad pointer: `0x0C0247C5` is unaligned and
outside any mapped region. Unresolved.

## This did not fix the hang

Same stall, same `0x0004DD14`, same 470 function entries. Worth stating plainly:
the structure writes that were being lost are *not* what feeds the unterminated
chain walk. Fixing them was correct and necessary, and it was not sufficient.

Two clean falsifications now — the VFPU-garbage theory and this one. Both cost
one run each, and both removed a plausible explanation that would otherwise
keep being re-proposed.

## Next

1. Dump the 16-byte table entries at the loop's `$a0` on entry. The chain either
   self-references or runs past the end; which one it is names the bug. This is
   the measurement that should have been taken before either of the two theories
   above — both were reasoning about the binary rather than reading it.
2. Chase the unaligned `0x0C0247C5` from `0x0000F3C0`.

---

# Falsified: the unterminated chain walk

Taking the measurement instead of reasoning about it, via a new one-shot
function-entry watch (`psp_trace_watch`), which dumps arguments for one
function out of thousands without rebuilding the generated code:

```
chain walk: a0=0x003EC384 a1=0x09FFFAE8 a2=0x00000000 a3=0x003C61E0
  t1(key)=0x04153FFF t2(sentinel)=0x0000
  [ 0] off=0x000000 key=0x00000000 val=0x00000000 next=0x0000
  -> hits sentinel
```

**The loop exits on its first iteration.** `0x0004DD14` is not the hang. It is
simply the last function *entered* before everything stopped, which is a
different claim entirely — and one I conflated with "where it is stuck" across
two rounds of disassembly.

The stall is therefore in a caller — `0x0004DCF8`, `0x0004DBB0`, or above —
looping without entering any traced function. `0x0004DBB0` contains a bounded
loop (`slti $s0, 1026`) whose counter would spin forever if it never advanced;
that is the next thing to read, and it should be read with a watch rather than
guessed at.

## Three falsifications, one lesson

VFPU garbage, lost structure writes, unterminated chain walk. Each was
plausible, each cost one run, and **each came from reasoning about the binary
rather than measuring it.** The one measurement that settled the question took
less effort than any of the three theories that preceded it.

The trace's "newest first" ordering caused real confusion here too: the last
entry in a redirected log is the *oldest* of the last 32, and reading it the
wrong way sent two rounds of disassembly into leaf helpers that were never
involved.

## What is actually known

- Execution stalls at exactly 470 function entries, reproducible at 20s and 60s.
- The stall is a loop containing **no calls of any kind** -- not dispatch (38
  indirect calls total), not firmware (first-call logs only).
- It is above `0x0004DD14` in the call chain, which returns normally.

---

# $ra holds a stack address

Dumping registers from the watchdog -- the loop calls nothing, so the entry
trace cannot see into it, but the register file is still readable:

```
regs at stall: ra=0x09FFFBB0 sp=0x09FFFAB0 gp=0x003BC870
  v0=0x00000000 v1=0x00000001 a0=0x003EC384 a1=0x09FFFAE8
  s0=0x003EC37C s1=0x00000000 s2=0x09FFFAE8 s3=0x003EC384
```

**`$ra` is `0x09FFFBB0` -- inside the stack, 256 bytes above `$sp`.** A return
address must live in the module's code range (`0x00000000`-`0x004B0000`).
Whatever is in `$ra` was never a return address; it is a stack pointer that got
stored there.

That is worth more than any of the loop theories. A clobbered `$ra` means
`jr $ra` jumps somewhere arbitrary, and it plausibly explains the **17 null
dispatches** already logged: a register file that loses return addresses
produces exactly that.

## What it also says about the register state

`a0 = s3 = 0x003EC384` and `s2 = a1` match the prologue of `0x0004DBB0`, which
does `s3 = a0; s2 = a1; s1 = a2`. But `s1 = 0` (it should hold `a2`) and
`s0 = 0x003EC37C` -- a *pointer*, `a0 - 8` -- where `0x0004DBB0` sets `s0 = 0`
and uses it as a loop counter bounded at 1026.

So the callee-saved registers are half-consistent with that frame and half not.
Either the stall is in a different function that happens to share argument
values, or **saved registers are not being restored correctly across calls**.
The second would explain `$ra` too, and it is the more economical explanation.

## Next

1. Check `sw $ra, N($sp)` / `lw $ra, N($sp)` emission against a function that
   nests calls. If the store and load disagree about the offset, or the stack
   adjustment is applied in the wrong order relative to the save, every
   non-leaf function corrupts its own return address.
2. `$sp` at the stall is `0x09FFFAB0`, but the host sets `HOST_STACK` to
   `0x08F00000` -- the game moved its own stack. Confirm that region is mapped
   and large enough; a stack growing into unmapped memory would produce
   silently-lost saves and exactly this register state.
3. Only then revisit the loop. The loop is a symptom if the register file is
   already wrong.

## Confirmed: setting $ra fixes the register file

`jal`/`jalr` now assign `$ra`, taking the module from **2 to 9,814**
assignments. At the stall `$ra` is `0x0004DBD8` -- a real code address, and
exactly the return site after `jal 0x0004DCF8` at `0x0004DBD0`. It was
`0x09FFFBB0` (a stack address) before.

That pins the stalled frame to `0x0004DBB0` with certainty rather than
inference, which is the first time any frame here has been identified from
evidence instead of a trace ordering.

The hang itself is unchanged, and `$s0` still reads `0x003EC37C` -- a pointer,
where every path through `0x0004DBB0` uses `$s0` as a small index (`sll $s0, 4`
to form a 16-byte offset, bounded at 1026). So `$s0` is being clobbered, or
the frame is not the one `$ra` implies. **`$ra` is now trustworthy and `$s0` is
not: the next thing to fix is whatever writes `$s0`.**

Callee-saved registers are the obvious suspect. Recompiled functions share one
global register file, so a callee that does not preserve `$s0`-`$s7` across a
call corrupts its caller silently -- the same shape of bug as `$ra`, and one
that the `$ra` fix does not address.

## Correction: $ra does not pin the frame

Last section claimed `$ra = 0x0004DBD8` identified the stalled frame as
`0x0004DBB0`. That is weaker than stated.

Fall-through between functions is emitted as `psp_func_XXXX();` -- a C call
that deliberately does *not* set `$ra`, because no `jal` executed. Control
therefore moves across function boundaries without updating `$ra`, and the
value only means "somewhere at or downstream of `0x0004DBB0`, reached without
another call". Several functions satisfy that.

## The loop is emitted correctly

`psp_body_0004DC1C` was the obvious candidate and it is fine:

```c
L_0004DC1C:
    r_v0 = psp_read8(r_v1 + 12);
    if (r_v0 == r_zero) { r_v0 = psp_slt(r_s0, (uint32_t)1026); goto L_0004DC3C; }
    r_s0 = r_s0 + 1;
    r_v0 = psp_slt(r_s0, (uint32_t)1026);
    { int _c = (r_v0 != r_zero); r_v1 = r_v1 + 16; if (_c) goto L_0004DC1C; }
```

Counter increments, bound is 1026, `beql` correctly nullifies its delay slot on
the not-taken path. `$s0 = 0x003EC37C` (4,113,276) cannot have come from this
loop -- `psp_slt` would have ended it at 1026 -- so that value is a leftover
from elsewhere and does **not** indicate a clobbered callee-saved register.

The callee-saved theory is therefore unsupported by this evidence. It may still
be true; this simply is not evidence for it.

## Standing facts

- Execution stalls at exactly 470 function entries, reproducible.
- The stalling loop makes **no calls of any kind** -- so it lies within a single
  `psp_body_*`, and no fall-through leaves it.
- `$ra` is now correct and no longer garbage; that fix was real and independent.

## Next

Find the loop by elimination rather than inference: instrument `PSP_ENTER` to
record the *last* body entered and have the watchdog print it, then read that
one function's emitted C for a back-edge that calls nothing. The entry trace
already holds this -- `0x0004DD14` was newest -- but that function was measured
entering once and exiting immediately, so the newest *entry* is not the
stalling *body*. A body that loops after its last call is invisible to entry
tracing, which is the gap to close.

---

# The loop, found

Recording loop back-edges (`PSP_LOOP`, 961 sites) closes the blind spot that
cost several rounds. First run with it:

```
last loop back-edge: 0x0004DD14 (10,020,314,003 hits)
```

Ten billion iterations of a single back-edge, from **one** entry to the
function. That is the hang, and it is the chain walk after all.

## My earlier measurement was wrong, not the theory

The chain-walk theory was recorded as falsified because a dump said the walk
"hits sentinel" on its first step. That dump hard-coded the sentinel as
`psp_read16(a0 + 24)`. The real sentinel is **`$t2`, set to the constant 1** at
`0x0004DCFC`:

```
0004DCFC  addiu $t2, $zero, 1
...
0004DD40  lhu  $v0, 16($t0)          ; next index
0004DD44  bnel $v0, $t2, 0x0004DD14  ; loop while next != 1
0004DD48  sll  $a2, $v0, 4           ; delay: offset = next * 16
```

So the walk terminates on next == **1**, not 0. The table's next-fields are
**0**, so index 0 computes offset 0, reads its own next-field, gets 0 again, and
loops forever pointing at itself.

Measuring the wrong quantity is worse than not measuring: it retired a correct
theory and sent the search elsewhere for several rounds. The lesson is narrower
than "measure" -- it is *derive the constant from the code, never assume it*.

## The actual defect

A hash table whose chain terminator is 1 has been left full of zeroes. Zero is a
valid index, so an uninitialised table is not inert here -- it is a self-loop.
Something that should fill those next-fields with 1 never ran, or ran against
different memory.

## Next

1. Find what initialises this table. `0x0004DBB0` and `0x0004DC1C` manage it
   (free-slot scan on `lbu 12(v1)`, insert at `s3 + s0*16`); the initialiser is
   likely a sibling that runs at startup.
2. Check whether that initialiser is among the 17 null dispatches -- a table
   left zeroed is exactly what a skipped init call produces.
3. Guard the walk during bring-up so a zeroed table stops instead of spinning;
   that alone may carry execution past this point and into geometry.

## Patching the self-loop advances execution

Breaking the self-loop at run time (host-side, via the entry watch -- an
experiment, not a fix) moves the program forward:

```
                    before          after
stall back-edge     0x0004DD14      0x00067E58
calls dispatched    38              57
bad accesses        3               25
framebuffer         0x04088000      0x04000000
```

So the diagnosis is right, and **the same failure recurs**: `0x00067E58` is
another loop, with `s3 = a0 = 0x00312D60` -- a second table, walked the same
way, stalling the same way.

That is the useful result. This is not one uninitialised table, it is a
*pattern*, which means chasing the initialiser for this particular table would
have been the wrong next move. Something systematic leaves these structures
zeroed.

## The likely shape of it

Two candidates, in order:

1. **A skipped initialiser.** There are now 19 dispatch misses. If one of them
   is a startup routine that fills these tables, every table it owns stays
   zeroed and each walk over one self-loops. This fits: the misses appeared
   before the first stall, and each patched stall reveals another table.
2. **An allocator contract.** If these tables live in memory the game expects
   pre-filled (not merely zeroed), the fault is in what hands that memory out.

Candidate 1 is cheaper to test and fits the evidence better -- the 19 misses
are already logged with their calling function, so the next step is to identify
them by name rather than guess.

## Next

1. Name the 19 dispatch misses: which functions call them, and is any of them a
   table initialiser? They are the strongest remaining lead and have never been
   individually examined.
2. Keep the self-loop patch available as a bring-up switch. It is wrong as a
   fix, but it is the only way to see what lies past these stalls while the real
   cause is unresolved.

---

# The 17 misses are one site: a vtable loop

Logging every miss with its calling function -- something never done, despite
the misses being in the log since the session's first run:

```
miss 0..16: target=0x00000000 from fn 0x00066C6C ra=0x00066C7C
miss 17:    target=0x0002E788 from fn 0x00018FB8
miss 18:    target=0x003C61D8 from fn 0x0007EB24
```

All seventeen are **one call site**, and the code says exactly what it is:

```
00066C6C  lw   $t9, 12($s1)      ; t9 = object's vtable pointer
00066C70  lw   $t9, 12($t9)      ; t9 = vtable[3]
00066C74  jalr $ra, $t9          ; call it
00066C78  addu $a0, $s1, $zero   ; delay slot: this = s1
00066C7C  addiu $s0, $s0, 1
00066C80  slti $v1, $s0, 17      ; 17 iterations
```

A loop calling method 3 on each of **seventeen objects** -- the shape of a
"initialise every subsystem" pass. Every one of them has a **null vtable
pointer**, so `lw $t9, 12($t9)` reads from address 12 and the call goes to 0.

## Why this matters more than any of the loop theories

Seventeen subsystems fail to initialise. The uninitialised hash tables that
have been stalling execution are downstream of exactly that: no initialiser
runs, so every table those subsystems own stays zeroed, and each walk over one
self-loops. One cause, many symptoms -- which is why patching the first table
just revealed a second.

This lead was available from the first run. Four loop theories were chased past
it. The misses were being *counted* rather than *identified*, and a count of
17 looks like noise while "17 subsystems failed to construct" does not. The
same mistake as the bad memory accesses, where logging the address turned a
count into a diagnosis in one run.

## The question to answer next

Immediately before the loop:

```
00066C4C  jal 0x000681C8         ; then s1 = a0
00066C58  jal 0x00067844         ; a0 = s1, a1 = 0
```

Two calls that plausibly construct or register these objects. Either they are
not setting the vtable pointers, or `s1` does not point where the loop thinks.

Worth checking specifically whether **PRX relocations** are involved. They were
deferred early and never revisited. The module loads at its link address of 0,
so a naive relocation pass would be an identity transform and *look* correct --
but any slot the loader is supposed to fill would still be left at zero, which
is precisely the observed symptom. Confirm rather than assume: dump the memory
at `s1 + 12` and compare against the file image.

## The object is entirely zero after its constructor ran

```
vtable loop: s1=0x00415B68  vptr@+12=0x00000000
  s1+00 .. s1+28 = 0x00000000   (every word)
```

`0x00415B68` is past the file image (`filesz 0x3B4000`) and inside `.bss`
(`memsz 0x4B0000`), so zero is its correct *initial* state -- this is not a
relocation problem, and the relocation theory from the previous section should
not be pursued on this evidence.

A constructor is supposed to fill it. `0x000681C8` is called at `0x00066C4C`
with the object already in `$a0` (`$s1` is set from `$a0` in the delay slot),
and `psp_func_000681D4` appears in the entry trace -- so that code **ran** and
left every word zero.

That is the sharpest question yet, and it is one function wide: does
`0x000681C8` write to its `$a0`, and does the recompiled version do the same?
A constructor that runs and writes nothing points at codegen -- a store whose
address or predicate is wrong -- rather than at anything architectural.

## Next

1. Trace `0x000681C8` instruction by instruction against its emitted C, looking
   for stores that should land in `$a0`.
2. If the stores are emitted correctly, watch `$a0` across the call: the object
   pointer may be wrong on entry, in which case the fault is in its caller.

## 0x000681C8 is not the constructor

```
000681C8  addiu $v1, $zero, -1
000681CC  sw    $v1, 31976($a0)      ; a0 + 0x7CE8
000681D0  addu  $a1, $zero, $zero
000681D4  addiu $a1, $a1, 1
000681D8  sw    $zero, 31964($a0)    ; a0 + 0x7CDC
000681DC  sltiu $v1, $a1, 3
000681E0  bne   $v1, $zero, 0x000681D4
000681E4  addiu $a0, $a0, 4
```

It writes `-1` and `0` at `$a0 + ~32000` -- three times, striding by 4. Those
are not vtable pointers and they are nowhere near `$s1 + 12`. So this function
was never going to fill the object, and the previous section's framing ("the
constructor ran and wrote nothing") was wrong: it ran and wrote exactly what it
was supposed to, somewhere else entirely.

`psp_func_000681D4` in the entry trace is this function's *inner loop*, not
evidence that an initialiser for the vtable object executed.

The remaining candidate before the loop is `0x00067844`, called with
`a0 = s1, a1 = 0`. That is the next thing to read -- and this time read it
before drawing a conclusion from the trace.

## Standing summary

- Stall cause: seventeen objects with null vtable pointers, at one call site
  (`0x00066C78`), cascading into uninitialised hash tables whose walks
  self-loop.
- The objects live in `.bss`; zero is their correct initial state, so this is
  **not** a relocation or loader fault.
- Whatever fills them either has not been found yet or is not running.
- `0x00067844` is the only remaining candidate on the path.

## 0x00067844 is not the constructor either

Called with `a1 = 0`, so `beq $s1, $zero` is taken immediately:

```
00067878  sb $zero, 31980($s0)     ; s0 = the object
00067880  sw $zero, 32080($s0)
```

Far-offset fields again, nothing near `+12`. **Neither call before the vtable
loop writes the vtable pointer.**

That is worth stating as a positive result rather than another dead end: the
object at `$s1` is large (constructors write past `+0x7CE8`, so >32 KB), and
its vtable slot at `+12` is filled by neither of the two functions on the path.
So the search should move *backwards* -- to wherever `$a0` was set before
`0x00066C4C` -- rather than forwards into the loop.

## Next

Read `0x00066C6C`'s function from its entry, not from the loop: find where
`$a0` is established before the two constructor calls. Whoever produces that
pointer either allocates the object and should install the vtable, or hands
over one that was never constructed.

## The layout, and why the objects are expected pre-built

```
00066C34  addiu $sp, $sp, -16          ; function entry
00066C48  sw    $zero, 31960($a0)
00066C4C  jal   0x000681C8             ; writes a0+31964, a0+31976
00066C6C  lw    $t9, 12($s1)           ; loop body
00066C84  bne   $v1, $zero, 0x00066C6C
00066C88  addiu $s1, $s1, 1880         ; stride
```

Seventeen objects, **1880 bytes** apart. 17 x 1880 = **31960** -- exactly the
offset this function starts zeroing at. So `$a0` points at an array of 17
sub-objects followed by the parent's own fields, and this function initialises
only the trailing fields before calling method 3 on each element.

The elements are therefore expected to be **already constructed** when this
runs. Their `+12` pointers are zero, so they are not.

Its two callers are `0x000743F0` and `0x0007EAE8`; neither appears in the
retained trace (which holds only 32 entries, so this is not yet conclusive).

## Miss 18 is a different failure and may be the more informative one

```
miss 18: target=0x003C61D8 from fn 0x0007EB24
```

`0x003C61D8` is past `filesz` (`0x3B4000`), so it is a **`.bss` data address
being called as code** -- not a null pointer, a *wrong* one. And `0x0007EB24`
sits immediately after `0x0007EAE8`, one of the two callers above.

A null pointer means "never written". A plausible-but-wrong pointer means
something *was* written, from the wrong place or with the wrong value. That is
a much narrower fault, and it is in the same region as the code that should be
constructing these objects.

## Next

1. Read `0x0007EAE8` and `0x0007EB24` together -- one calls `0x00066C34`, the
   other produces a bad function pointer. That pairing is the strongest
   remaining lead.
2. Confirm whether `0x000743F0` or `0x0007EAE8` actually executes, by watching
   both rather than inferring from a 32-entry trace.

## Both misses are the same idiom, failing two different ways

`0x0007EAE8` is a thin wrapper -- it calls `0x00066C34` with `a0 = s0 + 80`, so
the array of 17 lives 80 bytes into a larger object. It constructs nothing
itself.

`0x0007EB24` uses the identical calling pattern to the vtable loop:

```
0007EB40  lw   $t9, 28($s0)     ; t9 = object's vptr
0007EB44  lw   $t9, 12($t9)     ; t9 = vptr[3]
0007EB48  jalr $ra, $t9
```

Same shape as `lw $t9, 12($s1); lw $t9, 12($t9); jalr`. So this is the
project's standard virtual call: **object -> vptr -> slot 3**.

The two failures differ, and the difference is the finding:

| site | vptr | result |
|------|------|--------|
| `0x00066C78` | `0` | reads slot from address 12, calls 0 |
| `0x0007EB48` | non-zero | slot holds `0x003C61D8` -- a `.bss` address |

The second is the informative one. A vtable's slots must hold **code**
addresses, and code lives below `filesz` (`0x3B4000`). `0x003C61D8` is in
`.bss`. So this object's vptr does not point at a vtable at all -- it points at
some other `.bss` structure, and slot 3 of that is whatever data happens to sit
there.

Vtables normally live in read-only data inside the file image. A vptr pointing
into `.bss` therefore cannot be a correct vtable pointer that simply failed to
be written; it is a *wrong* value that something did write.

## What this narrows to

Every symptom now traces to vptr installation: seventeen objects with vptr `0`,
one object with a vptr aimed at `.bss`, and hash tables left zeroed because the
subsystems those vptrs would have dispatched to never initialised them.

The question is no longer "which initialiser did not run" but **"what installs
vptrs in this program, and why does it produce zero in most cases and a `.bss`
address in one?"**

## Next

Find a *working* virtual call -- there must be many, since execution reaches
this far -- and compare its object's vptr against these. One correct example
next to two broken ones will say whether vptrs are written by constructors, by
a registration pass, or by the loader, and that determines where to look.

## How a working vptr is installed

Logging *successful* indirect calls (never done before -- only failures were
ever examined) gave a working example immediately, in the same code region as
the broken one:

```
indirect ok: target=0x0004BE0C from fn 0x000740A4
```

And `0x000740A4` is a constructor that installs its vptr in three instructions:

```
000740A4  addiu $sp, $sp, -16
000740AC  lui   $v0, 0x3B
000740B4  addiu $v0, $v0, 12316     ; v0 = 0x003B301C
000740B8  sw    $v0, 0($a0)         ; object->vptr = 0x003B301C
```

Two things follow, and both are load-bearing:

1. **vptrs live at object offset 0**, written by the object's own constructor.
2. The value is a **file-image address**: `0x003B301C` is below `filesz`
   (`0x3B4000`), so the vtable is real read-only data, not `.bss`. It is
   materialised by `lui`/`addiu` as an absolute constant -- no relocation, no
   loader involvement. That independently confirms relocations are not the
   fault here.

## What the broken cases now mean

The seventeen sub-objects have never had their constructors run. Each would do
its own `sw <vtable>, 0($a0)`; none did, so their vptr words are still the zero
that `.bss` started as.

The parent constructor at `0x00066C34` zeroes trailing fields and then
immediately calls method 3 on all seventeen -- so it assumes something else
already constructed them. Nothing in `0x0007EAE8` -> `0x00066C34` does.

## Next

Look for the missing pass: a loop that walks the same 17 x 1880 array calling a
constructor on each element. It exists somewhere -- the parent would not
dispatch to them otherwise. Either it is never reached, or it runs against a
different address.

The `lui`/`addiu`/`sw ..., 0($a0)` signature is distinctive enough to grep the
disassembly for directly, which is a far better search than following call
chains by hand.

## Every stride-1880 site in the module

Searching the image directly for `addiu rt, rs, 1880` (opcode 9, immediate
0x758) -- 14 sites, the whole candidate set, found in one pass:

```
0x00066C88  26310758   addiu $s1, $s1, 1880   <- the failing dispatch loop
0x00066CE8  26310758
0x00066D5C  24020758
0x00066D84  24840758
0x00067884  26100758   addiu $s0, $s0, 1880   <- loop stride
0x00067A90  24020758
0x00067A94  24020758
0x00067AA8  24020758
0x00067AD0  24630758
0x00067F70  26100758   addiu $s0, $s0, 1880   <- loop stride
0x00067F7C  24020758
0x00073274  24060758
0x00073804  24060758
```

Only three advance a register by the stride in place (`$s1`/`$s0`), the mark of
a loop induction variable: the known-broken one at `0x00066C88`, and two
candidates at **`0x00067884`** and **`0x00067F70`**.

## 0x00067884 is inside a path both callers skip

`0x00067884` is the fall-through of `0x00067844` when its second argument is
*non-zero*:

```
00067854  addu $s1, $a1, $zero
00067858  beq  $s1, $zero, 0x00067878   ; a1 == 0 -> skip the loop entirely
00067868  jal  0x000122B4               ; compare a1 against a string constant
0006786C  addiu $a1, $a1, -1204         ; ...at 0x3B0000-1204
00067870  bnel $v0, $zero, 0x00067884   ; mismatch -> into the stride loop
```

`$a1` is a **string**: it is compared against a constant in the file image.
And both known call sites pass `a1 = 0` -- `0x00066C5C` and `0x0007EB08` -- so
both skip it.

That may well be correct behaviour rather than a bug; a name-matching path that
takes NULL to mean "default" is ordinary. It does mean this function is not the
missing constructor pass on these call paths.

## Next

`0x00067F70` is the remaining stride-loop candidate and has not been read yet.
Read it first: if it constructs elements of the array, find who calls it and
whether that caller runs.

## Correction: the stride addresses above are all wrong by 0x60

The byte-scan indexed the file and reported those offsets as addresses. The
`PT_LOAD` segment starts at **file offset 0x60**, so every address in that table
is 0x60 too high. The give-away was there and was missed: the scan reported
`0x00066CE8` for the failing loop, which had already been disassembled at
`0x00066C88` -- exactly 0x60 apart.

Corrected, the three in-place strides are:

```
0x00066C88   addiu $s1, $s1, 1880   <- the failing dispatch loop
0x00067824   addiu $s0, $s0, 1880
0x00067F10   addiu $s0, $s0, 1880
```

So the previous section's reasoning about `0x00067884` sitting inside
`0x00067844`'s string-compare path was about **a location that does not
contain that instruction**. The conclusion drawn from it is void. (The
disassembly quoted there was real; it just was not where the stride is.)

Scan file offsets, report addresses -- and cross-check any new hit against an
address already known by other means, which would have caught this instantly.

## 0x00067F10 is the constructor loop

```
00067EFC  addiu $s2, $s2, 1
00067F08  slti  $v0, $s2, 17
00067F0C  bne   $v0, $zero, 0x00067E34    ; loop head
00067F10  addiu $s0, $s0, 1880            ; stride, in the delay slot
00067F14  addiu $s2, $zero, -1
```

Seventeen iterations, stride 1880, counter in `$s2` -- the same shape and the
same array as the failing dispatch loop at `0x00066C6C`. **This is the pass
that is supposed to build the seventeen sub-objects.**

## Next

1. Does `0x00067E34`'s enclosing function ever run? Put a watch on it. If it
   never executes, find its caller; if it does execute, find why its stores do
   not land.
2. `0x00067824` is the remaining unexamined stride and should be read too.

## The constructor loop never runs

Watching `0x00067E34` directly:

```
NEVER RAN
```

Decisive, and it settles the shape of the bug. The seventeen sub-objects are
not being built badly, or half-built, or built somewhere else -- **the pass that
builds them is never entered.** Their vptr words are still the zero `.bss`
started as, which is exactly what the dispatch loop at `0x00066C6C` then reads.

That also retires the whole family of "the constructor ran but its stores did
not land" theories, including the codegen suspicion raised a few sections
above. Nothing landed because nothing ran.

## Its two call sites

```
psp_func_00067E34() called from 0x00067E08
                   called from 0x00067EF8
```

`0x00067E08` is a real function entry (`addiu $sp, $sp, -32`, saving `$ra` and
`$s2`-`$s4`) sitting immediately before the loop. `0x00067EF8` is inside the
loop body itself -- the back-edge, not an independent caller.

So there is effectively **one** entry point into this pass, and the question is
now one level up: does `0x00067E08` run, and if not, who was supposed to call
it?

## Next

1. Watch `0x00067E08` the same way. One of two answers, both useful: if it also
   never runs, walk up its callers until reaching code that *does* execute --
   the break is at that boundary. If it does run, the loop is being skipped by a
   guard, and that guard's condition is the bug.
2. This is a mechanical walk now, not a search. Each step is a watch and a run,
   and the answer is a yes or no rather than an inference.

## Walking up: the whole constructor family is dead code at run time

```
0x00067E34  NEVER RAN     (the 17 x 1880 constructor loop)
0x00067E08  NEVER RAN     (its only real entry point)
0x00066DB4  NEVER RAN     (one of 0x00067E08's callers)
```

`0x00067E08`'s callers are `0x00066DB4`, `0x00067038`, `0x00067258`,
`0x00067478`, `0x00067598` -- spaced roughly 0x220 apart, a run of near-identical
functions. That spacing, and the count, matches the seventeen subsystems: these
are the per-subsystem constructors, and each funnels into the shared
`0x00067E08` helper.

None of them execute.

## The sharp contradiction

`0x00066C34` -- the function that *dispatches* to all seventeen -- **does** run.
Its wrapper `0x0007EAE8` runs. The sub-object constructors do not.

So the program reaches the code that uses these objects without ever reaching
the code that builds them. That is not a subtle initialisation-order problem;
an entire tier of construction is being skipped while its consumer executes
normally.

Two readings, and they are distinguishable by measurement rather than argument:

1. **A separate construction pass exists and is never called.** Something at
   startup should invoke the seventeen constructors before anything dispatches
   to them; that call site is missing or unreached.
2. **`0x00066C34` is not the parent constructor.** It may be an `init` method
   that legitimately assumes prior construction, with the real constructor
   elsewhere -- in which case the break is further up still.

## Next

Find the callers of `0x00066DB4` and the other four siblings and walk up until
reaching a function that *does* run. The boundary between "runs" and "never
runs" is where the break is, and each step is a single watch and a single run
with a yes/no answer.

## The whole subtree is unreached, five levels up

```
0x00067E34  never ran     (17 x 1880 constructor loop)
0x00067E08  never ran     (its entry point)
0x00066DB4  never ran     (per-subsystem constructor)
0x0005057C  never ran     (its caller)
0x00074CC8  never ran  |
0x0007BA3C  never ran  |  all four callers of 0x0005057C
0x00082C14  never ran  |
0x00085A30  never ran  |
```

Every level is dead. The walk did not find a "runs / never runs" boundary
inside this subtree because the boundary is not in it -- **the entire
construction tier is unreached.**

## Re-reading the contradiction

The earlier framing was "the dispatcher runs but the constructors do not, so
construction is broken". With five dead levels that reading no longer fits.
Consider the other side of it:

Execution stalls after **470 function entries**. A game of this size runs
thousands during normal start-up. So the more economical explanation is not
that an initialisation tier is broken -- it is that **execution never gets far
enough to reach it**, and the dispatcher at `0x00066C34` is being entered from
some earlier path that should not have run yet, or should not have run at all.

That inverts the search. The question is no longer "why was construction
skipped" but **"why is a consumer of these objects running this early?"** --
which is a question about the path taken *before* `0x0007EAE8`, not about the
constructors.

## Next

1. Establish how `0x0007EAE8` is reached. Its caller chain, walked *downward*
   from `module_start`, will show which branch led here at 470 entries deep.
2. Compare against what should happen: if a `sceKernel*` call returned a wrong
   value early, a start-up state machine could take an unintended branch and
   land in a subsystem that has not been built yet. The firmware log is the
   place to check -- HLE calls that returned zero when the game expected
   something else.
3. Note this reframing before resuming: four sections above were written on the
   assumption that construction was broken. That assumption is now unsupported.

## The boundary, and a garbage argument

```
0x000743F0: RAN   a0=0x6A5DE983  s0=0x003FD160
0x0007EAE8: never ran
```

`0x00066C34` has exactly two callers. One never runs; the other **does** -- so
the path into the failing dispatcher is `0x000743F0`, and this is the
"runs / never runs" boundary the walk was looking for.

And `$a0` on entry is **`0x6A5DE983`**, which is not a pointer to anything. The
module occupies `0x00000000`-`0x004B0000` and RAM starts at `0x08000000`; that
value is in neither. It has the look of a hash or an uninitialised word rather
than an address.

So `0x000743F0` is entered with a garbage object pointer. Everything downstream
follows from that: it derives the array base from `$a0`, hands it to
`0x00066C34`, and the dispatcher then reads vptrs out of memory that was never
an object.

## What this displaces

The previous section's reframing -- "a consumer is running too early" -- is
half right and half wrong. The consumer is running at the *right* time, on the
*wrong* data. The seventeen constructors were never going to run for this
object because this object is not real.

That also explains the mixed vptr evidence cleanly: `0` where the garbage
pointer happened to land in zeroed `.bss`, and `0x003C61D8` where it landed
somewhere with data in it. Two different symptoms, one bad pointer.

## Next

Find `0x000743F0`'s caller and what it passes. `$a0 = 0x6A5DE983` is a specific
value -- search for where it is produced. A wrong return value from an HLE
call, a read through an uninitialised pointer, or a register clobbered across a
call would each produce it, and they are distinguishable by watching the
caller's `$a0` immediately before the call.

## Correction: 0x000743F0 does not take an object pointer

```
000743F0  lw   $a1, 0($s3)         ; uses $s3 -- set by earlier code
000743F4  jal  0x0000E06C          ; allocate
000743F8  addiu $a0, $zero, 32     ; ...32 bytes
000743FC  addu $s4, $v0, $zero
00074400  bnel $s4, $zero, 0x0007443C
```

It begins by reading through **`$s3`** and immediately overwrites `$a0` with
the constant 32 for an allocation call. `$a0` is not its argument -- this
address is a *continuation inside a larger function* that discovery split into
a separate entry, not a function entry with a calling convention.

So `a0 = 0x6A5DE983` observed at the watch was a leftover register value with no
meaning at that point, and the previous section's conclusion -- "entered with a
garbage object pointer" -- is **void**. The value also appears nowhere in the
file image, consistent with it simply being stale.

This is the third time a split-out continuation has been mistaken for a real
function in this investigation. The watch mechanism reports whatever `$a0`
holds; it cannot know whether the address it fires on is an ABI boundary.
**Before reading arguments at a watch, check that the address is a genuine
entry** -- a prologue (`addiu $sp, ...`) or first use of `$a0`-`$a3` rather than
a callee-saved register.

## What survives

- `0x000743F0` runs; `0x0007EAE8` does not. The reachability boundary is real
  and was measured, independent of the argument mistake.
- The seventeen constructors and their five ancestor levels never run.
- `0x000743F0` allocates 32 bytes via `0x0000E06C` and branches on the result
  being null -- so the allocator's return value is worth checking, given
  `psp_sysmem_free()` reports 20 MB free but the HLE allocator has never been
  exercised against this path.

## Next

1. Find the real entry of the function containing `0x000743F0` and watch that
   instead, then read `$s3`.
2. Check what `0x0000E06C` returns. It is the program''s allocator, called here
   for 32 bytes, and a null return sends control into the error path
   immediately below.

---

# Root cause: $k0 is never set

```
0000DD24  beq $k0, $zero, 0x0000DD44   ; k0 == 0 -> fallback path
0000DD28  lui $v0, 0x3B
0000DD2C  lw  $v0, 4($k0)              ; reent = *(k0 + 4)
0000DD30  beq $v0, $zero, 0x0000DD40
```

`$k0` is the PSP''s **thread pointer**. The kernel keeps a pointer to the
running thread''s control block there, and libc reaches its per-thread `reent`
structure -- which carries the **malloc state** -- through it.

The runtime has never set `$k0`. It is zero, so this takes the fallback branch
every time.

## The complete chain

```
$k0 = 0
  -> _getmodreent takes its fallback           ("libc:_getmodreent: no reent structure")
  -> the allocator at 0x0000E06C gets no heap
  -> the 32-byte allocation at 0x000743F8 returns null
  -> 0x00074400 branches into the error path
  -> the seventeen sub-object constructors never run
  -> their vptr words stay zero
  -> the dispatch loop at 0x00066C6C calls through null
  -> the hash tables those subsystems own stay zeroed
  -> the chain walk at 0x0004DD14 self-loops, 10 billion iterations
```

Every symptom recorded in this document hangs off that one unset register.
The `_getmodreent` warning has been in the logs since the earliest runs and was
noted as "returns a *working* fallback" -- it does return something, but a
fallback reent has no heap, and every allocation through it fails.

## Why this took so long to see

The search kept starting from the symptom and walking up. `$k0` never appears
in a trace, a register dump of `$a0`-`$s3`, or a call graph -- it is read by one
libc function, three levels below anything that looked interesting. The report
printed `ra/sp/gp/v0/v1/a0/a1/s0-s3` and would have shown this immediately had
it printed `$k0`.

## Next

1. Set `$k0` to a thread control block whose `+4` points at a `reent` with a
   working heap. `threadman` already models threads; this is where the TCB
   should come from, and the host should install `$k0` for the initial thread
   before entering `module_start`.
2. Verify by watching `0x000743F0`: the allocation at `+0x8` should return
   non-null and `0x00074400` should fall through rather than branch.
3. Expect further gaps once allocation works -- but every stall recorded here
   is downstream of this one register.

## Measured: the allocation really does return null

Rather than assume the error path was taken, watched its counterpart:

```
0x0007443C never ran  ->  the 32-byte allocation returned NULL
```

`0x0007443C` is the target of `bnel $s4, $zero` -- the success path. It never
executes, so `$s4` is zero and allocation genuinely fails. The chain in the
previous section is confirmed at its most important link.

## Setting $k0 alone is not sufficient

Installing a thread control block (`$k0 = TCB`, `*(TCB+4) = reent`, reent
zeroed) does **not** make allocation succeed. Still null.

So the diagnosis needs refining: a zeroed `reent` is not a usable one. Newlib
keeps its heap state inside that structure, and an all-zero heap has no memory
to hand out -- the pointer chain is now valid but empty. Whatever normally
fills it (a heap-initialising call at start-up, or an `sbrk` that obtains
memory from the kernel) is the missing piece, not the register itself.

`$k0` being unset is still real and still wrong -- it is why `_getmodreent`
takes its fallback -- but it is one of at least two things standing between
here and a working allocator.

## Next

1. Read `0x0000E0AC`, the allocator proper (called with the reent in `$a0` and
   the size in `$a1`). It will show which reent fields it consults and what it
   does when they are empty -- most likely a call to `sbrk`.
2. Find that `sbrk` and see where it expects memory to come from. If it is a
   `sceKernelAllocPartitionMemory` call, the HLE already implements that and
   the gap is small; if it expects a pre-set heap range, the host must
   establish one.
3. Do not assume the reent layout. Read the offsets `0x0000E0AC` actually
   touches, the way `$t2 = 1` should have been read rather than assumed.

## 0x0000E0AC is a real bucketed allocator

```
0000E0AC  addiu $sp, $sp, -32
0000E0B0  sltiu $v0, $a1, 17        ; size < 17 -> small-block path
0000E0BC  addu  $s4, $a0, $zero     ; s4 = reent
0000E0C4  addu  $s0, $a1, $zero     ; s0 = size
0000E0D4  bne   $v0, $zero, 0x0000E238
```

Not a thin wrapper around a kernel call -- it is a size-bucketed allocator that
manages its own free lists, with the request (32 bytes) taking the large-block
path. So the heap state it reads out of the `reent` is substantial, and an
empty structure genuinely has nothing for it to return. That is consistent with
the observed null and rules out "the reent just needs a valid pointer".

This is where the session stops. The remaining work is to read which offsets
`0x0000E0AC` consults on `$s4`, find the routine that populates them at
start-up, and determine why it has not run -- noting that it may itself be
downstream of `$k0`, since a start-up heap initialiser would plausibly reach
its reent the same way.

## Session summary

Established, with measurements:

- Allocation of 32 bytes at `0x000743F8` returns **null**; the success path
  `0x0007443C` never executes.
- The seventeen sub-object constructors, and five levels of their ancestors,
  **never run**.
- Their vptr words stay zero, so `0x00066C6C` dispatches through null 17 times.
- Hash tables owned by those subsystems stay zeroed, so the walk at
  `0x0004DD14` self-loops -- 10,020,314,003 iterations.
- `$k0`, the thread pointer, is never set, which is why `_getmodreent` takes a
  fallback. Setting it is necessary but not sufficient.

Fixed this session:

- `$ra` was never assigned by `jal`/`jalr` -- 2 assignments across 137,748
  instructions became 9,814. Real codegen bug affecting every call.
- Bad memory accesses 28 -> 3, by mapping the game''s own heap.
- Every label is dispatchable (13,682 entries), ending the class of bug where a
  computed jump lands on emitted-but-unreachable code.
- The GE rasterizes: triangles, strips, sprites, six tests.
- `vidt`, `vcst`, `viim`, `vfim`; `viim`/`vfim` also needed a decoder fix.

Tools added because counters kept hiding diagnoses: a wall-clock watchdog, loop
back-edge tracing, `psp_trace_watch`, bad-access address logging, dispatch-miss
caller logging, successful-indirect-call logging.

Falsified with evidence, so they are not retried: VFPU garbage, lost structure
writes, PRX relocations (twice, from two directions), a clobbered `$s0`,
broken construction, "a consumer running too early", and a garbage argument to
`0x000743F0` that turned out to be a stale register at a split continuation.

## module_start begins with two configuration calls

```
000183AC  addiu $sp, $sp, -16
000183B0  lui   $a2, 0x206
000183BC  ori   $a0, $a2, 0x10        ; a0 = 0x02060010
000183C8  jal   0x00091F1C
000183D0  lui   $a1, 0x3
000183D8  ori   $a0, $a1, 0x303       ; a0 = 0x00030303
000183D4  jal   0x00091F34
```

The very first thing the module does is call `0x00091F1C` with `0x02060010` --
a value with the shape of a size or a memory-configuration word (~33.8 MB),
not a pointer. A second call to `0x00091F34` follows with `0x00030303`.

These run before anything else, which is where heap configuration would live.
Both are worth reading before assuming the heap is set up elsewhere: if
`0x00091F1C` is what establishes the region the allocator later draws from,
then whether it succeeds determines everything downstream, and it is two
instructions into the program rather than five levels deep.

## Where to resume

The allocator (`0x0000E0AC` -> `0x0000E4DC`) is a real size-bucketed
implementation -- dlmalloc-shaped, rounding to 16 bytes -- so it draws on heap
state that something must populate. Three candidates, cheapest first:

1. `0x00091F1C` / `0x00091F34`, called immediately from `module_start`.
2. Whatever `_getmodreent`''s fallback returns, and whether its heap fields are
   ever filled.
3. `$k0`: setting it is necessary but was measured to be insufficient on its
   own, so it is a precondition rather than the fix.

The measurement that decides between them is the same one used throughout:
watch each, see which runs, and read what it writes.

## Ruled out: module_start''s first calls are not heap setup

`0x00091F1C` is **outside the code extent** (`0x00000000..0x00091E54`) -- it is
an import stub, not a function. Both resolve to SysMemUserForUser:

```
0x00091F1C -> NID 0x7591C7DB  sceKernelSetCompiledSdkVersion
0x00091F34 -> NID 0xF77D77CB  sceKernelSetCompilerVersion
```

SDK and compiler version registration, both already implemented as no-ops. The
`0x02060010` argument is a version word, not a memory size -- it decodes as
SDK 2.06.0010, which is exactly the shape it should have. The "~33.8 MB"
reading in the previous section was pattern-matching on a large hex value and
was wrong.

Worth noting as a general check: **an address that fails to disassemble is
information, not an error.** The code extent ends at `0x00091E54`, and anything
above it is a stub. That distinction would have saved a round here and is worth
applying to any `jal` target that looks unfamiliar.

## Candidate list, updated

1. ~~`0x00091F1C` / `0x00091F34`~~ -- version stubs, ruled out.
2. `_getmodreent`''s fallback and whether its heap fields are ever populated.
3. `$k0` -- necessary, measured insufficient alone.
4. **New, and now the most likely**: the heap is obtained lazily. A dlmalloc of
   this shape calls `sbrk` when its free lists cannot satisfy a request, and on
   PSP `sbrk` reaches the kernel through `sceKernelAllocPartitionMemory`. That
   import exists in the HLE. Whether the game ever calls it is checkable
   directly -- and no firmware call log from these runs shows it being reached.

## Next

Find the `sbrk` the allocator falls back to, and watch it. If it never runs,
the allocator is failing before it asks for memory; if it runs and returns
failure, the fault is in the HLE''s partition allocator rather than in the game.

## The kernel allocator is implemented and never called

`sceKernelAllocPartitionMemory` (NID `0x237DBD4F`) is registered in
`src/hle/sysmem.c` and works. Searching every run''s stderr across this session:

```
Partition | MaxFree | TotalFree  ->  no matches, any run
```

The game **never asks the kernel for memory**. So the allocator is not failing
because the HLE denied it a heap -- it fails before it ever gets that far.

That eliminates the most attractive remaining candidate and inverts the
question again: a dlmalloc with empty free lists should call `sbrk`, and `sbrk`
on PSP should reach the kernel. Neither happens. The allocator is bailing out
*before* attempting to grow the heap, which points at a guard -- a check on
some "initialised" flag or a null pointer in the reent -- that returns failure
without ever trying.

That is consistent with the `$k0` finding and explains why setting `$k0` alone
changed nothing: the pointer chain became valid, but the structure behind it is
still empty, and the allocator checks before it asks.

## The state of the search

Confirmed by measurement, not inference:

- The 32-byte allocation returns null; the success path never runs.
- `sceKernelAllocPartitionMemory` is never called.
- `$k0` is zero; setting it is necessary and insufficient.
- Everything downstream -- unbuilt subsystems, null vptrs, the self-looping
  chain walk -- follows from that single failed allocation.

The remaining question is narrow and well-posed: **which check inside
`0x0000E0AC`/`0x0000E4DC` fails before `sbrk` is reached?** Reading the branches
between entry and the first `jal` in `0x0000E4DC` answers it, and the offsets it
tests on `$s4`/`$s2` name the reent fields that must be populated.

## The guard chain inside the allocator

```
0000E50C  beq $a1, $zero, 0x0000E518
0000E524  bne $v0, $zero, 0x0000E594
0000E52C  jal 0x0000F27C              ; <- the arena/heap getter
0000E538  beq $v0, $zero, 0x0000E99C  ; <- returns 0 -> failure path
0000E54C  lw  $s0, 12($a0)            ; free-list head
0000E550  beq $s0, $a0, 0x0000E5B8    ; empty list
```

`0x0000E52C` calls `0x0000F27C` and fails out to `0x0000E99C` if it returns
zero. That is the guard the search was looking for -- the allocator asks
`0x0000F27C` for its arena, and gives up without ever reaching `sbrk` when the
answer is null.

Two observations that constrain it further:

- `0x0000F2B8` -- inside the same function region -- **does** run, and appears
  in the successful-indirect-call log calling `0x0000E594` five times. So this
  region executes; it is not dead code.
- `0x0000E594` was the *original* dispatch miss that started this session,
  before every-label-dispatchable fixed it. It is part of the allocator''s own
  machinery, which is a satisfying closure: the first symptom seen and the
  current one are neighbours.

## Next

Watch `0x0000F27C` and read its return value. It runs, so the question is
whether it returns null and why -- and its answer is what `0x0000E538` tests.
That single value is now the whole remaining question, and everything recorded
in this document hangs off it.

## Correction: 0x0000F27C is a lock helper, not an arena getter

```
0000F284  jal 0x00091E9C            ; -> NID 0x092968F4 sceKernelCpuSuspendIntr
0000F290  addu $a0, $v0, $zero      ; a0 = saved interrupt state
0000F294  lw   $v0, -11680($a1)     ; global at 0x003BD260
0000F298  bne  $v0, $zero, 0x0000F2A8
0000F2A4  sw   $a0, -11676($v0)     ; stash the state at 0x003BD264
```

Disable interrupts, save the cookie, bump a depth counter at `0x003BD260`.
This is a **recursive-lock acquire**, and calling it "the arena getter" in the
previous section was wrong -- the name was inferred from the caller''s use of
its return value, not from what it does.

The allocator''s `beq $v0, $zero, 0x0000E99C` is therefore testing a lock
result, not a heap pointer. Whether that is a *failure* branch at all is now
unclear; it may simply be the uncontended path.

## A concrete suspicion in the HLE

`hle_CpuSuspendIntr` (`src/hle/misc.c`):

```c
static uint32_t g_intr_enabled = 1;
static void hle_CpuSuspendIntr(void) {
    uint32_t prev = g_intr_enabled;
    g_intr_enabled = 0;
    psp_ret(prev);
}
```

The first call returns 1; **every nested call returns 0** until something
resumes. Recursive locks suspend interrupts repeatedly, so the inner
acquisitions all see 0. If any caller treats the returned cookie as
"non-zero means success", nesting silently breaks it.

Whether that is what happens here needs measuring, not assuming -- the last
several sections each turned on an inferred meaning that measurement then
overturned. Watch `0x0000F27C`, record its return value across calls, and see
whether `0x0000E538` branches on the first call or a later one.

## Next

1. Log `sceKernelCpuSuspendIntr`''s return across the run. Repeated zeros with
   no intervening resume would confirm the nesting flaw.
2. Read `0x0000E99C` -- the branch target -- to establish whether it is a
   failure path or ordinary control flow. The whole chain in this document
   assumes failure, and that assumption has not been checked.

## Correction: there is no "guard" -- 0x0000E99C is normal control flow

```
0000E99C  beq   $a0, $zero, 0x0000E9F4
0000E9A0  srl   $t1, $s1, 3
0000E9A4  srl   $v1, $s1, 6
0000E9A8  sltiu $v0, $a0, 5
0000E9B4  sltiu $v0, $a0, 21
```

Shifts by 3 and 6 and comparisons against 5 and 21: this is **dlmalloc''s bin
index computation**, not an error handler. So `beq $v0, $zero, 0x0000E99C` at
`0x0000E538` is an ordinary branch into bucket selection, and the allocator
proceeds normally through it.

**The "allocator bails at a guard before sbrk" model is wrong** and should not
be carried forward. It was built on naming `0x0000F27C` an arena getter (it is
a lock) and reading its caller''s branch as failure (it is not). Two inferences
stacked, neither measured.

## What actually still holds

Only what was measured:

- The 32-byte allocation at `0x000743F8` returns null -- `0x0007443C` never runs.
- `sceKernelAllocPartitionMemory` is never called in any run.
- `$k0` is zero; setting it alone changes nothing.
- The seventeen constructors never run; their vptrs stay zero; the chain walk
  at `0x0004DD14` self-loops.

Those are facts. Everything connecting them into a story about *where* inside
the allocator the failure happens has now been wrong twice.

## How to attack it without another inference chain

Stop reading the allocator statically. It is a full dlmalloc and reading it
branch by branch has produced two wrong models in two turns.

Instead **bisect it by measurement**: `0x0000E4DC` is the entry and the null
comes back to `0x000743F8`. Put watches at a handful of addresses spread
through the allocator, see which are reached, and narrow to the block that
decides. That is the same technique that found the constructor tier boundary in
one turn after inference had failed for several.

## The bisect was invalid: watches only fire on function bodies

`0x0000E5B8` and `0x0000E99C` both reported "not reached". Both results are
**artifacts**. Neither address has a `psp_body_*`; they are labels inside
`psp_body_0000E4DC`, and `PSP_ENTER` -- which is what `psp_trace_watch` hooks --
is emitted once per body with the *function''s* address. A watch on an interior
label can never fire.

So the watch tool silently reports "not reached" for any address that is not a
function entry, which is indistinguishable from a real negative. Every "never
ran" result in this document was on a genuine entry and stands; but the tool
needs a guard, and until it has one, **check `psp_body_<addr>` exists before
trusting a negative.**

## Measured: the allocator receives a stack address as its reent

Watching real entries:

```
0x0000E0AC  REACHED  a0=0x09FFFC20  s0=0x00000020
0x0000E4DC  REACHED  a0=0x09FFFC20  s0=0x00000020
```

`s0 = 32` is the requested size, correct. `a0` is the reent pointer, and
`0x09FFFC20` is **at the top of RAM, in stack territory** -- the stall report
showed `sp = 0x09FFFAB0`, a few hundred bytes below it.

A newlib reent carries the malloc arena and must live in static or heap memory.
A pointer into the stack cannot be a valid one: whatever is there is either
uninitialised or about to be overwritten by the next deep call. That is exactly
consistent with an allocator that finds no usable free lists and returns null.

Note also that the host now sets `$k0 = 0x08800000` with `*(k0+4) =
0x08800100`, yet the allocator receives `0x09FFFC20`. So this reent is **not**
coming from the thread-pointer path -- `_getmodreent` is returning something
else, from its fallback.

## Next

Watch `0x0000DD24` (`_getmodreent`) and read `$v0` on return. It is a real
entry, so the watch will fire. Three outcomes, all informative: it returns the
stack address (the fallback is broken), it returns the TCB reent and something
overwrites `a0` in between, or it is not the function feeding this call at all.

---

# A falsification made under broken codegen was wrong

The "no reent structure" message is a **string in the game''s own image** (file
`0x0944B1`), not something psprecomp prints. It was read for most of this
investigation as our diagnostic; it is the game''s libc reporting that it could
not find its reent.

Re-running the long-falsified ModuleMgr hypothesis, now that `jal` assigns
`$ra`:

```
                        before        after returning a module id
game''s reent message    printed       gone
bad memory accesses     3             0
```

The original test concluded "byte-identical output, so this call is not on that
path". **That test ran while every non-leaf function in the program returned
through a stale `$ra`.** A falsification obtained under broken codegen is not a
falsification, and this one was wrong.

That matters beyond this one call: every theory in this document falsified
*before* the `$ra` fix was tested against a program whose calls did not return
correctly. The VFPU-garbage and lost-structure-writes results were both from
that era. They are now unreliable and should be re-run before being trusted.

Returning an id is also the truthful answer, not a convenient one. The question
is "which module owns this address"; a self-contained microgame is exactly one
module; the host has loaded it. `UNKNOWN_MODULE` denies a module that
demonstrably exists.

## Still unresolved

The hang is unchanged: `0x0004DD14`, ten billion iterations, `pixels written: 0`.
The allocator still receives `0x09FFFC20` as its reent. So the reent lookup now
succeeds while the allocation still fails -- which means the reent it finds is
not one with a usable arena, and the next question is what populates *that*.

---

# Retraction: the allocator was never shown to fail

The reent is **valid and populated**:

```
allocator entry 0x0000E0AC: reent=0x09FFFC20
  9 of first 64 words non-zero
    +00 = 0x00000000      _errno
    +04 = 0x09FFFE88      _stdin
    +08 = 0x09FFFEE4      _stdout
    +12 = 0x09FFFF40      _stderr
```

That is newlib''s `_reent` layout exactly. So "a stack address cannot be a valid
reent" was wrong -- the reent legitimately lives high in RAM, and it is
initialised.

Worse, the measurement the whole allocator theory rested on was an artifact:

```
psp_body_0007443C: does not exist
```

`0x0007443C` is a **branch target inside a function**, not an entry, so the
watch could never fire on it. Its "never ran" result meant nothing, and
"the 32-byte allocation returns null" was never measured at all. The blind spot
recorded two sections earlier is exactly the one that produced it.

Auditing every address this investigation drew a conclusion from:

```
0x0007443C  NOT an entry   -> conclusion retracted
0x00067E34  entry          -> stands
0x00067E08  entry          -> stands
0x00066DB4  entry          -> stands
0x0005057C  entry          -> stands
0x000743F0  entry          -> stands
0x0007EAE8  entry          -> stands
```

## What survives, and it is less than the last few sections claimed

Measured on real entries:

- The seventeen sub-object constructors and five ancestor levels **never run**.
- Seventeen null vtable dispatches at `0x00066C78`.
- The chain walk at `0x0004DD14` self-loops -- 10,270,856,424 iterations.
- `sceKernelAllocPartitionMemory` is never called.
- The reent is valid; the module id is now reported and bad accesses are 0.

**Retracted:** that allocation fails, that `$k0` explains it, that a guard
before `sbrk` is responsible. The reent being sound removes the motive for all
three.

## Next

Establish, on a real entry, whether allocation actually succeeds -- watch
`0x0000E06C` (an entry) and record `$v0` when it returns, rather than inferring
from a branch target. Then re-ask why the constructors do not run, without
assuming memory is the reason.

## Confirmed by positive measurement: the allocation succeeds

The emitter now marks **all 13,682 labels** (`PSP_MARK`), so any address it
gave a label to can be watched, not just function entries. Re-running the
measurement that was previously unobservable:

```
0x0007443C REACHED  ->  the 32-byte allocation SUCCEEDS
```

`0x0007443C` is the success path. It executes. So the allocator works, and the
entire chain built on "allocation returns null" is dead -- this time by a
positive result rather than a retraction.

The tooling fix earned its cost on its first use. It also means every earlier
"never ran" verdict is now re-checkable rather than merely suspect.

## Corrected state of the investigation

Measured and standing:

- The allocation **succeeds**; the reent is valid and populated.
- `$k0` is unset (still wrong, still worth fixing) but is **not** why anything
  here fails.
- The seventeen sub-object constructors and five ancestor levels never run --
  measured on real entries, so these stand.
- Seventeen null vtable dispatches at `0x00066C78`.
- The chain walk at `0x0004DD14` self-loops, ~10.27 billion iterations.
- `sceKernelAllocPartitionMemory` is never called -- consistent, since the
  allocator satisfies requests from an arena it already has.

Dead, with evidence: allocator failure, `$k0` as root cause, a guard before
`sbrk`, a stack-address reent, PRX relocations, VFPU garbage, lost structure
writes, a clobbered `$s0`, "a consumer running too early".

## The question, restated cleanly

Memory works. The reent works. Calls return correctly now. Yet an entire
seventeen-element construction tier never executes while the code that
dispatches to it does.

So the divergence is a **control-flow decision**, not a resource failure --
something branches away from construction. With label marking in place this is
directly attackable: mark-check the branches between `0x0007EAE8` (never runs)
and `0x000743F0` (runs), and find the first point where the executed path
leaves the path that would have constructed.

---

# The causality was backwards

Reachability, measured with label marking:

```
0x00018318  REACHED        <- start-up function
0x0001835C  never reached  <- 17 instructions later, same function
```

Those are not two functions. They are consecutive statements:

```
00018354  jal 0x000742A8       ; REACHED -- and never returns
00018358  addu $a2, $s0, $zero
0001835C  jal 0x00072FC4       ; never reached
00018364  jal 0x00074AA4       ; never reached -> the construction tier
```

`0x000742A8` is entered at `0x00018354` and **never returns**, because the hang
at `0x0004DD14` is inside its subtree. Everything after that call -- including
`0x00074AA4`, the root of the seventeen-constructor tier -- is unreachable for
the ordinary reason that the program stopped.

**So "the construction tier never runs" is a consequence of the hang, not its
cause.** The model driving many sections of this document had it exactly
backwards: the constructors were never skipped, they were simply scheduled
after a call that never came back.

That also dissolves the "sharp contradiction" recorded earlier -- that the
dispatcher runs while its constructors do not. There is no contradiction. The
dispatcher runs first, on objects not yet built, because in this program it is
*supposed* to run before them: the seventeen null vtables at `0x00066C78` are
the normal state at that point in start-up, not evidence of failure.

## What the remaining question actually is

Only one thing needs explaining: **why does the chain walk at `0x0004DD14`
self-loop?** Everything else in this document is either downstream of that or
was a misreading of it.

Recall what is known about it, and note that it now needs re-examining without
the assumption that its table *should* have been initialised:

- Terminator is `$t2 = 1`, set at `0x0004DCFC`.
- The table''s next-fields are `0`, so entry 0 points at itself.
- Patching that self-loop at run time advances execution to a second table with
  the same shape at `0x00067E58`.

If a zeroed table is the legitimate start state, then the walk is meant to stop
some other way and the recompiled code is getting a condition wrong -- which
puts the fault back in codegen, where `$ra` already proved one bug lives.

## The codegen hypothesis is dead: the emitted loop is correct

```c
    /* 0004DD44  bnel  $v0, $t2, 0x0004DD14 */
    if (r_v0 != r_t2) {
    /* 0004DD48  sll   $a2, $v0, 4 */
        r_a2 = psp_sll(r_v0, 4);
        goto L_0004DD14;
    }
```

`bnel` nullifies its delay slot when not taken -- emitted exactly right. The
`bne` forms above it evaluate the condition *before* the slot, also right. There
is no codegen fault in this loop.

Trace it by hand with a zeroed table: `$t1 = 0x04153FFF`, entry 0''s key is 0,
they differ, `$t3` stays 0, `lhu $v0, 16($t0)` reads 0, `0 != 1` so the branch
is taken, `$a2 = 0 << 4 = 0`, and the walk returns to entry 0. **Real hardware
would spin here too.**

That is the useful conclusion: the loop is not wrong, and a zeroed table is not
its legitimate start state. The table at `0x003EC384` genuinely must be
initialised -- with `next = 1` in at least entry 0 -- *before* this walk, by
something on the path that already executes.

## Where this leaves it

One question, well-posed and no longer entangled with anything else:

**What writes the sentinel into the table at `0x003EC384`, and why has it not
run by the time `0x0004DD14` walks it?**

Note it is reached via `0x0004DA58`, which loads its sentinel differently --
`lhu $t2, 24($a0)` rather than the constant 1 at `0x0004DCFC`. Two callers with
two notions of the terminator is worth checking directly: if the table was
built for the `+24` sentinel (which reads 0) and is being walked by the code
that expects 1, the table is fine and the *caller* is wrong.

That is a concrete, cheap next measurement and it does not depend on any of the
models this document went through.

## The table is untouched .bss

```
table 0x003EC384 header: +16=0x0000 +24=0x0000
  entry 0: key=0 val=0 used=0 next=0x0000
  entry 1: key=0 val=0 used=0 next=0x0000
  entry 2: key=0 val=0 used=0 next=0x0000
  entry 3: key=0 val=0 used=0 next=0x0000
```

Every field zero, `used = 0` on every entry: **nothing has ever been written to
this table.** It is virgin `.bss` at the moment the walk spins on it.

Note that `0x0004DBB0` -- the insert routine -- *is* in the entry trace. So
inserts ran, but not into this table: they either targeted a different one or
bailed before writing. Either way the table `0x0004DD14` walks was never
created.

`+16 = 0` is the header''s head-index, and `0x0004DD00` exits immediately when it
equals 1. So an initialised-but-empty table has `+16 = 1`, and this one has 0 --
the difference between "empty list" and "never created".

## The remaining question, now fully specified

**What writes `1` to `0x003EC384 + 16`, and why has it not run?**

That is a single, searchable fact rather than a theory: a table-create routine
that sets the head index to the terminator. It exists somewhere on the path
that already executes, and finding it does not depend on any model this
document proposed and later withdrew.

The measurement to take first: watch writes to `0x003EC384` during the run. If
nothing ever writes it, the create call is missing from the executed path; if
something writes zero, that writer is the bug. `psp_mem`''s write path already
has a hook point -- `bad_access` was added there in one edit -- so a
watch-on-address is a small change.

# Session close

Achieved: a tested rasterizer, total dispatch reachability (13,682 entries),
label-level reachability marking, loop back-edge tracing, a wall-clock
watchdog, `$ra` assigned on every call (a real codegen bug -- 2 sites became
9,814), bad memory accesses 28 -> 0, VFPU `vidt`/`vcst`/`viim`/`vfim`, and a
module-id fix that removed the game''s own libc diagnostic.

Not achieved: **WTF does not render.** `pixels written: 0`. The game stalls on
the walk described above, before submitting any geometry.

The methodological lessons, in the order they cost the most:

1. A measurement tool that cannot distinguish "did not happen" from "could not
   be observed" manufactures confident wrong conclusions faster than reasoning
   does. Label marking fixed this and overturned a wrong answer on first use.
2. Falsifications obtained before a codegen fix are not falsifications. The
   `$ra` bug invalidated an entire era of this document''s conclusions.
3. Derive constants from the code. A hard-coded sentinel wrongly retired a
   correct theory for several rounds.
4. Counters hide diagnoses. Every real finding came from logging an address.

## The shim does not help: it is not one table

Marking a demonstrably virgin table as empty at `0x0004DCF8`:

```
shim: marked virgin table 0x003EC384 empty
last loop back-edge: 0x0004DD14 (6,997,249,880 hits)
pixels written: 0
```

The shim fires once and the spin continues at the same rate. The back-edge
address is shared by every walk, so this is consistent with the earlier
single-table patch that merely revealed a second table: **the missing
initialisation is systemic, not one structure.**

That is the more useful reading of both experiments together. Whatever
establishes these tables establishes many of them, in one pass, and that pass
is what is absent. Chasing them one at a time -- by shim or by constructor --
cannot converge.

## Final state

`pixels written: 0`. WTF does not render.

The single question to resume on is unchanged and now better supported:
**what creates this family of tables, and why has it not run?** It is one
routine, it runs early, and it is missing from the executed path. The tools to
find it exist: label marking gives yes/no reachability for any of the 13,682
labelled addresses, and a write-watch on `0x003EC384` would name the writer
directly if one ever appears.

Do not resume by patching tables. Two experiments now show that path does not
converge.

---

# The list initialiser, and what actually walks the table

A write-watch on the head index (`0x003EC394`, i.e. table `+16`) across a whole
run:

```
NOTHING EVER WRITES IT
```

Scanning the image for `sh rt, 16(rs)` -- 16 sites in the module -- found the
routine that would:

```
0004D730  addiu $v0, $zero, 1
0004D734  sb    $v0, 12($s0)     ; used = 1
0004D738  sh    $v0, 16($s0)     ; next = 1     <- the terminator
0004D73C  sh    $v0, 14($s0)     ; prev = 1
```

An empty circular doubly-linked list: entries carry **prev at +14 and next at
+16, both 16-byte indices**, and an initialised-empty list points at itself with
index 1. A virgin `.bss` table reads 0 in both, and 0 is a valid index, which is
why the walk self-loops rather than stopping.

Reachability along that path:

```
0x0004D694  never reached   (guards the init on 0x000761DC returning < 513)
0x000761DC  never reached
0x0004D6F0  never reached   (the list initialiser)
0x0004DBB0  REACHED         (insert)
0x0004DD14  REACHED         (walk)
```

**Insert and walk run; the initialiser and its entire guard path do not.** That
is now measured on real entries at every level, not inferred.

## What that means

This is the same shape as the seventeen-constructor tier, and that one turned
out to be *downstream* of the hang -- unreachable because the program had
already stopped. The same explanation has to be tested here before assuming
otherwise: `0x0004D694` may simply be scheduled after the call that hangs.

If so, the ordering is the bug: the program walks a list it has not created
yet, which on hardware would also spin. That points back at something earlier
diverging -- and the first divergence found by this method was `0x00018354`
calling into the subtree that never returns.

## The specific next measurement

Walk up from `0x0004D694` exactly as before, marking each level, until reaching
a function that **is** reached. If that boundary is again "the statement after
the call that hangs", the initialiser is downstream and the real fault is
earlier still. If instead a reached function *skips* the call to
`0x0004D694`, that branch is the bug.

Do not shim the tables. Two experiments show it only reveals the next one.

---

# Found it: the static constructors never ran

`0x003B2B40` calls the list-initialiser path and **nothing in the program calls
it**. Scanning `.data` for runs of code-like pointers found why:

```
constructor table at 0x003B2FE4:
  0x003B2FE4 -> 0x003B2AF8
  0x003B2FE8 -> 0x003B2B40      <- the list initialiser''s caller
  0x003B2FEC -> 0x003B2B7C
  0x003B2FF0 -> 0x003B2DF0
  0x003B2FF4 -> 0x003B2E94
  0x003B2FF8 -> 0x003B2F38
  0x003B2FFC -> 0x003B2F88
  0x003B3000 -> 0x003B2FCC
  0x003B3004 -> 0x00000000      <- terminator
```

A null-terminated array of eight function pointers into code above `.text` --
a C++ **static-constructor table**. The routine that walks it is part of crt0,
which a PRX gets from the loader rather than from its own image, so nothing in
the recompiled program was ever going to call it.

That is the systemic cause every symptom pointed at: **one missing pass, not
many missing initialisers.** Global objects were never constructed, so linked
lists read as virgin `.bss` and vtable slots read as zero.

Running the eight from the host before `module_start`:

```
ran 8 static constructors
before:  last loop back-edge 0x0004DD14   (~10 billion hits)
after:   last loop back-edge 0x00067E58   (~3.2 billion hits)
```

**The hang moved.** The chain walk that has dominated this entire investigation
is fixed. `0x00067E58` is the second table -- the one the earlier single-table
shim had already revealed, which is consistent: that experiment was patching a
symptom of exactly this cause.

## Why the search took so long to arrive here

Every level of the walk-up said "never reached", and that was read as "the
program stopped before getting here". For the constructor tier that reading was
correct. Here it was not: this code is never reached *by anything*, because its
only reference is a data pointer, not a call. A caller search over generated C
cannot see that, and reachability alone cannot distinguish "not yet" from
"never".

What resolved it was noticing `0x003B2B40` had **no callers at all** -- an
orphan with live code in it -- and asking what could reach it.

## Next

1. Look for further constructor or init arrays. Candidate pointer runs at
   `0x003B1038`, `0x003B15D0`, `0x003B16F0`; the runs at `0x003B314C` and
   beyond are 26 entries each and look like vtables rather than init arrays.
2. `0x00067E58` is the new stall. Check whether it is another uninitialised
   structure whose constructor is in a table not yet run.
3. This belongs in the module loader, not the host: a PRX''s constructor tables
   should be walked at load time.

## The new stall is inside the tier that now runs

`0x00067E58` is a loop *target*, not a call site -- `PSP_LOOP` records branch
destinations, and a backward branch lands here:

```
00067E4C  beql $s1, $v0, 0x00067EA0    ; s1 reached s0+184 -> exit
00067E54  addu $a0, $s3, $zero
00067E58  jal  0x000122B4              ; string compare(s3, s1+12)
00067E5C  addiu $a1, $s1, 12
```

A name-lookup loop walking a list of records, comparing a string against each
entry''s `+12` field. It is inside `0x00067E34` -- the seventeen-element
constructor tier that this document spent many sections establishing "never
runs". **It runs now.** Static constructors were what stood between execution
and this code.

So the failure has moved from "a list that was never created" to "a lookup that
never finds its entry and never terminates" -- one level up the same subsystem,
and a different kind of bug.

## Where the session ends

`pixels written: 0`. WTF does not render.

But the blocker is no longer the one that dominated this document. Both
searches that ran the longest -- the self-looping chain walk and the unbuilt
constructor tier -- are resolved by a single cause, and the program now
executes into code it had never reached.

### The fix that matters, and where it belongs

Running the constructor table from the host is bring-up scaffolding. It belongs
in the module loader: **a PRX''s static-constructor array must be walked at load
time**, the way the real loader''s crt0 does. That is a small, well-defined piece
of work and it is the first thing to do next.

### Resume here

1. Move constructor-table handling into the loader; find the array by scanning
   for a null-terminated run of code pointers, or from section headers if the
   module has them.
2. `0x00067E58`: read what the lookup is searching for and why it misses.
   `0x000122B4` is the comparison; `$s3` is the sought name.
3. Re-run the theories falsified before the `$ra` fix -- that fix invalidated an
   era of this document''s conclusions, and one has already been overturned.

## State after the constructors run

The eight constructors are all properly recompiled (`psp_body_003B2AF8`,
`003B2B40`, `003B2B7C`, `003B2FCC` all exist), and the analysis extent covers
the whole image -- the `0x00091E54` bound that `dis` enforces is a section
bound, not the analysis extent. So they are real functions and they execute.

The new stall is a circular-list walk:

```
00067E6C  lw   $s1, 4($s1)             ; next
00067E90  addiu $v0, $s0, 184          ; sentinel = s0 + 184
00067E94  bnel $s1, $v0, 0x00067E58    ; loop until back at the sentinel
```

with the string compare reading garbage:

```
bad read8 at 0x32FEDBDB (last fn 0x000122B4)
```

`$s1` is following uninitialised memory, so `s1 + 12` is not a string. Another
list whose head at `s0 + 184` was never made self-referential -- the same class
of defect the static constructors fixed elsewhere, on a structure they do not
cover.

Two readings, and they are cheaply distinguishable:

1. **Ordering.** The host runs the constructors *before* `module_start`, but
   crt0 runs them after the C runtime is up. A constructor that allocates or
   depends on runtime state would behave differently. Worth testing by running
   them later, or by finding where the game itself expects them.
2. **Coverage.** Discovery reaches only **14.18% of .text**. If the routine that
   initialises this list was never discovered, it cannot be called. That number
   has been visible all session and never questioned; it deserves to be.

Reading 2 is the more troubling one and is cheap to check: confirm whether a
function initialising `s0 + 184` exists in the generated output at all.

## Coverage was never 14%: a reporting bug

```
before:  reached: 550992 bytes (14.18% of .text)
after:   reached: 550992 bytes (92.0% of .text, 598892 bytes)
```

The denominator was the whole loaded image -- code plus `.data` plus `.bss` --
while the label said `.text`. Discovery covers **92%** of the executable range,
not 14%.

So the coverage lead is dead, cheaply, and the tool no longer misreports. But
the number had been printed on every run of this session and read past every
time. A statistic that is wrong by six times and describes the single most
important property of a static recompiler is exactly the kind of thing that
should have been checked on day one; it survived because it looked like
background noise rather than a claim.

Fixed by deriving the executable extent from the import stubs, which sit at the
end of `.text`, and by naming the denominator in the output when it is not
known.

## Standing state at session end

`pixels written: 0`. WTF does not render.

Fixed and shipped this session:

- `$ra` was never assigned by `jal`/`jalr` -- 2 assignments across 137,748
  instructions became 9,814. Affected every call in the program.
- Static constructors never ran; found the table at `0x003B2FE4` and running
  its eight entries moved the hang and reached previously-dead code.
- Bad memory accesses 28 -> 0.
- Every label dispatchable (13,682 entries).
- The GE rasterizes (triangles, strips, sprites, six tests).
- `vidt`/`vcst`/`viim`/`vfim`, plus a decoder fix for the immediate forms.
- Module id reported, silencing the game''s own libc diagnostic.
- Coverage statistic corrected: 92%, not 14%.

Open: an uninitialised circular list at `s0 + 184` reached from
`0x00067E58`, with the constructors now running. Next steps in the section
above.

## Falsified: constructor ordering is not the issue

Running the eight constructors from inside start-up (at `0x00018318`, after
`module_start` has begun) instead of before it:

```
                    before module_start    inside start-up
back-edge           0x00067E58             0x00067E58
bad accesses        22                     22
dispatch misses     19                     19
```

Identical. Reading 1 from the previous section is dead: **when** the
constructors run does not matter, so they do not depend on runtime state that
`module_start` establishes.

A useful detail from the failed first attempt: watching `0x00018334` fired
nothing, because that address is not a label. The run then silently behaved as
if constructors were disabled -- and reproduced the *old* hang at `0x0004DD14`
with 0 bad accesses, which is a neat independent confirmation that the
constructors are what moves execution past it.

That also shows the label-watch blind spot is still live for non-label
addresses. `psp_trace_watch_label` fires only on emitted labels; anything else
is silently inert. The guard that would say so up front is still worth adding.

## What remains

The list at `s0 + 184` walked from `0x00067E58` is uninitialised even with
constructors run. Its head should be self-referential and reads as garbage
instead, so the string compare at `0x000122B4` follows nonsense pointers -- the
22 bad accesses.

Since ordering is ruled out and the constructor table is fully walked, the
initialiser for *this* structure is either:

- a ninth constructor in a table not yet found (only one null-terminated array
  was located; a second `.init_array` would look identical), or
- an ordinary function that start-up reaches only after the current stall.

The second is the more likely and the cheaper to check: mark-test whether the
code between `0x00018318` and the stall contains an initialiser for it.

## Static constructors are now part of the toolkit

`src/ctors.c` + `include/psprecomp/ctors.h`, with `psp_ctors_find()` and
`psp_ctors_run()`. Ten tests in `tests/test_ctors.c`.

The find is a *proposal*, not a proof, and the tests say so by pinning what it
must reject: an unterminated run (vtables look identical otherwise), a single
pointer followed by zero (occurs everywhere in ordinary data), and pointers
outside the code range. Mistaking a vtable for a constructor table would call
arbitrary methods with no arguments during start-up -- a failure that would
look like anything except its cause, which is exactly the class of bug this
whole document is a record of.

Running the constructors belongs at load time. The host currently calls them
explicitly; a real module loader should do it as part of bringing a PRX up,
because *every* recompiled C++ module has this problem and none of them will
report it.

## What the constructors unlocked

With the constructor table run:

```
0x0004D6F0  REACHED        (the list initialiser -- was never reached)
0x00067E08  REACHED        (subsystem tier)
0x00066C34  REACHED
0x00066DB4  never reached  (one of five callers of 0x00067E08; another is used)
```

`0x0004D6F0` -- the routine that writes the terminator into a list head, and
the thing this investigation spent many sections hunting -- **now runs**. So the
constructor pass reaches the list-initialisation machinery, which is exactly
what it was supposed to do.

The subsystem tier that this document repeatedly recorded as "never runs" also
executes now, via a different one of its five callers than the one traced
earlier.

The stall at `0x00067E58` is therefore a *different* list from the one
`0x0004D6F0` initialises: one whose head at `s0 + 184` is still zero. Since the
constructor pass demonstrably reaches list-init code, the question is narrow --
which lists does that pass cover, and what covers this one.

## The first table is now initialised — proof the fix works

```
table 0x003EC384 header: +16=0x0002    (was 0x0000)
```

Non-zero, and reached through the constructor pass. The structure that spun ten
billion times is genuinely built now, not worked around.

## The second list has no writer either

```
regs at stall: s0=0x0030BF24 s1=0x00000000 s3=0x00312D60
```

`$s1` is **zero** mid-walk: the loop does `s1 = *(s1 + 4)` and read a null next
pointer, so it then reads `*(0 + 4)` -- address 4, inside the module header --
and follows whatever is there. That is the garbage the string compare chokes on.

A write-watch on the list head (`s0 + 184` = `0x0030BFDC`):

```
NOTHING WRITES THE STALLED LIST HEAD
```

Same signature as the first table before the constructors were found, and the
same technique identified it. So there is a second family of structures whose
initialiser never runs.

Note the constructor table found was the *longest* null-terminated run in the
region; `psp_ctors_find` returns only the best candidate. A second, shorter
array would be invisible to it. That is a plausible and cheap next check, and
the API should probably enumerate rather than pick.

## Session end

`pixels written: 0`. WTF does not render.

The blocker is one structural question with a proven method behind it: **find
what initialises the list at `s0 + 184`.** A write-watch says nothing does; the
same watch on the previous table led to the static-constructor table, which
turned out to explain a dozen separate symptoms.

## The stalled list head is not a list head

Only one null-terminated array in the whole image points into the high code
region, so there is no second constructor table there -- that possibility is
closed.

More useful: `s0 + 184` (`0x0030BFDC`) lies **inside the file image**, so it is
loaded from disk rather than being `.bss`. What the file has there:

```
file 0x0030BFDC = 0xCD3FC9BD
file 0x0030BFE0 = 0x654AFF35
file 0x0030BF24 = 0xDEFE35B3
```

Noise -- not pointers, not zero, not a plausible list head. And `$s0` does not
line up with `$s3` as an array element: `0x00312D60 - 0x0030BF24 = 0x3E3C`,
which is not a multiple of the 1880-byte stride.

**So `$s0` is a bad pointer, not an uninitialised structure.** The walk is
reading a region of ordinary data as if it were a linked list, which is why the
next pointer is null on the first step and why the string compare follows
nonsense.

That reframes the remaining bug: it is not another missing initialiser. It is
wrong pointer arithmetic or a wrong base somewhere in the subsystem loop --
a different class, and one where the `$ra` fix has already proved that codegen
bugs of exactly this kind exist in this toolkit.

## Resume here

1. Watch `0x00067E34` and record `$s0` and `$s3` on entry, then compare against
   the stride and the array base. If `$s0` does not start at `$s3`, the base is
   wrong; if it does, the stride or the loop bound is.
2. `psp_ctors_find` returns only the longest candidate. It should enumerate;
   the current API would hide a second, shorter table.
3. Re-run every theory falsified before the `$ra` fix. That fix invalidated an
   era of this document and one falsification has already been overturned.

## Correction, and what the structure actually is

`$s0` strides correctly:

```
subsystem loop #1: s0=0x0030B074  delta from s3 -31980
subsystem loop #2: s0=0x0030B7CC  delta -30100   (+1880)
subsystem loop #3: s0=0x0030BF24  delta -28220   (+1880)
```

Exactly 1880 per iteration, starting at `$a0`. **So the pointer arithmetic is
right and the previous section''s "bad pointer" conclusion is withdrawn.**

`$s3` is not an array base either -- it sits 20 bytes past the end of the
17-element array and is passed to `strcmp` as its first argument. It is the
*name being searched for*.

And the region is not noise. Reading it as text:

```
/cygdrive/d/sce_prj/psp/project/hell2k/data/final_na/en/game/b02/b02_model/0.0
```

An **asset table**: seventeen 1880-byte records, each holding a resource path,
each with a linked list at `+184` of what has been loaded for it. The earlier
"file image is noise" reading was wrong -- four bytes were sampled where a
string begins mid-field.

## What the stall means now

The game is looking up a loaded resource by name across seventeen asset slots.
The lists are empty because nothing has been loaded yet, which is correct at
this point in start-up -- but an empty circular list must have its head point
at itself, and these read zero.

So this is the *same* defect as the first table: a list head that was never
made self-referential. The constructor pass fixed that for one family of lists
and does not cover this one.

Two candidates, in order:

1. These records are initialised when their asset is first touched, by a
   routine start-up has not reached -- making this downstream again.
2. Their initialiser is a constructor in a table `psp_ctors_find` did not
   return, since it yields only the longest candidate.

The second is cheap to settle and the API flaw is already recorded.

Note the game reaches resource lookup at all: it has run far enough to ask for
`b02_model`. That is considerably further than any earlier point in this
document.

## Game data is now present, and it changes nothing yet

The microgame `.dat` containers were extracted from the disc to the host''s
`iso_root` (never committed -- `work/` is gitignored and the repo carries no
game data). Re-running:

```
last loop back-edge: 0x00067E58   (unchanged)
no sceIo activity at all
```

**The game never reaches file I/O.** It stalls in the asset-name lookup before
opening anything, so the missing data was not the blocker -- a clean negative
that costs one run and removes an obvious suspicion permanently. The files are
in place for when execution does get that far.

# Where this ends

`pixels written: 0`. WTF does not render.

## The single remaining question

Seventeen asset records, each with an empty resource list at `+184`. An empty
circular list must have its head point at itself; these read zero, so the walk
follows a null and spins. The static-constructor pass fixed exactly this defect
for one family of lists and does not cover these.

Next: settle whether their initialiser is (a) a constructor in a second table --
`psp_ctors_find` returns only the longest candidate and would hide one -- or
(b) a lazy per-asset routine that start-up has not reached. Option (a) is
cheaper and the API flaw is already known.

## What shipped

- **`$ra` was never assigned by `jal`/`jalr`** -- 2 assignments across 137,748
  instructions became 9,814. Every call in the program was affected, and the
  bug invalidated every falsification recorded before it.
- **Static constructors never ran.** A PRX gets its crt0 from the loader, so
  the constructor table sat unwalked. Now discovered and executed by the
  toolkit (`src/ctors.c`, ten tests). One missing pass explained a
  ten-billion-iteration spin, virgin hash tables, null vtables, and a subsystem
  tier that appeared never to initialise.
- Bad memory accesses 28 -> 0. Every label dispatchable (13,682 entries).
- The GE rasterizes: triangles, strips, sprites, six tests.
- `vidt`/`vcst`/`viim`/`vfim` and a decoder fix for the immediate forms.
- Coverage reporting corrected: 92% of `.text`, not 14% of the image.

## What to distrust

Every theory falsified before the `$ra` fix was tested against a program whose
calls did not return correctly. One has already been overturned by re-running
it. The rest are marked in this document and should be re-run before being
relied on.

## The "no callers" discriminator does not work

Recorded here because it was asserted in this document and in `src/ctors.c`
before being tested. Enumerating every null-terminated run of code pointers
whose entries are never called by name in the generated C:

```
candidate constructor tables (all entries uncalled): 1327
```

Useless. **Vtable entries have no callers either** -- they are reached through
a vptr, never by name -- so the property does not separate the two at all. The
claim was plausible, written down twice, and wrong; one scan settled it.

What actually separated the real table was **locality**: all eight of its
targets lie in `0x3B2AF8..0x3B2FCC`, a region above `.text` holding little
else, because the compiler emits constructors together. Exactly one
null-terminated array in the image points there. That is the property worth
implementing, and the code comment now says so.

## Locality narrows but does not decide

Filtering null-terminated pointer runs by span (targets clustered relative to
entry count) takes 1327 candidates down to **28**. A large improvement over the
useless no-callers test, but not a decision: a vtable is also a tight cluster,
because one class''s methods are emitted together too.

So locality is a necessary property, not a sufficient one. Running the 28
blind is not an option -- calling a vtable method with no arguments during
start-up would corrupt state in a way that looks like anything but its cause,
which is the precise failure this document exists to avoid repeating.

The property that actually settled it for the real table was **which region the
targets live in**: `0x3B2AF8..0x3B2FCC`, above `.text`, holding constructors and
almost nothing else. Exactly one array points there. Generalising that means
identifying the region first -- by finding code that no `jal` anywhere targets --
and only then looking for arrays into it. That is the right shape for the next
implementation.

# Session end

`pixels written: 0`. WTF does not render.

Everything above is measured. The open question is one structure: seventeen
asset records whose resource lists at `+184` read zero where an empty circular
list must point at itself.

## The "never a jal target" discriminator fails too

```
constructor-table candidates (never jal'd, null-terminated): 1383
```

The test validates correctly on known inputs -- the real constructor
`0x003B2B40` is not a `jal` target, an ordinary function `0x0004DBB0` is -- and
still accepts 1383 arrays. Same reason as the no-callers test: **vtable methods
are never `jal`''d either**, because virtual dispatch goes through `jalr`.

Two proposed discriminators, both plausible, both measured, both useless. They
were the same property wearing different clothes: "is not called by name" does
not distinguish a constructor from a virtual method, and every entry in either
kind of table qualifies.

What remains, and it is the only thing that has actually worked: **the code
region**. Lumberjack''s constructors occupy `0x3B2AF8..0x3B2FCC`, above `.text`,
holding little else; exactly one null-terminated array points there. The
generalisable form is to find code regions that no static call reaches *and*
that no vtable references, then look for arrays into them -- the second half
being what both failed tests omitted.

This is worth recording precisely because it was cheap to test and expensive to
assume. Two sections of this document previously asserted the first
discriminator as fact.

# Closing state

`pixels written: 0`. WTF does not render.

One open structure: seventeen asset records whose resource lists at `+184` read
zero where an empty circular list must point at itself. The static-constructor
pass fixes exactly this defect for a different family of lists, so the
initialiser for these is either in a second table -- not findable by any test
tried so far -- or in a routine start-up has not yet reached.

## The records are individually guarded — and only one is built

`0x00067E34` does not walk unconditionally:

```
00067E34  lw   $v0, 16($s0)             ; count
00067E38  bnel $v0, $zero, 0x00067E48   ; count == 0 -> skip this record
00067E3C  lw   $s1, 52($s0)             ; delay slot: list head
00067E40  beq  $zero, $zero, 0x00067EF8 ; skipped
00067E4C  beql $s1, $v0, 0x00067EA0     ; s1 == s0+184 -> empty, done
```

So an untouched record is *skipped*, not walked. Measuring the three records the
loop reaches:

```
record #1 s0=0x0030B074  count=0x00000000  head=0x0030B12C  sentinel=0x0030B12C
record #2 s0=0x0030B7CC  count=0x00000000  head=0x00000000  sentinel=0x0030B884
record #3 s0=0x0030BF24  count=0x6D39C076  head=0x8E62E00E  sentinel=0x0030BFDC
```

- **Record 1 is correct**: an empty circular list, head pointing at its own
  sentinel. The file image is zero there, so this was written at run time --
  the constructor pass built it.
- **Record 2 is zero**: count 0, so the guard skips it. Harmless.
- **Record 3 is garbage** in both fields, count non-zero, so it *is* walked --
  and its head is nonsense. This is the spin.

The stride is right (1880 apart, exactly), the base is right (record 1 is
correctly built at it), and the loop bound of 17 is hard-coded in the
instruction. So the array is where the code says it is, and **only one of its
seventeen records has been constructed**.

Record 3''s garbage comes from the file image (`0x0030BF24 = 0xDEFE35B3`), not
from `.bss`. An unconstructed record that happens to sit on file data reads a
non-zero count and defeats the guard that protects record 2.

## What that isolates

This is no longer "a list was not initialised". It is: **the pass that builds
these seventeen records builds one and stops.** The static-constructor table
covers whatever built record 1; something else was meant to build the rest, or
the same routine was meant to loop and does not.

That is a much smaller question than any previously open in this document, and
it has a direct measurement: watch writes to record 2''s head (`0x0030B884`) and
record 1''s (`0x0030B12C`) and compare who writes one but not the other.

## Skipping unbuilt records is not viable — and the chain closes

Shimming the guard to treat unconstructed records as empty (zeroing the count
when the head does not point inside the record) moves the stall but destroys
the run:

```
last loop back-edge  0x00041294 (35.8M hits, was 2.5B at 0x00067E58)
calls dispatched     20,000,000  (budget exhausted)
dispatch misses      19,999,920
bad memory accesses  364,919,061
```

Execution goes wild. So those records are genuinely needed -- their absence is
not cosmetic, and no shim substitutes for building them. A clean negative.

## What builds a record, and why it never happens

`0x00068360` writes record 1''s list head; it is the per-record constructor.
Its four callers:

```
0x0006662C  REACHED
0x00065D34  REACHED
0x00073CB0  never reached  -- and has NO callers anywhere
0x00073DA4  never reached  -- and has NO callers anywhere
```

Two orphans. Same signature as `0x003B2B40`, which turned out to be reachable
only through the static-constructor table -- these are reachable only through a
**vtable**.

Which closes the loop on something recorded early and set aside as
"downstream": the seventeen null virtual dispatches at `0x00066C78`.

```
00066C6C  lw   $t9, 12($s1)     ; vptr
00066C70  lw   $t9, 12($t9)     ; slot 3
00066C74  jalr $ra, $t9         ; 17 times, stride 1880
```

That loop is calling **slot 3 on each of the seventeen records** -- and slot 3
is the record constructor. Every vptr is null, so every construction call goes
to address 0, so sixteen records stay unbuilt, so the later lookup walks
garbage and spins.

The full chain, now end to end:

```
sub-object vptrs never installed
  -> 0x00066C78 dispatches slot 3 to address 0, seventeen times
  -> records 2..17 never constructed
  -> record 3 reads a non-zero count from file data, defeating the guard
  -> 0x00067E58 walks a garbage list head and spins
```

**The remaining question is exactly the one this document opened with**: what
installs the vptrs on these seventeen sub-objects? Static constructors built
record 1 and whatever owns it; they do not reach these. The answer is a
constructor pass over the array -- and its entry point is one of the two
orphans above, reachable only once a vptr exists to dispatch through.

## Measured: the record builder runs four times, not seventeen

```
build #1: record=0x0030B0A0 arg1=0x003AF8F0     <- a real record
build #2: record=0x09FFF8F0 ...                 <- stack temporary
build #3: record=0x09FFF8F0 ...                 <- stack temporary
build #4: record=0x09FFF8F0 ...                 <- stack temporary
```

`0x00068360` is called four times: once on a static record and three times on
stack objects. **There is no loop building seventeen records through it.** So
the array elements are constructed some other way -- which is consistent with
`0x00066C78` dispatching slot 3 on each of seventeen objects, that being the
construction call, and every vptr being null.

`0x00073CB0` and `0x00073DA4` are not the answer either: scanning the whole
image finds **no data reference to them at all**, so they are neither called
nor present in any vtable. Dead code in this module.

## Where the vptrs would come from

The seventeen objects live at `0x00415B68` onward, in `.bss` (past
`filesz 0x3B4880`, inside `memsz 0x4B0180`). A write-watch on the first
object''s vptr slot (`+12`, `0x00415B74`):

```
NOTHING WRITES THE VPTR SLOT
```

And the generated code contains **no `lui 0x0041`**, so that address is never
formed as an absolute constant -- it must arrive via a stored pointer or a
different base. That is the thread to pull next: find how `0x00415B68` is
computed, and the code that computes it is the code that owns the array.

## Session state

`pixels written: 0`. The chain from cause to symptom is measured end to end:

```
vptrs at 0x00415B68+12.. never written
  -> 0x00066C78 dispatches slot 3 to address 0, seventeen times
  -> records 2..17 never constructed
  -> record 3 reads a non-zero count from file data, defeating the guard
  -> 0x00067E58 walks a garbage list head and spins
```

Every link is a measurement, not an inference. The open end is the first line.

## The 240 "undecoded" GE commands are an init sequence, not drawing

Naming them instead of counting them:

```
GE cmd 0x15 arg 0x000000
GE cmd 0x16 arg 0x000000
...
GE cmd 0x2B arg 0x000000
```

A contiguous run of state opcodes, **every argument zero**. That is
`sceGuInit`''s state reset -- the library clearing the whole GE register file at
start-up. Not draw setup, not geometry.

So the three display lists the game submits contain no drawing at all, and
`vtype` staying zero is correct rather than a symptom. **There is no hidden
geometry the rasterizer is failing to draw.**

That closes a question worth closing: rendering is not blocked by anything in
the GE path. The rasterizer is tested and idle because the game has not asked
it to draw, and it will not until start-up completes. No shortcut to pixels
exists that bypasses the hang.

## Every allocation the game makes

```
alloc #1: 8,601,600 bytes
alloc #2: 2,150,400
alloc #3: 1,025,024
alloc #4: 2,560,000
alloc #5: 1,536,000
```

Five, all large pools, ~15.9 MB total -- and the allocator works, so memory is
not the constraint. **None is the 31,960 bytes (17 x 1880) the object array
needs**, and a write-watch on the array base `0x00415B68` shows nothing writes
it either.

So the array sits in memory carved from one of those pools, and its seventeen
elements are never constructed. Which is the same statement the chain already
made, now confirmed from the allocation side rather than the walk side.

## The state this session ends in

Measured end to end, no inferred links:

```
the 17-element array at 0x00415B68 is never written
  -> vptrs at +12 stay null
  -> 0x00066C78 dispatches slot 3 to address 0, seventeen times
  -> records 2..17 never constructed
  -> record 3 reads a non-zero count from file data, defeating the guard
     that correctly skips the all-zero record 2
  -> 0x00067E58 walks a garbage list head and spins
```

`pixels written: 0`. And the GE decode above proves that is not a rendering
problem: the game submits only `sceGuInit`''s state reset and never asks to
draw, so nothing downstream of the hang can produce pixels.

### The next measurement

Find what computes `0x00415B68`. There is no `lui 0x0041` in the generated
code, no allocation of the array''s size, and no write to its base -- so the
pointer is derived from one of the five pools by arithmetic. Watching the
allocator''s return values and following which pool contains `0x00415B68` names
the owner, and the owner is what should be constructing the elements.

## Correction: the allocations are five 32-byte requests

The earlier figures (8.6 MB, 2.15 MB, ...) were `$a1` read at `0x0000E06C`,
which is not the size argument there -- `0x0000E06C` takes the size in `$a0`
and passes it on as `$a1` to `0x0000E0AC`. Reading the right register:

```
alloc #1: size=32  from fn 0x000743F0
alloc #2: size=32  from fn 0x000123A4
alloc #3: size=32  from fn 0x000123A4
alloc #4: size=32  from fn 0x000123A4
alloc #5: size=32  from fn 0x000123A4
```

Five 32-byte allocations, nothing else. **No pools, and no allocation of the
array''s 31,960 bytes.** So the seventeen-object array at `0x00415B68` is not
heap memory at all -- it is a static array in `.bss`, and a static array of C++
objects is constructed by a **static constructor**.

Which narrows the remaining question sharply: the constructor table has exactly
eight entries, all eight run, and none of them writes `0x00415B68`. So one of
those eight is bailing out before it constructs the array.

That is a small, bounded search: watch each of the eight in turn, find the one
whose job includes this array, and read where it stops. Every earlier candidate
-- missing pools, failed allocation, a second constructor table, heap
exhaustion -- is now ruled out by measurement rather than argument.

### On the register mistake

`$a1` looked like a plausible size field and produced plausible-looking numbers
(8.6 MB reads like a texture pool), which is exactly why it went unquestioned
for a round. The check that caught it was reading the callee''s prologue to see
which register it actually consumes. Plausible output is not verification --
this document now contains four separate instances of the same error.

## None of the eight constructors touches the array

Snapshotting `0x00415B68` around each constructor:

```
ctor 0x003B2AF8: array untouched
ctor 0x003B2B40: array untouched
ctor 0x003B2B7C: array untouched
ctor 0x003B2DF0: array untouched
ctor 0x003B2E94: array untouched
ctor 0x003B2F38: array untouched
ctor 0x003B2F88: array untouched
ctor 0x003B2FCC: array untouched
```

So it is **not** a static array with a missing constructor either. That was the
last standing hypothesis, and it is dead.

## What that forces

Nothing allocates 31,960 bytes, nothing writes the array base, and no
constructor claims it. The remaining possibility is that **`0x00415B68` is not
a real object address at all** -- that `$a0` reaching `0x00066C34` is wrong, and
the seventeen-element loop is striding across unallocated `.bss`.

That fits the evidence better than any construction theory:

- The five allocations are 32 bytes each. If the object handed to
  `0x00066C34` is one of them, a loop striding 1880 bytes seventeen times walks
  ~32 KB past a 32-byte allocation -- straight into untouched `.bss`, which is
  exactly what records 2..17 look like.
- Record 1 alone is correctly built, because record 1 *is* the real object.
- Records 3+ read file data or zeros depending on where the stride lands.

## The next measurement, and a caution

Watch `0x00066C34` on entry and print `$a0`, then compare it against the return
value of the 32-byte allocation at `0x000743F0`. If they match, the loop bound
of seventeen is being applied to a single object and the fault is upstream in
whatever supplies the count.

The caution: this document has recorded a confident "the pointer is wrong"
conclusion once before and withdrawn it, because the pointer was fine and the
measurement was reading a stale register at a non-entry address. `0x00066C34`
is a real function entry, so a watch there is observable -- but check
`psp_body_00066C34` exists before trusting a negative.

## The allocation does fail after all

Measuring at `0x0007443C`:

```
alloc result: v0=0x00834000 s4=0x00000000
alloc result: v0=0x0020D000 s4=0x00000000
...
```

`$s4` holds the allocation result (`addu $s4, $v0, $zero` at `0x000743FC`) and
it is **zero**. The guard is `bnel $s4, $zero, 0x0007443C` -- a likely branch
taken only when the allocation succeeded -- so with `$s4 == 0` it is not taken,
the delay slot is nullified, and control falls into the error path at
`0x00074408`.

`0x0007443C` is nevertheless *marked reached*, because execution arrives there
later by fall-through from the error path rather than through the branch.

**So the earlier conclusion "the 32-byte allocation succeeds" was wrong.** It
was drawn from label reachability alone, and reachability does not say *how* a
label was reached. That is the third distinct way this document has been misled
by the watch/mark mechanism, and the sharpest: the address was a real label,
the mark was real, and the inference was still invalid.

The `v0` values (`0x00834000` = 8,601,600, `0x0020D000` = 2,150,400, ...) are
the sizes seen earlier from the wrong register -- they are live in `$v0` at
this point, not allocation results.

## What this reopens

Allocation failure is back, and it now has direct evidence rather than
inference. That is consistent with everything downstream: a null object pointer
handed on, seventeen records that were never real, vptrs that stay zero, and a
walk over `.bss`.

It also re-raises `sceKernelAllocPartitionMemory` never being called: the
game''s dlmalloc has an arena but nothing grows it, so once the arena cannot
satisfy a request the allocator returns null and never asks the kernel for more.

### Next

Read the failure path at `0x00074408` to confirm it is an out-of-memory branch,
then find where the game''s heap is supposed to come from -- the arena base and
limit that `0x0000E0AC` consults. That is a concrete search and the allocator
is already understood as far as its bucket selection.

**Do not** re-derive "the allocation succeeds" from a reachability mark. Read
`$s4`.

## The heap request never reaches the kernel

The game does ask for memory. `0x00000290` -- the only caller of the
`sceKernelAllocPartitionMemory` stub -- runs, three times, with real sizes:

```
sbrk/heap-grow: a0=0x00834060  (8,601,824 bytes)
sbrk/heap-grow: a0=0x0020D060  (2,150,496)
sbrk/heap-grow: a0=0x000FA460  (1,025,120)
```

But instrumenting `hle_AllocPartitionMemory` directly produces **no output at
all**, and `user memory free` stays at 20,840,448 for the whole run. The
request is raised and never arrives.

Its prologue says why it might not:

```
00000290  addiu $sp, $sp, -16
00000298  lui   $s0, 0x3B
0000029C  lw    $v1, 18564($s0)      ; v1 = *(0x003B4884)
000002AC  bne   $v1, $zero, 0x00000328   ; already-initialised path
000002B4  lui   $v1, 0x4B
000002B8  addiu $a0, $v1, 372           ; a0 = 0x004B0174
000002BC  beq   $a0, $zero, 0x000002DC
```

It reads a guard word at `0x003B4884` and, if zero, takes a first-time path
that computes `0x004B0174`. That address is **four bytes past `memsz`**
(`0x4B0180` is the module end -- `0x4B0174` is inside it, just). The heap this
routine is establishing sits at the very top of the module image, which is
exactly where a linker puts `_end` and where `sbrk` starts.

So the shape is: first call sets up a heap based at the end of the module, and
something in that path fails before the kernel call. Note `0x003B4884` is past
`filesz` (`0x3B4880`) by four bytes -- it is the first word of `.bss`, so it
reads zero, which is correct for a first-time guard.

### Next

Single-step `0x00000290` from its entry -- it is short, and only a handful of
branches separate the entry from the import call. One of them diverts. This is
the last link in the chain and the first one where the fault is plausibly in
psprecomp rather than in the game.

## The branch that skips the kernel call

```
000002B8  addiu $a0, $v1, 372          ; a0 = 0x004B0174
000002BC  beq   $a0, $zero, 0x000002DC
000002C4  lw    $a3, 372($v1)          ; a3 = *(0x004B0174)
000002C8  beq   $a3, $zero, 0x00000368  ; <-- ZERO: skip the kernel entirely
000002D0  sll   $s1, $a3, 10           ; s1 = a3 * 1024  -> size in KB
...
000002F0  jal   0x00091F14             ; sceKernelAllocPartitionMemory
```

`*(0x004B0174)` is the **heap size in kilobytes**, shifted left by 10 to become
bytes. It is zero, so `beq` at `0x000002C8` is taken and the allocation call at
`0x000002F0` is never reached. That is the whole reason the kernel never sees a
request.

`0x004B0174` sits twelve bytes before the module end (`memsz 0x4B0180`) and well
past `filesz` (`0x3B4880`), so it is `.bss` -- zero unless something writes it.
Nothing does.

This is the PSP SDK''s heap-size variable, the one `PSP_HEAP_SIZE_KB()` defines.
On hardware the **loader** supplies it: it is part of the module''s declared
parameters, not something the module computes for itself. A recompiled module
loaded by a host that does not know about it gets zero, takes the
"no heap configured" branch, and every allocation afterwards fails.

**This is a psprecomp gap, not a game bug** -- the first one in this chain that
is. And it is the same class as the static constructors: a responsibility that
belongs to the loader, invisible because the module contains the consumer but
never the provider.

### Next

Find how the heap size is declared -- module info, an export, or a known symbol
-- and have the loader write it before entry. Then re-run: the allocation should
reach `hle_AllocPartitionMemory`, which is implemented and has 20 MB free.

## Setting the heap-size word is not sufficient

Writing 16 MB (in KB) to `0x004B0174` before entry changes behaviour -- the
spin count fell from ~3.0B to ~0.9B back-edges -- but the kernel allocator is
still never called, and `user memory free` is unchanged at 20,840,448.

Reachability of the branch targets in `0x00000290`:

```
0x00000290  REACHED
0x000002DC  REACHED
0x00000368  REACHED     <- the "no heap" path
0x0000032C  REACHED
```

**All four**, including both sides of the branch. So the routine runs more than
once and takes different paths on different calls, which means a single
reachability answer cannot say which path the *failing* call took. Same
limitation that has now misled this investigation three times, appearing in a
new form: not "was it reached" but "was it reached *this way*".

The `jal` at `0x000002F0` is not a label, so it cannot be marked directly. The
measurement that would settle it is a watch on the import stub
(`psp_import_00091F14`), which needs `PSP_ENTER` in generated import thunks --
they do not have it. That is a small, worthwhile emitter change: **import stubs
should be traceable like any other function.**

### Standing conclusion

The heap-size word being zero is real and is a genuine loader gap. It is
evidently not the *only* thing gating the allocation, since supplying it did
not produce a kernel call. The next step is to make import calls observable and
then re-read this path -- guessing further without that is how the last several
wrong turns happened.

## Import stubs are now traceable, and the stub *is* reached

Generated import thunks now carry `PSP_ENTER`, so a firmware call can be
watched like any other function -- previously impossible, and the reason a heap
allocation that never reached the kernel could not be diagnosed.

With the heap-size word supplied, watching the stub directly:

```
STUB REACHED: AllocPartitionMemory part=2 size=15976448
STUB REACHED: AllocPartitionMemory part=2 size=15976448
...
```

**The game does reach `sceKernelAllocPartitionMemory`**, asking partition 2 for
15,976,448 bytes -- about 15.2 MB, against 20.8 MB free. So the earlier
conclusion "the kernel allocator is never called" was an artefact of grepping
for a log line that untraceable stubs could never produce.

Yet `user memory free` stays at 20,840,448 for the whole run, and the
instrumentation inside `hle_AllocPartitionMemory` never prints. The stub is
entered; the handler does not appear to run; nothing reports an unimplemented
NID either.

That is a narrow, three-way question -- the NID lookup, the handler, or a stale
build -- and it is the first point in this chain that lies wholly inside
psprecomp, with both ends observable.

### Why this took so long to become visible

Every earlier statement about this call rested on the absence of a log line.
Absence of evidence from a mechanism that *cannot* produce evidence is not
evidence, and that pattern has now cost this investigation four separate wrong
conclusions. Making import stubs traceable removes the blind spot permanently.

## One unimplemented firmware call, and it is on the heap path

With import stubs traceable and a clean rebuild:

```
psprecomp: unimplemented firmware call 0xF9275D98   (x3)
```

**That is the only unimplemented NID in the entire run**, and it is called
exactly three times -- the same count as the three heap-grow requests.

```
ModuleMgrForUser :: NID 0xF9275D98
void psp_import_00091EEC(void) { PSP_ENTER(0x00091EECu); psp_hle_call(0xF9275D98u); }
```

It is a `ModuleMgrForUser` function and does not match the SHA-1 of any of the
fourteen common ModuleMgr names tried, so it is a firmware-specific entry point
whose name still needs identifying. (NID = first four bytes of SHA-1(name), so
identification is exact once the right name is guessed -- no ambiguity.)

The matching call counts are the significant part: the routine at `0x00000290`
that establishes the heap makes three requests, and this unimplemented call
happens three times. An HLE stub returning 0 to a module-manager query on the
heap-setup path is exactly the shape that would make heap establishment fail
silently -- which is what every measurement downstream has been reporting.

### Next

1. Identify `0xF9275D98` by brute-forcing plausible `ModuleMgr` names against
   SHA-1; the test harness already verifies NIDs this way, so the machinery
   exists.
2. Implement it, re-run, and see whether the heap is established.

This is the first actionable item in the chain that is a *missing feature*
rather than a bug -- and it is one function.

## The NID test refused a fabricated name — correctly

Registering `0xF9275D98` under the placeholder name
`"ModuleMgrForUser_F9275D98"` so it could return a module id instead of zero
**failed the HLE test suite immediately**.

That is the self-verifying NID property doing its job: every registered entry is
checked against the SHA-1 of its own declared name, so a made-up name cannot be
registered. The change was reverted rather than the test weakened.

It is worth being explicit about why that matters here. The temptation was to
get past a blocker by claiming a name the function does not have -- and the
resulting table would have run, looked fine, and lied about what it implements.
A "plausible lie that turns into a silent failure later" is the exact phrase
already recorded in this document about a different fabricated value, and the
test exists because of it.

Eighteen candidate `ModuleMgr` names have now been tried against SHA-1 with no
match, and the method is verified -- `sceKernelStopModule` reproduces
`0xD1FF982A` exactly. So `0xF9275D98` is a real firmware entry point whose name
is simply not among the obvious guesses.

### The correct way to unblock this

Either identify the name (a larger dictionary of PSP firmware exports, or a
name list from a header), or add a mechanism for registering a NID whose name
is genuinely unknown -- one that records it as unidentified rather than
inventing a name. The second is honest and small: an explicit
`psp_hle_register_unnamed(nid, lib, fn)` that the NID test skips by design and
that reports "unidentified NID" in listings.

That is the next change, and it is a better outcome than guessing: it makes
"we do not know what this is" representable instead of unrepresentable.

## `psp_hle_register_unnamed` — making "unidentified" representable

Shipped in `src/hle/hle.c`: a NID can now be registered without a name. The
test suite skips unnamed entries **by construction** and reports how many there
are, rather than by an exception carved out for one case.

`0xF9275D98` is registered this way -- observed, exact, unidentified -- and the
unimplemented-call warning is gone. Ten test suites still pass, and the
self-verifying property is intact for every entry that claims a name.

This is the right shape for a recompiler''s firmware table: a game will always
reach entry points nobody has named yet, and the choice between "invent a name"
and "stay blocked" was a false one.

## The open question, stated precisely

Returning a module id from `0xF9275D98` did **not** establish the heap. And a
sharper puzzle remains, entirely inside psprecomp:

- The import stub for `sceKernelAllocPartitionMemory` (`0x00091F14`) is
  **entered** -- `PSP_ENTER` fires, three times, with `size=15,976,448`.
- Registration is correctly wired: `psp_hle_init` -> `psp_sysmem_register` ->
  `psp_hle_register(0x237DBD4F, ..., hle_AllocPartitionMemory)`.
- `hle_AllocPartitionMemory` contains an unconditional `fprintf` on entry.
- **That `fprintf` never appears**, and `user memory free` never changes.

Stub entered, handler registered, handler apparently not run, nothing reporting
an unimplemented NID. Those four facts cannot all be true, so one of the
measurements is lying -- and identifying which is the next step, not another
hypothesis about the game.

The most likely candidate on past form: a stale build. This has already caused
one wrong result this session, and the submodule copy is refreshed by
`git checkout` in a way that has silently reverted edits before. Verify by
rebuilding from clean and checking the binary contains the string, rather than
checking the source does.

## The contradiction, fully verified

Every fact re-checked directly rather than inferred:

```
import stub 0x00091F14 -> psp_hle_call(0x237DBD4F)     verified in generated source
PSP_ENTER at that stub fires                            5 times, logged
0x237DBD4F registered -> hle_AllocPartitionMemory       verified in sysmem.c:256
psp_hle_init -> psp_sysmem_register                     verified in hle.c:144
fprintf on entry to hle_AllocPartitionMemory            verified in source
that string present in the built executable             verified by scanning the .exe
stderr unbuffered (watchdog leaves via _exit)           fixed; no change
handler log lines                                       0
user memory free                                        unchanged, all runs
```

The stub is entered five times and the handler it dispatches to never runs.
Both strings are in the binary, so this is not a stale build in the ordinary
sense -- though a stale *object file* contributing the string while an older
`sysmem.o` is linked would satisfy every check above and remains the leading
candidate.

Ruled out along the way: buffering (stderr made unbuffered, no change), a
wrong stub-to-NID mapping (verified in the generated file), missing
registration (verified in both directions).

### How to settle it

Put a print at the top of `psp_hle_call` itself, before the lookup loop. If it
fires, the fault is in the lookup; if it does not, the stub is not calling what
its source says it calls, and the generated code or its build is wrong. That
single measurement splits the remaining space in half and needs no new theory.

This is a good place to hand off: the question is small, entirely inside
psprecomp, and every surrounding fact is verified rather than assumed.

---

# The heap allocation succeeds

The four-fact contradiction was **my own instrumentation**: the `fprintf` sat
after an early return inside `hle_AllocPartitionMemory`, so the handler was
running all along and the log could never fire. Instrumenting `psp_hle_call`
itself proved entry, and moving the print to the top of the handler produced:

```
AllocPartitionMemory: part=2 name=0x00093058 type=3 size=15976448
                      attr=0x3AC85C (20840448 free)
```

`type=3` is `PSP_SMEM_LowAligned`, and the fifth argument -- the alignment --
is `0x3AC85C`. That is a data address, not a power of two, so the handler took
its `ILLEGAL_ATTR` early return every time.

Falling back to the 256-byte granule instead of rejecting:

```
                      before              after
user memory free      20,840,448          4,864,000   (15.9 MB allocated)
loop back-edge hits   ~2,400,000,000      765
dispatch misses       19                  1
```

**The heap is established and the spin is gone.** Execution now proceeds
normally through `0x0000E6C0`-`0x0000E73C` -- allocator internals -- rather than
looping on an uninitialised structure. Every symptom this document chased for
its entire length traced back to this one rejected allocation.

## What is not yet right

The alignment argument is still wrong, and the shim hides that rather than
fixing it. `0x3AC85C` arriving where a power of two belongs means either
`psp_arg(4)` reads the wrong stack slot, or the recompiled caller never wrote
the fifth argument. o32 places argument five at `sp+16`, which is what
`psp_arg(4)` reads -- so the caller is the more likely fault, and that is a
codegen question of exactly the kind `$ra` already turned out to be.

Execution now stops earlier (`GE: 0 lists`) because it takes a different, and
presumably correct, path -- 265 function entries in the watchdog window against
470 before. That is not a regression: the previous run reached the GE only to
spin forever afterwards.

## Next

1. Find why argument five is `0x3AC85C`. Disassemble the caller of
   `sceKernelAllocPartitionMemory` at `0x00000290` and check whether it writes
   `sp+16` before the call. If it does, `psp_arg` is wrong; if not, the emitter
   is dropping a stack argument store.
2. Then remove the granule fallback -- it is a bring-up shim, not a fix.

---

# Fixed: firmware calls pass arguments 5-8 in $t0-$t3

`psp_arg(n)` read `sp + n*4` for `n >= 4`, which is plain MIPS o32. PSP
firmware stubs do not follow it: they load `$t0`-`$t3` and branch, and the
delay slot of the call says so plainly --

```
000002EC  addu  $a3, $s1, $zero
000002F0  jal   0x00091F14        ; sceKernelAllocPartitionMemory
000002F4  addiu $t0, $zero, 4096  ; argument 5: the alignment
```

Reading `sp+16` returned whatever was on the stack -- `0x3AC85C` for this call,
where a power-of-two alignment belonged. The allocator correctly rejected a
15.9 MB request as `ILLEGAL_ATTR`, and **every failure in this document
descends from that one bad read**.

With `psp_arg` corrected and the real alignment check restored (no shim):

```
AllocPartitionMemory: part=2 type=3 size=15976448 attr=0x1000 (20840448 free)

                      before              after
user memory free      20,840,448          4,864,000    (15.9 MB allocated)
loop back-edge hits   ~2,400,000,000      765
dispatch misses       19                  1
```

The heap is established, the ten-billion-iteration spin is gone, and it is a
fix rather than a workaround: the game passes a valid alignment and psprecomp
now reads it.

This affects **every firmware function taking more than four arguments**, not
just this one. It is the second bug of its class this session, after `$ra` was
never assigned by `jal` -- both were calling-convention details that produced
plausible-looking values instead of obvious failures.

## Status

`pixels written: 0`. The game now stops at `0x000743F0` after 265 function
entries, inside allocator internals, on what is finally a correct path -- as
against 470 entries that reached the GE only to spin forever afterwards.
`GE: 0 lists` is therefore not a regression; the earlier lists were produced by
a run that was already doomed.

## Next

Follow execution from `0x000743F0`. One dispatch miss remains, and 23 bad
memory accesses -- both small enough to read individually, which is how every
real finding in this document was made.

## Remaining after the calling-convention fix

One dispatch miss and 23 bad accesses -- both small enough to read individually.

### The miss is a discovery gap

```
miss 0: target=0x0000E588 from fn 0x0000E2A8 ra=0x0000E7D4
```

`0x0000E588` is a real instruction (`jal 0x0000F2B8`) but has **no label and no
body** in the generated code, so the every-label-is-dispatchable guarantee does
not cover it -- discovery never reached this block, and a computed jump lands
there at run time.

That is a genuine gap rather than an emitter bug: the block is inside the
allocator, which discovery only partly walked. Worth checking whether the
allocator''s jump tables are being resolved, since dlmalloc''s bin selection is
exactly the shape that compiles to one.

### The bad accesses are code being read as data

```
bad read32 at 0x8E420004 (last fn 0x000123C4)
bad read32 at 0x8E420004 (last fn 0x00012328)
bad write32 at 0x28420054 (last fn 0x000123A4)
```

`0x8E420004` is not an address -- it is the encoding of `lw $v0, 4($s2)`, and
`0x28420054` is `slti $v0, $v0, 0x54`. **Instruction words are being used as
pointers.** Something loads from the code segment and dereferences the result,
which is the signature of a function-pointer table read at the wrong offset, or
a structure whose pointer field overlaps code.

All three cluster in `0x00012328`-`0x000123C4`, one small region, and they
appear only now that execution gets this far. That is the next thing to read.

### The bad pointers go to strcpy and strlen

```
000123A4  lbu $v0, 0($a1) / sb $v0, 0($v1) / bne  -> strcpy
000123C4  lbu $v1, 0($a0) / bne                   -> strlen
```

So a **string pointer** is wrong, not a structure pointer: something passes a
`char *` of `0x8E420004` -- an instruction encoding -- to `strcpy`/`strlen`.
The pointer was loaded from a location holding code, so either a string-pointer
field is being read at the wrong offset, or the object it is read from is
itself misplaced.

This surfaced only after the allocation fix let execution reach here, so it is
newly-exposed rather than newly-broken. Twenty-three occurrences, one small
region, and both callees now identified -- the caller of `strcpy` is where to
look, and the argument is `$a1`.

# Session summary

`pixels written: 0`. WTF does not render.

## The two fixes that mattered

Both were **calling-convention details producing plausible values rather than
visible failures**, which is why each survived so long:

1. **`$ra` was never assigned by `jal`/`jalr`.** Two assignments existed across
   137,748 instructions; there are now 9,814. Every non-leaf function in the
   program was returning through a stale register.
2. **`psp_arg` read arguments 5-8 from the stack.** PSP firmware passes them in
   `$t0`-`$t3`. The wrong read turned a valid 4096-byte alignment into
   `0x3AC85C`, so a correct 15.9 MB allocation was rejected as `ILLEGAL_ATTR`
   -- and every symptom chased in this document descended from that.

After the second fix: heap established (15.9 MB), the ten-billion-iteration
spin gone, dispatch misses 19 -> 1, and the real alignment check restored
rather than shimmed.

## Also shipped

Static-constructor discovery and execution (`src/ctors.c`, ten tests) --
a PRX gets its crt0 from the loader, so nothing was walking the table.
Traceable import stubs. `psp_hle_register_unnamed`, making "observed but
unidentified" representable instead of forcing an invented name. A tested
rasterizer. Total dispatch reachability (13,682 entries). Bad accesses 28 -> 0.
Four VFPU ops and a decoder fix. Coverage reporting corrected from 14% to 92%.

## What to distrust

Every theory falsified *before* the `$ra` fix was tested against a program
whose calls did not return correctly; one has already been overturned by
re-running it. And four separate conclusions in this document came from
instrumentation that could not observe what it claimed to measure -- a watch on
a non-entry address, a mark that says "reached" without saying how, a grep for
a log line untraceable stubs could never emit, and a `printf` placed after an
early return. Verify the instrument before trusting the negative.

### Traced: both arguments are garbage at the call site

```
strcpy BAD src=0x8E420004 dst=0xFFFFFFFF caller fn 0x00012328 ra=0x000009AC
```

The call site:

```
000009A0  addu $a1, $s2, $zero
000009A4  jal  0x00012328        ; strcpy-family
000009A8  addu $a0, $s1, $zero   ; delay slot
```

`$a0` comes from `$s1` and `$a1` from `$s2`, and **both are garbage** --
`0xFFFFFFFF` and an instruction word. So this is not one mis-loaded string
pointer: two callee-saved registers hold nonsense at the same point, in a
function at `0x000009xx` (very low, so early start-up).

Two registers wrong together points at the frame rather than at either value:
either this function''s `$s1`/`$s2` were never loaded, or they were clobbered by
a callee that failed to preserve them. Recompiled functions share one global
register file, so a callee that does not restore `$s0`-`$s7` corrupts its
caller silently -- the same shape as `$ra`, and a hypothesis raised early in
this document that was never properly tested.

That is the thread to pull next, and it is testable directly: watch this
function''s entry, record `$s1`/`$s2`, then watch again at `0x000009A0` and see
whether they changed across the intervening calls.

### The object pointer is a code address

```
0x00000914 entry: a0=0x0000E124 a1=0x008400A4 a2=0x28420074 caller 0x0000F2B8
```

`0x00000914` has a proper prologue (saves `$ra`, `$s0`-`$s3`) and immediately
zeroes `a0+0x00` through `a0+0x28`, so `$a0` is an object it is initialising.
It receives **`0x0000E124` -- an address inside `.text`** -- and `$a2` is
`0x28420074`, an instruction encoding.

So the garbage `$s1`/`$s2` seen at `0x000009A0` are downstream: this function is
handed a code address as its object and writes zeros over it, then reads its
own fields back as instruction words.

`$a0 = 0x0000E124` lands in the allocator''s code, which is suggestive: a
returned pointer that is actually a code address is what a mis-read return
value looks like. Given `psp_arg` was just found reading the wrong registers
for firmware calls, the same class of mistake in the *recompiled* call path --
a return value taken from the wrong place, or a caller-saved register not
treated as clobbered -- would produce exactly this.

Note `psp_trace_last()` reports the last function *entered*, which is not
necessarily the caller. That distinction has already caused one wrong
conclusion in this document; confirm the caller by reading the call site rather
than trusting the trace.

### Where to resume

`0x00000914` is entered with a code address as its object pointer. Find its
real caller (read the call site, do not trust the trace), and check where that
pointer came from. The two fixes this session were both calling-convention
faults; this has the same shape and is the natural third.

### The caller, read from the call site

`0x00000914` has four callers; the one on this path is `0x000743F0`, whose call
site is unambiguous:

```
00074440  addu $a0, $s1, $zero
00074444  addu $a1, $s4, $zero
00074448  jal  0x00000914
0007444C  addu $a2, $s4, $v0      ; delay slot
```

So the bad object pointer is **`$s1`**, and `$s4` is the allocation result
captured at `0x000743FC` (`addu $s4, $v0, $zero`). `$s1` is advanced by 48 each
time round the enclosing five-iteration loop:

```
00074450  addiu $s5, $s5, 1
00074454  slti  $v0, $s5, 5
00074464  addiu $s1, $s1, 48       ; delay slot
```

`$s1 = 0x0000E124` is inside `.text`, so whatever seeds it before the loop is
wrong -- five 48-byte objects are being initialised over code. Reading where
`$s1` is first set, before `0x000743F0`, is the next step, and the call site
above is read rather than inferred from the trace.

This is a tractable end point: one register, one loop, one function, with the
allocation now succeeding underneath it.

### $s1 is set correctly and then clobbered across a call

```
0007436C  lui   $s1, 0x40           ; s1 = 0x00400000
000743A0  addiu $s1, $s1, -12192    ; s1 = 0x003FD060   <- a valid data address
...
000743F4  jal   0x0000E06C          ; the allocator
...
00074440  addu  $a0, $s1, $zero     ; s1 measured here as 0x0000E124
```

`$s1` is `0x003FD060` when set and `0x0000E124` when used. **The allocator call
between them does not preserve it.**

`$s1` is callee-saved, and `0x0000E06C` saves it properly in its own prologue:

```
0000E06C  addiu $sp, $sp, -16
0000E074  sw    $s1, 4($sp)
0000E078  addu  $s1, $a1, $zero
```

So the original preserves `$s1`; the recompiled version evidently does not
restore it. That points at emission rather than at the game -- most likely an
exit path that leaves without running the epilogue: a tail call, a fall-through
into another body, or a `return` emitted where the original would have reached
its restore sequence.

`0x0000E588` -- the one remaining dispatch miss -- sits inside this same
allocator and has no label, so at least one control-flow edge here is already
known to be unmodelled. The two are plausibly the same defect.

**This is the third calling-convention fault of the session**, after `$ra`
never being assigned and `psp_arg` reading the wrong registers. All three
produce plausible values rather than visible failures, which is why each
survived so long, and callee-saved restoration is the natural next one to
verify systematically rather than case by case.

### Concretely

Compare `psp_body_0000E06C`''s emitted exits against the original''s epilogue at
every `return`. Any path that returns without the `lw $s1, 4($sp)` sequence is
the bug.

### The allocator restores; its callee does not

`psp_body_0000E06C` is correct -- it emits `r_s1 = psp_read32(r_sp + 4);`
before its `return`. So the clobber is deeper, in `0x0000E0AC`, which saves
`$s1` at `sp+4` in its own prologue and has:

```
returns:                8
restores of $s1:        2
tail calls / fall-throughs: 14
```

Eight exits and two restores. Not proof on its own -- this function is split
across bodies, so a `return` after `psp_func_X()` is legitimate when the callee
carries the epilogue -- but six exits with no visible restore, in exactly the
function measured to clobber `$s1`, is a strong and checkable lead.

`0x0000E588` also lies in this function''s range and is the one remaining
dispatch miss, with no label emitted. An unmodelled edge and a missing restore
in the same split function are plausibly one defect: a control-flow path the
walk never took, whose exit therefore never got its epilogue.

### To settle it

For each of the eight returns, check whether the path reaching it passes
through the original''s `lw $s1, 4($sp)`. The ones that do not are the bug. If
they correspond to edges discovery missed -- `0x0000E588` among them -- then
the fix is in discovery, not in emission, and the every-label-dispatchable
guarantee needs extending to cover computed targets that were never labelled.

### The "no restore" count was a measurement artefact

Classifying each `return` by scanning six lines above it gave six exits with no
`$s1` restore. Reading one of them shows why that is worthless:

```
    /* 0000E1FC  lw    $s0, 0($sp) */
    r_s0 = psp_read32(r_sp + 0);
    /* 0000E204  jr    $ra */
    r_sp = r_sp + 32;
    return;
```

The emission is a **faithful translation** of the original epilogue -- and the
original restores `$s0` there, not `$s1`, because `$s1` was restored earlier on
that path, outside a six-line window. A fixed-size context window cannot
classify epilogues that span more instructions than the window.

So "8 returns, 2 restores" says nothing, and the conclusion drawn from it is
withdrawn. This is the fifth time in this document a crude measurement produced
a confident wrong reading; the pattern is consistent enough to state as a rule:
**a measurement whose window or resolution is chosen for convenience will
manufacture whatever conclusion that choice implies.**

### What still stands, measured directly

`$s1` is `0x003FD060` at `0x000743A0` and `0x0000E124` at `0x00074440`, with a
call to `0x0000E06C` between them. That comparison is two register reads at two
addresses and does not depend on any window. **The clobber is real.**

What is not established is where inside the call it happens.

### To settle it properly

Watch `$s1` at entry and exit of `0x0000E06C` itself, then at entry and exit of
each function it calls, narrowing until the clobbering callee is identified by
measurement rather than by reading emitted code. That is the same
bisect-by-measurement that found the constructor-tier boundary in one pass
after inference had failed for several rounds.

---

# The stack pointer leaks

Measuring `$sp` and `$s1` at each entry to the allocator, from inside the
five-iteration loop that calls it:

```
call 1:  sp=0x09FFFB60  s1=0x003FD060   (correct)
call 2:  sp=0x09FFFB30  s1=0x0000E154   (already wrong)
```

**`$sp` is 48 bytes lower on the second call and never came back.** Some callee
in between decremented the stack pointer and did not restore it.

That explains the `$s1` clobber completely, and without any appeal to a missing
epilogue: every callee-saved register is restored by reading `sp + <offset>`.
If `$sp` is not what it was at the matching prologue, the restore reads a
*different stack slot* -- so a function can execute its epilogue perfectly and
still load garbage. `$s1` was not failing to be restored; it was being restored
from the wrong address.

This also subsumes the earlier withdrawn reading: the emitted epilogues are
faithful, exactly as they appeared to be. The fault is one level down, in `$sp`
itself.

48 bytes is a specific frame size, and the loop body calls a small number of
functions -- `0x00000914` opens with `addiu $sp, $sp, -32`, so it is not the
only candidate. The next measurement is `$sp` at entry and exit of each
function the loop calls; the one whose exit `$sp` differs from its entry `$sp`
is the bug.

## Why this is the right kind of finding

Every symptom this document has chased -- garbage pointers, unconstructed
objects, null vtables, self-looping lists -- is what a shifting stack frame
produces: values restored from adjacent slots, plausible and wrong. It is the
same signature as the two calling-convention faults already fixed, and it is
measured with two register reads rather than inferred from emitted code.

### Narrowing the leak: one iteration still takes the failure path

`0x0000E06C` is not the leaker -- its emitted epilogue is exact:

```
    r_s1 = psp_read32(r_sp + 4);
    r_s0 = psp_read32(r_sp + 0);
    r_sp = r_sp + 16;
    return;
```

And measuring the loop''s other callee:

```
at 0x00000914: sp=0x09FFFB30 s1=0x0000E124     (one call only)
at 0x0000E06C: sp=0x09FFFB60 / 0x09FFFB30      (two calls)
```

**Two allocator calls, one call to `0x00000914`.** The loop body calls the
allocator, then branches on the result:

```
00074400  bnel $s4, $zero, 0x0007443C   ; success -> use it
00074408  addiu $a0, $zero, 10          ; failure path
...
00074448  jal  0x00000914               ; only on the success path
```

So one of the two iterations still takes the **allocation-failure path**, and
`$sp` drops 48 bytes across it. The leak is in that error path, not in the
success path -- which is why it survived the allocation fix: that fix made
allocations succeed, but evidently not all of them.

Two things follow, and both are checkable:

1. Some allocation still fails. The 32-byte request succeeds now; something
   else in this loop does not. Logging every allocator return, not just its
   arguments, would say which.
2. The failure path leaks `$sp`. It calls `0x000472D0` and others; measuring
   `$sp` at entry and exit of each names the function whose frame is unbalanced.

The second is the more valuable: a function that leaves `$sp` unbalanced
corrupts every callee-saved register its caller restores afterwards, and would
do so for any game, not just this one.

---

# A stack-balance checker, and three real leaks

The emitter now records `$sp` at every function entry and checks it at every
`return` (3,866 sites). A recompiled function must leave `$sp` as it found it;
one that does not corrupts every callee-saved register its caller restores
afterwards, because the restores read `sp + offset` and a shifted `$sp` reads a
neighbouring slot -- loading a plausible wrong value instead of failing.

First run:

```
sp UNBALANCED in 0x0004D7BC: +16
sp UNBALANCED in 0x0004D6F0: +16
sp UNBALANCED in 0x0004C290: +16
sp UNBALANCED in 0x0000F680: +16
sp UNBALANCED in 0x0006D7D8: +16
sp UNBALANCED in 0x0000E2A8: +48
sp UNBALANCED in 0x0000E460: +48
sp UNBALANCED in 0x0000D410: -16   <-- leak
sp UNBALANCED in 0x0000E0AC: -48   <-- leak
sp UNBALANCED in 0x0000E06C: -48   <-- leak
```

**`0x0000E06C` at -48 is exactly the drop measured earlier** between the two
allocator calls, arrived at independently.

## Reading the two signs

**Positive deltas are mostly noise.** `0x0004D7BC` is not a function prologue --
it is mid-loop code that discovery split into its own body. A body holding an
epilogue but not its matching prologue legitimately raises `$sp`, and the
invariant does not apply to it. That is a limitation of the check, not a bug in
the game, and it should be suppressed by only checking bodies that begin with
`addiu $sp, $sp, -N`.

**Negative deltas are the dangerous ones**: stack consumed and never returned.
Three of them, and they nest -- `0x0000E06C` calls `0x0000E0AC`, and both show
-48, so the outer figure is inherited rather than independent. `0x0000D410` at
-16 is the smallest and most likely the root.

## Why this is worth having regardless

This check finds a whole class of fault in one run that took most of a session
to reach by tracing consequences. Every symptom chased in this document --
garbage pointers, unconstructed objects, null vtables, self-looping lists -- is
what a shifted stack frame produces. The checker states the invariant directly
instead of waiting for it to be violated somewhere observable.

## Next

1. Suppress the false positives: only check bodies whose first instruction is a
   stack decrement.
2. Start at `0x0000D410` (-16), the innermost leak, and find the path through
   it that skips its epilogue.

## The checker needs filtering, and the "leaks" are mostly legitimate

Reporting the exact return site rather than the function shows what the flagged
exits actually are:

```
0x0000D60C: -16   ->  j 0x0000D520      -- a tail jump, not a return
0x0000E204: -48   ->  jr $ra in an epilogue whose callee was still mid-frame
0x0004D824: +16   ->  a split continuation holding an epilogue, no prologue
```

`0x0000D60C` is `j 0x0000D520`. The emitter renders an out-of-function jump as
`psp_func_X(); return;`, and at that synthesized `return` the frame is
*correctly* still open -- the target runs the epilogue. Checking there reports
every tail jump as a leak.

So the invariant "`$sp` at entry equals `$sp` at return" holds only for a
genuine `jr $ra` in a body that also contains its own prologue. Neither
condition is true for split continuations or tail jumps, and this module has
many of both.

An attempt to restrict the check to `in.is_return` did not change the counts,
so the restriction is not yet effective -- the emitted check count stayed at
3,866. The filter needs to be written deliberately rather than by pattern
substitution, which is how the last several edits in this session went wrong.

## What the checker is worth anyway

It reduced "somewhere a stack frame shifts" to eleven specific return sites in
one run, and reading three of them settled what they are. That is the right
kind of tool even unfiltered: it produces candidates with addresses instead of
symptoms without them.

**But the -48 measured between the two allocator calls is still unexplained.**
That was two register reads at two addresses -- no window, no synthesized
return, no split body -- and it remains the one solid piece of evidence that a
frame really does shift. The checker has not yet found the site responsible.

### Next

1. Write the filter properly: emit `PSP_SP_CHECK` only where `in.is_return` is
   true **and** the enclosing body begins with a stack decrement.
2. Re-run. If the -48 site appears, it is the bug; if nothing appears, the
   shift happens somewhere the check cannot see -- most likely across a tail
   jump between split bodies, where no single body owns both halves of a frame.

## With the filter working: one real leak

The tool had to be **rebuilt** after the source was copied into the submodule --
the previous run used a stale `allegrexrecomp` and reported the unfiltered
result. That is the fifth stale-build confusion in this session, and the
symptom is always the same: an edit that provably exists in the source has no
effect on the output.

Rebuilt properly, checks drop from 3,866 to 3,038 and violations from 11 to 9:

```
sp UNBALANCED in 0x0004D824: +16     |
sp UNBALANCED in 0x0004D758: +16     |  split continuations: an epilogue
sp UNBALANCED in 0x0004C394: +16     |  without its prologue. Still noise,
sp UNBALANCED in 0x0000F76C: +16     |  filterable by requiring the body to
sp UNBALANCED in 0x0006DAD4: +16     |  begin with a stack decrement.
sp UNBALANCED in 0x0000E36C: +48     |
sp UNBALANCED in 0x0000E204: -48   <-- the leak
```

The tail-jump false positive at `0x0000D60C` is gone, and **exactly one
negative delta survives**: `0x0000E204`, the `jr $ra` of `0x0000E0AC`, returning
48 bytes short.

That is the same -48 measured independently between the two allocator calls, at
a genuine return in a body that owns its prologue. Two different methods, same
number, same function.

### This is the site

`0x0000E0AC` decrements `$sp` by 32 at entry and its epilogue adds 32 back, so
a balanced return is expected. Returning -48 short means either an exit path
skips part of the restore, or a callee left the stack 48 low and the epilogue''s
`+32` cannot compensate for what it never allocated.

Reading `psp_body_0000E0AC` around the return at `0x0000E204`, with the frame
arithmetic in mind rather than a fixed context window, is the next step -- and
this time the address is exact rather than a function-wide count.

### The epilogue at 0x0000E204 is faithful — so the leak is inherited

```c
    r_ra = psp_read32(r_sp + 20);
    r_s4 = psp_read32(r_sp + 16);
    r_s3 = psp_read32(r_sp + 12);
    r_s2 = psp_read32(r_sp + 8);
    r_s1 = psp_read32(r_sp + 4);
    r_s0 = psp_read32(r_sp + 0);
    r_sp = r_sp + 32;
    PSP_SP_CHECK(0x0000E204u);
    return;
```

Exact: every saved register restored, `$sp` returned by the same 32 the
prologue took. `_sp0` is captured at the top of the body, before the
`addiu $sp, -32`, so a balanced return should show zero delta.

It shows -48. **The deficit is therefore inherited from a callee** -- something
`0x0000E0AC` calls consumed 48 bytes and never returned them, and `+32` cannot
repay what this frame never allocated.

That callee does not appear in the violation list, which means its own leak
happens somewhere the check does not look: a tail jump, or a split body whose
prologue and epilogue live in different `psp_body_*` functions. The current
filter excludes exactly those cases -- correctly, to remove false positives,
but that is also where the real fault is hiding.

### The shape of the remaining work

The check as built cannot see a leak that occurs across a body boundary,
because no single body owns both halves of the frame. Making it see one means
tracking `$sp` per *original function* rather than per emitted body -- the
entry-address plumbing for that already exists, since every body takes an
`_entry` parameter and interior labels are dispatchable.

That is a real piece of work rather than a one-line filter, and it is the
correct next step: the -48 is measured twice by independent means, the epilogue
that reports it is provably faithful, and the only place left for the fault to
be is the boundary the instrument currently ignores.

---

# The leak, isolated: two functions, and the arithmetic proves it

Checking `$sp` **across every call** rather than within each body -- 10,112
sites -- names the callees directly, and is blind to neither split bodies nor
tail jumps:

```
sp NOT RESTORED by callee 0x0000E4DC: -32   <- root
sp NOT RESTORED by callee 0x0000D410: -16   <- root
sp NOT RESTORED by callee 0x0000E0AC: -48   = -32 + -16, inherited
sp NOT RESTORED by callee 0x0000E06C: -48   inherited
```

**The deltas decompose exactly.** `0x0000E0AC` calls both roots and inherits
their sum; `0x0000E06C` calls `0x0000E0AC` and inherits it in turn. That
arithmetic is the confirmation -- an inherited figure that happens to equal the
sum of its two children is not coincidence.

And -48 is the same number measured independently at the very start of this
trail, between two allocator calls, by reading `$sp` at two addresses.
**Three methods, one answer.**

## The two roots

- `0x0000E4DC` opens with `addiu $sp, $sp, -32` and never returns it.
- `0x0000D410` opens with `addiu $sp, $sp, -16` and never returns it.

Both have correct epilogues in their emitted code -- verified by reading them.
So on the path actually taken, neither reaches its epilogue: each exits through
a tail jump into a continuation body, and that continuation does not restore
the frame its caller opened.

That is the defect, and it is structural rather than incidental: **when
discovery splits a function, the prologue and epilogue can land in different
bodies, and a path that leaves through the split never runs the half it
needs.** It explains every stack-derived symptom in this document -- garbage
pointers, unconstructed objects, null vtables, self-looping lists -- because
every callee-saved restore reads `sp + offset` and a shifted `$sp` reads a
neighbouring slot.

## Next

Fix the split, not the symptom. Either keep prologue and epilogue in one body,
or make a tail jump between bodies carry the frame correctly. The call-site
checker (`PSP_SP_CALL`) stays as a regression guard: it found this in one run
after the per-body check could not see it, and it will catch any recurrence
immediately.

### How the frame is lost

`psp_body_0000E4DC` ends this way:

```c
    /* 0000EA14  lw   $v0, 4($s0) */
    r_v0 = psp_read32(r_s0 + 4);
    /* falls through into the next function */
    psp_func_0000EA18();
}
```

The body opens with `addiu $sp, $sp, -32` and ends by **falling through** into
`0x0000EA18` -- 1,340 bytes further on -- and then returning. Its own epilogue
is never reached on this path; whatever restores the frame lives beyond the
fall-through, and the chain evidently does not get there.

This is the fall-through emission working exactly as designed, and being wrong
here. That mechanism was added earlier in this session to fix 1,548 silently
truncated functions, and it does fix them -- but it converts a fall-through
into a *nested C call*, so the callee''s eventual `return` unwinds to the
caller''s frame rather than continuing in the original one. When the epilogue
lives past that boundary, the frame it was meant to close is already gone.

So the defect is a consequence of a fix, which is worth stating plainly: the
fall-through change was necessary and correct in what it addressed, and it
introduced this. Both can be true.

### The fix has a shape

A fall-through is not a call and should not be emitted as one. The target
should be entered as a *continuation* -- a `goto` into the same C function
where the bodies allow it, or a tail-call that does not add a frame. The
entry-address plumbing needed already exists: every body takes `_entry`, and
interior labels are dispatchable, so a merged body indexed by entry address is
reachable without new machinery.

That is the next change, and the call-site checker will confirm it: the four
violations should go to zero, and the -48 that has been measured three ways
should disappear with them.

### Correction: the nested-call explanation is wrong

The previous section claimed the fall-through emission loses the frame because
it becomes a nested C call whose `return` unwinds to the caller. **That is
wrong.** `r_sp` is a *global* in the generated code, not a C local -- a callee''s
change to it persists after the C call returns. A tail-call chain therefore
restores `$sp` correctly, however deeply nested, provided some link in the
chain actually performs the restore.

What is true, and checked:

```
psp_body_0000EA18  exists, 44 lines, contains no `r_sp = r_sp` at all
```

So the fall-through target does not restore `$sp`, and neither does
`0x0000E4DC` on the path taken. The 32 bytes are simply never returned by any
link that executes.

That leaves two possibilities, and they are distinguishable:

1. The chain diverges from the original somewhere -- a branch taken differently,
   so the epilogue that would restore is never reached. A recompiler fault.
2. The original genuinely does not restore on this path, because it is an error
   path that never returns normally on hardware either -- for example one that
   ends in a longjmp or an abort. Then the leak is a symptom of taking an error
   path that should not have been taken, not of stack handling at all.

The second deserves weight: this is the *allocation-failure* path, reached
because something in the loop still fails to allocate. An error path that
abandons its frame is ordinary in C runtime code.

So the honest state is: the stack imbalance is real and measured three ways,
its immediate mechanism is now known (no link in the executed chain restores),
but whether it is a *cause* or another *symptom* of the remaining allocation
failure is not yet established. Establishing that comes first -- fixing stack
handling that is not broken would be the fourth wrong turn of its kind here.

---

# A discovery gap: backward `j` into another function's block

`0x0000E588` was the one remaining dispatch miss, and the emitted code showed
why:

```c
    /* 0000E604  j          0x0000E588 */
    psp_dispatch(0x0000E588u);  /* not discovered */
```

A **direct** jump, target known at compile time, and discovery had never
claimed it. The cause is a gap between two paths that should have matched:

- A conditional **branch** whose target is already claimed by an earlier walk
  promotes that target to an entry (`c->cross`), because the emitter cannot
  express a branch into another C function''s middle.
- A backward **jump** in the same situation did not. It was pushed as a local
  block, the walk found it already `SEEN_CODE`, and it was silently dropped.

The branch path had carried the comment explaining exactly this problem since
it was written. The jump path, twenty lines below, needed the same three lines
and never got them.

Fixed by mirroring the branch logic. Effect on Lumberjack:

```
                        before        after
function entries run    265           4,309,119,950
calls dispatched        16            108
GE lists                0             3
"not discovered" sites  25            24
```

Execution now runs *through* the allocator instead of stalling inside it, and
reaches the asset-record lookup at `0x00067E58` -- the stall this document
described before the allocator work began. That is forward progress: the
allocator is no longer the blocker, and the GE is being fed again.

## What this says about the earlier allocator hunt

Much of the preceding investigation chased consequences of this one gap. The
allocator appeared to fail, to leak stack, to return null -- all of it
downstream of a jump that dispatched to an address that was never registered.
The `psp_arg` fix was independently real and necessary; the stack-imbalance
readings were measurements of a program already off its intended path.

Worth stating because the stack-balance checker is still reporting 247
violations, and they should be re-read now rather than trusted from before.

## Retracted: the heap-size word was never needed

An earlier section concluded the loader must supply the SDK''s heap-size
variable at `0x004B0174`, because the game "skipped the kernel allocation
entirely" without it. Writing it was recorded as a genuine loader gap.

With the backward-jump discovery gap closed, that is measurably false:

```
                        16 MB written    8 MB written    nothing written
AllocPartition size     15,976,448       15,976,448      15,976,448
block address           0x08C00000       0x08C00000      0x08C00000
bad accesses            97               97              97
```

**Identical in all three cases.** The game reaches the allocation on its own,
requests the same size, and the word it supposedly needed changes nothing.

The original observation was real -- the allocation genuinely did not happen --
but the cause was the discovery gap, not a missing loader responsibility. The
host hack has been removed. One less fiction between the recompiled program and
what the hardware would do.

That is the second time in this document a host-side workaround turned out to
be compensating for a toolkit bug rather than a real gap. The pattern is worth
naming: **when a workaround is needed to get past a symptom, it is evidence
that the cause is not yet understood, and it should be re-tested after every
fix rather than left in place.**

## Current state

```
partition allocations   0x09FE0000 (128 KB)  and  0x08C00000 (15,976,448)
heap                    established, in range
GE                      3 lists, 255 commands (sceGuInit reset)
dispatch misses         18 -- all null vtable dispatches at 0x00066C6C
bad accesses            97 -- addresses past the top of RAM (0x0A000000)
stall                   0x00067E58, the asset-record lookup
```

The bad addresses are the next thread: `0x0A833D68`, `0x13FFFAA8`,
`0x31BDFD10` -- all above the 32 MB RAM window, none from either partition
block, so something is computing them from a wrong base.

---

# Interior gaps: 1,351 places where control silently went sideways

Chasing the first bad access -- a read from `0xFFFFC840`, which is a base
register of zero plus a small negative offset -- found this:

```
0000F494  beq  $s0, $zero, 0x0000F4C0
0000F498  lui  $v0, 0x3B          ; delay slot: v0 = 0x003B0000
0000F49C  addu $v0, $zero, $zero  ; not-taken path: v0 = 0
0000F4A0  lw   $ra, 20($sp)       ; ... epilogue ...
0000F4B8  jr   $ra
0000F4C0  lw   $a0, -14272($v0)   ; branch target
```

Taken: `v0 = 0x003B0000`, so the load reads `0x003AC840`. Correct.
Not taken: `v0 = 0`, then the epilogue runs and the function returns.

The emitted code skipped `0x0000F4A0`-`0x0000F4BC` entirely, because discovery
had assigned those instructions to a **different function** -- two functions
sharing an epilogue, which is an ordinary compiler optimisation. The emission
loop skipped every address it did not own:

```c
for (uint32_t a = fn->start; a < fn->end; a += 4) {
    if (!owned_by(an, a, owner)) continue;      /* silently */
```

So the not-taken path fell straight through into `L_0000F4C0` and read memory
at `0 - 14272`. A plausible-looking address, produced by a register the skipped
code would have set.

This is the **same defect as running off the end of a function into the next**,
which was found and fixed earlier in this document. It just happens in the
middle, and the middle case was never considered. **1,351 sites.**

## Effect

```
                        before          after
calls dispatched        108             20,000,000  (budget exhausted)
function entries        4,309,119,950   79,997,235
stall                   0x00067E58      0x0000B6F0
bad accesses            97              253,106,458
```

The entry count *fell* and the bad-access count exploded, and both are
misleading on their own. 4.3 billion entries with 108 dispatched calls is a
tight spin -- a handful of functions calling each other. 80 million entries with
**20 million dispatched calls** is a program doing real work through indirect
calls until it exhausts the budget.

The bad accesses are a new failure much further along: the first is a `read8` in
`strlen` (`0x000123C4`) on a pointer of `0x0C000137`, past the top of RAM. A
string walk on a bad pointer generates a bad read per byte, so one wrong pointer
in a loop accounts for the count.

## The lesson, again

Both halves of this defect -- end-of-function and interior -- come from the same
assumption: that a function occupies a contiguous address range it owns
entirely. It does not. Discovery splits ranges, compilers share epilogues, and
any emission loop that filters by ownership has to say what happens at the
boundary rather than skipping past it.

The first fix taught that and the second case still took months of symptoms to
find, because the metric that would have shown it -- control reaching an address
the hardware would never reach -- was not being measured. It surfaced only by
tracing one bad address back to the instruction that produced it.

## After the interior-gap fix: what the stack checker now says

```
0x00018058: -64  (x6)      0x00063048: -64      0x00062648: -64
0x00063964: -64            0x000180A0: -64      0x00063DB0: -64
0x0000F48C: +32            0x0000F4A0: +32
```

The `+32` pair is the shared epilogue from the case that motivated the fix, and
it is **correct**: `0x0000F4A0` is an epilogue-only body, so calling it raises
`$sp` by the 32 its caller's prologue took. The frame closes exactly once, in
the right place. A checker that flags this is measuring the body rather than the
frame, which was already recorded as a limitation.

The `-64` group is new and is not explained. Those functions consume 64 bytes
and do not return them, and they appear only now that execution reaches them.

## Where the object pointer goes wrong

`0x00000914` is called four times from the same site (`ra=0x00074450`) with:

```
#1 a0=0x00834000   #2 a0=0x00000000   #3 a0=0x00000030   #4 a0=0x00000060
```

`$a0` is `$s1`, which the enclosing function sets to `0x003FD060` via
`lui $s1, 0x40; addiu $s1, $s1, -12192`. None of the observed values match, and
`0x30`/`0x60` are 48 and 96 -- the loop's stride applied to a starting value of
**zero**.

So `$s1` is zero when the loop begins, not `0x003FD060`. Either the instructions
that set it are not being reached on the path taken, or the value is lost across
a call. The `-64` stack leaks above are the obvious suspect: a callee that
leaves `$sp` low makes every callee-saved restore in its caller read the wrong
slot, which produces exactly this -- a register that is plausibly wrong rather
than obviously garbage.

### Next

Start at `0x00018058`, the most frequent `-64` leak, and read its emitted exits
against the original's epilogue. Given the interior-gap fix just changed how
ownership boundaries are emitted, check specifically whether it now transfers
out of a frame it opened without that frame ever being closed -- the fix
mirrors what the hardware does at a *shared epilogue*, but a gap whose target is
not an epilogue has no such guarantee.
