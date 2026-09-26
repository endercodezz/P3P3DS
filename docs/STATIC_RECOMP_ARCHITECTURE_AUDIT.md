# P3P3DS — Comprehensive Architectural Audit of Modern Static Recompilation Ecosystems
**Comparing XenonRecomp, ReXGlue, Xenia, and N64Recomp with P3P3DS/PSPRecomp**

**Document Version:** 1.0.0  
**Date:** 2026-09-26  
**Primary Target Console:** New Nintendo 3DS / New 3DS XL / New 2DS XL  
**Host Target Profile:** PC Bootstrap (`platform/pc/main.cpp`) & 3DS Native (`platform/3ds/`)  
**Analyzed Executable:** *Shin Megami Tensei: Persona 3 Portable* (`ULUS-10512`, North American release)  
**Baseline Commit:** `646fa1dfbd4d583bb70e936ad8f0a5f4a5b4cc77`  
**Primary Milestone Goal:** **FIRST VISIBLE P3P FRAME ON NINTENDO 3DS**

---

## Executive Summary & Strategic Directive

P3P3DS is an ahead-of-time (AOT) static recompilation engine targeting the New Nintendo 3DS hardware. Its core objective is to execute *Persona 3 Portable* by translating Allegrex MIPS-II machine code into native C++ translation units, backed by a lightweight High-Level Emulation (HLE) kernel, a native Citro3d/PICA200 graphics translator, and an NDSP audio backend.

This architectural audit evaluates modern production-grade static recompiler projects:
1. **XenonRecomp** (Xbox 360 PowerPC to C++)
2. **ReXGlue** (Xbox 360 AOT C++ runtime SDK derived from Xenia)
3. **Xenia** (Xbox 360 research emulator and reference codebase)
4. **N64Recomp / N64ModernRuntime / Zelda64Recomp** (Nintendo 64 MIPS-III to C / runtime)

The goal is to determine what P3P3DS can adopt from these proven ecosystems to advance from its current verified milestone (`user_main` bootstrap stopping deterministically at `sub_08B4E6A0`) to the **First Visible P3P Frame on Nintendo 3DS**, while avoiding two fatal pitfalls:
- Over-engineering a bloated general-purpose PSP emulator (the "Mini-PPSSPP trap").
- Adopting desktop-oriented abstractions (e.g. 4 GiB virtual address spaces, multi-hundred-megabyte flat lookup arrays, heavy multithreading) that exceed the New 3DS hardware envelope (804 MHz ARM11 dual-core, ~178 MiB available heap).

---

## Reference Workspace Inventory

The architectural findings in this audit are grounded in direct source code inspection of vendored technical reference snapshots integrated into `references/static-recomp/`, `references/persona-3-dual/`, and `recomp/N64Recomp/` (tracked as regular files in the P3P3DS repository, without nested `.git`, submodules, or gitlinks):

| Repository | Vendored Path | Inspected HEAD Commit | License | Primary Architectural Focus |
| :--- | :--- | :--- | :--- | :--- |
| **XenonRecomp** | `references/static-recomp/XenonRecomp` | `ddd128bcca99fe8bfbb99bea583c972351fa6ace` | MIT | AOT Codegen, CFG, Jump Tables, Register Opts |
| **rexglue-sdk** | `references/static-recomp/rexglue-sdk` | `c94f5ebdcb3c9d1a460ca48e04f9758448f8d518` | BSD 3-Clause | Runtime SDK, Dispatcher, Thunks, Memory, VFS |
| **xenia** | `references/static-recomp/xenia` | `95a5c3ee250f80c3b9d139658649d9ffb6db3eec` | BSD 3-Clause | Kernel HLE, Memory, Graphics & Thread Emulation |
| **N64Recomp** | `recomp/N64Recomp` | `ffb39cdad1da5de07eaaa48bd1db4a89a7986771` | MIT | MIPS-to-C, Delay Slots, Jump Tables, Relocs |
| **N64ModernRuntime** | `references/static-recomp/N64ModernRuntime` | `cdf5abbd5026fef5c364c676e4667c45e42b6863` | GPL-3.0-or-later | Dynamic Overlays, Patch System, Function Map |
| **Zelda64Recomp** | `references/static-recomp/Zelda64Recomp` | `b65c482ed672258eb684ab94a4e8c8aa662615ee` | GPL-3.0-or-later | Game runtime, C Patches, RT64 Graphics Bridge |
| **UnleashedRecomp**| `references/static-recomp/UnleashedRecomp` | `cf829a9eca8fb680fba4b0409ddeb6ca92f22e3c` | GPL-3.0-or-later | Production XenonRecomp runtime & memory model |
| **Persona 3 Dual** | `references/persona-3-dual` | `0c6dccac10e946f7850cfb10ef6a551d2a849c6d` | CC-BY-NC-SA-4.0 | Dual-Screen Persona Presentation, UI/Bust Separation & Resource Budgeting |

---

## 1. Static Recompilation Pipeline Comparison

Every static recompilation pipeline follows a multi-stage transformation from original console binary to a native executable running on the target hardware. Below is the structural comparison of the four pipelines:

```
P3P3DS / PSPRecomp:
  eboot.elf (PSP PRX)
    └──> Elf32Image (parser & relocator)
          └──> psp_recomp (CFG analysis + recursive descent)
                └──> C++ Translation Units (p3p_generated.cpp, 16 KiB unit buckets)
                      └──> GCC / Clang (PC) or devkitARM GCC (3DS)
                            └──> Runtime + GuestMemory + KernelState + Platform Backend

XenonRecomp:
  game.xex (Xbox 360 PE)
    └──> XenonAnalyse (CFG & Jump Table detection -> TOML config)
          └──> XenonRecomp (PPC disassembly -> C++ emission)
                └──> MSVC / Clang / GCC
                      └──> UnleashedRecomp / ReXGlue Runtime (4GB memory base + dispatcher)

ReXGlue SDK:
  game.xex + manifest
    └──> rex_codegen (sig scan, vtable scan, function graph, template registry)
          └──> Recompiled C++ DLLs / Units (PPCFunc: void(PPCContext&, uint8_t*))
                └──> IModuleRegistrar (registers functions & allocates thunks)
                      └──> rex::Runtime (KernelState, VFS, Graphics, Audio, Memory)

N64Recomp:
  game.z64 (N64 ROM)
    └──> Offline Mod / Analysis (ELF/symbol lists, MIPS disassembly via rabbitizer)
          └──> CGenerator (MIPS-to-C, local registers, jump tables to switch)
                └──> Native C/C++ Compiler
                      └──> librecomp (rdram base + recomp_context + func_map overlays)
```

### Detailed Pipeline Comparison Table

| Pipeline Stage | P3P3DS / PSPRecomp | XenonRecomp | ReXGlue SDK | N64Recomp |
| :--- | :--- | :--- | :--- | :--- |
| **Input Format** | Sony Relocatable PRX (`ELF32`, `ET_SCE_EXEC`) | Xbox 360 Compressed `XEX2` | Xbox 360 `XEX2` / Disassembled Modules | N64 Raw ROM (`.z64`) / ELF |
| **Analysis Pass** | Integrated recursive descent from seeds (`program_analysis.cpp`) | Dedicated tool `XenonAnalyse` (`function.cpp`, `main.cpp`) | Multi-phase `rex::codegen::analyze` (`phases.h`, `analyze.h`) | Disassembly sweep with `rabbitizer` (`analysis.cpp`) |
| **Metadata Storage** | CSV lists (`p3p_functions.csv`, `nids.csv`) | TOML switch tables & hook configs (`recompiler_config.cpp`) | TOML project manifests & embedded metadata | TOML configuration (`config.cpp`, symbols lists) |
| **Intermediate Code** | Direct C++ emission (`codegen_policy.cpp`) | Direct C++ emission (`recompiler.cpp`) | Templated C++ via Inja (`code_emitter.h`) | C source code (`cgenerator.cpp`) |
| **Unit Partitioning** | 16 KiB address-bucketed units (`recomp_unit_XXXX`) | Multi-file split by function size/count | Partitioned by module & address ranges | Multi-file split by code sections & overlays |
| **Target Compiler** | devkitARM GCC (`-O2 -march=armv6k`) & Host MSVC/Clang | MSVC, Clang-CL, GCC (`-O3`) | MSVC, Clang, GCC | MSVC, Clang, GCC |
| **Function Signature** | `void (*)(Runtime &, AllegrexContext &)` | `void (*)(PPCContext &__restrict, uint8_t *base)` | `void (*)(PPCContext &__restrict, uint8_t *base)` | `void (*)(uint8_t *rdram, recomp_context *ctx)` |
| **Indirect Dispatch** | Direct flat array (`direct_functions_`) + hash map | Flat function table placed at `base + IMAGE_BASE + IMAGE_SIZE` | `FunctionDispatcher` (hash map + module tables + thunks) | `get_function(vram)` (`func_map` hash map / section map) |
| **HLE Integration** | NID hash map dispatch + direct C++ callbacks | Export ordinal table to native kernel functions | `ExportResolver` + kernel modules (`xboxkrnl`, `xam`) | Syscall dispatch (`recomp_syscall_handler`) |

