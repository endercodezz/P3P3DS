#version 450
//---------------------------------------------------------------------------//
// SimulateHDR - fullscreen triangle, shared by every pass.
//
// Ported to Vulkan/GLSL for VCSNative from ProperShaders' SimulateHDR, itself a
// fork of "Blooming HDR" by Jose Negrete (BlueSkyDefender), CC BY-SA 4.0.
// See shaders/hdr/LICENSE_SIMULATEHDR.txt. Changes were made.
//
// No vertex buffer: three vertices covering the screen, derived from the index.
//---------------------------------------------------------------------------//

layout(location = 0) out vec2 v_uv;

void main() {
    v_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(v_uv * 2.0 - 1.0, 0.0, 1.0);
}
