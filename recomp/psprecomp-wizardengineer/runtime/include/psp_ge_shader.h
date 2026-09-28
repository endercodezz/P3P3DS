#pragma once
#include "psp_ge.h"
#include <glad/glad.h>

/// Shader uniform locations cache.
struct ShaderUniforms {
    GLint u_texture_enable;
    GLint u_texture;
    GLint u_tex_func;
    GLint u_alpha_test_enable;
    GLint u_alpha_test_ref;
    GLint u_alpha_test_func;
};

/// Compile and link the uber-shader pair. Call after GL init.
void ge_shader_init();

/// Delete shader program.
void ge_shader_shutdown();

/// Bind the shader program.
void ge_shader_use();

/// Update all uniforms from current GE state.
void ge_shader_set_uniforms(const GeState& state);

/// Get the shader program ID (for external uniform queries).
GLuint ge_shader_get_program();
