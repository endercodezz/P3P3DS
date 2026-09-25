#include "mhp3rd_profile.hpp"

#include "app_paths.hpp"

#include "adhoc/client.hpp"
#include "adhoc/discovery.hpp"
#include "adhoc/server.hpp"
#include "adhoc/session.hpp"

#include "install/game_identity.hpp"
#include "install/installer.hpp"
#include "install/user_data.hpp"
#include "kernel/kernel.hpp"
#include "camera/game_aspect.hpp"
#include "camera/game_camera.hpp"

#include "psprecomp/common.hpp"
#include "psprecomp/elf32.hpp"
#include "psprecomp/runtime.hpp"
#include "psprecomp/sha256.hpp"

#if defined(MHP3RD_CAMERA_HELPER_UNIT)
// Declared here rather than through generated_units.hpp, which corpora
// generated before that header existed do not have. Every generated unit has
// this signature.
namespace psprecomp {
void MHP3RD_CAMERA_HELPER_UNIT(Runtime &, AllegrexContext &);
}
#endif

#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <thread>

#if defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace {

// configs/nids.csv, compiled in (see tools/embed_nids.cmake), so the program
// needs nothing from the checkout it was built in.
struct EmbeddedNid {
    const char *library;
    std::uint32_t nid;
    const char *name;
};
constexpr EmbeddedNid kEmbeddedNids[] = {
#include "nid_table.inc"
};

#if defined(__linux__) && !defined(MHP3RD_ANDROID_APP)
// Chained AOT calls nest native frames deeply and need the 64 MiB stack the
// other platforms get from their linker. ELF has no such option: the main
// thread's stack is sized by RLIMIT_STACK when the program starts, so raise the
// limit and start again once. A launcher that already ran `ulimit -s 65536`
// never gets here.
void ensure_main_stack(char **argv) {
    constexpr rlim_t kWanted = 64ull * 1024u * 1024u;
    constexpr const char *kMarker = "MHP3RD_STACK_RAISED";
    rlimit limit{};
    if (getrlimit(RLIMIT_STACK, &limit) != 0) return;
    if (limit.rlim_cur == RLIM_INFINITY || limit.rlim_cur >= kWanted) return;
    if (std::getenv(kMarker) != nullptr) return;
    const rlim_t target = limit.rlim_max == RLIM_INFINITY || limit.rlim_max >= kWanted ? kWanted : limit.rlim_max;
    if (target <= limit.rlim_cur) {
        std::cerr << "warning: the stack is limited to " << (limit.rlim_cur >> 20) << " MiB and cannot be raised to 64 MiB\n";
        return;
    }
    limit.rlim_cur = target;
    if (setrlimit(RLIMIT_STACK, &limit) != 0) return;
    setenv(kMarker, "1", 1);
    // Through the resolved path: executing /proc/self/exe itself would rename
    // the process to "exe".
    const std::filesystem::path self = mhp3rd::executable_path();
    execv(self.empty() ? "/proc/self/exe" : self.c_str(), argv);
    // Still here: carry on with the stack there is.
    std::cerr << "warning: could not restart with a larger stack; deep call chains may overflow\n";
}
#endif

std::uint64_t configured_max_dispatches() {
    constexpr std::uint64_t default_limit = 4'000'000'000ull;
    const char *text = std::getenv("PSPRECOMP_MAX_DISPATCHES");
    if (text == nullptr || *text == '\0') return default_limit;
    errno = 0;
    char *end = nullptr;
    const unsigned long long parsed = std::strtoull(text, &end, 0);
    if (errno == ERANGE || end == text || *end != '\0' || parsed == 0u)
        throw psprecomp::Error(std::string("Invalid PSPRECOMP_MAX_DISPATCHES value: ") + text);
    return static_cast<std::uint64_t>(parsed);
}

constexpr const char *kUsage =
    "usage: Yakumo [game_dir]\n"
    "       Yakumo --install [image.iso [--in-place]]\n"
    "       Yakumo --adhoc-server [port]\n"
    "  game_dir        play from a directory holding EBOOT.ELF, disc.iso and ms0/\n"
    "  --install       run the setup again on screen, then play\n"
    "  --install image set up from image.iso without the setup screens, then exit\n"
    "  --in-place      use the image where it is instead of copying it\n"
    "  --adhoc-server  run only the ad hoc server that Network > Host a session\n"
    "                  starts, on TCP port (default 27312) and the next one up,\n"
    "                  announced on the local network, until Ctrl+C\n";

