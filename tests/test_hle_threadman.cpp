#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/hle/threadman.hpp"
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
    std::cout << "Running P3P3DS ThreadMan HLE Unit Tests...\n";

    // 1. ThreadMan kernel error code verification against exact numerical values
    {
        TEST_CHECK_EQ(p3p3ds::hle::SCE_KERNEL_ERROR_NOT_DORMANT, static_cast<std::int32_t>(0x800201A4u));
        TEST_CHECK_EQ(p3p3ds::hle::SCE_KERNEL_ERROR_DORMANT, static_cast<std::int32_t>(0x800201A2u));
        TEST_CHECK_EQ(p3p3ds::hle::SCE_KERNEL_ERROR_SUSPEND, static_cast<std::int32_t>(0x800201A3u));
        TEST_CHECK_EQ(p3p3ds::hle::SCE_KERNEL_ERROR_ILLEGAL_THID, static_cast<std::int32_t>(0x80020197u));
        TEST_CHECK_EQ(p3p3ds::hle::SCE_KERNEL_ERROR_UNKNOWN_THID, static_cast<std::int32_t>(0x80020198u));
        TEST_CHECK_EQ(p3p3ds::hle::SCE_KERNEL_ERROR_ILLEGAL_PRIORITY, static_cast<std::int32_t>(0x80020193u));
        TEST_CHECK_EQ(p3p3ds::hle::SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE, static_cast<std::int32_t>(0x80020194u));
        TEST_CHECK_EQ(p3p3ds::hle::SCE_KERNEL_ERROR_ILLEGAL_ENTRY, static_cast<std::int32_t>(0x80020192u));
        TEST_CHECK_EQ(p3p3ds::hle::SCE_KERNEL_ERROR_NO_MEMORY, static_cast<std::int32_t>(0x80020190u));
        std::cout << "  [PASS] Verified exact numerical PSP error constants\n";
    }

    // 2. Thread creation, stack layout, and validation
    {
        psprecomp::GuestMemory memory(32u * 1024u * 1024u);
        p3p3ds::hle::ThreadManager tm;

        const std::int32_t uid1 = tm.create_thread("thread_one", 0x08804000u, 0x20, 0x10000u, 0x80000000u, 0u, memory);
        TEST_CHECK(uid1 > 0);

        const std::int32_t uid2 = tm.create_thread("thread_two", 0x08805000u, 0x22, 0x20000u, 0x80000000u, 0u, memory);
        TEST_CHECK(uid2 > 0);
        TEST_CHECK(uid1 != uid2);

        auto *t1 = tm.get_thread(uid1);
        TEST_CHECK(t1 != nullptr);
        if (t1) {
            TEST_CHECK_EQ(t1->uid, uid1);
            TEST_CHECK(t1->name == "thread_one");
            TEST_CHECK_EQ(t1->entry_pc, 0x08804000u);
            TEST_CHECK_EQ(t1->initial_priority, 0x20);
            TEST_CHECK_EQ(t1->current_priority, 0x20);
            TEST_CHECK_EQ(t1->stack_size, 0x10000u);
            TEST_CHECK(t1->status == p3p3ds::hle::InternalThreadState::Dormant);
            TEST_CHECK_EQ(t1->context.gpr[31], p3p3ds::hle::ThreadManager::kThreadReturnSentinel);
            TEST_CHECK(t1->stack_address >= 0x09000000u && t1->stack_top <= 0x09FF0000u);

            // Verify 0xFF fill in stack body and UID at stack base
            TEST_CHECK_EQ(memory.load32(t1->stack_address), static_cast<std::uint32_t>(uid1));
            TEST_CHECK_EQ(memory.load32(t1->stack_address + 4u), 0xFFFFFFFFu);

            // Verify zero-filled k0 section and k0 UID header
            const std::uint32_t k0 = t1->stack_top - 256u;
            TEST_CHECK_EQ(memory.load32(k0 + 0xC0u), static_cast<std::uint32_t>(uid1));
            TEST_CHECK_EQ(memory.load32(k0 + 0xC8u), t1->stack_address);
            TEST_CHECK_EQ(memory.load32(k0 + 0xF8u), 0xFFFFFFFFu);
            TEST_CHECK_EQ(memory.load32(k0 + 0xFCu), 0xFFFFFFFFu);
        }

        // Error validation
        TEST_CHECK_EQ(tm.create_thread("bad_entry", 0u, 0x20, 0x1000u, 0u, 0u, memory),
                      p3p3ds::hle::SCE_KERNEL_ERROR_ILLEGAL_ENTRY);
        TEST_CHECK_EQ(tm.create_thread("bad_prio", 0x08804000u, 2, 0x1000u, 0u, 0u, memory),
                      p3p3ds::hle::SCE_KERNEL_ERROR_ILLEGAL_PRIORITY);
        TEST_CHECK_EQ(tm.create_thread("bad_stack", 0x08804000u, 0x20, 128u, 0u, 0u, memory),
                      p3p3ds::hle::SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE);

        std::cout << "  [PASS] Thread creation, stack initialization, and parameter validation\n";
    }

    // 3. Test A: Equal priority thread start (no immediate preemption)
    {
        psprecomp::GuestMemory memory(32u * 1024u * 1024u);
        p3p3ds::hle::ThreadManager tm;

        const std::int32_t root_uid = tm.init_root_thread("root", 0x08804108u, 0x09FFFF00u, 0x08C42A50u);
        const std::int32_t child_uid = tm.create_thread("user_main", 0x0880421Cu, 0x20, 0x40000u, 0x80000000u, 0u, memory);

        psprecomp::AllegrexContext caller_ctx{};
        caller_ctx.set_gpr(28, 0x08C42A50u);
        caller_ctx.set_gpr(29, 0x09FFFF00u);
        caller_ctx.set_gpr(31, 0x088041F0u);
        caller_ctx.pc = 0x088041E8u;

        const std::int32_t ret = tm.start_thread(child_uid, 0u, 0u, memory, caller_ctx);
        TEST_CHECK_EQ(ret, 0);

        // Child becomes Ready, root remains Running, current thread remains root
        auto *child = tm.get_thread(child_uid);
        auto *root = tm.get_thread(root_uid);
        TEST_CHECK(child != nullptr && child->status == p3p3ds::hle::InternalThreadState::Ready);
        TEST_CHECK(root != nullptr && root->status == p3p3ds::hle::InternalThreadState::Running);
        TEST_CHECK_EQ(tm.current_thread_id(), root_uid);

        // Caller returns normally with $v0 == 0 and PC intact
        TEST_CHECK_EQ(caller_ctx.gpr[2], 0u);
        TEST_CHECK_EQ(caller_ctx.pc, 0x088041E8u);

        std::cout << "  [PASS] Test A: Equal priority thread start (no preemption, child becomes Ready)\n";
    }

    // 4. Test B: Higher priority child thread start (preemption)
    {
        psprecomp::GuestMemory memory(32u * 1024u * 1024u);
        p3p3ds::hle::ThreadManager tm;

        const std::int32_t root_uid = tm.init_root_thread("root", 0x08804108u, 0x09FFFF00u, 0x08C42A50u);
        // Child priority 0x10 is higher priority than root 0x20
        const std::int32_t child_uid = tm.create_thread("high_prio_child", 0x0880421Cu, 0x10, 0x40000u, 0x80000000u, 0u, memory);

        psprecomp::AllegrexContext caller_ctx{};
        caller_ctx.set_gpr(28, 0x08C42A50u);
        caller_ctx.set_gpr(29, 0x09FFFF00u);
        caller_ctx.set_gpr(31, 0x088041F0u);
        caller_ctx.pc = 0x088041E8u;

        const std::int32_t ret = tm.start_thread(child_uid, 0u, 0u, memory, caller_ctx);
        TEST_CHECK_EQ(ret, 0);

        // Child preempts root: child is Running, root is Ready, current thread is child
        auto *child = tm.get_thread(child_uid);
        auto *root = tm.get_thread(root_uid);
        TEST_CHECK(child != nullptr && child->status == p3p3ds::hle::InternalThreadState::Running);
        TEST_CHECK(root != nullptr && root->status == p3p3ds::hle::InternalThreadState::Ready);
        TEST_CHECK_EQ(tm.current_thread_id(), child_uid);

        // CPU context switched to child's entry
        TEST_CHECK_EQ(caller_ctx.pc, 0x0880421Cu);
        TEST_CHECK_EQ(caller_ctx.gpr[28], 0x08C42A50u);
        TEST_CHECK_EQ(caller_ctx.gpr[31], p3p3ds::hle::ThreadManager::kThreadReturnSentinel);

        std::cout << "  [PASS] Test B: Higher priority child thread preempts current thread\n";
    }

    // 5. Test C: Repeated start on non-dormant thread returns 0x800201A4
    {
        psprecomp::GuestMemory memory(32u * 1024u * 1024u);
        p3p3ds::hle::ThreadManager tm;

        tm.init_root_thread("root", 0x08804108u, 0x09FFFF00u, 0x08C42A50u);
        const std::int32_t child_uid = tm.create_thread("user_main", 0x0880421Cu, 0x20, 0x40000u, 0x80000000u, 0u, memory);

        psprecomp::AllegrexContext ctx{};
        ctx.set_gpr(28, 0x08C42A50u);
        TEST_CHECK_EQ(tm.start_thread(child_uid, 0u, 0u, memory, ctx), 0);

        // Re-starting an already started thread must return SCE_KERNEL_ERROR_NOT_DORMANT (0x800201A4)
        const std::int32_t err = tm.start_thread(child_uid, 0u, 0u, memory, ctx);
        TEST_CHECK_EQ(err, static_cast<std::int32_t>(0x800201A4u));
        TEST_CHECK_EQ(err, p3p3ds::hle::SCE_KERNEL_ERROR_NOT_DORMANT);

        std::cout << "  [PASS] Test C: Starting non-dormant thread returns exact 0x800201A4\n";
    }

    // 6. Test D & E: Thread arguments copying and stack underflow protection
    {
        psprecomp::GuestMemory memory(32u * 1024u * 1024u);
        p3p3ds::hle::ThreadManager tm;

        tm.init_root_thread("root", 0x08804108u, 0x09FFFF00u, 0x08C42A50u);
        const std::int32_t child_uid = tm.create_thread("args_child", 0x0880421Cu, 0x20, 0x1000u, 0x80000000u, 0u, memory);

        // Store 13 bytes of test data
        constexpr std::uint32_t kSourceArgs = 0x08900000u;
        const char sample_args[] = "Hello Thread";
        for (std::size_t i = 0; i < sizeof(sample_args); ++i) {
            memory.store8(kSourceArgs + static_cast<std::uint32_t>(i), static_cast<std::uint8_t>(sample_args[i]));
        }

        psprecomp::AllegrexContext ctx{};
        ctx.set_gpr(28, 0x08C42A50u);
        TEST_CHECK_EQ(tm.start_thread(child_uid, sizeof(sample_args), kSourceArgs, memory, ctx), 0);

        auto *child = tm.get_thread(child_uid);
        TEST_CHECK(child != nullptr);
        if (child) {
            TEST_CHECK_EQ(child->context.gpr[4], sizeof(sample_args)); // $a0 = arg_size
            const std::uint32_t arg_sp = child->context.gpr[5];        // $a1 = stack arg pointer
            TEST_CHECK((arg_sp & 0xFu) == 0u);                          // 16-byte aligned
            TEST_CHECK(arg_sp < child->stack_top && arg_sp >= child->stack_address);
            // Verify argument data was copied to guest stack
            for (std::size_t i = 0; i < sizeof(sample_args); ++i) {
                TEST_CHECK_EQ(memory.load8(arg_sp + static_cast<std::uint32_t>(i)), static_cast<std::uint8_t>(sample_args[i]));
            }
        }

        // Test E: Overly large argument block must not underflow stack
        const std::int32_t child_small = tm.create_thread("small_stack", 0x0880421Cu, 0x20, 0x400u, 0x80000000u, 0u, memory);
        TEST_CHECK_EQ(tm.start_thread(child_small, 0x1000u, kSourceArgs, memory, ctx),
                      p3p3ds::hle::SCE_KERNEL_ERROR_NO_MEMORY);

        std::cout << "  [PASS] Test D & E: Thread arguments copied, 16-byte aligned, and bounded against stack underflow\n";
    }

    // 7. Test F: Thread return trampoline and scheduler continuation
    {
        psprecomp::Runtime runtime(32u * 1024u * 1024u);
        p3p3ds::KernelState kernel;

        p3p3ds::hle::register_all_hle_modules(runtime, kernel);
        const std::int32_t root_uid = kernel.threads().init_root_thread("root", 0x08804108u, 0x09FFFF00u, 0x08C42A50u);
        const std::int32_t user_uid = kernel.threads().create_thread("user_main", 0x0880421Cu, 0x20, 0x40000u, 0x80000000u, 0u, runtime.memory());

        psprecomp::AllegrexContext caller_ctx{};
        caller_ctx.set_gpr(28, 0x08C42A50u);
        kernel.threads().start_thread(user_uid, 0u, 0u, runtime.memory(), caller_ctx);

        // Root thread finishes its entry function and returns to kThreadReturnSentinel with $v0 = 0
        runtime.cpu().pc = p3p3ds::hle::ThreadManager::kThreadReturnSentinel;
        runtime.cpu().set_gpr(2, 0u); // exit status 0

        // Simulate trampoline dispatch
        const bool switched = kernel.threads().exit_current_thread(0, runtime.cpu(), runtime);
        TEST_CHECK(switched);

        // Root is now Stopped, user_main is Running, PC is at user_main entry (not 0)
        auto *root = kernel.threads().get_thread(root_uid);
        auto *user = kernel.threads().get_thread(user_uid);
        TEST_CHECK(root != nullptr && root->status == p3p3ds::hle::InternalThreadState::Stopped);
        TEST_CHECK(user != nullptr && user->status == p3p3ds::hle::InternalThreadState::Running);
        TEST_CHECK_EQ(kernel.threads().current_thread_id(), user_uid);
        TEST_CHECK_EQ(runtime.cpu().pc, 0x0880421Cu);

        std::cout << "  [PASS] Test F: Thread return trampoline, termination, and scheduler continuation\n";
    }

    if (g_failure_count > 0) {
        std::cerr << "[FAIL] " << g_failure_count << " test check(s) failed.\n";
        return 1;
    }

    std::cout << "All ThreadMan unit tests passed successfully.\n";
    return 0;
}
