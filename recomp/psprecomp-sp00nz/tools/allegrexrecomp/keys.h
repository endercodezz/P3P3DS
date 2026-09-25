/* External key material.
 *
 * PSP decryption constants are published facts in the same sense the Lynx boot
 * ROM's RSA modulus is, but they are **not distributed with this toolkit**.
 * They live in a file the user supplies, `keys/` is gitignored, and nothing in
 * this repository contains one.
 *
 * That is not only a licensing posture — it is also better engineering. Key
 * material as data rather than baked-in constants means a key can be corrected
 * without a rebuild, a wrong key produces a clear diagnostic instead of a
 * mysterious failure, and the tool can say precisely *which* named key it
 * wanted and could not find.
 *
 * Format — one `name = hex` per line, `#` starts a comment:
 *
 *     # keys/psp_keys.txt
 *     kirk1      = 00112233445566778899aabbccddeeff
 *     prx.F8710C50 = ...
 *
 * Lookup order: an explicit --keys path, then $PSPRECOMP_KEYS, then
 * ./keys/psp_keys.txt.
 */
#ifndef PSPRECOMP_KEYS_H
#define PSPRECOMP_KEYS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEYS_MAX_ENTRIES 512
#define KEYS_MAX_NAME    64
#define KEYS_MAX_VALUE   256

typedef struct {
    char    name[KEYS_MAX_NAME];
    uint8_t value[KEYS_MAX_VALUE];
    size_t  len;
} key_entry;

typedef struct {
    key_entry entry[KEYS_MAX_ENTRIES];
    int       count;
    char      path[512];       /* where they were loaded from, for diagnostics */
    int       loaded;
} key_store;

/* Load a key file. `path` may be NULL to use the search order above.
 * Returns 0 on success, -1 if no file was found or it could not be parsed.
 * A missing file is not an error condition worth aborting on — the caller
 * decides, since most subcommands need no keys at all. */
int keys_load(key_store *ks, const char *path);

/* Look up a named key. Returns NULL if absent. `len` receives its length. */
const uint8_t *keys_get(const key_store *ks, const char *name, size_t *len);

/* Look up a key that must be exactly `want_len` bytes. Returns NULL and leaves
 * a message in `err` (size `errlen`) explaining precisely what was missing or
 * wrong — "kirk1 not found in keys/psp_keys.txt" beats a null pointer. */
const uint8_t *keys_require(const key_store *ks, const char *name, size_t want_len,
                            char *err, size_t errlen);

/* Where keys_load looked, for an error message when nothing was found. */
void keys_describe_search(char *out, size_t outlen);

#ifdef __cplusplus
}
#endif

#endif /* PSPRECOMP_KEYS_H */