struct Options {
    std::optional<std::filesystem::path> game_dir;
    bool install = false;
    std::optional<std::filesystem::path> install_image;
    bool in_place = false;
};

class UsageError final : public std::runtime_error {
public:
    explicit UsageError(const std::string &message) : std::runtime_error(message) {}
};

Options parse_options(int argc, char **argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") throw UsageError("");
        if (arg == "--install") options.install = true;
        else if (arg == "--in-place") options.in_place = true;
        else if (!arg.empty() && arg[0] == '-') throw UsageError("unknown option " + arg);
        else if (options.install && !options.install_image) options.install_image = mhp3rd::install::path_from_utf8(arg);
        else if (!options.install && !options.game_dir) options.game_dir = std::filesystem::path(arg);
        else throw UsageError("unexpected argument " + arg);
    }
    if (options.in_place && !options.install_image) throw UsageError("--in-place needs --install image.iso");
    if (options.install && options.game_dir) throw UsageError("--install does not take a game_dir");
    return options;
}

struct GameFiles {
    std::filesystem::path executable;
    std::filesystem::path disc_image; // empty: disc0: is unavailable
    std::filesystem::path memory_stick;
};

// The layout a game_dir has always had; profiles/mhp3rd/game by default.
GameFiles files_in_game_directory(const std::filesystem::path &game_dir) {
    GameFiles files;
    files.executable = game_dir / "EBOOT.ELF";
    files.disc_image = game_dir / "disc.iso";
    files.memory_stick = mhp3rd::memory_stick_directory(game_dir);
    if (!std::filesystem::exists(files.disc_image)) {
        std::cerr << "warning: " << files.disc_image.string() << " not found; disc0: is unavailable\n";
        files.disc_image.clear();
    }
    return files;
}

bool has_game_data(const std::filesystem::path &game_dir) {
    std::error_code ec;
    return std::filesystem::exists(std::filesystem::symlink_status(game_dir / "EBOOT.ELF", ec)) ||
           std::filesystem::exists(std::filesystem::symlink_status(game_dir / "disc.iso", ec));
}

// profiles/mhp3rd/game in the checkout this was built from, for developer
// builds; empty in a release build, which must not depend on the machine it
// was built on.
std::filesystem::path checkout_game_directory() {
#if defined(MHP3RD_DEFAULT_GAME_DIR)
    return MHP3RD_DEFAULT_GAME_DIR;
#else
    return {};
#endif
}

// ms0 of an installation lives in the per-user data directory with the rest
// of it. Saves a developer build made in the checkout before that stay in use
// until ms0 exists in the data directory.
std::filesystem::path installed_memory_stick(const std::filesystem::path &data_dir,
                                             const std::filesystem::path &checkout_game_dir) {
    const std::filesystem::path memory_stick = mhp3rd::memory_stick_directory(data_dir);
    std::error_code ec;
    if (!checkout_game_dir.empty() && !std::filesystem::exists(memory_stick, ec)) {
        const std::filesystem::path legacy = mhp3rd::memory_stick_directory(checkout_game_dir);
        if (std::filesystem::is_directory(legacy / "PSP" / "SAVEDATA", ec)) {
            std::cerr << "note: using the saves in " << legacy.string() << "; move that directory to "
                      << memory_stick.string() << " to keep them with the installation\n";
            return legacy;
        }
    }
    return memory_stick;
}

