// ThreadMan core: threads, scheduler, virtual clock, waits and callbacks.
//
// Sources, in AGENTS.md section 4 order: uOFW threadman is unreversed
// (references/uofw/src/kd/threadman/threadman.c has empty bodies), so hardware
// expectations from references/pspautotests/tests/threads/** and the PSPSDK
// contracts in psp/pspsdk/src/user/pspthreadman.h define the behaviour here.
// PPSSPP was consulted only as a behavioural cross-check; no code was copied.
#include "p3p3ds/hle/threadman.hpp"
#include "p3p3ds/kernel_state.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>

namespace p3p3ds::hle {

namespace {

KernelState *g_active_kernel = nullptr;

void copy_guest_bytes(psprecomp::GuestMemory &memory, std::uint32_t dst, std::uint32_t src, std::size_t length) {
    for (std::size_t i = 0; i < length; ++i) {
        memory.store8(dst + static_cast<std::uint32_t>(i), memory.load8(src + static_cast<std::uint32_t>(i)));
    }
}

void thread_return_trampoline(psprecomp::Runtime &runtime, psprecomp::AllegrexContext &ctx) {
    if (g_active_kernel != nullptr) {
        const std::int32_t exit_status = static_cast<std::int32_t>(ctx.gpr[2]);
        g_active_kernel->threads().exit_current_thread(exit_status, ctx, runtime);
    } else {
        runtime.stop("Thread return without active kernel state");
    }
}

void callback_return_trampoline(psprecomp::Runtime &runtime, psprecomp::AllegrexContext &ctx) {
    if (g_active_kernel != nullptr) g_active_kernel->threads().callback_returned(runtime, ctx);
    else runtime.stop("Callback return without active kernel state");
}

std::uint32_t u(std::int32_t value) { return static_cast<std::uint32_t>(value); }

} // namespace

ThreadManager::ThreadManager() {
    reset();
}

void ThreadManager::reset() {
    next_uid_ = 1;
    current_thread_id_ = 0;
    next_stack_top_ = 0x09FF0000u;
    now_us_ = 0;
    idle_advances_ = 0;
    threads_.clear();
    callbacks_.clear();
    semas_.clear();
    event_flags_.clear();
    mutexes_.clear();
    lw_mutexes_.clear();
    ready_queue_.clear();
    active_entry_.reset();
}

// ---------------------------------------------------------------------------
// Threads
// ---------------------------------------------------------------------------

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
    root.gp = gp;
    root.status = InternalThreadState::Running;
    root.started = true;
    root.context.pc = entry_pc;
    root.context.set_gpr(26, root.stack_top - 256u); // $k0 = 256-byte thread context block
    root.context.set_gpr(28, gp);
    root.context.set_gpr(29, sp);
    root.context.set_gpr(30, sp);
    root.context.set_gpr(31, kThreadReturnSentinel);

    threads_[uid] = root;
    current_thread_id_ = uid;
    psprecomp::set_runtime_thread_identity(uid, root.name);
    return uid;
}

std::int32_t ThreadManager::create_thread(std::string_view name, std::uint32_t entry_pc, std::int32_t init_priority,
                                          std::uint32_t stack_size, std::uint32_t attributes,
                                          std::uint32_t option_address, psprecomp::GuestMemory &memory) {
    memory_ = &memory;
    if (entry_pc == 0u) return SCE_KERNEL_ERROR_ILLEGAL_ENTRY;
    if (init_priority < 8 || init_priority > 119) return SCE_KERNEL_ERROR_ILLEGAL_PRIORITY;
    if (stack_size < 512u) return SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE;

    const std::uint32_t aligned_stack_size = (stack_size + 0xFFu) & ~0xFFu;
    const std::uint32_t stack_top = next_stack_top_;
    if (stack_top < aligned_stack_size || stack_top - aligned_stack_size < 0x09000000u) {
        return SCE_KERNEL_ERROR_NO_MEMORY;
    }
    const std::uint32_t stack_base = stack_top - aligned_stack_size;
    next_stack_top_ = stack_base;

    // Fill stack with 0xFF unless PSP_THREAD_ATTR_NO_FILLSTACK is set
    if ((attributes & kThreadAttrNoFillStack) == 0u) {
        for (std::uint32_t offset = 0u; offset < aligned_stack_size; offset += 4u) {
            memory.store32(stack_base + offset, 0xFFFFFFFFu);
        }
    }

    // Zero out top 256-byte k0 section
    memory.zero(stack_top - 256u, 256u);

    const std::int32_t uid = next_uid_++;

    // Write thread UID at base of stack and in k0 section
    memory.store32(stack_base, static_cast<std::uint32_t>(uid));
    memory.store32(stack_top - 256u + 0xC0u, static_cast<std::uint32_t>(uid));
    memory.store32(stack_top - 256u + 0xC8u, stack_base);
    memory.store32(stack_top - 256u + 0xF8u, 0xFFFFFFFFu);
    memory.store32(stack_top - 256u + 0xFCu, 0xFFFFFFFFu);

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
    tcb.status = InternalThreadState::Dormant;
    tcb.context.set_gpr(31, kThreadReturnSentinel);

    threads_[uid] = std::move(tcb);
    std::cout << "[THREAD CREATE] uid=" << uid << " name=" << name
              << " entry=0x" << std::hex << entry_pc << std::dec
              << " priority=" << init_priority << " stack=0x" << std::hex << stack_base
              << "-0x" << stack_top << std::dec << "\n";
    return uid;
}

std::int32_t ThreadManager::start_thread(std::int32_t thid, std::uint32_t arg_size, std::uint32_t arg_ptr,
                                         psprecomp::GuestMemory &memory, psprecomp::AllegrexContext &caller_ctx) {
    memory_ = &memory;
    if (thid <= 0) return SCE_KERNEL_ERROR_ILLEGAL_THID;
    auto *target = get_thread(thid);
    if (!target) return SCE_KERNEL_ERROR_UNKNOWN_THID;
    if (target->status != InternalThreadState::Dormant) return SCE_KERNEL_ERROR_NOT_DORMANT;
    if (static_cast<std::int32_t>(arg_size) < 0) return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
    if (arg_size > 0u && (arg_ptr == 0u || !memory.contains(arg_ptr, arg_size))) return SCE_KERNEL_ERROR_ILLEGAL_ADDR;

    const std::uint32_t aligned_arg_size = (arg_size + 15u) & ~15u;
    const std::uint32_t required_bytes = 256u + aligned_arg_size + 64u;
    if (required_bytes >= target->stack_size || target->stack_top - required_bytes < target->stack_address) {
        return SCE_KERNEL_ERROR_NO_MEMORY;
    }

    target->context = psprecomp::AllegrexContext{};
    target->context.pc = target->entry_pc;
    target->gp = caller_ctx.gpr[28];
    target->context.set_gpr(26, target->stack_top - 256u); // $k0 = 256-byte thread context block
    target->context.set_gpr(28, caller_ctx.gpr[28]);        // Inherit $gp
    target->context.set_gpr(31, kThreadReturnSentinel);     // Return to trampoline

    std::uint32_t sp = target->stack_top - 256u;
    if (arg_ptr != 0u && arg_size > 0u) {
        sp -= aligned_arg_size;
        copy_guest_bytes(memory, sp, arg_ptr, arg_size);
        target->context.set_gpr(4, arg_size); // $a0 = arg_size
        target->context.set_gpr(5, sp);       // $a1 = arg_ptr on stack
    } else {
        target->context.set_gpr(4, 0u);
        target->context.set_gpr(5, 0u);
    }
    sp -= 64u;
    target->context.set_gpr(29, sp); // $sp
    target->context.set_gpr(30, sp); // $fp

    target->status = InternalThreadState::Ready;
    target->started = true;
    target->entry_dispatch_pending = true;
    target->exit_status = 0;
    target->wakeup_count = 0;
    target->wait = {};
    enqueue_ready(thid);
    std::cout << "[THREAD START] uid=" << thid << " name=" << target->name << " args=" << arg_size << "\n";

    auto *current = current_thread();
    // Smaller number = higher priority. A strictly higher-priority new thread
    // preempts the caller immediately; the caller later resumes with 0.
    if (current != nullptr && target->current_priority < current->current_priority) {
        caller_ctx.set_gpr(2, 0u);
        caller_ctx.pc = caller_ctx.gpr[31];
        current->context = caller_ctx;
        current->status = InternalThreadState::Ready;
        enqueue_ready(current->uid, true);
        remove_ready(thid);
        const auto from = current->uid;
        current_thread_id_ = thid;
        target->status = InternalThreadState::Running;
        caller_ctx = target->context;
        psprecomp::set_runtime_thread_identity(target->uid, target->name);
        select_initial_entry(*target, from, caller_ctx);
        return 0;
    }
    caller_ctx.set_gpr(2, 0u);
    return 0;
}

