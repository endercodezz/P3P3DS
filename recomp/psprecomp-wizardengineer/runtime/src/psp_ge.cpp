#include "psp_ge.h"
#include "psp_ge_constants.h"
#include "psp_ge_draw.h"
#include "recomp.h"
#include "hle/psp_hle_kernel.h"  // psp_alloc_stack (needs recomp.h first)

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// -- Module state --
static GeState g_ge_state;
static bool g_ge_trace = false;
static int g_total_lists = 0;
static int g_total_prims = 0;
static bool g_first_prim_logged = false;

// -- Signal callback state --
static uint8_t*  g_signal_rdram  = nullptr;
static uint32_t  g_signal_fn     = 0;
static uint32_t  g_signal_arg    = 0;
static int       g_signal_cb_uid = -1;

// -- Finish callback state --
static uint32_t  g_finish_fn  = 0;
static uint32_t  g_finish_arg = 0;

// -- GE-callback guest stack --
// Lazily carved once from the top of PSP user memory (same allocator as
// thread stacks). GE callbacks run guest code that pushes a stack frame
// (a typical finish handler opens with `addiu sp, sp, -0x10`), so a fresh zeroed ctx
// with sp=0 would corrupt low memory. Shared by FINISH and SIGNAL paths.
static uint32_t g_ge_cb_stack_top = 0;

static uint32_t ge_cb_stack(uint8_t* rdram) {
    if (g_ge_cb_stack_top == 0) {
        g_ge_cb_stack_top = psp_alloc_stack(rdram, 0x1000);
        std::fprintf(stderr,
            "[GE] Callback guest stack carved: sp=0x%08X\n",
            g_ge_cb_stack_top);
    }
    return g_ge_cb_stack_top;
}

// -- Call stack entry for CALL/RET --
struct GeCallStackEntry {
    uint32_t pc;
    uint32_t offset_addr;
    uint32_t base_addr;
};

// -- Helpers --

/// Resolve a 24-bit address field using the current BASE register.
static inline uint32_t resolve_addr(uint32_t data) {
    return g_ge_state.base_addr | (data & 0x00FFFFFF);
}

/// Convert 24-bit GE float data to IEEE 754 float.
/// GE encodes floats as the upper 24 bits of a 32-bit IEEE float.
static inline float data_to_float(uint32_t data) {
    uint32_t bits = data << 8;
    float result;
    std::memcpy(&result, &bits, sizeof(float));
    return result;
}

// -- Atexit summary --
static void ge_atexit_summary() {
    std::fprintf(stderr,
        "[GE_SUMMARY] Total display lists processed: %d\n",
        g_total_lists);
    std::fprintf(stderr,
        "[GE_SUMMARY] Total PRIM commands: %d\n",
        g_total_prims);
}

// -- Public API --

const GeState& ge_get_state() {
    return g_ge_state;
}

GeState& ge_get_state_mut() {
    return g_ge_state;
}

void ge_init() {
    g_ge_state.reset();

    // Set reasonable defaults
    g_ge_state.framebuf_format = GE_FORMAT_8888;
    g_ge_state.tex_scale_u = 1.0f;
    g_ge_state.tex_scale_v = 1.0f;
    g_ge_state.viewport_x_center = 2048.0f;
    g_ge_state.viewport_y_center = 2048.0f;
    g_ge_state.viewport_z_center = 32767.5f;
    g_ge_state.viewport_x_scale = 240.0f;
    g_ge_state.viewport_y_scale = -136.0f;
    g_ge_state.viewport_z_scale = 32767.5f;
    g_ge_state.min_z = 0;          // depth range default [0, 65535]
    g_ge_state.max_z = 65535;
    g_ge_state.shade_mode = 1;  // gouraud
    g_ge_state.depth_func = GE_COMP_GEQUAL;

    // Check GE trace env var
    const char* trace_env = std::getenv("PSPRECOMP_GE_TRACE");
    g_ge_trace = (trace_env && trace_env[0] == '1');

    if (g_ge_trace) {
        std::fprintf(stderr, "[GE] Trace mode enabled\n");
    }

    g_total_lists = 0;
    g_total_prims = 0;
    g_first_prim_logged = false;
    std::atexit(ge_atexit_summary);
    std::fprintf(stderr, "[GE] Initialized\n");
}

