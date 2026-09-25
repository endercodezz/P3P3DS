// Checks for mod support that need no game data: reading mod folders in the
// community's format, the choices and what they add up to (priority,
// conflicts, packs), importing, the DATA.BIN obfuscation, a synthetic archive
// served with replaced, grown and patched files, and the patch format.
//
//   mhp3rd_mods_tests
//   mhp3rd_mods_tests --check-disc <image.iso>
//
// The second form reads the real DATA.BIN directory from a disc image (read
// only) and checks that it parses and encodes back to the same bytes, and that
// a layout with a grown entry re-keys the entries after it consistently.
#include "mods/file_overlay.hpp"
#include "mods/mhp3rd_data_bin.hpp"
#include "mods/mhp3rd_mod_format.hpp"
#include "mods/mod_import.hpp"
#include "mods/mod_ini.hpp"
#include "mods/mod_library.hpp"
#include "mods/mod_session.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

using namespace mhp3rd::mods;
namespace fs = std::filesystem;
namespace p3 = mhp3rd::mods::p3rd;
using Bytes = std::vector<std::uint8_t>;

namespace {

int failures = 0;

void check(bool condition, const std::string &what) {
    std::printf("%s %s\n", condition ? "ok  " : "FAIL", what.c_str());
    if (!condition) ++failures;
}

void write(const fs::path &path, const std::string &text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}

void write(const fs::path &path, const Bytes &bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char *>(bytes.data()),
                                                static_cast<std::streamsize>(bytes.size()));
}

void put32(Bytes &bytes, std::size_t at, std::uint32_t value) {
    for (unsigned i = 0; i < 4u; ++i) bytes[at + i] = static_cast<std::uint8_t>(value >> (8u * i));
}

void append32(Bytes &bytes, std::uint32_t value) {
    bytes.resize(bytes.size() + 4u);
    put32(bytes, bytes.size() - 4u, value);
}

Bytes pattern(std::size_t size, std::uint32_t seed) {
    std::mt19937 random(seed);
    Bytes bytes(size);
    for (std::uint8_t &b : bytes) b = static_cast<std::uint8_t>(random());
    return bytes;
}

