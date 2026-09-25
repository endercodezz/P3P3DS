#include "vcs_bootstrap_paths.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path temp =
        std::filesystem::temp_directory_path() /
        ("vcs_bootstrap_paths_" + std::to_string(nonce));

    try {
        const std::filesystem::path sysdir = temp / "PSP_DATA/PSP_GAME/SYSDIR";
        std::filesystem::create_directories(sysdir);
        const std::filesystem::path auto_eboot = sysdir / "EBOOT_DECRYPTED.ELF";
        std::ofstream(auto_eboot, std::ios::binary).put('\0');

        const char *no_args[]{"VCSNative.exe"};
        const vcs::BootstrapPaths automatic =
            vcs::resolve_bootstrap_paths(1, no_args, temp);
        require(automatic.discovered_from_psp_data,
                "no-argument launch did not select PSP_DATA");
        require(automatic.psp_executable == auto_eboot,
                "no-argument launch selected the wrong EBOOT");
        require(automatic.game_root == temp / "PSP_DATA",
                "no-argument launch selected the wrong asset root");

        const char *explicit_args[]{"VCSNative.exe", "custom/game.elf", "custom/root"};
        const vcs::BootstrapPaths explicit_paths =
            vcs::resolve_bootstrap_paths(3, explicit_args, temp);
        require(!explicit_paths.discovered_from_psp_data,
                "explicit command line unexpectedly selected PSP_DATA");
        require(explicit_paths.psp_executable == std::filesystem::path("custom/game.elf"),
                "explicit EBOOT was not preserved");
        require(explicit_paths.game_root == std::filesystem::path("custom/root"),
                "explicit game root was not preserved");

        std::filesystem::remove_all(temp);
        std::cout << "vcs_bootstrap_paths_tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::error_code ignored;
        std::filesystem::remove_all(temp, ignored);
        std::cerr << "vcs_bootstrap_paths_tests failed: " << error.what() << '\n';
        return 1;
    }
}
