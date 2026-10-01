// See psp_eboot.hpp. Container layout and key unwrapping follow
// tools/prepare_game.py line by line (header offsets 0x80/0xB0/0xC0/0xD0/
// 0x12C/0x140, payload at 0x150).
#include "psp_eboot.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace p3p3ds::builder {
namespace {

// Sony PSP PRX / KIRK keys for tag 0xD91613F0 (same values as tools/prepare_game.py).
constexpr std::uint32_t kTag = 0xD91613F0u;
constexpr std::uint8_t kTagKey[16] = {0xEB, 0xFF, 0x40, 0xD8, 0xB4, 0x1A, 0xE1, 0x66, 0x91, 0x3B, 0x8F, 0x64, 0xB6, 0xFC, 0xB7, 0x12};
constexpr std::uint8_t kHeaderKey[16] = {0x11, 0x5A, 0x5D, 0x20, 0xD5, 0x3A, 0x8D, 0xD3, 0x9C, 0xC5, 0xAF, 0x41, 0x0F, 0x0F, 0x18, 0x6F};
constexpr std::uint8_t kPayloadWrappingKey[16] = {0x98, 0xC9, 0x40, 0x97, 0x5C, 0x1D, 0x10, 0xE8, 0x7F, 0xE6, 0x0E, 0xA3, 0xFD, 0x03, 0xA8, 0xBA};

// ---- AES-128 (FIPS-197), decryption only -----------------------------------------
constexpr std::uint8_t kSbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};

std::uint8_t xtime(std::uint8_t a) { return static_cast<std::uint8_t>((a << 1) ^ ((a & 0x80) ? 0x1B : 0)); }
std::uint8_t mul(std::uint8_t a, std::uint8_t b) {
    std::uint8_t r = 0;
    while (b) { if (b & 1) r ^= a; a = xtime(a); b >>= 1; }
    return r;
}

struct Aes128 {
    std::array<std::uint8_t, 176> rk{};
    std::array<std::uint8_t, 256> inv{};
    explicit Aes128(const std::uint8_t key[16]) {
        for (int i = 0; i < 256; ++i) inv[kSbox[i]] = static_cast<std::uint8_t>(i);
        static constexpr std::uint8_t rcon[11] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36};
        std::memcpy(rk.data(), key, 16);
        for (int i = 16; i < 176; i += 4) {
            std::uint8_t t[4] = {rk[i - 4], rk[i - 3], rk[i - 2], rk[i - 1]};
            if (i % 16 == 0) {
                const std::uint8_t t0 = t[0];
                t[0] = static_cast<std::uint8_t>(kSbox[t[1]] ^ rcon[i / 16]);
                t[1] = kSbox[t[2]]; t[2] = kSbox[t[3]]; t[3] = kSbox[t0];
            }
            for (int j = 0; j < 4; ++j) rk[i + j] = static_cast<std::uint8_t>(rk[i - 16 + j] ^ t[j]);
        }
    }
    void decrypt_block(const std::uint8_t in[16], std::uint8_t out[16]) const {
        std::uint8_t s[16];
        for (int j = 0; j < 16; ++j) s[j] = static_cast<std::uint8_t>(in[j] ^ rk[160 + j]);
        auto inv_shift_sub = [&] {
            const std::uint8_t t[16] = {s[0], s[13], s[10], s[7], s[4], s[1], s[14], s[11], s[8], s[5], s[2], s[15], s[12], s[9], s[6], s[3]};
            for (int j = 0; j < 16; ++j) s[j] = inv[t[j]];
        };
        for (int round = 9; round > 0; --round) {
            inv_shift_sub();
            for (int j = 0; j < 16; ++j) s[j] ^= rk[round * 16 + j];
            for (int c = 0; c < 4; ++c) {
                const std::uint8_t a0 = s[c * 4], a1 = s[c * 4 + 1], a2 = s[c * 4 + 2], a3 = s[c * 4 + 3];
                s[c * 4 + 0] = mul(a0, 14) ^ mul(a1, 11) ^ mul(a2, 13) ^ mul(a3, 9);
                s[c * 4 + 1] = mul(a0, 9) ^ mul(a1, 14) ^ mul(a2, 11) ^ mul(a3, 13);
                s[c * 4 + 2] = mul(a0, 13) ^ mul(a1, 9) ^ mul(a2, 14) ^ mul(a3, 11);
                s[c * 4 + 3] = mul(a0, 11) ^ mul(a1, 13) ^ mul(a2, 9) ^ mul(a3, 14);
            }
        }
        inv_shift_sub();
        for (int j = 0; j < 16; ++j) out[j] = static_cast<std::uint8_t>(s[j] ^ rk[j]);
    }
};

