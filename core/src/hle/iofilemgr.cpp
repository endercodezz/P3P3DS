// IoFileMgrForUser / StdioForUser over p3p3ds::vfs.
// Contract source: references/uofw/src/kd/iofilemgr/iofilemgr.c (do_open,
// do_read, do_lseek, do_close, do_get_async_stat): one pending async operation
// per descriptor, PollAsync returns 1 while pending, WaitAsync blocks and
// stores the 64-bit result, ASYNC_BUSY / NO_ASYNC_OP otherwise.
#include "p3p3ds/hle/iofilemgr.hpp"

#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>

namespace p3p3ds::hle {
namespace {
constexpr std::uint32_t kOpenWriteMask = 0x0002u | 0x0100u | 0x0200u | 0x0400u; // WRONLY|APPEND|CREAT|TRUNC
constexpr std::uint32_t kOpenDirectory = 0x0008u;

std::uint32_t u(std::int32_t v) { return static_cast<std::uint32_t>(v); }

std::optional<std::string> guest_path(const psprecomp::GuestMemory &memory, std::uint32_t address) {
    if (address == 0u || !memory.contains(address, 1u)) return std::nullopt;
    std::string path;
    for (std::uint32_t i = 0; i < 1024u && memory.contains(address + i, 1u); ++i) {
        const char c = static_cast<char>(memory.load8(address + i));
        if (c == '\0') return path;
        path += c;
    }
    return std::nullopt;
}

// SceIoStat (PSPSDK pspiofilemgr_stat.h): mode, attr, size(64), 3 x ScePspDateTime, private[6].
void write_stat(psprecomp::GuestMemory &memory, std::uint32_t address, const vfs::Entry &entry) {
    for (std::uint32_t i = 0; i < 88u; i += 4u) memory.store32(address + i, 0u);
    // [INFERRED] mode/attr values: FIO_S_IFDIR 0x1000 / FIO_S_IFREG 0x2000 with
    // 0x16D permissions; FIO_SO_IFDIR 0x10 / FIO_SO_IFREG 0x20.
    memory.store32(address + 0u, (entry.directory ? 0x1000u : 0x2000u) | 0x16Du);
    memory.store32(address + 4u, entry.directory ? 0x10u : 0x20u);
    memory.store32(address + 8u, static_cast<std::uint32_t>(entry.size));
    memory.store32(address + 12u, static_cast<std::uint32_t>(entry.size >> 32u));
}
} // namespace

void IoManager::mount(std::string device, std::shared_ptr<vfs::FileSystem> fs, bool writable) {
    devices_[std::move(device)] = Device{std::move(fs), writable};
}

namespace {
std::string lower(std::string text) {
    for (auto &c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}
} // namespace

void IoManager::alias(const std::string &prefix, std::shared_ptr<vfs::FileSystem> fs, bool writable) {
    const auto colon = prefix.find(':');
    const auto path = vfs::normalize(std::string_view(prefix).substr(colon + 1)).value_or("");
    aliases_[lower(prefix.substr(0, colon + 1) + path)] = Device{std::move(fs), writable};
}

std::optional<IoManager::Resolved> IoManager::resolve(const std::string &psp_path) const {
    const auto colon = psp_path.find(':');
    if (colon == std::string::npos) return std::nullopt;
    const std::string device = lower(psp_path.substr(0, colon + 1));
    const auto path = vfs::normalize(std::string_view(psp_path).substr(colon + 1));
    if (!path) return std::nullopt;
    const std::string key = device + lower(*path);
    const Device *best = nullptr;
    std::size_t best_length = 0;
    for (const auto &[prefix, target] : aliases_) {
        const auto rest = prefix.size() - device.size();
        if (key.starts_with(prefix) && (key.size() == prefix.size() || rest == 0u || key[prefix.size()] == '/') &&
            prefix.size() >= best_length) {
            best = &target;
            best_length = prefix.size();
        }
    }
    if (best != nullptr) {
        const auto skip = best_length - device.size();
        std::string rest = path->size() > skip ? path->substr(skip) : std::string{};
        if (!rest.empty() && rest.front() == '/') rest.erase(0, 1);
        return Resolved{best->fs, rest, best->writable};
    }
    const auto it = devices_.find(device);
    if (it == devices_.end()) return std::nullopt;
    return Resolved{it->second.fs, *path, it->second.writable};
}

IoManager::Fd *IoManager::get(std::int32_t fd) {
    const auto it = fds_.find(fd);
    return it == fds_.end() ? nullptr : &it->second;
}

std::int32_t IoManager::open(const std::string &path, std::uint32_t flags) {
    std::int32_t result = 0;
    const auto resolved = resolve(path);
    std::shared_ptr<const vfs::Source> source;
    if (!resolved) result = static_cast<std::int32_t>(SCE_ERROR_KERNEL_NO_SUCH_DEVICE);
    else if ((flags & kOpenWriteMask) != 0u && !resolved->writable) result = static_cast<std::int32_t>(SCE_ERROR_ERRNO_READ_ONLY);
    else if (!(source = resolved->fs->open(resolved->path))) result = static_cast<std::int32_t>(SCE_ERROR_ERRNO_FILE_NOT_FOUND);
    if (result == 0) {
        std::int32_t fd = kFirstFd;
        while (fds_.contains(fd)) ++fd;
        if (fd >= kMaxFds) result = static_cast<std::int32_t>(SCE_ERROR_KERNEL_TOO_MANY_OPEN_FILES);
        else {
            fds_[fd] = Fd{path, std::move(source), 0u, false, {}, 0u, flags, -1, {}};
            result = fd;
        }
    }
    if (open_log.size() < 4096u) open_log.push_back({path, result, 0u});
    return result;
}

std::int32_t IoManager::open_directory(const std::string &path) {
    const auto resolved = resolve(path);
    if (!resolved) return static_cast<std::int32_t>(SCE_ERROR_KERNEL_NO_SUCH_DEVICE);
    auto entries = resolved->fs->list(resolved->path);
    if (!entries) {
        return resolved->fs->stat(resolved->path) ? static_cast<std::int32_t>(SCE_ERROR_ERRNO_NOT_A_DIRECTORY)
                                                  : static_cast<std::int32_t>(SCE_ERROR_ERRNO_FILE_NOT_FOUND);
    }
    std::int32_t fd = kFirstFd;
    while (fds_.contains(fd)) ++fd;
    if (fd >= kMaxFds) return static_cast<std::int32_t>(SCE_ERROR_KERNEL_TOO_MANY_OPEN_FILES);
    // Directory reads list "." and ".." first, like the PSP ISO driver.
    std::vector<vfs::Entry> list{{".", true, 0u}, {"..", true, 0u}};
    list.insert(list.end(), entries->begin(), entries->end());
    fds_[fd] = Fd{path, nullptr, 0u, true, std::move(list), 0u, kOpenDirectory, -1, {}};
    return fd;
}

std::int32_t IoManager::close(std::int32_t fd) {
    return fds_.erase(fd) != 0u ? 0 : static_cast<std::int32_t>(SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR);
}

void register_iofilemgr_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    const auto reg = [&runtime](std::uint32_t nid, psprecomp::Runtime::HleFunction fn) {
        runtime.register_hle("IoFileMgrForUser", nid, std::move(fn));
    };
    auto &io = kernel.io();
    auto &tm = kernel.threads();