### Key Assessment
PSPRecomp already follows the proven static recompilation architecture established by XenonRecomp and N64Recomp. Its core approach—analyzing binary blocks, lowering them into structured C++, partitioning into compilation units, and executing them against an explicit context and guest memory buffer—is verified by all four reference projects.

However, P3P3DS currently lacks:
1. **Separation of analysis metadata from code generation** (XenonAnalyse and N64Recomp both output external metadata files).
2. **Automatic jump table detection and lowering into C++ switch-cases** (N64Recomp and XenonAnalyse both do this automatically).
3. **A unified indirect call / JALR bridge** connecting recompiled code, HLE trampolines, and synthetic callbacks.

---

## 2. Function Discovery & Control Flow Analysis

### 2.1. Implementations in Reference Projects

#### XenonAnalyse (`references/static-recomp/XenonRecomp/XenonAnalyse/function.cpp`)
- Performs block discovery via recursive descent starting from function entries.
- Maintains a `blockStack` of pending blocks. For conditional branches (`PPC_OP_BC`), it splits execution into left (`false`, fallthrough) and right (`true`, branch target) blocks.
- Uses `projectedSize` tracking to detect continuous blocks and avoids splitting fallthrough code unnecessarily.
- For unconditional branches (`PPC_OP_B`), branches pointing backwards before `fn.base` are classified as **tail calls** (`line 155: if (branchDest < base) { continue; }`), terminating the block traversal without polluting the current function's CFG.
- Uses `pdata` (Xbox 360 PE Exception Directory) to discover initial function bounds, and uses manual overrides in `recompiler_config.cpp` (`functions` map) when pdata is incomplete or tail calls confuse boundary heuristics.

#### ReXGlue Codegen (`references/static-recomp/rexglue-sdk/src/codegen/`)
- Implements a multi-pass pipeline:
  1. `FunctionScanner`: sweeps the binary looking for prologues and branch-and-link (`bl`) call sites.
  2. `VtableScanner`: analyzes read-only data sections (`.rdata`) to detect virtual method tables and registers virtual call targets as function seeds.
  3. `SigScanner`: matches known standard library / engine function signatures.
  4. `FunctionGraph`: constructs an inter-procedural call graph, resolving tail calls and shared code blocks.

#### N64Recomp (`references/static-recomp/N64Recomp/src/analysis.cpp`)
- Relies on symbol lists from ELF binaries, `.mdebug` sections, or user-supplied TOML (`[[section.functions]]`).
- Functions without symbols are identified by scanning for `jal` (jump-and-link) targets across all executable sections.
- When an unconditional jump (`j`) occurs outside the current function's address range, N64Recomp classifies it as a tail call and emits a direct tail-call invocation:
  ```cpp
  // cgenerator.cpp:428
  fmt::print(output_file, "{}(rdram, ctx);\n", function_name);
  fmt::print(output_file, "return;\n");
  ```

### 2.2. Comparison with Current PSPRecomp CFG
In P3P3DS, `recomp/PSPRecomp/src/program_analysis.cpp` currently implements recursive descent CFG discovery starting from seeds provided in `profiles/p3p/config/p3p_functions.csv`.
- Discovered entries (`module_start`, `user_main`, etc.) trace conditional branches (`beq`, `bne`, `bgtz`, etc.) and jump targets.
- The current discovery mechanism works well for simple linear functions, but stops when an unlisted indirect call or non-contiguous tail-call block is reached (e.g. current stop at `0x08B4E6A0`).

### 2.3. Architectural Decision: Do We Need a Standalone "PSPAnalyse"?
**Recommendation: REJECT a separate standalone binary; KEEP and EVOLVE the integrated analysis pass.**

**Rationale:**
1. *Why XenonAnalyse is a separate binary:* In the Xbox 360 ecosystem, XEX files are 20–80 MiB binaries with dozens of threads and complex PowerPC idioms. Running analysis took minutes, so caching switch-table results in a TOML file avoided repeated expensive disassembly passes.
2. *P3P Reality:* The P3P `eboot.elf` `.text` is only 3.65 MiB (913,059 instructions). `psp_recomp` analyzes and recompiles the binary in under 3 seconds on PC. Creating a separate `PSPAnalyse.exe` introduces unnecessary build friction, synchronization bugs between analyzer and compiler, and workflow complexity.
3. *What WE DO NEED from XenonAnalyse / N64Recomp:*
   - **Exportable metadata:** Instead of a separate executable, add a `--dump-analysis` mode to `psp_recomp` that serializes discovered function boundaries, jump tables, and unverified branch targets to `profiles/p3p/analysis/p3p_discovered.toml`.
   - **External override ingestion:** Allow `profiles/p3p/config/p3p_functions.csv` or a TOML file to specify manual boundaries, tail-call targets, and switch bounds when automated heuristics reach ambiguous code.

---

## 3. Indirect Calls, Function Pointers, and JALR Resolution

This is one of the highest-risk architectural areas for P3P3DS. Persona 3 Portable contains **835 `jalr` call sites** and **15,940 `jr` sites** (`[VERIFIED]`, `docs/P3P_EXECUTABLE_ANALYSIS.md`).

### 3.1. Reference Architectures

#### XenonRecomp: Fixed Offset Direct Memory Table
In `references/static-recomp/XenonRecomp/XenonUtils/ppc_context.h`:
```cpp
// line 110:
#define PPC_LOOKUP_FUNC(x, y) \
    *(PPCFunc**)(x + PPC_IMAGE_BASE + PPC_IMAGE_SIZE + (uint64_t(uint32_t(y) - PPC_CODE_BASE) * 2))

#define PPC_CALL_INDIRECT_FUNC(x) (PPC_LOOKUP_FUNC(base, x))(ctx, base)
```
- In UnleashedRecomp, the entire 4 GiB guest memory address space is mapped via `VirtualAlloc` or `mmap`.
- Immediately following the game image in memory (`base + PPC_IMAGE_BASE + PPC_IMAGE_SIZE`), XenonRecomp reserves a contiguous array of function pointers.
- Every guest address is 4-byte aligned. By subtracting `PPC_CODE_BASE` and multiplying by 2 (on 64-bit systems: `(delta / 4) * 8 == delta * 2`), it performs an $O(1)$ direct array dereference.
- **Cost on PC:** Extremely fast, zero cache misses beyond L1/L2.
- **Cost on 3DS:** Xenon's 64-bit pointer table would require 8 bytes per 4 bytes of code ($2\times$ code size). For a 40 MB game, that is 80 MB. On 3DS, that would consume half the available RAM!

#### ReXGlue: Central FunctionDispatcher with Thunk Allocator
In `references/static-recomp/rexglue-sdk/include/rex/system/function_dispatcher.h`:
```cpp
// lines 83-92:
bool InitializeFunctionTable(uint32_t code_base, uint32_t code_size, uint32_t image_base,
                             uint32_t image_size, bool is_entrypoint = false);
bool SetFunction(uint32_t guest_address, ::PPCFunc* func) override;
::PPCFunc* GetFunction(uint32_t guest_address);
uint32_t AllocateThunk(::PPCFunc* func, uint32_t caller_address);
```
In `rexglue-sdk/src/system/function_dispatcher.cpp`:
- `ResolveIndirectFunction(guest_address)` queries `FunctionDispatcher::GetFunction(guest_address)`.
- If found, it returns the recompiled native function pointer.
- If not found, it routes to `InvalidFunctionTrap`, logging the missing guest address (`ctx.last_indirect_target`).
- **`AllocateThunk`:** When host C++ code needs to register a callback (e.g. for audio streaming or OS timers), it reserves a guest-visible address in a dedicated 64 KiB reserve window (`kThunkReserveSize = 0x10000`), maps that address to the host callback, and returns the synthetic address. When the guest makes a virtual call or callback via `bctrl`, the dispatcher routes it seamlessly to the host!

