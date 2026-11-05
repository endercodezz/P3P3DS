#pragma once
// XXH32 and XXH64 (xxHash specification by Yann Collet, BSD-2-Clause algorithm;
// this is an independent implementation from the specification). Texture packs
// made for PPSSPP key textures by these hashes (texture_replacement.hpp).
// Checked against the xxhash-rust crate by tests/test_texture_replacement.cpp.
#include <cstddef>
#include <cstdint>

namespace p3p3ds::ge {

[[nodiscard]] std::uint32_t xxh32(const void *data, std::size_t size, std::uint32_t seed);
[[nodiscard]] std::uint64_t xxh64(const void *data, std::size_t size, std::uint64_t seed);

} // namespace p3p3ds::ge
