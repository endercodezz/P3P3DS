#include "mods/mod_library.hpp"

#include "mods/mod_ini.hpp"

#include <algorithm>
#include <fstream>
#include <set>
#include <system_error>

namespace mhp3rd::mods {

namespace fs = std::filesystem;

namespace {

constexpr const char *kGeneral = "general";
constexpr const char *kModPrefix = "mod ";

} // namespace

void ModLibrary::scan(const fs::path &folder) {
    mods_.clear();
    std::error_code ec;
    fs::directory_iterator it(folder, ec);
    if (ec) return;
    std::vector<fs::path> folders;
    for (const fs::directory_entry &entry : it) {
        const std::string name = to_utf8(entry.path().filename());
        if (name.empty() || name.front() == '.') continue;  // .backup and hidden folders
        if (entry.is_directory(ec)) folders.push_back(entry.path());
    }
    std::sort(folders.begin(), folders.end());
    int top = 0;
    for (const auto &[id, c] : choices_) top = std::max(top, c.rank);
    for (const fs::path &path : folders) {
        std::optional<Mod> mod = format_->read(path);
        if (!mod) continue;
        mod->id = to_utf8(path.filename());
        mod->folder = path;
        // A mod seen for the first time goes above the others: the one added
        // last wins, as players expect.
        if (!choices_.contains(mod->id)) choices_[mod->id].rank = ++top;
        ModChoice &c = choices_[mod->id];
        c.slots.resize(mod->slots.size());
        if (!mod->unusable.empty()) c.enabled = false;
        mods_.push_back(std::move(*mod));
    }
    sort();
}

void ModLibrary::sort() {
    std::stable_sort(mods_.begin(), mods_.end(), [this](const Mod &a, const Mod &b) {
        const int ra = choices_.at(a.id).rank;
        const int rb = choices_.at(b.id).rank;
        if (ra != rb) return ra > rb;
        return a.id < b.id;
    });
}

void ModLibrary::load_choices(const fs::path &file) {
    choices_.clear();
    master_ = true;
    const std::optional<IniFile> ini = IniFile::load(file);
    if (!ini) return;
    master_ = ini->get(kGeneral, "enabled", "1") != "0";
    for (const std::string &section : ini->sections()) {
        if (!iequals(std::string_view(section).substr(0, 4), kModPrefix)) continue;
        const std::string id = section.substr(4);
        ModChoice c;
        c.enabled = ini->get(section, "enabled") == "1";
        try {
            c.rank = std::stoi(ini->get(section, "rank", "0"));
        } catch (const std::exception &) {
            c.rank = 0;
        }
        for (std::size_t slot = 0;; ++slot) {
            const std::string *value = ini->find(section, "slot" + std::to_string(slot + 1u));
            if (value == nullptr) break;
            c.slots.push_back(format_->parse_file(*value));
        }
        choices_[id] = std::move(c);
    }
}

bool ModLibrary::save_choices(const fs::path &file, std::string &error) const {
    std::error_code ec;
    fs::create_directories(file.parent_path(), ec);
    fs::path partial = file;
    partial += ".part";
    {
        std::ofstream out(partial, std::ios::trunc);
        out << "# Written by Yakumo's Mods menu: which mods are on, and their order (a higher rank wins where\n"
               "# two mods change the same file). The mods themselves are folders in the mods folder.\n"
            << "[" << kGeneral << "]\nenabled=" << (master_ ? 1 : 0) << "\n";
        for (const auto &[id, c] : choices_) {
            out << "\n[" << kModPrefix << id << "]\nenabled=" << (c.enabled ? 1 : 0) << "\nrank=" << c.rank << "\n";
            for (std::size_t slot = 0; slot < c.slots.size(); ++slot)
                out << "slot" << slot + 1u << "=" << (c.slots[slot] ? format_->file_name(*c.slots[slot]) : "")
                    << "\n";
        }
        if (!out) {
            error = "cannot write " + to_utf8(partial);
            return false;
        }
    }
    fs::rename(partial, file, ec);
    if (ec) {
        error = "cannot replace " + to_utf8(file) + ": " + ec.message();
        return false;
    }
    return true;
}

const Mod *ModLibrary::find(const std::string &id) const {
    for (const Mod &mod : mods_) {
        if (mod.id == id) return &mod;
    }
    return nullptr;
}

ModChoice ModLibrary::choice(const std::string &id) const {
    const auto found = choices_.find(id);
    return found != choices_.end() ? found->second : ModChoice{};
}

ModChoice &ModLibrary::choice_for(const std::string &id) { return choices_[id]; }

void ModLibrary::set_enabled(const std::string &id, bool on) {
    std::set<std::string> visited;
    // Dependencies and pack members may name each other; each mod once.
    const auto apply = [&](const auto &self, const std::string &target, bool value) -> void {
        if (!visited.insert(target).second) return;
        const Mod *mod = find(target);
        if (mod == nullptr) return;
        if (value && !mod->unusable.empty()) return;
        choice_for(target).enabled = value;
        for (const std::string &member : mod->members) self(self, member, value);
        if (value) {
            for (const std::string &dependency : mod->depends) self(self, dependency, true);
        }
    };
    apply(apply, id, on);
}

void ModLibrary::set_slot(const std::string &id, std::size_t slot, std::optional<FileId> file) {
    ModChoice &c = choice_for(id);
    if (c.slots.size() <= slot) c.slots.resize(slot + 1u);
    c.slots[slot] = file;
}

void ModLibrary::move(const std::string &id, int delta) {
    const auto it = std::find_if(mods_.begin(), mods_.end(), [&id](const Mod &m) { return m.id == id; });
    if (it == mods_.end()) return;
    const auto index = static_cast<std::ptrdiff_t>(it - mods_.begin());
    // The list is highest first: raising a mod swaps it with the one above.
    const std::ptrdiff_t other = index - delta;
    if (other < 0 || other >= static_cast<std::ptrdiff_t>(mods_.size())) return;
    ModChoice &a = choice_for(id);
    ModChoice &b = choice_for(mods_[static_cast<std::size_t>(other)].id);
    if (a.rank == b.rank) a.rank += delta;
    else std::swap(a.rank, b.rank);
    sort();
}

Resolution ModLibrary::resolve() const {
    Resolution result;
    if (!master_) return result;
    struct Touch {
        std::vector<std::string> replaced;  // lowest priority first
        std::vector<std::string> patched;
    };
    std::map<FileId, Touch> touches;
    // Lowest priority first, so a later mod's replacement wins and its patch
    // is applied after (over) the earlier ones.
    for (auto it = mods_.rbegin(); it != mods_.rend(); ++it) {
        const Mod &mod = *it;
        const ModChoice c = choice(mod.id);
        if (!c.enabled || !mod.unusable.empty()) continue;
        std::vector<FileChange> changes = mod.changes;
        for (std::size_t slot = 0; slot < mod.slots.size() && slot < c.slots.size(); ++slot) {
            if (c.slots[slot]) changes.push_back({FileChange::Kind::Replace, *c.slots[slot], mod.slots[slot].source});
        }
        for (const FileChange &change : changes) {
            Touch &touch = touches[change.file];
            if (change.kind == FileChange::Kind::Replace) {
                result.replacements[change.file] = {mod.id, change.source};
                std::erase(touch.replaced, mod.id);
                touch.replaced.push_back(mod.id);
            } else {
                result.patches[change.file].push_back({mod.id, change.source});
                touch.patched.push_back(mod.id);
            }
        }
    }
    for (const auto &[file, touch] : touches) {
        const bool several = touch.replaced.size() > 1u;
        bool foreign_patch = false;
        for (const std::string &patcher : touch.patched)
            foreign_patch = foreign_patch || (!touch.replaced.empty() && patcher != touch.replaced.back());
        if (!several && !foreign_patch) continue;
        Resolution::Conflict conflict;
        conflict.file = file;
        if (!touch.replaced.empty()) conflict.winner = touch.replaced.back();
        for (auto r = touch.replaced.rbegin(); r != touch.replaced.rend(); ++r) {
            if (*r != conflict.winner) conflict.overridden.push_back(*r);
        }
        conflict.patched_by = touch.patched;
        result.conflicts.push_back(std::move(conflict));
    }
    return result;
}

} // namespace mhp3rd::mods
