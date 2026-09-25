#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace mhp3rd::audio {

enum class AtracCodec {
    Atrac3,     // baseline ATRAC3: 1024 samples per channel per frame
    Atrac3Plus, // ATRAC3plus: 2048 samples per channel per frame
};

[[nodiscard]] constexpr std::size_t atrac_frame_samples(AtracCodec codec) noexcept {
    return codec == AtracCodec::Atrac3 ? 1024u : 2048u;
}

// One ATRAC stream, decoded frame by frame with FFmpeg's libavcodec. Without
// FFmpeg the class still exists but open() fails, so callers fall back to
// silence.
class AtracDecoder {
public:
    AtracDecoder();
    ~AtracDecoder();
    AtracDecoder(const AtracDecoder &) = delete;
    AtracDecoder &operator=(const AtracDecoder &) = delete;

    // True when the build links a decoder.
    [[nodiscard]] static bool available() noexcept;

    // For ATRAC3, `extradata` is the 14 codec-specific bytes that follow
    // cbSize in the WAVE `fmt ` chunk (coding mode and frame factor);
    // ATRAC3plus needs none.
    bool open(AtracCodec codec, unsigned channels, unsigned block_align, std::span<const std::uint8_t> extradata);
    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] AtracCodec codec() const noexcept;

    // Decodes one frame of `block_align` bytes into interleaved 16-bit stereo;
    // `out` must hold atrac_frame_samples(codec) * 2 samples. Mono streams come
    // out on both sides. Returns the samples per channel written, 0 on error.
    std::size_t decode(std::span<const std::uint8_t> frame, std::int16_t *out);

    // Drops the overlap state, for decoding from another position.
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mhp3rd::audio
