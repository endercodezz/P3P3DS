// sceSasCore. Behaviour verified against pspautotests audio/sascore/*.expected
// (tests/test_sascore.cpp). The envelope state machine (32-step key-on delay,
// linear/bent/exponential curves, sustain level) reproduces the hardware
// heights in keyon/keyoff/adsrcurve.expected; PPSSPP's SasAudio.cpp was a
// behavioural cross-check only.
#include "p3p3ds/hle/sascore.hpp"

#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

#include <algorithm>
#include <vector>

namespace p3p3ds::hle {
namespace {
using namespace sas;

// VAG ADPCM prediction filters (PS SPU format), coefficients / 64.
constexpr std::int32_t kVagFilter[16][2] = {{0, 0}, {60, 0}, {115, -52}, {98, -55}, {122, -60}};

std::int32_t simple_rate(std::int32_t n) {
    n &= 0x7F;
    if (n == 0x7F) return 0;
    const std::int32_t rate = ((7 - (n & 3)) << 26) >> (n >> 2);
    return rate == 0 ? 1 : rate;
}
std::int32_t exponent_rate(std::int32_t n) {
    n &= 0x7F;
    if (n == 0x7F) return 0;
    const std::int32_t rate = ((7 - (n & 3)) << 24) >> (n >> 2);
    return rate == 0 ? 1 : rate;
}

void walk(std::int64_t &height, std::int32_t type, std::int32_t rate) {
    switch (type) {
    case LinearIncrease: height += rate; break;
    case LinearDecrease: height -= rate; break;
    case LinearBent: height += height <= kEnvelopeMax * 3 / 4 ? rate : rate / 4; break;
    case ExponentDecrease: {
        std::int64_t delta = height - kEnvelopeMax;
        delta += (-delta * static_cast<std::int64_t>(static_cast<std::uint32_t>(rate))) >> 32;
        height = delta + kEnvelopeMax - (static_cast<std::int64_t>(rate) + 3) / 4;
        break;
    }
    case ExponentIncrease: {
        std::int64_t delta = height - kEnvelopeMax;
        delta += (-delta * static_cast<std::int64_t>(static_cast<std::uint32_t>(rate))) >> 32;
        height = delta + 0x4000 + kEnvelopeMax;
        break;
    }
    case Direct: height = rate; break;
    default: break;
    }
}
} // namespace

void SasEnvelope::step() {
    auto clamp_and = [this](State next) { if (height > kEnvelopeMax) height = kEnvelopeMax; state = next; };
    switch (state) {
    case State::KeyOn: height = 0; state = State::KeyOnStep; break;
    case State::KeyOnStep:
        // The hardware holds the envelope at 0 for 32 samples after key on.
        if (++height >= 31) { height = 0; state = State::Attack; }
        break;
    case State::Attack:
        walk(height, attack_type, attack_rate);
        if (height >= kEnvelopeMax || height < 0) clamp_and(State::Decay);
        break;
    case State::Decay:
        walk(height, decay_type, decay_rate);
        if (height < sustain_level) clamp_and(State::Sustain);
        break;
    case State::Sustain:
        walk(height, sustain_type, sustain_rate);
        if (height <= 0) { height = 0; clamp_and(State::Release); }
        break;
    case State::Release:
        walk(height, release_type, release_rate);
        if (height <= 0) { height = 0; state = State::Off; }
        break;
    case State::Off: break;
    }
}

namespace {
std::int16_t next_sample(SasVoice &v, psprecomp::GuestMemory &memory) {
    switch (v.type) {
    case SasVoice::Type::Pcm: {
        if (v.read_offset >= v.size) {
            if (v.loop_position >= 0 && v.loop) v.read_offset = static_cast<std::uint32_t>(v.loop_position);
            else { v.ended = true; return 0; }
        }
        const auto sample = static_cast<std::int16_t>(memory.load16(v.address + 2u * v.read_offset));
        ++v.read_offset;
        return sample;
    }
    case SasVoice::Type::Vag: {
        if (v.block_index >= 28u) {
            if (v.ended || v.read_offset + 16u > v.size) { v.ended = true; return 0; }
            const std::uint32_t at = v.address + v.read_offset;
            const auto header = memory.load8(at);
            const auto flags = memory.load8(at + 1u);
            const std::int32_t shift = header & 0xF;
            const auto filter = std::min<std::uint32_t>(header >> 4, 4u);
            if ((flags & 4u) != 0u) v.loop_start = v.read_offset;
            for (std::uint32_t i = 0; i < 28u; ++i) {
                const auto byte = memory.load8(at + 2u + i / 2u);
                std::int32_t nibble = (i & 1u) ? (byte >> 4) : (byte & 0xF);
                nibble = static_cast<std::int16_t>(nibble << 12) >> shift;
                std::int32_t s = nibble + ((v.s1 * kVagFilter[filter][0] + v.s2 * kVagFilter[filter][1]) >> 6);
                s = std::clamp<std::int32_t>(s, -32768, 32767);
                v.s2 = v.s1;
                v.s1 = s;
                v.block[i] = static_cast<std::int16_t>(s);
            }
            v.block_index = 0;
            v.read_offset += 16u;
            if ((flags & 1u) != 0u) { // loop end / sample end
                if (v.loop && (flags & 2u) != 0u) v.read_offset = v.loop_start;
                else v.read_offset = v.size; // ends after this block
            }
        }
        return v.block[v.block_index++];
    }
    case SasVoice::Type::Noise: {
        // [INFERRED] 16-bit LFSR noise; only presence/length is observable here.
        v.noise_state = (v.noise_state >> 1) ^ (-(v.noise_state & 1u) & 0xB400u);
        return static_cast<std::int16_t>(v.noise_state);
    }
    default: return 0;
    }
}
} // namespace

void SasState::mix(SasCore &core, psprecomp::GuestMemory &memory, std::uint32_t out, bool with_mix,
                   std::int32_t mix_left, std::int32_t mix_right) {
    std::vector<std::int32_t> acc(core.grain * 2u, 0);
    for (std::uint32_t vi = 0; vi < core.max_voices; ++vi) {
        auto &v = core.voices[vi];
        if (!v.on || v.paused) continue;
        for (std::uint32_t i = 0; i < core.grain; ++i) {
            v.envelope.step();
            v.pitch_accumulator += static_cast<std::uint32_t>(v.pitch);
            while (v.pitch_accumulator >= 0x1000u) { v.pitch_accumulator -= 0x1000u; v.current = next_sample(v, memory); }
            // [INFERRED] envelope height 0x40000000 = unity, volume 0x1000 = unity.
            const std::int64_t s = (static_cast<std::int64_t>(v.current) * (v.envelope.height >> 15)) >> 15;
            acc[2 * i] += static_cast<std::int32_t>((s * v.left) >> 12);
            acc[2 * i + 1] += static_cast<std::int32_t>((s * v.right) >> 12);
        }
        // A voice ends when its data runs out or its release reaches zero.
        if (v.ended || v.envelope.state == SasEnvelope::State::Off) {
            v.ended = true;
            v.on = false;
            v.envelope.state = SasEnvelope::State::Off;
            v.envelope.height = 0;
        }
    }
    for (std::uint32_t i = 0; i < core.grain * 2u; ++i) {
        std::int32_t value = acc[i];
        if (with_mix) {
            const auto existing = static_cast<std::int16_t>(memory.load16(out + 2u * i));
            value += (existing * ((i & 1u) ? mix_right : mix_left)) >> 12;
        }
        memory.store16(out + 2u * i, static_cast<std::uint16_t>(std::clamp<std::int32_t>(value, -32768, 32767)));
    }
}

void register_sascore_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    auto &sas = kernel.sas();
    const auto reg = [&runtime](std::uint32_t nid, psprecomp::Runtime::HleFunction fn) {
        runtime.register_hle("sceSasCore", nid, std::move(fn));
    };
    auto s = [](std::uint32_t v) { return static_cast<std::int32_t>(v); };
    // Resolves (core, voice) with the hardware's error codes.
    auto voice_of = [&sas](std::uint32_t address, std::uint32_t voice, std::uint32_t &error) -> SasVoice * {
        auto it = sas.cores.find(address);
        if (it == sas.cores.end()) { error = kBadAddress; return nullptr; }
        if (voice >= 32u) { error = kInvalidVoice; return nullptr; }
        return &it->second.voices[voice];
    };

