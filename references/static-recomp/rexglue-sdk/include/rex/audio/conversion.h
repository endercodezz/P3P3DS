/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#pragma once

#include <algorithm>
#include <cstdint>

#include <rex/assert.h>
#include <rex/audio/downmix.h>
#include <rex/platform.h>
#include <rex/types.h>

namespace rex::audio::conversion {

#if REX_ARCH_AMD64
inline void sequential_6_BE_to_interleaved_6_LE(float* output, const float* input,
                                                size_t ch_sample_count, const SurroundMix& mix,
                                                float gain) {
  // The gain pass below walks the output four floats at a time, and its weight
  // table assumes 6 channels divide evenly into those windows.
  assert_true(ch_sample_count % 2 == 0);

  const uint32_t* in = reinterpret_cast<const uint32_t*>(input);
  uint32_t* out = reinterpret_cast<uint32_t*>(output);
  const __m128i byte_swap_shuffle =
      _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);
  for (size_t sample = 0; sample < ch_sample_count; sample++) {
    __m128i sample0 =
        _mm_set_epi32(in[3 * ch_sample_count + sample], in[2 * ch_sample_count + sample],
                      in[1 * ch_sample_count + sample], in[0 * ch_sample_count + sample]);
    uint32_t sample1 = in[4 * ch_sample_count + sample];
    uint32_t sample2 = in[5 * ch_sample_count + sample];
    sample0 = _mm_shuffle_epi8(sample0, byte_swap_shuffle);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(&out[sample * 6]), sample0);
    sample1 = rex::byte_swap(sample1);
    out[sample * 6 + 4] = sample1;
    sample2 = rex::byte_swap(sample2);
    out[sample * 6 + 5] = sample2;
  }

  // Second pass rather than fusing into the shuffle above, which stores as integers.
  // 6 KB per 5.33 ms frame, so skipping it at unity gain is not worth the asymmetry.
  //
  // Six interleaved channels across four-float windows repeat every 12 floats,
  // so three weight vectors cover every alignment the loop ever sees. The
  // assert above makes the sample count even, which makes the float count a
  // multiple of 12, so the cycle closes with no tail to handle.
  const float c = mix.center * gain;
  const float s = mix.surround * gain;
  const float l = mix.lfe * gain;
  const __m128 w[3] = {_mm_setr_ps(gain, gain, c, l), _mm_setr_ps(s, s, gain, gain),
                       _mm_setr_ps(c, l, s, s)};
  const __m128 lo = _mm_set1_ps(-1.0f);
  const __m128 hi = _mm_set1_ps(1.0f);
  const size_t count = ch_sample_count * 6;
  for (size_t i = 0, phase = 0; i < count; i += 4, phase = (phase + 1) % 3) {
    const __m128 v = _mm_mul_ps(_mm_loadu_ps(&output[i]), w[phase]);
    _mm_storeu_ps(&output[i], _mm_min_ps(_mm_max_ps(v, lo), hi));
  }
}

