#include "mods/mhp3rd_mod_format.hpp"

#include "mods/mod_ini.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>
#include <system_error>

namespace mhp3rd::mods::p3rd {
namespace {

namespace fs = std::filesystem;

constexpr const char *kInfo = "MOD INFO";

// Guest RAM on the PSP; addresses given through a cached or uncached mirror
// are folded onto it first.
constexpr std::uint32_t kRamStart = 0x08000000u;
constexpr std::uint32_t kRamEnd = 0x0C000000u;
constexpr std::uint32_t kMirrorMask = 0x3FFFFFFFu;

// Equipment types from the manager's documentation, and how they are shown.
struct EquipType {
    const char *key;
    const char *label;
};
constexpr EquipType kEquipTypes[] = {
    {"GS", "Great Sword"},    {"LS", "Long Sword"},       {"SNS", "Sword and Shield"}, {"DB", "Dual Blades"},
    {"LNC", "Lance"},         {"GL", "Gunlance"},         {"HMR", "Hammer"},           {"HH", "Hunting Horn"},
    {"LBG", "Light Bowgun"},  {"HBG", "Heavy Bowgun"},    {"BOW", "Bow"},              {"SAXE", "Switch Axe"},
    {"HEAD", "Head armour"},  {"ARMS", "Arm armour"},     {"BODY", "Body armour"},     {"WAIST", "Waist armour"},
    {"LEGS", "Leg armour"},   {"CATHELM", "Felyne helm"}, {"CATPLATE", "Felyne plate"}, {"CATWPN", "Felyne weapon"},
};

std::string hex4(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof(text), "%04X", value);
    return text;
}

std::string hex8(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof(text), "0x%08X", value);
    return text;
}

// A file of the mod, found without regard to case (mods are made on Windows
// and played on Linux too). `relative` may use either slash.
std::optional<fs::path> find_file(const fs::path &folder, const std::string &relative) {
    fs::path at = folder;
    std::string rest = relative;
    std::replace(rest.begin(), rest.end(), '\\', '/');
    for (const std::string &part : split_list(rest, '/')) {
        std::error_code ec;
        if (fs::exists(at / from_utf8(part), ec)) {
            at /= from_utf8(part);
            continue;
        }
        bool found = false;
        for (fs::directory_iterator it(at, ec), end; !ec && it != end; it.increment(ec)) {
            if (iequals(to_utf8(it->path().filename()), part)) {
                at = it->path();
                found = true;
                break;
            }
        }
        if (!found) return std::nullopt;
    }
    std::error_code ec;
    if (!fs::is_regular_file(at, ec)) return std::nullopt;
    return at;
}

std::uint32_t load32(const std::vector<std::uint8_t> &bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) | static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u |
           static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u | static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u;
}

// "0601" -> 0x601; also without leading zeros, and with 0x.
std::optional<FileId> parse_hex(std::string text) {
    text = trim(text);
    if (text.size() > 2u && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) text = text.substr(2);
    if (text.empty() || text.size() > 6u) return std::nullopt;
    FileId value{};
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, 16);
    if (ec != std::errc{} || ptr != text.data() + text.size()) return std::nullopt;
    return value;
}

// A file named like mhp3reload's files folder has them: 0601 or 0601P.
struct IdFile {
    FileId file{};
    bool patch{};
};
std::optional<IdFile> id_file_name(const std::string &name) {
    std::string stem = name;
    bool patch = false;
    if (stem.size() == 5u && (stem.back() == 'P' || stem.back() == 'p')) {
        patch = true;
        stem.pop_back();
    }
    if (stem.size() != 4u || !std::all_of(stem.begin(), stem.end(), [](unsigned char c) { return std::isxdigit(c); }))
        return std::nullopt;
    const auto file = parse_hex(stem);
    if (!file) return std::nullopt;
    return IdFile{*file, patch};
}

class Reader {
public:
    Reader(const ModFolderFormat &format, std::uint32_t entries, const fs::path &folder, Mod &mod)
        : format_(format), entries_(entries), folder_(folder), mod_(mod) {}

