#pragma once

#include <cstdint>

// Android's performance hints (ADPF, APerformanceHint, Android 13 and later).
// The game's thread works in a burst each frame and then sleeps until real
// time catches up, so on its own the kernel sees a thread that is mostly idle
// and keeps its core's clock low; each frame's burst then runs slowly. Telling
// the system how long a frame's work took, against the time it has, lets it
// raise the clock before frames run late. Only in the Android app.
namespace mhp3rd::android {

// Called once per game frame on the game's thread: `work_ns` is the time the
// frame kept the thread busy (not the time it slept to hold real time).
// Opens the session on the first call; does nothing where the system offers
// no hints or MHP3RD_NO_PERFORMANCE_HINT is set.
void report_frame_work(std::int64_t work_ns);

} // namespace mhp3rd::android
