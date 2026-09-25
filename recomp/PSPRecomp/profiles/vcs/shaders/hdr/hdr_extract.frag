#version 450
//---------------------------------------------------------------------------//
// SimulateHDR - bright pass.
//
// Ported from PS_BloomExtract in SimulateHDR_Bloom.fxh. See
// shaders/hdr/LICENSE_SIMULATEHDR.txt (CC BY-SA 4.0). Changes were made.
//
// Runs at quarter resolution straight off the scene copy; the bilinear fetch of
// a full resolution source at reduced UVs is already a box filter, so no
// explicit downsample is needed.
//---------------------------------------------------------------------------//

layout(set = 0, binding = 0) uniform sampler2D u_scene;

layout(push_constant) uniform Push {
    float sensitivity;
    float threshold;
    float saturation;   // min(10, Saturation * 10) on the host side
} pc;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;

float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); } // Rec. 709

void main() {
    vec3 c = texture(u_scene, v_uv).rgb;
    const float l = luma(c);

    // Sensitivity curve, then divide out the luma so the threshold below is the
    // only thing deciding how bright a pixel ends up.
    c = pow(abs(c), vec3(pc.sensitivity)) / max(l, 0.001);

    const float bright = max(0.0, l - pc.threshold);
    c *= bright;
    c = mix(vec3(bright), c, pc.saturation);

    o_color = vec4(clamp(c, 0.0, 1.0), 0.0);
}
