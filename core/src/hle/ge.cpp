#include "p3p3ds/hle/ge.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/runtime.hpp"

#include <iostream>

namespace p3p3ds::hle {

void register_ge_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    // sceGe_user::0xE47E40E4 - sceGeEdramGetAddr
    runtime.register_hle("sceGe_user", 0xE47E40E4u,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, kernel.ge().edram_base());
        });

    // sceGe_user::0x1F6752AD - sceGeEdramGetSize
    runtime.register_hle("sceGe_user", 0x1F6752ADu,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, kernel.ge().edram_size());
        });

    // sceGe_user::0xA4FC06A4 - sceGeSetCallback
    runtime.register_hle("sceGe_user", 0xA4FC06A4u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // Return callback ID 1
            ctx.set_gpr(2, 1u);
        });

    // sceGe_user::0x05DB22CE - sceGeUnsetCallback
    runtime.register_hle("sceGe_user", 0x05DB22CEu,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, 0u);
        });

    // sceGe_user::0xAB49E76A - sceGeListEnQueue
    runtime.register_hle("sceGe_user", 0xAB49E76Au,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t list_addr = ctx.gpr[4];
            const std::uint32_t stall_addr = ctx.gpr[5];
            const int cb_id = static_cast<int>(ctx.gpr[6]);
            const std::uint32_t opt = ctx.gpr[7];
            const std::uint32_t list_id = kernel.ge().enqueue_list(list_addr, stall_addr, cb_id, opt);
            std::cout << "[GE] sceGeListEnQueue(list=0x" << std::hex << list_addr
                      << ", stall=0x" << stall_addr << ", cb=" << std::dec << cb_id << ") -> id=" << list_id << "\n";
            // Dump initial commands from list
            const std::uint32_t canonical_list = psprecomp::GuestMemory::canonical(list_addr);
            if (rt.memory().contains(canonical_list, 32u)) {
                for (std::uint32_t i = 0; i < 16; ++i) {
                    const std::uint32_t cmd_word = rt.memory().load32(canonical_list + i * 4);
                    const std::uint32_t op = (cmd_word >> 24) & 0xFFu;
                    const std::uint32_t arg = cmd_word & 0x00FFFFFFu;
                    std::cout << "  GE[" << std::dec << i << "]: 0x" << std::hex << cmd_word
                              << " (OP=0x" << op << ", ARG=0x" << arg << ")" << std::dec << "\n";
                    if (op == 0x0B || op == 0x0C) break; // FINISH or END
                }
            }
            ctx.set_gpr(2, list_id);
        });

    // sceGe_user::0x1C0D95A6 - sceGeListEnQueueHead
    runtime.register_hle("sceGe_user", 0x1C0D95A6u,
        [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            const std::uint32_t list_addr = ctx.gpr[4];
            const std::uint32_t stall_addr = ctx.gpr[5];
            const int cb_id = static_cast<int>(ctx.gpr[6]);
            const std::uint32_t opt = ctx.gpr[7];
            const std::uint32_t list_id = kernel.ge().enqueue_list(list_addr, stall_addr, cb_id, opt);
            std::cout << "[GE] sceGeListEnQueueHead(list=0x" << std::hex << list_addr
                      << ", stall=0x" << stall_addr << ", cb=" << std::dec << cb_id << ") -> id=" << list_id << "\n";
            ctx.set_gpr(2, list_id);
        });

    // sceGe_user::0x03444EB4 - sceGeListSync
    runtime.register_hle("sceGe_user", 0x03444EB4u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // 0 = complete
            ctx.set_gpr(2, 0u);
        });

    // sceGe_user::0xB287BD61 - sceGeDrawSync
    runtime.register_hle("sceGe_user", 0xB287BD61u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            // 0 = complete
            ctx.set_gpr(2, 0u);
        });

    // sceGe_user::0xB448EC0D - sceGeBreak
    runtime.register_hle("sceGe_user", 0xB448EC0Du,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, 0u);
        });

    // sceGe_user::0x4C06E472 - sceGeContinue
    runtime.register_hle("sceGe_user", 0x4C06E472u,
        [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) {
            ctx.set_gpr(2, 0u);
        });

    // sceGe_user::0xE0D68148 - sceGeListUpdateStallAddr
    runtime.register_hle("sceGe_user", 0xE0D68148u,
        [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const int list_id = static_cast<int>(ctx.gpr[4]);
            const std::uint32_t stall_addr = ctx.gpr[5];
            std::cout << "[GE] sceGeListUpdateStallAddr(list=" << list_id
                      << ", stall=0x" << std::hex << stall_addr << std::dec << ")\n";
            const std::uint32_t canonical_stall = psprecomp::GuestMemory::canonical(stall_addr);
            std::cout << "  Canonical stall address: 0x" << std::hex << canonical_stall << std::dec << "\n";
            ctx.set_gpr(2, 0u);
        });
}

} // namespace p3p3ds::hle
