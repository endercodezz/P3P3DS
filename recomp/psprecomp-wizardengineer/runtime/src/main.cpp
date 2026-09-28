#include "recomp.h"
#include "funcs.h"
#include "psp_memory.h"
#include "psp_scheduler.h"
#include "psp_render_queue.h"
#include "psp_event_loop.h"
#include "psp_runtime.h"
#include "psp_ge.h"
#include "psp_ge_draw.h"
#include "psp_ge_texture.h"
#include "psp_ge_test.h"
#include "hle/psp_hle.h"
#include "hle/psp_hle_io.h"
#include "psp_debug_socket.h"
#include "psp_game_module.h"
#include "psp_vfpu.h"  // vfpu_init_context — VFPU prefix reset default

// Build fingerprint of the output/ this binary was generated from (issue #36).
// Guarded so pre-fingerprint output dirs still build (warning at configure
// time, "unavailable" banner at boot).
#if __has_include("recomp_fingerprint.h")
#include "recomp_fingerprint.h"
#endif

// Generated module facts (issue #47 Phase 2): RECOMP_MODULE_* / RECOMP_SEG0_*
// / RECOMP_HEAP_BASE / RECOMP_CTOR_* — per-game boot constants sourced from
// analysis.json. Hard include: recompile refuses to produce an output/ that
// lacks it, so a missing header means a stale output dir (regenerate it).
#include "recomp_module.h"

// Generated per-game choices (issues #46/#47 Phase 4): RECOMP_GAME_ID /
// RECOMP_BOOT_PATH / RECOMP_HEAP_OVERRIDE / RECOMP_ASSET_LAYER_BND from the
// --config manifest. Guarded so output dirs generated before Phase 4 still
// build; the fallbacks ARE the zero-manifest defaults.
#if __has_include("recomp_game_config.h")
#include "recomp_game_config.h"
#endif
#ifndef RECOMP_GAME_ID
#define RECOMP_GAME_ID ""
#endif
#ifndef RECOMP_BOOT_PATH
#define RECOMP_BOOT_PATH "disc0:/PSP_GAME/SYSDIR/BOOT.BIN"
#endif

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

// From emitter output (dispatch.cpp, data_sections.cpp)
extern void psp_init_dispatch_table();
extern void psp_init_data_sections(uint8_t* rdram);

// module_start entry point — the emitter canonically names the function at
// the module entry address (RECOMP_MODULE_ENTRY) "entry" for every input
// (issue #52 T5.7), so this hard extern is a contract, not a Ghidra accident.
extern RECOMP_FUNC void entry(uint8_t* rdram, recomp_context* ctx);

/// main — boot sequence with critical ordering:
///   signal handlers -> memory -> dispatch -> data_sections -> SDL/GL ->
///   constructors -> scheduler -> module_start -> event_loop -> shutdown
///
/// The ordering guarantees:
/// - Data sections loaded BEFORE constructors (constructors read .data/.rodata)
/// - Constructors run BEFORE module_start (static initializers set up vtables)
/// - SDL/GL initialized BEFORE module_start (render queue can accept requests)
/// - Event loop runs AFTER module_start returns (module_start spawns threads)
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // 0. Identify the output/ this binary was built against (issue #36).
    //    One grep-stable line; the configure-time fingerprint check is the
    //    enforcement, this is the audit trail in every run log.
#ifdef RECOMP_FINGERPRINT_HASH
    std::fprintf(stderr,
        "[RT] output fingerprint: %s (cross_mid=%d, recompiled %s)\n",
        RECOMP_FINGERPRINT_HASH,
        RECOMP_FINGERPRINT_CROSS_MID,
        RECOMP_FINGERPRINT_TIMESTAMP);
#else
    std::fprintf(stderr,
        "[RT] output fingerprint: unavailable (output/ predates issue #36)\n");
