/* KIRK command 1. See kirk.h for the header layout. */

#include "kirk.h"
#include "aes.h"
#include "cmac.h"

#include <string.h>

static uint32_t rd32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void wr32le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

const char *kirk_strerror(kirk_result r) {
    switch (r) {
    case KIRK_OK:              return "ok";
    case KIRK_ERR_TOO_SMALL:   return "buffer is smaller than a KIRK CMD1 header";
    case KIRK_ERR_BAD_MODE:    return "metadata mode is not 1 (not a CMD1 blob)";
    case KIRK_ERR_ECDSA:       return "module is ECDSA-signed; only AES-CMAC is supported";
    case KIRK_ERR_HEADER_CMAC: return "header CMAC mismatch (wrong KIRK1 key)";
    case KIRK_ERR_DATA_CMAC:   return "body CMAC mismatch (header ok, body corrupt or truncated)";
    case KIRK_ERR_SIZE_OVERFLOW: return "declared sizes do not fit in the buffer";
    default:                   return "unknown error";
    }
}

static void read_meta(const uint8_t *buf, kirk1_info *info) {
    const uint8_t *m = buf + KIRK1_META_OFFSET;
    memset(info, 0, sizeof *info);
    info->mode        = rd32le(m + 0x00);
    info->use_ecdsa   = rd32le(m + 0x04);
    info->is_devkit   = rd32le(m + 0x0C);
    info->data_size   = rd32le(m + 0x10);
    info->data_offset = rd32le(m + 0x14);
}

kirk_result kirk_cmd1_peek(const uint8_t *buf, size_t len, kirk1_info *info) {
    if (len < KIRK1_HEADER_SIZE) return KIRK_ERR_TOO_SMALL;
    read_meta(buf, info);
    if (info->mode != KIRK1_MODE_CMD1) return KIRK_ERR_BAD_MODE;
    return KIRK_OK;
}

kirk_result kirk_cmd1_decrypt(const uint8_t kirk1_key[16],
                              uint8_t *buf, size_t len,
                              kirk1_info *info,
                              uint8_t **out, uint32_t *out_len) {
    if (len < KIRK1_HEADER_SIZE) return KIRK_ERR_TOO_SMALL;

    /* The metadata at 0x60 is plaintext, so it can be trusted for sizing
     * decisions before any key is applied — but it is attacker-controlled in
     * the sense that a corrupt file can say anything, so every derived offset
     * is bounds-checked below rather than assumed. */
    read_meta(buf, info);
    if (info->mode != KIRK1_MODE_CMD1) return KIRK_ERR_BAD_MODE;
    if (info->use_ecdsa)               return KIRK_ERR_ECDSA;

    /* Body extent, and the CMAC extent that covers it. Computed in size_t and
     * compared against len so a bogus 0xFFFFFFFF cannot wrap. */
    const size_t body_off  = (size_t)KIRK1_HEADER_SIZE + info->data_offset;
    const size_t body_len  = info->data_size;
    const size_t body_pad  = (body_len + 15u) & ~(size_t)15u;  /* AES works in blocks */
    if (body_off > len || body_pad > len - body_off) return KIRK_ERR_SIZE_OVERFLOW;

    const size_t data_cmac_len =
        (size_t)KIRK1_META_SIZE + info->data_offset + body_pad;
    if ((size_t)KIRK1_META_OFFSET + data_cmac_len > len) return KIRK_ERR_SIZE_OVERFLOW;

    /* Step 1: recover the two keys. Offsets 0x00-0x1F are one AES-CBC run
     * under the KIRK1 key with a zero IV, so the block at 0x10 is chained
     * against the *ciphertext* at 0x00. */
    aes_ctx kek;
    if (aes_init(&kek, kirk1_key, 128) != 0) return KIRK_ERR_TOO_SMALL;

    uint8_t keys[0x20];
    const uint8_t zero_iv[16] = { 0 };
    memcpy(keys, buf, 0x20);
    aes_cbc_decrypt(&kek, zero_iv, keys, keys, 0x20);

    const uint8_t *body_key = keys;
    const uint8_t *cmac_key = keys + 0x10;

    /* Step 2: the header CMAC, over the 0x30-byte metadata block. This is the
     * decisive check — if it passes, the KIRK1 key is right and the CMAC key
     * was recovered correctly, so any later failure is about the body, not
     * about the key. That separation is most of this function's value. */
    uint8_t mac[16];
    aes_cmac(cmac_key, 128, buf + KIRK1_META_OFFSET, KIRK1_META_SIZE, mac);
    info->header_cmac_ok = cmac_equal(mac, buf + 0x20);

    /* Step 3: the body CMAC, over the metadata plus the padding plus the
     * still-encrypted body. Computed before decryption — it authenticates the
     * ciphertext, not the plaintext. */
    aes_cmac(cmac_key, 128, buf + KIRK1_META_OFFSET, data_cmac_len, mac);
    info->data_cmac_ok = cmac_equal(mac, buf + 0x30);

    if (!info->header_cmac_ok) return KIRK_ERR_HEADER_CMAC;
    if (!info->data_cmac_ok)   return KIRK_ERR_DATA_CMAC;

    /* Step 4: decrypt the body in place, again CBC with a zero IV. */
    aes_ctx body;
    aes_init(&body, body_key, 128);
    aes_cbc_decrypt(&body, zero_iv, buf + body_off, buf + body_off, body_pad);

    if (out)     *out = buf + body_off;
    if (out_len) *out_len = info->data_size;
    return KIRK_OK;
}

