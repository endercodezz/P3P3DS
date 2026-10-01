#pragma once
// ModuleMgrForUser: loads game-shipped PRX modules.
// Contract: references/uofw/src/kd/modulemgr/{loadModule.c,modulemgr.c}.
// Unencrypted ELF PRX code is really loaded, relocated and executed (by the
// interpreter fallback; it has no AOT units). Its imports are served by HLE
// and the EBOOT's imports of its libraries are bound to its exports.
// Encrypted (~SCE/~PSP) Sony libraries cannot be executed and are accepted as
// HLE-provided modules only when listed in kHleModules.
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace psprecomp { class Runtime; }
namespace p3p3ds { class KernelState; }

namespace p3p3ds::hle {

struct LoadedModule {
    std::int32_t uid{};
    std::string path, name;
    bool hle_only{};
    std::uint32_t base{}, size{}, gp{}, module_start{};
    std::int32_t memory_block{};
    std::int32_t start_thread{};  // thread running module_start while starting
    bool started{};
    std::map<std::pair<std::string, std::uint32_t>, std::uint32_t> exports; // (library, nid) -> address
};

class ModuleManager {
public:
    // EBOOT imports (library, nid, stub) so loaded exports can be bound.
    void set_host_imports(std::vector<std::tuple<std::string, std::uint32_t, std::uint32_t>> imports) {
        host_imports_ = std::move(imports);
    }
    [[nodiscard]] const auto &host_imports() const noexcept { return host_imports_; }
    std::map<std::int32_t, LoadedModule> modules;
    std::int32_t next_uid{0x100}; // own UID space, distinct from ThreadMan objects

private:
    std::vector<std::tuple<std::string, std::uint32_t, std::uint32_t>> host_imports_;
};

void register_modulemgr_module(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
