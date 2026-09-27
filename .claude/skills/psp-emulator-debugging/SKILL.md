---
name: psp-emulator-debugging
description: Guide using PPSSPP as a runtime reference environment for reverse engineering PSP games and developing P3P3DS. Provides workflows for breakpoints, register comparison, function boundary verification, memory investigation, and GPU/GE debugging.
---

# PSP Emulator Debugging Workflow

Use this skill when:
- P3P3DS stopped on a missing guest function and the semantics are unknown;
- Need to confirm a real function boundary before adding it to the recomp pipeline;
- Need to discover real arguments, return values, or calling conventions of a function;
- Need to compare original PSP execution with recompiled execution;
- Need to investigate memory layouts, structures, or field accesses;
- Need to investigate GPU/GE behaviour for the first frame or draw commands.

Do NOT use this skill for:
- Adding speculative function seeds without static CFG confirmation;
- Bypassing runtime errors instead of fixing them properly;
- Replacing static analysis with runtime observation alone;
- Creating fake stubs to pass execution.

PPSSPP is a supplementary evidence source. It does not replace ELF analysis, CFG analysis, PSPRecomp, or generated code review.

---

## 1. Obtaining the Frontier

Before using PPSSPP, capture the current P3P3DS execution state:

- PC (program counter at the stop point)
- RA (return address)
- SP (stack pointer)
- A0-A3 (argument registers)
- V0-V1 (return value registers)
- GP (global pointer)
- Thread/context information if available

Example frontier:

```
PC = 0x08B17054
RA = 0x08B1D138
A0 = 0x08E054F0
```

---

## 2. Locating an Address in PPSSPP

Workflow:

1. Launch the original PSP game ISO through PPSSPP.
2. Open the CPU Debugger / Disassembler (`Debug` menu or `Ctrl+D`).
3. Navigate to the guest address from the frontier.
4. Set an **Execute breakpoint** on that address.
5. Let the game run and wait for the breakpoint to hit.

Breakpoint configuration:

```
Address:  0x08B17054
Type:     Execute
Break:    ON
```

Use **Memory breakpoints** only when specifically analyzing read/write patterns on a data address, not for code flow investigation.

---

## 3. Function Boundary Verification

Before adding any function to the recomp pipeline, verify ALL of the following:

- The address is the true start of the function, not a mid-function label;
- There is a genuine prologue;
- There is a correct return path;
- The caller contract (arguments, stack frame) is consistent.

Typical function entry indicators:

```mips
addiu  sp, sp, -N     ; stack frame allocation
sw     ra, offset(sp)  ; save return address
sw     sX, offset(sp)  ; save callee-saved registers
```

Typical function exit indicators:

```mips
lw     ra, offset(sp)  ; restore return address
addiu  sp, sp, N       ; deallocate stack frame
jr     ra              ; return to caller
```

**A function is NOT confirmed solely because PPSSPP reached the address.** The correct validation requires:

```
runtime evidence  +  static CFG analysis  =  function seed
```

Both sources must agree before a function seed is added.

---

## 4. Register Comparison

Compare register state between PPSSPP and P3P3DS at corresponding execution points:

```
Register   PPSSPP         P3P3DS         Match?
PC         0x08B17054     0x08B17054     YES
RA         0x08B1D138     0x08B1D138     YES
A0         0x08E054F0     0x09100A20     (dynamic)
SP         0x09FBFE00     0x09FC0100     (stack)
GP         0x08CB5920     0x08CB5920     YES
V0         0x00000001     0x00000001     YES
```

Allowed differences (non-critical):
- Stack addresses (SP) — different memory layouts are expected;
- Temporary registers (t0-t9) — not preserved across calls;
- Dynamic heap addresses — allocator differences are expected.

Critical matches required:
- Control flow (PC, RA at call/return boundaries);
- Function arguments (A0-A3);
- Global state (GP-relative accesses);
- Return values and behaviour (V0, V1).

---

## 5. Function Investigation

For an unknown function at a given address:

