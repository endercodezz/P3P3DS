#include "p3p3ds/hle/modulemgr.hpp"

#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/elf32.hpp"
#include "psprecomp/runtime.hpp"

#include <algorithm>
#include <iostream>

namespace p3p3ds::hle {
namespace {
constexpr std::uint32_t kUnknownModule = 0x8002012Eu;  // SCE_ERROR_KERNEL_UNKNOWN_MODULE
constexpr std::uint32_t kFileNotFound = 0x80010002u;
constexpr std::uint32_t kModuleStartNid = 0xD632ACDBu; // system export "module_start"
// [INFERRED] by file name: these USRDIR modules are Sony-signed (~SCE header),
// cannot be executed here, and their libraries are provided by HLE.
constexpr const char *kHleModules[] = {"libfont.prx", "libccc.prx"};

std::map<std::uint32_t, std::pair<std::string, std::uint32_t>> g_dynamic_imports; // stub -> (library, nid)
std::map<std::uint32_t, std::uint32_t> g_bound_stubs;                            // EBOOT stub -> export

// Import stub of a loaded module: same contract as the generated wrappers.
void dynamic_import(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
    const auto it = g_dynamic_imports.find(ctx.pc);
    if (it == g_dynamic_imports.end()) { rt.stop("Unbound dynamic import stub"); return; }
    const auto token = psprecomp::capture_runtime_execution_context();
    const auto pc = ctx.pc, ra = ctx.gpr[31];
    rt.invoke_import(it->second.first, it->second.second, ctx);
    if (!rt.stopped() && psprecomp::runtime_execution_context_matches(token) && ctx.pc == pc) ctx.pc = ra;
}

// EBOOT stub of a library exported by a loaded module: jump to the export.
void bound_stub(psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
    const auto it = g_bound_stubs.find(ctx.pc);
    if (it == g_bound_stubs.end()) { rt.stop("Unbound module export stub"); return; }
    ctx.pc = it->second;
}

std::string file_name(const std::string &path) {
    const auto slash = path.find_last_of("/:");
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    for (auto &c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return name;
}
} // namespace

void register_modulemgr_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    const auto reg = [&runtime](std::uint32_t nid, psprecomp::Runtime::HleFunction fn) {
        runtime.register_hle("ModuleMgrForUser", nid, std::move(fn));
    };

    reg(0x977DE386u, [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelLoadModule
        auto &memory = rt.memory();
        const auto path_ptr = ctx.gpr[4];
        if (path_ptr == 0u || !memory.contains(path_ptr, 1u)) { ctx.set_gpr(2, 0x800200D3u); return; }
        const auto path = memory.read_c_string(path_ptr, 1024u);
        auto &mm = kernel.modules();
        const auto resolved = kernel.io().resolve(path);
        const auto source = resolved ? resolved->fs->open(resolved->path) : nullptr;
        if (!source) { ctx.set_gpr(2, kFileNotFound); rt.event("module_load", {{"result", kFileNotFound}}, path); return; }
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(source->size()));
        (void)source->read(0, bytes.data(), bytes.size());

        LoadedModule module;
        module.uid = mm.next_uid++;
        module.path = path;
        const bool elf = bytes.size() >= 4u && bytes[0] == 0x7Fu && bytes[1] == 'E' && bytes[2] == 'L' && bytes[3] == 'F';
        if (!elf) {
            const auto name = file_name(path);
            if (std::find(std::begin(kHleModules), std::end(kHleModules), name) == std::end(kHleModules)) {
                rt.stop("Unsupported: encrypted module without HLE: " + path);
                return;
            }
            module.hle_only = true;
            module.name = name;
            mm.modules[module.uid] = module;
            rt.event("module_load", {{"result", static_cast<std::uint32_t>(module.uid)}, {"hle", 1u}}, path);
            ctx.set_gpr(2, static_cast<std::uint32_t>(module.uid));
            return;
        }

        const auto image = psprecomp::Elf32Image::from_bytes(bytes, path);
        std::uint32_t span = 0u;
        for (const auto &segment : image.segments()) span = std::max(span, segment.vaddr + segment.memory_size);
        span = (span + 0xFFu) & ~0xFFu;
        const auto block = kernel.sysmem().alloc_partition_memory(2u, path, 0u, span, 0u);
        if (block < 0) { ctx.set_gpr(2, static_cast<std::uint32_t>(block)); return; }
        module.memory_block = block;
        module.base = kernel.sysmem().get_block_head_addr(block);
        module.size = span;
        memory.zero(module.base, span);
        (void)image.load_and_relocate(memory, module.base);
        const auto info = image.find_module_info(memory, module.base);
        if (!info) { rt.stop("Module without module info: " + path); return; }
        module.name = info->name;
        module.gp = info->gp;
        // e_entry 0xFFFFFFFF means "no entry point" (scesupPreAcc_library has none).
        const auto entry = image.runtime_entry(module.base);
        module.module_start = entry - module.base == 0xFFFFFFFFu ? 0u : entry;

        // Imports -> HLE through dynamic wrappers.
        for (const auto &import : image.scan_imports(memory, *info)) {
            g_dynamic_imports[import.stub_address] = {import.library, import.nid};
            rt.register_function(import.stub_address, &dynamic_import, import.library + "::dynamic");
            kernel.threads().add_import_stub(import.stub_address);
        }
        // Exports (SceLibraryEntryTable: name, version, attr, len, vstubcount, stubcount, table).
        for (std::uint32_t entry = info->ent_top; entry + 16u <= info->ent_end;) {
            const auto name_ptr = memory.load32(entry);
            const auto length = memory.load8(entry + 8u);
            const auto vstubs = memory.load8(entry + 9u);
            const auto stubs = memory.load16(entry + 10u);
            const auto table = memory.load32(entry + 12u);
            const std::string library = name_ptr != 0u ? memory.read_c_string(name_ptr, 64u) : std::string{};
            for (std::uint32_t i = 0; i < stubs; ++i) {
                const auto nid = memory.load32(table + 4u * i);
                const auto address = memory.load32(table + 4u * (stubs + vstubs + i));
                if (library.empty() && nid == kModuleStartNid) module.module_start = address;
                if (!library.empty()) module.exports[{library, nid}] = address;
            }
            entry += std::max<std::uint32_t>(length, 4u) * 4u;
        }
        // Bind the EBOOT's imports of exported libraries.
        std::uint32_t bound = 0u;
        for (const auto &[library, nid, stub] : mm.host_imports()) {
            if (auto it = module.exports.find({library, nid}); it != module.exports.end()) {
                g_bound_stubs[stub] = it->second;
                rt.register_function(stub, &bound_stub, library + "::bound");
                ++bound;
            }
        }
        rt.event("module_load", {{"result", static_cast<std::uint32_t>(module.uid)}, {"base", module.base},
            {"size", span}, {"exports", module.exports.size()}, {"bound", bound}, {"start", module.module_start}}, path);
        std::cout << "[MODULE LOAD] " << module.name << " base=0x" << std::hex << module.base << " start=0x"
                  << module.module_start << std::dec << " exports=" << module.exports.size() << " bound=" << bound << "\n";
        mm.modules[module.uid] = module;
        ctx.set_gpr(2, static_cast<std::uint32_t>(module.uid));
    });

