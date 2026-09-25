#include "kernel.hpp"

#include "kernel/fast_loading.hpp"
#include "kernel/load_trace.hpp"
#include "perf/frame_stats.hpp"
#include "settings/settings.hpp"
#include "psprecomp/common.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace mhp3rd {
namespace {

constexpr std::uint32_t kMemoryGranularity = 0x100u;
constexpr std::uint32_t kLoaderPriority = 0x20u;
constexpr std::uint32_t kLoaderStackSize = 0x40000u;
constexpr std::uint32_t kThreadAttrNoFillStack = 0x00100000u;
constexpr std::uint32_t kThreadArgumentHome = 0x40u;
// Virtual time charged per runtime starvation boundary, i.e. per
// kStarvationInterval dispatches without an import call.
constexpr std::uint64_t kStarvationQuantumUs = 1000u;
// Idle vblanks with no runnable thread before the scheduler reports a deadlock.
constexpr std::uint64_t kIdleVBlankLimit = 60u * 60u * 5u;
constexpr std::int32_t kInterruptIdentity = -2;
constexpr std::int32_t kIdleIdentity = -3;
// Scratch for SceKernelSysClock values passed to VTimer handlers.
constexpr std::uint32_t kVTimerClockScratch = 0x08180000u;

// PSP reserves 256 bytes at the top of every thread stack for the thread
// control block and points $k0 at it; SDK code reads and writes it directly.
constexpr std::uint32_t kThreadControlBlockSize = 0x100u;

std::uint32_t align_up(std::uint32_t value, std::uint32_t alignment) noexcept {
    return (value + alignment - 1u) & ~(alignment - 1u);
}

bool trace_enabled() {
    static const bool enabled = std::getenv("MHP3RD_TRACE_KERNEL") != nullptr;
    return enabled;
}

void native_thread_exit(Runtime &, AllegrexContext &ctx) { kernel().thread_exit_stub(ctx); }
void native_interrupt_return(Runtime &, AllegrexContext &ctx) { kernel().interrupt_return_stub(ctx); }
void native_idle(Runtime &, AllegrexContext &ctx) { kernel().idle_stub(ctx); }
void native_guest_call_return(Runtime &, AllegrexContext &ctx) { kernel().guest_call_return_stub(ctx); }
void native_starvation(Runtime &, AllegrexContext &ctx) { kernel().on_starvation(ctx); }

const char *status_name(ThreadStatus status) {
    switch (status) {
    case ThreadStatus::Dormant: return "dormant";
    case ThreadStatus::Ready: return "ready";
    case ThreadStatus::Running: return "running";
    case ThreadStatus::Waiting: return "waiting";
    case ThreadStatus::Dead: return "dead";
    }
    return "?";
}

const char *wait_name(WaitType type) {
    switch (type) {
    case WaitType::None: return "none";
    case WaitType::Delay: return "delay";
    case WaitType::Sleep: return "sleep";
    case WaitType::Semaphore: return "sema";
    case WaitType::EventFlag: return "eventflag";
    case WaitType::Mutex: return "mutex";
    case WaitType::VBlank: return "vblank";
    case WaitType::ThreadEnd: return "thread-end";
    case WaitType::Host: return "host";
    }
    return "?";
}

} // namespace

Kernel &kernel() {
    static Kernel instance;
    return instance;
}

void Kernel::install(Runtime &runtime, std::uint32_t gp, std::uint32_t image_end) {
    runtime_ = &runtime;
    gp_ = gp;
    pristine_context_ = AllegrexContext{};
    pristine_context_.eat_vfpu_prefixes();
    pristine_context_.set_gpr(28, gp);

    free_ranges_.clear();
    const std::uint32_t user_start = align_up(image_end, 0x1000u);
    free_ranges_.push_back(FreeRange{user_start, kUserMemoryEnd - user_start});

    runtime.register_function(kThreadExitStub, &native_thread_exit, "mhp3rd_thread_exit");
    runtime.register_function(kInterruptReturnStub, &native_interrupt_return, "mhp3rd_interrupt_return");
    runtime.register_function(kIdleStub, &native_idle, "mhp3rd_idle");
    runtime.register_function(kGuestCallReturnStub, &native_guest_call_return, "mhp3rd_guest_call_return");
    std::uint64_t interval = 20'000u;
    if (const char *text = std::getenv("MHP3RD_STARVATION_INTERVAL")) interval = std::strtoull(text, nullptr, 0);
    psprecomp::set_runtime_starvation_hook(&native_starvation, interval);
}

// ---------------------------------------------------------------------------
// Threads

// Fills the stack, lays out the $k0 control block and returns the initial $sp.
std::uint32_t Kernel::prepare_thread_stack(Thread &thread) {
    auto &memory = runtime_->memory();
    if ((thread.attributes & kThreadAttrNoFillStack) == 0u) {
        for (std::uint32_t offset = 0; offset < thread.stack_size; offset += 4u)
            memory.store32(thread.stack_bottom + offset, 0xFFFFFFFFu);
    }
    const std::uint32_t control_block = thread.stack_bottom + thread.stack_size - kThreadControlBlockSize;
    for (std::uint32_t offset = 0; offset < kThreadControlBlockSize; offset += 4u)
        memory.store32(control_block + offset, 0u);
    memory.store32(control_block + 0xC0u, static_cast<std::uint32_t>(thread.uid));
    memory.store32(control_block + 0xC8u, thread.stack_bottom);
    memory.store32(control_block + 0xF8u, 0xFFFFFFFFu);
    memory.store32(control_block + 0xFCu, 0xFFFFFFFFu);
    memory.store32(thread.stack_bottom, static_cast<std::uint32_t>(thread.uid));
    thread.control_block = control_block;
    return control_block;
}

