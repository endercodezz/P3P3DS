#include "platform/android_performance.hpp"

#include <dlfcn.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>

namespace mhp3rd::android {
namespace {

// The time a game frame's work should take: three quarters of the PSP's
// 29.97 frames a second, so the system raises the clock while a frame still
// has room rather than once it is late.
constexpr std::int64_t kTargetWorkNs = 25'000'000;

// Resolved at run time: the functions exist from API 33, and the app runs on
// Android 10 and later.
struct Hints {
    using GetManager = void *(*)();
    using CreateSession = void *(*)(void *, const std::int32_t *, std::size_t, std::int64_t);
    using ReportActual = int (*)(void *, std::int64_t);
    void *session{};
    ReportActual report{};
    bool opened{};
};

Hints &hints() {
    static Hints value;
    return value;
}

void open(Hints &hints) {
    hints.opened = true;
    if (std::getenv("MHP3RD_NO_PERFORMANCE_HINT") != nullptr) {
        std::cout << "[perf] performance hints off (MHP3RD_NO_PERFORMANCE_HINT)\n";
        return;
    }
    void *library = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) return;
    const auto get_manager = reinterpret_cast<Hints::GetManager>(dlsym(library, "APerformanceHint_getManager"));
    const auto create_session =
        reinterpret_cast<Hints::CreateSession>(dlsym(library, "APerformanceHint_createSession"));
    hints.report = reinterpret_cast<Hints::ReportActual>(dlsym(library, "APerformanceHint_reportActualWorkDuration"));
    if (get_manager == nullptr || create_session == nullptr || hints.report == nullptr) {
        std::cout << "[perf] no performance hints on this Android version\n";
        return;
    }
    void *manager = get_manager();
    const std::int32_t thread = static_cast<std::int32_t>(gettid());
    hints.session = manager != nullptr ? create_session(manager, &thread, 1u, kTargetWorkNs) : nullptr;
    std::cout << "[perf] performance hints " << (hints.session != nullptr ? "on" : "unavailable")
              << " for the game's thread, target " << kTargetWorkNs / 1'000'000 << " ms of work a frame\n";
}

} // namespace

void report_frame_work(std::int64_t work_ns) {
    Hints &state = hints();
    if (!state.opened) open(state);
    if (state.session == nullptr || work_ns <= 0) return;
    state.report(state.session, work_ns);
}

} // namespace mhp3rd::android
