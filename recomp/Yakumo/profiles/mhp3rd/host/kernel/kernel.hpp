#pragma once

#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/runtime.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Minimal PSP kernel for the MHP3rd profile: threads with a deterministic
// virtual clock, synchronization objects, partition memory, and guest interrupt
// delivery (VBlank sub-interrupts, VTimers, GE callbacks).
//
// Execution model. Every HLE import runs at the outer runtime dispatch level,
// with `ctx` aliasing Runtime::cpu(). An import that blocks or preempts saves
// the caller's context with pc = $ra and v0 = result, loads another thread's
// context into `ctx`, and changes the runtime thread identity. The generated
// import wrapper sees the identity change and leaves `ctx.pc` alone, so the
// dispatcher continues in the new thread.
namespace mhp3rd {

using psprecomp::AllegrexContext;
using psprecomp::Runtime;
using SceUID = std::int32_t;

namespace error {
inline constexpr std::uint32_t kIllegalArgument = 0x800200D2u;
inline constexpr std::uint32_t kIllegalMemblockType = 0x800200D8u;
inline constexpr std::uint32_t kUnknownUid = 0x800200CBu;
inline constexpr std::uint32_t kNoMemory = 0x80020190u;
inline constexpr std::uint32_t kIllegalEntry = 0x80020192u;
inline constexpr std::uint32_t kIllegalPriority = 0x80020193u;
inline constexpr std::uint32_t kIllegalStackSize = 0x80020194u;
inline constexpr std::uint32_t kIllegalMode = 0x80020195u;
inline constexpr std::uint32_t kIllegalThid = 0x80020197u;
inline constexpr std::uint32_t kUnknownThid = 0x80020198u;
inline constexpr std::uint32_t kUnknownSemid = 0x80020199u;
inline constexpr std::uint32_t kUnknownEvfid = 0x8002019Au;
inline constexpr std::uint32_t kUnknownCbid = 0x800201A1u;
inline constexpr std::uint32_t kDormant = 0x800201A2u;
inline constexpr std::uint32_t kNotDormant = 0x800201A4u;
inline constexpr std::uint32_t kCanNotWait = 0x800201A7u;
inline constexpr std::uint32_t kWaitTimeout = 0x800201A8u;
inline constexpr std::uint32_t kSemaZero = 0x800201ADu;
inline constexpr std::uint32_t kSemaOverflow = 0x800201AEu;
inline constexpr std::uint32_t kEvfCond = 0x800201AFu;
inline constexpr std::uint32_t kEvfMulti = 0x800201B0u;
inline constexpr std::uint32_t kEvfIllegalPattern = 0x800201B1u;
inline constexpr std::uint32_t kWaitDelete = 0x800201B5u;
inline constexpr std::uint32_t kIllegalMemblock = 0x800201B6u;
inline constexpr std::uint32_t kIllegalCount = 0x800201BDu;
inline constexpr std::uint32_t kUnknownVtid = 0x800201BEu;
inline constexpr std::uint32_t kMutexNotFound = 0x800201C3u;
inline constexpr std::uint32_t kMutexLocked = 0x800201C4u;
inline constexpr std::uint32_t kMutexUnlocked = 0x800201C5u;
inline constexpr std::uint32_t kMutexLockOverflow = 0x800201C6u;
inline constexpr std::uint32_t kMutexUnlockUnderflow = 0x800201C7u;
inline constexpr std::uint32_t kMutexRecursiveNotAllowed = 0x800201C8u;
} // namespace error

// Guest addresses reserved by the host. 0x08800000..0x08804000 lies in user
// RAM below the load image, inside the runtime's direct dispatch window.
inline constexpr std::uint32_t kThreadExitStub = 0x08800000u;
inline constexpr std::uint32_t kInterruptReturnStub = 0x08800004u;
inline constexpr std::uint32_t kIdleStub = 0x08800008u;
inline constexpr std::uint32_t kGuestCallReturnStub = 0x0880000Cu;
// Kernel partition scratch used by the host (boot arguments, interrupt stack).
inline constexpr std::uint32_t kBootArgumentAddress = 0x08100000u;
inline constexpr std::uint32_t kInterruptStackTop = 0x08200000u;
inline constexpr std::uint32_t kVolatileMemoryBase = 0x08400000u;
inline constexpr std::uint32_t kVolatileMemorySize = 0x00400000u;
inline constexpr std::uint32_t kUserMemoryEnd = 0x0C000000u;

inline constexpr std::uint64_t kVBlankPeriodUs = 16'683u;
inline constexpr std::uint64_t kHostWaitPollUs = 1'000u;
inline constexpr std::uint32_t kVBlankInterrupt = 30u;

enum class ThreadStatus { Dormant, Ready, Running, Waiting, Dead };

enum class WaitType {
    None,
    Delay,
    Sleep,
    Semaphore,
    EventFlag,
    Mutex,
    VBlank,
    ThreadEnd,
    Host,  // a condition only the host can check, such as network data arriving
};

// Checks a host wait. Returns v0 for the thread once the wait is over, or
// nothing to keep waiting. With `timed_out` set the deadline has passed and it
// must return a value.
using HostWaitPoll = std::function<std::optional<std::uint32_t>(bool timed_out)>;

struct WaitState {
    WaitType type{WaitType::None};
    SceUID object{};
    std::uint32_t value{};        // requested count / bit pattern
    std::uint32_t mode{};         // event flag wait mode
    std::uint32_t out_address{};  // event flag result pattern pointer
    std::uint32_t timeout_address{};
    std::optional<std::uint64_t> deadline_us;
    HostWaitPoll host_poll;       // WaitType::Host
};

struct Thread {
    SceUID uid{};
    std::string name;
    std::uint32_t entry{};
    std::uint32_t priority{};
    std::uint32_t initial_priority{};
    std::uint32_t attributes{};
    std::uint32_t stack_size{};
    std::uint32_t stack_bottom{};
    std::uint32_t control_block{};  // $k0 block, 256 bytes at the stack top
    std::uint32_t gp{};
    SceUID stack_block{};
    ThreadStatus status{ThreadStatus::Dormant};
    AllegrexContext context{};
    WaitState wait{};
    std::int32_t exit_status{};
    std::uint32_t wakeup_count{};
    std::uint64_t ready_sequence{};
};

struct Semaphore {
    std::string name;
    std::uint32_t attributes{};
    std::int32_t count{};
    std::int32_t max_count{};
    std::deque<SceUID> waiters;
};

struct EventFlag {
    std::string name;
    std::uint32_t attributes{};
    std::uint32_t pattern{};
    std::deque<SceUID> waiters;
};

struct Mutex {
    std::string name;
    std::uint32_t attributes{};
    SceUID owner{};
    std::int32_t lock_count{};
    std::deque<SceUID> waiters;
};

struct Callback {
    std::string name;
    std::uint32_t function{};
    std::uint32_t argument{};
    SceUID owner{};
    std::uint32_t notify_count{};
    std::uint32_t notify_argument{};
    bool pending{};
};

struct VTimer {
    std::string name;
    bool active{};
    std::uint64_t base_us{};        // virtual clock value when started
    std::uint64_t accumulated_us{}; // timer value while stopped
    std::uint64_t schedule_us{};    // timer value at which the handler fires
    std::uint32_t handler{};
    std::uint32_t common{};
};

struct MemoryBlock {
    std::string name;
    std::uint32_t address{};
    std::uint32_t size{};
};

struct SubInterruptHandler {
    std::uint32_t handler{};
    std::uint32_t argument{};
    bool enabled{};
};

// A guest function queued for execution in interrupt context.
struct InterruptCall {
    std::uint32_t function{};
    std::array<std::uint32_t, 4> arguments{};
    // Invoked on the host after the guest handler returns, with its v0.
    std::function<void(std::uint32_t)> on_return;
};

class Kernel {
public:
    void install(Runtime &runtime, std::uint32_t gp, std::uint32_t image_end);
    [[nodiscard]] Runtime &runtime() noexcept { return *runtime_; }

