/* SHA-1.
 *
 * Present for one specific reason: a PSP **NID** — the identifier every
 * firmware call is made by — is the first four bytes of SHA-1(function name),
 * little-endian. That makes the NID table self-verifying. Rather than trusting
 * a transcribed list of "NID = name" pairs, the test suite hashes each declared
 * name and checks it produces the NID we registered.
 *
 * A wrong name or a mistyped NID therefore cannot survive the build, which
 * matters more than it might seem: an HLE table with one entry pointing at the
 * wrong function produces a game that runs and misbehaves in a way that looks
 * like a codegen bug.
 */
#ifndef PSPRECOMP_SHA1_H
#define PSPRECOMP_SHA1_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHA1_DIGEST_SIZE 20

void sha1(const void *data, size_t len, uint8_t out[SHA1_DIGEST_SIZE]);

/* The PSP NID for a function name: SHA-1(name), first 4 bytes, little-endian. */
uint32_t psp_nid(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* PSPRECOMP_SHA1_H */
