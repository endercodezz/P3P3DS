#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <vector>

namespace mhp3rd::movie {

// Size of one MPEG program stream pack in a PSMF file.
inline constexpr std::size_t kPackSize = 2048u;
// 90 kHz ticks per video frame at 29.97 Hz.
inline constexpr std::int64_t kVideoFrameTicks = 3003;
// ATRAC3plus frames in a PSMF audio stream: an 8-byte header (0x0FD0 and the
// codec parameters) in front of each.
inline constexpr std::size_t kAtracFrameHeader = 8u;
inline constexpr std::size_t kAtracFrameSamples = 2048u;

struct AccessUnit {
    std::vector<std::uint8_t> data;
    std::int64_t pts{-1};
    std::int64_t dts{-1};
};

// Splits the MPEG-PS packs of a PSMF movie into access units: H.264 pictures
// from video stream 0xE0, delimited by their access unit delimiters, and
// ATRAC3plus frames from private stream 1. Timestamps come from the PES
// headers, which stamp only some units; the rest are extrapolated at the
// stream's fixed frame rate.
class PsmfDemuxer {
public:
    void reset();

    // Adds one pack. Returns false if it is not a program stream pack.
    bool push_pack(std::span<const std::uint8_t> pack);
    // No more packs will come: the last picture is complete.
    void end_of_stream();

    // A picture is only complete once the next one has started (or the
    // stream has ended).
    [[nodiscard]] std::optional<AccessUnit> pop_video();
    [[nodiscard]] std::optional<AccessUnit> pop_audio();
    [[nodiscard]] bool video_ready() const;
    [[nodiscard]] bool audio_ready() const;
    // Frame size, including its header, and the ATRAC3plus parameters from
    // the first frame header; 0 until one has been seen.
    [[nodiscard]] std::size_t audio_frame_size() const noexcept { return audio_frame_size_; }
    [[nodiscard]] unsigned audio_channels() const noexcept { return audio_channels_; }

private:
    struct Stamp {
        std::size_t offset{};  // position in the elementary stream buffer
        std::int64_t pts{-1};
        std::int64_t dts{-1};
    };
    void add_video(std::span<const std::uint8_t> payload, std::int64_t pts, std::int64_t dts);
    void add_audio(std::span<const std::uint8_t> payload, std::int64_t pts);
    void split_video();
    void split_audio();

    std::vector<std::uint8_t> video_;          // bytes not yet split into pictures
    std::size_t video_scanned_{};              // where the delimiter search resumes
    std::vector<std::size_t> video_starts_;    // delimiter offsets in video_
    std::deque<Stamp> video_stamps_;
    std::deque<AccessUnit> video_units_;
    std::int64_t last_video_pts_{-1};
    std::int64_t last_video_dts_{-1};
    bool ended_{};

    std::vector<std::uint8_t> audio_;
    std::deque<Stamp> audio_stamps_;
    std::deque<AccessUnit> audio_units_;
    std::int64_t last_audio_pts_{-1};
    std::size_t audio_frame_size_{};
    unsigned audio_channels_{};
};

} // namespace mhp3rd::movie