struct Scratch {
    fs::path root;
    Scratch() {
        root = fs::temp_directory_path() /
               ("yakumo_mods_tests_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(root);
    }
    ~Scratch() {
        std::error_code ec;
        fs::remove_all(root, ec);  // only this test's own scratch folder
    }
};

void test_ini() {
    const IniFile ini = IniFile::parse("\xEF\xBB\xBF; comment\r\n[MOD INFO]\r\nName=\"A mod\"\r\nFiles=\"a;b; c;;\"\r\n"
                                       "ModList=\"x;y\r\nVersion=HD\r\n# another\r\n[sub]\r\nType = Patch \r\n");
    check(ini.get("mod info", "name") == "A mod", "ini: quoted value, section and key without case");
    check(split_list(ini.get("MOD INFO", "Files")) == std::vector<std::string>{"a", "b", "c"}, "ini: list split and trimmed");
    check(ini.get("MOD INFO", "ModList") == "x;y", "ini: a quote left open runs to the end of the line");
    check(ini.get("MOD INFO", "Version") == "HD", "ini: unquoted value");
    check(ini.get("sub", "Type") == "Patch", "ini: spaces around = are trimmed");
    check(ini.find("MOD INFO", "missing") == nullptr, "ini: missing key");
}

// A mod folder with a mod.ini in the manager's format.
void make_mod(const fs::path &folder, const std::string &info, const std::vector<std::string> &files = {}) {
    write(folder / "mod.ini", "[MOD INFO]\r\n" + info);
    for (const std::string &file : files) write(folder / file, pattern(64, 7));
}

void test_format(const fs::path &root) {
    const p3::ModFolderFormat format(6043u);
    const fs::path mods = root / "format";
    make_mod(mods / "file_hd", "Name=\"File HD\"\nAuthor=\"Someone\"\nFiles=\"a.pac;B.PAC\"\nTarget=\"0601;05d2\"\n"
                               "Type=\"File\"\nVersion=\"HD\"\nDescription=\"Line one.\\Line two.\"\n",
             {"a.pac", "b.pac"});
    std::optional<Mod> mod = format.read(mods / "file_hd");
    check(mod && mod->unusable.empty() && mod->changes.size() == 2u, "format: a file mod for the HD version");
    check(mod && mod->changes[1].file == 0x5D2 && mod->changes[1].kind == FileChange::Kind::Replace,
          "format: targets are hex file ids; file names match without case");
    check(mod && mod->description == "Line one.\nLine two." && mod->author == "Someone", "format: description lines");

    make_mod(mods / "psp", "Name=\"PSP\"\nFiles=\"a.pac\"\nTarget=\"0601\"\nType=\"File\"\n", {"a.pac"});
    mod = format.read(mods / "psp");
    check(mod && !mod->unusable.empty(), "format: no Version means the PSP version, which cannot be used");

    make_mod(mods / "both", "Name=\"Both\"\nFiles=\"a.pac\"\nTarget=\"0601\"\nFilesHD=\"h.pac\"\nTargetHD=\"0602\"\n"
                            "Type=\"Patch\"\nVersion=\"BOTH\"\n",
             {"a.pac", "h.pac"});
    mod = format.read(mods / "both");
    check(mod && mod->unusable.empty() && mod->changes.size() == 1u && mod->changes[0].file == 0x602 &&
              mod->changes[0].kind == FileChange::Kind::Patch && mod->changes[0].source.filename() == "h.pac",
          "format: BOTH uses FilesHD and TargetHD");

    make_mod(mods / "count", "Files=\"a.pac;b.pac\"\nTarget=\"0601\"\nType=\"File\"\nVersion=\"HD\"\n", {"a.pac", "b.pac"});
    check(!format.read(mods / "count")->unusable.empty(), "format: files and targets must pair up");
    make_mod(mods / "missing", "Files=\"gone.pac\"\nTarget=\"0601\"\nType=\"File\"\nVersion=\"HD\"\n");
    check(!format.read(mods / "missing")->unusable.empty(), "format: a missing file makes the mod unusable");
    make_mod(mods / "range", "Files=\"a.pac\"\nTarget=\"FFFF\"\nType=\"File\"\nVersion=\"HD\"\n", {"a.pac"});
    check(!format.read(mods / "range")->unusable.empty(), "format: a target past the archive is refused");
    make_mod(mods / "code", "Name=\"Code\"\nFiles=\"c.bin\"\nType=\"Code\"\nVersion=\"HD\"\n", {"c.bin"});
    check(!format.read(mods / "code")->unusable.empty(), "format: code mods are listed but cannot be used");
    make_mod(mods / "pack", "Name=\"Pack\"\nType=\"Pack\"\nModList=\"file_hd;both\n");
    mod = format.read(mods / "pack");
    check(mod && mod->members == std::vector<std::string>{"file_hd", "both"}, "format: a pack lists its mods");
    make_mod(mods / "pseudo", "Name=\"Pseudo\"\nType=\"PseudoPack\"\nVersion=\"HD\"\nSubModList=\"one;two;three\"\n"
                              "[one]\nFiles=\"a.pac\"\nTarget=\"0010\"\nType=\"File\"\n[two]\nFiles=\"p.bin\"\n"
                              "Target=\"0011\"\nType=\"Patch\"\n[three]\nFiles=\"c.bin\"\nType=\"Code\"\n",
             {"a.pac", "p.bin", "c.bin"});
    mod = format.read(mods / "pseudo");
    check(mod && mod->unusable.empty() && mod->changes.size() == 2u && mod->notes.size() == 1u,
          "format: a pseudo pack takes its file and patch parts and reports the rest");
    make_mod(mods / "set", "Name=\"Set\"\nType=\"EquipSET\"\nFiles=\"h.pac;null;b.pac;null;l.pac\"\n",
             {"h.pac", "b.pac", "l.pac"});
    mod = format.read(mods / "set");
    check(mod && mod->slots.size() == 3u && mod->slots[1].label == "Body", "format: an armour set has a slot per piece");
    make_mod(mods / "weapon", "Name=\"GS\"\nType=\"EquipGS\"\nFiles=\"w.pac\"\nAnimation=\"w.json\"\n", {"w.pac"});
    mod = format.read(mods / "weapon");
    check(mod && mod->slots.size() == 1u && mod->type == "Great Sword" && !mod->notes.empty(),
          "format: a weapon model, its animations reported");
    write(mods / "withpreview" / "PREVIEW.PNG", "x");
    make_mod(mods / "withpreview", "Name=\"P\"\nType=\"Pack\"\nModList=\"a\"\n");
    check(!format.read(mods / "withpreview")->preview.empty(), "format: the preview is found without case");

    // mhp3reload's own files folder: files named by id, patches with P.
    write(mods / "raw" / "files" / "0601", pattern(16, 1));
    write(mods / "raw" / "files" / "0602P", pattern(16, 2));
    write(mods / "raw" / "files" / "readme.txt", "x");
    mod = format.read(mods / "raw");
    check(mod && mod->changes.size() == 2u && mod->changes[1].kind == FileChange::Kind::Patch,
          "format: a folder of files named by file id");
    fs::create_directories(mods / "empty");
    check(!format.read(mods / "empty"), "format: a folder with neither is not a mod");
    check(format.file_name(0x601) == "0601" && format.parse_file("0x5d2") == 0x5D2u, "format: file id spelling");
}

void test_library(const fs::path &root) {
    const p3::ModFolderFormat format(6043u);
    const fs::path mods = root / "library";
    make_mod(mods / "a", "Name=\"A\"\nFiles=\"x.pac;y.pac\"\nTarget=\"0001;0002\"\nType=\"File\"\nVersion=\"HD\"\n",
             {"x.pac", "y.pac"});
    make_mod(mods / "b", "Name=\"B\"\nFiles=\"x.pac\"\nTarget=\"0001\"\nType=\"File\"\nVersion=\"HD\"\n", {"x.pac"});
    make_mod(mods / "c", "Name=\"C\"\nFiles=\"p.bin\"\nTarget=\"0001\"\nType=\"Patch\"\nVersion=\"HD\"\n"
                         "Depends=\"d\"\n",
             {"p.bin"});
    make_mod(mods / "d", "Name=\"D\"\nFiles=\"p.bin\"\nTarget=\"0003\"\nType=\"Patch\"\nVersion=\"HD\"\n", {"p.bin"});
    make_mod(mods / "pack", "Name=\"Pack\"\nType=\"Pack\"\nModList=\"a;b\"\n");
    make_mod(mods / "code", "Name=\"Code\"\nFiles=\"c.bin\"\nType=\"Code\"\nVersion=\"HD\"\n", {"c.bin"});

    ModLibrary library(format);
    library.load_choices(root / "none.ini");
    library.scan(mods);
    check(library.mods().size() == 6u, "library: every mod folder is read");
    check(library.resolve().empty(), "library: nothing is on at first");

    library.set_enabled("pack", true);
    check(library.enabled("a") && library.enabled("b"), "library: a pack turns its mods on");
    Resolution r = library.resolve();
    const std::string top = library.mods().front().id;
    // The mods seen first, in name order, get the lower ranks: b is above a.
    check(r.replacements.at(1).mod == "b" && r.replacements.at(2).mod == "a", "library: the higher mod wins a file");
    check(r.conflicts.size() == 1u && r.conflicts[0].file == 1u && r.conflicts[0].winner == "b" &&
              r.conflicts[0].overridden == std::vector<std::string>{"a"},
          "library: the conflict is reported");
    library.move("a", +1);
    r = library.resolve();
    check(r.replacements.at(1).mod == "a", "library: raising a mod makes it win");

    library.set_enabled("c", true);
    check(library.enabled("d"), "library: turning a mod on turns on what it depends on");
    r = library.resolve();
    check(r.patches.at(1).size() == 1u && r.patches.at(3).size() == 1u, "library: patches are collected");
    const auto conflict = std::find_if(r.conflicts.begin(), r.conflicts.end(), [](const auto &c) { return c.file == 1u; });
    check(conflict != r.conflicts.end() && conflict->patched_by == std::vector<std::string>{"c"},
          "library: a patch on another mod's replacement is shown");

    library.set_enabled("code", true);
    check(!library.enabled("code"), "library: a mod that cannot be used stays off");
    library.set_enabled("pack", false);
    check(!library.enabled("a") && !library.enabled("b"), "library: a pack turns its mods off");

    std::string error;
    library.set_enabled("b", true);
    check(library.save_choices(root / "mods.ini", error), "library: choices are saved");
    ModLibrary again(format);
    again.load_choices(root / "mods.ini");
    again.scan(mods);
    check(again.enabled("b") && !again.enabled("a") && again.resolve().same_files(library.resolve()),
          "library: saved choices come back");
    again.set_master(false);
    check(again.resolve().empty(), "library: the master switch turns every mod off");
    (void)top;

    // The session applies at once when the game can take it.
    bool accept = true;
    int activations = 0;
    ModSession session(format, {mods, root / "session.ini", nullptr},
                       {[&](const Resolution &) { return accept; }, [&](const Resolution &) { ++activations; }});
    session.start();
    session.library().set_enabled("b", true);
    session.commit();
    check(!session.restart_pending() && activations == 2, "session: a change the game can take applies now");
    accept = false;
    session.library().set_enabled("a", true);
    session.commit();
    check(session.restart_pending() && activations == 2, "session: one it cannot take waits for the restart");
    session.library().set_enabled("a", false);
    session.commit();
    check(!session.restart_pending(), "session: undoing it clears the restart");
}

void test_import(const fs::path &root) {
    const p3::ModFolderFormat format(6043u);
    const fs::path downloads = root / "downloads";
    const fs::path mods = root / "import_mods";
    make_mod(downloads / "single", "Name=\"One\"\nType=\"Pack\"\nModList=\"x\"\n");
    make_mod(downloads / "bundle" / "wrapper" / "m1", "Name=\"M1\"\nType=\"Pack\"\nModList=\"x\"\n");
    make_mod(downloads / "bundle" / "wrapper" / "m2", "Name=\"M2\"\nType=\"Pack\"\nModList=\"x\"\n");
    ImportCheck c = check_import(downloads / "single", format, mods);
    check(c.mods.size() == 1u && c.mods[0].id == "single", "import: a mod folder itself");
    c = check_import(downloads / "bundle", format, mods);
    check(c.mods.size() == 2u, "import: mods one folder further down");
    ImportResult result = import_mods(c, mods);
    check(result.imported.size() == 2u && fs::exists(mods / "m1" / "mod.ini"), "import: copied into the mods folder");
    write(mods / "m1" / "mine.txt", "kept");
    c = check_import(downloads / "bundle", format, mods);
    check(c.mods[0].replaces, "import: an installed mod of the same name is noticed");
    result = import_mods(c, mods);
    check(result.backups.size() == 2u && fs::exists(result.backups[0] / "mine.txt"),
          "import: the replaced mod moves to .backup, nothing is deleted");
    c = check_import(mods / "m1", format, mods);
    check(c.mods.empty() && !c.problem.empty(), "import: not from inside the mods folder");
    c = check_import(root / "nothing", format, mods);
    check(c.mods.empty(), "import: a folder that is not there");
}

void test_cipher() {
    const Bytes plain = pattern(10007, 3);
    Bytes data = plain;
    p3::encrypt(data, 0x12345u, 0u);
    check(data != plain, "cipher: encrypting changes the bytes");
    Bytes pieces = plain;
    // Any split into pieces, even unaligned, gives the same bytes.
    std::size_t at = 0u;
    for (const std::size_t size : {3u, 1u, 4096u, 5u, 131u}) {
        p3::encrypt(std::span(pieces).subspan(at, size), 0x12345u, at);
        at += size;
    }
    p3::encrypt(std::span(pieces).subspan(at), 0x12345u, at);
    check(pieces == data, "cipher: a keystream started at any byte matches");
    Bytes back = data;
    p3::decrypt(back, 0x12345u, 0u);
    check(back == plain, "cipher: decrypting restores the bytes");
    Bytes moved = data;
    p3::transcode(moved, 0x12345u, 0x12400u, 0u);
    Bytes expected = plain;
    p3::encrypt(expected, 0x12400u, 0u);
    check(moved == expected, "cipher: re-keying equals decrypting and encrypting again");
    Bytes zero = plain;
    p3::encrypt(zero, 0u, 0u);
    Bytes seeded = plain;
    p3::encrypt(seeded, 0x10000u, 0u);  // low half 0: its default seed
    check(zero != seeded, "cipher: each half has its own default seed");
    const std::uint8_t psmf[] = {'P', 'S', 'M', 'F', '0'};
    check(p3::verbatim_magic(psmf) && !p3::verbatim_magic(std::span(psmf, 3)), "cipher: verbatim entries by magic");
}

// A small archive: a directory of 2 blocks and five entries. Entry 1 has an
// exact size; entry 3 is stored verbatim.
struct Archive {
    std::vector<Bytes> plain;
    p3::Directory directory;
    Bytes bytes;
};

Archive make_archive() {
    Archive a;
    const std::vector<std::size_t> sizes = {2048, 3000, 6144, 2048, 1000};
    a.directory.directory_blocks = 2u;
    std::uint32_t block = 2u;
    for (std::size_t i = 0; i < sizes.size(); ++i) {
        a.plain.push_back(pattern(sizes[i], static_cast<std::uint32_t>(10 + i)));
        a.directory.blocks.push_back(block);
        block += static_cast<std::uint32_t>((sizes[i] + p3::kBlock - 1u) / p3::kBlock);
    }
    std::memcpy(a.plain[3].data(), "PSMF", 4u);
    a.directory.blocks.push_back(block);
    a.directory.sizes = {{1u, 3000u}, {4u, 1000u}};
    // The trailer: whatever is left of the two directory blocks.
    const std::size_t tables = (a.directory.blocks.size() + 4u) * 4u;
    a.directory.trailer = pattern(2u * p3::kBlock - tables, 99);
    a.bytes = a.directory.encode();
    a.bytes.resize(static_cast<std::size_t>(block) * p3::kBlock, 0u);
    for (std::size_t i = 0; i < sizes.size(); ++i) {
        Bytes stored = a.plain[i];
        if (i != 3u) p3::encrypt(stored, a.directory.blocks[i], 0u);
        std::copy(stored.begin(), stored.end(), a.bytes.begin() + a.directory.blocks[i] * p3::kBlock);
    }
    return a;
}

// What the game would get for entry `i` reading `view` through `directory`.
Bytes game_reads(p3::ArchiveView &view, const p3::Directory &directory, std::uint32_t i) {
    Bytes out(static_cast<std::size_t>(directory.size(i)));
    view.read(static_cast<std::uint64_t>(directory.blocks[i]) * p3::kBlock, out);
    if (!p3::verbatim_magic(out)) p3::decrypt(out, directory.blocks[i], 0u);
    return out;
}

void test_archive(const fs::path &root) {
    const Archive a = make_archive();
    const std::optional<p3::Directory> parsed = p3::Directory::parse(std::span(a.bytes).first(2u * p3::kBlock),
                                                                     a.bytes.size());
    check(parsed && parsed->blocks == a.directory.blocks && parsed->sizes == a.directory.sizes,
          "archive: the directory parses");
    check(parsed && parsed->encode() == Bytes(a.bytes.begin(), a.bytes.begin() + 2 * p3::kBlock),
          "archive: and encodes back to the same bytes");
    const p3::Directory &disc = *parsed;
    const auto raw = [&a](std::uint64_t offset, std::span<std::uint8_t> out) -> std::size_t {
        if (offset >= a.bytes.size()) return 0u;
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(out.size(), a.bytes.size() - offset));
        std::copy_n(a.bytes.begin() + static_cast<std::ptrdiff_t>(offset), count, out.begin());
        return count;
    };
    p3::ArchiveView view(disc, raw);
    check(game_reads(view, disc, 2) == a.plain[2], "archive: without mods the view is the disc");

