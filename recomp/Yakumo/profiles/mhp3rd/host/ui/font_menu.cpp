#include "ui/font_menu.hpp"

#include "ui/layer.hpp"
#include "ui/widgets.hpp"

#include "fonts/game_font.hpp"
#include "install/user_data.hpp"
#include "settings/settings.hpp"

#include "imgui.h"
#include "imgui_internal.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace mhp3rd::ui {
namespace {

// The preview line: English as the patched game shows it, and some of the
// Japanese it still has.
constexpr char32_t kPreviewText[] = U"Urgent Quest  Yukumo Chief  1500z  装備 アイテム";
// Text height in PSP pixels of most of the game's text.
constexpr float kPreviewTextSize = 14.0f;
constexpr int kPreviewWidth = 1024;
constexpr int kPreviewHeight = 64;

bool list_open = false;
bool focus_current = false;   // focus the chosen font's row when the list opens
bool focus_font_row = false;  // focus the Font row when the list closes

float px(float value) { return std::round(value * Layer::get().scale()); }

RowOptions options_for(const char *key, std::string description) {
    RowOptions options;
    options.description = std::move(description);
    if (const char *variable = settings::overridden_by(key)) {
        options.disabled = true;
        options.note = std::string("Set by ") + variable;
    }
    return options;
}

std::string file_url(const std::string &path) {
    std::string url = "file://";
    for (const unsigned char c : path) {
        if (std::isalnum(c) != 0 || c == '/' || c == '-' || c == '_' || c == '.' || c == '~') {
            url += static_cast<char>(c);
        } else {
            char escaped[4];
            std::snprintf(escaped, sizeof(escaped), "%%%02X", c);
            url += escaped;
        }
    }
    return url;
}

void apply(const std::string &value) {
    settings::Settings &s = settings::current();
    s.font = value;
    settings::save();
    fonts::reload();
}

// Half-width in the game's sense: Latin, digits and symbols, and half-width
// kana.
bool half_width(char32_t c) { return c < 0x250u || (c >= 0xFF61u && c <= 0xFF9Fu); }

// A glyph cell as the game fills it: centred, or at its left bearing, and a
// half-width character drawn twice, the second time 31/64 of a pixel left.
void fill_cell(char32_t code, std::vector<float> &cell) {
    std::fill(cell.begin(), cell.end(), 0.0f);
    const fonts::GlyphMetrics m = fonts::metrics(code);
    if (!m.found || m.width <= 0) return;
    const auto stamp = [&](const fonts::GlyphBitmap &b, int x, int y) {
        for (int row = 0; row < b.height; ++row)
            for (int column = 0; column < b.width; ++column) {
                const int cx = x + b.x + column;
                const int cy = y + b.y + row;
                if (cx < 0 || cy < 0 || cx >= fonts::kCell || cy >= fonts::kCell) continue;
                float &out = cell[static_cast<std::size_t>(cy) * fonts::kCell + cx];
                out = std::max(out, b.pixels[static_cast<std::size_t>(row) * b.width + column] / 255.0f);
            }
    };
    const int y = fonts::kBaseline - m.top;
    if (half_width(code)) {
        const int x = (fonts::kCell - m.width) / 2;
        stamp(fonts::render(code, 0.0f, 0.0f), x, y);
        stamp(fonts::render(code, 33.0f / 64.0f, 0.0f), x - 1, y);
    } else {
        stamp(fonts::render(code, 0.0f, 0.0f), m.left, y);
    }
}

// The preview, drawn the way the game draws text: each character's cell
// squeezed into a sprite as tall as the text and, for a half-width character,
// half as wide.
struct Preview {
    ImTextureData *texture{};
    std::uint64_t generation{~0ull};
    int size{};
    int width{};
};

Preview &preview() {
    static Preview value;
    return value;
}

void build_preview(Preview &p, int size) {
    if (p.texture == nullptr) {
        p.texture = new ImTextureData();
        p.texture->Create(ImTextureFormat_RGBA32, kPreviewWidth, kPreviewHeight);
        ImGui::RegisterUserTexture(p.texture);
    }
    auto *pixels = static_cast<std::uint32_t *>(p.texture->GetPixels());
    std::fill(pixels, pixels + kPreviewWidth * kPreviewHeight, 0u);
    // The cell's texel rows: 20 of glyph and the 2 empty rows between cells.
    constexpr int kRows = fonts::kCell + 2;
    std::vector<float> cell(static_cast<std::size_t>(fonts::kCell) * fonts::kCell);
    const auto texel = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= fonts::kCell || y >= fonts::kCell) return 0.0f;
        return cell[static_cast<std::size_t>(y) * fonts::kCell + x];
    };
    int pen = 0;
    for (const char32_t *c = kPreviewText; *c != 0; ++c) {
        const int width = half_width(*c) ? size / 2 : size;
        if (pen + width > kPreviewWidth) break;
        if (*c != U' ') {
            fill_cell(*c, cell);
            for (int oy = 0; oy < size; ++oy) {
                const float v = (static_cast<float>(oy) + 0.5f) * kRows / static_cast<float>(size) - 0.5f;
                const int y0 = static_cast<int>(std::floor(v));
                const float fy = v - static_cast<float>(y0);
                for (int ox = 0; ox < width; ++ox) {
                    const float u = (static_cast<float>(ox) + 0.5f) * fonts::kCell / static_cast<float>(width) - 0.5f;
                    const int x0 = static_cast<int>(std::floor(u));
                    const float fx = u - static_cast<float>(x0);
                    const float a = (texel(x0, y0) * (1.0f - fx) + texel(x0 + 1, y0) * fx) * (1.0f - fy) +
                                    (texel(x0, y0 + 1) * (1.0f - fx) + texel(x0 + 1, y0 + 1) * fx) * fy;
                    const auto alpha = static_cast<std::uint32_t>(std::clamp(a, 0.0f, 1.0f) * 255.0f + 0.5f);
                    pixels[static_cast<std::size_t>(oy) * kPreviewWidth + pen + ox] = (alpha << 24u) | 0x00FFFFFFu;
                }
            }
        }
        pen += width;
    }
    p.width = pen;
    p.size = size;
    p.generation = fonts::generation();
    if (p.texture->Status == ImTextureStatus_OK || p.texture->Status == ImTextureStatus_WantUpdates)
        ImTextureDataQueueUpload(p.texture, 0, 0, kPreviewWidth, kPreviewHeight);
}

