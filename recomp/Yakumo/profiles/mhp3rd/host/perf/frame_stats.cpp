#include "frame_stats.hpp"

#include "settings/settings.hpp"

#if defined(MHP3RD_ANDROID_APP)
#include "platform/android_performance.hpp"
#endif

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace mhp3rd::perf {
namespace {

double to_ms(Clock::duration duration) {
    return std::chrono::duration<double, std::milli>(duration).count();
}

constexpr std::size_t kStallKinds = static_cast<std::size_t>(Stall::Count);

struct StallTally {
    std::uint32_t count{};
    Clock::duration total{};
    Clock::duration longest{};

    void add(Clock::duration duration) {
        ++count;
        total += duration;
        longest = std::max(longest, duration);
    }
    void add(const StallTally &other) {
        count += other.count;
        total += other.total;
        longest = std::max(longest, other.longest);
    }
};
using StallTallies = std::array<StallTally, kStallKinds>;

// MHP3RD_TRACE_STALLS: a [stalls] line once a second, and a [slow-frame] line
// for every frame longer than MHP3RD_TRACE_STALLS_MS (default 40 ms).
struct StallTrace {
    bool enabled{};
    double slow_frame_ms{40.0};
};

struct Alternate {
    std::uint32_t paths{};  // bit per NewPath named in MHP3RD_PERF_ALTERNATE
    bool off{};             // the second in progress takes the old paths
};

Alternate &alternate() {
    static Alternate value = [] {
        Alternate result{};
        const char *text = std::getenv("MHP3RD_PERF_ALTERNATE");
        if (text == nullptr) return result;
        const std::string names = std::string(",") + text + ",";
        const char *known[] = {"direct", "lookup", "reuse", "merge", "store", "decode", "alpha", "uploads", "clearload",
                               "gpudecode", "texturedecode"};
        static_assert(sizeof(known) / sizeof(known[0]) == static_cast<std::size_t>(NewPath::Count));
        for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(NewPath::Count); ++i)
            if (names.find(std::string(",") + known[i] + ",") != std::string::npos) result.paths |= 1u << i;
        return result;
    }();
    return value;
}

const StallTrace &stall_trace() {
    static const StallTrace value = [] {
        StallTrace trace{};
        trace.enabled = std::getenv("MHP3RD_TRACE_STALLS") != nullptr;
        if (const char *text = std::getenv("MHP3RD_TRACE_STALLS_MS"); text != nullptr) {
            const double ms = std::strtod(text, nullptr);
            if (ms > 0.0) trace.slow_frame_ms = ms;
        }
        return trace;
    }();
    return value;
}

struct State {
    // Frame in progress.
    Clock::time_point frame_start{Clock::now()};
    Clock::time_point frame_start_previous{Clock::now()};  // of the frame end_frame() closes
    Clock::duration render{};
    Clock::duration wait{};
    Clock::duration pacing{};
    Clock::duration overlay{};
    std::uint32_t lists{};
    std::uint32_t draws{};
    Clock::time_point last_present{Clock::now()};
    double frame_rate{};
    double requested_rate{};
    std::uint32_t recorded_draws{};
    StallTallies stalls{};
    double gpu_ms{};
    std::uint32_t gpu_samples{};

    // Second in progress.
    Clock::time_point window_start{Clock::now()};
    std::uint64_t window_virtual_us{};
    bool window_has_clock{};
    std::uint32_t frames{};    // guest flips
    double frame_sum_ms{};     // flip to flip
    std::uint32_t presents{};
    double present_sum_ms{};   // present to present
    double present_max_ms{};
    Clock::duration render_sum{};
    Clock::duration wait_sum{};
    Clock::duration pacing_sum{};
    Clock::duration overlay_sum{};
    std::uint32_t list_sum{};
    std::uint64_t draw_sum{};
    std::uint64_t recorded_draw_sum{};
    std::uint64_t pass_sum{};
    std::uint64_t cleared_pass_sum{};
    std::uint64_t copy_sum{};
    std::uint64_t vertex_peak{};
    std::uint64_t index_peak{};
    StallTallies stall_sum{};
    double gpu_sum_ms{};
    double gpu_max_ms{};
    std::uint32_t gpu_frames{};
    bool gpu_unavailable{};
    std::uint64_t frame_number{};

    Summary summary;
    std::string present_mode{"none"};
    std::uint32_t width{};
    std::uint32_t height{};
    float refresh_hz{};
    std::array<float, kHistoryFrames> history{};
    std::size_t cursor{};
};

State &state() {
    static State value;
    return value;
}

void print(const Summary &s) {
    char line[512];
    int length = std::snprintf(line, sizeof(line),
                               "[perf] fps %.1f game %.1f speed %.0f%% | frame avg %.1f max %.1f ms | guest %.1f "
                               "render %.1f wait %.1f ms | lists %.0f/s draws %.0f/%.0f | %s %ux%u",
                               s.fps, s.game_fps, s.speed * 100.0, s.frame_avg_ms, s.frame_max_ms, s.guest_ms,
                               s.render_ms, s.wait_ms, s.lists, s.draws, s.recorded_draws, s.present_mode.c_str(),
                               s.width, s.height);
    if (s.refresh_hz > 0.0f && length > 0 && static_cast<std::size_t>(length) < sizeof(line))
        length += std::snprintf(line + length, sizeof(line) - length, " %.0fHz", s.refresh_hz);
    if (length > 0 && static_cast<std::size_t>(length) < sizeof(line)) {
        if (s.gpu_valid)
            length += std::snprintf(line + length, sizeof(line) - length, " | gpu %.1f max %.1f ms", s.gpu_avg_ms,
                                    s.gpu_max_ms);
        else
            length += std::snprintf(line + length, sizeof(line) - length, " | gpu n/a");
    }
    if (s.frame_rate > 0.0 && length > 0 && static_cast<std::size_t>(length) < sizeof(line)) {
        if (s.requested_rate > s.frame_rate + 0.5)
            length += std::snprintf(line + length, sizeof(line) - length, " | interpolation %.0f of %.0f", s.frame_rate,
                                    s.requested_rate);
        else
            length += std::snprintf(line + length, sizeof(line) - length, " | interpolation %.0f", s.frame_rate);
    }
    if (s.vertex_mib > 0.0 && length > 0 && static_cast<std::size_t>(length) < sizeof(line))
        length += std::snprintf(line + length, sizeof(line) - length, " | space vertex %.1f index %.1f MiB",
                                s.vertex_mib, s.index_mib);
    if (s.passes > 0.0 && length > 0 && static_cast<std::size_t>(length) < sizeof(line))
        length += std::snprintf(line + length, sizeof(line) - length, " | passes %.1f (%.1f cleared) copies %.1f",
                                s.passes, s.cleared_passes, s.copies);
    if (s.overlay_ms > 0.0 && length > 0 && static_cast<std::size_t>(length) < sizeof(line))
        length += std::snprintf(line + length, sizeof(line) - length, " | overlay %.2f ms", s.overlay_ms);
    if (alternate().paths != 0u && length > 0 && static_cast<std::size_t>(length) < sizeof(line))
        std::snprintf(line + length, sizeof(line) - length, " | alt %s", alternate().off ? "off" : "on");
    // Flushed per line: the log is read while the game runs, often through a
    // pipe where stdout would otherwise sit in a block buffer.
    std::cout << line << std::endl;
}

// " name avg max M xN" for every kind that happened: the average per frame
// over `frames` frames, the longest single stall and how many there were. For
// a single frame, " name total xN".
std::string format_stalls(const StallTallies &tallies, double frames, bool per_frame_average) {
    std::string text;
    char part[96];
    for (std::size_t i = 0; i < kStallKinds; ++i) {
        const StallTally &tally = tallies[i];
        if (tally.count == 0u) continue;
        const double total = to_ms(tally.total);
        if (per_frame_average)
            std::snprintf(part, sizeof(part), " %s %.2f max %.2f x%u", stall_name(static_cast<Stall>(i)),
                          total / frames, to_ms(tally.longest), tally.count);
        else
            std::snprintf(part, sizeof(part), " %s %.2f x%u", stall_name(static_cast<Stall>(i)), total, tally.count);
        text += part;
    }
    return text.empty() ? std::string(" none") : text;
}

} // namespace

