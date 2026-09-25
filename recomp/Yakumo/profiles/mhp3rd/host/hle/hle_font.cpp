// sceLibFont: the PSP system fonts live in the console's internal flash, which
// a dumped disc does not contain, so glyphs come from a font on the host
// (fonts/game_font.hpp, which also describes how the game lays them out). The
// metrics reported are those of the bitmaps drawn.
#include "hle_common.hpp"

#include "fonts/game_font.hpp"

#include "psprecomp/common.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace mhp3rd {
namespace {

// FontPixelFormat values used by SceFontGlyphImage.
constexpr std::uint32_t kPixelFormat4 = 0u;
constexpr std::uint32_t kPixelFormat4Reversed = 1u;
constexpr std::uint32_t kPixelFormat8 = 2u;
constexpr std::uint32_t kPixelFormat24 = 3u;
constexpr std::uint32_t kPixelFormat32 = 4u;

constexpr std::uint32_t kLibraryHandle = 0x00F0F000u;
constexpr std::uint32_t kFontHandle = 0x00F0F100u;

// MHP3RD_TRACE_FONT=1: every sceLibFont call with its arguments and results.
bool trace_font() {
    static const bool enabled = std::getenv("MHP3RD_TRACE_FONT") != nullptr;
    return enabled;
}

#if defined(__GNUC__)
__attribute__((format(printf, 1, 2)))
#endif
void trace(const char *format, ...) {
    if (!trace_font()) return;
    char line[512];
    va_list args;
    va_start(args, format);
    std::vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    std::cerr << "[font] " << line << "\n";
}

float load_float(const psprecomp::GuestMemory &memory, std::uint32_t address) {
    const std::uint32_t bits = memory.load32(address);
    float value{};
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void store_float(psprecomp::GuestMemory &memory, std::uint32_t address, float value) {
    std::uint32_t bits{};
    std::memcpy(&bits, &value, sizeof(bits));
    memory.store32(address, bits);
}

std::uint32_t to_fixed26(float value) {
    return static_cast<std::uint32_t>(static_cast<std::int32_t>(value * 64.0f));
}

// SceFontCharInfo, 60 bytes: the bitmap's size and bearings in whole pixels,
// then the same and the advances in 26.6 fixed point.
void write_char_info(psprecomp::GuestMemory &memory, std::uint32_t address, const fonts::GlyphMetrics &glyph) {
    const auto put = [&](std::uint32_t offset, std::uint32_t value) { memory.store32(address + offset, value); };
    put(0u, static_cast<std::uint32_t>(glyph.width));
    put(4u, static_cast<std::uint32_t>(glyph.height));
    put(8u, static_cast<std::uint32_t>(glyph.left));
    put(12u, static_cast<std::uint32_t>(glyph.top));
    put(16u, to_fixed26(static_cast<float>(glyph.width)));
    put(20u, to_fixed26(static_cast<float>(glyph.height)));
    put(24u, to_fixed26(static_cast<float>(fonts::kAscender)));                  // ascender
    put(28u, to_fixed26(static_cast<float>(fonts::kAscender - fonts::kCell)));   // descender
    put(32u, to_fixed26(static_cast<float>(glyph.left)));                        // bearing HX
    put(36u, to_fixed26(static_cast<float>(glyph.top)));                         // bearing HY
    put(40u, 0u);                                                                // bearing VX
    put(44u, to_fixed26(static_cast<float>(glyph.top)));                         // bearing VY
    put(48u, to_fixed26(glyph.advance));                                         // advance H
    put(52u, to_fixed26(static_cast<float>(fonts::kCell)));                      // advance V
    memory.store16(address + 56u, 0u);                                          // shadow flags
    memory.store16(address + 58u, 0u);                                          // shadow id
}

// SceFontInfo: the maximum glyph metrics, as 26.6 fixed point and as floats,
// then the maximum bitmap size. The game sizes its glyph cells from these, so
// they describe the 20x20 cell every glyph is fitted into.
void write_font_info(psprecomp::GuestMemory &memory, std::uint32_t address) {
    const float cell = static_cast<float>(fonts::kCell);
    const float ascender = static_cast<float>(fonts::kAscender);
    const float values[10] = {
        cell,             // max glyph width
        cell,             // max glyph height
        ascender,         // max ascender
        ascender - cell,  // max descender
        0.0f,             // max left X
        ascender,         // max base Y
        cell / 2.0f,      // min centre X
        ascender,         // max top Y
        cell,             // max advance X
        cell,             // max advance Y
    };
    for (std::uint32_t i = 0; i < 10u; ++i) {
        memory.store32(address + i * 4u, to_fixed26(values[i]));
        store_float(memory, address + 40u + i * 4u, values[i]);
    }
    memory.store16(address + 80u, static_cast<std::uint16_t>(fonts::kCell));  // max glyph bitmap width
    memory.store16(address + 82u, static_cast<std::uint16_t>(fonts::kCell));  // max glyph bitmap height
    memory.store32(address + 84u, 0x10000u);                                  // glyph count
    memory.store32(address + 88u, 0u);                                        // shadow map length
}

// The game keeps every glyph it has drawn in a texture atlas and draws it
// again only once the atlas needs the cell for another glyph, which can take a
// whole play session. To change the font while the game runs, the host makes
// the game forget those glyphs: its text code (traced from the call that asks
// for glyph images) keeps, in the object that owns the atlas, a table from
// character code to atlas cell, where 0xFFFF means "not drawn yet". Clearing
// it makes the game draw each character again the next time it shows it.
constexpr std::uint32_t kGlyphImageCaller = 0x088EA3A4u;   // return address of the game's only call
constexpr std::uint32_t kCellWidthOffset = 276u;            // u8, from the font info's maximum width
constexpr std::uint32_t kCellHeightOffset = 277u;           // u8
constexpr std::uint32_t kCellCountOffset = 286u;            // u16, cells in the whole atlas
constexpr std::uint32_t kCodeToCellOffset = 22168u;         // u16 per character code below 0xFFF0
constexpr std::uint32_t kCodeToCellEntries = 0xFFF0u;
constexpr std::uint32_t kAtlasPages = 8u;

struct GameGlyphCache {
    psprecomp::GuestMemory *memory{};
    std::uint32_t object{};  // 0: not recognised
};

GameGlyphCache &game_cache() {
    static GameGlyphCache value;
    return value;
}

// The game's cell count for our cell size: 256x256 pages, cells a cell width
// apart and the cell height plus 2 apart.
constexpr std::uint32_t expected_cells() {
    return (256u / fonts::kCell) * (256u / (fonts::kCell + 2u)) * kAtlasPages;
}

// Remembers the object whose atlas the game is filling, once its layout
// matches what the game's text code builds from our font info.
void note_game_cache(psprecomp::GuestMemory &memory, const AllegrexContext &ctx) {
    GameGlyphCache &cache = game_cache();
    cache.memory = &memory;
    if (cache.object != 0u || ctx.gpr[31] != kGlyphImageCaller) return;
    const std::uint32_t object = ctx.gpr[17];  // s1 in that function
    if (!memory.contains(object, kCodeToCellOffset + kCodeToCellEntries * 2u)) return;
    if (memory.load8(object + kCellWidthOffset) != fonts::kCell ||
        memory.load8(object + kCellHeightOffset) != fonts::kCell ||
        memory.load16(object + kCellCountOffset) != expected_cells())
        return;
    cache.object = object;
    trace("game glyph cache at %08X", object);
}

void forget_game_glyphs() {
    const GameGlyphCache &cache = game_cache();
    if (cache.memory == nullptr || cache.object == 0u) {
        std::cout << "[font] the game has not drawn any text yet; nothing to redraw\n";
        return;
    }
    for (std::uint32_t code = 0; code < kCodeToCellEntries; ++code)
        cache.memory->store16(cache.object + kCodeToCellOffset + code * 2u, 0xFFFFu);
    std::cout << "[font] the game redraws its text with the new font\n";
}

std::int32_t floor_div64(std::int32_t value) { return value >= 0 ? value / 64 : -((-value + 63) / 64); }

// Draws one glyph into the guest buffer described by SceFontGlyphImage. The
// position is 26.6 fixed point; its fraction becomes a subpixel shift. Pixels
// only ever get darker ink: the game draws some glyphs twice, slightly apart,
// to embolden them, and a second pass must not erase the first.
void blit_glyph(psprecomp::GuestMemory &memory, std::uint32_t image_address, std::uint32_t code) {
    const std::uint32_t pixel_format = memory.load32(image_address);
    const auto x64 = static_cast<std::int32_t>(memory.load32(image_address + 4u));
    const auto y64 = static_cast<std::int32_t>(memory.load32(image_address + 8u));
    const std::uint32_t buffer_width = memory.load16(image_address + 12u);
    const std::uint32_t buffer_height = memory.load16(image_address + 14u);
    const std::uint32_t bytes_per_line = memory.load16(image_address + 16u);
    const std::uint32_t buffer = memory.load32(image_address + 20u);
    if (buffer == 0u) return;

    const std::int32_t x_whole = floor_div64(x64);
    const std::int32_t y_whole = floor_div64(y64);
    const fonts::GlyphBitmap glyph = fonts::render(code, static_cast<float>(x64 - x_whole * 64) / 64.0f,
                                                   static_cast<float>(y64 - y_whole * 64) / 64.0f);
    if (glyph.pixels.empty()) return;
    const std::int32_t x_origin = x_whole + glyph.x;
    const std::int32_t y_origin = y_whole + glyph.y;

    for (int row = 0; row < glyph.height; ++row) {
        const std::int32_t y = y_origin + row;
        if (y < 0 || static_cast<std::uint32_t>(y) >= buffer_height) continue;
        const std::uint32_t line = buffer + static_cast<std::uint32_t>(y) * bytes_per_line;
        for (int column = 0; column < glyph.width; ++column) {
            const std::int32_t x = x_origin + column;
            if (x < 0 || static_cast<std::uint32_t>(x) >= buffer_width) continue;
            const std::uint8_t value = glyph.pixels[static_cast<std::size_t>(row) * glyph.width + column];
            if (value == 0u) continue;
            switch (pixel_format) {
            case kPixelFormat4:
            case kPixelFormat4Reversed: {
                const std::uint32_t at = line + static_cast<std::uint32_t>(x) / 2u;
                const std::uint8_t existing = memory.load8(at);
                const bool high = pixel_format == kPixelFormat4 ? ((x & 1) != 0) : ((x & 1) == 0);
                const std::uint8_t old = high ? static_cast<std::uint8_t>(existing >> 4u) : (existing & 0x0Fu);
                const std::uint8_t nibble = std::max(old, static_cast<std::uint8_t>(value >> 4u));
                memory.store8(at, high ? static_cast<std::uint8_t>((existing & 0x0Fu) | (nibble << 4u))
                                       : static_cast<std::uint8_t>((existing & 0xF0u) | nibble));
                break;
            }
            case kPixelFormat8: {
                const std::uint32_t at = line + static_cast<std::uint32_t>(x);
                memory.store8(at, std::max(memory.load8(at), value));
                break;
            }
            case kPixelFormat24: {
                const std::uint32_t at = line + static_cast<std::uint32_t>(x) * 3u;
                const std::uint8_t merged = std::max(memory.load8(at), value);
                memory.store8(at, merged);
                memory.store8(at + 1u, merged);
                memory.store8(at + 2u, merged);
                break;
            }
            case kPixelFormat32:
            default: {
                const std::uint32_t at = line + static_cast<std::uint32_t>(x) * 4u;
                const std::uint8_t merged = std::max(static_cast<std::uint8_t>(memory.load32(at) >> 24u), value);
                memory.store32(at, (static_cast<std::uint32_t>(merged) << 24u) | 0x00FFFFFFu);
                break;
            }
            }
        }
    }
}

} // namespace

void register_font(HleRegistrar &hle) {
    fonts::ready();
    fonts::set_reload_hook(forget_game_glyphs);

    hle.add("sceLibFont", "sceFontNewLib", [](Runtime &rt, AllegrexContext &ctx) {
        if (trace_font() && arg(ctx, 0) != 0u) {
            const auto &m = rt.memory();
            const std::uint32_t p = arg(ctx, 0);
            trace("NewLib params=%08X userData=%08X numFonts=%u cache=%08X alloc=%08X free=%08X ra=%08X", p,
                  m.load32(p), m.load32(p + 4u), m.load32(p + 8u), m.load32(p + 12u), m.load32(p + 16u),
                  ctx.gpr[31]);
        }
        if (arg(ctx, 1) != 0u) rt.memory().store32(arg(ctx, 1), 0u);
        kernel().finish(ctx, kLibraryHandle);
    });
    hle.add("sceLibFont", "sceFontDoneLib", [](Runtime &, AllegrexContext &ctx) {
        trace("DoneLib lib=%08X", arg(ctx, 0));
        kernel().finish(ctx, 0u);
    });
    hle.add("sceLibFont", "sceFontGetNumFontList", [](Runtime &rt, AllegrexContext &ctx) {
        trace("GetNumFontList lib=%08X", arg(ctx, 0));
        if (arg(ctx, 1) != 0u) rt.memory().store32(arg(ctx, 1), 0u);
        kernel().finish(ctx, 1u);
    });
    hle.add("sceLibFont", "sceFontFindOptimumFont", [](Runtime &rt, AllegrexContext &ctx) {
        // The game asks for a Japanese font and leaves the size unset (zero):
        // every font it opens is the host's.
        const std::uint32_t style = arg(ctx, 1);
        if (style != 0u && trace_font()) {
            const auto &m = rt.memory();
            trace("FindOptimumFont style=%08X h=%g v=%g hres=%g vres=%g weight=%g family=%u style=%u sub=%u "
                  "lang=%u region=%u country=%u name='%s' file='%s' attr=%08X ra=%08X",
                  style, static_cast<double>(load_float(m, style)), static_cast<double>(load_float(m, style + 4u)),
                  static_cast<double>(load_float(m, style + 8u)), static_cast<double>(load_float(m, style + 12u)),
                  static_cast<double>(load_float(m, style + 16u)), m.load16(style + 20u), m.load16(style + 22u),
                  m.load16(style + 24u), m.load16(style + 26u), m.load16(style + 28u), m.load16(style + 30u),
                  read_cstring(m, style + 32u, 64u).c_str(), read_cstring(m, style + 96u, 64u).c_str(),
                  m.load32(style + 160u), ctx.gpr[31]);
        }
        if (arg(ctx, 2) != 0u) rt.memory().store32(arg(ctx, 2), 0u);
        kernel().finish(ctx, 0u);
    });
    hle.add("sceLibFont", "sceFontOpen", [](Runtime &rt, AllegrexContext &ctx) {
        trace("Open lib=%08X index=%u mode=%u ra=%08X", arg(ctx, 0), arg(ctx, 1), arg(ctx, 2), ctx.gpr[31]);
        if (arg(ctx, 3) != 0u) rt.memory().store32(arg(ctx, 3), 0u);
        kernel().finish(ctx, fonts::ready() ? kFontHandle : 0u);
    });
    hle.add("sceLibFont", "sceFontClose", [](Runtime &, AllegrexContext &ctx) {
        trace("Close font=%08X", arg(ctx, 0));
        kernel().finish(ctx, 0u);
    });

    hle.add("sceLibFont", "sceFontGetFontInfo", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t address = arg(ctx, 1);
        if (address != 0u && fonts::ready()) write_font_info(rt.memory(), address);
        trace("GetFontInfo font=%08X info=%08X max=%dx%d ascender=%d ra=%08X", arg(ctx, 0), address, fonts::kCell,
              fonts::kCell, fonts::kAscender, ctx.gpr[31]);
        kernel().finish(ctx, 0u);
    });

