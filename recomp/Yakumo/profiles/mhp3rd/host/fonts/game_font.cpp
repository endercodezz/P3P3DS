#include "fonts/game_font.hpp"

#include "app_paths.hpp"

#include "install/user_data.hpp"
#include "settings/settings.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace mhp3rd::fonts {
namespace {

// Default fonts, in order: the first that loads is the fallback for every
// glyph the chosen font lacks, and the font used when none is chosen. A
// Japanese face is needed for the game's text.
const char *const kFontCandidates[] = {
    // macOS ships Hiragino Kaku Gothic W4 under its Japanese file name.
    "/System/Library/Fonts/ヒラギノ角ゴシック W4.ttc",
    "/System/Library/Fonts/Hiragino Sans GB.ttc",
    "/System/Library/Fonts/AquaKana.ttc",
    "/Library/Fonts/Arial Unicode.ttf",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/truetype/fonts-japanese-gothic.ttf",
    "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc",
    // Inside a Flatpak the host's fonts appear under /run/host/fonts.
    "/run/host/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/run/host/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
    "/run/host/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc",
    "C:/Windows/Fonts/msgothic.ttc",
    "C:/Windows/Fonts/meiryo.ttc",
};

// Where installed fonts are looked for. The fonts folder in the data
// directory is added to these.
const char *const kFontFolders[] = {
#if defined(__APPLE__)
    "/System/Library/Fonts",
    "/Library/Fonts",
    "~/Library/Fonts",
#elif defined(_WIN32)
    "%WINDIR%/Fonts",
    "%LOCALAPPDATA%/Microsoft/Windows/Fonts",
#else
    "/usr/share/fonts",
    "/usr/local/share/fonts",
    "~/.local/share/fonts",
    "~/.fonts",
    // A Flatpak sees the host's fonts here.
    "/run/host/fonts",
    "/run/host/user-fonts",
#endif
};

// Glyphs that set a face's size: the tallest and deepest letters of the text
// the game shows, Latin and Japanese. Brackets and other symbols that reach
// further are fitted one by one instead, so they do not shrink every letter.
constexpr char32_t kSizingSample[] = U"AHMWbdfghjklpqy0123456789あアがギ装備剣竜龍";
// A face offered in the menu has every printable ASCII character. It is marked
// Japanese when it also has these.
constexpr char32_t kJapaneseSample[] = U"あいうアイウー、。【】装備剣竜龍素材";

// The largest em size used: about the size the text had with a 16-pixel face
// in the game's former 16x18 cells, now in 20x22 ones.
constexpr float kMaxEm = 19.0f;
constexpr float kMinEm = 8.0f;

std::filesystem::path expand(const std::string &folder) {
    std::string text = folder;
    if (!text.empty() && text[0] == '~') {
        const char *home = std::getenv("HOME");
        if (home == nullptr) return {};
        text = std::string(home) + text.substr(1);
    }
    for (std::size_t start = text.find('%'); start != std::string::npos; start = text.find('%')) {
        const std::size_t end = text.find('%', start + 1);
        if (end == std::string::npos) return {};
        const char *value = std::getenv(text.substr(start + 1, end - start - 1).c_str());
        if (value == nullptr) return {};
        text.replace(start, end - start + 1, value);
    }
    return install::path_from_utf8(text);
}

// A font file mapped into memory: only the tables a lookup touches are read,
// so looking through every installed font stays quick.
class MappedFile {
public:
    MappedFile() = default;
    MappedFile(const MappedFile &) = delete;
    MappedFile &operator=(const MappedFile &) = delete;
    ~MappedFile() { close(); }

