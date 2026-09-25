/* KIRK command 1 — decrypt-and-verify.
 *
 * KIRK is the PSP's hardware crypto engine. Command 1 is the one that matters
 * for executable modules: it takes a 0x90-byte header followed by an encrypted
 * body, recovers the body's AES key from the header, verifies two CMACs, and
 * decrypts.
 *
 * Header layout (0x90 bytes):
 *
 *   0x00  16  body AES key      } AES-CBC encrypted with the KIRK1 key,
 *   0x10  16  CMAC key          } IV = 0, as one 0x20-byte run
 *   0x20  16  expected CMAC of the metadata header
 *   0x30  16  expected CMAC of the metadata header + body
 *   0x40  32  zero
 *   0x60   4  mode — 1 for KIRK1
 *   0x64   4  0 = AES CMAC, 1 = ECDSA
 *   0x68   4  zero
 *   0x6C   4  0 retail, 0xFFFFFFFF devkit
 *   0x70   4  decrypted data length
 *   0x74   4  padding length between the header and the data
 *   0x78  24  zero
 *   0x90  ..  padding, then the encrypted body
 *
 * Note that 0x60 onward is *plaintext* even though 0x00-0x3F is not, which is
 * what lets a caller read the data length before holding any key.
 *
 * The two CMACs are the reason this is worth implementing carefully rather
 * than treating decryption as a black box: they turn "did the key work?" from
 * a judgement call into a yes/no answer.
 */
#ifndef PSPRECOMP_KIRK_H
#define PSPRECOMP_KIRK_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KIRK1_HEADER_SIZE   0x90
#define KIRK1_META_OFFSET   0x60
#define KIRK1_META_SIZE     0x30
#define KIRK1_MODE_CMD1     1

typedef enum {
    KIRK_OK = 0,
    KIRK_ERR_TOO_SMALL,       /* buffer shorter than a header */
    KIRK_ERR_BAD_MODE,        /* metadata mode field is not 1 */
    KIRK_ERR_ECDSA,           /* ECDSA-signed; we verify CMAC only */
    KIRK_ERR_HEADER_CMAC,     /* header CMAC mismatch — wrong key, almost surely */
    KIRK_ERR_DATA_CMAC,       /* header verified but the body did not */
    KIRK_ERR_SIZE_OVERFLOW,   /* declared sizes do not fit in the buffer */
} kirk_result;

const char *kirk_strerror(kirk_result r);

typedef struct {
    uint32_t mode;
    uint32_t use_ecdsa;
    uint32_t is_devkit;
    uint32_t data_size;      /* decrypted length, from 0x70 */
    uint32_t data_offset;    /* padding before the body, from 0x74 */
    int      header_cmac_ok;
    int      data_cmac_ok;
} kirk1_info;

/* Decrypt a CMD1 blob in place.
 *
 * `buf` holds the 0x90-byte header followed by the body; `len` is the whole
 * thing. On success `*out` points into `buf` at the decrypted body and
 * `*out_len` is its length.
 *
 * The CMAC checks are performed and reported in `info` even when they fail;
 * a caller that wants to inspect a partially-wrong decryption can proceed,
 * but the return value is an error so nobody does it by accident.
 */
kirk_result kirk_cmd1_decrypt(const uint8_t kirk1_key[16],
                              uint8_t *buf, size_t len,
                              kirk1_info *info,
                              uint8_t **out, uint32_t *out_len);

/* Read the plaintext metadata without decrypting or holding a key. Used to
 * report what a module claims about itself before anything else happens. */
kirk_result kirk_cmd1_peek(const uint8_t *buf, size_t len, kirk1_info *info);

/* Build a CMD1 blob. This exists for the test suite: it lets the decrypt path
 * be verified end to end — including both CMACs — against a synthetic module
 * built with an arbitrary key, with no PSP key material involved.
 *
 * `pad` is the gap between the header and the body. Real modules use a nonzero
 * value and it participates in the body CMAC, so it is a parameter rather than
 * hardcoded to zero — a decryptor that ignores padding passes every test built
 * without it and then fails on the first real module. */
kirk_result kirk_cmd1_build(const uint8_t kirk1_key[16],
                            const uint8_t body_key[16],
                            const uint8_t cmac_key[16],
                            const uint8_t *data, uint32_t data_len, uint32_t pad,
                            uint8_t *out, size_t out_cap, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* PSPRECOMP_KIRK_H */
