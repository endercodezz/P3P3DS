#include "audio/sas_core.hpp"

#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <map>

namespace mhp3rd::audio {
namespace {

// The five SPU ADPCM predictor coefficient pairs, in 1/64ths.
constexpr std::int32_t kPredictorA[5] = {0, 60, 115, 98, 122};
constexpr std::int32_t kPredictorB[5] = {0, 0, -52, -55, -60};

constexpr std::uint32_t kPitchUnity = 0x1000u;
constexpr std::uint32_t kPitchMax = 0x4000u;
constexpr std::int32_t kVolumeUnity = 0x1000;

[[nodiscard]] std::int32_t clamp16(std::int32_t value) noexcept {
    return std::clamp(value, -32768, 32767);
}

struct EnvelopeRate {
    std::int32_t step{};
    std::int32_t period{1};
};

// The SPU rate encoding the SAS inherits: the low two bits pick the step and
// the rest a shift. Below a shift of 11 the envelope moves every sample by a
// larger step; above it, by the smallest step but only every 1<<(shift-11)
// samples. Exponential attack slows fourfold past three quarters of full
// scale; exponential decrease scales the step by the current level.
[[nodiscard]] EnvelopeRate decode_rate(std::uint32_t rate, bool increase, bool exponential,
                                       std::int32_t level) noexcept {
    const std::int32_t shift = static_cast<std::int32_t>((rate >> 2) & 0x1Fu);
    const auto step_index = static_cast<std::int32_t>(rate & 3u);
    EnvelopeRate result{};
    result.step = increase ? (7 - step_index) : (-8 + step_index);
    if (shift > 11)
        result.period = 1 << (shift - 11);
    else
        result.step <<= (11 - shift);
    if (exponential) {
        if (increase && level > 0x6000)
            result.period *= 4;
        else if (!increase)
            result.step = static_cast<std::int32_t>(static_cast<std::int64_t>(result.step) * level / 0x8000);
    }
    if (result.step == 0) result.step = increase ? 1 : -1;
    return result;
}

[[nodiscard]] bool envelopes_disabled() {
    static const bool disabled = std::getenv("MHP3RD_SAS_NO_ENV") != nullptr;
    return disabled;
}

[[nodiscard]] bool tracing() {
    static const bool enabled = std::getenv("MHP3RD_TRACE_AUDIO") != nullptr;
    return enabled;
}

// Per-second view of what the SAS is actually doing, so a silent mix can be
// told apart from a mix nobody asked for.
struct RenderTrace {
    std::uint64_t frames{};
    std::uint64_t key_ons{};
    std::int32_t peak{};
    std::uint32_t voices{};
};
RenderTrace &trace() {
    static RenderTrace state;
    return state;
}

} // namespace

void SasCore::init(std::uint32_t grain, std::uint32_t max_voices, std::uint32_t output_mode) {
    grain_ = grain != 0u && grain <= 2048u ? grain : 256u;
    max_voices_ = std::clamp(max_voices, 1u, kSasMaxVoices);
    output_mode_ = output_mode;
    voices_ = {};
}

void SasCore::set_voice(std::uint32_t voice, std::uint32_t address, std::uint32_t size, bool looping) {
    if (voice >= kSasMaxVoices) return;
    SasVoice &v = voices_[voice];
    v.address = address;
    v.size = size & ~0xFu;
    v.looping = looping;
    v.pcm = false;
    v.loop_block = 0u;
    v.source_ended = address == 0u || v.size < 16u;
}

void SasCore::set_voice_pcm(std::uint32_t voice, std::uint32_t address, std::uint32_t size, std::int32_t loop) {
    if (voice >= kSasMaxVoices) return;
    SasVoice &v = voices_[voice];
    v.address = address;
    v.size = size;
    v.looping = loop >= 0;
    v.pcm = true;
    v.loop_block = 0u;
    v.source_ended = address == 0u || size < 2u;
}

void SasCore::set_pitch(std::uint32_t voice, std::uint32_t pitch) {
    if (voice >= kSasMaxVoices) return;
    voices_[voice].pitch = std::min(pitch, kPitchMax);
}

void SasCore::set_volume(std::uint32_t voice, std::int32_t left, std::int32_t right) {
    if (voice >= kSasMaxVoices) return;
    voices_[voice].left = std::clamp(left, -kVolumeUnity, kVolumeUnity);
    voices_[voice].right = std::clamp(right, -kVolumeUnity, kVolumeUnity);
}

void SasCore::set_simple_adsr(std::uint32_t voice, std::uint32_t adsr1, std::uint32_t adsr2) {
    if (voice >= kSasMaxVoices) return;
    voices_[voice].adsr1 = adsr1 & 0xFFFFu;
    voices_[voice].adsr2 = adsr2 & 0xFFFFu;
    voices_[voice].have_adsr = true;
}

void SasCore::set_pause(std::uint32_t mask, bool paused) {
    for (std::uint32_t voice = 0; voice < kSasMaxVoices; ++voice)
        if ((mask & (1u << voice)) != 0u) voices_[voice].paused = paused;
}

void SasCore::key_on(std::uint32_t voice) {
    if (voice >= kSasMaxVoices) return;
    SasVoice &v = voices_[voice];
    v.block = 0u;
    v.loop_block = 0u;
    v.index = 28;
    v.phase = 0u;
    v.history1 = 0;
    v.history2 = 0;
    v.previous = 0;
    v.current = 0;
    v.source_ended = v.address == 0u || v.size < (v.pcm ? 2u : 16u);
    v.primed = false;
    v.paused = false;
    v.playing = !v.source_ended;
    v.envelope_counter = 0;
    if (tracing()) ++trace().key_ons;
    if (v.have_adsr && !envelopes_disabled()) {
        v.stage = EnvelopeStage::Attack;
        v.envelope = 0;
    } else {
        v.stage = EnvelopeStage::Sustain;
        v.envelope = kEnvelopeMax;
    }
}

void SasCore::key_off(std::uint32_t voice) {
    if (voice >= kSasMaxVoices) return;
    SasVoice &v = voices_[voice];
    if (!v.playing) return;
    if (v.have_adsr && !envelopes_disabled()) {
        v.stage = EnvelopeStage::Release;
        v.envelope_counter = 0;
    } else {
        v.playing = false;
        v.stage = EnvelopeStage::Off;
        v.envelope = 0;
    }
}

std::uint32_t SasCore::end_flag() const noexcept {
    std::uint32_t flags = 0u;
    for (std::uint32_t voice = 0; voice < kSasMaxVoices; ++voice)
        if (!voices_[voice].playing) flags |= 1u << voice;
    return flags;
}

std::int32_t SasCore::envelope_height(std::uint32_t voice) const noexcept {
    if (voice >= kSasMaxVoices) return 0;
    return voices_[voice].envelope << 15;
}

void SasCore::decode_block(const psprecomp::GuestMemory &memory, SasVoice &voice) {
    if (voice.block + 16u > voice.size) {
        voice.source_ended = true;
        return;
    }
    const std::uint8_t *block = memory.raw_pointer(voice.address + voice.block, 16u);
    if (block == nullptr) {
        voice.source_ended = true;
        return;
    }
    const std::uint32_t flags = block[1];
    // A terminator block carries no samples; everything else is decoded and the
    // flags only decide where playback goes next.
    if (flags == 7u) {
        voice.source_ended = true;
        return;
    }
    if ((flags & 4u) != 0u) voice.loop_block = voice.block;

    std::int32_t shift = block[0] & 0x0F;
    std::int32_t predictor = block[0] >> 4;
    if (predictor > 4) predictor = 0;
    if (shift > 12) shift = 9;
    for (std::int32_t sample = 0; sample < 28; ++sample) {
        const std::uint8_t byte = block[2 + sample / 2];
        const std::int32_t nibble = (sample & 1) != 0 ? (byte >> 4) : (byte & 0x0F);
        std::int32_t value = static_cast<std::int16_t>(nibble << 12) >> shift;
        value += (voice.history1 * kPredictorA[predictor] + voice.history2 * kPredictorB[predictor]) >> 6;
        value = clamp16(value);
        voice.decoded[static_cast<std::size_t>(sample)] = static_cast<std::int16_t>(value);
        voice.history2 = voice.history1;
        voice.history1 = value;
    }
    voice.index = 0;

    if ((flags & 1u) != 0u) {
        // Past this block playback either repeats or stops; a block offset at
        // the end of the sample makes the next decode report the end.
        voice.block = (flags & 2u) != 0u ? voice.loop_block : voice.size;
    } else {
        voice.block += 16u;
        if (voice.block + 16u > voice.size && voice.looping) voice.block = voice.loop_block;
    }
}

void SasCore::advance_source(const psprecomp::GuestMemory &memory, SasVoice &voice) {
    voice.previous = voice.current;
    if (voice.pcm) {
        const std::uint32_t offset = voice.block;
        if (offset + 2u > voice.size) {
            if (voice.looping) {
                voice.block = 0u;
            } else {
                voice.source_ended = true;
                voice.current = 0;
                return;
            }
        }
        const std::uint8_t *sample = memory.raw_pointer(voice.address + voice.block, 2u);
        voice.current = sample != nullptr ? static_cast<std::int16_t>(sample[0] | (sample[1] << 8))
                                          : static_cast<std::int16_t>(0);
        voice.block += 2u;
        return;
    }
    if (voice.index >= 28) {
        decode_block(memory, voice);
        if (voice.source_ended) {
            voice.current = 0;
            return;
        }
    }
    voice.current = voice.decoded[static_cast<std::size_t>(voice.index++)];
}

void SasCore::step_envelope(SasVoice &voice) {
    if (voice.stage == EnvelopeStage::Off) return;
    // Without an ADSR the guest drives the level with SetVolume alone, so the
    // envelope stays wide open.
    if (!voice.have_adsr || envelopes_disabled()) return;
    if (--voice.envelope_counter > 0) return;

    const std::uint32_t adsr1 = voice.adsr1;
    const std::uint32_t adsr2 = voice.adsr2;
    EnvelopeRate rate{};
    switch (voice.stage) {
    case EnvelopeStage::Attack:
        rate = decode_rate((adsr1 >> 8) & 0x7Fu, true, (adsr1 & 0x8000u) != 0u, voice.envelope);
        break;
    case EnvelopeStage::Decay:
        // Decay has no step field: the shift alone, always an exponential fall.
        rate = decode_rate(((adsr1 >> 4) & 0x0Fu) << 2u, false, true, voice.envelope);
        break;
    case EnvelopeStage::Sustain:
        rate = decode_rate((adsr2 >> 6) & 0x7Fu, (adsr2 & 0x4000u) == 0u, (adsr2 & 0x8000u) != 0u,
                           voice.envelope);
        break;
    case EnvelopeStage::Release:
        rate = decode_rate((adsr2 & 0x1Fu) << 2u, false, (adsr2 & 0x20u) != 0u, voice.envelope);
        break;
    case EnvelopeStage::Off:
        return;
    }
    voice.envelope_counter = rate.period;
    voice.envelope = std::clamp(voice.envelope + rate.step, 0, kEnvelopeMax);

    switch (voice.stage) {
    case EnvelopeStage::Attack:
        if (voice.envelope >= kEnvelopeMax) {
            voice.envelope = kEnvelopeMax;
            voice.stage = EnvelopeStage::Decay;
            voice.envelope_counter = 0;
        }
        break;
    case EnvelopeStage::Decay: {
        const std::int32_t sustain = std::min<std::int32_t>(
            (static_cast<std::int32_t>(adsr1 & 0x0Fu) + 1) * 0x800, kEnvelopeMax);
        if (voice.envelope <= sustain) {
            voice.envelope = sustain;
            voice.stage = EnvelopeStage::Sustain;
            voice.envelope_counter = 0;
        }
        break;
    }
    case EnvelopeStage::Release:
        if (voice.envelope <= 0) {
            voice.envelope = 0;
            voice.stage = EnvelopeStage::Off;
            voice.playing = false;
        }
        break;
    default:
        break;
    }
}

void SasCore::render(const psprecomp::GuestMemory &memory, std::int16_t *output, std::size_t frames) {
    std::fill_n(output, frames * 2u, static_cast<std::int16_t>(0));
    // Sampled before mixing: a short voice can finish inside this very block.
    if (tracing())
        trace().voices = std::max(trace().voices, static_cast<std::uint32_t>(std::popcount(
                                                      ~end_flag() & ((1u << max_voices_) - 1u))));
    for (std::uint32_t index = 0; index < max_voices_; ++index) {
        SasVoice &voice = voices_[index];
        if (!voice.playing || voice.paused) continue;

        // Keying on leaves the decoder empty, so the first two source samples
        // that the interpolator needs are fetched here.
        if (!voice.primed) {
            voice.primed = true;
            advance_source(memory, voice);
            advance_source(memory, voice);
        }

        for (std::size_t frame = 0; frame < frames; ++frame) {
            const std::int32_t span = voice.current - voice.previous;
            const std::int32_t sample =
                voice.previous + ((span * static_cast<std::int32_t>(voice.phase)) >> 12);
            step_envelope(voice);
            const std::int32_t scaled = (sample * voice.envelope) >> 15;
            const std::size_t slot = frame * 2u;
            output[slot] = static_cast<std::int16_t>(
                clamp16(output[slot] + ((scaled * voice.left) >> 12)));
            output[slot + 1u] = static_cast<std::int16_t>(
                clamp16(output[slot + 1u] + ((scaled * voice.right) >> 12)));

            voice.phase += voice.pitch;
            while (voice.phase >= kPitchUnity) {
                voice.phase -= kPitchUnity;
                advance_source(memory, voice);
            }
            if (voice.source_ended) {
                voice.playing = false;
                voice.stage = EnvelopeStage::Off;
                voice.envelope = 0;
                break;
            }
            if (!voice.playing) break;
        }
    }
    if (!tracing()) return;
    RenderTrace &stats = trace();
    stats.frames += frames;
    for (std::size_t sample = 0; sample < frames * 2u; ++sample)
        stats.peak = std::max(stats.peak, std::abs(static_cast<std::int32_t>(output[sample])));
    if (stats.frames >= 44'100u) {
        std::printf("[sas] frames=%llu peak=%d voices=%u key_ons=%llu\n",
                    static_cast<unsigned long long>(stats.frames), stats.peak, stats.voices,
                    static_cast<unsigned long long>(stats.key_ons));
        std::fflush(stdout);
        stats = RenderTrace{};
    }
}

SasCore &sas_core(std::uint32_t handle) {
    static std::map<std::uint32_t, SasCore> cores;
    return cores[handle];
}

} // namespace mhp3rd::audio
