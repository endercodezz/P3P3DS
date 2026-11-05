// Builder update check (#67, platform/pc/builder/update_check.cpp): version
// parsing and comparison, the tag of a GitHub releases/latest response, and
// the release page URL built from it.
//   test_update_check <FileVersion from builder.rc>
#include "update_check.hpp"

#include <cstdio>
#include <string>

using namespace p3p3ds::builder;

namespace {
int failures = 0;
void check(bool ok, const char *what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

// Shape of a real api.github.com releases/latest response (trimmed): the
// author object and assets carry their own html_url / name fields.
const char *kResponse = R"({
  "url": "https://api.github.com/repos/endercodezz/P3P3DS/releases/1",
  "html_url": "https://github.com/endercodezz/P3P3DS/releases/tag/v0.3.3",
  "id": 1,
  "author": {"login": "endercodezz", "tag_name": "v9.9.9", "html_url": "https://github.com/endercodezz"},
  "tag_name" : "v0.3.3",
  "name": "P3P3DS Builder 0.3.3",
  "draft": false,
  "prerelease": false,
  "assets": [{"name": "P3P3DS-Builder-0.3.3.zip", "tag_name": "v8.0.0"}]
})";
} // namespace

int main(int argc, char **argv) {
    // Versions.
    check(parse_version("v0.3.2") == Version{0, 3, 2}, "v0.3.2");
    check(parse_version("1.10.0") == Version{1, 10, 0}, "1.10.0");
    check(!parse_version("v0.3"), "two parts");
    check(!parse_version("v0.3.2.1"), "four parts");
    check(!parse_version("v0.3.3-rc1"), "suffix");
    check(!parse_version("v0.3.x"), "letters");
    check(!parse_version(""), "empty");
    check(!parse_version("v12345.0.0"), "too long");
    check(Version{0, 10, 0} > Version{0, 9, 9}, "numeric, not lexical order");
    check(version_string({1, 2, 30}) == "1.2.30", "version_string");

    // Tag of the release object, not of nested objects.
    const auto tag = latest_release_tag(kResponse);
    check(tag && *tag == "v0.3.3", "top-level tag_name");
    check(!latest_release_tag(R"({"message":"API rate limit exceeded for 1.2.3.4.","documentation_url":"https://docs.github.com"})"),
          "rate-limit body");
    check(!latest_release_tag(R"({"message":"Not Found"})"), "not found body");
    check(!latest_release_tag(R"({"tag_name": 3})"), "non-string tag");
    check(!latest_release_tag(R"({"tag_name": "v0.3.3)"), "unterminated");
    check(!latest_release_tag(R"({"tag_name": "v0\".3.3"})"), "escaped quote");
    check(!latest_release_tag("<html>Service unavailable</html>"), "not JSON");
    check(!latest_release_tag(""), "empty body");
    check(!latest_release_tag(R"({"name": "tag_name"})"), "tag_name as a value");

    // Newer?
    auto update = newer_release(kResponse, {0, 3, 2});
    check(update && update->version == Version{0, 3, 3}, "0.3.3 is newer than 0.3.2");
    check(update && update->page == "https://github.com/endercodezz/P3P3DS/releases/tag/v0.3.3", "page built from the tag");
    check(!newer_release(kResponse, {0, 3, 3}), "same version");
    check(!newer_release(kResponse, {0, 4, 0}), "older release");
    check(!newer_release(R"({"tag_name": "v0.4.0-beta"})", {0, 3, 2}), "pre-release style tag");
    check(!newer_release(R"j({"tag_name": "javascript:alert(1)"})j", {0, 0, 0}), "a tag that is not a version never becomes a URL");

    // Built-in version = builder.rc FileVersion.
    if (argc > 1) check(parse_version(argv[1]) == builder_version(), "builder_version matches builder.rc");
    check(builder_version() > Version{0, 0, 0}, "builder_version parsed");

    if (failures == 0) std::printf("test_update_check: all checks passed (builder %s)\n", version_string(builder_version()).c_str());
    return failures == 0 ? 0 : 1;
}
