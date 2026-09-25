#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace vcs {

struct VcsConfiguration;

void runtime_log_initialize(const VcsConfiguration &configuration);
void runtime_log_shutdown() noexcept;
[[nodiscard]] bool runtime_log_enabled() noexcept;
[[nodiscard]] std::filesystem::path runtime_log_path();
void runtime_log_line(std::string_view line);
void runtime_log_error(std::string_view category, std::string_view message);

} // namespace vcs
