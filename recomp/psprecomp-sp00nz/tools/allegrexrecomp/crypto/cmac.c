/* AES-CMAC, RFC 4493. See cmac.h. */

#include "cmac.h"

#include <string.h>

/* Left-shift a 128-bit big-endian value by one bit. Returns the bit shifted
 * out of the top, which selects whether the Rb constant gets folded back in. */
static uint8_t shift_left_1(const uint8_t in[16], uint8_t out[16]) {
    uint8_t carry = (uint8_t)(in[0] >> 7);
    for (int i = 0; i < 15; i++)
        out[i] = (uint8_t)((in[i] << 1) | (in[i + 1] >> 7));
    out[15] = (uint8_t)(in[15] << 1);
    return carry;
}

/* Subkey generation (RFC 4493 section 2.3).
 *   L  = AES-Encrypt(K, 0^128)
 *   K1 = L  << 1, xor Rb if the shifted-out bit was set
 *   K2 = K1 << 1, xor Rb if the shifted-out bit was set
 * Rb is 0x87 for the 128-bit block size. */
static void generate_subkeys(const aes_ctx *ctx, uint8_t k1[16], uint8_t k2[16]) {
    static const uint8_t RB = 0x87;
    uint8_t zero[16] = { 0 };
    uint8_t l[16];

    aes_encrypt_block(ctx, zero, l);

    if (shift_left_1(l, k1)) k1[15] ^= RB;
    if (shift_left_1(k1, k2)) k2[15] ^= RB;
}

void aes_cmac(const uint8_t *key, int key_bits,
              const uint8_t *msg, size_t len, uint8_t mac[16]) {
    aes_ctx ctx;
    if (aes_init(&ctx, key, key_bits) != 0) {
        memset(mac, 0, 16);
        return;
    }

    uint8_t k1[16], k2[16];
    generate_subkeys(&ctx, k1, k2);

    /* A message whose length is a nonzero multiple of the block size uses K1
     * on its final block as-is; anything else is 0x80-padded and uses K2.
     * The empty message takes the padded path. */
    const int complete = (len != 0) && (len % 16 == 0);
    const size_t nblocks = complete ? len / 16 : len / 16 + 1;

    uint8_t last[16];
    const size_t last_off = (nblocks - 1) * 16;

    if (complete) {
        memcpy(last, msg + last_off, 16);
        for (int i = 0; i < 16; i++) last[i] ^= k1[i];
    } else {
        const size_t rem = len - last_off;
        memset(last, 0, 16);
        if (rem) memcpy(last, msg + last_off, rem);
        last[rem] = 0x80;
        for (int i = 0; i < 16; i++) last[i] ^= k2[i];
    }

    /* CBC-MAC: chain through every block but the last, then the tweaked last
     * block. The final ciphertext block is the MAC. */
    uint8_t x[16] = { 0 };
    for (size_t b = 0; b + 1 < nblocks; b++) {
        for (int i = 0; i < 16; i++) x[i] ^= msg[b * 16 + i];
        aes_encrypt_block(&ctx, x, x);
    }
    for (int i = 0; i < 16; i++) x[i] ^= last[i];
    aes_encrypt_block(&ctx, x, mac);
}

int cmac_equal(const uint8_t a[16], const uint8_t b[16]) {
    uint8_t diff = 0;
    for (int i = 0; i < 16; i++) diff |= (uint8_t)(a[i] ^ b[i]);
    return diff == 0;
}
