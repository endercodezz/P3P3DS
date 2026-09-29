#include "p3p3ds/frontier.hpp"
#include "psprecomp/decoder.hpp"
#include "psprecomp/common.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
int main(int argc,char **argv) {
    try {
        const bool legacy_direct=argc==6;
        if(!legacy_direct && (argc!=7 || (std::string_view(argv[3])!="--direct-jal" &&
                       std::string_view(argv[3])!="--thread-entry")))
            throw psprecomp::Error("usage: p3p_frontier_check ELF merged.csv --direct-jal target caller word | --thread-entry target uid entry");
        auto elf=psprecomp::Elf32Image::from_file(argv[1]); psprecomp::GuestMemory m;
        (void)elf.load_and_relocate(m);
        auto ranges=psprecomp::executable_ranges(elf);
        struct Seed {std::uint32_t pc,size;}; std::vector<Seed> entries;
        std::map<std::uint32_t,std::string> seeds; std::set<std::uint32_t> owned,imports;
        std::ifstream in(argv[2]); if(!in) throw psprecomp::Error("cannot read seed manifest");
        std::string line; std::getline(in,line);
        while(std::getline(in,line)) {
            if(line.empty() || line=="\r") continue;
            std::istringstream row(line); std::string name,address,size;
            std::getline(row,name,',');std::getline(row,address,',');std::getline(row,size);
            if(!size.empty() && size.back()=='\r') size.pop_back();
            auto pc=static_cast<std::uint32_t>(std::stoul(address,nullptr,0));
            if(elf.is_psp_prx() && pc<psprecomp::kDefaultPspUserLoadBase) pc+=psprecomp::kDefaultPspUserLoadBase;
            seeds.emplace(pc,name);
            entries.push_back({pc,(size=="cfg" || size=="auto")?0u:static_cast<std::uint32_t>(std::stoul(size,nullptr,0))});
        }
        for(auto s:entries) {
            std::set<std::uint32_t> labels;
            if(!s.size) labels=psprecomp::analyze_function(s.pc,m,ranges,seeds).labels;
            else for(auto pc=s.pc;pc<s.pc+s.size;) {labels.insert(pc);pc+=psprecomp::decode_allegrex(m.load32(pc)).has_delay_slot()?8:4;}
            for(auto pc:labels) {owned.insert(pc);if(psprecomp::decode_allegrex(m.load32(pc)).has_delay_slot())owned.insert(pc+4);}
        }
        if(auto module=elf.find_module_info(m)) for(auto i:elf.scan_imports(m,*module)) {imports.insert(i.stub_address);imports.insert(i.stub_address+4);}
        const bool thread_entry=!legacy_direct && std::string_view(argv[3])=="--thread-entry";
        const int offset=legacy_direct ? -1 : 0;
        auto origin=thread_entry ? p3p3ds::FrontierOrigin::thread_entry(
                std::stoi(argv[5],nullptr,0),std::stoul(argv[6],nullptr,0)) :
            p3p3ds::FrontierOrigin::direct_jal(std::stoul(argv[5+offset],nullptr,0),std::stoul(argv[6+offset],nullptr,0));
        auto p=p3p3ds::validate_frontier(m,ranges,seeds,owned,imports,
            std::stoul(argv[4+offset],nullptr,0),origin);
        std::cout << "{\"accepted\":" << (p.accepted?"true":"false") << ",\"proof_kind\":\""
                  << (thread_entry?"thread_entry":"direct_jal") << "\",\"reason\":\"" << p.reason
                  << "\",\"instructions\":" << p.instructions << ",\"blocks\":" << p.blocks << "}\n";
        return p.accepted?0:2;
    } catch(const std::exception &e) {std::cerr<<e.what()<<"\n";return 1;}
}
