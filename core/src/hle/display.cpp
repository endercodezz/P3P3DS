#include "p3p3ds/hle/display.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/runtime.hpp"

#include <iostream>

namespace p3p3ds::hle {

void register_display_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    // sceDisplay::0x0E20F177 - sceDisplaySetMode
    runtime.register_hle("sceDisplay", 0x0E20F177u,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            const int mode = static_cast<int>(ctx.gpr[4]);
            const int width = static_cast<int>(ctx.gpr[5]);
            const int height = static_cast<int>(ctx.gpr[6]);
            kernel.display().set_mode(mode, width, height);
            std::cout << "[DISPLAY] sceDisplaySetMode(mode=" << mode
                      << ", width=" << width << ", height=" << height << ")\n";
            ctx.set_gpr(2, 0u);
        });

    // sceDisplay::0x289D82FE - sceDisplaySetFrameBuf
    runtime.register_hle("sceDisplay", 0x289D82FEu,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t topaddr = ctx.gpr[4];
            const int bufferwidth = static_cast<int>(ctx.gpr[5]);
            const int pixelformat = static_cast<int>(ctx.gpr[6]);
            const int sync = static_cast<int>(ctx.gpr[7]);
            kernel.display().set_framebuf(topaddr, bufferwidth, pixelformat, sync);
            std::cout << "\n====================================================\n"
                      << "   [P3P3DS VISUAL PROBE] REAL PSP FRAMEBUFFER SET!\n"
                      << "====================================================\n"
                      << "  topaddr:      0x" << std::hex << topaddr << "\n"
                      << "  bufferwidth:  " << std::dec << bufferwidth << "\n"
                      << "  pixelformat:  " << pixelformat << " (0:565, 1:5551, 2:4444, 3:8888)\n"
                      << "  sync:         " << sync << "\n"
                      << "====================================================\n\n";
            ctx.set_gpr(2, 0u);
        });

    // sceDisplay::0xEEDA2E54 - sceDisplayGetFrameBuf
    runtime.register_hle("sceDisplay", 0xEEDA2E54u,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t topaddr_ptr = ctx.gpr[4];
            const std::uint32_t bufwidth_ptr = ctx.gpr[5];
            const std::uint32_t format_ptr = ctx.gpr[6];
            if (ctx.gpr[7] > 1u) { ctx.set_gpr(2, 0x80000107u); return; }
            for (auto ptr : {topaddr_ptr, bufwidth_ptr, format_ptr}) {
                if (ptr && ((ptr & 3u) || !rt.memory().contains(ptr, 4u))) {
                    ctx.set_gpr(2, 0x80000103u); return;
                }
            }
            const auto &info = kernel.display().framebuf(ctx.gpr[7]);
            if (topaddr_ptr != 0u) rt.memory().store32(topaddr_ptr, info.topaddr);
            if (bufwidth_ptr != 0u) rt.memory().store32(bufwidth_ptr, static_cast<std::uint32_t>(info.bufferwidth));
            if (format_ptr != 0u) rt.memory().store32(format_ptr, static_cast<std::uint32_t>(info.pixelformat));
            ctx.set_gpr(2, 0u);
        });

    // sceDisplay::0x7ED59BC4 - sceDisplaySetHoldMode
    runtime.register_hle("sceDisplay", 0x7ED59BC4u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: hold mode stub
            ctx.set_gpr(2, 0u);
        });

    // DIRTY_FIRST_FRAME: deterministic vblank boundary, no wall-clock simulation.
    for (auto nid : {0x36CDFADEu, 0x8EB9EC49u, 0x984C27E7u, 0x46F186C3u}) {
        runtime.register_hle("sceDisplay", nid,
            [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
                kernel.display().advance_vblank();
                ctx.set_gpr(2, 0u);
            });
    }
    // sceDisplayWaitVblankStartMultiCB(int vblanks)
    runtime.register_hle("sceDisplay", 0x77ED8B3Au,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            const auto count=static_cast<std::int32_t>(ctx.gpr[4]);
            if (count<=0) { ctx.set_gpr(2,0x800001FEu); return; }
            for (std::int32_t i=0;i<count;++i) kernel.display().advance_vblank();
            ctx.set_gpr(2,0u);
        });
    runtime.register_hle("sceDisplay", 0x9C6EAAD7u,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, static_cast<std::uint32_t>(kernel.display().vcount()));
        });
    runtime.register_hle("sceDisplay", 0xDBA6C4C4u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.fpr[0] = 59.94005995f; // PSP float return ABI, not $v0.
        });
}
} // namespace p3p3ds::hle
