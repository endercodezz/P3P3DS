/**
 * @file        codegen/file_io.h
 * @brief       Content-addressed output writing shared by the codegen writers
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#pragma once

#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <rex/filesystem.h>

namespace rex::codegen {

inline std::optional<std::string> ReadFileBytes(const std::filesystem::path& path) {
  FILE* f = rex::filesystem::OpenFile(path, "rb");
  if (!f)
    return std::nullopt;
  std::string buf;
  char chunk[64 * 1024];
  size_t got = 0;
  while ((got = fread(chunk, 1, sizeof(chunk), f)) > 0) {
    buf.append(chunk, got);
  }
  fclose(f);
  return buf;
}

inline bool WriteFileBytes(const std::filesystem::path& path, std::string_view content) {
  FILE* f = rex::filesystem::OpenFile(path, "wb");
  if (!f)
    return false;
  size_t wrote = fwrite(content.data(), 1, content.size(), f);
  return fclose(f) == 0 && wrote == content.size();
}

enum class WriteOutcome { Failed, Unchanged, Written };

/// Leaves the file untouched when it already holds `content`, so an unchanged
/// output keeps its mtime and does not retrigger a build.
inline WriteOutcome WriteIfChanged(const std::filesystem::path& path, std::string_view content) {
  if (auto existing = ReadFileBytes(path); existing && *existing == content)
    return WriteOutcome::Unchanged;
  return WriteFileBytes(path, content) ? WriteOutcome::Written : WriteOutcome::Failed;
}

}  // namespace rex::codegen
