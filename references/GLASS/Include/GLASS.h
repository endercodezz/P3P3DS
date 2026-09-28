/**
 * @file GLASS.h
 * @brief Interface to the GLASS library.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _GLASS_H
#define _GLASS_H

#include <GLASS/Defs.h>

/// @brief Number of fog LUT values.
#define GLASS_NUM_FOG_LUT_VALUES 129

/// @brief GLASS context.
typedef struct GLASSCtxImpl* GLASSCtx;

/// @brief OpenGL ES version.
typedef enum {
    GLASS_VERSION_ES_2, ///< OpenGL ES 2.
} GLASSVersion;

/// @brief Target screen.
typedef enum {
    GLASS_SCREEN_TOP,    ///< Top screen.
    GLASS_SCREEN_BOTTOM, ///< Bottom screen.
} GLASSScreen;

/// @brief Target screen side.
typedef enum {
    GLASS_SIDE_LEFT,  ///< Left side.
    GLASS_SIDE_RIGHT, ///< Right side.
} GLASSSide;

/// @brief GPU command list.
typedef struct {
    void* mainBuffer;    ///< Main command buffer.
    void* secondBuffer;  ///< Second command buffer.
    size_t capacity;     ///< Max size of each buffer, in bytes.
    size_t offset;       ///< Offset of the current GPU command location.
} GLASSGPUCommandList;

/// @brief Framebuffer dimension downscale (anti-aliasing).
typedef enum {
    GLASS_DOWNSCALE_NONE, ///< No downscale.
    GLASS_DOWNSCALE_1X2,  ///< Halve height.
    GLASS_DOWNSCALE_2X2,  ///< Halve width and height.
} GLASSDownscale;

/// @brief Context parameters.
typedef struct {
    GLASSVersion version;           ///< Context version.
    GLASSScreen targetScreen;       ///< Draw target screen (default: GLASS_SCREEN_TOP).
    GLASSSide targetSide;           ///< Draw target side (default: GLASS_SIDE_LEFT).
    GLASSGPUCommandList GPUCmdList; ///< GPU command list (default: all NULL).
    bool vsync;                     ///< Enable VSync (default: true).
    bool horizontalFlip;            ///< Flip display buffer horizontally (default: false).
    bool flushAllLinearMem;         ///< Whether to flush all linear memory (default: true).
    GLASSDownscale downscale;       ///< Set downscale for anti-aliasing (default: GLASS_DOWNSCALE_NONE).
} GLASSCtxParams;

/// @brief Fog LUT.
typedef struct {
    GLfloat values[GLASS_NUM_FOG_LUT_VALUES];
} GLASSFogLUT;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Get default context params.
KYGX_INLINE void glassGetDefaultContextParams(GLASSCtxParams* ctxParams, GLASSVersion version) {
    KYGX_ASSERT(ctxParams);

    ctxParams->version = version;
    ctxParams->targetScreen = GLASS_SCREEN_TOP;
    ctxParams->targetSide = GLASS_SIDE_LEFT;
    ctxParams->GPUCmdList.mainBuffer = NULL;
    ctxParams->GPUCmdList.secondBuffer = NULL;
    ctxParams->GPUCmdList.capacity = 0;
    ctxParams->GPUCmdList.offset = 0;
    ctxParams->vsync = true;
    ctxParams->horizontalFlip = false;
    ctxParams->flushAllLinearMem = true;
    ctxParams->downscale = GLASS_DOWNSCALE_NONE;
}

// Create context.
GLASSCtx glassCreateContext(const GLASSCtxParams* ctxParams);

// Create context with default params.
KYGX_INLINE GLASSCtx glassCreateDefaultContext(GLASSVersion version) {
    GLASSCtxParams defaultParams;
    glassGetDefaultContextParams(&defaultParams, version);
    return glassCreateContext(&defaultParams);
}

// Destroy context. Unbind if bound.
void glassDestroyContext(GLASSCtx ctx);

// Bind context and flag the state as dirty. Pass NULL to unbind.
void glassBindContext(GLASSCtx ctx);

// Get bound context. UB if no bound context.
GLASSCtx glassGetBoundContext(void);

// Whether there's a bound context.
bool glassHasBoundContext(void);

// Check if that's the bound context.
bool glassIsBoundContext(GLASSCtx ctx);

// Flush pending GPU commands to the GPU command list and clean the dirty state. Use carefully.
void glassFlushPendingGPUCommands(GLASSCtx ctx);

// Swap buffers of the bound context. UB if no bound context.
void glassSwapBuffers(void);

// Swap buffers of the contexts. Can bind any of the two.
void glassSwapContextBuffers(GLASSCtx top, GLASSCtx bottom);

// Get the screen framebuffer targeted by a context.
u8* glassGetScreenFramebuffer(GLASSCtx ctx, u16* width, u16* height, GLenum* format);

// Get context version.
GLASSVersion glassGetVersion(GLASSCtx ctx);

// Get target screen.
GLASSScreen glassGetTargetScreen(GLASSCtx ctx);

// Set target screen.
void glassSetTargetScreen(GLASSCtx ctx, GLASSScreen screen);

// Get target side.
GLASSSide glassGetTargetSide(GLASSCtx ctx);

// Set target side, flushes pending GPU commands if different.
void glassSetTargetSide(GLASSCtx ctx, GLASSSide side);

// Get GPU command list. Should only be called after GPU commands are flushed.
void glassGetGPUCommandList(GLASSCtx ctx, GLASSGPUCommandList* list);

// Set GPU command list.
void glassSetGPUCommandList(GLASSCtx ctx, const GLASSGPUCommandList* list);

// Get VSync.
bool glassHasVSync(GLASSCtx ctx);

// Set VSync.
void glassSetVSync(GLASSCtx ctx, bool enabled);

// Get horizontal flip.
bool glassHasHorizontalFlip(GLASSCtx ctx);

// Set horizontal flip.
void glassSetHorizontalFlip(GLASSCtx ctx, bool enabled);

// Get flush all linear mem.
bool glassFlushesAllLinearMem(GLASSCtx ctx);

// Set flush all linear mem.
void glassSetFlushAllLinearMem(GLASSCtx ctx, bool enabled);

// Get downscale.
GLASSDownscale glassGetDownscale(GLASSCtx ctx);

// Set downscale.
void glassSetDownscale(GLASSCtx ctx, GLASSDownscale downscale);

// Move Tex3DS texture data in the currently bound texture object. UB if no bound context.
void glassMoveTex3DS(RIPTex3DS* tex);

void* glassHeapAlloc(size_t size);
void glassHeapFree(void* p);
size_t glassHeapSize(const void* p);
bool glassIsHeap(const void* p);

void* glassLinearAlloc(size_t size);
void glassLinearFree(void* p);
size_t glassLinearSize(const void* p);
bool glassIsLinear(const void* p);

void* glassVRAMAlloc(size_t size, KYGXVRAMBank bank);
void glassVRAMFree(void* p);
size_t glassVRAMSize(const void* p);
bool glassIsVRAM(const void* p);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _GLASS_H */