Thread *Kernel::find_thread(SceUID uid) noexcept {
    const auto found = threads_.find(uid);
    return found != threads_.end() ? found->second.get() : nullptr;
}

void Kernel::start_loader_thread(AllegrexContext &ctx, std::uint32_t entry, std::uint32_t stack_top) {
    auto thread = std::make_unique<Thread>();
    thread->uid = allocate_uid();
    thread->name = "module_start";
    thread->entry = entry;
    thread->priority = thread->initial_priority = kLoaderPriority;
    thread->gp = gp_;
    thread->stack_size = kLoaderStackSize;
    thread->stack_block = allocate_block("module_start stack", 1u, kLoaderStackSize, 0u);
    if (const MemoryBlock *block = find_block(thread->stack_block)) {
        thread->stack_bottom = block->address;
        stack_top = block->address + block->size;
    }
    thread->status = ThreadStatus::Running;
    (void)stack_top;
    const std::uint32_t sp = prepare_thread_stack(*thread);

    const std::uint32_t a0 = ctx.gpr[4];
    const std::uint32_t a1 = ctx.gpr[5];
    ctx = pristine_context_;
    ctx.set_gpr(4, a0);
    ctx.set_gpr(5, a1);
    ctx.set_gpr(26, thread->control_block);
    ctx.set_gpr(29, sp - kThreadArgumentHome);
    ctx.set_gpr(31, kThreadExitStub);
    ctx.pc = entry;

    current_uid_ = thread->uid;
    psprecomp::set_runtime_thread_identity(thread->uid, thread->name);
    threads_.emplace(thread->uid, std::move(thread));
}

std::int32_t Kernel::create_thread(const std::string &name, std::uint32_t entry, std::uint32_t priority,
                                   std::uint32_t stack_size, std::uint32_t attributes, std::uint32_t gp) {
    if (entry == 0u || (entry & 3u) != 0u) return static_cast<std::int32_t>(error::kIllegalEntry);
    if (priority == 0u || priority > 0x7Fu) return static_cast<std::int32_t>(error::kIllegalPriority);
    if (stack_size < 0x200u) return static_cast<std::int32_t>(error::kIllegalStackSize);

    auto thread = std::make_unique<Thread>();
    thread->uid = allocate_uid();
    thread->name = name;
    thread->entry = entry;
    thread->priority = thread->initial_priority = priority;
    thread->attributes = attributes;
    thread->gp = gp;
    thread->stack_size = align_up(stack_size, kMemoryGranularity);
    const std::int32_t block = allocate_block("stack:" + name, 1u, thread->stack_size, 0u);
    if (block < 0) return static_cast<std::int32_t>(error::kNoMemory);
    thread->stack_block = block;
    thread->stack_bottom = find_block(block)->address;
    (void)prepare_thread_stack(*thread);
    thread->status = ThreadStatus::Dormant;
    const SceUID uid = thread->uid;
    threads_.emplace(uid, std::move(thread));
    if (trace_enabled())
        std::cerr << "[kernel] create thread uid=" << uid << " name=" << name
                  << " entry=" << psprecomp::hex32(entry) << " prio=" << psprecomp::hex32(priority) << "\n";
    return uid;
}

std::int32_t Kernel::start_thread(AllegrexContext &ctx, SceUID uid, std::uint32_t argument_size,
                                  std::uint32_t argument_address) {
    Thread *thread = find_thread(uid);
    if (thread == nullptr) return static_cast<std::int32_t>(error::kUnknownThid);
    if (thread->status != ThreadStatus::Dormant) return static_cast<std::int32_t>(error::kNotDormant);
    (void)ctx;

    auto &memory = runtime_->memory();
    AllegrexContext &context = thread->context;
    context = pristine_context_;
    context.set_gpr(28, thread->gp);
    std::uint32_t sp = prepare_thread_stack(*thread);
    std::uint32_t argument_copy = 0u;
    if (argument_address != 0u && argument_size != 0u) {
        sp -= align_up(argument_size, 0x10u);
        for (std::uint32_t i = 0; i < argument_size; ++i)
            memory.store8(sp + i, memory.load8(argument_address + i));
        argument_copy = sp;
    }
    context.set_gpr(4, argument_size);
    context.set_gpr(5, argument_copy);
    context.set_gpr(26, thread->control_block);
    context.set_gpr(29, sp - kThreadArgumentHome);
    context.set_gpr(31, kThreadExitStub);
    context.pc = thread->entry;
    thread->priority = thread->initial_priority;
    thread->wakeup_count = 0u;
    thread->exit_status = 0;
    make_ready(*thread);
    if (trace_enabled())
        std::cerr << "[kernel] start thread uid=" << uid << " name=" << thread->name << "\n";
    return 0;
}