void ge_shutdown() {
    std::fprintf(stderr,
        "[GE] Shutdown (processed %d display lists)\n",
        g_total_lists);
}

void ge_set_signal_callback(
    uint8_t* rdram, uint32_t signal_fn,
    uint32_t signal_arg, int cb_uid
) {
    g_signal_rdram  = rdram;
    g_signal_fn     = signal_fn;
    g_signal_arg    = signal_arg;
    g_signal_cb_uid = cb_uid;
    std::fprintf(stderr,
        "[GE] Signal callback registered: fn=0x%08X arg=0x%08X uid=%d\n",
        signal_fn, signal_arg, cb_uid);
}

void ge_set_finish_callback(
    uint32_t finish_fn, uint32_t finish_arg
) {
    g_finish_fn  = finish_fn;
    g_finish_arg = finish_arg;
    std::fprintf(stderr,
        "[GE] Finish callback registered: fn=0x%08X arg=0x%08X\n",
        finish_fn, finish_arg);
}

GeListResult ge_process_display_list(
    uint8_t* rdram,
    uint32_t list_addr,
    uint32_t stall_addr
) {
    uint32_t pc = list_addr;
    int prim_count = 0;
    int cmd_count = 0;

    // Call stack for CALL/RET
    std::array<GeCallStackEntry, GE_CALL_STACK_SIZE> call_stack{};
    int stack_ptr = 0;

    // Begin list: bind FBO, set viewport
    ge_draw_begin_list();

    // Clear log-once flags for this list (allow re-warn per list)
    // Actually, keep across lists to avoid spam -- only clear on init

    while (cmd_count < GE_MAX_COMMANDS) {
        // Stall check
        if (stall_addr != 0 && pc >= stall_addr) {
            if (g_ge_trace) {
                std::fprintf(stderr,
                    "[GE] Hit stall addr 0x%08X at pc 0x%08X\n",
                    stall_addr, pc);
            }
            // Return stalled result — caller keeps the list alive
            return GeListResult{pc, false, prim_count};
        }

        uint32_t word = psp_mem_read<uint32_t>(rdram, pc);
        uint8_t cmd = static_cast<uint8_t>(word >> 24);
        uint32_t data = word & 0x00FFFFFF;

        if (g_ge_trace) {
            std::fprintf(stderr,
                "[GE:0x%08X] cmd=0x%02X data=0x%06X\n",
                pc, cmd, data);
        }

        switch (cmd) {
        // ---- Control flow ----
        case GE_CMD_NOP:
            break;

        case GE_CMD_END:
            if (g_ge_trace) {
                std::fprintf(stderr,
                    "[GE] END at 0x%08X (%d PRIMs, %d cmds)\n",
                    pc, prim_count, cmd_count);
            }
            ge_draw_end_list();   // marks front buffer dirty (liveness net)
            g_total_lists++;
            // Present is decoupled from list completion (issue #59): it is
            // driven only by the guest's page-flip (sceDisplaySetFrameBuf),
            // with ge_draw_present_if_stale as a no-flip safety net. The
            // per-END auto-present was removed — it published intermediate
            // buffers (e.g. a trailing clear) as if they were frames.
            return GeListResult{pc + 4, true, prim_count};

        case GE_CMD_FINISH: {
            // FINISH signals that the display list is complete.
            // On real PSP the GE FINISH interrupt fires the registered
            // finish callback (sceGeSetCallback finish_func,
            // registered by the game) with a0 = FINISH data & 0xffff (the token the
            // game embedded in the list -- 0xffff for the frame tick)
            // and a1 = the registered finish arg. The handler pushes a
            // stack frame, so it needs a valid guest sp.
            if (g_finish_fn != 0) {
                FuncPtr fn = RECOMP_LOOKUP(g_finish_fn);
                if (fn) {
                    static bool finish_logged = false;
                    if (!finish_logged) {
                        std::fprintf(stderr,
                            "[GE] finish-cb fn=0x%08X token=0x%04X "
                            "arg=0x%08X\n",
                            g_finish_fn, data & 0xFFFF, g_finish_arg);
                        finish_logged = true;
                    }
                    recomp_context fin_ctx{};
                    fin_ctx.r[4] = static_cast<int64_t>(data & 0xFFFF);
                    fin_ctx.r[5] = static_cast<int64_t>(g_finish_arg);
                    fin_ctx.r[29] = static_cast<int32_t>(
                        ge_cb_stack(rdram));
                    fn(rdram, &fin_ctx);
                } else {
                    static bool finish_miss_logged = false;
                    if (!finish_miss_logged) {
                        std::fprintf(stderr,
                            "[GE] FINISH: RECOMP_LOOKUP(0x%08X) MISS\n",
                            g_finish_fn);
                        finish_miss_logged = true;
                    }
                }
            }
            break;
        }

        case GE_CMD_SIGNAL: {
            // SIGNAL behaviors 0x10/0x11/0x12 are GE flow control
            // (PPSSPP GPUCommon::ProcessSignal): JUMP/CALL/RET. They
            // pair with the following END word, whose low 16 bits
            // supply the low half of the target address. The paired
            // END is consumed (skipped) -- it is NOT a list-end.
            // These must NOT fire the user signal callback.
            uint32_t behavior = (data >> 16) & 0xFF;
            uint32_t signal_val = data & 0xFFFF;
            if (behavior == 0x10 || behavior == 0x11) {
                uint32_t end_data =
                    psp_mem_read<uint32_t>(rdram, pc + 4);
                uint32_t target =
                    ((signal_val << 16) | (end_data & 0xFFFF))
                    & 0xFFFFFFFC;  // PPSSPP parity: keep bits 28-31
                                   // so uncached-space pcs still
                                   // compare against stall_addr
                if (g_ge_trace) {
                    std::fprintf(stderr,
                        "[GE] SIGNAL %s at 0x%08X -> 0x%08X\n",
                        behavior == 0x11 ? "CALL" : "JUMP",
                        pc, target);
                }
                if (behavior == 0x11) {
                    if (stack_ptr >= GE_CALL_STACK_SIZE) {
                        // PPSSPP behavior: ignore the call, skip the
                        // paired END and keep processing (do not park
                        // the list as stalled).
                        std::fprintf(stderr,
                            "[GE] SIGNAL CALL stack overflow "
                            "at 0x%08X -- skipping\n", pc);
                        pc += 4;
                        break;
                    }
                    // Return to the word AFTER the paired END
                    call_stack[stack_ptr].pc = pc + 8;
                    call_stack[stack_ptr].offset_addr =
                        g_ge_state.offset_addr;
                    call_stack[stack_ptr].base_addr =
                        g_ge_state.base_addr;
                    stack_ptr++;
                }
                pc = target - 4;  // -4: pc += 4 at end of loop
                break;
            }
            if (behavior == 0x12) {
                if (stack_ptr <= 0) {
                    std::fprintf(stderr,
                        "[GE] SIGNAL RET with empty stack "
                        "at 0x%08X\n", pc);
                    // Skip the paired END and keep processing --
                    // do not treat it as list-end.
                    pc += 4;
                    break;
                }
                stack_ptr--;
                g_ge_state.offset_addr =
                    call_stack[stack_ptr].offset_addr;
                g_ge_state.base_addr =
                    call_stack[stack_ptr].base_addr;
                if (g_ge_trace) {
                    std::fprintf(stderr,
                        "[GE] SIGNAL RET at 0x%08X -> 0x%08X\n",
                        pc, call_stack[stack_ptr].pc);
                }
                pc = call_stack[stack_ptr].pc - 4;
                break;
            }
            // Fire the registered GE signal callback.
            // On real PSP the GE interrupt handler calls this when the
            // SIGNAL command is encountered during list processing.
            // The game's registered signal callback calls sceKernelSetEventFlag
            // which wakes the thread that calls sceKernelSignalSema(uid=259).
            std::fprintf(stderr,
                "[GE] SIGNAL cmd at 0x%08X data=0x%06X fn=0x%08X\n",
                pc, data, g_signal_fn);
            if (g_signal_fn != 0) {
                FuncPtr fn = RECOMP_LOOKUP(g_signal_fn);
                if (fn) {
                    recomp_context sig_ctx{};
                    // a0 = SIGNAL data & 0xffff (the in-list token),
                    // matching the FINISH path. The handler runs guest
                    // code, so give it the carved guest stack.
                    sig_ctx.r[4] = static_cast<int64_t>(data & 0xFFFF);
                    sig_ctx.r[5] = static_cast<int64_t>(g_signal_arg);
                    sig_ctx.r[29] = static_cast<int32_t>(
                        ge_cb_stack(rdram));
                    fn(g_signal_rdram ? g_signal_rdram : rdram, &sig_ctx);
                } else {
                    std::fprintf(stderr,
                        "[GE] SIGNAL: RECOMP_LOOKUP(0x%08X) MISS\n",
                        g_signal_fn);
                }
            }
            break;
        }

        case GE_CMD_JUMP: {
            uint32_t target = resolve_addr(data & 0x00FFFFFC);
            pc = target - 4;  // -4 because pc += 4 at end of loop
            break;
        }

        case GE_CMD_CALL: {
            if (stack_ptr >= GE_CALL_STACK_SIZE) {
                std::fprintf(stderr,
                    "[GE] CALL stack overflow at 0x%08X\n", pc);
                return GeListResult{pc, false, prim_count};
            }
            call_stack[stack_ptr].pc = pc + 4;
            call_stack[stack_ptr].offset_addr =
                g_ge_state.offset_addr;
            call_stack[stack_ptr].base_addr =
                g_ge_state.base_addr;
            stack_ptr++;
            uint32_t target = resolve_addr(data & 0x00FFFFFC);
            pc = target - 4;
            break;
        }

        case GE_CMD_RET: {
            if (stack_ptr <= 0) {
                std::fprintf(stderr,
                    "[GE] RET with empty stack at 0x%08X\n", pc);
                return GeListResult{pc, false, prim_count};
            }
            stack_ptr--;
            g_ge_state.offset_addr =
                call_stack[stack_ptr].offset_addr;
            g_ge_state.base_addr =
                call_stack[stack_ptr].base_addr;
            pc = call_stack[stack_ptr].pc - 4;
            break;
        }

        case GE_CMD_BJUMP:
            // Bounding box conditional jump -- skip for Phase 5
            break;

        // ---- Address / type ----
        case GE_CMD_BASE:
            g_ge_state.base_addr = (data << 8) & 0xFF000000;
            break;

        case GE_CMD_OFFSETADDR:
            g_ge_state.offset_addr = data << 8;
            break;

        case GE_CMD_ORIGIN:
            g_ge_state.offset_addr = pc;
            break;

        case GE_CMD_VADDR:
            g_ge_state.vertex_addr = resolve_addr(data);
            break;

        case GE_CMD_IADDR:
            g_ge_state.index_addr = resolve_addr(data);
            break;

        case GE_CMD_VERTEXTYPE:
            g_ge_state.vertex_type = data;
            break;

        // ---- PRIM (draw) ----
        case GE_CMD_PRIM: {
            int prim_type = (data >> 16) & 0x7;
            int count = data & 0xFFFF;
            prim_count++;
            g_total_prims++;
            if (!g_first_prim_logged) {
                std::fprintf(stderr,
                    "[GE] First PRIM command received! "
                    "list_id=%d type=%d count=%d\n",
                    g_total_lists, prim_type, count);
                g_first_prim_logged = true;
            }
            if (g_ge_trace) {
                std::fprintf(stderr,
                    "[GE] PRIM type=%d count=%d "
                    "vtype=0x%06X vaddr=0x%08X\n",
                    prim_type, count,
                    g_ge_state.vertex_type,
                    g_ge_state.vertex_addr);
            }
            ge_draw_prim(rdram, prim_type, count);
            break;
        }

        // ---- Matrix data ----
        case GE_CMD_WORLDMATRIXNUMBER:
            g_ge_state.world_mtx_num = data & 0xF;
            break;
        case GE_CMD_WORLDMATRIXDATA:
            if (g_ge_state.world_mtx_num < 12) {
                g_ge_state.world_matrix[
                    g_ge_state.world_mtx_num++] =
                    data_to_float(data);
            }
            break;

        case GE_CMD_VIEWMATRIXNUMBER:
            g_ge_state.view_mtx_num = data & 0xF;
            break;
        case GE_CMD_VIEWMATRIXDATA:
            if (g_ge_state.view_mtx_num < 12) {
                g_ge_state.view_matrix[
                    g_ge_state.view_mtx_num++] =
                    data_to_float(data);
            }
            break;

        case GE_CMD_PROJMATRIXNUMBER:
            g_ge_state.proj_mtx_num = data & 0xF;
            break;
        case GE_CMD_PROJMATRIXDATA:
            if (g_ge_state.proj_mtx_num < 16) {
                g_ge_state.proj_matrix[
                    g_ge_state.proj_mtx_num++] =
                    data_to_float(data);
            }
            break;

        case GE_CMD_TGENMATRIXNUMBER:
            g_ge_state.tgen_mtx_num = data & 0xF;
            break;
        case GE_CMD_TGENMATRIXDATA:
            if (g_ge_state.tgen_mtx_num < 12) {
                g_ge_state.tgen_matrix[
                    g_ge_state.tgen_mtx_num++] =
                    data_to_float(data);
            }
            break;

        // ---- Viewport ----
        case GE_CMD_VIEWPORTXSCALE:
            g_ge_state.viewport_x_scale = data_to_float(data);
            break;
        case GE_CMD_VIEWPORTYSCALE:
            g_ge_state.viewport_y_scale = data_to_float(data);
            break;
        case GE_CMD_VIEWPORTZSCALE:
            g_ge_state.viewport_z_scale = data_to_float(data);
            break;
        case GE_CMD_VIEWPORTXCENTER:
            g_ge_state.viewport_x_center = data_to_float(data);
            break;
        case GE_CMD_VIEWPORTYCENTER:
            g_ge_state.viewport_y_center = data_to_float(data);
            break;
        case GE_CMD_VIEWPORTZCENTER:
            g_ge_state.viewport_z_center = data_to_float(data);
            break;

        case GE_CMD_TEXSCALEU:
            g_ge_state.tex_scale_u = data_to_float(data);
            break;
        case GE_CMD_TEXSCALEV:
            g_ge_state.tex_scale_v = data_to_float(data);
            break;
        case GE_CMD_TEXOFFSETU:
            g_ge_state.tex_offset_u = data_to_float(data);
            break;
        case GE_CMD_TEXOFFSETV:
            g_ge_state.tex_offset_v = data_to_float(data);
            break;

        case GE_CMD_OFFSETX:
            g_ge_state.offset_x = data;
            break;
        case GE_CMD_OFFSETY:
            g_ge_state.offset_y = data;
            break;

        // ---- Texture state ----
        case GE_CMD_TEXADDR0: case GE_CMD_TEXADDR1:
        case GE_CMD_TEXADDR2: case GE_CMD_TEXADDR3:
        case GE_CMD_TEXADDR4: case GE_CMD_TEXADDR5:
        case GE_CMD_TEXADDR6: case GE_CMD_TEXADDR7:
            g_ge_state.tex_addr[cmd - GE_CMD_TEXADDR0] = data;
            break;

        case GE_CMD_TEXBUFWIDTH0: case GE_CMD_TEXBUFWIDTH1:
        case GE_CMD_TEXBUFWIDTH2: case GE_CMD_TEXBUFWIDTH3:
        case GE_CMD_TEXBUFWIDTH4: case GE_CMD_TEXBUFWIDTH5:
        case GE_CMD_TEXBUFWIDTH6: case GE_CMD_TEXBUFWIDTH7:
            g_ge_state.tex_bufw[cmd - GE_CMD_TEXBUFWIDTH0] = data;
            break;

        case GE_CMD_TEXSIZE0: case GE_CMD_TEXSIZE1:
        case GE_CMD_TEXSIZE2: case GE_CMD_TEXSIZE3:
        case GE_CMD_TEXSIZE4: case GE_CMD_TEXSIZE5:
        case GE_CMD_TEXSIZE6: case GE_CMD_TEXSIZE7:
            g_ge_state.tex_size[cmd - GE_CMD_TEXSIZE0] = data;
            break;

        case GE_CMD_TEXFORMAT:
            g_ge_state.tex_format = data & 0xF;
            break;
        case GE_CMD_TEXMODE:
            g_ge_state.tex_mode = data;
            break;
        case GE_CMD_TEXFUNC:
            g_ge_state.tex_func = data & 0x7;
            break;
        case GE_CMD_TEXFILTER:
            g_ge_state.tex_filter = data;
            break;
        case GE_CMD_TEXWRAP:
            g_ge_state.tex_wrap = data;
            break;
        case GE_CMD_TEXENVCOLOR:
            g_ge_state.tex_env_color = data;
            break;
        case GE_CMD_TEXFLUSH:
        case GE_CMD_TEXSYNC:
            break;  // No-op for our implementation

        // ---- CLUT ----
        case GE_CMD_CLUTADDR:
            g_ge_state.clut_addr = data;
            break;
        case GE_CMD_CLUTADDRUPPER:
            g_ge_state.clut_addr_upper = data;
            break;
        case GE_CMD_CLUTFORMAT:
            g_ge_state.clut_format = data;
            break;
        case GE_CMD_LOADCLUT:
            // Trigger CLUT load (texture cache handles this)
            break;

        // ---- Enable flags ----
        case GE_CMD_TEXTUREMAPENABLE:
            g_ge_state.texture_enable = (data & 1) != 0;
            break;
        case GE_CMD_LIGHTINGENABLE:
            g_ge_state.lighting_enable = (data & 1) != 0;
            break;
        case GE_CMD_ALPHABLENDENABLE:
            g_ge_state.alpha_blend_enable = (data & 1) != 0;
            break;
        case GE_CMD_ALPHATESTENABLE:
            g_ge_state.alpha_test_enable = (data & 1) != 0;
            break;
        case GE_CMD_ZTESTENABLE:
            g_ge_state.depth_test_enable = (data & 1) != 0;
            break;
        case GE_CMD_CULLFACEENABLE:
            g_ge_state.cull_enable = (data & 1) != 0;
            break;
        case GE_CMD_FOGENABLE:
            g_ge_state.fog_enable = (data & 1) != 0;
            break;
        case GE_CMD_STENCILTESTENABLE:
            g_ge_state.stencil_test_enable = (data & 1) != 0;
            break;

        // Ignored enable flags (no effect in Phase 5)
        case GE_CMD_LIGHTENABLE0: case GE_CMD_LIGHTENABLE1:
        case GE_CMD_LIGHTENABLE2: case GE_CMD_LIGHTENABLE3:
        case GE_CMD_DEPTHCLAMPENABLE:
        case GE_CMD_DITHERENABLE:
        case GE_CMD_ANTIALIASENABLE:
        case GE_CMD_PATCHCULLENABLE:
        case GE_CMD_COLORTESTENABLE:
        case GE_CMD_LOGICOPENABLE:
            break;

        // ---- Render state ----
        case GE_CMD_CLEARMODE:
            g_ge_state.clear_mode = (data & 1) != 0;
            g_ge_state.clear_flags = (data >> 8) & 0x7;
            break;

        case GE_CMD_BLENDMODE:
            g_ge_state.blend_mode = data;
            break;
        case GE_CMD_BLENDFIXEDA:
            g_ge_state.blend_fix_a = data;
            break;
        case GE_CMD_BLENDFIXEDB:
            g_ge_state.blend_fix_b = data;
            break;

        case GE_CMD_ALPHATEST:
            g_ge_state.alpha_test = data;
            break;
        case GE_CMD_STENCILTEST:
            g_ge_state.stencil_test = data;
            break;
        case GE_CMD_STENCILOP:
            g_ge_state.stencil_op = data;
            break;
        case GE_CMD_ZTEST:
            g_ge_state.depth_func = data & 0x7;
            break;
        case GE_CMD_ZWRITEDISABLE:
            g_ge_state.depth_write_disable = (data & 1) != 0;
            break;
        case GE_CMD_CULL:
            g_ge_state.cull_face = data & 1;
            break;

        // ---- Framebuffer ----
        case GE_CMD_FRAMEBUFPTR:
            g_ge_state.framebuf_ptr = data;
            // Bind-on-FRAMEBUFPTR (issue #59): a list that switches its
            // render target mid-stream (clear buffer A then draw buffer B)
            // must composite into the right per-address FBO. Cheap (a bind).
            ge_draw_select_target(g_ge_state.framebuf_ptr);
            break;
        case GE_CMD_FRAMEBUFWIDTH:
            g_ge_state.framebuf_width = data;
            break;
        case GE_CMD_FRAMEBUFPIXFORMAT:
            g_ge_state.framebuf_format = data & 0x3;
            break;

        // ---- Material / ambient ----
        case GE_CMD_SHADEMODE:
            g_ge_state.shade_mode = data & 1;
            break;
        case GE_CMD_AMBIENTCOLOR:
            g_ge_state.ambient_color = data;
            break;
        case GE_CMD_AMBIENTALPHA:
            g_ge_state.ambient_alpha = data;
            break;
        case GE_CMD_MATERIALEMISSIVE:
            g_ge_state.material_emissive = data;
            break;
        case GE_CMD_MATERIALAMBIENT:
            g_ge_state.material_ambient = data;
            break;
        case GE_CMD_MATERIALDIFFUSE:
            g_ge_state.material_diffuse = data;
            break;

        // ---- Fog ----
        case GE_CMD_FOGCOLOR:
            g_ge_state.fog_color = data;
            break;
        case GE_CMD_FOG1:
        case GE_CMD_FOG2:
            break;  // Store if needed later

        // ---- Region / scissor ----
        case GE_CMD_REGION1:
            g_ge_state.region1 = data;
            break;
        case GE_CMD_REGION2:
            g_ge_state.region2 = data;
            break;
        case GE_CMD_SCISSOR1:
            g_ge_state.scissor1 = data;
            break;
        case GE_CMD_SCISSOR2:
            g_ge_state.scissor2 = data;
            break;
        case GE_CMD_MINZ:
            g_ge_state.min_z = data & 0xFFFFu;
            break;
        case GE_CMD_MAXZ:
            g_ge_state.max_z = data & 0xFFFFu;
            break;

        // ---- Depth / Z buffer ----
        case GE_CMD_ZBUFPTR:
        case GE_CMD_ZBUFWIDTH:
            break;  // Not needed for Phase 5

        // ---- Mask ----
        case GE_CMD_MASKRGB:
        case GE_CMD_MASKALPHA:
            break;

        // ---- Color test ----
        case GE_CMD_COLORTEST:
        case GE_CMD_COLORREF:
        case GE_CMD_COLORTESTMASK:
            break;

        // ---- Misc ignored for Phase 5 ----
        case GE_CMD_BONEMATRIXNUMBER:
        case GE_CMD_BONEMATRIXDATA:
        case GE_CMD_MORPHWEIGHT0: case GE_CMD_MORPHWEIGHT1:
        case GE_CMD_MORPHWEIGHT2: case GE_CMD_MORPHWEIGHT3:
        case GE_CMD_MORPHWEIGHT4: case GE_CMD_MORPHWEIGHT5:
        case GE_CMD_MORPHWEIGHT6: case GE_CMD_MORPHWEIGHT7:
        case GE_CMD_PATCHDIVISION: case GE_CMD_PATCHPRIMITIVE:
        case GE_CMD_PATCHFACING:
        case GE_CMD_BEZIER: case GE_CMD_SPLINE:
        case GE_CMD_BOUNDINGBOX:
        case GE_CMD_MATERIALSPECULAR:
        case GE_CMD_MATERIALALPHA:
        case GE_CMD_MATERIALSPECULARCOEF:
        case GE_CMD_MATERIALUPDATE:
        case GE_CMD_REVERSENORMAL:
        case GE_CMD_LIGHTMODE:
        case GE_CMD_TEXMAPMODE: case GE_CMD_TEXSHADELS:
        case GE_CMD_TEXLEVEL: case GE_CMD_TEXLODSLOPE:
        case GE_CMD_LOGICOP:
        case GE_CMD_DITH0: case GE_CMD_DITH1:
        case GE_CMD_DITH2: case GE_CMD_DITH3:
        case GE_CMD_TRANSFERSRC: case GE_CMD_TRANSFERSRCW:
        case GE_CMD_TRANSFERDST: case GE_CMD_TRANSFERDSTW:
        case GE_CMD_TRANSFERSRCPOS: case GE_CMD_TRANSFERDSTPOS:
        case GE_CMD_TRANSFERSIZE: case GE_CMD_TRANSFERSTART:
            break;

        // ---- Light positions/colors (skip all in Phase 5) ----
        case GE_CMD_LIGHTTYPE0: case GE_CMD_LIGHTTYPE1:
        case GE_CMD_LIGHTTYPE2: case GE_CMD_LIGHTTYPE3:
        case GE_CMD_LX0: case GE_CMD_LY0: case GE_CMD_LZ0:
        case GE_CMD_LX1: case GE_CMD_LY1: case GE_CMD_LZ1:
        case GE_CMD_LX2: case GE_CMD_LY2: case GE_CMD_LZ2:
        case GE_CMD_LX3: case GE_CMD_LY3: case GE_CMD_LZ3:
        case GE_CMD_LDX0: case GE_CMD_LDY0: case GE_CMD_LDZ0:
        case GE_CMD_LDX1: case GE_CMD_LDY1: case GE_CMD_LDZ1:
        case GE_CMD_LDX2: case GE_CMD_LDY2: case GE_CMD_LDZ2:
        case GE_CMD_LDX3: case GE_CMD_LDY3: case GE_CMD_LDZ3:
        case GE_CMD_LKA0: case GE_CMD_LKB0: case GE_CMD_LKC0:
        case GE_CMD_LKA1: case GE_CMD_LKB1: case GE_CMD_LKC1:
        case GE_CMD_LKA2: case GE_CMD_LKB2: case GE_CMD_LKC2:
        case GE_CMD_LKA3: case GE_CMD_LKB3: case GE_CMD_LKC3:
        case GE_CMD_LKS0: case GE_CMD_LKS1:
        case GE_CMD_LKS2: case GE_CMD_LKS3:
        case GE_CMD_LKO0: case GE_CMD_LKO1:
        case GE_CMD_LKO2: case GE_CMD_LKO3:
        case GE_CMD_LAC0: case GE_CMD_LDC0: case GE_CMD_LSC0:
        case GE_CMD_LAC1: case GE_CMD_LDC1: case GE_CMD_LSC1:
        case GE_CMD_LAC2: case GE_CMD_LDC2: case GE_CMD_LSC2:
        case GE_CMD_LAC3: case GE_CMD_LDC3: case GE_CMD_LSC3:
            break;

        default:
            // Log-once for truly unknown commands
            if (!g_ge_state.cmd_warned[cmd]) {
                std::fprintf(stderr,
                    "[GE] Unknown cmd 0x%02X data=0x%06X "
                    "at pc=0x%08X\n",
                    cmd, data, pc);
                g_ge_state.cmd_warned[cmd] = true;
            }
            break;
        }

        pc += 4;
        cmd_count++;
    }

    if (cmd_count >= GE_MAX_COMMANDS) {
        std::fprintf(stderr,
            "[GE] Hit command limit (%d) without END "
            "at pc=0x%08X\n",
            GE_MAX_COMMANDS, pc);
    }

    g_total_lists++;
    return GeListResult{pc, false, prim_count};
}
