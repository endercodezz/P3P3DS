// See xxhash.hpp. Little-endian reads, as the specification defines.
#include "p3p3ds/ge/xxhash.hpp"

namespace p3p3ds::ge {
namespace {

constexpr std::uint32_t P32_1 = 0x9E3779B1u, P32_2 = 0x85EBCA77u, P32_3 = 0xC2B2AE3Du, P32_4 = 0x27D4EB2Fu, P32_5 = 0x165667B1u;
constexpr std::uint64_t P64_1 = 0x9E3779B185EBCA87ull, P64_2 = 0xC2B2AE3D27D4EB4Full, P64_3 = 0x165667B19E3779F9ull,
                        P64_4 = 0x85EBCA77C2B2AE63ull, P64_5 = 0x27D4EB2F165667C5ull;

inline std::uint32_t rotl32(std::uint32_t x, int r) { return (x << r) | (x >> (32 - r)); }
inline std::uint64_t rotl64(std::uint64_t x, int r) { return (x << r) | (x >> (64 - r)); }

inline std::uint32_t read32(const std::uint8_t *p) {
    return static_cast<std::uint32_t>(p[0]) | static_cast<std::uint32_t>(p[1]) << 8 | static_cast<std::uint32_t>(p[2]) << 16 |
           static_cast<std::uint32_t>(p[3]) << 24;
}
inline std::uint64_t read64(const std::uint8_t *p) {
    return static_cast<std::uint64_t>(read32(p)) | static_cast<std::uint64_t>(read32(p + 4)) << 32;
}

inline std::uint32_t round32(std::uint32_t acc, std::uint32_t lane) { return rotl32(acc + lane * P32_2, 13) * P32_1; }
inline std::uint64_t round64(std::uint64_t acc, std::uint64_t lane) { return rotl64(acc + lane * P64_2, 31) * P64_1; }
inline std::uint64_t merge64(std::uint64_t acc, std::uint64_t v) { return (acc ^ round64(0, v)) * P64_1 + P64_4; }

} // namespace

std::uint32_t xxh32(const void *data, std::size_t size, std::uint32_t seed) {
    const auto *p = static_cast<const std::uint8_t *>(data);
    const std::uint8_t *const end = p + size;
    std::uint32_t h;
    if (size >= 16) {
        std::uint32_t v1 = seed + P32_1 + P32_2, v2 = seed + P32_2, v3 = seed, v4 = seed - P32_1;
        for (; end - p >= 16; p += 16) {
            v1 = round32(v1, read32(p));
            v2 = round32(v2, read32(p + 4));
            v3 = round32(v3, read32(p + 8));
            v4 = round32(v4, read32(p + 12));
        }
        h = rotl32(v1, 1) + rotl32(v2, 7) + rotl32(v3, 12) + rotl32(v4, 18);
    } else {
        h = seed + P32_5;
    }
    h += static_cast<std::uint32_t>(size);
    for (; end - p >= 4; p += 4) h = rotl32(h + read32(p) * P32_3, 17) * P32_4;
    for (; p < end; ++p) h = rotl32(h + *p * P32_5, 11) * P32_1;
    h ^= h >> 15;
    h *= P32_2;
    h ^= h >> 13;
    h *= P32_3;
    h ^= h >> 16;
    return h;
}

std::uint64_t xxh64(const void *data, std::size_t size, std::uint64_t seed) {
    const auto *p = static_cast<const std::uint8_t *>(data);
    const std::uint8_t *const end = p + size;
    std::uint64_t h;
    if (size >= 32) {
        std::uint64_t v1 = seed + P64_1 + P64_2, v2 = seed + P64_2, v3 = seed, v4 = seed - P64_1;
        for (; end - p >= 32; p += 32) {
            v1 = round64(v1, read64(p));
            v2 = round64(v2, read64(p + 8));
            v3 = round64(v3, read64(p + 16));
            v4 = round64(v4, read64(p + 24));
        }
        h = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);
        h = merge64(h, v1);
        h = merge64(h, v2);
        h = merge64(h, v3);
        h = merge64(h, v4);
    } else {
        h = seed + P64_5;
    }
    h += static_cast<std::uint64_t>(size);
    for (; end - p >= 8; p += 8) h = rotl64(h ^ round64(0, read64(p)), 27) * P64_1 + P64_4;
    if (end - p >= 4) {
        h = rotl64(h ^ static_cast<std::uint64_t>(read32(p)) * P64_1, 23) * P64_2 + P64_3;
        p += 4;
    }
    for (; p < end; ++p) h = rotl64(h ^ *p * P64_5, 11) * P64_1;
    h ^= h >> 33;
    h *= P64_2;
    h ^= h >> 29;
    h *= P64_3;
    h ^= h >> 32;
    return h;
}

} // namespace p3p3ds::ge
