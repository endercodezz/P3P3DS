// See pipeline.hpp. Windows-only (CreateProcessW job pool).
#include "pipeline.hpp"

#include "psp_eboot.hpp"
#include "p3p3ds/vfs.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
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
    "-DPSPRECOMP_AOT_PRODUCTION_FASTPATHS=1 -DPSPRECOMP_NO_FRONTIER_DIAGNOSTICS=1 -DPSPRECOMP_CHAIN_NOINLINE=1";
constexpr const char *kArchFlags = "-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -mword-relocations";

std::wstring widen(const std::string &s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}
std::wstring quote(const fs::path &p) { return L"\"" + p.wstring() + L"\""; }
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
    const fs::path need[] = {"devkitARM/bin/arm-none-eabi-g++.exe", "libctru/lib/libctru.a", "libctru/lib/libcitro3d.a",
                             "libctru/default_icon.png", "tools/bin/3dsxtool.exe", "tools/bin/smdhtool.exe"};
    for (const auto &n : need)
        if (!fs::exists(root / n)) return "Missing " + (root / n).generic_string() + ": install the \"3DS Development\" packages.";
    return {};
}

fs::path Pipeline::find_devkitpro() {
    std::vector<fs::path> candidates;
    if (const char *env = std::getenv("DEVKITPRO")) candidates.emplace_back(env); // usable only when it is a Windows path
    for (const char *drive : {"C:", "D:", "E:"}) candidates.emplace_back(std::string(drive) + "\\devkitPro");
    for (const auto &c : candidates)
        if (check_devkitpro(c).empty()) return c;
    return {};
}

unsigned Pipeline::automatic_jobs() {
    const unsigned cores = std::max<unsigned>(1, GetActiveProcessorCount(ALL_PROCESSOR_GROUPS));
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof mem;
    GlobalMemoryStatusEx(&mem);
    // Measured: 383 MiB peak working set per compiler on the largest units
    // (-Os, devkitARM GCC 16.1); 450 MiB leaves headroom. Keep 1.5 GiB for the system.
    const auto avail_mib = static_cast<long long>(mem.ullAvailPhys / (1024 * 1024));
    const unsigned by_memory = static_cast<unsigned>(std::max<long long>(1, (avail_mib - 1536) / 450));
    return std::max<unsigned>(1, std::min(cores, by_memory));
}

