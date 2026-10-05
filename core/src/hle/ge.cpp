#include "p3p3ds/hle/ge.hpp"
#include "p3p3ds/ge/geometry.hpp"
#include "p3p3ds/profile.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"
#include "psprecomp/common.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
namespace p3p3ds::hle {
namespace {
constexpr std::int32_t invalid_id=static_cast<std::int32_t>(0x80000100u);
constexpr std::int32_t invalid_mode=static_cast<std::int32_t>(0x80000107u);
constexpr std::int32_t invalid_pointer=static_cast<std::int32_t>(0x80000023u);
constexpr std::int32_t invalid_size=static_cast<std::int32_t>(0x80000104u);
constexpr std::int32_t already=static_cast<std::int32_t>(0x80000020u);
}
const GeListInfo *GeManager::find(int id) const { auto i=lists_.find(id); return i==lists_.end()?nullptr:&i->second; }
std::int32_t GeManager::set_callback(psprecomp::GuestMemory &mem, std::uint32_t ptr) {
    if ((ptr&3) || !mem.contains(ptr,16)) return invalid_pointer;
    const int id=next_callback_++;
    callbacks_[id]={mem.load32(ptr),mem.load32(ptr+4),mem.load32(ptr+8),mem.load32(ptr+12)};
    std::cout << "[GE CALLBACK] id=" << id << " signal=" << psprecomp::hex32(callbacks_[id].signal)
              << " signal_arg=" << psprecomp::hex32(callbacks_[id].signal_arg)
              << " finish=" << psprecomp::hex32(callbacks_[id].finish)
              << " finish_arg=" << psprecomp::hex32(callbacks_[id].finish_arg) << "\n";
    return id;
}
std::int32_t GeManager::unset_callback(int id) { return callbacks_.erase(id)?0:invalid_id; }
std::int32_t GeManager::enqueue_list(psprecomp::Runtime &rt, std::uint32_t start, std::uint32_t stall, int cb, std::uint32_t opt, bool head) {
    if (((start|stall)&3) || !rt.memory().contains(start,4)) return invalid_pointer;
    if (cb>=0 && !callbacks_.contains(cb)) return invalid_id;
    std::uint32_t context=0, stack_count=0, stack=0;
    if (opt) {
        if ((opt&3) || !rt.memory().contains(opt,4)) return invalid_pointer;
        const auto size=rt.memory().load32(opt);
        if (size>=8) {
            if (!rt.memory().contains(opt,8)) return invalid_pointer;
            context=rt.memory().load32(opt+4);
            if (context && ((context&3) || !rt.memory().contains(context,2048))) return invalid_pointer;
        }
        if (size>=16) {
            if (!rt.memory().contains(opt,16)) return invalid_pointer;
            stack_count=rt.memory().load32(opt+8); stack=rt.memory().load32(opt+12);
            if (stack_count>=256) return invalid_size;
            if (stack_count && ((stack&3) || !rt.memory().contains(stack,stack_count*32u))) return invalid_pointer;
        }
    }
    // Drop the oldest completed lists (none is referenced here).
    while (lists_.size() >= kKeptLists && lists_.begin()->second.status==GeStatus::Completed) lists_.erase(lists_.begin());
    const int id=next_id_++;
    ++enqueued_;
    GeListInfo list; list.id=id; list.list_address=start; list.pc=start&0x0FFFFFFF;
    list.stall_address=stall&0x0FFFFFFF; list.callback_id=cb; list.opt_param=opt;
    list.context_address=context; list.stack_count=stack_count; list.stack_address=stack;
    lists_.emplace(id,list); head ? queue_.push_front(id) : queue_.push_back(id);
    rt.event("ge_enqueue", {{"list",static_cast<unsigned>(id)}, {"start",start}, {"stall",stall}, {"callback",static_cast<std::uint32_t>(cb)}});
    if (rt.frontier_diagnostics) std::cout << "[GE ENQUEUE] id=" << id << " start=" << psprecomp::hex32(start)
              << " stall=" << psprecomp::hex32(stall) << " cb=" << cb
              << " context=" << psprecomp::hex32(context) << " stacks=" << stack_count
              << " stack=" << psprecomp::hex32(stack) << (head?" head":"") << "\n";
    pump(rt); return id;
}
std::int32_t GeManager::update_stall(psprecomp::Runtime &rt, int id, std::uint32_t stall) {
    auto i=lists_.find(id); if(i==lists_.end()) return invalid_id;
    if(i->second.status==GeStatus::Completed) return already;
    if(stall&3) return invalid_pointer;
    i->second.stall_address=stall&0x0FFFFFFF;
    rt.event("ge_stall_update", {{"list",static_cast<unsigned>(id)}, {"stall",stall}});
    pump(rt); return 0;
}
std::int32_t GeManager::sync(psprecomp::Runtime &rt, int id, int mode, bool all) {
    if(mode!=0 && mode!=1) return invalid_mode;
    if(!all && !find(id)) return invalid_id;
    pump(rt);
    const auto status=all ? (queue_.empty()?GeStatus::Completed:lists_.at(queue_.front()).status) : find(id)->status;
    if(mode==0 && status!=GeStatus::Completed) {
        // DIRTY_FIRST_FRAME: no scheduler GE wait integration yet; never fake completion.
        rt.stop("GE wait blocked by uncompleted list");
    }
    return static_cast<std::int32_t>(status);
}
void GeManager::deliver_finish_callback(psprecomp::Runtime &rt, const GeListInfo &l, std::uint32_t end_pc) {
    if (l.callback_id<0) return;
    const auto &callback=callbacks_.at(l.callback_id);
    if (!callback.finish) return;
    rt.event("ge_callback", {{"list",l.id}, {"target",callback.finish}, {"end_pc",end_pc}});
    // DIRTY_FIRST_FRAME: model the kernel interrupt stack with a private guest
    // scratch window. Reusing the interrupted user SP corrupts live frames.
    constexpr std::uint32_t callback_stack_base=0x09FFE000u;
    constexpr std::uint32_t callback_stack_size=0x1000u;
    std::vector<std::uint8_t> saved_stack(callback_stack_size);
    rt.memory().copy_out(callback_stack_base,saved_stack);
    psprecomp::AllegrexContext callback_ctx=rt.cpu();
    callback_ctx.gpr[29]=callback_stack_base+callback_stack_size;
    callback_ctx.gpr[30]=callback_ctx.gpr[29];
    callback_ctx.gpr[4]=l.finish_token;
    callback_ctx.gpr[5]=callback.finish_arg;
    callback_ctx.gpr[6]=end_pc;
    callback_ctx.gpr[31]=0x20;
    callback_ctx.pc=callback.finish;
    // Interrupt-context handler: it cannot block, so it runs to completion in
    // isolation. The cap only guards against a runaway handler.
    unsigned budget=1000000;
    while (callback_ctx.pc!=0x20 && !rt.stopped() && budget--) {
        const auto pc=callback_ctx.pc;
        if (!rt.invoke_isolated_aot(pc,callback_ctx)) {
            std::cout << "[GE CALLBACK MISSING] id=" << l.callback_id << " pc=" << psprecomp::hex32(pc) << "\n";
            rt.stop("GE finish callback target is not recompiled");
            rt.memory().copy_in(callback_stack_base,saved_stack);
            return;
        }
    }
    rt.memory().copy_in(callback_stack_base,saved_stack);
    if (!rt.stopped() && callback_ctx.pc!=0x20) rt.stop("GE finish callback dispatch budget exceeded");
    if (!rt.stopped()) rt.event("ge_callback_complete", {{"list",l.id}, {"target",callback.finish}});
    if (!rt.stopped() && rt.frontier_diagnostics) std::cout << "[GE CALLBACK COMPLETE] id=" << l.callback_id
                                 << " token=" << l.finish_token << " end=" << psprecomp::hex32(end_pc) << "\n";
}
// Debug draw log: vertex type, texture, fragment state and the screen bounds
// of the transformed vertices (registers as in count_features).
void GeManager::log_draw(psprecomp::GuestMemory &mem, std::uint32_t prim_type, std::uint32_t count) {
    const auto &r=regs_.reg;
    std::vector<ge::ScreenVertex> v;
    ge::decode_screen_vertices(mem,regs_,count,state_.vertex,state_.index,v);
    float x0=1e9f,y0=1e9f,x1=-1e9f,y1=-1e9f; unsigned clipped=0;
    for(const auto &s:v) {
        if(s.clipped) { ++clipped; continue; }
        x0=std::min(x0,s.x); y0=std::min(y0,s.y); x1=std::max(x1,s.x); y1=std::max(y1,s.y);
    }
    const auto tex=(r[0xA0]&0xFFFFF0u)|((r[0xA8]>>16)&0xFFu)<<24;
    std::fprintf(draw_log,"prim=%u n=%u vtype=%06X fb=%06X tex=%s%08X %ux%u fmt=%u clut=%06X tfunc=%06X blend=%u:%06X atest=%u:%06X "
        "ztest=%u:%u zmask=%u light=%u fog=%u cull=%u:%u bbox=%.0f,%.0f-%.0f,%.0f clipped=%u\n",
        prim_type,count,r[0x12]&0xFFFFFFu,r[0x9C]&0xFFFFFFu,(r[0x1E]&1u)?"":"off:",tex,1u<<(r[0xB8]&0xFu),1u<<((r[0xB8]>>8)&0xFu),
        r[0xC3]&0xFu,r[0xC5]&0xFFFFFFu,r[0xC9]&0xFFFFFFu,r[0x21]&1u,r[0xDF]&0xFFFFFFu,r[0x22]&1u,r[0xDB]&0xFFFFFFu,
        r[0x23]&1u,r[0xDE]&7u,r[0xE7]&0xFFFFFFu,r[0x17]&1u,r[0x1F]&1u,r[0x1D]&1u,r[0x9B]&1u,x0,y0,x1,y1,clipped);
    const auto layout=ge::vertex_layout(r[0x12]);
    if(layout.weights==0u || layout.through) return;
    // Skinned draws: the bone matrices, then per vertex the weights and the screen position.
    for(std::uint32_t b=0;b<layout.weights;++b) {
        std::fprintf(draw_log,"  bone%u",b);
        for(std::uint32_t k=0;k<12u;++k) std::fprintf(draw_log," %.4f",regs_.bone[b*12u+k]);
        std::fprintf(draw_log,"\n");
    }
    for(std::uint32_t i=0;i<count;++i) {
        ge::ModelVertex m;
        if(!ge::decode_model_vertex(mem,regs_,layout,state_.vertex,ge::vertex_index(mem,layout,state_.index,i),m)) continue;
        float sum=0.0f;
        std::fprintf(draw_log,"  v%u w=",i);
        for(std::uint32_t k=0;k<layout.weights;++k) { std::fprintf(draw_log,"%.3f ",m.weights[k]); sum+=m.weights[k]; }
        std::fprintf(draw_log,"sum=%.3f pos=%.4f,%.4f,%.4f screen=%.0f,%.0f%s\n",sum,m.pos[0],m.pos[1],m.pos[2],v[i].x,v[i].y,v[i].clipped?" clipped":"");
    }
}

// Enable bits and modes: PSPSDK pspge.h / pspgu.h command list.
void GeManager::count_features(std::uint32_t prim_type) {
    const auto &r=regs_.reg;
    auto on=[&](std::uint32_t i){ return (r[i]&1u)!=0u; };
    const auto layout=ge::vertex_layout(r[0x12]);
    static constexpr const char *kPrims[]={"prim_points","prim_lines","prim_line_strip","prim_triangles","prim_triangle_strip","prim_triangle_fan","prim_sprites","prim_7"};
    ++features_["draws"]; ++features_[kPrims[prim_type&7u]];
    // Render targets (FBP/FBW, PSM) and textures sampled from EDRAM: what a
    // hardware backend must keep as GPU surfaces.
    char key[48];
    std::snprintf(key,sizeof key,"target_%08X_fmt%u",static_cast<unsigned>(0x04000000u|(r[0x9C]&0x1FFFF0u)),static_cast<unsigned>(r[0xD2]&3u));
    ++features_[key];
    ++features_[layout.through?"through_mode":"transform_mode"];
    const bool clear=(r[0xD3]&1u)!=0u;
    if(clear){ ++features_["clear_mode"]; return; }
    if(!layout.through && on(0x17)) ++features_["lighting"];
    if(!layout.through && on(0x1F)) ++features_["fog"];
    if(on(0x20)) ++features_["dither"];
    if(on(0x21)) ++features_["alpha_blend"];
    if(on(0x22)) ++features_["alpha_test"];
    if(on(0x23)) ++features_["depth_test"];
    if(on(0x24)) ++features_["stencil_test"];
    if(on(0x25)) ++features_["antialias"];
    if(on(0x27)) ++features_["color_test"];
    if(on(0x28)) ++features_["logic_op"];
    if(on(0x1D)) ++features_["cull"];
    if(layout.weights) ++features_["skinning"];
    if(layout.morphs>1u) ++features_["morph"];
    if(layout.index_format) ++features_["indexed"];
    if(on(0x1E) && layout.uv_format) {
        ++features_["textured"];
        static constexpr const char *kFormats[]={"tex_565","tex_5551","tex_4444","tex_8888","tex_clut4","tex_clut8","tex_clut16","tex_clut32","tex_dxt1","tex_dxt3","tex_dxt5"};
        const auto fmt=r[0xC3]&0xFu; ++features_[fmt<11u?kFormats[fmt]:"tex_other"];
        if((r[0xC6]&0x0101u)!=0u) ++features_["tex_filter_linear"];
        if((r[0xC6]&0x4u)!=0u) ++features_["tex_mipmap_filter"]; // min filter 4..7
        if(!layout.through && (r[0xC0]&3u)!=0u) {
            ++features_["texgen"];
            // TEXTURE_MAP_MODE: bits 0-1 mode (1 texture matrix, 2 environment map), bits 8-9 matrix source
            std::snprintf(key,sizeof key,"texgen_mode%u_src%u",static_cast<unsigned>(r[0xC0]&3u),static_cast<unsigned>((r[0xC0]>>8)&3u));
            ++features_[key];
        }
        if((r[0xC2]&0xFF0000u)!=0u) ++features_["tex_levels"];
        const auto tex=(r[0xA0]&0xFFFFF0u)|((r[0xA8]>>16)&0xFFu)<<24;
        if((tex&0x0F000000u)==0x04000000u) {
            std::snprintf(key,sizeof key,"tex_from_edram_%08X",static_cast<unsigned>(tex&0x0FFFFFFFu));
            ++features_[key];
        }
    }
}

// GE command execution. Opcodes: PSPSDK psp/pspsdk/src/ge/pspge.h and the
// gu command list (pspgu.h); PPSSPP GPU/ge_constants.h cross-checked only.
void GeManager::execute(psprecomp::Runtime &rt, GeListInfo &l, std::uint32_t word) {
    const auto op=word>>24, arg=word&0xFFFFFF;
    auto &mem=rt.memory();
    regs_.reg[op]=arg;
    state_.registers[op]=arg;
    auto target=[&](std::uint32_t a){ return state_.relative(a)&~3u; };
    switch(op) {
    case 0x01: state_.vertex=state_.relative(arg); break;
    case 0x02: state_.index=state_.relative(arg); break;
    case 0x04: { // PRIM
        const auto count=arg&0xFFFF, type=(arg>>16)&7;
        if(type==7) { rt.stop("GE PRIM type 7 unsupported"); return; }
        if(census) count_features(type);
        const ProfileScope timer(host_profile().render_ns, host_profile().render_calls);
        if(!skip_rasterization) renderer_->draw(mem,regs_,static_cast<ge::Prim>(type),count,state_.vertex,state_.index);
        if(draw_log) log_draw(mem,type,count);
        const auto layout=ge::vertex_layout(regs_.reg[0x12]);
        if(layout.index_format) state_.index+=count*(layout.index_format==1?1u:2u);
        else state_.vertex+=count*layout.size;
        if(!writer_observed_) { writer_observed_=true; writer_pc_=l.pc;
            rt.event("first_graphics_writer",{{"list",static_cast<std::uint32_t>(l.id)},{"ge_pc",l.pc},{"word",word}}); }
        break;
    }
    case 0x05: case 0x06: rt.stop("GE BEZIER/SPLINE unsupported "+psprecomp::hex32(word)); return;
    case 0x07: l.bbox_failed=false; break; // [INFERRED] bounding boxes treated as visible
    case 0x08: l.pc=target(arg); return;
    case 0x09: if(l.bbox_failed) { l.pc=target(arg); return; } break;
    case 0x0A:
        if(l.call_stack.size()>=32) { rt.stop("GE call stack overflow"); return; }
        l.call_stack.emplace_back(l.pc+4,state_.offset);
        l.pc=target(arg); return;
    case 0x0B:
        if(l.call_stack.empty()) { rt.stop("GE RET with empty call stack"); return; }
        l.pc=l.call_stack.back().first; state_.offset=l.call_stack.back().second; l.call_stack.pop_back(); return;
    case 0x0E: { // SIGNAL: behaviours 1-3 call the signal handler (pspge.h PSP_GE_SIGNAL_HANDLER_*)
        const auto behaviour=arg>>16;
        if(behaviour<1 || behaviour>3) { rt.stop("GE SIGNAL behaviour unsupported "+psprecomp::hex32(word)); return; }
        rt.event("ge_signal",{{"list",static_cast<std::uint32_t>(l.id)},{"behaviour",behaviour},{"value",arg&0xFFFF}});
        break;
    }
    case 0x0F: l.finish_token=arg&0xFFFF; break;
    case 0x13: state_.offset=arg<<8; break;
    case 0x14: state_.offset=l.pc; break; // ORIGIN
    case 0x2A: regs_.reg[0x2A]=arg; state_.matrix_index[0]=arg&0x7F; break;
    case 0x2B: { auto &i=state_.matrix_index[0]; if(i<regs_.bone.size()) regs_.bone[i]=ge::ge_float(arg); ++i; break; }
    case 0x3A: state_.matrix_index[1]=arg&0xF; break;
    case 0x3B: { auto &i=state_.matrix_index[1]; if(i<12) regs_.world[i]=ge::ge_float(arg); ++i; break; }
    case 0x3C: state_.matrix_index[2]=arg&0xF; break;
    case 0x3D: { auto &i=state_.matrix_index[2]; if(i<12) regs_.view[i]=ge::ge_float(arg); ++i; break; }
    case 0x3E: state_.matrix_index[3]=arg&0xF; break;
    case 0x3F: { auto &i=state_.matrix_index[3]; if(i<16) regs_.proj[i]=ge::ge_float(arg); ++i; break; }
    case 0x40: state_.matrix_index[4]=arg&0xF; break;
    case 0x41: { auto &i=state_.matrix_index[4]; if(i<12) regs_.tgen[i]=ge::ge_float(arg); ++i; break; }
    case 0xC4: { // LOADCLUT: arg blocks of 32 bytes from CLUTADDR (+ upper bits)
        const auto address=(regs_.reg[0xB0]&0xFFFFF0u)|((regs_.reg[0xB1]&0x0F0000u)<<8);
        const auto bytes=std::min<std::uint32_t>((arg&0x3F)*32u,1024u);
        regs_.clut.fill(0); regs_.clut_words=bytes/4u;
        for(std::uint32_t i=0;i<bytes;i+=4) if(mem.contains(address+i,4)) regs_.clut[i/4]=mem.load32(address+i);
        std::uint64_t h=0x84222325CBF29CE4ull^regs_.clut_words;
        for(std::uint32_t i=0;i<regs_.clut_words;++i) h=(h^regs_.clut[i])*0x100000001B3ull;
        regs_.clut_hash=h;
        break;
    }
    case 0xEA: {
        const auto src=(regs_.reg[0xB2]&0xFFFFF0u)|((regs_.reg[0xB3]>>16)&0xFFu)<<24, dst=(regs_.reg[0xB4]&0xFFFFF0u)|((regs_.reg[0xB5]>>16)&0xFFu)<<24;
        if(census) {
            char key[48];
            std::snprintf(key,sizeof key,"transfer_%s_to_%s",(src&0x0F000000u)==0x04000000u?"edram":"ram",(dst&0x0F000000u)==0x04000000u?"edram":"ram");
            ++features_[key];
        }
        const ProfileScope timer(host_profile().render_ns, host_profile().render_calls); if(!skip_rasterization) renderer_->transfer(mem,regs_); break;
    }
    default: break;
    }
    l.pc+=4;
}

void GeManager::pump(psprecomp::Runtime &rt) {
    std::uint64_t budget=4u<<20;
    auto &mem=rt.memory();
    while(!queue_.empty() && !rt.stopped()) {
        const int id=queue_.front();
        auto &l=lists_.at(id);
        if(l.status==GeStatus::Paused) return;
        // Commands of the list at the head of the queue (one map lookup per
        // list, not per command).
        while(true) {
            if(l.pc==l.stall_address && l.stall_address) { l.status=GeStatus::Stalled; return; }
            l.status=GeStatus::Running;
            if(!budget--) {rt.stop("GE command budget exceeded");return;}
            if((l.pc&3) || !mem.contains(l.pc,4)) {rt.stop("GE invalid command fetch at "+psprecomp::hex32(l.pc));return;}
            const auto word=mem.aot_load32(l.pc), op=word>>24;
            ++l.commands;
            if(op==0x0C) { // END (after FINISH: completion; after SIGNAL: continue)
                l.pc+=4;
                if(l.previous==0x0E) { l.previous=op; continue; }
                ++l.completions; l.status=GeStatus::Completed;
                queue_.pop_front();
                rt.event("ge_finish", {{"list",l.id},{"ge_pc",l.pc},{"commands",l.commands},{"completions",l.completions}});
                if(l.previous==0x0F) deliver_finish_callback(rt,l,l.pc);
                break;
            }
            execute(rt,l,word);
            l.previous=op;
            if(rt.stopped() || queue_.empty() || queue_.front()!=id) break;
        }
    }
}

void GeManager::report() const {
    std::cout << "[GE STATE] color=" << psprecomp::hex32(state_.color_address()) << " stride=" << state_.color_stride()
              << " format=" << state_.format() << " depth=" << psprecomp::hex32(state_.depth_address())
              << " stride=" << state_.depth_stride() << "\n";
    for(const auto &[id,l]:lists_) std::cout << "[GE LIST] id=" << id << " pc=" << psprecomp::hex32(l.pc)
        << " status=" << static_cast<unsigned>(l.status) << " stall=" << psprecomp::hex32(l.stall_address)
        << " commands=" << l.commands << " completions=" << l.completions << " cb=" << l.callback_id << "\n";
}
void register_ge_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    runtime.register_hle("sceGe_user",0xE47E40E4u,[&kernel](auto &,auto &c){c.set_gpr(2,kernel.ge().edram_base());});
    runtime.register_hle("sceGe_user",0x1F6752ADu,[&kernel](auto &,auto &c){c.set_gpr(2,kernel.ge().edram_size());});
    runtime.register_hle("sceGe_user",0xA4FC06A4u,[&kernel](auto &r,auto &c){c.set_gpr(2,kernel.ge().set_callback(r.memory(),c.gpr[4]));});
    runtime.register_hle("sceGe_user",0x05DB22CEu,[&kernel](auto &,auto &c){c.set_gpr(2,kernel.ge().unset_callback(c.gpr[4]));});
    for(auto nid:{0xAB49E76Au,0x1C0D95A6u}) runtime.register_hle("sceGe_user",nid,[&kernel,nid](auto &r,auto &c){
        c.set_gpr(2,kernel.ge().enqueue_list(r,c.gpr[4],c.gpr[5],static_cast<int>(c.gpr[6]),c.gpr[7],nid==0x1C0D95A6u));});
    runtime.register_hle("sceGe_user",0xE0D68148u,[&kernel](auto &r,auto &c){c.set_gpr(2,kernel.ge().update_stall(r,c.gpr[4],c.gpr[5]));});
    runtime.register_hle("sceGe_user",0x03444EB4u,[&kernel](auto &r,auto &c){c.set_gpr(2,kernel.ge().sync(r,c.gpr[4],c.gpr[5]));});
    runtime.register_hle("sceGe_user",0xB287BD61u,[&kernel](auto &r,auto &c){c.set_gpr(2,kernel.ge().sync(r,0,c.gpr[4],true));});
    for(auto nid:{0xB448EC0Du,0x4C06E472u}) runtime.register_hle("sceGe_user",nid,[](auto &r,auto &){r.stop("GE break/continue requires implementation");});
    // sceGeGetCmd(cmd) / sceGeGetMtx(id, out): uOFW src/kd/ge/ge.c (sceGeGetCmd,
    // sceGeGetMtx); out-of-range index -> SCE_ERROR_INVALID_INDEX (0x80000102).
    // P3P reads them for the battle-transition effect (first Shadow, New 3DS).
    // [INFERRED] the command register holds the whole word (opcode << 24 | argument).
    runtime.register_hle("sceGe_user",0xDC93CFEFu,[&kernel](auto &,auto &c){
        const auto cmd=c.gpr[4];
        if(cmd>=0xFFu) { c.set_gpr(2,0x80000102u); return; }
        c.set_gpr(2,(cmd<<24)|(kernel.ge().registers().reg[cmd]&0xFFFFFFu));
    });
    runtime.register_hle("sceGe_user",0x57C8945Bu,[&kernel](auto &r,auto &c){
        const auto id=static_cast<std::int32_t>(c.gpr[4]); const auto out=c.gpr[5];
        if(id<0 || id>=12) { c.set_gpr(2,0x80000102u); return; }
        const auto &g=kernel.ge().registers();
        const float *m=id==10?g.proj.data():id==11?g.tgen.data():id==8?g.world.data():id==9?g.view.data():&g.bone[static_cast<std::size_t>(id)*12u];
        const unsigned n=id==10?16u:12u;
        if(!r.memory().contains(out,n*4u)) { c.set_gpr(2,0x80000023u); return; }
        for(unsigned i=0;i<n;++i) { std::uint32_t bits; std::memcpy(&bits,&m[i],4); r.memory().store32(out+i*4u,bits>>8); }
        c.set_gpr(2,0u);
    });
}
} // namespace p3p3ds::hle
