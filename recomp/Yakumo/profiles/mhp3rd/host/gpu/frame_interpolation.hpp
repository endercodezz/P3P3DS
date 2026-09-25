#pragma once

#include "ge_state.hpp"

#include <array>
#include <cstdint>
#include <vector>

// Frame interpolation: the game simulates at 30 frames per second, and the
// renderer presents extra frames between two of them by drawing the older
// frame's draws with their transforms blended towards the newer frame's.
// This part is independent of the backend: it recognises a draw of one frame
// in the next, decides whether two frames may be blended at all, and blends
// matrices. The renderer records the draws and replays them.
namespace mhp3rd::gpu::interpolation {

using Matrix = std::array<float, 16>;  // column-major, as in DrawCall

// What matching and blending need to know about one draw.
struct DrawSummary {
    // Identity: the same object drawn in two frames reads the same vertices
    // and indices from the same addresses with the same vertex type and
    // texture. Draws with the same identity (instances of one mesh) pair up
    // in the order the frame draws them.
    std::uint32_t vertex_address{};
    std::uint32_t index_address{};
    std::uint32_t vertex_type{};
    std::uint32_t texture_address{};
    std::uint32_t count{};
    PrimitiveType primitive{};
    std::uint32_t target{};  // framebuffer address drawn into
    // Filled by summarize() while the draw's matrices are at hand, so that
    // matching a frame need not read them again: the identity's hash, and
    // the translation column of view times world. `prepared` says they are
    // there; a summary made otherwise has them computed when matched.
    bool prepared{};
    std::uint64_t key_hash{};
    std::array<float, 3> eye_translation{};
    // Transformed through a perspective projection, and not a clear.
    bool perspective{};
    // A perspective draw into the framebuffer the game showed. Only these are
    // blended; 2D and interface draws, orthographic ones, clears and
    // render-to-texture passes are shown as the frame drew them.
    bool eligible{};
    bool skinned{};  // the vertices were blended by bone matrices
    Matrix world{};
    Matrix view{};
    Matrix projection{};
};

// Fills the fields of a summary that come from the draw itself; `eligible`
// is decided once the frame's displayed framebuffer is known.
DrawSummary summarize(const DrawCall &call);
// Marks the perspective draws into `displayed` eligible.
void mark_eligible(std::vector<DrawSummary> &draws, std::uint32_t displayed) noexcept;

// True for a projection without perspective division.
[[nodiscard]] bool is_orthographic(const Matrix &projection) noexcept;

// How two consecutive frames' draws correspond.
struct Matching {
    // For each draw of the older frame, the index of the same draw in the
    // newer frame, kNoPartner, or kFollowCamera for a pair whose own motion
    // is not believable (see CutThresholds::max_own_motion). Only eligible
    // draws are matched.
    std::vector<std::int32_t> newer_of;
    static constexpr std::int32_t kNoPartner = -1;
    static constexpr std::int32_t kFollowCamera = -2;
    // Pairs given up because a draw moved too far on its own in one game
    // frame, and of those, how many had other draws of the same key (the
    // same mesh drawn more than once) it may have been mistaken for.
    std::uint32_t rejected{};
    std::uint32_t rejected_shared{};
    float max_own_motion{};  // the largest own motion among the pairs kept
    float max_rejected_motion{};  // the largest among those given up
    std::uint32_t eligible_older{};
    std::uint32_t eligible_newer{};
    std::uint32_t matched{};
    // How the camera moved between the frames, as the motion of eye space:
    // for scenery that stands still, newer eye transform = camera * older.
    // Taken from the matched draw whose turn is the median of a sample, so
    // characters moving on their own do not count. The angle is its turn;
    // the distance is how far the eye itself moved, not how far distant
    // scenery swings when the camera turns.
    bool camera_found{};
    float camera_angle_degrees{};
    float camera_distance{};
    Matrix camera{};
    // The reason the two frames must not be blended, or null.
    const char *cut{};
    // Blended only because the motion continues the previous pair's: past
    // the plain limits of CutThresholds.
    bool continued{};
};

// Thresholds for telling a camera cut or a scene change from motion.
struct CutThresholds {
    float min_matched_fraction{0.5f};   // of the newer frame's eligible draws
    float max_camera_angle_degrees{30.0f};
    float max_camera_distance{200.0f};  // world units the camera moves in one frame
    // Motion that continues the previous pair's is not a cut even past the
    // limits above: the analog camera turns up to 720 degrees a second, 24
    // degrees a game frame in yaw alone, and more on a diagonal, with the eye
    // orbiting its look-at point. After a blended pair the limits become the
    // previous pair's turn and move times `continuous_growth` plus a margin,
    // never more than the `max_continuous_*` values. A cut in a cutscene
    // jumps from standing still or slow motion, so it still trips the limits.
    float continuous_growth{1.5f};
    float continuous_angle_margin_degrees{6.0f};
    float continuous_distance_margin{60.0f};
    float max_continuous_angle_degrees{75.0f};
    float max_continuous_distance{800.0f};
    // A draw whose position in eye space moves more than this in one game
    // frame beyond what the camera's motion explains is not blended with its
    // partner, which is most likely another object: instances of one mesh
    // pair up in drawing order, and that order can change. 0 turns the
    // guard off (MHP3RD_INTERPOLATION_NO_MOTION_GUARD).
    float max_own_motion{120.0f};
};

class Matcher {
public:
    // Pairs `older`'s eligible draws with `newer`'s and decides whether the
    // frames may be blended.
    // Each call is taken to follow the previous one (the pair before shares
    // `older` with this pair's newer frame): a pair that was blended lets the
    // next one continue its motion.
    const Matching &match(const std::vector<DrawSummary> &older, const std::vector<DrawSummary> &newer,
                          const CutThresholds &thresholds);
    // Forgets the previous pair, when the next call does not follow it.
    void forget_motion() noexcept { previous_blended_ = false; }

private:
    struct Key {
        std::uint32_t vertex_address, index_address, vertex_type, texture_address, count;
        std::uint8_t primitive;
        bool operator==(const Key &) const = default;
    };
    struct KeyHash {
        std::size_t operator()(const Key &key) const noexcept;
    };
    // One key of the newer frame: its first and last draw, chained through
    // next_ in drawing order, and the next one to hand out; first < 0 marks
    // an empty slot.
    struct Slot {
        Key key{};
        std::int32_t first{-1};
        std::int32_t last{-1};
        std::int32_t cursor{-1};
    };
    static Key key_of(const DrawSummary &draw) noexcept;

public:
    // The identity's hash and eye-space translation of a draw, as summarize()
    // stores them.
    static std::uint64_t hash_of(const DrawSummary &draw) noexcept;
    static std::array<float, 3> eye_translation_of(const DrawSummary &draw) noexcept;

private:

