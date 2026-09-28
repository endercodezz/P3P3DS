#pragma once
#include "psp_ge_constants.h"
#include <cstdint>
#include <cstring>

/// Maximum call stack depth for display list CALL/RET commands.
static constexpr int GE_CALL_STACK_SIZE = 32;

/// Maximum number of GE commands processed per display list (safety valve).
static constexpr int GE_MAX_COMMANDS = 4 * 1024 * 1024;

/// GE state machine -- tracks all state-setting commands for rendering.
/// Updated by the command decoder; read by vertex, texture, and shader code.
struct GeState {
    // -- Address state --
    uint32_t base_addr;       // GE_CMD_BASE: bits [31:8] address base
    uint32_t vertex_addr;     // GE_CMD_VADDR: vertex buffer address
    uint32_t index_addr;      // GE_CMD_IADDR: index buffer address
    uint32_t offset_addr;     // GE_CMD_OFFSETADDR / ORIGIN

    // -- Vertex format --
    uint32_t vertex_type;     // GE_CMD_VERTEXTYPE: raw VTYPE bitfield

    // -- Matrices --
    float world_matrix[12];   // 4x3 column-major (world transform)
    float view_matrix[12];    // 4x3 column-major (view transform)
    float proj_matrix[16];    // 4x4 column-major (projection)
    float tgen_matrix[12];    // 4x3 column-major (texgen)
    int world_mtx_num;        // Current write index for WORLDMATRIXDATA
    int view_mtx_num;         // Current write index for VIEWMATRIXDATA
    int proj_mtx_num;         // Current write index for PROJMATRIXDATA
    int tgen_mtx_num;         // Current write index for TGENMATRIXDATA

    // -- Texture state --
    uint32_t tex_addr[8];     // GE_CMD_TEXADDR0-7: mipmap addresses
    uint32_t tex_bufw[8];     // GE_CMD_TEXBUFWIDTH0-7: buffer widths
    uint32_t tex_size[8];     // GE_CMD_TEXSIZE0-7: log2_w | (log2_h << 8)
    uint32_t tex_format;      // GE_CMD_TEXFORMAT: GETextureFormat value
    uint32_t tex_mode;        // GE_CMD_TEXMODE: bit 0 = swizzle
    uint32_t tex_func;        // GE_CMD_TEXFUNC: GeTexFunc value
    uint32_t tex_filter;      // GE_CMD_TEXFILTER
    uint32_t tex_wrap;        // GE_CMD_TEXWRAP
    uint32_t tex_env_color;   // GE_CMD_TEXENVCOLOR
    bool texture_enable;      // GE_CMD_TEXTUREMAPENABLE

    // -- CLUT state --
    uint32_t clut_addr;       // GE_CMD_CLUTADDR (low 24 bits)
    uint32_t clut_addr_upper; // GE_CMD_CLUTADDRUPPER (high bits)
    uint32_t clut_format;     // GE_CMD_CLUTFORMAT: palette format + shift

    // -- Viewport / screen --
    float viewport_x_scale, viewport_y_scale, viewport_z_scale;
    float viewport_x_center, viewport_y_center, viewport_z_center;
    float tex_scale_u, tex_scale_v;
    float tex_offset_u, tex_offset_v;
    uint32_t offset_x, offset_y;    // GE_CMD_OFFSETX/Y (1/16 subpixel)
    uint32_t region1, region2;       // GE_CMD_REGION1/2
    uint32_t scissor1, scissor2;     // GE_CMD_SCISSOR1/2
    uint32_t min_z, max_z;           // GE_CMD_MINZ/MAXZ: depth range (u16)

    // -- Framebuffer --
    uint32_t framebuf_ptr;    // GE_CMD_FRAMEBUFPTR
    uint32_t framebuf_width;  // GE_CMD_FRAMEBUFWIDTH
    uint32_t framebuf_format; // GE_CMD_FRAMEBUFPIXFORMAT

    // -- Render state --
    bool alpha_blend_enable;
    uint32_t blend_mode;      // GE_CMD_BLENDMODE: src|dst|op packed
    uint32_t blend_fix_a;     // GE_CMD_BLENDFIXEDA
    uint32_t blend_fix_b;     // GE_CMD_BLENDFIXEDB

    bool depth_test_enable;
    uint32_t depth_func;      // GE_CMD_ZTEST
    bool depth_write_disable; // GE_CMD_ZWRITEDISABLE

    bool alpha_test_enable;
    uint32_t alpha_test;      // GE_CMD_ALPHATEST: func|ref|mask packed

    bool stencil_test_enable;
    uint32_t stencil_test;    // GE_CMD_STENCILTEST
    uint32_t stencil_op;      // GE_CMD_STENCILOP

    bool cull_enable;
    uint32_t cull_face;       // GE_CMD_CULL: 0=CW, 1=CCW

    bool clear_mode;          // GE_CMD_CLEARMODE bit 0
    uint32_t clear_flags;     // GE_CMD_CLEARMODE bits [10:8]

    // -- Lighting (minimal for Phase 5) --
    bool lighting_enable;
    uint32_t shade_mode;      // 0=flat, 1=gouraud
    uint32_t ambient_color;   // GE_CMD_AMBIENTCOLOR
    uint32_t ambient_alpha;   // GE_CMD_AMBIENTALPHA
    uint32_t material_emissive;
    uint32_t material_ambient;
    uint32_t material_diffuse;

    // -- Fog --
    bool fog_enable;
    uint32_t fog_color;

    // -- Log-once tracking --
    bool cmd_warned[256];     // One flag per command ID

    /// Zero-initialize all fields.
    void reset() { std::memset(this, 0, sizeof(*this)); }
};

/// Initialize the GE subsystem. Call once before any display list processing.
void ge_init();

/// Register the GE signal callback (called from sceGeSetCallback HLE).
/// Fired when a display list encounters a SIGNAL GE command.
/// signal_func: PSP virtual address of the signal handler function.
/// signal_arg:  passed as a1 to the handler.
/// cb_uid:      the UID returned to the game (passed as a0).
void ge_set_signal_callback(
    uint8_t* rdram, uint32_t signal_func,
    uint32_t signal_arg, int cb_uid);

/// Register the GE finish callback (called from sceGeSetCallback HLE).
/// Fired when a display list executes a FINISH GE command.
/// finish_func: PSP virtual address of the finish handler function.
///              Receives a0 = FINISH data & 0xffff, a1 = finish_arg.
void ge_set_finish_callback(
    uint32_t finish_func, uint32_t finish_arg);

/// Shutdown the GE subsystem. Call during cleanup.
void ge_shutdown();

/// Process a display list starting at list_addr in rdram.
/// Result from processing a display list.
struct GeListResult {
    uint32_t stopped_pc = 0;  // PC where processing stopped
    bool completed = false;    // True if END was reached
    int prim_count = 0;        // Number of PRIM commands processed
};

/// Reads 32-bit command words, dispatches via switch, updates GeState.
/// Returns when END command is reached or stall_addr is hit (if nonzero).
GeListResult ge_process_display_list(
    uint8_t* rdram,
    uint32_t list_addr,
    uint32_t stall_addr
);

/// Read-only access to the current GE state (for vertex/texture/shader).
const GeState& ge_get_state();

/// Mutable access to GE state (for ge.cpp internal + draw wiring).
GeState& ge_get_state_mut();
