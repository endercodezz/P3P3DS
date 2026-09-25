#pragma once
#include <cstdint>
#include <filesystem>
#include <span>

namespace vcs::project2dfx_data {

struct LodLight {
    float x{}, y{}, z{};
    float custom_size_multiplier{1.0f};
    float source_far_clip{100.0f};
    float heading{};
    std::int32_t r{}, g{}, b{}, a{255};
    std::int32_t show_mode{};
    std::int32_t no_distance{};
};

enum BlinkType : std::int32_t {
    Default = 0,
    RandomFlashing = 1,
    OneSecondOnOff = 2,
    TwoSecondsOnOff = 3,
    ThreeSecondsOnOff = 4,
    FourSecondsOnOff = 5,
    FiveSecondsOnOff = 6,
    SixSecondsOnFourOff = 7,
};

// Loads the generated portable table shipped beside the INI. For developer
// builds it also accepts the upstream Project2DFX lodl.c directly, making the
// importer/re-generator deterministic and easy to audit.
bool initialize_vcs_lod_lights(const std::filesystem::path &ini_path) noexcept;
std::span<const LodLight> vcs_lod_lights() noexcept;
const std::filesystem::path &vcs_lod_light_source() noexcept;

} // namespace vcs::project2dfx_data
