// Installer front end built from SDL3's native dialogs: message boxes and the
// system file picker. The port's own setup screens (host/ui) replace it
// wherever the game's window can be created; this remains for systems where
// it cannot, such as a machine without a working Vulkan driver.

#include "install/installer.hpp"

#include "install/game_identity.hpp"
#include "install/user_data.hpp"

#include <cstdlib>
#include <iostream>

#if defined(MHP3RD_HAS_SDL)
#include <SDL3/SDL.h>

#if defined(MHP3RD_ANDROID_APP)
#include "platform/android_jni.hpp"
#endif

#include <atomic>
#include <mutex>
#include <vector>
#endif

namespace mhp3rd::install {

#if defined(MHP3RD_HAS_SDL)
namespace {

constexpr const char *kTitle = "Yakumo setup";

struct Button {
    int id;
    const char *text;
    SDL_MessageBoxButtonFlags flags;
};

// Shows a message box and returns the id of the button pressed, or -1 when
// the box could not be shown or was closed.
int ask(SDL_MessageBoxFlags kind, const char *title, const std::string &message, const std::vector<Button> &buttons) {
    std::vector<SDL_MessageBoxButtonData> data;
    for (const Button &button : buttons) data.push_back({button.flags, button.id, button.text});
    SDL_MessageBoxData box{};
    box.flags = kind | SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT;
    box.title = title;
    box.message = message.c_str();
    box.numbuttons = static_cast<int>(data.size());
    box.buttons = data.data();
    int pressed = -1;
    if (!SDL_ShowMessageBox(&box, &pressed)) {
        std::cerr << "Setup: cannot show a dialog (" << SDL_GetError() << ")\n";
        return -1;
    }
    return pressed;
}

class DialogUi final : public InstallerUi {
public:
    bool introduce(const std::filesystem::path &data_dir) override {
#if defined(MHP3RD_ANDROID_APP)
        // The setup screens could not be shown (the game's window or its
        // renderer failed), so these dialogs stand in for them.
        (void)data_dir;
        const std::string text = std::string("Yakumo needs your own copy of ") + kGameTitle + " (" + kDiscIdDisplay +
                                 ") as a disc image (.iso).\n\n"
                                 "Choose the image next, in Android's file picker. Yakumo copies it into its own "
                                 "storage (about 1.3 GB), checks it and prepares the game from it.";
#else
        std::string text = std::string("Yakumo needs your own copy of ") + kGameTitle + " (" + kDiscIdDisplay +
                           ") as a disc image (.iso).\n\n"
                           "Choose the image next. Yakumo checks it, prepares the game's executable from it and "
                           "copies it into its data folder, so the game keeps working if you move or delete the "
                           "original. You can also choose to use the image where it is.\n\n"
                           "Data folder:\n" +
                           path_to_utf8(data_dir);
#if defined(__linux__)
        text += "\n\nOn a Steam Deck, the file dialog may need Desktop Mode the first time.";
#endif
#endif
        return ask(SDL_MESSAGEBOX_INFORMATION, kTitle, text,
                   {{1, "Choose image...", SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT},
                    {0, "Quit", SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT}}) == 1;
    }

    std::optional<std::filesystem::path> choose_image() override {
#if defined(MHP3RD_ANDROID_APP)
        // Android's picker gives a content:// document, not a file;
        // run_installer() copies it into the data folder before checking it.
        const std::optional<std::string> uri = android::pick_document();
        if (!uri) return std::nullopt;
        std::cout << "[setup] picked " << *uri << std::endl;
        ask(SDL_MESSAGEBOX_INFORMATION, kTitle,
            "Yakumo copies the disc image into its own storage now. This takes a minute or two, and nothing moves "
            "on the screen until it is done.",
            {{0, "OK", SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT}});
        return path_from_utf8(*uri);
#endif
        struct Pick {
            std::mutex mutex;
            std::atomic<bool> done{false};
            std::optional<std::filesystem::path> path;
            std::string error;
        } pick;
        static const SDL_DialogFileFilter filters[] = {{"Disc images (*.iso)", "iso"}, {"All files", "*"}};
        const auto callback = [](void *userdata, const char *const *files, int) {
            auto &state = *static_cast<Pick *>(userdata);
            {
                std::lock_guard lock(state.mutex);
                if (files == nullptr) state.error = SDL_GetError();
                else if (files[0] != nullptr) state.path = path_from_utf8(files[0]);
            }
            state.done = true;
        };
        SDL_ShowOpenFileDialog(callback, &pick, nullptr, filters, 2, nullptr, false);
        // The callback may run on another thread or from the event loop.
        while (!pick.done) {
            SDL_Event event;
            if (SDL_WaitEventTimeout(&event, 50) && event.type == SDL_EVENT_QUIT) return std::nullopt;
        }
        std::lock_guard lock(pick.mutex);
        if (!pick.path && !pick.error.empty()) {
            ask(SDL_MESSAGEBOX_ERROR, kTitle,
                "The file dialog could not be opened (" + pick.error +
                    ").\n\nRun the setup from a terminal instead:\n  Yakumo --install /path/to/image.iso",
                {{0, "Quit", SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT}});
        }
        return pick.path;
    }

