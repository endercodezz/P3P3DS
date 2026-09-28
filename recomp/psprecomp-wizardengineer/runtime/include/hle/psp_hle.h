#pragma once
#include <cstdint>
#include <cstdio>
#include <atomic>
#include <unordered_map>
#include <unordered_set>

// Forward declarations
struct recomp_context;
using FuncPtr = void(*)(uint8_t*, recomp_context*);

// HLE function pointer type
using HleFunc = void(*)(uint8_t*, recomp_context*);

// ---- PSP Error Codes ----
// Standard PSP kernel error codes (from PSP SDK / PPSSPP)
constexpr int32_t SCE_OK = 0;

// Errno errors
constexpr int32_t SCE_ERROR_ERRNO_ENOENT        = (int32_t)0x80010002U;
constexpr int32_t SCE_ERROR_ERRNO_EIO            = (int32_t)0x80010005U;
constexpr int32_t SCE_ERROR_ERRNO_ENOMEM         = (int32_t)0x8001000CU;
constexpr int32_t SCE_ERROR_ERRNO_EACCES         = (int32_t)0x8001000DU;
constexpr int32_t SCE_ERROR_ERRNO_EEXIST         = (int32_t)0x80010011U;
constexpr int32_t SCE_ERROR_ERRNO_ENODEV         = (int32_t)0x80010013U;
constexpr int32_t SCE_ERROR_ERRNO_EINVAL         = (int32_t)0x80010016U;
constexpr int32_t SCE_ERROR_ERRNO_EMFILE         = (int32_t)0x80010018U;
constexpr int32_t SCE_ERROR_ERRNO_ENOSPC         = (int32_t)0x8001001CU;

// Kernel errors
constexpr int32_t SCE_KERNEL_ERROR_OK            = 0;
constexpr int32_t SCE_KERNEL_ERROR_ERROR         = (int32_t)0x80020001U;
constexpr int32_t SCE_KERNEL_ERROR_ACCESS_ERROR  = (int32_t)0x8000020DU;
constexpr int32_t SCE_KERNEL_ERROR_NO_MEMORY     = (int32_t)0x800200D8U;
constexpr int32_t SCE_KERNEL_ERROR_ILLEGAL_ATTR  = (int32_t)0x800200CBU;
constexpr int32_t SCE_KERNEL_ERROR_ILLEGAL_ADDR  = (int32_t)0x800200D3U;
constexpr int32_t SCE_KERNEL_ERROR_ILLEGAL_THREAD  = (int32_t)0x800201BDU;
constexpr int32_t SCE_KERNEL_ERROR_NOT_FOUND_THREAD = (int32_t)0x800201BEU;
constexpr int32_t SCE_KERNEL_ERROR_WAIT_TIMEOUT  = (int32_t)0x800201A8U;
constexpr int32_t SCE_KERNEL_ERROR_WAIT_DELETE   = (int32_t)0x800201B5U;
constexpr int32_t SCE_KERNEL_ERROR_ILLEGAL_COUNT = (int32_t)0x800201BDU;
constexpr int32_t SCE_KERNEL_ERROR_SEMA_ZERO     = (int32_t)0x800201AEU;
constexpr int32_t SCE_KERNEL_ERROR_SEMA_OVERFLOW = (int32_t)0x800201AFU;
constexpr int32_t SCE_KERNEL_ERROR_MUTEX_NOT_FOUND = (int32_t)0x800201C3U;
// LwMutex errors (PPSSPP Core/HLE/ErrorCodes.h). The base TryLock variant
// returns the plain-mutex TRYLOCK_FAILED code (PPSSPP parity).
constexpr int32_t SCE_MUTEX_ERROR_TRYLOCK_FAILED    = (int32_t)0x800201C4U;
constexpr int32_t SCE_LWMUTEX_ERROR_NO_SUCH_LWMUTEX = (int32_t)0x800201CAU;
constexpr int32_t SCE_LWMUTEX_ERROR_NOT_LOCKED      = (int32_t)0x800201CCU;
constexpr int32_t SCE_LWMUTEX_ERROR_LOCK_OVERFLOW   = (int32_t)0x800201CDU;
constexpr int32_t SCE_LWMUTEX_ERROR_UNLOCK_UNDERFLOW = (int32_t)0x800201CEU;
constexpr int32_t SCE_LWMUTEX_ERROR_ALREADY_LOCKED  = (int32_t)0x800201CFU;
constexpr int32_t SCE_KERNEL_ERROR_EVF_NOT_FOUND = (int32_t)0x800201BFU;
constexpr int32_t SCE_KERNEL_ERROR_NOT_FOUND_MODULE = (int32_t)0x80020196U;
// MsgPipe errors (PPSSPP Core/HLE/ErrorCodes.h; match pspkerror.h)
constexpr int32_t SCE_KERNEL_ERROR_UNKNOWN_MPPID = (int32_t)0x8002019EU;
constexpr int32_t SCE_KERNEL_ERROR_MPP_FULL      = (int32_t)0x800201B3U;
constexpr int32_t SCE_KERNEL_ERROR_MPP_EMPTY     = (int32_t)0x800201B4U;
constexpr int32_t SCE_KERNEL_ERROR_ILLEGAL_SIZE  = (int32_t)0x800201BCU;
// IO kernel errors (PPSSPP Core/HLE/sceIo.cpp). NOTE: distinct from the
// errno-style SCE_ERROR_ERRNO_EMFILE (0x80010018) above.
constexpr int32_t SCE_KERNEL_ERROR_MFILE         = (int32_t)0x80020320U;
constexpr int32_t SCE_KERNEL_ERROR_BADF          = (int32_t)0x80020323U;

