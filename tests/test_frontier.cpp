#include "p3p3ds/frontier.hpp"
#include "../platform/pc/telemetry.hpp"
#include <iostream>
static int failures;
#define CHECK(x) do {if(!(x)){std::cerr<<__LINE__<<": "<<#x<<"\n";++failures;}}while(0)
int main() {
    using namespace psprecomp;
    {
        GuestMemory m;
        const std::uint32_t a=0x08801000,c=0x08800000;
        const std::uint32_t jal=0x0C000000u|((a>>2)&0x03FFFFFF);
        m.store32(c,jal);m.store32(c+4,0);
        m.store32(a-8,0x03E00008);m.store32(a-4,0);
        m.store32(a,0x24020001);m.store32(a+4,0x03E00008);m.store32(a+8,0);
        std::vector<ExecutableRange> ranges={{c,a+12}};
        std::map<std::uint32_t,std::string> seeds={{c,"caller"}};
        std::set<std::uint32_t> owned={c,c+4};
        auto check=[&](std::uint32_t target=0x08801000,std::uint32_t word=0) {return p3p3ds::validate_frontier(m,ranges,seeds,owned,{},target,c,word?word:jal);};
        CHECK(check().accepted);
        CHECK(!check(a+1).accepted);CHECK(!check(0x04000000).accepted);
        CHECK(!check(a,0x0320F809).accepted); // JALR, not an executed direct call.
        CHECK(!p3p3ds::validate_frontier(m,ranges,seeds,owned,{a},a,c,jal).accepted);
        owned.insert(a+8);CHECK(!check().accepted);owned.erase(a+8); // delay-slot overlap
        m.store32(a-8,0);CHECK(!check().accepted);m.store32(a-8,0x03E00008);
        m.store32(a,0x03200008);CHECK(!check().accepted); // JR t9
        m.store32(a,0x0000000D);CHECK(!check().accepted); // BREAK is not lowered.
        m.store32(a,0x10000040);CHECK(!check().accepted); // branch leaves executable range
        m.store32(a,0x24020001);m.store32(a+8,jal);CHECK(!check().accepted); // control in slot
        // Delayed RA save in the first ordinary branch, after a long prologue.
        ranges.back().end=a+72;m.store32(a-8,0);m.store32(a,0x27BDFFE0);
        for(unsigned off=4;off<48;off+=4)m.store32(a+off,0);
        m.store32(a+48,0x10000001);m.store32(a+52,0xAFBF000C);
        m.store32(a+56,0x8FBF000C);m.store32(a+60,0x03E00008);m.store32(a+64,0x27BD0020);
        CHECK(check().accepted);
        m.store32(a+48,0x50000001);CHECK(!check().accepted); // BEQL may annul RA save.
    }
    {
        Runtime r; p3p3ds::KernelState k;r.frontier_diagnostics=true;
        set_runtime_thread_identity(2,"test");
        r.cpu().pc=0x08801000;r.cpu().gpr[31]=0x08800008;
        r.record_transfer(0x08800000,0x0E200400,0x08801000);
        r.stop("No recompiled function registered at 0x08801000");
        CHECK(p3p3ds::blocker_type(r,k)=="missing_guest_function");
        r.last_transfer.thread=3;CHECK(p3p3ds::blocker_type(r,k)=="unproven_transfer");
        r.stop("replacement");CHECK(r.stop_reason().starts_with("No recompiled"));
        CHECK(p3p3ds::json_string("a\n\"\\")=="\"a\\u000a\\\"\\\\\"");
    }
    for(unsigned op:{4u,5u,6u,0xEAu}) {
        Runtime r;p3p3ds::hle::GeManager g;r.frontier_diagnostics=true;
        r.memory().store32(0x08800000,(op<<24)|3);
        auto id=g.enqueue_list(r,0x08800000,0,-1,0);
        CHECK(r.stopped());CHECK(g.writer_observed());CHECK(g.writer_pc()==0x08800000);
        CHECK(g.find(id)->commands==0);CHECK(r.memory().vram_writes().operations==0);
        bool found=false;for(auto &e:r.events)found|=e.type=="first_graphics_writer" && e.fields.at("opcode")==op;
        CHECK(found);
    }
    {
        GuestMemory m;unsigned count=0;
        m.vram_write_observer=[&](auto a,auto size){++count;CHECK(a==0x04000000);CHECK(size==4);CHECK(m.load32(a)==0x12345678);throw FrontierHalt{};};
        try {m.aot_store32(0x44000000,0x12345678);CHECK(false);}catch(const FrontierHalt &){}
        CHECK(count==1);CHECK(m.vram_writes().operations==1);
        m.vram_write_observer=[&](auto,auto){++count;};
        m.store16(0x04000004,0);m.store8(0x04000006,0);m.zero(0x04000010,4);
        const std::uint8_t bytes[]={1,2};m.copy_in(0x04000020,bytes);
        CHECK(count==5);CHECK(m.vram_writes().operations==5);
    }
    std::cout<<"frontier checks failures="<<failures<<"\n";return failures?1:0;
}