    // Files and targets as the Version key selects them.
    void file_changes(const IniFile &ini, const std::string &section, const std::string &version,
                      FileChange::Kind kind) {
        std::string files = ini.get(section, "Files");
        std::string targets = ini.get(section, "Target");
        if (version == "BOTH") {
            if (const std::string *hd = ini.find(section, "FilesHD")) files = *hd;
            if (const std::string *hd = ini.find(section, "TargetHD")) targets = *hd;
        } else if (version != "HD") {
            unusable("Made for the PSP version" +
                     std::string(version.empty() ? " (it gives no Version, which means PSP)" : "") +
                     ": its file ids are not the HD version's.");
            return;
        }
        const std::vector<std::string> names = split_list(files);
        const std::vector<std::string> ids = split_list(targets);
        if (names.empty()) {
            unusable("Its mod.ini lists no files.");
            return;
        }
        if (names.size() != ids.size()) {
            unusable("Its mod.ini lists " + std::to_string(names.size()) + " files for " + std::to_string(ids.size()) +
                     " targets.");
            return;
        }
        for (std::size_t i = 0; i < names.size(); ++i) {
            const std::optional<FileId> file = format_.parse_file(ids[i]);
            if (!file) {
                unusable("Target \"" + ids[i] + "\" is not a file id.");
                return;
            }
            if (entries_ != 0u && *file >= entries_) {
                unusable("Target " + format_.file_name(*file) + " is not a file of this game.");
                return;
            }
            const std::optional<fs::path> source = find_file(folder_, names[i]);
            if (!source) {
                unusable("The file \"" + names[i] + "\" is missing from the folder.");
                return;
            }
            mod_.changes.push_back({kind, *file, *source});
        }
    }

    void slots(const std::string &files, const std::vector<std::string> &labels) {
        const std::vector<std::string> names = split_list(files);
        if (names.size() != labels.size()) {
            unusable("Its mod.ini lists " + std::to_string(names.size()) + " files; this type takes " +
                     std::to_string(labels.size()) + ".");
            return;
        }
        for (std::size_t i = 0; i < names.size(); ++i) {
            if (iequals(names[i], "null")) continue;
            const std::optional<fs::path> source = find_file(folder_, names[i]);
            if (!source) {
                unusable("The file \"" + names[i] + "\" is missing from the folder.");
                return;
            }
            mod_.slots.push_back({labels[i], *source});
        }
    }

    void unusable(std::string why) {
        if (mod_.unusable.empty()) mod_.unusable = std::move(why);
    }

private:
    const ModFolderFormat &format_;
    std::uint32_t entries_;
    const fs::path &folder_;
    Mod &mod_;
};

std::string version_label(const std::string &version) {
    if (version == "HD") return "HD version";
    if (version == "BOTH") return "PSP and HD versions";
    return "PSP version";
}

// A folder without a mod.ini, holding files named by file id.
std::optional<Mod> read_id_files(const ModFolderFormat &format, std::uint32_t entries, const fs::path &folder) {
    Mod mod;
    for (const fs::path &where : {folder, folder / "files"}) {
        std::error_code ec;
        for (fs::directory_iterator it(where, ec), end; !ec && it != end; it.increment(ec)) {
            if (!it->is_regular_file(ec)) continue;
            const auto id = id_file_name(to_utf8(it->path().filename()));
            if (!id || (entries != 0u && id->file >= entries)) continue;
            mod.changes.push_back(
                {id->patch ? FileChange::Kind::Patch : FileChange::Kind::Replace, id->file, it->path()});
        }
    }
    if (mod.changes.empty()) return std::nullopt;
    std::sort(mod.changes.begin(), mod.changes.end(),
              [](const FileChange &a, const FileChange &b) { return a.file < b.file; });
    mod.name = to_utf8(folder.filename());
    const bool patches = std::any_of(mod.changes.begin(), mod.changes.end(),
                                     [](const FileChange &c) { return c.kind == FileChange::Kind::Patch; });
    const bool files = std::any_of(mod.changes.begin(), mod.changes.end(),
                                   [](const FileChange &c) { return c.kind == FileChange::Kind::Replace; });
    mod.type = files && patches ? "Files and patches" : patches ? "Patch" : "Files";
    mod.version = "HD version (assumed)";
    mod.description = "No mod.ini: its files are named by the file id they replace (" +
                      format.file_name(mod.changes.front().file) +
                      "), as in mhp3reload's files folder. Taken to be made for the HD version.";
    return mod;
}

} // namespace

