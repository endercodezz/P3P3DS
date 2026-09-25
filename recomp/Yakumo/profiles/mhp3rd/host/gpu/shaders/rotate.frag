#version 450

// Android pre-rotation: copies the upright frame's texel.

layout(set = 0, binding = 0) uniform sampler2D upright;

layout(location = 0) in vec2 upright_uv;
layout(location = 0) out vec4 color;

void main() {
    color = texture(upright, upright_uv);
}
