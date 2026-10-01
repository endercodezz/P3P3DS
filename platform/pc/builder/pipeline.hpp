#pragma once
// P3P3DS Builder pipeline: user's ULUS-10512 ISO -> p3p3ds.3dsx.
//
//   1. read EBOOT.BIN and PARAM.SFO from the ISO, check the disc ID
//   2. decrypt EBOOT.BIN (psp_eboot.cpp), check the reference SHA-256
//   3. generate the AOT C++ units (bundled psp_recomp.exe)
//   4. compile them with the user's devkitARM, one job per core
//   5. link with the prebuilt runtime objects shipped in sdk/lib
//   6. smdhtool + 3dsxtool (romfs: the decrypted EBOOT)
//
// Everything the game is not involved in (runtime, HLE, GPU renderer) is
// prebuilt in the release package; only code derived from the user's own
// executable is generated and compiled on the user's machine. Steps 3-4 are
// cached in the work directory, so an interrupted build resumes.
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace p3p3ds::builder {

struct Settings {
    std::filesystem::path iso, devkitpro, sdk, work, output; // output: the .3dsx file
    unsigned jobs{};                                          // 0: automatic
};

struct Progress {
    std::string step;        // short user-facing description
    double fraction{};       // 0..1 over the whole build
    double eta_seconds{-1};  // < 0: unknown
    unsigned units_done{}, units_total{};
};

class Pipeline {
public:
    using Report = std::function<void(const Progress &)>;
    using Log = std::function<void(const std::string &)>;
    Pipeline(Settings settings, Report report, Log log);
    // Runs the whole build; throws std::runtime_error with a user-facing message.
    void run();
    void cancel() { cancelled_ = true; }

    // Validation helpers for the wizard.
    static std::string check_iso(const std::filesystem::path &iso);           // "" when it is ULUS-10512
    static std::string check_devkitpro(const std::filesystem::path &root);    // "" when usable
    static std::filesystem::path find_devkitpro();                            // best guess, may be empty
    static unsigned automatic_jobs();

private:
    Settings s_;
    Report report_;
    Log log_;
    std::atomic<bool> cancelled_{false};
    std::filesystem::path elf_, gen_, obj_;
    void *job_{}; // Windows job object: child tools die with the builder

    void decrypt();
    void generate();
    void compile();
    void link_and_package();
    void progress(const std::string &step, double fraction, double eta = -1, unsigned done = 0, unsigned total = 0);
    int run_tool(const std::wstring &command, const std::filesystem::path &log_file);
};

} // namespace p3p3ds::builder
