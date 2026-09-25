#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/hle/sysmem.hpp"

namespace p3p3ds::hle {

void register_all_hle_modules(psprecomp::Runtime &runtime, KernelState &kernel) {
    register_sysmem_user_for_user(runtime, kernel);
}

} // namespace p3p3ds::hle
