#pragma once
// Statistical profiler for the PC runner (--sample <file>): a helper thread
// suspends the runner thread about every millisecond and records its
// instruction pointer while `active` is set. The file starts with the loaded
// modules ("module <base> <size> <name>"), then "address count" lines; map
// them to functions with tools/profile_symbols.py.
// Windows only; elsewhere the option is accepted and does nothing.
#include <atomic>
#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#include <timeapi.h>
#include <thread>
#endif

namespace p3p3ds::pc {

class Sampler {
public:
    std::atomic<bool> active{true};

    explicit Sampler(std::string path, bool start_active = true) : path_(std::move(path)) {
        active = start_active;
#ifdef _WIN32
        if (path_.empty()) return;
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &target_,
                        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, 0);
        timeBeginPeriod(1);
        worker_ = std::thread([this] {
            while (!stop_.load()) {
                Sleep(1);
                if (!active.load(std::memory_order_relaxed)) continue;
                if (SuspendThread(target_) == static_cast<DWORD>(-1)) continue;
                CONTEXT c{};
                c.ContextFlags = CONTEXT_CONTROL;
                if (GetThreadContext(target_, &c)) ++counts_[c.Rip];
                ResumeThread(target_);
            }
        });
#endif
    }
    ~Sampler() {
#ifdef _WIN32
        if (path_.empty()) return;
        stop_ = true;
        worker_.join();
        timeEndPeriod(1);
        CloseHandle(target_);
        std::ofstream out(path_);
        HMODULE modules[512];
        DWORD needed = 0;
        if (EnumProcessModules(GetCurrentProcess(), modules, sizeof modules, &needed)) {
            for (DWORD i = 0; i < needed / sizeof(HMODULE) && i < 512u; ++i) {
                MODULEINFO info{};
                char name[MAX_PATH] = {};
                GetModuleInformation(GetCurrentProcess(), modules[i], &info, sizeof info);
                GetModuleBaseNameA(GetCurrentProcess(), modules[i], name, MAX_PATH);
                out << "module " << std::hex << reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll) << ' ' << info.SizeOfImage
                    << std::dec << ' ' << name << '\n';
            }
        }
        for (const auto &[address, count] : counts_) out << std::hex << address << ' ' << std::dec << count << '\n';
#endif
    }
    Sampler(const Sampler &) = delete;
    Sampler &operator=(const Sampler &) = delete;

private:
    std::string path_;
#ifdef _WIN32
    HANDLE target_{};
    std::thread worker_;
    std::atomic<bool> stop_{false};
    std::unordered_map<std::uint64_t, std::uint64_t> counts_;
#endif
};

} // namespace p3p3ds::pc
