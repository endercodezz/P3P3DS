#pragma once

#include <cstdint>
#include <span>

namespace vcs {

// Host audio sink for the sceAudio HLE.  The PSP exposes eight regular PCM
// channels plus one SRC/Output2 channel; submissions are mixed on the guest's
// virtual-time line before they are handed to the native audio device.
[[nodiscard]] bool audio_output_enabled();

// Mix one PSP buffer into the host stream.  `guest_time_us` is the virtual time
// at which the PSP submitted the buffer.  Supplying that timestamp is important:
// it lets simultaneous guest channels land on the same output frames instead of
// racing one another through an append-only host queue.
void audio_output_submit(std::span<const std::int16_t> pcm, std::uint32_t frames,
                         bool stereo, std::uint32_t left, std::uint32_t right,
                         std::uint32_t source_rate, std::uint32_t channel,
                         std::uint64_t guest_time_us);

// Seal and queue audio whose guest time is safely in the past.  Call this from
// the vblank path even on frames where the game submitted no new audio so the
// native device keeps receiving silence rather than underrunning.
void audio_output_advance(std::uint64_t guest_time_us);

// Forget stream/resampler continuity for one PSP channel (release/re-reserve).
void audio_output_reset_channel(std::uint32_t channel);

// Releases the device. Safe to call when nothing was ever opened.
void audio_output_shutdown();

} // namespace vcs
