#include "kernel/fast_loading.hpp"

#include "hle/hle_common.hpp"
#include "settings/settings.hpp"
#include "adhoc/session.hpp"
#if defined(MHP3RD_HAS_RENDERER)
#include "ui/ui.hpp"
#endif

#include <chrono>
#include <cstdio>
#include <cstdlib>

namespace mhp3rd::fast_loading {

namespace {

using Clock = std::chrono::steady_clock;

struct State {
    Detector detector;
    bool buttons_held{};
    // The episode running fast now: when it started, in both clocks.
    Clock::time_point real_start{};
    std::uint64_t emulated_start{};
    // Everything saved in this run.
    double saved_ms{};
    std::uint64_t episodes{};
};

State &state() {
    static State instance;
    return instance;
}

bool windowed() {
    static const bool value = std::getenv("MHP3RD_NO_RENDER") == nullptr;
    return value;
}

Guards sample_guards() {
    const settings::Settings &s = settings::current();
    Guards guards;
    guards.enabled = s.fast_loading && !s.unthrottled && windowed();
    guards.buttons_held = state().buttons_held;
    guards.movie = mpeg_active();
    guards.online = adhoc_networking_on() || adhoc_session_active();
#if defined(MHP3RD_HAS_RENDERER)
    guards.menu = ui::menu_over_game();
#endif
    return guards;
}

void begin_episode() {
    State &s = state();
    s.real_start = Clock::now();
    s.emulated_start = kernel().now_us();
}

void end_episode(Reason reason) {
    State &s = state();
    const double real_ms = std::chrono::duration<double, std::milli>(Clock::now() - s.real_start).count();
    const double emulated_ms = static_cast<double>(kernel().now_us() - s.emulated_start) / 1000.0;
    const double saved = emulated_ms > real_ms ? emulated_ms - real_ms : 0.0;
    s.saved_ms += saved;
    ++s.episodes;
    std::printf("[load] fast %.0f ms of game time in %.0f ms real, %.0f ms saved (%s; %.1f s saved so far)\n",
                emulated_ms, real_ms, saved, reason_name(reason), s.saved_ms / 1000.0);
    std::fflush(stdout);
}

} // namespace

void note_disc_read() { state().detector.disc_read(kernel().now_us()); }

bool note_audio(int peak) {
    Detector &detector = state().detector;
    const bool was_fast = detector.fast();
    const bool drop = detector.audio(kernel().now_us(), peak);
    if (was_fast && !detector.fast()) end_episode(detector.reason());
    return drop;
}

void note_buttons(bool held) { state().buttons_held = held; }

void update() {
    Detector &detector = state().detector;
    const bool was_fast = detector.fast();
    const bool fast = detector.update(kernel().now_us(), sample_guards());
    if (fast && !was_fast) begin_episode();
    else if (!fast && was_fast) end_episode(detector.reason());
}

bool active() { return state().detector.fast(); }

} // namespace mhp3rd::fast_loading
