#include "audio/audio_sink.hpp"

#include "settings/settings.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#if defined(MHP3RD_HAS_SDL_AUDIO)
#include <SDL3/SDL.h>
#endif

namespace mhp3rd::audio {
namespace {

// ~186 ms. Long enough that a slow guest frame does not underrun, short enough
// that the guest's audio never lags visibly behind what is on screen.
constexpr std::size_t kRingFrames = 8192u;
// The device is kept two guest buffers ahead of the write cursor.
constexpr std::size_t kPrebufferFrames = 2048u;

[[nodiscard]] std::int16_t saturate(std::int32_t value) noexcept {
    return static_cast<std::int16_t>(std::clamp(value, -32768, 32767));
}

// Minimal 16-bit PCM WAV writer. The sizes are patched on close, so a run that
// is killed still leaves a file that most players will accept.
class WavWriter {
public:
    bool open(const std::string &path) {
        stream_.open(path, std::ios::binary | std::ios::trunc);
        if (!stream_) return false;
        static const char header[44] = {};
        stream_.write(header, sizeof(header));
        return true;
    }
    [[nodiscard]] bool active() const { return stream_.is_open(); }
    void write(const std::int16_t *samples, std::size_t count) {
        stream_.write(reinterpret_cast<const char *>(samples), static_cast<std::streamsize>(count * 2u));
        written_ += count;
    }
    void close() {
        if (!stream_.is_open()) return;
        flush();
        stream_.close();
    }
    // Rewrites the sizes in place so a run that is killed still leaves a file
    // that plays up to the last complete second.
    void flush() {
        if (!stream_.is_open()) return;
        const auto data_bytes = static_cast<std::uint32_t>(written_ * 2u);
        const std::uint32_t byte_rate = kSampleRate * kChannels * 2u;
        stream_.seekp(0);
        put("RIFF", 4);
        put32(36u + data_bytes);
        put("WAVEfmt ", 8);
        put32(16u);
        put16(1u);
        put16(static_cast<std::uint16_t>(kChannels));
        put32(kSampleRate);
        put32(byte_rate);
        put16(static_cast<std::uint16_t>(kChannels * 2u));
        put16(16u);
        put("data", 4);
        put32(data_bytes);
        stream_.seekp(0, std::ios::end);
        stream_.flush();
    }

private:
    void put(const char *text, std::size_t size) { stream_.write(text, static_cast<std::streamsize>(size)); }
    void put16(std::uint16_t value) {
        const std::uint8_t bytes[2] = {static_cast<std::uint8_t>(value), static_cast<std::uint8_t>(value >> 8u)};
        stream_.write(reinterpret_cast<const char *>(bytes), 2);
    }
    void put32(std::uint32_t value) {
        const std::uint8_t bytes[4] = {static_cast<std::uint8_t>(value), static_cast<std::uint8_t>(value >> 8u),
                                       static_cast<std::uint8_t>(value >> 16u), static_cast<std::uint8_t>(value >> 24u)};
        stream_.write(reinterpret_cast<const char *>(bytes), 4);
    }
    std::ofstream stream_;
    std::size_t written_{};
};

} // namespace

struct AudioSink::Impl {
    std::mutex lock;
    std::vector<std::int16_t> ring = std::vector<std::int16_t>(kRingFrames * kChannels, 0);
    std::uint64_t read_position{};   // frames handed to the device
    std::uint64_t write_end{};       // frames any producer has written up to
    bool started{};
    bool closed{};
    bool enabled{true};
    bool trace{};

    WavWriter dump;
    std::vector<std::int16_t> scratch;

    // Statistics over the current one-second reporting window.
    std::uint64_t window_frames{};
    std::uint64_t window_silent{};
    std::uint64_t window_dropped{};
    std::int32_t window_peak{};
    double window_energy{};
    std::uint64_t total_frames{};
    std::uint64_t total_silent{};
    std::uint64_t total_dropped{};
    std::uint64_t seconds{};

    float gain{1.0f};
#if defined(MHP3RD_HAS_SDL_AUDIO)
    SDL_AudioStream *stream{};
#endif

