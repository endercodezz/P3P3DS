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
        std::uint32_t left_volume{0}, right_volume{0};
        // uOFW audio.c::audioOutput keeps one queued buffer per channel
        // (channel->buf); the mixer takes it when the playing one ends.
        std::uint64_t slot_free_at{0}; // virtual us when the queued buffer starts playing
        std::uint64_t play_end{0};     // virtual us when the last accepted buffer ends
        std::uint64_t buffers{0};
    };
    static constexpr std::uint32_t kSampleRate = 44100u;
    std::uint32_t reserve(std::int32_t channel, std::int32_t sample_count, std::int32_t format) noexcept;
    // Accepts a buffer at time `now` if the queue slot is free; returns the
    // sample count, OUTPUT_BUSY (slot occupied) or another error.
    std::uint32_t output(std::uint32_t channel, std::uint32_t left, std::uint32_t right, std::uint64_t now) noexcept;
    Channel &channel_state(unsigned index) noexcept { return channels_[index]; }
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
