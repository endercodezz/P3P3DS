#pragma once
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"
#include <algorithm>
namespace p3p3ds {
// Address membership is evidence of a target write, not proof of a visible pixel.
// Untagged VRAM is not automatically called allocator metadata or texture data.
struct VramActivity {
    std::uint64_t resource{}, texture{}, color{}, depth{}, suppressed{};
    static bool overlaps(std::uint32_t address, std::size_t bytes, std::uint32_t target, std::size_t size) {
        using M=psprecomp::GuestMemory;
        const auto canonical=M::canonical(target);
        if(!bytes || !size || canonical<M::kVramPhysicalBase || canonical>=M::kVramPhysicalBase+M::kVramAddressSpan) return false;
        const auto a=(address-M::kVramPhysicalBase)%M::kVramSize;
        const auto b=(canonical-M::kVramPhysicalBase)%M::kVramSize;
        if(bytes>=M::kVramSize || size>=M::kVramSize) return true;
        // Circular physical VRAM intervals, including writes through mirrors.
        for(auto shift:{-std::int64_t(M::kVramSize),std::int64_t(0),std::int64_t(M::kVramSize)}) {
            const auto start=std::int64_t(b)+shift;
            if(std::int64_t(a)<start+std::int64_t(size) && std::int64_t(a)+std::int64_t(bytes)>start) return true;
        }
        return false;
    }
    void observe(psprecomp::Runtime &r,const KernelState &k,std::uint32_t address,std::size_t bytes,bool stop_any=false) {
        const auto &g=k.ge().state();const auto &d=k.display().framebuf(0);
        const auto height=static_cast<unsigned>(std::clamp(d.height,0,1024));
        const bool is_color=overlaps(address,bytes,g.color_address(),std::size_t(g.color_stride())*height*(g.format()==3?4:2)) ||
            (d.active && d.bufferwidth>0 && d.pixelformat>=0 && d.pixelformat<=3 &&
             overlaps(address,bytes,d.topaddr,std::size_t(d.bufferwidth)*height*(d.pixelformat==3?4:2)));
        const bool is_depth=overlaps(address,bytes,g.depth_address(),std::size_t(g.depth_stride())*height*2);
        bool is_texture=false;
        // Diagnostic membership only for bound, enabled, unswizzled direct-color textures.
        // PPSSPP GPU/GPUState.h::getTextureAddress/getTextureHeight/getTextureFormat;
        // GPU/Common/TextureDecoder.cpp::GetTextureBufw (16-byte alignment).
        const auto format=g.registers[0xC3]&15;
        if((g.registers[0x1E]&1) && !(g.registers[0xC2]&1) && format<=3) for(unsigned level=0;level<=((g.registers[0xC2]>>16)&7);++level) {
            const auto base=(g.registers[0xA0+level]&0xFFFFF0)|((g.registers[0xA8+level]<<8)&0x0F000000);
            const unsigned alignment=format==3?4:8;
            const auto stride=std::max(alignment,g.registers[0xA8+level]&(0x7FF&~(alignment-1)));
            const auto h=1u<<((g.registers[0xB8+level]>>8)&15);
            is_texture |= overlaps(address,bytes,base,std::size_t(stride)*h*(format==3?4:2));
        }
        color+=is_color;depth+=is_depth;texture+=is_texture;
        const bool other=!is_color && !is_depth && !is_texture;
        resource+=other;
        const char *category=is_color?"color_framebuffer":is_depth?"depth_buffer":is_texture?"bound_texture_resource":"unclassified_resource";
        // First 32 non-render records plus complete aggregate counts; never
        // terminate guest execution merely because metadata writes are numerous.
        if(is_color || is_depth || resource+texture<=32 || stop_any) {
            r.event("cpu_vram_write",{{"address",address},{"bytes",bytes},{"color",is_color},{"depth",is_depth},{"texture",is_texture},
                {"instruction",r.memory().contains(r.diagnostic_pc,4)?r.memory().load32(r.diagnostic_pc):0}},category);
        } else ++suppressed;
        if(is_color || is_depth || stop_any) {
            r.cpu().pc=r.diagnostic_pc;
            r.stop(is_color?"CPU VRAM color target write observed":is_depth?"CPU VRAM depth target write observed":"CPU VRAM write observed; inspect target and producer");
            throw psprecomp::FrontierHalt{};
        }
    }
};
}
