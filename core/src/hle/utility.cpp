// sceUtility: system parameters. Contract and values from
// references/pspautotests/tests/utility/systemparam/systemparam.{c,expected}
// (hardware) and psp/pspsdk/src/utility/psputility_sysparam.h (IDs, codes).
// Values are console settings: KernelState::system_params() defaults to the
// reference console of that transcript and can be changed by the host.
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

namespace p3p3ds::hle {
namespace {
constexpr std::uint32_t kParamFail = 0x80110103u;      // PSP_SYSTEMPARAM_RETVAL_FAIL
constexpr std::uint32_t kStringTooShort = 0x80110102u; // systemparam.expected, lengths 0..6
} // namespace

std::optional<std::int32_t> SystemParams::int_param(std::int32_t id) const noexcept {
    switch (id) {
    case 2: return adhoc_channel;
    case 3: return wlan_powersave;
    case 4: return date_format;
    case 5: return time_format;
    case 6: return timezone_minutes;
    case 7: return daylight_savings;
    case 8: return language;
    case 9: return button_swap;
    case 10: return parental_level;
    default: return std::nullopt; // 1 is the nickname string; others fail on hardware
    }
}

void register_utility_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    runtime.register_hle("sceUtility", 0xA5DA2406u, [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // GetSystemParamInt
        const auto value = kernel.system_params().int_param(static_cast<std::int32_t>(ctx.gpr[4]));
        if (!value) { ctx.set_gpr(2, kParamFail); return; }
        if (rt.memory().contains(ctx.gpr[5], 4u)) rt.memory().store32(ctx.gpr[5], static_cast<std::uint32_t>(*value));
        ctx.set_gpr(2, 0u);
    });
    runtime.register_hle("sceUtility", 0x34B78343u, [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // GetSystemParamString
        if (ctx.gpr[4] != 1u) { ctx.set_gpr(2, kParamFail); return; }
        const auto &nick = kernel.system_params().nickname;
        const auto length = static_cast<std::int32_t>(ctx.gpr[6]);
        // Hardware: the buffer must hold the whole name and its terminator,
        // otherwise nothing is written (lengths 0..6 and -1 for "shadow").
        if (length < static_cast<std::int32_t>(nick.size() + 1u)) { ctx.set_gpr(2, kStringTooShort); return; }
        if (!rt.memory().contains(ctx.gpr[5], nick.size() + 1u)) { ctx.set_gpr(2, kParamFail); return; }
        for (std::size_t i = 0; i <= nick.size(); ++i)
            rt.memory().store8(ctx.gpr[5] + static_cast<std::uint32_t>(i), i < nick.size() ? static_cast<std::uint8_t>(nick[i]) : 0u);
        ctx.set_gpr(2, 0u);
    });
}

} // namespace p3p3ds::hle
