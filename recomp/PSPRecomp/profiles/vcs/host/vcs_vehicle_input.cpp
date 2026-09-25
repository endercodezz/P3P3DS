#include "vcs_vehicle_input.hpp"

#include "vcs_config.hpp"

#include <atomic>

namespace vcs {
namespace {

// CPad layout, from ThirteenAG's struct: two bytes of padding, then
// CControllerState as sixteen-bit fields in a fixed order. Confirmed against
// the disassembly -- the stock accelerator loads 0x2A and the stock brake loads
// 0x26, which is exactly where CROSS and SQUARE land.
constexpr std::uint32_t kPadCross = 0x2Au;
constexpr std::uint32_t kPadSquare = 0x26u;
constexpr std::uint32_t kPadRightShoulder1 = 0x0Eu;
constexpr std::uint32_t kPadLeftShoulder1 = 0x0Au;

std::atomic<bool> g_accelerate{false};
std::atomic<bool> g_brake{false};

// Counts reads of the vehicle control accessors; see vcs_player_in_vehicle.
std::atomic<std::uint64_t> g_vehicle_reads{0u};

// Roughly a third of a second at 60 Hz. Long enough to bridge the gap between
// the guest's pad poll and the host's, short enough that stepping off a bike
// gives the on-foot mapping back before the player can notice.
constexpr std::uint64_t kVehicleHoldPolls = 20u;

} // namespace

bool vcs_modern_control_scheme_enabled() noexcept {
    static const bool value = [] {
        const VcsConfiguration &config = vcs_configuration();
        return config.initialized && config.controls.modern_control_scheme;
    }();
    return value;
}

std::uint32_t vcs_accelerate_pad_offset() noexcept {
    return vcs_modern_control_scheme_enabled() ? kPadRightShoulder1 : kPadCross;
}

std::uint32_t vcs_brake_pad_offset() noexcept {
    return vcs_modern_control_scheme_enabled() ? kPadLeftShoulder1 : kPadSquare;
}

bool vcs_host_accelerate() noexcept { return g_accelerate.load(std::memory_order_relaxed); }
bool vcs_host_brake() noexcept { return g_brake.load(std::memory_order_relaxed); }

void vcs_set_host_drive_inputs(bool accelerate, bool brake) noexcept {
    g_accelerate.store(accelerate, std::memory_order_relaxed);
    g_brake.store(brake, std::memory_order_relaxed);
}

void vcs_note_vehicle_control_read() noexcept {
    g_vehicle_reads.fetch_add(1u, std::memory_order_relaxed);
}

bool vcs_player_in_vehicle() noexcept {
    // Called from the input poll, so the poll itself is the clock: remember the
    // count and how many polls ago it last moved.
    static std::uint64_t last_seen = 0u;
    static std::uint64_t quiet_polls = kVehicleHoldPolls;
    const std::uint64_t reads = g_vehicle_reads.load(std::memory_order_relaxed);
    if (reads != last_seen) {
        last_seen = reads;
        quiet_polls = 0u;
    } else if (quiet_polls < kVehicleHoldPolls) {
        ++quiet_polls;
    }
    return quiet_polls < kVehicleHoldPolls;
}

} // namespace vcs
