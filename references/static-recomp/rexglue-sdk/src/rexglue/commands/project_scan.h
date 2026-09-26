/**
 * @file        rexglue/commands/project_scan.h
 * @brief       Project-tree checks that run on every codegen
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace rexglue::cli {

/// A source line a human needs to look at.
struct SourceWarning {
  std::filesystem::path file;
  std::size_t line_number;
  std::string detail;
  std::string hint;
};

std::string RenderRexglueCmake(std::string_view project_name, std::string_view sdk_version,
                               std::string_view entrypoint_out_dir);

/// Rewrite generated/rexglue.cmake when it differs from what the installed SDK
/// renders. Returns true when the file was rewritten.
bool RefreshGeneratedGlue(const std::filesystem::path& project_root, std::string_view project_name,
                          std::string_view sdk_version, std::string_view entrypoint_out_dir);

/// Source files still including headers this run no longer emits.
std::vector<SourceWarning> ScanStaleIncludes(
    const std::filesystem::path& src_dir, const std::unordered_set<std::string>& removed_basenames);

}  // namespace rexglue::cli