bool alternate_off(NewPath path) {
    const Alternate &value = alternate();
    return value.off && (value.paths & (1u << static_cast<std::uint32_t>(path))) != 0u;
}

const char *stall_name(Stall kind) {
    switch (kind) {
    case Stall::Fence: return "fence";
    case Stall::Acquire: return "acquire";
    case Stall::Submit: return "submit";
    case Stall::Present: return "present";
    case Stall::Upload: return "upload";
    case Stall::Evict: return "evict";
    case Stall::Readback: return "readback";
    case Stall::Idle: return "idle";
    case Stall::Pacing: return "pacing";
    case Stall::Copy: return "copy";
    case Stall::Store: return "store";
    case Stall::Pipeline: return "pipeline";
    case Stall::Decode: return "decode";
    case Stall::Count: break;
    }
    return "?";
}

Options options() {
    Options result{};
    switch (settings::current().perf) {
    case settings::PerfDisplay::Off: break;
    case settings::PerfDisplay::Overlay: result.overlay = true; break;
    case settings::PerfDisplay::OverlayAndLog: result.overlay = result.log = true; break;
    case settings::PerfDisplay::Log: result.log = true; break;
    }
    return result;
}

void restart_measurement() {
    State &s = state();
    const Clock::time_point now = Clock::now();
    s.frame_start = now;
    s.last_present = now;
    s.render = s.wait = s.pacing = s.overlay = Clock::duration{};
    s.lists = 0u;
    s.draws = s.recorded_draws = 0u;
    s.stalls = StallTallies{};
    s.gpu_ms = 0.0;
    s.gpu_samples = 0u;
    s.window_start = now;
    s.window_has_clock = false;
    s.frames = 0u;
    s.frame_sum_ms = 0.0;
    s.presents = 0u;
    s.present_sum_ms = 0.0;
    s.present_max_ms = 0.0;
    s.render_sum = s.wait_sum = s.pacing_sum = s.overlay_sum = Clock::duration{};
    s.list_sum = 0u;
    s.draw_sum = s.recorded_draw_sum = 0u;
    s.pass_sum = s.copy_sum = s.cleared_pass_sum = 0u;
    s.stall_sum = StallTallies{};
    s.gpu_sum_ms = s.gpu_max_ms = 0.0;
    s.vertex_peak = s.index_peak = 0u;
    s.gpu_frames = 0u;
}

