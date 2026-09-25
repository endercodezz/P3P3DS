#include "camera/game_aspect.hpp"

#include "psprecomp/runtime.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace mhp3rd::camera {
namespace {

// NPJB-40001. The camera's initialisation (0x0882D5A4) copies four constants
// into the camera: near 30, far 65000, aspect 480/272 and a vertical field of
// view of 0.8722 radians. The projection (0x0882CF0C) and the culling planes
// (0x0882BF1C) are built from those fields, and the camera's update
// (0x0882D150) builds them again only when the field of view differs from the
// one they were last built with, kept at +0x10.
constexpr std::uint32_t kAspectConstant = 0x08969F74u;
constexpr std::uint32_t kGameAspectBits = 0x3FE1E1E2u;  // 480/272
// The culling planes put the frustum's side corners at depth 1.5 times the far
// distance, with the sides at cot(fov/2) times it; the top and bottom are
// divided by the aspect ratio as well. That leaves the vertical field of view
// uncovered once the aspect ratio passes cot^2(fov/2) / 1.5, about 3.07 with
// the usual field of view, and the horizontal one just the same. Beyond that
// the depth factor shrinks so both stay inside the planes.
constexpr std::uint32_t kCullDepthConstant = 0x08969ED4u;
constexpr std::uint32_t kGameCullDepthBits = 0xBFC00000u;  // -1.5
constexpr float kCullBudget = 4.0f;  // cot^2(0.8722 / 2) is 4.6: some margin kept
constexpr std::uint32_t kCameraPointer = 0x08A2F958u;
constexpr std::uint32_t kNear = 0x00u;
constexpr std::uint32_t kFar = 0x04u;
constexpr std::uint32_t kAspect = 0x08u;
constexpr std::uint32_t kFieldOfView = 0x0Cu;
constexpr std::uint32_t kBuiltFieldOfView = 0x10u;
constexpr std::uint32_t kCameraBytes = 0x11D0u;

constexpr std::array<CodeWord, 11> kSignature{{
    {0x0882D5D0u, 0xC4409F74u, "lwc1 f0,-0x608c(v0): the camera's initialisation reads the aspect constant"},
    {0x0882D5D4u, 0xE4800008u, "swc1 f0,0x8(a0): into the camera's aspect"},
    {0x0882D7A8u, 0xAC44F958u, "sw a0,-0x6a8(v0): the camera's address, kept at 0x08A2F958"},
    {0x0882CF1Cu, 0xC48C000Cu, "lwc1 f12,0xc(a0): the projection's field of view"},
    {0x0882CF20u, 0xC48D0008u, "lwc1 f13,0x8(a0): the projection's aspect"},
    {0x0882CE88u, 0x46170DC3u, "div.s f23,f1,f23: the projection's x scale is cot(fov/2) / aspect"},
    {0x0882D18Cu, 0xC4810010u, "lwc1 f1,0x10(a0): the camera update reads the field of view last built"},
    {0x0882D190u, 0xC480000Cu, "lwc1 f0,0xc(a0): and the current one"},
    {0x0882D194u, 0x46000832u, "c.eq.s f1,f0: and builds the projection again when they differ"},
    {0x0882BF64u, 0xC4850008u, "lwc1 f5,0x8(a0): the culling planes read the aspect"},
    {0x0882BF74u, 0xC4439ED4u, "lwc1 f3,-0x612c(v0): and the depth factor constant"},
}};

struct State {
    bool prepared{};
    // The port has written something since the game last had its own values.
    bool changed{};
    float applied{};
};

State state;

float load_float(const psprecomp::GuestMemory &memory, std::uint32_t address) {
    return std::bit_cast<float>(memory.load32(address));
}

void store_float(psprecomp::GuestMemory &memory, std::uint32_t address, float value) {
    memory.store32(address, std::bit_cast<std::uint32_t>(value));
}

// The camera, when the pointer leads to one that looks like it.
std::uint32_t camera_address(const psprecomp::GuestMemory &memory) {
    const std::uint32_t address = memory.load32(kCameraPointer);
    if (address == 0u || memory.raw_pointer(address, kCameraBytes) == nullptr) return 0u;
    const float near_plane = load_float(memory, address + kNear);
    const float far_plane = load_float(memory, address + kFar);
    const float aspect = load_float(memory, address + kAspect);
    const float fov = load_float(memory, address + kFieldOfView);
    const bool plausible = std::isfinite(near_plane) && std::isfinite(far_plane) && near_plane > 0.0f &&
                           far_plane > near_plane && aspect > 0.25f && aspect < 8.0f && fov > 0.01f && fov < 3.1f;
    return plausible ? address : 0u;
}

} // namespace

std::span<const CodeWord> game_aspect_signature() { return kSignature; }

bool prepare_game_aspect(psprecomp::Runtime &runtime) {
    state = State{};
    const psprecomp::GuestMemory &memory = runtime.memory();
    for (const CodeWord &expected : kSignature) {
        const std::uint32_t found = memory.load32(expected.address);
        if (found == expected.word) continue;
        std::cerr << "[aspect] game code differs at 0x" << std::hex << std::uppercase << std::setfill('0')
                  << std::setw(8) << expected.address << ": 0x" << std::setw(8) << found << ", expected 0x"
                  << std::setw(8) << expected.word << std::dec << std::nouppercase << std::setfill(' ') << " ("
                  << expected.what << "); the view keeps the PSP's shape\n";
        return false;
    }
    if (memory.load32(kAspectConstant) != kGameAspectBits || memory.load32(kCullDepthConstant) != kGameCullDepthBits) {
        std::cerr << "[aspect] the game's aspect or culling constant differs; the view keeps the PSP's shape\n";
        return false;
    }
    state.prepared = true;
    return true;
}

void game_aspect_frame(psprecomp::Runtime &runtime, float aspect) {
    if (!state.prepared) return;
    psprecomp::GuestMemory &memory = runtime.memory();
    const float game = std::bit_cast<float>(kGameAspectBits);
    // Within a hair of the game's own shape is the game's own shape, written
    // bit for bit as the game has it.
    const bool own = !std::isfinite(aspect) || std::abs(aspect - game) < 0.002f;
    if (own && !state.changed) return;
    const float wanted = own ? game : std::clamp(aspect, 0.5f, 5.0f);

    if (!state.changed || wanted != state.applied) {
        store_float(memory, kAspectConstant, wanted);
        if (own) memory.store32(kCullDepthConstant, kGameCullDepthBits);
        else store_float(memory, kCullDepthConstant, -std::min(1.5f, kCullBudget / wanted));
        state.applied = wanted;
        std::cout << "[aspect] the game's view is " << wanted << " wide to 1 high\n";
    }
    // The camera took its aspect from the constant when it was set up. Give
    // it the new one and make the next camera update build the projection
    // and the culling planes again: it does when the field of view it last
    // built them with differs from the current one.
    if (const std::uint32_t found = camera_address(memory); found != 0u) {
        if (load_float(memory, found + kAspect) != wanted) {
            store_float(memory, found + kAspect, wanted);
            store_float(memory, found + kBuiltFieldOfView, -load_float(memory, found + kFieldOfView));
        }
    }
    // Back to the game's own values once the camera has them too. Until a
    // camera turns up this keeps looking, which costs one read a frame.
    const std::uint32_t camera = camera_address(memory);
    state.changed = !own || camera == 0u || memory.load32(camera + kAspect) != kGameAspectBits;
}

} // namespace mhp3rd::camera