void draw_preview() {
    Preview &p = preview();
    // As large as the game's text is in the window.
    const float window_scale = ImGui::GetIO().DisplaySize.y / 272.0f;
    const int size = std::clamp(static_cast<int>(std::lround(kPreviewTextSize * window_scale)), 8, kPreviewHeight);
    if (p.generation != fonts::generation() || p.size != size) build_preview(p, size);

    const float height = static_cast<float>(size) + px(20.0f);
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    ImGui::Dummy({width, height});
    ImDrawList *draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled({min.x + px(16.0f), min.y + px(4.0f)}, {min.x + width - px(16.0f), min.y + height - px(4.0f)},
                        IM_COL32(0, 0, 0, 90), px(6.0f));
    const float shown = std::min(static_cast<float>(p.width), width - px(48.0f));
    const ImVec2 at{min.x + px(24.0f), min.y + px(10.0f)};
    draw->AddImage(p.texture->GetTexRef(), at, {at.x + shown, at.y + static_cast<float>(size)}, {0.0f, 0.0f},
                   {shown / kPreviewWidth, static_cast<float>(size) / kPreviewHeight}, colors::kText);
}

std::string current_font_label() {
    const settings::Settings &s = settings::current();
    if (s.font.empty()) return "Default";
    if (!fonts::problem().empty()) return "Default (chosen font unreadable)";
    return fonts::active_name();
}

} // namespace