#### N64Recomp: Lookup Function Macro & Overlay Map
In `references/static-recomp/N64Recomp/include/recomp.h`:
```cpp
// lines 448-451:
recomp_func_t* get_function(int32_t vram);

#define LOOKUP_FUNC(val) get_function((int32_t)(val))
```
In `references/static-recomp/N64ModernRuntime/librecomp/src/overlays.cpp`:
- `get_function(addr)` looks up `addr` in `func_map` (`std::unordered_map<int32_t, recomp_func_t*>`).
- When dynamic overlays are swapped into RAM, `func_map[ram + offset]` is updated.
- When an unknown address is invoked, `get_function` asserts and reports: `Failed to find function at 0x%08X`.

### 3.2. Evaluation for Nintendo 3DS ARM11 Architecture
On Nintendo 3DS (ARM11 MPCore @ 804 MHz, 32-bit architecture):
- Host pointers are **32-bit (4 bytes)**.
- Guest Allegrex instructions are **32-bit (4 bytes)**.
- Therefore, a direct flat lookup table indexed by instruction offset requires **exactly 1 byte of table per 1 byte of guest code**:
  $$\text{Table Size} = \frac{\text{Code Size}}{4} \times 4\text{ bytes} = \text{Code Size}$$
- For Persona 3 Portable, the entire `.text` section is **3,652,236 bytes (~3.48 MiB)**.
- A flat array covering the entire P3P code section requires **only 3.48 MiB of RAM**!
- On New 3DS, the available application heap in extended memory mode is **~178 MiB**. A 3.48 MiB lookup table is **less than 2% of available heap**!
- On ARM11, a flat array lookup:
  ```armasm
  sub   r0, r0, #0x08800000   @ subtract base
  lsr   r0, r0, #2            @ divide by 4 (instruction index)
  ldr   pc, [r1, r0, lsl #2]  @ direct indexed branch
  ```
  executes in **2 to 3 clock cycles**, completely avoiding hash-table bucket traversals, string hashing, and pointer chasing.

### 3.3. Recommended Architecture for P3P3DS
1. **Unified Function Dispatch Table:**
   Keep and standardize PSPRecomp's `direct_functions_` array in `Runtime`.
   - Base address: `0x08804000` (P3P load base).
   - Span: `0x003B1900` (3.69 MiB).
   - Capacity: ~965,000 pointer slots (3.69 MiB contiguous memory).
2. **JALR Lowering:**
   In `recomp/PSPRecomp/src/codegen_policy.cpp`, lower `jalr $rs` (e.g. `jalr $t9`) to:
   ```cpp
   ctx.gpr[31] = return_pc; // $ra = delay slot end
   auto target_fn = runtime.lookup_direct(ctx.gpr[reg]);
   if (target_fn != nullptr) {
       target_fn(runtime, ctx);
   } else {
       runtime.unresolved_indirect_call(ctx.gpr[reg], return_pc);
   }
   ```
3. **Synthetic Function / Trampoline Registry:**
   Reserve guest address range `0x00000000 - 0x00000FFF` (first 4 KiB of memory, illegal for real PSP code). Any address in this range routes to a registered host C++ trampoline (thread return, HLE callbacks, audio buffers).

---

## 4. Jump Table Detection & Lowering

Persona 3 Portable contains high-density switch statements in event scripts, battle state machines, and menu UI.

### 4.1. Reference Implementations

#### XenonAnalyse Pattern Scanner (`XenonAnalyse/main.cpp`)
- Scans backwards up to 32 instructions from indirect branch `bctr`.
- Identifies:
  1. Upper bounds check: `cmplwi crX, rY, <max_cases>` followed by `bgt`/`ble`.
  2. Jump table offset calculation: `SWITCH_ABSOLUTE` (32-bit absolute pointers), `SWITCH_COMPUTED` (base + shifted offset), or `SWITCH_BYTEOFFSET` / `SWITCH_SHORTOFFSET`.
- Emits switch table definitions into TOML:
  ```toml
  [[switch]]
  base = 0x82154320
  r = 11
  default = 0x82154380
  labels = [ 0x82154340, 0x82154358, 0x82154370 ]
  ```
- XenonRecomp then consumes this TOML and emits native C++:
  ```cpp
  // recompiler.cpp:612
  switch (r11.u64) {
      case 0: goto loc_82154340;
      case 1: goto loc_82154358;
      case 2: goto loc_82154370;
      default: __builtin_unreachable();
  }
  ```

#### N64Recomp Jump Table Detector (`N64Recomp/src/analysis.cpp`)
- Identifies the standard MIPS GCC jump table sequence:
  1. Index bounds check (`sltu` / `bnez`).
  2. `sll reg, index, 2` (multiply case index by 4).
  3. `lui reg, %hi(table)` + `addu reg, reg, table_base`.
  4. `lw target, %lo(table)(reg)`.
  5. `jr target`.
- Scans the jump table words in ROM/RAM sequentially until it encounters an address outside the current function's extent (`jtbl_word < func.vram || jtbl_word >= func.vram + func.size`).
- Emits a clean C++ switch statement in `cgenerator.cpp`.

### 4.2. MIPS Jump Table Patterns in Persona 3 Portable
P3P was compiled with GCC 3.3.6 (Allegrex target). In P3P, jump tables consistently follow two idioms:
1. **Absolute Address Tables:**
   ```mips
   sll   $v0, $a0, 2            # index * 4
   lui   $v1, %hi(jtbl_08B81234)
   addu  $v1, $v1, $v0
   lw    $v0, %lo(jtbl_08B81234)($v1)
   nop                          # load delay slot
   jr    $v0
   nop                          # branch delay slot
   ```
2. **GP-Relative Table Offsets:**
   ```mips
   lw    $v0, %gprel(table)($gp)
   sll   $v1, $a0, 2
   addu  $v0, $v0, $v1
   lw    $v0, 0($v0)
   jr    $v0
   ```

### 4.3. Architectural Recommendation for P3P3DS
1. **Automated MIPS Jump Table Scanner:**
   Add pattern recognition to `recomp/PSPRecomp/src/program_analysis.cpp` matching the `sll -> addu -> lw -> jr` sequence.
2. **Bounds Determination:**
   Table size must be bounded by:
   - The preceding `slt` / `sltu` immediate value if present.
   - Or terminating when the loaded word falls outside the current function's `.text` bounds.
3. **Lowering to C++ `switch`:**
   Generate native C++ `switch (index) { case 0: goto loc_...; }` inside the enclosing recompiled unit. This allows native GCC on 3DS to generate optimal hardware branch tables (`ldr pc, [pc, rX, lsl #2]`).
4. **Fallback Config:**
   Provide `profiles/p3p/config/p3p_jumptables.toml` for manually specifying table bounds when automated boundary detection fails.

---

## 5. CPU Context & Register Allocation

### 5.1. Reference Architectures

| Feature | XenonRecomp PPC | N64Recomp MIPS | PSPRecomp Allegrex |
| :--- | :--- | :--- | :--- |
| **GPR Size** | 64-bit (`uint64_t`) | 64-bit (`gpr` / `uint64_t`) | 32-bit (`uint32_t`) |
| **GPR Storage** | `PPCContext::r[32]` union | `recomp_context::r[32]` | `AllegrexContext::gpr[32]` |
| **Volatile Register Caching** | Optional locals (`ctrAsLocalVariable`, `r14`..) | `hi`, `lo`, `result`, `c1cs` function-local | Context struct field only |
| **Delay Slots** | N/A (PowerPC has no delay slots) | Inline execution before branch/return | Lowered into sequential C++ statements |
| **Condition Codes** | `PPCCRRegister cr[8]` bitfields | `c1cs` flag (FPU condition) | Context flag / GPR comparison |
| **FPU / Vector** | 128-bit VMX (`simde__m128i`) | 32-bit/64-bit Cop1 (`fegetround`) | Single-precision FPU + VFPU matrix |
| **Function Argument Passing**| `ctx` passed by reference + `base` pointer | `uint8_t *rdram, recomp_context *ctx` | `Runtime &runtime, AllegrexContext &ctx` |

### 5.2. Register Allocation Analysis for 3DS ARM11
- **ARM11 Register Constraints:** The ARMv6K architecture provides 16 general-purpose registers:
  - `r0 - r3`: Argument / Scratch registers.
  - `r4 - r11`: Callee-saved local variables.
  - `r12` (`ip`): Intra-procedure call scratch.
  - `r13` (`sp`): Stack pointer.
  - `r14` (`lr`): Link register.
  - `r15` (`pc`): Program counter.
