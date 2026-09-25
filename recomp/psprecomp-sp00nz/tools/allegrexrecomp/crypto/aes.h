/* AES-128/192/256 — the primitive underneath everything the PSP encrypts.
 *
 * Self-contained on purpose: the toolkit's core has no external dependencies,
 * and dragging in OpenSSL to decrypt a game module would be absurd. This is
 * ~250 lines and it is validated against the FIPS-197 and NIST SP 800-38A
 * vectors before it is ever pointed at a game.
 *
 * One deliberate choice: the S-box is *computed* at init from its algebraic
 * definition (multiplicative inverse in GF(2^8), then the affine transform)
 * rather than pasted in as a 256-byte table. A mistyped constant in a pasted
 * table produces an implementation that fails only for certain inputs, which
 * is exactly the kind of bug that survives casual testing. Deriving it means
 * the table is either completely right or obviously wrong.
 */
#ifndef PSPRECOMP_AES_H
#define PSPRECOMP_AES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AES_BLOCK_SIZE 16
#define AES_MAX_ROUNDS 14

typedef struct {
    uint32_t rk[4 * (AES_MAX_ROUNDS + 1)];  /* expanded round keys */
    int      rounds;                        /* 10, 12 or 14 */
} aes_ctx;

/* key_bits must be 128, 192 or 256. Returns 0 on success. */
int aes_init(aes_ctx *ctx, const uint8_t *key, int key_bits);

/* Single-block ECB. `in` and `out` may alias. */
void aes_encrypt_block(const aes_ctx *ctx, const uint8_t in[16], uint8_t out[16]);
void aes_decrypt_block(const aes_ctx *ctx, const uint8_t in[16], uint8_t out[16]);

/* CBC over a whole buffer. `len` must be a multiple of 16 — no padding is
 * applied or expected, because none of the PSP formats use any. `iv` is
 * consumed but not updated; pass a copy if you need it preserved.
 * `in` and `out` may alias for in-place operation. */
void aes_cbc_encrypt(const aes_ctx *ctx, const uint8_t iv[16],
                     const uint8_t *in, uint8_t *out, size_t len);
void aes_cbc_decrypt(const aes_ctx *ctx, const uint8_t iv[16],
                     const uint8_t *in, uint8_t *out, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* PSPRECOMP_AES_H */
