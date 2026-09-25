#include "kernel/load_trace.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace mhp3rd::load_trace {
namespace {

using Clock = std::chrono::steady_clock;

struct State {
    Clock::time_point interval_start{Clock::now()};
    Clock::time_point last_switch{Clock::now()};
    Clock::time_point run_start{Clock::now()};
    std::uint64_t interval_emulated_start{};
    bool emulated_known{};
    std::string current{"idle"};
    std::map<std::string, double> busy_ms;
    std::size_t disc_bytes{};
    std::size_t disc_reads{};
    std::size_t stick_bytes{};
    std::size_t flips{};
    double pacing_ms{};
    int audio_peak{};
    std::size_t audio_buffers{};
};

State &state() {
    static State instance;
    return instance;
}

} // namespace

bool enabled() {
    static const bool on = std::getenv("MHP3RD_TRACE_LOAD") != nullptr;
    return on;
}

void run_as(std::string_view who) {
    if (!enabled()) return;
    State &s = state();
    const Clock::time_point now = Clock::now();
    s.busy_ms[s.current] += std::chrono::duration<double, std::milli>(now - s.last_switch).count();
    s.last_switch = now;
    s.current.assign(who);
}

void note_disc_read(std::size_t bytes) {
    if (!enabled()) return;
    state().disc_bytes += bytes;
    ++state().disc_reads;
}

void note_memory_stick_read(std::size_t bytes) {
    if (enabled()) state().stick_bytes += bytes;
}

void note_flip() {
    if (enabled()) ++state().flips;
}

void note_audio_peak(int peak) {
    if (!enabled()) return;
    state().audio_peak = std::max(state().audio_peak, peak);
    ++state().audio_buffers;
}

void note_pacing_sleep(double milliseconds) {
    if (enabled()) state().pacing_ms += milliseconds;
}

void tick(std::uint64_t emulated_us) {
    if (!enabled()) return;
    State &s = state();
    if (!s.emulated_known) {
        s.emulated_known = true;
        s.interval_emulated_start = emulated_us;
    }
    const Clock::time_point now = Clock::now();
    const double real_ms = std::chrono::duration<double, std::milli>(now - s.interval_start).count();
    if (real_ms < 250.0) return;
    run_as(s.current);
    std::vector<std::pair<std::string, double>> busy(s.busy_ms.begin(), s.busy_ms.end());
    std::sort(busy.begin(), busy.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
    std::string threads;
    for (std::size_t i = 0; i < busy.size() && i < 5u; ++i) {
        if (busy[i].second < 1.0) break;
        char part[96];
        std::snprintf(part, sizeof(part), " %s=%.0f", busy[i].first.c_str(), busy[i].second);
        threads += part;
    }
    const double at_s = std::chrono::duration<double>(now - s.run_start).count();
    std::printf("[loadtrace] t=%.2fs real=%.0fms emu=%.0fms flips=%zu disc=%zuKiB/%zu ms0=%zuKiB pace=%.0fms audio=%d/%zu |%s\n",
                at_s, real_ms, static_cast<double>(emulated_us - s.interval_emulated_start) / 1000.0, s.flips,
                s.disc_bytes / 1024u, s.disc_reads, s.stick_bytes / 1024u, s.pacing_ms, s.audio_peak, s.audio_buffers, threads.c_str());
    std::fflush(stdout);
    s.interval_start = now;
    s.interval_emulated_start = emulated_us;
    s.busy_ms.clear();
    s.disc_bytes = 0u;
    s.disc_reads = 0u;
    s.stick_bytes = 0u;
    s.flips = 0u;
    s.pacing_ms = 0.0;
    s.audio_peak = 0;
    s.audio_buffers = 0u;
}

} // namespace mhp3rd::load_trace
