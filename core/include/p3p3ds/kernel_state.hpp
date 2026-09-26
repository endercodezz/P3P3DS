#pragma once

#include "p3p3ds/hle/display.hpp"
#include "p3p3ds/hle/ge.hpp"
#include "p3p3ds/hle/sysmem.hpp"
#include "p3p3ds/hle/threadman.hpp"

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

    [[nodiscard]] bool has_compiled_sdk_version() const noexcept {
        return (system_flags_ & kSystemFlagSdkSet) != 0;
    }

    void set_compiler_version(std::uint32_t version) noexcept {
        compiler_version_ = version;
        // In PSP kernel (SysMem / PPSSPP): SystemGameInfo.flags |= 0x2000
        system_flags_ |= kSystemFlagCompilerVersionSet;
    }

    [[nodiscard]] std::uint32_t compiler_version() const noexcept {
        return compiler_version_;
    }

    [[nodiscard]] bool has_compiler_version() const noexcept {
        return (system_flags_ & kSystemFlagCompilerVersionSet) != 0;
    }

    [[nodiscard]] std::uint32_t system_flags() const noexcept {
        return system_flags_;
    }

    [[nodiscard]] hle::ThreadManager &threads() noexcept {
        return thread_manager_;
    }

    [[nodiscard]] const hle::ThreadManager &threads() const noexcept {
        return thread_manager_;
    }

    [[nodiscard]] hle::SysMemManager &sysmem() noexcept {
        return sysmem_manager_;
    }

    [[nodiscard]] const hle::SysMemManager &sysmem() const noexcept {
        return sysmem_manager_;
    }

    [[nodiscard]] hle::DisplayManager &display() noexcept {
        return display_manager_;
    }

    [[nodiscard]] const hle::DisplayManager &display() const noexcept {
        return display_manager_;
    }

    [[nodiscard]] hle::GeManager &ge() noexcept {
        return ge_manager_;
    }

    [[nodiscard]] const hle::GeManager &ge() const noexcept {
        return ge_manager_;
    }

private:
    static constexpr std::uint32_t kSystemFlagSdkSet = 0x1000u;
    static constexpr std::uint32_t kSystemFlagCompilerVersionSet = 0x2000u;

    std::uint32_t compiled_sdk_version_{0};
    std::uint32_t compiler_version_{0};
    std::uint32_t system_flags_{0};
    hle::ThreadManager thread_manager_;
    hle::SysMemManager sysmem_manager_;
    hle::DisplayManager display_manager_;
    hle::GeManager ge_manager_;
};

} // namespace p3p3ds
