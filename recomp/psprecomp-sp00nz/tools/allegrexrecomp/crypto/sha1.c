/* SHA-1 (FIPS 180-4). See sha1.h for why this exists. */

#include "sha1.h"

#include <string.h>

static uint32_t rotl(uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }

static void sha1_block(uint32_t h[5], const uint8_t block[64]) {
    uint32_t w[80];

    /* The message schedule is big-endian, unlike everything else on the PSP. */
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4] << 24) | ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) | (uint32_t)block[i * 4 + 3];
    }
    for (int i = 16; i < 80; i++)
        w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];

    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20)      { f = (b & c) | (~b & d);            k = 0x5A827999u; }
        else if (i < 40) { f = b ^ c ^ d;                     k = 0x6ED9EBA1u; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d);   k = 0x8F1BBCDCu; }
        else             { f = b ^ c ^ d;                     k = 0xCA62C1D6u; }

        uint32_t t = rotl(a, 5) + f + e + k + w[i];
        e = d; d = c; c = rotl(b, 30); b = a; a = t;
    }

    h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
}

void sha1(const void *data, size_t len, uint8_t out[SHA1_DIGEST_SIZE]) {
    uint32_t h[5] = { 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u };
    const uint8_t *p = (const uint8_t *)data;

    size_t full = len / 64;
    for (size_t i = 0; i < full; i++) sha1_block(h, p + i * 64);

    /* Final block(s): the remainder, a 0x80 terminator, zero padding, and the
     * bit length as a big-endian 64-bit value. If the remainder leaves no room
     * for the length, that takes a second block. */
    uint8_t tail[128];
    size_t rem = len - full * 64;
    memcpy(tail, p + full * 64, rem);
    tail[rem] = 0x80;

    size_t total = (rem + 1 <= 56) ? 64 : 128;
    memset(tail + rem + 1, 0, total - rem - 1);

    uint64_t bits = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++)
        tail[total - 1 - i] = (uint8_t)(bits >> (8 * i));

    sha1_block(h, tail);
    if (total == 128) sha1_block(h, tail + 64);

    for (int i = 0; i < 5; i++) {
        out[i * 4 + 0] = (uint8_t)(h[i] >> 24);
        out[i * 4 + 1] = (uint8_t)(h[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(h[i] >> 8);
        out[i * 4 + 3] = (uint8_t)(h[i]);
    }
}

uint32_t psp_nid(const char *name) {
    uint8_t d[SHA1_DIGEST_SIZE];
    sha1(name, strlen(name), d);
    return (uint32_t)d[0] | ((uint32_t)d[1] << 8) |
           ((uint32_t)d[2] << 16) | ((uint32_t)d[3] << 24);
}