bool ThreadManager::schedule(psprecomp::AllegrexContext &ctx) {
    auto *target = best_ready();
    if (!target) return false;
    load_thread(*target, ctx, current_thread_id_);
    return true;
}

void ThreadManager::select_initial_entry(ThreadControlBlock &target, std::int32_t from_uid,
                                         const psprecomp::AllegrexContext &ctx) noexcept {
    if (target.started && target.entry_dispatch_pending && !target.entry_executed &&
        target.status == InternalThreadState::Running && ctx.pc == target.entry_pc &&
        target.context.pc == target.entry_pc) {
        active_entry_ = ThreadEntryProvenance{target.uid, from_uid, target.entry_pc};
        target.entry_dispatch_pending = false;
    }
}

std::optional<ThreadEntryProvenance> ThreadManager::verified_thread_entry(
    const psprecomp::AllegrexContext &ctx, std::int32_t runtime_uid) const noexcept {
    if (!active_entry_ || current_thread_id_ != runtime_uid || active_entry_->uid != runtime_uid ||
        ctx.pc != active_entry_->entry_pc) return std::nullopt;
    const auto *target = get_thread(runtime_uid);
    if (!target || !target->started || target->entry_executed ||
        target->status != InternalThreadState::Running || target->entry_pc != ctx.pc ||
        target->context.pc != ctx.pc) return std::nullopt;
    return active_entry_;
}

void ThreadManager::note_guest_execution(std::int32_t uid, std::uint32_t pc) noexcept {
    if (!active_entry_) return;
    if (uid == active_entry_->uid && pc == active_entry_->entry_pc)
        if (auto *target = get_thread(uid)) target->entry_executed = true;
    active_entry_.reset();
}

bool ThreadManager::exit_current_thread(std::int32_t exit_status, psprecomp::AllegrexContext &ctx, psprecomp::Runtime &runtime) {
    auto *cur = current_thread();
    if (cur != nullptr) {
        std::cout << "[THREAD EXIT] uid=" << cur->uid << " name=" << cur->name
                  << " status=" << exit_status << " ready=" << ready_queue_.size() << "\n";
        cur->status = InternalThreadState::Stopped;
        cur->exit_status = exit_status;
        cur->callback.reset();
        // Threads waiting for this one's end resume with its exit status.
        std::vector<std::int32_t> enders;
        for (auto &[uid, t] : threads_)
            if (t.status == InternalThreadState::Waiting && t.wait.type == WaitType::ThreadEnd &&
                t.wait.object == cur->uid) enders.push_back(uid);
        for (auto uid : enders) wake(runtime, uid, exit_status);
    }
    const bool any_ready = best_ready() != nullptr;
    const bool any_waiting = std::any_of(threads_.begin(), threads_.end(), [](const auto &entry) {
        return entry.second.status == InternalThreadState::Waiting;
    });
    if (!any_ready && !any_waiting) {
        runtime.stop("All threads completed");
        return false;
    }
    dispatch_next(runtime, ctx);
    if (runtime.stopped()) return false;
    runtime.event("thread_switch", {{"uid", u(current_thread_id_)}, {"entry", ctx.pc}});
    if (auto proof = verified_thread_entry(ctx, psprecomp::runtime_thread_uid()))
        runtime.event("thread_entry_transfer", {{"uid", u(proof->uid)}, {"entry", proof->entry_pc},
            {"from_uid", u(proof->from_uid)}, {"started", 1u}}, get_thread(proof->uid)->name);
    return true;
}

ThreadControlBlock *ThreadManager::get_thread(std::int32_t uid) noexcept {
    const auto it = threads_.find(uid);
    return it != threads_.end() ? &it->second : nullptr;
}

const ThreadControlBlock *ThreadManager::get_thread(std::int32_t uid) const noexcept {
    const auto it = threads_.find(uid);
    return it != threads_.end() ? &it->second : nullptr;
}

ThreadControlBlock *ThreadManager::current_thread() noexcept { return get_thread(current_thread_id_); }
const ThreadControlBlock *ThreadManager::current_thread() const noexcept { return get_thread(current_thread_id_); }

bool ThreadManager::switch_to(std::int32_t target_thid, psprecomp::AllegrexContext &ctx) {
    auto *target = get_thread(target_thid);
    if (!target) return false;
    auto *current = current_thread();
    if (current != nullptr) {
        current->context = ctx;
        if (current->status == InternalThreadState::Running) {
            current->status = InternalThreadState::Ready;
            enqueue_ready(current->uid);
        }
    }
    remove_ready(target_thid);
    active_entry_.reset();
    target->entry_dispatch_pending = false; // Arbitrary switch is not a proven initial start.
    current_thread_id_ = target_thid;
    target->status = InternalThreadState::Running;
    ctx = target->context;
    psprecomp::set_runtime_thread_identity(target->uid, target->name);
    return true;
}

std::int32_t ThreadManager::change_current_thread_attr(std::uint32_t clear_attr, std::uint32_t set_attr) noexcept {
    auto *current = current_thread();
    if (current == nullptr) return SCE_KERNEL_ERROR_ILLEGAL_THID;
    current->attributes = (current->attributes & ~clear_attr) | set_attr;
    return 0;
}

// ---------------------------------------------------------------------------
// Scheduler
// ---------------------------------------------------------------------------

std::uint32_t ThreadManager::resume_pc(const psprecomp::AllegrexContext &ctx) const noexcept {
    return import_stubs_.contains(ctx.pc) ? ctx.gpr[31] : ctx.pc;
}

bool ThreadManager::runnable(const ThreadControlBlock &t) const noexcept {
    return t.status == InternalThreadState::Ready && !t.suspended;
}

void ThreadManager::enqueue_ready(std::int32_t uid, bool front) {
    remove_ready(uid);
    if (front) ready_queue_.insert(ready_queue_.begin(), uid);
    else ready_queue_.push_back(uid);
}

