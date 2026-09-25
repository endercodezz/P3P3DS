#include "save_data/save_transfer.hpp"

#include "save_data/param_sfo.hpp"
#include "save_data/savedata_crypto.hpp"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iterator>
#include <mutex>
#include <system_error>

namespace mhp3rd::savedata {
namespace {

namespace fs = std::filesystem;

constexpr const char *kParamSfo = "PARAM.SFO";
// A real PARAM.SFO is a few kilobytes; anything far larger is not one.
constexpr std::uintmax_t kMaxParamSfoBytes = 64u * 1024u;
// SAVEDATA_FILE_LIST: entries of a 13-byte name, its 16-byte hash and 3 bytes
// of padding.
constexpr std::size_t kFileListEntrySize = 32u;
constexpr std::size_t kFileListNameSize = 13u;
// The folders an export carries: the install data is a cache the game
// rebuilds, and it is several times the size of the rest.
constexpr std::string_view kExportedFolders[] = {"ULJM05800", "ULJM05800QST"};

struct Session {
    std::mutex mutex;
    std::optional<Block> key;
    fs::path memory_stick;
};

Session &session() {
    static Session s;
    return s;
}

std::optional<std::vector<std::uint8_t>> read_file(const fs::path &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// A path as UTF-8, for messages and names; path::string() can throw on
// Windows for names outside the system code page.
std::string text(const fs::path &path) {
    const std::u8string utf8 = path.u8string();
    return std::string(reinterpret_cast<const char *>(utf8.data()), utf8.size());
}

bool has_param_sfo(const fs::path &folder) {
    std::error_code ec;
    return fs::is_regular_file(folder / kParamSfo, ec);
}

std::tm local_time(std::chrono::system_clock::time_point time) {
    const std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return tm;
}

std::chrono::system_clock::time_point to_system(fs::file_time_type time) {
    // file_clock has no portable conversion before every standard library
    // implements clock_cast; the offset between the clocks now is exact
    // enough for showing a date.
    return std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
}

// Copies the regular files of a save folder (saves are flat), keeping their
// modification times so the copy shows when the save was made.
bool copy_folder_files(const fs::path &from, const fs::path &to, std::string &error) {
    std::error_code ec;
    fs::create_directories(to, ec);
    if (ec) {
        error = "cannot create " + text(to) + ": " + ec.message();
        return false;
    }
    fs::directory_iterator it(from, ec);
    if (ec) {
        error = "cannot read " + text(from) + ": " + ec.message();
        return false;
    }
    for (; it != fs::directory_iterator(); it.increment(ec)) {
        if (ec) break;
        const fs::directory_entry &entry = *it;
        if (!entry.is_regular_file(ec)) continue;
        const fs::path target = to / entry.path().filename();
        if (!fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing, ec)) {
            error = "cannot copy " + text(entry.path().filename()) + ": " + ec.message();
            return false;
        }
        const auto time = entry.last_write_time(ec);
        if (!ec) fs::last_write_time(target, time, ec);
        ec.clear();
    }
    if (ec) {
        error = "cannot read " + text(from) + ": " + ec.message();
        return false;
    }
    return true;
}

// A path in `parent` named `name`, or `name-2`, `name-3`... when taken.
fs::path unused_path(const fs::path &parent, const std::string &name) {
    std::error_code ec;
    fs::path path = parent / name;
    for (int n = 2; fs::exists(path, ec); ++n) path = parent / (name + "-" + std::to_string(n));
    return path;
}

bool same_folder(const fs::path &a, const fs::path &b) {
    std::error_code ec;
    return fs::exists(a, ec) && fs::exists(b, ec) && fs::equivalent(a, b, ec);
}

} // namespace

bool is_game_save_name(std::string_view folder_name) {
    return std::find(std::begin(kSaveFolderNames), std::end(kSaveFolderNames), folder_name) !=
           std::end(kSaveFolderNames);
}

std::string save_label(std::string_view folder_name) {
    if (folder_name == "ULJM05800") return "Game data";
    if (folder_name == "ULJM05800QST") return "Downloaded quests";
    if (folder_name == "ULJM05800DAT") return "Install data";
    return std::string(folder_name);
}

void remember_game_key(const std::string &game_name, const Block &key) {
    if (game_name != kGameName || is_zero(key)) return;
    std::lock_guard lock(session().mutex);
    session().key = key;
}

std::optional<Block> game_key() {
    std::lock_guard lock(session().mutex);
    return session().key;
}

void set_memory_stick(const fs::path &memory_stick) {
    std::lock_guard lock(session().mutex);
    session().memory_stick = memory_stick;
}

fs::path memory_stick() {
    std::lock_guard lock(session().mutex);
    return session().memory_stick;
}

FolderSummary summarize_folder(const fs::path &folder) {
    FolderSummary summary;
    std::error_code ec;
    if (!fs::is_directory(folder, ec)) return summary;
    summary.exists = true;
    std::optional<fs::file_time_type> newest;
    for (fs::directory_iterator it(folder, ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        summary.bytes += it->file_size(ec);
        const auto time = it->last_write_time(ec);
        if (!ec && (!newest || time > *newest)) newest = time;
    }
    if (newest) summary.modified = to_system(*newest);
    return summary;
}

SaveCheck check_save_folder(const fs::path &folder, const std::optional<Block> &key) {
    SaveCheck check;
    check.folder = folder;
    check.name = text(folder.filename());
    std::error_code ec;
    const fs::path sfo_path = folder / kParamSfo;
    if (!fs::is_regular_file(sfo_path, ec)) {
        check.problem = "This folder is not a save: it has no PARAM.SFO.";
        return check;
    }
    if (fs::file_size(sfo_path, ec) > kMaxParamSfoBytes) {
        check.problem = "PARAM.SFO is damaged.";
        return check;
    }
    const auto sfo_bytes = read_file(sfo_path);
    const auto sfo = sfo_bytes ? ParamSfo::parse(*sfo_bytes) : std::nullopt;
    if (!sfo) {
        check.problem = "PARAM.SFO is damaged.";
        return check;
    }
    const auto directory = sfo->string("SAVEDATA_DIRECTORY");
    if (!directory || directory->empty()) {
        check.problem = "PARAM.SFO does not name the save.";
        return check;
    }
    check.name = *directory;
    if (!is_game_save_name(*directory)) {
        check.other_game = true;
        check.problem = "This save belongs to another game (" + *directory + ").";
        return check;
    }

    const auto *params = sfo->binary("SAVEDATA_PARAMS");
    if (params == nullptr || params->size() < kParamsSize) {
        check.problem = "PARAM.SFO is damaged: its protection is missing.";
        return check;
    }
    const std::uint8_t flags = (*params)[kParamsFlagsOffset];
    // Flag 0: a save stored without encryption, as early PPSSPP versions
    // wrote them. There is nothing to verify; the game loads it as it is.
    if (flags == 0u) return check;
    const auto mode = mode_from_flags(flags);
    if (!mode) {
        check.problem = "The save is protected in a way Yakumo does not know.";
        return check;
    }
    const auto params_offset = sfo->data_offset("SAVEDATA_PARAMS");
    if (!params_offset || !verify_param_sfo(*sfo_bytes, *params_offset)) {
        check.problem = "PARAM.SFO is damaged: its hashes do not match.";
        return check;
    }

    std::vector<std::pair<std::string, Block>> listed;
    if (const auto *list = sfo->binary("SAVEDATA_FILE_LIST"); list != nullptr) {
        for (std::size_t offset = 0; offset + kFileListEntrySize <= list->size(); offset += kFileListEntrySize) {
            std::string name;
            for (std::size_t i = 0; i < kFileListNameSize && (*list)[offset + i] != 0u; ++i)
                name.push_back(static_cast<char>((*list)[offset + i]));
            if (name.empty()) continue;
            Block hash{};
            std::copy_n(list->begin() + static_cast<std::ptrdiff_t>(offset + kFileListNameSize), hash.size(),
                        hash.begin());
            listed.emplace_back(std::move(name), hash);
        }
    }
    // The game data and the quests are written through the save-data utility,
    // which lists their file; the install data is written file by file and
    // lists none.
    if (listed.empty() && check.name != "ULJM05800DAT") {
        check.problem = "PARAM.SFO lists no data file.";
        return check;
    }
    const Block *key_pointer = key ? &*key : nullptr;
    for (const auto &[name, hash] : listed) {
        if (name.find_first_of("/\\") != std::string::npos || name == "." || name == "..") {
            check.problem = "PARAM.SFO is damaged.";
            return check;
        }
        const auto file = read_file(folder / name);
        if (!file) {
            check.problem = name + " is missing.";
            return check;
        }
        if (*mode != CryptMode::Mode1 && key_pointer == nullptr) {
            check.problem = "The save cannot be checked yet: the game has not read its own save. Try again at "
                            "the title screen.";
            return check;
        }
        if (data_file_hash(*file, *mode, key_pointer) != hash) {
            check.problem = name + " is damaged or does not belong to this game: it does not match its hash.";
            return check;
        }
        if (!decrypt_data(*file, *mode, key_pointer)) {
            check.problem = name + " is damaged: it is too short.";
            return check;
        }
    }
    return check;
}

std::vector<SaveCheck> find_saves(const fs::path &picked, const std::optional<Block> &key) {
    if (has_param_sfo(picked)) return {check_save_folder(picked, key)};
    std::vector<SaveCheck> found;
    for (const fs::path &base : {picked, picked / "SAVEDATA", picked / "PSP" / "SAVEDATA"}) {
        std::error_code ec;
        std::vector<fs::path> folders;
        for (fs::directory_iterator it(base, ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
            const std::string name = text(it->path().filename());
            if (name.empty() || name[0] == '.') continue;
            if (it->is_directory(ec) && has_param_sfo(it->path())) folders.push_back(it->path());
        }
        if (folders.empty()) continue;
        std::sort(folders.begin(), folders.end());
        for (const fs::path &folder : folders) found.push_back(check_save_folder(folder, key));
        break;
    }
    return found;
}

std::string timestamp_for_path(std::chrono::system_clock::time_point time) {
    const std::tm tm = local_time(time);
    char text[32];
    std::strftime(text, sizeof(text), "%Y-%m-%d_%H-%M-%S", &tm);
    return text;
}

std::string timestamp_for_display(std::chrono::system_clock::time_point time) {
    const std::tm tm = local_time(time);
    char text[32];
    std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M", &tm);
    return text;
}

fs::path backup_directory(const fs::path &savedata_root, std::chrono::system_clock::time_point time) {
    return unused_path(savedata_root / ".backup", timestamp_for_path(time));
}

ImportResult import_save(const SaveCheck &save, const fs::path &memory_stick, const fs::path &backup_dir) {
    ImportResult result;
    if (!save.ok()) {
        result.error = save.problem;
        return result;
    }
    const fs::path root = memory_stick / "PSP" / "SAVEDATA";
    result.destination = root / save.name;
    if (same_folder(save.folder, result.destination)) {
        result.error = "This is the save Yakumo already uses.";
        return result;
    }

    // Copy beside the destination first: a copy that fails half-way leaves
    // the save in use untouched.
    std::error_code ec;
    const fs::path staging = root / (".import-" + save.name);
    fs::remove_all(staging, ec);  // a partial copy left by an earlier failed import
    std::string error;
    if (!copy_folder_files(save.folder, staging, error)) {
        fs::remove_all(staging, ec);
        result.error = error;
        return result;
    }

    if (fs::exists(result.destination, ec)) {
        fs::create_directories(backup_dir, ec);
        const fs::path backup = backup_dir / save.name;
        if (!ec && !fs::exists(backup, ec)) fs::rename(result.destination, backup, ec);
        else if (!ec) ec = std::make_error_code(std::errc::file_exists);
        if (ec) {
            fs::remove_all(staging, ec);
            result.error = "cannot move the current save aside: " + ec.message();
            return result;
        }
        result.backup = backup;
    }
    fs::rename(staging, result.destination, ec);
    if (ec) {
        const std::string message = ec.message();
        // Put the replaced save back where it was.
        if (!result.backup.empty()) {
            std::error_code restore;
            fs::rename(result.backup, result.destination, restore);
            if (!restore) result.backup.clear();
        }
        fs::remove_all(staging, ec);
        result.error = "cannot move the imported save into place: " + message;
        return result;
    }
    result.ok = true;
    return result;
}

ExportResult export_saves(const fs::path &memory_stick, const fs::path &target,
                          std::chrono::system_clock::time_point time) {
    ExportResult result;
    const fs::path root = memory_stick / "PSP" / "SAVEDATA";
    std::vector<std::string> names;
    for (const std::string_view name : kExportedFolders)
        if (has_param_sfo(root / std::string(name))) names.emplace_back(name);
    if (names.empty()) {
        result.error = "There is no save to export yet.";
        return result;
    }
    std::error_code ec;
    if (!fs::is_directory(target, ec)) {
        result.error = "The folder " + text(target) + " does not exist.";
        return result;
    }
    result.folder = unused_path(target, "MHP3rd saves " + timestamp_for_path(time));
    const fs::path savedata = result.folder / "PSP" / "SAVEDATA";
    for (const std::string &name : names) {
        std::string error;
        if (!copy_folder_files(root / name, savedata / name, error)) {
            result.error = error;
            return result;
        }
        result.exported.push_back(name);
    }
    result.ok = true;
    return result;
}

std::vector<std::string> saves_to_back_up(const fs::path &memory_stick) {
    std::vector<std::string> names;
    const fs::path root = memory_stick / "PSP" / "SAVEDATA";
    for (const std::string_view name : kSaveFolderNames)
        if (has_param_sfo(root / std::string(name))) names.emplace_back(name);
    return names;
}

fs::path backup_folder(const fs::path &target, std::optional<std::chrono::system_clock::time_point> time) {
    return time ? unused_path(target, timestamp_for_path(*time)) : target;
}

std::vector<std::string> backup_conflicts(const fs::path &memory_stick, const fs::path &folder) {
    std::vector<std::string> taken;
    std::error_code ec;
    for (const std::string &name : saves_to_back_up(memory_stick))
        if (fs::exists(folder / name, ec)) taken.push_back(name);
    return taken;
}

BackupResult back_up_saves(const fs::path &memory_stick, const fs::path &folder, bool replace) {
    BackupResult result;
    result.folder = folder;
    const std::vector<std::string> names = saves_to_back_up(memory_stick);
    if (names.empty()) {
        result.error = "There is no save to back up yet.";
        return result;
    }
    if (!replace && !backup_conflicts(memory_stick, folder).empty()) {
        result.error = "An earlier backup is in the way.";
        return result;
    }
    const fs::path root = memory_stick / "PSP" / "SAVEDATA";
    for (const std::string &name : names) {
        std::error_code ec;
        const fs::path destination = folder / name;
        if (same_folder(root / name, destination)) {
            result.error = "The backup would replace the save itself.";
            return result;
        }
        // Copy beside the destination, then swap: a failed copy leaves an
        // earlier backup as it was.
        const fs::path partial = folder / ("." + name + ".partial");
        fs::remove_all(partial, ec);
        std::string error;
        if (!copy_folder_files(root / name, partial, error)) {
            fs::remove_all(partial, ec);
            result.error = error;
            return result;
        }
        if (fs::exists(destination, ec)) fs::remove_all(destination, ec);
        if (!ec) fs::rename(partial, destination, ec);
        if (ec) {
            result.error = "cannot write " + text(destination) + ": " + ec.message();
            return result;
        }
        result.saved.push_back(name);
    }
    result.ok = true;
    return result;
}

} // namespace mhp3rd::savedata
