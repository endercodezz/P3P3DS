#pragma once
#include <cstdint>

/// PSP GE Command IDs -- derived from PPSSPP GPU/ge_constants.h.
/// Each display list word: bits [31:24] = command, bits [23:0] = data.
enum GeCommand : uint8_t {
    // Control flow
    GE_CMD_NOP             = 0x00,
    GE_CMD_VADDR           = 0x01,
    GE_CMD_IADDR           = 0x02,
    GE_CMD_PRIM            = 0x04,
    GE_CMD_BEZIER          = 0x05,
    GE_CMD_SPLINE          = 0x06,
    GE_CMD_BOUNDINGBOX     = 0x07,
    GE_CMD_JUMP            = 0x08,
    GE_CMD_BJUMP           = 0x09,
    GE_CMD_CALL            = 0x0A,
    GE_CMD_RET             = 0x0B,
    GE_CMD_END             = 0x0C,
    GE_CMD_SIGNAL          = 0x0E,
    GE_CMD_FINISH          = 0x0F,

    // Address / vertex type
    GE_CMD_BASE            = 0x10,
    GE_CMD_VERTEXTYPE      = 0x12,
    GE_CMD_OFFSETADDR      = 0x13,
    GE_CMD_ORIGIN          = 0x14,
    GE_CMD_REGION1         = 0x15,
    GE_CMD_REGION2         = 0x16,

    // Enable flags
    GE_CMD_LIGHTINGENABLE      = 0x17,
    GE_CMD_LIGHTENABLE0        = 0x18,
    GE_CMD_LIGHTENABLE1        = 0x19,
    GE_CMD_LIGHTENABLE2        = 0x1A,
    GE_CMD_LIGHTENABLE3        = 0x1B,
    GE_CMD_DEPTHCLAMPENABLE    = 0x1C,
    GE_CMD_CULLFACEENABLE      = 0x1D,
    GE_CMD_TEXTUREMAPENABLE    = 0x1E,
    GE_CMD_FOGENABLE           = 0x1F,
    GE_CMD_DITHERENABLE        = 0x20,
    GE_CMD_ALPHABLENDENABLE    = 0x21,
    GE_CMD_ALPHATESTENABLE     = 0x22,
    GE_CMD_ZTESTENABLE         = 0x23,
    GE_CMD_STENCILTESTENABLE   = 0x24,
    GE_CMD_ANTIALIASENABLE     = 0x25,
    GE_CMD_PATCHCULLENABLE     = 0x26,
    GE_CMD_COLORTESTENABLE     = 0x27,
    GE_CMD_LOGICOPENABLE       = 0x28,

    // Bone / morph
    GE_CMD_BONEMATRIXNUMBER    = 0x2A,
    GE_CMD_BONEMATRIXDATA      = 0x2B,
    GE_CMD_MORPHWEIGHT0        = 0x2C,
    GE_CMD_MORPHWEIGHT1        = 0x2D,
    GE_CMD_MORPHWEIGHT2        = 0x2E,
    GE_CMD_MORPHWEIGHT3        = 0x2F,
    GE_CMD_MORPHWEIGHT4        = 0x30,
    GE_CMD_MORPHWEIGHT5        = 0x31,
    GE_CMD_MORPHWEIGHT6        = 0x32,
    GE_CMD_MORPHWEIGHT7        = 0x33,

    // Patch
    GE_CMD_PATCHDIVISION       = 0x36,
    GE_CMD_PATCHPRIMITIVE      = 0x37,
    GE_CMD_PATCHFACING         = 0x38,

    // World / View / Proj / TexGen matrices
    GE_CMD_WORLDMATRIXNUMBER   = 0x3A,
    GE_CMD_WORLDMATRIXDATA     = 0x3B,
    GE_CMD_VIEWMATRIXNUMBER    = 0x3C,
    GE_CMD_VIEWMATRIXDATA      = 0x3D,
    GE_CMD_PROJMATRIXNUMBER    = 0x3E,
    GE_CMD_PROJMATRIXDATA      = 0x3F,
    GE_CMD_TGENMATRIXNUMBER    = 0x40,
    GE_CMD_TGENMATRIXDATA      = 0x41,

    // Viewport
    GE_CMD_VIEWPORTXSCALE      = 0x42,
    GE_CMD_VIEWPORTYSCALE      = 0x43,
    GE_CMD_VIEWPORTZSCALE      = 0x44,
    GE_CMD_VIEWPORTXCENTER     = 0x45,
    GE_CMD_VIEWPORTYCENTER     = 0x46,
    GE_CMD_VIEWPORTZCENTER     = 0x47,

