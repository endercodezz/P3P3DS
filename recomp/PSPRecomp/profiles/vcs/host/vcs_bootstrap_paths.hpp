#pragma once

#include <filesystem>

namespace vcs {

struct BootstrapPaths {
    std::filesystem::path psp_executable;
    std::filesystem::path game_root;
    bool discovered_from_psp_data{};
};

// Explicit command-line paths retain the old behavior. With no arguments,
// discover both the decrypted EBOOT and assets below <exe_dir>/PSP_DATA.
[[nodiscard]] BootstrapPaths resolve_bootstrap_paths(
    int argc, const char *const *argv,
    const std::filesystem::path &executable_directory);

} // namespace vcs