kirk_result kirk_cmd1_build(const uint8_t kirk1_key[16],
                            const uint8_t body_key[16],
                            const uint8_t cmac_key[16],
                            const uint8_t *data, uint32_t data_len, uint32_t pad,
                            uint8_t *out, size_t out_cap, size_t *out_len) {
    const size_t body_pad = ((size_t)data_len + 15u) & ~(size_t)15u;
    const size_t body_off = (size_t)KIRK1_HEADER_SIZE + pad;
    const size_t total    = body_off + body_pad;
    if (out_cap < total) return KIRK_ERR_SIZE_OVERFLOW;

    memset(out, 0, total);

    /* Metadata first — the CMACs cover it, so it must be final before they
     * are computed. */
    uint8_t *m = out + KIRK1_META_OFFSET;
    wr32le(m + 0x00, KIRK1_MODE_CMD1);
    wr32le(m + 0x04, 0);            /* AES CMAC, not ECDSA */
    wr32le(m + 0x0C, 0);            /* retail */
    wr32le(m + 0x10, data_len);
    wr32le(m + 0x14, pad);

    /* Encrypt the body. */
    const uint8_t zero_iv[16] = { 0 };
    aes_ctx body;
    aes_init(&body, body_key, 128);
    memcpy(out + body_off, data, data_len);
    aes_cbc_encrypt(&body, zero_iv, out + body_off, out + body_off, body_pad);

    /* CMACs over the plaintext metadata, and over metadata + padding + the
     * now-encrypted body. */
    aes_cmac(cmac_key, 128, out + KIRK1_META_OFFSET, KIRK1_META_SIZE, out + 0x20);
    aes_cmac(cmac_key, 128, out + KIRK1_META_OFFSET,
             (size_t)KIRK1_META_SIZE + pad + body_pad, out + 0x30);

    /* Finally wrap the two keys under the KIRK1 key. */
    memcpy(out + 0x00, body_key, 16);
    memcpy(out + 0x10, cmac_key, 16);
    aes_ctx kek;
    aes_init(&kek, kirk1_key, 128);
    aes_cbc_encrypt(&kek, zero_iv, out, out, 0x20);

    if (out_len) *out_len = total;
    return KIRK_OK;
}
