#include "vcs_native_fast_paths.hpp"

#include "vcs_collision_fast_paths.hpp"
#include "vcs_fast_paths.hpp"
#include "psprecomp/common.hpp"
#include "psprecomp/runtime.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

namespace vcs {
namespace {
constexpr std::uint32_t kCollisionFastPath = 0x088B1554u;
constexpr std::uint32_t kVfpuFastPath = 0x088B1780u;

bool truthy(const char *text) {
    return text != nullptr && *text != '\0' && std::strcmp(text, "0") != 0;
}

std::uint64_t validation_limit(const char *name) {
    const char *text = std::getenv(name);
    if (text == nullptr || *text == '\0') return 0u;
    char *end = nullptr;
    const unsigned long long value = std::strtoull(text, &end, 0);
    return end != text && *end == '\0' ? static_cast<std::uint64_t>(value) : 0u;
}

void collision_fast_path(psprecomp::Runtime &runtime, psprecomp::AllegrexContext &ctx) {
    static const bool enabled = [] {
        if (truthy(std::getenv("PSPRECOMP_NO_FAST_088B1554"))) return false;
        if (truthy(std::getenv("PSPRECOMP_VALIDATE_FAST_088B1554"))) return true;
        // This leaf has already matched the generated AOT reference across
        // real gameplay validation runs. Keep it on in production; the NO_
        // switch remains available for an immediate A/B or compatibility bisect.
        return true;
    }();

    if (!enabled) {
        if (!runtime.invoke_isolated_aot(kCollisionFastPath, ctx))
            throw psprecomp::Error("Missing AOT reference for VCS native fast path 0x088B1554");
        return;
    }

    static const std::uint64_t limit = validation_limit("PSPRECOMP_VALIDATE_FAST_088B1554");
    static std::uint64_t count = 0u;
    if (count < limit) {
        const psprecomp::AllegrexContext input = ctx;
        const std::uint32_t point = input.gpr[6];
        const std::uint32_t mindist = input.gpr[7];
        std::uint8_t point_before[32]{};
        for (std::uint32_t i = 0; i < 32u; ++i)
            point_before[i] = runtime.memory().aot_load8(point + i);
        const std::uint32_t mindist_before = runtime.memory().aot_load32(mindist);

        constexpr std::uint32_t validation_return = 0xFFFFFFFFu;
        psprecomp::AllegrexContext reference = input;
        reference.gpr[31] = validation_return;
        if (!runtime.invoke_isolated_aot(kCollisionFastPath, reference))
            throw psprecomp::Error("Missing AOT reference for VCS native fast path 0x088B1554");
        if (reference.pc != validation_return)
            throw psprecomp::Error("VCS native reference 0x088B1554 escaped validation return sentinel");
        reference.gpr[31] = input.gpr[31];
        reference.pc = input.gpr[31];

        std::uint8_t point_expected[32]{};
        for (std::uint32_t i = 0; i < 32u; ++i)
            point_expected[i] = runtime.memory().aot_load8(point + i);
        const std::uint32_t mindist_expected = runtime.memory().aot_load32(mindist);

        for (std::uint32_t i = 0; i < 32u; ++i)
            runtime.memory().aot_store8(point + i, point_before[i]);
        runtime.memory().aot_store32(mindist, mindist_before);
        ctx = input;
        psprecomp::vcs_fast_088B1554(runtime, ctx);
        ctx.gpr[0] = 0u;

        bool memory_matches = runtime.memory().aot_load32(mindist) == mindist_expected;
        for (std::uint32_t i = 0; i < 32u && memory_matches; ++i)
            memory_matches = runtime.memory().aot_load8(point + i) == point_expected[i];
        const bool abi_matches = ctx.pc == reference.pc && ctx.gpr[2] == reference.gpr[2] &&
            ctx.gpr[29] == reference.gpr[29] && ctx.gpr[31] == reference.gpr[31];
        if (!memory_matches || !abi_matches) {
            std::ostringstream message;
            message << "VCS native fast path 0x088B1554 diverged at validation call " << count
                    << " reference_v0=" << psprecomp::hex32(reference.gpr[2])
                    << " fast_v0=" << psprecomp::hex32(ctx.gpr[2])
                    << " reference_dist=" << psprecomp::hex32(mindist_expected)
                    << " fast_dist=" << psprecomp::hex32(runtime.memory().aot_load32(mindist));
            throw psprecomp::Error(message.str());
        }
        ++count;
        if (count == limit)
            std::cerr << "[fast-validate] 0x088B1554 matched " << count << " consecutive real calls\n";
        return;
    }

    psprecomp::vcs_fast_088B1554(runtime, ctx);
}

void vfpu_fast_path(psprecomp::Runtime &runtime, psprecomp::AllegrexContext &ctx) {
    static const std::uint64_t limit = validation_limit("PSPRECOMP_VALIDATE_FAST_088B1780");
    static std::uint64_t count = 0u;
    if (count < limit) {
        psprecomp::AllegrexContext reference = ctx;
        if (!runtime.invoke_isolated_aot(kVfpuFastPath, reference))
            throw psprecomp::Error("Missing AOT reference for VCS native fast path 0x088B1780");
        psprecomp::vcs_fast_088B1780(runtime, ctx);
        ctx.gpr[0] = 0u;
        if (std::memcmp(&reference, &ctx, sizeof(psprecomp::AllegrexContext)) != 0) {
            std::ostringstream message;
            message << "VCS native fast path 0x088B1780 diverged at validation call " << count
                    << " reference_pc=" << psprecomp::hex32(reference.pc)
                    << " fast_pc=" << psprecomp::hex32(ctx.pc)
                    << " reference_v0=" << psprecomp::hex32(reference.gpr[2])
                    << " fast_v0=" << psprecomp::hex32(ctx.gpr[2]);
            throw psprecomp::Error(message.str());
        }
        ++count;
        if (count == limit)
            std::cerr << "[fast-validate] 0x088B1780 matched " << count << " consecutive real calls\n";
        return;
    }
    psprecomp::vcs_fast_088B1780(runtime, ctx);
}
} // namespace

void install_native_fast_paths(psprecomp::Runtime &runtime) {
    runtime.register_native_fast_path(kCollisionFastPath, &collision_fast_path);
    runtime.register_native_fast_path(kVfpuFastPath, &vfpu_fast_path);
}

} // namespace vcs
