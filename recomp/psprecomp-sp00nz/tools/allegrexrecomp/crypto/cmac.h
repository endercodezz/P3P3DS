/* AES-CMAC (RFC 4493) — the integrity check on KIRK CMD1 headers and bodies.
 *
 * KIRK uses this to authenticate a module: the CMD1 header carries a CMAC of
 * its own metadata and a CMAC of the body. We compute both and compare.
 *
 * We only ever *verify*, never sign. That distinction is the whole reason this
 * is a decryption tool and not a signing tool: verifying tells you your key
 * material and your algorithm are right, which is exactly the feedback needed
 * while bringing decryption up.
 */
#ifndef PSPRECOMP_CMAC_H
#define PSPRECOMP_CMAC_H

#include "aes.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compute the 16-byte CMAC of `msg` under `key`. `len` may be zero and need
 * not be a multiple of the block size. */
void aes_cmac(const uint8_t *key, int key_bits,
              const uint8_t *msg, size_t len, uint8_t mac[16]);

/* Constant-time-ish comparison. Timing is not a threat here — nobody is
 * attacking a local decryption tool — but returning a bool from memcmp reads
 * badly at call sites, and this makes the intent explicit. */
int cmac_equal(const uint8_t a[16], const uint8_t b[16]);

#ifdef __cplusplus
}
#endif

#endif /* PSPRECOMP_CMAC_H */
