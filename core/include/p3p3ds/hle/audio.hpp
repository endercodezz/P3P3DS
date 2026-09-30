#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace psprecomp { class Runtime; class GuestMemory; }
namespace p3p3ds { class KernelState; }

namespace p3p3ds::hle {

// sceAudio state after references/uofw/src/kd/audio/audio.c. Timing runs on
// ThreadManager's virtual clock: each channel holds at most one queued buffer
// (uOFW channel->buf); the mixer takes it when the previous buffer ends.
class AudioState {
public:
    static constexpr std::uint32_t kSampleRate = 44100u;
    static constexpr unsigned kSrcChannel = 8u; // sceAudioOutput2 / SRC channel

    struct Channel {
        bool reserved{false};
        std::uint32_t sample_count{0};
        std::uint32_t format{0};           // 0 stereo, 0x10 mono
        std::uint32_t left_volume{0}, right_volume{0};
        std::uint64_t slot_free_at{0};     // virtual us when the queued buffer starts playing
        std::uint64_t play_end{0};         // virtual us when the last accepted buffer ends
        std::uint64_t buffers{0};
        std::uint32_t blocked{0};          // threads waiting in *Blocking (uOFW unk10)
    };

    // Receives every accepted buffer as interleaved stereo S16 at 44.1 kHz,
    // already scaled by the channel volume (0x8000 = unity).
    using PcmSink = std::function<void(unsigned channel, std::uint64_t start_us,
                                       const std::vector<std::int16_t> &stereo)>;
    void set_sink(PcmSink sink) { sink_ = std::move(sink); }

    std::uint32_t reserve(std::int32_t channel, std::int32_t sample_count, std::int32_t format) noexcept;
    std::uint32_t release(std::uint32_t channel, std::uint64_t now) noexcept;
    std::uint32_t set_data_len(std::uint32_t channel, std::int32_t sample_count) noexcept;
    std::uint32_t change_volume(std::uint32_t channel, std::int32_t left, std::int32_t right) noexcept;
    std::uint32_t change_config(std::uint32_t channel, std::int32_t format, std::uint64_t now) noexcept;
    std::uint32_t rest_length(std::uint32_t channel, std::uint64_t now) const noexcept;
    // Accepts a buffer at `now` if the queue slot is free; returns the sample
    // count, OUTPUT_BUSY (slot occupied) or another error. Reads PCM for the sink.
    std::uint32_t output(std::uint32_t channel, std::int32_t left, std::int32_t right, std::uint32_t buffer,
                         std::uint64_t now, const psprecomp::GuestMemory *memory);
    std::uint32_t src_reserve(std::int32_t sample_count, std::int32_t frequency, std::int32_t channels) noexcept;
    std::uint32_t src_release(std::uint64_t now) noexcept;

    const Channel &channel(unsigned index) const noexcept { return channels_[index]; }
    Channel &channel_state(unsigned index) noexcept { return channels_[index]; }
    bool record_telemetry() noexcept {
        if (telemetry_count_ == 16) return false;
        ++telemetry_count_;
        return true;
    }

private:
    std::array<Channel, 9> channels_{};
    unsigned telemetry_count_{0};
    PcmSink sink_;
};

void register_audio_module(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