    // Synchronous calls complete immediately but hold the caller for the
    // modelled device time, so other PSP threads run meanwhile.
    const auto finish_sync = [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::uint32_t result,
                                   std::uint64_t cost_us, std::optional<std::uint32_t> high = std::nullopt) {
        ctx.set_gpr(2, result);
        if (high) ctx.set_gpr(3, *high);
        WaitInfo wait{WaitType::Io, 0, tm.now() + cost_us};
        tm.block_current(rt, ctx, wait);
    };
    // Starts an async operation whose 64-bit result is published at complete_at.
    const auto start_async = [&tm](IoManager::Fd &fd, std::int64_t result, std::uint64_t cost_us) {
        fd.async = IoManager::Async{true, tm.now() + cost_us, result, false};
    };

    reg(0x109F50BCu, [&io, &tm, finish_sync](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceIoOpen
        const auto path = guest_path(rt.memory(), ctx.gpr[4]);
        if (!path) { ctx.set_gpr(2, 0x800200D3u); return; }
        const auto result = (ctx.gpr[5] & kOpenDirectory) != 0u ? io.open_directory(*path) : io.open(*path, ctx.gpr[5]);
        if (!io.open_log.empty()) io.open_log.back().time = tm.now();
        rt.event("io_open", {{"result", u(result)}, {"flags", ctx.gpr[5]}, {"async", 0u}}, *path);
        finish_sync(rt, ctx, u(result), IoManager::kCommandLatencyUs);
    });
    reg(0x89AA9906u, [&io, &tm, start_async](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceIoOpenAsync
        const auto path = guest_path(rt.memory(), ctx.gpr[4]);
        if (!path) { ctx.set_gpr(2, 0x800200D3u); return; }
        const auto result = io.open(*path, ctx.gpr[5]);
        if (!io.open_log.empty()) io.open_log.back().time = tm.now();
        rt.event("io_open", {{"result", u(result)}, {"flags", ctx.gpr[5]}, {"async", 1u}}, *path);
        if (result < 0) { ctx.set_gpr(2, u(result)); return; }
        start_async(*io.get(result), result, IoManager::kCommandLatencyUs);
        ctx.set_gpr(2, u(result));
    });
    reg(0x810C4BC3u, [&io](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceIoClose
        const auto fd = static_cast<std::int32_t>(ctx.gpr[4]);
        auto *f = io.get(fd);
        if (f == nullptr || f->directory) { ctx.set_gpr(2, SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR); return; }
        if (f->async.pending) { ctx.set_gpr(2, SCE_ERROR_KERNEL_ASYNC_BUSY); return; }
        ctx.set_gpr(2, u(io.close(fd)));
    });
    reg(0xFF5940B6u, [&io, start_async](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceIoCloseAsync
        auto *f = io.get(static_cast<std::int32_t>(ctx.gpr[4]));
        if (f == nullptr || f->directory) { ctx.set_gpr(2, SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR); return; }
        if (f->async.pending) { ctx.set_gpr(2, SCE_ERROR_KERNEL_ASYNC_BUSY); return; }
        start_async(*f, 0, IoManager::kCommandLatencyUs);
        f->async.close_after = true;
        ctx.set_gpr(2, 0u);
    });

