// P3P3DS New Nintendo 3DS runner (first bring-up build).
//
// Top screen: the PSP framebuffer (480x272) scaled to 400x240 (nearest).
// Bottom screen: authorship and live debug information; the same data is
// written to sdmc:/p3p3ds/report.txt every few seconds and on exit.
//
// Game data: sdmc:/p3p3ds/*.iso (the user's ULUS-10512 image) mounted as
// disc0:, sdmc:/p3p3ds/ms0 as ms0:, sdmc:/p3p3ds/mods as ms0:/PSP/P3P.
// The decrypted executable is read from romfs:/eboot.elf (embedded at build
// time from the local build; it must match the generated AOT code).
//
// GE rendering runs on the PICA200 (gpu_renderer.cpp); no audio output yet.
// Frame dumps for comparison with the PC runner: put a number N in
// sdmc:/p3p3ds/dump_every.txt to save the top screen every N vblanks to
// sdmc:/p3p3ds/frames/.
// Movies: there is no H.264 decoder yet (pictures are black), so while the
// game decodes movie frames START is pressed for it, which skips the opening
// movie (checked on the PC runner: title screen 0.6 s after START instead of
// at 133 s). Only while the screen shows a CPU-written picture: the title
// screen also decodes video, and a START there would pick a menu entry.
// An empty sdmc:/p3p3ds/play_movies.txt turns this off.
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/input.hpp"
#include "p3p3ds/interpreter.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/profile.hpp"
#include "p3p3ds/vfs.hpp"
#include "bootstrap_expectations.hpp"
#include "gpu_renderer.hpp"
#include "psprecomp/elf32.hpp"
#include "psprecomp/runtime.hpp"

#include <3ds.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <malloc.h>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <vector>

namespace psprecomp {
void register_generated_functions(Runtime &);
void apply_generated_patches(GuestMemory &);
} // namespace psprecomp

// libctru: main thread stack. Translated guest code chains through host calls.
// Linear (GPU-visible) heap: screen framebuffers, the citro3d command buffer,
// vertices and decoded textures (gpu_renderer.cpp budgets 6 MiB of them).
// libctru's default split caps the normal heap at 24 MiB
// (libctru/source/system/allocateHeaps.c, HEAP_SPLIT_SIZE_CAP), which cannot
// hold the 32 MiB guest RAM; a fixed linear size gives the rest to the heap.
extern "C" {
u32 __stacksize__ = 1024u * 1024u;
u32 __ctru_linear_heap_size = 12u * 1024u * 1024u;
}

// Heap diagnostics for the report: size of the last allocation that failed.
// The caller address resolves with arm-none-eabi-addr2line -f -C -e p3p3ds.elf.
static std::size_t g_failed_alloc_bytes = 0;
static void *g_failed_alloc_caller = nullptr;
[[gnu::noinline]] static void *checked_alloc(std::size_t size, void *caller) {
    if (void *p = std::malloc(size ? size : 1u)) return p;
    g_failed_alloc_bytes = size;
    g_failed_alloc_caller = caller;
    throw std::bad_alloc();
}
void *operator new(std::size_t size) { return checked_alloc(size, __builtin_return_address(0)); }
void *operator new[](std::size_t size) { return checked_alloc(size, __builtin_return_address(0)); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }

#ifndef P3P3DS_BUILD_ID
#define P3P3DS_BUILD_ID "unknown"
#endif