    // Texture scale/offset
    GE_CMD_TEXSCALEU           = 0x48,
    GE_CMD_TEXSCALEV           = 0x49,
    GE_CMD_TEXOFFSETU          = 0x4A,
    GE_CMD_TEXOFFSETV          = 0x4B,

    // Screen offset
    GE_CMD_OFFSETX             = 0x4C,
    GE_CMD_OFFSETY             = 0x4D,

    // Shade / normals / material
    GE_CMD_SHADEMODE           = 0x50,
    GE_CMD_REVERSENORMAL       = 0x51,
    GE_CMD_MATERIALUPDATE      = 0x53,
    GE_CMD_MATERIALEMISSIVE    = 0x54,
    GE_CMD_MATERIALAMBIENT     = 0x55,
    GE_CMD_MATERIALDIFFUSE     = 0x56,
    GE_CMD_MATERIALSPECULAR    = 0x57,
    GE_CMD_MATERIALALPHA       = 0x58,
    GE_CMD_MATERIALSPECULARCOEF = 0x5B,
    GE_CMD_AMBIENTCOLOR        = 0x5C,
    GE_CMD_AMBIENTALPHA        = 0x5D,
    GE_CMD_LIGHTMODE           = 0x5E,

    // Light types / positions / colors
    GE_CMD_LIGHTTYPE0          = 0x5F,
    GE_CMD_LIGHTTYPE1          = 0x60,
    GE_CMD_LIGHTTYPE2          = 0x61,
    GE_CMD_LIGHTTYPE3          = 0x62,
    GE_CMD_LX0 = 0x63, GE_CMD_LY0 = 0x64, GE_CMD_LZ0 = 0x65,
    GE_CMD_LX1 = 0x66, GE_CMD_LY1 = 0x67, GE_CMD_LZ1 = 0x68,
    GE_CMD_LX2 = 0x69, GE_CMD_LY2 = 0x6A, GE_CMD_LZ2 = 0x6B,
    GE_CMD_LX3 = 0x6C, GE_CMD_LY3 = 0x6D, GE_CMD_LZ3 = 0x6E,
    GE_CMD_LDX0 = 0x6F, GE_CMD_LDY0 = 0x70, GE_CMD_LDZ0 = 0x71,
    GE_CMD_LDX1 = 0x72, GE_CMD_LDY1 = 0x73, GE_CMD_LDZ1 = 0x74,
    GE_CMD_LDX2 = 0x75, GE_CMD_LDY2 = 0x76, GE_CMD_LDZ2 = 0x77,
    GE_CMD_LDX3 = 0x78, GE_CMD_LDY3 = 0x79, GE_CMD_LDZ3 = 0x7A,
    GE_CMD_LKA0 = 0x7B, GE_CMD_LKB0 = 0x7C, GE_CMD_LKC0 = 0x7D,
    GE_CMD_LKA1 = 0x7E, GE_CMD_LKB1 = 0x7F, GE_CMD_LKC1 = 0x80,
    GE_CMD_LKA2 = 0x81, GE_CMD_LKB2 = 0x82, GE_CMD_LKC2 = 0x83,
    GE_CMD_LKA3 = 0x84, GE_CMD_LKB3 = 0x85, GE_CMD_LKC3 = 0x86,
    GE_CMD_LKS0 = 0x87, GE_CMD_LKS1 = 0x88,
    GE_CMD_LKS2 = 0x89, GE_CMD_LKS3 = 0x8A,
    GE_CMD_LKO0 = 0x8B, GE_CMD_LKO1 = 0x8C,
    GE_CMD_LKO2 = 0x8D, GE_CMD_LKO3 = 0x8E,
    GE_CMD_LAC0 = 0x8F, GE_CMD_LDC0 = 0x90, GE_CMD_LSC0 = 0x91,
    GE_CMD_LAC1 = 0x92, GE_CMD_LDC1 = 0x93, GE_CMD_LSC1 = 0x94,
    GE_CMD_LAC2 = 0x95, GE_CMD_LDC2 = 0x96, GE_CMD_LSC2 = 0x97,
    GE_CMD_LAC3 = 0x98, GE_CMD_LDC3 = 0x99, GE_CMD_LSC3 = 0x9A,

