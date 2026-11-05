// See update_check.hpp.
#include "update_check.hpp"

#ifndef P3P3DS_BUILDER_VERSION
#error "P3P3DS_BUILDER_VERSION must come from builder.rc (CMakeLists.txt)"
#endif

namespace p3p3ds::builder {

std::optional<Version> parse_version(std::string_view text) {
    if (!text.empty() && (text.front() == 'v' || text.front() == 'V')) text.remove_prefix(1);
    unsigned parts[3]{};
    for (int i = 0; i < 3; ++i) {
        std::size_t n = 0;
        unsigned value = 0;
        while (n < text.size() && text[n] >= '0' && text[n] <= '9') {
            value = value * 10 + static_cast<unsigned>(text[n] - '0');
            if (++n > 4) return std::nullopt;
        }
        if (n == 0) return std::nullopt;
        parts[i] = value;
        text.remove_prefix(n);
        if (i < 2) {
            if (text.empty() || text.front() != '.') return std::nullopt;
            text.remove_prefix(1);
        }
    }
    if (!text.empty()) return std::nullopt;
    return Version{parts[0], parts[1], parts[2]};
}

std::string version_string(const Version &v) {
    return std::to_string(v.major) + "." + std::to_string(v.minor) + "." + std::to_string(v.patch);
}

std::optional<std::string> latest_release_tag(std::string_view json) {
    // The release object's own keys sit at depth 1; nested objects (author,
    // assets) are skipped so their fields can't be mistaken for the tag.
    int depth = 0;
    bool in_string = false;
    std::size_t string_start = 0;
    for (std::size_t i = 0; i < json.size(); ++i) {
        const char c = json[i];
        if (in_string) {
            if (c == '\\') { ++i; continue; }
            if (c != '"') continue;
            in_string = false;
            if (depth != 1 || json.substr(string_start, i - string_start) != "tag_name") continue;
            std::size_t j = i + 1;
            while (j < json.size() && (json[j] == ' ' || json[j] == '\t' || json[j] == '\r' || json[j] == '\n')) ++j;
            if (j >= json.size() || json[j] != ':') continue; // a value, not a key
            ++j;
            while (j < json.size() && (json[j] == ' ' || json[j] == '\t' || json[j] == '\r' || json[j] == '\n')) ++j;
            if (j >= json.size() || json[j] != '"') return std::nullopt;
            const std::size_t start = ++j;
            while (j < json.size() && json[j] != '"' && json[j] != '\\') ++j;
            if (j >= json.size() || json[j] != '"') return std::nullopt; // unterminated or escaped
            return std::string(json.substr(start, j - start));
        }
        if (c == '"') { in_string = true; string_start = i + 1; }
        else if (c == '{' || c == '[') ++depth;
        else if (c == '}' || c == ']') --depth;
    }
    return std::nullopt;
}

std::optional<Update> newer_release(std::string_view json, const Version &current) {
    const auto tag = latest_release_tag(json);
    if (!tag) return std::nullopt;
    const auto version = parse_version(*tag);
    if (!version || !(*version > current)) return std::nullopt;
    // The tag passed parse_version, so it is only 'v', digits and dots.
    return Update{*version, std::string(kReleasesPageBase) + *tag};
}

Version builder_version() {
    static const Version v = parse_version(P3P3DS_BUILDER_VERSION).value_or(Version{});
    return v;
}

} // namespace p3p3ds::builder