inline void sequential_6_BE_to_interleaved_2_LE(float* output, const float* input,
                                                size_t ch_sample_count, const StereoFold& fold,
                                                float gain) {
  assert_true(ch_sample_count % 4 == 0);

  const __m128i byte_swap_shuffle =
      _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);
  const __m128 center = _mm_set1_ps(fold.center);
  const __m128 lfe = _mm_set1_ps(fold.lfe);
  const __m128 surround = _mm_set1_ps(fold.surround);
  const __m128 scale = _mm_set1_ps(fold.scale * gain);
  const __m128 lo = _mm_set1_ps(-1.0f);
  const __m128 hi = _mm_set1_ps(1.0f);

  for (size_t sample = 0; sample < ch_sample_count; sample += 4) {
    // load 4 samples from 6 channels each
    __m128 fl = _mm_loadu_ps(&input[0 * ch_sample_count + sample]);
    __m128 fr = _mm_loadu_ps(&input[1 * ch_sample_count + sample]);
    __m128 fc = _mm_loadu_ps(&input[2 * ch_sample_count + sample]);
    __m128 lf = _mm_loadu_ps(&input[3 * ch_sample_count + sample]);
    __m128 bl = _mm_loadu_ps(&input[4 * ch_sample_count + sample]);
    __m128 br = _mm_loadu_ps(&input[5 * ch_sample_count + sample]);
    // byte swap
    fl = _mm_castsi128_ps(_mm_shuffle_epi8(_mm_castps_si128(fl), byte_swap_shuffle));
    fr = _mm_castsi128_ps(_mm_shuffle_epi8(_mm_castps_si128(fr), byte_swap_shuffle));
    fc = _mm_castsi128_ps(_mm_shuffle_epi8(_mm_castps_si128(fc), byte_swap_shuffle));
    lf = _mm_castsi128_ps(_mm_shuffle_epi8(_mm_castps_si128(lf), byte_swap_shuffle));
    bl = _mm_castsi128_ps(_mm_shuffle_epi8(_mm_castps_si128(bl), byte_swap_shuffle));
    br = _mm_castsi128_ps(_mm_shuffle_epi8(_mm_castps_si128(br), byte_swap_shuffle));

    // Center and LFE land on both sides.
    const __m128 mid = _mm_add_ps(_mm_mul_ps(fc, center), _mm_mul_ps(lf, lfe));
    __m128 left = _mm_mul_ps(_mm_add_ps(_mm_add_ps(fl, mid), _mm_mul_ps(bl, surround)), scale);
    __m128 right = _mm_mul_ps(_mm_add_ps(_mm_add_ps(fr, mid), _mm_mul_ps(br, surround)), scale);
    left = _mm_min_ps(_mm_max_ps(left, lo), hi);
    right = _mm_min_ps(_mm_max_ps(right, lo), hi);

    _mm_storeu_ps(&output[sample * 2], _mm_unpacklo_ps(left, right));
    _mm_storeu_ps(&output[(sample + 2) * 2], _mm_unpackhi_ps(left, right));
  }
}
#else
inline void sequential_6_BE_to_interleaved_6_LE(float* output, const float* input,
                                                size_t ch_sample_count, const SurroundMix& mix,
                                                float gain) {
  const float w[6] = {
      gain, gain, mix.center * gain, mix.lfe * gain, mix.surround * gain, mix.surround * gain};
  for (size_t sample = 0; sample < ch_sample_count; sample++) {
    for (size_t channel = 0; channel < 6; channel++) {
      const float v = rex::byte_swap(input[channel * ch_sample_count + sample]) * w[channel];
      output[sample * 6 + channel] = std::clamp(v, -1.0f, 1.0f);
    }
  }
}

inline void sequential_6_BE_to_interleaved_2_LE(float* output, const float* input,
                                                size_t ch_sample_count, const StereoFold& fold,
                                                float gain) {
  // Default 5.1 channel mapping is fl, fr, fc, lf, bl, br
  // https://docs.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-default-channel-mapping
  const float scale = fold.scale * gain;
  for (size_t sample = 0; sample < ch_sample_count; sample++) {
    const float fl = rex::byte_swap(input[0 * ch_sample_count + sample]);
    const float fr = rex::byte_swap(input[1 * ch_sample_count + sample]);
    const float fc = rex::byte_swap(input[2 * ch_sample_count + sample]);
    const float lf = rex::byte_swap(input[3 * ch_sample_count + sample]);
    const float bl = rex::byte_swap(input[4 * ch_sample_count + sample]);
    const float br = rex::byte_swap(input[5 * ch_sample_count + sample]);
    // Center and LFE land on both sides.
    const float mid = fc * fold.center + lf * fold.lfe;
    output[sample * 2] = std::clamp((fl + mid + bl * fold.surround) * scale, -1.0f, 1.0f);
    output[sample * 2 + 1] = std::clamp((fr + mid + br * fold.surround) * scale, -1.0f, 1.0f);
  }
}
#endif

}  // namespace rex::audio::conversion
