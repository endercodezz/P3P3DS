#pragma once
#include "p3p3ds/hle/threadman.hpp"
namespace p3p3ds::hle {
// Presence and registration only. No activation, notifications, waits or delivery.
class UmdState {
public:
    void set_medium_present(bool present) noexcept { medium_present_ = present; }
    bool medium_present() const noexcept { return medium_present_; }
    std::uint32_t register_callback(std::int32_t uid, const ThreadManager &threads) {
        // uOFW mediaman.c::sceUmdRegisterUMDCallBack validates Callback type.
        if (!threads.get_callback(uid)) return 0x80010016u;
        callback_uid_ = uid;
        return 0;
    }
    std::int32_t registered_callback() const noexcept { return callback_uid_; }
private:
    bool medium_present_{false}; // Fresh kernel has no configured game medium.
    std::int32_t callback_uid_{};
};
void register_umd_module(psprecomp::Runtime &, KernelState &);
}
