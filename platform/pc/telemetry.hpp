#pragma once
#include "psprecomp/runtime.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "../../profiles/p3p/config/bootstrap_expectations.hpp"
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
namespace p3p3ds {
inline std::string json_string(const std::string &s) {
    std::ostringstream out; out << '"';
    for(unsigned char c:s) {
        if(c=='"' || c=='\\') out << '\\' << c;
        else if(c<32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << unsigned(c);
        else out << c;
    }
    return out.str()+'"';
}
struct BootstrapCheckpoint {
    bool passed{}, root{}, user{}, created{}, started{}, mode{}, framebuffer{}, callback{}, returned{}, continued{};
    void observe(const psprecomp::Runtime &r, const KernelState &k) {
        if(r.events.empty() || passed) return;
        const auto &e=r.events.back();
        auto field=[&](const char *n) {auto it=e.fields.find(n);return it==e.fields.end()?std::uint64_t(0):it->second;};
        root |= e.type=="guest_enter" && field("target")==profile::entry;
        user |= e.type=="guest_enter" && field("target")==profile::user_main;
        created |= e.type=="thread_create" && field("uid")==2 && field("entry")==profile::user_main && e.detail=="user_main";
        started |= e.type=="thread_start" && field("uid")==2 && field("result")==0;
        mode |= e.type=="display_mode" && field("mode")==0 && field("width")==480 && field("height")==272;
        framebuffer |= e.type=="display_framebuffer" && field("address")==profile::display && field("stride")==512 && field("format")==3 && field("sync")==1;
        callback |= e.type=="ge_callback_complete" && field("list")==2;
        returned |= e.type=="guest_transfer" && e.pc==profile::return_site && field("target")==profile::return_target;
        continued |= returned && e.type=="guest_transfer" && e.pc==profile::return_target && (field("word")>>26)==3;
        const auto *s=k.ge().find(1), *d=k.ge().find(2); const auto &g=k.ge().state();
        passed=root && user && created && started && mode && framebuffer && callback && returned && continued &&
            k.compiled_sdk_version()==profile::sdk && k.compiler_version()==profile::compiler &&
            s && d && s->completions==1 && d->completions==1 && s->commands==212 && d->commands==29 &&
            s->status==hle::GeStatus::Completed && d->status==hle::GeStatus::Completed &&
            g.color_address()==profile::color && g.depth_address()==profile::depth &&
            g.color_stride()==512 && g.depth_stride()==512 && g.format()==3;
    }
};
inline std::string blocker_type(const psprecomp::Runtime &r, const KernelState &k) {
    const auto &s=r.stop_reason();
    if(k.ge().writer_observed()) return "graphics_writer";
    if(s.starts_with("CPU VRAM")) return "cpu_vram_write";
    if(s.starts_with("Missing HLE")) return "missing_hle";
    if(s.starts_with("Unsupported Allegrex")) return "unsupported_instruction";
    if(s.starts_with("Memory fault")) return "memory_fault";
    if(s.starts_with("No recompiled function")) {
        if(k.threads().verified_thread_entry(r.cpu(),psprecomp::runtime_thread_uid()))
            return "missing_guest_function";
        const auto &t=r.last_transfer;
        if(t.target==r.cpu().pc && t.thread==psprecomp::runtime_thread_uid() && (t.word>>26)==3 && r.cpu().gpr[31]==t.pc+8)
            return "missing_guest_function";
        return "unproven_transfer";
    }
    if(s.starts_with("Stable bootstrap")) return "bootstrap_checkpoint";
    if(s.find("budget")!=std::string::npos || !r.stopped()) return "budget_exhausted";
    return "runtime_semantics";
}
inline void dump_events(const std::filesystem::path &path, const psprecomp::Runtime &r,
                        const KernelState &k, const BootstrapCheckpoint &check) {
    std::ofstream o(path); if(!o) throw std::runtime_error("cannot open event output");
    const auto &t=r.last_transfer; const auto &w=r.memory().vram_writes(); const auto &g=k.ge().state();
    const auto type=blocker_type(r,k);
    const auto thread_entry=type=="missing_guest_function" ?
        k.threads().verified_thread_entry(r.cpu(),psprecomp::runtime_thread_uid()) : std::nullopt;
    const auto proof_kind=type=="missing_guest_function" ?
        (thread_entry ? "thread_entry" : "direct_jal") : "none";
    std::uint64_t cpu_color=0,cpu_depth=0;
    for(const auto &e:r.events) if(e.type=="vram_activity_summary") {cpu_color=e.fields.at("color");cpu_depth=e.fields.at("depth");}
    o << "{\n\"schema\":1,\n\"bootstrap_passed\":" << (check.passed?"true":"false")
      << ",\n\"blocker\":{\"type\":" << json_string(type) << ",\"pc\":" << r.cpu().pc
      << ",\"proof_kind\":" << json_string(proof_kind)
      << (thread_entry ? ",\"provenance\":{\"kind\":\"thread_entry\",\"uid\":"+
          std::to_string(thread_entry->uid)+",\"entry\":"+std::to_string(thread_entry->entry_pc)+
          ",\"from_uid\":"+std::to_string(thread_entry->from_uid)+"}" : "")
      << ",\"caller\":" << t.pc << ",\"word\":" << t.word << ",\"target\":" << t.target
      << ",\"thread\":" << psprecomp::runtime_thread_uid() << ",\"ra\":" << r.cpu().gpr[31]
      << ",\"instruction_pc\":" << r.diagnostic_pc
      << ",\"instruction\":" << (r.memory().contains(r.diagnostic_pc,4)?r.memory().load32(r.diagnostic_pc):0)
      << ",\"reason\":" << json_string(r.stop_reason()) << "},\n\"registered_entries\":" << r.function_count()
      << ",\n\"graphics\":{\"writer\":" << (k.ge().writer_observed()?"true":"false")
      << ",\"writer_pc\":" << k.ge().writer_pc() << ",\"vram_operations\":" << w.operations
      << ",\"bytes\":" << w.bytes << ",\"changed\":" << w.changed << ",\"color_writes\":" << w.color+cpu_color
      << ",\"depth_writes\":" << w.depth+cpu_depth << ",\"color\":" << g.color_address() << ",\"depth\":" << g.depth_address()
      << ",\"stride\":" << g.color_stride() << ",\"format\":" << g.format() << ",\"display\":" << k.display().info().topaddr << "},\n\"events\":[\n";
    bool first=true;
    for(const auto &e:r.events) {
        if(!first)o<<",\n";
        first=false;
        o << "{\"type\":" << json_string(e.type) << ",\"pc\":" << e.pc << ",\"thread\":" << e.thread
          << ",\"detail\":" << json_string(e.detail) << ",\"fields\":{";
        bool f=true; for(const auto &[key,value]:e.fields) {if(!f)o<<',';f=false;o<<json_string(key)<<':'<<value;}
        o << "}}";
    }
    o << "\n]}\n"; if(!o) throw std::runtime_error("failed writing events");
}
}
