#pragma once
// sceSasCore: software voice mixer (VAG ADPCM / PCM / noise voices, pitch,
// ADSR envelopes, volumes). Validation and envelope behaviour are checked
// against references/pspautotests/tests/audio/sascore/*.expected
// (tests/test_sascore.cpp); uOFW has no SAS sources.
#include <array>
#include <cstdint>
#include <map>

namespace psprecomp { class Runtime; class GuestMemory; }
namespace p3p3ds { class KernelState; }

namespace p3p3ds::hle {

namespace sas {
constexpr std::uint32_t kInvalidGrain = 0x80420001u, kInvalidMaxVoices = 0x80420002u, kInvalidOutputMode = 0x80420003u,
                        kInvalidSampleRate = 0x80420004u, kBadAddress = 0x80420005u, kInvalidVoice = 0x80420010u,
                        kInvalidNoiseFreq = 0x80420011u, kInvalidPitch = 0x80420012u, kInvalidAdsrCurve = 0x80420013u,
                        kInvalidParameter = 0x80420014u, kInvalidLoopPos = 0x80420015u, kVoicePaused = 0x80420016u,
                        kInvalidVolume = 0x80420018u, kInvalidAdsrRate = 0x80420019u;
constexpr std::int64_t kEnvelopeMax = 0x40000000;
enum Curve : std::int32_t { LinearIncrease = 0, LinearDecrease = 1, LinearBent = 2, ExponentDecrease = 3, ExponentIncrease = 4, Direct = 5 };
} // namespace sas

struct SasEnvelope {
    enum class State { Off, KeyOn, KeyOnStep, Attack, Decay, Sustain, Release };
    State state{State::Off};
    std::int64_t height{0};
    std::int32_t attack_type{sas::LinearIncrease}, decay_type{sas::LinearDecrease},
                 sustain_type{sas::LinearDecrease}, release_type{sas::LinearDecrease};
    std::int32_t attack_rate{0}, decay_rate{0}, sustain_rate{0}, release_rate{0};
    std::int64_t sustain_level{0};
    void step();
};

struct SasVoice {
    enum class Type { None, Vag, Pcm, Noise };
    Type type{Type::None};
    std::uint32_t address{}, size{};      // VAG bytes or PCM samples
    bool loop{};
    std::int32_t loop_position{};         // PCM loop start (samples)
    std::int32_t noise_frequency{};
    std::int32_t pitch{0x1000};
    std::int32_t left{}, right{}, effect_left{}, effect_right{};
    bool on{}, paused{}, ended{true};
    SasEnvelope envelope;
    // Playback state
    std::uint32_t read_offset{};          // VAG byte offset / PCM sample index
    std::uint32_t loop_start{};           // VAG loop block offset
    std::array<std::int16_t, 28> block{}; // decoded VAG block
    std::uint32_t block_index{28};
    std::int32_t s1{}, s2{};
    std::uint32_t pitch_accumulator{};
    std::int16_t current{};
    std::uint32_t noise_state{1};
};

struct SasCore {
    std::uint32_t grain{}, max_voices{}, output_mode{}, sample_rate{};
    std::array<SasVoice, 32> voices{};
    std::int32_t reverb_type{}, reverb_delay{}, reverb_feedback{}, reverb_left{}, reverb_right{};
    bool reverb_dry{}, reverb_wet{};
};

class SasState {
public:
    std::map<std::uint32_t, SasCore> cores; // keyed by guest SasCore address
    // Mixes one grain into `out` (interleaved stereo S16); with_mix adds to the
    // existing samples scaled by left/right volumes (0..0x1000).
    void mix(SasCore &core, psprecomp::GuestMemory &memory, std::uint32_t out, bool with_mix,
             std::int32_t mix_left, std::int32_t mix_right);
};

void register_sascore_module(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
