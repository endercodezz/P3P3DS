/* KIRK CMD1 tests.
 *
 * These verify the decrypt path end to end — key unwrapping, both CMACs, and
 * body decryption — using synthetic blobs built with arbitrary keys chosen
 * here in the test. **No PSP key material is involved**, and none is needed:
 * CMD1's structure is independent of which key you feed it, so a round trip
 * under a test key proves the algorithm exactly as well as a round trip under
 * Sony's would.
 *
 * That property is the whole reason CMD1 was worth implementing before the
 * key question was settled.
 */

#include "crypto/kirk.h"
#include "crypto/aes.h"

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

/* Arbitrary, non-secret test keys. Deliberately not all-zero so a bug that
 * ignores the key still fails. */
static const uint8_t K_KIRK[16] = {
    0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
    0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF
};
static const uint8_t K_BODY[16] = {
    0xA5,0x5A,0x01,0x02,0x03,0x04,0x05,0x06,
    0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E
};
static const uint8_t K_CMAC[16] = {
    0xDE,0xAD,0xBE,0xEF,0xCA,0xFE,0xBA,0xBE,
    0x13,0x37,0x42,0x24,0x99,0x88,0x77,0x66
};

static uint8_t blob[4096];
static size_t  blob_len;

/* Build a blob carrying `data`, with `pad` bytes between header and body. */
static int make_blob(const uint8_t *data, uint32_t len, uint32_t pad) {
    kirk_result r = kirk_cmd1_build(K_KIRK, K_BODY, K_CMAC, data, len, pad,
                                    blob, sizeof blob, &blob_len);
    return r == KIRK_OK;
}

static void test_round_trip(void) {
    /* A recognisable payload: if decryption is subtly wrong, a repeating
     * pattern makes it obvious where. */
    uint8_t payload[256];
    for (int i = 0; i < 256; i++) payload[i] = (uint8_t)(i ^ 0x5A);

    CHECK(make_blob(payload, sizeof payload, 0), "build a CMD1 blob");
    CHECK(blob_len == 0x90 + 256, "blob size: got %zu", blob_len);

    kirk1_info info;
    uint8_t *out = NULL;
    uint32_t out_len = 0;
    kirk_result r = kirk_cmd1_decrypt(K_KIRK, blob, blob_len, &info, &out, &out_len);

    CHECK(r == KIRK_OK, "decrypt: %s", kirk_strerror(r));
    CHECK(info.header_cmac_ok, "header CMAC verified");
    CHECK(info.data_cmac_ok, "body CMAC verified");
    CHECK(out_len == sizeof payload, "decrypted length: got %u", out_len);
    CHECK(out && memcmp(out, payload, sizeof payload) == 0, "payload round-trips");
}

static void test_padding(void) {
    /* Real modules put a nonzero gap between the header and the body, and that
     * gap is covered by the body CMAC. An implementation that assumes zero
     * padding passes every other test here and then fails on the first real
     * module, so this case is pinned explicitly. */
    uint8_t payload[64];
    memset(payload, 0xC3, sizeof payload);

    CHECK(make_blob(payload, sizeof payload, 0x30), "build with 0x30 padding");
    CHECK(blob_len == 0x90 + 0x30 + 64, "padded blob size: got %zu", blob_len);

    kirk1_info info;
    uint8_t *out = NULL;
    uint32_t out_len = 0;
    kirk_result r = kirk_cmd1_decrypt(K_KIRK, blob, blob_len, &info, &out, &out_len);

    CHECK(r == KIRK_OK, "decrypt padded blob: %s", kirk_strerror(r));
    CHECK(info.data_offset == 0x30, "padding reported: got 0x%X", info.data_offset);
    CHECK(out == blob + 0x90 + 0x30, "body located past the padding");
    CHECK(out && memcmp(out, payload, sizeof payload) == 0, "padded payload round-trips");
}

static void test_unaligned_length(void) {
    /* A data length that is not a multiple of the AES block size. The body is
     * encrypted to the next block boundary but only data_size bytes are real. */
    uint8_t payload[100];
    for (int i = 0; i < 100; i++) payload[i] = (uint8_t)(i + 1);

    CHECK(make_blob(payload, sizeof payload, 0), "build with a 100-byte payload");
    CHECK(blob_len == 0x90 + 112, "body padded to a block boundary: got %zu", blob_len);

    kirk1_info info;
    uint8_t *out = NULL;
    uint32_t out_len = 0;
    kirk_result r = kirk_cmd1_decrypt(K_KIRK, blob, blob_len, &info, &out, &out_len);

    CHECK(r == KIRK_OK, "decrypt unaligned: %s", kirk_strerror(r));
    CHECK(out_len == 100, "reported length is the real one, not the padded one: got %u", out_len);
    CHECK(out && memcmp(out, payload, 100) == 0, "unaligned payload round-trips");
}