bool font_list_open() { return list_open; }

void font_rows() {
    settings::Settings &s = settings::current();
    fonts::start_catalog();
    section("Text");
    {
        RowOptions o = options_for("text.font", "The font the game's text is drawn with. Characters it lacks, such "
                                                "as Japanese in a Latin font, come from the default: " +
                                                    fonts::fallback_name() + ".");
        if (!fonts::problem().empty()) o.description += "\n" + fonts::problem();
        if (focus_font_row) {
            focus_next_row();
            focus_font_row = false;
        }
        if (value_row("Font", current_font_label(), o)) {
            list_open = true;
            focus_current = true;
        }
    }
    {
        static const char *const kWeights[] = {"Regular", "Bold", "Heavy"};
        const int current = static_cast<int>(std::min(s.font_weight, settings::kMaxFontWeight));
        if (const int delta = choice_row(
                "Weight", kWeights[current],
                options_for("text.weight", "Thickens the strokes of the game's text. The game squeezes Latin "
                                           "letters to half width, which thins them; Bold makes up for it."))) {
            const int count = static_cast<int>(settings::kMaxFontWeight) + 1;
            s.font_weight = static_cast<std::uint32_t>(((current + delta) % count + count) % count);
            settings::save();
            fonts::reload();
        }
    }
    draw_preview();
    if (button_row("Open the fonts folder",
                   {false, {}, "Fonts put in this folder (.ttf, .otf, .ttc) are listed first under Font: " +
                                   fonts::user_font_folder()})) {
        const std::string folder = fonts::user_font_folder();
        std::error_code ec;
        std::filesystem::create_directories(install::path_from_utf8(folder), ec);
        if (!SDL_OpenURL(file_url(folder).c_str()))
            std::cout << "[menu] cannot open " << folder << ": " << SDL_GetError() << "\n";
    }
}

bool font_list(bool back) {
    if (!list_open) return false;
    if (back) {
        list_open = false;
        focus_font_row = true;
        return true;
    }
    const settings::Settings &s = settings::current();
    section("Font");
    ImGui::Indent(px(16.0f));
    paragraph("The game's text changes as soon as you choose. Characters a font lacks come from the default; "
              "fonts in the fonts folder are listed first.",
              colors::kTextDim);
    ImGui::Unindent(px(16.0f));
    ImGui::Dummy({0.0f, px(4.0f)});

    bool done = false;
    const std::vector<fonts::FontChoice> choices = fonts::catalog(done);
    std::string chosen;
    bool picked = false;
    const auto entry = [&](const char *id, const std::string &name, const std::string &detail,
                           const std::string &value) {
        const bool current = value == s.font;
        if (current && focus_current) {
            focus_next_row();
            focus_current = false;
        }
        if (list_row(id, name, detail, ListIcon::None, current)) {
            chosen = value;
            picked = true;
        }
    };
    entry("##default", "Default: " + fonts::fallback_name(), "Japanese", "");
    for (std::size_t i = 0; i < choices.size(); ++i) {
        const fonts::FontChoice &choice = choices[i];
        ImGui::PushID(static_cast<int>(i));
        const std::string detail = choice.user_folder ? (choice.japanese ? "Your font, Japanese" : "Your font")
                                   : choice.japanese  ? "Japanese"
                                                      : "Latin";
        entry("##font", choice.name, detail, choice.value);
        ImGui::PopID();
    }
    // The chosen font is not in the list, for example a path typed into
    // settings.ini: the focus stays on the first row.
    focus_current = false;
    if (!done) {
        ImGui::Indent(px(16.0f));
        paragraph("Looking for installed fonts…", colors::kTextDim);
        ImGui::Unindent(px(16.0f));
    }
    if (picked) {
        apply(chosen);
        list_open = false;
        focus_font_row = true;
    }
    return true;
}

} // namespace mhp3rd::ui
