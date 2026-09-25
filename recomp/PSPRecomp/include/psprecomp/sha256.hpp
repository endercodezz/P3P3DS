#pragma once

#include <filesystem>
#include <string>

namespace psprecomp {

[[nodiscard]] std::string sha256_file(const std::filesystem::path &path);

} // namespace psprecomp
