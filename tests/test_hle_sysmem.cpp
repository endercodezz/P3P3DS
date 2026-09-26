#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/hle/sysmem.hpp"
#include "p3p3ds/hle/hle_modules.hpp"

#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/runtime.hpp"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace {

int g_failure_count = 0;

void check_condition(bool condition, std::string_view expr, std::string_view file, int line) {
    if (!condition) {
        std::cerr << "FAILED check at " << file << ":" << line << ": (" << expr << ")\n";
        ++g_failure_count;
    }
}

template <typename T, typename U>
void check_equal(const T &actual, const U &expected, std::string_view expr_actual,
                 std::string_view expr_expected, std::string_view file, int line) {
    if (actual != expected) {
        std::cerr << "FAILED check at " << file << ":" << line << ": "
                  << expr_actual << " == " << expr_expected << "\n"
                  << "  Actual:   0x" << std::hex << actual << "\n"
                  << "  Expected: 0x" << std::hex << expected << std::dec << "\n";
        ++g_failure_count;
    }
}

#define TEST_CHECK(cond) check_condition((cond), #cond, __FILE__, __LINE__)
#define TEST_CHECK_EQ(act, exp) check_equal((act), (exp), #act, #exp, __FILE__, __LINE__)

} // namespace

int main() {
    std::cout << "[TEST] Running focused SysMem HLE unit tests...\n";

    // 1. Direct function semantics test: sceKernelSetCompiledSdkVersion600_602
    {
        p3p3ds::KernelState direct_kernel;
        TEST_CHECK(!direct_kernel.has_compiled_sdk_version());
        TEST_CHECK_EQ(direct_kernel.compiled_sdk_version(), 0u);

        p3p3ds::hle::sceKernelSetCompiledSdkVersion600_602(direct_kernel, 0x06020010u);
        TEST_CHECK(direct_kernel.has_compiled_sdk_version());
        TEST_CHECK_EQ(direct_kernel.compiled_sdk_version(), 0x06020010u);
        TEST_CHECK((direct_kernel.system_flags() & 0x1000u) != 0u);
        std::cout << "  [PASS] Direct sceKernelSetCompiledSdkVersion600_602 semantics\n";
    }

    // 2. Direct function semantics test: sceKernelSetCompilerVersion
    {
        p3p3ds::KernelState direct_kernel;
        TEST_CHECK(!direct_kernel.has_compiler_version());
        TEST_CHECK_EQ(direct_kernel.compiler_version(), 0u);

        p3p3ds::hle::sceKernelSetCompilerVersion(direct_kernel, 0x00030306u);
        TEST_CHECK(direct_kernel.has_compiler_version());
        TEST_CHECK_EQ(direct_kernel.compiler_version(), 0x00030306u);
        TEST_CHECK((direct_kernel.system_flags() & 0x2000u) != 0u);
        std::cout << "  [PASS] Direct sceKernelSetCompilerVersion semantics\n";
    }

    // 3. Runtime import dispatch test: SysMemUserForUser::0x35669D4C
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
        TEST_CHECK(!runtime.stopped());

        // Verify kernel state recorded expected SDK version and flag
        TEST_CHECK(kernel.has_compiled_sdk_version());
        TEST_CHECK_EQ(kernel.compiled_sdk_version(), 0x06020010u);
        TEST_CHECK((kernel.system_flags() & 0x1000u) != 0u);

        // Verify guest return value in $v0 is 0 (SCE_KERNEL_ERROR_OK)
        TEST_CHECK_EQ(runtime.cpu().gpr[2], 0u);

        std::cout << "  [PASS] Runtime import dispatch (SysMemUserForUser::0x35669D4C)\n";
    }

    // 4. Runtime import dispatch test: SysMemUserForUser::0xF77D77CB
    {
        psprecomp::Runtime runtime;
        p3p3ds::KernelState kernel;

        p3p3ds::hle::register_all_hle_modules(runtime, kernel);

        // Prepare guest register state
        runtime.cpu().set_gpr(4, 0x00030306u); // $a0 = compiler version (3.3.6)
        runtime.cpu().set_gpr(2, 0xCAFEBABEu); // $v0 = dirty value

        // Dispatch SysMemUserForUser::0xF77D77CB (sceKernelSetCompilerVersion)
        runtime.invoke_import("SysMemUserForUser", 0xF77D77CBu, runtime.cpu());

        // Verify runtime did not encounter missing import error
        TEST_CHECK(!runtime.stopped());

        // Verify kernel state recorded expected compiler version and flag
        TEST_CHECK(kernel.has_compiler_version());
        TEST_CHECK_EQ(kernel.compiler_version(), 0x00030306u);
        TEST_CHECK((kernel.system_flags() & 0x2000u) != 0u);

        // Verify guest return value in $v0 is 0 (SCE_KERNEL_ERROR_OK)
        TEST_CHECK_EQ(runtime.cpu().gpr[2], 0u);

        std::cout << "  [PASS] Runtime import dispatch (SysMemUserForUser::0xF77D77CB)\n";
    }

    // 5. Combined sequence test: both services invoked on same kernel state
    {
        psprecomp::Runtime runtime;
        p3p3ds::KernelState kernel;

        p3p3ds::hle::register_all_hle_modules(runtime, kernel);

        runtime.cpu().set_gpr(4, 0x06020010u);
        runtime.cpu().set_gpr(2, 0x11111111u);
        runtime.invoke_import("SysMemUserForUser", 0x35669D4Cu, runtime.cpu());
        TEST_CHECK(!runtime.stopped());
        TEST_CHECK_EQ(runtime.cpu().gpr[2], 0u);

        runtime.cpu().set_gpr(4, 0x00030306u);
        runtime.cpu().set_gpr(2, 0x22222222u);
        runtime.invoke_import("SysMemUserForUser", 0xF77D77CBu, runtime.cpu());
        TEST_CHECK(!runtime.stopped());
        TEST_CHECK_EQ(runtime.cpu().gpr[2], 0u);

        TEST_CHECK_EQ(kernel.compiled_sdk_version(), 0x06020010u);
        TEST_CHECK_EQ(kernel.compiler_version(), 0x00030306u);
        TEST_CHECK_EQ(kernel.system_flags(), 0x3000u); // 0x1000 | 0x2000
        std::cout << "  [PASS] Combined SysMem services sequence (0x35669D4C + 0xF77D77CB)\n";
    }

    if (g_failure_count > 0) {
        std::cerr << "[FAIL] " << g_failure_count << " test check(s) failed.\n";
        return 1;
    }

    std::cout << "[TEST] All SysMem HLE tests passed successfully.\n";
    return 0;
}
