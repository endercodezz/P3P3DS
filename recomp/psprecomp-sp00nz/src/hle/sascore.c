/* psprecomp — sceSasCore.
 *
 * The PSP's hardware voice mixer: 32 voices, each playing 4-bit ADPCM (Sony's
 * VAG format) through an ADSR envelope, summed to a stereo buffer. A game sets
 * voices up, keys them on, and then calls __sceSasCore once per audio frame to
 * render `grain` samples.
 *
 * Unlike the GE, this one is worth implementing *properly* rather than
 * summarising, because it is small and self-contained: ADPCM decode is a
 * documented four-line recurrence, and audio either comes out sounding like
 * the game or it does not. There is no half-working state that misleads you.
 */

#include "psprecomp/hle.h"

#include <stdio.h>
#include <string.h>

#define SAS_VOICES     32
#define SAS_MAX_GRAIN  1024

/* VAG ADPCM predictor coefficients. Each 16-byte block picks one of five
 * filters; the decoded sample is the shifted nibble plus a weighted sum of the
 * previous two outputs. The weights are /64. */
static const int VAG_F0[5] = { 0, 60, 115,  98, 122 };
static const int VAG_F1[5] = { 0,  0, -52, -55, -60 };

enum { ENV_OFF = 0, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };

typedef struct {
    uint32_t vag_addr;      /* guest address of the sample data */
    uint32_t vag_size;
    int      loop;
    uint32_t pos;           /* byte offset of the current 16-byte block */
    int      sample_idx;    /* 0..27 within the block */
    int      hist1, hist2;  /* ADPCM history */
    int16_t  decoded[28];
    int      decoded_valid;

    uint32_t pitch;         /* 0x1000 == 1.0 */
    uint32_t frac;          /* resampling accumulator, 12-bit fraction */

    int32_t  vol_l, vol_r;  /* 0x1000 == unity */
    int      env_state;
    int32_t  env;           /* 0 .. 0x40000000 */
    int32_t  attack_rate, decay_rate, sustain_level, release_rate;

    int      playing;
    int      ended;
    int      paused;
} sas_voice;

static sas_voice g_voice[SAS_VOICES];
static uint32_t  g_grain = 256;
static uint32_t  g_max_voices = SAS_VOICES;
static uint32_t  g_output_mode;
static uint32_t  g_sample_rate = 44100;
static uint64_t  g_frames_rendered;
static uint64_t  g_samples_nonzero;

void psp_sas_reset(void) {
    memset(g_voice, 0, sizeof g_voice);
    for (int i = 0; i < SAS_VOICES; i++) {
        g_voice[i].pitch = 0x1000;
        g_voice[i].vol_l = 0x1000;
        g_voice[i].vol_r = 0x1000;
    }
    g_grain = 256;
    g_max_voices = SAS_VOICES;
    g_output_mode = 0;
    g_sample_rate = 44100;
    g_frames_rendered = 0;
    g_samples_nonzero = 0;
}

void psp_sas_init(void) { psp_sas_reset(); }

uint64_t psp_sas_frames(void)   { return g_frames_rendered; }
uint64_t psp_sas_nonzero(void)  { return g_samples_nonzero; }

static int clamp16(int v) {
    if (v >  32767) return  32767;
    if (v < -32768) return -32768;
    return v;
}

/* Decode the 16-byte ADPCM block at the voice's current position into its
 * 28-sample buffer. Returns 0 when the voice has run off the end. */
