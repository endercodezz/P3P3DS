---
name: psp-static-recomp
description: Guide offline static recompilation of PSP Allegrex MIPS code into C/C++ translation units, control flow analysis, jump table resolution, and VFPU lowering.
---

# PSP Allegrex Static Recompilation Workflow

Use this skill when:
- Analyzing the decrypted P3P executable (`EBOOT.ELF`);
- Constructing control flow graphs (CFG) and identifying basic block boundaries;
- Resolving indirect jumps, switch tables, and branch delay slots;
- Lowering Allegrex MIPS and VFPU instructions into C/C++ code;
- Generating modular translation units (`unit_*.cpp`) for compilation.

---

## 1. Key References

- `recomp/PSPRecomp/src/program_analysis.cpp` — Control flow analysis and function entry discovery.
- `recomp/PSPRecomp/tools/codegen_main.cpp` — C++ code generation, instruction lowering, VFPU macro expansion.
- `recomp/Yakumo/profiles/mhp3rd/` — Real-world example of 355 overlays recompiled into native code.
- `recomp/PSP-recompilation-project/tools/codegen.py` — Alternative Python-based MIPS-to-C lowering pipeline.
- `recomp/N64Recomp/src/analysis/` — State-of-the-art jump table analysis and block splitting.
- `psp/ghidra-allegrex/data/languages/allegrex.slaspec` — Allegrex opcode definitions for Ghidra verification.

---

## 2. Recompilation Pipeline Checklist

Follow this linear sequence:

1. **Section & Relocation Extraction:**
   - Parse ELF headers with `psp/prxtool` or `psprecomp::Elf32Image`.
   - Identify `.text`, `.rodata`, `.data`, `.bss`, and `.lib.stub` import tables.
   - Record virtual base address (`0x08804000` for standard user PRX).
2. **Entry Point & Seed Discovery:**
   - Locate primary entry point (`module_start` / `_start`).
   - Parse export tables and exception handlers.
   - Scan for function prologues (`addiu $sp, $sp, -imm` followed by `sw $ra, imm($sp)`).
3. **Control Flow Graph (CFG) Construction:**
   - Disassemble instructions recursively from all discovered seeds.
   - Ensure delay slots are paired with their branch/jump opcodes.
   - Handle branch likely (`beql`, `bnel`) vs standard branches correctly.
4. **Indirect Jump & Jump Table Resolution:**
   - Identify `jr $reg` patterns.
   - Trace backwards to find base register load (`lui` + `addu` / `lw`).
   - Extract jump table target addresses from `.rodata`.
   - Never leave an unresolved indirect jump without a fallback to `dispatch_indirect(target_pc)`.
5. **VFPU Lowering:**
   - Expand vector registers ($v0-$v7, matrices, and prefixes) into native structs.
   - Lower VFPU operations using templated accessors (`read_vfpu_vector_ct`, `write_vfpu_vector_ct`) as established in `PSPRecomp`.
6. **Code Partitioning:**
   - Split generated code into discrete translation units (256–512 KiB per unit).
   - *Never emit the entire game into a single C++ file* (devkitARM GCC will fail with Out-of-Memory).

---

## 3. Handling Unresolved or Erroneous Functions

When the recompiler encounters an unsupported opcode or malformed basic block:
1. **Do NOT make a blind workaround** (e.g. replacing with a dummy `nop` or skipping instructions).
2. **Disassemble the specific offset:**
   - Check the raw 32-bit hex word.
   - Cross-check against `psp/ghidra-allegrex` and `references/ppsspp/Core/MIPS/MIPSDis.cpp`.
3. **Isolate the function:** Create a minimal standalone microtest in `experiments/`.
4. **Determine root cause:** Is it a jump table index miscalculation, misidentified data embedded in `.text`, or an unhandled VFPU prefix register?
5. **Patch the recompiler or profile config:** Store game-specific jump table hints in `profiles/p3p/config/p3p_functions.csv`. *Never hand-edit generated `unit_*.cpp` files.*

---

## 4. Expected Output Format

When generating code or debugging translation passes:

```text
Pass:             <Analysis | Jump Table Resolution | Codegen>
Function Address: 0x088xxxxx (symbol if resolved)
Instructions:     <Count of instructions in basic block>
Disassembly:      <Relevant MIPS assembly excerpt>
Lowered C++:      <Generated C++ equivalent>
Status:           [VERIFIED | BLOCKED]
```
