/* Crypto tests — published standard vectors only.
 *
 * These are the foundation the whole decryption path stands on, so they are
 * checked against the primary sources rather than against another
 * implementation: FIPS-197 for the AES block cipher, NIST SP 800-38A for CBC
 * mode, and RFC 4493 for CMAC. No PSP key material and no game data is
 * involved — this suite proves the primitives are right *before* they are ever
 * pointed at a module, so that when decryption later fails, the primitives are
 * already ruled out.
 */

#include "crypto/aes.h"
#include "crypto/cmac.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond, ...)                                       \
    do {                                                       \
        if (!(cond)) {                                         \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);        \
            printf(__VA_ARGS__);                               \
            printf("\n");                                      \
            failures++;                                        \
        }                                                      \
    } while (0)

/* Parse a hex string into bytes. Returns the byte count. */
static size_t unhex(const char *hex, uint8_t *out, size_t max) {
    size_t n = 0;
    for (const char *p = hex; p[0] && p[1] && n < max; p += 2) {
        unsigned v;
        if (sscanf(p, "%2x", &v) != 1) break;
        out[n++] = (uint8_t)v;
    }
    return n;
}

static void hexdump(const uint8_t *b, size_t n, char *out) {
    for (size_t i = 0; i < n; i++) sprintf(out + i * 2, "%02x", b[i]);
    out[n * 2] = '\0';
}

/* Compare `got` against an expected hex string, reporting both on mismatch. */
static void expect(const uint8_t *got, size_t n, const char *want_hex, const char *label) {
    uint8_t want[64];
    size_t wn = unhex(want_hex, want, sizeof want);
    if (wn != n || memcmp(got, want, n) != 0) {
        char g[130];
        hexdump(got, n, g);
        printf("FAIL %s:\n  got  %s\n  want %s\n", label, g, want_hex);
        failures++;
    }
}

/* ---- FIPS-197 Appendix C: the block cipher ------------------------------- */

static void test_fips197(void) {
    /* All three key sizes share the same plaintext in Appendix C. */
    const char *PT = "00112233445566778899aabbccddeeff";
    uint8_t pt[16], key[32], out[16];
    unhex(PT, pt, sizeof pt);

    struct { const char *key; int bits; const char *ct; const char *label; } V[] = {
        { "000102030405060708090a0b0c0d0e0f", 128,
          "69c4e0d86a7b0430d8cdb78070b4c55a", "FIPS-197 C.1 AES-128" },
        { "000102030405060708090a0b0c0d0e0f1011121314151617", 192,
          "dda97ca4864cdfe06eaf70a0ec0d7191", "FIPS-197 C.2 AES-192" },
        { "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", 256,
          "8ea2b7ca516745bfeafc49904b496089", "FIPS-197 C.3 AES-256" },
    };

    for (int i = 0; i < 3; i++) {
        aes_ctx ctx;
        unhex(V[i].key, key, sizeof key);
        CHECK(aes_init(&ctx, key, V[i].bits) == 0, "%s: init", V[i].label);

        aes_encrypt_block(&ctx, pt, out);
        expect(out, 16, V[i].ct, V[i].label);

        /* Decryption must invert it exactly — a broken InvMixColumns still
         * round-trips if encryption is equally broken, so the ciphertext
         * check above is what actually pins correctness. This pins the
         * inverse path independently. */
        uint8_t back[16];
        aes_decrypt_block(&ctx, out, back);
        CHECK(memcmp(back, pt, 16) == 0, "%s: decrypt inverts encrypt", V[i].label);
    }

    /* Reject a nonsense key size rather than silently doing something. */
    aes_ctx ctx;
    CHECK(aes_init(&ctx, key, 64) != 0, "aes_init rejects a 64-bit key");
}

/* ---- NIST SP 800-38A section F.2: CBC mode ------------------------------- */