- Since Allegrex has **32 GPRs** plus HI, LO, and VFPU state, it is mathematically impossible to map all guest registers to host ARM11 registers simultaneously.
- **The Hot-Register Experiment in PSPRecomp:** An earlier experiment (`AotHotRegisterCache`, noted in `runtime.hpp:43-49`) attempted to cache seven GPRs and six FPRs in host locals across large 10,000-line units. It resulted in massive compiler degradation: MSVC and GCC spilled registers excessively, compilation memory exceeded 2 GiB per unit, and optimizer convergence failed.
- **Decision:**
  - Keep `AllegrexContext` as an explicit structure passed by reference (`AllegrexContext &ctx`).
  - Let devkitARM GCC perform standard local register allocation on a per-basic-block level.
  - For hot temporary operations within basic blocks, keep intermediate results in local C++ variables (e.g. `uint32_t lo = ...`, `uint32_t hi = ...`).

---

## 6. Runtime Subsystem Architecture & Boundary Formalization

### 6.1. ReXGlue Runtime Architecture Inspection
ReXGlue SDK provides the cleanest modern architectural decoupling for an AOT recompiled game.
In `references/static-recomp/rexglue-sdk/include/rex/runtime.h`:
```cpp
class Runtime {
 public:
  static Runtime* instance();
  memory::Memory* memory() const;
  rex::filesystem::VirtualFileSystem* file_system() const;
  system::KernelState* kernel_state() const;
  system::IGraphicsSystem* graphics_system() const;
  system::IAudioSystem* audio_system() const;
  system::IInputSystem* input_system() const;
  runtime::FunctionDispatcher* function_dispatcher() const;
  runtime::ExportResolver* export_resolver() const;
};
```
And subsystem injection via `RuntimeConfig`:
```cpp
struct RuntimeConfig {
  std::unique_ptr<system::IGraphicsSystem> graphics;
  std::function<std::unique_ptr<system::IAudioSystem>(runtime::FunctionDispatcher*)> audio_factory;
  std::function<std::unique_ptr<system::IInputSystem>(bool tool_mode)> input_factory;
  bool tool_mode = false;
};
```

### 6.2. Proposed P3P3DS Subsystem Architecture
P3P3DS should adopt ReXGlue's clean ownership hierarchy, formalizing target-agnostic core logic and target-specific backends:

```
                      ┌─────────────────────────────────┐
                      │          P3P3DS Runtime         │
                      │         (Master Context)        │
                      └────────────────┬────────────────┘
                                       │
        ┌───────────────┬──────────────┼──────────────┬───────────────┐
        ▼               ▼              ▼              ▼               ▼
┌──────────────┐ ┌─────────────┐ ┌───────────┐ ┌─────────────┐ ┌─────────────┐
│  PSPRecomp   │ │ GuestMemory │ │KernelState│ │     VFS     │ │  Platform   │
│ AOT Engine   │ │  (32 MiB)   │ │  (HLE)    │ │ (Mods/CPK)  │ │   Backend   │
└───────┬──────┘ └─────────────┘ └─────┬─────┘ └─────────────┘ └──────┬──────┘
        │                              │                              │
        │ Direct / JALR                │ Threads, SysMem,             │ PC Host vs
        │ Dispatch                     │ Events, Semaphores           │ 3DS Native
        ▼                              ▼                              ▼
┌──────────────┐                 ┌───────────┐                 ┌─────────────┐
│ Function     │                 │ThreadMan  │                 │  Citro3d /  │
│ Dispatcher   │                 │Priority   │                 │   PICA200   │
└──────────────┘                 │Scheduler  │                 └─────────────┘
                                 └───────────┘                 ┌─────────────┐
                                                               │  NDSP Audio │
                                                               └─────────────┘
```

**Formal Boundaries:**
1. `core/`: Target-agnostic logic (KernelState, ThreadManager, SysMem, VFS, GE command decoding). Zero 3DS or PC-specific includes.
2. `recomp/PSPRecomp/`: Reusable Allegrex static recompiler engine (Decoder, CFG analysis, C++ codegen, FunctionDispatcher, GuestMemory).
3. `platform/pc/`: PC development harness, CLI flags, milestone verification, headless testing, memory dumps.
4. `platform/3ds/`: Nintendo 3DS hardware backend (`libctru`, `citro3d`, `ndsp`, SDMC VFS).

---

## 7. Runtime & Kernel State Ownership: Eliminating `g_active_kernel`

### 7.1. The Current Debt in P3P3DS
At current HEAD (`646fa1d`), `core/src/hle/threadman.cpp` contains:
```cpp
// lines 12 & 30-37:
KernelState *g_active_kernel = nullptr;

void thread_return_trampoline(psprecomp::Runtime &runtime, psprecomp::AllegrexContext &ctx) {
    if (g_active_kernel != nullptr) {
        const std::int32_t exit_status = static_cast<std::int32_t>(ctx.gpr[2]);
        g_active_kernel->threads().exit_current_thread(exit_status, ctx, runtime);
    } else {
        runtime.stop("Thread return without active kernel state");
    }
}
```
This raw global pointer was introduced as an expedient bridge because `psprecomp::Runtime::register_function` accepts a bare C function pointer (`void (*)(Runtime &, AllegrexContext &)`), which cannot capture surrounding state.

### 7.2. How Reference Projects Solve This

#### ReXGlue Solution
In `references/static-recomp/rexglue-sdk/src/system/thread_state.cpp`:
```cpp
thread_local ThreadState* thread_state_ = nullptr;

void ThreadState::Bind(ThreadState* thread_state) {
  thread_state_ = thread_state;
}
ThreadState* ThreadState::Get() {
  return thread_state_;
}
```
When host code dispatches guest execution (`FunctionDispatcher::Execute`), it calls `ThreadState::Bind(thread_state)`. When HLE handlers or traps execute, `current_kernel_state()` accesses the thread-local state safely.

#### Xenia Solution
`xenia::kernel::KernelState` is owned by `Emulator`. Threads hold a direct pointer to `KernelState*`.

### 7.3. Recommended P3P3DS Design
**Proposal: Explicit User Data Pointer in `Runtime`**
Instead of a raw process-global variable or thread-local storage (which can add overhead on devkitARM PThreads), add an opaque context pointer to `psprecomp::Runtime`:

```cpp
class Runtime {
public:
    void set_user_data(void *user_data) noexcept { user_data_ = user_data; }
    [[nodiscard]] void *user_data() noexcept { return user_data_; }
    template <typename T>
    [[nodiscard]] T *user_data_as() noexcept { return static_cast<T *>(user_data_); }
private:
    void *user_data_{nullptr};
};
```
In `platform/pc/main.cpp`:
```cpp
runtime.set_user_data(&kernel_state);
```
In `core/src/hle/threadman.cpp`:
```cpp
void thread_return_trampoline(psprecomp::Runtime &runtime, psprecomp::AllegrexContext &ctx) {
    auto *kernel = runtime.user_data_as<KernelState>();
    if (kernel != nullptr) {
        const std::int32_t exit_status = static_cast<std::int32_t>(ctx.gpr[2]);
        kernel->threads().exit_current_thread(exit_status, ctx, runtime);
    } else {
        runtime.stop("Thread return without active kernel state");
    }
}
```
**Benefits:**
- **Zero global state:** Allows multiple independent runtime instances in unit tests.
- **Migration cost: LOW** (less than 10 lines of code changed across 2 files).
- **Zero runtime overhead:** Single pointer load from `runtime`.
- **Completely portable:** Works identically on PC (MSVC/Clang) and 3DS (devkitARM).

---

## 8. Host Trampolines & Synthetic Guest Functions

### 8.1. Current State
P3P3DS defines:
```cpp
// core/include/p3p3ds/hle/threadman.hpp:41
static constexpr std::uint32_t kThreadReturnSentinel = 0x00000020u;
```
When a thread starts, `$ra` is initialized to `0x00000020`. When the thread function returns via `jr $ra`, execution branches to `0x00000020`. `Runtime` dispatches to `thread_return_trampoline`, invoking the priority scheduler.

### 8.2. Reference Project Findings

#### ReXGlue: `AllocateThunk` (`function_dispatcher.cpp:285`)
ReXGlue reserves `kThunkReserveSize = 0x10000` (64 KiB) at the end of every module's code range.
When the host needs a guest-compatible function pointer (e.g. APC callbacks, timer routines, direct audio feeds), it calls:
```cpp
uint32_t thunk_addr = dispatcher->AllocateThunk(host_fn, caller_addr);
```
`AllocateThunk` increments `next_thunk_address += 4`, registers `host_fn` at that address in `function_table_`, and returns `thunk_addr`.

