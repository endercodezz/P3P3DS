#pragma once
// "A newer Builder is available" (#67): the wizard asks GitHub for the latest
// release and compares its tag with its own version. This part has no network
// code (builder_main.cpp does the request) so it can be unit-tested.
#include <compare>
#include <optional>
#include <string>
#include <string_view>

namespace p3p3ds::builder {

// GitHub REST API: latest published release (drafts and pre-releases excluded).
inline constexpr const char *kLatestReleaseUrl = "https://api.github.com/repos/endercodezz/P3P3DS/releases/latest";
inline constexpr const char *kReleasesPageBase = "https://github.com/endercodezz/P3P3DS/releases/tag/";
// Set to anything but "" or "0": no update check (also --no-update-check).
inline constexpr const char *kNoUpdateCheckEnv = "P3P3DS_NO_UPDATE_CHECK";
// Test hook: request this URL instead of kLatestReleaseUrl.
inline constexpr const char *kUpdateUrlEnv = "P3P3DS_UPDATE_URL";

struct Version {
    unsigned major{}, minor{}, patch{};
    auto operator<=>(const Version &) const = default;
};

// "v1.2.3" or "1.2.3"; anything else (suffixes such as "-rc1", missing or extra
// parts, values above 9999) -> nullopt.
[[nodiscard]] std::optional<Version> parse_version(std::string_view text);
[[nodiscard]] std::string version_string(const Version &v);

// The top-level "tag_name" string of a releases/latest response; nullopt for
// anything unexpected (error bodies, escapes, non-version tags).
[[nodiscard]] std::optional<std::string> latest_release_tag(std::string_view json);

struct Update {
    Version version;
    std::string page; // kReleasesPageBase + tag, built locally (never a URL from the response)
};

// The release in `json` when it is newer than `current`.
[[nodiscard]] std::optional<Update> newer_release(std::string_view json, const Version &current);

// The version this Builder was built as (FileVersion in builder.rc).
[[nodiscard]] Version builder_version();

} // namespace p3p3ds::builder