static void test_cbc(void) {
    uint8_t key[16], iv[16], pt[64], ct[64], out[64];

    unhex("2b7e151628aed2a6abf7158809cf4f3c", key, sizeof key);
    unhex("000102030405060708090a0b0c0d0e0f", iv, sizeof iv);
    unhex("6bc1bee22e409f96e93d7e117393172a"
          "ae2d8a571e03ac9c9eb76fac45af8e51"
          "30c81c46a35ce411e5fbc1191a0a52ef"
          "f69f2445df4f9b17ad2b417be66c3710", pt, sizeof pt);
    unhex("7649abac8119b246cee98e9b12e9197d"
          "5086cb9b507219ee95db113a917678b2"
          "73bed6b8e3c1743b7116e69e22229516"
          "3ff1caa1681fac09120eca307586e1a7", ct, sizeof ct);

    aes_ctx ctx;
    aes_init(&ctx, key, 128);

    aes_cbc_encrypt(&ctx, iv, pt, out, 64);
    expect(out, 64, "7649abac8119b246cee98e9b12e9197d"
                    "5086cb9b507219ee95db113a917678b2"
                    "73bed6b8e3c1743b7116e69e22229516"
                    "3ff1caa1681fac09120eca307586e1a7",
           "SP 800-38A F.2.1 CBC-AES128 encrypt");

    aes_cbc_decrypt(&ctx, iv, ct, out, 64);
    expect(out, 64, "6bc1bee22e409f96e93d7e117393172a"
                    "ae2d8a571e03ac9c9eb76fac45af8e51"
                    "30c81c46a35ce411e5fbc1191a0a52ef"
                    "f69f2445df4f9b17ad2b417be66c3710",
           "SP 800-38A F.2.2 CBC-AES128 decrypt");

    /* In-place operation: the KIRK path decrypts buffers in place, and a naive
     * CBC implementation corrupts the chain when in == out. */
    memcpy(out, ct, 64);
    aes_cbc_decrypt(&ctx, iv, out, out, 64);
    expect(out, 64, "6bc1bee22e409f96e93d7e117393172a"
                    "ae2d8a571e03ac9c9eb76fac45af8e51"
                    "30c81c46a35ce411e5fbc1191a0a52ef"
                    "f69f2445df4f9b17ad2b417be66c3710",
           "CBC decrypt in place");

    /* A zero IV is what KIRK uses; make sure the degenerate case works. */
    uint8_t zero_iv[16] = { 0 };
    aes_cbc_encrypt(&ctx, zero_iv, pt, out, 16);
    aes_cbc_decrypt(&ctx, zero_iv, out, out, 16);
    CHECK(memcmp(out, pt, 16) == 0, "CBC round-trips with a zero IV");
}

/* ---- RFC 4493: AES-CMAC -------------------------------------------------- */

static void test_cmac(void) {
    uint8_t key[16], msg[64], mac[16];
    unhex("2b7e151628aed2a6abf7158809cf4f3c", key, sizeof key);
    unhex("6bc1bee22e409f96e93d7e117393172a"
          "ae2d8a571e03ac9c9eb76fac45af8e51"
          "30c81c46a35ce411e5fbc1191a0a52ef"
          "f69f2445df4f9b17ad2b417be66c3710", msg, sizeof msg);

    /* The four RFC 4493 examples cover both padding paths: lengths 0 and 40
     * take the 0x80-pad / K2 branch, lengths 16 and 64 take the K1 branch.
     * Getting the branch condition backwards passes one pair and fails the
     * other, which is why both are here. */
    aes_cmac(key, 128, msg, 0, mac);
    expect(mac, 16, "bb1d6929e95937287fa37d129b756746", "RFC 4493 example 1 (len 0)");

    aes_cmac(key, 128, msg, 16, mac);
    expect(mac, 16, "070a16b46b4d4144f79bdd9dd04a287c", "RFC 4493 example 2 (len 16)");

    aes_cmac(key, 128, msg, 40, mac);
    expect(mac, 16, "dfa66747de9ae63030ca32611497c827", "RFC 4493 example 3 (len 40)");

    aes_cmac(key, 128, msg, 64, mac);
    expect(mac, 16, "51f0bebf7e3b9d92fc49741779363cfe", "RFC 4493 example 4 (len 64)");

    /* Flipping one message bit must change the MAC — the check that the CMAC
     * is actually reading the whole message. */
    uint8_t mac2[16];
    msg[0] ^= 0x01;
    aes_cmac(key, 128, msg, 64, mac2);
    CHECK(!cmac_equal(mac, mac2), "CMAC changes when the message changes");
    msg[0] ^= 0x01;

    CHECK(cmac_equal(mac, mac), "cmac_equal accepts identical MACs");
}

int main(void) {
    test_fips197();
    test_cbc();
    test_cmac();

    if (failures) {
        printf("\n%d check(s) failed\n", failures);
        return 1;
    }
    printf("all crypto checks passed (FIPS-197, SP 800-38A, RFC 4493)\n");
    return 0;
}
