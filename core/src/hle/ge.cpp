#include "p3p3ds/hle/ge.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"
#include "psprecomp/common.hpp"
#include <iostream>
#include <vector>
namespace p3p3ds::hle {
namespace {
constexpr std::int32_t invalid_id=static_cast<std::int32_t>(0x80000100u);
constexpr std::int32_t invalid_mode=static_cast<std::int32_t>(0x80000107u);
constexpr std::int32_t invalid_pointer=static_cast<std::int32_t>(0x80000023u);
constexpr std::int32_t invalid_size=static_cast<std::int32_t>(0x80000104u);
constexpr std::int32_t already=static_cast<std::int32_t>(0x80000020u);
bool setup_opcode(unsigned op) {
    // Whitelist of register writes in the captured static reset and dynamic setup.
    return op==0 || op==1 || op==2 || op==0x10 || op==0x12 || op==0x13 ||
        (op>=0x15 && op<=0x28) || (op>=0x2A && op<=0x33) ||
        (op>=0x36 && op<=0x38) || (op>=0x3A && op<=0x4D) ||
        op==0x50 || op==0x51 || (op>=0x53 && op<=0x58) ||
        (op>=0x5B && op<=0xB5) || (op>=0xB8 && op<=0xD0) ||
        (op>=0xD2 && op<=0xE9) || op==0xEB || op==0xEC || op==0xEE;
}
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
    const int id=next_id_++;
    GeListInfo list; list.id=id; list.list_address=start; list.pc=start&0x0FFFFFFF;
    list.stall_address=stall&0x0FFFFFFF; list.callback_id=cb; list.opt_param=opt;
    list.context_address=context; list.stack_count=stack_count; list.stack_address=stack;
    lists_.emplace(id,list); head ? queue_.push_front(id) : queue_.push_back(id);
    std::cout << "[GE ENQUEUE] id=" << id << " start=" << psprecomp::hex32(start)
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
void GeManager::capture_writer(psprecomp::Runtime &rt, const GeListInfo &l, std::uint32_t word) {
    writer_observed_=true; writer_pc_=l.pc;
    std::cout << "[GE FIRST WRITER] id=" << l.id << " pc=" << psprecomp::hex32(l.pc)
              << " word=" << psprecomp::hex32(word) << " op=" << psprecomp::hex32(word>>24)
              << " arg=" << psprecomp::hex32(word&0xFFFFFF) << " cb=" << l.callback_id
              << " stall=" << psprecomp::hex32(l.stall_address) << "\n";
    report();
    for(unsigned op : {0x10u,0x12u,0x13u,0x1Eu,0x42u,0x43u,0x44u,0x45u,0x46u,0x47u,0x4Cu,0x4Du,0xA0u,0xA8u,0xB8u,0xC2u,0xC3u,0xD3u,0xD4u,0xD5u})
        std::cout << "  reg=" << psprecomp::hex32(op) << " arg=" << psprecomp::hex32(state_.registers[op]) << "\n";
    std::cout << "  VADDR=" << psprecomp::hex32(state_.vertex) << " IADDR=" << psprecomp::hex32(state_.index)
              << " offset=" << psprecomp::hex32(state_.offset) << "\n";
    // Bounded raw prefix only; exact vertex stride will be decoded after the writer is observed.
    for(auto a : {state_.vertex,state_.index}) {
        std::cout << "  memory prefix " << psprecomp::hex32(a) << ":";
        for(unsigned i=0;i<64 && rt.memory().contains(a+i,4);i+=4) std::cout << " " << psprecomp::hex32(rt.memory().load32(a+i));
        std::cout << "\n";
    }
    rt.stop("GE first writer requires implementation");
}
void GeManager::deliver_finish_callback(psprecomp::Runtime &rt, const GeListInfo &l, std::uint32_t end_pc) {
    if (l.callback_id<0) return;
    const auto &callback=callbacks_.at(l.callback_id);
    if (!callback.finish) return;
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
    unsigned budget=32;
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
    if (!rt.stopped()) std::cout << "[GE CALLBACK COMPLETE] id=" << l.callback_id
                                 << " token=" << l.finish_token << " end=" << psprecomp::hex32(end_pc) << "\n";
}
void GeManager::pump(psprecomp::Runtime &rt) {
    unsigned budget=65536;
    while(!queue_.empty() && !rt.stopped()) {
        auto &l=lists_.at(queue_.front());
        if(l.status==GeStatus::Paused) return;
        if(l.pc==l.stall_address && l.stall_address) { l.status=GeStatus::Stalled; return; }
        l.status=GeStatus::Running;
        if(!budget--) {rt.stop("GE command budget exceeded");return;}
        if((l.pc&3) || !rt.memory().contains(l.pc,4)) {rt.stop("GE invalid command fetch");return;}
        const auto word=rt.memory().load32(l.pc), op=word>>24, arg=word&0xFFFFFF;
        if(op==4 || op==5 || op==6 || op==0xEA) { capture_writer(rt,l,word); return; }
        if(op==0x0B) {rt.stop("GE unsupported RET (not FINISH)");return;}
        if(op==0x0E) {rt.stop("GE unsupported SIGNAL");return;}
        if(op==0x0C) {
            if(l.previous!=0x0F) {rt.stop("GE END without FINISH unsupported");return;}
            ++l.commands; l.pc+=4; ++l.completions; l.status=GeStatus::Completed;
            std::cout << "[GE COMPLETED] id=" << l.id << " commands=" << l.commands << " completions=" << l.completions << "\n";
            queue_.pop_front();
            deliver_finish_callback(rt,l,l.pc);
            continue;
        }
        if(op==0x0F) l.finish_token=arg&0xFFFF;
        else {
            if(!setup_opcode(op)) {rt.stop("GE unsupported command "+psprecomp::hex32(word));return;}
            if(op==0xC4 && arg) {rt.stop("GE nonzero CLUT load requires implementation");return;}
            state_.registers[op]=arg;
            if(op==1) state_.vertex=state_.relative(arg);
            if(op==2) state_.index=state_.relative(arg);
            if(op==0x13) state_.offset=arg<<8;
            const unsigned matrix_ops[]={0x2A,0x3A,0x3C,0x3E,0x40};
            for(unsigned m=0;m<5;++m) {
                if(op==matrix_ops[m]) state_.matrix_index[m]=arg&127;
                if(op==matrix_ops[m]+1) state_.matrices[m][state_.matrix_index[m]++&127]=arg;
            }
        }
        ++l.commands; l.pc+=4; l.previous=op;
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
}
} // namespace p3p3ds::hle
