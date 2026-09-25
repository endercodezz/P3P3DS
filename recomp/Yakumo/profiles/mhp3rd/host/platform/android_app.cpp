// The entry point of the Android app. SDLActivity loads libmain.so and calls
// SDL_main on a Java thread whose stack is far too small for chained AOT
// calls, and an app cannot restart itself with a larger one the way the Linux
// executable does. So the game runs on a thread of its own with the 64 MiB
// stack the other platforms give their main thread.

#include "app_paths.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <pthread.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

int yakumo_main(int argc, char **argv);

namespace {

constexpr std::size_t kGameStackBytes = 64u * 1024u * 1024u;

struct Launch {
    int argc;
    char **argv;
    int result;
};

void *run_game(void *argument) {
    auto *launch = static_cast<Launch *>(argument);
    launch->result = yakumo_main(launch->argc, launch->argv);
    return nullptr;
}

// An app has no terminal: what the program prints goes to yakumo.log in its
// internal storage, where `adb shell run-as` and the menu's Save the log
// reach it; the one before is kept as yakumo-previous.log.
void redirect_output() {
    const char *storage = SDL_GetAndroidInternalStoragePath();
    if (storage == nullptr) return;
    const std::string log = std::string(storage) + "/yakumo.log";
    // The previous run's log survives one start, for reporting a crash.
    std::rename(log.c_str(), (std::string(storage) + "/yakumo-previous.log").c_str());
    if (std::freopen(log.c_str(), "w", stdout) != nullptr) std::setvbuf(stdout, nullptr, _IOLBF, 0);
    if (std::freopen(log.c_str(), "a", stderr) != nullptr) std::setvbuf(stderr, nullptr, _IONBF, 0);
}

// The APK carries the fallback font a release ships in fonts/, as an asset.
// The font code maps files, so it is unpacked to the app's storage once and
// found there.
constexpr const char *kBundledFonts[] = {"NotoSansCJKjp-Regular.otf"};

void unpack_bundled_fonts() {
    const char *storage = SDL_GetAndroidInternalStoragePath();
    if (storage == nullptr) return;
    const std::filesystem::path root = std::filesystem::path(storage) / "bundled";
    std::error_code ec;
    std::filesystem::create_directories(root / "fonts", ec);
    for (const char *name : kBundledFonts) {
        const std::filesystem::path target = root / "fonts" / name;
        const std::string asset = std::string("fonts/") + name;
        SDL_IOStream *stream = SDL_IOFromFile(asset.c_str(), "rb");
        if (stream == nullptr) continue;
        const Sint64 size = SDL_GetIOSize(stream);
        if (size > 0 && std::filesystem::file_size(target, ec) == static_cast<std::uintmax_t>(size)) {
            SDL_CloseIO(stream);
            continue;
        }
        std::size_t length = 0;
        void *data = SDL_LoadFile_IO(stream, &length, true);
        if (data == nullptr) continue;
        const std::filesystem::path partial = target.string() + ".partial";
        {
            std::ofstream out(partial, std::ios::binary | std::ios::trunc);
            out.write(static_cast<const char *>(data), static_cast<std::streamsize>(length));
        }
        SDL_free(data);
        std::filesystem::rename(partial, target, ec);
    }
    mhp3rd::set_bundled_resource_directory(root);
}

} // namespace

int main(int argc, char *argv[]) {
    redirect_output();
    // Back opens the in-game menu instead of leaving the app.
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    unpack_bundled_fonts();
    Launch launch{argc, argv, 1};
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setstacksize(&attributes, kGameStackBytes);
    pthread_t thread;
    if (pthread_create(&thread, &attributes, run_game, &launch) != 0) {
        std::fprintf(stderr, "Yakumo: cannot start the game thread with a %zu MiB stack\n", kGameStackBytes >> 20);
        pthread_attr_destroy(&attributes);
        return 1;
    }
    pthread_attr_destroy(&attributes);
    pthread_join(thread, nullptr);
    std::fflush(stdout);
    return launch.result;
}
