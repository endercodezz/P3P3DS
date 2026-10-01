// UtilsForUser: data-cache maintenance and libc time on the virtual clock.
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

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
}

} // namespace p3p3ds::hle
