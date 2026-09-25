#pragma once

#include <filesystem>
#include <vector>

namespace mhp3rd {

// The running executable, resolved through the operating system rather than
// argv[0]; empty if it cannot be determined.
[[nodiscard]] std::filesystem::path executable_path();

// The directory the executable is in. A release keeps everything it ships
// relative to it: overlays/, fonts/ and, on Linux, lib/. The macOS app bundle
// is the exception, see the two functions below.
[[nodiscard]] std::filesystem::path executable_directory();

// Where the overlay libraries a release ships are: overlays/ next to the
// executable, or Contents/Frameworks/overlays in a macOS app bundle, which
// keeps code out of Contents/MacOS other than the executable.
[[nodiscard]] std::filesystem::path bundled_overlay_directory();

// Where the fonts a release ships are: fonts/ next to the executable, or
// Contents/Resources/fonts in a macOS app bundle.
[[nodiscard]] std::filesystem::path bundled_font_directory();

// Font files a release ships in fonts/ next to the executable, as fallbacks
// after the system's own fonts.
[[nodiscard]] std::vector<std::filesystem::path> bundled_fonts();

// Where fonts/ is found instead of next to the executable. An Android app
// ships its font inside the APK and unpacks it to its own storage first.
void set_bundled_resource_directory(std::filesystem::path directory);

} // namespace mhp3rd