void Kernel::exit_current_thread(AllegrexContext &ctx, std::int32_t status, bool delete_thread) {
    Thread *thread = current_thread();
    if (trace_enabled())
        std::cerr << "[kernel] exit thread uid=" << current_uid_
                  << " name=" << (thread != nullptr ? thread->name : "?") << " status=" << status << "\n";
    if (thread != nullptr) {
        thread->exit_status = status;
        thread->status = ThreadStatus::Dormant;
        const SceUID uid = thread->uid;
        for (auto &[other_uid, other] : threads_) {
            (void)other_uid;
            if (other->status == ThreadStatus::Waiting && other->wait.type == WaitType::ThreadEnd &&
                other->wait.object == uid)
                wake(*other, static_cast<std::uint32_t>(status));
        }
        if (delete_thread) (void)this->delete_thread(uid);
    }
    current_uid_ = 0;
    schedule(ctx);
}

std::int32_t Kernel::terminate_thread(AllegrexContext &ctx, SceUID uid, bool delete_thread) {
    (void)ctx;
    Thread *thread = find_thread(uid);
    if (thread == nullptr) return static_cast<std::int32_t>(error::kUnknownThid);
    if (uid == current_uid_) return static_cast<std::int32_t>(error::kIllegalThid);
    if (thread->status == ThreadStatus::Waiting) remove_waiter(*thread);
    thread->status = ThreadStatus::Dormant;
    thread->exit_status = static_cast<std::int32_t>(0x800201ACu);
    if (delete_thread) return this->delete_thread(uid);
    return 0;
}

std::int32_t Kernel::delete_thread(SceUID uid) {
    Thread *thread = find_thread(uid);
    if (thread == nullptr) return static_cast<std::int32_t>(error::kUnknownThid);
    if (thread->status != ThreadStatus::Dormant && thread->status != ThreadStatus::Dead)
        return static_cast<std::int32_t>(error::kNotDormant);
    if (thread->stack_block != 0) (void)free_block(thread->stack_block);
    threads_.erase(uid);
    return 0;
}

std::int32_t Kernel::change_priority(AllegrexContext &ctx, SceUID uid, std::uint32_t priority) {
    (void)ctx;
    Thread *thread = uid == 0 ? current_thread() : find_thread(uid);
    if (thread == nullptr) return static_cast<std::int32_t>(error::kUnknownThid);
    if (thread->status == ThreadStatus::Dormant) return static_cast<std::int32_t>(error::kDormant);
    if (priority == 0u) priority = current_thread() != nullptr ? current_thread()->priority : thread->priority;
    if (priority > 0x7Fu) return static_cast<std::int32_t>(error::kIllegalPriority);
    thread->priority = priority;
    if (thread->status == ThreadStatus::Ready) thread->ready_sequence = next_ready_sequence_++;
    return 0;
}

// ---------------------------------------------------------------------------
// Scheduling

void Kernel::make_ready(Thread &thread) {
    thread.status = ThreadStatus::Ready;
    thread.wait = WaitState{};
    thread.ready_sequence = next_ready_sequence_++;
}

Thread *Kernel::best_ready_thread() noexcept {
    Thread *best = nullptr;
    for (auto &[uid, thread] : threads_) {
        (void)uid;
        if (thread->status != ThreadStatus::Ready) continue;
        if (best == nullptr || thread->priority < best->priority ||
            (thread->priority == best->priority && thread->ready_sequence < best->ready_sequence))
            best = thread.get();
    }
    return best;
}

void Kernel::save_current(const AllegrexContext &ctx, ThreadStatus status) {
    Thread *thread = current_thread();
    if (thread == nullptr) return;
    thread->context = ctx;
    if (status == ThreadStatus::Ready) make_ready(*thread);
    else thread->status = status;
}

void Kernel::switch_to(AllegrexContext &ctx, Thread &thread) {
    thread.status = ThreadStatus::Running;
    current_uid_ = thread.uid;
    ctx = thread.context;
    psprecomp::set_runtime_thread_identity(thread.uid, thread.name);
    load_trace::run_as(thread.name);
    idle_vblanks_ = 0u;
}

void Kernel::finish(AllegrexContext &ctx, std::uint32_t result) {
    ctx.set_gpr(2, result);
    if (interrupt_active_ || current_thread() == nullptr) return;

    if (interrupts_enabled_ && !pending_interrupts_.empty()) {
        interrupted_context_ = ctx;
        interrupted_context_.pc = ctx.gpr[31];
        interrupted_idle_ = false;
        (void)begin_pending_interrupt(ctx);
        return;
    }
    if (!dispatch_enabled_) return;
    Thread *best = best_ready_thread();
    Thread *current = current_thread();
    if (best != nullptr && best->priority < current->priority) {
        AllegrexContext saved = ctx;
        saved.pc = ctx.gpr[31];
        save_current(saved, ThreadStatus::Ready);
        switch_to(ctx, *best);
    }
}

void Kernel::finish64(AllegrexContext &ctx, std::uint64_t result) {
    ctx.set_gpr(3, static_cast<std::uint32_t>(result >> 32u));
    finish(ctx, static_cast<std::uint32_t>(result));
}

