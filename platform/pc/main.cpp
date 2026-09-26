#include "psprecomp/common.hpp"
#include "psprecomp/elf32.hpp"
#include "psprecomp/runtime.hpp"

#include "p3p3ds/kernel_state.hpp"
#include "p3p3ds/hle/hle_modules.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../recomp/Yakumo/profiles/mhp3rd/third_party/stb_image_write.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace psprecomp {
// Defined in the generated AOT translation unit
void register_generated_functions(Runtime &runtime);
}

namespace {

constexpr std::uint32_t kExpectedEntryPc = 0x08804108u;
constexpr std::uint32_t kExpectedStopPc  = 0x08B19890u;
constexpr std::uint32_t kExpectedReturnRa = 0x08B19BE0u;
constexpr std::string_view kExpectedStopReason = "No recompiled function registered at 0x08B19890";
constexpr std::uint32_t kExpectedSdkVersion = 0x06020010u;
constexpr std::uint32_t kExpectedCompilerVersion = 0x00030306u;
constexpr std::int32_t kExpectedThreadUid = 2;
constexpr std::string_view kExpectedThreadName = "user_main";

void print_registers(const psprecomp::AllegrexContext &ctx) {
    static const char *const kGprNames[32] = {
        "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
        "t0",   "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7",
        "t8",   "t9", "k0", "k1", "gp", "sp", "fp", "ra"
    };

    std::cout << "\n=== Guest Register State ===\n";
    std::cout << "  PC: " << psprecomp::hex32(ctx.pc) << "\n";
    for (std::size_t i = 0; i < 32; ++i) {
        if (i % 4 == 0) std::cout << "  ";
        std::cout << std::left << std::setw(4) << kGprNames[i] << " = "
                  << psprecomp::hex32(ctx.gpr[i]) << "  ";
        if (i % 4 == 3) std::cout << "\n";
    }
    std::cout << std::right;
}

struct MilestoneVerificationResult {
    bool passed{false};
    bool stopped{false};
    bool entry_matched{false};
    bool pc_matched{false};
    bool ra_matched{false};
    bool thread_uid_matched{false};
    bool thread_name_matched{false};
    bool sdk_version_matched{false};
    bool compiler_version_matched{false};
    bool stop_reason_matched{false};
    std::string failure_detail;
};

MilestoneVerificationResult verify_milestone(
    std::uint32_t entry_addr,
    const psprecomp::Runtime &runtime,
    const p3p3ds::KernelState &kernel)
{
    MilestoneVerificationResult res;
    res.stopped = runtime.stopped();
    res.entry_matched = (entry_addr == kExpectedEntryPc);
    res.pc_matched = (runtime.cpu().pc == kExpectedStopPc);
    res.ra_matched = (runtime.cpu().gpr[31] == kExpectedReturnRa);
    res.thread_uid_matched = (kernel.threads().current_thread_id() == kExpectedThreadUid);
    res.thread_name_matched = (kernel.threads().current_thread() != nullptr &&
                               kernel.threads().current_thread()->name == kExpectedThreadName);
    res.sdk_version_matched = (kernel.compiled_sdk_version() == kExpectedSdkVersion);
    res.compiler_version_matched = (kernel.compiler_version() == kExpectedCompilerVersion);

    const std::string &reason = runtime.stop_reason();
    res.stop_reason_matched = (reason.find(kExpectedStopReason) != std::string::npos);

    if (!res.stopped) {
        res.failure_detail = "Runtime did not stop (dispatch budget exhausted without hitting HLE/stop)";
    } else if (!res.entry_matched) {
        res.failure_detail = "Entry PC mismatch: expected " + psprecomp::hex32(kExpectedEntryPc) +
                             ", got " + psprecomp::hex32(entry_addr);
    } else if (!res.sdk_version_matched) {
        res.failure_detail = "Kernel SDK version mismatch: expected " + psprecomp::hex32(kExpectedSdkVersion) +
                             ", got " + psprecomp::hex32(kernel.compiled_sdk_version());
    } else if (!res.compiler_version_matched) {
        res.failure_detail = "Kernel compiler version mismatch: expected " + psprecomp::hex32(kExpectedCompilerVersion) +
                             ", got " + psprecomp::hex32(kernel.compiler_version());
    } else if (!res.pc_matched) {
        res.failure_detail = "Stop PC mismatch: expected " + psprecomp::hex32(kExpectedStopPc) +
                             ", got " + psprecomp::hex32(runtime.cpu().pc);
    } else if (!res.ra_matched) {
        res.failure_detail = "Return address ($ra) mismatch: expected " + psprecomp::hex32(kExpectedReturnRa) +
                             ", got " + psprecomp::hex32(runtime.cpu().gpr[31]);
    } else if (!res.thread_uid_matched) {
        res.failure_detail = "Current thread UID mismatch: expected " + std::to_string(kExpectedThreadUid) +
                             ", got " + std::to_string(kernel.threads().current_thread_id());
    } else if (!res.thread_name_matched) {
        res.failure_detail = "Current thread name mismatch: expected " + std::string(kExpectedThreadName) +
                             ", got " + (kernel.threads().current_thread() ? kernel.threads().current_thread()->name : "null");
    } else if (!res.stop_reason_matched) {
        res.failure_detail = "Stop reason mismatch: expected \"" + std::string(kExpectedStopReason) +
                             "\", got: \"" + reason + "\"";
    } else {
        res.passed = true;
    }
    return res;
}

void inspect_and_dump_framebuffers(const psprecomp::GuestMemory &memory, const p3p3ds::KernelState &kernel) {
    std::cout << "\n====================================================\n";
    std::cout << "   P3P3DS GUEST VRAM & FRAMEBUFFER PROBE\n";
    std::cout << "====================================================\n";

    const auto &disp = kernel.display().info();
    std::cout << "Display State:\n";
    std::cout << "  Active:           " << (disp.active ? "YES" : "NO") << "\n";
    std::cout << "  Registered Addr:  0x" << std::hex << disp.topaddr << std::dec << "\n";
    std::cout << "  Dimensions:       " << disp.width << "x" << disp.height << "\n";
    std::cout << "  Buffer Stride:    " << disp.bufferwidth << " pixels\n";
    std::cout << "  Pixel Format:     " << disp.pixelformat << " (3 = RGBA8888)\n";
    std::cout << "  Sync Mode:        " << disp.sync << "\n\n";

    constexpr std::uint32_t kBuf0Addr = 0x04000000u;
    constexpr std::uint32_t kBuf1Addr = 0x04088000u;
    constexpr std::uint32_t kVisibleWidth = 480u;
    constexpr std::uint32_t kVisibleHeight = 272u;
    constexpr std::uint32_t kStridePixels = 512u;
    constexpr std::uint32_t kBytesPerPixel = 4u;
    constexpr std::size_t kFbTotalBytes = static_cast<std::size_t>(kStridePixels) * kVisibleHeight * kBytesPerPixel; // 557,056 bytes (0x88000)

    // Helper to probe a specific buffer
    auto probe_buf = [&](std::uint32_t addr, std::string_view label) {
        std::cout << "--- Probing " << label << " at 0x" << std::hex << addr << std::dec << " ---\n";
        const bool backed = memory.contains(addr, kFbTotalBytes);
        std::cout << "  Backed by VRAM:   " << (backed ? "YES" : "NO") << "\n";
        if (!backed) return;

        const std::uint8_t *raw = memory.raw_pointer(addr, kFbTotalBytes);
        std::cout << "  Raw pointer:      " << (raw != nullptr ? "VALID" : "NULL") << "\n";
        if (raw == nullptr) return;

        std::size_t non_zero_bytes = 0;
        std::size_t first_non_zero = static_cast<std::size_t>(-1);
        std::size_t last_non_zero = 0;
        for (std::size_t i = 0; i < kFbTotalBytes; ++i) {
            if (raw[i] != 0) {
                ++non_zero_bytes;
                if (first_non_zero == static_cast<std::size_t>(-1)) first_non_zero = i;
                last_non_zero = i;
            }
        }

        std::cout << "  Buffer size:      " << kFbTotalBytes << " bytes (0x" << std::hex << kFbTotalBytes << std::dec << ")\n";
        std::cout << "  Non-zero bytes:   " << non_zero_bytes << " / " << kFbTotalBytes
                  << " (" << std::fixed << std::setprecision(2) << (100.0 * non_zero_bytes / kFbTotalBytes) << "%)\n";

        if (non_zero_bytes > 0) {
            std::cout << "  First non-zero:   offset 0x" << std::hex << first_non_zero
                      << " (byte value: 0x" << static_cast<int>(raw[first_non_zero]) << ")" << std::dec << "\n";
            std::cout << "  Last non-zero:    offset 0x" << std::hex << last_non_zero
                      << " (byte value: 0x" << static_cast<int>(raw[last_non_zero]) << ")" << std::dec << "\n";

            // Sample some non-zero pixels
            std::cout << "  Sample non-zero pixels:\n";
            int samples_printed = 0;
            for (std::size_t p = 0; p < kFbTotalBytes / 4 && samples_printed < 5; ++p) {
                const std::uint32_t px = *reinterpret_cast<const std::uint32_t *>(raw + p * 4);
                if (px != 0) {
                    const std::uint32_t y = static_cast<std::uint32_t>(p / kStridePixels);
                    const std::uint32_t x = static_cast<std::uint32_t>(p % kStridePixels);
                    std::cout << "    [x=" << x << ", y=" << y << "]: 0x" << std::hex << px
                              << " (R=" << (px & 0xFF) << ", G=" << ((px >> 8) & 0xFF)
                              << ", B=" << ((px >> 16) & 0xFF) << ", A=" << ((px >> 24) & 0xFF) << ")\n" << std::dec;
                    ++samples_printed;
                }
            }

            // Convert stride 512 to contiguous 480x272 RGBA
            std::vector<std::uint8_t> visible_pixels(kVisibleWidth * kVisibleHeight * kBytesPerPixel, 0);
            for (std::uint32_t y = 0; y < kVisibleHeight; ++y) {
                const std::uint8_t *src_row = raw + y * kStridePixels * kBytesPerPixel;
                std::uint8_t *dst_row = visible_pixels.data() + y * kVisibleWidth * kBytesPerPixel;
                std::memcpy(dst_row, src_row, kVisibleWidth * kBytesPerPixel);
            }

            std::filesystem::create_directories(".tmp");
            std::string out_png = ".tmp/p3p_framebuffer_0x" + psprecomp::hex32(addr).substr(2) + ".png";
            int res = stbi_write_png(out_png.c_str(), kVisibleWidth, kVisibleHeight, 4,
                                     visible_pixels.data(), kVisibleWidth * kBytesPerPixel);
            if (res != 0) {
                std::cout << "  -> DUMPED FRAMEBUFFER PNG: " << out_png << " (SUCCESS)\n";
            } else {
                std::cerr << "  -> FAILED to write PNG to " << out_png << "\n";
            }
        } else {
            std::cout << "  Status:           EMPTY (All 0x00 bytes)\n";
        }
    };

    probe_buf(kBuf0Addr, "Buffer 0 (Primary / Front Target)");
    probe_buf(kBuf1Addr, "Buffer 1 (Alternate / sceDisplaySetFrameBuf Target)");

    // Also scan entire 2 MiB VRAM
    const auto &vram = memory.vram_bytes();
    std::size_t vram_non_zero = 0;
    for (std::uint8_t b : vram) {
        if (b != 0) ++vram_non_zero;
    }
    std::cout << "\n--- Entire 2 MiB PSP VRAM Summary ---\n";
    std::cout << "  VRAM total size:  " << vram.size() << " bytes (2 MiB)\n";
    std::cout << "  Non-zero bytes:   " << vram_non_zero << " / " << vram.size()
              << " (" << std::fixed << std::setprecision(2) << (100.0 * vram_non_zero / vram.size()) << "%)\n";

    // Trace submitted GE display lists
    auto dump_ge_list = [&](std::uint32_t addr, std::size_t count, std::string_view label) {
        const std::uint32_t c = psprecomp::GuestMemory::canonical(addr);
        std::cout << "\n--- GE Display List Inspection: " << label << " (0x" << std::hex << addr << " -> 0x" << c << ") ---\n" << std::dec;
        if (!memory.contains(c, count * 4)) {
            std::cout << "  Not inside guest memory!\n";
            return;
        }
        for (std::size_t i = 0; i < count; ++i) {
            const std::uint32_t w = memory.load32(c + static_cast<std::uint32_t>(i) * 4);
            const std::uint32_t op = (w >> 24) & 0xFFu;
            const std::uint32_t arg = w & 0x00FFFFFFu;
            std::cout << "  [" << std::right << std::setw(2) << std::setfill('0') << i << "] 0x"
                      << std::hex << std::right << std::setw(8) << std::setfill('0') << w
                      << " -> OP=0x" << std::right << std::setw(2) << std::setfill('0') << op
                      << ", ARG=0x" << std::right << std::setw(6) << std::setfill('0') << arg << std::dec;

            // Decode known GE opcodes
            switch (op) {
            case 0x00: std::cout << " (NOP)"; break;
            case 0x01: std::cout << " (VADDR: 0x" << std::hex << arg << std::dec << ")"; break;
            case 0x02: std::cout << " (IADDR: 0x" << std::hex << arg << std::dec << ")"; break;
            case 0x04: std::cout << " (PRIM: count=" << (arg & 0xFFFF) << ", type=" << ((arg >> 16) & 7) << ")"; break;
            case 0x08: std::cout << " (JUMP: 0x" << std::hex << arg << std::dec << ")"; break;
            case 0x0A: std::cout << " (CALL: 0x" << std::hex << arg << std::dec << ")"; break;
            case 0x0B: std::cout << " (RET)"; break;
            case 0x0C: std::cout << " (END)"; break;
            case 0x0E: std::cout << " (SIGNAL)"; break;
            case 0x0F: std::cout << " (FINISH)"; break;
            case 0x10: std::cout << " (BASE: 0x" << std::hex << arg << std::dec << ")"; break;
            case 0x12: std::cout << " (VERTEXTYPE: 0x" << std::hex << arg << std::dec << ")"; break;
            case 0x13: std::cout << " (OFFSETADDR: 0x" << std::hex << arg << std::dec << ")"; break;
            case 0x14: std::cout << " (ORIGIN)"; break;
            case 0x15: std::cout << " (DRAWBOUNDINGBOX)"; break;
            case 0x16: std::cout << " (VSCX)"; break;
            case 0x9C: std::cout << " (FRAMEBUFPTR: 0x" << std::hex << arg << std::dec << ")"; break;
            case 0x9D: std::cout << " (FRAMEBUFWIDTH: " << arg << ")"; break;
            case 0xD2: std::cout << " (FRAMEBUFPIXFORMAT: " << arg << ")"; break;
            case 0xD3: std::cout << " (CLEARMODE: color=" << (arg & 1) << ", alpha/stencil=" << ((arg >> 1) & 1) << ", depth=" << ((arg >> 2) & 1) << ")"; break;
            case 0xD4: std::cout << " (SCISSOR1)"; break;
            case 0xD5: std::cout << " (SCISSOR2)"; break;
            case 0xD6: std::cout << " (MINZ)"; break;
            case 0xD7: std::cout << " (MAXZ)"; break;
            default: break;
            }
            std::cout << "\n";
            if (op == 0x0B || op == 0x0C) break;
        }
    };

    dump_ge_list(0x08BB40D4u, 32, "Initial GE Static List (0x08BB40D4)");
    dump_ge_list(0x48D14600u, 32, "Dynamic GE Display List (0x48D14600 / 0x08D14600)");
    dump_ge_list(0x48D14630u, 32, "Dynamic GE Display List Offset (0x48D14630 / 0x08D14630)");

    std::cout << "====================================================\n\n";
}

} // namespace