static void test_wrong_key(void) {
    /* The decisive property: a wrong KIRK1 key must fail at the *header* CMAC,
     * not somewhere vague later. That is what makes "is my key right?" a
     * yes/no question instead of a judgement call about whether the output
     * looks like code. */
    uint8_t payload[32];
    memset(payload, 0x77, sizeof payload);
    CHECK(make_blob(payload, sizeof payload, 0), "build");

    uint8_t bad_key[16];
    memcpy(bad_key, K_KIRK, 16);
    bad_key[0] ^= 0x01;                 /* one bit wrong */

    kirk1_info info;
    kirk_result r = kirk_cmd1_decrypt(bad_key, blob, blob_len, &info, NULL, NULL);

    CHECK(r == KIRK_ERR_HEADER_CMAC, "one wrong key bit is caught at the header CMAC: got %s",
          kirk_strerror(r));
    CHECK(!info.header_cmac_ok, "header CMAC reported as failed");
}

static void test_tampered_body(void) {
    /* A correct key but a corrupt body must fail at the *data* CMAC, so the
     * two failure modes stay distinguishable. */
    uint8_t payload[64];
    memset(payload, 0x11, sizeof payload);
    CHECK(make_blob(payload, sizeof payload, 0), "build");

    blob[0x90 + 5] ^= 0x80;             /* flip a bit in the ciphertext body */

    kirk1_info info;
    kirk_result r = kirk_cmd1_decrypt(K_KIRK, blob, blob_len, &info, NULL, NULL);

    CHECK(r == KIRK_ERR_DATA_CMAC, "body tampering caught at the data CMAC: got %s",
          kirk_strerror(r));
    CHECK(info.header_cmac_ok, "header still verifies — the key was fine");
    CHECK(!info.data_cmac_ok, "body CMAC reported as failed");
}

static void test_malformed(void) {
    uint8_t payload[16];
    memset(payload, 0x22, sizeof payload);

    kirk1_info info;

    /* Too short to even hold a header. */
    CHECK(kirk_cmd1_decrypt(K_KIRK, blob, 0x10, &info, NULL, NULL) == KIRK_ERR_TOO_SMALL,
          "reject a truncated buffer");

    /* Wrong mode. */
    CHECK(make_blob(payload, sizeof payload, 0), "build");
    blob[0x60] = 3;
    CHECK(kirk_cmd1_decrypt(K_KIRK, blob, blob_len, &info, NULL, NULL) == KIRK_ERR_BAD_MODE,
          "reject a non-CMD1 mode");

    /* ECDSA-signed modules are out of scope and must say so rather than
     * failing a CMAC check confusingly. */
    CHECK(make_blob(payload, sizeof payload, 0), "build");
    blob[0x64] = 1;
    CHECK(kirk_cmd1_decrypt(K_KIRK, blob, blob_len, &info, NULL, NULL) == KIRK_ERR_ECDSA,
          "report ECDSA rather than failing a CMAC");

    /* A declared size that cannot fit. This is the case a corrupt dump hits,
     * and the one where an unchecked implementation reads or writes out of
     * bounds. */
    CHECK(make_blob(payload, sizeof payload, 0), "build");
    blob[0x70] = 0xFF; blob[0x71] = 0xFF; blob[0x72] = 0xFF; blob[0x73] = 0xFF;
    CHECK(kirk_cmd1_decrypt(K_KIRK, blob, blob_len, &info, NULL, NULL) == KIRK_ERR_SIZE_OVERFLOW,
          "reject an absurd data size without reading out of bounds");

    /* Same for the padding field. */
    CHECK(make_blob(payload, sizeof payload, 0), "build");
    blob[0x74] = 0xFF; blob[0x75] = 0xFF; blob[0x76] = 0xFF; blob[0x77] = 0xFF;
    CHECK(kirk_cmd1_decrypt(K_KIRK, blob, blob_len, &info, NULL, NULL) == KIRK_ERR_SIZE_OVERFLOW,
          "reject an absurd padding offset");
}

static void test_peek(void) {
    /* The metadata is plaintext, so a caller can learn what a module claims
     * about itself while holding no key at all. */
    uint8_t payload[48];
    memset(payload, 0x33, sizeof payload);
    CHECK(make_blob(payload, sizeof payload, 0x20), "build");

    kirk1_info info;
    CHECK(kirk_cmd1_peek(blob, blob_len, &info) == KIRK_OK, "peek without a key");
    CHECK(info.mode == 1, "peek reads the mode");
    CHECK(info.data_size == 48, "peek reads the data size: got %u", info.data_size);
    CHECK(info.data_offset == 0x20, "peek reads the padding: got 0x%X", info.data_offset);
    CHECK(info.use_ecdsa == 0, "peek reads the signature type");
}

int main(void) {
    test_round_trip();
    test_padding();
    test_unaligned_length();
    test_wrong_key();
    test_tampered_body();
    test_malformed();
    test_peek();

    if (failures) {
        printf("\n%d check(s) failed\n", failures);
        return 1;
    }
    printf("all KIRK CMD1 checks passed (synthetic blobs, no PSP key material)\n");
    return 0;
}