// Finds the game: an explicit game_dir, then the per-user data directory the
// installer fills, then profiles/mhp3rd/game in a developer build. With none of
// them, runs the installer. Empty when the player quit or nothing could be set up.
std::optional<GameFiles> locate_game(const Options &options) {
    namespace install = mhp3rd::install;
    if (options.game_dir) return files_in_game_directory(*options.game_dir);
    if (const char *dir = std::getenv("MHP3RD_GAME_DIR"); dir != nullptr && *dir != '\0')
        return files_in_game_directory(dir);

    const std::filesystem::path checkout_game_dir = checkout_game_directory();
    const std::filesystem::path data_dir = install::user_data_directory();
    bool run_setup = options.install;
    for (;;) {
        if (!run_setup) {
            if (const auto installed = install::find_installation(data_dir)) {
                std::error_code ec;
                if (std::filesystem::is_regular_file(installed->disc_image, ec)) {
                    GameFiles files;
                    files.executable = installed->executable;
                    files.disc_image = installed->disc_image;
                    files.memory_stick = installed_memory_stick(data_dir, checkout_game_dir);
                    return files;
                }
                const std::string where = install::path_to_utf8(installed->disc_image);
                const std::string message =
                    installed->image_copied
                        ? "The copy of the disc image Yakumo made is missing:\n" + where +
                              "\n\nSet up again to restore it (Yakumo --install)."
                        : "The disc image Yakumo was set up with is no longer at:\n" + where +
                              "\n\nPut it back there, or set up again to choose where it is now "
                              "(Yakumo --install).";
                if (!install::report_problem("Disc image not found", message, true)) return std::nullopt;
                run_setup = true;
                continue;
            }
            if (!checkout_game_dir.empty() && has_game_data(checkout_game_dir))
                return files_in_game_directory(checkout_game_dir);
        }

        auto ui = install::make_installer_ui();
        if (!ui) {
            std::cerr << "No game data found in " << install::path_to_utf8(data_dir)
                      << (checkout_game_dir.empty() ? std::string() : " or " + checkout_game_dir.string()) << ".\n"
                      << "Set up from your disc image of " << install::kGameTitle << " (" << install::kDiscIdDisplay
                      << ") with:\n  Yakumo --install /path/to/image.iso\n";
            return std::nullopt;
        }
        if (!install::run_installer(*ui, data_dir)) return std::nullopt;
        run_setup = false;
    }
}

int install_from_command_line(const Options &options) {
    namespace install = mhp3rd::install;
    const std::filesystem::path data_dir = install::user_data_directory();
    try {
        install::install(*options.install_image,
                         options.in_place ? install::ImageStorage::InPlace : install::ImageStorage::Copy, data_dir,
                         install::print_progress);
    } catch (const install::InstallError &e) {
        std::cerr << "Setup failed: " << e.what() << "\n";
        return 1;
    }
    std::cout << "Game data is ready in " << install::path_to_utf8(data_dir) << ". Start Yakumo to play.\n";
    return 0;
}

std::atomic<bool> stop_server{false};

extern "C" void on_stop_signal(int) { stop_server = true; }

// The built-in ad hoc server without the game, for leaving it running.
int run_adhoc_server(int argc, char **argv) {
    using namespace mhp3rd::adhoc;
    ServerConfig config;
    config.print_events = true;
    if (argc > 2) {
        const unsigned long port = std::strtoul(argv[2], nullptr, 10);
        if (port < 1024u || port > 65534u) {
            std::cerr << "Yakumo: --adhoc-server takes a port from 1024 to 65534\n";
            return 2;
        }
        config.adhocctl_port = static_cast<std::uint16_t>(port);
    }
    Server server;
    if (!server.start(config)) {
        std::cerr << "Cannot start the ad hoc server: " << server.status().error << "\n";
        return 1;
    }
    std::string name = local_host_name();
    if (name.empty()) name = "Yakumo server";
    Discovery::get().start_announcing(config.adhocctl_port, [&server, name] {
        Announcement info;
        info.name = name;
        const ServerStatus status = server.status();
        info.players = static_cast<unsigned>(status.players.size());
        info.product = status.players.empty() ? "ULJM05800" : status.players.front().product;
        return info;
    });
    const std::string suffix =
        config.adhocctl_port == kAdhocctlPort ? std::string() : ":" + std::to_string(config.adhocctl_port);
    std::cout << "Ad hoc server on TCP " << config.adhocctl_port << " and " << relay_port_for(config.adhocctl_port)
              << ", announced on the local network as \"" << name << "\".\nPlayers join with one of:\n";
    for (const LocalAddress &address : local_addresses())
        std::cout << "  " << address.address << suffix << "   (" << address.network << ", " << address.interface
                  << ")\n";
    std::cout << "Ctrl+C stops it." << std::endl;
    std::signal(SIGINT, on_stop_signal);
    std::signal(SIGTERM, on_stop_signal);
    while (!stop_server) std::this_thread::sleep_for(std::chrono::milliseconds(200));
    Discovery::get().shutdown();
    server.stop();
    Client::get().shutdown();
    return 0;
}

} // namespace

