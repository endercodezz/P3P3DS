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

    // 6. Direct Partition Allocator test: LowAligned allocation matching P3P UserSbrk
    {
        p3p3ds::KernelState kernel;
        auto &sm = kernel.sysmem();

        // P3P UserSbrk params: partition 2, name "UserSbrk", type 3 (LowAligned), size 16 MiB, align 4096
        const std::int32_t uid = sm.alloc_partition_memory(2u, "UserSbrk", 3u, 0x01000000u, 4096u);
        TEST_CHECK(uid > 0);
        TEST_CHECK_EQ(sm.block_count(), 1u);

        const std::uint32_t head = sm.get_block_head_addr(uid);
        TEST_CHECK_EQ(head & 0xFFFu, 0u); // 4096-aligned
        TEST_CHECK(head >= p3p3ds::hle::SysMemManager::kDefaultUserHeapBase);
        TEST_CHECK(head + 0x01000000u <= p3p3ds::hle::SysMemManager::kDefaultUserHeapEnd);

        const auto *blk = sm.find_block(uid);
        TEST_CHECK(blk != nullptr);
        TEST_CHECK_EQ(blk->name, std::string("UserSbrk"));
        TEST_CHECK_EQ(blk->size, 0x01000000u);
        TEST_CHECK_EQ(static_cast<std::uint32_t>(blk->type), 3u);

        // Free block
        TEST_CHECK_EQ(sm.free_partition_memory(uid), 0);
        TEST_CHECK_EQ(sm.block_count(), 0u);
        TEST_CHECK_EQ(sm.get_block_head_addr(uid), 0u);

        std::cout << "  [PASS] Direct SysMemManager LowAligned 16 MiB UserSbrk allocation\n";
    }

    // 7. Runtime HLE dispatch test: sceKernelAllocPartitionMemory + sceKernelGetBlockHeadAddr
    {
        psprecomp::Runtime runtime;
        p3p3ds::KernelState kernel;

        p3p3ds::hle::register_all_hle_modules(runtime, kernel);

        // Store "UserSbrk" in guest RAM
        const std::uint32_t name_addr = 0x08880000u;
        runtime.memory().copy_in(name_addr, std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t *>("UserSbrk\0"), 9u));

        // Setup GPRs: a0=2 (user partition), a1=name_addr, a2=3 (LowAligned), a3=0x1000000 (16 MiB), t0=4096 (align)
        runtime.cpu().set_gpr(4, 2u);
        runtime.cpu().set_gpr(5, name_addr);
        runtime.cpu().set_gpr(6, 3u);
        runtime.cpu().set_gpr(7, 0x01000000u);
        runtime.cpu().set_gpr(8, 4096u); // $t0 / 5th arg
        runtime.cpu().set_gpr(2, 0xDEADBEEFu);

        runtime.invoke_import("SysMemUserForUser", 0x237DBD4Fu, runtime.cpu());
        TEST_CHECK(!runtime.stopped());
        const auto uid = static_cast<std::int32_t>(runtime.cpu().gpr[2]);
        TEST_CHECK(uid > 0);

        // Now call sceKernelGetBlockHeadAddr with a0 = uid
        runtime.cpu().set_gpr(4, static_cast<std::uint32_t>(uid));
        runtime.cpu().set_gpr(2, 0u);

        runtime.invoke_import("SysMemUserForUser", 0x9D9A5BA1u, runtime.cpu());
        TEST_CHECK(!runtime.stopped());
        const std::uint32_t head_addr = runtime.cpu().gpr[2];
        TEST_CHECK(head_addr != 0u);
        TEST_CHECK_EQ(head_addr & 0xFFFu, 0u);

        std::cout << "  [PASS] Runtime HLE dispatch sceKernelAllocPartitionMemory + sceKernelGetBlockHeadAddr\n";
    }

    if (g_failure_count > 0) {
        std::cerr << "[FAIL] " << g_failure_count << " test check(s) failed.\n";
        return 1;
    }

    std::cout << "[TEST] All SysMem HLE tests passed successfully.\n";
    return 0;
}
