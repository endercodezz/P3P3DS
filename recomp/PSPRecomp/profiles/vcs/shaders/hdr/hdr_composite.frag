#version 450
//---------------------------------------------------------------------------//
// SimulateHDR - final full-screen composite.
//
// Ported from HDR_Composite in SimulateHDR_Final.fxh and the two operators in
// SimulateHDR_Tonemap.fxh. See shaders/hdr/LICENSE_SIMULATEHDR.txt
// (CC BY-SA 4.0). Changes were made.
//
// Deliberate differences from the DX9 original:
//   - The tone mapper is a uniform branch instead of four compile-time
//     specialisations. ps_3_0 did not reward runtime branching; a desktop GPU
//     running a push-constant branch that is uniform across the draw does.
//   - Eye adaptation and the colour grading LUT are not ported. AUTOEXP needed a
//     frame-average luminance read back to the host, which is the readback this
//     port avoids on purpose; LUTMODE needed PNG tables. Both collapse to their
//     "off" branch here, exactly as the T#E0L0D0 variant did.
//---------------------------------------------------------------------------//

layout(set = 0, binding = 0) uniform sampler2D u_scene;
layout(set = 0, binding = 1) uniform sampler2D u_bloom;

layout(push_constant) uniform Push {
    vec4  timothy_curve;   // (contrast, b, c, unused)
    vec4  timothy_cross;   // (saturation/crossSat, crossSat, white point, unused)
    vec2  screen_size;
    float dither_amount;
    float bloom_opacity;
    float hdr_power;
    float exposure_bias;
    float gamma_in;
    float gamma_out;
    float saturate_amount;
    float contrast;
    float inv_white_point;
    int   tonemap;         // 0 = Timothy Lottes, 1 = ACES
} pc;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); } // Rec. 709

//---------------------------------------------------------------------------//
// Interleaved gradient noise, three decorrelated channels off one dot product.
//---------------------------------------------------------------------------//
vec3 dither_noise(vec2 pixel_pos) {
    const float t = pixel_pos.x * 0.06711056 + pixel_pos.y * 0.00583715;
    return fract(52.9829189 * fract(t + vec3(0.0, 0.3183099, 0.6366198)));
}

//---------------------------------------------------------------------------//
// Timothy Lottes. The shoulder term is fixed at 1.0, which collapses ColTone's
// pow(z, shoulder) to z, so the curve is one pow plus a reciprocal.
//---------------------------------------------------------------------------//
vec3 timothy_tonemap(vec3 color, float exposure) {
    color *= exposure;

    float peak = max(1e-6, max(color.r, max(color.g, color.b)));
    vec3 ratio = color / peak;

    const float z = pow(peak, pc.timothy_curve.x);
    peak = z / (z * pc.timothy_curve.y + pc.timothy_curve.z);

    // Channel crosstalk, wrapped in the saturation transform. The crosstalk
    // amount is pow(peak, 4), which is two multiplies rather than a real pow.
    ratio = pow(abs(ratio), vec3(pc.timothy_cross.x));
    const float peak2 = peak * peak;
    ratio = mix(ratio, vec3(pc.timothy_cross.z), peak2 * peak2);
    ratio = pow(abs(ratio), vec3(pc.timothy_cross.y));

    return peak * ratio;
}

//---------------------------------------------------------------------------//
// ACES fitted, after Stephen Hill / Krzysztof Narkowicz via MJP's BakingLab.
//
// HLSL mul(M, v) is a row-vector-times-matrix product; GLSL mat3 is column
// major, so the same maths is written here as v * M with the rows entered as
// columns. Transposing one and not the other is the classic way to port this
// wrong and get a colour cast.
//---------------------------------------------------------------------------//
const mat3 kAcesInput = mat3(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777);

const mat3 kAcesOutput = mat3(
     1.60475, -0.10208, -0.00327,
    -0.53108,  1.10813, -0.07276,
    -0.07367, -0.00605,  1.07602);

vec3 rrt_and_odt_fit(vec3 v) {
    const vec3 a = v * (v + 0.0245786) - 0.000090537;
    const vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 aces_tonemap(vec3 color, float exposure) {
    color *= exposure + 0.5;   // the fit expects exposure pre-adjusted this way
    color = kAcesInput * color;
    color = rrt_and_odt_fit(color);
    color = kAcesOutput * color;
    return color * pc.inv_white_point;
}

void main() {
    vec3 bloom = texture(u_bloom, v_uv).rgb;

    const vec3 noise = dither_noise(v_uv * pc.screen_size);
    bloom = clamp(bloom + noise * smoothstep(0.0, 0.1, bloom) * pc.dither_amount,
                  0.0, 1.0);
    bloom *= pc.bloom_opacity;

    vec3 color = texture(u_scene, v_uv).rgb;

    if (pc.tonemap == 1)
        color = mix(vec3(luma(color)), color, pc.saturate_amount);

    // De-gamma into linear before anything else touches the colour.
    color = pow(abs(color), vec3(pc.gamma_in));

    // Bloom goes in before the inverse tone map, otherwise the high range it is
    // supposed to feed has already been clipped away.
    color = max(vec3(0.0), color + bloom);

    // Inverse tone map (Timothy Lottes fast reversible). The floor on the
    // denominator is not paranoia: a blown highlight is exactly 1.0 and
    // pow(1.0, g) is exactly 1.0, so the denominator hits zero wherever bloom
    // equals hdr_power. That gives INF, then INF/INF = NaN inside the ACES fit,
    // and a NaN lands in an 8-bit target as black -- dark patches in the
    // brightest part of a bloom. Clamping scales every channel by the same
    // factor, so hue is untouched.
    const float hdr_scale = (1.0 + max(pc.hdr_power, 0.001)) -
        max(color.r, max(color.g, color.b));
    color *= 1.0 / max(hdr_scale, 0.001);

    color = (pc.tonemap == 0) ? timothy_tonemap(color, pc.exposure_bias)
                              : aces_tonemap(color, pc.exposure_bias);

    // gamma_out is 1/2.2, or 1.0 when the gamma conversion is switched off.
    color = pow(abs(color), vec3(pc.gamma_out));

    if (pc.tonemap == 1)
        color = (color - 0.5) * pc.contrast + 0.5;

    o_color = vec4(color, 1.0);
}
