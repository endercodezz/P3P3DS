// ExportPSPFunctions.java
// @category PSPRecomp

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

import java.io.File;
import java.io.PrintWriter;

public class ExportPSPFunctions extends GhidraScript {
    private static String csvEscape(String value) {
        if (value.contains(",") || value.contains("\"") || value.contains("\n")) {
            return "\"" + value.replace("\"", "\"\"") + "\"";
        }
        return value;
    }

    @Override
    public void run() throws Exception {
        File output = askFile("Export PSPRecomp function map", "Save");
        int count = 0;
        try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
            writer.println("name,address,size");
            FunctionIterator functions = currentProgram.getFunctionManager().getFunctions(true);
            while (functions.hasNext() && !monitor.isCancelled()) {
                Function function = functions.next();
                if (function.isExternal() || function.getBody().isEmpty()) {
                    continue;
                }
                Address entry = function.getEntryPoint();
                Address maximum = function.getBody().getMaxAddress();
                long size = maximum.subtract(entry) + 1L;
                writer.printf("%s,0x%08X,0x%08X%n",
                    csvEscape(function.getName()),
                    entry.getOffset() & 0xFFFFFFFFL,
                    size & 0xFFFFFFFFL);
                count++;
            }
        }
        println("Exported " + count + " functions to " + output.getAbsolutePath());
    }
}
