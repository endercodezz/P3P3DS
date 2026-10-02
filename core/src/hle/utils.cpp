// UtilsForUser (data-cache maintenance, libc time on the virtual clock),
// sceSuspendForUser power tick and sceDmac memcpy.
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

#include <vector>

namespace p3p3ds::hle {

void register_utils_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    const auto reg = [&runtime](std::uint32_t nid, psprecomp::Runtime::HleFunction fn) {
        runtime.register_hle("UtilsForUser", nid, std::move(fn));
    };
    // Host guest memory is coherent: write-back/invalidate have no observable
    // effect (same policy as CACHE in recomp/PSPRecomp/include/psprecomp/codegen_policy.hpp).
    for (const auto nid : {0x79D1C3FAu, 0xB435DEC5u, 0x34B9FA9Eu, 0x3EE30821u, 0xBFA98062u, 0x80001C4Cu}) {
        reg(nid, [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            rt.memory().memory_barrier();
            ctx.set_gpr(2, 0u);
        });
    }
    // [INFERRED] fixed epoch so wall-clock time is deterministic: 2008-01-01 UTC.
    constexpr std::uint64_t kEpochSeconds = 1199145600u;
    reg(0x91E4F6A7u, [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelLibcClock (us)
        ctx.set_gpr(2, static_cast<std::uint32_t>(kernel.threads().now()));
    });
    reg(0x27CC57F0u, [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelLibcTime(time_t*)
        const auto seconds = static_cast<std::uint32_t>(kEpochSeconds + kernel.threads().now() / 1000000u);
        if (ctx.gpr[4] != 0u && rt.memory().contains(ctx.gpr[4], 4u)) rt.memory().store32(ctx.gpr[4], seconds);
        ctx.set_gpr(2, seconds);
    });
    reg(0x71EC4271u, [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelLibcGettimeofday(tv, tz)
        const auto now = kernel.threads().now();
        if (ctx.gpr[4] != 0u && rt.memory().contains(ctx.gpr[4], 8u)) {
            rt.memory().store32(ctx.gpr[4], static_cast<std::uint32_t>(kEpochSeconds + now / 1000000u));
            rt.memory().store32(ctx.gpr[4] + 4u, static_cast<std::uint32_t>(now % 1000000u));
        }
        if (ctx.gpr[5] != 0u && rt.memory().contains(ctx.gpr[5], 8u)) { rt.memory().store32(ctx.gpr[5], 0u); rt.memory().store32(ctx.gpr[5] + 4u, 0u); }
        ctx.set_gpr(2, 0u);
    });
    // sceSuspendForUser::sceKernelPowerTick: references/uofw/src/kd/sysmem/suspend.c
    // forwards to the power handler's tick (idle-timer reset) or returns 0.
    // [INFERRED] the handler's tick also returns 0; there is no idle timer here.
    runtime.register_hle("sceSuspendForUser", 0x090CCB3Fu, [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
        ctx.set_gpr(2, 0u);
    });
    // scePowerSetClockFrequency(pll, cpu, bus) and its 3.50 variant
    // (0xEBD177D6, named scePowerSetClockFrequency350 in
    // recomp/PSP-recompilation-project/src/rt/nid_names.h). pspautotests
    // power/freq.expected: frequencies below 19 fail with 0x800001FE, 19..333
    // succeed. P3P calls the variant when a battle starts. CPU clock is not
    // modelled (the virtual clock does not count cycles), so nothing else changes.
    // [INFERRED] the variant validates like the original.
    for (const auto nid : {0x737486F2u, 0xEBD177D6u}) {
        runtime.register_hle("scePower", nid, [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            const auto pll = static_cast<std::int32_t>(ctx.gpr[4]), cpu = static_cast<std::int32_t>(ctx.gpr[5]);
            const bool ok = pll >= 19 && pll <= 333 && cpu >= 1 && cpu <= pll;
            ctx.set_gpr(2, ok ? 0u : 0x800001FEu);
        });
    }
    // sceDmac Memcpy/TryMemcpy: errors from pspautotests dmac/dmactest.expected
    // (0 length 0x80000104 before NULL 0x80000103). The copy completes
    // synchronously [INFERRED]: no DMA channel contention is modelled, so the
    // concurrent TryMemcpy busy result (0x80000021) never occurs.
    for (const auto nid : {0x617F3FE6u, 0xD97F94D8u}) {
        runtime.register_hle("sceDmac", nid, [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const auto dst = ctx.gpr[4], src = ctx.gpr[5], size = ctx.gpr[6];
            auto &m = rt.memory();
            if (size == 0u) { ctx.set_gpr(2, 0x80000104u); return; }
            if (dst == 0u || src == 0u || !m.contains(dst, size) || !m.contains(src, size)) { ctx.set_gpr(2, 0x80000103u); return; }
            std::vector<std::uint8_t> bytes(size);
            m.copy_out(src, bytes);
            m.copy_in(dst, bytes);
            ctx.set_gpr(2, 0u);
        });
    }
}

} // namespace p3p3ds::hle
