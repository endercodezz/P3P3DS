#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace psprecomp {

[[nodiscard]] std::string sha256_bytes(std::span<const std::uint8_t> bytes);
[[nodiscard]] std::string sha256_file(const std::filesystem::path &path);

} // namespace psprecomp
