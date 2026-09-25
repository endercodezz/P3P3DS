#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

namespace vcs {

// Audio and video decoding for the PSP media HLE, in process.
//
// This replaces spawning `ffmpeg` and reading its stdout through a pipe. That
// arrangement worked but could not ship: it made a separate program an install
// requirement for the game to play its own radio and cutscenes, and a process
// per stream is a poor place to be when the guest expects samples on time.
//
// Deliberately the same shape as the pipe it replaces -- open once, then pull
// bytes until the stream ends -- so the HLE around it did not have to be
// restructured. Audio arrives as interleaved signed 16-bit at the rate the
// caller asks for; video arrives as a continuous run of RGBA frames.
//
// The build is a minimal FFmpeg: atrac3p and h264 decoders, wav and mpegps
// demuxers, nothing else. See tools/ffmpeg-minimal in the handoff.

class AudioStreamDecoder {
public:
    AudioStreamDecoder();
    ~AudioStreamDecoder();
    AudioStreamDecoder(const AudioStreamDecoder &) = delete;
    AudioStreamDecoder &operator=(const AudioStreamDecoder &) = delete;
    // Movable: the HLE resets a context with `state = AtracContextState{}`.
    AudioStreamDecoder(AudioStreamDecoder &&) noexcept;
    AudioStreamDecoder &operator=(AudioStreamDecoder &&) noexcept;

    // start_sample seeks before the first read, matching what the pipe version
    // expressed as ffmpeg's -ss.
    [[nodiscard]] bool open(const std::filesystem::path &path, std::uint32_t sample_rate,
                            std::uint32_t channels, std::uint64_t start_sample);
    // Returns bytes written; less than the span means the stream ended.
    [[nodiscard]] std::size_t read(std::span<std::uint8_t> output);
    [[nodiscard]] bool is_open() const noexcept;
    void close() noexcept;

private:
    struct State;
    std::unique_ptr<State> state_;
};

// The audio track inside a PMF.
//
// It is there -- LOGO.PMF carries 90 private_stream_1 (0xBD) PES packets -- but
// no generic demuxer surfaces it, because the PSP puts ATRAC3+ in a private
// stream whose substream id means nothing to MPEG-PS. libavformat therefore
// reports the file as video-only, which is why the intro played silent.
//
// So the container is walked here instead: every 0xBD packet, minus a four-byte
// PSP substream header, concatenated into one ATRAC3+ elementary stream and
// handed to the same decoder the radio uses.
class PmfAudioDecoder {
public:
    PmfAudioDecoder();
    ~PmfAudioDecoder();
    PmfAudioDecoder(const PmfAudioDecoder &) = delete;
    PmfAudioDecoder &operator=(const PmfAudioDecoder &) = delete;
    PmfAudioDecoder(PmfAudioDecoder &&) noexcept;
    PmfAudioDecoder &operator=(PmfAudioDecoder &&) noexcept;

    [[nodiscard]] bool open(const std::filesystem::path &path);
    [[nodiscard]] std::size_t read(std::span<std::uint8_t> output);
    [[nodiscard]] bool is_open() const noexcept;
    void close() noexcept;

private:
    struct State;
    std::unique_ptr<State> state_;
};

class VideoStreamDecoder {
public:
    VideoStreamDecoder();
    ~VideoStreamDecoder();
    VideoStreamDecoder(const VideoStreamDecoder &) = delete;
    VideoStreamDecoder &operator=(const VideoStreamDecoder &) = delete;
    VideoStreamDecoder(VideoStreamDecoder &&) noexcept;
    VideoStreamDecoder &operator=(VideoStreamDecoder &&) noexcept;

    [[nodiscard]] bool open(const std::filesystem::path &path);
    // RGBA, tightly packed, frame after frame.
    [[nodiscard]] std::size_t read(std::span<std::uint8_t> output);
    [[nodiscard]] bool is_open() const noexcept;
    void close() noexcept;

private:
    struct State;
    std::unique_ptr<State> state_;
};

} // namespace vcs