    std::optional<ImageStorage> choose_storage(const std::filesystem::path &image, const ImageInfo &info,
                                               const std::filesystem::path &data_dir) override {
#if defined(MHP3RD_ANDROID_APP)
        // Already copied into the data folder; an app cannot keep reading a
        // file elsewhere.
        (void)image;
        (void)info;
        (void)data_dir;
        return ImageStorage::Copy;
#endif
        const std::uint64_t tenths = (info.size_bytes + 50'000'000u) / 100'000'000u;
        const std::string size = std::to_string(tenths / 10u) + "." + std::to_string(tenths % 10u) + " GB";
        const std::string text = "The image is " + std::string(kGameTitle) + " (" + kDiscIdDisplay +
                                 ") and passed its checks.\n\n"
                                 "Copy it into Yakumo's data folder (recommended, " +
                                 size +
                                 "), so the game keeps working if the original is moved or deleted?\n\n"
                                 "Or use it where it is, to save space. The image must then stay at:\n" +
                                 path_to_utf8(image) + "\n\nData folder:\n" + path_to_utf8(data_dir);
        switch (ask(SDL_MESSAGEBOX_INFORMATION, kTitle, text,
                    {{1, "Copy (recommended)", SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT},
                     {2, "Use it where it is", SDL_MessageBoxButtonFlags{0}},
                     {0, "Cancel", SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT}})) {
        case 1: return ImageStorage::Copy;
        case 2: return ImageStorage::InPlace;
        default: return std::nullopt;
        }
    }

    void progress(const std::string &stage, std::uint64_t done, std::uint64_t total) override {
        print_progress(stage, done, total);
        // Keep the system from treating the process as hung while it copies.
        SDL_PumpEvents();
    }

    bool offer_retry(const std::string &message) override {
        return ask(SDL_MESSAGEBOX_ERROR, kTitle, message,
                   {{1, "Choose another image...", SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT},
                    {0, "Quit", SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT}}) == 1;
    }

    void finished(const std::filesystem::path &) override {
        ask(SDL_MESSAGEBOX_INFORMATION, kTitle, "Setup is complete. The game starts now.",
            {{0, "Play", SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT}});
    }

    ~DialogUi() override {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        SDL_ResetHint(SDL_HINT_NO_SIGNAL_HANDLERS);
    }
};

bool dialogs_available() {
    if (const char *off = std::getenv("MHP3RD_NO_RENDER"); off != nullptr && *off != '\0' && *off != '0') return false;
    return true;
}

} // namespace

std::unique_ptr<InstallerUi> make_dialog_ui() {
    if (!dialogs_available()) return nullptr;
    // The file dialog needs the video subsystem, and with it a display. Keep
    // SDL from turning SIGINT/SIGTERM into a quit event while the installer
    // runs: nothing could act on it inside a modal dialog, so the process
    // would ignore Ctrl+C. The game installs SDL's handlers as usual.
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        SDL_ResetHint(SDL_HINT_NO_SIGNAL_HANDLERS);
        std::cerr << "Setup: no display for dialogs (" << SDL_GetError() << ")\n";
        return nullptr;
    }
    return std::make_unique<DialogUi>();
}

bool report_problem_in_dialog(const std::string &title, const std::string &message, bool ask_setup) {
    if (!dialogs_available()) return false;
    if (!ask_setup) {
        ask(SDL_MESSAGEBOX_ERROR, title.c_str(), message,
            {{0, "Quit", SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT}});
        return false;
    }
    return ask(SDL_MESSAGEBOX_ERROR, title.c_str(), message,
               {{1, "Set up again...", SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT},
                {0, "Quit", SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT}}) == 1;
}

#else

std::unique_ptr<InstallerUi> make_dialog_ui() { return nullptr; }

bool report_problem_in_dialog(const std::string &, const std::string &, bool) { return false; }

#endif

} // namespace mhp3rd::install
