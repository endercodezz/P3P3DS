// Run-time overlay support. The game copies overlay code into fixed slots, so
// the corpus for an overlay can only be installed once its bytes are in guest
// memory. Each recompiled overlay is a shared library next to the executable,
// identified by the size and hash of the dump it was generated from; on a
// dispatch miss inside a slot the matching corpus replaces whatever was
// registered there before.
#include "overlays.hpp"

#include "app_paths.hpp"
#include "hle/hle_common.hpp"
#include "overlay_module.hpp"
#include "psprecomp/common.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace mhp3rd {

void revalidate_overlays(Runtime &runtime);

namespace {

struct OverlayCorpus {
    std::string name;
    std::uint32_t base{};
    std::uint32_t size{};
    std::uint32_t code_size{};
    std::uint64_t hash{};
    decltype(&mhp3rd_register_overlay) install{};
};

std::vector<OverlayCorpus> &overlay_corpora() {
    static std::vector<OverlayCorpus> corpora;
    return corpora;
}

std::uint64_t fnv1a64(const std::uint8_t *data, std::size_t size) {
    std::uint64_t hash = 0xCBF29CE484222325ull;
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= data[i];
        hash *= 0x100000001B3ull;
    }
    return hash;
}

std::uint64_t fnv1a64(const std::vector<std::uint8_t> &data) { return fnv1a64(data.data(), data.size()); }

std::vector<std::uint8_t> read_guest(const psprecomp::GuestMemory &memory, std::uint32_t address,
                                     std::uint32_t size) {
    std::vector<std::uint8_t> data(size);
    for (std::uint32_t i = 0; i < size; ++i) data[i] = memory.load8(address + i);
    return data;
}

// Fields of the 64-byte overlay header the host needs. The rest is the image
// id, its load address, the bss size and two end-of-image addresses.
struct OverlayHeader {
    std::uint32_t image_size{};
    std::string name;
};

std::optional<OverlayHeader> read_overlay_header(const psprecomp::GuestMemory &memory, std::uint32_t base) {
    const std::vector<std::uint8_t> head = read_guest(memory, base, kOverlayHeaderBytes);
    if (head[0] != 'M' || head[1] != 'W' || head[2] != 'o' || head[3] != '3') return std::nullopt;
    const auto word = [&head](std::size_t offset) {
        return static_cast<std::uint32_t>(head[offset]) | static_cast<std::uint32_t>(head[offset + 1u]) << 8u |
               static_cast<std::uint32_t>(head[offset + 2u]) << 16u |
               static_cast<std::uint32_t>(head[offset + 3u]) << 24u;
    };
    if (word(8u) != base) return std::nullopt;
    OverlayHeader header;
    header.image_size = kOverlayHeaderBytes + word(12u) + word(16u);
    const auto *text = reinterpret_cast<const char *>(head.data()) + 32;
    header.name.assign(text, std::find(text, text + 32, '\0'));
    return header;
}

// The game writes into an overlay's data section while it runs, so the loaded
// image only stays comparable to the one the corpus was built from over its
// header and the code that follows it.
std::uint64_t identity_hash(const psprecomp::GuestMemory &memory, const OverlayCorpus &corpus) {
    if (const std::uint8_t *image = memory.raw_pointer(corpus.base, kOverlayHeaderBytes + corpus.code_size);
        image != nullptr)
        return fnv1a64(image, kOverlayHeaderBytes + corpus.code_size);
    return fnv1a64(read_guest(memory, corpus.base, kOverlayHeaderBytes + corpus.code_size));
}

// Hashing a megabyte of guest code is far too expensive to repeat every frame.
// A different image in the slot always differs in its 64-byte header, which
// carries the image id, its load address, its section sizes and its name, so
// the header decides whether the full comparison is worth doing at all.
std::uint64_t header_hash(const psprecomp::GuestMemory &memory, std::uint32_t base) {
    if (const std::uint8_t *head = memory.raw_pointer(base, kOverlayHeaderBytes); head != nullptr)
        return fnv1a64(head, kOverlayHeaderBytes);
    return fnv1a64(read_guest(memory, base, kOverlayHeaderBytes));
}

// Slot containing `address`, or {0, 0}.
std::pair<std::uint32_t, std::uint32_t> slot_of(std::uint32_t address) {
    for (std::size_t i = 0; i + 1u < std::size(kOverlaySlots); ++i) {
        if (address >= kOverlaySlots[i] && address < kOverlaySlots[i + 1u])
            return {kOverlaySlots[i], kOverlaySlots[i + 1u]};
    }
    return {0u, 0u};
}

std::map<std::uint32_t, std::uint64_t> &installed_overlays() {
    static std::map<std::uint32_t, std::uint64_t> installed;
    return installed;
}

// The header hash each installed slot had when it was last verified.
std::map<std::uint32_t, std::uint64_t> &installed_headers() {
    static std::map<std::uint32_t, std::uint64_t> headers;
    return headers;
}