    // Takes `count` frames off the front of the ring into `out` (which may be
    // null when nobody is listening), zeroing them so the next lap starts
    // clean. Frames past `write_end` were never written and come out silent.
    void retire(std::size_t count, std::int16_t *out, bool played) {
        const std::uint64_t available = write_end > read_position ? write_end - read_position : 0u;
        const std::uint64_t silent = count > available ? count - available : 0u;
        for (std::size_t frame = 0; frame < count; ++frame) {
            const std::size_t slot = static_cast<std::size_t>((read_position + frame) % kRingFrames) * kChannels;
            const std::int16_t left = ring[slot];
            const std::int16_t right = ring[slot + 1u];
            ring[slot] = 0;
            ring[slot + 1u] = 0;
            if (out != nullptr) {
                out[frame * kChannels] = left;
                out[frame * kChannels + 1u] = right;
            }
            const std::int32_t magnitude = std::max(std::abs(static_cast<std::int32_t>(left)),
                                                    std::abs(static_cast<std::int32_t>(right)));
            window_peak = std::max(window_peak, magnitude);
            window_energy += static_cast<double>(left) * left + static_cast<double>(right) * right;
        }
        read_position += count;
        window_frames += count;
        window_silent += silent;
        total_frames += count;
        total_silent += silent;
        if (!played) {
            window_dropped += count;
            total_dropped += count;
        }
        if (dump.active() && out != nullptr) dump.write(out, count * kChannels);
        if (window_frames >= kSampleRate) report();
    }

    void report() {
        const double rms = std::sqrt(window_energy / static_cast<double>(window_frames * kChannels));
        ++seconds;
        if (dump.active()) dump.flush();
        if (trace) {
            std::printf("[audio] %llus frames=%llu peak=%d rms=%.1f silent=%llu dropped=%llu\n",
                        static_cast<unsigned long long>(seconds),
                        static_cast<unsigned long long>(window_frames), window_peak, rms,
                        static_cast<unsigned long long>(window_silent),
                        static_cast<unsigned long long>(window_dropped));
            std::fflush(stdout);
        }
        window_frames = 0u;
        window_silent = 0u;
        window_dropped = 0u;
        window_peak = 0;
        window_energy = 0.0;
    }
};

namespace {

#if defined(MHP3RD_HAS_SDL_AUDIO)
void SDLCALL feed_device(void *user, SDL_AudioStream *stream, int additional, int) {
    if (additional <= 0) return;
    auto *impl = static_cast<AudioSink::Impl *>(user);
    const std::size_t frames = static_cast<std::size_t>(additional) / (kChannels * 2u);
    if (frames == 0u) return;
    std::lock_guard<std::mutex> guard(impl->lock);
    impl->scratch.resize(frames * kChannels);
    impl->retire(frames, impl->scratch.data(), true);
    SDL_PutAudioStreamData(stream, impl->scratch.data(), static_cast<int>(frames * kChannels * 2u));
}
#endif

} // namespace

AudioSink::AudioSink() : impl_(std::make_unique<Impl>()) {}
AudioSink::~AudioSink() { shutdown(); }

AudioSink &AudioSink::instance() {
    static AudioSink sink;
    return sink;
}

void AudioSink::initialize() {
    Impl &impl = *impl_;
    if (impl.started) return;
    impl.started = true;
    impl.trace = std::getenv("MHP3RD_TRACE_AUDIO") != nullptr;
    impl.enabled = std::getenv("MHP3RD_NO_AUDIO") == nullptr;
    const settings::Settings &player = settings::current();
    impl.gain = player.mute ? 0.0f : static_cast<float>(player.volume) / 100.0f;

    if (const char *path = std::getenv("MHP3RD_AUDIO_DUMP")) {
        if (impl.dump.open(path))
            std::cout << "Audio: writing the mix to " << path << "\n";
        else
            std::cerr << "Audio: cannot write " << path << "\n";
    }

    if (!impl.enabled) {
        std::cout << "Audio: disabled by MHP3RD_NO_AUDIO\n";
        return;
    }
#if defined(MHP3RD_HAS_SDL_AUDIO)
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        std::cerr << "Audio: SDL_InitSubSystem failed (" << SDL_GetError() << "); running silent\n";
        return;
    }
    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_S16LE;
    spec.channels = static_cast<int>(kChannels);
    spec.freq = static_cast<int>(kSampleRate);
    impl.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &feed_device, &impl);
    if (impl.stream == nullptr) {
        std::cerr << "Audio: SDL_OpenAudioDeviceStream failed (" << SDL_GetError() << "); running silent\n";
        return;
    }
    SDL_SetAudioStreamGain(impl.stream, impl.gain);
    SDL_ResumeAudioStreamDevice(impl.stream);
    std::cout << "Audio: 44100 Hz stereo playback open\n";
#else
    std::cout << "Audio: built without SDL; running silent\n";
#endif
}

