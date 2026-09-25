/* AES-128/192/256. See aes.h for why the S-box is derived rather than pasted. */

#include "aes.h"

#include <string.h>

/* ---- GF(2^8) arithmetic, modulus 0x11B ----------------------------------- */

static uint8_t gmul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1) p ^= a;
        uint8_t hi = a & 0x80;
        a <<= 1;
        if (hi) a ^= 0x1B;      /* reduce by the AES polynomial */
        b >>= 1;
    }
    return p;
}

/* ---- S-box, derived at first use ----------------------------------------- */

static uint8_t SBOX[256];
static uint8_t INV_SBOX[256];
static int tables_ready = 0;

static uint8_t rotl8(uint8_t x, int n) {
    return (uint8_t)((x << n) | (x >> (8 - n)));
}

static void build_tables(void) {
    if (tables_ready) return;

    /* Multiplicative inverse by exhaustive search. 64K operations, once, at
     * startup — and unambiguously correct, which a transcribed table is not. */
    uint8_t inv[256];
    inv[0] = 0;                     /* 0 has no inverse; AES defines it as 0 */
    for (int a = 1; a < 256; a++) {
        for (int b = 1; b < 256; b++) {
            if (gmul((uint8_t)a, (uint8_t)b) == 1) { inv[a] = (uint8_t)b; break; }
        }
    }

    for (int i = 0; i < 256; i++) {
        uint8_t b = inv[i];
        uint8_t s = (uint8_t)(b ^ rotl8(b, 1) ^ rotl8(b, 2) ^ rotl8(b, 3) ^ rotl8(b, 4) ^ 0x63);
        SBOX[i] = s;
        INV_SBOX[s] = (uint8_t)i;
    }
    tables_ready = 1;
}

/* ---- key expansion (FIPS-197 section 5.2) -------------------------------- */

static uint32_t sub_word(uint32_t w) {
    return ((uint32_t)SBOX[(w >> 24) & 0xFF] << 24) |
           ((uint32_t)SBOX[(w >> 16) & 0xFF] << 16) |
           ((uint32_t)SBOX[(w >>  8) & 0xFF] <<  8) |
           ((uint32_t)SBOX[ w        & 0xFF]);
}

static uint32_t rot_word(uint32_t w) {
    return (w << 8) | (w >> 24);
}

int aes_init(aes_ctx *ctx, const uint8_t *key, int key_bits) {
    build_tables();

    int nk;
    switch (key_bits) {
    case 128: nk = 4; break;
    case 192: nk = 6; break;
    case 256: nk = 8; break;
    default:  return -1;
    }
    ctx->rounds = nk + 6;

    const int total = 4 * (ctx->rounds + 1);
    uint32_t *w = ctx->rk;

    for (int i = 0; i < nk; i++) {
        w[i] = ((uint32_t)key[4 * i]     << 24) |
               ((uint32_t)key[4 * i + 1] << 16) |
               ((uint32_t)key[4 * i + 2] <<  8) |
               ((uint32_t)key[4 * i + 3]);
    }

    uint8_t rcon = 1;
    for (int i = nk; i < total; i++) {
        uint32_t temp = w[i - 1];
        if (i % nk == 0) {
            temp = sub_word(rot_word(temp)) ^ ((uint32_t)rcon << 24);
            /* rcon advances in GF(2^8): 01 02 04 08 10 20 40 80 1B 36 ... */
            rcon = gmul(rcon, 2);
        } else if (nk > 6 && i % nk == 4) {
            temp = sub_word(temp);
        }
        w[i] = w[i - nk] ^ temp;
    }
    return 0;
}

/* ---- the state -----------------------------------------------------------
 * State bytes are held in the same order as the input block: byte i is row
 * i%4, column i/4. That keeps load/store trivial and confines the row/column
 * distinction to ShiftRows and MixColumns, which are the only two steps that
 * care. */

static void add_round_key(uint8_t s[16], const uint32_t *rk) {
    for (int c = 0; c < 4; c++) {
        uint32_t k = rk[c];
        s[4 * c + 0] ^= (uint8_t)(k >> 24);
        s[4 * c + 1] ^= (uint8_t)(k >> 16);
        s[4 * c + 2] ^= (uint8_t)(k >>  8);
        s[4 * c + 3] ^= (uint8_t)(k);
    }
}

static void sub_bytes(uint8_t s[16])     { for (int i = 0; i < 16; i++) s[i] = SBOX[s[i]]; }
static void inv_sub_bytes(uint8_t s[16]) { for (int i = 0; i < 16; i++) s[i] = INV_SBOX[s[i]]; }

