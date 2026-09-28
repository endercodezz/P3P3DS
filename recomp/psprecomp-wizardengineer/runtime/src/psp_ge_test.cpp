#include "psp_ge_test.h"
#include "psp_ge.h"
#include "psp_ge_draw.h"
#include "psp_ge_constants.h"
#include "psp_memory.h"
#include "recomp.h"

#include <glad/glad.h>
#include <cstdio>
#include <cstring>
#include <cmath>

/// Test region in rdram that won't conflict with game data.
/// Using an area in VRAM space (0x04100000).
static constexpr uint32_t TEST_LIST_ADDR = 0x04100000u;
static constexpr uint32_t TEST_VERT_ADDR = 0x04100200u;

/// Expected test color: teal (R=0x44, G=0x88, B=0xFF, A=0xFF)
static constexpr uint8_t EXPECTED_R = 0x44;
static constexpr uint8_t EXPECTED_G = 0x88;
static constexpr uint8_t EXPECTED_B = 0xFF;
static constexpr uint8_t EXPECTED_A = 0xFF;

/// Write a 32-bit GE command word to rdram at the given address.
static void write_cmd(
    uint8_t* rdram, uint32_t addr, uint32_t word
) {
    uint32_t masked = addr & PSP_ADDR_MASK;
    rdram[masked + 0] = (word) & 0xFF;
    rdram[masked + 1] = (word >> 8) & 0xFF;
    rdram[masked + 2] = (word >> 16) & 0xFF;
    rdram[masked + 3] = (word >> 24) & 0xFF;
}

/// Write a float to rdram at the given address (little-endian).
static void write_float(
    uint8_t* rdram, uint32_t addr, float val
) {
    uint32_t bits;
    std::memcpy(&bits, &val, sizeof(float));
    write_cmd(rdram, addr, bits);
}

/// Write a uint32 to rdram at the given address (little-endian).
static void write_u32(
    uint8_t* rdram, uint32_t addr, uint32_t val
) {
    write_cmd(rdram, addr, val);
}

bool ge_run_self_test(uint8_t* rdram) {
    std::fprintf(stderr,
        "[GE-TEST] Running solid-fill self-test...\n");

    // Build display list: through-mode rectangle with solid color
    // VTYPE: through-mode + color 8888 + position float
    // through = bit 23 = 0x800000
    // col_8888 = 7 << 2 = 0x1C
    // pos_float = 3 << 7 = 0x180
    uint32_t vtype = 0x80019Cu;

    uint32_t pc = TEST_LIST_ADDR;

    // Cmd 0: BASE (upper 8 bits of address space)
    // 0x04000000 >> 8 = 0x040000; handler: (data<<8)&0xFF000000
    write_cmd(rdram, pc,
              (static_cast<uint32_t>(GE_CMD_BASE) << 24)
              | 0x040000u);
    pc += 4;

    // Cmd 1: VERTEXTYPE
    write_cmd(rdram, pc,
              (static_cast<uint32_t>(GE_CMD_VERTEXTYPE)
               << 24) | vtype);
    pc += 4;

    // Cmd 2: VADDR (point to vertex data)
    // VADDR uses the lower 24 bits as address
    write_cmd(rdram, pc,
              (static_cast<uint32_t>(GE_CMD_VADDR) << 24)
              | (TEST_VERT_ADDR & 0x00FFFFFF));
    pc += 4;

    // Cmd 3: PRIM - rectangle (type 6), count 2
    uint32_t prim_data = (GE_PRIM_RECTANGLES << 16) | 2;
    write_cmd(rdram, pc,
              (static_cast<uint32_t>(GE_CMD_PRIM) << 24)
              | prim_data);
    pc += 4;

    // Cmd 4: END
    write_cmd(rdram, pc,
              static_cast<uint32_t>(GE_CMD_END) << 24);

    // Write vertex data at TEST_VERT_ADDR
    // Through-mode with color_8888 + pos_float:
    // Vertex layout: color(4 bytes) + pos(3 floats)
    // Total per vertex: 4 + 12 = 16 bytes
    uint32_t va = TEST_VERT_ADDR;

    // Vertex 0: top-left corner (0, 0, 0)
    // Color: RGBA = 0x44, 0x88, 0xFF, 0xFF
    uint32_t color = static_cast<uint32_t>(EXPECTED_R)
                     | (static_cast<uint32_t>(EXPECTED_G) << 8)
                     | (static_cast<uint32_t>(EXPECTED_B) << 16)
                     | (static_cast<uint32_t>(EXPECTED_A) << 24);
    write_u32(rdram, va, color);      va += 4;
    write_float(rdram, va, 0.0f);     va += 4;  // x
    write_float(rdram, va, 0.0f);     va += 4;  // y
    write_float(rdram, va, 0.0f);     va += 4;  // z

    // Vertex 1: bottom-right corner (480, 272, 0)
    write_u32(rdram, va, color);      va += 4;
    write_float(rdram, va, 480.0f);   va += 4;  // x
    write_float(rdram, va, 272.0f);   va += 4;  // y
    write_float(rdram, va, 0.0f);     va += 4;  // z

    // Process the display list
    ge_process_display_list(rdram, TEST_LIST_ADDR, 0);

    // Verify: read center pixel from FBO
    // The FBO is still bound from ge_draw_end_list
    // Rebind read framebuffer to the GE FBO
    // We need the FBO ID -- it's internal to ge_draw.
    // Use glReadPixels on the currently bound FBO.
    // ge_draw_begin_list binds it, ge_draw_end_list
    // doesn't unbind. So it should still be bound.
    // Actually, after ge_process_display_list returns,
    // the FBO may be unbound. Let's call
    // ge_present_frame to push the output, then check.

    // Read center pixel
    uint8_t pixel[4] = {};
    glReadPixels(240, 136, 1, 1,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    std::fprintf(stderr,
        "[GE-TEST] Center pixel: R=%u G=%u B=%u A=%u\n",
        pixel[0], pixel[1], pixel[2], pixel[3]);
    std::fprintf(stderr,
        "[GE-TEST] Expected:     R=%u G=%u B=%u A=%u\n",
        EXPECTED_R, EXPECTED_G, EXPECTED_B, EXPECTED_A);

    // Check with tolerance (rounding errors)
    int dr = std::abs(pixel[0] - EXPECTED_R);
    int dg = std::abs(pixel[1] - EXPECTED_G);
    int db = std::abs(pixel[2] - EXPECTED_B);
    int da = std::abs(pixel[3] - EXPECTED_A);

    bool pass = (dr <= 2 && dg <= 2
                 && db <= 2 && da <= 2);

    if (pass) {
        std::fprintf(stderr,
            "[GE-TEST] GE self-test PASS\n");
    } else {
        std::fprintf(stderr,
            "[GE-TEST] GE self-test FAIL "
            "(delta: R=%d G=%d B=%d A=%d)\n",
            dr, dg, db, da);
    }

    // Also present to window so it's visible
    ge_present_frame(rdram, 0, 0, 0);

    return pass;
}