    const auto do_read = [&io, &tm](psprecomp::Runtime &rt, IoManager::Fd &f, std::uint32_t buffer, std::uint32_t size) -> std::int64_t {
        if (size != 0u && !rt.memory().contains(buffer, size)) return static_cast<std::int32_t>(0x800200D3u);
        std::vector<std::uint8_t> data(size);
        const auto n = f.source->read(f.position, data.data(), size);
        if (n != 0u) rt.memory().copy_in(buffer, std::span<const std::uint8_t>(data.data(), n));
        if (io.trace_reads && io.read_log.size() < 200000u) {
            std::uint64_t hash = 0xCBF29CE484222325ull;
            for (std::size_t i = 0; i < n; ++i) { hash ^= data[i]; hash *= 0x100000001B3ull; }
            io.read_log.push_back({f.path, f.position, n, hash, tm.now()});
        }
        f.position += n;
        return static_cast<std::int64_t>(n);
    };
    reg(0x6A638D83u, [&io, do_read, finish_sync](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceIoRead
        auto *f = io.get(static_cast<std::int32_t>(ctx.gpr[4]));
        if (f == nullptr || f->directory) { ctx.set_gpr(2, SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR); return; }
        if (f->async.pending) { ctx.set_gpr(2, SCE_ERROR_KERNEL_ASYNC_BUSY); return; }
        const auto n = do_read(rt, *f, ctx.gpr[5], ctx.gpr[6]);
        finish_sync(rt, ctx, static_cast<std::uint32_t>(n), IoManager::transfer_us(n > 0 ? static_cast<std::uint64_t>(n) : 0u));
    });
    reg(0xA0B5A7C2u, [&io, do_read, start_async](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceIoReadAsync
        auto *f = io.get(static_cast<std::int32_t>(ctx.gpr[4]));
        if (f == nullptr || f->directory) { ctx.set_gpr(2, SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR); return; }
        if (f->async.pending) { ctx.set_gpr(2, SCE_ERROR_KERNEL_ASYNC_BUSY); return; }
        // [INFERRED] data lands in guest memory at issue time; only the
        // completion is deferred.
        const auto n = do_read(rt, *f, ctx.gpr[5], ctx.gpr[6]);
        start_async(*f, n, IoManager::transfer_us(n > 0 ? static_cast<std::uint64_t>(n) : 0u));
        rt.event("io_read_async", {{"fd", ctx.gpr[4]}, {"size", ctx.gpr[6]}, {"result", static_cast<std::uint64_t>(n)},
            {"position", f->position}});
        ctx.set_gpr(2, 0u);
    });