    // Clock ---------------------------------------------------------------
    [[nodiscard]] std::uint64_t now_us() const noexcept { return now_us_; }
    [[nodiscard]] std::uint64_t vblank_count() const noexcept { return vblank_count_; }
    // Emulated time of the latest vblank, at or before now_us().
    [[nodiscard]] std::uint64_t last_vblank_us() const noexcept { return next_vblank_us_ - kVBlankPeriodUs; }
    // Forgets how far emulated time was ahead of or behind real time. Called
    // after the game was paused, so it resumes at normal speed rather than
    // racing to make up the pause.
    void resync_real_time() noexcept { pacing_started_ = false; }
    // The moment of real time the hold to real time maps `virtual_us` of
    // emulated time to; empty while emulated time is not held to real time,
    // a load running fast included (kernel/fast_loading.hpp).
    // Frame interpolation times its presents by it (gpu/frame_pacing.hpp).
    [[nodiscard]] std::optional<std::chrono::steady_clock::time_point> real_time_of(
        std::uint64_t virtual_us) const noexcept {
        if (!pacing_started_ || pacing_fast_) return std::nullopt;
        return pacing_real_base_ + std::chrono::microseconds(static_cast<std::int64_t>(virtual_us) -
                                                             static_cast<std::int64_t>(pacing_virtual_base_));
    }
    // Frame interpolation presents between the game's flips from two places.
    // The idle hook runs while the kernel waits for real time to catch up
    // with emulated time, with the moment it will wake; it presents what falls
    // due before then and accounts for its own time. The poll hook runs each
    // time the scheduler looks for a thread, while the game's code runs, and
    // presents one that is already due.
    using IdleHook = std::function<void(std::chrono::steady_clock::time_point wake)>;
    using PollHook = std::function<void()>;
    void set_idle_hook(IdleHook hook) { idle_hook_ = std::move(hook); }
    void set_poll_hook(PollHook hook) { poll_hook_ = std::move(hook); }

