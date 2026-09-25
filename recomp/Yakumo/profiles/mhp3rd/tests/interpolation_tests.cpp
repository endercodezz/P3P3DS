// Checks for frame interpolation's backend-independent part: matching draws
// between two frames, telling a cut from motion, blending transforms, when to
// present and what each present shows, and how the frame rate steps down and
// back up. Needs no game data and no GPU.
//
//   mhp3rd_interpolation_tests
#include "gpu/frame_interpolation.hpp"
#include "gpu/frame_pacing.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace mhp3rd::gpu;
using namespace mhp3rd::gpu::interpolation;

namespace {

int failures = 0;

void check(bool condition, const char *what) {
    std::printf("%s %s\n", condition ? "ok  " : "FAIL", what);
    if (!condition) ++failures;
}

bool near(float a, float b, float tolerance = 1e-3f) { return std::fabs(a - b) <= tolerance; }

constexpr float kPi = 3.14159265358979f;
constexpr std::uint32_t kShown = 0x04000000u;

// A rotation about y by `degrees` followed by a translation.
Matrix transform(float degrees, float x, float y, float z) {
    const float c = std::cos(degrees * kPi / 180.0f);
    const float s = std::sin(degrees * kPi / 180.0f);
    Matrix m{};
    m[0] = c;
    m[2] = -s;
    m[5] = 1.0f;
    m[8] = s;
    m[10] = c;
    m[12] = x;
    m[13] = y;
    m[14] = z;
    m[15] = 1.0f;
    return m;
}

Matrix perspective() {
    Matrix m{};
    m[0] = 1.0f;
    m[5] = 1.8f;
    m[10] = -1.0f;
    m[11] = -1.0f;
    m[14] = -60.0f;
    return m;
}

// One 3D draw of mesh `mesh` placed at `x`, seen by a camera turned by
// `camera_degrees` about the point 400 units in front of it (the way the
// game's camera orbits the hunter) and moved sideways by `camera_x`. Only
// eye space (view times world) counts, so where the game keeps the camera's
// turn makes no difference.
DrawSummary draw(std::uint32_t mesh, float x, float camera_degrees = 0.0f, float camera_x = 0.0f) {
    DrawSummary summary{};
    summary.vertex_address = 0x09000000u + mesh * 0x100u;
    summary.vertex_type = 0x11Cu;
    summary.texture_address = 0x09800000u + mesh * 0x40u;
    summary.count = 36u;
    summary.primitive = PrimitiveType::Triangles;
    summary.target = kShown;
    summary.perspective = true;
    summary.world = transform(0.0f, x, 0.0f, -500.0f);
    // Orbit: into the pivot's frame, turn, back out; then the slide.
    const Matrix to_pivot = transform(0.0f, 0.0f, 0.0f, 400.0f);
    const Matrix from_pivot = transform(0.0f, -camera_x, 0.0f, -400.0f);
    summary.view = multiply(from_pivot, multiply(transform(camera_degrees, 0.0f, 0.0f, 0.0f), to_pivot));
    summary.projection = perspective();
    return summary;
}

std::vector<DrawSummary> scene(std::uint32_t meshes, float camera_degrees = 0.0f, float camera_x = 0.0f) {
    std::vector<DrawSummary> frame;
    for (std::uint32_t i = 0; i < meshes; ++i) frame.push_back(draw(i, static_cast<float>(i) * 10.0f, camera_degrees, camera_x));
    mark_eligible(frame, kShown);
    return frame;
}

void blending() {
    const Matrix a = transform(10.0f, 100.0f, 20.0f, -300.0f);
    const Matrix b = transform(16.0f, 104.0f, 20.0f, -310.0f);
    const Matrix start = blend_affine(a, b, 0.0f);
    const Matrix end = blend_affine(a, b, 1.0f);
    bool same = true;
    for (std::size_t i = 0; i < 16u; ++i) same = same && near(start[i], a[i]) && near(end[i], b[i]);
    check(same, "blend_affine gives the ends at 0 and 1");
    const Matrix half = blend_affine(a, b, 0.5f);
    check(near(rotation_angle_degrees(a, half), 3.0f, 0.01f), "halfway turns half the angle");
    check(near(half[12], 102.0f) && near(half[14], -305.0f), "halfway moves half the distance");
    float length = 0.0f;
    for (std::size_t row = 0; row < 3u; ++row) length += half[row] * half[row];
    check(near(std::sqrt(length), 1.0f), "a blended rotation keeps its scale");
    check(near(rotation_angle_degrees(transform(10.0f, 0, 0, 0), transform(100.0f, 0, 0, 0)), 90.0f, 0.01f),
          "rotation_angle_degrees measures a quarter turn");
    Matrix ortho{};
    ortho[0] = ortho[5] = ortho[10] = ortho[15] = 1.0f;
    check(is_orthographic(ortho) && !is_orthographic(perspective()), "orthographic projections are told apart");
}

void rigid() {
    // A camera orbiting a point 400 units ahead by 24 degrees and sliding up
    // 5 units along the axis.
    const Matrix to_pivot = transform(0.0f, 0.0f, 0.0f, 400.0f);
    const Matrix from_pivot = transform(0.0f, 0.0f, 5.0f, -400.0f);
    const Matrix orbit = multiply(from_pivot, multiply(transform(24.0f, 0.0f, 0.0f, 0.0f), to_pivot));
    const RigidMotion motion = rigid_motion(orbit);
    check(motion.valid && near(motion.angle * 180.0f / kPi, 24.0f, 0.01f), "the orbit's turn is found");
    check(near(motion.centre[0], 0.0f, 0.05f) && near(motion.centre[2], -400.0f, 0.05f) && near(motion.slide[1], 5.0f),
          "its centre is the orbited point and the slide runs along the axis");
    const Matrix whole = rigid_at(motion, 1.0f);
    bool same = true;
    for (std::size_t i = 0; i < 16u; ++i) same = same && near(whole[i], orbit[i], 1e-2f);
    check(same, "all the way is the motion itself");
    // A distant point stays on its arc about the centre halfway.
    const Matrix half = rigid_at(motion, 0.5f);
    const float px = 0.0f, pz = -3000.0f;
    const float hx = half[0] * px + half[8] * pz + half[12];
    const float hz = half[2] * px + half[10] * pz + half[14];
    const float radius = std::sqrt(hx * hx + (hz + 400.0f) * (hz + 400.0f));
    check(near(radius, 2600.0f, 0.5f), "halfway, a point 2600 units from the centre stays 2600 from it");
    Matrix inverse{};
    check(affine_inverse(orbit, inverse), "an affine transform has an inverse");
    const Matrix product = multiply(orbit, inverse);
    bool identity = true;
    for (std::size_t i = 0; i < 16u; ++i) identity = identity && near(product[i], (i % 5u == 0u) ? 1.0f : 0.0f, 1e-3f);
    check(identity, "times its inverse it is the identity");

    // Scenery that stands still follows the orbit; the followed character,
    // fixed in eye space, stays where it is.
    const Matrix rock = transform(0.0f, 900.0f, 0.0f, -2500.0f);
    const Matrix rock_after = multiply(orbit, rock);
    const Matrix rock_half = blend_eye(rock, rock_after, motion, 0.5f);
    const Matrix expected = multiply(half, rock);
    bool follows = true;
    for (std::size_t i = 0; i < 16u; ++i) follows = follows && near(rock_half[i], expected[i], 0.05f);
    check(follows, "still scenery moves along the orbit's arc");
    const Matrix hunter = transform(10.0f, 0.0f, -20.0f, -400.0f);
    const Matrix hunter_half = blend_eye(hunter, hunter, motion, 0.5f);
    check(hunter_half == hunter, "the character the camera follows does not move on screen");
    // A character walking 6 units while the camera orbits.
    const Matrix walker = transform(0.0f, 30.0f, 0.0f, -380.0f);
    const Matrix walker_after = multiply(orbit, transform(0.0f, 36.0f, 0.0f, -380.0f));
    const Matrix walker_half = blend_eye(walker, walker_after, motion, 0.5f);
    const Matrix walker_expected = multiply(half, transform(0.0f, 33.0f, 0.0f, -380.0f));
    bool walks = true;
    for (std::size_t i = 12u; i < 15u; ++i) walks = walks && near(walker_half[i], walker_expected[i], 0.2f);
    check(walks, "a moving character is halfway along its own path, seen from halfway along the orbit");
}

void guards() {
    // The brazier fire in the gathering hall steps its offset by a quarter
    // of the texture each game frame; water scrolls by 0.004.
    check(near(blend_offset(0.25f, 0.5f, 0.5f), 0.25f), "a flipbook's step to its next cell is held");
    check(near(blend_offset(0.75f, 0.0f, 0.33f), 0.75f), "and so is its wrap back to the first");
    check(near(blend_offset(0.1024f, 0.1063f, 0.5f), 0.10435f, 1e-5f), "a scrolling texture blends");
    check(near(blend_offset(-1.7692f, -1.82477f, 0.5f), -1.796985f, 1e-4f), "so does a fast scroll of 0.056");
    check(near(blend_offset(0.25f, 0.5f, 0.5f, 0.5f), 0.375f), "with the old limit the flipbook step blended");

    // Two instances of one mesh swap their drawing order: pairing them in
    // order would blend each towards the other.
    const CutThresholds thresholds{};
    Matcher matcher;
    std::vector<DrawSummary> older = scene(40u);
    std::vector<DrawSummary> newer = scene(40u);
    DrawSummary left = draw(90u, -300.0f), right = draw(90u, 300.0f);
    older.push_back(left);
    older.push_back(right);
    newer.push_back(right);
    newer.push_back(left);
    mark_eligible(older, kShown);
    mark_eligible(newer, kShown);
    const Matching &swapped = matcher.match(older, newer, thresholds);
    check(swapped.rejected == 2u && swapped.rejected_shared == 2u &&
              swapped.newer_of[40] == Matching::kFollowCamera && swapped.newer_of[41] == Matching::kFollowCamera,
          "swapped instances 600 units apart are not blended into each other");
    check(swapped.cut == nullptr, "the rest of the frame still blends");

    // A character walking 20 units a frame while the camera turns keeps its pair.
    std::vector<DrawSummary> before = scene(40u, 0.0f), after = scene(40u, 5.0f);
    DrawSummary walker = draw(91u, 0.0f, 0.0f), walked = draw(91u, 20.0f, 5.0f);
    before.push_back(walker);
    after.push_back(walked);
    mark_eligible(before, kShown);
    mark_eligible(after, kShown);
    Matcher turning;
    const Matching &walking = turning.match(before, after, thresholds);
    check(walking.rejected == 0u && walking.newer_of[40] == 40, "a character's own walk during a turn is kept");
    check(near(walking.max_own_motion, 20.0f, 0.5f), "and measured apart from the camera's turn");

    CutThresholds off = thresholds;
    off.max_own_motion = 0.0f;
    Matcher unguarded;
    check(unguarded.match(older, newer, off).rejected == 0u, "with the guard off every pair is kept, as before");
}

void matching() {
    const CutThresholds thresholds{};
    Matcher matcher;

    const std::vector<DrawSummary> still = scene(100u);
    const Matching &same = matcher.match(still, still, thresholds);
    check(same.matched == 100u && same.cut == nullptr, "a still scene matches every draw and is no cut");

    const Matching &walking = matcher.match(scene(100u, 0.0f, 0.0f), scene(100u, 1.5f, 8.0f), thresholds);
    check(walking.matched == 100u && walking.cut == nullptr, "a camera turning 1.5 degrees and moving 8 units blends");
    // Orbiting 1.5 degrees at 400 units moves the eye 10.5 units sideways;
    // the slide takes 8 of them back.
    check(near(walking.camera_angle_degrees, 1.5f, 0.01f) && near(walking.camera_distance, 2.47f, 0.05f),
          "the camera's turn and the eye's own move are measured, in eye space");
    const Matching &orbit = matcher.match(scene(100u, 0.0f), scene(100u, 7.0f), thresholds);
    check(orbit.cut == nullptr && near(orbit.camera_distance, 48.8f, 0.5f),
          "a 7 degree orbit at 400 units moves the eye 49 units, however far the scenery swings");

    const Matching &turned = matcher.match(scene(100u), scene(100u, 40.0f), thresholds);
    check(turned.cut != nullptr && std::strcmp(turned.cut, "camera turned") == 0, "a 40 degree turn is a cut");

    Matcher fresh_matcher;
    const Matching &moved = fresh_matcher.match(scene(100u), scene(100u, 0.0f, 300.0f), thresholds);
    check(moved.cut != nullptr && std::strcmp(moved.cut, "camera moved") == 0, "a 300 unit jump is a cut");

    std::vector<DrawSummary> other = scene(100u);
    for (std::size_t i = 0; i < 60u; ++i) other[i].vertex_address += 0x00100000u;
    const Matching &replaced = matcher.match(scene(100u), other, thresholds);
    check(replaced.matched == 40u && replaced.cut != nullptr && std::strcmp(replaced.cut, "few draws match") == 0,
          "a frame with most draws new is a cut");

    // Instances of one mesh pair up in drawing order.
    std::vector<DrawSummary> older{draw(7u, 0.0f), draw(7u, 50.0f), draw(7u, 100.0f)};
    std::vector<DrawSummary> newer{draw(7u, 1.0f), draw(7u, 51.0f)};
    mark_eligible(older, kShown);
    mark_eligible(newer, kShown);
    const Matching &instances = matcher.match(older, newer, thresholds);
    check(instances.newer_of[0] == 0 && instances.newer_of[1] == 1 && instances.newer_of[2] == -1,
          "instances pair in order and an extra one stays unmatched");

    // Only perspective draws into the shown framebuffer take part.
    std::vector<DrawSummary> mixed{draw(1u, 0.0f), draw(2u, 0.0f), draw(3u, 0.0f)};
    mixed[1].target = 0x04100000u;  // render to texture
    mixed[2].perspective = false;   // 2D, orthographic or a clear
    mark_eligible(mixed, kShown);
    const Matching &eligible = matcher.match(mixed, mixed, thresholds);
    check(eligible.eligible_older == 1u && eligible.matched == 1u && eligible.newer_of[1] == -1 &&
              eligible.newer_of[2] == -1,
          "draws into other framebuffers and 2D draws are never matched");

    std::vector<DrawSummary> flat{draw(1u, 0.0f)};
    flat[0].perspective = false;
    mark_eligible(flat, kShown);
    const Matching &nothing = matcher.match(flat, flat, thresholds);
    check(nothing.cut != nullptr && std::strcmp(nothing.cut, "nothing to blend") == 0,
          "a frame without 3D draws, like a loading screen, is not blended");
}

// A fast analog turn keeps the camera turning by about the same angle each
// frame; a cut jumps from slow motion. Issue #39: 720 degrees a second is 24
// a frame in yaw alone, more on a diagonal.
void continuous_motion() {
    const CutThresholds thresholds{};
    Matcher matcher;
    float angle = 0.0f;
    const float steps[] = {20.0f, 26.0f, 34.0f, 36.0f, 36.0f};
    bool blended = true;
    for (const float step : steps) {
        const Matching &turn = matcher.match(scene(50u, angle), scene(50u, angle + step), thresholds);
        blended = blended && turn.cut == nullptr;
        angle += step;
    }
    check(blended, "a turn speeding up to 36 degrees a frame keeps blending");
    const Matching &more = matcher.match(scene(50u, angle), scene(50u, angle + 40.0f), thresholds);
    check(more.cut == nullptr && more.continued, "past 30 degrees it counts as continued motion");
    const Matching &jump = matcher.match(scene(50u, 0.0f), scene(50u, 120.0f), thresholds);
    check(jump.cut != nullptr, "a 120 degree jump during a turn is still a cut");

    Matcher fresh;
    (void)fresh.match(scene(50u), scene(50u, 1.0f), thresholds);
    const Matching &sudden = fresh.match(scene(50u, 1.0f), scene(50u, 41.0f), thresholds);
    check(sudden.cut != nullptr && std::strcmp(sudden.cut, "camera turned") == 0,
          "a 40 degree jump from a slow camera is a cut");

    Matcher moving;
    float x = 0.0f;
    bool moved = true;
    for (const float step : {150.0f, 190.0f, 250.0f, 300.0f}) {
        moved = moved && moving.match(scene(50u, 0.0f, x), scene(50u, 0.0f, x + step), thresholds).cut == nullptr;
        x += step;
    }
    check(moved, "an eye orbiting faster and faster keeps blending");
    moving.forget_motion();
    check(moving.match(scene(50u, 0.0f, x), scene(50u, 0.0f, x + 300.0f), thresholds).cut != nullptr,
          "after forget_motion the plain limits apply again");
}

using namespace mhp3rd::gpu::pacing;

void rates() {
    check(presents_per_frame(45.0) == 1.5 && presents_per_frame(60.0) == 2.0 && presents_per_frame(90.0) == 3.0 &&
              presents_per_frame(120.0) == 4.0,
          "45, 60, 90 and 120 are 1.5, 2, 3 and 4 presents per game frame");
    check(presents_per_frame(59.94) == 2.0 && presents_per_frame(89.9) == 3.0,
          "a display reporting 59.94 or 89.9 Hz counts as 60 or 90");
    check(near(static_cast<float>(presents_per_frame(144.0)), 4.8f), "144 Hz is 4.8 per game frame");
    check(plain_presents_per_frame(60.0) == 1.0 && plain_presents_per_frame(120.0) == 1.0 &&
              plain_presents_per_frame(45.0) == 0.5 && near(static_cast<float>(plain_presents_per_frame(144.0)), 0.2f),
          "presents that fall on a frame's own moment: every frame, every other one at 45, every fifth at 144");
}

// Runs the clock over `frames` game frames with the game's code taking
// `work_us` of each (the flip comes that late) and a present made as soon as
// it is due, checked every 250 us. Returns each present's blend factor, and
// checks that the moment shown never goes back.
std::vector<float> run_clock(double rate, std::int64_t work_us, int frames, bool &monotonic, std::uint32_t &skipped,
                             std::int64_t late_frame = -1) {
    PresentClock clock;
    clock.set_presents_per_frame(presents_per_frame(rate));
    std::vector<float> factors;
    monotonic = true;
    skipped = 0u;
    double shown = -1e18;
    std::int64_t next_frame = 0;
    std::int64_t older = 0, newer = 0;
    for (std::int64_t now = 0; now < frames * kGameFrameUs; now += 250) {
        const std::int64_t moment = next_frame * kGameFrameUs;
        const std::int64_t delay = next_frame == late_frame ? kGameFrameUs + 5000 : work_us;
        if (now >= moment + delay) {
            clock.flip(moment, now);
            older = newer;
            newer = moment;
            ++next_frame;
        }
        if (const auto present = clock.take(now)) {
            factors.push_back(present->t);
            skipped += present->skipped;
            const double at = static_cast<double>(older) + present->t * static_cast<double>(newer - older);
            if (at + 1.0 < shown) monotonic = false;
            shown = std::max(shown, at);
        }
    }
    return factors;
}

bool pattern(const std::vector<float> &factors, std::initializer_list<float> expected, std::size_t from) {
    if (std::getenv("SHOW_FACTORS") != nullptr) {
        for (std::size_t i = 0; i < std::min<std::size_t>(factors.size(), 16u); ++i) std::printf("%.3f ", factors[i]);
        std::printf("\n");
    }
    if (factors.size() < from + expected.size() * 3u) return false;
    std::size_t i = from;
    // Find the pattern's start, then follow it for three rounds.
    while (i < from + expected.size() && !near(factors[i], *expected.begin(), 0.01f)) ++i;
    for (int round = 0; round < 3; ++round)
        for (const float value : expected)
            if (!near(factors[i++], value, 0.01f)) return false;
    return true;
}

void present_clock() {
    bool monotonic = false;
    std::uint32_t skipped = 0u;
    const std::vector<float> at60 = run_clock(60.0, 12000, 40, monotonic, skipped);
    check(pattern(at60, {1.0f, 0.5f}, 4u) && monotonic && skipped == 0u,
          "60: each frame as it is, then halfway to the next");
    const std::vector<float> at45 = run_clock(45.0, 12000, 40, monotonic, skipped);
    check(pattern(at45, {1.0f, 2.0f / 3.0f, 1.0f / 3.0f}, 4u) && monotonic && skipped == 0u,
          "45: blend factors 1, 2/3, 1/3 over two game frames");
    const std::vector<float> at90 = run_clock(90.0, 12000, 40, monotonic, skipped);
    check(pattern(at90, {1.0f, 1.0f / 3.0f, 2.0f / 3.0f}, 4u) && monotonic && skipped == 0u,
          "90: thirds");
    const std::vector<float> at120 = run_clock(120.0, 25000, 40, monotonic, skipped);
    check(pattern(at120, {1.0f, 0.25f, 0.5f, 0.75f}, 4u) && monotonic && skipped == 0u,
          "120: quarters, even when the game's code takes 25 ms of each frame");
    check(at60.size() >= 76u && at60.size() <= 80u, "60 presents twice per game frame");

    // A frame whose code runs past its successor's moment: the presents hold
    // the newest frame instead of going back or guessing.
    const std::vector<float> late = run_clock(60.0, 12000, 40, monotonic, skipped, 20);
    check(monotonic, "a late frame never makes the picture go back");

    PresentClock clock;
    clock.set_presents_per_frame(2.0);
    clock.flip(0, 5000);
    clock.flip(kGameFrameUs, kGameFrameUs + 5000);
    check(clock.work_us() == 6000 && clock.delay_us() == kGameFrameUs / 2 + 6000,
          "at 60 the delay is half a frame plus the game's code time and a millisecond");
    const std::int64_t delay = clock.delay_us();
    check(!clock.take(delay - 10), "nothing is due before the delay has passed");
    const auto first = clock.take(kGameFrameUs + delay);
    check(first && first->t == 1.0f, "the newest frame's own moment shows it as it is");
    const auto behind = clock.take(kGameFrameUs + delay + 5 * kGameFrameUs / 2 + 10);
    check(behind && behind->skipped == 4u, "presents not made in time are skipped, not queued");
    clock.flip(2 * kGameFrameUs, 2 * kGameFrameUs + 15000);
    check(clock.work_us() == 16000, "while few frames are known, the longest code time sets the delay");
    clock.flip(3 * kGameFrameUs + 9000, 3 * kGameFrameUs + 14000);
    const auto moved = clock.next_due();
    check(moved && *moved == 3 * kGameFrameUs + 9000 + clock.delay_us(), "a flip off the grid moves the grid to it");
    clock.set_presents_per_frame(1.0);
    check(!clock.next_due() && !clock.take(10 * kGameFrameUs), "one present per frame stops the clock");

    // 60 frames whose code takes 8 ms, three of them 30 ms (a texture
    // upload): the delay allows for 8, not 30.
    PresentClock steady;
    steady.set_presents_per_frame(3.0);
    for (std::int64_t frame = 0; frame < 60; ++frame) {
        const std::int64_t code = frame % 20 == 7 ? 30000 : 8000;
        steady.flip(frame * kGameFrameUs, frame * kGameFrameUs + code);
    }
    check(steady.work_us() == 9000, "a few slow frames among many do not lengthen the delay");
    for (std::int64_t frame = 60; frame < 120; ++frame) {
        const std::int64_t code = frame % 5 == 0 ? 20000 : 8000;
        steady.flip(frame * kGameFrameUs, frame * kGameFrameUs + code);
    }
    check(steady.work_us() == 21000, "one frame in five that slow does");
}

Second second_with(double speed, double idle_ms, double blend_ms, double plain_ms, double rate) {
    Second second{};
    second.speed = speed;
    second.idle_ms = idle_ms;
    second.blend_ms = blend_ms;
    second.plain_ms = plain_ms;
    second.interpolation_ms = RateGovernor::cost_ms(rate, second);
    second.presents = static_cast<std::uint32_t>(rate);
    return second;
}

void governor() {
    RateGovernor governor;
    governor.set_requested(30.0);
    check(governor.rate() == 30.0 && !governor.update(second_with(0.5, 0.0, 5.0, 0.5, 30.0)),
          "30 stays 30 whatever happens");

    governor.set_requested(120.0);
    check(governor.rate() == 120.0, "the governor starts at the rate asked for");
    bool changed = false;
    for (int i = 0; i < 20; ++i) changed = changed || governor.update(second_with(1.0, 10.0, 3.0, 0.4, 120.0));
    check(!changed && governor.rate() == 120.0, "a game at full speed keeps its rate");

    // Deck-like: 6 ms a blended present, the game at 80%: 120 costs 18.4 ms a
    // frame, the game is short of 6.7 ms plus the margin.
    check(!governor.update(second_with(0.8, 0.0, 6.0, 0.4, 120.0)), "one slow second is not enough");
    check(governor.update(second_with(0.8, 0.0, 6.0, 0.4, 120.0)) && governor.rate() == 60.0,
          "two slow seconds drop straight to the rate that fits (120 to 60)");
    check(!governor.update(second_with(0.8, 0.0, 0.0, 0.0, 60.0)),
          "a slow second without presents to save is not blamed on them");

    for (int i = 0; i < RateGovernor::kSecondsAfterDown + 5; ++i)
        (void)governor.update(second_with(1.0, 20.0, 6.0, 0.4, 60.0));
    check(governor.rate() == 90.0, "with time to spare it tries the next rate up");
    for (int i = 0; i < 10; ++i) (void)governor.update(second_with(1.0, 20.0, 6.0, 0.4, 90.0));
    check(governor.rate() == 90.0, "120 is not tried again right after it failed");
    for (int i = 0; i < RateGovernor::kBlockSeconds; ++i) (void)governor.update(second_with(1.0, 20.0, 6.0, 0.4, 90.0));
    check(governor.rate() == 120.0, "once its wait is over, 120 is tried again");

    // A load: the game at 80% for a few seconds while presents of frames
    // with nothing to blend cost 1.2 ms a frame. Not their fault.
    RateGovernor loading;
    loading.set_requested(120.0);
    bool dropped = false;
    for (int i = 0; i < 5; ++i) dropped = dropped || loading.update(second_with(0.8, 5.0, 0.4, 0.4, 120.0));
    check(!dropped && loading.rate() == 120.0, "a load that slows the game does not drop the rate");

    // Back up from 30 in one go once there is room.
    RateGovernor recover;
    recover.set_requested(120.0);
    (void)recover.update(second_with(0.5, 0.0, 8.0, 0.4, 120.0));
    (void)recover.update(second_with(0.5, 0.0, 8.0, 0.4, 120.0));
    check(recover.rate() == 30.0, "far behind with 8 ms presents: 30");
    for (int i = 0; i < 15; ++i) (void)recover.update(second_with(1.0, 20.0, 5.0, 0.4, recover.rate()));
    check(recover.rate() == 90.0, "with room it climbs from 30 straight to 90 while 120 still waits");
    for (int i = 0; i < 30; ++i) (void)recover.update(second_with(1.0, 20.0, 5.0, 0.4, recover.rate()));
    check(recover.rate() == 120.0, "and to 120 once its wait is over");

    // Deck, entering the village: 90% speed while the kernel still waited
    // 28 ms a frame; plain presents of a loading screen cost 2.5 ms. Not
    // the presents.
    RateGovernor village;
    village.set_requested(90.0);
    for (int i = 0; i < 4; ++i) (void)village.update(second_with(0.9, 28.0, 1.4, 0.76, 90.0));
    check(village.rate() == 90.0, "a slow second with the kernel waiting is not blamed on the presents");

    // The display takes fewer presents than asked (a 90 Hz screen switched
    // to 60): images are not free for a third of them.
    RateGovernor display;
    display.set_requested(90.0);
    Second busy_display = second_with(1.0, 15.0, 2.0, 0.5, 90.0);
    busy_display.presents = 60u;
    busy_display.blocked = 30u;
    (void)display.update(busy_display);
    check(display.update(busy_display) && display.rate() == 60.0, "presents the display cannot take step down");

    // Off, the rate stays whatever happens.
    RateGovernor fixed;
    fixed.set_requested(120.0);
    fixed.set_automatic(false);
    bool moved_fixed = false;
    for (int i = 0; i < 10; ++i) moved_fixed = moved_fixed || fixed.update(second_with(0.5, 0.0, 8.0, 0.4, 120.0));
    check(!moved_fixed && fixed.rate() == 120.0, "with the automatic step-down off the rate never changes");
    for (int i = 0; i < 2; ++i) (void)fixed.update(second_with(0.5, 0.0, 8.0, 0.4, 120.0));
    fixed.set_automatic(true);
    for (int i = 0; i < 2; ++i) (void)fixed.update(second_with(0.5, 0.0, 8.0, 0.4, 120.0));
    check(fixed.rate() == 30.0, "turned on again, it steps down");

    RateGovernor busy;
    busy.set_requested(90.0);
    // Full speed, but only because the kernel never waits: 2 blended presents
    // of 16 ms leave no time, and the game's frames drift later and later.
    (void)busy.update(second_with(1.0, 0.2, 16.0, 2.0, 90.0));
    check(busy.update(second_with(1.0, 0.2, 16.0, 2.0, 90.0)) && busy.rate() == 60.0,
          "no spare time at full speed steps down too (90 to 60)");

    RateGovernor late;
    late.set_requested(90.0);
    Second skipping = second_with(1.0, 15.0, 2.0, 0.3, 90.0);
    skipping.presents = 60u;
    skipping.skipped = 30u;
    (void)late.update(skipping);
    check(late.update(skipping) && late.rate() == 60.0, "presents that keep coming late step down one rate");

    RateGovernor ladder;
    ladder.set_requested(144.0);
    check(ladder.rate() == 144.0, "a 144 Hz display is a rate of its own");
    for (int i = 0; i < 2; ++i) (void)ladder.update(second_with(0.5, 0.0, 4.0, 0.3, 144.0));
    check(ladder.rate() == 30.0, "far behind, it drops all the way to 30");
    RateGovernor slow45;
    slow45.set_requested(45.0);
    (void)slow45.update(second_with(0.9, 0.0, 5.0, 0.3, 45.0));
    check(slow45.update(second_with(0.9, 0.0, 5.0, 0.3, 45.0)) && slow45.rate() == 30.0,
          "45 steps down to 30");
}

} // namespace

int main() {
    blending();
    rigid();
    guards();
    matching();
    continuous_motion();
    rates();
    present_clock();
    governor();
    std::printf("%s\n", failures == 0 ? "all passed" : "FAILED");
    return failures == 0 ? 0 : 1;
}