void add_render_time(Clock::duration duration) { state().render += duration; }

namespace {

constexpr std::size_t kSplitKinds = static_cast<std::size_t>(Split::Count);

struct RenderSplit {
    std::array<std::uint64_t, kSplitKinds> ticks{};
    // Counter ticks per millisecond, measured against the steady clock over
    // the first second.
    double ticks_per_ms{};
    std::uint64_t calibration_ticks{};
    Clock::time_point calibration_start{};
};

RenderSplit &render_split() {
    static RenderSplit value;
    return value;
}

// Prints the second's split per game frame; `frames` flips in it.
void report_split(double frames) {
    RenderSplit &split = render_split();
    const std::uint64_t now_ticks = split_ticks();
    const Clock::time_point now = Clock::now();
    if (split.calibration_ticks == 0u) {
        split.calibration_ticks = now_ticks;
        split.calibration_start = now;
        split.ticks = {};
        return;
    }
    if (split.ticks_per_ms == 0.0) {
        const double ms = to_ms(now - split.calibration_start);
        if (ms > 0.0) split.ticks_per_ms = static_cast<double>(now_ticks - split.calibration_ticks) / ms;
    }
    if (split.ticks_per_ms <= 0.0 || frames <= 0.0) return;
    const auto ms = [&](Split kind) {
        return static_cast<double>(split.ticks[static_cast<std::size_t>(kind)]) / split.ticks_per_ms / frames;
    };
    const double lists = ms(Split::Lists), decode = ms(Split::Decode), draw = ms(Split::Draw);
    const double texture = ms(Split::Texture), record = ms(Split::Record), summary = ms(Split::Summary);
    const double replay = ms(Split::Replay);
    std::printf("[render-split] ms per game frame: lists %.2f = parse %.2f + decode %.2f + draw %.2f (host %.2f, "
                "texture %.2f, record %.2f, summary %.2f) | interp %.2f replay %.2f present %.2f writeback %.2f\n",
                lists, std::max(0.0, lists - decode - draw), decode, draw,
                std::max(0.0, draw - texture - record - summary), texture, record, summary, ms(Split::Interp),
                replay, std::max(0.0, ms(Split::Present) - replay), ms(Split::Writeback));
    std::fflush(stdout);
    split.ticks = {};
}

} // namespace

bool split_enabled() noexcept {
    static const bool enabled = std::getenv("MHP3RD_TRACE_RENDER") != nullptr;
    return enabled;
}

std::uint64_t split_ticks() noexcept {
#if defined(__aarch64__) && !defined(_MSC_VER)
    std::uint64_t value;
    asm volatile("mrs %0, cntvct_el0" : "=r"(value));
    return value;
#elif (defined(__x86_64__) || defined(_M_X64)) && !defined(_MSC_VER)
    return __builtin_ia32_rdtsc();
#else
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count());
#endif
}

