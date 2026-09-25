#pragma once

#include "psprecomp/decoder.hpp"

#include <string_view>

namespace psprecomp::codegen {

// PSP cache-maintenance instructions operate on a cache that does not exist in
// the native host mapping.  Guest RAM is already coherent, so CACHE is only a
// hint here and must not become a host synchronization point.  SYNC remains a
// real ordering operation.
[[nodiscard]] constexpr std::string_view memory_ordering_statement(OpcodeKind kind) noexcept {
    switch (kind) {
    case OpcodeKind::Sync:
        return "    rt.memory().memory_barrier();\n";
    case OpcodeKind::Cache:
        return "    // PSP CACHE is a no-op in coherent host memory.\n";
    default:
        return {};
    }
}

} // namespace psprecomp::codegen
