#include "p3p3ds/hle/sysmem.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/runtime.hpp"

#include <iostream>

namespace p3p3ds::hle {

SysMemManager::SysMemManager(std::uint32_t user_base, std::uint32_t user_end)
    : user_heap_base_(user_base), user_heap_end_(user_end), user_heap_current_(user_base) {}

std::int32_t SysMemManager::alloc_partition_memory(std::uint32_t partition_id, const std::string &name,
                                                   std::uint32_t type, std::uint32_t size,
                                                   std::uint32_t addr_or_align) {
    if (size == 0u) {
        return SCE_KERNEL_ERROR_MEMBLOCK_ALLOC_FAILED;
    }
    // Partition 2: User Memory; Partition 1: Kernel Memory
    if (partition_id != 2u && partition_id != 1u) {
        return SCE_KERNEL_ERROR_ILLEGAL_PARTITION;
    }

    std::uint32_t align = 256u;
    const auto block_type = static_cast<PspSysMemBlockType>(type);
    if (block_type == PspSysMemBlockType::LowAligned || block_type == PspSysMemBlockType::HighAligned) {
        align = addr_or_align != 0u ? addr_or_align : 256u;
        // Ensure alignment is power-of-two
        if ((align & (align - 1u)) != 0u) {
            align = 4096u;
        }
    }

    std::uint32_t alloc_addr = 0u;
    if (block_type == PspSysMemBlockType::Addr) {
        alloc_addr = addr_or_align;
        if (alloc_addr < user_heap_base_ || alloc_addr + size > user_heap_end_) {
            return SCE_KERNEL_ERROR_MEMBLOCK_ALLOC_FAILED;
        }
    } else {
        // Low or LowAligned
        const std::uint32_t aligned = (user_heap_current_ + align - 1u) & ~(align - 1u);
        if (aligned + size > user_heap_end_ || aligned < user_heap_current_) {
            return SCE_KERNEL_ERROR_MEMBLOCK_ALLOC_FAILED;
        }
        alloc_addr = aligned;
        user_heap_current_ = aligned + size;
    }

    const std::int32_t uid = next_uid_++;
    PartitionBlock block;
    block.uid = uid;
    block.partition_id = partition_id;
    block.name = name;
    block.type = block_type;
    block.size = size;
    block.address = alloc_addr;

    blocks_[uid] = std::move(block);
    return uid;
}

std::int32_t SysMemManager::free_partition_memory(std::int32_t uid) {
    auto it = blocks_.find(uid);
    if (it == blocks_.end()) {
        return SCE_KERNEL_ERROR_UNKNOWN_UID;
    }
    blocks_.erase(it);
    return 0;
}

std::uint32_t SysMemManager::get_block_head_addr(std::int32_t uid) const {
    auto it = blocks_.find(uid);
    if (it == blocks_.end()) {
        return 0u;
    }
    return it->second.address;
}

const PartitionBlock *SysMemManager::find_block(std::int32_t uid) const {
    auto it = blocks_.find(uid);
    return it != blocks_.end() ? &it->second : nullptr;
}

std::uint32_t SysMemManager::total_free_memory(std::uint32_t) const noexcept {
    return user_heap_current_ <= user_heap_end_ ? (user_heap_end_ - user_heap_current_) : 0u;
}

std::uint32_t SysMemManager::max_free_memory(std::uint32_t) const noexcept {
    return total_free_memory();
}

void sceKernelSetCompiledSdkVersion600_602(KernelState &kernel, std::uint32_t version) noexcept {
    kernel.set_compiled_sdk_version(version);
}

void sceKernelSetCompilerVersion(KernelState &kernel, std::uint32_t version) noexcept {
    kernel.set_compiler_version(version);
}

void register_sysmem_user_for_user(psprecomp::Runtime &runtime, KernelState &kernel) {
    // SysMemUserForUser::0x35669D4C - sceKernelSetCompiledSdkVersion600_602
    runtime.register_hle("SysMemUserForUser", 0x35669D4Cu,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t version = ctx.gpr[4]; // $a0
            sceKernelSetCompiledSdkVersion600_602(kernel, version);
            ctx.set_gpr(2, 0u); // $v0 = 0 (SCE_KERNEL_ERROR_OK)
        });

    // SysMemUserForUser::0xF77D77CB - sceKernelSetCompilerVersion
    runtime.register_hle("SysMemUserForUser", 0xF77D77CBu,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t version = ctx.gpr[4]; // $a0
            sceKernelSetCompilerVersion(kernel, version);
            ctx.set_gpr(2, 0u); // $v0 = 0 (SCE_KERNEL_ERROR_OK)
        });

    // SysMemUserForUser::0x237DBD4F - sceKernelAllocPartitionMemory
    runtime.register_hle("SysMemUserForUser", 0x237DBD4Fu,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t partition = ctx.gpr[4]; // $a0
            const std::uint32_t name_ptr = ctx.gpr[5];  // $a1
            const std::uint32_t type = ctx.gpr[6];      // $a2
            const std::uint32_t size = ctx.gpr[7];      // $a3
            const std::uint32_t addr_align = ctx.gpr[8]; // $t0 (5th argument in Allegrex ABI)
            const std::string name = name_ptr != 0u ? rt.memory().read_c_string(name_ptr, 64u) : "";
            const std::int32_t uid = kernel.sysmem().alloc_partition_memory(partition, name, type, size, addr_align);
            ctx.set_gpr(2, static_cast<std::uint32_t>(uid)); // $v0
        });

    // SysMemUserForUser::0x9D9A5BA1 - sceKernelGetBlockHeadAddr
    runtime.register_hle("SysMemUserForUser", 0x9D9A5BA1u,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            const auto uid = static_cast<std::int32_t>(ctx.gpr[4]); // $a0
            const std::uint32_t addr = kernel.sysmem().get_block_head_addr(uid);
            ctx.set_gpr(2, addr); // $v0
        });

    // SysMemUserForUser::0xB6D61D02 - sceKernelFreePartitionMemory
    runtime.register_hle("SysMemUserForUser", 0xB6D61D02u,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            const auto uid = static_cast<std::int32_t>(ctx.gpr[4]); // $a0
            const std::int32_t res = kernel.sysmem().free_partition_memory(uid);
            ctx.set_gpr(2, static_cast<std::uint32_t>(res)); // $v0
        });

    // SysMemUserForUser::0x13A5ABEF - printf / sceKernelPrintf
    runtime.register_hle("SysMemUserForUser", 0x13A5ABEFu,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // DIRTY_FIRST_FRAME: minimal printf stub for PSP kernel debug output
            ctx.set_gpr(2, 0u); // $v0 = 0
        });

    // SysMemUserForUser::0xF9100EB9 - sceKernelTotalFreeMemSize
    runtime.register_hle("SysMemUserForUser", 0xF9100EB9u,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, kernel.sysmem().total_free_memory());
        });

    // SysMemUserForUser::0xA291F107 - sceKernelMaxFreeMemSize
    runtime.register_hle("SysMemUserForUser", 0xA291F107u,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, kernel.sysmem().max_free_memory());
        });
}

} // namespace p3p3ds::hle