void add_split(Split kind, std::uint64_t ticks) noexcept { render_split().ticks[static_cast<std::size_t>(kind)] += ticks; }
void add_wait_time(Clock::duration duration, Stall kind) {
    State &s = state();
    s.wait += duration;
    s.stalls[static_cast<std::size_t>(kind)].add(duration);
}
void note_stall(Stall kind, Clock::duration duration) {
    state().stalls[static_cast<std::size_t>(kind)].add(duration);
}
void add_gpu_time(double milliseconds) {
    State &s = state();
    s.gpu_ms += milliseconds;
    ++s.gpu_samples;
}
void set_gpu_time_unavailable() { state().gpu_unavailable = true; }
void add_pacing_time(Clock::duration duration) {
    State &s = state();
    s.pacing += duration;
    s.stalls[static_cast<std::size_t>(Stall::Pacing)].add(duration);
}
void add_overlay_time(Clock::duration duration) { state().overlay += duration; }
void count_display_list() { ++state().lists; }
void count_draw() { ++state().draws; }
void count_recorded_draws(std::uint32_t count) { state().recorded_draws += count; }
void count_render_pass() { ++state().pass_sum; }
void count_target_copy() { ++state().copy_sum; }
void count_cleared_pass() { ++state().cleared_pass_sum; }

void note_frame_space(std::uint64_t vertex_bytes, std::uint64_t index_bytes) {
    State &s = state();
    s.vertex_peak = std::max(s.vertex_peak, vertex_bytes);
    s.index_peak = std::max(s.index_peak, index_bytes);
}

void set_display_info(const std::string &present_mode, std::uint32_t width, std::uint32_t height, float refresh_hz) {
    State &s = state();
    s.present_mode = present_mode;
    s.width = width;
    s.height = height;
    s.refresh_hz = refresh_hz;
}

void count_present() {
    State &s = state();
    const Clock::time_point now = Clock::now();
    const double present_ms = to_ms(now - s.last_present);
    s.last_present = now;
    s.history[s.cursor] = static_cast<float>(present_ms);
    s.cursor = (s.cursor + 1u) % kHistoryFrames;
    ++s.presents;
    s.present_sum_ms += present_ms;
    s.present_max_ms = std::max(s.present_max_ms, present_ms);
}

void set_frame_rate_info(double rate, double requested) {
    State &s = state();
    s.frame_rate = rate;
    s.requested_rate = requested;
}

void end_frame(std::uint64_t virtual_us, bool presented) {
    State &s = state();
    if (presented) count_present();
    const Clock::time_point now = Clock::now();
    const double frame_ms = to_ms(now - s.frame_start);
    s.frame_start_previous = s.frame_start;
    s.frame_start = now;

    if (!s.window_has_clock) {
        s.window_virtual_us = virtual_us;
        s.window_has_clock = true;
    }
    ++s.frames;
    s.frame_sum_ms += frame_ms;
    s.render_sum += s.render;
    s.wait_sum += s.wait;
    s.pacing_sum += s.pacing;
    s.overlay_sum += s.overlay;
    s.list_sum += s.lists;
    s.draw_sum += s.draws;
    s.recorded_draw_sum += s.recorded_draws;
    for (std::size_t i = 0; i < kStallKinds; ++i) s.stall_sum[i].add(s.stalls[i]);
    if (s.gpu_samples != 0u) {
        s.gpu_sum_ms += s.gpu_ms;
        s.gpu_max_ms = std::max(s.gpu_max_ms, s.gpu_ms);
        ++s.gpu_frames;
    }
    ++s.frame_number;
#if defined(MHP3RD_ANDROID_APP)
    // The frame's work, without the sleep that holds the game to real time.
    android::report_frame_work(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - (s.frame_start_previous + s.pacing)).count());
