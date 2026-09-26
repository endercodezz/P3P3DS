#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/hle/hle_modules.hpp"
#include "psprecomp/runtime.hpp"
#include "psprecomp/elf32.hpp"
#include <iostream>
#include <vector>
#include <cmath>
static int failures;
#define CHECK(x) do { if(!(x)) {std::cerr << __LINE__ << ": " << #x << "\n"; ++failures;} } while(0)
using namespace p3p3ds::hle;
int main() {
    {
        psprecomp::Runtime r; p3p3ds::KernelState k; register_all_hle_modules(r,k); auto &c=r.cpu(); auto &m=r.memory();
        m.store32(0x08800000,0x12345678); c.gpr[4]=0x08800010; c.gpr[5]=0x08800000; c.gpr[6]=4;
        r.invoke_import("Kernel_Library",0x1839852A,c);
        CHECK(m.load32(0x08800010)==0x12345678); CHECK(c.gpr[2]==0x08800010);
        c.gpr[4]=0;c.gpr[5]=480;c.gpr[6]=272;r.invoke_import("sceDisplay",0x0E20F177,c);
        c.gpr[4]=0x04088000;c.gpr[5]=512;c.gpr[6]=3;c.gpr[7]=1;r.invoke_import("sceDisplay",0x289D82FE,c);
        c.gpr[4]=0x08800020;c.gpr[5]=0x08800024;c.gpr[6]=0x08800028;c.gpr[7]=1;
        r.invoke_import("sceDisplay",0xEEDA2E54,c);
        CHECK(m.load32(0x08800020)==0x04088000); CHECK(m.load32(0x08800024)==512); CHECK(m.load32(0x08800028)==3);
        // Wait APIs take no pointers; poisoned argument registers must not be dereferenced.
        c.gpr[4]=c.gpr[5]=c.gpr[6]=0xDEADBEEF;
        for(auto nid:{0x36CDFADEu,0x8EB9EC49u,0x984C27E7u,0x46F186C3u}) r.invoke_import("sceDisplay",nid,c);
        CHECK(!r.stopped()); CHECK(m.load32(0x08800020)==0x04088000);
        r.invoke_import("sceDisplay",0x9C6EAAD7,c); CHECK(c.gpr[2]==4);
        r.invoke_import("sceDisplay",0x9C6EAAD7,c); CHECK(c.gpr[2]==4);
        c.gpr[4]=3;r.invoke_import("sceDisplay",0x77ED8B3A,c);CHECK(c.gpr[2]==0);
        r.invoke_import("sceDisplay",0x9C6EAAD7,c);CHECK(c.gpr[2]==7);
        r.invoke_import("sceDisplay",0xDBA6C4C4,c); CHECK(std::abs(c.fpr[0]-59.94006f)<0.001f);
        r.invoke_import("sceDisplay",0x7ED59BC4,c); CHECK(c.gpr[2]==0);
    }
    {
        psprecomp::Runtime r; GeManager g; auto &m=r.memory(); const unsigned a=0x08800000;
        const unsigned words[]={0xD2000003,0x9D000200,0x9C000000,0x9F000200,0x9E110000,0x0F000000,0x0C000000};
        for(unsigned i=0;i<7;++i)m.store32(a+4*i,words[i]);
        const auto id=g.enqueue_list(r,a,a,-1,0); CHECK(g.find(id)->commands==0); CHECK(g.sync(r,id,1)==3);
        CHECK(g.update_stall(r,id,a+8)==0); CHECK(g.find(id)->commands==2); CHECK(g.find(id)->pc==a+8);
        CHECK(g.update_stall(r,id,a+8)==0); CHECK(g.find(id)->commands==2);
        CHECK(g.update_stall(r,id,a+28)==0); CHECK(g.find(id)->commands==7); CHECK(g.sync(r,id,1)==0);
        CHECK(g.find(id)->completions==1); g.pump(r); CHECK(g.find(id)->completions==1);
        CHECK(g.state().color_address()==0x04000000); CHECK(g.state().color_stride()==512); CHECK(g.state().format()==3);
        CHECK(g.state().depth_address()==0x04110000); CHECK(g.state().depth_stride()==512);
        CHECK(m.vram_writes().operations==0);
        const auto second=g.enqueue_list(r,a,a,-1,0); CHECK(second!=id);
        CHECK(g.update_stall(r,999,a)<0); CHECK(g.sync(r,999,1)<0); CHECK(g.sync(r,id,2)<0);
        CHECK(g.sync(r,0,1,true)==3);
        g.sync(r,second,0); CHECK(r.stopped()); CHECK(g.find(second)->completions==0);
    }
    {
        psprecomp::Runtime r; GeManager g; auto &m=r.memory();
        m.store32(0x08800000,0x0F000000); m.store32(0x08800004,0x0C000000);
        m.store32(0x08800100,16); m.store32(0x08800104,0); m.store32(0x08800108,32); m.store32(0x0880010C,0x08800200);
        const auto id=g.enqueue_list(r,0x08800000,0,-1,0x08800100);
        CHECK(id>0); CHECK(g.find(id)->stack_count==32); CHECK(g.find(id)->stack_address==0x08800200);
        CHECK(g.find(id)->status==GeStatus::Completed);
    }
    {
        psprecomp::Runtime r; GeManager g; r.memory().store32(0x08800000,0x0B000000);
        int id=g.enqueue_list(r,0x08800000,0,-1,0); CHECK(r.stopped()); CHECK(g.find(id)->completions==0);
        CHECK(r.stop_reason().find("RET")!=std::string::npos);
    }
    {
        psprecomp::Runtime r; auto &m=r.memory(); m.store32(0x04000000,0); m.aot_store16(0x44000004,0);
        m.aot_fast_view().aot_store8(0x04000006,1);
        CHECK(m.vram_writes().operations==3); CHECK(m.vram_writes().bytes==7); CHECK(m.vram_writes().changed==1);
        CHECK(m.vram_writes().minimum==0x04000000); CHECK(m.vram_writes().maximum==0x04000006);
    }
    {
        // Real local ELF reset list, followed by the 29-word captured setup fixture.
        psprecomp::Runtime r; GeManager g;
        auto elf=psprecomp::Elf32Image::from_file("profiles/p3p/game/eboot.elf");
        (void)elf.load_and_relocate(r.memory());
        int reset=g.enqueue_list(r,0x08BB40D4,0,-1,0);
        CHECK(!r.stopped()); CHECK(g.find(reset)->commands==212); CHECK(g.find(reset)->completions==1);
        const unsigned words[]={0xE2001D0C,0xE300F3E2,0xE4000C1D,0xE500E2F3,0x36001010,0x53000007,0x5B3F8000,0x483F8000,0x493F8000,
            0xD2000001,0x9D0001E0,0x9C000000,0xD2000003,0x9D000200,0x9C000000,0x9F000200,0x9E110000,
            0x4C007100,0x4D007780,0x42437000,0x43C30800,0x45450000,0x46450000,0x44C69C40,0x4746EA60,
            0xD6002710,0xD700C350,0x0F000000,0x0C000000};
        for(unsigned i=0;i<29;++i)r.memory().store32(0x08D14600+i*4,words[i]);
        int id=g.enqueue_list(r,0x48D14600,0x48D14600,-1,0); CHECK(g.find(id)->commands==0);
        CHECK(g.update_stall(r,id,0x48D14674)==0); CHECK(g.find(id)->commands==29);
        CHECK(g.state().color_address()==0x04000000); CHECK(g.state().depth_address()==0x04110000);
        CHECK(g.state().color_stride()==512); CHECK(g.state().depth_stride()==512); CHECK(g.state().format()==3);
        CHECK(r.memory().vram_writes().operations==0); CHECK(!r.stopped());
    }
    std::cout << "graphics checks failures=" << failures << "\n"; return failures?1:0;
}
