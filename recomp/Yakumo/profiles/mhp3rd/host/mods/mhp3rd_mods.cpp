#include "mods/mhp3rd_mods.hpp"

#include "mods/file_overlay.hpp"
#include "mods/mhp3rd_data_bin.hpp"
#include "mods/mhp3rd_mod_format.hpp"
#include "mods/mod_ini.hpp"

#include "install/user_data.hpp"
#include "kernel/iso_image.hpp"

#include "psprecomp/runtime.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <vector>

namespace mhp3rd::mods {
namespace {

namespace fs = std::filesystem;
using p3rd::ArchiveView;
using p3rd::Directory;
using p3rd::Layout;

constexpr const char *kDataBin = "PSP_GAME/USRDIR/DATA.BIN";

bool trace() {
    static const bool enabled = std::getenv("MHP3RD_TRACE_MODS") != nullptr;
    return enabled;
}

const char *disabled_by() {
    const char *value = std::getenv("MHP3RD_NO_MODS");
    return value != nullptr && *value != '\0' && std::string(value) != "0" ? "MHP3RD_NO_MODS" : nullptr;
}

std::string hex(std::uint32_t value, int digits = 8) {
    char text[16];
    std::snprintf(text, sizeof(text), "%0*X", digits, value);
    return text;
}

struct State {
    IsoImage *disc{};
    std::optional<DiscRange> range;
    std::optional<Directory> directory;
    p3rd::ModFolderFormat format{0u};
    std::unique_ptr<ArchiveView> view;
    std::unique_ptr<FileOverlay> overlay;
    std::unique_ptr<ModSession> session;
    std::shared_ptr<const Layout> layout;  // null: the disc's own archive
    bool serving{};
    bool switched_live{};                  // a change was applied while running
    std::optional<Resolution> waiting;     // a switch waiting for a file's load to end
    std::optional<FileId> partial;         // a file the game has begun but not finished reading
    std::set<FileId> loaded_code;          // overlays read in full with memory writes due
    std::set<FileId> reported;             // files whose problems were logged
    std::vector<std::string> problems;
};

State &state() {
    static State s;
    return s;
}

std::size_t raw_read(std::uint64_t offset, std::span<std::uint8_t> out) {
    State &s = state();
    if (!s.disc || !s.range || offset >= s.range->size) return 0u;
    const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(out.size(), s.range->size - offset));
    return s.disc->read(s.range->offset + offset, out.first(count));
}

std::string mod_name(const std::string &id) {
    State &s = state();
    if (s.session) {
        if (const Mod *mod = s.session->library().find(id)) return mod->name;
    }
    return id;
}

// Sizes the files would have with `resolution`, as the layout needs them.
std::map<FileId, std::uint64_t> sizes_for(const Resolution &resolution) {
    State &s = state();
    FileOverlay sizing(FileOverlay::Game{
        {}, [&s](FileId file) { return s.directory->size(file); }, {}});
    sizing.set(resolution);
    return sizing.sizes();
}

std::shared_ptr<const Layout> layout_for(const Resolution &resolution) {
    State &s = state();
    if (resolution.empty() || !s.directory) return nullptr;
    return std::make_shared<const Layout>(Layout::build(*s.directory, sizes_for(resolution)));
}

void log_resolution(const Resolution &resolution, const Layout *layout) {
    State &s = state();
    if (resolution.empty()) {
        std::cout << "[mods] no mod changes the game's files\n";
        return;
    }
    std::cout << "[mods] " << resolution.replacements.size() << " file(s) replaced, " << resolution.patches.size()
              << " patched";
    if (layout != nullptr && layout->moved(*s.directory))
        std::cout << "; DATA.BIN grows from " << s.directory->archive_bytes() << " to "
                  << layout->directory.archive_bytes() << " bytes";
    std::cout << "\n";
    if (!trace()) return;
    for (const auto &[file, source] : resolution.replacements) {
        std::cout << "[mods]   " << s.format.file_name(file) << " <- " << to_utf8(source.path) << " ("
                  << mod_name(source.mod) << ", " << (layout ? layout->directory.size(file) : 0u) << " bytes, the disc's "
                  << s.directory->size(file) << ")";
        if (layout != nullptr && layout->padded.contains(file)) std::cout << ", padded with zeros to its blocks";
        std::cout << "\n";
    }
    for (const auto &[file, patches] : resolution.patches)
        for (const Resolution::Source &source : patches)
            std::cout << "[mods]   " << s.format.file_name(file) << " patched by " << to_utf8(source.path) << " ("
                      << mod_name(source.mod) << ")\n";
    for (const Resolution::Conflict &conflict : resolution.conflicts) {
        std::cout << "[mods]   conflict on " << s.format.file_name(conflict.file) << ":";
        if (!conflict.winner.empty()) std::cout << " " << mod_name(conflict.winner) << " wins";
        for (const std::string &loser : conflict.overridden) std::cout << ", over " << mod_name(loser);
        for (const std::string &patcher : conflict.patched_by) std::cout << ", patched by " << mod_name(patcher);
        std::cout << "\n";
    }
}

void activate_now(const Resolution &resolution) {
    State &s = state();
    s.waiting.reset();
    if (!s.overlay || !s.view) return;
    s.overlay->set(resolution);
    s.layout = layout_for(resolution);
    s.view->set(s.layout, s.overlay.get());
    s.serving = s.layout != nullptr;
    s.reported.clear();
    s.loaded_code.clear();
    log_resolution(resolution, s.layout.get());
}

bool can_switch(const Resolution &next) {
    State &s = state();
    if (!s.directory) return false;
    const std::shared_ptr<const Layout> wanted = layout_for(next);
    const Layout disc = Layout::build(*s.directory, {});
    const Layout &now = s.layout ? *s.layout : disc;
    return (wanted ? *wanted : disc).same_as(now);
}

// Whether switching to `next` would change a file the game is halfway
// through reading. Streamed music is read a piece at a time all along, so only
// the files the switch changes count.
bool switch_waits(const Resolution &next) {
    State &s = state();
    if (!s.serving || !s.partial) return false;
    const FileId file = *s.partial;
    return s.overlay->touches(file) || next.replacements.contains(file) || next.patches.contains(file);
}

void activate(const Resolution &next) {
    State &s = state();
    if (s.session && s.overlay && s.overlay->resolution().same_files(next)) return;
    // A file the game is halfway through reading keeps its bytes to the end.
    if (switch_waits(next)) {
        s.waiting = next;
        return;
    }
    if (s.session) s.switched_live = true;
    activate_now(next);
}

void note_problems(FileId file) {
    State &s = state();
    if (s.reported.contains(file)) return;
    s.reported.insert(file);
    const std::shared_ptr<const FileContent> content = s.overlay->content(file);
    if (!content) return;
    for (const std::string &problem : content->problems) {
        const std::string line = s.format.file_name(file) + ": " + problem;
        std::cout << "[mods] " << line << "\n";
        s.problems.push_back(line);
    }
}

} // namespace