#endif
    const StallTrace &trace = stall_trace();
    if (trace.enabled && frame_ms > trace.slow_frame_ms) {
        // The GPU time is the previous frame's: that is the work a fence wait
        // at the start of this frame was waiting for.
        const double render_ms = std::max(0.0, to_ms(s.render) - to_ms(s.wait));
        const double wait_ms = to_ms(s.wait) + to_ms(s.pacing);
        char head[192];
        std::snprintf(head, sizeof(head),
                      "[slow-frame] %llu %.1f ms | guest %.1f render %.1f wait %.1f ms | gpu(prev) ",
                      static_cast<unsigned long long>(s.frame_number), frame_ms,
                      std::max(0.0, frame_ms - render_ms - wait_ms), render_ms, wait_ms);
        std::string line = head;
        if (s.gpu_samples != 0u) {
            char gpu[32];
            std::snprintf(gpu, sizeof(gpu), "%.1f ms", s.gpu_ms);
            line += gpu;
        } else {
            line += "n/a";
        }
        line += " |" + format_stalls(s.stalls, 1.0, false);
        std::cout << line << std::endl;
    }
    s.render = s.wait = s.pacing = s.overlay = Clock::duration{};
    s.lists = 0u;
    s.draws = s.recorded_draws = 0u;
    s.stalls = StallTallies{};
    s.gpu_ms = 0.0;
    s.gpu_samples = 0u;

    const double window_ms = to_ms(now - s.window_start);
    if (window_ms < 1000.0) return;

    Summary &out = s.summary;
    const double frames = static_cast<double>(s.frames);
    const double virtual_ms = static_cast<double>(virtual_us - s.window_virtual_us) / 1000.0;
    out.valid = true;
    ++out.second;
    const double presents = static_cast<double>(s.presents);
    out.fps = presents * 1000.0 / window_ms;
    out.game_fps = virtual_ms > 0.0 ? frames * 1000.0 / virtual_ms : 0.0;
    out.speed = virtual_ms / window_ms;
    out.lists = static_cast<double>(s.list_sum) * 1000.0 / window_ms;
    out.draws = static_cast<double>(s.draw_sum) / frames;
    out.recorded_draws = static_cast<double>(s.recorded_draw_sum) / frames;
    out.passes = static_cast<double>(s.pass_sum) / frames;
    out.cleared_passes = static_cast<double>(s.cleared_pass_sum) / frames;
    out.copies = static_cast<double>(s.copy_sum) / frames;
    out.frame_avg_ms = s.presents != 0u ? s.present_sum_ms / presents : 0.0;
    out.frame_max_ms = s.present_max_ms;
    const double gpu_wait_ms = to_ms(s.wait_sum) / frames;
    out.pacing_ms = to_ms(s.pacing_sum) / frames;
    out.wait_ms = gpu_wait_ms + out.pacing_ms;
    // GPU waits happen inside the timed render calls; count them once. Pacing
    // happens outside them, so subtracting it too hid the render time
    // whenever the game was ahead of real time.
    out.render_ms = std::max(0.0, to_ms(s.render_sum) / frames - gpu_wait_ms);
    out.guest_ms = std::max(0.0, s.frame_sum_ms / frames - out.render_ms - out.wait_ms);
    out.overlay_ms = to_ms(s.overlay_sum) / frames;
    out.gpu_valid = !s.gpu_unavailable && s.gpu_frames != 0u;
    out.gpu_avg_ms = s.gpu_frames != 0u ? s.gpu_sum_ms / static_cast<double>(s.gpu_frames) : 0.0;
    out.gpu_max_ms = s.gpu_max_ms;
    out.vertex_mib = static_cast<double>(s.vertex_peak) / (1024.0 * 1024.0);
    out.index_mib = static_cast<double>(s.index_peak) / (1024.0 * 1024.0);
    out.present_mode = s.present_mode;
    out.width = s.width;
    out.height = s.height;
    out.refresh_hz = s.refresh_hz;
    out.frame_rate = s.frame_rate;
    out.requested_rate = s.requested_rate;
    if (options().log) print(out);
    if (split_enabled()) report_split(frames);
    // A phone has no environment to set MHP3RD_TRACE_STALLS in: there the
    // [stalls] line comes with the [perf] line (Performance: Log), so a log a
    // player saves from the menu says where the frame time went.
#if defined(__ANDROID__)
    const bool stalls_line = trace.enabled || options().log;
#else
    const bool stalls_line = trace.enabled;
#endif
    if (stalls_line)
        std::cout << "[stalls] ms per frame over " << s.frames << " frames:" << format_stalls(s.stall_sum, frames, true)
                  << std::endl;

    s.window_start = now;
    if (alternate().paths != 0u) alternate().off = !alternate().off;
    s.window_virtual_us = virtual_us;
    s.frames = 0u;
    s.frame_sum_ms = 0.0;
    s.presents = 0u;
    s.present_sum_ms = 0.0;
    s.present_max_ms = 0.0;
    s.render_sum = s.wait_sum = s.pacing_sum = s.overlay_sum = Clock::duration{};
    s.list_sum = 0u;
    s.draw_sum = s.recorded_draw_sum = 0u;
    s.pass_sum = s.copy_sum = s.cleared_pass_sum = 0u;
    s.stall_sum = StallTallies{};
    s.gpu_sum_ms = s.gpu_max_ms = 0.0;
    s.vertex_peak = s.index_peak = 0u;
    s.gpu_frames = 0u;
}

const Summary &last_second() { return state().summary; }
const std::array<float, kHistoryFrames> &frame_history() { return state().history; }
std::size_t history_cursor() { return state().cursor; }

} // namespace mhp3rd::perf