void AudioSink::shutdown() {
    Impl &impl = *impl_;
    if (impl.closed) return;
    impl.closed = true;
#if defined(MHP3RD_HAS_SDL_AUDIO)
    if (impl.stream != nullptr) {
        SDL_DestroyAudioStream(impl.stream);
        impl.stream = nullptr;
    }
#endif
    std::lock_guard<std::mutex> guard(impl.lock);
    if (impl.dump.active()) impl.dump.close();
    if (impl.trace) {
        std::printf("[audio] total frames=%llu silent=%llu dropped=%llu\n",
                    static_cast<unsigned long long>(impl.total_frames),
                    static_cast<unsigned long long>(impl.total_silent),
                    static_cast<unsigned long long>(impl.total_dropped));
        std::fflush(stdout);
    }
}

void AudioSink::set_volume(float gain) {
    Impl &impl = *impl_;
    impl.gain = std::clamp(gain, 0.0f, 1.0f);
#if defined(MHP3RD_HAS_SDL_AUDIO)
    if (impl.stream != nullptr) {
        SDL_SetAudioStreamGain(impl.stream, impl.gain);
        std::cout << "Audio: output gain " << SDL_GetAudioStreamGain(impl.stream) << "\n";
    }
#endif
}

bool AudioSink::has_device() const {
#if defined(MHP3RD_HAS_SDL_AUDIO)
    return impl_->stream != nullptr;
#else
    return false;
#endif
}

void AudioSink::set_paused(bool paused) {
#if defined(MHP3RD_HAS_SDL_AUDIO)
    Impl &impl = *impl_;
    if (impl.stream == nullptr) return;
    if (paused) SDL_PauseAudioStreamDevice(impl.stream);
    else SDL_ResumeAudioStreamDevice(impl.stream);
#else
    (void)paused;
#endif
}

void AudioSink::mix(std::uint64_t &cursor, const std::int16_t *frames, std::size_t count,
                    std::uint32_t left_volume, std::uint32_t right_volume) {
    if (count == 0u) return;
    Impl &impl = *impl_;
    std::lock_guard<std::mutex> guard(impl.lock);

    // A buffer larger than the whole ring would overwrite its own start; only
    // its tail can survive, so drop the head rather than corrupt the lap.
    if (count > kRingFrames) {
        const std::size_t skipped = count - kRingFrames;
        frames += skipped * kChannels;
        cursor += skipped;
        count = kRingFrames;
    }

    // A cursor behind the device belongs to a producer that stalled; restart it
    // at the write target rather than writing frames that already played.
    if (cursor < impl.read_position + kPrebufferFrames / 4u) cursor = impl.read_position + kPrebufferFrames;

    // The ring cannot hold what a runaway producer wants to write. Retire the
    // oldest frames unplayed to make room: the device misses them, but the
    // stream stays continuous and the dump stays complete.
    const std::uint64_t end = cursor + count;
    if (end > impl.read_position + kRingFrames) {
        const auto excess = static_cast<std::size_t>(end - (impl.read_position + kRingFrames));
        impl.scratch.resize(excess * kChannels);
        impl.retire(excess, impl.scratch.data(), false);
    }

    const std::int32_t left_gain = static_cast<std::int32_t>(std::min<std::uint32_t>(left_volume, 0x8000u));
    const std::int32_t right_gain = static_cast<std::int32_t>(std::min<std::uint32_t>(right_volume, 0x8000u));
    for (std::size_t frame = 0; frame < count; ++frame) {
        const std::size_t slot = static_cast<std::size_t>((cursor + frame) % kRingFrames) * kChannels;
        const std::int32_t left = (static_cast<std::int32_t>(frames[frame * kChannels]) * left_gain) >> 15;
        const std::int32_t right = (static_cast<std::int32_t>(frames[frame * kChannels + 1u]) * right_gain) >> 15;
        impl.ring[slot] = saturate(impl.ring[slot] + left);
        impl.ring[slot + 1u] = saturate(impl.ring[slot + 1u] + right);
    }
    cursor += count;
    impl.write_end = std::max(impl.write_end, cursor);

    // Without a device nothing drains the ring, so keep it moving here.
#if defined(MHP3RD_HAS_SDL_AUDIO)
    const bool draining = impl.stream != nullptr;
#else
    const bool draining = false;
#endif
    if (!draining && impl.write_end > impl.read_position + kPrebufferFrames) {
        const auto ready = static_cast<std::size_t>(impl.write_end - impl.read_position - kPrebufferFrames);
        impl.scratch.resize(ready * kChannels);
        impl.retire(ready, impl.scratch.data(), true);
    }
}

} // namespace mhp3rd::audio
