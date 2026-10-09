// See pipeline.hpp.
#include "pipeline.hpp"

#include "psp_eboot.hpp"
#include "p3p3ds/vfs.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace p3p3ds::builder {
namespace {

constexpr unsigned kUnitCount = 237;                 // profiles/p3p/config/aot_layout.cmake
constexpr const char *kLoadBase = "0x08804000";
constexpr const char *kUnitSpan = "16384";
// Measured on the largest unit and the whole program (docs/3DS_PLATFORM.md
// section 8.5): GCSE and instruction scheduling cost 27 % of the compile
// time for 0.8 % of code size.
constexpr const char *kCompileFlags =
    "-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -mword-relocations -ffunction-sections "
    "-D__3DS__ -DNDEBUG -std=gnu++20 -Os -fno-gcse -fno-schedule-insns -fno-schedule-insns2 "
    "-DPSPRECOMP_AOT_PRODUCTION_FASTPATHS=1 -DPSPRECOMP_NO_FRONTIER_DIAGNOSTICS=1 -DPSPRECOMP_CHAIN_NOINLINE=1 "
    "-DPSPRECOMP_AOT_ASSUME_NO_WRITE_WATCH=1";
constexpr const char *kArchFlags = "-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -mword-relocations";
#ifdef _WIN32
constexpr const char *kExe = ".exe";
#else
constexpr const char *kExe = "";
#endif

using Args = std::vector<fs::path>;

fs::path tool(const fs::path &dir, const char *name) { return dir / (std::string(name) + kExe); }
// "-I" + path as one argument, without converting the path through a narrow encoding.
fs::path joined(const char *flag, const fs::path &p) {
    fs::path r(flag);
    r += p.native();
    return r;
}
void append_words(Args &args, const char *words) {
    std::istringstream in(words);
    for (std::string w; in >> w;) args.emplace_back(w);
}

// ---- Child processes: stdout and stderr redirected to a log file ----------
#ifdef _WIN32
struct Child { HANDLE process{}; };

// CommandLineToArgvW quoting rules.
std::wstring quote_arg(const std::wstring &a) {
    if (!a.empty() && a.find_first_of(L" \t\"") == std::wstring::npos) return a;
    std::wstring r = L"\"";
    std::size_t slashes = 0;
    for (const wchar_t c : a) {
        if (c == L'\\') { ++slashes; continue; }
        r.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        slashes = 0;
        r += c;
    }
    r.append(slashes * 2, L'\\');
    return r + L"\"";
}

Child spawn_child(const Args &argv, const fs::path &log, void *job, bool low_priority) {
    std::wstring cmd;
    for (const auto &a : argv) cmd += (cmd.empty() ? L"" : L" ") + quote_arg(a.wstring());
    SECURITY_ATTRIBUTES sa{sizeof sa, nullptr, TRUE};
    HANDLE out = CreateFileW(log.wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (out == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create " + log.string() + ".");
    STARTUPINFOW si{};
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = si.hStdError = out;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    const DWORD flags = CREATE_NO_WINDOW | (low_priority ? BELOW_NORMAL_PRIORITY_CLASS : 0);
    const BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, flags, nullptr, nullptr, &si, &pi);
    CloseHandle(out);
    if (!ok)
        throw std::runtime_error("Cannot start " + argv[0].filename().string() + " (Windows error " + std::to_string(GetLastError()) + ").");
    if (job) AssignProcessToJobObject(static_cast<HANDLE>(job), pi.hProcess);
    CloseHandle(pi.hThread);
    return {pi.hProcess};
}

// True once the child has exited; `code` is its exit code.
bool child_done(Child &c, int &code) {
    if (WaitForSingleObject(c.process, 0) == WAIT_TIMEOUT) return false;
    DWORD exit_code = 1;
    GetExitCodeProcess(c.process, &exit_code);
    CloseHandle(c.process);
    c.process = nullptr;
    code = static_cast<int>(exit_code);
    return true;
}

void kill_child(Child &c) {
    if (!c.process) return;
    TerminateProcess(c.process, 1);
    WaitForSingleObject(c.process, 5000);
    CloseHandle(c.process);
    c.process = nullptr;
}
#else
struct Child { pid_t pid{-1}; };

Child spawn_child(const Args &argv, const fs::path &log, void *, bool low_priority) {
    std::vector<std::string> strings;
    for (const auto &a : argv) strings.push_back(a.string());
    std::vector<char *> ptrs;
    for (auto &s : strings) ptrs.push_back(s.data());
    ptrs.push_back(nullptr);
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);
    pid_t pid = -1;
    const int rc = posix_spawn(&pid, ptrs[0], &actions, nullptr, ptrs.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    if (rc != 0) throw std::runtime_error("Cannot start " + strings[0] + ": " + std::strerror(rc) + ".");
    if (low_priority) setpriority(PRIO_PROCESS, static_cast<id_t>(pid), 10); // like BELOW_NORMAL_PRIORITY_CLASS
    return {pid};
}

// True once the child has exited; `code` is its exit status (128 + signal when killed).
bool child_done(Child &c, int &code) {
    int status = 0;
    const pid_t r = waitpid(c.pid, &status, WNOHANG);
    if (r == 0) return false;
    code = r < 0 ? 1 : WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    c.pid = -1;
    return true;
}

void kill_child(Child &c) {
    if (c.pid <= 0) return;
    kill(c.pid, SIGKILL);
    int status = 0;
    waitpid(c.pid, &status, 0);
    c.pid = -1;
}
#endif

// gcc response files treat backslashes as escapes: forward slashes only.
std::string rsp_path(const fs::path &p) {
    const auto u = p.generic_u8string();
    return "\"" + std::string(u.begin(), u.end()) + "\"";
}

std::string read_text(const fs::path &p, std::size_t limit = 4000) {
    std::ifstream in(p, std::ios::binary);
    std::string s((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (s.size() > limit) s = s.substr(0, limit) + "\n...";
    return s;
}
void write_text(const fs::path &p, const std::string &text) { std::ofstream(p, std::ios::binary) << text; }
std::vector<std::uint8_t> read_source(const std::shared_ptr<const vfs::Source> &src) {
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(src->size()));
    if (!bytes.empty()) src->read(0, bytes.data(), bytes.size());
    return bytes;
}

// PARAM.SFO -> value of `key` (string entries only).
std::string sfo_string(const std::vector<std::uint8_t> &d, const std::string &key) {
    auto le16 = [&](std::size_t o) { return static_cast<std::uint32_t>(d[o] | (d[o + 1] << 8)); };
    auto le32 = [&](std::size_t o) { return d[o] | (d[o + 1] << 8) | (d[o + 2] << 16) | (static_cast<std::uint32_t>(d[o + 3]) << 24); };
    if (d.size() < 20 || std::memcmp(d.data(), "\0PSF", 4) != 0) return {};
    const std::uint32_t keys = le32(8), values = le32(12), count = le32(16);
    for (std::uint32_t i = 0; i < count && 20 + i * 16 + 16 <= d.size(); ++i) {
        const std::size_t e = 20 + i * 16;
        const std::size_t name_at = keys + le16(e);
        if (name_at >= d.size()) continue;
        const std::string name(reinterpret_cast<const char *>(&d[name_at]));
        if (name != key) continue;
        const std::size_t at = values + le32(e + 12), len = le32(e + 4);
        if (at + len > d.size()) return {};
        return std::string(reinterpret_cast<const char *>(&d[at]), strnlen(reinterpret_cast<const char *>(&d[at]), len));
    }
    return {};
}

std::uint64_t file_size_or_zero(const fs::path &p) {
    std::error_code ec;
    const auto n = fs::file_size(p, ec);
    return ec ? 0 : n;
}

} // namespace

Pipeline::Pipeline(Settings settings, Report report, Log log)
    : s_(std::move(settings)), report_(std::move(report)), log_(std::move(log)) {}

void Pipeline::progress(const std::string &step, double fraction, double eta, unsigned done, unsigned total) {
    if (report_) report_(Progress{step, fraction, eta, done, total});
}

std::string Pipeline::check_iso(const fs::path &iso) {
    try {
        vfs::IsoFileSystem fsys(iso);
        const auto sfo = fsys.open("PSP_GAME/PARAM.SFO");
        if (!sfo) return "This image has no PSP_GAME/PARAM.SFO: it is not a PSP game.";
        const auto id = sfo_string(read_source(sfo), "DISC_ID");
        if (id != "ULUS10512") return "This is " + (id.empty() ? std::string("an unknown disc") : id) + ", not Persona 3 Portable ULUS-10512.";
        if (!fsys.open("PSP_GAME/SYSDIR/EBOOT.BIN")) return "EBOOT.BIN is missing from the image.";
        return {};
    } catch (const std::exception &e) {
        return std::string("Cannot read the image: ") + e.what();
    }
}

std::string Pipeline::check_devkitpro(const fs::path &root) {
    if (root.empty()) return "devkitPro was not found.";
    const fs::path need[] = {tool("devkitARM/bin", "arm-none-eabi-g++"), "libctru/lib/libctru.a", "libctru/lib/libcitro3d.a",
                             "libctru/default_icon.png", tool("tools/bin", "3dsxtool"), tool("tools/bin", "smdhtool")};
    for (const auto &n : need)
        if (!fs::exists(root / n)) return "Missing " + (root / n).generic_string() + ": install the \"3DS Development\" packages.";
    return {};
}

fs::path Pipeline::find_devkitpro() {
    std::vector<fs::path> candidates;
    if (const char *env = std::getenv("DEVKITPRO")) candidates.emplace_back(env); // on Windows usable only when it is a Windows path
#ifdef _WIN32
    for (const char *drive : {"C:", "D:", "E:"}) candidates.emplace_back(std::string(drive) + "\\devkitPro");
#else
    candidates.emplace_back("/opt/devkitpro"); // devkitPro pacman default
#endif
    for (const auto &c : candidates)
        if (check_devkitpro(c).empty()) return c;
    return {};
}

unsigned Pipeline::automatic_jobs() {
#ifdef _WIN32
    const unsigned cores = std::max<unsigned>(1, GetActiveProcessorCount(ALL_PROCESSOR_GROUPS));
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof mem;
    GlobalMemoryStatusEx(&mem);
    const auto avail_mib = static_cast<long long>(mem.ullAvailPhys / (1024 * 1024));
#else
    const unsigned cores = std::max<unsigned>(1, std::thread::hardware_concurrency());
    // MemAvailable counts reclaimable page cache, like ullAvailPhys on Windows.
    long long avail_mib = -1;
    std::ifstream meminfo("/proc/meminfo");
    for (std::string line; avail_mib < 0 && std::getline(meminfo, line);)
        if (line.rfind("MemAvailable:", 0) == 0) avail_mib = std::atoll(line.c_str() + 13) / 1024;
    if (avail_mib < 0) return cores;
#endif
    // Measured: 383 MiB peak working set per compiler on the largest units
    // (-Os, devkitARM GCC 16.1); 450 MiB leaves headroom. Keep 1.5 GiB for the system.
    const unsigned by_memory = static_cast<unsigned>(std::max<long long>(1, (avail_mib - 1536) / 450));
    return std::max<unsigned>(1, std::min(cores, by_memory));
}

int Pipeline::run_tool(const Args &argv, const fs::path &log_file) {
    Child child = spawn_child(argv, log_file, job_, false);
    int code = 1;
    while (!child_done(child, code)) {
        if (cancelled_) { kill_child(child); throw std::runtime_error("Cancelled."); }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    if (cancelled_) throw std::runtime_error("Cancelled.");
    return code;
}

void Pipeline::decrypt() {
    progress("Reading the game image", 0.0);
    if (const auto problem = check_iso(s_.iso); !problem.empty()) throw std::runtime_error(problem);
    vfs::IsoFileSystem fsys(s_.iso);
    const auto eboot = read_source(fsys.open("PSP_GAME/SYSDIR/EBOOT.BIN"));
    progress("Decrypting the game executable", 0.01);
    const auto elf = decrypt_eboot(eboot);
    const auto sha = sha256_hex(elf);
    if (sha != kExpectedElfSha256)
        throw std::runtime_error("The decrypted executable does not match ULUS-10512 (SHA-256 " + sha + "). Is the image modified?");
    elf_ = s_.work / "eboot.elf";
    if (!fs::exists(elf_) || sha256_hex([&] { std::ifstream in(elf_, std::ios::binary); return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(in)), {}); }()) != sha) {
        std::ofstream(elf_, std::ios::binary).write(reinterpret_cast<const char *>(elf.data()), static_cast<std::streamsize>(elf.size()));
    }
    if (log_) log_("Executable decrypted, SHA-256 " + sha);
}

void Pipeline::generate() {
    gen_ = s_.work / "generated";
    const fs::path stamp = gen_ / "builder.stamp";
    const std::string want = std::string(kExpectedElfSha256) + " " + read_text(s_.sdk / "VERSION", 200);
    if (fs::exists(stamp) && read_text(stamp, 400) == want) {
        if (log_) log_("Generated code is up to date.");
        return;
    }
    progress("Translating the game code to C++", 0.02);
    std::error_code ec;
    fs::remove_all(gen_, ec);
    fs::create_directories(gen_);
    const Args cmd = {tool(s_.sdk / "bin", "psp_recomp"), elf_, "--auto", gen_, kLoadBase, kUnitSpan, s_.sdk / "patches.txt",
                      "--no-transfer-records"};
    const fs::path log = s_.work / "generate.log";
    if (run_tool(cmd, log) != 0) throw std::runtime_error("Code generation failed:\n" + read_text(log));
    unsigned units = 0;
    for (const auto &e : fs::directory_iterator(gen_))
        if (e.path().filename().string().rfind("generated_unit_", 0) == 0) ++units;
    if (units != kUnitCount) throw std::runtime_error("Code generation produced " + std::to_string(units) + " units, expected 237.");
    write_text(stamp, want);
}

void Pipeline::compile() {
    obj_ = s_.work / "obj";
    fs::create_directories(obj_);
    Args common = {tool(s_.devkitpro / "devkitARM" / "bin", "arm-none-eabi-g++")};
    append_words(common, kCompileFlags);
    common.insert(common.end(), {joined("-I", gen_), joined("-I", s_.sdk / "include"), "-isystem", s_.devkitpro / "libctru" / "include"});
    const std::string flags_stamp = std::string(kCompileFlags) + " " + read_text(gen_ / "builder.stamp", 400);

    struct Unit { fs::path src, obj; std::uint64_t bytes; };
    std::vector<Unit> pending;
    std::uint64_t total_bytes = 0, done_bytes = 0;
    unsigned total = 0, done = 0;
    for (const auto &e : fs::directory_iterator(gen_)) {
        if (e.path().extension() != ".cpp") continue;
        Unit u{e.path(), obj_ / (e.path().stem().string() + ".o"), file_size_or_zero(e.path())};
        total_bytes += u.bytes;
        ++total;
        const fs::path stamp = fs::path(u.obj).replace_extension(".stamp");
        if (fs::exists(u.obj) && read_text(stamp, 1000) == flags_stamp) { done_bytes += u.bytes; ++done; continue; }
        pending.push_back(u);
    }
    // Largest units first: they dominate the tail of a parallel build.
    std::sort(pending.begin(), pending.end(), [](const Unit &a, const Unit &b) { return a.bytes > b.bytes; });
    const unsigned jobs = s_.jobs ? s_.jobs : automatic_jobs();
    if (log_) log_("Compiling " + std::to_string(pending.size()) + " of " + std::to_string(total) + " units with " + std::to_string(jobs) + " parallel jobs.");

    struct Active { Child child; Unit unit; fs::path log; };
    std::vector<Active> active;
    const auto start = std::chrono::steady_clock::now();
    const std::uint64_t start_bytes = done_bytes;
    std::size_t next = 0;
    auto report = [&] {
        const double frac = total_bytes ? static_cast<double>(done_bytes) / static_cast<double>(total_bytes) : 1.0;
        double eta = -1;
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        const auto compiled = done_bytes - start_bytes, remaining = total_bytes - done_bytes;
        if (compiled > total_bytes / 20 && elapsed > 5) eta = elapsed * static_cast<double>(remaining) / static_cast<double>(compiled);
        progress("Compiling the game code for the 3DS", 0.12 + 0.76 * frac, eta, done, total);
    };
    auto kill_all = [&] {
        for (auto &a : active) kill_child(a.child);
        active.clear();
    };
    report();
    while (next < pending.size() || !active.empty()) {
        while (next < pending.size() && active.size() < jobs) {
            const Unit &u = pending[next++];
            const fs::path log = fs::path(u.obj).replace_extension(".log");
            Args cmd = common;
            cmd.insert(cmd.end(), {"-c", u.src, "-o", u.obj});
            try {
                active.push_back({spawn_child(cmd, log, job_, true), u, log});
            } catch (...) {
                kill_all();
                throw;
            }
        }
        if (cancelled_) { kill_all(); throw std::runtime_error("Cancelled."); }
        bool finished = false;
        for (std::size_t i = 0; i < active.size();) {
            int code = 1;
            if (!child_done(active[i].child, code)) { ++i; continue; }
            const Active a = active[i];
            active.erase(active.begin() + static_cast<std::ptrdiff_t>(i));
            if (code != 0) {
                kill_all();
                throw std::runtime_error("Compiling " + a.unit.src.filename().string() + " failed:\n" + read_text(a.log));
            }
            write_text(fs::path(a.unit.obj).replace_extension(".stamp"), flags_stamp);
            done_bytes += a.unit.bytes;
            ++done;
            finished = true;
            report();
        }
        if (!finished) {
            report();
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }
}

void Pipeline::link_and_package() {
    progress("Linking", 0.88);
    const fs::path dkp = s_.devkitpro, lib = s_.sdk / "lib";
    const fs::path rsp = s_.work / "link.rsp";
    {
        std::ostringstream r;
        r << kArchFlags << " -L" << rsp_path(dkp / "libctru" / "lib") << " -specs=3dsx.specs\n";
        for (const char *o : {"main.o", "gpu_renderer.o", "ge_shbin.o"}) r << rsp_path(lib / o) << "\n";
        for (const auto &e : fs::directory_iterator(obj_))
            if (e.path().extension() == ".o") r << rsp_path(e.path()) << "\n";
        r << "-o " << rsp_path(s_.work / "p3p3ds.elf") << "\n";
        r << rsp_path(lib / "libp3p3ds_core.a") << " -lcitro3d " << rsp_path(lib / "libpsprecomp_runtime.a") << " -lctru -lm\n";
        write_text(rsp, r.str());
    }
    const fs::path link_log = s_.work / "link.log";
    if (run_tool({tool(dkp / "devkitARM" / "bin", "arm-none-eabi-g++"), joined("@", rsp)}, link_log) != 0)
        throw std::runtime_error("Linking failed:\n" + read_text(link_log));

    progress("Packaging p3p3ds.3dsx", 0.98);
    const fs::path smdh = s_.work / "p3p3ds.smdh", romfs = s_.work / "romfs";
    fs::create_directories(romfs);
    fs::copy_file(elf_, romfs / "eboot.elf", fs::copy_options::overwrite_existing);
    const fs::path tool_log = s_.work / "package.log";
    if (run_tool({tool(dkp / "tools" / "bin", "smdhtool"), "--create", "P3P3DS", "Persona 3 Portable recompilation", "enderlit aka endercodezz",
                  dkp / "libctru" / "default_icon.png", smdh},
                 tool_log) != 0)
        throw std::runtime_error("smdhtool failed:\n" + read_text(tool_log));
    if (!s_.output.parent_path().empty()) fs::create_directories(s_.output.parent_path());
    if (run_tool({tool(dkp / "tools" / "bin", "3dsxtool"), s_.work / "p3p3ds.elf", s_.output, joined("--smdh=", smdh), joined("--romfs=", romfs)},
                 tool_log) != 0)
        throw std::runtime_error("3dsxtool failed:\n" + read_text(tool_log));
    progress("Done", 1.0);
}

void Pipeline::run() {
#ifdef _WIN32
    // Children (generator, compilers, packaging tools) are killed with the builder.
    // On POSIX they share the builder's process group, so Ctrl+C reaches them too.
    if (!job_) {
        job_ = CreateJobObjectW(nullptr, nullptr);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (job_) SetInformationJobObject(static_cast<HANDLE>(job_), JobObjectExtendedLimitInformation, &info, sizeof info);
    }
#endif
    fs::create_directories(s_.work);
    if (const auto problem = check_devkitpro(s_.devkitpro); !problem.empty()) throw std::runtime_error(problem);
    if (!fs::exists(tool(s_.sdk / "bin", "psp_recomp"))) throw std::runtime_error("The builder's sdk folder is incomplete: reinstall the builder.");
    decrypt();
    generate();
    compile();
    link_and_package();
}

} // namespace p3p3ds::builder
