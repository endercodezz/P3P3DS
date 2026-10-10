#pragma once
// Optional wall-time accounting (runner --profile) of the parts of a run that
// are not AOT code: interpreter fallback, GE rendering and host file reads
// (VFS sources; they run inside HLE calls). HLE time is kept by
// psprecomp::runtime_profile(). Off by default: no clock reads.
#include <chrono>
#include <cstdint>

namespace p3p3ds {

struct HostProfile {
    bool enabled{};
    // The *_ns fields count `clock` units: steady-clock nanoseconds by
    // default; a host with a cheaper counter sets clock and ns_per_unit (New
    // 3DS: raw system ticks, no division per measurement).
    std::uint64_t (*clock)() noexcept = nullptr;
    double ns_per_unit{1.0};
    std::uint64_t interpreter_ns{}, interpreter_entries{};
    std::uint64_t render_ns{}, render_calls{};
    std::uint64_t io_ns{}, io_calls{}, io_bytes{};
    std::uint64_t ge_commands{}; // GE display-list commands executed (counted always, one add per command)
    // Guest time between a list enqueue and the game's next wait for the GE
    // (sceGeDrawSync / sceGeListSync mode 0): what a GE back end on another
    // core could overlap. And the GE finish callbacks' own guest code.
    std::uint64_t ge_window_ns{}, ge_windows{}, ge_callback_ns{}, ge_callbacks{};
    std::uint32_t render_sample{}; // sampled render scopes (SampledProfileScope)
};

inline HostProfile &host_profile() noexcept {
    static HostProfile profile;
    return profile;
}

inline std::uint64_t profile_now(const HostProfile &p) noexcept {
    if (p.clock != nullptr) return p.clock();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

// Adds the scope's duration to `ns` and increments `calls` when profiling is on.
class ProfileScope {
public:
    ProfileScope(std::uint64_t &ns, std::uint64_t &calls) noexcept
        : ns_(ns), calls_(calls), on_(host_profile().enabled) {
        if (on_) start_ = profile_now(host_profile());
    }
    ~ProfileScope() {
        if (!on_) return;
        ns_ += profile_now(host_profile()) - start_;
        ++calls_;
    }
    ProfileScope(const ProfileScope &) = delete;
    ProfileScope &operator=(const ProfileScope &) = delete;

private:
    std::uint64_t &ns_, &calls_;
    bool on_;
    std::uint64_t start_{};
};

// For frequent scopes of similar cost (one per GE primitive): every call is
// counted, one in 2^kShift is timed and its duration added 2^kShift times.
class SampledProfileScope {
public:
    static constexpr std::uint32_t kShift = 3;
    SampledProfileScope(std::uint64_t &ns, std::uint64_t &calls) noexcept : ns_(ns) {
        auto &p = host_profile();
        if (!p.enabled) return;
        ++calls;
        on_ = (++p.render_sample & ((1u << kShift) - 1u)) == 0u;
        if (on_) start_ = profile_now(p);
    }
    ~SampledProfileScope() {
        if (on_) ns_ += (profile_now(host_profile()) - start_) << kShift;
    }
    SampledProfileScope(const SampledProfileScope &) = delete;
    SampledProfileScope &operator=(const SampledProfileScope &) = delete;

private:
    std::uint64_t &ns_;
    bool on_{};
    std::uint64_t start_{};
};

} // namespace p3p3ds
