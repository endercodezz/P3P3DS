#include "mods/mod_ini.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>

namespace mhp3rd::mods {

std::string lower(std::string_view text) {
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
    return out;
}

bool iequals(std::string_view a, std::string_view b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](unsigned char x, unsigned char y) {
               return std::tolower(x) == std::tolower(y);
           });
}

std::string trim(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return std::string(text.substr(first, last - first + 1u));
}

std::string to_utf8(const std::filesystem::path &path) {
    const std::u8string text = path.u8string();
    return {text.begin(), text.end()};
}

std::filesystem::path from_utf8(std::string_view text) { return std::u8string(text.begin(), text.end()); }

std::vector<std::string> split_list(std::string_view value, char separator) {
    std::vector<std::string> parts;
    std::size_t start = 0u;
    while (start <= value.size()) {
        const auto end = value.find(separator, start);
        std::string part = trim(value.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start));
        if (!part.empty()) parts.push_back(std::move(part));
        if (end == std::string_view::npos) break;
        start = end + 1u;
    }
    return parts;
}

namespace {

// `"text"` -> text; `"text` -> text; text -> text.
std::string unquote(std::string_view raw) {
    std::string value = trim(raw);
    if (value.empty() || value.front() != '"') return value;
    const auto close = value.find('"', 1u);
    if (close == std::string::npos) return trim(std::string_view(value).substr(1u));
    return value.substr(1u, close - 1u);
}

} // namespace

IniFile IniFile::parse(std::string_view text) {
    IniFile file;
    if (text.starts_with("\xEF\xBB\xBF")) text.remove_prefix(3u);
    Section *current = nullptr;
    const auto open_section = [&file](std::string name) -> Section * {
        for (Section &existing : file.sections_) {
            if (iequals(existing.name, name)) return &existing;
        }
        file.sections_.push_back(Section{std::move(name), {}});
        return &file.sections_.back();
    };
    std::size_t start = 0u;
    while (start < text.size()) {
        auto end = text.find('\n', start);
        if (end == std::string_view::npos) end = text.size();
        const std::string line = trim(text.substr(start, end - start));
        start = end + 1u;
        if (line.empty() || line.front() == ';' || line.front() == '#') continue;
        if (line.front() == '[') {
            const auto close = line.find(']');
            current = open_section(trim(std::string_view(line).substr(1u, close == std::string::npos
                                                                               ? std::string::npos
                                                                               : close - 1u)));
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string::npos) continue;
        if (current == nullptr) current = open_section({});
        current->entries.emplace_back(trim(std::string_view(line).substr(0u, equals)),
                                      unquote(std::string_view(line).substr(equals + 1u)));
    }
    return file;
}

std::optional<IniFile> IniFile::load(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;
    const std::string text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    return parse(text);
}

const IniFile::Section *IniFile::section(std::string_view name) const {
    for (const Section &s : sections_) {
        if (iequals(s.name, name)) return &s;
    }
    return nullptr;
}

bool IniFile::has_section(std::string_view name) const { return section(name) != nullptr; }

const std::string *IniFile::find(std::string_view section_name, std::string_view key) const {
    const Section *s = section(section_name);
    if (s == nullptr) return nullptr;
    for (auto it = s->entries.rbegin(); it != s->entries.rend(); ++it) {
        if (iequals(it->first, key)) return &it->second;
    }
    return nullptr;
}

std::string IniFile::get(std::string_view section_name, std::string_view key, std::string_view fallback) const {
    const std::string *value = find(section_name, key);
    return value != nullptr ? *value : std::string(fallback);
}

std::vector<std::string> IniFile::sections() const {
    std::vector<std::string> names;
    names.reserve(sections_.size());
    for (const Section &s : sections_) names.push_back(s.name);
    return names;
}

const IniFile::Entries &IniFile::entries(std::string_view name) const {
    static const Entries kEmpty;
    const Section *s = section(name);
    return s != nullptr ? s->entries : kEmpty;
}

} // namespace mhp3rd::mods
