#include "app_paths.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(MHP3RD_ANDROID_APP)
#include <dlfcn.h>
#endif

namespace mhp3rd {

std::filesystem::path executable_path() {
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;) {
        const DWORD written = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (written == 0u) return {};
        if (written < buffer.size()) return std::filesystem::path(buffer.substr(0, written));
        buffer.resize(buffer.size() * 2u);
    }
#elif defined(__APPLE__)
    std::uint32_t size = 0u;
    _NSGetExecutablePath(nullptr, &size);
    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) return {};
    return std::filesystem::canonical(buffer.c_str());
#elif defined(MHP3RD_ANDROID_APP)
    // An Android app runs inside app_process; its own code is libmain.so, in
    // the directory the package manager extracted the APK's libraries to.
    Dl_info info{};
    if (dladdr(reinterpret_cast<const void *>(&executable_path), &info) == 0 || info.dli_fname == nullptr) return {};
    return std::filesystem::path(info.dli_fname);
#else
    std::error_code ec;
    const std::filesystem::path self = std::filesystem::read_symlink("/proc/self/exe", ec);
    return ec ? std::filesystem::path{} : self;
#endif
}

std::filesystem::path executable_directory() {
    const std::filesystem::path executable = executable_path();
    return executable.empty() ? std::filesystem::path{} : executable.parent_path();
}

namespace {
std::filesystem::path &bundled_resource_directory() {
    static std::filesystem::path directory;
    return directory;
}

// Yakumo.app/Contents when the executable runs from Yakumo.app/Contents/MacOS,
// empty otherwise (and on other systems).
std::filesystem::path app_bundle_contents(const std::filesystem::path &executable_dir) {
#if defined(__APPLE__)
    if (executable_dir.filename() == "MacOS" && executable_dir.parent_path().filename() == "Contents")
        return executable_dir.parent_path();
#endif
    (void)executable_dir;
    return {};
}

// <subdirectory> of the app bundle's Contents, else <name> next to the
// executable; empty if the executable cannot be found.
std::filesystem::path shipped_directory(const char *bundle_subdirectory, const char *name) {
    const std::filesystem::path directory = executable_directory();
    if (directory.empty()) return {};
    if (const std::filesystem::path contents = app_bundle_contents(directory); !contents.empty())
        return contents / bundle_subdirectory / name;
    return directory / name;
}

} // namespace

void set_bundled_resource_directory(std::filesystem::path directory) {
    bundled_resource_directory() = std::move(directory);
}

std::filesystem::path bundled_overlay_directory() { return shipped_directory("Frameworks", "overlays"); }

// Android unpacks the APK's assets into a directory it names at start-up.
std::filesystem::path bundled_font_directory() {
    if (!bundled_resource_directory().empty()) return bundled_resource_directory() / "fonts";
    return shipped_directory("Resources", "fonts");
}

std::vector<std::filesystem::path> bundled_fonts() {
    std::vector<std::filesystem::path> fonts;
    const std::filesystem::path directory = bundled_font_directory();
    if (directory.empty()) return fonts;
    std::error_code ec;
    for (const auto &entry : std::filesystem::directory_iterator(directory, ec)) {
        const std::filesystem::path extension = entry.path().extension();
        if (extension == ".otf" || extension == ".ttf" || extension == ".ttc") fonts.push_back(entry.path());
    }
    std::sort(fonts.begin(), fonts.end());
    return fonts;
}

} // namespace mhp3rd