    hle.add("sceLibFont", "sceFontGetCharInfo", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t address = arg(ctx, 2);
        if (address == 0u) {
            kernel().finish(ctx, 0u);
            return;
        }
        const fonts::GlyphMetrics glyph = fonts::ready() ? fonts::metrics(arg(ctx, 1)) : fonts::GlyphMetrics{};
        write_char_info(rt.memory(), address, glyph);
        trace("GetCharInfo code=%04X w=%d h=%d left=%d top=%d adv=%.2f%s ra=%08X", arg(ctx, 1), glyph.width,
              glyph.height, glyph.left, glyph.top, static_cast<double>(glyph.advance), glyph.found ? "" : " missing",
              ctx.gpr[31]);
        kernel().finish(ctx, 0u);
    });

    hle.add("sceLibFont", "sceFontGetCharGlyphImage", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t image = arg(ctx, 2);
        if (trace_font() && image != 0u) {
            const auto &m = rt.memory();
            trace("GetCharGlyphImage code=%04X fmt=%u x64=%d y64=%d buf=%ux%u bpl=%u at=%08X ra=%08X", arg(ctx, 1),
                  m.load32(image), static_cast<std::int32_t>(m.load32(image + 4u)),
                  static_cast<std::int32_t>(m.load32(image + 8u)), m.load16(image + 12u), m.load16(image + 14u),
                  m.load16(image + 16u), m.load32(image + 20u), ctx.gpr[31]);
        }
        note_game_cache(rt.memory(), ctx);
        if (image != 0u && fonts::ready()) blit_glyph(rt.memory(), image, arg(ctx, 1));
        kernel().finish(ctx, 0u);
    });
}

} // namespace mhp3rd