// Slots holding an image no corpus matches, such as an overlay a mod has
// patched, by the header hash of that image. The image runs interpreted, and
// every call into it used to search the slot's corpora again, hashing each
// candidate's code: a patched demo_task.ovl took the game menu from 100% to
// 10% speed. Forgotten when the game loads code again.
std::map<std::uint32_t, std::uint64_t> &unmatched_slots() {
    static std::map<std::uint32_t, std::uint64_t> unmatched;
    return unmatched;
}

std::filesystem::path overlay_directory() {
    if (const char *dir = std::getenv("MHP3RD_OVERLAY_DIR"); dir != nullptr && *dir != '\0') return dir;
#if defined(MHP3RD_ANDROID_APP)
    // An APK's libraries all sit in one flat directory, next to libmain.so.
    return executable_directory();
#else
    const std::filesystem::path directory = bundled_overlay_directory();
    if (directory.empty()) return "overlays";
    return directory;
#endif
}

bool is_overlay_library(const std::filesystem::path &path) {
    const std::filesystem::path extension = path.extension();
#if defined(MHP3RD_ANDROID_APP)
    // The package manager extracts only lib*.so, so the overlays are named
    // libovl*.so there, among SDL's, FFmpeg's and the host's own.
    return extension == ".so" && path.filename().string().starts_with("libovl");
#else
    return extension == ".so" || extension == ".dylib" || extension == ".dll";
#endif
}

