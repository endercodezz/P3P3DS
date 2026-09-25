#pragma once

#include <cstdint>

namespace vcs {

// Accelerator and brake, hooked where only a vehicle can reach them.
//
// San Andreas drives with W and S. VCS reads the accelerator from CPad::CROSS
// and the brake from CPad::SQUARE, and on foot those same two buttons sprint
// and jump -- so binding W and S to them at the input layer would make the
// player sprint while walking forward and jump while walking backward.
//
// The way out is that the two functions hooked here are the vehicle's own
// accessors: nothing on foot calls them. Feeding the host keys in at that point
// makes W accelerate and S brake inside a car while leaving both keys as plain
// analog movement everywhere else, without the host ever having to ask the
// guest whether the player is driving.
//
// Addresses, found with ThirteenAG's own byte patterns against our EBOOT and
// cross-checked against the camera hook he resolves to 0x0898E098:
//
//   0x0898D2D8  accelerator, returns CROSS  (pad offset 0x2A) when enabled
//   0x0898D140  brake,       returns SQUARE (pad offset 0x26) when enabled
//
// Both are edited in generated_unit_0098.cpp, next to the camera hook.

// Pad field the accelerator reads: CROSS normally, RIGHTSHOULDER1 under the
// modern scheme. Byte offset into CPad, for aot_load16.
[[nodiscard]] std::uint32_t vcs_accelerate_pad_offset() noexcept;

// Pad field the brake reads: SQUARE normally, LEFTSHOULDER1 under the modern
// scheme.
[[nodiscard]] std::uint32_t vcs_brake_pad_offset() noexcept;

// ThirteenAG's ModernControlScheme: accelerate on R, brake on L, the GTA IV/V
// arrangement. Off by default, and deliberately so -- stock VCS already
// accelerates on Cross and brakes on Square, which is what San Andreas does, so
// turning this on moves away from San Andreas rather than toward it. It exists
// because the option was asked for, not because it is the parity setting.
[[nodiscard]] bool vcs_modern_control_scheme_enabled() noexcept;

// Whether the host is asking for throttle or brake this poll, independently of
// the pad buttons. Non-zero means pressed.
[[nodiscard]] bool vcs_host_accelerate() noexcept;
[[nodiscard]] bool vcs_host_brake() noexcept;

// Called once per controller poll from the input layer.
void vcs_set_host_drive_inputs(bool accelerate, bool brake) noexcept;

// Raised by the hooked accessors every time the guest asks for throttle.
//
// This is the answer to "is the player driving?", arriving from the only place
// that actually knows. Nothing on foot calls those accessors, so the call
// itself is the signal -- no player structure to find, no pointer to chase.
void vcs_note_vehicle_control_read() noexcept;

// True while the guest has been reading vehicle controls recently.
//
// Held for a short while after the last read rather than cleared immediately:
// the guest polls its pad and the host polls the keyboard on unrelated clocks,
// and a flag that decayed between two frames would make the controls flicker
// between the on-foot and the driving mapping.
[[nodiscard]] bool vcs_player_in_vehicle() noexcept;

} // namespace vcs
