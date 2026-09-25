#version 450

// Android pre-rotation: one triangle covering the swapchain image, which is in
// the display panel's own orientation, reading the upright frame turned by
// the surface's current transform, so the compositor does not have to.

layout(push_constant) uniform Rotation {
    int quarter_turns;
} rotation;

layout(location = 0) out vec2 upright_uv;

void main() {
    // (0,0), (2,0), (0,2): the triangle that covers the whole target.
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
    if (rotation.quarter_turns == 1) upright_uv = vec2(uv.y, 1.0 - uv.x);
    else if (rotation.quarter_turns == 2) upright_uv = vec2(1.0 - uv.x, 1.0 - uv.y);
    else if (rotation.quarter_turns == 3) upright_uv = vec2(1.0 - uv.y, uv.x);
    else upright_uv = uv;
}