### 8.3. Recommendation for P3P3DS
**Recommendation: Formalize a `SyntheticGuestRegistry` while keeping `kThreadReturnSentinel` as slot 0.**
- Keep `kThreadReturnSentinel = 0x00000020` for thread return (it is simple, verified, and works).
- Introduce a formal `SyntheticGuestRegistry` covering `0x00000000 - 0x00000FFF`:
  - `0x00000020`: Thread Return Trampoline
  - `0x00000040`: HLE Callback Dispatcher (for `sceKernelRegisterExitCallback`, etc.)
  - `0x00000060`: Audio Stream Refill Trampoline
  - `0x00000080`: GE Display List Finish Interrupt Trampoline
- When guest code receives an interrupt or needs to invoke a host callback, it is given one of these synthetic addresses. Because they are in the direct dispatch table, invocation is immediate and requires zero special-casing in the outer execution loop.

---

## 9. Patch & Override Architecture

### 9.1. Reference Architectures

#### XenonRecomp Mid-Assembly Hooks
In `XenonRecomp/recompiler_config.h`:
```cpp
struct RecompilerMidAsmHook {
    std::string name;
    std::vector<std::string> registers;
    bool ret = false;
    uint32_t jumpAddress = 0;
};
```
Allows arbitrary C++ hooks to be injected at any instruction address without touching the original binary.

#### N64ModernRuntime / Zelda64Recomp
In `N64ModernRuntime/librecomp/src/overlays.cpp`:
- `recomp::overlays::get_base_patched_funcs()`: Compares the vanilla function table against the patch function set.
- Any vanilla function present in the patch list is automatically substituted at startup.
- In Zelda64Recomp, community patches (high frame rate, camera control, UI overhauls) are written in C, compiled into object files, and hot-linked via `patches.toml`.

### 9.2. Application to Persona 3 Portable
Persona 3 Portable on 3DS will require targeted overrides:
1. **Atlus CRI CPK File I/O Hook:** Intercepting internal file reads to implement the multi-tier mod loading pipeline (`sdmc:/p3p3ds/mods/` -> `data.cpk`).
2. **GE Display List Submission:** Intercepting graphics kick-off routines to route display lists directly to Citro3d.
3. **Audio Buffer Hook:** Intercepting ATRAC3+ / ADPCM decoding routines to stream to 3DS NDSP hardware channels.
4. **Sleep/Home Menu Hook:** Intercepting OS power events.

### 9.3. Recommendation
Formalize `GuestFunctionOverride` in `psprecomp::Runtime`:
```cpp
struct FunctionOverride {
    std::uint32_t guest_address;
    Runtime::RecompiledFunction replacement;
    const char *name;
};
```
`Runtime::register_override(address, func, name)` overwrites the entry in `direct_functions_[index]`. This guarantees that both direct calls (via chaining fallback) and indirect calls (`jalr`) immediately execute the native replacement with zero hackery inside the recompiled units.

---

## 10. Memory Architecture & 3DS Constraints

### 10.1. Platform Memory Comparison

| Property | Xbox 360 (Xenon/Xenia) | N64 (N64Recomp) | PSP (P3P3DS Target) | Nintendo 3DS (Host) |
| :--- | :--- | :--- | :--- | :--- |
| **Address Space** | 32-bit (4 GiB virtual) | 32-bit (4-8 MiB RDRAM) | 32-bit (32 MiB User RAM)| 32-bit (physical / OS mapped)|
| **Endianness** | **Big-Endian** (PowerPC) | **Big-Endian** (MIPS) | **Little-Endian** (Allegrex)| **Little-Endian** (ARM11) |
| **Host Endianness**| Little-Endian (x86-64) | Little-Endian (x86-64) | Little-Endian (ARM11/PC) | **Little-Endian (Native)** |
| **Byte Swapping** | Required on every load/store | Required on every load/store | **NONE (Zero cost)** | **NONE (Zero cost)** |
| **Host RAM Size** | 16–64 GiB (PC) | 8–32 GiB (PC) | 32 MiB (PSP-1000) | **178 MiB usable (New 3DS)** |
| **Virtual Memory** | 4 GiB `VirtualAlloc` reserve | 8 MiB pointer offset | 32 MiB flat allocation | 32 MiB flat heap chunk |

### 10.2. Crucial Finding: Endianness Advantage
**Allegrex MIPS is natively LITTLE-ENDIAN.**  
**Nintendo 3DS ARM11 is natively LITTLE-ENDIAN.**  
Unlike XenonRecomp and N64Recomp—which burn significant CPU cycles on `__builtin_bswap16`, `__builtin_bswap32`, and `__builtin_bswap64` for every memory access—**P3P3DS requires ZERO byte-swapping on Nintendo 3DS.** All loads and stores map 1:1 to native ARM11 `ldr`, `str`, `ldrh`, `strh`, `ldrb`, `strb` instructions!

### 10.3. PSP Cached vs Uncached Memory Aliases
PSP hardware maps the 32 MiB User RAM into two virtual ranges:
- Cached: `0x08000000 - 0x09FFFFFF`
- Uncached: `0x48000000 - 0x49FFFFFF`
In `recomp/PSPRecomp/src/guest_memory.cpp`, `GuestMemory::canonical(address)` converts uncached addresses to physical/cached offsets:
```cpp
return (address & 0x03FFFFFFu) | 0x08000000u;
```
On 3DS, this mask operation is inexpensive, but can be optimized further in generated code:
```cpp
#define GUEST_PTR(addr) ((uint8_t*)g_ram_base + ((addr) & 0x01FFFFFFu))
```

---

## 11. Lessons from Xbox 360 Recompilation

### 11.1. DIRECTLY APPLICABLE
1. **Dynamic Thunk Allocation (`rexglue-sdk/src/system/function_dispatcher.cpp:285`):**
   *Implementation:* ReXGlue's `AllocateThunk` allocates synthetic guest addresses in a dedicated reserve pool for host callbacks.
   *P3P3DS Adoption:* Use this for thread return, audio stream updates, and HLE completion callbacks.
2. **Subsystem Interface Injection (`rexglue-sdk/include/rex/runtime.h:66-76`):**
   *Implementation:* Decouple graphics, audio, and platform runners via `RuntimeConfig` interfaces (`IGraphicsSystem`, `IAudioSystem`).
   *P3P3DS Adoption:* Keep `core/` strictly abstract; inject Citro3d graphics and NDSP audio at startup.
3. **Structured Jump Table Lowering to C++ `switch` (`XenonRecomp/recompiler.cpp:612`):**
   *Implementation:* Lower detected table jumps into native `switch` blocks with `__builtin_unreachable()` defaults.
   *P3P3DS Adoption:* Apply to Allegrex `sll + addu + lw + jr` patterns.

### 11.2. CONCEPTUALLY APPLICABLE
1. **Module Function Registration (`IModuleRegistrar`):**
   *Idea:* Generated code registers functions in batches rather than individual `runtime.register_function` calls.
   *P3P3DS Adaptation:* Group generated units into static registration tables to minimize startup time on 3DS.
2. **Thread State Binding (`ThreadState::Bind`):**
   *Idea:* Access kernel and context through the currently scheduled thread object rather than a raw global pointer.
   *P3P3DS Adaptation:* Pass `Runtime&` and `KernelState&` cleanly through `user_data` or execution tokens.

### 11.3. NOT APPLICABLE (Xbox / PowerPC Specific)
1. **Byte Swapping Infrastructure (`XenonUtils/byteswap.h`):** Xbox 360 is big-endian; PSP and 3DS are both little-endian.
2. **Condition Register Emulation (`PPCCRRegister`, 8 independent 4-bit fields):** Allegrex has standard MIPS branch conditions.
3. **64-bit Integer Arithmetic Lowering (`PPCRegister::u64`):** Allegrex is a 32-bit CPU.
4. **XEX Decryption & Pe/COFF Header Parsing:** P3P uses Sony ELF/PRX format.

### 11.4. DANGEROUS TO COPY (Do NOT use on 3DS)
1. **4 GiB Virtual Memory Reservation (`VirtualAlloc(0x100000000)`):** 3DS has only 178 MiB of total application memory.
2. **Dense 64-bit Function Tables for Multi-Gigabyte Spaces:** Xenon allocates tables sized at $2\times$ code size assuming massive host RAM.
3. **Heavy Multithreaded Graphics Engines (Vulkan / D3D12 pipelines):** 3DS has only 2 application CPU cores and a fixed-function PICA200 GPU.

---

