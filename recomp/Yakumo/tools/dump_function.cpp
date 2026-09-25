#include "psprecomp/common.hpp"
#include "psprecomp/decoder.hpp"
#include "psprecomp/elf32.hpp"
#include "psprecomp/guest_memory.hpp"
#include <charconv>
#include <cstdint>
#include <iostream>
#include <string>

static std::uint32_t parse(std::string s) {
    if (s.rfind("0x",0)==0 || s.rfind("0X",0)==0) s.erase(0,2);
    std::uint32_t v{}; std::from_chars(s.data(),s.data()+s.size(),v,16); return v;
}
int main(int argc,char**argv){
 if(argc<4){std::cerr<<"usage: dump_function elf address count\n";return 2;}
 auto elf=psprecomp::Elf32Image::from_file(argv[1]); psprecomp::GuestMemory mem(elf.required_ram_size()); (void)elf.load_and_relocate(mem);
 auto pc=parse(argv[2]); auto count=parse(argv[3]);
 for(std::uint32_t i=0;i<count;i++,pc+=4){ auto w=mem.load32(pc); auto d=psprecomp::decode_allegrex(w);
 std::cout<<psprecomp::hex32(pc)<<" "<<psprecomp::hex32(w)<<" "<<d.mnemonic
 <<" rs="<<d.rs<<" rt="<<d.rt<<" rd="<<d.rd<<" sa="<<d.sa<<" imm="<<d.immediate;
 if(d.kind==psprecomp::OpcodeKind::J||d.kind==psprecomp::OpcodeKind::Jal){auto t=((pc+4)&0xF0000000u)|(d.target<<2);std::cout<<" target="<<psprecomp::hex32(t);} 
 std::cout<<"\n";
 }
}
