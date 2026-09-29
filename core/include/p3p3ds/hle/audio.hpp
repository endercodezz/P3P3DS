#pragma once

#include <array>
#include <cstdint>

namespace psprecomp { class Runtime; }
namespace p3p3ds { class KernelState; }

namespace p3p3ds::hle {
class AudioState {
public:
    struct Channel {
        bool reserved{false};
        std::uint32_t sample_count{0};
        std::uint32_t format{0};
    };
    std::uint32_t reserve(std::int32_t channel, std::int32_t sample_count, std::int32_t format) noexcept;
    const Channel &channel(unsigned index) const noexcept { return channels_[index]; }
    bool record_telemetry() noexcept {
        if (telemetry_count_==16) return false;
        ++telemetry_count_;
        return true;
    }
private:
    std::array<Channel,8> channels_{};
    unsigned telemetry_count_{0};
};
void register_audio_module(psprecomp::Runtime &runtime, KernelState &kernel);
}
