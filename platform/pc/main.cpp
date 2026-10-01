#include "psprecomp/common.hpp"
#include "psprecomp/elf32.hpp"
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/interpreter.hpp"
#include "p3p3ds/vfs.hpp"
#include "telemetry.hpp"
#include "p3p3ds/vram_activity.hpp"
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <tuple>
#include <memory>
#include <vector>
namespace psprecomp {void register_generated_functions(Runtime &); void apply_generated_patches(GuestMemory &);}
int main(int argc,char **argv) {
    try {
        std::filesystem::path elf_path="profiles/p3p/game/eboot.elf", events_path, umd_path, io_trace_path,
            ms0_path="out/ms0", mods_path;
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
            else if(a=="--umd") umd_path=value();
            else if(a=="--io-trace") io_trace_path=value();
            else if(a=="--ms0") ms0_path=value();
            else if(a=="--mods") mods_path=value();
            else if(a=="--verbose" || a=="-v") {}
            else if(a=="--help" || a=="-h") {
                std::cout<<"--verify-bootstrap (stable checkpoint; --verify-milestone alias)\n"
                         <<"--run-until-blocker --expect-frontier <address> --dump-events <json>\n"
                         <<"--elf <path> --max-dispatches <count> --stop-on-any-vram-write\n"
                         <<"--no-interpreter (stop at unregistered PCs instead of interpreting)\n"
                         <<"--umd <iso> (disc0: image; default: the single *.iso in the working directory)\n";return 0;
            } else throw std::runtime_error("unknown option: "+a);
        }
        const auto elf=psprecomp::Elf32Image::from_file(elf_path);
        const auto entry=elf.runtime_entry();
        psprecomp::Runtime rt;
        const auto reloc=elf.load_and_relocate(rt.memory());
        // Profile CWCheat patches baked into the generated AOT (see
        // profiles/p3p/config/cwcheat_patches.txt) must also patch the image.
        psprecomp::apply_generated_patches(rt.memory());
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
        if(umd_path.empty()) {
            std::vector<std::filesystem::path> images;
            for(const auto &entry:std::filesystem::directory_iterator("."))
                if(entry.is_regular_file() && entry.path().extension()==".iso") images.push_back(entry.path());
            if(images.size()==1) umd_path=images.front();
        }
        if(!umd_path.empty()) {
            auto umd=std::make_shared<p3p3ds::vfs::IsoFileSystem>(umd_path);
            kernel.io().mount("disc0:",umd);
            std::cout<<"UMD image: "<<umd_path.filename().string()<<" files="<<umd->file_count()<<"\n";
        } else std::cout<<"UMD image: none (disc0: unmounted)\n";
        kernel.io().trace_reads=!io_trace_path.empty();
        {
            // ms0: memory stick root; the community Mod Support patch reads
            // ms0:/PSP/P3P/{bind/,mod.cpk,mod1-3.cpk}, mapped onto --mods.
            std::filesystem::create_directories(ms0_path);
            kernel.io().mount("ms0:",std::make_shared<p3p3ds::vfs::HostFileSystem>(ms0_path),true);
            if(!mods_path.empty()) {
                kernel.io().alias("ms0:/PSP/P3P",std::make_shared<p3p3ds::vfs::HostFileSystem>(mods_path));
                std::cout<<"Mods: ms0:/PSP/P3P -> "<<mods_path.string()<<"\n";
            }
        }
        kernel.threads().init_root_thread("root",entry,sp,module->gp);
        {
            std::set<std::uint32_t> stubs;
            std::vector<std::tuple<std::string,std::uint32_t,std::uint32_t>> imports;
            for(const auto &import:elf.scan_imports(rt.memory(),*module)) {
                stubs.insert(import.stub_address);
                imports.emplace_back(import.library,import.nid,import.stub_address);
            }
            kernel.modules().set_host_imports(std::move(imports));
            kernel.threads().set_import_stubs(std::move(stubs));
        }
        p3p3ds::hle::register_all_hle_modules(rt,kernel);
        // Unregistered PCs (indirect targets, interior labels) are interpreted;
        // the first entry at each PC is logged as an interpreter_enter event.
        p3p3ds::Interpreter interpreter;
        if(interpreter_enabled) p3p3ds::install_interpreter_fallback(&interpreter);
        rt.frontier_diagnostics=true;
        // Continuous runs: keep the first occurrences of high-frequency events
        // (all are counted and reported in the event dump) and a larger budget.
        rt.event_budget=400000;
        for(const char *type:{"guest_enter","guest_transfer","hle_hit"}) rt.event_type_limits[type]=20000;
        for(const char *type:{"thread_wait","thread_wake","thread_preempt","io_read_async","io_open","io_getstat",
                              "callback_run","callback_return","interpreter_enter"}) rt.event_type_limits[type]=4000;
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
        if(!io_trace_path.empty()) {
            std::ofstream trace(io_trace_path);
            trace<<"path,offset,size,fnv1a64,time_us\n";
            for(const auto &r:kernel.io().read_log)
                trace<<r.path<<','<<r.offset<<','<<r.size<<','<<psprecomp::hex32(static_cast<std::uint32_t>(r.fnv1a>>32))
                     <<psprecomp::hex32(static_cast<std::uint32_t>(r.fnv1a)).substr(2)<<','<<r.time<<'\n';
        }
        if(expected && (type!="missing_guest_function" || rt.cpu().pc!=*expected))return 4;
        if(!checkpoint.passed)return 3;
        return 0;
    } catch(const std::exception &e) {std::cerr<<"Fatal: "<<e.what()<<"\n";return 1;}
}
