// Static instruction histogram: decodes an ELF's .text (default the P3P
// EBOOT) and prints how often each decoder mnemonic occurs. Used to decide
// which pspautotests CPU/VFPU discrepancies matter for P3P.
//   mnemonic_histogram [elf] [mnemonic substrings...]
#include "psprecomp/decoder.hpp"
#include "psprecomp/elf32.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main(int argc, char **argv) {
    const std::string elf_path = argc > 1 ? argv[1] : "profiles/p3p/game/eboot.elf";
    std::vector<std::string> filters(argv + std::min(argc, 2), argv + argc);
    const auto elf = psprecomp::Elf32Image::from_file(elf_path);
    psprecomp::GuestMemory memory;
    (void)elf.load_and_relocate(memory, psprecomp::kDefaultPspUserLoadBase);
    std::map<std::string, std::uint64_t> counts;
    for (const auto &section : elf.sections()) {
        if (section.name != ".text") continue;
        const auto base = psprecomp::kDefaultPspUserLoadBase + section.address;
        for (std::uint32_t off = 0; off < section.size; off += 4u)
            ++counts[psprecomp::decode_allegrex(memory.load32(base + off)).mnemonic];
    }
    std::vector<std::pair<std::string, std::uint64_t>> sorted(counts.begin(), counts.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
    for (const auto &[name, count] : sorted) {
        if (!filters.empty() && std::none_of(filters.begin(), filters.end(), [&](const std::string &f) { return name.find(f) != std::string::npos; }))
            continue;
        std::cout << count << "\t" << name << "\n";
    }
    return 0;
}