    // SceOff offset arrives aligned in $a2:$a3 (PSP EABI 64-bit argument pair),
    // whence in $t0; the 64-bit result is returned in $v0:$v1.
    const auto do_seek = [](IoManager::Fd &f, std::int64_t offset, std::uint32_t whence) -> std::int64_t {
        std::int64_t base = 0;
        if (whence == 1u) base = static_cast<std::int64_t>(f.position);
        else if (whence == 2u) base = static_cast<std::int64_t>(f.source->size());
        else if (whence != 0u) return static_cast<std::int32_t>(SCE_ERROR_KERNEL_INVALID_ARGUMENT_IO);
        const auto target = base + offset;
        if (target < 0) return static_cast<std::int32_t>(SCE_ERROR_ERRNO_INVALID_ARGUMENT);
        f.position = static_cast<std::uint64_t>(target);
        return target;
    };
    reg(0x27EB27B8u, [&io, do_seek](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceIoLseek
        auto *f = io.get(static_cast<std::int32_t>(ctx.gpr[4]));
        if (f == nullptr || f->directory) { ctx.set_gpr(2, SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR); ctx.set_gpr(3, 0xFFFFFFFFu); return; }
        if (f->async.pending) { ctx.set_gpr(2, SCE_ERROR_KERNEL_ASYNC_BUSY); ctx.set_gpr(3, 0xFFFFFFFFu); return; }
        const auto offset = static_cast<std::int64_t>((static_cast<std::uint64_t>(ctx.gpr[7]) << 32u) | ctx.gpr[6]);
        const auto r = do_seek(*f, offset, ctx.gpr[8]);
        ctx.set_gpr(2, static_cast<std::uint32_t>(r));
        ctx.set_gpr(3, static_cast<std::uint32_t>(static_cast<std::uint64_t>(r) >> 32u));
    });
    reg(0x71B19E77u, [&io, do_seek, start_async](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceIoLseekAsync
        auto *f = io.get(static_cast<std::int32_t>(ctx.gpr[4]));
        if (f == nullptr || f->directory) { ctx.set_gpr(2, SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR); return; }
        if (f->async.pending) { ctx.set_gpr(2, SCE_ERROR_KERNEL_ASYNC_BUSY); return; }
        const auto offset = static_cast<std::int64_t>((static_cast<std::uint64_t>(ctx.gpr[7]) << 32u) | ctx.gpr[6]);
        start_async(*f, do_seek(*f, offset, ctx.gpr[8]), IoManager::kCommandLatencyUs);
        ctx.set_gpr(2, 0u);
    });