#if defined(MHP3RD_ANDROID_APP)
// android_app.cpp owns the entry point SDL calls and runs this on a thread
// with the stack the game needs; restarting the process to get one is not an
// option inside an app.
int yakumo_main(int argc, char **argv) {
#else
int main(int argc, char **argv) {
#endif
#if defined(__linux__) && !defined(MHP3RD_ANDROID_APP)
    ensure_main_stack(argv);
#endif
    try {
        if (argc > 1 && std::string(argv[1]) == "--adhoc-server") return run_adhoc_server(argc, argv);
        Options options;
        try {
            options = parse_options(argc, argv);
        } catch (const UsageError &e) {
            if (*e.what() == '\0') {
                std::cout << kUsage;
                return 0;
            }
            std::cerr << "Yakumo: " << e.what() << "\n" << kUsage;
            return 2;
        }
        if (options.install_image) return install_from_command_line(options);
#if defined(MHP3RD_ANDROID_APP)
        {
            // "Set up game data again" from the menu, left for this start.
            std::error_code ec;
            const std::filesystem::path marker =
                mhp3rd::install::user_data_directory() / mhp3rd::install::kSetupMarkerFile;
            if (std::filesystem::remove(marker, ec)) options.install = true;
        }
#endif

        const std::optional<GameFiles> files = locate_game(options);
        if (!files) return 1;
        const std::filesystem::path &executable = files->executable;
        mhp3rd::ProfilePaths paths;
        paths.disc_image = files->disc_image;
        paths.memory_stick = files->memory_stick;
        if (!std::filesystem::is_regular_file(executable))
            throw psprecomp::Error("Missing " + executable.string() + " (run profiles/mhp3rd/scripts/prepare_game.sh)");

        const std::string sha256 = psprecomp::sha256_file(executable);
        if (sha256 != mhp3rd::install::kExecutableSha256)
            std::cerr << "warning: unsupported executable hash " << sha256 << "\n";

        const psprecomp::Elf32Image elf = psprecomp::Elf32Image::from_file(executable);
        if (elf.required_ram_size(mhp3rd::kLoadBase) != mhp3rd::kGuestRamBytes)
            throw psprecomp::Error("Executable does not match the 64 MiB MHP3rd HD layout");

        psprecomp::Runtime runtime(mhp3rd::kGuestRamBytes);
        for (const EmbeddedNid &entry : kEmbeddedNids) runtime.nids().add(entry.library, entry.nid, entry.name);
        (void)elf.load_and_relocate(runtime.memory(), mhp3rd::kLoadBase);
        psprecomp::register_generated_functions(runtime);
        mhp3rd::install_profile(runtime, elf, paths);
        if (sha256 == mhp3rd::install::kExecutableSha256) (void)mhp3rd::camera::prepare_game_aspect(runtime);
#if defined(MHP3RD_CAMERA_HELPER_UNIT)
        // CMake names the generated unit that holds the camera's rotation
        // helper, so a new partition of the code cannot hand the camera the
        // wrong one.
        if (sha256 == mhp3rd::install::kExecutableSha256)
            (void)mhp3rd::camera::prepare_game_camera(runtime, &psprecomp::MHP3RD_CAMERA_HELPER_UNIT);
#endif

        std::cout << "Yakumo PSP bootstrap\n"
                  << "Executable: " << executable.string() << "\n"
                  << "SHA-256:    " << sha256 << "\n"
                  << "Disc image: " << (paths.disc_image.empty() ? "<none>" : paths.disc_image.string()) << "\n"
                  << "Entry:      " << psprecomp::hex32(elf.runtime_entry(mhp3rd::kLoadBase)) << "\n"
                  << "Functions:  " << runtime.function_count() << "\n";
        if (runtime.function_count() == 0u) {
            std::cout << "No generated functions are linked. Run profiles/mhp3rd/scripts/generate.sh and rebuild.\n";
            return 3;
        }

        runtime.run(elf.runtime_entry(mhp3rd::kLoadBase), configured_max_dispatches());
        std::cout << "Runtime stopped: " << runtime.stop_reason() << "\n";
        // Quit from the menu, a closed window or the game ending: the network
        // threads stop here, while everything they use still exists.
        mhp3rd::adhoc_shutdown();
        // "Set up game data again" in the in-game menu.
        if (mhp3rd::install::setup_requested_on_exit()) return mhp3rd::install::restart_for_setup(argv[0]);
        // "Restart now" after importing a save.
        if (mhp3rd::install::restart_requested_on_exit()) return mhp3rd::install::restart(argv);
        std::cout << mhp3rd::kernel().describe_threads() << "\n";
        runtime.report_hle_histogram();
        return runtime.stop_reason().empty() ? 0 : 4;
    } catch (const std::exception &e) {
        mhp3rd::adhoc_shutdown();
        std::cerr << "Yakumo error: " << e.what() << "\n";
        return 1;
    }
}
