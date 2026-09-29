#include "p3p3ds/frontier.hpp"
#include "p3p3ds/vram_activity.hpp"
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
        // A completed jump separates an entry whose multiply precedes the
        // ordinary RA-saving frame. A matching JAL alone is insufficient.
        GuestMemory m;
        const std::uint32_t c=0x08800000,a=0x08801000;
        const std::uint32_t jal=0x0C000000u|((a>>2)&0x03FFFFFFu);
        const std::uint32_t jump=0x08000000u|(((c+16)>>2)&0x03FFFFFFu);
        m.store32(c,jal);m.store32(c+4,0);
        m.store32(a-8,jump);m.store32(a-4,0x27BD0010);
        m.store32(a,0x00A40018); // mult a1,a0 before stack allocation
        m.store32(a+4,0x27BDFFF0);m.store32(a+8,0xAFBF000C);
        m.store32(a+12,0x00001012);m.store32(a+16,0x8FBF000C);
        m.store32(a+20,0x03E00008);m.store32(a+24,0x27BD0010);
        std::vector<ExecutableRange> ranges={{c,a+28}};
        std::map<std::uint32_t,std::string> seeds={{c,"caller"}};
        std::set<std::uint32_t> owned={c,c+4};
        auto check=[&](std::uint32_t target) {return p3p3ds::validate_frontier(m,ranges,seeds,owned,{},target,c,jal);};
        CHECK(check(a).accepted);
        m.store32(a-8,0);CHECK(!check(a).accepted); // straight-line interior label
        m.store32(a-8,jump);
        const std::uint32_t branch=(0x04u<<26)|(((a+12-(c+8+4))/4)&0xFFFFu);
        m.store32(c+8,branch);CHECK(!check(a).accepted); // external interior entry
        m.store32(c+8,(0x04u<<26)|(((a+24-(c+8+4))/4)&0xFFFFu));
        CHECK(!check(a).accepted); // external entry into a delay slot
        m.store32(c+8,0);
        owned.insert(a+12);CHECK(!check(a).accepted);owned.erase(a+12);
        CHECK(!check(a+12).accepted); // random interior direct-JAL claim
    }
    {
        GuestMemory m;
        const std::uint32_t a=0x08801000;
        m.store32(a-8,0x03E00008);m.store32(a-4,0);
        m.store32(a,0x27BDFFF0);m.store32(a+4,0xAFBF000C);
        m.store32(a+8,0x8FBF000C);m.store32(a+12,0x03E00008);
        m.store32(a+16,0x27BD0010);
        std::vector<ExecutableRange> ranges={{a-8,a+20}};
        std::map<std::uint32_t,std::string> seeds;
        std::set<std::uint32_t> owned;
        auto proof=[&](std::uint32_t target) {return p3p3ds::validate_frontier(
            m,ranges,seeds,owned,{},target,p3p3ds::FrontierOrigin::thread_entry(5,a));};
        CHECK(proof(a).accepted);
        CHECK(!p3p3ds::validate_frontier(m,ranges,seeds,owned,{},a,
            p3p3ds::FrontierOrigin::thread_entry(0,a)).accepted);
        CHECK(!proof(a+4).accepted);
        CHECK(!p3p3ds::validate_frontier(m,ranges,seeds,owned,{a},a,
            p3p3ds::FrontierOrigin::thread_entry(5,a)).accepted);
        owned.insert(a+8);CHECK(!proof(a).accepted);owned.clear();
        m.store32(a-8,0);m.store32(a,0);CHECK(!proof(a).accepted);
        m.store32(a-8,0x03E00008);m.store32(a,0x27BDFFF0);
        m.store32(a+8,0x0000000D);CHECK(!proof(a).accepted);
    }
    {
        Runtime r; p3p3ds::KernelState k; r.frontier_diagnostics=true;
        const std::uint32_t entry=0x08801000;
        k.threads().init_root_thread("root",0x08800000,0x09FFFF00,0);
        r.cpu()=k.threads().current_thread()->context;
        auto uid=k.threads().create_thread("worker",entry,16,512,0,0,r.memory());
        CHECK(uid>0);
        r.cpu().pc=entry;
        CHECK(!k.threads().verified_thread_entry(r.cpu(),uid)); // Dormant thread.
        r.cpu()=k.threads().current_thread()->context;
        CHECK(k.threads().start_thread(uid,0,0,r.memory(),r.cpu())==0);
        CHECK(k.threads().current_thread_id()==uid && r.cpu().pc==entry);
        CHECK(k.threads().verified_thread_entry(r.cpu(),uid));
        r.stop("No recompiled function registered at 0x08801000");
        CHECK(p3p3ds::blocker_type(r,k)=="missing_guest_function");
        set_runtime_thread_identity(uid+1,"wrong");
        CHECK(p3p3ds::blocker_type(r,k)=="unproven_transfer");
        set_runtime_thread_identity(uid,"worker");
        r.cpu().pc=entry+4;CHECK(p3p3ds::blocker_type(r,k)=="unproven_transfer");
        r.cpu().pc=entry;
        k.threads().note_guest_execution(uid,entry+4);
        CHECK(p3p3ds::blocker_type(r,k)=="unproven_transfer");
        auto other=k.threads().create_thread("other",entry+32,16,512,0,0,r.memory());
        CHECK(k.threads().switch_to(other,r.cpu()));
        CHECK(!k.threads().verified_thread_entry(r.cpu(),other)); // Arbitrary switch.
        Runtime delayed; p3p3ds::KernelState later;delayed.frontier_diagnostics=true;
        later.threads().init_root_thread("root",0x08800000,0x09FFFF00,0);
        delayed.cpu()=later.threads().current_thread()->context;
        auto deferred=later.threads().create_thread("deferred",entry,40,512,0,0,delayed.memory());
        CHECK(later.threads().start_thread(deferred,0,0,delayed.memory(),delayed.cpu())==0);
        CHECK(!later.threads().verified_thread_entry(delayed.cpu(),deferred)); // Still ready.
        CHECK(later.threads().exit_current_thread(0,delayed.cpu(),delayed));
        CHECK(later.threads().verified_thread_entry(delayed.cpu(),deferred));
        CHECK(!delayed.events.empty() && delayed.events.back().type=="thread_entry_transfer");
        later.threads().invalidate_thread_entry(); // Any subsequent guest transfer is stale.
        CHECK(!later.threads().verified_thread_entry(delayed.cpu(),deferred));
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
    {
        using V=p3p3ds::VramActivity;
        CHECK(V::overlaps(0x04400000,4,0x04000000,4));
        CHECK(V::overlaps(0x041FFFFE,4,0x04000000,4));
        CHECK(!V::overlaps(0x04154000,4,0x04110000,512*272*2));
        CHECK(!V::overlaps(0x04000000,4,0,4));
        Runtime r;p3p3ds::KernelState k;V activity;r.frontier_diagnostics=true;
        r.cpu().pc=0x08800000;r.diagnostic_pc=0x08801000;
        for(unsigned i=0;i<40;++i)activity.observe(r,k,0x04154004,4);
        CHECK(!r.stopped());CHECK(r.cpu().pc==0x08800000);
        CHECK(activity.resource==40);CHECK(activity.suppressed==8);CHECK(r.events.size()==32);
        // Pending display target is not active until vblank.
        k.display().set_framebuf(0x04088000,512,3,1);
        activity.observe(r,k,0x04088000,4);CHECK(!r.stopped());
        k.display().advance_vblank();
        try {activity.observe(r,k,0x04088000,4);CHECK(false);}catch(const FrontierHalt &){}
        CHECK(r.stopped());CHECK(r.cpu().pc==0x08801000);CHECK(activity.color==1);
    }
    {
        Runtime r;p3p3ds::KernelState k;p3p3ds::VramActivity activity;r.frontier_diagnostics=true;
        // GE color, depth and bound direct-color texture state, followed by FINISH/END.
        const std::uint32_t words[]={0x9C000000,0x9D000200,0xD2000003,0x9E110000,0x9F000200,
            0x1E000001,0xA0180000,0xA8040003,0xB8000101,0xC3000003,0x0F000000,0x0C000000};
        for(unsigned i=0;i<std::size(words);++i)r.memory().store32(0x08800000+i*4,words[i]);
        k.ge().enqueue_list(r,0x08800000,0,-1,0);
        activity.observe(r,k,0x04180000,4);CHECK(activity.texture==1);CHECK(!r.stopped());
        activity.observe(r,k,0x0418001F,1);CHECK(activity.texture==2); // minimum stride is four pixels
        activity.observe(r,k,0x04180020,1);CHECK(activity.resource==1);
        activity.observe(r,k,0x04154004,4);CHECK(activity.resource==2);CHECK(!r.stopped());
        try {activity.observe(r,k,0x04110000,4);CHECK(false);}catch(const FrontierHalt &){}
        CHECK(activity.depth==1);CHECK(activity.color==0);CHECK(r.stopped());
    }
    std::cout<<"frontier checks failures="<<failures<<"\n";return failures?1:0;
}
