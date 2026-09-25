#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/hle/sysmem.hpp"
#include "p3p3ds/hle/hle_modules.hpp"

#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/runtime.hpp"

#include <cassert>
#include <iostream>

int main() {
    std::cout << "[TEST] Running focused SysMem HLE unit tests...\n";

    // 1. Direct function semantics test
    {
        p3p3ds::KernelState direct_kernel;
        assert(!direct_kernel.has_compiled_sdk_version());
        assert(direct_kernel.compiled_sdk_version() == 0u);

        p3p3ds::hle::sceKernelSetCompiledSdkVersion600_602(direct_kernel, 0x06020010u);
        assert(direct_kernel.has_compiled_sdk_version());
        assert(direct_kernel.compiled_sdk_version() == 0x06020010u);
        assert((direct_kernel.system_flags() & 0x1000u) != 0u);
        std::cout << "  [PASS] Direct sceKernelSetCompiledSdkVersion600_602 semantics\n";
    }

    // 2. Runtime import dispatch test
    {
        psprecomp::Runtime runtime;
        p3p3ds::KernelState kernel;

        p3p3ds::hle::register_all_hle_modules(runtime, kernel);

        // Prepare guest register state
        runtime.cpu().set_gpr(4, 0x06020010u); // $a0 = compiled SDK version (6.2.0 r16)
        runtime.cpu().set_gpr(2, 0xDEADBEEFu); // $v0 = dirty value

        // Dispatch SysMemUserForUser::0x35669D4C (sceKernelSetCompiledSdkVersion600_602)
        runtime.invoke_import("SysMemUserForUser", 0x35669D4Cu, runtime.cpu());

        // Verify runtime did not encounter missing import error
        assert(!runtime.stopped());

        // Verify kernel state recorded expected SDK version
        assert(kernel.has_compiled_sdk_version());
        assert(kernel.compiled_sdk_version() == 0x06020010u);
        assert((kernel.system_flags() & 0x1000u) != 0u);

        // Verify guest return value in $v0 is 0 (SCE_KERNEL_ERROR_OK)
        assert(runtime.cpu().gpr[2] == 0u);

        std::cout << "  [PASS] Runtime import dispatch (SysMemUserForUser::0x35669D4C)\n";
    }

    std::cout << "[TEST] All SysMem HLE tests passed successfully.\n";
    return 0;
}
