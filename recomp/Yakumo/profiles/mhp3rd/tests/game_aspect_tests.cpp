// No game code or game data is needed: the words the aspect ratio checks are
// written into guest memory as stand-ins, with a camera beside them.
#include "camera/game_aspect.hpp"
#include "psprecomp/runtime.hpp"

#include <bit>
#include <cmath>
#include <iostream>

namespace {
using namespace mhp3rd::camera;

constexpr std::uint32_t kAspectConstant = 0x08969F74u;
constexpr std::uint32_t kCullDepthConstant = 0x08969ED4u;
constexpr std::uint32_t kCameraPointer = 0x08A2F958u;
constexpr std::uint32_t kCamera = 0x08900000u;
constexpr std::uint32_t kGameAspectBits = 0x3FE1E1E2u;
constexpr std::uint32_t kGameCullDepthBits = 0xBFC00000u;
constexpr float kGameAspect = 480.0f / 272.0f;
constexpr float kFieldOfView = 0.8722222f;
int failures{};

void check(bool condition, const char *message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

float read_float(const psprecomp::GuestMemory &memory, std::uint32_t address) {
    return std::bit_cast<float>(memory.load32(address));
}
void write_float(psprecomp::GuestMemory &memory, std::uint32_t address, float value) {
    memory.store32(address, std::bit_cast<std::uint32_t>(value));
}

struct Fixture {
    psprecomp::Runtime runtime;
    explicit Fixture(bool matching = true) {
        auto &memory = runtime.memory();
        for (const CodeWord &word : game_aspect_signature()) memory.store32(word.address, word.word);
        if (!matching) memory.store32(game_aspect_signature().front().address, 0u);
        memory.store32(kAspectConstant, kGameAspectBits);
        memory.store32(kCullDepthConstant, kGameCullDepthBits);
        memory.store32(kCameraPointer, kCamera);
        write_float(memory, kCamera + 0x0u, 30.0f);
        write_float(memory, kCamera + 0x4u, 65000.0f);
        memory.store32(kCamera + 0x8u, kGameAspectBits);
        write_float(memory, kCamera + 0xCu, kFieldOfView);
        write_float(memory, kCamera + 0x10u, kFieldOfView);
    }
    [[nodiscard]] const psprecomp::GuestMemory &memory() const { return runtime.memory(); }
};

void test_own_shape_writes_nothing() {
    Fixture f;
    check(prepare_game_aspect(f.runtime), "the matching game code is accepted");
    write_float(f.runtime.memory(), kCamera + 0x10u, 1.0f);  // marks a write, if any
    game_aspect_frame(f.runtime, kGameAspect);
    game_aspect_frame(f.runtime, 1.7650f);  // within a hair of it
    check(f.memory().load32(kAspectConstant) == kGameAspectBits, "the game's shape leaves the constant alone");
    check(read_float(f.memory(), kCamera + 0x10u) == 1.0f, "the game's shape leaves the camera alone");
}

void test_wide_and_back() {
    Fixture f;
    check(prepare_game_aspect(f.runtime), "the matching game code is accepted");
    game_aspect_frame(f.runtime, 21.0f / 9.0f);
    check(read_float(f.memory(), kAspectConstant) == 21.0f / 9.0f, "21:9 reaches the constant");
    check(read_float(f.memory(), kCamera + 0x8u) == 21.0f / 9.0f, "21:9 reaches the camera");
    check(read_float(f.memory(), kCamera + 0x10u) != kFieldOfView, "the camera builds its projection again");
    check(f.memory().load32(kCullDepthConstant) == kGameCullDepthBits, "21:9 keeps the game's culling planes");

    // The camera's update rebuilds and records the field of view again.
    write_float(f.runtime.memory(), kCamera + 0x10u, kFieldOfView);
    game_aspect_frame(f.runtime, 21.0f / 9.0f);
    check(read_float(f.memory(), kCamera + 0x10u) == kFieldOfView, "an unchanged shape forces nothing");

    game_aspect_frame(f.runtime, 32.0f / 9.0f);
    check(std::abs(read_float(f.memory(), kCullDepthConstant) + 4.0f / (32.0f / 9.0f)) < 1e-6f,
          "32:9 widens the culling planes");

    game_aspect_frame(f.runtime, kGameAspect);
    check(f.memory().load32(kAspectConstant) == kGameAspectBits, "off restores the constant bit for bit");
    check(f.memory().load32(kCullDepthConstant) == kGameCullDepthBits, "off restores the culling constant");
    check(f.memory().load32(kCamera + 0x8u) == kGameAspectBits, "off restores the camera bit for bit");

    write_float(f.runtime.memory(), kCamera + 0x10u, 1.0f);
    game_aspect_frame(f.runtime, kGameAspect);
    check(read_float(f.memory(), kCamera + 0x10u) == 1.0f, "once restored, nothing more is written");
}

void test_camera_set_up_later() {
    Fixture f;
    f.runtime.memory().store32(kCameraPointer, 0u);
    check(prepare_game_aspect(f.runtime), "the matching game code is accepted");
    game_aspect_frame(f.runtime, 16.0f / 10.0f);
    check(read_float(f.memory(), kAspectConstant) == 1.6f, "16:10 reaches the constant without a camera");
    game_aspect_frame(f.runtime, kGameAspect);
    check(f.memory().load32(kAspectConstant) == kGameAspectBits, "and leaves it again");
}

void test_other_code_is_left_alone() {
    Fixture f(false);
    check(!prepare_game_aspect(f.runtime), "different game code is refused");
    game_aspect_frame(f.runtime, 21.0f / 9.0f);
    check(f.memory().load32(kAspectConstant) == kGameAspectBits, "nothing is written to other code");
    check(f.memory().load32(kCamera + 0x8u) == kGameAspectBits, "nor to its camera");
}

} // namespace

int main() {
    test_own_shape_writes_nothing();
    test_wide_and_back();
    test_camera_set_up_later();
    test_other_code_is_left_alone();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "game aspect tests passed\n";
    return 0;
}