fs::path mods_directory() {
    if (const char *dir = std::getenv("MHP3RD_MODS_DIR"); dir != nullptr && *dir != '\0')
        return install::path_from_utf8(dir);
    try {
        return install::user_data_directory() / "mods";
    } catch (const std::exception &) {
        return {};
    }
}

void attach_disc(IsoImage *disc) {
    State &s = state();
    s = State{};
    s.disc = disc;
    if (disc == nullptr) return;
    const std::optional<IsoImage::Entry> entry = disc->find(kDataBin);
    if (!entry || entry->directory) return;
    s.range = DiscRange{static_cast<std::uint64_t>(entry->lba) * IsoImage::kSectorSize, entry->size};

    // The directory is small: its first word says how many blocks it takes.
    std::vector<std::uint8_t> head(p3rd::kBlock);
    head.resize(raw_read(0u, head));
    std::vector<std::uint8_t> first(head);
    p3rd::decrypt(first, 0u, 0u);
    const std::uint32_t blocks = first.size() >= 4u ? static_cast<std::uint32_t>(first[0] | first[1] << 8u |
                                                                                 first[2] << 16u | first[3] << 24u)
                                                    : 0u;
    if (blocks > 0u && blocks < 512u) {
        std::vector<std::uint8_t> encrypted(static_cast<std::size_t>(blocks) * p3rd::kBlock);
        encrypted.resize(raw_read(0u, encrypted));
        s.directory = Directory::parse(encrypted, s.range->size);
    }
    if (!s.directory) {
        std::cerr << "[mods] DATA.BIN's directory could not be read; mods are off\n";
        return;
    }
    s.format.set_entries(static_cast<std::uint32_t>(s.directory->entries()));
    s.view = std::make_unique<ArchiveView>(*s.directory, &raw_read);
    s.overlay = std::make_unique<FileOverlay>(FileOverlay::Game{
        [&s](FileId file) { return s.view->original(file); },
        [&s](FileId file) { return s.directory->size(file); },
        [](FileId, std::vector<std::uint8_t> &bytes, const fs::path &patch) {
            return p3rd::apply_patch(bytes, patch);
        }});

    ModSession::Paths paths;
    paths.folder = mods_directory();
    try {
        paths.choices = install::user_data_directory() / "mods.ini";
    } catch (const std::exception &) {
        paths.choices = paths.folder.parent_path() / "mods.ini";
    }
    paths.disabled_by = disabled_by();
    s.session = std::make_unique<ModSession>(s.format, paths, ModSession::Game{&can_switch, &activate});
    s.session->start();
    s.switched_live = false;
    if (paths.disabled_by != nullptr) std::cout << "[mods] off for this run: " << paths.disabled_by << " is set\n";
    if (trace())
        std::cout << "[mods] " << s.session->library().mods().size() << " mod(s) in " << to_utf8(paths.folder)
                  << "; choices in " << to_utf8(paths.choices) << "\n";
}

