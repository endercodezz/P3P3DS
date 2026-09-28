#include "hle/psp_hle.h"
#include "psp_memory.h"
#include "recomp.h"

#include <cstdint>
#include <cstring>
#include <mutex>

// ================================================================
// Minimal SAS (Software Audio Synthesis) voice state machine.
// Issue #29: timing/state only -- NO audio rendering.
//
// The game's sound-completion gate polls a per-voice active byte
// that only clears when __sceSasGetEndFlag reports the voice ended.
// With the previous all-no-op stubs no voice ever ended, so the
// post-menu PCM pipeline stalled forever. This module tracks voice
// lifetime: __sceSasCore consumes grain*pitch/0x1000 samples per
// call and flips voices to "ended" when their VAG data runs out.
//
// Argument convention: PSP syscalls pass a0..a3 in ctx->r[4..7] and
// the 5th+ args in t0.. (ctx->r[8..]) -- same as sceIoLseek's whence
// in psp_hle_io.cpp.
// ================================================================

namespace {

constexpr int SAS_VOICE_COUNT = 32;
constexpr int SAS_PITCH_BASE = 0x1000;   // pitch 0x1000 = 1.0x
constexpr int SAS_PITCH_MAX = 0x4000;
constexpr int SAS_GRAIN_MAX = 2048;
constexpr int32_t SAS_ENVELOPE_PLAYING = 0x10000000;

struct SasVoice {
    bool on = false;        // key-on state
    bool playing = false;   // still producing samples
    bool paused = false;
    uint32_t vag_addr = 0;
    uint32_t vag_size = 0;
    int loop = 0;
    uint32_t pitch = SAS_PITCH_BASE;
    int64_t remaining_samples = 0;
};

SasVoice g_voices[SAS_VOICE_COUNT];
int g_grain = 256;
// SAS calls come from the PCM thread while voice setup may come from
// other game threads -- guard all state with one mutex.
std::mutex g_sas_mtx;

/// VAG: each 16-byte block decodes to 28 PCM samples.
int64_t vag_total_samples(uint32_t vag_size) {
    return (static_cast<int64_t>(vag_size) / 16) * 28;
}

/// Voice index is always a1. Returns nullptr for out-of-range voices
/// (tolerated as no-op, matching the codebase's stub philosophy).
SasVoice* voice_arg(recomp_context* ctx) {
    uint32_t v = static_cast<uint32_t>(ctx->r[5]);
    if (v >= SAS_VOICE_COUNT) return nullptr;
    return &g_voices[v];
}

/// Advance every playing voice by one grain (scaled by pitch).
/// Caller must hold g_sas_mtx.
void sas_advance_voices() {
    for (auto& v : g_voices) {
        if (!v.playing || v.paused) continue;
        v.remaining_samples -=
            static_cast<int64_t>(g_grain) * v.pitch / SAS_PITCH_BASE;
        if (v.remaining_samples > 0) continue;
        if (v.loop != 0) {
            v.remaining_samples = vag_total_samples(v.vag_size);
        } else {
            v.playing = false;
            v.on = false;
        }
    }
}

/// Zero the output buffer: grain samples x 2 channels x 2 bytes.
void sas_zero_output(uint8_t* rdram, uint32_t out_addr) {
    if (out_addr == 0) return;
    uint32_t bytes = static_cast<uint32_t>(g_grain) * 4U;
    uint32_t off = out_addr & PSP_ADDR_MASK;
    if (off + bytes > PSP_MEM_SIZE) {
        bytes = PSP_MEM_SIZE - off;  // clamp at top of guest memory
    }
    std::memset(rdram + off, 0, bytes);
}

// ---- HLE handlers ----

void hle_sceSasInit(uint8_t* rdram, recomp_context* ctx) {
    // (core=a0, grain=a1, maxVoices=a2, outMode=a3, sampleRate=t0)
    int32_t grain = ctx->r[5];
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    for (auto& v : g_voices) v = SasVoice{};
    if (grain > 0 && grain <= SAS_GRAIN_MAX) g_grain = grain;
    ctx->r[2] = 0;
    (void)rdram;
}

void hle_sceSasSetGrain(uint8_t* rdram, recomp_context* ctx) {
    int32_t grain = ctx->r[5];
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    if (grain > 0 && grain <= SAS_GRAIN_MAX) g_grain = grain;
    ctx->r[2] = 0;
    (void)rdram;
}

void hle_sceSasGetGrain(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    ctx->r[2] = g_grain;
    (void)rdram;
}

void hle_sceSasSetVoice(uint8_t* rdram, recomp_context* ctx) {
    // (core=a0, voice=a1, vagAddr=a2, size=a3, loop=t0)
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    SasVoice* v = voice_arg(ctx);
    if (v != nullptr) {
        v->vag_addr = static_cast<uint32_t>(ctx->r[6]);
        v->vag_size = static_cast<uint32_t>(ctx->r[7]);
        v->loop = ctx->r[8];
        v->remaining_samples = vag_total_samples(v->vag_size);
        if (v->on) v->playing = true;
    }
    ctx->r[2] = 0;
    (void)rdram;
}

void hle_sceSasSetPitch(uint8_t* rdram, recomp_context* ctx) {
    // (core=a0, voice=a1, pitch=a2)
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    SasVoice* v = voice_arg(ctx);
    if (v != nullptr) {
        int32_t pitch = ctx->r[6];
        if (pitch < 1) pitch = 1;
        if (pitch > SAS_PITCH_MAX) pitch = SAS_PITCH_MAX;
        v->pitch = static_cast<uint32_t>(pitch);
    }
    ctx->r[2] = 0;
    (void)rdram;
}

void hle_sceSasSetKeyOn(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    SasVoice* v = voice_arg(ctx);
    if (v != nullptr) {
        v->on = true;
        v->playing = true;
        if (v->remaining_samples <= 0) {
            v->remaining_samples = vag_total_samples(v->vag_size);
        }
    }
    ctx->r[2] = 0;
    (void)rdram;
}

void hle_sceSasSetKeyOff(uint8_t* rdram, recomp_context* ctx) {
    // Skip the release envelope: end the voice immediately.
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    SasVoice* v = voice_arg(ctx);
    if (v != nullptr) {
        v->on = false;
        v->playing = false;
    }
    ctx->r[2] = 0;
    (void)rdram;
}

void hle_sceSasCore(uint8_t* rdram, recomp_context* ctx) {
    // (core=a0, out=a1)
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    sas_zero_output(rdram, static_cast<uint32_t>(ctx->r[5]));
    sas_advance_voices();
    ctx->r[2] = 0;
}

void hle_sceSasCoreWithMix(uint8_t* rdram, recomp_context* ctx) {
    // (core=a0, inout=a1, leftVol=a2, rightVol=a3)
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    sas_zero_output(rdram, static_cast<uint32_t>(ctx->r[5]));
    sas_advance_voices();
    ctx->r[2] = 0;
}

void hle_sceSasGetEndFlag(uint8_t* rdram, recomp_context* ctx) {
    // Bit i set iff voice i is NOT playing (initially 0xFFFFFFFF).
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    uint32_t mask = 0;
    for (int i = 0; i < SAS_VOICE_COUNT; i++) {
        if (!g_voices[i].playing) mask |= (1U << i);
    }
    ctx->r[2] = static_cast<int32_t>(mask);
    (void)rdram;
}

void hle_sceSasGetPauseFlag(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    uint32_t mask = 0;
    for (int i = 0; i < SAS_VOICE_COUNT; i++) {
        if (g_voices[i].paused) mask |= (1U << i);
    }
    ctx->r[2] = static_cast<int32_t>(mask);
    (void)rdram;
}

void hle_sceSasSetPause(uint8_t* rdram, recomp_context* ctx) {
    // (core=a0, voiceMask=a1, pause=a2)
    uint32_t voice_mask = static_cast<uint32_t>(ctx->r[5]);
    bool pause = ctx->r[6] != 0;
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    for (int i = 0; i < SAS_VOICE_COUNT; i++) {
        if (voice_mask & (1U << i)) g_voices[i].paused = pause;
    }
    ctx->r[2] = 0;
    (void)rdram;
}

void hle_sceSasGetEnvelopeHeight(uint8_t* rdram, recomp_context* ctx) {
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    SasVoice* v = voice_arg(ctx);
    ctx->r[2] = (v != nullptr && v->playing) ? SAS_ENVELOPE_PLAYING : 0;
    (void)rdram;
}

void hle_sceSasGetAllEnvelopeHeights(
    uint8_t* rdram, recomp_context* ctx
) {
    // (core=a0, heightsPtr=a1) -- write 32 ints.
    uint32_t ptr = static_cast<uint32_t>(ctx->r[5]);
    std::lock_guard<std::mutex> lock(g_sas_mtx);
    if (ptr != 0) {
        for (int i = 0; i < SAS_VOICE_COUNT; i++) {
            psp_mem_write<int32_t>(rdram, ptr + i * 4U,
                g_voices[i].playing ? SAS_ENVELOPE_PLAYING : 0);
        }
    }
    ctx->r[2] = 0;
}

/// Remaining SAS NIDs stay no-op 0 (volume/ADSR/noise/reverb/outmode
/// have no observable effect without audio rendering).
void hle_sas_noop(uint8_t* rdram, recomp_context* ctx) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

}  // namespace

