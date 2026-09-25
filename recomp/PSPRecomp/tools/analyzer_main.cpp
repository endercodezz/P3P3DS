#include "psprecomp/common.hpp"
#include "psprecomp/elf32.hpp"
#include "psprecomp/nid_registry.hpp"
#include "psprecomp/program_analysis.hpp"
#include "psprecomp/sha256.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

namespace {
std::string json_escape(const std::string &s) {
    std::string out;
    for (char c : s) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

void write_import_csv(const std::filesystem::path &path,
                      const std::vector<psprecomp::PspImport> &imports,
                      const psprecomp::NidRegistry &nids) {
    std::ofstream out(path);
    if (!out) throw psprecomp::Error("Cannot create import CSV: " + path.string());
    out << "library,nid,name,stub_address\n";
    for (const auto &imp : imports) {
        out << imp.library << ',' << psprecomp::hex32(imp.nid) << ','
            << nids.resolve(imp.library, imp.nid).value_or("") << ','
            << psprecomp::hex32(imp.stub_address) << '\n';
    }
}

void write_seed_csv(const std::filesystem::path &path,
                    const std::map<std::uint32_t, std::string> &seeds,
                    std::uint32_t entry) {
    std::ofstream out(path);
    if (!out) throw psprecomp::Error("Cannot create function seed CSV: " + path.string());
    out << "name,address,source\n";
    for (const auto &[address, source] : seeds) {
        out << (address == entry ? "module_start" : "sub_" + psprecomp::hex32(address).substr(2))
            << ',' << psprecomp::hex32(address) << ',' << source << '\n';
    }
}

void write_function_csv(const std::filesystem::path &path,
                        const psprecomp::ProgramAnalysis &program) {
    std::ofstream out(path);
    if (!out) throw psprecomp::Error("Cannot create automatic function CSV: " + path.string());
    out << "name,address,instructions,basic_blocks,direct_calls,indirect_call_sites,unsupported,truncated,seed_source\n";
    for (const auto &function : program.functions) {
        const auto source = program.seeds.find(function.entry);
        out << "sub_" << psprecomp::hex32(function.entry).substr(2) << ','
            << psprecomp::hex32(function.entry) << ','
            << function.labels.size() << ','
            << function.basic_block_count << ','
            << function.direct_calls.size() << ','
            << function.indirect_call_sites.size() << ','
            << function.unsupported_instruction_count << ','
            << (function.truncated ? "true" : "false") << ','
            << (source != program.seeds.end() ? source->second : "") << '\n';
    }
}
}

int main(int argc, char **argv) {
    try {
        if (argc < 2 || argc > 4) {
            std::cerr << "Usage: psp_analyze <decrypted ELF/PRX> [report.json] [load_base_hex]\n";
            return 2;
        }
        const std::filesystem::path input = argv[1];
        const std::filesystem::path output = argc >= 3 ? argv[2] : "psp_report.json";
        std::uint32_t load_base = psprecomp::kDefaultPspUserLoadBase;
        if (argc == 4) load_base = static_cast<std::uint32_t>(std::stoul(argv[3], nullptr, 0));

        auto elf = psprecomp::Elf32Image::from_file(input);
        psprecomp::GuestMemory memory(32u * 1024u * 1024u);
        const auto relocations = elf.load_and_relocate(memory, load_base);
        const auto module = elf.find_module_info(memory, load_base);
        psprecomp::NidRegistry nids;
        std::vector<psprecomp::PspImport> imports;
        if (module) imports = elf.scan_imports(memory, *module);
        const auto program = psprecomp::analyze_program(elf, memory, load_base);

        std::filesystem::create_directories(output.parent_path().empty() ? std::filesystem::path(".") : output.parent_path());
        const auto stem = output.parent_path() / output.stem();
        const auto imports_csv = std::filesystem::path(stem.string() + "_imports.csv");
        const auto seeds_csv = std::filesystem::path(stem.string() + "_function_seeds.csv");
        const auto functions_csv = std::filesystem::path(stem.string() + "_functions_auto.csv");
        write_import_csv(imports_csv, imports, nids);
        write_seed_csv(seeds_csv, program.seeds, elf.runtime_entry(load_base));
        write_function_csv(functions_csv, program);

        std::map<std::string, std::size_t> import_counts;
        for (const auto &imp : imports) ++import_counts[imp.library];
        std::size_t total_blocks = 0u;
        std::size_t total_indirect_sites = 0u;
        std::size_t total_unsupported = 0u;
        std::size_t truncated_functions = 0u;
        for (const auto &function : program.functions) {
            total_blocks += function.basic_block_count;
            total_indirect_sites += function.indirect_call_sites.size();
            total_unsupported += function.unsupported_instruction_count;
            truncated_functions += function.truncated ? 1u : 0u;
        }

        std::ofstream out(output);
        if (!out) throw psprecomp::Error("Cannot create report: " + output.string());
        out << "{\n"
            << "  \"input\": \"" << json_escape(input.string()) << "\",\n"
            << "  \"sha256\": \"" << psprecomp::sha256_file(input) << "\",\n"
            << "  \"elf_type\": " << elf.type() << ",\n"
            << "  \"is_psp_prx\": " << (elf.is_psp_prx() ? "true" : "false") << ",\n"
            << "  \"load_base\": \"" << psprecomp::hex32(load_base) << "\",\n"
            << "  \"entry_relative\": \"" << psprecomp::hex32(elf.entry()) << "\",\n"
            << "  \"entry_runtime\": \"" << psprecomp::hex32(elf.runtime_entry(load_base)) << "\",\n"
            << "  \"relocations\": {\"total\":" << relocations.total
            << ",\"r_mips_32\":" << relocations.r_mips_32
            << ",\"r_mips_26\":" << relocations.r_mips_26
            << ",\"r_mips_hi16\":" << relocations.r_mips_hi16
            << ",\"r_mips_lo16\":" << relocations.r_mips_lo16
            << ",\"unsupported\":" << relocations.unsupported
            << ",\"invalid\":" << relocations.invalid << "},\n"
            << "  \"segments\": [\n";
        for (std::size_t i = 0; i < elf.segments().size(); ++i) {
            const auto &s = elf.segments()[i];
            out << "    {\"index\":" << i
                << ",\"type\":" << s.type
                << ",\"vaddr_relative\":\"" << psprecomp::hex32(s.vaddr)
                << "\",\"vaddr_runtime\":\"" << psprecomp::hex32(elf.segment_runtime_address(i, load_base))
                << "\",\"filesz\":" << s.file_size
                << ",\"memsz\":" << s.memory_size
                << ",\"flags\":" << s.flags << "}"
                << (i + 1 < elf.segments().size() ? "," : "") << "\n";
        }
        out << "  ],\n  \"module\": ";
        if (module) {
            out << "{\"name\":\"" << json_escape(module->name)
                << "\",\"attributes\":" << module->attributes
                << ",\"version\":\"" << static_cast<unsigned>(module->major_version) << '.'
                << static_cast<unsigned>(module->minor_version)
                << "\",\"address\":\"" << psprecomp::hex32(module->address)
                << "\",\"gp\":\"" << psprecomp::hex32(module->gp)
                << "\",\"stub_top\":\"" << psprecomp::hex32(module->stub_top)
                << "\",\"stub_end\":\"" << psprecomp::hex32(module->stub_end) << "\"}";
        } else {
            out << "null";
        }
        out << ",\n  \"import_count\": " << imports.size() << ",\n"
            << "  \"import_libraries\": {\n";
        std::size_t library_index = 0;
        for (const auto &[library, count] : import_counts) {
            out << "    \"" << json_escape(library) << "\": " << count
                << (++library_index < import_counts.size() ? "," : "") << "\n";
        }
        out << "  },\n"
            << "  \"function_seed_count\": " << program.seeds.size() << ",\n"
            << "  \"automatic_analysis\": {\n"
            << "    \"function_count\": " << program.functions.size() << ",\n"
            << "    \"unique_instruction_labels\": " << program.covered_labels.size() << ",\n"
            << "    \"unique_block_entries\": " << program.covered_entry_labels.size() << ",\n"
            << "    \"basic_block_count_with_overlap\": " << total_blocks << ",\n"
            << "    \"indirect_call_sites\": " << total_indirect_sites << ",\n"
            << "    \"unsupported_instruction_occurrences\": " << total_unsupported << ",\n"
            << "    \"overlapping_label_occurrences\": " << program.overlapping_label_count << ",\n"
            << "    \"truncated_functions\": " << truncated_functions << "\n"
            << "  },\n"
            << "  \"artifacts\": {\"imports_csv\":\"" << json_escape(imports_csv.string())
            << "\",\"function_seeds_csv\":\"" << json_escape(seeds_csv.string())
            << "\",\"automatic_functions_csv\":\"" << json_escape(functions_csv.string()) << "\"}\n"
            << "}\n";

        std::cout << "Analyzed " << input.string() << "\n"
                  << "  runtime entry:       " << psprecomp::hex32(elf.runtime_entry(load_base)) << "\n"
                  << "  relocations:         " << relocations.total << " (invalid " << relocations.invalid
                  << ", unsupported " << relocations.unsupported << ")\n"
                  << "  module:              " << (module ? module->name : "<not found>") << "\n"
                  << "  imports:             " << imports.size() << "\n"
                  << "  automatic functions: " << program.functions.size() << "\n"
                  << "  unique code labels:  " << program.covered_labels.size() << "\n"
                  << "  unique block entries:" << program.covered_entry_labels.size() << "\n"
                  << "  blocks with overlap: " << total_blocks << "\n"
                  << "  indirect call sites: " << total_indirect_sites << "\n"
                  << "Wrote " << output.string() << "\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "psp_analyze error: " << e.what() << "\n";
        return 1;
    }
}
