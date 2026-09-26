#pragma once

#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/guest_memory.hpp"
#include "psprecomp/runtime.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace p3p3ds {
class KernelState;
}

namespace p3p3ds::hle {

// Real PSP Kernel Error Codes for ThreadMan
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT   = static_cast<std::int32_t>(0x800200D2u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_ADDR       = static_cast<std::int32_t>(0x800200D3u);
constexpr std::int32_t SCE_KERNEL_ERROR_NO_MEMORY          = static_cast<std::int32_t>(0x80020190u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_ATTR       = static_cast<std::int32_t>(0x80020191u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_ENTRY      = static_cast<std::int32_t>(0x80020192u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_PRIORITY   = static_cast<std::int32_t>(0x80020193u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE = static_cast<std::int32_t>(0x80020194u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_THID       = static_cast<std::int32_t>(0x80020197u);
constexpr std::int32_t SCE_KERNEL_ERROR_UNKNOWN_THID       = static_cast<std::int32_t>(0x80020198u);
constexpr std::int32_t SCE_KERNEL_ERROR_DORMANT            = static_cast<std::int32_t>(0x800201A2u);
constexpr std::int32_t SCE_KERNEL_ERROR_SUSPEND            = static_cast<std::int32_t>(0x800201A3u);
constexpr std::int32_t SCE_KERNEL_ERROR_NOT_DORMANT        = static_cast<std::int32_t>(0x800201A4u);
constexpr std::int32_t SCE_KERNEL_ERROR_NOT_SUSPEND        = static_cast<std::int32_t>(0x800201A5u);

// Internal runtime thread lifecycle states (distinct from public PSP bitmask flags)
enum class InternalThreadState : std::uint32_t {
    Dormant   = 0,
    Ready     = 1,
    Running   = 2,
    Waiting   = 3,
    Suspended = 4,
    Stopped   = 5,
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
    InternalThreadState status{InternalThreadState::Dormant};
    std::int32_t exit_status{0};
    psprecomp::AllegrexContext context{};
};

class ThreadManager {
public:
    static constexpr std::uint32_t kThreadReturnSentinel = 0x00000020u;
    static constexpr std::uint32_t kThreadAttrNoFillStack = 0x00100000u;

    ThreadManager();

    void reset();

    std::int32_t init_root_thread(std::string_view name, std::uint32_t entry_pc, std::uint32_t sp, std::uint32_t gp);

    std::int32_t create_thread(std::string_view name,
                               std::uint32_t entry_pc,
                               std::int32_t init_priority,
                               std::uint32_t stack_size,
                               std::uint32_t attributes,
                               std::uint32_t option_address,
                               psprecomp::GuestMemory &memory);

    std::int32_t start_thread(std::int32_t thid,
                              std::uint32_t arg_size,
                              std::uint32_t arg_ptr,
                              psprecomp::GuestMemory &memory,
                              psprecomp::AllegrexContext &caller_ctx);

    bool exit_current_thread(std::int32_t exit_status, psprecomp::AllegrexContext &ctx, psprecomp::Runtime &runtime);

    bool schedule(psprecomp::AllegrexContext &ctx);

    [[nodiscard]] ThreadControlBlock *get_thread(std::int32_t uid) noexcept;
    [[nodiscard]] const ThreadControlBlock *get_thread(std::int32_t uid) const noexcept;
    [[nodiscard]] ThreadControlBlock *current_thread() noexcept;
    [[nodiscard]] const ThreadControlBlock *current_thread() const noexcept;
    [[nodiscard]] std::int32_t current_thread_id() const noexcept { return current_thread_id_; }
    [[nodiscard]] std::size_t thread_count() const noexcept { return threads_.size(); }

    bool switch_to(std::int32_t target_thid, psprecomp::AllegrexContext &ctx);
    std::int32_t change_current_thread_attr(std::uint32_t clear_attr, std::uint32_t set_attr) noexcept;

private:
    std::int32_t next_uid_{1};
    std::int32_t current_thread_id_{0};
    std::uint32_t next_stack_top_{0x09FF0000u};
    std::map<std::int32_t, ThreadControlBlock> threads_;
    std::vector<std::int32_t> ready_queue_;
};

void register_threadman_for_user(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
