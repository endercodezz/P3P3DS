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
    std::uint64_t interpreter_ns{}, interpreter_entries{};
    std::uint64_t render_ns{}, render_calls{};
    std::uint64_t io_ns{}, io_calls{}, io_bytes{};
};

inline HostProfile &host_profile() noexcept {
    static HostProfile profile;
    return profile;
}

// Adds the scope's duration to `ns` and increments `calls` when profiling is on.
class ProfileScope {
public:
    ProfileScope(std::uint64_t &ns, std::uint64_t &calls) noexcept
        : ns_(ns), calls_(calls), on_(host_profile().enabled) {
        if (on_) start_ = std::chrono::steady_clock::now();
    }
    ~ProfileScope() {
        if (!on_) return;
        ns_ += static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_).count());
        ++calls_;
    }
    ProfileScope(const ProfileScope &) = delete;
    ProfileScope &operator=(const ProfileScope &) = delete;

private:
    std::uint64_t &ns_, &calls_;
    bool on_;
    std::chrono::steady_clock::time_point start_{};
};

} // namespace p3p3ds