    // Cull / framebuffer / Z buffer
    GE_CMD_CULL                = 0x9B,
    GE_CMD_FRAMEBUFPTR         = 0x9C,
    GE_CMD_FRAMEBUFWIDTH       = 0x9D,
    GE_CMD_ZBUFPTR             = 0x9E,
    GE_CMD_ZBUFWIDTH           = 0x9F,

    // Texture addresses (0xA0-0xA7)
    GE_CMD_TEXADDR0 = 0xA0, GE_CMD_TEXADDR1 = 0xA1,
    GE_CMD_TEXADDR2 = 0xA2, GE_CMD_TEXADDR3 = 0xA3,
    GE_CMD_TEXADDR4 = 0xA4, GE_CMD_TEXADDR5 = 0xA5,
    GE_CMD_TEXADDR6 = 0xA6, GE_CMD_TEXADDR7 = 0xA7,

    // Texture buffer widths (0xA8-0xAF)
    GE_CMD_TEXBUFWIDTH0 = 0xA8, GE_CMD_TEXBUFWIDTH1 = 0xA9,
    GE_CMD_TEXBUFWIDTH2 = 0xAA, GE_CMD_TEXBUFWIDTH3 = 0xAB,
    GE_CMD_TEXBUFWIDTH4 = 0xAC, GE_CMD_TEXBUFWIDTH5 = 0xAD,
    GE_CMD_TEXBUFWIDTH6 = 0xAE, GE_CMD_TEXBUFWIDTH7 = 0xAF,

    // CLUT address (0xB0-0xB1)
    GE_CMD_CLUTADDR            = 0xB0,
    GE_CMD_CLUTADDRUPPER       = 0xB1,

    // Transfer source/dest (0xB2-0xB5)
    GE_CMD_TRANSFERSRC         = 0xB2,
    GE_CMD_TRANSFERSRCW        = 0xB3,
    GE_CMD_TRANSFERDST         = 0xB4,
    GE_CMD_TRANSFERDSTW        = 0xB5,

    // Texture sizes (0xB8-0xBF)
    GE_CMD_TEXSIZE0 = 0xB8, GE_CMD_TEXSIZE1 = 0xB9,
    GE_CMD_TEXSIZE2 = 0xBA, GE_CMD_TEXSIZE3 = 0xBB,
    GE_CMD_TEXSIZE4 = 0xBC, GE_CMD_TEXSIZE5 = 0xBD,
    GE_CMD_TEXSIZE6 = 0xBE, GE_CMD_TEXSIZE7 = 0xBF,

    // Texture mode / format (PPSSPP actual values)
    GE_CMD_TEXMAPMODE          = 0xC0,
    GE_CMD_TEXSHADELS          = 0xC1,
    GE_CMD_TEXMODE             = 0xC2,
    GE_CMD_TEXFORMAT           = 0xC3,
    GE_CMD_LOADCLUT            = 0xC4,
    GE_CMD_CLUTFORMAT          = 0xC5,
    GE_CMD_TEXFILTER           = 0xC6,
    GE_CMD_TEXWRAP             = 0xC7,
    GE_CMD_TEXLEVEL            = 0xC8,
    GE_CMD_TEXFUNC             = 0xC9,
    GE_CMD_TEXENVCOLOR         = 0xCA,
    GE_CMD_TEXFLUSH            = 0xCB,
    GE_CMD_TEXSYNC             = 0xCC,
    GE_CMD_FOG1                = 0xCD,
    GE_CMD_FOG2                = 0xCE,
    GE_CMD_FOGCOLOR            = 0xCF,

    GE_CMD_TEXLODSLOPE         = 0xD0,
    GE_CMD_FRAMEBUFPIXFORMAT   = 0xD2,
    GE_CMD_CLEARMODE           = 0xD3,
    GE_CMD_SCISSOR1            = 0xD4,
    GE_CMD_SCISSOR2            = 0xD5,
    GE_CMD_MINZ                = 0xD6,
    GE_CMD_MAXZ                = 0xD7,
    GE_CMD_COLORTEST           = 0xD8,
    GE_CMD_COLORREF            = 0xD9,
    GE_CMD_COLORTESTMASK       = 0xDA,
    GE_CMD_ALPHATEST           = 0xDB,
    GE_CMD_STENCILTEST         = 0xDC,
    GE_CMD_STENCILOP           = 0xDD,
    GE_CMD_ZTEST               = 0xDE,
    GE_CMD_BLENDMODE           = 0xDF,
    GE_CMD_BLENDFIXEDA         = 0xE0,
    GE_CMD_BLENDFIXEDB         = 0xE1,
    GE_CMD_DITH0               = 0xE2,
    GE_CMD_DITH1               = 0xE3,
    GE_CMD_DITH2               = 0xE4,
    GE_CMD_DITH3               = 0xE5,
    GE_CMD_LOGICOP             = 0xE6,
    GE_CMD_ZWRITEDISABLE       = 0xE7,
    GE_CMD_MASKRGB             = 0xE8,
    GE_CMD_MASKALPHA           = 0xE9,
    GE_CMD_TRANSFERSTART       = 0xEA,
    GE_CMD_TRANSFERSRCPOS      = 0xEB,
    GE_CMD_TRANSFERDSTPOS      = 0xEC,
    GE_CMD_TRANSFERSIZE        = 0xEE,
};