int Pipeline::run_tool(const std::wstring &command, const fs::path &log_file) {
    SECURITY_ATTRIBUTES sa{sizeof sa, nullptr, TRUE};
    HANDLE out = CreateFileW(log_file.wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    STARTUPINFOW si{};
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = si.hStdError = out;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    std::wstring cmd = command;
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(out);
        throw std::runtime_error("Cannot start a build tool (Windows error " + std::to_string(GetLastError()) + ").");
    }
    if (job_) AssignProcessToJobObject(static_cast<HANDLE>(job_), pi.hProcess);
    CloseHandle(out);
    while (WaitForSingleObject(pi.hProcess, 200) == WAIT_TIMEOUT)
        if (cancelled_) TerminateProcess(pi.hProcess, 1);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (cancelled_) throw std::runtime_error("Cancelled.");
    return static_cast<int>(code);
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
    const std::wstring cmd = quote(s_.sdk / "bin" / "psp_recomp.exe") + L" " + quote(elf_) + L" --auto " + quote(gen_) + L" " +
                             widen(kLoadBase) + L" " + widen(kUnitSpan) + L" " + quote(s_.sdk / "patches.txt") + L" --no-transfer-records";
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
    const fs::path gxx = s_.devkitpro / "devkitARM" / "bin" / "arm-none-eabi-g++.exe";
    const std::wstring common = widen(kCompileFlags) + L" -I" + quote(gen_) + L" -I" + quote(s_.sdk / "include") + L" -isystem " +
                                quote(s_.devkitpro / "libctru" / "include");
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

    struct Active { PROCESS_INFORMATION pi; Unit unit; fs::path log; };
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
        for (auto &a : active) { TerminateProcess(a.pi.hProcess, 1); CloseHandle(a.pi.hProcess); CloseHandle(a.pi.hThread); }
        active.clear();
    };
    report();
    while (next < pending.size() || !active.empty()) {
        while (next < pending.size() && active.size() < jobs) {
            const Unit &u = pending[next++];
            const fs::path log = fs::path(u.obj).replace_extension(".log");
            const std::wstring cmd = quote(gxx) + L" " + common + L" -c " + quote(u.src) + L" -o " + quote(u.obj);
            SECURITY_ATTRIBUTES sa{sizeof sa, nullptr, TRUE};
            HANDLE out = CreateFileW(log.wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            STARTUPINFOW si{};
            si.cb = sizeof si;
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdOutput = si.hStdError = out;
            PROCESS_INFORMATION pi{};
            std::wstring mutable_cmd = cmd;
            const BOOL ok = CreateProcessW(nullptr, mutable_cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW | BELOW_NORMAL_PRIORITY_CLASS,
                                           nullptr, nullptr, &si, &pi);
            CloseHandle(out);
            if (!ok) { kill_all(); throw std::runtime_error("Cannot start the devkitARM compiler (Windows error " + std::to_string(GetLastError()) + ")."); }
            if (job_) AssignProcessToJobObject(static_cast<HANDLE>(job_), pi.hProcess);
            active.push_back({pi, u, log});
        }
        std::vector<HANDLE> handles;
        for (auto &a : active) handles.push_back(a.pi.hProcess);
        const DWORD w = WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), FALSE, 250);
        if (cancelled_) { kill_all(); throw std::runtime_error("Cancelled."); }
        if (w == WAIT_TIMEOUT) { report(); continue; }
        if (w >= WAIT_OBJECT_0 + handles.size()) { kill_all(); throw std::runtime_error("Waiting for the compiler failed."); }
        Active a = active[w - WAIT_OBJECT_0];
        active.erase(active.begin() + (w - WAIT_OBJECT_0));
        DWORD code = 1;
        GetExitCodeProcess(a.pi.hProcess, &code);
        CloseHandle(a.pi.hProcess);
        CloseHandle(a.pi.hThread);
        if (code != 0) {
            kill_all();
            throw std::runtime_error("Compiling " + a.unit.src.filename().string() + " failed:\n" + read_text(a.log));
        }
        write_text(fs::path(a.unit.obj).replace_extension(".stamp"), flags_stamp);
        done_bytes += a.unit.bytes;
        ++done;
        report();
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
    if (run_tool(quote(dkp / "devkitARM" / "bin" / "arm-none-eabi-g++.exe") + L" @" + quote(rsp), link_log) != 0)
        throw std::runtime_error("Linking failed:\n" + read_text(link_log));

    progress("Packaging p3p3ds.3dsx", 0.98);
    const fs::path smdh = s_.work / "p3p3ds.smdh", romfs = s_.work / "romfs";
    fs::create_directories(romfs);
    fs::copy_file(elf_, romfs / "eboot.elf", fs::copy_options::overwrite_existing);
    const fs::path tool_log = s_.work / "package.log";
    if (run_tool(quote(dkp / "tools" / "bin" / "smdhtool.exe") + L" --create \"P3P3DS\" \"Persona 3 Portable recompilation\" \"enderlit aka endercodezz\" " +
                     quote(dkp / "libctru" / "default_icon.png") + L" " + quote(smdh), tool_log) != 0)
        throw std::runtime_error("smdhtool failed:\n" + read_text(tool_log));
    fs::create_directories(s_.output.parent_path());
    if (run_tool(quote(dkp / "tools" / "bin" / "3dsxtool.exe") + L" " + quote(s_.work / "p3p3ds.elf") + L" " + quote(s_.output) +
                     L" --smdh=" + quote(smdh) + L" --romfs=" + quote(romfs), tool_log) != 0)
        throw std::runtime_error("3dsxtool failed:\n" + read_text(tool_log));
    progress("Done", 1.0);
}

void Pipeline::run() {
    // Children (generator, compilers, packaging tools) are killed with the builder.
    if (!job_) {
        job_ = CreateJobObjectW(nullptr, nullptr);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (job_) SetInformationJobObject(static_cast<HANDLE>(job_), JobObjectExtendedLimitInformation, &info, sizeof info);
    }
    fs::create_directories(s_.work);
    if (const auto problem = check_devkitpro(s_.devkitpro); !problem.empty()) throw std::runtime_error(problem);
    if (!fs::exists(s_.sdk / "bin" / "psp_recomp.exe")) throw std::runtime_error("The builder's sdk folder is incomplete: reinstall the builder.");
    decrypt();
    generate();
    compile();
    link_and_package();
}

} // namespace p3p3ds::builder