    reg(0x42778A9Fu, [&sas, s](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // __sceSasInit
        const auto address = ctx.gpr[4];
        const auto grain = s(ctx.gpr[5]), voices = s(ctx.gpr[6]), mode = s(ctx.gpr[7]), rate = s(ctx.gpr[8]);
        // sascore.expected: NULL/unaligned core, then grain 0x40..0x800 in steps
        // of 32, 1..32 voices, output mode 0/1, 44100 Hz only.
        std::uint32_t r = 0;
        if (address == 0u || (address & 63u) != 0u || !rt.memory().contains(address, 64u)) r = kBadAddress;
        else if (grain < 0x40 || grain > 0x800 || (grain & 0x1F) != 0) r = kInvalidGrain;
        else if (voices < 1 || voices > 32) r = kInvalidMaxVoices;
        else if (mode != 0 && mode != 1) r = kInvalidOutputMode;
        else if (rate != 44100) r = kInvalidSampleRate;
        if (r == 0u) {
            auto &core = sas.cores[address];
            core = SasCore{};
            core.grain = static_cast<std::uint32_t>(grain);
            core.max_voices = static_cast<std::uint32_t>(voices);
            core.output_mode = static_cast<std::uint32_t>(mode);
            core.sample_rate = static_cast<std::uint32_t>(rate);
        }
        rt.event("sas_init", {{"core", address}, {"grain", ctx.gpr[5]}, {"voices", ctx.gpr[6]}, {"mode", ctx.gpr[7]}, {"result", r}});
        ctx.set_gpr(2, r);
    });
    reg(0xA3589D81u, [&sas](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // __sceSasCore
        auto it = sas.cores.find(ctx.gpr[4]);
        if (it == sas.cores.end()) { ctx.set_gpr(2, kBadAddress); return; }
        if (!rt.memory().contains(ctx.gpr[5], it->second.grain * 4u)) { ctx.set_gpr(2, kBadAddress); return; }
        sas.mix(it->second, rt.memory(), ctx.gpr[5], false, 0, 0);
        ctx.set_gpr(2, 0u);
    });
    reg(0x50A14DFCu, [&sas, s](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // __sceSasCoreWithMix
        auto it = sas.cores.find(ctx.gpr[4]);
        if (it == sas.cores.end()) { ctx.set_gpr(2, kBadAddress); return; }
        if (!rt.memory().contains(ctx.gpr[5], it->second.grain * 4u)) { ctx.set_gpr(2, kBadAddress); return; }
        sas.mix(it->second, rt.memory(), ctx.gpr[5], true, s(ctx.gpr[6]), s(ctx.gpr[7]));
        ctx.set_gpr(2, 0u);
    });
    reg(0x99944089u, [voice_of, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetVoice
        std::uint32_t e = 0; auto *v = voice_of(ctx.gpr[4], ctx.gpr[5], e);
        if (!v) { ctx.set_gpr(2, e); return; }
        const auto size = ctx.gpr[7];
        const auto loop = s(ctx.gpr[8]);
        // vag.expected: size must be a non-zero multiple of 16; loop 0/1.
        if (size == 0u || (size & 0xFu) != 0u || (loop != 0 && loop != 1)) { ctx.set_gpr(2, kInvalidParameter); return; }
        v->type = SasVoice::Type::Vag;
        v->address = ctx.gpr[6]; v->size = size; v->loop = loop != 0;
        v->read_offset = 0; v->block_index = 28; v->s1 = v->s2 = 0; v->loop_start = 0; v->ended = false;
        ctx.set_gpr(2, 0u);
    });
    reg(0xE1CD9561u, [voice_of, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetVoicePCM
        std::uint32_t e = 0; auto *v = voice_of(ctx.gpr[4], ctx.gpr[5], e);
        if (!v) { ctx.set_gpr(2, e); return; }
        const auto size = s(ctx.gpr[7]);
        const auto loop = s(ctx.gpr[8]);
        // pcm.expected: 1..65536 samples (else 0x8042001A); loop position < size or -1.
        if (size <= 0 || size > 0x10000) { ctx.set_gpr(2, 0x8042001Au); return; }
        if (loop >= size) { ctx.set_gpr(2, kInvalidLoopPos); return; }
        v->type = SasVoice::Type::Pcm;
        v->address = ctx.gpr[6]; v->size = static_cast<std::uint32_t>(size);
        v->loop_position = loop; v->loop = loop >= 0; v->read_offset = 0; v->ended = false;
        ctx.set_gpr(2, 0u);
    });
    reg(0xB7660A23u, [voice_of, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetNoise
        std::uint32_t e = 0; auto *v = voice_of(ctx.gpr[4], ctx.gpr[5], e);
        if (!v) { ctx.set_gpr(2, e); return; }
        const auto freq = s(ctx.gpr[6]);
        if (freq < 0 || freq >= 64) { ctx.set_gpr(2, kInvalidNoiseFreq); return; } // noise.expected
        v->type = SasVoice::Type::Noise; v->noise_frequency = freq; v->ended = false;
        ctx.set_gpr(2, 0u);
    });
    reg(0xAD84D37Fu, [voice_of, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetPitch
        std::uint32_t e = 0; auto *v = voice_of(ctx.gpr[4], ctx.gpr[5], e);
        if (!v) { ctx.set_gpr(2, e); return; }
        const auto pitch = s(ctx.gpr[6]);
        if (pitch < 0 || pitch > 0x4000) { ctx.set_gpr(2, kInvalidPitch); return; } // pitch.expected
        v->pitch = pitch;
        ctx.set_gpr(2, 0u);
    });
    reg(0x440CA7D8u, [voice_of, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetVolume
        std::uint32_t e = 0; auto *v = voice_of(ctx.gpr[4], ctx.gpr[5], e);
        if (!v) { ctx.set_gpr(2, e); return; }
        const std::int32_t vols[4] = {s(ctx.gpr[6]), s(ctx.gpr[7]), s(ctx.gpr[8]), s(ctx.gpr[9])};
        for (auto vol : vols) if (vol < -0x1000 || vol > 0x1000) { ctx.set_gpr(2, kInvalidVolume); return; }
        v->left = vols[0]; v->right = vols[1]; v->effect_left = vols[2]; v->effect_right = vols[3];
        ctx.set_gpr(2, 0u);
    });
    reg(0x5F9529F6u, [voice_of, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetSL
        std::uint32_t e = 0; auto *v = voice_of(ctx.gpr[4], ctx.gpr[5], e);
        if (!v) { ctx.set_gpr(2, e); return; }
        v->envelope.sustain_level = s(ctx.gpr[6]);
        ctx.set_gpr(2, 0u);
    });
    reg(0x019B25EBu, [voice_of, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetADSR
        std::uint32_t e = 0; auto *v = voice_of(ctx.gpr[4], ctx.gpr[5], e);
        if (!v) { ctx.set_gpr(2, e); return; }
        const auto flag = ctx.gpr[6];
        const std::int32_t rates[4] = {s(ctx.gpr[7]), s(ctx.gpr[8]), s(ctx.gpr[9]), s(ctx.gpr[10])};
        for (int i = 0; i < 4; ++i) if ((flag & (1u << i)) && rates[i] < 0) { ctx.set_gpr(2, kInvalidAdsrRate); return; }
        auto &env = v->envelope;
        if (flag & 1u) env.attack_rate = rates[0];
        if (flag & 2u) env.decay_rate = rates[1];
        if (flag & 4u) env.sustain_rate = rates[2];
        if (flag & 8u) env.release_rate = rates[3];
        ctx.set_gpr(2, 0u);
    });
    reg(0x9EC3676Au, [voice_of, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetADSRmode
        std::uint32_t e = 0; auto *v = voice_of(ctx.gpr[4], ctx.gpr[5], e);
        if (!v) { ctx.set_gpr(2, e); return; }
        const auto flag = ctx.gpr[6];
        const std::int32_t modes[4] = {s(ctx.gpr[7]), s(ctx.gpr[8]), s(ctx.gpr[9]), s(ctx.gpr[10])};
        // setadsr.expected: attack {0,2,4}, decay/release {1,3,5}, sustain {0..5}.
        const auto valid = [](int which, std::int32_t m) {
            if (which == 0) return m == LinearIncrease || m == LinearBent || m == ExponentIncrease;
            if (which == 2) return m >= 0 && m <= 5;
            return m == LinearDecrease || m == ExponentDecrease || m == Direct;
        };
        for (int i = 0; i < 4; ++i) if ((flag & (1u << i)) && !valid(i, modes[i])) { ctx.set_gpr(2, kInvalidAdsrCurve); return; }
        auto &env = v->envelope;
        if (flag & 1u) env.attack_type = modes[0];
        if (flag & 2u) env.decay_type = modes[1];
        if (flag & 4u) env.sustain_type = modes[2];
        if (flag & 8u) env.release_type = modes[3];
        ctx.set_gpr(2, 0u);
    });
    reg(0xCBCD4F79u, [voice_of](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetSimpleADSR
        std::uint32_t e = 0; auto *v = voice_of(ctx.gpr[4], ctx.gpr[5], e);
        if (!v) { ctx.set_gpr(2, e); return; }
        const auto env1 = static_cast<std::int32_t>(ctx.gpr[6] & 0xFFFFu), env2 = static_cast<std::int32_t>(ctx.gpr[7] & 0xFFFFu);
        // setadsr.expected: bit 13 of env2 is an unknown/invalid value.
        if ((env2 & 0x2000) != 0) { ctx.set_gpr(2, kInvalidAdsrCurve); return; }
        auto &env = v->envelope;
        env.attack_rate = simple_rate(env1 >> 8);
        env.attack_type = (env1 & 0x8000) == 0 ? LinearIncrease : LinearBent;
        const auto decay = (env1 >> 4) & 0xF;
        env.decay_rate = decay == 0 ? 0x7FFFFFFF : static_cast<std::int32_t>(0x80000000u >> decay);
        env.decay_type = ExponentDecrease;
        env.sustain_type = (env2 >> 14) & 3;
        env.sustain_rate = env.sustain_type == ExponentDecrease ? exponent_rate(env2 >> 6) : simple_rate(env2 >> 6);
        env.release_type = (env2 & 0x20) == 0 ? LinearDecrease : ExponentDecrease;
        const auto n = env2 & 0x1F;
        if (n == 31) env.release_rate = 0;
        else if (env.release_type == LinearDecrease) env.release_rate = n == 30 ? 0x40000000 : n == 29 ? 1 : (0x10000000 >> n);
        else env.release_rate = static_cast<std::int32_t>(0x80000000u >> n);
        env.sustain_level = static_cast<std::int64_t>((env1 & 0xF) + 1) << 26;
        ctx.set_gpr(2, 0u);
    });
    reg(0x76F01ACAu, [voice_of](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetKeyOn
        std::uint32_t e = 0; auto *v = voice_of(ctx.gpr[4], ctx.gpr[5], e);
        if (!v) { ctx.set_gpr(2, e); return; }
        // keyon.expected: a key on still pending (no __sceSasCore since), or one
        // while paused, fails; after a core it restarts a playing voice.
        if (v->paused || (v->on && v->envelope.state == SasEnvelope::State::KeyOn)) { ctx.set_gpr(2, kVoicePaused); return; }
        v->on = true; v->ended = false;
        v->read_offset = 0; v->block_index = 28; v->s1 = v->s2 = 0; v->pitch_accumulator = 0; v->current = 0;
        v->envelope.state = SasEnvelope::State::KeyOn;
        ctx.set_gpr(2, 0u);
    });
    reg(0xA0CF2FA4u, [voice_of](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetKeyOff
        std::uint32_t e = 0; auto *v = voice_of(ctx.gpr[4], ctx.gpr[5], e);
        if (!v) { ctx.set_gpr(2, e); return; }
        // keyoff.expected: only a playing, unpaused voice can be keyed off.
        if (v->paused || !v->on) { ctx.set_gpr(2, kVoicePaused); return; }
        v->envelope.state = SasEnvelope::State::Release;
        ctx.set_gpr(2, 0u);
    });
    reg(0x787D04D5u, [&sas](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetPause
        auto it = sas.cores.find(ctx.gpr[4]);
        if (it == sas.cores.end()) { ctx.set_gpr(2, kBadAddress); return; }
        for (std::uint32_t i = 0; i < 32u; ++i)
            if (ctx.gpr[5] & (1u << i)) it->second.voices[i].paused = ctx.gpr[6] != 0u;
        ctx.set_gpr(2, 0u);
    });
    reg(0x2C8E6AB3u, [&sas](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasGetPauseFlag
        auto it = sas.cores.find(ctx.gpr[4]);
        if (it == sas.cores.end()) { ctx.set_gpr(2, kBadAddress); return; }
        std::uint32_t mask = 0;
        for (std::uint32_t i = 0; i < 32u; ++i) if (it->second.voices[i].paused) mask |= 1u << i;
        ctx.set_gpr(2, mask);
    });
    reg(0x68A46B95u, [&sas](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasGetEndFlag
        auto it = sas.cores.find(ctx.gpr[4]);
        if (it == sas.cores.end()) { ctx.set_gpr(2, kBadAddress); return; }
        std::uint32_t mask = 0;
        for (std::uint32_t i = 0; i < 32u; ++i) if (!it->second.voices[i].on) mask |= 1u << i;
        ctx.set_gpr(2, mask);
    });
    reg(0x74AE582Au, [voice_of](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasGetEnvelopeHeight
        std::uint32_t e = 0; auto *v = voice_of(ctx.gpr[4], ctx.gpr[5], e);
        if (!v) { ctx.set_gpr(2, e); return; }
        ctx.set_gpr(2, static_cast<std::uint32_t>(v->envelope.height));
    });
    reg(0x07F58C24u, [&sas](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // __sceSasGetAllEnvelopeHeights
        auto it = sas.cores.find(ctx.gpr[4]);
        if (it == sas.cores.end()) { ctx.set_gpr(2, kBadAddress); return; }
        if (!rt.memory().contains(ctx.gpr[5], 32u * 4u)) { ctx.set_gpr(2, 0x800200D3u); return; }
        for (std::uint32_t i = 0; i < 32u; ++i)
            rt.memory().store32(ctx.gpr[5] + 4u * i, static_cast<std::uint32_t>(it->second.voices[i].envelope.height));
        ctx.set_gpr(2, 0u);
    });
    reg(0xBD11B7C2u, [&sas](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasGetGrain
        auto it = sas.cores.find(ctx.gpr[4]);
        ctx.set_gpr(2, it == sas.cores.end() ? kBadAddress : it->second.grain);
    });
    reg(0xD1E0A01Eu, [&sas, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetGrain
        auto it = sas.cores.find(ctx.gpr[4]);
        if (it == sas.cores.end()) { ctx.set_gpr(2, kBadAddress); return; }
        const auto grain = s(ctx.gpr[5]);
        if (grain < 0x40 || grain > 0x800 || (grain & 0x1F) != 0) { ctx.set_gpr(2, kInvalidGrain); return; }
        it->second.grain = static_cast<std::uint32_t>(grain);
        ctx.set_gpr(2, 0u);
    });
    reg(0xE175EF66u, [&sas](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasGetOutputmode
        auto it = sas.cores.find(ctx.gpr[4]);
        ctx.set_gpr(2, it == sas.cores.end() ? kBadAddress : it->second.output_mode);
    });
    reg(0xE855BF76u, [&sas](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasSetOutputmode
        auto it = sas.cores.find(ctx.gpr[4]);
        if (it == sas.cores.end()) { ctx.set_gpr(2, kBadAddress); return; }
        if (ctx.gpr[5] > 1u) { ctx.set_gpr(2, kInvalidOutputMode); return; }
        it->second.output_mode = ctx.gpr[5];
        ctx.set_gpr(2, 0u);
    });
    // Reverb parameters are stored; [UNVERIFIED] no reverb effect is mixed yet.
    reg(0x33D4AB37u, [&sas, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasRevType
        auto it = sas.cores.find(ctx.gpr[4]);
        if (it == sas.cores.end()) { ctx.set_gpr(2, kBadAddress); return; }
        it->second.reverb_type = s(ctx.gpr[5]); ctx.set_gpr(2, 0u);
    });
    reg(0x267A6DD2u, [&sas, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasRevParam
        auto it = sas.cores.find(ctx.gpr[4]);
        if (it == sas.cores.end()) { ctx.set_gpr(2, kBadAddress); return; }
        it->second.reverb_delay = s(ctx.gpr[5]); it->second.reverb_feedback = s(ctx.gpr[6]); ctx.set_gpr(2, 0u);
    });
    reg(0xD5A229C9u, [&sas, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasRevEVOL
        auto it = sas.cores.find(ctx.gpr[4]);
        if (it == sas.cores.end()) { ctx.set_gpr(2, kBadAddress); return; }
        it->second.reverb_left = s(ctx.gpr[5]); it->second.reverb_right = s(ctx.gpr[6]); ctx.set_gpr(2, 0u);
    });
    reg(0xF983B186u, [&sas](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // __sceSasRevVON
        auto it = sas.cores.find(ctx.gpr[4]);
        if (it == sas.cores.end()) { ctx.set_gpr(2, kBadAddress); return; }
        it->second.reverb_dry = ctx.gpr[5] != 0u; it->second.reverb_wet = ctx.gpr[6] != 0u; ctx.set_gpr(2, 0u);
    });
}

} // namespace p3p3ds::hle
