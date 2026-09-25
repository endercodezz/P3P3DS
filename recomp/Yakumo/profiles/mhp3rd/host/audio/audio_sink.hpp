#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace mhp3rd::audio {

inline constexpr std::uint32_t kSampleRate = 44'100u;
inline constexpr std::uint32_t kChannels = 2u;

// Host playback for the guest's 44100 Hz stereo stream.
//
// The guest and the audio device run on unrelated clocks: the kernel's virtual
// clock advances whenever threads block, the device consumes exactly 44100
// frames of real time per second. The sink absorbs that mismatch in a circular
// buffer of monotonic frame indices. Producers mix at their own cursor, so
// several sceAudio channels overlap the way the hardware mixer overlaps them,
// and the device retires frames from the front. Frames that were never written
// play as silence; frames the device could not keep up with are retired
// without being played. Both are counted rather than hidden, because the
// difference between the two tells us whether emulation is running slow or
// fast.
class AudioSink {
public:
    static AudioSink &instance();

    // Opens the device. Safe to call more than once; honours MHP3RD_NO_AUDIO.
    void initialize();
    void shutdown();

    // Mixes `frames` stereo frames at `cursor`, a monotonic frame index, with
    // volumes in 0..0x8000, and advances the cursor past them. A cursor that
    // has fallen behind the device is snapped forward to the write target.
    void mix(std::uint64_t &cursor, const std::int16_t *frames, std::size_t count,
             std::uint32_t left_volume, std::uint32_t right_volume);

    // Output gain in 0..1, from the volume and mute settings. Only the device
    // output is scaled; MHP3RD_AUDIO_DUMP keeps the game's own levels.
    void set_volume(float gain);
    // Stops and restarts the device, for the in-game menu's pause. What the
    // game queued before the pause stays in the ring and plays on resume.
    void set_paused(bool paused);
    // Whether a playback device is open (not with MHP3RD_NO_AUDIO, or when
    // none could be opened).
    [[nodiscard]] bool has_device() const;

    // Public only so the device callback, which lives outside the class, can
    // name it.
    struct Impl;

private:
    AudioSink();
    ~AudioSink();
    std::unique_ptr<Impl> impl_;
};

} // namespace mhp3rd::audio
