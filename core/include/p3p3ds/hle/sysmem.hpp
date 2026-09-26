#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace psprecomp {
class Runtime;
}

namespace p3p3ds {
class KernelState;
}

namespace p3p3ds::hle {

// Specifies the allocation alignment and placement type (PSP SysMem / uOFW / PPSSPP)
enum class PspSysMemBlockType : std::uint32_t {
    Low = 0,
    High = 1,
    Addr = 2,
    LowAligned = 3,
    HighAligned = 4,
};

// Real PSP Kernel Error Codes for SysMem
constexpr std::int32_t SCE_KERNEL_ERROR_UNKNOWN_UID           = static_cast<std::int32_t>(0x800200CBu);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_PARTITION     = static_cast<std::int32_t>(0x800200D6u);
constexpr std::int32_t SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCKTYPE  = static_cast<std::int32_t>(0x800200D8u);
constexpr std::int32_t SCE_KERNEL_ERROR_MEMBLOCK_ALLOC_FAILED = static_cast<std::int32_t>(0x800200D9u);

struct PartitionBlock {
    std::int32_t uid{0};
    std::uint32_t partition_id{2};
    std::string name;
    PspSysMemBlockType type{PspSysMemBlockType::Low};
    std::uint32_t size{0};
    std::uint32_t address{0};
};

class SysMemManager {
public:
    static constexpr std::uint32_t kDefaultUserHeapBase = 0x08ED0000u;
    static constexpr std::uint32_t kDefaultUserHeapEnd  = 0x09FF0000u; // Below 64 KiB stack at 0x09FF0000

    explicit SysMemManager(std::uint32_t user_base = kDefaultUserHeapBase,
                           std::uint32_t user_end = kDefaultUserHeapEnd);

    std::int32_t alloc_partition_memory(std::uint32_t partition_id, const std::string &name,
                                        std::uint32_t type, std::uint32_t size, std::uint32_t addr_or_align);
    std::int32_t free_partition_memory(std::int32_t uid);
    std::uint32_t get_block_head_addr(std::int32_t uid) const;

    [[nodiscard]] const PartitionBlock *find_block(std::int32_t uid) const;
    [[nodiscard]] std::size_t block_count() const noexcept { return blocks_.size(); }
    [[nodiscard]] std::uint32_t total_free_memory(std::uint32_t partition_id = 2) const noexcept;
    [[nodiscard]] std::uint32_t max_free_memory(std::uint32_t partition_id = 2) const noexcept;
    [[nodiscard]] std::uint32_t user_heap_current() const noexcept { return user_heap_current_; }

private:
    std::uint32_t user_heap_base_{kDefaultUserHeapBase};
    std::uint32_t user_heap_end_{kDefaultUserHeapEnd};
    std::uint32_t user_heap_current_{kDefaultUserHeapBase};
    std::int32_t next_uid_{1};
    std::unordered_map<std::int32_t, PartitionBlock> blocks_;
};

// Real PSP API semantics (uOFW SysMem, pspautotests misc/sdkver.cpp, PPSSPP):
// int sceKernelSetCompiledSdkVersion600_602(int sdkVersion);
// Pure registration function. Records compiled SDK version in kernel state.
// Real API returns 0 (SCE_KERNEL_ERROR_OK) in $v0.
void sceKernelSetCompiledSdkVersion600_602(KernelState &kernel, std::uint32_t version) noexcept;

// Real PSP API semantics (uOFW SysMem, pspautotests misc/sdkver.cpp, PPSSPP):
// int sceKernelSetCompilerVersion(int version);
// Pure registration function. Records compiler version in kernel state.
// Real API returns 0 (SCE_KERNEL_ERROR_OK) in $v0.
void sceKernelSetCompilerVersion(KernelState &kernel, std::uint32_t version) noexcept;

// SysMemUserForUser module handler / registration
void register_sysmem_user_for_user(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
