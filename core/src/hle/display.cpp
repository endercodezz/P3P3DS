#include "p3p3ds/hle/display.hpp"
#include "p3p3ds/hle/threadman.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/runtime.hpp"

#include <iostream>

namespace p3p3ds::hle {

void register_display_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    // sceDisplay::0x0E20F177 - sceDisplaySetMode
    runtime.register_hle("sceDisplay", 0x0E20F177u,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const int mode = static_cast<int>(ctx.gpr[4]);
            const int width = static_cast<int>(ctx.gpr[5]);
            const int height = static_cast<int>(ctx.gpr[6]);
            kernel.display().set_mode(mode, width, height);
            rt.event("display_mode", {{"mode",ctx.gpr[4]}, {"width",ctx.gpr[5]}, {"height",ctx.gpr[6]}});
            std::cout << "[DISPLAY] sceDisplaySetMode(mode=" << mode
                      << ", width=" << width << ", height=" << height << ")\n";
            ctx.set_gpr(2, 0u);
        });

    // sceDisplay::0x289D82FE - sceDisplaySetFrameBuf
    runtime.register_hle("sceDisplay", 0x289D82FEu,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t topaddr = ctx.gpr[4];
            const int bufferwidth = static_cast<int>(ctx.gpr[5]);
            const int pixelformat = static_cast<int>(ctx.gpr[6]);
            const int sync = static_cast<int>(ctx.gpr[7]);
            kernel.display().set_framebuf(topaddr, bufferwidth, pixelformat, sync);
            rt.event("display_framebuffer", {{"address",topaddr},{"stride",ctx.gpr[5]},{"format",ctx.gpr[6]},{"sync",ctx.gpr[7]}});
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

    // VBlank waits block on ThreadManager's virtual clock (59.94 Hz period,
    // matching sceDisplayGetFramePerSec). WaitVblankStart waits for the next
    // vblank boundary; Multi waits for `count` boundaries.
    const struct { std::uint32_t nid; bool callbacks; } vblank_waits[] = {
        {0x984C27E7u, false}, // sceDisplayWaitVblankStart
        {0x46F186C3u, true},  // sceDisplayWaitVblankStartCB
        {0x36CDFADEu, false}, // sceDisplayWaitVblank
        {0x8EB9EC49u, true},  // sceDisplayWaitVblankCB
    };
    for (const auto wait : vblank_waits) {
        runtime.register_hle("sceDisplay", wait.nid,
            [&kernel, callbacks = wait.callbacks](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
                auto &tm = kernel.threads();
                kernel.display().advance_vblank();
                tm.block_current(rt, ctx, WaitInfo{WaitType::Vblank, 0, tm.next_vblank_time(), 0u, callbacks});
            });
    }
    // sceDisplayWaitVblankStartMultiCB(int vblanks)
    runtime.register_hle("sceDisplay", 0x77ED8B3Au,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const auto count=static_cast<std::int32_t>(ctx.gpr[4]);
            if (count<=0) { ctx.set_gpr(2,0x800001FEu); return; }
            auto &tm = kernel.threads();
            for (std::int32_t i=0;i<count;++i) kernel.display().advance_vblank();
            const auto deadline = (tm.vblank_count() + static_cast<std::uint64_t>(count)) * ThreadManager::kVblankPeriodUs;
            tm.block_current(rt, ctx, WaitInfo{WaitType::Vblank, 0, deadline, 0u, true});
        });
    runtime.register_hle("sceDisplay", 0x9C6EAAD7u, // sceDisplayGetVcount
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, static_cast<std::uint32_t>(kernel.threads().vblank_count()));
        });
    runtime.register_hle("sceDisplay", 0xDBA6C4C4u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.fpr[0] = 59.94005995f; // PSP float return ABI, not $v0.
        });
}
} // namespace p3p3ds::hle
