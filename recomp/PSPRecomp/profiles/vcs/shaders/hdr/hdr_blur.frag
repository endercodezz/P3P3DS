#version 450
//---------------------------------------------------------------------------//
// SimulateHDR - separable blur, shared by all six blur passes.
//
// Ported from PS_BloomBlur in SimulateHDR_Bloom.fxh. See
// shaders/hdr/LICENSE_SIMULATEHDR.txt (CC BY-SA 4.0). Changes were made.
//
// The kernel is uniform over the frame, so the host solves it once and hands
// over four taps per side with the UV offset already scaled for this octave and
// direction and the weight already normalised. Adjacent taps of the original
// fourteen are merged into a single bilinear fetch at their weighted centroid,
// preserving reach and total weight at eight fetches instead of fourteen.
//
// One shader serves every octave: a different image bound to u_input and a
// different tap set is the whole difference between the six passes.
//---------------------------------------------------------------------------//

layout(set = 0, binding = 0) uniform sampler2D u_input;

layout(push_constant) uniform Push {
    vec4 taps[4];   // xy = UV offset, z = normalised weight
} pc;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

void main() {
    vec4 blur = vec4(0.0);
    for (int i = 0; i < 4; ++i) {
        const vec2 offset = pc.taps[i].xy;
        blur += (texture(u_input, v_uv + offset) +
                 texture(u_input, v_uv - offset)) * pc.taps[i].z;
    }
    o_color = blur;
}