namespace {
constexpr const char *kAuthor = "enderlit aka endercodezz";
constexpr const char *kBase = "sdmc:/p3p3ds";

struct Stats {
    u64 start_ms{}, last_report_ms{}, last_fps_ms{};
    std::uint64_t frames{}, frames_at_last_fps{};
    // Game frames: changes of the displayed PSP framebuffer (the game swaps
    // buffers once per finished frame); vblanks run at 60/s at full speed.
    std::uint64_t game_frames{}, game_frames_at_last_fps{};
    std::uint32_t last_topaddr{};
    double fps{}, vblank_rate{};
    std::uint32_t buttons{};
    std::string status{"booting"};
    std::string iso;
    // Wall-time split (ARM11 system ticks): presentation is timed here, HLE
    // (includes GE rendering) and rendering by the shared profilers.
    u64 run_start_ticks{}, present_ticks{};
    std::uint64_t dump_every{};
    bool skip_movies{true};
    std::uint64_t movie_vblanks{}, movie_frames_seen{};
};

double ticks_to_s(u64 ticks) { return static_cast<double>(ticks) / SYSCLOCK_ARM11; }

// 3DS buttons -> PSP buttons by position (PSP: Cross bottom, Circle right,
// Square left, Triangle top).
class HidSource final : public p3p3ds::input::InputSource {
public:
    p3p3ds::input::PadState sample(std::uint64_t) override { return pad; }
    p3p3ds::input::PadState pad;
    void poll() {
        hidScanInput();
        const u32 held = hidKeysHeld();
        using namespace p3p3ds::input::button;
        static constexpr std::pair<u32, std::uint32_t> kMap[] = {
            {KEY_DUP, Up}, {KEY_DDOWN, Down}, {KEY_DLEFT, Left}, {KEY_DRIGHT, Right},
            {KEY_START, Start}, {KEY_SELECT, Select}, {KEY_L, LTrigger}, {KEY_R, RTrigger},
            {KEY_B, Cross}, {KEY_A, Circle}, {KEY_Y, Square}, {KEY_X, Triangle},
        };
        p3p3ds::input::PadState p;
        for (const auto &[from, to] : kMap)
            if (held & from) p.buttons |= to;
        circlePosition c;
        hidCircleRead(&c); // about -156..156
        auto axis = [](int v) { return static_cast<std::uint8_t>(std::clamp(128 + v * 127 / 156, 0, 255)); };
        p.analog_x = axis(c.dx);
        p.analog_y = axis(-c.dy);
        pad = p;
    }
};

p3p3ds::n3ds::GpuRenderer *g_gpu = nullptr;

// Save/load list dialogs of sceUtilitySavedata (the PSP draws them as system
// UI): shown on the bottom screen. D-Pad chooses, B confirms (Cross), A goes
// back (Circle); saving over an existing slot asks again. The game receives
// no buttons while the menu is open, nor until every button is released.
struct SaveMenu {
    bool active{}, confirm{}, dirty{}, suppress{};
    int cursor{};
};

// PARAM.SFO texts are UTF-8 (P3P writes the hero's name in full-width
// letters); the bottom-screen console is ASCII. Full-width forms map to ASCII,
// the ideographic space to a space, anything else to '?'.
std::string console_text(const std::string &utf8) {
    std::string out;
    for (std::size_t i = 0; i < utf8.size();) {
        const auto c = static_cast<unsigned char>(utf8[i]);
        std::uint32_t cp = c;
        std::size_t len = 1;
        if (c >= 0xF0) { cp = c & 0x07u; len = 4; }
        else if (c >= 0xE0) { cp = c & 0x0Fu; len = 3; }
        else if (c >= 0xC0) { cp = c & 0x1Fu; len = 2; }
        for (std::size_t k = 1; k < len && i + k < utf8.size(); ++k) cp = (cp << 6) | (static_cast<unsigned char>(utf8[i + k]) & 0x3Fu);
        i += len;
        if (cp == '\n' || cp == '\r') out += ' ';
        else if (cp >= 0x20 && cp < 0x7F) out += static_cast<char>(cp);
        else if (cp >= 0xFF01 && cp <= 0xFF5E) out += static_cast<char>(cp - 0xFEE0);
        else if (cp == 0x3000) out += ' ';
        else if (cp == 0x2640) out += 'F'; // female / male signs in P3P's save details
        else if (cp == 0x2642) out += 'M';
        else if (cp >= 0x80) out += '?';
    }
    return out;
}

void draw_save_menu(const p3p3ds::hle::SavedataDialog &d, const SaveMenu &menu) {
    using Kind = p3p3ds::hle::SavedataDialog::Kind;
    std::string out = "\x1b[1;1H\x1b[2J";
    out += d.kind == Kind::Load ? "  LOAD GAME\n" : d.kind == Kind::Save ? "  SAVE GAME\n" : "  DELETE SAVE\n";
    out += "  ----------------------------------\n";
    const int n = static_cast<int>(d.slots.size()), rows = 18;
    const int top = std::clamp(menu.cursor - rows / 2, 0, std::max(0, n - rows));
    for (int i = top; i < std::min(n, top + rows); ++i) {
        const auto &s = d.slots[static_cast<std::size_t>(i)];
        // Slots by number (P3P names them DATA00..DATA0f, DATA10..); the save's
        // detail line (date, place, level) tells the saves apart.
        char number[8];
        std::snprintf(number, sizeof number, "%02d", i + 1);
        std::string line = (i == menu.cursor ? "> " : "  ") + std::string(number) + "  " +
                           (s.exists ? console_text(s.detail.empty() ? s.savedata_title : s.detail) : std::string("- empty -"));
        if (line.size() > 39) line.resize(39);
        out += line + "\n";
    }
    for (int i = std::min(n, top + rows) - top; i < rows; ++i) out += "\n";
    out += "  ----------------------------------\n";
    const auto &sel = d.slots[static_cast<std::size_t>(menu.cursor)];
    const std::string detail = sel.exists ? console_text(sel.detail) : std::string();
    out += "  " + detail.substr(0, 37) + "\n  " + (detail.size() > 37 ? detail.substr(37, 37) : std::string()) + "\n\n";
    if (menu.confirm) out += "  Overwrite this save?  B: yes  A: no\n";
    else out += "  D-Pad: choose  B: confirm  A: back\n";
    std::fputs(out.c_str(), stdout);
}

// Handles one vblank of the menu. Returns true while it owns the input.
bool update_save_menu(p3p3ds::hle::SavedataUtility &sd, SaveMenu &menu) {
    const auto *d = sd.pending_dialog();
    if (d == nullptr) {
        if (menu.active) menu.active = false;
        return false;
    }
    const int n = static_cast<int>(d->slots.size());
    if (!menu.active) {
        menu = SaveMenu{};
        menu.active = menu.dirty = menu.suppress = true;
        int best = -1; // start on the newest existing save
        for (int i = 0; i < n; ++i)
            if (d->slots[static_cast<std::size_t>(i)].exists &&
                (best < 0 || d->slots[static_cast<std::size_t>(i)].modified > d->slots[static_cast<std::size_t>(best)].modified)) best = i;
        menu.cursor = best < 0 ? 0 : best;
    }
    const u32 repeat = hidKeysDownRepeat(), pressed = hidKeysDown();
    if (!menu.confirm && (repeat & KEY_DUP)) { menu.cursor = (menu.cursor + n - 1) % n; menu.dirty = true; }
    if (!menu.confirm && (repeat & KEY_DDOWN)) { menu.cursor = (menu.cursor + 1) % n; menu.dirty = true; }
    if (pressed & KEY_B) {
        const bool overwrite = d->kind != p3p3ds::hle::SavedataDialog::Kind::Load && d->slots[static_cast<std::size_t>(menu.cursor)].exists;
        if (overwrite && !menu.confirm) { menu.confirm = true; menu.dirty = true; }
        else { sd.resolve(menu.cursor); menu.active = false; return true; }
    }
    if (pressed & KEY_A) {
        if (menu.confirm) { menu.confirm = false; menu.dirty = true; }
        else { sd.resolve(-1); menu.active = false; return true; }
    }
    if (menu.dirty) { draw_save_menu(*d, menu); menu.dirty = false; }
    return true;
}
std::uint64_t g_skip_presses = 0;

void write_bmp(const char *path, const std::vector<std::uint8_t> &rgb, std::uint32_t w, std::uint32_t h) {
    FILE *f = std::fopen(path, "wb");
    if (!f) return;
    const std::uint32_t row = (w * 3u + 3u) & ~3u, size = 54u + row * h;
    std::uint8_t header[54] = {'B', 'M'};
    auto put32 = [&](int at, std::uint32_t v) { for (int i = 0; i < 4; ++i) header[at + i] = static_cast<std::uint8_t>(v >> (8 * i)); };
    put32(2, size); put32(10, 54); put32(14, 40); put32(18, w); put32(22, h);
    header[26] = 1; header[28] = 24; put32(34, row * h);
    std::fwrite(header, 1, sizeof header, f);
    std::vector<std::uint8_t> line(row, 0);
    for (std::uint32_t y = h; y-- > 0;) { // BMP rows bottom-up, BGR
        for (std::uint32_t x = 0; x < w; ++x) {
            const auto *p = &rgb[(static_cast<std::size_t>(y) * w + x) * 3u];
            line[x * 3u] = p[2]; line[x * 3u + 1u] = p[1]; line[x * 3u + 2u] = p[0];
        }
        std::fwrite(line.data(), 1, row, f);
    }
    std::fclose(f);
}

std::string gpu_text() {
    if (g_gpu == nullptr) return {};
    const auto &g = g_gpu->gpu_stats();
    char buf[256];
    std::snprintf(buf, sizeof buf,
        "gpu    : draws %llu tris %llu skip %llu\n"
        "         tex %lu up %llu hit %llu rtt %llu\n"
        "         present gpu %llu cpu %llu\n"
        "movie  : skip presses %llu\n",
        static_cast<unsigned long long>(g.draws), static_cast<unsigned long long>(g.triangles),
        static_cast<unsigned long long>(g.skipped_prims), static_cast<unsigned long>(g.textures),
        static_cast<unsigned long long>(g.texture_uploads), static_cast<unsigned long long>(g.texture_hits),
        static_cast<unsigned long long>(g.target_textures), static_cast<unsigned long long>(g.gpu_presents),
        static_cast<unsigned long long>(g.cpu_presents), static_cast<unsigned long long>(g_skip_presses));
    return buf;
}

std::string profile_text(const Stats &s) {
    if (s.run_start_ticks == 0) return {};
    const double run = ticks_to_s(svcGetSystemTick() - s.run_start_ticks);
    if (run <= 0) return {};
    const auto &h = psprecomp::runtime_profile();
    const auto &p = p3p3ds::host_profile();
    const double hle = static_cast<double>(h.hle_ns) / 1e9, render = static_cast<double>(p.render_ns) / 1e9;
    const double present = ticks_to_s(s.present_ticks), interp = static_cast<double>(p.interpreter_ns) / 1e9;
    char buf[256];
    std::snprintf(buf, sizeof buf,
        "time   : hle %.0f%% (render %.0f%%)\n"
        "         present %.0f%% interp %.1f%%\n"
        "         aot+dispatch %.0f%%  calls %llu\n",
        100 * hle / run, 100 * render / run, 100 * present / run, 100 * interp / run,
        100 * (run - hle - present - interp) / run, static_cast<unsigned long long>(h.hle_calls));
    return buf + gpu_text();
}

std::string report_text(const Stats &s, const psprecomp::Runtime &rt, p3p3ds::KernelState &k) {
    const u64 now = osGetTime();
    const double wall = static_cast<double>(now - s.start_ms) / 1000.0;
    const double guest = static_cast<double>(k.threads().now()) / 1e6;
    char buf[1536];
    std::snprintf(buf, sizeof buf,
        "P3P3DS - Persona 3 Portable (ULUS-10512)\n"
        "static recompilation port\n"
        "by %s\n"
        "build %s\n"
        "\n"
        "status : %s\n"
        "fps    : %.1f game frames/s (%.0f vblank/s)\n"
        "speed  : %.0f%% of real time\n"
        "guest  : %.1f s   wall: %.1f s\n"
        "frames : %llu   vblank: %llu\n"
        "input  : %08lx\n"
        "app mem free: %lu KiB / %lu KiB\n"
        "linear free : %lu KiB\n"
        "iso    : %s\n"
        "stop   : %s\n"
        "%s"
        "\nSTART+SELECT: quit\n",
        kAuthor, P3P3DS_BUILD_ID, s.status.c_str(), s.fps, s.vblank_rate, wall > 0 ? 100.0 * guest / wall : 0.0, guest, wall,
        static_cast<unsigned long long>(s.frames), static_cast<unsigned long long>(k.threads().vblank_count()),
        static_cast<unsigned long>(s.buttons),
        static_cast<unsigned long>(osGetMemRegionFree(MEMREGION_APPLICATION) / 1024u),
        static_cast<unsigned long>(osGetMemRegionSize(MEMREGION_APPLICATION) / 1024u),
        static_cast<unsigned long>(linearSpaceFree() / 1024u), s.iso.c_str(),
        rt.stopped() ? rt.stop_reason().c_str() : "-", profile_text(s).c_str());
    return buf;
}

void write_report(const std::string &text) {
    if (FILE *f = std::fopen("sdmc:/p3p3ds/report.txt", "w")) {
        std::fputs(text.c_str(), f);
        std::fclose(f);
    }
}

void show(const std::string &text) {
    std::printf("\x1b[1;1H\x1b[2J%s", text.c_str());
}

std::string find_iso() {
    std::error_code ec;
    for (const auto &e : std::filesystem::directory_iterator(kBase, ec)) {
        auto ext = e.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (e.is_regular_file(ec) && ext == ".iso") return e.path().string();
    }
    return {};
}

void wait_exit(const std::string &message) {
    std::printf("\n%s\n\nPress START to exit.\n", message.c_str());
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
        gspWaitForVBlank();
    }
}
} // namespace

