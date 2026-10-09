#include "psprecomp/common.hpp"
#include "psprecomp/elf32.hpp"
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/interpreter.hpp"
#include "p3p3ds/vfs.hpp"
#include "telemetry.hpp"
#include "frame_dump.hpp"
#include "pcm_mixer.hpp"
#include "host_input.hpp"
#include "p3p3ds/profile.hpp"
#include "sampler.hpp"

#include <algorithm>
#include <chrono>
#include "p3p3ds/vram_activity.hpp"
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <cstdio>
#include <tuple>
#include <memory>
#include <vector>
namespace psprecomp {void register_generated_functions(Runtime &); void apply_generated_patches(GuestMemory &);}
int main(int argc,char **argv) {
    try {
        std::filesystem::path elf_path="profiles/p3p/game/eboot.elf", events_path, umd_path, io_trace_path,
            ms0_path="out/ms0", mods_path, frames_dir, wav_path, input_path, sample_path, draw_log_path;
        std::string savedata_policy="latest";
        std::uint64_t frame_every=30, render_from=0, sample_from=0, draw_log_from=0, stop_vblank=0;
        std::uint64_t budget=100000;
        bool verify=false, chase=false, stop_any_vram=false, interpreter_enabled=true, gamepad=false, profile=false;
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
            else if(a=="--frames-dir") frames_dir=value();
            else if(a=="--wav") wav_path=value();
            else if(a=="--input") input_path=value();
            else if(a=="--gamepad") gamepad=true;
            else if(a=="--profile") profile=true;
            else if(a=="--sample") sample_path=value();
            else if(a=="--sample-from") sample_from=std::stoull(value());
            else if(a=="--stop-vblank") stop_vblank=std::stoull(value());
            else if(a=="--draw-log") draw_log_path=value();
            else if(a=="--draw-log-from") draw_log_from=std::stoull(value());
            else if(a=="--savedata") savedata_policy=value();
            else if(a=="--frame-every") frame_every=std::stoull(value());
            else if(a=="--render-from") render_from=std::stoull(value()); // debug fast-forward: no pixels before that vblank
            else if(a=="--verbose" || a=="-v") {}
            else if(a=="--help" || a=="-h") {
                std::cout<<"--verify-bootstrap (stable checkpoint; --verify-milestone alias)\n"
                         <<"--run-until-blocker --expect-frontier <address> --dump-events <json>\n"
                         <<"--elf <path> --max-dispatches <count> --stop-on-any-vram-write\n"
                         <<"--no-interpreter (stop at unregistered PCs instead of interpreting)\n"
                         <<"--umd <iso> (disc0: image; default: the single *.iso in the working directory)\n"
                         <<"--frames-dir <dir> [--frame-every N] (write every Nth displayed frame as BMP)\n"
                         <<"--wav <file> (mix all sceAudio output on the virtual clock into a 44.1 kHz stereo WAV)\n"
                         <<"--input <file> (vblank-keyed button script, see core/include/p3p3ds/input.hpp)\n"
                         <<"--gamepad (poll XInput pad 0 live; not paced to wall time)\n"
                         <<"--profile (wall time of HLE, interpreter and GE rendering; rest is AOT + dispatch)\n"
                         <<"--sample <file> (statistical profile: instruction pointer every ~1 ms; tools/profile_symbols.py)\n"
                         <<"--sample-from <vblank> (start sampling at that vblank)\n"
                         <<"--stop-vblank <vblank> (end the run at that vblank, e.g. the end of a sampled scene)\n"
                         <<"--draw-log <file> --draw-log-from <frame> (one line per GE draw of the 2 displayed frames after that frame index)\n"
                         <<"--savedata latest|cancel|<slot index> (choice in the save/load list dialogs; default latest)\n";return 0;
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
            // Saves: ms0:/PSP/SAVEDATA. The list dialogs have no window here, so
            // --savedata decides: "latest" loads the newest save and saves over the
            // newest one (or the first slot), "cancel" closes the dialog, N picks slot N.
            kernel.savedata().root=ms0_path/"PSP"/"SAVEDATA";
            kernel.savedata().auto_choice=[savedata_policy](const p3p3ds::hle::SavedataDialog &d) {
                if(savedata_policy=="cancel") return -1;
                if(savedata_policy!="latest") return std::stoi(savedata_policy);
                int best=-1;
                for(int i=0;i<static_cast<int>(d.slots.size());++i)
                    if(d.slots[i].exists && (best<0 || d.slots[i].modified>d.slots[best].modified)) best=i;
                return best>=0 ? best : (d.kind==p3p3ds::hle::SavedataDialog::Kind::Save ? 0 : -1);
            };
            if(!mods_path.empty()) {
                kernel.io().alias("ms0:/PSP/P3P",std::make_shared<p3p3ds::vfs::HostFileSystem>(mods_path,true)); // read-only: cached lookups
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
#if !defined(PSPRECOMP_NO_FRONTIER_DIAGNOSTICS) // production runtime (P3P_PRODUCTION_RUNTIME): no events
        rt.frontier_diagnostics=true;
#endif
        // Continuous runs: keep the first occurrences of high-frequency events
        // (all are counted and reported in the event dump) and a larger budget.
        rt.event_budget=400000;
        for(const char *type:{"guest_enter","guest_transfer","hle_hit"}) rt.event_type_limits[type]=20000;
        for(const char *type:{"thread_wait","thread_wake","thread_preempt","io_read_async","io_open","io_getstat",
                              "callback_run","callback_return","interpreter_enter","cpu_vram_write","ge_enqueue",
                              "ge_finish","ge_stall_update","ge_signal","ge_callback","ge_callback_complete"}) rt.event_type_limits[type]=4000;
        p3p3ds::BootstrapCheckpoint checkpoint;
        std::uint64_t frames_shown=0;
        p3p3ds::PcmMixer mixer(wav_path.string());
        {
            auto combined=std::make_shared<p3p3ds::pc::CombinedSource>();
            if(!input_path.empty()) {
                std::ifstream in(input_path);
                if(!in) throw std::runtime_error("cannot open input script "+input_path.string());
                const std::string text((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
                auto script=std::make_shared<p3p3ds::input::InputScript>(p3p3ds::input::InputScript::parse(text));
                std::cout<<"Input script: "<<input_path.string()<<" ("<<script->event_count()<<" events)\n";
                combined->add(script);
            }
#ifdef _WIN32
            if(gamepad) {
                auto pad=std::make_shared<p3p3ds::pc::XInputSource>();
                std::cout<<"Gamepad (XInput): "<<(pad->available()?"available":"xinput1_4.dll not found")<<"\n";
                combined->add(pad);
            }
#endif
            if(!combined->empty()) kernel.input().source=combined;
        }
        kernel.audio().set_sink([&mixer](unsigned,std::uint64_t start,const std::vector<std::int16_t> &stereo) {mixer.add(start,stereo);});
        // P3P_TRACE_FROM_US=<guest us>: restart the hle_hit event budget at that
        // time, so a window of HLE calls late in a run is kept (timing analysis).
        const char *trace_from=std::getenv("P3P_TRACE_FROM_US");
        bool trace_reset=false;
        kernel.ge().skip_rasterization=render_from>0;
        rt.event_observer=[&] {
            if(kernel.ge().skip_rasterization && kernel.threads().vblank_count()>=render_from) kernel.ge().skip_rasterization=false;
            if(trace_from && !trace_reset && kernel.threads().now()>=std::stoull(trace_from)) {
                rt.event_type_counts["hle_hit"]=0; trace_reset=true;
                rt.event("trace_window_start",{{"time_us",kernel.threads().now()}});
            }
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
            // Frame capture (after the checkpoint, which inspects events.back()): every displayed framebuffer is hashed (event
            // "frame"); every --frame-every-th one is written as BMP.
            if(!rt.events.empty() && rt.events.back().type=="display_framebuffer") {
                const auto f=rt.events.back().fields;
                const auto rgb=p3p3ds::framebuffer_rgb(rt.memory(),static_cast<std::uint32_t>(f.at("address")),
                    static_cast<std::uint32_t>(f.at("stride")),static_cast<std::uint32_t>(f.at("format")));
                const auto index=frames_shown++;
                // --draw-log: the draws that build displayed frames index+1 and index+2.
                if(!draw_log_path.empty()) {
                    auto &log=kernel.ge().draw_log;
                    if(index==draw_log_from) {
                        log=std::fopen(draw_log_path.string().c_str(),"w");
                        if(log) std::fprintf(log,"# after displayed frame %llu\n",static_cast<unsigned long long>(index));
                    } else if(log && index==draw_log_from+1) std::fprintf(log,"# displayed frame %llu\n",static_cast<unsigned long long>(index));
                    else if(log && index>=draw_log_from+2) { std::fclose(log); log=nullptr; }
                }
                std::uint64_t nonblack=0; for(std::size_t i=0;i<rgb.size();i+=3) nonblack+=(rgb[i]|rgb[i+1]|rgb[i+2])!=0;
                if(!frames_dir.empty() && index%frame_every==0) {
                    std::filesystem::create_directories(frames_dir);
                    char name[32]; std::snprintf(name,sizeof name,"frame_%05llu.bmp",static_cast<unsigned long long>(index));
                    p3p3ds::write_bmp(std::filesystem::path(frames_dir)/name,rgb);
                }
                rt.event("frame",{{"index",index},{"fnv1a",p3p3ds::fnv1a(rgb)},{"nonblack_pixels",nonblack},
                    {"time_us",kernel.threads().now()}});
            }
        };
        psprecomp::set_runtime_pre_dispatch_hook([](auto &r,auto &,auto pc,auto) {r.diagnostic_pc=pc;r.event("guest_enter",{{"target",pc}});});
        psprecomp::set_runtime_pre_chained_call_hook([](auto &r,auto &,auto pc,auto) {r.diagnostic_pc=pc;r.event("guest_enter",{{"target",pc}});});
        p3p3ds::VramActivity activity;
        rt.memory().vram_write_observer=[&](std::uint32_t address,std::size_t bytes) {activity.observe(rt,kernel,address,bytes,stop_any_vram);};
        std::cout<<"P3P3DS bootstrap: entry="<<psprecomp::hex32(entry)<<" relocations="<<reloc.total
                 <<" registered_entries="<<rt.function_count()<<"\n";
        psprecomp::runtime_profile().enabled=profile;
        psprecomp::runtime_profile().per_import=profile;
        p3p3ds::host_profile().enabled=profile;
        auto run_start=std::chrono::steady_clock::now();
        // Host time of the sampled window (from --sample-from to the end of the run).
        auto window_start=run_start; std::uint64_t window_vblank=0;
        try {
            p3p3ds::pc::Sampler sampler(sample_path.string(),sample_from==0);
            // Per guest vblank (also without diagnostic events): --render-from and --sample-from.
            kernel.display().on_vblank=[&](const p3p3ds::hle::DisplayFramebufInfo &) {
                const auto vblank=kernel.threads().vblank_count();
                if(kernel.ge().skip_rasterization && vblank>=render_from) kernel.ge().skip_rasterization=false;
                if(sample_from && vblank>=sample_from && !sampler.active) {
                    sampler.active=true; window_start=std::chrono::steady_clock::now(); window_vblank=vblank;
                    if(profile) { // --profile then covers the sampled window only
                        auto &h=psprecomp::runtime_profile(); h.hle_ns=h.hle_calls=0; h.imports.clear();
                        auto &p=p3p3ds::host_profile(); p.render_ns=p.render_calls=p.interpreter_ns=p.interpreter_entries=p.io_ns=p.io_calls=p.ge_commands=0;
                        run_start=window_start;
                    }
                }
                if(stop_vblank && vblank>=stop_vblank) {
                    rt.stop("Stop vblank "+std::to_string(stop_vblank)+" reached");
                    throw psprecomp::FrontierHalt{};
                }
            };
            rt.run(entry,budget);
        }
        catch(const psprecomp::FrontierHalt &) {}
        catch(const psprecomp::Error &e) {
            const std::string reason=e.what();
            rt.stop((reason.find("memory")!=std::string::npos?"Memory fault: ":"Runtime exception: ")+reason);
        }
        rt.event_observer={};
        if(!sample_path.empty()) {
            const double wall=std::chrono::duration<double>(std::chrono::steady_clock::now()-window_start).count();
            const auto vblanks=kernel.threads().vblank_count()-window_vblank;
            std::printf("[SAMPLE WINDOW] vblank %llu..%llu: %.2f s host, %.1f vblanks/s\n",static_cast<unsigned long long>(window_vblank),
                static_cast<unsigned long long>(kernel.threads().vblank_count()),wall,wall>0?vblanks/wall:0.0);
        }
        if(profile) {
            const auto wall=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-run_start).count());
            const auto &h=psprecomp::runtime_profile(); const auto &p=p3p3ds::host_profile();
            // HLE time includes GE rendering triggered inside list submission and the
            // post-call scheduler hook; interpreter time includes HLE it calls.
            auto pct=[&](std::uint64_t ns){return wall?100.0*static_cast<double>(ns)/static_cast<double>(wall):0.0;};
            std::printf("[PROFILE] wall=%.2fs hle=%.2fs (%.1f%%, %llu calls) render=%.2fs (%.1f%%, %llu draws/transfers) interpreter=%.3fs (%.2f%%, %llu entries)\n",
                wall/1e9,h.hle_ns/1e9,pct(h.hle_ns),static_cast<unsigned long long>(h.hle_calls),p.render_ns/1e9,pct(p.render_ns),
                static_cast<unsigned long long>(p.render_calls),p.interpreter_ns/1e9,pct(p.interpreter_ns),static_cast<unsigned long long>(p.interpreter_entries));
            std::printf("[PROFILE] ge commands=%llu\n",static_cast<unsigned long long>(p.ge_commands));
            // The HLE imports with the most time (inclusive: GE rendering, waits and file reads they start).
            std::vector<const psprecomp::RuntimeProfile::Import*> top;
            for(const auto &[key,import]:h.imports) top.push_back(&import);
            std::sort(top.begin(),top.end(),[](auto *x,auto *y){return x->ns>y->ns;});
            for(std::size_t i=0;i<top.size() && i<12;++i) {
                const auto &im=*top[i];
                std::printf("[PROFILE] import %s::%s %.3fs (%.1f%%) %llu calls %.2f us/call\n",im.library.c_str(),
                    rt.nids().resolve(im.library,im.nid).value_or(psprecomp::hex32(im.nid)).c_str(),im.ns/1e9,pct(im.ns),
                    static_cast<unsigned long long>(im.calls),im.calls?im.ns/1e3/static_cast<double>(im.calls):0.0);
            }
        }
        if(std::getenv("PSPRECOMP_HLE_HISTOGRAM")) rt.report_hle_histogram(80); // per-NID call counts
        p3p3ds::install_interpreter_fallback(nullptr);
        rt.event("interpreter_summary",{{"entries",interpreter.entries()},{"distinct_pcs",interpreter.entry_pcs().size()},
            {"instructions",interpreter.executed()}});
        mixer.finish();
        rt.event("audio_summary",{{"buffers",mixer.buffers()},{"frames",mixer.frames()},{"nonzero_frames",mixer.nonzero_frames()},
            {"peak",mixer.peak()},{"late_frames",mixer.late_frames()},{"clipped",mixer.clipped()}});
        std::cout<<"[AUDIO] buffers="<<mixer.buffers()<<" frames="<<mixer.frames()<<" nonzero="<<mixer.nonzero_frames()
                 <<" peak="<<mixer.peak()<<" late="<<mixer.late_frames()<<" clipped="<<mixer.clipped()<<"\n";
        {
            std::map<std::string,std::uint64_t> ge_features(kernel.ge().feature_counts().begin(),kernel.ge().feature_counts().end());
            rt.event_map("ge_feature_summary",std::move(ge_features));
        }
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
