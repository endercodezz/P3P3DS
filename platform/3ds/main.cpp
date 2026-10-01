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
// Rendering uses the target-agnostic software renderer (no PICA200 backend
// yet) and there is no audio output in this build.
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/input.hpp"
#include "p3p3ds/interpreter.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/vfs.hpp"
#include "bootstrap_expectations.hpp"
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
// Linear (GPU-visible) heap: only the GSP framebuffers and the console live
// there for now. libctru's default split caps the normal heap at 24 MiB
// (libctru/source/system/allocateHeaps.c, HEAP_SPLIT_SIZE_CAP), which cannot
// hold the 32 MiB guest RAM; a fixed linear size gives the rest to the heap.
extern "C" {
u32 __stacksize__ = 1024u * 1024u;
u32 __ctru_linear_heap_size = 4u * 1024u * 1024u;
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
    double fps{};
    std::uint32_t buttons{};
    std::string status{"booting"};
    std::string iso;
};

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

std::uint32_t psp_pixel(psprecomp::GuestMemory &m, std::uint32_t address, int format) {
    // Returns 0xRRGGBBAA for GSP_RGBA8_OES.
    std::uint32_t r, g, b;
    if (format == 3) {
        const auto c = m.load32(address);
        r = c & 0xFF; g = (c >> 8) & 0xFF; b = (c >> 16) & 0xFF;
    } else {
        const auto c = m.load16(address);
        if (format == 0) { r = (c & 0x1F) << 3; g = ((c >> 5) & 0x3F) << 2; b = ((c >> 11) & 0x1F) << 3; }
        else if (format == 1) { r = (c & 0x1F) << 3; g = ((c >> 5) & 0x1F) << 3; b = ((c >> 10) & 0x1F) << 3; }
        else { r = (c & 0xF) * 17; g = ((c >> 4) & 0xF) * 17; b = ((c >> 8) & 0xF) * 17; }
    }
    return (r << 24) | (g << 16) | (b << 8) | 0xFFu;
}

void present(psprecomp::GuestMemory &m, const p3p3ds::hle::DisplayFramebufInfo &fb) {
    u16 w = 0, h = 0;
    auto *dst = reinterpret_cast<std::uint32_t *>(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &w, &h)); // w=240 (rows), h=400
    if (!dst) return;
    if (!fb.active || fb.topaddr == 0) {
        std::memset(dst, 0, static_cast<std::size_t>(w) * h * 4u);
    } else {
        const std::uint32_t bpp = fb.pixelformat == 3 ? 4u : 2u;
        const auto stride = static_cast<std::uint32_t>(fb.bufferwidth);
        if (!m.contains(fb.topaddr, stride * 272u * bpp)) return;
        // GSP framebuffers are rotated: column x holds 240 pixels, bottom row first.
        for (std::uint32_t x = 0; x < 400u; ++x) {
            const std::uint32_t sx = x * 480u / 400u;
            auto *column = dst + x * 240u;
            for (std::uint32_t y = 0; y < 240u; ++y) {
                const std::uint32_t sy = y * 272u / 240u;
                column[239u - y] = psp_pixel(m, fb.topaddr + (sy * stride + sx) * bpp, fb.pixelformat);
            }
        }
    }
    gfxFlushBuffers();
    gfxSwapBuffers();
}

std::string report_text(const Stats &s, const psprecomp::Runtime &rt, p3p3ds::KernelState &k) {
    const u64 now = osGetTime();
    const double wall = static_cast<double>(now - s.start_ms) / 1000.0;
    const double guest = static_cast<double>(k.threads().now()) / 1e6;
    char buf[1024];
    std::snprintf(buf, sizeof buf,
        "P3P3DS - Persona 3 Portable (ULUS-10512)\n"
        "static recompilation port\n"
        "by %s\n"
        "build %s\n"
        "\n"
        "status : %s\n"
        "fps    : %.1f (presented frames/s)\n"
        "speed  : %.0f%% of real time\n"
        "guest  : %.1f s   wall: %.1f s\n"
        "frames : %llu   vblank: %llu\n"
        "input  : %08lx\n"
        "app mem free: %lu KiB / %lu KiB\n"
        "linear free : %lu KiB\n"
        "iso    : %s\n"
        "stop   : %s\n"
        "\nSTART+SELECT: quit\n",
        kAuthor, P3P3DS_BUILD_ID, s.status.c_str(), s.fps, wall > 0 ? 100.0 * guest / wall : 0.0, guest, wall,
        static_cast<unsigned long long>(s.frames), static_cast<unsigned long long>(k.threads().vblank_count()),
        static_cast<unsigned long>(s.buttons),
        static_cast<unsigned long>(osGetMemRegionFree(MEMREGION_APPLICATION) / 1024u),
        static_cast<unsigned long>(osGetMemRegionSize(MEMREGION_APPLICATION) / 1024u),
        static_cast<unsigned long>(linearSpaceFree() / 1024u), s.iso.c_str(),
        rt.stopped() ? rt.stop_reason().c_str() : "-");
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
    gfxSetScreenFormat(GFX_TOP, GSP_RGBA8_OES);
    gfxSetDoubleBuffering(GFX_TOP, true);
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

        stats.status = "running";
        kernel.display().on_vblank = [&](const p3p3ds::hle::DisplayFramebufInfo &fb) {
            if (!aptMainLoop()) { rt.stop("Closed by the system (HOME)"); throw psprecomp::FrontierHalt{}; }
            hid->poll();
            stats.buttons = hid->pad.buttons;
            if ((hidKeysHeld() & (KEY_START | KEY_SELECT)) == (KEY_START | KEY_SELECT)) {
                rt.stop("Quit by user (START+SELECT)");
                throw psprecomp::FrontierHalt{};
            }
            present(rt.memory(), fb);
            ++stats.frames;
            const u64 now = osGetTime();
            if (now - stats.last_fps_ms >= 1000u) {
                stats.fps = static_cast<double>(stats.frames - stats.frames_at_last_fps) * 1000.0 / static_cast<double>(now - stats.last_fps_ms);
                stats.frames_at_last_fps = stats.frames;
                stats.last_fps_ms = now;
                show(report_text(stats, rt, kernel));
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
    romfsExit();
    gfxExit();
    return 0;
}