void ThreadManager::remove_ready(std::int32_t uid) {
    ready_queue_.erase(std::remove(ready_queue_.begin(), ready_queue_.end(), uid), ready_queue_.end());
}

ThreadControlBlock *ThreadManager::best_ready() noexcept {
    ThreadControlBlock *best = nullptr;
    for (const auto uid : ready_queue_) {
        auto *t = get_thread(uid);
        if (t == nullptr || !runnable(*t)) continue;
        if (best == nullptr || t->current_priority < best->current_priority) best = t;
    }
    return best;
}

void ThreadManager::load_thread(ThreadControlBlock &t, psprecomp::AllegrexContext &ctx, std::int32_t from_uid) {
    // Always advance the runtime's context generation, even when the same
    // thread resumes: the generated import wrapper must not rewrite ctx.pc to
    // its $ra for a thread that was suspended in between (retry waits resume
    // at the stub itself).
    if (t.uid == psprecomp::runtime_thread_uid()) psprecomp::set_runtime_thread_identity(-1, "");
    remove_ready(t.uid);
    active_entry_.reset();
    current_thread_id_ = t.uid;
    t.status = InternalThreadState::Running;
    if (t.callback_pending_run) {
        // Woken from a CB wait to run notified callbacks. If the wait itself
        // completed meanwhile (wait.type cleared), resume afterwards instead.
        t.callback_pending_run = false;
        const auto resume = t.context;
        psprecomp::set_runtime_thread_identity(t.uid, t.name);
        ctx = resume;
        if (!start_callback(t, ctx, resume, t.wait.type != WaitType::None)) ctx = resume;
        return;
    }
    ctx = t.context;
    psprecomp::set_runtime_thread_identity(t.uid, t.name);
    select_initial_entry(t, from_uid, ctx);
}

void ThreadManager::dispatch_next(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
    const auto from = current_thread_id_;
    for (;;) {
        if (auto *next = best_ready()) {
            load_thread(*next, ctx, from);
            return;
        }
        std::uint64_t deadline = kNoDeadline;
        for (const auto &[uid, t] : threads_)
            if (t.status == InternalThreadState::Waiting && !t.suspended) deadline = std::min(deadline, t.wait.deadline);
        if (deadline == kNoDeadline) {
            std::string detail = "Deadlock: all threads wait without a deadline:";
            for (const auto &[uid, t] : threads_)
                if (t.status == InternalThreadState::Waiting)
                    detail += " " + t.name + "(uid=" + std::to_string(uid) + ",wait=" +
                              std::to_string(static_cast<std::uint32_t>(t.wait.type)) + ",obj=" +
                              std::to_string(t.wait.object) + ")";
            rt.stop(detail);
            return;
        }
        ++idle_advances_;
        if (deadline > now_us_) now_us_ = deadline;
        expire_deadlines(rt);
    }
}

void ThreadManager::expire_deadlines(psprecomp::Runtime &rt) {
    std::vector<std::int32_t> due;
    for (const auto &[uid, t] : threads_)
        if (t.status == InternalThreadState::Waiting && t.wait.deadline <= now_us_) due.push_back(uid);
    for (const auto uid : due) {
        auto &t = threads_.at(uid);
        const bool timed_out = t.wait.type != WaitType::Delay && t.wait.type != WaitType::Vblank &&
                               t.wait.type != WaitType::Audio && t.wait.type != WaitType::Io;
        // Synchronous IO already stored its result ($v0/$v1) before waiting.
        finish_wait(rt, t, timed_out ? SCE_KERNEL_ERROR_WAIT_TIMEOUT : 0, t.wait.type != WaitType::Io);
    }
}

void ThreadManager::insert_waiter(std::vector<std::int32_t> &waiters, std::int32_t uid, bool priority_order) {
    if (!priority_order) { waiters.push_back(uid); return; }
    const auto *t = get_thread(uid);
    auto it = std::find_if(waiters.begin(), waiters.end(), [&](std::int32_t other) {
        const auto *o = get_thread(other);
        return o != nullptr && t != nullptr && t->current_priority < o->current_priority;
    });
    waiters.insert(it, uid);
}

void ThreadManager::remove_from_object(ThreadControlBlock &t) {
    auto erase = [&](std::vector<std::int32_t> &w) { w.erase(std::remove(w.begin(), w.end(), t.uid), w.end()); };
    switch (t.wait.type) {
    case WaitType::Sema: if (auto it = semas_.find(t.wait.object); it != semas_.end()) erase(it->second.waiters); break;
    case WaitType::EventFlag: if (auto it = event_flags_.find(t.wait.object); it != event_flags_.end()) erase(it->second.waiters); break;
    case WaitType::Mutex: if (auto it = mutexes_.find(t.wait.object); it != mutexes_.end()) erase(it->second.waiters); break;
    case WaitType::LwMutex:
        if (auto it = lw_mutexes_.find(t.wait.object); it != lw_mutexes_.end()) {
            erase(it->second.waiters);
            if (memory_ != nullptr) memory_->store32(it->second.workarea + 12u, u(static_cast<std::int32_t>(it->second.waiters.size())));
        }
        break;
    default: break;
    }
}

void ThreadManager::finish_wait(psprecomp::Runtime &rt, ThreadControlBlock &t, std::int32_t result, bool set_result) {
    remove_from_object(t);
    if (t.wait.timeout_ptr != 0u && memory_ != nullptr && memory_->contains(t.wait.timeout_ptr, 4u)) {
        const std::uint64_t left = t.wait.deadline == kNoDeadline || t.wait.deadline <= now_us_ ? 0u : t.wait.deadline - now_us_;
        memory_->store32(t.wait.timeout_ptr, static_cast<std::uint32_t>(left));
    }
    if (t.callback) {
        // Woken while running callbacks: deliver the result after they finish.
        t.callback->woken = result;
        t.callback->rewait = false;
        t.wait = {};
        return;
    }
    if (set_result) t.context.set_gpr(2, u(result));
    t.wait = {};
    if (t.status == InternalThreadState::Waiting) {
        t.status = InternalThreadState::Ready;
        enqueue_ready(t.uid);
    }
    rt.event("thread_wake", {{"uid", u(t.uid)}, {"result", u(result)}, {"time", now_us_}});
}

void ThreadManager::wake(psprecomp::Runtime &rt, std::int32_t uid, std::int32_t result) {
    auto *t = get_thread(uid);
    if (t == nullptr) return;
    if (t->status != InternalThreadState::Waiting && !t->callback && !t->callback_pending_run) return;
    finish_wait(rt, *t, result);
}

std::size_t ThreadManager::wake_host_waiters(psprecomp::Runtime &rt, WaitType type, std::int32_t object, std::int32_t result) {
    std::vector<std::int32_t> matches;
    for (const auto &[uid, t] : threads_)
        if (t.status == InternalThreadState::Waiting && t.wait.type == type && t.wait.object == object) matches.push_back(uid);
    for (auto uid : matches) wake(rt, uid, result);
    return matches.size();
}

