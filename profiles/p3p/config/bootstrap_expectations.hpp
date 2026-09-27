#pragma once
#include <cstdint>
namespace p3p3ds::profile {
// [VERIFIED] ULUS-10512 relocated ELF / prior canonical bootstrap trace.
inline constexpr std::uint32_t entry=0x08804108, user_main=0x0880421C;
inline constexpr std::uint32_t sdk=0x06020010, compiler=0x00030306;
// Completed return covering the sparse LOCAL_DISPATCH regression (Stop #8).
inline constexpr std::uint32_t return_site=0x08B1D0D8, return_target=0x08AB306C;
inline constexpr std::uint32_t color=0x04000000, display=0x04088000, depth=0x04110000;
inline constexpr std::uint32_t stack_top=0x0A000000, stack_size=0x10000;
}