    // Replacements: entry 1 grows past its blocks, entry 4 shrinks.
    const Bytes grown = pattern(5000, 50);
    const Bytes small = pattern(700, 51);
    write(root / "archive" / "grown", grown);
    write(root / "archive" / "small", small);
    Bytes patch;
    append32(patch, 0x10u);
    append32(patch, 4u);
    append32(patch, 0xAABBCCDDu);
    append32(patch, 0xFFFFFFFFu);
    append32(patch, 0u);
    write(root / "archive" / "patch", patch);
    Resolution r;
    r.replacements[1] = {"m", root / "archive" / "grown"};
    r.replacements[4] = {"m", root / "archive" / "small"};
    r.patches[2] = {{"m", root / "archive" / "patch"}};
    FileOverlay overlay({[&view](FileId file) { return view.original(file); },
                         [&disc](FileId file) { return disc.size(file); },
                         [](FileId, Bytes &bytes, const fs::path &path) { return p3::apply_patch(bytes, path); }});
    overlay.set(r);
    const auto layout = std::make_shared<const p3::Layout>(p3::Layout::build(disc, overlay.sizes()));
    const p3::Directory &d = layout->directory;
    check(d.blocks[2] == disc.blocks[2] + 1u && d.blocks.back() == disc.blocks.back() + 1u,
          "archive: a grown entry moves the ones after it");
    check(d.size(1) == 5000u && d.size(4) == 700u, "archive: the size table takes the new sizes");
    view.set(layout, &overlay);
    check(view.size() == disc.archive_bytes() + p3::kBlock, "archive: the archive grows");
    Bytes dir(2u * p3::kBlock);
    view.read(0u, dir);
    const std::optional<p3::Directory> served = p3::Directory::parse(dir, view.size());
    check(served && served->blocks == d.blocks && served->sizes == d.sizes, "archive: the game gets the new directory");
    check(game_reads(view, d, 1) == grown, "archive: a grown replacement reads back whole");
    check(game_reads(view, d, 4) == small, "archive: a smaller one too");
    check(game_reads(view, d, 0) == a.plain[0], "archive: an entry before the change is untouched");
    Bytes patched = a.plain[2];
    put32(patched, 0x10u, 0xAABBCCDDu);
    check(game_reads(view, d, 2) == patched, "archive: a moved entry is re-keyed, and patched");
    check(game_reads(view, d, 3) == a.plain[3], "archive: a moved verbatim entry is served as stored");
    // The game reads in pieces; so does this.
    Bytes piecewise(static_cast<std::size_t>(d.size(1)));
    for (std::size_t at = 0; at < piecewise.size(); at += 1024u)
        view.read(static_cast<std::uint64_t>(d.blocks[1]) * p3::kBlock + at,
                  std::span(piecewise).subspan(at, std::min<std::size_t>(1024u, piecewise.size() - at)));
    p3::decrypt(piecewise, d.blocks[1], 0u);
    check(piecewise == grown, "archive: reading in pieces gives the same bytes");