    // Async completion (uOFW do_get_async_stat).
    const auto async_stat = [&io, &tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, bool poll, bool callbacks) {
        const auto fd = static_cast<std::int32_t>(ctx.gpr[4]);
        const auto res = ctx.gpr[5];
        if (res == 0u || !rt.memory().contains(res, 8u)) { ctx.set_gpr(2, 0x800200D3u); return; }
        auto *f = io.get(fd);
        if (f == nullptr) { ctx.set_gpr(2, SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR); return; }
        if (!f->async.pending) { ctx.set_gpr(2, SCE_ERROR_KERNEL_NO_ASYNC_OP); return; }
        if (tm.now() < f->async.complete_at) {
            if (poll) { ctx.set_gpr(2, 1u); return; }
            // Retry at the stub once the operation completes.
            WaitInfo wait{WaitType::Io, fd, f->async.complete_at, 0u, callbacks};
            wait.retry = true;
            tm.block_current(rt, ctx, wait);
            return;
        }
        const auto value = static_cast<std::uint64_t>(f->async.result);
        rt.memory().store32(res, static_cast<std::uint32_t>(value));
        rt.memory().store32(res + 4u, static_cast<std::uint32_t>(value >> 32u));
        f->async.pending = false;
        if (f->async.close_after) (void)io.close(fd);
        ctx.set_gpr(2, 0u);
    };
    reg(0x3251EA56u, [async_stat](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { async_stat(rt, ctx, true, false); });  // sceIoPollAsync
    reg(0xE23EEC33u, [async_stat](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { async_stat(rt, ctx, false, false); }); // sceIoWaitAsync
    reg(0x35DBD746u, [async_stat](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { async_stat(rt, ctx, false, true); });  // sceIoWaitAsyncCB
    reg(0xB293727Fu, [&io](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceIoChangeAsyncPriority
        auto *f = io.get(static_cast<std::int32_t>(ctx.gpr[4]));
        const auto priority = static_cast<std::int32_t>(ctx.gpr[5]);
        if (f == nullptr) { ctx.set_gpr(2, SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR); return; }
        if (priority != -1 && (priority < 8 || priority > 119)) { ctx.set_gpr(2, 0x80020193u); return; }
        f->async_priority = priority;
        ctx.set_gpr(2, 0u);
    });

    reg(0xACE946E8u, [&io](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceIoGetstat
        const auto path = guest_path(rt.memory(), ctx.gpr[4]);
        if (!path || !rt.memory().contains(ctx.gpr[5], 88u)) { ctx.set_gpr(2, 0x800200D3u); return; }
        const auto resolved = io.resolve(*path);
        const auto entry = resolved ? resolved->fs->stat(resolved->path) : std::nullopt;
        rt.event("io_getstat", {{"found", entry ? 1u : 0u}}, *path);
        if (!resolved) { ctx.set_gpr(2, SCE_ERROR_KERNEL_NO_SUCH_DEVICE); return; }
        if (!entry) { ctx.set_gpr(2, SCE_ERROR_ERRNO_FILE_NOT_FOUND); return; }
        write_stat(rt.memory(), ctx.gpr[5], *entry);
        ctx.set_gpr(2, 0u);
    });
    reg(0xB29DDF9Cu, [&io](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceIoDopen
        const auto path = guest_path(rt.memory(), ctx.gpr[4]);
        if (!path) { ctx.set_gpr(2, 0x800200D3u); return; }
        const auto result = io.open_directory(*path);
        rt.event("io_dopen", {{"result", u(result)}}, *path);
        ctx.set_gpr(2, u(result));
    });
    reg(0xE3EB004Cu, [&io](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceIoDread
        auto *f = io.get(static_cast<std::int32_t>(ctx.gpr[4]));
        const auto dirent = ctx.gpr[5];
        if (f == nullptr || !f->directory) { ctx.set_gpr(2, SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR); return; }
        if (!rt.memory().contains(dirent, 352u)) { ctx.set_gpr(2, 0x800200D3u); return; }
        if (f->entry_index >= f->entries.size()) { ctx.set_gpr(2, 0u); return; }
        const auto &entry = f->entries[f->entry_index++];
        // SceIoDirent: SceIoStat (88) + d_name[256]; d_private at 344 is left untouched.
        write_stat(rt.memory(), dirent, entry);
        for (std::uint32_t i = 0; i < 256u; ++i)
            rt.memory().store8(dirent + 88u + i, i < entry.name.size() ? static_cast<std::uint8_t>(entry.name[i]) : 0u);
        ctx.set_gpr(2, 1u);
    });
    reg(0xEB092469u, [&io](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceIoDclose
        const auto fd = static_cast<std::int32_t>(ctx.gpr[4]);
        auto *f = io.get(fd);
        if (f == nullptr || !f->directory) { ctx.set_gpr(2, SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR); return; }
        ctx.set_gpr(2, u(io.close(fd)));
    });
    reg(0x42EC03ACu, [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceIoWrite
        const auto fd = ctx.gpr[4], buffer = ctx.gpr[5], size = ctx.gpr[6];
        if ((fd == 1u || fd == 2u) && rt.memory().contains(buffer, size)) {
            std::string text(size, '\0');
            for (std::uint32_t i = 0; i < size; ++i) text[i] = static_cast<char>(rt.memory().load8(buffer + i));
            std::cout << "[PSP STDOUT] " << text << (text.ends_with('\n') ? "" : "\n");
            ctx.set_gpr(2, size);
            return;
        }
        ctx.set_gpr(2, SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR); // no writable devices mounted yet
    });

    const auto stdio = [&runtime](std::uint32_t nid, std::uint32_t fd) {
        runtime.register_hle("StdioForUser", nid, [fd](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { ctx.set_gpr(2, fd); });
    };
    stdio(0x172D316Eu, 0u); // sceKernelStdin
    stdio(0xA6BAB2E9u, 1u); // sceKernelStdout
    stdio(0xF78BA90Au, 2u); // sceKernelStderr
}

} // namespace p3p3ds::hle