std::optional<DiscRange> data_bin_on_disc() { return state().range; }

bool serving() { return state().serving; }

std::uint64_t data_bin_size() {
    State &s = state();
    if (s.view && s.serving) return s.view->size();
    return s.range ? s.range->size : 0u;
}

std::size_t read_data_bin(std::uint64_t offset, std::span<std::uint8_t> out) {
    State &s = state();
    if (!s.serving || !s.view) return raw_read(offset, out);
    if (s.waiting && !switch_waits(*s.waiting)) activate_now(*s.waiting);
    if (!s.serving) return raw_read(offset, out);

    std::vector<FileId> touched;
    const std::size_t count = s.view->read(offset, out, &touched);
    if (count == 0u) return 0u;

    // Which file the read ended in, and whether the game has all of it now.
    const Directory &d = s.layout->directory;
    const std::uint64_t end = offset + count;
    s.partial.reset();
    if (const std::int64_t last = d.entry_at(static_cast<std::uint32_t>((end - 1u) / p3rd::kBlock)); last >= 0) {
        const auto file = static_cast<FileId>(last);
        // Which file each read ends in: how to find the id of a model the game shows.
        static const bool trace_reads = std::getenv("MHP3RD_TRACE_DATA_BIN") != nullptr;
        if (trace_reads) std::cout << "[mods] data " << s.format.file_name(file) << " (" << d.size(file) << " bytes)\n";
        const std::uint64_t file_end = static_cast<std::uint64_t>(d.blocks[file]) * p3rd::kBlock + d.size(file);
        if (end < file_end) s.partial = file;
        else if (s.overlay->touches(file)) {
            const auto content = s.overlay->content(file);
            if (content && !content->after_load.empty()) s.loaded_code.insert(file);
        }
    }
    if (!touched.empty()) {
        FileId previous = ~FileId{};
        for (const FileId file : touched) {
            if (file == previous) continue;
            previous = file;
            note_problems(file);
            if (!trace()) continue;
            const std::uint64_t start = static_cast<std::uint64_t>(d.blocks[file]) * p3rd::kBlock;
            const std::uint64_t from = offset > start ? offset - start : 0u;
            const Resolution &r = s.overlay->resolution();
            std::string source;
            if (const auto found = r.replacements.find(file); found != r.replacements.end())
                source = "replaced by " + mod_name(found->second.mod);
            if (const auto found = r.patches.find(file); found != r.patches.end()) {
                for (const Resolution::Source &patch : found->second)
                    source += (source.empty() ? "patched by " : ", patched by ") + mod_name(patch.mod);
            }
            std::cout << "[mods] read " << s.format.file_name(file) << " +" << from << " "
                      << std::min<std::uint64_t>(end, start + d.span(file)) - (start + from) << " of "
                      << d.size(file) << " bytes: " << source << "\n";
        }
    }
    return count;
}

void code_loaded(psprecomp::Runtime &runtime) {
    State &s = state();
    if (s.loaded_code.empty() || !s.overlay) return;
    for (const FileId file : s.loaded_code) {
        const std::shared_ptr<const FileContent> content = s.overlay->content(file);
        if (!content) continue;
        const std::optional<p3rd::OverlayImage> image = p3rd::overlay_image(content->bytes);
        if (!image) continue;
        // Only if that overlay is the one in its slot now.
        auto &memory = runtime.memory();
        bool present = true;
        for (std::uint32_t i = 0; i < 8u && present; ++i)
            present = memory.load8(image->load + i) == content->bytes[i];
        if (!present) continue;
        for (const MemoryWrite &write : content->after_load) {
            memory.copy_in(write.address, write.bytes);
            if (trace())
                std::cout << "[mods] after loading " << s.format.file_name(file) << ": wrote " << write.bytes.size()
                          << " bytes at 0x" << hex(write.address) << "\n";
        }
    }
    s.loaded_code.clear();
}

ModSession *session() { return state().session.get(); }

std::string pending_note() {
    State &s = state();
    if (!s.session) return {};
    if (s.session->restart_pending())
        return "Restart Yakumo to apply: a file changes size in a way the running game cannot take.";
    if (s.waiting) return "Applies once the game finishes loading the file it is reading.";
    if (s.switched_live)
        return "Applied: each file changes the next time the game loads it, usually at the next area or menu. "
               "Files it loads only once at start need a restart.";
    return {};
}

const std::vector<std::string> &problems() { return state().problems; }

} // namespace mhp3rd::mods