/// GE Primitive types (bits [18:16] of PRIM command data).
enum GePrimType : uint8_t {
    GE_PRIM_POINTS         = 0,
    GE_PRIM_LINES          = 1,
    GE_PRIM_LINE_STRIP     = 2,
    GE_PRIM_TRIANGLES      = 3,
    GE_PRIM_TRIANGLE_STRIP = 4,
    GE_PRIM_TRIANGLE_FAN   = 5,
    GE_PRIM_RECTANGLES     = 6,
};

/// VTYPE bitfield masks and shifts.
#define GE_VTYPE_TC_SHIFT     0
#define GE_VTYPE_TC_MASK      0x3
#define GE_VTYPE_TC_NONE      0
#define GE_VTYPE_TC_8BIT      1
#define GE_VTYPE_TC_16BIT     2
#define GE_VTYPE_TC_FLOAT     3

#define GE_VTYPE_COL_SHIFT    2
#define GE_VTYPE_COL_MASK     0x7
#define GE_VTYPE_COL_NONE     0
#define GE_VTYPE_COL_565      4
#define GE_VTYPE_COL_5551     5
#define GE_VTYPE_COL_4444     6
#define GE_VTYPE_COL_8888     7

#define GE_VTYPE_NRM_SHIFT    5
#define GE_VTYPE_NRM_MASK     0x3
#define GE_VTYPE_NRM_NONE     0
#define GE_VTYPE_NRM_8BIT     1
#define GE_VTYPE_NRM_16BIT    2
#define GE_VTYPE_NRM_FLOAT    3

#define GE_VTYPE_POS_SHIFT    7
#define GE_VTYPE_POS_MASK     0x3
#define GE_VTYPE_POS_8BIT     1
#define GE_VTYPE_POS_16BIT    2
#define GE_VTYPE_POS_FLOAT    3

#define GE_VTYPE_WEIGHT_SHIFT 9
#define GE_VTYPE_WEIGHT_MASK  0x3

#define GE_VTYPE_IDX_SHIFT    11
#define GE_VTYPE_IDX_MASK     0x3
#define GE_VTYPE_IDX_NONE     0
#define GE_VTYPE_IDX_8BIT     1
#define GE_VTYPE_IDX_16BIT    2
#define GE_VTYPE_IDX_32BIT    3

#define GE_VTYPE_WEIGHTCOUNT_SHIFT 14
#define GE_VTYPE_WEIGHTCOUNT_MASK  0x7

#define GE_VTYPE_MORPHCOUNT_SHIFT  18
#define GE_VTYPE_MORPHCOUNT_MASK   0x7

#define GE_VTYPE_THROUGH_SHIFT 23
#define GE_VTYPE_THROUGH      (1 << 23)

/// Extract VTYPE fields.
inline int ge_vtype_tc(uint32_t vt) {
    return (vt >> GE_VTYPE_TC_SHIFT) & GE_VTYPE_TC_MASK;
}
inline int ge_vtype_col(uint32_t vt) {
    return (vt >> GE_VTYPE_COL_SHIFT) & GE_VTYPE_COL_MASK;
}
inline int ge_vtype_nrm(uint32_t vt) {
    return (vt >> GE_VTYPE_NRM_SHIFT) & GE_VTYPE_NRM_MASK;
}
inline int ge_vtype_pos(uint32_t vt) {
    return (vt >> GE_VTYPE_POS_SHIFT) & GE_VTYPE_POS_MASK;
}
inline int ge_vtype_weight(uint32_t vt) {
    return (vt >> GE_VTYPE_WEIGHT_SHIFT) & GE_VTYPE_WEIGHT_MASK;
}
inline int ge_vtype_idx(uint32_t vt) {
    return (vt >> GE_VTYPE_IDX_SHIFT) & GE_VTYPE_IDX_MASK;
}
inline bool ge_vtype_through(uint32_t vt) {
    return (vt & GE_VTYPE_THROUGH) != 0;
}