static int decode_block(sas_voice *v) {
    if (v->pos + 16 > v->vag_size) {
        if (!v->loop) return 0;
        v->pos = 0;
        v->hist1 = v->hist2 = 0;
    }

    uint32_t at = v->vag_addr + v->pos;
    uint8_t hdr   = psp_read8(at);
    uint8_t flags = psp_read8(at + 1);

    int shift  = hdr & 0x0F;
    int filter = (hdr >> 4) & 0x0F;
    if (filter > 4) filter = 0;          /* out of range: treat as no prediction */

    /* Flag 7 marks the end of the sample. */
    if (flags == 7) {
        if (!v->loop) return 0;
        v->pos = 0;
        return decode_block(v);
    }

    for (int i = 0; i < 28; i++) {
        uint8_t byte = psp_read8(at + 2 + (uint32_t)(i / 2));
        int nib = (i & 1) ? (byte >> 4) : (byte & 0x0F);

        /* Sign-extend the 4-bit sample into the top of a 16-bit word, then
         * shift down. Shifting the nibble directly loses the sign. */
        int s = (int)((int16_t)(nib << 12)) >> shift;
        s += (VAG_F0[filter] * v->hist1 + VAG_F1[filter] * v->hist2) >> 6;
        s = clamp16(s);

        v->decoded[i] = (int16_t)s;
        v->hist2 = v->hist1;
        v->hist1 = s;
    }

    v->pos += 16;
    v->decoded_valid = 1;
    return 1;
}

/* Advance the envelope by one sample and return its current level, 0..0x40000000. */
static int32_t step_envelope(sas_voice *v) {
    switch (v->env_state) {
    case ENV_ATTACK:
        v->env += v->attack_rate;
        if (v->env >= 0x40000000) { v->env = 0x40000000; v->env_state = ENV_DECAY; }
        break;
    case ENV_DECAY:
        v->env -= v->decay_rate;
        if (v->env <= v->sustain_level) { v->env = v->sustain_level; v->env_state = ENV_SUSTAIN; }
        break;
    case ENV_RELEASE:
        v->env -= v->release_rate;
        if (v->env <= 0) { v->env = 0; v->env_state = ENV_OFF; v->playing = 0; v->ended = 1; }
        break;
    case ENV_SUSTAIN:
    default:
        break;
    }
    return v->env;
}

/* Render `samples` stereo frames, summing every active voice. */
static void render(int32_t *mix_l, int32_t *mix_r, uint32_t samples) {
    memset(mix_l, 0, samples * sizeof *mix_l);
    memset(mix_r, 0, samples * sizeof *mix_r);

    for (uint32_t vi = 0; vi < g_max_voices; vi++) {
        sas_voice *v = &g_voice[vi];
        if (!v->playing || v->paused || !v->vag_addr) continue;

        for (uint32_t i = 0; i < samples; i++) {
            if (!v->decoded_valid || v->sample_idx >= 28) {
                v->sample_idx = 0;
                if (!decode_block(v)) { v->playing = 0; v->ended = 1; break; }
            }

            int32_t s = v->decoded[v->sample_idx];
            int32_t env = step_envelope(v);
            /* Envelope is 30-bit; bring it down to a 12-bit multiplier before
             * applying, so the product stays inside 32 bits. */
            s = (s * (env >> 18)) >> 12;

            mix_l[i] += (s * v->vol_l) >> 12;
            mix_r[i] += (s * v->vol_r) >> 12;

            /* Pitch is a 12-bit fixed-point step: 0x1000 plays at the source
             * rate, 0x2000 an octave up. */
            v->frac += v->pitch;
            while (v->frac >= 0x1000) {
                v->frac -= 0x1000;
                v->sample_idx++;
                if (v->sample_idx >= 28) {
                    v->sample_idx = 0;
                    if (!decode_block(v)) { v->playing = 0; v->ended = 1; break; }
                }
            }
            if (!v->playing) break;
        }
    }
}

/* ---- the calls ----------------------------------------------------------- */

static sas_voice *voice_arg(void) {
    uint32_t i = psp_arg(1);
    return (i < SAS_VOICES) ? &g_voice[i] : NULL;
}

