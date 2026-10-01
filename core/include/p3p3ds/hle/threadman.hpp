#pragma once

#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/guest_memory.hpp"
#include "psprecomp/runtime.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace p3p3ds {
class KernelState;
}

namespace p3p3ds::hle {

// Kernel error codes: references/uofw/include/common/errors.h.
constexpr std::int32_t kernel_error(std::uint32_t code) noexcept { return static_cast<std::int32_t>(code); }
constexpr std::int32_t SCE_KERNEL_ERROR_ERROR                  = kernel_error(0x80020001u);
constexpr std::int32_t SCE_KERNEL_ERROR_CANNOT_BE_CALLED_FROM_INTERRUPT = kernel_error(0x80020064u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT       = kernel_error(0x800200D2u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_ADDR           = kernel_error(0x800200D3u);
constexpr std::int32_t SCE_KERNEL_ERROR_NO_MEMORY              = kernel_error(0x80020190u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_ATTR           = kernel_error(0x80020191u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_ENTRY          = kernel_error(0x80020192u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_PRIORITY       = kernel_error(0x80020193u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE     = kernel_error(0x80020194u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_MODE           = kernel_error(0x80020195u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_THID           = kernel_error(0x80020197u);
constexpr std::int32_t SCE_KERNEL_ERROR_UNKNOWN_THID           = kernel_error(0x80020198u);
constexpr std::int32_t SCE_KERNEL_ERROR_UNKNOWN_SEMID          = kernel_error(0x80020199u);
constexpr std::int32_t SCE_KERNEL_ERROR_UNKNOWN_EVFID          = kernel_error(0x8002019Au);
constexpr std::int32_t SCE_KERNEL_ERROR_UNKNOWN_CBID           = kernel_error(0x800201A1u);
constexpr std::int32_t SCE_KERNEL_ERROR_DORMANT                = kernel_error(0x800201A2u);
constexpr std::int32_t SCE_KERNEL_ERROR_SUSPEND                = kernel_error(0x800201A3u);
constexpr std::int32_t SCE_KERNEL_ERROR_NOT_DORMANT            = kernel_error(0x800201A4u);
constexpr std::int32_t SCE_KERNEL_ERROR_NOT_SUSPEND            = kernel_error(0x800201A5u);
constexpr std::int32_t SCE_KERNEL_ERROR_NOT_WAIT               = kernel_error(0x800201A6u);
constexpr std::int32_t SCE_KERNEL_ERROR_CAN_NOT_WAIT           = kernel_error(0x800201A7u);
constexpr std::int32_t SCE_KERNEL_ERROR_WAIT_TIMEOUT           = kernel_error(0x800201A8u);
constexpr std::int32_t SCE_KERNEL_ERROR_WAIT_CANCEL            = kernel_error(0x800201A9u);
constexpr std::int32_t SCE_KERNEL_ERROR_RELEASE_WAIT           = kernel_error(0x800201AAu);
constexpr std::int32_t SCE_KERNEL_ERROR_SEMA_ZERO              = kernel_error(0x800201ADu);
constexpr std::int32_t SCE_KERNEL_ERROR_SEMA_OVERFLOW          = kernel_error(0x800201AEu);
constexpr std::int32_t SCE_KERNEL_ERROR_EVF_COND               = kernel_error(0x800201AFu);
constexpr std::int32_t SCE_KERNEL_ERROR_EVF_MULTI              = kernel_error(0x800201B0u);
constexpr std::int32_t SCE_KERNEL_ERROR_EVF_ILPAT              = kernel_error(0x800201B1u);
constexpr std::int32_t SCE_KERNEL_ERROR_WAIT_DELETE            = kernel_error(0x800201B5u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_COUNT          = kernel_error(0x800201BDu);
constexpr std::int32_t SCE_KERNEL_ERROR_MUTEX_NOT_FOUND        = kernel_error(0x800201C3u);
constexpr std::int32_t SCE_KERNEL_ERROR_MUTEX_LOCKED           = kernel_error(0x800201C4u);
constexpr std::int32_t SCE_KERNEL_ERROR_MUTEX_UNLOCKED         = kernel_error(0x800201C5u);
constexpr std::int32_t SCE_KERNEL_ERROR_MUTEX_LOCK_OVERFLOW    = kernel_error(0x800201C6u);
constexpr std::int32_t SCE_KERNEL_ERROR_MUTEX_UNLOCK_UNDERFLOW = kernel_error(0x800201C7u);
constexpr std::int32_t SCE_KERNEL_ERROR_MUTEX_RECURSIVE        = kernel_error(0x800201C8u);
constexpr std::int32_t SCE_KERNEL_ERROR_LWMUTEX_NOT_FOUND      = kernel_error(0x800201CAu);
constexpr std::int32_t SCE_KERNEL_ERROR_LWMUTEX_LOCKED         = kernel_error(0x800201CBu);
constexpr std::int32_t SCE_KERNEL_ERROR_LWMUTEX_UNLOCKED       = kernel_error(0x800201CCu);
constexpr std::int32_t SCE_KERNEL_ERROR_LWMUTEX_LOCK_OVERFLOW  = kernel_error(0x800201CDu);
constexpr std::int32_t SCE_KERNEL_ERROR_LWMUTEX_UNLOCK_UNDERFLOW = kernel_error(0x800201CEu);
constexpr std::int32_t SCE_KERNEL_ERROR_LWMUTEX_RECURSIVE      = kernel_error(0x800201CFu);

enum class InternalThreadState : std::uint32_t {
    Dormant   = 0,
    Ready     = 1,
    Running   = 2,
    Waiting   = 3,
    Suspended = 4,
    Stopped   = 5,
};

// PSP wait types as reported by sceKernelReferThreadStatus (PSPSDK
// pspthreadman.h ordering). Host waits (vblank, audio, GE, UMD) are internal.
enum class WaitType : std::uint32_t {
    None = 0, Sleep = 1, Delay = 2, Sema = 3, EventFlag = 4, ThreadEnd = 9,
    Mutex = 12, LwMutex = 13,
    Vblank = 0x100, Audio = 0x101, GeSync = 0x102, Umd = 0x103, Io = 0x104,
};

inline constexpr std::uint64_t kNoDeadline = std::numeric_limits<std::uint64_t>::max();

struct WaitInfo {
    WaitType type{WaitType::None};
    std::int32_t object{};          // UID or host channel
    std::uint64_t deadline{kNoDeadline};
    std::uint32_t timeout_ptr{};    // guest u32* updated with remaining microseconds
    bool callbacks{};               // *CB variant: deliver notified callbacks while waiting
    std::uint32_t a{}, b{}, c{};    // per-type parameters (count, bits, mode, out pointer...)
    // Resume at the import stub instead of $ra, re-running the HLE call on wake
    // (uOFW-style retry loops such as sceAudioOutputBlocking on OUTPUT_BUSY).
    bool retry{};
};

struct CallbackFrame {
    std::int32_t callback{};
    psprecomp::AllegrexContext resume{};  // context to continue after callbacks
    bool rewait{};                        // re-enter `wait` after callbacks
    std::optional<std::int32_t> woken;    // wake result delivered while running callbacks
};

struct ThreadControlBlock {
    std::int32_t uid{0};
    std::string name;
    std::uint32_t entry_pc{0};
    std::int32_t initial_priority{0x20};
    std::int32_t current_priority{0x20};
    std::uint32_t stack_size{0};
    std::uint32_t stack_address{0};
    std::uint32_t stack_top{0};
    std::uint32_t attributes{0};
    std::uint32_t option_address{0};
    std::uint32_t gp{0};
    InternalThreadState status{InternalThreadState::Dormant};
    bool started{false};
    bool entry_dispatch_pending{false};
    bool entry_executed{false};
    bool suspended{false};
    std::int32_t exit_status{0};
    std::int32_t wakeup_count{0};
    WaitInfo wait;
    std::optional<CallbackFrame> callback;
    bool callback_pending_run{};          // scheduled to run callbacks from a CB wait
    psprecomp::AllegrexContext context{};
};

struct CallbackObject {
    std::int32_t uid{}, owner_thread_uid{};
    std::string name;
    std::uint32_t function{}, common_argument{};
    std::int32_t notify_count{};
    std::uint32_t notify_arg{};
};

struct SemaphoreObject {
    std::int32_t uid{};
    std::string name;
    std::uint32_t attr{};
    std::int32_t init{}, count{}, max{};
    std::vector<std::int32_t> waiters;
};

struct EventFlagObject {
    std::int32_t uid{};
    std::string name;
    std::uint32_t attr{}, init{}, bits{};
    std::vector<std::int32_t> waiters;
};

struct MutexObject {
    std::int32_t uid{};
    std::string name;
    std::uint32_t attr{};
    std::int32_t init{}, count{}, owner{};
    std::vector<std::int32_t> waiters;
};

struct LwMutexObject {
    std::int32_t uid{};
    std::string name;
    std::uint32_t attr{}, workarea{};
    std::int32_t init{};
    std::vector<std::int32_t> waiters;
};

struct ThreadEntryProvenance {
    std::int32_t uid{}, from_uid{};
    std::uint32_t entry_pc{};
};

class ThreadManager {
public:
    static constexpr std::uint32_t kThreadReturnSentinel = 0x00000020u;
    static constexpr std::uint32_t kCallbackReturnSentinel = 0x00000024u;
    static constexpr std::uint32_t kThreadAttrNoFillStack = 0x00100000u;
    // [INFERRED] Fixed virtual cost of one HLE call so guest polling loops on
    // sceKernelGetSystemTime* terminate deterministically.
    static constexpr std::uint64_t kSyscallCostUs = 1u;
    // 59.94 Hz vblank period in microseconds (sceDisplayGetFramePerSec).
    static constexpr std::uint64_t kVblankPeriodUs = 16683u;

    ThreadManager();

    void reset();
    std::int32_t create_callback(std::string_view name, std::uint32_t function, std::uint32_t common_argument);
    [[nodiscard]] const CallbackObject *get_callback(std::int32_t uid) const noexcept;

    std::int32_t init_root_thread(std::string_view name, std::uint32_t entry_pc, std::uint32_t sp, std::uint32_t gp);

    std::int32_t create_thread(std::string_view name, std::uint32_t entry_pc, std::int32_t init_priority,
                               std::uint32_t stack_size, std::uint32_t attributes, std::uint32_t option_address,
                               psprecomp::GuestMemory &memory);

    // `allow_preempt == false` leaves the caller running (it blocks itself
    // next); a non-zero `gp` overrides the inherited $gp (module_start threads).
    std::int32_t start_thread(std::int32_t thid, std::uint32_t arg_size, std::uint32_t arg_ptr,
                              psprecomp::GuestMemory &memory, psprecomp::AllegrexContext &caller_ctx,
                              bool allow_preempt = true, std::uint32_t gp = 0u);

    bool exit_current_thread(std::int32_t exit_status, psprecomp::AllegrexContext &ctx, psprecomp::Runtime &runtime);

    bool schedule(psprecomp::AllegrexContext &ctx);

    [[nodiscard]] ThreadControlBlock *get_thread(std::int32_t uid) noexcept;
    [[nodiscard]] const ThreadControlBlock *get_thread(std::int32_t uid) const noexcept;
    [[nodiscard]] ThreadControlBlock *current_thread() noexcept;
    [[nodiscard]] const ThreadControlBlock *current_thread() const noexcept;
    [[nodiscard]] std::int32_t current_thread_id() const noexcept { return current_thread_id_; }
    [[nodiscard]] std::size_t thread_count() const noexcept { return threads_.size(); }
    [[nodiscard]] std::optional<ThreadEntryProvenance> verified_thread_entry(
        const psprecomp::AllegrexContext &ctx, std::int32_t runtime_uid) const noexcept;
    void note_guest_execution(std::int32_t uid, std::uint32_t pc) noexcept;
    void invalidate_thread_entry() noexcept { active_entry_.reset(); }

    bool switch_to(std::int32_t target_thid, psprecomp::AllegrexContext &ctx);
    std::int32_t change_current_thread_attr(std::uint32_t clear_attr, std::uint32_t set_attr) noexcept;

    // ---- Virtual clock ----------------------------------------------------
    [[nodiscard]] std::uint64_t now() const noexcept { return now_us_; }
    [[nodiscard]] std::uint64_t vblank_count() const noexcept { return now_us_ / kVblankPeriodUs; }
    [[nodiscard]] std::uint64_t next_vblank_time() const noexcept { return (vblank_count() + 1u) * kVblankPeriodUs; }

    // PCs of PSP import stubs; ctx.pc equal to one of them means "inside an HLE
    // call", whose resume PC is $ra.
    void set_import_stubs(std::set<std::uint32_t> stubs) { import_stubs_ = std::move(stubs); }
    void add_import_stub(std::uint32_t pc) { import_stubs_.insert(pc); }

    // ---- Blocking ----------------------------------------------------------
    // Blocks the current thread inside an HLE handler (resume at $ra with $v0
    // supplied by the waker) and switches `ctx` to the next runnable thread,
    // advancing virtual time when all threads wait. Stops the runtime on a
    // deadlock (every thread waits without a deadline).
    void block_current(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, WaitInfo wait);
    // Makes a waiting thread ready with `result` in $v0 (no immediate switch).
    void wake(psprecomp::Runtime &rt, std::int32_t uid, std::int32_t result);
    // After a waker HLE returns `result` to the current thread: switch if a
    // strictly higher-priority thread is ready.
    void preempt_if_needed(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx);
    // Post-HLE hook: advances time by kSyscallCostUs, expires deadlines,
    // delivers pending callbacks and preempts.
    void on_syscall_return(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx);
    // Host-side wake helper for internal waits keyed by (type, object).
    std::size_t wake_host_waiters(psprecomp::Runtime &rt, WaitType type, std::int32_t object, std::int32_t result);
    [[nodiscard]] std::uint64_t idle_advances() const noexcept { return idle_advances_; }

    // ---- Callbacks -----------------------------------------------------------
    std::int32_t delete_callback(std::int32_t uid);
    std::int32_t notify_callback(psprecomp::Runtime &rt, std::int32_t uid, std::uint32_t arg);
    // sceKernelCheckCallback: runs pending callbacks of the current thread.
    // Returns true if a callback frame was started (ctx now executes it).
    bool check_callbacks(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx);
    void callback_returned(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx);

    // ---- Thread services ----------------------------------------------------
    std::int32_t delete_thread(std::int32_t uid);
    std::int32_t change_priority(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid, std::int32_t priority);
    std::int32_t wakeup_thread(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid);
    std::int32_t suspend_thread(std::int32_t uid);
    std::int32_t resume_thread(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid);
    std::int32_t refer_thread_status(psprecomp::GuestMemory &memory, std::int32_t uid, std::uint32_t info) const;

    // ---- Synchronisation objects ------------------------------------------
    std::int32_t create_sema(std::string_view name, std::uint32_t attr, std::int32_t init, std::int32_t max);
    std::int32_t delete_sema(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid);
    std::int32_t signal_sema(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid, std::int32_t signal);
    // Returns nullopt when the caller blocked (ctx switched).
    std::optional<std::int32_t> wait_sema(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid,
                                          std::int32_t count, std::uint32_t timeout_ptr, bool callbacks);
    std::int32_t poll_sema(std::int32_t uid, std::int32_t count);
    [[nodiscard]] const SemaphoreObject *get_sema(std::int32_t uid) const noexcept;

    std::int32_t create_event_flag(std::string_view name, std::uint32_t attr, std::uint32_t bits);
    std::int32_t delete_event_flag(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid);
    std::int32_t set_event_flag(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid, std::uint32_t bits);
    std::int32_t clear_event_flag(std::int32_t uid, std::uint32_t bits);
    std::optional<std::int32_t> wait_event_flag(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid,
                                                std::uint32_t bits, std::uint32_t mode, std::uint32_t out_ptr,
                                                std::uint32_t timeout_ptr, bool callbacks);
    std::int32_t poll_event_flag(psprecomp::GuestMemory &memory, std::int32_t uid, std::uint32_t bits,
                                 std::uint32_t mode, std::uint32_t out_ptr);
    [[nodiscard]] const EventFlagObject *get_event_flag(std::int32_t uid) const noexcept;

    std::int32_t create_mutex(std::string_view name, std::uint32_t attr, std::int32_t init);
    std::int32_t delete_mutex(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid);
    std::optional<std::int32_t> lock_mutex(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid,
                                           std::int32_t count, std::uint32_t timeout_ptr, bool callbacks);
    std::int32_t unlock_mutex(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::int32_t uid, std::int32_t count);
    [[nodiscard]] const MutexObject *get_mutex(std::int32_t uid) const noexcept;

    std::int32_t create_lw_mutex(psprecomp::GuestMemory &memory, std::uint32_t workarea, std::string_view name,
                                 std::uint32_t attr, std::int32_t init);
    std::int32_t delete_lw_mutex(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::uint32_t workarea);
    std::optional<std::int32_t> lock_lw_mutex(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx,
                                              std::uint32_t workarea, std::int32_t count, std::uint32_t timeout_ptr,
                                              bool callbacks);
    std::int32_t try_lock_lw_mutex(psprecomp::GuestMemory &memory, std::uint32_t workarea, std::int32_t count);
    std::int32_t unlock_lw_mutex(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::uint32_t workarea,
                                 std::int32_t count);

    std::optional<std::int32_t> delay_current(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx,
                                              std::uint32_t microseconds, bool callbacks);
    std::optional<std::int32_t> sleep_current(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, bool callbacks);
    std::optional<std::int32_t> wait_thread_end(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx,
                                                std::int32_t uid, std::uint32_t timeout_ptr, bool callbacks);

private:
    void select_initial_entry(ThreadControlBlock &target, std::int32_t from_uid,
                              const psprecomp::AllegrexContext &ctx) noexcept;
    [[nodiscard]] std::uint32_t resume_pc(const psprecomp::AllegrexContext &ctx) const noexcept;
    [[nodiscard]] bool runnable(const ThreadControlBlock &t) const noexcept;
    [[nodiscard]] ThreadControlBlock *best_ready() noexcept;
    void enqueue_ready(std::int32_t uid, bool front = false);
    void remove_ready(std::int32_t uid);
    // Loads the next thread into ctx; advances time while idle.
    void dispatch_next(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx);
    void load_thread(ThreadControlBlock &t, psprecomp::AllegrexContext &ctx, std::int32_t from_uid);
    void expire_deadlines(psprecomp::Runtime &rt);
    void remove_from_object(ThreadControlBlock &t);
    void finish_wait(psprecomp::Runtime &rt, ThreadControlBlock &t, std::int32_t result, bool set_result = true);
    void insert_waiter(std::vector<std::int32_t> &waiters, std::int32_t uid, bool priority_order);
    bool start_callback(ThreadControlBlock &t, psprecomp::AllegrexContext &ctx,
                        const psprecomp::AllegrexContext &resume, bool rewait);
    [[nodiscard]] bool has_pending_callbacks(std::int32_t uid) const noexcept;
    std::optional<std::int32_t> block_or_result(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx,
                                                WaitInfo wait, std::uint32_t timeout_ptr);
    void release_sema_waiters(psprecomp::Runtime &rt, SemaphoreObject &sema);
    void release_event_flag_waiters(psprecomp::Runtime &rt, EventFlagObject &flag, psprecomp::GuestMemory &memory);
    void release_mutex_waiters(psprecomp::Runtime &rt, MutexObject &mutex);
    void release_lw_mutex_waiters(psprecomp::Runtime &rt, LwMutexObject &mutex);
    std::int32_t fail_waiters(psprecomp::Runtime &rt, std::vector<std::int32_t> &waiters, std::int32_t result);

    std::int32_t next_uid_{1};
    std::int32_t current_thread_id_{0};
    std::uint32_t next_stack_top_{0x09FF0000u};
    std::uint64_t now_us_{0};
    std::uint64_t idle_advances_{0};
    std::map<std::int32_t, ThreadControlBlock> threads_;
    std::map<std::int32_t, CallbackObject> callbacks_;
    std::map<std::int32_t, SemaphoreObject> semas_;
    std::map<std::int32_t, EventFlagObject> event_flags_;
    std::map<std::int32_t, MutexObject> mutexes_;
    std::map<std::int32_t, LwMutexObject> lw_mutexes_;
    std::vector<std::int32_t> ready_queue_;
    std::set<std::uint32_t> import_stubs_;
    std::optional<ThreadEntryProvenance> active_entry_;
    psprecomp::GuestMemory *memory_{};
    psprecomp::Runtime *runtime_{}; // event sink for paths without a Runtime argument
};

void register_threadman_for_user(psprecomp::Runtime &runtime, KernelState &kernel);
// Installs ThreadManager::on_syscall_return as the Runtime post-import hook
// for the kernel most recently passed to register_thread_trampolines().
void install_threadman_post_import_hook();
// Registers the thread-return and callback-return sentinel PCs.
void register_thread_trampolines(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
