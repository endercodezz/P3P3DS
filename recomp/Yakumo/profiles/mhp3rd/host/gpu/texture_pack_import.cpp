#include "texture_pack_import.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <set>
#include <system_error>
#include <utility>

namespace mhp3rd::gpu {
namespace {

namespace fs = std::filesystem;

std::string lower(std::string text) {
    for (char &c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

std::string utf8(const fs::path &path) {
    const std::u8string text = path.u8string();
    return {text.begin(), text.end()};
}

std::string name_of(const fs::path &path) { return utf8(path.filename()); }

bool is_folder(const fs::path &path) {
    std::error_code ec;
    return fs::is_directory(path, ec);
}

bool is_file(const fs::path &path) {
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

// ULJM05800, NPJB40001: four letters and five digits.
bool looks_like_game_id(const std::string &name) {
    if (name.size() != 9u) return false;
    for (std::size_t i = 0; i < 4u; ++i)
        if (std::isalpha(static_cast<unsigned char>(name[i])) == 0) return false;
    for (std::size_t i = 4u; i < 9u; ++i)
        if (std::isdigit(static_cast<unsigned char>(name[i])) == 0) return false;
    return true;
}

// The folder `name` in `parent`, whatever the case of either spelling; empty
// when there is none.
fs::path child(const fs::path &parent, const std::string &name) {
    if (parent.empty()) return {};
    // Listed rather than probed, so the path keeps the spelling on disk even
    // where the file system ignores case.
    const std::string wanted = lower(name);
    fs::path found;
    std::error_code ec;
    for (fs::directory_iterator it(parent, fs::directory_options::skip_permission_denied, ec);
         !ec && it != fs::directory_iterator(); it.increment(ec)) {
        const std::string entry = name_of(it->path());
        if (lower(entry) != wanted || !is_folder(it->path())) continue;
        if (entry == name) return it->path();
        if (found.empty()) found = it->path();
    }
    return found;
}

bool holds_pack(const fs::path &folder) {
    return !folder.empty() && (is_file(folder / "textures.ini") || is_file(folder / "textures.zip"));
}

bool lists(const std::vector<std::string> &games, const std::string &game_id) {
    const std::string wanted = lower(game_id);
    return std::any_of(games.begin(), games.end(), [&](const std::string &game) { return lower(game) == wanted; });
}

// Files a copy leaves out: the file managers' own (.DS_Store, Thumbs.db,
// desktop.ini) and everything hidden.
bool skipped(const std::string &name) {
    const std::string l = lower(name);
    return name.empty() || name[0] == '.' || l == "thumbs.db" || l == "desktop.ini";
}

bool is_image(const fs::path &path) {
    const std::string extension = lower(utf8(path.extension()));
    return extension == ".png" || extension == ".dds" || extension == ".ktx2" || extension == ".zim" ||
           extension == ".jpg" || extension == ".jpeg";
}

struct PackFile {
    fs::path path;
    std::string relative;  // generic form
    std::uint64_t size{};
};

// Every file a copy takes, hidden ones left out, sorted.
std::vector<PackFile> list_files(const fs::path &folder, std::string &error) {
    std::vector<PackFile> files;
    std::error_code ec;
    fs::recursive_directory_iterator it(folder, fs::directory_options::skip_permission_denied, ec);
    for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        const fs::directory_entry &entry = *it;
        if (skipped(name_of(entry.path()))) {
            if (entry.is_directory(ec)) it.disable_recursion_pending();
            ec.clear();
            continue;
        }
        std::error_code inner;
        if (!entry.is_regular_file(inner)) continue;
        PackFile file;
        file.path = entry.path();
        const std::u8string relative = fs::relative(entry.path(), folder, inner).generic_u8string();
        file.relative.assign(relative.begin(), relative.end());
        file.size = entry.file_size(inner);
        files.push_back(std::move(file));
    }
    if (ec) error = "cannot read " + utf8(folder) + ": " + ec.message();
    std::sort(files.begin(), files.end(), [](const PackFile &a, const PackFile &b) { return a.relative < b.relative; });
    return files;
}

std::string timestamp_for_path(std::chrono::system_clock::time_point time) {
    const std::time_t t = std::chrono::system_clock::to_time_t(time);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char text[32];
    std::strftime(text, sizeof(text), "%Y-%m-%d_%H-%M-%S", &tm);
    return text;
}

fs::path unused_path(const fs::path &parent, const std::string &name) {
    std::error_code ec;
    fs::path path = parent / name;
    for (int n = 2; fs::exists(path, ec); ++n) path = parent / (name + "-" + std::to_string(n));
    return path;
}

constexpr const char *kStagingPrefix = ".incomplete-";

// Finds the pack folder in `chosen`, setting `layout` (and, for a pack named
// for another release, `made_for`).
fs::path find_pack(const fs::path &chosen, const std::string &game_id, std::string &layout, std::string &made_for) {
    if (holds_pack(chosen)) {
        layout = "The folder holds textures.ini.";
        return chosen;
    }
    const fs::path textures = child(chosen, "textures");
    const fs::path psp_textures = child(child(chosen, "PSP"), "TEXTURES");
    struct Place {
        fs::path folder;
        const char *layout;
    };
    const Place exact[] = {
        {child(textures, game_id), "Found in its textures folder."},
        {child(chosen, game_id), "Found in the folder."},
        {child(psp_textures, game_id), "Found in PPSSPP's PSP/TEXTURES folder."},
    };
    for (const Place &place : exact) {
        if (holds_pack(place.folder)) {
            layout = place.layout;
            return place.folder;
        }
    }
    // A pack under another name whose textures.ini says it covers this game.
    for (const fs::path &parent : {chosen, textures, psp_textures}) {
        if (parent.empty()) continue;
        std::vector<fs::path> candidates;
        std::error_code ec;
        for (fs::directory_iterator it(parent, fs::directory_options::skip_permission_denied, ec);
             !ec && it != fs::directory_iterator(); it.increment(ec)) {
            if (is_folder(it->path()) && is_file(it->path() / "textures.ini")) candidates.push_back(it->path());
        }
        std::sort(candidates.begin(), candidates.end());
        for (const fs::path &candidate : candidates) {
            if (!lists(texture_pack_games(candidate / "textures.ini"), game_id)) continue;
            layout = "Found as " + name_of(candidate) + ", whose textures.ini lists " + game_id + ".";
            if (looks_like_game_id(name_of(candidate))) made_for = name_of(candidate);
            return candidate;
        }
    }
    return {};
}

std::string friendly(const std::string &error) {
    if (lower(error).find("hash = quick") != std::string::npos)
        return "The pack uses the old \"quick\" hash, which Yakumo does not support. Only packs that use xxh64 or "
               "xxh32 work.";
    if (error.find("zipped") != std::string::npos)
        return "The pack is zipped (textures.zip). Unpack it first, then choose the unpacked folder.";
    return "textures.ini cannot be used: " + error;
}

} // namespace

TexturePackLocation texture_pack_location(const fs::path &textures_root, const std::string &game_id,
                                          const std::string &in_place) {
    if (const char *variable = std::getenv("MHP3RD_TEXTURE_PACK"); variable != nullptr) {
        const std::string value = variable;
        const std::string l = lower(value);
        if (!value.empty() && l != "0" && l != "1" && l != "on" && l != "off" && l != "yes" && l != "no" &&
            l != "true" && l != "false")
            return {fs::path(std::u8string(value.begin(), value.end())), TexturePackLocation::Source::Variable};
    }
    if (!in_place.empty())
        return {fs::path(std::u8string(in_place.begin(), in_place.end())), TexturePackLocation::Source::InPlace};
    return {textures_root / game_id, TexturePackLocation::Source::Installed};
}

TexturePackCheck check_texture_pack(const fs::path &chosen, const std::string &game_id) {
    TexturePackCheck check;
    check.chosen = chosen;
    check.folder = find_pack(chosen, game_id, check.layout, check.made_for);
    if (check.folder.empty()) {
        bool zipped = false;
        std::error_code ec;
        for (fs::directory_iterator it(chosen, fs::directory_options::skip_permission_denied, ec);
             !ec && it != fs::directory_iterator(); it.increment(ec))
            zipped = zipped || lower(utf8(it->path().extension())) == ".zip";
        check.problem = "No texture pack for " + game_id +
                        " was found here. Choose the folder that holds textures.ini, or one that holds it in "
                        "textures/" + game_id + ", " + game_id + " or PSP/TEXTURES/" + game_id + ".";
        if (zipped) check.problem += " A .zip file is not read: unpack it first.";
        return check;
    }

    // A folder named for another release is only taken when it says it
    // covers this game too.
    const std::string folder_name = name_of(check.folder);
    const std::vector<std::string> games = texture_pack_games(check.folder / "textures.ini");
    if (lower(folder_name) != lower(game_id) && !lists(games, game_id)) {
        if (looks_like_game_id(folder_name)) {
            check.problem = "This pack is for " + folder_name + ", and its textures.ini does not list " + game_id +
                            " under [games], so its textures would not match this release.";
            return check;
        }
        if (!games.empty()) {
            check.problem = "This pack's textures.ini lists other games under [games] (" + games.front() +
                            (games.size() > 1u ? ", …" : "") + ") but not " + game_id + ".";
            return check;
        }
    } else if (lower(folder_name) != lower(game_id) && looks_like_game_id(folder_name)) {
        check.made_for = folder_name;
    }

    TexturePackInfo info;
    std::string error;
    if (!TexturePack::inspect(check.folder, game_id, info, error)) {
        check.problem = friendly(error);
        return check;
    }
    check.hash = info.options.hash;
    check.ignore_address = info.options.ignore_address;
    check.keys = info.keys;

    const std::vector<PackFile> files = list_files(check.folder, error);
    if (!error.empty()) {
        check.problem = "The folder cannot be read completely: " + error;
        return check;
    }
    std::set<std::string> present;
    for (const PackFile &file : files) {
        ++check.files;
        check.bytes += file.size;
        if (is_image(file.path)) ++check.images;
        present.insert(lower(file.relative));
    }
    for (const std::string &name : info.files) {
        if (present.count(lower(name)) != 0u) continue;
        ++check.missing;
        if (check.missing_names.size() < 3u) check.missing_names.push_back(name);
    }
    return check;
}

InstalledTexturePack summarize_texture_pack(const fs::path &folder, const std::string &game_id) {
    InstalledTexturePack installed;
    if (folder.empty() || !is_folder(folder)) return installed;
    installed.exists = true;
    std::string error;
    for (const PackFile &file : list_files(folder, error)) {
        ++installed.files;
        installed.bytes += file.size;
    }
    TexturePackInfo info;
    if (TexturePack::inspect(folder, game_id, info, error)) installed.keys = info.keys;
    else installed.problem = error;
    return installed;
}

std::optional<std::uint64_t> texture_pack_free_space(const fs::path &folder) {
    std::error_code ec;
    fs::path probe = fs::absolute(folder, ec);
    while (!probe.empty() && !fs::exists(probe, ec)) {
        if (probe == probe.parent_path()) break;
        probe = probe.parent_path();
    }
    const fs::space_info space = fs::space(probe, ec);
    if (ec) return std::nullopt;
    return space.available;
}

fs::path texture_pack_backup_directory(const fs::path &textures_root, std::chrono::system_clock::time_point time) {
    return unused_path(textures_root / ".backup", timestamp_for_path(time));
}

TexturePackCopy::~TexturePackCopy() {
    cancel();
    join();
}

void TexturePackCopy::start(const TexturePackCheck &check, const fs::path &textures_root) {
    join();
    cancel_ = false;
    {
        std::lock_guard lock(mutex_);
        progress_ = {};
        progress_.total_bytes = check.bytes;
        progress_.total_files = check.files;
        error_.clear();
    }
    // Copies that never finished (the game was closed or crashed during one)
    // are this code's own folders; nothing else is named like them.
    std::error_code ec;
    for (fs::directory_iterator it(textures_root, ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
        if (name_of(it->path()).rfind(kStagingPrefix, 0) != 0 || !is_folder(it->path())) continue;
        std::error_code remove_error;
        fs::remove_all(it->path(), remove_error);
        std::cout << "[texpack] removed an unfinished copy, " << utf8(it->path()) << "\n";
    }
    staging_ = unused_path(textures_root, kStagingPrefix + timestamp_for_path(std::chrono::system_clock::now()));
    state_ = State::Copying;
    thread_ = std::thread([this, source = check.folder] { run(source); });
}

void TexturePackCopy::join() {
    if (thread_.joinable()) thread_.join();
}

TexturePackCopy::Progress TexturePackCopy::progress() const {
    std::lock_guard lock(mutex_);
    return progress_;
}

std::string TexturePackCopy::error() const {
    std::lock_guard lock(mutex_);
    return error_;
}

void TexturePackCopy::run(fs::path source) {
    std::string error;
    const std::vector<PackFile> files = list_files(source, error);
    std::uint64_t total = 0u;
    for (const PackFile &file : files) total += file.size;
    {
        std::lock_guard lock(mutex_);
        progress_.total_bytes = total;
        progress_.total_files = files.size();
    }
    std::error_code ec;
    if (error.empty()) {
        fs::create_directories(staging_, ec);
        if (ec) error = "cannot create " + utf8(staging_) + ": " + ec.message();
    }
    std::uint64_t done = 0u;
    for (std::size_t i = 0; error.empty() && i < files.size(); ++i) {
        if (cancel_) break;
        const PackFile &file = files[i];
        {
            std::lock_guard lock(mutex_);
            progress_.current = file.relative;
        }
        const fs::path target = staging_ / fs::path(std::u8string(file.relative.begin(), file.relative.end()));
        fs::create_directories(target.parent_path(), ec);
        if (!ec) fs::copy_file(file.path, target, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            error = "cannot copy " + file.relative + ": " + ec.message();
            break;
        }
        done += file.size;
        std::lock_guard lock(mutex_);
        progress_.bytes = done;
        progress_.files = i + 1u;
    }
    if (error.empty() && !cancel_) {
        std::cout << "[texpack] copied " << files.size() << " files (" << (done >> 20u) << " MB) to "
                  << utf8(staging_) << std::endl;
        state_ = State::Done;
        return;
    }
    discard_staged_texture_pack(staging_);
    if (!error.empty()) {
        std::cout << "[texpack] copy failed: " << error << std::endl;
        std::lock_guard lock(mutex_);
        error_ = error;
        state_ = State::Failed;
    } else {
        std::cout << "[texpack] copy cancelled" << std::endl;
        state_ = State::Cancelled;
    }
}

bool install_staged_texture_pack(const fs::path &staging, const fs::path &textures_root, const std::string &game_id,
                                 const fs::path &backup_dir, fs::path &backup, std::string &error) {
    backup.clear();
    const fs::path destination = textures_root / game_id;
    std::error_code ec;
    if (fs::exists(fs::symlink_status(destination, ec))) {
        fs::create_directories(backup_dir, ec);
        const fs::path moved = backup_dir / game_id;
        if (!ec) fs::rename(destination, moved, ec);
        if (ec) {
            error = "cannot move the installed pack to " + utf8(moved) + ": " + ec.message();
            return false;
        }
        backup = moved;
    }
    fs::rename(staging, destination, ec);
    if (ec) {
        error = "cannot put the new pack in place: " + ec.message();
        if (!backup.empty()) {
            std::error_code restore;
            fs::rename(backup, destination, restore);
            if (!restore) backup.clear();
        }
        return false;
    }
    return true;
}

void discard_staged_texture_pack(const fs::path &staging) {
    if (name_of(staging).rfind(kStagingPrefix, 0) != 0) return;
    std::error_code ec;
    fs::remove_all(staging, ec);
}

} // namespace mhp3rd::gpu
