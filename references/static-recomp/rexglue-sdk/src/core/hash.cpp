/**
 * @file        core/hash.cpp
 * @brief       XXH3 digest helpers for build fingerprints and cache keys
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <rex/hash.h>

#include <cstdio>
#include <memory>

#include <fmt/format.h>

#include <rex/filesystem.h>

namespace rex {

namespace {

constexpr size_t kChunkSize = 256 * 1024;

struct FileCloser {
  void operator()(FILE* file) const { fclose(file); }
};

struct StateDeleter {
  void operator()(XXH3_state_t* state) const { XXH3_freeState(state); }
};

std::string HexDigest(XXH128_hash_t digest) {
  return fmt::format("{:016x}{:016x}", digest.high64, digest.low64);
}

}  // namespace

std::string hash_bytes(std::string_view data) {
  return HexDigest(XXH3_128bits(data.data(), data.size()));
}

std::string hash_file(const std::filesystem::path& path) {
  std::unique_ptr<FILE, FileCloser> file(filesystem::OpenFile(path, "rb"));
  if (!file)
    return {};

  std::unique_ptr<XXH3_state_t, StateDeleter> state(XXH3_createState());
  if (!state || XXH3_128bits_reset(state.get()) == XXH_ERROR)
    return {};

  auto chunk = std::make_unique_for_overwrite<char[]>(kChunkSize);
  while (size_t got = fread(chunk.get(), 1, kChunkSize, file.get())) {
    if (XXH3_128bits_update(state.get(), chunk.get(), got) == XXH_ERROR)
      return {};
  }

  // A short read ends the loop the same way EOF does.
  if (ferror(file.get()))
    return {};

  return HexDigest(XXH3_128bits_digest(state.get()));
}

}  // namespace rex