    std::vector<Slot> slots_;
    std::vector<std::int32_t> next_;  // for each newer draw, the next with its key
    std::vector<std::uint8_t> shared_;  // for each older draw: its key had several newer draws
    Matching result_;
    // The previous pair's camera motion, when that pair was blended.
    bool previous_blended_{};
    float previous_angle_degrees_{};
    float previous_distance_{};
};

// Blends two affine transforms: each basis vector keeps a length between the
// two lengths while its direction turns (a normalised blend, close to a
// rotation's slerp for the few degrees one game frame turns), and the
// translation moves linearly. `t` = 0 gives `a`, 1 gives `b`.
[[nodiscard]] Matrix blend_affine(const Matrix &a, const Matrix &b, float t) noexcept;
// a * b for column-major matrices.
[[nodiscard]] Matrix multiply(const Matrix &a, const Matrix &b) noexcept;
// Element-wise linear blend, for projections.
[[nodiscard]] Matrix blend_linear(const Matrix &a, const Matrix &b, float t) noexcept;

// The angle between the rotations of two transforms.
[[nodiscard]] float rotation_angle_degrees(const Matrix &a, const Matrix &b) noexcept;

// A texture offset between two frames. A scroll moves it a little each
// frame; a flipbook (fire, smoke) moves it to the next cell of an atlas, a
// quarter of the texture or more at once, and blending that would sweep
// across the cells in between. Steps below `max_scroll` are scrolling and
// blended; bigger ones hold the older offset.
inline constexpr float kMaxScrollStep = 0.1f;
[[nodiscard]] float blend_offset(float from, float to, float t, float max_scroll = kMaxScrollStep) noexcept;
[[nodiscard]] inline bool scrolls(float from, float to, float max_scroll = kMaxScrollStep) noexcept {
    const float step = to > from ? to - from : from - to;
    return step < max_scroll;
}

// The inverse of an affine transform; false when it has none.
[[nodiscard]] bool affine_inverse(const Matrix &m, Matrix &out) noexcept;

// A rigid motion taken apart so that it can be followed part of the way: a
// turn about an axis through a centre, and a slide along that axis. A
// camera orbiting its target is a turn about the target, and following it a
// fraction of the way keeps every point on its arc, where blending matrices
// would cut across the arc and pull distant scenery in.
struct RigidMotion {
    bool valid{};
    float angle{};                      // radians
    std::array<float, 3> axis{0.0f, 1.0f, 0.0f};
    std::array<float, 3> centre{};      // on the axis, nearest the origin
    std::array<float, 3> slide{};       // along the axis
    std::array<float, 3> translation{};
    Matrix inverse{};                   // of the whole motion
};
[[nodiscard]] RigidMotion rigid_motion(const Matrix &m) noexcept;
// The motion a fraction `t` of the way: 0 is the identity, 1 the motion.
[[nodiscard]] Matrix rigid_at(const RigidMotion &motion, float t) noexcept;

// Where a draw is between two frames, in eye space (view times world), when
// the camera moved by `camera` (Matching::camera, taken apart): its own
// motion is blended in the older frame's eye space and the camera's motion
// is followed along its arcs. A draw that stays put in eye space, like the
// character the camera follows, stays put.
[[nodiscard]] Matrix blend_eye(const Matrix &older, const Matrix &newer, const RigidMotion &camera,
                               float t) noexcept;

} // namespace mhp3rd::gpu::interpolation