    // Threads -------------------------------------------------------------
    [[nodiscard]] SceUID allocate_uid() noexcept { return next_uid_++; }
    [[nodiscard]] Thread *find_thread(SceUID uid) noexcept;
    [[nodiscard]] Thread *current_thread() noexcept { return find_thread(current_uid_); }
    [[nodiscard]] SceUID current_uid() const noexcept { return current_uid_; }
    // Creates the thread that runs module_start and makes it current.
    void start_loader_thread(AllegrexContext &ctx, std::uint32_t entry, std::uint32_t stack_top);
    std::int32_t create_thread(const std::string &name, std::uint32_t entry, std::uint32_t priority,
                               std::uint32_t stack_size, std::uint32_t attributes, std::uint32_t gp);
    std::int32_t start_thread(AllegrexContext &ctx, SceUID uid, std::uint32_t argument_size,
                              std::uint32_t argument_address);
    void exit_current_thread(AllegrexContext &ctx, std::int32_t status, bool delete_thread);
    std::int32_t terminate_thread(AllegrexContext &ctx, SceUID uid, bool delete_thread);
    std::int32_t delete_thread(SceUID uid);
    std::int32_t change_priority(AllegrexContext &ctx, SceUID uid, std::uint32_t priority);

    // HLE completion ------------------------------------------------------
    // Sets v0 and gives a higher-priority ready thread the CPU if one exists.
    void finish(AllegrexContext &ctx, std::uint32_t result);
    void finish64(AllegrexContext &ctx, std::uint64_t result);
    // Blocks the current thread. `result` is what v0 holds if the wait ends
    // normally without an explicit wake result.
    void block(AllegrexContext &ctx, const WaitState &wait, std::uint32_t result = 0u);
    // Makes a waiting thread ready with v0 = result.
    void wake(Thread &thread, std::uint32_t result);
    void delay_current(AllegrexContext &ctx, std::uint64_t microseconds, std::uint32_t result = 0u);
    // Blocks the current thread until `poll` returns a value, which becomes
    // its v0. `poll` runs on the emulation thread every time the scheduler
    // looks for work, and at least every kHostWaitPollUs of emulated time,
    // which the kernel holds to real time. `timeout_us` (emulated time; none
    // waits indefinitely) makes the last poll a timed-out one.
    void wait_host(AllegrexContext &ctx, std::optional<std::uint64_t> timeout_us, HostWaitPoll poll);
    // Calls a guest function from an HLE import, in the calling thread, the
    // way a library calls back into the game: the function runs like any
    // other guest code, so it may block, and when it returns `on_return` gets
    // its v0 with `ctx` back at the import's return address. `on_return` then
    // either finishes the import or makes another call. Use it instead of
    // finish(), as the last thing the import does.
    using GuestCallReturn = std::function<void(AllegrexContext &, std::uint32_t)>;
    void call_guest(AllegrexContext &ctx, std::uint32_t function, const std::array<std::uint32_t, 4> &arguments,
                    GuestCallReturn on_return);
    void set_dispatch_enabled(bool enabled) noexcept { dispatch_enabled_ = enabled; }
    [[nodiscard]] bool dispatch_enabled() const noexcept { return dispatch_enabled_; }

    // Synchronization objects ---------------------------------------------
    std::map<SceUID, Semaphore> semaphores;
    std::map<SceUID, EventFlag> event_flags;
    std::map<SceUID, Mutex> mutexes;
    std::map<SceUID, Callback> callbacks;
    std::map<SceUID, VTimer> vtimers;
    // Re-evaluates the waiters of an object after its state changed.
    void release_semaphore_waiters(SceUID uid);
    void release_event_flag_waiters(SceUID uid);
    void release_mutex_waiters(SceUID uid);
    void cancel_waiters(std::deque<SceUID> &waiters, std::uint32_t result);
    [[nodiscard]] static bool event_flag_matches(std::uint32_t pattern, std::uint32_t bits, std::uint32_t mode) noexcept;
    [[nodiscard]] std::uint64_t vtimer_value(const VTimer &timer) const noexcept;

