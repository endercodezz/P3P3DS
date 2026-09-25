#include "frame_pacing.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <cmath>

namespace mhp3rd::gpu::pacing {
namespace {

// A flip this far off the grid of frame moments moves the grid to it: the
// kernel's hold to real time started over after a pause or a slow frame.
constexpr std::int64_t kRealignUs = 2000;
// A gap of this many frames between two flips (a load, a pause) starts over.
constexpr std::int64_t kGapFrames = 10;

} // namespace

double presents_per_frame(double rate) noexcept {
    const double presents = rate / 30.0;
    // A display that reports 59.94 or 89.9 Hz means 2 or 3.
    const double whole = std::round(presents);
    return std::fabs(presents - whole) < 0.03 ? whole : presents;
}

double plain_presents_per_frame(double rate) noexcept {
    const double presents = presents_per_frame(rate);
    if (presents <= 1.0) return 1.0;
    // The grid meets a frame's moment every q frames for the smallest q that
    // makes q * presents whole.
    for (int q = 1; q <= 10; ++q) {
        const double over_q = presents * q;
        if (std::fabs(over_q - std::round(over_q)) < 1e-3) return 1.0 / q;
    }
    return 0.0;
}

void PresentClock::set_presents_per_frame(double presents) noexcept {
    if (presents == per_frame_) return;
    per_frame_ = presents;
    first_step_us_ = frame_us_;
    if (presents > 1.0) {
        // The grid repeats after the few frames plain_presents_per_frame
        // finds; the smallest offset of a grid moment into its frame over
        // them is the first step.
        const double step = static_cast<double>(frame_us_) / presents;
        for (int index = 1; index <= 64; ++index) {
            const auto offset = static_cast<std::int64_t>(std::llround(index * step)) % frame_us_;
            if (offset > 0) first_step_us_ = std::min(first_step_us_, offset);
        }
    }
    // The grid's spacing changed: start it again at the newest frame.
    anchored_ = false;
    if (has_newer_) {
        anchored_ = true;
        anchor_us_ = newer_us_;
        next_index_ = 0;
    }
}

void PresentClock::reset() noexcept {
    has_newer_ = has_older_ = anchored_ = false;
    next_index_ = 0;
}

std::int64_t PresentClock::delay_us() const noexcept { return frame_us_ - first_step_us_ + work_us_; }

void PresentClock::flip(std::int64_t time_us, std::int64_t now_us) noexcept {
    if (has_newer_ && (time_us <= newer_us_ || time_us - newer_us_ > kGapFrames * frame_us_)) reset();
    // The code time a present allows for: the (kWorkRank + 1)th longest of
    // the window, so that a few late frames do not keep every present late.
    const std::int64_t work = std::clamp<std::int64_t>(now_us - time_us, 0, frame_us_) + kWorkMarginUs;
    if (work_window_.size() < kWorkWindowFrames) {
        work_window_.push_back(work);
    } else {
        work_window_[work_cursor_] = work;
        work_cursor_ = (work_cursor_ + 1u) % kWorkWindowFrames;
    }
    std::array<std::int64_t, kWorkWindowFrames> sorted{};
    std::copy(work_window_.begin(), work_window_.end(), sorted.begin());
    const std::size_t count = work_window_.size();
    const std::size_t rank = std::min(count - 1u, count * kWorkRank / kWorkWindowFrames);
    std::nth_element(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(rank), sorted.begin() + static_cast<std::ptrdiff_t>(count),
                     std::greater<>{});
    work_us_ = sorted[rank];
    if (has_newer_) {
        older_us_ = newer_us_;
        has_older_ = true;
    }
    newer_us_ = time_us;
    has_newer_ = true;
    if (anchored_) {
        const std::int64_t offset = ((time_us - anchor_us_) % frame_us_ + frame_us_) % frame_us_;
        if (std::min(offset, frame_us_ - offset) <= kRealignUs) return;
    }
    anchored_ = true;
    anchor_us_ = time_us;
    next_index_ = 0;
}

std::int64_t PresentClock::moment(std::int64_t index) const noexcept {
    return anchor_us_ + std::llround(static_cast<double>(index) * static_cast<double>(frame_us_) / per_frame_);
}

std::optional<std::int64_t> PresentClock::next_due() const noexcept {
    if (!has_newer_ || per_frame_ <= 1.0) return std::nullopt;
    return due(next_index_);
}

float PresentClock::blend_at(std::int64_t time_us) const noexcept {
    if (!has_older_ || newer_us_ <= older_us_) return 1.0f;
    const double t = static_cast<double>(time_us - older_us_) / static_cast<double>(newer_us_ - older_us_);
    return static_cast<float>(std::clamp(t, 0.0, 1.0));
}

std::optional<PresentClock::Present> PresentClock::take(std::int64_t now_us) noexcept {
    if (!has_newer_ || per_frame_ <= 1.0 || due(next_index_) > now_us) return std::nullopt;
    const double step = static_cast<double>(frame_us_) / per_frame_;
    std::int64_t index =
        next_index_ + static_cast<std::int64_t>(std::floor(static_cast<double>(now_us - due(next_index_)) / step));
    while (due(index + 1) <= now_us) ++index;
    while (index > next_index_ && due(index) > now_us) --index;
    Present present{};
    present.time_us = moment(index);
    present.t = blend_at(present.time_us);
    present.skipped = static_cast<std::uint32_t>(index - next_index_);
    next_index_ = index + 1;
    return present;
}

void RateGovernor::set_requested(double rate) {
    requested_ = std::max(rate, 30.0);
    ladder_.clear();
    for (const double known : kRates)
        if (known < requested_ - 0.5) ladder_.push_back(known);
    ladder_.push_back(requested_);
    index_ = ladder_.size() - 1u;
    blocked_.assign(ladder_.size(), 0);
    block_length_.assign(ladder_.size(), kBlockSeconds);
    slow_seconds_ = skipping_seconds_ = spare_seconds_ = wait_up_ = 0;
    reason_ = "";
}

void RateGovernor::set_automatic(bool automatic) {
    if (automatic_ == automatic) return;
    automatic_ = automatic;
    set_requested(requested_);
}

double RateGovernor::rate() const noexcept { return ladder_[index_]; }

double RateGovernor::cost_ms(double rate, const Second &second) noexcept {
    const double presents = presents_per_frame(rate);
    if (presents <= 1.0) return 0.0;
    const double plain = plain_presents_per_frame(rate);
    return (presents - plain) * second.blend_ms + plain * second.plain_ms;
}

void RateGovernor::step_to(std::size_t index, const char *why) {
    if (index < index_) {
        // The rate left behind waits before it is tried again, longer each
        // time it fails.
        blocked_[index_] = block_length_[index_];
        block_length_[index_] = std::min(block_length_[index_] * 2, kMaxBlockSeconds);
        wait_up_ = kSecondsAfterDown;
    }
    index_ = index;
    slow_seconds_ = skipping_seconds_ = spare_seconds_ = 0;
    reason_ = why;
}

bool RateGovernor::update(const Second &second) {
    for (int &seconds : blocked_) seconds = std::max(0, seconds - 1);
    wait_up_ = std::max(0, wait_up_ - 1);
    if (!automatic_ || (index_ == 0u && ladder_.size() == 1u)) return false;
    const double current_cost = cost_ms(rate(), second);

    // Behind real time, or keeping up only by using every moment, while
    // presents cost something: drop at once to the fastest rate whose
    // presents fit into what the game is short of. Without spare time the
    // game's frames come later and later after their moments, and the
    // presents waiting for them are skipped.
    // A load or a stall slows the game too, with presents that cost little:
    // they are blamed only when they take at least half of what is missing.
    const bool slow = second.speed < kSlowSpeed;
    const bool no_spare = second.idle_ms < kMinIdleMs;
    const double missing_ms = std::max(0.0, 1.0 - second.speed) * static_cast<double>(kGameFrameUs) / 1000.0 +
                              std::max(0.0, kMinIdleMs - second.idle_ms);
    if (index_ > 0u && (slow || no_spare) && second.interpolation_ms >= std::max(kMinCostMs, 0.5 * missing_ms) &&
        second.idle_ms < second.interpolation_ms) {
        if (++slow_seconds_ >= 2) {
            std::size_t index = index_ - 1u;
            while (index > 0u && cost_ms(ladder_[index], second) > current_cost - missing_ms - kMarginMs) --index;
            step_to(index, slow ? "the game fell behind real time" : "the game had no time to spare");
            return true;
        }
    } else {
        slow_seconds_ = 0;
    }

    // Presents that come too late to be shown on time: the display or the
    // queue cannot take this many.
    const std::uint32_t due = second.presents + second.skipped + second.blocked;
    if (index_ > 0u && due != 0u &&
        static_cast<double>(second.skipped + second.blocked) > kMaxSkippedShare * due) {
        if (++skipping_seconds_ >= 2) {
            step_to(index_ - 1u, "presents were late");
            return true;
        }
    } else {
        skipping_seconds_ = 0;
    }

    // Spare time for a faster rate's extra presents, for a few seconds: the
    // fastest one the costs measured so far say fits, and that is not
    // waiting after it failed.
    std::size_t faster = index_;
    if (wait_up_ == 0 && second.speed >= kSteadySpeed) {
        for (std::size_t index = index_ + 1u; index < ladder_.size(); ++index) {
            if (blocked_[index] != 0) break;
            if (second.idle_ms < cost_ms(ladder_[index], second) - current_cost + kMarginMs) break;
            faster = index;
        }
    }
    if (faster != index_) {
        if (++spare_seconds_ >= kSecondsBeforeUp) {
            step_to(faster, "there was time to spare");
            return true;
        }
    } else {
        spare_seconds_ = 0;
    }
    return false;
}

} // namespace mhp3rd::gpu::pacing
