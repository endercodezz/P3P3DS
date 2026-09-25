#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

// First-run installer: checks the player's disc image, copies it into the
// per-user data directory (or records where it is), and prepares the game's
// executable from it.
//
// The logic here knows nothing about how questions are asked. InstallerUi is
// the only thing a front end implements: the port's own setup screens
// (host/ui), or SDL message boxes where those cannot be shown.
namespace mhp3rd::install {

// A problem the player can act on; what() is written for them.
class InstallError final : public std::runtime_error {
public:
    explicit InstallError(const std::string &message) : std::runtime_error(message) {}
};

// Thrown from a progress callback to stop an installation the player
// cancelled. Nothing is left behind: partial files are removed.
class InstallCancelled final : public std::exception {
public:
    [[nodiscard]] const char *what() const noexcept override { return "setup cancelled"; }
};

enum class ImageStorage {
    Copy,    // copy the image into the data directory (default)
    InPlace, // use it where it is; it must stay there
};

struct ImageInfo {
    std::uint64_t size_bytes{};
};

// Checks that path is an unmodified image of the supported release: the disc
// id in PARAM.SFO and the hash of the encrypted executable. Throws
// InstallError saying plainly what is wrong otherwise.
ImageInfo check_image(const std::filesystem::path &path);

// Called with the bytes done so far and the total; stage names the step.
using ProgressFn = std::function<void(const std::string &stage, std::uint64_t done, std::uint64_t total)>;

// Free space where data_dir is or would be created, if it can be told.
[[nodiscard]] std::optional<std::uint64_t> available_space(const std::filesystem::path &data_dir);
// Space a copy of the image needs, with a margin so the disk is not filled to
// the last byte.
[[nodiscard]] std::uint64_t space_needed_to_copy(const ImageInfo &info);

// Checks the image, stores it according to storage and prepares the
// executable in data_dir. Nothing in data_dir changes until the image has
// passed its checks; files are written under temporary names and renamed at
// the end. Throws InstallError.
void install(const std::filesystem::path &image, ImageStorage storage, const std::filesystem::path &data_dir,
             const ProgressFn &progress);

#if defined(MHP3RD_ANDROID_APP)
// Android: whether an image the player chose is a content:// document (from
// the system's file picker or another app) rather than a file. A document can
// only be read through a file descriptor, never opened by its name.
[[nodiscard]] bool is_document_uri(const std::filesystem::path &image);
// Copies such a document into data_dir as the copied image (kCopiedImageFile)
// and returns where it is. Throws InstallError, or InstallCancelled from
// progress; a partial copy is removed.
std::filesystem::path copy_image_document(const std::string &uri, const std::filesystem::path &data_dir,
                                          const ProgressFn &progress);
#endif

class InstallerUi {
public:
    virtual ~InstallerUi() = default;
    // Explains what the installer needs. False: the player quit.
    virtual bool introduce(const std::filesystem::path &data_dir) = 0;
    // Lets the player pick the disc image. Empty: cancelled.
    virtual std::optional<std::filesystem::path> choose_image() = 0;
    // Copy the image or use it in place. Empty: cancelled.
    virtual std::optional<ImageStorage> choose_storage(const std::filesystem::path &image, const ImageInfo &info,
                                                       const std::filesystem::path &data_dir) = 0;
    // Runs work that may take a while: checking an image, installing. The
    // default runs it right here. A front end with a window runs it on
    // another thread and keeps drawing; progress() is then called from that
    // thread, and may throw InstallCancelled to stop the work.
    virtual void run_task(const std::string &title, const std::function<void()> &work);
    virtual void progress(const std::string &stage, std::uint64_t done, std::uint64_t total) = 0;
    // Reports a failed check or step. True: pick another image; false: quit.
    // Cancelling in choose_image() or choose_storage() goes back a step: to
    // introduce(), or to choose_image().
    virtual bool offer_retry(const std::string &message) = 0;
    virtual void finished(const std::filesystem::path &data_dir) = 0;
};

// The whole interactive flow. True when the game is installed and can start.
bool run_installer(InstallerUi &ui, const std::filesystem::path &data_dir);

// Front ends. Console progress is shared by all of them.
void print_progress(const std::string &stage, std::uint64_t done, std::uint64_t total);
// The setup screens in the game's window, or the SDL dialogs when the window
// cannot be created, or null when this build or session can show neither.
std::unique_ptr<InstallerUi> make_installer_ui();
std::unique_ptr<InstallerUi> make_dialog_ui();
// Tells the player (on screen when possible, console always) that something
// prevents the game from starting. With ask_setup, offers to run the installer
// again and returns whether the player chose to.
bool report_problem(const std::string &title, const std::string &message, bool ask_setup);
bool report_problem_in_dialog(const std::string &title, const std::string &message, bool ask_setup);

// "Set up game data again" in the in-game menu: the game quits, and the
// program starts again with --install once it has shut down.
void request_setup_on_exit();
[[nodiscard]] bool setup_requested_on_exit();
// Replaces this process with `program --install`. Returns only on failure,
// with the exit code to use.
int restart_for_setup(const char *program);

// "Restart now" after a save import in the in-game menu: the game quits, and
// the program starts again as it was started, which returns to the title
// screen, where the game reads its saves.
void request_restart_on_exit();
[[nodiscard]] bool restart_requested_on_exit();
// Replaces this process with a new run of the same program and arguments.
// Returns only on failure, with the exit code to use.
int restart(char **argv);

} // namespace mhp3rd::install
