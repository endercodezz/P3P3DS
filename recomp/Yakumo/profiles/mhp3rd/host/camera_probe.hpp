#pragma once

// MHP3RD_FIND_CAMERA: finds the guest variables the game keeps its camera in.
//
// The game's camera is only observable from outside through the view matrix it
// uploads to the GE, which MHP3RD_TRACE_CAMERA turns into a yaw and a turn per
// frame. This walks guest RAM alongside that measurement and keeps only the
// words that stay in a fixed proportion to it: a word that always equals
// c * turn is the camera's own turn rate, and one whose change always equals
// c * turn is the camera's angle. Everything else in 32 MB fails within a few
// frames.
//
// Set MHP3RD_FIND_CAMERA=1 and turn the camera; the surviving addresses are
// printed as they thin out. PSPRECOMP_WATCH_WRITE=<address> then names the
// guest code that writes one.

#include <cstdint>

namespace psprecomp {
class Runtime;
}

namespace mhp3rd::probe {

// Called once per presented frame, after the frame's camera has been measured.
// `view_matrix_source` is the display-list address the frame's view matrix was
// uploaded from, which is the one address the host can always name.
void camera_frame(psprecomp::Runtime &runtime, std::uint32_t view_matrix_source);

} // namespace mhp3rd::probe