    // sceKernelStartModule(uid, argsize, argp, *status, option): uOFW
    // _StartModule runs module_start in its own thread and waits for its end;
    // RESIDENT (0) returns the module id, NO_RESIDENT (1) returns 0.
    reg(0x50F0C1ECu, [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
        const auto uid = static_cast<std::int32_t>(ctx.gpr[4]);
        const auto status_ptr = ctx.gpr[7];
        auto &mm = kernel.modules();
        auto &tm = kernel.threads();
        const auto it = mm.modules.find(uid);
        if (it == mm.modules.end()) { ctx.set_gpr(2, kUnknownModule); return; }
        auto &module = it->second;
        auto write_status = [&](std::int32_t status) {
            if (status_ptr != 0u && rt.memory().contains(status_ptr, 4u)) rt.memory().store32(status_ptr, static_cast<std::uint32_t>(status));
        };
        // uOFW _StartModule: only a relocated (not yet started) module starts.
        if (module.started) { ctx.set_gpr(2, 0x80020001u); return; }
        // uOFW _StartModule: a module without an entry point starts with
        // SCE_KERNEL_START_SUCCESS and no thread.
        if (module.hle_only || (module.module_start == 0u && module.start_thread == 0)) {
            module.started = true;
            write_status(0);
            ctx.set_gpr(2, static_cast<std::uint32_t>(uid));
            return;
        }
        if (module.start_thread != 0) {
            const auto *thread = tm.get_thread(module.start_thread);
            if (thread == nullptr || thread->status != InternalThreadState::Stopped) { ctx.set_gpr(2, 0x80020001u); return; }
            const auto status = thread->exit_status;
            (void)tm.delete_thread(module.start_thread);
            module.start_thread = 0;
            write_status(status);
            rt.event("module_start", {{"uid", static_cast<std::uint32_t>(uid)}, {"status", static_cast<std::uint32_t>(status)}}, module.name);
            if (status == 0) { module.started = true; ctx.set_gpr(2, static_cast<std::uint32_t>(uid)); return; }
            if (status == 1) {
                (void)kernel.sysmem().free_partition_memory(module.memory_block);
                mm.modules.erase(it);
                ctx.set_gpr(2, 0u);
                return;
            }
            ctx.set_gpr(2, static_cast<std::uint32_t>(status));
            return;
        }
        // SCE_KERNEL_MODULE_INIT_PRIORITY 0x20; [INFERRED] 256 KiB user stack.
        const auto thid = tm.create_thread("SceModmgrStart", module.module_start, 0x20, 0x40000u, 0x80000000u, 0u, rt.memory());
        if (thid < 0) { ctx.set_gpr(2, static_cast<std::uint32_t>(thid)); return; }
        module.start_thread = thid;
        auto caller = ctx;
        const auto started = tm.start_thread(thid, ctx.gpr[5], ctx.gpr[6], rt.memory(), caller, false, module.gp);
        if (started < 0) { ctx.set_gpr(2, static_cast<std::uint32_t>(started)); return; }
        WaitInfo wait{WaitType::ThreadEnd, thid};
        wait.retry = true;
        tm.block_current(rt, ctx, wait);
    });
}

} // namespace p3p3ds::hle