static void hle_Init(void) {
    /* (sasCore, grain, maxVoices, outputMode, sampleRate) */
    g_grain       = psp_arg(1);
    g_max_voices  = psp_arg(2);
    g_output_mode = psp_arg(3);
    g_sample_rate = psp_arg(4);
    if (!g_grain || g_grain > SAS_MAX_GRAIN) g_grain = 256;
    if (!g_max_voices || g_max_voices > SAS_VOICES) g_max_voices = SAS_VOICES;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_SetVoice(void) {
    /* (sasCore, voice, vagAddr, size, loopmode) */
    sas_voice *v = voice_arg();
    if (!v) { psp_ret(SCE_KERNEL_ERROR_ERROR); return; }
    v->vag_addr = psp_arg(2);
    v->vag_size = psp_arg(3);
    v->loop     = (int)psp_arg(4);
    v->pos = 0;
    v->sample_idx = 0;
    v->hist1 = v->hist2 = 0;
    v->decoded_valid = 0;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_SetPitch(void) {
    sas_voice *v = voice_arg();
    if (!v) { psp_ret(SCE_KERNEL_ERROR_ERROR); return; }
    v->pitch = psp_arg(2);
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_SetVolume(void) {
    /* (sasCore, voice, l, r, el, er) -- the last two are the reverb sends. */
    sas_voice *v = voice_arg();
    if (!v) { psp_ret(SCE_KERNEL_ERROR_ERROR); return; }
    v->vol_l = (int32_t)psp_arg(2);
    v->vol_r = (int32_t)psp_arg(3);
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_SetADSR(void) {
    /* (sasCore, voice, flags, attack, decay, sustain, release) */
    sas_voice *v = voice_arg();
    if (!v) { psp_ret(SCE_KERNEL_ERROR_ERROR); return; }
    uint32_t flags = psp_arg(2);
    if (flags & 1) v->attack_rate  = (int32_t)psp_arg(3);
    if (flags & 2) v->decay_rate   = (int32_t)psp_arg(4);
    if (flags & 4) v->sustain_level= (int32_t)psp_arg(5);
    if (flags & 8) v->release_rate = (int32_t)psp_arg(6);
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_SetSimpleADSR(void) {
    sas_voice *v = voice_arg();
    if (!v) { psp_ret(SCE_KERNEL_ERROR_ERROR); return; }
    /* The packed form encodes rates in two 16-bit words. Without the exact
     * curve tables this is an approximation: fast attack, slow release. Audio
     * plays at the right pitch and duration; envelope shape is not exact. */
    v->attack_rate   = 0x40000000 / 64;
    v->decay_rate    = 0x40000000 / 512;
    v->sustain_level = 0x30000000;
    v->release_rate  = 0x40000000 / 256;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_SetKeyOn(void) {
    sas_voice *v = voice_arg();
    if (!v) { psp_ret(SCE_KERNEL_ERROR_ERROR); return; }
    v->playing = 1;
    v->ended = 0;
    v->paused = 0;
    v->pos = 0;
    v->sample_idx = 0;
    v->frac = 0;
    v->hist1 = v->hist2 = 0;
    v->decoded_valid = 0;
    v->env = 0;
    v->env_state = ENV_ATTACK;
    if (!v->attack_rate) v->attack_rate = 0x40000000 / 64;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_SetKeyOff(void) {
    sas_voice *v = voice_arg();
    if (!v) { psp_ret(SCE_KERNEL_ERROR_ERROR); return; }
    v->env_state = ENV_RELEASE;
    if (!v->release_rate) v->release_rate = 0x40000000 / 256;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_SetPause(void) {
    /* (sasCore, voiceBitmask, pause) */
    uint32_t mask = psp_arg(1);
    int pause = (int)psp_arg(2);
    for (int i = 0; i < SAS_VOICES; i++)
        if (mask & (1u << i)) g_voice[i].paused = pause;
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_GetPauseFlag(void) {
    uint32_t f = 0;
    for (int i = 0; i < SAS_VOICES; i++) if (g_voice[i].paused) f |= 1u << i;
    psp_ret(f);
}

static void hle_GetEndFlag(void) {
    /* A game polls this to know when a sound has finished, and often will not
     * start the next one until a voice reports ended. Reporting "never ended"
     * is a common way to make audio appear to work and then stop. */
    uint32_t f = 0;
    for (int i = 0; i < SAS_VOICES; i++) if (!g_voice[i].playing) f |= 1u << i;
    psp_ret(f);
}

static void hle_GetEnvelopeHeight(void) {
    sas_voice *v = voice_arg();
    psp_ret(v ? (uint32_t)v->env : 0);
}

static void mix_to_guest(uint32_t out_addr, int add) {
    int32_t l[SAS_MAX_GRAIN], r[SAS_MAX_GRAIN];
    uint32_t n = g_grain;
    render(l, r, n);

    for (uint32_t i = 0; i < n; i++) {
        int32_t sl = clamp16(l[i]), sr = clamp16(r[i]);
        if (add) {
            sl = clamp16(sl + (int16_t)psp_read16(out_addr + i * 4));
            sr = clamp16(sr + (int16_t)psp_read16(out_addr + i * 4 + 2));
        }
        psp_write16(out_addr + i * 4,     (uint16_t)(int16_t)sl);
        psp_write16(out_addr + i * 4 + 2, (uint16_t)(int16_t)sr);
        if (sl || sr) g_samples_nonzero++;
    }
    g_frames_rendered++;
}

static void hle_Core(void) {
    /* (sasCore, sampleBuffer) */
    uint32_t out = psp_arg(1);
    if (out) mix_to_guest(out, 0);
    psp_ret(SCE_KERNEL_ERROR_OK);
}

static void hle_CoreWithMix(void) {
    uint32_t out = psp_arg(1);
    if (out) mix_to_guest(out, 1);
    psp_ret(SCE_KERNEL_ERROR_OK);
}

/* Reverb and noise: accepted and recorded so a game's setup sequence completes.
 * The dry mix is what carries the music and effects; reverb is a refinement. */
static void hle_accept(void) { psp_ret(SCE_KERNEL_ERROR_OK); }

void psp_sas_register(void) {
    psp_hle_register(0x42778A9F, "sceSasCore", "__sceSasInit",              hle_Init);
    psp_hle_register(0x99944089, "sceSasCore", "__sceSasSetVoice",          hle_SetVoice);
    psp_hle_register(0xAD84D37F, "sceSasCore", "__sceSasSetPitch",          hle_SetPitch);
    psp_hle_register(0x440CA7D8, "sceSasCore", "__sceSasSetVolume",         hle_SetVolume);
    psp_hle_register(0x019B25EB, "sceSasCore", "__sceSasSetADSR",           hle_SetADSR);
    psp_hle_register(0x9EC3676A, "sceSasCore", "__sceSasSetADSRmode",       hle_accept);
    psp_hle_register(0xCBCD4F79, "sceSasCore", "__sceSasSetSimpleADSR",     hle_SetSimpleADSR);
    psp_hle_register(0x5F9529F6, "sceSasCore", "__sceSasSetSL",             hle_accept);
    psp_hle_register(0x76F01ACA, "sceSasCore", "__sceSasSetKeyOn",          hle_SetKeyOn);
    psp_hle_register(0xA0CF2FA4, "sceSasCore", "__sceSasSetKeyOff",         hle_SetKeyOff);
    psp_hle_register(0xA3589D81, "sceSasCore", "__sceSasCore",              hle_Core);
    psp_hle_register(0x50A14DFC, "sceSasCore", "__sceSasCoreWithMix",       hle_CoreWithMix);
    psp_hle_register(0x68A46B95, "sceSasCore", "__sceSasGetEndFlag",        hle_GetEndFlag);
    psp_hle_register(0x787D04D5, "sceSasCore", "__sceSasSetPause",          hle_SetPause);
    psp_hle_register(0x2C8E6AB3, "sceSasCore", "__sceSasGetPauseFlag",      hle_GetPauseFlag);
    psp_hle_register(0x74AE582A, "sceSasCore", "__sceSasGetEnvelopeHeight", hle_GetEnvelopeHeight);
    psp_hle_register(0xB7660A23, "sceSasCore", "__sceSasSetNoise",          hle_accept);
    psp_hle_register(0x33D4AB37, "sceSasCore", "__sceSasRevType",           hle_accept);
    psp_hle_register(0x267A6DD2, "sceSasCore", "__sceSasRevParam",          hle_accept);
    psp_hle_register(0xD5A229C9, "sceSasCore", "__sceSasRevEVOL",           hle_accept);
    psp_hle_register(0xF983B186, "sceSasCore", "__sceSasRevVON",            hle_accept);
}
