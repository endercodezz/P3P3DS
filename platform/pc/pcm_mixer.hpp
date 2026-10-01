#pragma once
// Host PCM output for the PC runner: mixes every sceAudio buffer at its
// virtual start time (AudioState::PcmSink) into one 44.1 kHz stereo S16
// timeline and streams it to a WAV file. Buffers arrive slightly ahead of or
// behind each other, so only samples older than the newest start minus a
// window are final and written.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

namespace p3p3ds {

class PcmMixer {
public:
    static constexpr std::uint32_t kRate = 44100u;
    explicit PcmMixer(const std::string &wav_path) {
        if (!wav_path.empty()) file_ = std::fopen(wav_path.c_str(), "wb");
        if (file_) write_header(0u);
    }
    ~PcmMixer() { finish(); }
    PcmMixer(const PcmMixer &) = delete;
    PcmMixer &operator=(const PcmMixer &) = delete;

    void add(std::uint64_t start_us, const std::vector<std::int16_t> &stereo) {
        const auto start = start_us * kRate / 1000000u; // sample index
        const auto frames = stereo.size() / 2u;
        if (start < flushed_) { late_ += frames; return; }
        const auto end = start + frames;
        if (end > flushed_ + pending_.size() / 2u) pending_.resize((end - flushed_) * 2u, 0);
        for (std::size_t i = 0; i < stereo.size(); ++i) pending_[(start - flushed_) * 2u + i] += stereo[i];
        ++buffers_;
        // Samples more than one second before this buffer are final.
        if (start > flushed_ + kRate) flush_until(start - kRate);
    }
    void finish() {
        if (finished_) return;
        finished_ = true;
        flush_until(flushed_ + pending_.size() / 2u);
        if (file_) {
            std::fseek(file_, 0, SEEK_SET);
            write_header(static_cast<std::uint32_t>(frames_written_ * 4u));
            std::fclose(file_);
            file_ = nullptr;
        }
    }
    std::uint64_t frames() const noexcept { return frames_written_ + pending_.size() / 2u; }
    std::uint64_t nonzero_frames() const noexcept { return nonzero_frames_; }
    std::uint32_t peak() const noexcept { return peak_; }
    std::uint64_t buffers() const noexcept { return buffers_; }
    std::uint64_t late_frames() const noexcept { return late_; }
    std::uint64_t clipped() const noexcept { return clipped_; }

private:
    void flush_until(std::uint64_t sample) {
        const auto count = std::min<std::uint64_t>(sample - flushed_, pending_.size() / 2u);
        std::vector<std::int16_t> out(count * 2u);
        for (std::size_t i = 0; i < out.size(); ++i) {
            const auto v = pending_[i];
            const auto c = std::clamp<std::int32_t>(v, -32768, 32767);
            clipped_ += c != v;
            out[i] = static_cast<std::int16_t>(c);
            peak_ = std::max<std::uint32_t>(peak_, static_cast<std::uint32_t>(c < 0 ? -c : c));
        }
        for (std::size_t f = 0; f < count; ++f) nonzero_frames_ += (out[f * 2u] | out[f * 2u + 1u]) != 0;
        if (file_ && !out.empty()) std::fwrite(out.data(), sizeof(std::int16_t), out.size(), file_);
        pending_.erase(pending_.begin(), pending_.begin() + static_cast<std::ptrdiff_t>(count * 2u));
        flushed_ += count;
        frames_written_ += count;
    }
    void write_header(std::uint32_t data_bytes) {
        auto u32 = [&](std::uint32_t v) { std::fwrite(&v, 4, 1, file_); };
        auto u16 = [&](std::uint16_t v) { std::fwrite(&v, 2, 1, file_); };
        std::fwrite("RIFF", 1, 4, file_); u32(36u + data_bytes); std::fwrite("WAVEfmt ", 1, 8, file_);
        u32(16u); u16(1u); u16(2u); u32(kRate); u32(kRate * 4u); u16(4u); u16(16u);
        std::fwrite("data", 1, 4, file_); u32(data_bytes);
    }

    std::FILE *file_{};
    std::deque<std::int32_t> pending_;
    std::uint64_t flushed_{}, frames_written_{}, nonzero_frames_{}, buffers_{}, late_{}, clipped_{};
    std::uint32_t peak_{};
    bool finished_{};
};

} // namespace p3p3ds
