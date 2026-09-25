#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace vcs {

// Streaming linear PCM resampler used by the host sceAudio sink.
//
// The important property here is not fancy filtering; it is continuity.  The
// previous implementation restarted the fractional source position at every
// PSP buffer.  Rates such as 32000 and 48000 therefore changed phase at every
// 512/1024-sample boundary and produced an audible click/warble.  This class
// keeps one time base and the last source frame across every submission on a
// guest channel.  Chunking a stream differently therefore produces exactly the
// same output samples (apart from the final, intentionally unflushed source
// interval).
class StreamingLinearResampler {
public:
    static constexpr std::uint32_t kOutputRate = 44100u;

    void reset(std::uint32_t source_rate = kOutputRate, bool stereo = true) noexcept {
        source_rate_ = source_rate == 0u ? kOutputRate : source_rate;
        stereo_ = stereo;
        have_previous_ = false;
        previous_left_ = 0;
        previous_right_ = 0;
        source_index_ = 0u;
        next_output_position_ = 0u;
    }

    [[nodiscard]] std::uint32_t source_rate() const noexcept { return source_rate_; }
    [[nodiscard]] bool stereo() const noexcept { return stereo_; }
    [[nodiscard]] bool initialized() const noexcept { return have_previous_; }

    template <typename Emit>
    std::uint64_t process(std::span<const std::int16_t> pcm, std::uint32_t frames,
                          bool stereo, std::uint32_t source_rate, Emit &&emit) {
        if (frames == 0u) return 0u;
        if (source_rate == 0u) source_rate = kOutputRate;
        const std::size_t channels = stereo ? 2u : 1u;
        if (pcm.size() < static_cast<std::size_t>(frames) * channels) return 0u;

        if ((have_previous_ && (source_rate_ != source_rate || stereo_ != stereo)) ||
            (!have_previous_ && (source_rate_ != source_rate || stereo_ != stereo))) {
            reset(source_rate, stereo);
        }

        std::uint64_t emitted = 0u;
        for (std::uint32_t frame = 0u; frame < frames; ++frame) {
            const std::size_t offset = static_cast<std::size_t>(frame) * channels;
            const std::int32_t current_left = pcm[offset];
            const std::int32_t current_right = stereo ? pcm[offset + 1u] : current_left;

            if (!have_previous_) {
                previous_left_ = current_left;
                previous_right_ = current_right;
                have_previous_ = true;
                source_index_ = 0u;
                next_output_position_ = 0u;
                // Source position zero is known exactly and can be emitted
                // without waiting for a look-ahead sample.
                emit(static_cast<std::int16_t>(previous_left_),
                     static_cast<std::int16_t>(previous_right_));
                ++emitted;
                next_output_position_ += source_rate_;
                continue;
            }

            const std::uint64_t previous_position = source_index_ * kOutputRate;
            const std::uint64_t current_position = (source_index_ + 1u) * kOutputRate;
            while (next_output_position_ <= current_position) {
                const std::uint64_t numerator = next_output_position_ - previous_position;
                const std::int64_t left = previous_left_ +
                    ((static_cast<std::int64_t>(current_left) - previous_left_) *
                     static_cast<std::int64_t>(numerator)) /
                        static_cast<std::int64_t>(kOutputRate);
                const std::int64_t right = previous_right_ +
                    ((static_cast<std::int64_t>(current_right) - previous_right_) *
                     static_cast<std::int64_t>(numerator)) /
                        static_cast<std::int64_t>(kOutputRate);
                emit(static_cast<std::int16_t>(std::clamp<std::int64_t>(left, -32768, 32767)),
                     static_cast<std::int16_t>(std::clamp<std::int64_t>(right, -32768, 32767)));
                ++emitted;
                next_output_position_ += source_rate_;
            }

            previous_left_ = current_left;
            previous_right_ = current_right;
            ++source_index_;
        }
        return emitted;
    }

private:
    std::uint32_t source_rate_{kOutputRate};
    bool stereo_{true};
    bool have_previous_{};
    std::int32_t previous_left_{};
    std::int32_t previous_right_{};
    std::uint64_t source_index_{};
    // Source position multiplied by kOutputRate.  Moving one output sample
    // advances this by source_rate_, so no floating point drift accumulates.
    std::uint64_t next_output_position_{};
};

} // namespace vcs
