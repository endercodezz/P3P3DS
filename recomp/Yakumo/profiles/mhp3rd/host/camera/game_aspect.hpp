#pragma once

#include "camera/game_camera.hpp"

#include <span>

namespace psprecomp {
class Runtime;
}

// Gives the game's 3D view the shape of the picture it is drawn into.
//
// NPJB-40001 only. The game keeps its projection's parameters in its camera
// object (the pointer at 0x08A2F958): near plane, far plane, aspect ratio and
// vertical field of view at +0x0, +0x4, +0x8 and +0xC. The aspect ratio starts
// as 480/272, a constant at 0x08969F74 that the camera's initialisation copies
// in, and the projection matrix and the camera's culling planes are both built
// from it. With another aspect ratio the game keeps its vertical field of view
// and widens (or narrows) the horizontal one, and draws that view into its
// 480x272 screen; the renderer then spreads the screen over the window.
namespace mhp3rd::camera {

// Everything the aspect ratio relies on in the game's code, checked before
// anything is written. Exposed for tests.
[[nodiscard]] std::span<const CodeWord> game_aspect_signature();

// Checks the game's code against game_aspect_signature(). Returns false,
// having said why, if it differs; nothing is ever written then.
bool prepare_game_aspect(psprecomp::Runtime &runtime);

// Once per game frame, at the flip: gives the game `aspect` (width over
// height of the picture) when it differs from what the game has, or puts back
// the game's own 480/272 when the port had changed it. At exactly 480/272 and
// with nothing changed before, it writes nothing at all.
void game_aspect_frame(psprecomp::Runtime &runtime, float aspect);

} // namespace mhp3rd::camera
