// p3p_prepare_elf: the Builder's ISO -> eboot.elf path as a host command for
// the developer setup (tools/prepare_game.py delegates to it).
//
//   p3p_prepare_elf <iso> [--decrypted-eboot <elf>] [--output <eboot.elf>]
//
// Prints key=value lines on stdout (disc_id, title, version, source,
// eboot_size, eboot_sha256, elf_size, elf_sha256). With --output the verified
// ELF is written there (through a .part file, so a failure leaves no partial
// ELF). Errors: "error: <message>" on stderr, exit code 1; usage: exit code 2.
#include "game_prep.hpp"

#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace p3p3ds::builder;

namespace {
int usage() {
    std::fprintf(stderr, "usage: p3p_prepare_elf <iso> [--decrypted-eboot <elf>] [--output <eboot.elf>]\n");
    return 2;
}

void write_atomically(const fs::path &target, const std::vector<std::uint8_t> &bytes) {
    if (target.has_parent_path()) fs::create_directories(target.parent_path());
    fs::path part = target;
    part += ".part";
    {
        std::ofstream out(part, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!out) throw std::runtime_error("Cannot write " + part.string());
    }
    fs::rename(part, target); // replaces an existing file
}

int run(const std::vector<fs::path> &args) {
    fs::path iso, override_elf, output;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const auto &a = args[i].native();
        const bool is_output = a == fs::path("--output").native(), is_override = a == fs::path("--decrypted-eboot").native();
        if ((is_output || is_override) && i + 1 < args.size()) {
            (is_output ? output : override_elf) = args[++i];
        } else if (!a.empty() && a[0] != '-' && iso.empty()) {
            iso = args[i];
        } else {
            return usage();
        }
    }
    if (iso.empty()) return usage();
    try {
        const PreparedExecutable exe = prepare_executable(iso, override_elf);
        if (!output.empty()) write_atomically(output, exe.elf);
        std::printf("disc_id=%s\ntitle=%s\nversion=%s\nsource=%s\n", exe.disc_id.c_str(), exe.title.c_str(), exe.version.c_str(),
                    exe.from_override ? "override" : "iso");
        if (!exe.from_override)
            std::printf("eboot_size=%llu\neboot_sha256=%s\n", static_cast<unsigned long long>(exe.eboot_size), exe.eboot_sha256.c_str());
        std::printf("elf_size=%llu\nelf_sha256=%s\n", static_cast<unsigned long long>(exe.elf.size()), exe.elf_sha256.c_str());
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
} // namespace

#ifdef _WIN32
// Wide arguments: ISO paths outside the ANSI code page must survive.
int wmain(int argc, wchar_t **argv) {
    std::vector<fs::path> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
    return run(args);
}
#else
int main(int argc, char **argv) {
    std::vector<fs::path> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
    return run(args);
}
#endif
