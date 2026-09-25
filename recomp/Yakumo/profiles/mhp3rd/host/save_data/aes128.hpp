#pragma once

// AES-128 on single blocks, the only form the save-data format needs: it
// builds its own modes on top. The cipher itself is tiny-AES-c.
#include "tiny_aes/aes.h"

#include <array>
#include <cstdint>

namespace mhp3rd::savedata {

using Block = std::array<std::uint8_t, 16>;

class Aes128 {
public:
    explicit Aes128(const Block &key);

    [[nodiscard]] Block encrypt(const Block &input) const;
    [[nodiscard]] Block decrypt(const Block &input) const;

private:
    AES_ctx context_{};
};

[[nodiscard]] inline Block xor_blocks(const Block &a, const Block &b) {
    Block out{};
    for (std::size_t i = 0; i < out.size(); ++i) out[i] = static_cast<std::uint8_t>(a[i] ^ b[i]);
    return out;
}

} // namespace mhp3rd::savedata
