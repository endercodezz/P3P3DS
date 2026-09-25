// SysMemUserForUser, sceSuspendForUser and sceDmac.
#include "hle_common.hpp"

#include "psprecomp/common.hpp"

#include "gpu/ge_state.hpp"

#include <cstdlib>
#include <iostream>

namespace mhp3rd {
namespace {

std::uint32_t as_unsigned(std::int32_t value) { return static_cast<std::uint32_t>(value); }

// Formats the subset of printf used for diagnostics. Arguments are taken from
// a1..a3 and t0..t3.
std::string format_guest(const psprecomp::GuestMemory &memory, const AllegrexContext &ctx, const std::string &format) {
    std::string out;
    unsigned next = 1u;
    for (std::size_t i = 0; i < format.size(); ++i) {
        if (format[i] != '%' || i + 1 >= format.size()) {
            out.push_back(format[i]);
            continue;
        }
        std::size_t j = i + 1;
        while (j < format.size() && std::string_view("-+ #0123456789.lhz").find(format[j]) != std::string_view::npos) ++j;
        if (j >= format.size()) break;
        const char conversion = format[j];
        const std::uint32_t value = next < 8u ? arg(ctx, next) : 0u;
        switch (conversion) {
        case '%': out.push_back('%'); break;
        case 'd': case 'i': out += std::to_string(static_cast<std::int32_t>(value)); ++next; break;
        case 'u': out += std::to_string(value); ++next; break;
        case 'x': case 'X': case 'p': out += psprecomp::hex32(value); ++next; break;
        case 'c': out.push_back(static_cast<char>(value)); ++next; break;
        case 's': out += read_cstring(memory, value, 256u); ++next; break;
        default: out += format.substr(i, j - i + 1); ++next; break;
        }
        i = j;
    }
    return out;
}

} // namespace

void register_sysmem(HleRegistrar &hle) {
    hle.add("SysMemUserForUser", "sceKernelAllocPartitionMemory", [](Runtime &rt, AllegrexContext &ctx) {
        const std::string name = read_cstring(rt.memory(), arg(ctx, 1), 32u);
        kernel().finish(ctx, as_unsigned(kernel().allocate_block(name, arg(ctx, 2), arg(ctx, 3), arg(ctx, 4))));
    });
    hle.add("SysMemUserForUser", "sceKernelFreePartitionMemory", [](Runtime &, AllegrexContext &ctx) {
        kernel().finish(ctx, as_unsigned(kernel().free_block(static_cast<SceUID>(arg(ctx, 0)))));
    });
    hle.add("SysMemUserForUser", "sceKernelGetBlockHeadAddr", [](Runtime &, AllegrexContext &ctx) {
        const MemoryBlock *block = kernel().find_block(static_cast<SceUID>(arg(ctx, 0)));
        kernel().finish(ctx, block != nullptr ? block->address : 0u);
    });
    hle.add("SysMemUserForUser", "sceKernelAllocMemoryBlock", [](Runtime &rt, AllegrexContext &ctx) {
        const std::string name = read_cstring(rt.memory(), arg(ctx, 0), 32u);
        const std::uint32_t type = arg(ctx, 1);
        if (type > 1u) {
            kernel().finish(ctx, error::kIllegalMemblockType);
            return;
        }
        kernel().finish(ctx, as_unsigned(kernel().allocate_block(name, type, arg(ctx, 2), 0u)));
    });
    hle.add("SysMemUserForUser", "sceKernelGetMemoryBlockAddr", [](Runtime &rt, AllegrexContext &ctx) {
        const MemoryBlock *block = kernel().find_block(static_cast<SceUID>(arg(ctx, 0)));
        if (block == nullptr) {
            kernel().finish(ctx, error::kIllegalMemblock);
            return;
        }
        if (arg(ctx, 1) != 0u) rt.memory().store32(arg(ctx, 1), block->address);
        kernel().finish(ctx, 0u);
    });
    hle.add("SysMemUserForUser", "sceKernelFreeMemoryBlock", [](Runtime &, AllegrexContext &ctx) {
        kernel().finish(ctx, as_unsigned(kernel().free_block(static_cast<SceUID>(arg(ctx, 0)))));
    });
    const auto success = [](Runtime &, AllegrexContext &ctx) { kernel().finish(ctx, 0u); };
    hle.add("SysMemUserForUser", "sceKernelSetCompilerVersion", success);
    hle.add("SysMemUserForUser", "sceKernelSetCompiledSdkVersion603_605", success);
    hle.add("SysMemUserForUser", "sceKernelPrintf", [](Runtime &rt, AllegrexContext &ctx) {
        std::cerr << "[guest] " << format_guest(rt.memory(), ctx, read_cstring(rt.memory(), arg(ctx, 0), 1024u));
        kernel().finish(ctx, 0u);
    });

    hle.add("sceSuspendForUser", "sceKernelPowerTick", success);
    hle.add("sceSuspendForUser", "sceKernelVolatileMemLock", [](Runtime &rt, AllegrexContext &ctx) {
        if (arg(ctx, 1) != 0u) rt.memory().store32(arg(ctx, 1), kVolatileMemoryBase);
        if (arg(ctx, 2) != 0u) rt.memory().store32(arg(ctx, 2), kVolatileMemorySize);
        kernel().finish(ctx, 0u);
    });
    hle.add("sceSuspendForUser", "sceKernelVolatileMemUnlock", success);

    hle.add("sceDmac", "sceDmacMemcpy", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t destination = arg(ctx, 0);
        const std::uint32_t source = arg(ctx, 1);
        const std::uint32_t size = arg(ctx, 2);
        // MHP3RD_TRACE_FB_TEXTURES: copies into or out of VRAM, where the
        // framebuffers are.
        static const bool trace = std::getenv("MHP3RD_TRACE_FB_TEXTURES") != nullptr;
        const auto in_vram = [](std::uint32_t address) { return (address & 0x1F000000u) == 0x04000000u; };
        if (trace && (in_vram(source) || in_vram(destination))) {
            static std::uint32_t traced = 0u;
            if (traced++ < 400u)
                std::cout << "[fbtex] sceDmacMemcpy 0x" << std::hex << source << " -> 0x" << destination << std::dec
                          << " (" << size << " bytes)\n";
            if (in_vram(source)) gpu::note_vram_copy(destination, source, size);
        }
        if (destination < source || destination >= source + size) {
            for (std::uint32_t i = 0; i < size; ++i) rt.memory().store8(destination + i, rt.memory().load8(source + i));
        } else {
            for (std::uint32_t i = size; i-- > 0u;) rt.memory().store8(destination + i, rt.memory().load8(source + i));
        }
        kernel().finish(ctx, 0u);
    });
}

} // namespace mhp3rd