void Kernel::block(AllegrexContext &ctx, const WaitState &wait, std::uint32_t result) {
    Thread *thread = current_thread();
    if (interrupt_active_ || thread == nullptr) {
        ctx.set_gpr(2, error::kCanNotWait);
        return;
    }
    thread->wait = wait;
    if (wait.timeout_address != 0u)
        thread->wait.deadline_us = now_us_ + runtime_->memory().load32(wait.timeout_address);
    AllegrexContext saved = ctx;
    saved.set_gpr(2, result);
    saved.pc = ctx.gpr[31];
    save_current(saved, ThreadStatus::Waiting);
    if (trace_enabled())
        std::cerr << "[kernel] block uid=" << thread->uid << " name=" << thread->name
                  << " wait=" << wait_name(wait.type) << " object=" << wait.object << "\n";
    current_uid_ = 0;
    schedule(ctx);
}

void Kernel::wake(Thread &thread, std::uint32_t result) {
    if (thread.wait.timeout_address != 0u && thread.wait.deadline_us) {
        const std::uint64_t remaining = *thread.wait.deadline_us > now_us_ ? *thread.wait.deadline_us - now_us_ : 0u;
        runtime_->memory().store32(thread.wait.timeout_address, static_cast<std::uint32_t>(remaining));
    }
    thread.context.set_gpr(2, result);
    make_ready(thread);
}

void Kernel::delay_current(AllegrexContext &ctx, std::uint64_t microseconds, std::uint32_t result) {
    WaitState wait{};
    wait.type = WaitType::Delay;
    wait.deadline_us = now_us_ + std::max<std::uint64_t>(microseconds, 1u);
    block(ctx, wait, result);
}

void Kernel::wait_host(AllegrexContext &ctx, std::optional<std::uint64_t> timeout_us, HostWaitPoll poll) {
    // Already satisfied: no thread switch.
    if (const auto result = poll(false)) {
        finish(ctx, *result);
        return;
    }
    if (timeout_us && *timeout_us == 0u) {
        finish(ctx, *poll(true));
        return;
    }
    WaitState wait{};
    wait.type = WaitType::Host;
    if (timeout_us) wait.deadline_us = now_us_ + *timeout_us;
    wait.host_poll = std::move(poll);
    // The poll result replaces v0 when the wait ends.
    block(ctx, wait, 0u);
}

void Kernel::schedule(AllegrexContext &ctx) {
    if (poll_hook_) poll_hook_();
    load_trace::run_as("idle");
    for (;;) {
        if (runtime_->stopped()) return;
        process_timers();
        if (interrupts_enabled_ && !pending_interrupts_.empty()) {
            interrupted_idle_ = true;
            if (begin_pending_interrupt(ctx)) return;
        }
        if (Thread *best = best_ready_thread()) {
            switch_to(ctx, *best);
            return;
        }

        const auto next = next_event_us();
        if (!next || idle_vblanks_ > kIdleVBlankLimit) {
            std::string report = "PSP scheduler deadlock: no runnable thread";
            for (const auto &[uid, thread] : threads_) {
                report += "\n  uid=" + std::to_string(uid) + " name=" + thread->name + " status=" +
                          status_name(thread->status);
                if (thread->status == ThreadStatus::Waiting)
                    report += std::string(" wait=") + wait_name(thread->wait.type) +
                              " object=" + std::to_string(thread->wait.object) +
                              " resume=" + psprecomp::hex32(thread->context.pc);
            }
            psprecomp::set_runtime_thread_identity(kIdleIdentity, "idle");
            ctx.pc = kIdleStub;
            runtime_->stop(report);
            return;
        }
        advance_clock(*next);
    }
}

void Kernel::advance_clock(std::uint64_t target_us) {
    if (target_us <= now_us_) return;
    const std::uint64_t step_us = target_us - now_us_;
    now_us_ = target_us;
    if (pace_to_real_time()) return;
    // Unpaced, idle time costs nothing, so a thread waiting for the network
    // would see its PSP timeout expire long before a reply could arrive. Let
    // real time pass with the emulated time the wait covers.
    const bool host_wait = std::any_of(threads_.begin(), threads_.end(), [](const auto &entry) {
        return entry.second->status == ThreadStatus::Waiting && entry.second->wait.type == WaitType::Host;
    });
    if (host_wait) std::this_thread::sleep_for(std::chrono::microseconds(std::min(step_us, kHostWaitPollUs)));
}

