#pragma once
#include "psp_ge.h"
#include <glad/glad.h>
#include <cstdint>
#include <array>

/// Single entry in the texture cache.
struct TexCacheEntry {
    GLuint gl_tex = 0;
    uint32_t psp_addr = 0;
    uint32_t content_hash = 0;
    int width = 0;
    int height = 0;
    int format = 0;
    uint32_t last_frame = 0;
    bool valid = false;
};

/// Fixed-capacity LRU texture cache. Stores GL texture objects
/// keyed by (psp_addr, content_hash). When full, evicts the
/// least-recently-used entry.
static constexpr int TEX_CACHE_SIZE = 256;

/// Initialize the texture cache (call after GL context is ready).
void ge_texture_init();

/// Shutdown the texture cache and release all GL textures.
void ge_texture_shutdown();

/// Look up or create a GL texture from current GE texture state.
/// Binds the result to GL_TEXTURE_2D. Returns the GL texture ID
/// (0 if texture is disabled or decode fails).
GLuint ge_texture_bind(
    uint8_t* rdram,
    uint32_t frame_num
);

/// Invalidate the entire texture cache (debug/reset).
void ge_texture_invalidate_all();
