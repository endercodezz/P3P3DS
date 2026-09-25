#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace mhp3rd::movie {

// A decoded picture as planar 4:2:0: a full-size luma plane, then the two
// half-size chroma planes, all tightly packed.
struct Picture {
    std::uint32_t width{};
    std::uint32_t height{};
    bool full_range{};
    std::vector<std::uint8_t> y;
    std::vector<std::uint8_t> cb;
    std::vector<std::uint8_t> cr;
};

// H.264 pictures decoded with FFmpeg's libavcodec. Without FFmpeg the class
// still exists but open() fails.
class AvcDecoder {
public:
    AvcDecoder();
    ~AvcDecoder();
    AvcDecoder(const AvcDecoder &) = delete;
    AvcDecoder &operator=(const AvcDecoder &) = delete;

    [[nodiscard]] static bool available() noexcept;

    bool open();
    void close();
    [[nodiscard]] bool is_open() const noexcept;

    // Decodes one access unit (Annex B). Returns true and fills `picture`
    // when a picture comes out; the decoder may hold pictures back.
    bool decode(std::span<const std::uint8_t> unit, Picture &picture);
    // Returns a held-back picture at the end of the stream, if any is left.
    bool drain(Picture &picture);
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mhp3rd::movie