    // A replacement that is not a whole number of blocks where the disc has
    // no exact size is padded.
    const p3::Layout padded = p3::Layout::build(disc, {{2u, 100u}});
    check(padded.padded.contains(2u) && padded.directory.size(2) == 3u * p3::kBlock && !padded.moved(disc),
          "archive: a size the table cannot hold is padded to the blocks");
}

void test_patches(const fs::path &root) {
    // A code overlay: MWo3, id, load address, code and data sizes.
    Bytes overlay(64u + 0x100u + 0x40u, 0u);
    std::memcpy(overlay.data(), "MWo3", 4u);
    put32(overlay, 8u, 0x0A055E80u);
    put32(overlay, 12u, 0x100u);
    put32(overlay, 16u, 0x40u);
    Bytes patch;
    const auto block = [&patch](std::uint32_t address, const Bytes &payload, bool run = false) {
        append32(patch, address);
        append32(patch, static_cast<std::uint32_t>(payload.size()) | (run ? 0x80000000u : 0u));
        patch.insert(patch.end(), payload.begin(), payload.end());
    };
    block(0x0A055E80u + 64u + 0x100u + 4u, {1, 2, 3, 4});  // data section
    block(0x4A055E80u + 64u, {9, 9, 9, 9});                // code, through the uncached mirror
    block(0x08801000u, {5, 6});                            // elsewhere in memory
    block(0x08900000u, {7}, true);                         // code to run
    append32(patch, 0xFFFFFFFFu);
    append32(patch, 0u);
    block(0x0A055E80u + 64u, {0xEE});  // after the end marker: ignored
    write(root / "patches" / "p", patch);
    PatchOutcome outcome = p3::apply_patch(overlay, root / "patches" / "p");
    check(overlay[64u + 0x100u + 4u] == 1u && overlay[64u + 0x103u + 4u] == 4u, "patch: an overlay's data by address");
    check(overlay[64u] == 9u, "patch: its code, through a mirror address");
    check(outcome.applied == 2u && outcome.after_load.size() == 1u && outcome.after_load[0].address == 0x08801000u,
          "patch: memory outside the overlay is written after it loads");
    check(outcome.problems.size() == 2u, "patch: code to run and code changed are reported");

    Bytes data = pattern(256, 5);
    patch.clear();
    block(0x20u, {0xAB, 0xCD});
    block(0x08801000u, {1});
    block(0x1000u, {1});
    write(root / "patches" / "q", patch);  // no end marker, as the manager installs it
    outcome = p3::apply_patch(data, root / "patches" / "q");
    check(data[0x20] == 0xABu && data[0x21] == 0xCDu && outcome.applied == 1u, "patch: an offset into a data file");
    check(outcome.problems.size() == 2u && outcome.after_load.empty(),
          "patch: memory writes and blocks past the end of a data file are refused");
}