## 12. Where N64Recomp Is Superior to Xbox Recomp for P3P3DS

Because N64 is a MIPS architecture, N64Recomp provides solutions directly applicable to Allegrex that Xbox 360 projects do not have:

| Architectural Problem | Why Prefer N64Recomp over XenonRecomp |
| :--- | :--- |
| **Branch Delay Slots** | PowerPC has no delay slots. N64Recomp (`recompilation.cpp:209-240`) correctly handles executing the delay slot instruction before branch/return in C++. |
| **Jump Table Pattern Detection**| Xenon looks for PowerPC `bctr`/`cmplwi`. N64Recomp (`analysis.cpp:288-347`) recognizes MIPS `lui`/`addiu`/`sll`/`addu`/`lw`/`jr` sequences. |
| **Function-Local `hi` / `lo` Registers** | Allegrex uses MIPS `mult`/`div` writing to `hi`/`lo`. N64Recomp (`cgenerator.cpp:400`) makes these C++ local variables (`uint64_t hi = 0, lo = 0;`), avoiding context writes. |
| **Relocation Handling** | P3P has 178,513 MIPS relocations (`R_MIPS_26`, `R_MIPS_HI16`, `R_MIPS_LO16`). N64Recomp handles identical MIPS relocations via `LO16` and `HI16` macros. |
| **Indirect Function Calls (`LOOKUP_FUNC`)** | N64Recomp maps MIPS `jalr` directly to `LOOKUP_FUNC(reg)(rdram, ctx);`, matching our need for dynamic JALR resolution. |

---

## 13. ReXGlue+Xenia vs PSPRecomp+PPSSPP: Architecture & Licensing

### 13.1. Licensing Audit
- **ReXGlue & Xenia:** Both released under the **BSD 3-Clause License**. ReXGlue was legally able to copy, modify, and vendor Xenia source code directly into `rexglue-sdk`.
- **PPSSPP:** Released under **GPLv2 or later** (`GPL-2.0-or-later`, `references/ppsspp/LICENSE.txt`).
- **PSPRecomp & P3P3DS:** Released under the **MIT License**.
- **LEGAL DIRECTIVE:** **Never copy source code from PPSSPP into P3P3DS.** Copying PPSSPP code would impose GPLv2 copyleft requirements on the entire P3P3DS codebase, restricting distribution and complicating homebrew toolchain integration.

### 13.2. Technical Reuse Strategy

| PPSSPP Subsystem | Reusable Directly? | Proper Usage for P3P3DS |
| :--- | :--- | :--- |
| **Core HLE (ThreadMan, SysMem)** | NO (GPL + Heavy) | Clean-room implementation in `core/src/hle/`, using PPSSPP solely as behavioral reference and oracle. |
| **GE Command Decoder** | NO (GPL + Desktop) | Clean-room MIT decoder in `core/include/p3p3ds/ge/`. Use PPSSPP tests to verify command unpacking. |
| **ATRAC3+ / Audio** | NO (GPL / FFmpeg) | Route audio to native 3DS DSP (`ndsp`) hardware channels. |
| **VFS / ISO / Directory** | NO (GPL) | Clean-room multi-tier VFS (`sdmc:/p3p3ds/mods/` -> `data.cpk`). |

---

## 14. Static Recompiler vs Mini-PPSSPP: Drawing the Boundary

**Fundamental Principle: Implement enough PSP environment to run Persona 3 Portable. Do not implement the entire PSP upfront.**

```
Universal Core Infrastructure (Implement Cleanly):
├── Flat Guest Memory (32 MiB User RAM + uncached alias)
├── Allegrex Context & Function Dispatcher
├── Basic Thread Lifecycle (Create, Start, Exit, Delay, Priority Scheduling)
├── SysMem Partition Allocations (User Heap)
├── Basic IoFileMgr (Open, Read, Seek, Close, GetStat)
└── Synchronous GE Display List kick-off (sceGeListEnQueue / Sync)

Defer Until Requested by P3P:
├── Asynchronous File I/O threads
├── Network / Ad-hoc / WLAN services
├── UMD drive authentication / power management
├── Complex ATRAC3+ looping / decode streams
└── Full 3D GE primitive clipping and software curve tessellation
```

---

## 15. Graphics Strategy: From Blocker 0x08B4E6A0 to First Visible P3P Frame

### 15.1. The Dependency Chain to First Frame

```
[Current Blocker: 0x08B4E6A0]
  │ (Unknown function called by user_main @ 0x0880421C)
  ▼
[1. CFG & Function Expansion]
  │ Lower sub_08B4E6A0 and its callees in p3p_functions.csv
  ▼
[2. SysMem Allocations]
  │ Implement sceKernelAllocPartitionMemory / sceKernelGetBlockHeadAddr
  ▼
[3. Display Initialization]
  │ Implement sceDisplaySetMode(0, 480, 272) & sceDisplaySetFrameBuf
  ▼
[4. GE Display List Submission]
  │ Implement sceGeListEnQueue & sceGeListSync
  ▼
[5. Progressive GE Command Decoding]
  │ Minimal command parser: VTYPE, CLEAR, PRIM (sprites/quads), TEXTURE
  ▼
[6. 3DS Citro3d / PICA200 Backend]
  │ Initialize C3D, map PSP 480x272 to 3DS Top Screen (400x240), C3D_FrameDraw
  ▼
[FIRST VISIBLE P3P FRAME: Atlus Disclaimer / Splash Screen]
```

### 15.2. Progressive GE Command Subset
To render the initial copyright/disclaimer screens, P3P does not use 3D skeletal skinning or vertex shaders. It renders **2D textured screen-aligned quads (sprites)**.
The minimal required GE commands for the first frame:
- `GE_CMD_OFFSETADDR` / `GE_CMD_ORIGIN` (base memory address)
- `GE_CMD_VTYPE` (vertex format: 2D texture coordinates + screen coordinates)
- `GE_CMD_TEXTURE_MODE` / `GE_CMD_TEXTURE_ADDR` (texture format: 8-bit paletted or 16-bit RGBA)
- `GE_CMD_CLUTADDR` / `GE_CMD_CLUTMODE` (color lookup table for paletted sprites)
- `GE_CMD_PRIM` (Primitive type: `GE_PRIM_RECTANGLES`)
- `GE_CMD_FINISH` / `GE_CMD_END` (display list completion)

---

## 16. Nintendo 3DS Constraints Analysis

| Hardware Subsystem | Constraint on New 3DS | Architectural Impact & Design Choice |
| :--- | :--- | :--- |
| **CPU Architecture** | Dual-core ARM11 MPCore @ 804 MHz | Little-endian (matches PSP!). No byte swapping overhead. Keep context accesses tight. |
| **CPU Cache** | 32 KB L1 data, 32 KB L1 instruction, 2 MB L2 | Compact code units (16 KiB buckets). Avoid giant multi-megabyte C++ functions. |
| **RAM Budget** | ~178 MiB available in extended mode | Table sizes must be bounded. 3.48 MiB direct dispatch table is ~2% of RAM (Safe). |
| **JIT Support** | **STRICTLY BLOCKED** by 3DS kernel OS | AOT static recompilation is the ONLY viable path; no runtime interpreters/JIT fallbacks. |
| **GPU (PICA200)** | Fixed-function rasterizer + TEV stages | PSP GE commands map naturally to PICA200 combiners and textured quads via Citro3d. |
| **Screen Resolution**| Top: 400x240 (or 800x240 3D); Bottom: 320x240 | PSP resolution is 480x272. Top screen fit: integer crop (400x240) or scale (5:6). |

---

## 17. Comprehensive Gap Analysis: Current P3P3DS vs Reference State