bool Kernel::pace_to_real_time() {
    // Virtual time jumps to the next event whenever every thread waits, so
    // without this the game runs as fast as frames can be presented: two to
    // three times PSP speed on a 60-90 Hz display. Runs without a window, and
    // the unthrottled setting (MHP3RD_UNTHROTTLED), keep the unpaced clock.
    static const bool windowed = std::getenv("MHP3RD_NO_RENDER") == nullptr;
    if (!windowed || settings::current().unthrottled) {
        // Turning pacing back on starts from the current moment.
        pacing_started_ = false;
        return false;
    }
    // While the game loads, emulated time may run up to kMaxSpeed times as
    // fast as real time (kernel/fast_loading.hpp). Either change starts the
    // hold over from the current moment, so a load that ran fast is not made
    // up for afterwards, and normal time is not rushed to catch up with it.
    const bool fast = fast_loading::active();
    if (fast != pacing_fast_) {
        pacing_fast_ = fast;
        pacing_started_ = false;
    }
    using Clock = std::chrono::steady_clock;
    const Clock::time_point now = Clock::now();
    if (!pacing_started_) {
        pacing_started_ = true;
        pacing_real_base_ = now;
        pacing_virtual_base_ = now_us_;
        return true;
    }
    const std::int64_t real_us =
        std::chrono::duration_cast<std::chrono::microseconds>(now - pacing_real_base_).count();
    const std::int64_t virtual_us = static_cast<std::int64_t>(now_us_ - pacing_virtual_base_);
    const std::int64_t ahead_us =
        fast ? static_cast<std::int64_t>(static_cast<double>(virtual_us) / fast_loading::kMaxSpeed) - real_us
             : virtual_us - real_us;
    // Ahead of real time: wait. More than a tenth of a second behind (a slow
    // frame, a load): drop the debt instead of racing to make it up.
    constexpr std::int64_t kMinSleepUs = 1000;
    constexpr std::int64_t kMaxSleepUs = 100000;
    constexpr std::int64_t kMaxLagUs = 100000;
    if (ahead_us >= kMinSleepUs) {
        const Clock::time_point wake = now + std::chrono::microseconds(std::min(ahead_us, kMaxSleepUs));
        Clock::time_point sleep_start = now;
        if (idle_hook_) {
            idle_hook_(wake);
            sleep_start = Clock::now();
        }
        if (sleep_start < wake) std::this_thread::sleep_until(wake);
        perf::add_pacing_time(Clock::now() - sleep_start);
        load_trace::note_pacing_sleep(std::chrono::duration<double, std::milli>(Clock::now() - now).count());
    } else if (ahead_us < -kMaxLagUs) {
        pacing_real_base_ = now;
        pacing_virtual_base_ = now_us_;
    }
    return true;
}

std::optional<std::uint64_t> Kernel::next_event_us() const {
    std::optional<std::uint64_t> next = next_vblank_us_;
    const auto consider = [&](std::uint64_t value) {
        if (!next || value < *next) next = value;
    };
    for (const auto &[uid, thread] : threads_) {
        (void)uid;
        if (thread->status != ThreadStatus::Waiting) continue;
        if (thread->wait.deadline_us) consider(*thread->wait.deadline_us);
        if (thread->wait.type == WaitType::Host) consider(now_us_ + kHostWaitPollUs);
    }
    for (const auto &[uid, timer] : vtimers) {
        (void)uid;
        if (timer.active && timer.handler != 0u && timer.schedule_us >= vtimer_value(timer))
            consider(now_us_ + (timer.schedule_us - vtimer_value(timer)));
    }
    return next;
}

void Kernel::process_timers() {
    while (now_us_ >= next_vblank_us_) {
        next_vblank_us_ += kVBlankPeriodUs;
        if (current_uid_ == 0) ++idle_vblanks_;
        on_vblank();
    }
    for (auto &[uid, thread] : threads_) {
        (void)uid;
        if (thread->status != ThreadStatus::Waiting) continue;
        if (thread->wait.type == WaitType::Host) {
            const bool timed_out = thread->wait.deadline_us && *thread->wait.deadline_us <= now_us_;
            std::optional<std::uint32_t> result = thread->wait.host_poll(timed_out);
            if (timed_out && !result) result = error::kWaitTimeout;
            if (result) {
                thread->wait.host_poll = nullptr;
                thread->context.set_gpr(2, *result);
                make_ready(*thread);
            }
            continue;
        }
        if (thread->wait.deadline_us && *thread->wait.deadline_us <= now_us_) finish_wait_timeout(*thread);
    }
    for (auto &[uid, timer] : vtimers) {
        if (!timer.active || timer.handler == 0u || vtimer_value(timer) < timer.schedule_us) continue;
        const SceUID timer_uid = uid;
        auto &memory = runtime_->memory();
        const std::uint64_t schedule = timer.schedule_us;
        const std::uint64_t current = vtimer_value(timer);
        memory.store32(kVTimerClockScratch, static_cast<std::uint32_t>(schedule));
        memory.store32(kVTimerClockScratch + 4u, static_cast<std::uint32_t>(schedule >> 32u));
        memory.store32(kVTimerClockScratch + 8u, static_cast<std::uint32_t>(current));
        memory.store32(kVTimerClockScratch + 12u, static_cast<std::uint32_t>(current >> 32u));
        InterruptCall call{};
        call.function = timer.handler;
        call.arguments = {static_cast<std::uint32_t>(timer_uid), kVTimerClockScratch, kVTimerClockScratch + 8u,
                          timer.common};
        // The handler returns the delay until the next call, measured from the
        // schedule it fired at; 0 stops it.
        call.on_return = [timer_uid, schedule](std::uint32_t next) {
            auto found = kernel().vtimers.find(timer_uid);
            if (found == kernel().vtimers.end()) return;
            if (next == 0u) {
                found->second.handler = 0u;
                return;
            }
            found->second.schedule_us = schedule + next;
        };
        // Disarm until the handler returns, so it cannot fire again meanwhile.
        timer.schedule_us = UINT64_MAX;
        queue_interrupt(std::move(call));
    }
}

void Kernel::finish_wait_timeout(Thread &thread) {
    if (thread.wait.type == WaitType::Delay) {
        make_ready(thread);
        return;
    }
    remove_waiter(thread);
    if (thread.wait.timeout_address != 0u) runtime_->memory().store32(thread.wait.timeout_address, 0u);
    thread.context.set_gpr(2, error::kWaitTimeout);
    make_ready(thread);
}

