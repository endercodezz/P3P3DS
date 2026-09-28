#pragma once
#include "p3p3ds/hle/threadman.hpp"
#include <string_view>
namespace p3p3ds::hle {
// Presence, callback identity and accepted activation request only.
// No drive readiness, filesystem mount, notifications, waits or delivery.
class UmdState {
public:
    void set_medium_present(bool present) noexcept { medium_present_ = present; }
    bool medium_present() const noexcept { return medium_present_; }
    std::uint32_t activate(std::uint32_t mode, std::string_view alias) noexcept {
        // uOFW mediaman.c::sceUmdActivate validates 1/2 and the exact "disc0:" alias.
        if ((mode != 1u && mode != 2u) || alias != "disc0:") return 0x80010016u;
        activation_requested_ = true;
        return 0u;
    }
    bool activation_requested() const noexcept { return activation_requested_; }
    std::uint32_t register_callback(std::int32_t uid, const ThreadManager &threads) {
        // uOFW mediaman.c::sceUmdRegisterUMDCallBack validates Callback type.
        if (!threads.get_callback(uid)) return 0x80010016u;
        callback_uid_ = uid;
        return 0;
    }
    std::int32_t registered_callback() const noexcept { return callback_uid_; }
private:
    bool medium_present_{false}; // Fresh kernel has no configured game medium.
    bool activation_requested_{false}; // Accepted request, not mounted/ready status.
    std::int32_t callback_uid_{};
};
void register_umd_module(psprecomp::Runtime &, KernelState &);
}
