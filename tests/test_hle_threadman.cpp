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

    // 1. Thread creation test
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
            TEST_CHECK(t1->status == p3p3ds::hle::ThreadStatus::Dormant);
            TEST_CHECK(t1->stack_address >= 0x09000000u && t1->stack_top <= 0x09FF0000u);
        }

        auto *t2 = tm.get_thread(uid2);
        TEST_CHECK(t2 != nullptr);
        if (t2 && t1) {
            TEST_CHECK_EQ(t2->uid, uid2);
            TEST_CHECK(t2->name == "thread_two");
            TEST_CHECK(t2->stack_address != t1->stack_address);
            TEST_CHECK(t2->stack_top <= t1->stack_address); // Stacks allocated downwards without overlap
        }

        // Error validation
        TEST_CHECK_EQ(tm.create_thread("bad_entry", 0u, 0x20, 0x1000u, 0u, 0u, memory),
                      p3p3ds::hle::SCE_KERNEL_ERROR_ILLEGAL_ENTRY);
        TEST_CHECK_EQ(tm.create_thread("bad_prio", 0x08804000u, 2, 0x1000u, 0u, 0u, memory),
                      p3p3ds::hle::SCE_KERNEL_ERROR_ILLEGAL_PRIORITY);
        TEST_CHECK_EQ(tm.create_thread("bad_stack", 0x08804000u, 0x20, 128u, 0u, 0u, memory),
                      p3p3ds::hle::SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE);

        std::cout << "  [PASS] Thread creation, UID allocation, and validation\n";
    }

    // 2. Thread start and context switch test
    {
        psprecomp::GuestMemory memory(32u * 1024u * 1024u);
        p3p3ds::hle::ThreadManager tm;

        const std::int32_t root_uid = tm.init_root_thread("root", 0x08804108u, 0x09FFFF00u, 0x08C42A50u);
        TEST_CHECK_EQ(root_uid, 1);
        TEST_CHECK_EQ(tm.current_thread_id(), root_uid);

        const std::int32_t child_uid = tm.create_thread("child_main", 0x0880421Cu, 0x20, 0x40000u, 0x80000000u, 0u, memory);
        TEST_CHECK(child_uid > 1);

        // Unknown thread id start failure
        psprecomp::AllegrexContext caller_ctx{};
        caller_ctx.set_gpr(28, 0x08C42A50u);
        caller_ctx.set_gpr(29, 0x09FFFF00u);
        caller_ctx.set_gpr(31, 0x088041F0u);
        caller_ctx.pc = 0x088041E8u;

        TEST_CHECK_EQ(tm.start_thread(999, 0u, 0u, memory, caller_ctx),
                      p3p3ds::hle::SCE_KERNEL_ERROR_UNKNOWN_THID);

        // Valid thread start
        const std::int32_t start_res = tm.start_thread(child_uid, 0u, 0u, memory, caller_ctx);
        TEST_CHECK_EQ(start_res, 0);

        // Verify active thread switched
        TEST_CHECK_EQ(tm.current_thread_id(), child_uid);
        TEST_CHECK(tm.current_thread()->status == p3p3ds::hle::ThreadStatus::Running);

        // Verify caller context was saved in root thread
        auto *root = tm.get_thread(root_uid);
        TEST_CHECK(root != nullptr);
        if (root) {
            TEST_CHECK(root->status == p3p3ds::hle::ThreadStatus::Ready);
            TEST_CHECK_EQ(root->context.pc, 0x088041F0u);
            TEST_CHECK_EQ(root->context.gpr[2], 0u); // $v0 = 0
        }

        // Verify child thread context was loaded into active CPU context
        TEST_CHECK_EQ(caller_ctx.pc, 0x0880421Cu);
        TEST_CHECK_EQ(caller_ctx.gpr[28], 0x08C42A50u);
        TEST_CHECK_EQ(caller_ctx.gpr[4], 0u);
        TEST_CHECK_EQ(caller_ctx.gpr[5], 0u);
        TEST_CHECK(caller_ctx.gpr[29] >= 0x09000000u && caller_ctx.gpr[29] < 0x09FF0000u);

        // Starting already running thread fails
        TEST_CHECK_EQ(tm.start_thread(child_uid, 0u, 0u, memory, caller_ctx),
                      p3p3ds::hle::SCE_KERNEL_ERROR_NOT_DORMANT);

        std::cout << "  [PASS] Thread start and cooperative context switch\n";
    }

    // 3. Runtime HLE import dispatch test for sceKernelCreateThread and sceKernelStartThread
    {
        psprecomp::Runtime runtime(32u * 1024u * 1024u);
        p3p3ds::KernelState kernel;

        p3p3ds::hle::register_all_hle_modules(runtime, kernel);
        kernel.threads().init_root_thread("root", 0x08804108u, 0x09FFFF00u, 0x08C42A50u);
        runtime.cpu().set_gpr(28, 0x08C42A50u); // $gp

        // Store thread name string "user_main" in guest memory
        constexpr std::uint32_t kNameAddr = 0x08B809C8u;
        const char name_str[] = "user_main";
        for (std::size_t i = 0; i < sizeof(name_str); ++i) {
            runtime.memory().store8(kNameAddr + static_cast<std::uint32_t>(i), static_cast<std::uint8_t>(name_str[i]));
        }

        // Dispatch sceKernelCreateThread
        runtime.cpu().set_gpr(4, kNameAddr);       // $a0 = name pointer
        runtime.cpu().set_gpr(5, 0x0880421Cu);     // $a1 = entry PC
        runtime.cpu().set_gpr(6, 0x00000020u);     // $a2 = priority
        runtime.cpu().set_gpr(7, 0x00040000u);     // $a3 = stack size
        runtime.cpu().set_gpr(8, 0x80000000u);     // $t0 = attr
        runtime.cpu().set_gpr(9, 0x00000000u);     // $t1 = option
        runtime.cpu().set_gpr(2, 0xDEADBEEFu);     // $v0 = dirty

        runtime.invoke_import("ThreadManForUser", 0x446D8DE6u, runtime.cpu());
        TEST_CHECK(!runtime.stopped());

        const std::int32_t created_uid = static_cast<std::int32_t>(runtime.cpu().gpr[2]);
        TEST_CHECK(created_uid > 1);

        auto *t = kernel.threads().get_thread(created_uid);
        TEST_CHECK(t != nullptr);
        if (t) {
            TEST_CHECK(t->name == "user_main");
            TEST_CHECK_EQ(t->entry_pc, 0x0880421Cu);
            TEST_CHECK_EQ(t->initial_priority, 0x20);
            TEST_CHECK_EQ(t->stack_size, 0x40000u);
            TEST_CHECK(t->status == p3p3ds::hle::ThreadStatus::Dormant);
        }

        // Dispatch sceKernelStartThread
        runtime.cpu().set_gpr(4, static_cast<std::uint32_t>(created_uid)); // $a0 = thread id
        runtime.cpu().set_gpr(5, 0u);                                      // $a1 = arglen
        runtime.cpu().set_gpr(6, 0u);                                      // $a2 = argptr
        runtime.cpu().set_gpr(31, 0x088041F0u);                            // $ra = caller return PC
        runtime.cpu().pc = 0x088041E8u;

        runtime.invoke_import("ThreadManForUser", 0xF475845Du, runtime.cpu());
        TEST_CHECK(!runtime.stopped());

        // Verify context switched to user_main
        TEST_CHECK_EQ(kernel.threads().current_thread_id(), created_uid);
        TEST_CHECK_EQ(runtime.cpu().pc, 0x0880421Cu);
        TEST_CHECK_EQ(runtime.cpu().gpr[28], 0x08C42A50u);

        std::cout << "  [PASS] Runtime HLE dispatch of sceKernelCreateThread & sceKernelStartThread\n";
    }

    if (g_failure_count > 0) {
        std::cerr << "[FAIL] " << g_failure_count << " test check(s) failed.\n";
        return 1;
    }

    std::cout << "All ThreadMan unit tests passed successfully.\n";
    return 0;
}