std::string ModFolderFormat::file_name(FileId file) const { return hex4(file); }

std::optional<FileId> ModFolderFormat::parse_file(const std::string &text) const { return parse_hex(text); }

std::optional<Mod> ModFolderFormat::read(const fs::path &folder) const {
    const std::optional<fs::path> ini_path = find_file(folder, "mod.ini");
    if (!ini_path) return read_id_files(*this, entries_, folder);
    const std::optional<IniFile> ini = IniFile::load(*ini_path);
    if (!ini) return std::nullopt;

    Mod mod;
    mod.name = ini->get(kInfo, "Name");
    if (mod.name.empty()) mod.name = to_utf8(folder.filename());
    mod.author = ini->get(kInfo, "Author");
    std::string description = ini->get(kInfo, "Description");
    std::replace(description.begin(), description.end(), '\\', '\n');
    mod.description = std::move(description);
    const std::string version = [&] {
        std::string v = trim(ini->get(kInfo, "Version"));
        std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return std::toupper(c); });
        return v;
    }();
    mod.depends = split_list(ini->get(kInfo, "Depends"));
    if (const std::optional<fs::path> preview = find_file(folder, "preview.png")) mod.preview = *preview;
    if (ini->find(kInfo, "Script") != nullptr)
        mod.notes.push_back("Its settings script is not run: scripts are for the PSP mod manager.");
    if (!ini->has_section(kInfo)) mod.unusable = "Its mod.ini has no [MOD INFO] section.";

    Reader reader(*this, entries_, folder, mod);
    const std::string type = trim(ini->get(kInfo, "Type"));
    mod.version = version_label(version);
    if (iequals(type, "File")) {
        mod.type = "Files";
        reader.file_changes(*ini, kInfo, version, FileChange::Kind::Replace);
    } else if (iequals(type, "Patch")) {
        mod.type = "Patch";
        reader.file_changes(*ini, kInfo, version, FileChange::Kind::Patch);
    } else if (iequals(type, "Code")) {
        mod.type = "Code";
        reader.unusable("A code mod runs its own PSP code inside the game. Yakumo runs the game as recompiled "
                        "native code, so it cannot load these yet.");
    } else if (iequals(type, "Pack")) {
        mod.type = "Pack";
        mod.members = split_list(ini->get(kInfo, "ModList"));
        mod.version.clear();
        if (mod.members.empty()) reader.unusable("The pack lists no mods.");
    } else if (iequals(type, "PseudoPack")) {
        mod.type = "Pack";
        for (const std::string &sub : split_list(ini->get(kInfo, "SubModList"))) {
            const std::string sub_type = trim(ini->get(sub, "Type"));
            Mod part;
            Reader sub_reader(*this, entries_, folder, part);
            if (iequals(sub_type, "File")) sub_reader.file_changes(*ini, sub, version, FileChange::Kind::Replace);
            else if (iequals(sub_type, "Patch")) sub_reader.file_changes(*ini, sub, version, FileChange::Kind::Patch);
            else part.unusable = "its type " + (sub_type.empty() ? std::string("is missing") : sub_type) +
                                 " cannot be part of a pack here";
            if (!part.unusable.empty()) {
                mod.notes.push_back("Part \"" + sub + "\" is left out: " + part.unusable);
                continue;
            }
            mod.changes.insert(mod.changes.end(), part.changes.begin(), part.changes.end());
        }
        if (mod.changes.empty()) reader.unusable("None of its parts can be used here.");
    } else if (type.size() > 5u && iequals(std::string_view(type).substr(0, 5), "Equip")) {
        const std::string kind = type.substr(5);
        mod.version.clear();
        if (iequals(kind, "SET")) {
            mod.type = "Armour set";
            reader.slots(ini->get(kInfo, "Files"), {"Head", "Arms", "Body", "Waist", "Legs"});
        } else if (iequals(kind, "CATSET")) {
            mod.type = "Felyne armour set";
            reader.slots(ini->get(kInfo, "Files"), {"Felyne helm", "Felyne plate"});
        } else {
            const auto found = std::find_if(std::begin(kEquipTypes), std::end(kEquipTypes),
                                            [&kind](const EquipType &t) { return iequals(t.key, kind); });
            if (found == std::end(kEquipTypes)) {
                mod.type = type;
                reader.unusable("Unknown equipment type " + kind + ".");
            } else {
                mod.type = found->label;
                reader.slots(ini->get(kInfo, "Files"), {found->label});
            }
        }
        if (ini->find(kInfo, "Animation") != nullptr)
            mod.notes.push_back("Its custom animations are not used: they need a code mod.");
        if (ini->find(kInfo, "Audio") != nullptr)
            mod.notes.push_back("Its custom sounds are not used yet.");
    } else {
        mod.type = type.empty() ? "Unknown" : type;
        reader.unusable(type.empty() ? "Its mod.ini gives no Type." : "Unknown type " + type + ".");
    }
    return mod;
}

