#pragma once

namespace psprecomp {
class Runtime;
}

namespace p3p3ds {
class KernelState;
}

namespace p3p3ds::hle {

void register_all_hle_modules(psprecomp::Runtime &runtime, KernelState &kernel);
void register_utils_module(psprecomp::Runtime &runtime, KernelState &kernel);
void register_ctrl_module(psprecomp::Runtime &runtime, KernelState &kernel);
void register_utility_module(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