std::uint32_t le32(const std::uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<std::uint32_t>(p[3]) << 24); }

std::vector<std::uint8_t> unwrap_payload_key(const std::uint8_t *header) {
    std::vector<std::uint8_t> record;
    record.insert(record.end(), header + 0x140, header + 0x150);
    record.insert(record.end(), header + 0x12C, header + 0x140);
    record.insert(record.end(), header + 0x80, header + 0xB0);
    record.insert(record.end(), header + 0xC0, header + 0xCC);
    const auto dec = aes128_cbc_decrypt(kHeaderKey, record.data(), record.size());
    std::vector<std::uint8_t> raw;
    for (int index = 0; index < 9; ++index) {
        raw.insert(raw.end(), kTagKey, kTagKey + 16);
        raw[raw.size() - 16] = static_cast<std::uint8_t>(index);
    }
    const auto mask = aes128_cbc_decrypt(kHeaderKey, raw.data(), raw.size());
    std::uint8_t key[16];
    for (int i = 0; i < 16; ++i) key[i] = static_cast<std::uint8_t>(dec[0x24 + i] ^ mask[0x10 + i]);
    auto k2 = aes128_cbc_decrypt(kHeaderKey, key, 16);
    for (int i = 0; i < 16; ++i) k2[i] ^= mask[0x50 + i];
    return aes128_cbc_decrypt(kPayloadWrappingKey, k2.data(), 16);
}

// ---- SHA-256 (FIPS 180-4) -------------------------------------------------------
constexpr std::uint32_t kK[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
    0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
    0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
std::uint32_t rotr(std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

} // namespace

std::vector<std::uint8_t> aes128_cbc_decrypt(const std::uint8_t key[16], const std::uint8_t *data, std::size_t size) {
    const Aes128 aes(key);
    std::vector<std::uint8_t> out(size);
    std::uint8_t prev[16] = {};
    for (std::size_t i = 0; i + 16 <= size; i += 16) {
        std::uint8_t block[16];
        aes.decrypt_block(data + i, block);
        for (int j = 0; j < 16; ++j) out[i + j] = static_cast<std::uint8_t>(block[j] ^ prev[j]);
        std::memcpy(prev, data + i, 16);
    }
    return out;
}

std::vector<std::uint8_t> decrypt_eboot(const std::vector<std::uint8_t> &eboot) {
    if (eboot.size() < 0x150) throw std::runtime_error("EBOOT.BIN is truncated.");
    if (std::memcmp(eboot.data(), "~PSP", 4) != 0) throw std::runtime_error("EBOOT.BIN is not an encrypted PSP executable (~PSP header missing).");
    if (le32(&eboot[0xD0]) != kTag) throw std::runtime_error("EBOOT.BIN uses an unsupported encryption tag: is this ULUS-10512?");
    if ((eboot[0x06] | (eboot[0x07] << 8)) & 1u) throw std::runtime_error("Compressed EBOOT.BIN is not supported.");
    if (eboot[0x7C] != 9) throw std::runtime_error("EBOOT.BIN is not a UMD game executable.");
    const std::uint32_t payload_size = le32(&eboot[0xB0]);
    const std::size_t padded = (static_cast<std::size_t>(payload_size) + 15u) & ~static_cast<std::size_t>(15u);
    if (0x150 + padded > eboot.size()) throw std::runtime_error("EBOOT.BIN is truncated.");
    const auto key = unwrap_payload_key(eboot.data());
    auto elf = aes128_cbc_decrypt(key.data(), eboot.data() + 0x150, padded);
    elf.resize(payload_size);
    if (elf.size() < 4 || std::memcmp(elf.data(), "\x7f" "ELF", 4) != 0) throw std::runtime_error("Decryption failed (no ELF header).");
    return elf;
}

std::string sha256_hex(const std::vector<std::uint8_t> &data) {
    std::uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::vector<std::uint8_t> m(data);
    const std::uint64_t bits = static_cast<std::uint64_t>(data.size()) * 8u;
    m.push_back(0x80);
    while (m.size() % 64 != 56) m.push_back(0);
    for (int i = 7; i >= 0; --i) m.push_back(static_cast<std::uint8_t>(bits >> (8 * i)));
    for (std::size_t off = 0; off < m.size(); off += 64) {
        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (static_cast<std::uint32_t>(m[off + 4 * i]) << 24) | (m[off + 4 * i + 1] << 16) | (m[off + 4 * i + 2] << 8) | m[off + 4 * i + 3];
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            const std::uint32_t t1 = hh + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + kK[i] + w[i];
            const std::uint32_t t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    std::string hex;
    char buf[9];
    for (auto v : h) { std::snprintf(buf, sizeof buf, "%08x", v); hex += buf; }
    return hex;
}

} // namespace p3p3ds::builder
