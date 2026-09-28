#pragma once
#include <cstdint>
#include <cstddef>

/// PSP address space constants (matching emitter's recomp.h mask 0x07FFFFFFU).
/// The runtime allocates 128MB via mmap(MAP_ANON) for sparse page allocation.

/// Address mask applied by emitter's MEM_* accessors
static constexpr uint32_t PSP_ADDR_MASK       = 0x07FFFFFFu;

/// Total mmap allocation size (128MB — covers full masked address range)
static constexpr size_t   PSP_MEM_SIZE        = 0x08000000u;

/// PSP main RAM: 64MB at physical 0x08000000 (masked to 0x00000000)
/// PSP Slim (2000/3000) 64MB RAM
static constexpr uint32_t PSP_RAM_BASE        = 0x08000000u;
static constexpr uint32_t PSP_RAM_SIZE        = 0x04000000u;  // 64MB

/// PSP VRAM (EDRAM): 2MB at physical 0x04000000
static constexpr uint32_t PSP_VRAM_BASE       = 0x04000000u;
static constexpr uint32_t PSP_VRAM_SIZE       = 0x00200000u;  // 2MB

/// PSP Scratchpad: 16KB at physical 0x00010000
static constexpr uint32_t PSP_SCRATCHPAD_BASE = 0x00010000u;
static constexpr uint32_t PSP_SCRATCHPAD_SIZE = 0x00004000u;  // 16KB

/// Allocate 128MB sparse memory region via mmap(MAP_ANON).
/// Returns nullptr on failure.
uint8_t* psp_memory_init();

/// Release the mmap allocation.
void psp_memory_cleanup(uint8_t* rdram);
