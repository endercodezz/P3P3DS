#pragma once

#include "psprecomp/guest_memory.hpp"

#include <array>
#include <cstdint>

namespace mhp3rd::audio {

inline constexpr std::uint32_t kSasMaxVoices = 32u;
// SAS envelope heights are 0..0x40000000 on hardware; the rate encoding it
// inherits from the SPU works in 15-bit steps, so the envelope is kept there
// and scaled up only when the guest asks to read it.
inline constexpr std::int32_t kEnvelopeMax = 0x7FFF;

enum class EnvelopeStage { Off, Attack, Decay, Sustain, Release };

struct SasVoice {
    // Source
    std::uint32_t address{};
    std::uint32_t size{};
    bool looping{};
    bool pcm{};  // raw 16-bit PCM instead of VAG
    std::uint32_t pitch{0x1000u};
    std::int32_t left{0x1000};
    std::int32_t right{0x1000};

    bool playing{};
    bool paused{};

    // VAG decoder position: `block` is a byte offset from `address`, `index`
    // the sample inside the 28-sample block it decoded, `phase` the 12-bit
    // fraction of a source sample the resampler has advanced past `index`.
    std::uint32_t block{};
    std::uint32_t loop_block{};
    std::int32_t index{28};
    std::uint32_t phase{};
    std::int32_t history1{};
    std::int32_t history2{};
    std::array<std::int16_t, 28> decoded{};
    std::int16_t previous{};
    std::int16_t current{};
    bool source_ended{};
    bool primed{};

    // Envelope
    bool have_adsr{};
    std::uint32_t adsr1{};
    std::uint32_t adsr2{};
    EnvelopeStage stage{EnvelopeStage::Off};
    std::int32_t envelope{};
    std::int32_t envelope_counter{};
};

// Software SAS: up to 32 VAG voices mixed at 44100 Hz. Reverb is not modelled,
// so the effect sends are accepted and ignored.
class SasCore {
public:
    void init(std::uint32_t grain, std::uint32_t max_voices, std::uint32_t output_mode);
    [[nodiscard]] std::uint32_t grain() const noexcept { return grain_; }
    [[nodiscard]] std::uint32_t output_mode() const noexcept { return output_mode_; }

    void set_voice(std::uint32_t voice, std::uint32_t address, std::uint32_t size, bool looping);
    void set_voice_pcm(std::uint32_t voice, std::uint32_t address, std::uint32_t size, std::int32_t loop);
    void set_pitch(std::uint32_t voice, std::uint32_t pitch);
    void set_volume(std::uint32_t voice, std::int32_t left, std::int32_t right);
    void set_simple_adsr(std::uint32_t voice, std::uint32_t adsr1, std::uint32_t adsr2);
    void set_pause(std::uint32_t mask, bool paused);
    void key_on(std::uint32_t voice);
    void key_off(std::uint32_t voice);
    // One bit per voice, set while the voice is *not* playing.
    [[nodiscard]] std::uint32_t end_flag() const noexcept;
    [[nodiscard]] std::int32_t envelope_height(std::uint32_t voice) const noexcept;

    // Renders `frames` stereo frames into `output`, overwriting it.
    void render(const psprecomp::GuestMemory &memory, std::int16_t *output, std::size_t frames);

private:
    void advance_source(const psprecomp::GuestMemory &memory, SasVoice &voice);
    void decode_block(const psprecomp::GuestMemory &memory, SasVoice &voice);
    void step_envelope(SasVoice &voice);

    std::uint32_t grain_{256u};
    std::uint32_t max_voices_{kSasMaxVoices};
    std::uint32_t output_mode_{};
    std::array<SasVoice, kSasMaxVoices> voices_{};
};

// One core per guest SAS handle. MHP3rd only ever opens one, but the handle is
// an argument to every call, so it is keyed rather than assumed.
[[nodiscard]] SasCore &sas_core(std::uint32_t handle);

} // namespace mhp3rd::audio