void Kernel::remove_waiter(Thread &thread) {
    const auto erase = [&](std::deque<SceUID> &waiters) {
        waiters.erase(std::remove(waiters.begin(), waiters.end(), thread.uid), waiters.end());
    };
    switch (thread.wait.type) {
    case WaitType::Semaphore:
        if (auto found = semaphores.find(thread.wait.object); found != semaphores.end()) erase(found->second.waiters);
        break;
    case WaitType::EventFlag:
        if (auto found = event_flags.find(thread.wait.object); found != event_flags.end()) erase(found->second.waiters);
        break;
    case WaitType::Mutex:
        if (auto found = mutexes.find(thread.wait.object); found != mutexes.end()) erase(found->second.waiters);
        break;
    default:
        break;
    }
}

std::string Kernel::describe_threads() const {
    std::string report = "threads (virtual time " + std::to_string(now_us_ / 1000u) + " ms, vblanks " +
                         std::to_string(vblank_count_) + "):";
    for (const auto &[uid, thread] : threads_) {
        report += "\n  uid=" + std::to_string(uid) + " " + thread->name + " prio=" + psprecomp::hex32(thread->priority) +
                  " " + status_name(thread->status);
        if (thread->status == ThreadStatus::Waiting) {
            report += std::string(" wait=") + wait_name(thread->wait.type) + " object=" + std::to_string(thread->wait.object);
            if (thread->wait.type == WaitType::EventFlag) {
                report += " bits=" + psprecomp::hex32(thread->wait.value) + " mode=" + psprecomp::hex32(thread->wait.mode);
                const auto flag = event_flags.find(thread->wait.object);
                if (flag != event_flags.end()) report += " pattern=" + psprecomp::hex32(flag->second.pattern);
            }
            if (thread->wait.type == WaitType::Semaphore) {
                const auto sema = semaphores.find(thread->wait.object);
                report += " want=" + std::to_string(thread->wait.value);
                if (sema != semaphores.end())
                    report += " count=" + std::to_string(sema->second.count) + " name=" + sema->second.name;
            }
            if (thread->wait.deadline_us) report += " deadline=" + std::to_string(*thread->wait.deadline_us / 1000u) + "ms";
        }
        if (thread->status != ThreadStatus::Dormant) report += " pc=" + psprecomp::hex32(thread->context.pc);
    }
    return report;
}

void Kernel::on_starvation(AllegrexContext &ctx) {
    if (interrupt_active_ || current_thread() == nullptr) return;
    advance_clock(now_us_ + kStarvationQuantumUs);
    process_timers();
    if (interrupts_enabled_ && !pending_interrupts_.empty()) {
        interrupted_context_ = ctx;
        interrupted_idle_ = false;
        (void)begin_pending_interrupt(ctx);
        return;
    }
    if (!dispatch_enabled_) return;
    Thread *best = best_ready_thread();
    if (best != nullptr && best->priority <= current_thread()->priority) {
        save_current(ctx, ThreadStatus::Ready);
        switch_to(ctx, *best);
    }
}

// ---------------------------------------------------------------------------
// Synchronization helpers

void Kernel::cancel_waiters(std::deque<SceUID> &waiters, std::uint32_t result) {
    for (const SceUID uid : waiters) {
        if (Thread *thread = find_thread(uid); thread != nullptr && thread->status == ThreadStatus::Waiting)
            wake(*thread, result);
    }
    waiters.clear();
}

void Kernel::release_semaphore_waiters(SceUID uid) {
    auto found = semaphores.find(uid);
    if (found == semaphores.end()) return;
    Semaphore &sema = found->second;
    while (!sema.waiters.empty()) {
        Thread *thread = find_thread(sema.waiters.front());
        if (thread == nullptr || thread->status != ThreadStatus::Waiting) {
            sema.waiters.pop_front();
            continue;
        }
        const auto requested = static_cast<std::int32_t>(thread->wait.value);
        if (requested > sema.count) break;
        sema.count -= requested;
        sema.waiters.pop_front();
        wake(*thread, 0u);
    }
}

bool Kernel::event_flag_matches(std::uint32_t pattern, std::uint32_t bits, std::uint32_t mode) noexcept {
    return (mode & 1u) != 0u ? (pattern & bits) != 0u : (pattern & bits) == bits;
}

void Kernel::release_event_flag_waiters(SceUID uid) {
    auto found = event_flags.find(uid);
    if (found == event_flags.end()) return;
    EventFlag &flag = found->second;
    for (auto it = flag.waiters.begin(); it != flag.waiters.end();) {
        Thread *thread = find_thread(*it);
        if (thread == nullptr || thread->status != ThreadStatus::Waiting) {
            it = flag.waiters.erase(it);
            continue;
        }
        if (!event_flag_matches(flag.pattern, thread->wait.value, thread->wait.mode)) {
            ++it;
            continue;
        }
        if (thread->wait.out_address != 0u) runtime_->memory().store32(thread->wait.out_address, flag.pattern);
        if ((thread->wait.mode & 0x10u) != 0u) flag.pattern = 0u;
        else if ((thread->wait.mode & 0x20u) != 0u) flag.pattern &= ~thread->wait.value;
        it = flag.waiters.erase(it);
        wake(*thread, 0u);
    }
}

