#pragma once

// What the Android app asks of its Java activity (YakumoActivity) through JNI:
// the display cutout, and folders the player picks through the system's
// document picker, read and written as content:// documents. Only in the
// Android app (MHP3RD_ANDROID_APP); every call runs on the calling thread and
// the picker blocks it until the player has chosen.

#include <optional>
#include <string>
#include <vector>

namespace mhp3rd::android {

struct Insets {
    int left{};
    int top{};
    int right{};
    int bottom{};
};
// The display cutout, in window pixels; zero where there is none.
[[nodiscard]] Insets cutout_insets();

// Ends this process and starts the app again; returns only if it cannot.
void relaunch();

// A folder the player picks, as a tree URI; nothing when cancelled.
[[nodiscard]] std::optional<std::string> pick_folder();
// A file the player picks to read, as a document URI; nothing when cancelled.
[[nodiscard]] std::optional<std::string> pick_document();
// The document standing for a picked tree's folder itself.
[[nodiscard]] std::string tree_root(const std::string &tree_uri);

struct Entry {
    bool directory{};
    std::string name;
    std::string uri;
};
// The children of a folder document; nothing if it cannot be read.
[[nodiscard]] std::optional<std::vector<Entry>> list_folder(const std::string &folder_uri);
// A new folder or file in a folder document, its URI; nothing on failure.
[[nodiscard]] std::optional<std::string> create(const std::string &folder_uri, const std::string &name,
                                                bool directory);
// A file descriptor the caller closes, for reading ("r") or writing ("w",
// truncating); -1 on failure.
[[nodiscard]] int open_document(const std::string &uri, const char *mode);

} // namespace mhp3rd::android