| Subsystem | Current P3P3DS State | Reference Standard | Identified Gap | Risk | Recommended Action | Cost | Priority |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Instruction Decoder**| High coverage (`decoder.cpp`) | Xenon/N64 rabbitizer | Minor Allegrex-specific opcodes | Low | Keep current decoder; extend on demand | LOW | Low |
| **CFG Discovery** | Trace from CSV seeds | XenonAnalyse / N64Recomp | Stops at unseeded indirect calls | **HIGH** | Add recursive expansion and analysis dump | MED | **CRITICAL** |
| **JALR Resolution** | Stops at unmapped target | Xenon `PPC_LOOKUP_FUNC` | No dynamic JALR resolution | **HIGH** | Lower JALR to `runtime.lookup_direct` | LOW | **CRITICAL** |
| **Jump Tables** | Unconditional `jr` only | N64Recomp MIPS switch | No switch-case emission | MED | Pattern scanner for `sll+addu+lw+jr` | MED | HIGH |
| **Function Dispatch** | `direct_functions_` array | Xenon table / ReXGlue | Good foundation, needs JALR tie | Low | **KEEP CURRENT DESIGN**; expose to JALR | LOW | HIGH |
| **Kernel Ownership** | `g_active_kernel` global | ReXGlue `ThreadState::Bind` | Process-global raw pointer debt | Low | Add `runtime.set_user_data` pointer | LOW | MED |
| **Host Trampolines** | `kThreadReturnSentinel` | ReXGlue `AllocateThunk` | Hardcoded single sentinel | Low | Formalize `SyntheticGuestRegistry` | LOW | MED |
| **Function Overrides**| `register_native_fast_path` | Zelda64 C patches / Xenon | Basic fast path exists | Low | Formalize `GuestFunctionOverride` | LOW | Low |
| **SysMem HLE** | Partition allocation stub | PPSSPP SysMem / Xenia | Needs partition block handles | MED | Implement basic block allocator | LOW | HIGH |
| **ThreadMan HLE** | Priority scheduler verified | ReXGlue / Xenia threads | Good, verified at HEAD | Low | **KEEP CURRENT DESIGN** | NONE | Complete |
| **Graphics (GE)** | Stubbed | ReXGlue/Xenia GPU / Citro3d | Zero display list decoding | **HIGH** | Progressive GE parser (sprites first) | HIGH | HIGH |
| **Audio** | Unimplemented | ReXGlue audio / 3DS NDSP | Zero audio output | Low | Defer until first frame boots | MED | Low |
| **VFS** | Path translation in Runtime | ReXGlue VFS | Multi-tier mod/CPK not wired | Low | Defer until game opens first CPK | MED | Low |

---

## 18. Architectural Decisions: Keep, Adopt, or Reject

### 18.1. KEEP CURRENT DESIGN (Do Not Touch)
- **`GuestMemory` 32 MiB Buffer:** Compact, simple, matches PSP User RAM exactly.
- **16 KiB Unit Partitioning (`recomp_unit_XXXX`):** Keeps generated translation units small enough for devkitARM GCC to compile quickly without optimizer stalls.
- **Priority Thread Scheduler in `ThreadManager`:** Verified working at HEAD; passes FIFO priority scheduling tests.
- **Zero Byte-Swapping Policy:** Exploits native little-endian match between Allegrex and ARM11.

### 18.2. ADOPT NOW (Next Milestones)
- **Direct JALR Resolution via `direct_functions_`:** Enables P3P's 835 indirect call sites to execute without halting.
- **`Runtime::set_user_data` / `user_data_as<T>`:** Eliminates `g_active_kernel` raw global pointer.
- **MIPS Jump Table Pattern Lowering to C++ `switch`:** Translates `jr` jump tables into native fast branches.

### 18.3. ADOPT LATER
- **`SyntheticGuestRegistry`:** Formal thunk allocator for host audio/GE callbacks once hardware backends are connected.
- **Multi-tier VFS (CPK + Loose Files):** Needed when `user_main` attempts to open `data.cpk`.
- **Progressive GE Command Interpreter:** Needed when `user_main` submits the first display list.

### 18.4. REJECT
- **Standalone `PSPAnalyse.exe`:** Unnecessary build complexity for a 3.65 MiB binary.
- **4 GiB Memory Mapping:** Incompatible with Nintendo 3DS physical memory limitations.
- **Direct Source Code Copying from PPSSPP:** GPLv2 licensing contamination risk.

---

## 19. Mandatory Answers to the 14 Architecture Questions

1. **Do we need a standalone `PSPAnalyse` like `XenonAnalyse`?**  
   **NO.** Keep the analysis pass integrated in `psp_recomp`, but add `--dump-analysis` to emit structured TOML metadata for inspection and manual override.
2. **Do we need a unified guest-address to native-function lookup?**  
   **YES.** We already have `Runtime::direct_functions_`. It must be formalized as the single lookup source for JALR, direct call fallback, and HLE overrides.
3. **How should `jalr` / function pointers be handled?**  
   Lower `jalr $rs` to an $O(1)$ indexed lookup into `direct_functions_[(addr - base) >> 2]`. On 3DS, this costs 2–3 CPU cycles and consumes only 3.48 MiB of RAM.
4. **How should callbacks be handled?**  
   Assign callbacks synthetic guest addresses in `0x00000000 - 0x00000FFF` registered in `direct_functions_`, returning to host C++ functors.
5. **How should jump tables be handled?**  
   Detect the MIPS `sll + addu + lw + jr` pattern during analysis and emit native C++ `switch` statements inside the enclosing unit.
6. **Should we formalize native overrides?**  
   **YES.** Provide `register_override(address, func)` to allow P3P-specific hooks (CPK redirect, display list interception) without editing generated code.
7. **How to eliminate `g_active_kernel`?**  
   Add an opaque `user_data` pointer to `Runtime`. `thread_return_trampoline` retrieves `KernelState*` via `runtime.user_data_as<KernelState>()`.
8. **Should synthetic trampolines be formalized?**  
   **YES.** Keep `0x00000020` for thread return now; expand into a formal `SyntheticGuestRegistry` for future callbacks.
9. **Use PPSSPP as reference or runtime foundation?**  
   **REFERENCE ONLY.** PPSSPP is GPLv2+ (licensing risk) and heavyweight (desktop architecture). Write clean-room MIT implementations tailored for 3DS.
10. **How to separate reusable PSPRecomp from P3P-specific code?**  
    Keep `recomp/PSPRecomp/` strictly generic (MIPS AOT, Memory, Dispatcher). Keep all game addresses, configs, and hooks in `profiles/p3p/` and `core/`.
11. **Minimal GE architecture for first frame?**  
    A progressive command parser decoding 2D textured rectangles (`GE_PRIM_RECTANGLES`) and clearing buffers, hooked to Citro3d on 3DS top screen.
12. **Which Xbox recomp ideas are most valuable?**  
    Dynamic thunk allocation (`AllocateThunk`), subsystem interface decoupling (`RuntimeConfig`), and clean switch-case lowering.
13. **Which N64Recomp ideas are more useful than Xbox?**  
    MIPS delay slot lowering, MIPS jump table detection, local `hi`/`lo` variables, and MIPS relocation handling.
14. **Which current PSPRecomp parts are good and must NOT be touched?**  
    The 32 MiB `GuestMemory` model, 16 KiB unit partitioning, the priority thread scheduler, and the zero-byte-swapping policy.

---

## 20. Practical Roadmap to First Visible Frame

```
[Milestone 1] (Immediate Next Step)
  Goal: Resolve current execution blocker at 0x08B4E6A0.
  Scope: Expand CFG discovery to include sub_08B4E6A0 and its immediate call tree; generate updated AOT units.
  Verification: Execution progresses past 0x08B4E6A0 to the next deterministic stop.

[Milestone 2]
  Goal: Implement Unified JALR Dynamic Resolution.
  Scope: Update codegen to route JALR through direct_functions_; handle register indirect branches.
  Verification: AOT functions executing JALR succeed without halting.

[Milestone 3]
  Goal: Clean Architecture Cleanup (Eliminate g_active_kernel).
  Scope: Add user_data to Runtime; wire ThreadManager to KernelState through Runtime instance.
  Verification: Zero global state; existing unit tests pass cleanly.

[Milestone 4]
  Goal: Basic SysMem Partition Allocator.
  Scope: Implement sceKernelAllocPartitionMemory, sceKernelGetBlockHeadAddr in core/src/hle/sysmem.cpp.
  Verification: P3P successfully allocates its primary engine heap buffers.

[Milestone 5]
  Goal: Display & Framebuffer HLE Stubs.
  Scope: Implement sceDisplaySetMode and sceDisplaySetFrameBuf in core/src/hle/.
  Verification: P3P registers its double-buffered VRAM pointers (0x04000000 / 0x04088000).

[Milestone 6]
  Goal: Minimal Progressive GE Command Parser.
  Scope: Implement sceGeListEnQueue / Sync; parse CLEAR and 2D PRIM display lists in memory.
  Verification: Display list commands decoded into structured draw packets.

[Milestone 7] (MILESTONE ACHIEVED: FIRST VISIBLE FRAME)
  Goal: 3DS Citro3d Native Display Output.
  Scope: Connect decoded display list packets to Citro3d textured quads; present to 3DS top screen.
  Verification: Atlus disclaimer / copyright screen renders visibly on Nintendo 3DS hardware / Citra.
```

---

## 21. If P3P3DS Were Started Today

