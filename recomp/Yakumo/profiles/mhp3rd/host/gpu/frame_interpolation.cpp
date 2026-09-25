#include "frame_interpolation.hpp"

#include <algorithm>
#include <cmath>

namespace mhp3rd::gpu::interpolation {
namespace {

constexpr float kPi = 3.14159265358979f;

float length3(const float *v) noexcept { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

} // namespace

DrawSummary summarize(const DrawCall &call) {
    DrawSummary draw{};
    draw.vertex_address = call.vertex_address;
    draw.index_address = call.index_address;
    draw.vertex_type = call.vertex_type;
    draw.texture_address = call.texture.enabled ? call.texture.address : 0u;
    draw.count = call.primitive_count;
    draw.primitive = call.primitive;
    draw.target = call.target.color_address;
    draw.perspective = !call.through && !call.clear_mode && !is_orthographic(call.projection);
    // Vertex type bits 9..10 give the weight format; ge_state.cpp's
    // decode_vertices skins exactly the transformed vertices that have one.
    draw.skinned = ((call.vertex_type >> 9u) & 3u) != 0u && !call.through;
    draw.world = call.world;
    draw.view = call.view;
    draw.projection = call.projection;
    draw.key_hash = Matcher::hash_of(draw);
    draw.eye_translation = Matcher::eye_translation_of(draw);
    draw.prepared = true;
    return draw;
}

void mark_eligible(std::vector<DrawSummary> &draws, std::uint32_t displayed) noexcept {
    for (DrawSummary &draw : draws) draw.eligible = draw.perspective && draw.target == displayed;
}

bool is_orthographic(const Matrix &projection) noexcept {
    // A perspective projection copies -z into w: row 3 is (0, 0, -1, 0).
    return std::fabs(projection[11]) < 1e-6f;
}

std::size_t Matcher::KeyHash::operator()(const Key &key) const noexcept {
    std::uint64_t hash = 1469598103934665603ull;
    const auto mix = [&](std::uint32_t value) {
        hash ^= value;
        hash *= 1099511628211ull;
    };
    mix(key.vertex_address);
    mix(key.index_address);
    mix(key.vertex_type);
    mix(key.texture_address);
    mix(key.count);
    mix(key.primitive);
    return static_cast<std::size_t>(hash);
}

std::uint64_t Matcher::hash_of(const DrawSummary &draw) noexcept { return KeyHash{}(key_of(draw)); }

std::array<float, 3> Matcher::eye_translation_of(const DrawSummary &draw) noexcept {
    // Column 3 of multiply(view, world), term for term as multiply() sums it.
    std::array<float, 3> column{};
    for (std::uint32_t row = 0; row < 3u; ++row) {
        float sum = 0.0f;
        for (std::uint32_t k = 0; k < 4u; ++k) sum += draw.view[k * 4u + row] * draw.world[12u + k];
        column[row] = sum;
    }
    return column;
}

Matcher::Key Matcher::key_of(const DrawSummary &draw) noexcept {
    return Key{draw.vertex_address, draw.index_address, draw.vertex_type, draw.texture_address, draw.count,
               static_cast<std::uint8_t>(draw.primitive)};
}

const Matching &Matcher::match(const std::vector<DrawSummary> &older, const std::vector<DrawSummary> &newer,
                               const CutThresholds &thresholds) {
    Matching &out = result_;
    // The vector keeps its capacity from one frame to the next.
    std::vector<std::int32_t> newer_of = std::move(out.newer_of);
    out = Matching{};
    out.newer_of = std::move(newer_of);
    out.newer_of.assign(older.size(), -1);

    // The newer frame's eligible draws by key, in an open-addressed table
    // rebuilt each frame: each key's draws are chained in drawing order, and
    // the chain's cursor hands them to the older frame's draws of that key
    // one after another.
    std::size_t eligible = 0u;
    for (const DrawSummary &draw : newer) eligible += draw.eligible ? 1u : 0u;
    std::size_t capacity = 64u;
    while (capacity < eligible * 2u) capacity *= 2u;
    slots_.assign(capacity, Slot{});
    next_.assign(newer.size(), -1);
    const std::size_t mask = capacity - 1u;
    const auto slot_for = [&](const Key &key, const DrawSummary &draw) -> Slot & {
        std::size_t at = static_cast<std::size_t>(draw.prepared ? draw.key_hash : KeyHash{}(key)) & mask;
        while (slots_[at].first >= 0 && !(slots_[at].key == key)) at = (at + 1u) & mask;
        return slots_[at];
    };
    for (std::size_t i = 0; i < newer.size(); ++i) {
        if (!newer[i].eligible) continue;
        ++out.eligible_newer;
        const Key key = key_of(newer[i]);
        Slot &slot = slot_for(key, newer[i]);
        const auto index = static_cast<std::int32_t>(i);
        if (slot.first < 0) {
            slot.key = key;
            slot.first = slot.cursor = index;
        } else {
            next_[static_cast<std::size_t>(slot.last)] = index;
        }
        slot.last = index;
    }
    shared_.assign(older.size(), 0u);
    for (std::size_t i = 0; i < older.size(); ++i) {
        if (!older[i].eligible) continue;
        ++out.eligible_older;
        Slot &slot = slot_for(key_of(older[i]), older[i]);
        if (slot.first < 0 || slot.cursor < 0) continue;
        out.newer_of[i] = slot.cursor;
        shared_[i] = slot.first != slot.last ? 1u : 0u;
        slot.cursor = next_[static_cast<std::size_t>(slot.cursor)];
        ++out.matched;
    }

    // How the camera moved: this game keeps the view matrix almost fixed and
    // puts the camera's rotation into every world matrix, so the camera shows
    // in how each matched draw moved in eye space (view times world). Most of
    // a scene stands still, so the median over the matched draws is the
    // camera's turn and move, and characters moving on their own do not
    // count. A sample of up to 256 pairs spread over the frame is enough.
    struct Sample {
        float turn;
        Matrix motion;
    };
    std::vector<Sample> samples;
    const std::size_t stride = std::max<std::size_t>(1u, out.matched / 256u);
    std::size_t seen = 0u;
    for (std::size_t i = 0; i < older.size(); ++i) {
        const std::int32_t partner = out.newer_of[i];
        if (partner < 0 || seen++ % stride != 0u) continue;
        const DrawSummary &from = older[i];
        const DrawSummary &to = newer[static_cast<std::size_t>(partner)];
        Matrix before_inverse{};
        if (!affine_inverse(multiply(from.view, from.world), before_inverse)) continue;
        const Matrix motion = multiply(multiply(to.view, to.world), before_inverse);
        Matrix identity{};
        identity[0] = identity[5] = identity[10] = identity[15] = 1.0f;
        samples.push_back({rotation_angle_degrees(identity, motion), motion});
    }
    if (!samples.empty()) {
        const auto middle = samples.begin() + static_cast<std::ptrdiff_t>(samples.size() / 2u);
        std::nth_element(samples.begin(), middle, samples.end(),
                         [](const Sample &a, const Sample &b) { return a.turn < b.turn; });
        out.camera_found = true;
        out.camera_angle_degrees = middle->turn;
        out.camera = middle->motion;
        out.camera_distance = std::sqrt(out.camera[12] * out.camera[12] + out.camera[13] * out.camera[13] +
                                        out.camera[14] * out.camera[14]);
    }

    // Each pair's own motion: where the camera's motion would have taken the
    // older draw against where the newer one is, in eye space.
    if (out.camera_found && thresholds.max_own_motion > 0.0f) {
        const Matrix &c = out.camera;
        // Only the translation column of view times world is used; summarize()
        // stores it (eye_translation_of, the same sums as multiply()).
        for (std::size_t i = 0; i < older.size(); ++i) {
            const std::int32_t partner = out.newer_of[i];
            if (partner < 0) continue;
            const DrawSummary &from = older[i];
            const DrawSummary &to = newer[static_cast<std::size_t>(partner)];
            const std::array<float, 3> before_column = from.prepared ? from.eye_translation : eye_translation_of(from);
            const std::array<float, 3> after_column = to.prepared ? to.eye_translation : eye_translation_of(to);
            float distance = 0.0f;
            for (std::size_t row = 0; row < 3u; ++row) {
                const float predicted = c[row] * before_column[0] + c[4u + row] * before_column[1] +
                                        c[8u + row] * before_column[2] + c[12u + row];
                const float d = after_column[row] - predicted;
                distance += d * d;
            }
            distance = std::sqrt(distance);
            if (distance > thresholds.max_own_motion) {
                out.newer_of[i] = Matching::kFollowCamera;
                ++out.rejected;
                out.max_rejected_motion = std::max(out.max_rejected_motion, distance);
                if (shared_[i] != 0u) ++out.rejected_shared;
            } else {
                out.max_own_motion = std::max(out.max_own_motion, distance);
            }
        }
    }

    float max_angle = thresholds.max_camera_angle_degrees;
    float max_distance = thresholds.max_camera_distance;
    if (previous_blended_) {
        max_angle = std::clamp(previous_angle_degrees_ * thresholds.continuous_growth +
                                   thresholds.continuous_angle_margin_degrees,
                               max_angle, std::max(max_angle, thresholds.max_continuous_angle_degrees));
        max_distance = std::clamp(previous_distance_ * thresholds.continuous_growth +
                                      thresholds.continuous_distance_margin,
                                  max_distance, std::max(max_distance, thresholds.max_continuous_distance));
    }
    if (out.eligible_newer == 0u || out.matched == 0u) {
        out.cut = "nothing to blend";
    } else if (static_cast<float>(out.matched) <
               thresholds.min_matched_fraction * static_cast<float>(std::max(out.eligible_newer, out.eligible_older))) {
        out.cut = "few draws match";
    } else if (out.camera_found && out.camera_angle_degrees > max_angle) {
        out.cut = "camera turned";
    } else if (out.camera_found && out.camera_distance > max_distance) {
        out.cut = "camera moved";
    }
    out.continued = out.cut == nullptr && out.camera_found &&
                    (out.camera_angle_degrees > thresholds.max_camera_angle_degrees ||
                     out.camera_distance > thresholds.max_camera_distance);
    previous_blended_ = out.cut == nullptr;
    previous_angle_degrees_ = out.camera_found ? out.camera_angle_degrees : 0.0f;
    previous_distance_ = out.camera_found ? out.camera_distance : 0.0f;
    return out;
}

Matrix blend_affine(const Matrix &a, const Matrix &b, float t) noexcept {
    Matrix out = blend_linear(a, b, t);
    for (std::uint32_t column = 0; column < 3u; ++column) {
        float *axis = out.data() + column * 4u;
        const float length = length3(axis);
        if (length < 1e-12f) continue;
        const float wanted = length3(a.data() + column * 4u) * (1.0f - t) + length3(b.data() + column * 4u) * t;
        const float scale = wanted / length;
        for (std::uint32_t row = 0; row < 3u; ++row) axis[row] *= scale;
    }
    return out;
}

Matrix multiply(const Matrix &a, const Matrix &b) noexcept {
    Matrix out{};
    for (std::uint32_t column = 0; column < 4u; ++column) {
        for (std::uint32_t row = 0; row < 4u; ++row) {
            float sum = 0.0f;
            for (std::uint32_t k = 0; k < 4u; ++k) sum += a[k * 4u + row] * b[column * 4u + k];
            out[column * 4u + row] = sum;
        }
    }
    return out;
}

Matrix blend_linear(const Matrix &a, const Matrix &b, float t) noexcept {
    Matrix out{};
    for (std::size_t i = 0; i < out.size(); ++i) out[i] = a[i] + (b[i] - a[i]) * t;
    return out;
}

float rotation_angle_degrees(const Matrix &a, const Matrix &b) noexcept {
    // trace(Ra^T Rb) = 1 + 2 cos(angle) for two rotations; the columns are
    // normalised first so a uniform scale does not read as a turn.
    float trace = 0.0f;
    for (std::uint32_t column = 0; column < 3u; ++column) {
        const float *x = a.data() + column * 4u;
        const float *y = b.data() + column * 4u;
        const float lengths = length3(x) * length3(y);
        if (lengths < 1e-12f) return 180.0f;
        trace += (x[0] * y[0] + x[1] * y[1] + x[2] * y[2]) / lengths;
    }
    const float cosine = std::clamp((trace - 1.0f) * 0.5f, -1.0f, 1.0f);
    return std::acos(cosine) * 180.0f / kPi;
}

float blend_offset(float from, float to, float t, float max_scroll) noexcept {
    return scrolls(from, to, max_scroll) ? from + (to - from) * t : from;
}

bool affine_inverse(const Matrix &m, Matrix &out) noexcept {
    // The 3x3 part by its adjugate; column-major, element (row, column) at
    // column * 4 + row.
    const auto at = [&](int row, int column) { return m[static_cast<std::size_t>(column * 4 + row)]; };
    const float c00 = at(1, 1) * at(2, 2) - at(1, 2) * at(2, 1);
    const float c01 = at(1, 2) * at(2, 0) - at(1, 0) * at(2, 2);
    const float c02 = at(1, 0) * at(2, 1) - at(1, 1) * at(2, 0);
    const float determinant = at(0, 0) * c00 + at(0, 1) * c01 + at(0, 2) * c02;
    if (!(std::fabs(determinant) > 1e-12f)) return false;
    const float f = 1.0f / determinant;
    Matrix r{};
    const auto set = [&](int row, int column, float value) { r[static_cast<std::size_t>(column * 4 + row)] = value; };
    set(0, 0, c00 * f);
    set(1, 0, c01 * f);
    set(2, 0, c02 * f);
    set(0, 1, (at(0, 2) * at(2, 1) - at(0, 1) * at(2, 2)) * f);
    set(1, 1, (at(0, 0) * at(2, 2) - at(0, 2) * at(2, 0)) * f);
    set(2, 1, (at(0, 1) * at(2, 0) - at(0, 0) * at(2, 1)) * f);
    set(0, 2, (at(0, 1) * at(1, 2) - at(0, 2) * at(1, 1)) * f);
    set(1, 2, (at(0, 2) * at(1, 0) - at(0, 0) * at(1, 2)) * f);
    set(2, 2, (at(0, 0) * at(1, 1) - at(0, 1) * at(1, 0)) * f);
    for (int row = 0; row < 3; ++row) {
        float sum = 0.0f;
        for (int k = 0; k < 3; ++k) sum += r[static_cast<std::size_t>(k * 4 + row)] * m[static_cast<std::size_t>(12 + k)];
        set(row, 3, -sum);
    }
    r[15] = 1.0f;
    out = r;
    return true;
}

namespace {

using Vec3 = std::array<float, 3>;

Vec3 cross(const Vec3 &a, const Vec3 &b) noexcept {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
float dot(const Vec3 &a, const Vec3 &b) noexcept { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

// The rotation by `angle` radians about the unit `axis`, as a 3x3 inside a
// column-major 4x4.
void put_rotation(Matrix &m, const Vec3 &u, float angle) noexcept {
    const float c = std::cos(angle), s = std::sin(angle), k = 1.0f - c;
    m[0] = c + u[0] * u[0] * k;
    m[1] = u[1] * u[0] * k + u[2] * s;
    m[2] = u[2] * u[0] * k - u[1] * s;
    m[4] = u[0] * u[1] * k - u[2] * s;
    m[5] = c + u[1] * u[1] * k;
    m[6] = u[2] * u[1] * k + u[0] * s;
    m[8] = u[0] * u[2] * k + u[1] * s;
    m[9] = u[1] * u[2] * k - u[0] * s;
    m[10] = c + u[2] * u[2] * k;
}

} // namespace

RigidMotion rigid_motion(const Matrix &m) noexcept {
    RigidMotion motion{};
    if (!affine_inverse(m, motion.inverse)) return motion;
    // The nearest rotation's quaternion, from the columns normalised.
    std::array<Vec3, 3> column{};
    for (std::size_t c = 0; c < 3u; ++c) {
        const float length = length3(m.data() + c * 4u);
        if (length < 1e-12f) return motion;
        for (std::size_t r = 0; r < 3u; ++r) column[c][r] = m[c * 4u + r] / length;
    }
    const float trace = column[0][0] + column[1][1] + column[2][2];
    float w = 0.0f, x = 0.0f, y = 0.0f, z = 0.0f;
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        w = 0.25f * s;
        x = (column[1][2] - column[2][1]) / s;
        y = (column[2][0] - column[0][2]) / s;
        z = (column[0][1] - column[1][0]) / s;
    } else if (column[0][0] > column[1][1] && column[0][0] > column[2][2]) {
        const float s = std::sqrt(1.0f + column[0][0] - column[1][1] - column[2][2]) * 2.0f;
        w = (column[1][2] - column[2][1]) / s;
        x = 0.25f * s;
        y = (column[1][0] + column[0][1]) / s;
        z = (column[2][0] + column[0][2]) / s;
    } else if (column[1][1] > column[2][2]) {
        const float s = std::sqrt(1.0f + column[1][1] - column[0][0] - column[2][2]) * 2.0f;
        w = (column[2][0] - column[0][2]) / s;
        x = (column[1][0] + column[0][1]) / s;
        y = 0.25f * s;
        z = (column[2][1] + column[1][2]) / s;
    } else {
        const float s = std::sqrt(1.0f + column[2][2] - column[0][0] - column[1][1]) * 2.0f;
        w = (column[0][1] - column[1][0]) / s;
        x = (column[2][0] + column[0][2]) / s;
        y = (column[2][1] + column[1][2]) / s;
        z = 0.25f * s;
    }
    if (w < 0.0f) {
        w = -w;
        x = -x;
        y = -y;
        z = -z;
    }
    const float sine = std::sqrt(x * x + y * y + z * z);
    motion.translation = {m[12], m[13], m[14]};
    motion.angle = 2.0f * std::atan2(sine, w);
    motion.valid = true;
    if (sine < 1e-6f) {
        motion.angle = 0.0f;
        motion.slide = motion.translation;
        return motion;
    }
    motion.axis = {x / sine, y / sine, z / sine};
    const Vec3 &u = motion.axis;
    const Vec3 &p = motion.translation;
    const float along = dot(p, u);
    motion.slide = {u[0] * along, u[1] * along, u[2] * along};
    const Vec3 across{p[0] - motion.slide[0], p[1] - motion.slide[1], p[2] - motion.slide[2]};
    // The point the turn leaves in place: (I - R) c = across, with c on the
    // plane through the origin across the axis.
    const float cotangent = 1.0f / std::tan(0.5f * motion.angle);
    const Vec3 side = cross(u, across);
    motion.centre = {0.5f * (across[0] + cotangent * side[0]), 0.5f * (across[1] + cotangent * side[1]),
                     0.5f * (across[2] + cotangent * side[2])};
    return motion;
}

Matrix rigid_at(const RigidMotion &motion, float t) noexcept {
    Matrix out{};
    out[0] = out[5] = out[10] = out[15] = 1.0f;
    if (!motion.valid) return out;
    put_rotation(out, motion.axis, motion.angle * t);
    // x -> R_t (x - c) + c + t * slide
    const Vec3 &c = motion.centre;
    for (std::size_t row = 0; row < 3u; ++row) {
        const float turned = out[row] * c[0] + out[4u + row] * c[1] + out[8u + row] * c[2];
        out[12u + row] = c[row] - turned + t * motion.slide[row];
    }
    return out;
}

Matrix blend_eye(const Matrix &older, const Matrix &newer, const RigidMotion &camera, float t) noexcept {
    if (older == newer) return older;
    if (!camera.valid) return blend_affine(older, newer, t);
    // The newer transform as the older camera would have seen it, then the
    // camera's own motion a fraction of the way.
    const Matrix own = multiply(camera.inverse, newer);
    return multiply(rigid_at(camera, t), blend_affine(older, own, t));
}

} // namespace mhp3rd::gpu::interpolation
