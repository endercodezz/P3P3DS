#pragma once
#include "p3p3ds/hle/threadman.hpp"
namespace p3p3ds::hle {
// Registration only. No drive notifications, waits or callback delivery.
class UmdState {
public:
    std::uint32_t register_callback(std::int32_t uid, const ThreadManager &threads) {
        // uOFW mediaman.c::sceUmdRegisterUMDCallBack validates Callback type.
        if (!threads.get_callback(uid)) return 0x80010016u;
        callback_uid_ = uid;
        return 0;
    }
    std::int32_t registered_callback() const noexcept { return callback_uid_; }
private:
    std::int32_t callback_uid_{};
};
void register_umd_module(psprecomp::Runtime &, KernelState &);
}
