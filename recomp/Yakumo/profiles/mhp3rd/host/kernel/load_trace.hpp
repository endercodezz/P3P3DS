#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

// MHP3RD_TRACE_LOAD: what the game does while it loads, a few lines a second.
// Each line covers one interval of real time and says how much emulated time
// passed in it, how many frames the game flipped, how much it read from the
// disc and the memory stick, which of its threads had the CPU and how long the
// host held the game back to real time. Collecting costs nothing while the
// switch is off.
namespace mhp3rd::load_trace {

[[nodiscard]] bool enabled();

// The emulation thread starts running `who` (a guest thread's name,
// "interrupt" or "idle"); the time since the previous call is charged to
// whoever ran before.
void run_as(std::string_view who);
void note_disc_read(std::size_t bytes);
void note_memory_stick_read(std::size_t bytes);
void note_flip();
// The loudest sample of one buffer the game handed to sceAudio.
void note_audio_peak(int peak);
// Real time the kernel slept to hold emulated time to real time.
void note_pacing_sleep(double milliseconds);
// Prints the interval's line when it is due. Called at every vblank.
void tick(std::uint64_t emulated_us);

} // namespace mhp3rd::load_trace