/// PSP texture formats.
enum GETextureFormat : uint8_t {
    GE_TFMT_5650   = 0,
    GE_TFMT_5551   = 1,
    GE_TFMT_4444   = 2,
    GE_TFMT_8888   = 3,
    GE_TFMT_CLUT4  = 4,
    GE_TFMT_CLUT8  = 5,
    GE_TFMT_CLUT16 = 6,
    GE_TFMT_CLUT32 = 7,
    GE_TFMT_DXT1   = 8,
    GE_TFMT_DXT3   = 9,
    GE_TFMT_DXT5   = 10,
};

/// PSP framebuffer/color buffer formats.
enum GEBufferFormat : uint8_t {
    GE_FORMAT_565  = 0,
    GE_FORMAT_5551 = 1,
    GE_FORMAT_4444 = 2,
    GE_FORMAT_8888 = 3,
};

/// PSP palette (CLUT) formats.
enum GEPaletteFormat : uint8_t {
    GE_CMODE_16BIT_BGR5650  = 0,
    GE_CMODE_16BIT_ABGR5551 = 1,
    GE_CMODE_16BIT_ABGR4444 = 2,
    GE_CMODE_32BIT_ABGR8888 = 3,
};

/// Comparison functions (alpha test, depth test, stencil).
enum GeCompFunc : uint8_t {
    GE_COMP_NEVER    = 0,
    GE_COMP_ALWAYS   = 1,
    GE_COMP_EQUAL    = 2,
    GE_COMP_NOTEQUAL = 3,
    GE_COMP_LESS     = 4,
    GE_COMP_LEQUAL   = 5,
    GE_COMP_GREATER  = 6,
    GE_COMP_GEQUAL   = 7,
};

/// Blend src/dst factors.
enum GeBlendSrcFactor : uint8_t {
    GE_SRCBLEND_DSTCOLOR          = 0,
    GE_SRCBLEND_INVDSTCOLOR       = 1,
    GE_SRCBLEND_SRCALPHA          = 2,
    GE_SRCBLEND_INVSRCALPHA       = 3,
    GE_SRCBLEND_DSTALPHA          = 4,
    GE_SRCBLEND_INVDSTALPHA       = 5,
    GE_SRCBLEND_DOUBLESRCALPHA    = 6,
    GE_SRCBLEND_DOUBLEINVSRCALPHA = 7,
    GE_SRCBLEND_DOUBLEDSTALPHA    = 8,
    GE_SRCBLEND_DOUBLEINVDSTALPHA = 9,
    GE_SRCBLEND_FIXA              = 10,
};

enum GeBlendDstFactor : uint8_t {
    GE_DSTBLEND_SRCCOLOR          = 0,
    GE_DSTBLEND_INVSRCCOLOR       = 1,
    GE_DSTBLEND_SRCALPHA          = 2,
    GE_DSTBLEND_INVSRCALPHA       = 3,
    GE_DSTBLEND_DSTALPHA          = 4,
    GE_DSTBLEND_INVDSTALPHA       = 5,
    GE_DSTBLEND_DOUBLESRCALPHA    = 6,
    GE_DSTBLEND_DOUBLEINVSRCALPHA = 7,
    GE_DSTBLEND_DOUBLEDSTALPHA    = 8,
    GE_DSTBLEND_DOUBLEINVDSTALPHA = 9,
    GE_DSTBLEND_FIXB              = 10,
};

/// Blend equations.
enum GeBlendOp : uint8_t {
    GE_BLENDMODE_MUL_AND_ADD              = 0,
    GE_BLENDMODE_MUL_AND_SUBTRACT         = 1,
    GE_BLENDMODE_MUL_AND_SUBTRACT_REVERSE = 2,
    GE_BLENDMODE_MIN                      = 3,
    GE_BLENDMODE_MAX                      = 4,
    GE_BLENDMODE_ABSDIFF                  = 5,
};

/// Texture functions.
enum GeTexFunc : uint8_t {
    GE_TEXFUNC_MODULATE = 0,
    GE_TEXFUNC_DECAL    = 1,
    GE_TEXFUNC_BLEND    = 2,
    GE_TEXFUNC_REPLACE  = 3,
    GE_TEXFUNC_ADD      = 4,
};