// ---- Registration ----

void psp_hle_register_sas() {
    psp_hle_register("__sceSasInit", hle_sceSasInit);
    psp_hle_register("__sceSasCore", hle_sceSasCore);
    psp_hle_register("__sceSasCoreWithMix", hle_sceSasCoreWithMix);
    psp_hle_register("__sceSasSetVoice", hle_sceSasSetVoice);
    psp_hle_register("__sceSasSetPitch", hle_sceSasSetPitch);
    psp_hle_register("__sceSasSetKeyOn", hle_sceSasSetKeyOn);
    psp_hle_register("__sceSasSetKeyOff", hle_sceSasSetKeyOff);
    psp_hle_register("__sceSasSetPause", hle_sceSasSetPause);
    psp_hle_register("__sceSasSetGrain", hle_sceSasSetGrain);
    psp_hle_register("__sceSasGetGrain", hle_sceSasGetGrain);
    psp_hle_register("__sceSasGetEndFlag", hle_sceSasGetEndFlag);
    psp_hle_register("__sceSasGetPauseFlag", hle_sceSasGetPauseFlag);
    psp_hle_register("__sceSasGetEnvelopeHeight",
                      hle_sceSasGetEnvelopeHeight);
    psp_hle_register("__sceSasGetAllEnvelopeHeights",
                      hle_sceSasGetAllEnvelopeHeights);
    // No-ops (no audible effect without rendering)
    psp_hle_register("__sceSasSetVolume", hle_sas_noop);
    psp_hle_register("__sceSasSetADSR", hle_sas_noop);
    psp_hle_register("__sceSasSetADSRmode", hle_sas_noop);
    psp_hle_register("__sceSasSetSL", hle_sas_noop);
    psp_hle_register("__sceSasSetSimpleADSR", hle_sas_noop);
    psp_hle_register("__sceSasSetNoise", hle_sas_noop);
    psp_hle_register("__sceSasRevParam", hle_sas_noop);
    psp_hle_register("__sceSasRevType", hle_sas_noop);
    psp_hle_register("__sceSasRevEVOL", hle_sas_noop);
    psp_hle_register("__sceSasRevVON", hle_sas_noop);
    psp_hle_register("__sceSasGetOutputmode", hle_sas_noop);
    psp_hle_register("__sceSasSetOutputmode", hle_sas_noop);
}