void ThreadManager::block_current(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, WaitInfo wait) {
    runtime_ = &rt;
    auto *cur = current_thread();
    if (cur == nullptr) { rt.stop("Blocking wait without a current thread"); return; }
    if (cur->callback) {
        rt.stop("Unsupported: blocking wait inside a PSP callback (thread " + cur->name + ")");
        return;
    }
    memory_ = &rt.memory();
    auto resume = ctx;
    resume.pc = wait.retry ? ctx.pc : ctx.gpr[31];
    cur->context = resume;
    cur->wait = wait;
    cur->status = InternalThreadState::Waiting;
    rt.event("thread_wait", {{"uid", u(cur->uid)}, {"type", static_cast<std::uint32_t>(wait.type)},
        {"object", u(wait.object)}, {"deadline", wait.deadline}, {"time", now_us_}});
    if (wait.callbacks && has_pending_callbacks(cur->uid)) {
        cur->status = InternalThreadState::Running;
        if (start_callback(*cur, ctx, resume, true)) return;
        cur->status = InternalThreadState::Waiting;
    }
    dispatch_next(rt, ctx);
}

std::optional<std::int32_t> ThreadManager::block_or_result(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx,
                                                           WaitInfo wait, std::uint32_t timeout_ptr) {
    // A zero timeout never blocks: it reports WAIT_TIMEOUT immediately.
    if (timeout_ptr != 0u && rt.memory().contains(timeout_ptr, 4u)) {
        const std::uint32_t micros = rt.memory().load32(timeout_ptr);
        if (micros == 0u) {
            if (auto *cur = current_thread()) { cur->wait = wait; remove_from_object(*cur); cur->wait = {}; }
            return SCE_KERNEL_ERROR_WAIT_TIMEOUT;
        }
        wait.deadline = now_us_ + micros;
        wait.timeout_ptr = timeout_ptr;
    }
    block_current(rt, ctx, wait);
    return std::nullopt;
}

void ThreadManager::preempt_if_needed(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
    auto *cur = current_thread();
    auto *best = best_ready();
    if (cur == nullptr || best == nullptr || cur->status != InternalThreadState::Running) return;
    if (best->current_priority >= cur->current_priority) return;
    auto saved = ctx;
    saved.pc = resume_pc(ctx);
    cur->context = saved;
    cur->status = InternalThreadState::Ready;
    enqueue_ready(cur->uid, true); // [INFERRED] a preempted thread keeps its place
    load_thread(*best, ctx, cur->uid);
    rt.event("thread_preempt", {{"from", u(cur->uid)}, {"to", u(best->uid)}, {"time", now_us_}});
}