void Kernel::release_mutex_waiters(SceUID uid) {
    auto found = mutexes.find(uid);
    if (found == mutexes.end()) return;
    Mutex &mutex = found->second;
    while (mutex.lock_count == 0 && !mutex.waiters.empty()) {
        Thread *thread = find_thread(mutex.waiters.front());
        mutex.waiters.pop_front();
        if (thread == nullptr || thread->status != ThreadStatus::Waiting) continue;
        mutex.owner = thread->uid;
        mutex.lock_count = static_cast<std::int32_t>(thread->wait.value);
        wake(*thread, 0u);
    }
}

std::uint64_t Kernel::vtimer_value(const VTimer &timer) const noexcept {
    return timer.active ? timer.accumulated_us + (now_us_ - timer.base_us) : timer.accumulated_us;
}

// ---------------------------------------------------------------------------
// Memory

std::int32_t Kernel::allocate_block(const std::string &name, std::uint32_t type, std::uint32_t size,
                                    std::uint32_t address_or_alignment) {
    if (size == 0u) return static_cast<std::int32_t>(error::kIllegalArgument);
    std::uint32_t alignment = kMemoryGranularity;
    if ((type == 3u || type == 4u) && address_or_alignment != 0u) {
        if ((address_or_alignment & (address_or_alignment - 1u)) != 0u)
            return static_cast<std::int32_t>(error::kIllegalArgument);
        alignment = std::max(alignment, address_or_alignment);
    }
    const std::uint32_t aligned_size = align_up(size, kMemoryGranularity);

    std::optional<std::uint32_t> address;
    if (type == 0u || type == 3u) {
        for (const FreeRange &range : free_ranges_) {
            const std::uint32_t start = align_up(range.address, alignment);
            if (start + aligned_size <= range.address + range.size) {
                address = start;
                break;
            }
        }
    } else if (type == 1u || type == 4u) {
        for (auto it = free_ranges_.rbegin(); it != free_ranges_.rend(); ++it) {
            const std::uint64_t end = static_cast<std::uint64_t>(it->address) + it->size;
            if (end < aligned_size) continue;
            const std::uint32_t start = static_cast<std::uint32_t>(end - aligned_size) & ~(alignment - 1u);
            if (start >= it->address) {
                address = start;
                break;
            }
        }
    } else if (type == 2u) {
        const std::uint32_t start = address_or_alignment & ~(kMemoryGranularity - 1u);
        for (const FreeRange &range : free_ranges_) {
            if (start >= range.address && start + aligned_size <= range.address + range.size) {
                address = start;
                break;
            }
        }
    } else {
        return static_cast<std::int32_t>(error::kIllegalMemblockType);
    }
    if (!address) return static_cast<std::int32_t>(error::kNoMemory);

    // Carve [address, address + aligned_size) out of its free range.
    for (auto it = free_ranges_.begin(); it != free_ranges_.end(); ++it) {
        if (*address < it->address || *address + aligned_size > it->address + it->size) continue;
        const FreeRange before{it->address, *address - it->address};
        const FreeRange after{*address + aligned_size, it->address + it->size - (*address + aligned_size)};
        it = free_ranges_.erase(it);
        if (after.size != 0u) it = free_ranges_.insert(it, after);
        if (before.size != 0u) free_ranges_.insert(it, before);
        break;
    }
    const SceUID uid = allocate_uid();
    blocks_.emplace(uid, MemoryBlock{name, *address, aligned_size});
    if (trace_enabled())
        std::cerr << "[kernel] alloc " << name << " size=" << psprecomp::hex32(size)
                  << " -> " << psprecomp::hex32(*address) << " uid=" << uid << "\n";
    return uid;
}

std::int32_t Kernel::free_block(SceUID uid) {
    const auto found = blocks_.find(uid);
    if (found == blocks_.end()) return static_cast<std::int32_t>(error::kIllegalMemblock);
    FreeRange freed{found->second.address, found->second.size};
    blocks_.erase(found);
    auto position = std::lower_bound(free_ranges_.begin(), free_ranges_.end(), freed,
                                     [](const FreeRange &a, const FreeRange &b) { return a.address < b.address; });
    position = free_ranges_.insert(position, freed);
    // Coalesce with neighbours.
    if (position + 1 != free_ranges_.end() && position->address + position->size == (position + 1)->address) {
        position->size += (position + 1)->size;
        free_ranges_.erase(position + 1);
    }
    if (position != free_ranges_.begin() && (position - 1)->address + (position - 1)->size == position->address) {
        (position - 1)->size += position->size;
        free_ranges_.erase(position);
    }
    return 0;
}

const MemoryBlock *Kernel::find_block(SceUID uid) const {
    const auto found = blocks_.find(uid);
    return found != blocks_.end() ? &found->second : nullptr;
}

std::uint32_t Kernel::free_memory() const noexcept {
    std::uint32_t total = 0u;
    for (const FreeRange &range : free_ranges_) total += range.size;
    return total;
}

// ---------------------------------------------------------------------------
// Interrupts

void Kernel::queue_interrupt(InterruptCall call) { pending_interrupts_.push_back(std::move(call)); }

