#pragma once

#include "psprecomp/runtime.hpp"

#include <bit>
#include <cmath>
#include <cstdint>

namespace psprecomp {

// Native lowering of VCS CCollision::ProcessLineSphere (0x088B1554).
//
// The generated version materializes every three-component VFPU temporary in
// guest stack memory and repeatedly moves it through AllegrexContext.  This
// leaf is called once for every candidate collision sphere in line-of-sight
// queries, so expressing the same math directly avoids a disproportionate
// amount of host work.  Only ABI-visible state is committed: v0, the optional
// CColPoint/mindist writes, and the return PC.  Caller-saved scalar/VFPU
// scratch registers are deliberately not part of the function contract.
inline void vcs_fast_088B1554(Runtime &rt, AllegrexContext &ctx) {
    const std::uint32_t line = ctx.gpr[4];
    const std::uint32_t sphere = ctx.gpr[5];
    const std::uint32_t point = ctx.gpr[6];
    const std::uint32_t mindist_address = ctx.gpr[7];

    // Cache the AOT RAM view once for the whole collision leaf.  This routine
    // performs a dense cluster of loads/stores and otherwise re-enters the
    // GuestMemory address classifier for every component.
    const auto memory = rt.memory().aot_fast_view();
    const auto load_float = [&memory](std::uint32_t address) noexcept {
        return std::bit_cast<float>(memory.aot_load32(address));
    };
    const auto store_float = [&memory](std::uint32_t address, float value) noexcept {
        memory.aot_store32(address, std::bit_cast<std::uint32_t>(value));
    };
    const auto allegrex_mul = [](float left, float right) noexcept {
        if ((std::isinf(left) && right == 0.0f) ||
            (std::isinf(right) && left == 0.0f)) {
            return std::bit_cast<float>(0x7FC00000u);
        }
        return left * right;
    };
    const auto dot3 = [](const float left[3], const float right[3]) noexcept {
        // VFPU VDOT accumulates from an exact +0 in lane order.
        float result = 0.0f;
        result += left[0] * right[0];
        result += left[1] * right[1];
        result += left[2] * right[2];
        result += 0.0f;
        return result;
    };

    const float p0[3]{load_float(line + 0u), load_float(line + 4u), load_float(line + 8u)};
    const float p1[3]{load_float(line + 16u), load_float(line + 20u), load_float(line + 24u)};
    const float center[3]{load_float(sphere + 0u), load_float(sphere + 4u),
                          load_float(sphere + 8u)};
    const float direction[3]{p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
    const float to_center[3]{center[0] - p0[0], center[1] - p0[1], center[2] - p0[2]};

    const float line_squared = dot3(direction, direction);
    const float projection = dot3(direction, to_center);
    const float radius = load_float(sphere + 12u);
    const float tangent_squared = allegrex_mul(
        dot3(to_center, to_center) - allegrex_mul(radius, radius), line_squared);
    const float difference_squared = allegrex_mul(projection, projection) - tangent_squared;

    bool hit = !(difference_squared < 0.0f);
    float t = 0.0f;
    if (hit) {
        t = (projection - std::sqrt(difference_squared)) / line_squared;
        const float mindist = load_float(mindist_address);
        // Spell these exactly like the guest branches.  In particular, NaN
        // must fail the <= 1 and < mindist tests.
        hit = !(t < 0.0f) && (t <= 1.0f) && (t < mindist);
    }

    if (hit) {
        const float position[3]{
            p0[0] + direction[0] * t,
            p0[1] + direction[1] * t,
            p0[2] + direction[2] * t,
        };
        float normal[3]{position[0] - center[0], position[1] - center[1],
                        position[2] - center[2]};
        const float normal_squared = dot3(normal, normal);
        if (normal_squared > 0.0f) {
            const float inverse_length = 1.0f / std::sqrt(normal_squared);
            normal[0] *= inverse_length;
            normal[1] *= inverse_length;
            normal[2] *= inverse_length;
        } else {
            // Matches CVector::Normalise's deterministic degenerate fallback.
            normal[0] = 1.0f;
        }

        store_float(point + 0u, position[0]);
        store_float(point + 4u, position[1]);
        store_float(point + 8u, position[2]);
        store_float(point + 16u, normal[0]);
        store_float(point + 20u, normal[1]);
        store_float(point + 24u, normal[2]);
        memory.aot_store8(point + 28u, 0u);
        memory.aot_store8(point + 29u, 0u);
        memory.aot_store8(point + 30u, memory.aot_load8(sphere + 16u));
        memory.aot_store8(point + 31u, memory.aot_load8(sphere + 17u));
        store_float(mindist_address, t);
    }

    ctx.set_gpr(2, hit ? 1u : 0u);
    ctx.pc = ctx.gpr[31];
}

} // namespace psprecomp
