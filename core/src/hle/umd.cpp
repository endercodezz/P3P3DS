#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/common.hpp"
namespace p3p3ds::hle {
void register_umd_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    runtime.register_hle("sceUmdUser", 0xC6183D47u,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const auto mode = ctx.gpr[4];
            const auto alias_ptr = ctx.gpr[5];
            const auto v0_before = ctx.gpr[2];
            std::uint32_t result = 0x80010016u;
            bool valid_pointer = rt.memory().contains(alias_ptr, 7u);
            bool valid_alias = false;
            if (valid_pointer) {
                try {
                    const auto alias = rt.memory().read_c_string(alias_ptr, 7u);
                    valid_alias = alias == "disc0:";
                    result = kernel.umd().activate(mode, alias);
                } catch (const psprecomp::Error &) {
                    valid_pointer = false; // Unterminated or unreadable guest string.
                }
            }
            ctx.set_gpr(2, result);
            rt.event("umd_activate", {{"mode",mode}, {"alias_ptr",alias_ptr},
                {"pointer_valid",valid_pointer}, {"alias_valid",valid_alias}, {"result",result},
                {"activation_requested",kernel.umd().activation_requested()},
                {"medium_present",kernel.umd().medium_present()},
                {"v0_before",v0_before}, {"a2",ctx.gpr[6]}, {"a3",ctx.gpr[7]},
                {"gp",ctx.gpr[28]}, {"sp",ctx.gpr[29]},
                {"caller",ctx.gpr[31]-8u}, {"return_pc",ctx.gpr[31]}});
        });
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