void Kernel::notify_callback(SceUID callback, std::uint32_t argument) {
    const auto found = callbacks.find(callback);
    if (found == callbacks.end()) return;
    ++found->second.notify_count;
    found->second.notify_argument = argument;
    found->second.pending = true;
    if (trace_enabled())
        std::cerr << "[kernel] notify callback " << callback << " " << found->second.name
                  << " arg=" << psprecomp::hex32(argument) << "\n";
}

bool Kernel::deliver_callbacks() {
    Thread *thread = current_thread();
    if (thread == nullptr || interrupt_active_) return false;
    bool queued = false;
    for (auto &[uid, callback] : callbacks) {
        if (!callback.pending || callback.owner != thread->uid || callback.function == 0u) continue;
        callback.pending = false;
        InterruptCall call{};
        call.function = callback.function;
        // PSP callbacks receive (notify count, notify argument, common pointer).
        call.arguments = {callback.notify_count, callback.notify_argument, callback.argument, 0u};
        callback.notify_count = 0u;
        queue_interrupt(std::move(call));
        queued = true;
        if (trace_enabled()) std::cerr << "[kernel] deliver callback " << uid << " " << callback.name << "\n";
    }
    return queued;
}

void Kernel::on_vblank() {
    for (const auto &hook : vblank_hooks_) hook();
    load_trace::tick(now_us_);
    fast_loading::update();
    ++vblank_count_;
    for (auto &[uid, thread] : threads_) {
        (void)uid;
        if (thread->status == ThreadStatus::Waiting && thread->wait.type == WaitType::VBlank)
            wake(*thread, thread->context.gpr[2]);
    }
    const auto handlers = sub_interrupts.find(kVBlankInterrupt);
    if (handlers == sub_interrupts.end()) return;
    for (const auto &[sub, handler] : handlers->second) {
        if (!handler.enabled || handler.handler == 0u) continue;
        InterruptCall call{};
        call.function = handler.handler;
        call.arguments = {sub, handler.argument, 0u, 0u};
        queue_interrupt(std::move(call));
    }
}

bool Kernel::begin_pending_interrupt(AllegrexContext &ctx) {
    if (pending_interrupts_.empty()) return false;
    InterruptCall call = std::move(pending_interrupts_.front());
    pending_interrupts_.pop_front();
    interrupt_active_ = true;
    interrupt_on_return_ = std::move(call.on_return);
    ctx = pristine_context_;
    for (std::uint32_t i = 0; i < 4u; ++i) ctx.set_gpr(4u + i, call.arguments[i]);
    ctx.set_gpr(29, kInterruptStackTop - kThreadArgumentHome);
    ctx.set_gpr(31, kInterruptReturnStub);
    ctx.pc = call.function;
    psprecomp::set_runtime_thread_identity(kInterruptIdentity, "interrupt");
    load_trace::run_as("interrupt");
    return true;
}

void Kernel::interrupt_return_stub(AllegrexContext &ctx) {
    const std::uint32_t result = ctx.gpr[2];
    interrupt_active_ = false;
    if (interrupt_on_return_) {
        auto on_return = std::move(interrupt_on_return_);
        interrupt_on_return_ = nullptr;
        on_return(result);
    }
    if (interrupts_enabled_ && begin_pending_interrupt(ctx)) return;

    if (interrupted_idle_ || current_thread() == nullptr) {
        interrupted_idle_ = false;
        current_uid_ = 0;
        schedule(ctx);
        return;
    }
    ctx = interrupted_context_;
    Thread *current = current_thread();
    psprecomp::set_runtime_thread_identity(current->uid, current->name);
    load_trace::run_as(current->name);
    if (!dispatch_enabled_) return;
    Thread *best = best_ready_thread();
    if (best != nullptr && best->priority < current->priority) {
        save_current(ctx, ThreadStatus::Ready);
        switch_to(ctx, *best);
    }
}

void Kernel::thread_exit_stub(AllegrexContext &ctx) {
    exit_current_thread(ctx, static_cast<std::int32_t>(ctx.gpr[2]), current_thread() != nullptr &&
                                                                         current_thread()->name == "module_start");
}

void Kernel::call_guest(AllegrexContext &ctx, std::uint32_t function, const std::array<std::uint32_t, 4> &arguments,
                        GuestCallReturn on_return) {
    guest_calls_[current_uid_].push_back(GuestCall{ctx.gpr[31], std::move(on_return)});
    for (std::uint32_t i = 0; i < 4u; ++i) ctx.set_gpr(4u + i, arguments[i]);
    ctx.set_gpr(31, kGuestCallReturnStub);
    // A pc other than the import's own tells the import wrapper not to return.
    ctx.pc = function;
}

void Kernel::guest_call_return_stub(AllegrexContext &ctx) {
    auto found = guest_calls_.find(current_uid_);
    if (found == guest_calls_.end() || found->second.empty()) {
        runtime_->stop("guest call returned in a thread that made none");
        return;
    }
    GuestCall call = std::move(found->second.back());
    found->second.pop_back();
    if (found->second.empty()) guest_calls_.erase(found);
    ctx.set_gpr(31, call.return_address);
    ctx.pc = call.return_address;
    call.on_return(ctx, ctx.gpr[2]);
}

void Kernel::idle_stub(AllegrexContext &ctx) {
    current_uid_ = 0;
    schedule(ctx);
}

} // namespace mhp3rd
