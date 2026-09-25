#include "mods/mod_import.hpp"

#include "mods/mod_ini.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <system_error>

namespace mhp3rd::mods {

namespace fs = std::filesystem;

namespace {

std::vector<fs::path> subfolders(const fs::path &folder) {
    std::vector<fs::path> result;
    std::error_code ec;
    for (fs::directory_iterator it(folder, ec), end; !ec && it != end; it.increment(ec)) {
        const std::string name = to_utf8(it->path().filename());
        if (!name.empty() && name.front() != '.' && it->is_directory(ec)) result.push_back(it->path());
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool inside(const fs::path &path, const fs::path &folder) {
    std::error_code ec;
    const fs::path a = fs::weakly_canonical(path, ec);
    const fs::path b = fs::weakly_canonical(folder, ec);
    if (a.empty() || b.empty()) return false;
    auto ai = a.begin();
    for (auto bi = b.begin(); bi != b.end(); ++bi, ++ai) {
        if (bi->empty()) continue;
        if (ai == a.end() || *ai != *bi) return false;
    }
    return true;
}

std::string timestamp() {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char text[32];
    std::strftime(text, sizeof(text), "%Y%m%d-%H%M%S", &tm);
    return text;
}

} // namespace

ImportCheck check_import(const fs::path &chosen, const ModFormat &format, const fs::path &mods_folder) {
    ImportCheck check;
    check.chosen = chosen;
    std::error_code ec;
    if (!fs::is_directory(chosen, ec)) {
        check.problem = "This is not a folder.";
        return check;
    }
    if (inside(chosen, mods_folder)) {
        check.problem = "This folder is in the mods folder already.";
        return check;
    }
    const auto add = [&](const fs::path &folder) {
        std::optional<Mod> mod = format.read(folder);
        if (!mod) return false;
        ImportCandidate candidate;
        candidate.folder = folder;
        candidate.id = to_utf8(folder.filename());
        candidate.mod = std::move(*mod);
        candidate.replaces = fs::exists(mods_folder / from_utf8(candidate.id), ec);
        check.mods.push_back(std::move(candidate));
        return true;
    };
    if (add(chosen)) return check;
    for (const fs::path &folder : subfolders(chosen)) add(folder);
    if (check.mods.empty()) {
        for (const fs::path &folder : subfolders(chosen))
            for (const fs::path &inner : subfolders(folder)) add(inner);
    }
    if (check.mods.empty())
        check.problem = "No mod here: a mod is a folder with a mod.ini, or with files named by the file id they "
                        "replace. Unpack a downloaded archive first.";
    return check;
}

ImportResult import_mods(const ImportCheck &check, const fs::path &mods_folder) {
    ImportResult result;
    std::error_code ec;
    fs::create_directories(mods_folder, ec);
    if (ec) {
        result.error = "Cannot create " + to_utf8(mods_folder) + ": " + ec.message();
        return result;
    }
    const std::string stamp = timestamp();
    for (const ImportCandidate &candidate : check.mods) {
        const fs::path target = mods_folder / from_utf8(candidate.id);
        if (fs::exists(target, ec)) {
            const fs::path backup = mods_folder / ".backup" / from_utf8(candidate.id + "-" + stamp);
            fs::create_directories(backup.parent_path(), ec);
            fs::rename(target, backup, ec);
            if (ec) {
                if (result.error.empty())
                    result.error = "Cannot move the installed " + candidate.id + " aside: " + ec.message();
                continue;
            }
            result.backups.push_back(backup);
        }
        // Into a temporary name first, so a failed copy leaves no half mod.
        const fs::path partial = mods_folder / from_utf8(".import-" + candidate.id);
        fs::remove_all(partial, ec);
        fs::copy(candidate.folder, partial, fs::copy_options::recursive, ec);
        if (!ec) fs::rename(partial, target, ec);
        if (ec) {
            if (result.error.empty()) result.error = "Cannot copy " + candidate.id + ": " + ec.message();
            fs::remove_all(partial, ec);
            continue;
        }
        result.imported.push_back(candidate.id);
    }
    return result;
}

} // namespace mhp3rd::mods
