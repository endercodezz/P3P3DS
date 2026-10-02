// sceCtrl after references/uofw/src/kd/ctrl/ctrl.c: sampling is updated every
// vblank; ReadBuffer* waits for the next update, Peek* returns immediately.
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

#include <algorithm>
#include <map>

namespace p3p3ds::hle {
namespace {
constexpr std::uint32_t kInvalidMode = 0x80000107u, kInvalidValue = 0x800001FEu, kInvalidSize = 0x80000104u;

// SceCtrlData: timeStamp, buttons, aX, aY, rsrv[6] (16 bytes).
void write_samples(psprecomp::GuestMemory &memory, std::uint32_t data, std::uint32_t count,
                   const HostInput &input, std::uint32_t timestamp, bool negative) {
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto at = data + 16u * i;
        memory.store32(at, timestamp);
        memory.store32(at + 4u, negative ? ~input.buttons : input.buttons);
        memory.store8(at + 8u, input.analog_x);
        memory.store8(at + 9u, input.analog_y);
        for (std::uint32_t b = 10; b < 16u; ++b) memory.store8(at + b, 0u);
    }
}
} // namespace

void register_ctrl_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    const auto reg = [&runtime](std::uint32_t nid, psprecomp::Runtime::HleFunction fn) {
        runtime.register_hle("sceCtrl", nid, std::move(fn));
    };
    reg(0x1F4011E6u, [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceCtrlSetSamplingMode
        const auto mode = ctx.gpr[4] & 0xFFu;
        if (mode > 1u) { ctx.set_gpr(2, kInvalidMode); return; }
        ctx.set_gpr(2, kernel.input().sampling_mode);
        kernel.input().sampling_mode = mode;
    });
    reg(0xA7144800u, [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceCtrlSetIdleCancelThreshold
        const auto unhold = static_cast<std::int32_t>(ctx.gpr[4]), hold = static_cast<std::int32_t>(ctx.gpr[5]);
        if (unhold < -1 || unhold > 128 || hold < -1 || hold > 128) { ctx.set_gpr(2, kInvalidValue); return; }
        kernel.input().idle_unhold = unhold; kernel.input().idle_hold = hold;
        ctx.set_gpr(2, 0u);
    });
    const auto peek = [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, bool negative) {
        const auto count = ctx.gpr[5] & 0xFFu;
        if (count >= 64u) { ctx.set_gpr(2, kInvalidSize); return; }
        if (count != 0u && !rt.memory().contains(ctx.gpr[4], 16u * count)) { ctx.set_gpr(2, 0x800200D3u); return; }
        kernel.input().refresh(kernel.threads().vblank_count());
        write_samples(rt.memory(), ctx.gpr[4], count, kernel.input(), static_cast<std::uint32_t>(kernel.threads().now()), negative);
        ctx.set_gpr(2, count);
    };
    reg(0x3A622550u, [peek](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { peek(rt, ctx, false); }); // PeekBufferPositive
    reg(0xC152080Au, [peek](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { peek(rt, ctx, true); });  // PeekBufferNegative
    const auto read = [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, bool negative) {
        const auto count = ctx.gpr[5] & 0xFFu;
        if (count >= 64u) { ctx.set_gpr(2, kInvalidSize); return; }
        if (count != 0u && !rt.memory().contains(ctx.gpr[4], 16u * count)) { ctx.set_gpr(2, 0x800200D3u); return; }
        auto &tm = kernel.threads();
        auto &input = kernel.input();
        const auto uid = tm.current_thread_id();
        const auto pending = input.pending_reads.find(uid);
        if (pending == input.pending_reads.end() || tm.now() < pending->second) {
            // Wait for the next sampling update and retry at the stub. Samples are
            // taken shortly after vblank start, not at it: in
            // references/pspautotests/tests/ctrl/vblank.expected a vblank interrupt
            // handler still peeks the previous frame's sample, while each blocking
            // read returns outside the vblank interval 10-15 lines (~0.7-0.95 ms)
            // into the frame. A thread woken at vblank start therefore gets the
            // new sample within a millisecond, not one frame later (P3P's main
            // menu loop: WaitVblankStartMultiCB(2) then ReadBufferPositive).
            constexpr std::uint64_t kSampleOffsetUs = 700u; // [INFERRED] within the measured window
            const auto now = tm.now(), period = ThreadManager::kVblankPeriodUs;
            const auto next_index = now < kSampleOffsetUs ? 0u : (now - kSampleOffsetUs) / period + 1u;
            const auto deadline = next_index * period + kSampleOffsetUs;
            input.pending_reads[uid] = deadline;
            WaitInfo wait{WaitType::Vblank, 0, deadline};
            wait.retry = true;
            tm.block_current(rt, ctx, wait);
            return;
        }
        input.pending_reads.erase(pending);
        // Buffers updated since the previous read, capped at the request.
        const auto frames = tm.vblank_count() - input.last_read_vcount;
        input.last_read_vcount = tm.vblank_count();
        const auto n = static_cast<std::uint32_t>(std::min<std::uint64_t>(count, std::max<std::uint64_t>(frames, 1u)));
        input.refresh(tm.vblank_count());
        if (input.buttons != input.last_buttons) {
            rt.event("ctrl_buttons", {{"buttons", input.buttons}, {"previous", input.last_buttons}, {"vblank", tm.vblank_count()}});
            input.last_buttons = input.buttons;
        }
        write_samples(rt.memory(), ctx.gpr[4], n, input, static_cast<std::uint32_t>(tm.now()), negative);
        ctx.set_gpr(2, n);
    };
    reg(0x1F803938u, [read](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { read(rt, ctx, false); }); // ReadBufferPositive
    reg(0x60B81F86u, [read](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { read(rt, ctx, true); });  // ReadBufferNegative
}

} // namespace p3p3ds::hle
