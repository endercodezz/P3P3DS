// Exports function boundaries, xrefs, and constructor tables from a PSP binary.
// Run as: analyzeHeadless ... -postScript ExtractAnalysis.java /path/to/output.json
// Requires: ghidra-allegrex v21.3 (Ghidra 12.0.2 build), installed under
//           GHIDRA_INSTALL_DIR/Ghidra/Extensions/ (unzip) or the Ghidra user-settings
//           Extensions dir (GUI: File -> Install Extensions).
//@category PSPrecomp

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceManager;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Ghidra headless postScript for PSP binary analysis.
 *
 * <p>Exports raw function boundaries, cross-references, and constructor tables to JSON.
 * This script runs after Ghidra's auto-analysis is complete (postScript, not preScript).
 *
 * <p>Output JSON fields:
 * <ul>
 *   <li>functions - detected function entries (ghidra + jal_target sources)</li>
 *   <li>xrefs - cross-references from .data/.rodata (3 detection layers)</li>
 *   <li>constructors - ordered init_array entries</li>
 *   <li>blocks - per-block SHA-256 of initialized memory (byte-equality gate, issue #52)</li>
 *   <li>imports - empty array (filled by Rust post-processing stage)</li>
 *   <li>relocations - empty array (filled by Rust post-processing stage)</li>
 *   <li>mid_entries - empty array (filled by Rust post-processing stage)</li>
 * </ul>
 */
public class ExtractAnalysis extends GhidraScript {

    /** PSP user memory range: [0x08800000, 0x0A000000) */
    private static final long PSP_CODE_START = 0x08800000L;
    /** PSP user memory range upper bound (exclusive). */
    private static final long PSP_CODE_END   = 0x0A000000L;

    /**
     * Minimum consecutive code-range pointer run length to treat a .data/.rodata region
     * as an embedded init_array table. Matches V1's _INIT_ARRAY_MIN_RUN = 10.
     */
    private static final int INIT_ARRAY_MIN_RUN = 10;

    /**
     * Maximum number of instructions to walk backward when tracing API callback arguments.
     * Matches V1's backward scan window (32 instructions).
     */
    private static final int CALLBACK_BACKWARD_WINDOW = 32;

    /**
     * PSP APIs that accept a function pointer argument, and the argument index (0-based register
     * index into the a0..a3 argument registers, where index 4 = a0, 5 = a1, 6 = a2, 7 = a3).
     *
     * <p>This matches V1 funcptr.py PSP_FUNCPTR_APIS table exactly.
     * The argument index in the comment is the MIPS register number (a1=5, a2=6, a3=7).
     */
    private static final Map<String, Integer> PSP_FUNCPTR_APIS = new HashMap<>();

    static {
        // a1 = register index 5
        PSP_FUNCPTR_APIS.put("sceKernelCreateThread",               5);
        PSP_FUNCPTR_APIS.put("sceKernelCreateCallback",             5);
        PSP_FUNCPTR_APIS.put("sceKernelSetAlarm",                   5);
        PSP_FUNCPTR_APIS.put("sceKernelSetSysClockAlarm",           5);
        // a2 = register index 6
        PSP_FUNCPTR_APIS.put("sceKernelSetVTimerHandler",           6);
        PSP_FUNCPTR_APIS.put("sceKernelSetVTimerHandlerWide",       6);
        // a3 = register index 7
        PSP_FUNCPTR_APIS.put("sceKernelRegisterThreadEventHandler", 7);
    }

    /**
     * Main entry point called by Ghidra headless after auto-analysis completes.
     *
     * @throws Exception on any error (Ghidra logs the exception and continues headless teardown)
     */
    @Override
    public void run() throws Exception {
        try {
            runExtraction();
        } catch (Exception e) {
            println("ExtractAnalysis FAILED: " + e.getMessage());
            e.printStackTrace();
            throw e;
        }
    }

    /**
     * Core extraction logic: collect all data, assemble JSON, write to output file.
     *
     * @throws Exception on I/O or Ghidra API errors
     */
    private void runExtraction() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            throw new IllegalArgumentException(
                "Missing output path argument. Usage: -postScript ExtractAnalysis.java /path/to/output.json"
            );
        }
        String outputPath = args[0];
        println("ExtractAnalysis: output -> " + outputPath);

        Memory mem       = currentProgram.getMemory();
        FunctionManager fm = currentProgram.getFunctionManager();

        // --- Step 1: Function extraction + JAL cross-validation ---
        JsonArray functions = extractFunctions(fm, mem);

        // --- Step 2-4: Xref extraction (all 3 detection layers) ---
        JsonArray xrefs = extractXrefs(mem);

        // --- Step 5: Init_array / constructor detection ---
        JsonArray constructors = extractConstructors(mem);

        // --- Step 5.5: Per-block SHA-256 hashes (byte-equality gate, issue #52) ---
        JsonArray blocks = extractBlockHashes(mem);

        // --- Step 6: Assemble root JSON object ---
        JsonObject root = new JsonObject();
        root.add("functions",    functions);
        root.add("xrefs",        xrefs);
        root.add("constructors", constructors);
        root.add("blocks",       blocks);
        root.add("imports",      new JsonArray()); // filled by Rust stage
        root.add("relocations",  new JsonArray()); // filled by Rust stage
        root.add("mid_entries",  new JsonArray()); // filled by Rust stage

        Gson gson = new GsonBuilder().setPrettyPrinting().create();
        try (FileWriter writer = new FileWriter(outputPath)) {
            gson.toJson(root, writer);
        }
        println("ExtractAnalysis: wrote " + functions.size() + " functions, "
                + xrefs.size() + " xrefs, "
                + constructors.size() + " constructors");
    }

    // =========================================================================
    // Step 1: Function extraction (ANALYSIS-01, ANALYSIS-03)
    // =========================================================================

    /**
     * Extracts all functions via Ghidra FunctionManager and cross-validates against JAL targets.
     *
     * <p>Any JAL target not in the Ghidra function set is added with source="jal_target" and a
     * placeholder size of 4 bytes. This fixes Ghidra truncation where function starts are missed
     * because they are only reached via JAL (V1 pitfall #10 / ANALYSIS-03).
     *
     * @param fm Ghidra FunctionManager
     * @param mem Ghidra Memory (used to locate the .text block for JAL scanning)
     * @return JsonArray of function entry objects
     * @throws Exception on Ghidra API errors
     */
    private JsonArray extractFunctions(FunctionManager fm, Memory mem) throws Exception {
        JsonArray result = new JsonArray();

        // Collect Ghidra-detected functions. Build an address set for JAL cross-validation.
        Set<Long> functionStarts = new HashSet<>();
        List<JsonObject> ghidraFunctions = new ArrayList<>();

        for (Function f : fm.getFunctions(true)) {
            if (f.isExternal()) {
                continue;
            }

            long entry = f.getEntryPoint().getOffset();

            // IMPORTANT: Do NOT compute size from body address-counting methods.
            // Ghidra function bodies are AddressSetViews that can be non-contiguous (gaps between
            // basic blocks). Address-count approaches return only addresses in the set, missing gap
            // bytes and producing sizes smaller than the actual span. This causes cross-function
            // gotos at branch targets within gaps. Always use maxAddress - entry + 1 instead.
            long maxAddr = f.getBody().getMaxAddress().getOffset();
            long size    = maxAddr - entry + 1; // inclusive range

            JsonObject obj = new JsonObject();
            obj.addProperty("name",     f.getName());
            obj.addProperty("address",  String.format("0x%08X", entry));
            obj.addProperty("size",     size);
            obj.addProperty("is_thunk", f.isThunk());
            obj.addProperty("source",   "ghidra");

            ghidraFunctions.add(obj);
            functionStarts.add(entry);
        }

        // JAL cross-validation: scan .text instructions for jal targets Ghidra missed.
        Set<Long> jalTargets = collectJalTargets(mem);

        // Emit Ghidra functions first, then any JAL-only discoveries.
        for (JsonObject obj : ghidraFunctions) {
            result.add(obj);
        }

        for (long target : jalTargets) {
            if (functionStarts.contains(target)) {
                continue; // already known
            }
            // Ghidra missed this function start. Add as jal_target with placeholder size.
            JsonObject obj = new JsonObject();
            obj.addProperty("name",     String.format("FUN_%08X", target));
            obj.addProperty("address",  String.format("0x%08X", target));
            obj.addProperty("size",     4L); // placeholder — Rust/Ghidra will refine
            obj.addProperty("is_thunk", false);
            obj.addProperty("source",   "jal_target");
            result.add(obj);
            println(String.format("ExtractAnalysis: JAL target 0x%08X not in FunctionManager — added", target));
        }

        return result;
    }

    /**
     * Scans all .text block instructions for MIPS JAL opcodes and collects their targets.
     *
     * @param mem Ghidra Memory
     * @return set of absolute 32-bit target addresses from all JAL instructions
     * @throws Exception on Ghidra Listing API errors
     */
    private Set<Long> collectJalTargets(Memory mem) throws Exception {
        Set<Long> targets = new HashSet<>();
        Listing listing = currentProgram.getListing();

        for (MemoryBlock block : mem.getBlocks()) {
            String name = block.getName();
            if (!name.equalsIgnoreCase(".text")) {
                continue;
            }
            InstructionIterator iter = listing.getInstructions(block.getStart(), true);
            while (iter.hasNext()) {
                Instruction instr = iter.next();
                if (!instr.getMnemonicString().equalsIgnoreCase("jal")) {
                    continue;
                }
                // The first (and only) operand of JAL is the branch target address.
                int numOps = instr.getNumOperands();
                if (numOps < 1) {
                    continue;
                }
                Object[] refs = instr.getOpObjects(0);
                if (refs == null || refs.length == 0) {
                    continue;
                }
                Object op = refs[0];
                if (op instanceof Address) {
                    targets.add(((Address) op).getOffset());
                } else if (op instanceof ghidra.program.model.scalar.Scalar) {
                    targets.add(((ghidra.program.model.scalar.Scalar) op).getUnsignedValue());
                }
            }
        }
        return targets;
    }

    // =========================================================================
    // Steps 2-4: Xref extraction — all 3 detection layers
    // =========================================================================

    /**
     * Collects all xrefs from .data and .rodata using all three detection layers:
     * <ol>
     *   <li>Layer 1: Ghidra ReferenceManager (ANALYSIS-04)</li>
     *   <li>Layer 2: PSP API callback argument backward scan (ANALYSIS-05)</li>
     *   <li>Layer 3: Raw 4-byte-aligned value scan for code-range pointers (ANALYSIS-06)</li>
     * </ol>
     *
     * @param mem Ghidra Memory
     * @return JsonArray of xref objects
     * @throws Exception on Ghidra API errors
     */
    private JsonArray extractXrefs(Memory mem) throws Exception {
        JsonArray xrefs = new JsonArray();
        ReferenceManager rm = currentProgram.getReferenceManager();

        for (MemoryBlock block : mem.getBlocks()) {
            String blockName = block.getName().toLowerCase();
            if (!blockName.equals(".data") && !blockName.equals(".rodata")) {
                continue;
            }

            // Layer 1: ReferenceManager xrefs sourced from this block.
            collectLayer1Xrefs(block, rm, xrefs);

            // Layer 3: Raw byte scan for code-range pointers.
            collectLayer3RawScan(block, xrefs);
        }

        // Layer 2: API callback argument scan (scans all functions, not just data blocks).
        collectLayer2ApiCallbacks(xrefs);

        return xrefs;
    }

    /**
     * Layer 1 — Ghidra ReferenceManager cross-reference export.
     *
     * <p>For each source address in the given block that Ghidra has cross-reference data for,
     * emits all memory references as xref objects with the Ghidra reference type name.
     *
     * @param block .data or .rodata MemoryBlock
     * @param rm ReferenceManager
     * @param out output array to append xref objects to
     */
    private void collectLayer1Xrefs(MemoryBlock block, ReferenceManager rm, JsonArray out) {
        AddressSet addrSet = new AddressSet(block.getStart(), block.getEnd());
        for (Address fromAddr : rm.getReferenceSourceIterator(addrSet, true)) {
            for (Reference ref : rm.getReferencesFrom(fromAddr)) {
                if (!ref.isMemoryReference()) {
                    continue;
                }
                JsonObject xref = new JsonObject();
                xref.addProperty("from_addr", String.format("0x%08X", fromAddr.getOffset()));
                xref.addProperty("to_addr",   String.format("0x%08X", ref.getToAddress().getOffset()));
                xref.addProperty("ref_type",  ref.getReferenceType().getName());
                out.add(xref);
            }
        }
    }

    /**
     * Layer 2 — PSP API callback argument backward scan.
     *
     * <p>For each function in the program, scans instructions for JAL/JALR to any API in
     * PSP_FUNCPTR_APIS. When a call site is found, walks backward up to CALLBACK_BACKWARD_WINDOW
     * instructions looking for LUI+ADDIU or LUI+ORI pairs that reconstruct a 32-bit code pointer.
     *
     * <p>Address reconstruction: (hi16 << 16) + sign_extend(lo16), where sign_extend treats the
     * 16-bit immediate as a signed value in the range [-32768, 32767].
     *
     * @param out output array to append xref objects to (ref_type="API_CALLBACK")
     */
    private void collectLayer2ApiCallbacks(JsonArray out) {
        Listing listing = currentProgram.getListing();

        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            if (f.isExternal()) {
                continue;
            }

            List<Instruction> instrList = collectFunctionInstructions(f, listing);
            int count = instrList.size();

            for (int i = 0; i < count; i++) {
                Instruction instr = instrList.get(i);
                String mnemonic = instr.getMnemonicString().toLowerCase();

                if (!mnemonic.equals("jal") && !mnemonic.equals("jalr")) {
                    continue;
                }

                // Check if this call targets a known funcptr API.
                String calledName = resolveCallTarget(instr);
                if (calledName == null || !PSP_FUNCPTR_APIS.containsKey(calledName)) {
                    continue;
                }

                // Walk backward to find LUI+ADDIU or LUI+ORI pairs encoding a code pointer.
                int windowStart = Math.max(0, i - CALLBACK_BACKWARD_WINDOW);
                long callAddr = instr.getAddress().getOffset();
                List<Long> found = scanBackwardForCodePointer(instrList, windowStart, i);
                for (long target : found) {
                    JsonObject xref = new JsonObject();
                    xref.addProperty("from_addr", String.format("0x%08X", callAddr));
                    xref.addProperty("to_addr",   String.format("0x%08X", target));
                    xref.addProperty("ref_type",  "API_CALLBACK");
                    out.add(xref);
                }
            }
        }
    }

    /**
     * Collects all instructions belonging to a single function in address order.
     *
     * @param f Ghidra Function
     * @param listing Ghidra Listing
     * @return list of instructions (may be empty for external or empty functions)
     */
    private List<Instruction> collectFunctionInstructions(Function f, Listing listing) {
        List<Instruction> result = new ArrayList<>();
        InstructionIterator iter = listing.getInstructions(f.getBody(), true);
        while (iter.hasNext()) {
            result.add(iter.next());
        }
        return result;
    }

    /**
     * Attempts to resolve a JAL/JALR call target to a named function.
     *
     * @param instr a JAL or JALR instruction
     * @return function name at the call target, or null if not resolvable
     */
    private String resolveCallTarget(Instruction instr) {
        int numOps = instr.getNumOperands();
        if (numOps < 1) {
            return null;
        }
        Object[] refs = instr.getOpObjects(0);
        if (refs == null || refs.length == 0) {
            return null;
        }
        Object op = refs[0];
        Address targetAddr = null;
        if (op instanceof Address) {
            targetAddr = (Address) op;
        }
        if (targetAddr == null) {
            return null;
        }
        Function called = currentProgram.getFunctionManager().getFunctionAt(targetAddr);
        if (called == null) {
            return null;
        }
        return called.getName();
    }

    /**
     * Walks backward through an instruction list looking for LUI+ADDIU or LUI+ORI pairs
     * that reconstruct a 32-bit PSP code pointer in [PSP_CODE_START, PSP_CODE_END).
     *
     * <p>The MIPS calling convention loads function pointer arguments using:
     * <pre>
     *   lui  $reg, hi16
     *   addiu $reg, $reg, lo16   (sign-extends lo16)
     * </pre>
     * or the ORI variant when the upper bit of lo16 is 0.
     *
     * @param instrs ordered instruction list for the enclosing function
     * @param from   start index (inclusive) of backward scan window
     * @param to     end index (exclusive) — the call site
     * @return list of reconstructed absolute addresses in the PSP code range
     */
    private List<Long> scanBackwardForCodePointer(List<Instruction> instrs, int from, int to) {
        List<Long> results = new ArrayList<>();

        // Collect LUI immediate values seen in the window, keyed by destination register name.
        Map<String, Long> luiRegs = new HashMap<>();

        for (int i = from; i < to; i++) {
            Instruction instr = instrs.get(i);
            String mnemonic = instr.getMnemonicString().toLowerCase();

            if (mnemonic.equals("lui")) {
                String destReg = getRegisterName(instr, 0);
                long imm = getScalarImmediate(instr, 1);
                if (destReg != null && imm >= 0) {
                    luiRegs.put(destReg, imm);
                }
            } else if (mnemonic.equals("addiu") || mnemonic.equals("ori")) {
                String srcReg = getRegisterName(instr, 1);
                long lo16 = getScalarImmediate(instr, 2);
                if (srcReg == null || lo16 < 0 || !luiRegs.containsKey(srcReg)) {
                    continue;
                }
                long hi16 = luiRegs.get(srcReg);
                long reconstructed = buildAddress(hi16, lo16, mnemonic.equals("addiu"));
                if (reconstructed >= PSP_CODE_START && reconstructed < PSP_CODE_END) {
                    results.add(reconstructed);
                }
            }
        }
        return results;
    }

    /**
     * Reconstructs a 32-bit address from a 16-bit upper half and a 16-bit lower half.
     *
     * <p>For ADDIU (sign-extends lo16): combined = (hi16 << 16) + sign_extend16(lo16).
     * For ORI (zero-extends lo16): combined = (hi16 << 16) | (lo16 & 0xFFFF).
     *
     * @param hi16      upper 16-bit immediate from LUI (not yet shifted)
     * @param lo16      lower 16-bit immediate from ADDIU or ORI (raw unsigned value from Ghidra)
     * @param signExtend true for ADDIU (sign-extend), false for ORI (zero-extend)
     * @return reconstructed 32-bit address
     */
    private long buildAddress(long hi16, long lo16, boolean signExtend) {
        long base = (hi16 & 0xFFFFL) << 16;
        long low;
        if (signExtend) {
            // Treat lo16 as signed 16-bit: values >= 0x8000 are negative.
            long signed = (lo16 & 0xFFFFL);
            if (signed >= 0x8000L) {
                signed -= 0x10000L;
            }
            low = signed;
        } else {
            low = lo16 & 0xFFFFL;
        }
        return (base + low) & 0xFFFFFFFFL;
    }

    /**
     * Gets the register name from an instruction operand at the given index.
     *
     * @param instr Ghidra Instruction
     * @param operandIndex 0-based operand index
     * @return register name string, or null if not a register operand
     */
    private String getRegisterName(Instruction instr, int operandIndex) {
        if (operandIndex >= instr.getNumOperands()) {
            return null;
        }
        Object[] ops = instr.getOpObjects(operandIndex);
        if (ops == null || ops.length == 0) {
            return null;
        }
        Object op = ops[0];
        if (op instanceof ghidra.program.model.lang.Register) {
            return ((ghidra.program.model.lang.Register) op).getName();
        }
        return null;
    }

    /**
     * Gets a scalar (immediate) value from an instruction operand.
     *
     * @param instr Ghidra Instruction
     * @param operandIndex 0-based operand index
     * @return unsigned scalar value, or -1 if not a scalar operand
     */
    private long getScalarImmediate(Instruction instr, int operandIndex) {
        if (operandIndex >= instr.getNumOperands()) {
            return -1;
        }
        Object[] ops = instr.getOpObjects(operandIndex);
        if (ops == null || ops.length == 0) {
            return -1;
        }
        Object op = ops[0];
        if (op instanceof ghidra.program.model.scalar.Scalar) {
            return ((ghidra.program.model.scalar.Scalar) op).getUnsignedValue();
        }
        return -1;
    }

    /**
     * Layer 3 — Raw 4-byte-aligned scan for PSP code-range pointers.
     *
     * <p>Reads all bytes from the block and checks every 4-byte-aligned u32 (little-endian).
     * Any value in [PSP_CODE_START, PSP_CODE_END) is emitted as an xref with ref_type="RAW_SCAN".
     *
     * @param block .data or .rodata MemoryBlock
     * @param out output array to append xref objects to
     * @throws Exception on memory read errors
     */
    private void collectLayer3RawScan(MemoryBlock block, JsonArray out) throws Exception {
        int size = (int) block.getSize();
        if (size < 4) {
            return;
        }
        byte[] bytes = new byte[size];
        block.getBytes(block.getStart(), bytes);
        long blockBase = block.getStart().getOffset();

        // Scan every 4-byte-aligned offset (little-endian u32).
        for (int i = 0; i + 3 < size; i += 4) {
            long val = readU32LE(bytes, i);
            if (val >= PSP_CODE_START && val < PSP_CODE_END) {
                JsonObject xref = new JsonObject();
                xref.addProperty("from_addr", String.format("0x%08X", blockBase + i));
                xref.addProperty("to_addr",   String.format("0x%08X", val));
                xref.addProperty("ref_type",  "RAW_SCAN");
                out.add(xref);
            }
        }
    }

    // =========================================================================
    // Step 5: Init_array / constructor detection (ANALYSIS-08)
    // =========================================================================

    /**
     * Detects constructor functions from three sources:
     * <ol>
     *   <li>.init_array section — each 4-byte pointer is a constructor address</li>
     *   <li>.ctors section — same layout as .init_array</li>
     *   <li>Dense pointer runs in .data/.rodata — runs of >= INIT_ARRAY_MIN_RUN consecutive
     *       4-byte-aligned code pointers indicate an embedded init table (V1 heuristic)</li>
     * </ol>
     *
     * @param mem Ghidra Memory
     * @return JsonArray of hex address strings in discovery order
     * @throws Exception on memory read errors
     */
    private JsonArray extractConstructors(Memory mem) throws Exception {
        JsonArray constructors = new JsonArray();
        List<Long> ctorAddrs = new ArrayList<>();

        for (MemoryBlock block : mem.getBlocks()) {
            String name = block.getName().toLowerCase();

            if (name.equals(".init_array") || name.equals(".ctors")) {
                // Scan every 4-byte-aligned pointer in the section.
                int size = (int) block.getSize();
                if (size < 4) {
                    continue;
                }
                byte[] bytes = new byte[size];
                block.getBytes(block.getStart(), bytes);
                for (int i = 0; i + 3 < size; i += 4) {
                    long ptr = readU32LE(bytes, i);
                    if (ptr >= PSP_CODE_START && ptr < PSP_CODE_END) {
                        ctorAddrs.add(ptr);
                    }
                }

            } else if (name.equals(".data") || name.equals(".rodata")) {
                // Scan for dense runs of consecutive code-range pointers (embedded init tables).
                collectDensePointerRuns(block, ctorAddrs);
            }
        }

        for (long addr : ctorAddrs) {
            constructors.add(String.format("0x%08X", addr));
        }
        return constructors;
    }

    /**
     * Scans a data block for runs of >= INIT_ARRAY_MIN_RUN consecutive 4-byte-aligned values
     * that all fall in the PSP code range.
     *
     * <p>Matches V1's _find_funcptr_runs() heuristic. A "run" is a contiguous sequence of
     * 4-byte-aligned slots where every slot holds a value in [PSP_CODE_START, PSP_CODE_END).
     * When a run reaches or exceeds INIT_ARRAY_MIN_RUN entries, all addresses in that run
     * are added to the constructor list.
     *
     * @param block .data or .rodata MemoryBlock
     * @param out mutable list to append discovered constructor addresses to
     * @throws Exception on memory read errors
     */
    private void collectDensePointerRuns(MemoryBlock block, List<Long> out) throws Exception {
        int size = (int) block.getSize();
        if (size < 4) {
            return;
        }
        byte[] bytes = new byte[size];
        block.getBytes(block.getStart(), bytes);
        long blockBase = block.getStart().getOffset();

        List<Long> currentRun = new ArrayList<>();

        for (int i = 0; i + 3 < size; i += 4) {
            long val = readU32LE(bytes, i);
            if (val >= PSP_CODE_START && val < PSP_CODE_END) {
                currentRun.add(val);
            } else {
                if (currentRun.size() >= INIT_ARRAY_MIN_RUN) {
                    out.addAll(currentRun);
                    println(String.format(
                        "ExtractAnalysis: dense pointer run at 0x%08X, length %d — added as constructors",
                        blockBase + (i - currentRun.size() * 4L),
                        currentRun.size()
                    ));
                }
                currentRun.clear();
            }
        }

        // Handle run that extends to end of block.
        if (currentRun.size() >= INIT_ARRAY_MIN_RUN) {
            out.addAll(currentRun);
            println(String.format(
                "ExtractAnalysis: dense pointer run at end of %s, length %d — added as constructors",
                block.getName(),
                currentRun.size()
            ));
        }
    }

    // =========================================================================
    // Step 5.5: Per-block byte hashes (issue #52 byte-equality gate)
    // =========================================================================

    /**
     * Exports a SHA-256 digest of every initialized, loaded memory block.
     *
     * <p>The Rust analyze stage recomputes the same hashes over its own relocated
     * segment bytes and hard-fails on mismatch, so the two relocation engines
     * (psp-parser reloc.rs vs ghidra-allegrex) can never silently diverge.
     * Uninitialized blocks (bss) have no bytes to hash; non-loaded blocks
     * (Ghidra's OTHER-space artifacts like _elfHeader) have address offsets that
     * can collide numerically with real ram addresses, so both are skipped.
     *
     * @param mem Ghidra Memory
     * @return JsonArray of {name, start, size, sha256} objects
     * @throws Exception on memory read or digest errors
     */
    private JsonArray extractBlockHashes(Memory mem) throws Exception {
        JsonArray blocks = new JsonArray();
        byte[] buf = new byte[65536];
        for (MemoryBlock block : mem.getBlocks()) {
            if (!block.isInitialized() || !block.isLoaded()) {
                continue;
            }
            java.security.MessageDigest md = java.security.MessageDigest.getInstance("SHA-256");
            try (java.io.InputStream in = block.getData()) {
                int n;
                while ((n = in.read(buf)) > 0) {
                    md.update(buf, 0, n);
                }
            }
            StringBuilder hex = new StringBuilder();
            for (byte b : md.digest()) {
                hex.append(String.format("%02x", b));
            }
            JsonObject obj = new JsonObject();
            obj.addProperty("name",   block.getName());
            obj.addProperty("start",  String.format("0x%08X", block.getStart().getOffset()));
            obj.addProperty("size",   block.getSize());
            obj.addProperty("sha256", hex.toString());
            blocks.add(obj);
        }
        return blocks;
    }

    // =========================================================================
    // Utilities
    // =========================================================================

    /**
     * Reads a 4-byte little-endian unsigned 32-bit integer from a byte array.
     *
     * @param bytes source byte array
     * @param offset byte offset (must have at least 4 bytes remaining)
     * @return unsigned 32-bit value as a long
     */
    private static long readU32LE(byte[] bytes, int offset) {
        return ((bytes[offset]     & 0xFFL))
             | ((bytes[offset + 1] & 0xFFL) << 8)
             | ((bytes[offset + 2] & 0xFFL) << 16)
             | ((bytes[offset + 3] & 0xFFL) << 24);
    }
}
