#include "psprecomp/nid_registry.hpp"
#include "psprecomp/common.hpp"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <sstream>

namespace psprecomp {

std::string NidRegistry::key(const std::string &library, std::uint32_t nid) {
    return library + ":" + hex32(nid);
}

NidRegistry::NidRegistry() {
    // Verified against PSPSDK's IoFileMgrForUser.S.
    add("IoFileMgrForUser", 0x109F50BCu, "sceIoOpen");
    add("IoFileMgrForUser", 0x810C4BC3u, "sceIoClose");
    add("IoFileMgrForUser", 0x6A638D83u, "sceIoRead");
    add("IoFileMgrForUser", 0x42EC03ACu, "sceIoWrite");
    add("IoFileMgrForUser", 0x27EB27B8u, "sceIoLseek");
    add("IoFileMgrForUser", 0x68963324u, "sceIoLseek32");
    add("IoFileMgrForUser", 0xACE946E8u, "sceIoGetstat");
    add("IoFileMgrForUser", 0x54F5FB11u, "sceIoDevctl");
    add("IoFileMgrForUser", 0xB293727Fu, "sceIoChangeAsyncPriority");
    add("IoFileMgrForUser", 0xB29DDF9Cu, "sceIoDopen");
    add("IoFileMgrForUser", 0xE3EB004Cu, "sceIoDread");
    add("IoFileMgrForUser", 0xEB092469u, "sceIoDclose");

    // Common user-mode imports used by the currently covered PSP profiles.
    add("SysMemUserForUser", 0xA291F107u, "sceKernelMaxFreeMemSize");
    add("SysMemUserForUser", 0x7591C7DBu, "sceKernelSetCompiledSdkVersion");
    add("SysMemUserForUser", 0xF77D77CBu, "sceKernelSetCompilerVersion");
    add("SysMemUserForUser", 0x237DBD4Fu, "sceKernelAllocPartitionMemory");
    add("SysMemUserForUser", 0xB6D61D02u, "sceKernelFreePartitionMemory");
    add("SysMemUserForUser", 0x9D9A5BA1u, "sceKernelGetBlockHeadAddr");
    add("SysMemUserForUser", 0x13A5ABEFu, "sceKernelPrintf");
    add("ThreadManForUser", 0x446D8DE6u, "sceKernelCreateThread");
    add("ThreadManForUser", 0xF475845Du, "sceKernelStartThread");
    add("ThreadManForUser", 0x809CE29Bu, "sceKernelExitDeleteThread");
    add("ThreadManForUser", 0x293B45B8u, "sceKernelGetThreadId");
    add("ThreadManForUser", 0xCEADEB47u, "sceKernelDelayThread");
    add("ThreadManForUser", 0x278C0DF5u, "sceKernelWaitThreadEnd");
    add("ThreadManForUser", 0x68DA9E36u, "sceKernelDelayThreadCB");
    add("ThreadManForUser", 0xE81CAF8Fu, "sceKernelCreateCallback");
    add("ThreadManForUser", 0xEDBA5844u, "sceKernelDeleteCallback");
    add("ThreadManForUser", 0xD6DA4BA1u, "sceKernelCreateSema");
    add("ThreadManForUser", 0x28B6489Cu, "sceKernelDeleteSema");
    add("ThreadManForUser", 0x3F53E640u, "sceKernelSignalSema");
    add("ThreadManForUser", 0x4E3A1105u, "sceKernelWaitSema");
    add("ThreadManForUser", 0x110DEC9Au, "sceKernelUSec2SysClock");
    add("ThreadManForUser", 0xC8CD158Cu, "sceKernelUSec2SysClockWide");
    add("ThreadManForUser", 0xBA6B92E2u, "sceKernelSysClock2USec");
    add("ThreadManForUser", 0xE1619D7Cu, "sceKernelSysClock2USecWide");
    add("ThreadManForUser", 0xDB738F35u, "sceKernelGetSystemTime");
    add("ThreadManForUser", 0x82BC5777u, "sceKernelGetSystemTimeWide");
    add("ThreadManForUser", 0x369ED59Du, "sceKernelGetSystemTimeLow");
    add("ThreadManForUser", 0xEA748E31u, "sceKernelChangeCurrentThreadAttr");
    add("ThreadManForUser", 0x55C20A00u, "sceKernelCreateEventFlag");
    add("ThreadManForUser", 0xEF9E4C70u, "sceKernelDeleteEventFlag");
    add("ThreadManForUser", 0x1FB15A32u, "sceKernelSetEventFlag");
    add("ThreadManForUser", 0x812346E4u, "sceKernelClearEventFlag");
    add("ThreadManForUser", 0x402FCF22u, "sceKernelWaitEventFlag");
    add("ThreadManForUser", 0x328C546Au, "sceKernelWaitEventFlagCB");
    add("ThreadManForUser", 0x30FD48F0u, "sceKernelPollEventFlag");
    add("ThreadManForUser", 0xC07BB470u, "sceKernelCreateFpl");
    add("ThreadManForUser", 0xD979E9BFu, "sceKernelAllocateFpl");
    add("sceSuspendForUser", 0xEADB1BD7u, "sceKernelPowerLock");
    add("sceSuspendForUser", 0x3AEE7261u, "sceKernelPowerUnlock");
    add("sceSuspendForUser", 0x090CCB3Fu, "sceKernelPowerTick");
    add("sceSuspendForUser", 0x3E0271D3u, "sceKernelVolatileMemLock");
    add("sceSuspendForUser", 0xA14F40B2u, "sceKernelVolatileMemTryLock");
    add("sceSuspendForUser", 0xA569E425u, "sceKernelVolatileMemUnlock");
    add("UtilsForUser", 0x37FB5C42u, "sceKernelGetGPI");
    add("UtilsForUser", 0x6AD345D7u, "sceKernelSetGPO");
    add("UtilsForUser", 0xBFA98062u, "sceKernelDcacheInvalidateRange");
    add("UtilsForUser", 0x79D1C3FAu, "sceKernelDcacheWritebackAll");
    add("UtilsForUser", 0xB435DEC5u, "sceKernelDcacheWritebackInvalidateAll");
    add("UtilsForUser", 0x3EE30821u, "sceKernelDcacheWritebackRange");
    add("UtilsForUser", 0x34B9FA9Eu, "sceKernelDcacheWritebackInvalidateRange");
    add("UtilsForUser", 0x80001C4Cu, "sceKernelDcacheProbe");
    add("UtilsForUser", 0x16641D70u, "sceKernelDcacheReadTag");
    add("UtilsForUser", 0x4FD31C9Du, "sceKernelIcacheProbe");
    add("UtilsForUser", 0xFB05FAD0u, "sceKernelIcacheReadTag");
    add("UtilsForUser", 0x920F104Au, "sceKernelIcacheInvalidateAll");
    add("UtilsForUser", 0xC2DF770Eu, "sceKernelIcacheInvalidateRange");
    add("sceCtrl", 0x6A2774F3u, "sceCtrlSetSamplingCycle");
    add("sceCtrl", 0x02BAAD91u, "sceCtrlGetSamplingCycle");
    add("sceCtrl", 0x1F4011E6u, "sceCtrlSetSamplingMode");
    add("sceCtrl", 0xDA6B76A1u, "sceCtrlGetSamplingMode");
    add("sceCtrl", 0x3A622550u, "sceCtrlPeekBufferPositive");
    add("sceCtrl", 0xC152080Au, "sceCtrlPeekBufferNegative");
    add("sceCtrl", 0x1F803938u, "sceCtrlReadBufferPositive");
    add("sceCtrl", 0x60B81F86u, "sceCtrlReadBufferNegative");
    add("sceCtrl", 0xB1D0E5CDu, "sceCtrlPeekLatch");
    add("sceCtrl", 0x0B588501u, "sceCtrlReadLatch");
    add("sceGe_user", 0xE47E40E4u, "sceGeEdramGetAddr");
    add("sceGe_user", 0x1F6752ADu, "sceGeEdramGetSize");
    add("sceGe_user", 0xB77905EAu, "sceGeEdramSetAddrTranslation");
    add("sceUmdUser", 0x46EBB729u, "sceUmdCheckMedium");
    add("sceUmdUser", 0x6B4A146Cu, "sceUmdGetDriveStat");
    add("sceUmdUser", 0x8EF08FCEu, "sceUmdWaitDriveStat");
    add("sceUmdUser", 0xAEE7404Du, "sceUmdRegisterUMDCallBack");
    add("sceUmdUser", 0xBD2BDE07u, "sceUmdUnRegisterUMDCallBack");
    add("sceUmdUser", 0xC6183D47u, "sceUmdActivate");
    add("LoadExecForUser", 0x4AC57943u, "sceKernelRegisterExitCallback");
}

void NidRegistry::add(std::string library, std::uint32_t nid, std::string name) {
    NidSymbol symbol{std::move(library), nid, std::move(name)};
    symbols_[key(symbol.library, symbol.nid)] = std::move(symbol);
}

void NidRegistry::load_csv(const std::filesystem::path &path) {
    std::ifstream in(path);
    if (!in) throw Error("Cannot open NID CSV: " + path.string());
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(in, line)) {
        ++line_number;
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string library, nid_text, name;
        if (!std::getline(ss, library, ',') || !std::getline(ss, nid_text, ',') || !std::getline(ss, name)) {
            throw Error("Invalid NID CSV line " + std::to_string(line_number));
        }
        std::uint32_t nid{};
        const char *begin = nid_text.data();
        const char *end = begin + nid_text.size();
        if (nid_text.starts_with("0x") || nid_text.starts_with("0X")) begin += 2;
        const auto result = std::from_chars(begin, end, nid, 16);
        if (result.ec != std::errc{}) throw Error("Invalid NID at CSV line " + std::to_string(line_number));
        add(std::move(library), nid, std::move(name));
    }
}

std::optional<std::string> NidRegistry::resolve(const std::string &library, std::uint32_t nid) const {
    const auto it = symbols_.find(key(library, nid));
    if (it == symbols_.end()) return std::nullopt;
    return it->second.name;
}

std::vector<NidSymbol> NidRegistry::all() const {
    std::vector<NidSymbol> result;
    result.reserve(symbols_.size());
    for (const auto &[_, symbol] : symbols_) result.push_back(symbol);
    std::sort(result.begin(), result.end(), [](const NidSymbol &a, const NidSymbol &b) {
        if (a.library != b.library) return a.library < b.library;
        return a.nid < b.nid;
    });
    return result;
}

} // namespace psprecomp
