#version 450
//---------------------------------------------------------------------------//
// SimulateHDR - octave combine.
//
// Ported from PS_BloomCombine in SimulateHDR_Bloom.fxh. See
// shaders/hdr/LICENSE_SIMULATEHDR.txt (CC BY-SA 4.0). Changes were made.
//
// Both the horizontal-only and the fully blurred target of each octave go into
// the sum -- the horizontal ones are what give the bloom its streak, so
// dropping them would change the look.
//---------------------------------------------------------------------------//

layout(set = 0, binding = 0) uniform sampler2D u_h_a;
layout(set = 0, binding = 1) uniform sampler2D u_v_a;
layout(set = 0, binding = 2) uniform sampler2D u_h_b;
layout(set = 0, binding = 3) uniform sampler2D u_v_b;
layout(set = 0, binding = 4) uniform sampler2D u_h_c;
layout(set = 0, binding = 5) uniform sampler2D u_v_c;

layout(push_constant) uniform Push {
    float mix_scale;   // intensity * 10 / 6 on the host side
} pc;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

void main() {
    vec3 bloom = texture(u_h_a, v_uv).rgb;
    bloom += texture(u_v_a, v_uv).rgb;
    bloom += texture(u_h_b, v_uv).rgb;
    bloom += texture(u_v_b, v_uv).rgb;
    bloom += texture(u_h_c, v_uv).rgb;
    bloom += texture(u_v_c, v_uv).rgb;
    o_color = vec4(bloom * pc.mix_scale, 0.0);
}