int check_disc(const char *image) {
    std::ifstream in(image, std::ios::binary);
    if (!in) {
        std::printf("cannot open %s\n", image);
        return 1;
    }
    // Find /PSP_GAME/USRDIR/DATA.BIN the simple way: the tool in
    // profiles/mhp3rd/tools/databin.py does the same walk.
    const auto sector = [&in](std::uint64_t lba, std::size_t count) {
        Bytes bytes(count * 2048u);
        in.seekg(static_cast<std::streamoff>(lba * 2048u));
        in.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        return bytes;
    };
    const auto load = [](const Bytes &b, std::size_t at) {
        return static_cast<std::uint32_t>(b[at]) | static_cast<std::uint32_t>(b[at + 1]) << 8u |
               static_cast<std::uint32_t>(b[at + 2]) << 16u | static_cast<std::uint32_t>(b[at + 3]) << 24u;
    };
    const Bytes primary = sector(16, 1);
    std::uint32_t lba = load(primary, 156 + 2);
    std::uint32_t size = load(primary, 156 + 10);
    for (const char *want : {"PSP_GAME", "USRDIR", "DATA.BIN"}) {
        const Bytes dir = sector(lba, (size + 2047u) / 2048u);
        bool found = false;
        for (std::size_t at = 0; at < dir.size() && !found;) {
            const std::uint8_t length = dir[at];
            if (length == 0u) {
                at = (at / 2048u + 1u) * 2048u;
                continue;
            }
            const std::string name(reinterpret_cast<const char *>(&dir[at + 33]), dir[at + 32]);
            if (name.substr(0, name.find(';')) == want) {
                lba = load(dir, at + 2);
                size = load(dir, at + 10);
                found = true;
            }
            at += length;
        }
        if (!found) {
            std::printf("no %s in the image\n", want);
            return 1;
        }
    }
    const Bytes head = sector(lba, 17);
    const std::optional<p3::Directory> directory = p3::Directory::parse(head, size);
    check(directory.has_value(), "disc: DATA.BIN's directory parses");
    if (!directory) return 1;
    std::printf("     %zu entries, %zu exact sizes, %u directory blocks, %zu trailer bytes\n", directory->entries(),
                directory->sizes.size(), directory->directory_blocks, directory->trailer.size());
    const Bytes encoded = directory->encode();
    check(encoded == Bytes(head.begin(), head.begin() + static_cast<std::ptrdiff_t>(encoded.size())),
          "disc: the directory encodes back to the bytes on the disc");
    const p3::Layout grown = p3::Layout::build(*directory, {{0x0FEEu, directory->size(0x0FEE) + 300u * 1024u}});
    check(grown.moved(*directory) && grown.directory.blocks[0x0FEF] == directory->blocks[0x0FEF] + 150u,
          "disc: growing an entry moves the ones after it");
    return failures == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char **argv) {
    if (argc == 3 && std::string(argv[1]) == "--check-disc") return check_disc(argv[2]);
    const Scratch scratch;
    test_ini();
    test_format(scratch.root);
    test_library(scratch.root);
    test_import(scratch.root);
    test_cipher();
    test_archive(scratch.root);
    test_patches(scratch.root);
    std::printf("%s\n", failures == 0 ? "all mod checks passed" : "mod checks FAILED");
    return failures == 0 ? 0 : 1;
}