### Greenfield Architecture
If started today with complete hindsight from XenonRecomp, ReXGlue, and N64Recomp:
1. We would have built `psp_recomp` with a pure decoupling from day one: an offline compiler emitting C99/C++ against an abstract `libpspruntime` SDK.
2. We would have structured `Runtime` around an interface-injected `RuntimeConfig` matching ReXGlue.
3. We would have adopted N64Recomp's MIPS delay-slot and jump-table analysis upfront.

### Should We Rewrite Now?
**EMPHATICALLY NO.**
P3P3DS already has a working decoder, verified PRX relocator, 16 KiB unit partitioning, priority thread scheduler, and a verified boot path from `module_start` through `user_main`.
Rewriting would introduce months of regressions for zero user-visible gain.
The correct strategy is **evolutionary adoption**: retain what works, apply targeted architectural refinements (JALR resolution, `user_data` context, progressive GE), and march directly toward the **First Visible P3P Frame**.

---

## 22. Lessons from Persona 3 Dual

`references/persona-3-dual` (p3d-project) is an open-source homebrew implementation of Persona 3 mechanics targeting the Nintendo DS. 

### Critical Role Clarification
Persona 3 Dual is **NOT** a static recompilation engine or a PSP emulator. It does not execute Allegrex MIPS code or interpret PSP PRX binaries. Instead, it serves as a specialized **Nintendo dual-screen Persona presentation, UI layout, handheld asset pipeline, and resource-management reference**.

Inspection of its primary subsystems (`source/views/EnvironmentView.cpp/.hpp`, `source/components/screens/DialogueScreen.cpp/.hpp`, `source/systems/UISystem.cpp/.hpp`, `source/managers/RenderManager.cpp/.hpp`, `source/controllers/AnimationController.cpp/.hpp`, and `tools/converters/`) yields concrete technical insights for P3P3DS:

1. **Main-Screen 3D + Sub-Screen 2D/UI Separation:**
   - In `RenderManager.cpp` and `EnvironmentView.cpp`, Persona 3 Dual dedicates the upper screen primarily to 3D environment rendering (geometry, camera, lighting, character models) while assigning the lower touch screen to 2D UI panels, text rendering, and state selection.
   - This cleanly decouples 3D vertex throughput from 2D sprite fill-rate, preventing UI overdraw from starving 3D rasterization.

2. **Dialogue UI & Choice Presentation on Lower Screen:**
   - In `DialogueScreen.cpp`, dialogue text boxes, speaker nameplates, and multiple-choice prompt trees are handled as stateful lower-screen components.
   - Dialogue advances via touch taps or gamepad buttons without obscuring the 3D scene or character models rendered above.

3. **Bust / Portrait Sprite Lifecycle:**
   - In `AnimationController.cpp` and `UISystem.cpp`, character bustups and emotion portraits are managed through dedicated texture staging slots with explicit lifecycle states (fade-in, idle blink, mouth flap sync, fade-out).
   - Textures for inactive characters are evicted or banked to avoid exhausting limited handheld VRAM.

4. **Touch Interaction Model:**
   - `UISystem.cpp` maps touch-screen coordinates directly to menu selection indices and confirmation buttons, while simultaneously maintaining full gamepad input parity.

5. **VRAM and Resource Budgeting:**
   - Constrained handheld hardware requires strict partitioning of video memory between 2D tilemaps, 3D texture banks, and framebuffers.
   - Double-buffering must be budgeted explicitly to avoid frame tearing without exceeding onboard VRAM limits.

6. **Hardware-Layer Management:**
   - Handheld GPUs provide hardware 2D background layers alongside 3D polygon engines. Persona 3 Dual delegates static backgrounds and text planes to dedicated 2D hardware layers, preserving 3D pipeline capacity for dynamic meshes.

7. **Fixed-Point Rendering:**
   - Persona 3 Dual relies heavily on fixed-point arithmetic (`f32`, `v16`) suited to the ARM946E-S processor on Nintendo DS, which lacks hardware floating-point units.

8. **Display-List & Model Preprocessing:**
   - `tools/converters/` (`obj2environment.py`, `glb2anim_model.py`, `glb2static_model.py`) pre-bakes 3D meshes into optimized display lists and binary vertex streams offline, eliminating runtime parsing overhead.

9. **Asset Preprocessing Pipeline:**
   - `tools/build_asset.py` and `tools/converters/preconvert_assets.py` demonstrate automated offline conversion of audio, video, textures, and geometry into formats matching handheld hardware capabilities.

---

### APPLY TO P3P3DS

The following architectural concepts from Persona 3 Dual provide direct architectural value to the future P3P3DS 3DS platform backend:

- **Decoupled Dual-Screen Presentation Architecture:** Structuring the P3P3DS rendering subsystem to support splitting 3D scenes (top screen) from 2D UI/dialogues (bottom screen) when running in enhanced dual-screen mode.
- **Dedicated Portrait / Bust Sprite Cache:** Establishing a dedicated texture slot and cache manager for character dialogue bustups (`.spr` / `.gim`) to prevent redundant texture uploads during dialogue sequences.
- **Touch-to-Input Translation Layer:** Mapping lower-screen touch rectangles to PSP virtual button events (`SCE_CTRL_CROSS`, `SCE_CTRL_CIRCLE`, directional d-pad), allowing seamless touch menu navigation without modifying recompiled game logic.
- **Staging Buffer Memory Partitioning:** Isolating Citro3d vertex command buffers, display list staging areas, and PICA200 render targets within the New 3DS 124–178 MiB application heap and 6 MiB VRAM.

---

### REFERENCE ONLY

The following DS-specific implementations in Persona 3 Dual must **NOT** be copied into P3P3DS:

- **libnds Hardware Register Access:** Code directly manipulating DS Nitro registers (`videoSetMode`, `vramSetBank*`, `REG_DISPCNT`) is DS-specific and incompatible with Nintendo 3DS Horizon OS / `libctru` / `citro3d`.
- **Fixed-Point Arithmetic:** P3P3DS targets the New 3DS ARM11 MPCore with full VFPv2 hardware floating-point support; replacing IEEE 754 floats with DS fixed-point math would degrade accuracy and performance.
- **DS Polygon Pipeline & Vertex Formats:** Low-level polygon submission tailored to the DS 3D engine does not map to the PICA200 programmable/fixed-function Tev pipeline.
- **Destructive Asset Conversions:** P3P3DS preserves the original game's CPK archives, PAC containers, and GIM textures via its VFS and runtime format translators, rather than requiring destructive offline re-authoring of retail assets.

---

## 23. Dual-Screen Presentation Architecture for P3P3DS

P3P3DS targets the New Nintendo 3DS family, which features two distinct physical screens: Top Screen (400×240, stereoscopic 3D capable) and Bottom Screen (320×240, resistive touch panel).

The project adopts a two-phase presentation architecture:

### Phase 1: Early Hardware Bring-up & Diagnostics
During initial kernel development, static recompilation expansion, and renderer bring-up:
- **Top Screen (400×240):** Displays the raw PSP/P3P framebuffer (480×272 native, scaled to 400×227 with letterboxing or 1:1 pixel-centered crop) or the output of the first native Citro3d display list renderer.
- **Bottom Screen (320×240):** Dedicated P3P3DS diagnostic HUD:
  - Active guest PC (`vCPU.pc`) and current executing thread (`user_main`, priority, state).
  - Kernel HLE summary (SysMem heap allocation meters, active threads, open I/O descriptors).
  - Performance telemetry (FPS, frame rendering time in microseconds, CPU Core 0 / Core 2 utilization).
  - Scrollable on-screen diagnostic log buffer capturing recent `P3P_INFO` / `P3P_DEBUG` events.

### Phase 2: Mature Gameplay & Enhanced Presentation
Once the game engine, GE display lists, and event scripting run stably:
- **Top Screen (400×240):** Dedicated to high-immersion visual elements: 3D Tartarus exploration, 3D battle scenes, animated movie sequences, and field camera perspectives.
- **Bottom Screen (320×240):** Dedicated to interactive HUD elements: dialogue boxes, choice selection trees, Tartarus minimap, party HP/SP status, inventory menus, and touch-screen action shortcuts.
- **Strict Invariance Requirement:** Classic 1:1 original single-screen PSP presentation (rendering the full PSP display on the top screen with bottom screen idle or displaying classic art/status) must **always remain selectable as a configuration setting**, ensuring 100% fidelity to the original release.

*(Note: The dual-screen presentation design is an architectural specification for future renderer milestones. It is not implemented at the current bootstrap milestone.)*