int main() {
    osSetSpeedupEnable(true); // New 3DS: 804 MHz + L2 cache
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, nullptr);
    romfsInit();
    std::cout.rdbuf(nullptr); // runtime diagnostics on std::cout are discarded on 3DS

    Stats stats;
    stats.start_ms = stats.last_report_ms = stats.last_fps_ms = osGetTime();
    std::printf("P3P3DS by %s\nbuild %s\n\nLoading...\n", kAuthor, P3P3DS_BUILD_ID);

    stats.iso = find_iso();
    if (stats.iso.empty()) {
        wait_exit("No .iso found in sdmc:/p3p3ds/.\nCopy your Persona 3 Portable\n(ULUS-10512) image there.");
        romfsExit(); gfxExit(); return 0;
    }

    const auto mem_line = [] {
        char buf[256];
        const struct mallinfo mi = mallinfo();
        std::snprintf(buf, sizeof buf,
                      "app mem: %lu KiB / %lu KiB\nheap: %lu KiB  in use: %lu KiB\nlinear free: %lu KiB\n"
                      "failed alloc: %lu bytes from %p\n",
                      static_cast<unsigned long>((osGetMemRegionSize(MEMREGION_APPLICATION) - osGetMemRegionFree(MEMREGION_APPLICATION)) / 1024u),
                      static_cast<unsigned long>(osGetMemRegionSize(MEMREGION_APPLICATION) / 1024u),
                      static_cast<unsigned long>(envGetHeapSize() / 1024u),
                      static_cast<unsigned long>(mi.uordblks / 1024u),
                      static_cast<unsigned long>(linearSpaceFree() / 1024u),
                      static_cast<unsigned long>(g_failed_alloc_bytes), g_failed_alloc_caller);
        return std::string(buf);
    };
    std::printf("%s", mem_line().c_str());
    write_report("booting\n" + mem_line());

    // Guest RAM (32 MiB) and kernel state are the largest heap allocations.
    std::unique_ptr<psprecomp::Runtime> rt_owner;
    std::unique_ptr<p3p3ds::KernelState> kernel_owner;
    try {
        rt_owner = std::make_unique<psprecomp::Runtime>();
        kernel_owner = std::make_unique<p3p3ds::KernelState>();
    } catch (const std::exception &e) {
        const auto text = std::string("Out of memory creating the runtime\n(") + e.what() + ")\n" + mem_line();
        write_report(text);
        wait_exit(text);
        romfsExit(); gfxExit(); return 0;
    }
    auto &rt = *rt_owner;
    auto &kernel = *kernel_owner;
    auto hid = std::make_shared<HidSource>();
    std::string fatal;
    std::string stage = "load eboot";
    try {
        const auto elf = psprecomp::Elf32Image::from_file("romfs:/eboot.elf");
        const auto entry = elf.runtime_entry();
        (void)elf.load_and_relocate(rt.memory());
        psprecomp::apply_generated_patches(rt.memory());
        const auto module = elf.find_module_info(rt.memory());
        if (!module) throw std::runtime_error("missing module info");
        const auto sp = p3p3ds::profile::stack_top - 0x100;
        rt.memory().zero(p3p3ds::profile::stack_top - p3p3ds::profile::stack_size, p3p3ds::profile::stack_size);
        rt.cpu().gpr[28] = module->gp; rt.cpu().gpr[29] = sp; rt.cpu().gpr[26] = sp;
        rt.cpu().gpr[31] = p3p3ds::hle::ThreadManager::kThreadReturnSentinel;
        stage = "register AOT";
        std::printf("Registering AOT code...\n");
        psprecomp::register_generated_functions(rt);

        stage = "mount iso";
        kernel.umd().set_medium_present(true);
        kernel.io().mount("disc0:", std::make_shared<p3p3ds::vfs::IsoFileSystem>(stats.iso));
        std::error_code ec;
        std::filesystem::create_directories(std::string(kBase) + "/ms0", ec);
        std::filesystem::create_directories(std::string(kBase) + "/mods", ec);
        kernel.io().mount("ms0:", std::make_shared<p3p3ds::vfs::HostFileSystem>(std::string(kBase) + "/ms0"), true);
        kernel.savedata().root = std::string(kBase) + "/ms0/PSP/SAVEDATA";
        kernel.io().alias("ms0:/PSP/P3P", std::make_shared<p3p3ds::vfs::HostFileSystem>(std::string(kBase) + "/mods"));
        kernel.threads().init_root_thread("root", entry, sp, module->gp);
        std::set<std::uint32_t> stubs;
        std::vector<std::tuple<std::string, std::uint32_t, std::uint32_t>> imports;
        for (const auto &import : elf.scan_imports(rt.memory(), *module)) {
            stubs.insert(import.stub_address);
            imports.emplace_back(import.library, import.nid, import.stub_address);
        }
        kernel.modules().set_host_imports(std::move(imports));
        kernel.threads().set_import_stubs(std::move(stubs));
        stage = "register HLE";
        p3p3ds::hle::register_all_hle_modules(rt, kernel);
        static p3p3ds::Interpreter interpreter;
        p3p3ds::install_interpreter_fallback(&interpreter);
        kernel.input().source = hid;
        stage = "init gpu";
        auto gpu = std::make_unique<p3p3ds::n3ds::GpuRenderer>();
        g_gpu = gpu.get();
        kernel.ge().set_renderer(std::move(gpu));
        rt.memory().vram_write_observer = [](std::uint32_t address, std::size_t bytes) { g_gpu->note_cpu_write(address, bytes); };
        if (FILE *f = std::fopen("sdmc:/p3p3ds/play_movies.txt", "r")) { stats.skip_movies = false; std::fclose(f); }
        if (FILE *f = std::fopen("sdmc:/p3p3ds/dump_every.txt", "r")) {
            unsigned long long every = 0;
            if (std::fscanf(f, "%llu", &every) == 1) stats.dump_every = every;
            std::fclose(f);
            if (stats.dump_every) std::filesystem::create_directories(std::string(kBase) + "/frames", ec);
        }

        stats.status = "running";
        psprecomp::runtime_profile().enabled = true;
        p3p3ds::host_profile().enabled = true;
        stats.run_start_ticks = svcGetSystemTick();
        // on_vblank fires once per guest vblank wait, several times per virtual
        // vblank (5,808 calls for 1,854 vblanks in Azahar): present once per vblank.
        std::uint64_t last_presented_vblank = ~0ull;
        SaveMenu save_menu;
        // Real-time pacing: the guest clock may not run ahead of the wall clock
        // (in Azahar the title screen reached 95 vblanks/s). When the guest
        // falls behind, the reference moves instead of catching up in a burst.
        const double ticks_per_us = static_cast<double>(SYSCLOCK_ARM11) / 1e6;
        double wall_minus_guest_us = 0.0;
        bool pacing_started = false;
        hidSetRepeatParameters(20, 6);
        kernel.display().on_vblank = [&](const p3p3ds::hle::DisplayFramebufInfo &fb) {
            const auto vblank = kernel.threads().vblank_count();
            if (vblank == last_presented_vblank) return;
            last_presented_vblank = vblank;
            if (!aptMainLoop()) { rt.stop("Closed by the system (HOME)"); throw psprecomp::FrontierHalt{}; }
            hid->poll();
            if (stats.skip_movies) {
                std::uint64_t decoded = 0;
                for (const auto &[handle, inst] : kernel.mpeg().instances) decoded += inst.video_decoded;
                if (decoded != stats.movie_frames_seen && g_gpu->last_present_from_cpu()) {
                    stats.movie_frames_seen = decoded;
                    // Hold START for 6 vblanks out of every 40 while movie frames are decoded.
                    if (stats.movie_vblanks++ % 40u < 6u) {
                        hid->pad.buttons |= p3p3ds::input::button::Start;
                        ++g_skip_presses;
                    }
                } else {
                    stats.movie_vblanks = 0;
                }
            }
            if (update_save_menu(kernel.savedata(), save_menu)) {
                hid->pad = {};
                save_menu.suppress = true;
            } else if (save_menu.suppress) {
                if (hidKeysHeld() == 0) save_menu.suppress = false;
                else hid->pad = {};
            }
            stats.buttons = hid->pad.buttons;
            if ((hidKeysHeld() & (KEY_START | KEY_SELECT)) == (KEY_START | KEY_SELECT)) {
                rt.stop("Quit by user (START+SELECT)");
                throw psprecomp::FrontierHalt{};
            }
            {
                const double guest_us = static_cast<double>(kernel.threads().now());
                const double wall_us = static_cast<double>(svcGetSystemTick() - stats.run_start_ticks) / ticks_per_us;
                if (!pacing_started || save_menu.active) { wall_minus_guest_us = wall_us - guest_us; pacing_started = true; }
                const double ahead_us = guest_us + wall_minus_guest_us - wall_us;
                if (ahead_us > 1000.0) svcSleepThread(static_cast<s64>(ahead_us * 1000.0));
                else if (ahead_us < -50000.0) wall_minus_guest_us = wall_us - guest_us; // behind: do not race later
            }
            const u64 present_start = svcGetSystemTick();
            g_gpu->present(rt.memory(), fb);
            stats.present_ticks += svcGetSystemTick() - present_start;
            if (stats.dump_every && vblank % stats.dump_every == 0u) {
                std::vector<std::uint8_t> rgb;
                if (g_gpu->read_top_screen(rgb)) {
                    char path[96];
                    std::snprintf(path, sizeof path, "%s/frames/vblank_%06llu.bmp", kBase, static_cast<unsigned long long>(vblank));
                    write_bmp(path, rgb, 400u, 240u);
                }
            }
            ++stats.frames;
            if (fb.active && fb.topaddr != stats.last_topaddr) { ++stats.game_frames; stats.last_topaddr = fb.topaddr; }
            const u64 now = osGetTime();
            if (now - stats.last_fps_ms >= 1000u) {
                const double seconds = static_cast<double>(now - stats.last_fps_ms) / 1000.0;
                stats.fps = static_cast<double>(stats.game_frames - stats.game_frames_at_last_fps) / seconds;
                stats.vblank_rate = static_cast<double>(stats.frames - stats.frames_at_last_fps) / seconds;
                stats.game_frames_at_last_fps = stats.game_frames;
                stats.frames_at_last_fps = stats.frames;
                stats.last_fps_ms = now;
                if (!save_menu.active) show(report_text(stats, rt, kernel));
            }
            if (now - stats.last_report_ms >= 5000u) {
                write_report(report_text(stats, rt, kernel));
                stats.last_report_ms = now;
            }
        };
        stage = "run";
        std::printf("Starting game...\n");
        try { rt.run(entry, ~0ull); }
        catch (const psprecomp::FrontierHalt &) {}
    } catch (const std::exception &e) {
        fatal = e.what();
        if (!rt.stopped()) rt.stop(std::string("Exception: ") + fatal);
    }
    if (!rt.stopped()) rt.stop("Runtime returned");
    stats.status = "stopped";
    const auto text = report_text(stats, rt, kernel) + "stage  : " + stage + "\n" + mem_line();
    write_report(text);
    show(text);
    wait_exit("Report saved to sdmc:/p3p3ds/report.txt");
    g_gpu = nullptr;
    kernel_owner.reset(); // the GPU renderer (citro3d) shuts down before gfx
    romfsExit();
    gfxExit();
    return 0;
}