// The library stays mapped for the rest of the process: its code can be reached
// through a dispatch table long after another corpus has taken over the slot,
// and unregistering only removes the entries the runtime knows about.
void *load_library(const std::filesystem::path &path) {
#if defined(_WIN32)
    return reinterpret_cast<void *>(LoadLibraryW(path.c_str()));
#else
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

void *library_symbol(void *handle, const char *name) {
#if defined(_WIN32)
    return reinterpret_cast<void *>(GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
#else
    return dlsym(handle, name);
#endif
}

std::string library_error() {
#if defined(_WIN32)
    return "error " + std::to_string(GetLastError());
#else
    const char *message = dlerror();
    return message != nullptr ? message : "unknown error";
#endif
}

void load_overlay_library(const std::filesystem::path &path) {
    void *handle = load_library(path);
    if (handle == nullptr) {
        std::cerr << "[overlay] cannot load " << path.filename().string() << ": " << library_error() << "\n";
        return;
    }
    const auto info_of = reinterpret_cast<decltype(&mhp3rd_overlay_info)>(
        library_symbol(handle, "mhp3rd_overlay_info"));
    const auto install = reinterpret_cast<decltype(&mhp3rd_register_overlay)>(
        library_symbol(handle, "mhp3rd_register_overlay"));
    if (info_of == nullptr || install == nullptr) {
        std::cerr << "[overlay] " << path.filename().string() << " is not an overlay library\n";
        return;
    }
    const OverlayModuleInfo *info = info_of();
    if (info == nullptr || info->abi_version != kOverlayAbiVersion) {
        std::cerr << "[overlay] " << path.filename().string() << " was built for a different host\n";
        return;
    }
    overlay_corpora().push_back(
        OverlayCorpus{info->name, info->base, info->size, info->code_size, info->hash, install});
}

void load_overlay_libraries() {
    const std::filesystem::path directory = overlay_directory();
    std::error_code ec;
    std::filesystem::directory_iterator entries(directory, ec);
    if (ec) {
        std::cerr << "[overlay] no overlay libraries in " << directory.string() << "\n";
        return;
    }
    std::vector<std::filesystem::path> paths;
    for (const std::filesystem::directory_entry &entry : entries) {
        if (entry.is_regular_file() && is_overlay_library(entry.path())) paths.push_back(entry.path());
    }
    std::sort(paths.begin(), paths.end());
    for (const std::filesystem::path &path : paths) load_overlay_library(path);
    std::cout << "Overlay corpora: " << overlay_corpora().size() << " from " << directory.string() << "\n";
}

// Writes the loaded image next to the other dumps so the corpus can be built.
void dump_slot(const psprecomp::GuestMemory &memory, std::uint32_t slot_start, std::uint32_t slot_end) {
    const char *directory = std::getenv("MHP3RD_DUMP_OVERLAYS");
    if (directory == nullptr) {
        log_once("overlay-dump-hint",
                 "[overlay] set MHP3RD_DUMP_OVERLAYS=<dir> to dump the loaded overlay for recompilation");
        return;
    }
    std::vector<std::uint8_t> image;
    if (const std::optional<OverlayHeader> header = read_overlay_header(memory, slot_start);
        header.has_value() && header->image_size != 0u) {
        image = read_guest(memory, slot_start, header->image_size);
    } else {
        // Not a recognisable image: fall back to the whole slot without its
        // trailing padding.
        image = read_guest(memory, slot_start, slot_end - slot_start);
        while (!image.empty() && image.back() == 0u) image.pop_back();
    }
    if (image.empty()) {
        std::cerr << "[overlay] slot " << psprecomp::hex32(slot_start) << " is empty\n";
        return;
    }
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    const std::filesystem::path path =
        std::filesystem::path(directory) / ("overlay_" + psprecomp::hex32(slot_start).substr(2) + ".bin");
    std::ofstream out(path, std::ios::binary);
    if (!out) throw psprecomp::Error("Cannot write overlay dump: " + path.string());
    out.write(reinterpret_cast<const char *>(image.data()), static_cast<std::streamsize>(image.size()));
    std::cout << "[overlay] dumped " << image.size() / 1024u << " KiB to " << path.string() << "\n"
              << "[overlay] recompile it with: profiles/mhp3rd/tools/add_overlay.py <build_dir> "
              << path.string() << " " << psprecomp::hex32(slot_start) << "\n";
}

bool install_overlay_for(Runtime &runtime, std::uint32_t pc) {
    const auto [slot_start, slot_end] = slot_of(pc);
    if (slot_start == 0u) return false;
    if (const auto unmatched = unmatched_slots().find(slot_start);
        unmatched != unmatched_slots().end() && unmatched->second == header_hash(runtime.memory(), slot_start))
        return false;

    for (const OverlayCorpus &corpus : overlay_corpora()) {
        // The slot list records where images start, not how much room each one
        // has: a big overlay such as lobby_task runs past the next slot's base,
        // which is fine because the two are never loaded at the same time.
        if (corpus.base != slot_start) continue;
        if (identity_hash(runtime.memory(), corpus) != corpus.hash) continue;
        auto &installed = installed_overlays();
        if (installed[slot_start] == corpus.hash) return false;  // already installed: a real miss
        runtime.unregister_functions(slot_start, slot_start + corpus.size);
        corpus.install(runtime);
        installed[slot_start] = corpus.hash;
        installed_headers()[slot_start] = header_hash(runtime.memory(), slot_start);
        std::cout << "[overlay] installed " << corpus.name << " (" << corpus.size / 1024u << " KiB) at "
                  << psprecomp::hex32(slot_start) << "\n";
        return true;
    }

    const std::optional<OverlayHeader> header = read_overlay_header(runtime.memory(), slot_start);
    log_once("overlay-miss:" + psprecomp::hex32(slot_start),
             "[overlay] no recompiled corpus for " + (header.has_value() ? header->name : std::string("the overlay")) +
                 " loaded at " + psprecomp::hex32(slot_start));
    dump_slot(runtime.memory(), slot_start, slot_end);
    unmatched_slots()[slot_start] = header_hash(runtime.memory(), slot_start);
    return false;
}

bool missing_function_hook(Runtime &runtime, AllegrexContext &, std::uint32_t pc) {
    return install_overlay_for(runtime, pc);
}

// Generated overlay code that no longer matches memory decodes as nonsense. If
// the slot really did change, drop the stale corpus and install the right one,
// then let the dispatch retry.
bool unsupported_instruction_hook(Runtime &runtime, AllegrexContext &, std::uint32_t pc, std::uint32_t) {
    const auto [slot_start, slot_end] = slot_of(pc);
    if (slot_start == 0u) return false;
    (void)slot_end;
    revalidate_overlays(runtime);
    if (installed_overlays().contains(slot_start)) return false;  // corpus still matches: a real problem
    return install_overlay_for(runtime, pc);
}

} // namespace

void revalidate_overlays(Runtime &runtime) {
    for (auto it = installed_overlays().begin(); it != installed_overlays().end();) {
        const std::uint32_t slot_start = it->first;
        const std::uint64_t hash = it->second;
        const OverlayCorpus *corpus = nullptr;
        for (const OverlayCorpus &candidate : overlay_corpora()) {
            if (candidate.base == slot_start && candidate.hash == hash) corpus = &candidate;
        }
        if (corpus == nullptr) {
            installed_headers().erase(slot_start);
            it = installed_overlays().erase(it);
            continue;
        }
        // Cheap first: the header settles almost every call without touching the
        // code behind it.
        if (const std::uint64_t head = header_hash(runtime.memory(), slot_start);
            head == installed_headers()[slot_start]) {
            ++it;
            continue;
        }
        if (identity_hash(runtime.memory(), *corpus) == hash) {
            installed_headers()[slot_start] = header_hash(runtime.memory(), slot_start);
            ++it;
            continue;
        }
        // A different overlay now occupies the slot: drop the stale code so the
        // next dispatch there goes through the miss hook.
        runtime.unregister_functions(corpus->base, corpus->base + corpus->size);
        std::cout << "[overlay] " << corpus->name << " was replaced in " << psprecomp::hex32(slot_start) << "\n";
        installed_headers().erase(slot_start);
        it = installed_overlays().erase(it);
    }
}

void forget_unmatched_overlays() { unmatched_slots().clear(); }

void install_overlay_support(Runtime &runtime) {
    (void)runtime;
    load_overlay_libraries();
    psprecomp::set_runtime_missing_function_hook(&missing_function_hook);
    psprecomp::set_runtime_unsupported_hook(&unsupported_instruction_hook);
}

} // namespace mhp3rd
