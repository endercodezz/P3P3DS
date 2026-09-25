#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

// The mods folder, what each mod in it changes, which ones the player has
// turned on and in what order, and what that adds up to: for each game file,
// the replacement that wins and the patches that apply.
//
// Game-independent: a game says how a mod folder describes itself and how its
// files are numbered (ModFormat); nothing here knows an archive or a format.
namespace mhp3rd::mods {

// A file of the game's own, as the game's mods address it (for this game, an
// entry of DATA.BIN).
using FileId = std::uint32_t;

// One thing a mod does to one game file.
struct FileChange {
    enum class Kind { Replace, Patch };
    Kind kind{Kind::Replace};
    FileId file{};
    std::filesystem::path source;  // the mod's own file
};

// A file of a mod whose target the player chooses, such as a model that can
// stand in for any one piece of equipment.
struct Slot {
    std::string label;  // "Head", "Weapon"
    std::filesystem::path source;
};

struct Mod {
    std::string id;  // the folder's name; the key its choices are saved under
    std::filesystem::path folder;
    std::string name;
    std::string author;
    std::string type;         // as shown: "Files", "Patch", "Pack", ...
    std::string description;  // may hold line breaks
    std::string version;      // what the mod says it was made for, as shown
    std::filesystem::path preview;  // an image, when the mod has one
    std::vector<FileChange> changes;
    std::vector<Slot> slots;
    std::vector<std::string> members;  // a pack: the mods it turns on and off
    std::vector<std::string> depends;  // mods it needs, turned on with it
    std::vector<std::string> notes;    // parts that do nothing here, and why
    std::string unusable;              // why it cannot be turned on; empty when it can
};

// What a game supplies: how a folder describes a mod, and how files are named.
class ModFormat {
public:
    virtual ~ModFormat() = default;
    // The mod in `folder`, or nothing when the folder is not one.
    [[nodiscard]] virtual std::optional<Mod> read(const std::filesystem::path &folder) const = 0;
    // "0601": how the game's mods spell a file.
    [[nodiscard]] virtual std::string file_name(FileId file) const = 0;
    [[nodiscard]] virtual std::optional<FileId> parse_file(const std::string &text) const = 0;
};

// The player's choices for one mod.
struct ModChoice {
    bool enabled{};
    int rank{};  // higher wins where two mods change the same file
    std::vector<std::optional<FileId>> slots;  // per Mod::slots: the file each replaces
};

// What the enabled mods add up to.
struct Resolution {
    struct Source {
        std::string mod;  // Mod::id
        std::filesystem::path path;
        bool operator==(const Source &) const = default;
    };
    struct Conflict {
        FileId file{};
        std::string winner;                  // the replacement used; empty when only patches
        std::vector<std::string> overridden;  // replacements that lost, highest first
        std::vector<std::string> patched_by;  // patches applied to it, in order
    };
    std::map<FileId, Source> replacements;         // the winning replacement per file
    std::map<FileId, std::vector<Source>> patches;  // applied in this order: the last wins
    std::vector<Conflict> conflicts;

    [[nodiscard]] bool empty() const { return replacements.empty() && patches.empty(); }
    // Same files from the same sources; conflicts are not compared.
    [[nodiscard]] bool same_files(const Resolution &other) const {
        return replacements == other.replacements && patches == other.patches;
    }
};

class ModLibrary {
public:
    explicit ModLibrary(const ModFormat &format) : format_(&format) {}

    // Reads every folder in `folder` (not recursively). A folder that is not a
    // mod is skipped. Choices of mods that are gone are kept, not shown.
    void scan(const std::filesystem::path &folder);
    // The saved choices. A missing file means no mod is on.
    void load_choices(const std::filesystem::path &file);
    bool save_choices(const std::filesystem::path &file, std::string &error) const;

    // Highest priority first.
    [[nodiscard]] const std::vector<Mod> &mods() const noexcept { return mods_; }
    [[nodiscard]] const Mod *find(const std::string &id) const;
    [[nodiscard]] ModChoice choice(const std::string &id) const;
    [[nodiscard]] bool enabled(const std::string &id) const { return choice(id).enabled; }

    // On also turns on the mods it depends on; a pack turns its members on
    // and off with it. A mod that cannot be used stays off.
    void set_enabled(const std::string &id, bool on);
    void set_slot(const std::string &id, std::size_t slot, std::optional<FileId> file);
    // +1 raises the mod's priority by one place, -1 lowers it.
    void move(const std::string &id, int delta);

    // The switch for all mods at once.
    [[nodiscard]] bool master() const noexcept { return master_; }
    void set_master(bool on) noexcept { master_ = on; }

    [[nodiscard]] Resolution resolve() const;
    [[nodiscard]] const ModFormat &format() const noexcept { return *format_; }

private:
    void sort();
    ModChoice &choice_for(const std::string &id);

    const ModFormat *format_;
    std::vector<Mod> mods_;
    std::map<std::string, ModChoice> choices_;
    bool master_{true};
};

} // namespace mhp3rd::mods