    // Memory --------------------------------------------------------------
    // type: 0 low, 1 high, 2 at address, 3 low aligned, 4 high aligned.
    std::int32_t allocate_block(const std::string &name, std::uint32_t type, std::uint32_t size,
                                std::uint32_t address_or_alignment);
    std::int32_t free_block(SceUID uid);
    [[nodiscard]] const MemoryBlock *find_block(SceUID uid) const;
    [[nodiscard]] std::uint32_t free_memory() const noexcept;

    // Interrupts ----------------------------------------------------------
    std::map<std::uint32_t, std::map<std::uint32_t, SubInterruptHandler>> sub_interrupts;
    void queue_interrupt(InterruptCall call);
    // Marks a callback as notified; it runs the next time its owner thread
    // performs a callback-aware wait, as on hardware.
    void notify_callback(SceUID callback, std::uint32_t argument);
    // Queues every notified callback owned by the current thread. Returns true
    // if any were queued.
    bool deliver_callbacks();
    [[nodiscard]] bool interrupts_enabled() const noexcept { return interrupts_enabled_; }
    void set_interrupts_enabled(bool enabled) noexcept { interrupts_enabled_ = enabled; }
    [[nodiscard]] bool in_interrupt() const noexcept { return interrupt_active_; }
    // Called on vblank: wakes vblank waiters and queues sub-interrupt handlers.
    void on_vblank();
    // Runs `hook` on the emulation thread at every vblank, for host services
    // that deliver events to the game on their own, such as the network.
    void add_vblank_hook(std::function<void()> hook) { vblank_hooks_.push_back(std::move(hook)); }

    // One line per thread: state, what it waits for and where it resumes.
    [[nodiscard]] std::string describe_threads() const;

    // Periodic hook from the runtime's starvation boundary.
    void on_starvation(AllegrexContext &ctx);

    // Guest entry stubs.
    void thread_exit_stub(AllegrexContext &ctx);
    void interrupt_return_stub(AllegrexContext &ctx);
    void idle_stub(AllegrexContext &ctx);
    void guest_call_return_stub(AllegrexContext &ctx);

private:
    struct FreeRange {
        std::uint32_t address{};
        std::uint32_t size{};
    };

    std::uint32_t prepare_thread_stack(Thread &thread);
    void make_ready(Thread &thread);
    void save_current(const AllegrexContext &ctx, ThreadStatus status);
    void switch_to(AllegrexContext &ctx, Thread &thread);
    // Picks the next thread to run, delivering timers/interrupts and idling
    // the virtual clock forward when nothing is ready.
    void schedule(AllegrexContext &ctx);
    [[nodiscard]] Thread *best_ready_thread() noexcept;
    void advance_clock(std::uint64_t target_us);
    // Holds the virtual clock to real time, so the game runs at PSP speed
    // however fast frames are drawn and presented. False when pacing is off.
    bool pace_to_real_time();
    [[nodiscard]] std::optional<std::uint64_t> next_event_us() const;
    void process_timers();
    bool begin_pending_interrupt(AllegrexContext &ctx);
    void finish_wait_timeout(Thread &thread);
    void remove_waiter(Thread &thread);

    Runtime *runtime_{};
    std::uint32_t gp_{};
    AllegrexContext pristine_context_{};
    std::uint64_t idle_vblanks_{};
    SceUID next_uid_{0x100};
    SceUID current_uid_{};
    std::uint64_t next_ready_sequence_{1u};
    std::map<SceUID, std::unique_ptr<Thread>> threads_;
    std::uint64_t now_us_{};
    bool pacing_started_{};
    bool pacing_fast_{};  // the hold lets emulated time run ahead: a load runs fast
    std::chrono::steady_clock::time_point pacing_real_base_{};
    std::uint64_t pacing_virtual_base_{};
    IdleHook idle_hook_;
    PollHook poll_hook_;
    std::uint64_t next_vblank_us_{kVBlankPeriodUs};
    std::uint64_t vblank_count_{};
    bool dispatch_enabled_{true};
    bool interrupts_enabled_{true};

    std::map<SceUID, MemoryBlock> blocks_;
    std::vector<FreeRange> free_ranges_;

    struct GuestCall {
        std::uint32_t return_address{};
        GuestCallReturn on_return;
    };
    // Calls in progress per thread, innermost last.
    std::map<SceUID, std::vector<GuestCall>> guest_calls_;

    std::vector<std::function<void()>> vblank_hooks_;
    std::deque<InterruptCall> pending_interrupts_;
    bool interrupt_active_{};
    bool interrupted_idle_{};
    AllegrexContext interrupted_context_{};
    std::function<void(std::uint32_t)> interrupt_on_return_;
};

[[nodiscard]] Kernel &kernel();

} // namespace mhp3rd
