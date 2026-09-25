#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Reading the small INI files mods describe themselves with, and the list
// values in them. Part of the game-independent mod machinery (host/mods/),
// which knows nothing about any one game's archive or mod format.
namespace mhp3rd::mods {

// [section] headers and key=value lines. A value may be in double quotes; a
// quote left open runs to the end of the line, as hand-written files have it.
// Keys and section names compare without regard to case. Lines starting with
// ';' or '#' are comments, blank lines are skipped, and a UTF-8 byte order
// mark and CR line ends are accepted. Lines before the first header belong to
// the section named "".
class IniFile {
public:
    using Entries = std::vector<std::pair<std::string, std::string>>;

    [[nodiscard]] static IniFile parse(std::string_view text);
    // The file's contents, or nothing when it cannot be read.
    [[nodiscard]] static std::optional<IniFile> load(const std::filesystem::path &path);

    [[nodiscard]] bool has_section(std::string_view section) const;
    // The value, or null. With a key given twice, the last one counts.
    [[nodiscard]] const std::string *find(std::string_view section, std::string_view key) const;
    [[nodiscard]] std::string get(std::string_view section, std::string_view key,
                                  std::string_view fallback = {}) const;
    // Section names in file order, as written.
    [[nodiscard]] std::vector<std::string> sections() const;
    // The section's lines in file order; empty when there is no such section.
    [[nodiscard]] const Entries &entries(std::string_view section) const;

private:
    struct Section {
        std::string name;
        Entries entries;
    };
    [[nodiscard]] const Section *section(std::string_view name) const;

    std::vector<Section> sections_;
};

// "a;b; c;;" -> {"a", "b", "c"}: split at `separator`, trimmed, empty parts dropped.
[[nodiscard]] std::vector<std::string> split_list(std::string_view value, char separator = ';');

// ASCII-only case folding and comparison, for keys and file names.
[[nodiscard]] std::string lower(std::string_view text);
[[nodiscard]] bool iequals(std::string_view a, std::string_view b);
[[nodiscard]] std::string trim(std::string_view text);

// Paths as UTF-8, the way mod.ini and mods.ini spell them, on every system
// (path::string() uses the ANSI code page on Windows).
[[nodiscard]] std::string to_utf8(const std::filesystem::path &path);
[[nodiscard]] std::filesystem::path from_utf8(std::string_view text);

} // namespace mhp3rd::mods
