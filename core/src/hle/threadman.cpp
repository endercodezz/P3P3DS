#include "p3p3ds/hle/threadman.hpp"
#include "p3p3ds/kernel_state.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>

namespace p3p3ds::hle {

namespace {

std::string read_safe_string(const psprecomp::GuestMemory &memory, std::uint32_t address, std::size_t max_len = 64u) {
    std::string result;
    for (std::size_t i = 0; i < max_len && memory.contains(address + static_cast<std::uint32_t>(i), 1u); ++i) {
        char c = static_cast<char>(memory.load8(address + static_cast<std::uint32_t>(i)));
        if (c == '\0') break;
        result += c;
    }
    return result;
}

void copy_guest_bytes(psprecomp::GuestMemory &memory, std::uint32_t dst, std::uint32_t src, std::size_t length) {
    for (std::size_t i = 0; i < length; ++i) {
        memory.store8(dst + static_cast<std::uint32_t>(i), memory.load8(src + static_cast<std::uint32_t>(i)));
    }
}

} // namespace

ThreadManager::ThreadManager() {
    reset();
}

void ThreadManager::reset() {
    next_uid_ = 1;
    current_thread_id_ = 0;
    next_stack_top_ = 0x09FF0000u;
    threads_.clear();
}

std::int32_t ThreadManager::init_root_thread(std::string_view name, std::uint32_t entry_pc, std::uint32_t sp, std::uint32_t gp) {
    const std::int32_t uid = next_uid_++;
    ThreadControlBlock root{};
    root.uid = uid;
    root.name = std::string(name);
    root.entry_pc = entry_pc;
    root.initial_priority = 0x20;
    root.current_priority = 0x20;
    root.stack_size = 0x10000u;
    root.stack_address = 0x09FF0000u;
    root.stack_top = 0x0A000000u;
    root.attributes = 0x80000000u;
    root.status = ThreadStatus::Running;
    root.context.pc = entry_pc;
    root.context.set_gpr(28, gp);
    root.context.set_gpr(29, sp);
    root.context.set_gpr(30, sp);

    threads_[uid] = root;
    current_thread_id_ = uid;
    psprecomp::set_runtime_thread_identity(uid, root.name);
    return uid;
}

std::int32_t ThreadManager::create_thread(std::string_view name,
                                          std::uint32_t entry_pc,
                                          std::int32_t init_priority,
                                          std::uint32_t stack_size,
                                          std::uint32_t attributes,
                                          std::uint32_t option_address,
                                          psprecomp::GuestMemory &memory) {
    if (entry_pc == 0u) {
        return SCE_KERNEL_ERROR_ILLEGAL_ENTRY;
    }
    if (init_priority < 8 || init_priority > 119) {
        return SCE_KERNEL_ERROR_ILLEGAL_PRIORITY;
    }
    if (stack_size < 512u) {
        return SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE;
    }

    // Align stack size to 256 bytes
    const std::uint32_t aligned_stack_size = (stack_size + 0xFFu) & ~0xFFu;

    // Allocate stack arena downwards from next_stack_top_
    const std::uint32_t stack_top = next_stack_top_;
    if (stack_top < aligned_stack_size || stack_top - aligned_stack_size < 0x09000000u) {
        return SCE_KERNEL_ERROR_NO_MEMORY;
    }
    const std::uint32_t stack_base = stack_top - aligned_stack_size;
    next_stack_top_ = stack_base;

    // Zero out newly allocated stack
    memory.zero(stack_base, aligned_stack_size);

    const std::int32_t uid = next_uid_++;
    ThreadControlBlock tcb{};
    tcb.uid = uid;
    tcb.name = std::string(name);
    tcb.entry_pc = entry_pc;
    tcb.initial_priority = init_priority;
    tcb.current_priority = init_priority;
    tcb.stack_size = aligned_stack_size;
    tcb.stack_address = stack_base;
    tcb.stack_top = stack_top;
    tcb.attributes = attributes;
    tcb.option_address = option_address;
    tcb.status = ThreadStatus::Dormant;

    // Place thread UID at base of stack (standard PSP behavior checked by pspautotests)
    memory.store32(stack_base, static_cast<std::uint32_t>(uid));

    threads_[uid] = std::move(tcb);
    return uid;
}

std::int32_t ThreadManager::start_thread(std::int32_t thid,
                                         std::uint32_t arg_size,
                                         std::uint32_t arg_ptr,
                                         psprecomp::GuestMemory &memory,
                                         psprecomp::AllegrexContext &caller_ctx) {
    auto *target = get_thread(thid);
    if (!target) {
        return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }
    if (target->status != ThreadStatus::Dormant) {
        return SCE_KERNEL_ERROR_NOT_DORMANT;
    }

    // Initialize target thread context
    target->context = psprecomp::AllegrexContext{};
    target->context.pc = target->entry_pc;
    target->context.set_gpr(28, caller_ctx.gpr[28]); // Inherit $gp from caller module

    // Calculate initial SP with PSP top-of-stack headroom (256 bytes k0 section + optional args + 64 byte margin)
    std::uint32_t sp = target->stack_top - 256u;
    if (arg_ptr != 0u && arg_size > 0u) {
        sp -= (arg_size + 15u) & ~15u;
        if (memory.contains(arg_ptr, arg_size)) {
            copy_guest_bytes(memory, sp, arg_ptr, arg_size);
        }
        target->context.set_gpr(4, arg_size); // $a0 = arg_size
        target->context.set_gpr(5, sp);       // $a1 = arg_ptr on stack
    } else {
        target->context.set_gpr(4, 0u);
        target->context.set_gpr(5, 0u);
    }
    sp -= 64u;
    target->context.set_gpr(29, sp); // $sp
    target->context.set_gpr(30, sp); // $fp
    target->context.set_gpr(31, 0u); // $ra

    // Perform context switch from caller to newly started thread
    auto *current = current_thread();
    if (current != nullptr) {
        // Save caller's resume state: return value 0 in $v0, return address in PC
        caller_ctx.set_gpr(2, 0u);
        caller_ctx.pc = caller_ctx.gpr[31];
        current->context = caller_ctx;
        current->status = ThreadStatus::Ready;
    }

    // Switch active thread
    current_thread_id_ = thid;
    target->status = ThreadStatus::Running;
    caller_ctx = target->context;

    // Notify runtime of thread identity and context switch generation change
    psprecomp::set_runtime_thread_identity(target->uid, target->name);
    return 0;
}

ThreadControlBlock *ThreadManager::get_thread(std::int32_t uid) noexcept {
    const auto it = threads_.find(uid);
    return it != threads_.end() ? &it->second : nullptr;
}

const ThreadControlBlock *ThreadManager::get_thread(std::int32_t uid) const noexcept {
    const auto it = threads_.find(uid);
    return it != threads_.end() ? &it->second : nullptr;
}

ThreadControlBlock *ThreadManager::current_thread() noexcept {
    return get_thread(current_thread_id_);
}

const ThreadControlBlock *ThreadManager::current_thread() const noexcept {
    return get_thread(current_thread_id_);
}

bool ThreadManager::switch_to(std::int32_t target_thid, psprecomp::AllegrexContext &ctx) {
    auto *target = get_thread(target_thid);
    if (!target) return false;

    auto *current = current_thread();
    if (current != nullptr) {
        current->context = ctx;
        if (current->status == ThreadStatus::Running) {
            current->status = ThreadStatus::Ready;
        }
    }

    current_thread_id_ = target_thid;
    target->status = ThreadStatus::Running;
    ctx = target->context;
    psprecomp::set_runtime_thread_identity(target->uid, target->name);
    return true;
}

void register_threadman_for_user(psprecomp::Runtime &runtime, KernelState &kernel) {
    // ThreadManForUser::0x446D8DE6 (sceKernelCreateThread)
    runtime.register_hle("ThreadManForUser", 0x446D8DE6u,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t name_ptr = ctx.gpr[4];
            std::string name;
            if (name_ptr != 0u && rt.memory().contains(name_ptr, 1u)) {
                name = read_safe_string(rt.memory(), name_ptr, 64u);
            }
            const std::uint32_t entry = ctx.gpr[5];
            const std::int32_t prio = static_cast<std::int32_t>(ctx.gpr[6]);
            const std::uint32_t stack_size = ctx.gpr[7];
            const std::uint32_t attr = ctx.gpr[8];      // 5th argument in $t0
            const std::uint32_t option_ptr = ctx.gpr[9]; // 6th argument in $t1

            const std::int32_t result = kernel.threads().create_thread(
                name, entry, prio, stack_size, attr, option_ptr, rt.memory());
            ctx.set_gpr(2, static_cast<std::uint32_t>(result));
        });

    // ThreadManForUser::0xF475845D (sceKernelStartThread)
    runtime.register_hle("ThreadManForUser", 0xF475845Du,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const std::int32_t thid = static_cast<std::int32_t>(ctx.gpr[4]);
            const std::uint32_t arg_size = ctx.gpr[5];
            const std::uint32_t arg_ptr = ctx.gpr[6];

            const std::int32_t result = kernel.threads().start_thread(
                thid, arg_size, arg_ptr, rt.memory(), ctx);
            if (result < 0) {
                ctx.set_gpr(2, static_cast<std::uint32_t>(result));
            }
        });
}

} // namespace p3p3ds::hle