#endif

    // 0b. Identify the compiled-in game module (issues #46/#47 Phase 4) and
    //     cross-check it against the output dir's manifest id. A mismatch is
    //     not fatal (generic boot may be intentional) but must be loud:
    //     hooks compiled for one title silently corrupt another.
    const PspGameModule* game = psp_game_module();
    std::fprintf(stderr, "[RT] game module: %s\n",
        game->id[0] != '\0' ? game->id : "(none — generic build)");
    if (std::strcmp(game->id, RECOMP_GAME_ID) != 0) {
        std::fprintf(stderr,
            "[RT] WARNING: game-module mismatch — runtime compiled with "
            "PSPRECOMP_GAME='%s' but this output dir was recompiled with "
            "manifest id '%s'. Game hooks may target the wrong binary.\n",
            game->id, RECOMP_GAME_ID);
    }

    // 1. Install SIGTERM/SIGINT signal handlers
    psp_install_signal_handlers();

    // 2. Allocate PSP memory (128MB sparse mmap)
    uint8_t* rdram = psp_memory_init();
    if (!rdram) {
        std::fprintf(stderr, "[RT] psp_memory_init failed\n");
        return 1;
    }
    std::fprintf(stderr,
        "[RT] Memory initialized (%zu MB)\n",
        PSP_MEM_SIZE / (1024 * 1024));

    // 2b. Start TCP debug socket (loopback port 9999) for external memory
    //     reads/writes, info queries, button injection, and screenshots.
    //     The hooks give the I and S commands read-only views into the GE,
    //     dispatch, and scheduler subsystems (issue #35).
    psp_debug_socket_start(rdram, PSP_MEM_SIZE, 9999);
    {
        PspDebugHooks hooks;
        hooks.ge_stats           = ge_draw_get_stats;
        hooks.lookup_miss_stats  = psp_dispatch_get_miss_stats;
        hooks.recent_funcs       = psp_dispatch_get_recent_funcs;
        hooks.thread_list        = psp_scheduler_snapshot;
        hooks.capture_screenshot = ge_draw_capture_screenshot;
        psp_debug_socket_set_hooks(hooks);
    }

    // 3. Populate dispatch table (must be before any RECOMP_LOOKUP)
    psp_init_dispatch_table();
    std::fprintf(stderr, "[RT] Dispatch table initialized\n");

    // 3b. Initialize I/O subsystem (disc0 path mapping)
    //     PSPRECOMP_DISC0 env var overrides default ./disc0 path
    const char* disc0_env = std::getenv("PSPRECOMP_DISC0");
    const char* disc0_path = disc0_env ? disc0_env : "./disc0";
    {
        struct stat disc0_stat;
        if (::stat(disc0_path, &disc0_stat) != 0) {
            std::fprintf(stderr,
                "[RT] WARNING: disc0 path '%s' does not exist. "
                "Set PSPRECOMP_DISC0=/path/to/extracted/iso "
                "to provide game data.\n", disc0_path);
        } else {
            std::fprintf(stderr,
                "[RT] disc0 path: %s\n", disc0_path);
        }
    }
    psp_io_init(disc0_path);

    // 3c. Initialize HLE subsystem — overrides dispatch entries for
    //     237 import stubs with HLE implementations
    psp_hle_init();
    std::fprintf(stderr, "[RT] HLE subsystem initialized\n");

    // 3d. Game-module hooks (issues #46/#47 Phase 4). Everything
    //    address-keyed or title-specific — dispatch wrappers, allocator/CRT
    //    override inits, asset-layer init — registers through this seam.
    //    PSPRECOMP_GAME selects the module at build time; the generic build
    //    (-DPSPRECOMP_GAME=none) registers nothing here.
    game->register_hooks(rdram);

    // 4. RUNTIME-03: Copy .data/.rodata into rdram
    psp_init_data_sections(rdram);
    std::fprintf(stderr, "[RT] Data sections loaded\n");

    // 4b. Populate boot module NativeModule struct in kernel memory
    //     196-byte struct at BOOT_MODULE_ADDR per PPSSPP sceKernelModule.h
    {
        uint32_t mod_base = BOOT_MODULE_ADDR & PSP_ADDR_MASK;
        std::memset(rdram + mod_base, 0, NATIVE_MODULE_SIZE);

        // +0x00: next = NULL (linked list pointer)
        psp_mem_write<uint32_t>(rdram, BOOT_MODULE_ADDR + 0x00, 0);
        // +0x04: attribute = 0x0002 (user mode)
        psp_mem_write<uint16_t>(rdram, BOOT_MODULE_ADDR + 0x04,
            0x0002);
        // +0x06: version[0] = 0x01
        psp_mem_write<uint8_t>(rdram, BOOT_MODULE_ADDR + 0x06, 0x01);
        // +0x07: version[1] = 0x01
        psp_mem_write<uint8_t>(rdram, BOOT_MODULE_ADDR + 0x07, 0x01);
        // +0x08: name (28-byte field) — SceModuleInfo name from the binary
        // (issue #47 Phase 2: all values below come from recomp_module.h)
        const char* mod_name = RECOMP_MODULE_NAME;
        std::memcpy(
            rdram + ((BOOT_MODULE_ADDR + 0x08) & PSP_ADDR_MASK),
            mod_name, std::strlen(mod_name) + 1);
        // +0x24: status = 5 (MODULE_STATUS_STARTED)
        psp_mem_write<uint32_t>(rdram, BOOT_MODULE_ADDR + 0x24, 5);
        // +0x2C: modid = BOOT_MODULE_UID
        psp_mem_write<uint32_t>(rdram, BOOT_MODULE_ADDR + 0x2C,
            static_cast<uint32_t>(BOOT_MODULE_UID));
        // +0x50: module_start_func
        psp_mem_write<uint32_t>(rdram, BOOT_MODULE_ADDR + 0x50,
            RECOMP_MODULE_ENTRY);
        // +0x64: entry_addr
        psp_mem_write<uint32_t>(rdram, BOOT_MODULE_ADDR + 0x64,
            RECOMP_MODULE_ENTRY);
        // +0x68: gp_value
        psp_mem_write<uint32_t>(rdram, BOOT_MODULE_ADDR + 0x68,
            RECOMP_MODULE_GP);
        // +0x6C: text_addr
        psp_mem_write<uint32_t>(rdram, BOOT_MODULE_ADDR + 0x6C,
            RECOMP_MODULE_TEXT_START);
        // +0x70: text_size (PPSSPP GetTotalTextSize semantics; the previous
        // hardcoding used the exec phdr's p_filesz — see DEBUGGING.md #47 P2)
        psp_mem_write<uint32_t>(rdram, BOOT_MODULE_ADDR + 0x70,
            RECOMP_MODULE_TEXT_SIZE);
        // +0x7C: nsegment = 1
        psp_mem_write<uint32_t>(rdram, BOOT_MODULE_ADDR + 0x7C, 1);
        // +0x80: segmentaddr[0]
        psp_mem_write<uint32_t>(rdram, BOOT_MODULE_ADDR + 0x80,
            RECOMP_SEG0_VADDR);
        // +0x90: segmentsize[0]
        psp_mem_write<uint32_t>(rdram, BOOT_MODULE_ADDR + 0x90,
            RECOMP_SEG0_MEMSZ);

        std::fprintf(stderr,
            "[RT] Boot module NativeModule at 0x%08X\n",
            BOOT_MODULE_ADDR);
    }

    // 5. Initialize SDL2 window + OpenGL 3.3 context (main thread)
    if (psp_runtime_init_sdl() != 0) {
        std::fprintf(stderr, "[RT] SDL/GL init failed\n");
        psp_memory_cleanup(rdram);
        return 1;
    }

    // 5b. Initialize GE subsystem (command decoder + state + draw)
    ge_init();
    ge_draw_init();  // FBO, VAO/VBO, shader (must be after GL init)
    // Note: ge_texture_init() is called inside ge_draw_init()'s
    // shader/FBO init sequence, but we also call it explicitly here
    // in case draw_init changes in the future.
    ge_texture_init();

    // 5c. GE self-test (if PSPRECOMP_GE_TEST=1)
    // When PSPRECOMP_GE_TEST_ONLY=1, capture the test frame and exit
    // before the game thread starts — this lets us verify the renderer
    // independently of game-side asset loading.
    {
        const char* ge_test_env =
            std::getenv("PSPRECOMP_GE_TEST");
        const char* ge_test_only_env =
            std::getenv("PSPRECOMP_GE_TEST_ONLY");
        bool want_test = (ge_test_env && ge_test_env[0] == '1')
                      || (ge_test_only_env && ge_test_only_env[0] == '1');
        bool test_only = (ge_test_only_env && ge_test_only_env[0] == '1');
        if (want_test) {
            bool pass = ge_run_self_test(rdram);
            if (pass) {
                std::fprintf(stderr,
                    "[RT] GE self-test PASS\n");
            } else {
                std::fprintf(stderr,
                    "[RT] GE self-test FAIL "
                    "(continuing anyway)\n");
            }
            if (test_only) {
                std::fprintf(stderr,
                    "[RT] PSPRECOMP_GE_TEST_ONLY=1: "
                    "capturing frame and exiting before game thread\n");
                // ge_draw_shutdown auto-captures frame.tga when
                // g_has_drawn_prims is true (which the GE self-test
                // sets). Process cleanup handles SDL teardown.
                ge_draw_shutdown();
                std::fprintf(stderr, "[RT] Exit 0\n");
                std::fflush(stderr);
                std::_Exit(0);
            }
        }
    }

    // 6. Create context for boot-time calls (constructors + module_start)
    recomp_context ctx{};
    // Zero-init leaves VFPU S/T prefixes at 0 (= "all lanes <- component 0"),
    // not the hardware-reset identity 0xE4; reset them so the boot thread's
    // first VFPU arithmetic op isn't silently corrupted (issue #27).
    vfpu_init_context(&ctx);
    // Boot context k0 area: 256 bytes at the very top of user RAM.
    // k0 register points to this area; usable SP starts below it.
    // k0+04 = heap descriptor (dlmalloc checks this to select heap)
    // k0+C0 = thread UID, k0+C8 = stack_top
    // k0+F8/FC = 0xFFFFFFFF (PPSSPP sentinels)
    uint32_t boot_k0 = PSP_USER_MEM_END - 1;  // 0x0BFFFFFF with Slim model
    uint32_t boot_k0_masked = boot_k0 & PSP_ADDR_MASK;
    std::memset(rdram + boot_k0_masked, 0, 0x100);
    // k0+4 heap descriptor: the generic boot writes 0 (an unconfigured
    // PPSSPP-style k0 area, plan decision P12). Title-specific values (e.g.
    // Patapon's dlmalloc default-heap descriptor) are written by the game
    // module's on_boot_context hook just before entry().
    psp_mem_write<uint32_t>(rdram, boot_k0 + 0x04, 0U);
    psp_mem_write<int32_t>(rdram, boot_k0 + 0xC0, 0);
    psp_mem_write<uint32_t>(rdram, boot_k0 + 0xC8, boot_k0);
    psp_mem_write<uint32_t>(rdram, boot_k0 + 0xF8, 0xFFFFFFFFU);
    psp_mem_write<uint32_t>(rdram, boot_k0 + 0xFC, 0xFFFFFFFFU);

    // Allocate newlib _reent structure for the boot thread.
    // PSP __getreent() reads from k0+0x00; if NULL, newlib asserts
    // "no reent structure found" and calls sceKernelExitThread(1).
    {
        uint32_t reent_addr =
            psp_alloc_kernel_memory(1024);
        if (reent_addr != 0) {
            std::memset(
                rdram + (reent_addr & PSP_ADDR_MASK),
                0, 1024);
            psp_mem_write<uint32_t>(
                rdram, boot_k0 + 0x00, reent_addr);
            std::fprintf(stderr,
                "[RT] Boot _reent at 0x%08X\n", reent_addr);
        }
    }

    ctx.r[26] = static_cast<int32_t>(boot_k0);        // k0
    ctx.r[29] = static_cast<int32_t>(boot_k0 - 0x100); // SP below k0 area

    // 6b. Set GP register to module gp_value (PPSSPP __KernelSetupRootThread)
    ctx.r[28] = static_cast<int32_t>(RECOMP_MODULE_GP);  // GP = module gp_value

    // 6c. Copy boot path to stack as module_start arguments
    //     PPSSPP __KernelSetupRootThread pattern:
    //     r[4] = args (strlen + 1), r[5] = argp (SP pointer to path)
    {
        // Boot exec path: PSP-universal default, overridable per game via
        // the manifest's [boot] boot_path (-> generated recomp_game_config.h).
        const char* boot_path = RECOMP_BOOT_PATH;
        uint32_t path_len =
            static_cast<uint32_t>(std::strlen(boot_path) + 1);
        uint32_t aligned_len = (path_len + 0xFU) & ~0xFU;
        uint32_t sp = static_cast<uint32_t>(ctx.r[29]);
        sp -= aligned_len;
        std::memcpy(rdram + (sp & PSP_ADDR_MASK),
            boot_path, path_len);
        ctx.r[4] = static_cast<int32_t>(path_len);   // a0 = args
        ctx.r[5] = static_cast<int32_t>(sp);          // a1 = argp
        sp -= 64;  // safety margin (PPSSPP convention)
        ctx.r[29] = static_cast<int32_t>(sp);

        std::fprintf(stderr,
            "[RT] Boot args: path=\"%s\" len=%u SP=0x%08X\n",
            boot_path, path_len, sp);
    }

    // 7. Phase 3 Success Criterion 3: verify constructor ordering
    //    psp_init_data_sections() then psp_call_constructors() execute before
    //    module_start. The game's CRT inside module_start handles constructors
    //    via RECOMP_LOOKUP of .init_array pointers read from rdram.
    //    We verify: (a) first constructor address is in dispatch table,
    //    (b) data sections were loaded (non-zero bytes in .data region).
    {
        // (a) Verify the first constructor is in the dispatch table
        //     (fact-derived, issue #47 Phase 2: the address comes from
        //     analysis.json constructors[0] via recomp_module.h, and the
        //     probe uses psp_dispatch_probe_lookup — the plain RECOMP_LOOKUP
        //     never returns nullptr in non-STRICT mode, so the old check
        //     could not actually fail). Skipped for ctor-less modules.
        if (RECOMP_CTOR_COUNT > 0u) {
            FuncPtr first_ctor = psp_dispatch_probe_lookup(RECOMP_FIRST_CTOR);
            if (first_ctor == nullptr) {
                std::fprintf(stderr,
                    "FAIL: Criterion 3 — first constructor 0x%08X "
                    "not in dispatch table\n", RECOMP_FIRST_CTOR);
                return 1;
            }
        } else {
            std::fprintf(stderr,
                "[RT] Criterion 3: module has no constructors[]; "
                "ctor probe skipped\n");
        }
        // (b) Verify data sections were loaded: check non-zero byte at the
        //     start of the first load segment (fact-derived; if
        //     psp_init_data_sections was NOT called this region is all
        //     zeroes — segment starts are code/data, never a zero page).
        bool data_loaded = false;
        uint32_t seg0 = RECOMP_SEG0_VADDR & PSP_ADDR_MASK;
        for (uint32_t off = seg0; off < seg0 + 0x100; off++) {
            if (rdram[off] != 0) {
                data_loaded = true;
                break;
            }
        }
        if (!data_loaded) {
            std::fprintf(stderr,
                "FAIL: Criterion 3 — data sections appear "
                "empty (first 256 bytes of segment 0 at 0x%08X all zero)\n",
                RECOMP_SEG0_VADDR);
            return 1;
        }
        std::fprintf(stderr,
            "[RT] Criterion 3 PASS: data sections loaded, "
            "constructors callable\n");
    }

    // 8. Phase 3 Success Criterion 5: verify STRICT mode behavior
    //    PSPRECOMP_STRICT=1 must abort on LOOKUP_MISS.
    //    Patapon BOOT.BIN has 0 imports so no natural misses occur during
    //    module_start. We probe a known-invalid address (0xDEADBEEF) to
    //    exercise the STRICT path. In STRICT mode this aborts (expected);
    //    in non-STRICT mode it returns a noop stub and we continue.
    {
        const char* strict_env = std::getenv("PSPRECOMP_STRICT");
        bool strict_active = (strict_env && strict_env[0] == '1');

        if (strict_active) {
            std::fprintf(stderr,
                "[RT] Criterion 5: STRICT mode active — "
                "probing invalid address...\n");
            // This call will abort via psp_on_lookup_miss -> std::abort()
            // if STRICT mode is correctly implemented.
            RECOMP_LOOKUP(0xDEADBEEFU);
            // If we reach here, STRICT mode is broken
            std::fprintf(stderr,
                "FAIL: Criterion 5 — STRICT mode did not "
                "abort on LOOKUP_MISS\n");
            return 1;
        } else {
            // Non-STRICT: verify miss returns a callable stub (not nullptr)
            FuncPtr stub = RECOMP_LOOKUP(0xDEADBEEFU);
            if (stub == nullptr) {
                std::fprintf(stderr,
                    "FAIL: Criterion 5 — non-STRICT miss "
                    "returned nullptr instead of noop stub\n");
                return 1;
            }
            std::fprintf(stderr,
                "[RT] Criterion 5 PASS: non-STRICT miss "
                "returns noop stub (STRICT abort tested "
                "separately)\n");
        }
    }

    // 9. Initialize thread pool (scheduler)
    psp_scheduler_init();
    std::fprintf(stderr, "[RT] Scheduler initialized\n");

    // 9b. Game-module boot-context hook (issues #46/#47 Phase 4): the last
    //     title-specific touch point before module_start. The Patapon module
    //     writes its k0+4 dlmalloc heap descriptor and handles the opt-in
    //     PSPRECOMP_RUN_CTORS constructor walk here; the generic module is a
    //     no-op.
    game->on_boot_context(rdram, &ctx);

    // 10. Call module_start — typically creates game threads and returns
    std::fprintf(stderr,
        "[RT] Calling module_start (0x%08X)...\n",
        RECOMP_MODULE_ENTRY);
    entry(rdram, &ctx);
    std::fprintf(stderr, "[RT] module_start returned\n");

    // 11. RUNTIME-08: Enter event loop — blocks until shutdown
    psp_event_loop(rdram);

    // 12. RUNTIME-10: Clean shutdown
    psp_debug_socket_stop();
    ge_draw_shutdown();
    ge_texture_shutdown();
    ge_shutdown();
    psp_runtime_shutdown();

    psp_memory_cleanup(rdram);

    std::fprintf(stderr, "[RT] Exit 0\n");
    return 0;
}