    bool open(const std::filesystem::path &path) {
        close();
#if defined(_WIN32)
        file_ = CreateFileW(path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_ == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(file_, &size) || size.QuadPart <= 0) return close(), false;
        mapping_ = CreateFileMappingW(file_, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (mapping_ == nullptr) return close(), false;
        data_ = static_cast<const std::uint8_t *>(MapViewOfFile(mapping_, FILE_MAP_READ, 0, 0, 0));
        size_ = static_cast<std::size_t>(size.QuadPart);
#else
        const int fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0) return false;
        struct stat info {};
        if (fstat(fd, &info) != 0 || info.st_size <= 0) {
            ::close(fd);
            return false;
        }
        void *view = mmap(nullptr, static_cast<std::size_t>(info.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
        ::close(fd);
        if (view == MAP_FAILED) return false;
        data_ = static_cast<const std::uint8_t *>(view);
        size_ = static_cast<std::size_t>(info.st_size);
#endif
        if (data_ == nullptr) return close(), false;
        return true;
    }

    [[nodiscard]] const std::uint8_t *data() const noexcept { return data_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

private:
    void close() {
#if defined(_WIN32)
        if (data_ != nullptr) UnmapViewOfFile(data_);
        if (mapping_ != nullptr) CloseHandle(mapping_);
        if (file_ != INVALID_HANDLE_VALUE) CloseHandle(file_);
        mapping_ = nullptr;
        file_ = INVALID_HANDLE_VALUE;
#else
        if (data_ != nullptr) munmap(const_cast<std::uint8_t *>(data_), size_);
#endif
        data_ = nullptr;
        size_ = 0;
    }

    const std::uint8_t *data_{};
    std::size_t size_{};
#if defined(_WIN32)
    HANDLE file_{INVALID_HANDLE_VALUE};
    HANDLE mapping_{};
#endif
};

std::string utf16be_to_utf8(const char *bytes, int length) {
    std::string out;
    for (int i = 0; i + 1 < length; i += 2) {
        char32_t c = (static_cast<unsigned char>(bytes[i]) << 8u) | static_cast<unsigned char>(bytes[i + 1]);
        if (c >= 0xD800u && c < 0xDC00u && i + 3 < length) {
            const char32_t low = (static_cast<unsigned char>(bytes[i + 2]) << 8u) |
                                 static_cast<unsigned char>(bytes[i + 3]);
            c = 0x10000u + ((c - 0xD800u) << 10u) + (low - 0xDC00u);
            i += 2;
        }
        if (c < 0x80u) {
            out += static_cast<char>(c);
        } else if (c < 0x800u) {
            out += static_cast<char>(0xC0u | (c >> 6u));
            out += static_cast<char>(0x80u | (c & 0x3Fu));
        } else if (c < 0x10000u) {
            out += static_cast<char>(0xE0u | (c >> 12u));
            out += static_cast<char>(0x80u | ((c >> 6u) & 0x3Fu));
            out += static_cast<char>(0x80u | (c & 0x3Fu));
        } else {
            out += static_cast<char>(0xF0u | (c >> 18u));
            out += static_cast<char>(0x80u | ((c >> 12u) & 0x3Fu));
            out += static_cast<char>(0x80u | ((c >> 6u) & 0x3Fu));
            out += static_cast<char>(0x80u | (c & 0x3Fu));
        }
    }
    return out;
}

// The face's full name, in English where the font has it.
std::string face_name(const stbtt_fontinfo &info) {
    int length = 0;
    for (const int language : {0x409, 0x411, 0x804, 0x404, 0x412}) {
        if (const char *name = stbtt_GetFontNameString(&info, &length, STBTT_PLATFORM_ID_MICROSOFT,
                                                       STBTT_MS_EID_UNICODE_BMP, language, 4))
            return utf16be_to_utf8(name, length);
    }
    if (const char *name = stbtt_GetFontNameString(&info, &length, STBTT_PLATFORM_ID_MAC, STBTT_MAC_EID_ROMAN,
                                                   STBTT_MAC_LANG_ENGLISH, 4))
        return std::string(name, static_cast<std::size_t>(length));
    return {};
}

bool has_all(const stbtt_fontinfo &info, const char32_t *sample) {
    for (const char32_t *c = sample; *c != 0; ++c)
        if (stbtt_FindGlyphIndex(&info, static_cast<int>(*c)) == 0) return false;
    return true;
}

bool has_ascii(const stbtt_fontinfo &info) {
    for (int c = 0x21; c < 0x7F; ++c)
        if (stbtt_FindGlyphIndex(&info, c) == 0) return false;
    return true;
}

// A loaded face and the size its glyphs are drawn at.
struct Face {
    std::shared_ptr<MappedFile> file;
    stbtt_fontinfo info{};
    std::string name;
    std::string path;
    int index{};
    float scale{};  // pixels per font unit
    float em{};     // pixels per em
};

// How far the face's letters reach above and below the baseline, in ems.
void measure(const Face &face, float &above, float &below) {
    const float units = stbtt_ScaleForMappingEmToPixels(&face.info, 1.0f);  // em per unit
    above = below = 0.0f;
    for (const char32_t *c = kSizingSample; *c != 0; ++c) {
        const int glyph = stbtt_FindGlyphIndex(&face.info, static_cast<int>(*c));
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        if (glyph == 0 || !stbtt_GetGlyphBox(&face.info, glyph, &x0, &y0, &x1, &y1)) continue;
        above = std::max(above, static_cast<float>(y1) * units);
        below = std::max(below, static_cast<float>(-y0) * units);
    }
}

// Picks the row of the cell the face's baseline goes on, from how far its
// letters reach above and below it.
int pick_baseline(const Face &face) {
    float above = 0.0f, below = 0.0f;
    measure(face, above, below);
    if (above + below <= 0.0f) return kBaseline;
    const float rows = static_cast<float>(kInkBottom - kInkTop);
    return std::clamp(kInkTop + static_cast<int>(std::lround(rows * above / (above + below))), kInkTop + 10,
                      kInkBottom - 3);
}

// Picks the em size: as large as kMaxEm allows while the sample's tallest
// glyph still fits above the baseline and its deepest below it.
void size_face(Face &face, int baseline) {
    float above = 0.0f, below = 0.0f;
    measure(face, above, below);
    float em = kMaxEm;
    // Rasterised boxes round outwards, so leave half a pixel on each side.
    if (above > 0.0f) em = std::min(em, (static_cast<float>(baseline - kInkTop) - 0.5f) / above);
    if (below > 0.0f) em = std::min(em, (static_cast<float>(kInkBottom - baseline) - 0.5f) / below);
    face.em = std::max(em, kMinEm);
    face.scale = stbtt_ScaleForMappingEmToPixels(&face.info, face.em);
}

std::unique_ptr<Face> load_face(const std::string &path, int index, std::string &error) {
    auto file = std::make_shared<MappedFile>();
    if (!file->open(install::path_from_utf8(path))) {
        error = "cannot open " + path;
        return nullptr;
    }
    const int count = std::max(stbtt_GetNumberOfFonts(file->data()), 1);
    if (index < 0 || index >= count) {
        error = path + " has no face " + std::to_string(index);
        return nullptr;
    }
    const int offset = stbtt_GetFontOffsetForIndex(file->data(), index);
    auto face = std::make_unique<Face>();
    if (offset < 0 || stbtt_InitFont(&face->info, file->data(), offset) == 0) {
        error = path + " is not a TrueType or OpenType font stb_truetype can read";
        return nullptr;
    }
    face->file = std::move(file);
    face->name = face_name(face->info);
    if (face->name.empty()) face->name = install::path_to_utf8(install::path_from_utf8(path).filename());
    face->path = path;
    face->index = index;
    return face;
}

// Where a glyph comes from and how it is placed in the cell.
struct Layout {
    const Face *face{};
    int glyph{};
    int bold{};        // columns of horizontal emboldening
    float scale_x{};
    float scale_y{};
    GlyphMetrics metrics;
    int natural_x0{};  // box of the unshifted bitmap, to place shifted ones
    int natural_y0{};
};

struct State {
    bool loaded{};
    std::unique_ptr<Face> chosen;    // null: the fallback is used for everything
    std::unique_ptr<Face> fallback;
    std::string problem;
    int bold{};
    int baseline{kBaseline};  // row of the cell the baselines are on
    std::unordered_map<std::uint32_t, Layout> layouts;
    std::unordered_map<std::uint64_t, GlyphBitmap> bitmaps;
    std::atomic<std::uint64_t> generation{};
};

State &state() {
    static State value;
    return value;
}

void load(State &s) {
    s.loaded = true;
    s.chosen.reset();
    s.problem.clear();
    s.bold = static_cast<int>(settings::current().font_weight);
    std::string error;
    if (!s.fallback) {
        for (const char *candidate : kFontCandidates) {
            s.fallback = load_face(candidate, 0, error);
            if (s.fallback) break;
        }
        // A release's own font in fonts/ next to the executable comes last.
        for (const std::filesystem::path &bundled : bundled_fonts()) {
            if (s.fallback) break;
            s.fallback = load_face(install::path_to_utf8(bundled), 0, error);
        }
    }
    const std::string &value = settings::current().font;
    if (!value.empty()) {
        std::string path;
        int face = 0;
        parse_value(value, path, face);
        s.chosen = load_face(path, face, error);
        if (!s.chosen) {
            s.problem = error;
            std::cout << "[font] " << error << "; using the default font\n";
        } else if (s.fallback && s.chosen->path == s.fallback->path && s.chosen->index == s.fallback->index) {
            s.chosen.reset();
        }
    }
    const Face *used = s.chosen ? s.chosen.get() : s.fallback.get();
    // The chosen face decides where the baseline is; the fallback is sized
    // to the same one so their letters line up.
    if (used != nullptr) {
        s.baseline = pick_baseline(*used);
        if (s.chosen) size_face(*s.chosen, s.baseline);
        if (s.fallback) size_face(*s.fallback, s.baseline);
    }
    if (used == nullptr) {
        std::cout << "[font] no TrueType font found; the game's text stays blank. Choose one in the menu or set "
                     "MHP3RD_FONT=<path to a .ttf, .otf or .ttc>\n";
        return;
    }
    std::cout << "Fonts: game text from " << used->name << " (" << used->path << "), " << used->em << " px em, baseline " << s.baseline;
    if (s.chosen && s.fallback) std::cout << "; missing glyphs from " << s.fallback->name;
    std::cout << "\n";
}

State &loaded_state() {
    State &s = state();
    if (!s.loaded) load(s);
    return s;
}

// Fits the glyph into the cell's ink area: scaled down if it is larger, then
// shifted inside it.
Layout lay_out(State &s, std::uint32_t code) {
    Layout layout;
    for (const Face *face : {s.chosen.get(), s.fallback.get()}) {
        if (face == nullptr) continue;
        const int glyph = stbtt_FindGlyphIndex(&face->info, static_cast<int>(code));
        if (glyph == 0) continue;
        layout.face = face;
        layout.glyph = glyph;
        break;
    }
    GlyphMetrics &m = layout.metrics;
    if (layout.face == nullptr) {
        // Say it once per character: the game asks again for every string.
        static std::set<std::uint32_t> reported;
        if (reported.insert(code).second)
            std::cout << "[font] no font has U+" << std::hex << code << std::dec << "; it is left blank\n";
        return layout;
    }
    m.found = true;
    const Face &face = *layout.face;
    int advance = 0, bearing = 0;
    stbtt_GetGlyphHMetrics(&face.info, layout.glyph, &advance, &bearing);

    // Emboldening widens the bitmap by that many columns.
    layout.bold = s.bold;
    const int kMaxWidth = kInkRight - kInkLeft - layout.bold;
    constexpr int kMaxHeight = kInkBottom - kInkTop;
    float scale = face.scale;
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    for (int attempt = 0; attempt < 8; ++attempt) {
        stbtt_GetGlyphBitmapBox(&face.info, layout.glyph, scale, scale, &x0, &y0, &x1, &y1);
        const int width = x1 - x0;
        const int height = y1 - y0;
        if (width <= kMaxWidth && height <= kMaxHeight) break;
        const float fit = std::min(static_cast<float>(kMaxWidth) / static_cast<float>(std::max(width, 1)),
                                   static_cast<float>(kMaxHeight) / static_cast<float>(std::max(height, 1)));
        scale *= std::min(fit, 0.97f);
    }
    layout.scale_x = scale;
    layout.scale_y = scale;
    layout.natural_x0 = x0;
    layout.natural_y0 = y0;
    m.width = std::max(x1 - x0, 0);
    m.height = std::max(y1 - y0, 0);
    if (m.width > 0) m.width += layout.bold;
    m.advance = static_cast<float>(advance) * scale;
    if (m.width == 0 || m.height == 0) {
        m.width = m.height = 0;
        m.left = 0;
        m.top = 0;
        return layout;
    }
    // Vertically the game puts the bitmap's top at kBaseline - top.
    // The top is reported relative to kBaseline, so the face's own baseline
    // lands on s.baseline.
    int top = -y0;
    top = std::min(top, s.baseline - kInkTop);
    top = std::max(top, s.baseline + m.height - kInkBottom);
    m.top = top + kBaseline - s.baseline;
    // Full-width characters are placed at the left bearing.
    m.left = std::clamp(x0, kInkLeft, kInkRight - m.width);
    m.advance = std::max(m.advance, static_cast<float>(m.left + m.width));
    return layout;
}

const Layout &layout_for(State &s, std::uint32_t code) {
    auto found = s.layouts.find(code);
    if (found == s.layouts.end()) found = s.layouts.emplace(code, lay_out(s, code)).first;
    return found->second;
}

// Looking for installed fonts runs once, on a worker thread.
struct Catalog {
    std::mutex mutex;
    std::vector<FontChoice> fonts;
    bool started{};
    bool done{};
};

Catalog &catalog_state() {
    static Catalog value;
    return value;
}

bool font_extension(const std::filesystem::path &path) {
    std::string extension = path.extension().string();
    for (char &c : extension) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return extension == ".ttf" || extension == ".otf" || extension == ".ttc" || extension == ".otc";
}

void add_file(const std::filesystem::path &path, bool user_folder, std::vector<FontChoice> &out,
              std::set<std::string> &names) {
    MappedFile file;
    if (!file.open(path)) return;
    const int count = std::max(stbtt_GetNumberOfFonts(file.data()), 1);
    const std::string utf8 = install::path_to_utf8(path);
    for (int index = 0; index < count && index < 64; ++index) {
        const int offset = stbtt_GetFontOffsetForIndex(file.data(), index);
        stbtt_fontinfo info{};
        if (offset < 0 || stbtt_InitFont(&info, file.data(), offset) == 0) continue;
        if (!has_ascii(info)) continue;
        std::string name = face_name(info);
        // Faces whose names start with a dot are the system's private ones.
        if (name.empty() || name[0] == '.' || name == "LastResort") continue;
        // Slanted faces are left out to keep the list short.
        if (!user_folder && (name.find("Italic") != std::string::npos || name.find("Oblique") != std::string::npos))
            continue;
        if (!names.insert(name).second) continue;
        FontChoice choice;
        choice.path = utf8;
        choice.face = index;
        choice.value = index == 0 ? utf8 : utf8 + "#" + std::to_string(index);
        choice.name = std::move(name);
        choice.japanese = has_all(info, kJapaneseSample);
        choice.user_folder = user_folder;
        out.push_back(std::move(choice));
    }
}

void scan_folder(const std::filesystem::path &folder, bool user_folder, std::vector<FontChoice> &out,
                 std::set<std::string> &names) {
    std::error_code ec;
    if (folder.empty() || !std::filesystem::is_directory(folder, ec)) return;
    std::size_t files = 0;
    auto it = std::filesystem::recursive_directory_iterator(
        folder, std::filesystem::directory_options::skip_permission_denied, ec);
    for (; !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (++files > 20000u) break;
        if (!it->is_regular_file(ec) || !font_extension(it->path())) continue;
        add_file(it->path(), user_folder, out, names);
    }
}

void scan() {
    std::vector<FontChoice> fonts;
    std::set<std::string> names;
    const std::string user = user_font_folder();
    if (!user.empty()) scan_folder(install::path_from_utf8(user), true, fonts, names);
    for (const char *folder : kFontFolders) scan_folder(expand(folder), false, fonts, names);
    // The player's own fonts first, then those with Japanese, then the rest.
    std::stable_sort(fonts.begin(), fonts.end(), [](const FontChoice &a, const FontChoice &b) {
        if (a.user_folder != b.user_folder) return a.user_folder;
        if (a.japanese != b.japanese) return a.japanese;
        return a.name < b.name;
    });
    Catalog &c = catalog_state();
    std::lock_guard lock(c.mutex);
    c.fonts = std::move(fonts);
    c.done = true;
}

// Thickens vertical strokes: each pixel takes the darkest of itself and the
// `columns` pixels to its left, which widens the bitmap by that much. The game
// squeezes a half-width character's cell to about a third of its width on
// screen, so its vertical strokes come out much thinner than its horizontal
// ones, too thin for the one-pixel shadow some text has.
void embolden(GlyphBitmap &bitmap, int columns) {
    const int width = bitmap.width + columns;
    std::vector<std::uint8_t> out(static_cast<std::size_t>(width) * static_cast<std::size_t>(bitmap.height), 0u);
    for (int y = 0; y < bitmap.height; ++y) {
        const std::uint8_t *in = bitmap.pixels.data() + static_cast<std::size_t>(y) * bitmap.width;
        std::uint8_t *row = out.data() + static_cast<std::size_t>(y) * width;
        for (int x = 0; x < width; ++x) {
            std::uint8_t value = 0;
            for (int j = 0; j <= columns; ++j) {
                const int from = x - j;
                if (from >= 0 && from < bitmap.width) value = std::max(value, in[from]);
            }
            row[x] = value;
        }
    }
    bitmap.pixels = std::move(out);
    bitmap.width = width;
}

} // namespace

bool ready() {
    const State &s = loaded_state();
    return s.chosen || s.fallback;
}

GlyphMetrics metrics(std::uint32_t code) { return layout_for(loaded_state(), code).metrics; }

GlyphBitmap render(std::uint32_t code, float shift_x, float shift_y) {
    State &s = loaded_state();
    const int sx = std::clamp(static_cast<int>(shift_x * 64.0f), 0, 63);
    const int sy = std::clamp(static_cast<int>(shift_y * 64.0f), 0, 63);
    const std::uint64_t key = (static_cast<std::uint64_t>(code) << 12u) | (static_cast<std::uint64_t>(sx) << 6u) |
                              static_cast<std::uint64_t>(sy);
    if (const auto found = s.bitmaps.find(key); found != s.bitmaps.end()) return found->second;

    const Layout &layout = layout_for(s, code);
    GlyphBitmap bitmap;
    if (layout.face != nullptr && layout.metrics.width > 0) {
        const float fx = static_cast<float>(sx) / 64.0f;
        const float fy = static_cast<float>(sy) / 64.0f;
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        stbtt_GetGlyphBitmapBoxSubpixel(&layout.face->info, layout.glyph, layout.scale_x, layout.scale_y, fx, fy, &x0,
                                        &y0, &x1, &y1);
        bitmap.width = std::max(x1 - x0, 0);
        bitmap.height = std::max(y1 - y0, 0);
        bitmap.x = x0 - layout.natural_x0;
        bitmap.y = y0 - layout.natural_y0;
        bitmap.pixels.assign(static_cast<std::size_t>(bitmap.width) * static_cast<std::size_t>(bitmap.height), 0u);
        if (!bitmap.pixels.empty())
            stbtt_MakeGlyphBitmapSubpixel(&layout.face->info, bitmap.pixels.data(), bitmap.width, bitmap.height,
                                          bitmap.width, layout.scale_x, layout.scale_y, fx, fy, layout.glyph);
        if (layout.bold > 0 && !bitmap.pixels.empty()) embolden(bitmap, layout.bold);
    }
    return s.bitmaps.emplace(key, std::move(bitmap)).first->second;
}

static void (*reload_hook)() = nullptr;

void reload() {
    State &s = state();
    s.layouts.clear();
    s.bitmaps.clear();
    load(s);
    ++s.generation;
    if (reload_hook != nullptr) reload_hook();
}

void set_reload_hook(void (*hook)()) { reload_hook = hook; }

std::uint64_t generation() { return state().generation.load(); }

void start_catalog() {
    Catalog &c = catalog_state();
    {
        std::lock_guard lock(c.mutex);
        if (c.started) return;
        c.started = true;
    }
    std::thread(scan).detach();
}

std::vector<FontChoice> catalog(bool &done) {
    Catalog &c = catalog_state();
    std::lock_guard lock(c.mutex);
    done = c.done;
    return c.fonts;
}

std::string user_font_folder() {
    try {
        return install::path_to_utf8(install::user_data_directory() / "fonts");
    } catch (const std::exception &) {
        return {};
    }
}

std::string active_name() {
    const State &s = loaded_state();
    if (s.chosen) return s.chosen->name;
    return s.fallback ? s.fallback->name : std::string();
}

std::string fallback_name() {
    const State &s = loaded_state();
    return s.fallback ? s.fallback->name : std::string();
}

std::string problem() { return loaded_state().problem; }

void parse_value(const std::string &value, std::string &path, int &face) {
    path = value;
    face = 0;
    const std::size_t hash = value.rfind('#');
    if (hash == std::string::npos || hash + 1 >= value.size()) return;
    for (std::size_t i = hash + 1; i < value.size(); ++i)
        if (std::isdigit(static_cast<unsigned char>(value[i])) == 0) return;
    path = value.substr(0, hash);
    face = std::atoi(value.c_str() + hash + 1);
}

} // namespace mhp3rd::fonts
