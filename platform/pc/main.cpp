#include "psprecomp/common.hpp"
#include "psprecomp/elf32.hpp"
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/interpreter.hpp"
#include "telemetry.hpp"
#include "p3p3ds/vram_activity.hpp"
#include <iostream>
#include <optional>
namespace psprecomp {void register_generated_functions(Runtime &);}
int main(int argc,char **argv) {
    try {
        std::filesystem::path elf_path="profiles/p3p/game/eboot.elf", events_path;
        std::uint64_t budget=100000;
        bool verify=false, chase=false, stop_any_vram=false, interpreter_enabled=true;
        std::optional<std::uint32_t> expected;
        for(int i=1;i<argc;++i) {
            const std::string a=argv[i];
            auto value=[&]() -> std::string {if(++i>=argc)throw std::runtime_error("missing value for "+a);return argv[i];};
            if(a=="--elf") elf_path=value();
            else if(a=="--dump-events") events_path=value();
            else if(a=="--max-dispatches") budget=std::stoull(value());
            else if(a=="--expect-frontier") expected=std::stoul(value(),nullptr,0);
            else if(a=="--verify-bootstrap" || a=="--verify-milestone" || a=="--verify") verify=true;
            else if(a=="--run-until-blocker") chase=true;
            else if(a=="--stop-on-any-vram-write") stop_any_vram=true;
            else if(a=="--no-interpreter") interpreter_enabled=false;
            else if(a=="--verbose" || a=="-v") {}
            else if(a=="--help" || a=="-h") {
                std::cout<<"--verify-bootstrap (stable checkpoint; --verify-milestone alias)\n"
                         <<"--run-until-blocker --expect-frontier <address> --dump-events <json>\n"
                         <<"--elf <path> --max-dispatches <count> --stop-on-any-vram-write\n";return 0;
            } else throw std::runtime_error("unknown option: "+a);
        }
        const auto elf=psprecomp::Elf32Image::from_file(elf_path);
        const auto entry=elf.runtime_entry();
        psprecomp::Runtime rt;
        const auto reloc=elf.load_and_relocate(rt.memory());
        const auto module=elf.find_module_info(rt.memory());
        if(!module)throw std::runtime_error("missing module info");
        const auto sp=p3p3ds::profile::stack_top-0x100;
        rt.memory().zero(p3p3ds::profile::stack_top-p3p3ds::profile::stack_size,p3p3ds::profile::stack_size);
        rt.cpu().gpr[28]=module->gp;rt.cpu().gpr[29]=sp;rt.cpu().gpr[26]=sp;
        rt.cpu().gpr[31]=p3p3ds::hle::ThreadManager::kThreadReturnSentinel;
        psprecomp::register_generated_functions(rt);
        p3p3ds::KernelState kernel;
        // Explicit PC game-launch environment: medium present. This does not
        // imply implemented UMD activation, drive readiness or filesystem mounting.
        kernel.umd().set_medium_present(true);
        kernel.threads().init_root_thread("root",entry,sp,module->gp);
        p3p3ds::hle::register_all_hle_modules(rt,kernel);
        // Unregistered PCs (indirect targets, interior labels) are interpreted;
        // the first entry at each PC is logged as an interpreter_enter event.
        p3p3ds::Interpreter interpreter;
        if(interpreter_enabled) p3p3ds::install_interpreter_fallback(&interpreter);
        rt.frontier_diagnostics=true;
        p3p3ds::BootstrapCheckpoint checkpoint;
        rt.event_observer=[&] {
            if(!rt.events.empty() && rt.events.back().type=="guest_transfer")
                kernel.threads().invalidate_thread_entry();
            if(!rt.events.empty() && rt.events.back().type=="guest_enter") {
                const auto &e=rt.events.back();
                kernel.threads().note_guest_execution(e.thread,static_cast<std::uint32_t>(e.fields.at("target")));
            }
            const bool previous=checkpoint.passed;
            checkpoint.observe(rt,kernel);
            if(!previous && checkpoint.passed && verify && !chase && !expected) {
                rt.cpu().pc=rt.last_transfer.target;
                rt.stop("Stable bootstrap checkpoint reached");
                throw psprecomp::FrontierHalt{};
            }
        };
        psprecomp::set_runtime_pre_dispatch_hook([](auto &r,auto &,auto pc,auto) {r.diagnostic_pc=pc;r.event("guest_enter",{{"target",pc}});});
        psprecomp::set_runtime_pre_chained_call_hook([](auto &r,auto &,auto pc,auto) {r.diagnostic_pc=pc;r.event("guest_enter",{{"target",pc}});});
        p3p3ds::VramActivity activity;
        rt.memory().vram_write_observer=[&](std::uint32_t address,std::size_t bytes) {activity.observe(rt,kernel,address,bytes,stop_any_vram);};
        std::cout<<"P3P3DS bootstrap: entry="<<psprecomp::hex32(entry)<<" relocations="<<reloc.total
                 <<" registered_entries="<<rt.function_count()<<"\n";
        try {rt.run(entry,budget);}
        catch(const psprecomp::FrontierHalt &) {}
        catch(const psprecomp::Error &e) {
            const std::string reason=e.what();
            rt.stop((reason.find("memory")!=std::string::npos?"Memory fault: ":"Runtime exception: ")+reason);
        }
        rt.event_observer={};
        p3p3ds::install_interpreter_fallback(nullptr);
        rt.event("interpreter_summary",{{"entries",interpreter.entries()},{"distinct_pcs",interpreter.entry_pcs().size()},
            {"instructions",interpreter.executed()}});
        rt.event("vram_activity_summary",{{"unclassified_resource",activity.resource},{"bound_texture_resource",activity.texture},
            {"color",activity.color},{"depth",activity.depth},{"suppressed",activity.suppressed}});
        if(!rt.stopped())rt.stop("Dispatch budget exhausted");
        const auto type=p3p3ds::blocker_type(rt,kernel);
        rt.event("blocker",{{"target",rt.cpu().pc}},rt.stop_reason());
        rt.event("final_blocker",{{"target",rt.cpu().pc}},type);
        std::cout<<"Stop Reason: "<<rt.stop_reason()<<"\nFinal Guest PC: "<<psprecomp::hex32(rt.cpu().pc)
                 <<"\nBlocker type: "<<type<<"\nLast executed transfer: "<<psprecomp::hex32(rt.last_transfer.pc)
                 <<" word="<<psprecomp::hex32(rt.last_transfer.word)<<" target="<<psprecomp::hex32(rt.last_transfer.target)
                 <<"\nStable bootstrap: "<<(checkpoint.passed?"PASS":"FAIL")
                 <<"\n[INTERPRETER] entries="<<interpreter.entries()<<" distinct_pcs="<<interpreter.entry_pcs().size()
                 <<" instructions="<<interpreter.executed()<<"\n";
        kernel.ge().report();
        const auto &w=rt.memory().vram_writes();std::size_t nonzero=0;
        for(auto b:rt.memory().vram_bytes()) nonzero+=b!=0;
        std::cout<<"[VRAM WRITES] operations="<<w.operations<<" bytes="<<w.bytes<<" changed="<<w.changed
                 <<" color="<<w.color<<" depth="<<w.depth<<" nonzero="<<nonzero<<"\n";
        std::cout<<"[CPU VRAM ACTIVITY] resource="<<activity.resource<<" texture="<<activity.texture
                 <<" color="<<activity.color<<" depth="<<activity.depth<<" suppressed="<<activity.suppressed<<"\n";
        if(!events_path.empty())p3p3ds::dump_events(events_path,rt,kernel,checkpoint);
        if(expected && (type!="missing_guest_function" || rt.cpu().pc!=*expected))return 4;
        if(!checkpoint.passed)return 3;
        return 0;
    } catch(const std::exception &e) {std::cerr<<"Fatal: "<<e.what()<<"\n";return 1;}
}
