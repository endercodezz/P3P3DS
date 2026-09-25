#pragma once

#include <cstdint>

namespace p3p3ds {

class KernelState {
public:
    KernelState() = default;

    void set_compiled_sdk_version(std::uint32_t version) noexcept {
        compiled_sdk_version_ = version;
        // In uOFW / real PSP kernel (SysMem): SystemGameInfo.flags |= 0x1000
        system_flags_ |= kSystemFlagSdkSet;
    }

    [[nodiscard]] std::uint32_t compiled_sdk_version() const noexcept {
        return compiled_sdk_version_;
    }

    [[nodiscard]] std::uint32_t system_flags() const noexcept {
        return system_flags_;
    }

    [[nodiscard]] bool has_compiled_sdk_version() const noexcept {
        return (system_flags_ & kSystemFlagSdkSet) != 0;
    }

private:
    static constexpr std::uint32_t kSystemFlagSdkSet = 0x1000u;

    std::uint32_t compiled_sdk_version_{0};
    std::uint32_t system_flags_{0};
};

} // namespace p3p3ds
