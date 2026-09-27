#include "p3p3ds/kernel_state.hpp"
namespace p3p3ds::hle {
void register_umd_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    // uOFW mediaman_user.h / PSPSDK pspumd.h: zero absent, nonzero present.
    runtime.register_hle("sceUmdUser", 0x46EBB729u,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const bool present = kernel.umd().medium_present();
            ctx.set_gpr(2, present ? 1u : 0u);
            rt.event("umd_check_medium", {{"present",present}, {"result",ctx.gpr[2]},
                {"caller",ctx.gpr[31]-8}, {"return_pc",ctx.gpr[31]}});
        });
    runtime.register_hle("sceUmdUser", 0xAEE7404Du,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const auto uid = static_cast<std::int32_t>(ctx.gpr[4]);
            const auto result = kernel.umd().register_callback(uid, kernel.threads());
            ctx.set_gpr(2, result);
            rt.event("umd_callback_register", {{"uid",ctx.gpr[4]}, {"result",result},
                {"registered_uid",static_cast<std::uint32_t>(kernel.umd().registered_callback())},
                {"caller",ctx.gpr[31]-8}, {"return_pc",ctx.gpr[31]}});
        });
}
}