/* Row r rotates left by r. */
static void shift_rows(uint8_t s[16]) {
    uint8_t t[16];
    memcpy(t, s, 16);
    for (int r = 1; r < 4; r++)
        for (int c = 0; c < 4; c++)
            s[4 * c + r] = t[4 * ((c + r) & 3) + r];
}

static void inv_shift_rows(uint8_t s[16]) {
    uint8_t t[16];
    memcpy(t, s, 16);
    for (int r = 1; r < 4; r++)
        for (int c = 0; c < 4; c++)
            s[4 * ((c + r) & 3) + r] = t[4 * c + r];
}

static void mix_columns(uint8_t s[16]) {
    for (int c = 0; c < 4; c++) {
        uint8_t *p = s + 4 * c;
        uint8_t a0 = p[0], a1 = p[1], a2 = p[2], a3 = p[3];
        p[0] = (uint8_t)(gmul(a0, 2) ^ gmul(a1, 3) ^ a2 ^ a3);
        p[1] = (uint8_t)(a0 ^ gmul(a1, 2) ^ gmul(a2, 3) ^ a3);
        p[2] = (uint8_t)(a0 ^ a1 ^ gmul(a2, 2) ^ gmul(a3, 3));
        p[3] = (uint8_t)(gmul(a0, 3) ^ a1 ^ a2 ^ gmul(a3, 2));
    }
}

static void inv_mix_columns(uint8_t s[16]) {
    for (int c = 0; c < 4; c++) {
        uint8_t *p = s + 4 * c;
        uint8_t a0 = p[0], a1 = p[1], a2 = p[2], a3 = p[3];
        p[0] = (uint8_t)(gmul(a0, 0x0E) ^ gmul(a1, 0x0B) ^ gmul(a2, 0x0D) ^ gmul(a3, 0x09));
        p[1] = (uint8_t)(gmul(a0, 0x09) ^ gmul(a1, 0x0E) ^ gmul(a2, 0x0B) ^ gmul(a3, 0x0D));
        p[2] = (uint8_t)(gmul(a0, 0x0D) ^ gmul(a1, 0x09) ^ gmul(a2, 0x0E) ^ gmul(a3, 0x0B));
        p[3] = (uint8_t)(gmul(a0, 0x0B) ^ gmul(a1, 0x0D) ^ gmul(a2, 0x09) ^ gmul(a3, 0x0E));
    }
}

/* ---- block cipher -------------------------------------------------------- */

void aes_encrypt_block(const aes_ctx *ctx, const uint8_t in[16], uint8_t out[16]) {
    uint8_t s[16];
    memcpy(s, in, 16);

    add_round_key(s, ctx->rk);
    for (int r = 1; r < ctx->rounds; r++) {
        sub_bytes(s);
        shift_rows(s);
        mix_columns(s);
        add_round_key(s, ctx->rk + 4 * r);
    }
    /* Final round omits MixColumns. */
    sub_bytes(s);
    shift_rows(s);
    add_round_key(s, ctx->rk + 4 * ctx->rounds);

    memcpy(out, s, 16);
}

void aes_decrypt_block(const aes_ctx *ctx, const uint8_t in[16], uint8_t out[16]) {
    uint8_t s[16];
    memcpy(s, in, 16);

    add_round_key(s, ctx->rk + 4 * ctx->rounds);
    for (int r = ctx->rounds - 1; r > 0; r--) {
        inv_shift_rows(s);
        inv_sub_bytes(s);
        add_round_key(s, ctx->rk + 4 * r);
        inv_mix_columns(s);
    }
    inv_shift_rows(s);
    inv_sub_bytes(s);
    add_round_key(s, ctx->rk);

    memcpy(out, s, 16);
}

/* ---- CBC ----------------------------------------------------------------- */

void aes_cbc_encrypt(const aes_ctx *ctx, const uint8_t iv[16],
                     const uint8_t *in, uint8_t *out, size_t len) {
    uint8_t chain[16];
    memcpy(chain, iv, 16);

    for (size_t off = 0; off + 16 <= len; off += 16) {
        for (int i = 0; i < 16; i++) chain[i] ^= in[off + i];
        aes_encrypt_block(ctx, chain, chain);
        memcpy(out + off, chain, 16);
    }
}

void aes_cbc_decrypt(const aes_ctx *ctx, const uint8_t iv[16],
                     const uint8_t *in, uint8_t *out, size_t len) {
    uint8_t chain[16], next[16], plain[16];
    memcpy(chain, iv, 16);

    for (size_t off = 0; off + 16 <= len; off += 16) {
        /* Save the ciphertext before writing, so in==out works in place. */
        memcpy(next, in + off, 16);
        aes_decrypt_block(ctx, next, plain);
        for (int i = 0; i < 16; i++) plain[i] ^= chain[i];
        memcpy(out + off, plain, 16);
        memcpy(chain, next, 16);
    }
}
