#pragma once

#include <cstdint>
#include <string>
#include <vector>

// The font the game's text is drawn with. The PSP's system fonts live in the
// console's flash, which a dumped disc does not contain, so sceLibFont glyphs
// are rasterised from a TrueType or OpenType font on the host: the player's
// choice (settings text.font, or MHP3RD_FONT), with a Japanese system font as
// the fallback for every glyph the chosen font lacks.
//
// How the game uses the glyphs, traced with MHP3RD_TRACE_FONT=1:
//
// - It reads the font info once and sizes a glyph cell from the maximum glyph
//   width and height, each rounded up to an even number. A texture atlas of
//   256x256 pages holds the cells, one cell width apart horizontally and the
//   cell height plus 2 apart vertically.
// - Each glyph is rendered on its own into a cleared 20x20 buffer (4 bits per
//   pixel), and the whole buffer is copied into the glyph's atlas cell. With a
//   cell smaller than 20 pixels the copy spills into the neighbouring cells,
//   and at the right edge of a page it wraps into the cells of the next rows:
//   whichever glyph is copied later wins, so ink near a cell's edges is lost.
// - Half-width characters (Latin letters, digits, most symbols) are centred in
//   the cell by their bitmap width: x = (cell width - bitmap width) / 2,
//   rounded down; the left bearing is ignored. They are then drawn a second
//   time 31/64 of a pixel further left, which makes them bolder. Full-width
//   characters are placed at their bitmap left bearing.
// - The top of the bitmap goes to 1 + ascender - bitmap top, the ascender being
//   the font info's maximum glyph ascender.
// - Text is laid out in whole cells: a half-width character advances by half
//   the text size and a full-width one by the full size; the advances the font
//   reports are never read. Each character is drawn as a sprite showing its
//   whole cell (one of the game's text paths leaves out the last column and
//   row), squeezed into the character's advance.
//
// So the host reports a 20x20 maximum glyph, which gives cells that are as
// large as the buffer and never overlap, and fits every glyph inside the cell,
// away from its edges, by shifting it and, if it is still too large, scaling it
// down. The metrics reported are those of the bitmap drawn.
namespace mhp3rd::fonts {

// The game's glyph buffer and, with the metrics below, its atlas cell.
inline constexpr int kCell = 20;
// Where ink may go inside the cell: a column is kept free on each side, for the
// second, bolder pass on the left and for the sprites that leave out the
// cell's last column on the right.
inline constexpr int kInkLeft = 1;
inline constexpr int kInkRight = 19;   // exclusive
inline constexpr int kInkTop = 0;
inline constexpr int kInkBottom = 20;  // exclusive
// Row of the baseline inside the cell, as reported to the game. Each face's
// own baseline goes on the row that best splits the cell between its
// ascenders and descenders; glyph tops are reported relative to this one.
inline constexpr int kBaseline = 16;
// The maximum ascender reported in the font info: the game adds 1.
inline constexpr int kAscender = kBaseline - 1;

// One glyph as the game should lay it out: the size of the bitmap and where
// its top-left pixel sits relative to the pen position on the baseline.
struct GlyphMetrics {
    int width{};
    int height{};
    int left{};
    int top{};       // pixels above the baseline
    float advance{};
    bool found{};    // false: no font has the character; nothing is drawn
};

// An 8-bit coverage bitmap. `x` and `y` are where its top-left pixel goes
// relative to the position the game gave, which is that of the metrics'
// bitmap: a subpixel shift can move the bitmap by a pixel.
struct GlyphBitmap {
    std::vector<std::uint8_t> pixels;
    int width{};
    int height{};
    int x{};
    int y{};
};

// Loads the fonts on first use. False when no font could be loaded at all.
bool ready();
[[nodiscard]] GlyphMetrics metrics(std::uint32_t code);
// Renders `code` shifted right by shift_x and down by shift_y, both in [0, 1).
[[nodiscard]] GlyphBitmap render(std::uint32_t code, float shift_x, float shift_y);

// Rereads the font settings and clears every cached glyph, so the next glyph
// the game asks for comes from the new font, then runs the hook.
void reload();
// Called by reload(): makes the game forget the glyphs it has already drawn.
void set_reload_hook(void (*hook)());
// Counts reload() calls.
[[nodiscard]] std::uint64_t generation();

// A font the player can choose.
struct FontChoice {
    std::string value;     // what settings.ini stores: "" for the default, else path, and "#face" for a collection
    std::string name;      // the face's full name
    std::string path;      // UTF-8
    int face{};            // index in a collection
    bool japanese{};       // has the kana and kanji the game uses; otherwise those come from the default
    bool user_folder{};    // from the fonts folder in the data directory
};

// Starts looking for installed fonts on a worker thread.
void start_catalog();
// The fonts found so far; `done` tells whether the search has finished.
[[nodiscard]] std::vector<FontChoice> catalog(bool &done);
// The fonts folder in the per-user data directory, created on demand.
[[nodiscard]] std::string user_font_folder();

// The face the game's text is drawn with now, and the fallback's.
[[nodiscard]] std::string active_name();
[[nodiscard]] std::string fallback_name();
// Why the chosen font was not used, or "" when it was.
[[nodiscard]] std::string problem();

// Splits a stored value into path and face index.
void parse_value(const std::string &value, std::string &path, int &face);

} // namespace mhp3rd::fonts
