#pragma once
// IoFileMgrForUser over the VFS (references/uofw/src/kd/iofilemgr/iofilemgr.c).
#include "p3p3ds/vfs.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace psprecomp { class Runtime; }
namespace p3p3ds { class KernelState; }

namespace p3p3ds::hle {

// references/uofw/include/common/errors.h
constexpr std::uint32_t SCE_ERROR_ERRNO_FILE_NOT_FOUND = 0x80010002u;
constexpr std::uint32_t SCE_ERROR_ERRNO_NOT_A_DIRECTORY = 0x80010014u;
constexpr std::uint32_t SCE_ERROR_ERRNO_INVALID_ARGUMENT = 0x80010016u;
constexpr std::uint32_t SCE_ERROR_ERRNO_READ_ONLY = 0x8001001Eu;
constexpr std::uint32_t SCE_ERROR_KERNEL_TOO_MANY_OPEN_FILES = 0x80020320u;
constexpr std::uint32_t SCE_ERROR_KERNEL_NO_SUCH_DEVICE = 0x80020321u;
constexpr std::uint32_t SCE_ERROR_KERNEL_BAD_FILE_DESCRIPTOR = 0x80020323u;
constexpr std::uint32_t SCE_ERROR_KERNEL_INVALID_ARGUMENT_IO = 0x80020324u;
constexpr std::uint32_t SCE_ERROR_KERNEL_UNSUPPORTED_OPERATION = 0x80020325u;
constexpr std::uint32_t SCE_ERROR_KERNEL_ASYNC_BUSY = 0x80020329u;
constexpr std::uint32_t SCE_ERROR_KERNEL_NO_ASYNC_OP = 0x8002032Au;

class IoManager {
public:
    static constexpr int kFirstFd = 3;
    static constexpr int kMaxFds = 64;
    // [INFERRED] deterministic UMD timing: fixed command latency plus bytes at
    // ~2 MB/s. Only virtual time is affected.
    static constexpr std::uint64_t kCommandLatencyUs = 200u;
    [[nodiscard]] static std::uint64_t transfer_us(std::uint64_t bytes) noexcept { return kCommandLatencyUs + bytes / 2u; }

    // Mounts a backend under a device name such as "disc0:" (lower-case, with colon).
    void mount(std::string device, std::shared_ptr<vfs::FileSystem> fs, bool writable = false);
    [[nodiscard]] bool mounted(const std::string &device) const { return devices_.contains(device); }
    // Maps a path prefix such as "ms0:/PSP/P3P" onto another backend (case-
    // insensitive); the longest matching alias wins over the device mount.
    void alias(const std::string &prefix, std::shared_ptr<vfs::FileSystem> fs, bool writable = false);

    struct Async {
        bool pending{};
        std::uint64_t complete_at{};
        std::int64_t result{};
        bool close_after{};
    };
    struct Fd {
        std::string path;                       // device-qualified, as opened
        std::shared_ptr<const vfs::Source> source;
        std::uint64_t position{};
        bool directory{};
        std::vector<vfs::Entry> entries;
        std::size_t entry_index{};
        std::uint32_t flags{};
        std::int32_t async_priority{-1};
        Async async;
    };

    struct Resolved {
        std::shared_ptr<vfs::FileSystem> fs;
        std::string path;
        bool writable{};
    };
    // Splits "device:path"; nullopt when the device is unknown or the path escapes.
    [[nodiscard]] std::optional<Resolved> resolve(const std::string &psp_path) const;

    // Opens (without timing); returns fd or a negative PSP error.
    std::int32_t open(const std::string &path, std::uint32_t flags);
    std::int32_t open_directory(const std::string &path);
    std::int32_t close(std::int32_t fd);
    [[nodiscard]] Fd *get(std::int32_t fd);
    [[nodiscard]] const std::map<std::int32_t, Fd> &files() const noexcept { return fds_; }
    struct OpenRecord { std::string path; std::int32_t result; std::uint64_t time; };
    std::vector<OpenRecord> open_log;          // bounded diagnostic log of opens
    // Optional read trace for independent verification (tools/cpk-check):
    // path, byte offset, delivered length and FNV-1a 64 of the delivered bytes.
    struct ReadRecord { std::string path; std::uint64_t offset; std::uint64_t size; std::uint64_t fnv1a; std::uint64_t time; };
    bool trace_reads{};
    std::vector<ReadRecord> read_log;

private:
    struct Device { std::shared_ptr<vfs::FileSystem> fs; bool writable{}; };
    std::map<std::string, Device> devices_;
    std::map<std::string, Device> aliases_; // lower-case "device:normalized/prefix"
    std::map<std::int32_t, Fd> fds_;
};

void register_iofilemgr_module(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