1. Set a breakpoint at the function entry.
2. When hit, record all entry-state registers.
3. Use **Step Into** to trace internal calls if needed.
4. Use **Step Out** to observe the return state.

After Step Out, record:

- Return PC (should match RA from entry);
- V0 (primary return value);
- V1 (secondary return value, if used);
- Any memory regions modified during execution.

Example investigation record:

```
Entry:
  PC:  0x08B17054
  A0:  0x08E054F0
  A1:  0x00000010
  RA:  0x08B1D138

Return:
  PC:  0x08B1D138
  V0:  0x08EEAD08
  V1:  0x00000000
```

---

## 6. Memory Investigation

Use the PPSSPP memory viewer/debugger to determine:

- Which addresses are read during a function;
- Which offsets from a base pointer are accessed;
- Which fields of a structure are modified.

Record findings in structured format:

```
Base: 0x08E054F0

Reads:
  +0x08
  +0x10

Writes:
  +0x00
  +0x2C
```

**Naming discipline:** Do not assign semantic names to structures or objects without evidence.

Correct: `"structure at 0x08E054F0 with linked-list-like fields at +0x00 and +0x04"`

Incorrect: `"EnemyManager"` (unless proven by string references, RTTI, or symbol tables)

---

## 7. GPU / GE Debugging

Use the PPSSPP GE debugger only when:

- CPU execution is already working correctly up to the point of interest;
- Investigating the first rendered frame or display list setup;
- Diagnosing VRAM or framebuffer issues;
- Tracing missing or incorrect draw commands.

Collect from the GE debugger:

- GE display list commands;
- GPU command sequence and parameters;
- Framebuffer address and format;
- Texture state (address, dimensions, format, CLUT);
- VRAM write patterns.

---

## 8. Evidence Storage

For significant investigations, store reference traces in:

```
docs/reference_traces/
```

File naming convention: use the guest address as the filename.

Example: `docs/reference_traces/08B17054.md`

Template:

```markdown
# Function 0x08B17054

## Entry State

PC:   0x08B17054
RA:   0x08B1D138
A0:   0x08E054F0
A1:   0x00000010
SP:   0x09FBFE00

## Return State

PC:   0x08B1D138
V0:   0x08EEAD08
V1:   0x00000000

## Verified

- Function entry confirmed via prologue pattern
- Caller contract confirmed via RA consistency

## Inferred

- Possible structure allocation (V0 returns heap pointer)
```

---

## 9. Evidence Classification Rules

Every observation must be classified:

**[VERIFIED]:** Confirmed by runtime execution AND corroborated by at least one other source (static disassembly, CFG, PPSSPP cross-reference).

**[INFERRED]:** Suggested by runtime observation but not yet independently confirmed.

Do not write: `"this is an allocator"`
when the evidence only supports: `"this function returns a pointer in V0"`

Do not write: `"this initializes the battle system"`
when the evidence only supports: `"this function writes to 14 fields of a structure at 0x08E054F0"`

---

## 10. Integration with P3P3DS

After obtaining PPSSPP evidence, use it to:

- Verify generated C++ code matches original behaviour;
- Confirm runtime register state at function boundaries;
- Identify divergence points between recompiled and original execution.

Do NOT modify the following based on a single PPSSPP observation alone:

- Runtime core (`core/`);
- HLE modules (`core/hle/`);
- Code generator or recompilation pipeline;
- Generated translation units.

Changes require corroboration from multiple evidence sources.

---

## 11. Summary Rules

**DO:**
- Use PPSSPP as a reference execution environment;
- Save reproducible traces with full register state;
- Confirm function boundaries using BOTH runtime AND static evidence;
- Clearly separate [VERIFIED] from [INFERRED] in all records;
- Record raw observations before drawing conclusions.

**DO NOT:**
- Add a function seed only because PPSSPP reached that address;
- Treat a single runtime observation as complete semantic understanding;
- Replace CFG analysis with emulator stepping;
- Manually patch generated C++ to force execution past a problem.
