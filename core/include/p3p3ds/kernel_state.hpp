#pragma once

#include "p3p3ds/hle/display.hpp"
#include "p3p3ds/hle/audio.hpp"
#include "p3p3ds/hle/ge.hpp"
#include "p3p3ds/hle/iofilemgr.hpp"
#include "p3p3ds/hle/modulemgr.hpp"
#include "p3p3ds/hle/mpeg.hpp"
#include "p3p3ds/hle/sascore.hpp"
#include "p3p3ds/hle/sysmem.hpp"
#include "p3p3ds/hle/threadman.hpp"
#include "p3p3ds/hle/umd.hpp"

#include "p3p3ds/input.hpp"

#include <cstdint>
#include <memory>
#include <map>
#include <optional>
#include <string>

namespace p3p3ds {

namespace hle {
// Console settings returned by sceUtilityGetSystemParam* (IDs and value
// encodings: psp/pspsdk/src/utility/psputility_sysparam.h). Defaults are the
// reference console of pspautotests utility/systemparam.expected.
struct SystemParams {
    std::string nickname{"shadow"};
    std::int32_t adhoc_channel{0}, wlan_powersave{0}, date_format{2}, time_format{0}, timezone_minutes{60},
                 daylight_savings{0}, language{1}, button_swap{1}, parental_level{9};
    [[nodiscard]] std::optional<std::int32_t> int_param(std::int32_t id) const noexcept;
};
// Host-provided controller state (PSP button bits, analog 0..255, 128 = centre).
struct HostInput {
    std::uint32_t buttons{0};
    std::uint8_t analog_x{128}, analog_y{128};
    std::uint32_t sampling_mode{0};
    std::int32_t idle_unhold{-1}, idle_hold{-1};
    std::map<std::int32_t, std::uint64_t> pending_reads; // thread -> sampling deadline
    std::uint64_t last_read_vcount{0};
    // Host source sampled at each read (none: buttons/analog above stay as set).
    std::shared_ptr<input::InputSource> source;
    std::uint32_t last_buttons{0}; // for the ctrl_buttons trace event
    void refresh(std::uint64_t vblank) {
        if (!source) return;
        const auto pad = source->sample(vblank);
        buttons = pad.buttons;
        analog_x = pad.analog_x;
        analog_y = pad.analog_y;
    }
};
} // namespace hle

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

    // Kernel_Library sceKernelCpuSuspendIntr/ResumeIntr state (enabled bit only).
    [[nodiscard]] bool interrupts_enabled() const noexcept { return interrupts_enabled_; }
    void set_interrupts_enabled(bool enabled) noexcept { interrupts_enabled_ = enabled; }

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

    hle::UmdState &umd() noexcept { return umd_; }
    const hle::UmdState &umd() const noexcept { return umd_; }
    hle::ModuleManager &modules() noexcept { return modules_; }
    hle::SasState &sas() noexcept { return sas_; }
    hle::MpegState &mpeg() noexcept { return mpeg_; }
    hle::SystemParams &system_params() noexcept { return system_params_; }
    hle::HostInput &input() noexcept { return input_; }
    hle::IoManager &io() noexcept { return io_; }
    const hle::IoManager &io() const noexcept { return io_; }
    hle::AudioState &audio() noexcept { return audio_; }
    const hle::AudioState &audio() const noexcept { return audio_; }
private:
    // Per-kernel state, never process-global registration.
    hle::UmdState umd_;
    hle::IoManager io_;
    hle::ModuleManager modules_;
    hle::SasState sas_;
    hle::MpegState mpeg_;
    hle::SystemParams system_params_;
    hle::HostInput input_;
    bool interrupts_enabled_{true};
    hle::AudioState audio_;
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