// ---- PSP Memory Constants ----
// PSP_ADDR_MASK (0x07FFFFFFU) is defined in psp_memory.h
constexpr uint32_t PSP_USER_MEM_BASE  = 0x08800000U;
constexpr uint32_t PSP_USER_MEM_END   = 0x0C000000U;  // PSP Slim (64MB): user space up to 0x0C000000
// (The BND asset-arena constants that lived here moved with the BND layer
// to games/patapon/runtime/asset_bnd.h — issue #47 Phase 5.)
constexpr uint32_t PSP_KERNEL_MEM_BASE = 0x08000000U;

// ---- Kernel Memory Allocator ----

/// Allocate memory in the PSP kernel memory region (0x08000000-0x083FFFFF).
/// Simple bump allocator for NativeModule structs and similar kernel objects.
/// Returns PSP virtual address (not masked). Returns 0 on OOM.
uint32_t psp_alloc_kernel_memory(uint32_t size);

// ---- Boot Module Constants ----

/// Boot module UID (assigned during init, returned by sceKernelGetModuleId)
constexpr int BOOT_MODULE_UID = 1;
/// Boot module NativeModule address in kernel memory
constexpr uint32_t BOOT_MODULE_ADDR = 0x08000100U;
/// NativeModule struct size (from PPSSPP sceKernelModule.h)
constexpr uint32_t NATIVE_MODULE_SIZE = 0xC4U;  // 196 bytes
/// Get the boot module's GP value
uint32_t psp_get_boot_module_gp();

// ---- HLE Trace Control ----

/// When true, every HLE function call logs thread name and function name
/// to stderr. Controlled by PSPRECOMP_HLE_TRACE=1 environment variable.
/// Initialized during psp_hle_init().
extern bool g_hle_trace_enabled;

// ---- HLE Dispatch API ----

/// Register an HLE function for a specific NID name.
/// Called during init by each per-module registration function.
void psp_hle_register(const char* nid_name, HleFunc fn);

/// Initialize HLE subsystem: register all module stubs, then override the
/// dispatch table entries for every import stub in the generated table
/// (<output>/syscall_table.cpp, issue #40). Stubs without a registered
/// handler get a loud per-NID unimplemented no-op — never a silent gap.
void psp_hle_init();

/// Stub address of `nid` in the generated import table, or 0 when this game
/// does not import it (issue #40). Runtime code keys on NIDs — universal PSP
/// API constants — never on per-game stub addresses.
uint32_t psp_hle_stub_addr_for_nid(uint32_t nid);

/// Central HLE syscall dispatcher (for actual syscall instructions in binary).
/// Called from generated code for MipsOp::Syscall instructions.
void psp_hle_syscall(uint8_t* rdram, recomp_context* ctx, uint32_t code);

/// Override a dispatch table entry (provided by generated dispatch.cpp).
extern void psp_dispatch_register(uint32_t vaddr, FuncPtr fn);

/// Resolve a guest address against the dispatch table WITHOUT the
/// LOOKUP_MISS machinery: returns nullptr when absent (no log, no STRICT
/// abort, no miss counters). Boot-time only — not thread-safe (#47 P1).
extern FuncPtr psp_dispatch_probe_lookup(uint32_t vaddr);

/// Game-module LOOKUP_MISS handler (#47 P5 seam): invoked from the
/// non-STRICT miss stub with (rdram, ctx, missed addr, per-address miss
/// count) BEFORE the stub's deterministic `v0 = 0`. The game module may
/// log address-keyed diagnostics or apply title-specific workarounds
/// (e.g. Patapon's 0x438 corrupt-vtable sema signal). nullptr = none.
using PspLookupMissHandler =
    void (*)(uint8_t* rdram, recomp_context* ctx, uint32_t addr, int count);
void psp_dispatch_set_miss_handler(PspLookupMissHandler fn);

// ---- Game-module seams over the user-memory bump heap (#47 Phase 5) ----
// Game modules (games/<id>/runtime/, e.g. Patapon's allocator overrides)
// reach the generic bump heap ONLY through these; the heap statics stay
// private to psp_hle_kernel_memory.cpp and core carries no game knowledge.

/// Observer invoked after every successful sceKernelAllocPartitionMemory
/// with the block's name, address, and (256-byte-aligned) size. One slot;
/// installed by the game module's register_hooks (nullptr = no observer).
using PspPartitionAllocObserver =
    void (*)(const char* name, uint32_t addr, uint32_t size);
void psp_kmem_set_partition_alloc_observer(PspPartitionAllocObserver fn);

/// Bump-allocate exactly `size` bytes (caller pre-rounds; the heap position
/// itself is NOT realigned — byte-identical to the historical layout).
/// Returns the PSP address, or 0 on OOM / size==0.
uint32_t psp_kmem_bump_alloc(uint32_t size);

/// Claim everything left in the bump heap: returns [start, end) and
/// advances the heap position to end, so later partition allocations
/// cannot overlap the claimed range.
void psp_kmem_reserve_remaining(uint32_t* start, uint32_t* end);

// ---- Per-Module Registration Functions ----
// Each HLE module file provides a registration function.
// Called by psp_hle_register_all_modules() during init.
void psp_hle_register_all_modules();

// Kernel modules (04-02, 04-03)
void psp_hle_register_kernel_thread();
void psp_hle_register_kernel_memory();
void psp_hle_register_kernel_sema();
void psp_hle_register_kernel_mutex();
void psp_hle_register_kernel_lwmutex();
void psp_hle_register_kernel_eventflag();

// I/O module (04-04)
void psp_hle_register_io();

// Display, GE, power, ctrl, utility (04-05)
void psp_hle_register_display();
void psp_hle_register_ge();
void psp_hle_register_power();
void psp_hle_register_ctrl();
void psp_hle_register_utility();

// SAS voice state machine (issue #29)
void psp_hle_register_sas();

