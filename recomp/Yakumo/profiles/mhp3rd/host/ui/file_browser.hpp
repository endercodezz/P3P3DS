#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The setup's own file browser. The system file dialog may not appear at all
// under gamescope (Steam Deck Game Mode), so picking the disc image must work
// with a gamepad alone: folders open with confirm, back goes up a folder,
// common places (home, downloads, SD cards and other removable drives) are one
// press away, and only .iso files are listed unless the player asks for all.
// The menu's save import and export use it to choose folders.
namespace mhp3rd::ui {

class FileBrowser {
public:
    struct Options {
        // Files listed unless the player asks for all, as lower-case extensions.
        std::vector<std::string> extensions{".iso"};
        std::string filter_name{".iso"};          // "Showing .iso only"
        std::string listed_name{".iso images"};   // "only .iso images are listed"
        std::string empty_note{"No folders or disc images here."};
        // A row that chooses the folder being shown, with this label; none when empty.
        std::string choose_folder;
        // Folders that are chosen when opened instead of being entered, such as a save folder.
        std::function<bool(const std::filesystem::path &)> choose_on_open;
    };

    // Starts in `folder`, or the home folder when it does not exist.
    explicit FileBrowser(const std::filesystem::path &folder);
    FileBrowser(const std::filesystem::path &folder, Options options);
    ~FileBrowser();
    FileBrowser(const FileBrowser &) = delete;
    FileBrowser &operator=(const FileBrowser &) = delete;

    enum class Result { Browsing, Chosen, Cancelled };
    // Draws the browser as the content of the current panel.
    Result frame(bool back);
    [[nodiscard]] const std::filesystem::path &chosen() const noexcept { return chosen_; }
    [[nodiscard]] const std::filesystem::path &folder() const noexcept { return folder_; }

    // The player's home folder.
    static std::filesystem::path home();

private:
    struct Entry {
        std::filesystem::path path;
        std::string name;
        bool directory{};
        bool choosable{};  // a folder chosen when opened, or a listed file
        std::uint64_t size{};
    };
    struct Place {
        std::string name;
        std::filesystem::path path;
    };
    struct SystemDialog;

    // Lists `folder`, focusing `focus` when it is one of its entries. The
    // latter is taken by value: callers pass folder_ itself.
    void open(const std::filesystem::path &folder, std::filesystem::path focus = {});
    void find_places();
    // Whether a file is listed without "Showing all files".
    [[nodiscard]] bool listed(const std::filesystem::path &file) const;

    Options options_;
    std::filesystem::path folder_;
    std::filesystem::path chosen_;
    std::vector<Entry> entries_;
    std::vector<Place> places_;
    std::string error_;
    bool show_all_{};
    std::size_t hidden_files_{};
    // Entry to put the focus on when the list is next drawn.
    std::optional<std::filesystem::path> focus_;
    bool focus_first_{};
    std::shared_ptr<SystemDialog> dialog_;
};

// "1.3 GB", "532 MB", "12 KB".
std::string human_size(std::uint64_t bytes);

} // namespace mhp3rd::ui
