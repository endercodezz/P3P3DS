#pragma once

#include <array>
#include <cstddef>
#include <limits>

namespace vcs {

// GE affine matrices use the same layout as ge_renderer::transform_4x3:
//
//   view = A * world + t
//   A = [m0 m3 m6; m1 m4 m7; m2 m5 m8], t = [m9 m10 m11].
//
// view_to_world is A^-1 in row-major order. The three named axes are its
// columns, so A^-1 * view_direction is exactly
// right*x + up*y + forward*z. They are intentionally not normalized: retaining
// scale/shear makes the result a true inverse for every nonsingular GE view.
struct GeCloudCameraFrame {
    std::array<float, 9> view_to_world{};
    std::array<float, 3> position{};
    std::array<float, 3> right{};
    std::array<float, 3> up{};
    std::array<float, 3> forward{};
};

namespace ge_cloud_camera_detail {

constexpr double absolute(double value) noexcept {
    return value < 0.0 ? -value : value;
}

constexpr bool finite(double value) noexcept {
    return value == value &&
           value <= std::numeric_limits<double>::max() &&
           value >= -std::numeric_limits<double>::max();
}

constexpr bool finite_float_result(double value) noexcept {
    return finite(value) &&
           absolute(value) <= static_cast<double>(std::numeric_limits<float>::max());
}

} // namespace ge_cloud_camera_detail

// Computes a full inverse of the GE view's 3x3 linear part. Returns false for
// non-finite or (numerically) singular input and leaves `out` unchanged.
[[nodiscard]] inline constexpr bool ge_cloud_invert_view_linear(
    const std::array<float, 12> &view,
    std::array<float, 9> &out) noexcept {
    using ge_cloud_camera_detail::absolute;
    using ge_cloud_camera_detail::finite;
    using ge_cloud_camera_detail::finite_float_result;

    for (float value : view) {
        if (!finite(static_cast<double>(value))) return false;
    }

    const double a = view[0], b = view[3], c = view[6];
    const double d = view[1], e = view[4], f = view[7];
    const double g = view[2], h = view[5], i = view[8];

    double scale = 0.0;
    for (std::size_t index = 0; index < 9; ++index) {
        const double value = absolute(static_cast<double>(view[index]));
        if (value > scale) scale = value;
    }
    if (scale == 0.0) return false;

    const double determinant =
        a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    const double singular_threshold =
        8.0 * static_cast<double>(std::numeric_limits<float>::epsilon()) *
        scale * scale * scale;
    if (!finite(determinant) || absolute(determinant) <= singular_threshold)
        return false;

    const double inverse_determinant = 1.0 / determinant;
    const std::array<double, 9> inverse{
        (e * i - f * h) * inverse_determinant,
        (c * h - b * i) * inverse_determinant,
        (b * f - c * e) * inverse_determinant,
        (f * g - d * i) * inverse_determinant,
        (a * i - c * g) * inverse_determinant,
        (c * d - a * f) * inverse_determinant,
        (d * h - e * g) * inverse_determinant,
        (b * g - a * h) * inverse_determinant,
        (a * e - b * d) * inverse_determinant,
    };

    std::array<float, 9> result{};
    for (std::size_t index = 0; index < inverse.size(); ++index) {
        if (!finite_float_result(inverse[index])) return false;
        result[index] = static_cast<float>(inverse[index]);
    }
    out = result;
    return true;
}

// Builds the world-space camera origin and the exact view-to-world basis from
// a GE world-to-view matrix. Returns false and leaves `out` unchanged if the
// matrix cannot be inverted or any derived value is non-finite.
[[nodiscard]] inline constexpr bool ge_cloud_camera_frame_from_view(
    const std::array<float, 12> &view,
    GeCloudCameraFrame &out) noexcept {
    std::array<float, 9> inverse{};
    if (!ge_cloud_invert_view_linear(view, inverse)) return false;

    const double tx = view[9], ty = view[10], tz = view[11];
    const std::array<double, 3> position{
        -(static_cast<double>(inverse[0]) * tx +
          static_cast<double>(inverse[1]) * ty +
          static_cast<double>(inverse[2]) * tz),
        -(static_cast<double>(inverse[3]) * tx +
          static_cast<double>(inverse[4]) * ty +
          static_cast<double>(inverse[5]) * tz),
        -(static_cast<double>(inverse[6]) * tx +
          static_cast<double>(inverse[7]) * ty +
          static_cast<double>(inverse[8]) * tz),
    };
    for (double value : position) {
        if (!ge_cloud_camera_detail::finite_float_result(value)) return false;
    }

    GeCloudCameraFrame result{};
    result.view_to_world = inverse;
    result.position = {static_cast<float>(position[0]),
                       static_cast<float>(position[1]),
                       static_cast<float>(position[2])};
    result.right = {inverse[0], inverse[3], inverse[6]};
    result.up = {inverse[1], inverse[4], inverse[7]};
    result.forward = {inverse[2], inverse[5], inverse[8]};
    out = result;
    return true;
}

namespace ge_cloud_camera_detail {

// Compile-time contract check using an exact 90-degree rotation and
// translation. This also catches accidental row/column transposition.
static_assert([] {
    constexpr std::array<float, 12> view{
         0.0f, 1.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f,
         3.0f, -2.0f, -4.0f,
    };
    GeCloudCameraFrame frame{};
    return ge_cloud_camera_frame_from_view(view, frame) &&
           frame.position == std::array<float, 3>{2.0f, 3.0f, 4.0f} &&
           frame.right == std::array<float, 3>{0.0f, -1.0f, 0.0f} &&
           frame.up == std::array<float, 3>{1.0f, 0.0f, 0.0f} &&
           frame.forward == std::array<float, 3>{0.0f, 0.0f, 1.0f};
}());

static_assert([] {
    constexpr std::array<float, 12> singular{
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f,
    };
    GeCloudCameraFrame frame{};
    return !ge_cloud_camera_frame_from_view(singular, frame);
}());

} // namespace ge_cloud_camera_detail
} // namespace vcs