void ThreadManager::on_syscall_return(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
    memory_ = &rt.memory();
    runtime_ = &rt;
    now_us_ += kSyscallCostUs;
    expire_deadlines(rt);
    // Callbacks are delivered only at CB waits and sceKernelCheckCallback.
    preempt_if_needed(rt, ctx);
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

std::int32_t ThreadManager::create_callback(std::string_view name, std::uint32_t function, std::uint32_t common_argument) {
    const auto uid = next_uid_++;
    callbacks_.emplace(uid, CallbackObject{uid, current_thread_id_, std::string(name.substr(0, 31)), function, common_argument});
    return uid;
}

const CallbackObject *ThreadManager::get_callback(std::int32_t uid) const noexcept {
    const auto it = callbacks_.find(uid);
    return it == callbacks_.end() ? nullptr : &it->second;
}

std::int32_t ThreadManager::delete_callback(std::int32_t uid) {
    return callbacks_.erase(uid) != 0u ? 0 : SCE_KERNEL_ERROR_UNKNOWN_CBID;
}

bool ThreadManager::has_pending_callbacks(std::int32_t uid) const noexcept {
    return std::any_of(callbacks_.begin(), callbacks_.end(), [&](const auto &entry) {
        return entry.second.owner_thread_uid == uid && entry.second.notify_count > 0;
    });
}

std::int32_t ThreadManager::notify_callback(psprecomp::Runtime &rt, std::int32_t uid, std::uint32_t arg) {
    auto it = callbacks_.find(uid);
    if (it == callbacks_.end()) return SCE_KERNEL_ERROR_UNKNOWN_CBID;
    ++it->second.notify_count;
    it->second.notify_arg = arg;
    rt.event("callback_notify", {{"uid", u(uid)}, {"arg", arg}, {"count", u(it->second.notify_count)}}, it->second.name);
    // An owner already sleeping in a CB wait is woken to run it.
    if (auto *owner = get_thread(it->second.owner_thread_uid);
        owner != nullptr && owner->status == InternalThreadState::Waiting && owner->wait.callbacks &&
        !owner->callback && !owner->callback_pending_run) {
        owner->callback_pending_run = true;
        owner->status = InternalThreadState::Ready;
        enqueue_ready(owner->uid);
    }
    return 0;
}

bool ThreadManager::start_callback(ThreadControlBlock &t, psprecomp::AllegrexContext &ctx,
                                   const psprecomp::AllegrexContext &resume, bool rewait) {
    CallbackObject *cb = nullptr;
    for (auto &[uid, object] : callbacks_)
        if (object.owner_thread_uid == t.uid && object.notify_count > 0) { cb = &object; break; }
    if (cb == nullptr) return false;
    if (!t.callback) t.callback = CallbackFrame{cb->uid, resume, rewait, std::nullopt};
    else t.callback->callback = cb->uid;
    const auto count = cb->notify_count;
    const auto arg = cb->notify_arg;
    cb->notify_count = 0;
    cb->notify_arg = 0;
    ctx = resume;
    ctx.pc = cb->function;
    ctx.set_gpr(4, u(count));
    ctx.set_gpr(5, arg);
    ctx.set_gpr(6, cb->common_argument);
    ctx.set_gpr(31, kCallbackReturnSentinel);
    ctx.set_gpr(29, (resume.gpr[29] - 64u) & ~15u); // below the interrupted frame
    if (runtime_ != nullptr)
        runtime_->event("callback_run", {{"uid", u(cb->uid)}, {"thread", u(t.uid)}, {"count", u(count)}, {"arg", arg}}, cb->name);
    return true;
}

bool ThreadManager::check_callbacks(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
    auto *cur = current_thread();
    if (cur == nullptr || cur->callback || !has_pending_callbacks(cur->uid)) return false;
    auto resume = ctx;
    resume.pc = ctx.gpr[31];
    resume.set_gpr(2, 1u); // sceKernelCheckCallback reports that callbacks ran
    runtime_ = &rt;
    return start_callback(*cur, ctx, resume, false);
}

void ThreadManager::callback_returned(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
    auto *cur = current_thread();
    if (cur == nullptr || !cur->callback) { rt.stop("Callback return outside a callback frame"); return; }
    const auto finished = cur->callback->callback;
    if (ctx.gpr[2] != 0u) callbacks_.erase(finished); // non-zero return deletes the callback
    rt.event("callback_return", {{"uid", u(finished)}, {"result", ctx.gpr[2]}});
    const auto frame = *cur->callback;
    if (start_callback(*cur, ctx, frame.resume, frame.rewait)) return;
    cur->callback.reset();
    if (frame.woken) {
        ctx = frame.resume;
        ctx.set_gpr(2, u(*frame.woken));
        return;
    }
    if (!frame.rewait) { ctx = frame.resume; return; }
    // Back into the interrupted CB wait.
    cur->context = frame.resume;
    cur->status = InternalThreadState::Waiting;
    if (cur->wait.deadline <= now_us_) {
        finish_wait(rt, *cur, cur->wait.type == WaitType::Delay ? 0 : SCE_KERNEL_ERROR_WAIT_TIMEOUT);
    }
    if (cur->status == InternalThreadState::Ready) { remove_ready(cur->uid); cur->status = InternalThreadState::Running; ctx = cur->context; return; }
    dispatch_next(rt, ctx);
}

// ---------------------------------------------------------------------------
// Thread services
// ---------------------------------------------------------------------------

std::optional<std::int32_t> ThreadManager::delay_current(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx,
                                                         std::uint32_t microseconds, bool callbacks) {
    WaitInfo wait{WaitType::Delay, 0, now_us_ + std::max<std::uint32_t>(microseconds, 1u), 0u, callbacks};
    block_current(rt, ctx, wait);
    return std::nullopt;
}

std::optional<std::int32_t> ThreadManager::sleep_current(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, bool callbacks) {
    auto *cur = current_thread();
    if (cur == nullptr) return SCE_KERNEL_ERROR_ILLEGAL_THID;
    if (cur->wakeup_count > 0) { --cur->wakeup_count; return 0; }
    block_current(rt, ctx, WaitInfo{WaitType::Sleep, 0, kNoDeadline, 0u, callbacks});
    return std::nullopt;
}

std::int32_t ThreadManager::wakeup_thread(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid) {
    auto *t = get_thread(uid == 0 ? current_thread_id_ : uid);
    if (t == nullptr) return SCE_KERNEL_ERROR_UNKNOWN_THID;
    if (t->status == InternalThreadState::Dormant || t->status == InternalThreadState::Stopped) return SCE_KERNEL_ERROR_DORMANT;
    ctx.set_gpr(2, 0u);
    if (t->status == InternalThreadState::Waiting && t->wait.type == WaitType::Sleep) {
        wake(rt, t->uid, 0);
        preempt_if_needed(rt, ctx);
    } else {
        ++t->wakeup_count;
    }
    return 0;
}

std::optional<std::int32_t> ThreadManager::wait_thread_end(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx,
                                                           std::int32_t uid, std::uint32_t timeout_ptr, bool callbacks) {
    auto *t = get_thread(uid);
    if (t == nullptr) return SCE_KERNEL_ERROR_UNKNOWN_THID;
    if (uid == current_thread_id_) return SCE_KERNEL_ERROR_ILLEGAL_THID;
    if (t->status == InternalThreadState::Dormant || t->status == InternalThreadState::Stopped) return t->exit_status;
    return block_or_result(rt, ctx, WaitInfo{WaitType::ThreadEnd, uid, kNoDeadline, 0u, callbacks}, timeout_ptr);
}

std::int32_t ThreadManager::delete_thread(std::int32_t uid) {
    auto *t = get_thread(uid);
    if (uid == 0 || uid == current_thread_id_) return SCE_KERNEL_ERROR_ILLEGAL_THID;
    if (t == nullptr) return SCE_KERNEL_ERROR_UNKNOWN_THID;
    if (t->status != InternalThreadState::Dormant && t->status != InternalThreadState::Stopped) return SCE_KERNEL_ERROR_NOT_DORMANT;
    remove_ready(uid);
    threads_.erase(uid);
    return 0;
}

std::int32_t ThreadManager::change_priority(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid, std::int32_t priority) {
    auto *t = get_thread(uid == 0 ? current_thread_id_ : uid);
    if (t == nullptr) return SCE_KERNEL_ERROR_UNKNOWN_THID;
    if (t->status == InternalThreadState::Dormant || t->status == InternalThreadState::Stopped) return SCE_KERNEL_ERROR_DORMANT;
    if (priority == 0) priority = get_thread(current_thread_id_)->current_priority;
    if (priority < 8 || priority > 119) return SCE_KERNEL_ERROR_ILLEGAL_PRIORITY;
    t->current_priority = priority;
    if (t->status == InternalThreadState::Ready) enqueue_ready(t->uid);
    ctx.set_gpr(2, 0u);
    preempt_if_needed(rt, ctx);
    return 0;
}

std::int32_t ThreadManager::suspend_thread(std::int32_t uid) {
    auto *t = get_thread(uid);
    if (uid == 0 || uid == current_thread_id_) return SCE_KERNEL_ERROR_ILLEGAL_THID;
    if (t == nullptr) return SCE_KERNEL_ERROR_UNKNOWN_THID;
    if (t->status == InternalThreadState::Dormant || t->status == InternalThreadState::Stopped) return SCE_KERNEL_ERROR_DORMANT;
    if (t->suspended) return SCE_KERNEL_ERROR_SUSPEND;
    t->suspended = true;
    return 0;
}

std::int32_t ThreadManager::resume_thread(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid) {
    auto *t = get_thread(uid);
    if (t == nullptr) return SCE_KERNEL_ERROR_UNKNOWN_THID;
    if (!t->suspended) return SCE_KERNEL_ERROR_NOT_SUSPEND;
    t->suspended = false;
    ctx.set_gpr(2, 0u);
    preempt_if_needed(rt, ctx);
    return 0;
}

std::int32_t ThreadManager::refer_thread_status(psprecomp::GuestMemory &memory, std::int32_t uid, std::uint32_t info) const {
    const auto *t = get_thread(uid == 0 ? current_thread_id_ : uid);
    if (t == nullptr) return SCE_KERNEL_ERROR_UNKNOWN_THID;
    if (info == 0u || !memory.contains(info, 4u)) return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
    const std::uint32_t size = memory.load32(info);
    if (size < 4u || !memory.contains(info, size)) return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
    // SceKernelThreadInfo (PSPSDK pspthreadman.h): size, name[32], attr,
    // status, entry, stack, stackSize, gpReg, initPriority, currentPriority,
    // waitType, waitId, wakeupCount, exitStatus, runClocks[2], counters[3].
    std::uint32_t status = 0u;
    switch (t->status) {
    case InternalThreadState::Running: status = 1u; break;
    case InternalThreadState::Ready: status = 2u; break;
    case InternalThreadState::Waiting: status = 4u; break;
    case InternalThreadState::Suspended: status = 8u; break;
    default: status = 16u; break;
    }
    if (t->suspended) status |= 8u;
    std::uint32_t words[26]{};
    words[0] = size;
    words[9] = t->attributes; words[10] = status; words[11] = t->entry_pc; words[12] = t->stack_address;
    words[13] = t->stack_size; words[14] = t->gp; words[15] = u(t->initial_priority);
    words[16] = u(t->current_priority);
    const auto wait_type = static_cast<std::uint32_t>(t->wait.type);
    words[17] = t->status == InternalThreadState::Waiting && wait_type < 0x100u ? wait_type : 0u;
    words[18] = t->status == InternalThreadState::Waiting ? u(t->wait.object) : 0u;
    words[19] = u(t->wakeup_count); words[20] = u(t->exit_status);
    for (std::uint32_t i = 1; i < 26u && i * 4u < size; ++i) memory.store32(info + i * 4u, words[i]);
    for (std::uint32_t i = 0; i < 32u && 4u + i < size; ++i)
        memory.store8(info + 4u + i, i < t->name.size() ? static_cast<std::uint8_t>(t->name[i]) : 0u);
    return 0;
}

// ---------------------------------------------------------------------------
// Semaphores  (pspautotests threads/semaphores/*.expected)
// ---------------------------------------------------------------------------

std::int32_t ThreadManager::create_sema(std::string_view name, std::uint32_t attr, std::int32_t init, std::int32_t max) {
    if ((attr & ~0x1FFu) != 0u) return SCE_KERNEL_ERROR_ILLEGAL_ATTR;
    const auto uid = next_uid_++;
    semas_.emplace(uid, SemaphoreObject{uid, std::string(name.substr(0, 31)), attr, init, init, max, {}});
    return uid;
}

const SemaphoreObject *ThreadManager::get_sema(std::int32_t uid) const noexcept {
    const auto it = semas_.find(uid);
    return it == semas_.end() ? nullptr : &it->second;
}

std::int32_t ThreadManager::fail_waiters(psprecomp::Runtime &rt, std::vector<std::int32_t> &waiters, std::int32_t result) {
    const auto copy = waiters;
    waiters.clear();
    for (const auto uid : copy) wake(rt, uid, result);
    return static_cast<std::int32_t>(copy.size());
}

std::int32_t ThreadManager::delete_sema(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid) {
    auto it = semas_.find(uid);
    if (it == semas_.end()) return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
    auto waiters = it->second.waiters;
    semas_.erase(it);
    for (const auto w : waiters) wake(rt, w, SCE_KERNEL_ERROR_WAIT_DELETE);
    ctx.set_gpr(2, 0u);
    preempt_if_needed(rt, ctx);
    return 0;
}

void ThreadManager::release_sema_waiters(psprecomp::Runtime &rt, SemaphoreObject &sema) {
    // [INFERRED] every waiter whose request fits is released in queue order.
    for (std::size_t i = 0; i < sema.waiters.size();) {
        auto *t = get_thread(sema.waiters[i]);
        const auto need = t != nullptr ? static_cast<std::int32_t>(t->wait.a) : 0;
        if (t != nullptr && need <= sema.count) {
            sema.count -= need;
            wake(rt, t->uid, 0);
            continue; // wake() removed it from the vector
        }
        ++i;
    }
}

std::int32_t ThreadManager::signal_sema(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid, std::int32_t signal) {
    auto it = semas_.find(uid);
    if (it == semas_.end()) return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
    auto &sema = it->second;
    // signal.expected: count + signal above max fails; negative signals are allowed.
    if (static_cast<std::int64_t>(sema.count) + signal > sema.max) return SCE_KERNEL_ERROR_SEMA_OVERFLOW;
    sema.count += signal;
    release_sema_waiters(rt, sema);
    ctx.set_gpr(2, 0u);
    preempt_if_needed(rt, ctx);
    return 0;
}

std::optional<std::int32_t> ThreadManager::wait_sema(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid,
                                                     std::int32_t count, std::uint32_t timeout_ptr, bool callbacks) {
    auto it = semas_.find(uid);
    if (it == semas_.end()) return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
    auto &sema = it->second;
    if (count <= 0 || count > sema.max) return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    if (sema.waiters.empty() && sema.count >= count) {
        sema.count -= count;
        return 0;
    }
    insert_waiter(sema.waiters, current_thread_id_, (sema.attr & 0x100u) != 0u);
    WaitInfo wait{WaitType::Sema, uid, kNoDeadline, 0u, callbacks, u(count)};
    auto result = block_or_result(rt, ctx, wait, timeout_ptr);
    if (result) sema.waiters.erase(std::remove(sema.waiters.begin(), sema.waiters.end(), current_thread_id_), sema.waiters.end());
    return result;
}

std::int32_t ThreadManager::poll_sema(std::int32_t uid, std::int32_t count) {
    auto it = semas_.find(uid);
    if (it == semas_.end()) return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
    auto &sema = it->second;
    // poll.expected: an empty semaphore reports SEMA_ZERO even for count 0.
    if (sema.count <= 0) return SCE_KERNEL_ERROR_SEMA_ZERO;
    if (count <= 0) return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    if (!sema.waiters.empty() || count > sema.count) return SCE_KERNEL_ERROR_SEMA_ZERO;
    sema.count -= count;
    return 0;
}

// ---------------------------------------------------------------------------
// Event flags  (pspautotests threads/events/*/*.expected)
// ---------------------------------------------------------------------------

namespace {
constexpr std::uint32_t kEvfWaitOr = 0x01u, kEvfWaitClearAll = 0x10u, kEvfWaitClear = 0x20u;
constexpr std::uint32_t kEvfAttrWaitMultiple = 0x200u;

bool evf_mode_valid(std::uint32_t mode) {
    return (mode & ~(kEvfWaitOr | kEvfWaitClearAll | kEvfWaitClear)) == 0u &&
           (mode & (kEvfWaitClearAll | kEvfWaitClear)) != (kEvfWaitClearAll | kEvfWaitClear);
}
bool evf_matches(std::uint32_t current, std::uint32_t bits, std::uint32_t mode) {
    return (mode & kEvfWaitOr) != 0u ? (current & bits) != 0u : (current & bits) == bits;
}
void evf_consume(std::uint32_t &current, std::uint32_t bits, std::uint32_t mode) {
    if ((mode & kEvfWaitClearAll) != 0u) current = 0u;
    else if ((mode & kEvfWaitClear) != 0u) current &= ~bits;
}
} // namespace

std::int32_t ThreadManager::create_event_flag(std::string_view name, std::uint32_t attr, std::uint32_t bits) {
    if ((attr & ~0x3FFu) != 0u) return SCE_KERNEL_ERROR_ILLEGAL_ATTR;
    const auto uid = next_uid_++;
    event_flags_.emplace(uid, EventFlagObject{uid, std::string(name.substr(0, 31)), attr, bits, bits, {}});
    return uid;
}

const EventFlagObject *ThreadManager::get_event_flag(std::int32_t uid) const noexcept {
    const auto it = event_flags_.find(uid);
    return it == event_flags_.end() ? nullptr : &it->second;
}

std::int32_t ThreadManager::delete_event_flag(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid) {
    auto it = event_flags_.find(uid);
    if (it == event_flags_.end()) return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
    auto waiters = it->second.waiters;
    event_flags_.erase(it);
    for (const auto w : waiters) wake(rt, w, SCE_KERNEL_ERROR_WAIT_DELETE);
    ctx.set_gpr(2, 0u);
    preempt_if_needed(rt, ctx);
    return 0;
}

void ThreadManager::release_event_flag_waiters(psprecomp::Runtime &rt, EventFlagObject &flag, psprecomp::GuestMemory &memory) {
    for (std::size_t i = 0; i < flag.waiters.size();) {
        auto *t = get_thread(flag.waiters[i]);
        if (t != nullptr && evf_matches(flag.bits, t->wait.a, t->wait.b)) {
            if (t->wait.c != 0u && memory.contains(t->wait.c, 4u)) memory.store32(t->wait.c, flag.bits);
            evf_consume(flag.bits, t->wait.a, t->wait.b);
            wake(rt, t->uid, 0);
            continue;
        }
        ++i;
    }
}

std::int32_t ThreadManager::set_event_flag(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid, std::uint32_t bits) {
    auto it = event_flags_.find(uid);
    if (it == event_flags_.end()) return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
    it->second.bits |= bits;
    release_event_flag_waiters(rt, it->second, rt.memory());
    ctx.set_gpr(2, 0u);
    preempt_if_needed(rt, ctx);
    return 0;
}

std::int32_t ThreadManager::clear_event_flag(std::int32_t uid, std::uint32_t bits) {
    auto it = event_flags_.find(uid);
    if (it == event_flags_.end()) return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
    it->second.bits &= bits; // sceKernelClearEventFlag keeps the bits given
    return 0;
}

std::optional<std::int32_t> ThreadManager::wait_event_flag(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx,
                                                           std::int32_t uid, std::uint32_t bits, std::uint32_t mode,
                                                           std::uint32_t out_ptr, std::uint32_t timeout_ptr, bool callbacks) {
    if (!evf_mode_valid(mode)) return SCE_KERNEL_ERROR_ILLEGAL_MODE;
    if (bits == 0u) return SCE_KERNEL_ERROR_EVF_ILPAT;
    auto it = event_flags_.find(uid);
    if (it == event_flags_.end()) return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
    auto &flag = it->second;
    auto &memory = rt.memory();
    if (evf_matches(flag.bits, bits, mode)) {
        if (out_ptr != 0u && memory.contains(out_ptr, 4u)) memory.store32(out_ptr, flag.bits);
        evf_consume(flag.bits, bits, mode);
        return 0;
    }
    if (!flag.waiters.empty() && (flag.attr & kEvfAttrWaitMultiple) == 0u) return SCE_KERNEL_ERROR_EVF_MULTI;
    if (timeout_ptr != 0u && memory.contains(timeout_ptr, 4u) && memory.load32(timeout_ptr) == 0u) {
        if (out_ptr != 0u && memory.contains(out_ptr, 4u)) memory.store32(out_ptr, flag.bits);
        return SCE_KERNEL_ERROR_WAIT_TIMEOUT;
    }
    insert_waiter(flag.waiters, current_thread_id_, (flag.attr & 0x100u) != 0u);
    WaitInfo wait{WaitType::EventFlag, uid, kNoDeadline, 0u, callbacks, bits, mode, out_ptr};
    return block_or_result(rt, ctx, wait, timeout_ptr);
}

std::int32_t ThreadManager::poll_event_flag(psprecomp::GuestMemory &memory, std::int32_t uid, std::uint32_t bits,
                                            std::uint32_t mode, std::uint32_t out_ptr) {
    if (!evf_mode_valid(mode)) return SCE_KERNEL_ERROR_ILLEGAL_MODE;
    if (bits == 0u) return SCE_KERNEL_ERROR_EVF_ILPAT;
    auto it = event_flags_.find(uid);
    if (it == event_flags_.end()) return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
    auto &flag = it->second;
    if (!evf_matches(flag.bits, bits, mode)) {
        if (out_ptr != 0u && memory.contains(out_ptr, 4u)) memory.store32(out_ptr, flag.bits);
        return SCE_KERNEL_ERROR_EVF_COND;
    }
    if (out_ptr != 0u && memory.contains(out_ptr, 4u)) memory.store32(out_ptr, flag.bits);
    evf_consume(flag.bits, bits, mode);
    return 0;
}

// ---------------------------------------------------------------------------
// Kernel mutexes
// ---------------------------------------------------------------------------

namespace { constexpr std::uint32_t kMutexRecursive = 0x200u; }

std::int32_t ThreadManager::create_mutex(std::string_view name, std::uint32_t attr, std::int32_t init) {
    if ((attr & ~0xBFFu) != 0u) return SCE_KERNEL_ERROR_ILLEGAL_ATTR;
    if (init < 0 || (init > 1 && (attr & kMutexRecursive) == 0u)) return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    const auto uid = next_uid_++;
    mutexes_.emplace(uid, MutexObject{uid, std::string(name.substr(0, 31)), attr, init, init, init > 0 ? current_thread_id_ : 0, {}});
    return uid;
}

const MutexObject *ThreadManager::get_mutex(std::int32_t uid) const noexcept {
    const auto it = mutexes_.find(uid);
    return it == mutexes_.end() ? nullptr : &it->second;
}

std::int32_t ThreadManager::delete_mutex(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid) {
    auto it = mutexes_.find(uid);
    if (it == mutexes_.end()) return SCE_KERNEL_ERROR_MUTEX_NOT_FOUND;
    auto waiters = it->second.waiters;
    mutexes_.erase(it);
    for (const auto w : waiters) wake(rt, w, SCE_KERNEL_ERROR_WAIT_DELETE);
    ctx.set_gpr(2, 0u);
    preempt_if_needed(rt, ctx);
    return 0;
}

void ThreadManager::release_mutex_waiters(psprecomp::Runtime &rt, MutexObject &mutex) {
    if (mutex.count != 0 || mutex.waiters.empty()) return;
    auto *t = get_thread(mutex.waiters.front());
    if (t == nullptr) return;
    mutex.owner = t->uid;
    mutex.count = static_cast<std::int32_t>(t->wait.a);
    wake(rt, t->uid, 0);
}

std::optional<std::int32_t> ThreadManager::lock_mutex(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid,
                                                      std::int32_t count, std::uint32_t timeout_ptr, bool callbacks) {
    auto it = mutexes_.find(uid);
    if (it == mutexes_.end()) return SCE_KERNEL_ERROR_MUTEX_NOT_FOUND;
    auto &mutex = it->second;
    if (count <= 0 || (count > 1 && (mutex.attr & kMutexRecursive) == 0u)) return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    if (mutex.count == 0) { mutex.count = count; mutex.owner = current_thread_id_; return 0; }
    if (mutex.owner == current_thread_id_) {
        if ((mutex.attr & kMutexRecursive) == 0u) return SCE_KERNEL_ERROR_MUTEX_RECURSIVE;
        if (static_cast<std::int64_t>(mutex.count) + count > std::numeric_limits<std::int32_t>::max())
            return SCE_KERNEL_ERROR_MUTEX_LOCK_OVERFLOW;
        mutex.count += count;
        return 0;
    }
    insert_waiter(mutex.waiters, current_thread_id_, (mutex.attr & 0x100u) != 0u);
    return block_or_result(rt, ctx, WaitInfo{WaitType::Mutex, uid, kNoDeadline, 0u, callbacks, u(count)}, timeout_ptr);
}

std::int32_t ThreadManager::unlock_mutex(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid, std::int32_t count) {
    auto it = mutexes_.find(uid);
    if (it == mutexes_.end()) return SCE_KERNEL_ERROR_MUTEX_NOT_FOUND;
    auto &mutex = it->second;
    if (count <= 0 || (count > 1 && (mutex.attr & kMutexRecursive) == 0u)) return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    if (mutex.count == 0 || mutex.owner != current_thread_id_) return SCE_KERNEL_ERROR_MUTEX_UNLOCKED;
    if (mutex.count < count) return SCE_KERNEL_ERROR_MUTEX_UNLOCK_UNDERFLOW;
    mutex.count -= count;
    if (mutex.count == 0) {
        mutex.owner = 0;
        release_mutex_waiters(rt, mutex);
    }
    ctx.set_gpr(2, 0u);
    preempt_if_needed(rt, ctx);
    return 0;
}

// ---------------------------------------------------------------------------
// LwMutex: user-space workarea (references/uofw/src/kd/usersystemlib/lwmutex.c,
// references/uofw/include/threadman_user.h) with kernel wait queues.
// Workarea: +0 lockCount, +4 owner thid, +8 attr, +12 numWaitThreads, +16 uid.
// ---------------------------------------------------------------------------

std::int32_t ThreadManager::create_lw_mutex(psprecomp::GuestMemory &memory, std::uint32_t workarea, std::string_view name,
                                            std::uint32_t attr, std::int32_t init) {
    memory_ = &memory;
    if (!memory.contains(workarea, 32u)) return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
    if ((attr & ~0x3FFu) != 0u) return SCE_KERNEL_ERROR_ILLEGAL_ATTR;
    if (init < 0 || (init > 1 && (attr & kMutexRecursive) == 0u)) return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    const auto uid = next_uid_++;
    lw_mutexes_.emplace(uid, LwMutexObject{uid, std::string(name.substr(0, 31)), attr, workarea, init, {}});
    memory.store32(workarea + 0u, u(init));
    memory.store32(workarea + 4u, init > 0 ? u(current_thread_id_) : 0u);
    memory.store32(workarea + 8u, attr);
    memory.store32(workarea + 12u, 0u);
    memory.store32(workarea + 16u, u(uid));
    for (std::uint32_t o = 20u; o < 32u; o += 4u) memory.store32(workarea + o, 0u);
    return 0;
}

std::int32_t ThreadManager::delete_lw_mutex(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::uint32_t workarea) {
    auto &memory = rt.memory();
    if (!memory.contains(workarea, 32u)) return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
    const auto uid = static_cast<std::int32_t>(memory.load32(workarea + 16u));
    auto it = lw_mutexes_.find(uid);
    if (it == lw_mutexes_.end() || it->second.workarea != workarea) return SCE_KERNEL_ERROR_LWMUTEX_NOT_FOUND;
    auto waiters = it->second.waiters;
    lw_mutexes_.erase(it);
    for (const auto w : waiters) wake(rt, w, SCE_KERNEL_ERROR_WAIT_DELETE);
    memory.store32(workarea + 16u, 0xFFFFFFFFu);
    ctx.set_gpr(2, 0u);
    preempt_if_needed(rt, ctx);
    return 0;
}

std::int32_t ThreadManager::try_lock_lw_mutex(psprecomp::GuestMemory &memory, std::uint32_t workarea, std::int32_t count) {
    if (!memory.contains(workarea, 32u)) return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
    if (count <= 0) return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    if (static_cast<std::int32_t>(memory.load32(workarea + 16u)) < 0) return SCE_KERNEL_ERROR_LWMUTEX_NOT_FOUND;
    const auto lock_count = static_cast<std::int32_t>(memory.load32(workarea + 0u));
    const auto owner = static_cast<std::int32_t>(memory.load32(workarea + 4u));
    const auto attr = memory.load32(workarea + 8u);
    const bool recursive = (attr & kMutexRecursive) != 0u;
    if (owner == current_thread_id_) {
        if (!recursive) return SCE_KERNEL_ERROR_LWMUTEX_RECURSIVE;
        if (static_cast<std::int64_t>(lock_count) + count > std::numeric_limits<std::int32_t>::max())
            return SCE_KERNEL_ERROR_LWMUTEX_LOCK_OVERFLOW;
        memory.store32(workarea + 0u, u(lock_count + count));
        return 0;
    }
    if (owner != 0) return SCE_KERNEL_ERROR_LWMUTEX_LOCKED;
    if (count != 1 && !recursive) return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    memory.store32(workarea + 4u, u(current_thread_id_));
    memory.store32(workarea + 0u, u(count));
    return 0;
}

std::optional<std::int32_t> ThreadManager::lock_lw_mutex(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx,
                                                         std::uint32_t workarea, std::int32_t count,
                                                         std::uint32_t timeout_ptr, bool callbacks) {
    auto &memory = rt.memory();
    memory_ = &memory;
    const auto result = try_lock_lw_mutex(memory, workarea, count);
    if (result != SCE_KERNEL_ERROR_LWMUTEX_LOCKED) return result;
    const auto uid = static_cast<std::int32_t>(memory.load32(workarea + 16u));
    auto it = lw_mutexes_.find(uid);
    if (it == lw_mutexes_.end()) return SCE_KERNEL_ERROR_LWMUTEX_NOT_FOUND;
    auto &mutex = it->second;
    insert_waiter(mutex.waiters, current_thread_id_, (mutex.attr & 0x100u) != 0u);
    memory.store32(workarea + 12u, u(static_cast<std::int32_t>(mutex.waiters.size())));
    auto blocked = block_or_result(rt, ctx, WaitInfo{WaitType::LwMutex, uid, kNoDeadline, 0u, callbacks, u(count), workarea},
                                   timeout_ptr);
    if (blocked) {
        mutex.waiters.erase(std::remove(mutex.waiters.begin(), mutex.waiters.end(), current_thread_id_), mutex.waiters.end());
        memory.store32(workarea + 12u, u(static_cast<std::int32_t>(mutex.waiters.size())));
    }
    return blocked;
}

void ThreadManager::release_lw_mutex_waiters(psprecomp::Runtime &rt, LwMutexObject &mutex) {
    auto &memory = rt.memory();
    if (memory.load32(mutex.workarea + 4u) != 0u || mutex.waiters.empty()) return;
    auto *t = get_thread(mutex.waiters.front());
    if (t == nullptr) return;
    memory.store32(mutex.workarea + 4u, u(t->uid));
    memory.store32(mutex.workarea + 0u, t->wait.a);
    wake(rt, t->uid, 0);
}

std::int32_t ThreadManager::unlock_lw_mutex(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::uint32_t workarea,
                                            std::int32_t count) {
    auto &memory = rt.memory();
    memory_ = &memory;
    if (!memory.contains(workarea, 32u)) return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
    if (count <= 0) return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    const auto uid = static_cast<std::int32_t>(memory.load32(workarea + 16u));
    if (uid < 0) return SCE_KERNEL_ERROR_LWMUTEX_NOT_FOUND;
    const auto attr = memory.load32(workarea + 8u);
    if (count != 1 && (attr & kMutexRecursive) == 0u) return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    if (static_cast<std::int32_t>(memory.load32(workarea + 4u)) != current_thread_id_) return SCE_KERNEL_ERROR_LWMUTEX_UNLOCKED;
    const auto lock_count = static_cast<std::int32_t>(memory.load32(workarea + 0u));
    if (lock_count - count > 0) { memory.store32(workarea + 0u, u(lock_count - count)); ctx.set_gpr(2, 0u); return 0; }
    if (lock_count - count < 0) return SCE_KERNEL_ERROR_LWMUTEX_UNLOCK_UNDERFLOW;
    memory.store32(workarea + 0u, 0u);
    memory.store32(workarea + 4u, 0u);
    if (auto it = lw_mutexes_.find(uid); it != lw_mutexes_.end()) release_lw_mutex_waiters(rt, it->second);
    ctx.set_gpr(2, 0u);
    preempt_if_needed(rt, ctx);
    return 0;
}

void install_threadman_post_import_hook() {
    psprecomp::set_runtime_post_import_hook([](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
        if (g_active_kernel != nullptr) g_active_kernel->threads().on_syscall_return(rt, ctx);
    });
}

void register_thread_trampolines(psprecomp::Runtime &runtime, KernelState &kernel) {
    g_active_kernel = &kernel;
    runtime.register_function(ThreadManager::kThreadReturnSentinel, &thread_return_trampoline, "thread_return_trampoline");
    runtime.register_function(ThreadManager::kCallbackReturnSentinel, &callback_return_trampoline, "callback_return_trampoline");
}

} // namespace p3p3ds::hle