std::optional<OverlayImage> overlay_image(const std::vector<std::uint8_t> &bytes) {
    constexpr std::size_t kHeader = 64u;
    if (bytes.size() < kHeader || bytes[0] != 'M' || bytes[1] != 'W' || bytes[2] != 'o' || bytes[3] != '3')
        return std::nullopt;
    OverlayImage image;
    image.load = load32(bytes, 8u);
    image.code_end = image.load + static_cast<std::uint32_t>(kHeader) + load32(bytes, 12u);
    image.size = static_cast<std::uint32_t>(bytes.size());
    return image;
}

PatchOutcome apply_patch(std::vector<std::uint8_t> &bytes, const fs::path &patch) {
    PatchOutcome outcome;
    std::ifstream in(patch, std::ios::binary);
    if (!in) {
        outcome.problems.push_back("cannot read " + to_utf8(patch.filename()));
        return outcome;
    }
    const std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    const std::optional<OverlayImage> overlay = overlay_image(bytes);
    std::size_t at = 0u;
    while (at + 8u <= data.size()) {
        const std::uint32_t address = load32(data, at);
        const std::uint32_t length_word = load32(data, at + 4u);
        if (address == 0xFFFFFFFFu) break;
        const std::uint32_t length = length_word & 0x7FFFFFFFu;
        if (at + 8u + length > data.size()) {
            outcome.problems.push_back("the block for " + hex8(address) + " runs past the end of the patch");
            break;
        }
        const auto payload = data.begin() + static_cast<std::ptrdiff_t>(at + 8u);
        at += 8u + length;
        if ((length_word & 0x80000000u) != 0u) {
            outcome.problems.push_back("the block for " + hex8(address) +
                                       " is code that runs as it loads, which this port cannot run");
            continue;
        }
        const std::uint32_t guest = address & kMirrorMask;
        const bool in_ram = guest >= kRamStart && guest < kRamEnd;
        if (overlay && in_ram && guest >= overlay->load && guest - overlay->load + length <= overlay->size) {
            std::copy_n(payload, length, bytes.begin() + (guest - overlay->load));
            ++outcome.applied;
            if (guest < overlay->code_end)
                outcome.problems.push_back("the block for " + hex8(address) +
                                           " changes code: that overlay then runs in the interpreter");
        } else if (overlay && in_ram) {
            outcome.after_load.push_back({guest, std::vector<std::uint8_t>(payload, payload + length)});
        } else if (!in_ram && static_cast<std::uint64_t>(address) + length <= bytes.size()) {
            std::copy_n(payload, length, bytes.begin() + address);
            ++outcome.applied;
        } else if (in_ram) {
            outcome.problems.push_back("the block for " + hex8(address) +
                                       " writes to memory, but this file does not load to a fixed address");
        } else {
            outcome.problems.push_back("the block for " + hex8(address) + " is outside the file");
        }
    }
    return outcome;
}

} // namespace mhp3rd::mods::p3rd