int main(int argc, char **argv) {
    std::filesystem::path elf_path = "profiles/p3p/game/eboot.elf";
    std::uint64_t max_dispatches = 1000u;
    bool verbose = false;
    bool verify_mode = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg == "--elf" && i + 1 < argc) {
            elf_path = argv[++i];
        } else if (arg == "--max-dispatches" && i + 1 < argc) {
            max_dispatches = std::stoull(argv[++i]);
        } else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--verify-milestone" || arg == "--verify") {
            verify_mode = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: p3p_pc_bootstrap [options]\n"
                      << "  --elf <path>            Path to decrypted P3P ELF (default: profiles/p3p/game/eboot.elf)\n"
                      << "  --max-dispatches <N>    Maximum dispatch count (default: 1000)\n"
                      << "  --verify-milestone      Strict milestone verification (returns 0 only on exact milestone match)\n"
                      << "  --verbose, -v           Enable verbose runtime traces\n";
            return 0;
        }
    }

    std::cout << "====================================================\n";
    std::cout << "   P3P3DS PC Bootstrap Execution Harness\n";
    std::cout << "====================================================\n";
    std::cout << "Target ELF:       " << elf_path.string() << "\n";
    std::cout << "Dispatch budget:  " << max_dispatches << "\n";
    std::cout << "Verify mode:      " << (verify_mode ? "STRICT (--verify-milestone)" : "standard") << "\n";

    try {
        // 1. Load decrypted P3P ELF
        if (!std::filesystem::exists(elf_path)) {
            std::cerr << "Error: ELF file not found at " << elf_path.string() << "\n";
            return 1;
        }
        const auto elf = psprecomp::Elf32Image::from_file(elf_path);
        const std::uint32_t load_base = psprecomp::kDefaultPspUserLoadBase;
        const std::uint32_t entry_addr = elf.runtime_entry(load_base);

        std::cout << "ELF Type:         " << elf.type()
                  << (elf.is_psp_prx() ? " (PSP PRX relocatable)" : " (Static ELF)") << "\n";
        std::cout << "Runtime Entry:    " << psprecomp::hex32(entry_addr) << "\n";

        // 2. Initialize PSPRecomp Runtime with 32 MiB PSP RAM
        psprecomp::Runtime runtime(32u * 1024u * 1024u);

        // 3. Load & relocate ELF into GuestMemory
        const auto rel_stats = elf.load_and_relocate(runtime.memory(), load_base);
        std::cout << "Relocations:      " << rel_stats.total << " total (R_26="
                  << rel_stats.r_mips_26 << ", R_32=" << rel_stats.r_mips_32
                  << ", R_HI=" << rel_stats.r_mips_hi16 << ", R_LO=" << rel_stats.r_mips_lo16 << ")\n";

        // 4. Find relocated PSP module info
        const auto module = elf.find_module_info(runtime.memory(), load_base);
        if (!module) {
            std::cerr << "Error: Failed to find relocated PSP module info in ELF\n";
            return 2;
        }
        std::cout << "Module Info:      " << module->name << " v"
                  << static_cast<int>(module->major_version) << "."
                  << static_cast<int>(module->minor_version) << "\n";
        std::cout << "Module GP:        " << psprecomp::hex32(module->gp) << "\n";
        std::cout << "Module Stubs:     " << psprecomp::hex32(module->stub_top)
                  << " - " << psprecomp::hex32(module->stub_end) << "\n";

        // 5. Set $gp from module info
        runtime.cpu().set_gpr(28, module->gp);

        // 6. Setup minimal valid module_start stack
        constexpr std::uint32_t kStackTop = 0x0A000000u;
        constexpr std::uint32_t kStackSize = 0x10000u; // 64 KiB
        constexpr std::uint32_t kStackBottom = kStackTop - kStackSize;
        const std::uint32_t initial_sp = kStackTop - 0x100u;

        runtime.memory().zero(kStackBottom, kStackSize);
        std::cout << "Stack Arena:      " << psprecomp::hex32(kStackBottom)
                  << " - " << psprecomp::hex32(kStackTop) << "\n";

        // 7. Set $sp and $k0 (thread context block at stack top - 256)
        runtime.cpu().set_gpr(29, initial_sp);
        runtime.cpu().set_gpr(26, kStackTop - 0x100u); // $k0

        // 8. Set $ra = thread return sentinel, $a0 = 0, $a1 = 0
        runtime.cpu().set_gpr(31, p3p3ds::hle::ThreadManager::kThreadReturnSentinel);
        runtime.cpu().set_gpr(4, 0u);
        runtime.cpu().set_gpr(5, 0u);

        std::cout << "Initial SP:       " << psprecomp::hex32(initial_sp) << "\n";
        std::cout << "Initial RA:       " << psprecomp::hex32(p3p3ds::hle::ThreadManager::kThreadReturnSentinel)
                  << " (Thread return sentinel)\n";
        std::cout << "Initial A0 / A1:  0x00000000 / 0x00000000\n";

        // 9. Register generated recompiled functions and import wrappers
        psprecomp::register_generated_functions(runtime);
        std::cout << "Registered Entries: " << runtime.function_count()
                  << " (functions, block labels, and import wrappers)\n";

        // 10. Register target-agnostic HLE service modules
        p3p3ds::KernelState kernel_state;
        kernel_state.threads().init_root_thread("root", entry_addr, initial_sp, module->gp);
        p3p3ds::hle::register_all_hle_modules(runtime, kernel_state);

        if (verbose) {
            std::cout << "\nStarting execution at " << psprecomp::hex32(entry_addr) << "...\n";
        }

        // 11. Execute recompiled code
        runtime.run(entry_addr, max_dispatches);

        // 12. Diagnostic summary
        std::cout << "\n=== Execution Result ===\n";
        std::cout << "Stop Reason:      " << runtime.stop_reason() << "\n";
        std::cout << "Stopped:          " << (runtime.stopped() ? "yes" : "no") << "\n";
        std::cout << "Final Guest PC:   " << psprecomp::hex32(runtime.cpu().pc) << "\n";
        std::cout << "Kernel SDK Ver:   " << psprecomp::hex32(kernel_state.compiled_sdk_version()) << "\n";

        print_registers(runtime.cpu());

        // 12b. Inspect Guest VRAM and dump real framebuffers if present
        inspect_and_dump_framebuffers(runtime.memory(), kernel_state);

        // 13. Milestone Verification
        const auto v = verify_milestone(entry_addr, runtime, kernel_state);
        std::cout << "\n=== Milestone Verification ===\n";
        std::cout << "Target:           module_start -> user_main -> GE/Display init -> sub_08B19890\n";
        std::cout << "Entry (0x" << std::hex << kExpectedEntryPc << "):   "
                  << (v.entry_matched ? "OK" : "FAILED") << "\n";
        std::cout << "SDK Ver (0x" << std::hex << kExpectedSdkVersion << "): "
                  << (v.sdk_version_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Compiler Ver (0x" << std::hex << kExpectedCompilerVersion << "): "
                  << (v.compiler_version_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Final PC (0x" << std::hex << kExpectedStopPc << "): "
                  << (v.pc_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Return RA (0x" << std::hex << kExpectedReturnRa << "):"
                  << (v.ra_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Thread UID (" << std::dec << kExpectedThreadUid << "):      "
                  << (v.thread_uid_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Thread Name (" << kExpectedThreadName << "): "
                  << (v.thread_name_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Stop Reason:      " << (v.stop_reason_matched ? "OK" : "FAILED") << "\n";
        std::cout << "Result:           " << (v.passed ? "[VERIFIED] Milestone passed" : "[FAILED] " + v.failure_detail) << "\n";

        if (v.passed) {
            std::cout << "\nExecution milestone reached and verified successfully.\n";
            return 0;
        } else {
            std::cerr << "\nMilestone verification failed: " << v.failure_detail << "\n";
            return 3;
        }

    } catch (const std::exception &e) {
        std::cerr << "Fatal Exception: " << e.what() << "\n";
        return 1;
    }
}
