#include "vcs_bootstrap_paths.hpp"

#include "psprecomp/common.hpp"

#include <array>
#include <sstream>

namespace vcs {

BootstrapPaths resolve_bootstrap_paths(
    int argc, const char *const *argv,
    const std::filesystem::path &executable_directory) {
    if (argc >= 2 && argv != nullptr && argv[1] != nullptr && *argv[1] != '\0') {
        const std::filesystem::path psp_executable = argv[1];
        return {
            psp_executable,
            argc >= 3 && argv[2] != nullptr && *argv[2] != '\0'
                ? std::filesystem::path(argv[2])
                : psp_executable.parent_path().parent_path(),
            false,
        };
    }

    const std::filesystem::path root = executable_directory / "PSP_DATA";
    constexpr std::array<const char *, 8> candidates{
        "PSP_GAME/SYSDIR/EBOOT_DECRYPTED.ELF",
        "EBOOT_DECRYPTED.ELF",
        "PSP_GAME/SYSDIR/EBOOT_DECRYPTED.BIN",
        "EBOOT_DECRYPTED.BIN",
        "PSP_GAME/SYSDIR/BOOT.BIN",
        "BOOT.BIN",
        "PSP_GAME/SYSDIR/EBOOT.BIN",
        "EBOOT.BIN",
    };

    for (const char *relative : candidates) {
        const std::filesystem::path candidate = root / relative;
        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error))
            return {candidate, root, true};
    }

    std::ostringstream message;
    message << "PSP_DATA was not found or contains no decrypted EBOOT.\n"
            << "For direct launch, place the extracted game at:\n  "
            << root.string() << "\n"
            << "Recommended EBOOT path:\n  "
            << (root / "PSP_GAME/SYSDIR/EBOOT_DECRYPTED.ELF").string() << "\n"
            << "Command-line launch remains available:\n"
            << "  VCSNative <decrypted EBOOT> [game_root]";
    throw psprecomp::Error(message.str());
}

} // namespace vcs
