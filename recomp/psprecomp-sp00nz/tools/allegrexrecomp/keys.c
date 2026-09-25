/* External key material. See keys.h. */

#include "keys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DEFAULT_KEY_PATH "keys/psp_keys.txt"
#define ENV_KEY_PATH     "PSPRECOMP_KEYS"

void keys_describe_search(char *out, size_t outlen) {
    snprintf(out, outlen,
             "--keys <path>, then $%s, then ./%s", ENV_KEY_PATH, DEFAULT_KEY_PATH);
}

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Parse a run of hex digits, tolerating embedded whitespace so a long key can
 * be wrapped or spaced into readable groups. Returns bytes written, or -1 if a
 * non-hex character appears or the digit count is odd. */
static int parse_hex(const char *s, uint8_t *out, size_t max) {
    size_t n = 0;
    int hi = -1;

    for (; *s; s++) {
        if (isspace((unsigned char)*s) || *s == ':' || *s == ',') continue;
        int v = hexval((unsigned char)*s);
        if (v < 0) return -1;
        if (hi < 0) {
            hi = v;
        } else {
            if (n >= max) return -1;
            out[n++] = (uint8_t)((hi << 4) | v);
            hi = -1;
        }
    }
    if (hi >= 0) return -1;          /* odd number of digits */
    return (int)n;
}

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) *--end = '\0';
    return s;
}

static int load_file(key_store *ks, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[1024];
    int lineno = 0;
    ks->count = 0;

    while (fgets(line, sizeof line, f)) {
        lineno++;

        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';

        char *eq = strchr(line, '=');
        if (!eq) {
            /* A line with no '=' is either blank or garbage; blank is fine. */
            if (*trim(line)) {
                fprintf(stderr, "%s:%d: ignoring line without '='\n", path, lineno);
            }
            continue;
        }
        *eq = '\0';

        char *name = trim(line);
        char *val  = trim(eq + 1);
        if (!*name || !*val) continue;

        if (ks->count >= KEYS_MAX_ENTRIES) {
            fprintf(stderr, "%s:%d: too many keys (max %d)\n", path, lineno, KEYS_MAX_ENTRIES);
            break;
        }

        key_entry *e = &ks->entry[ks->count];
        int n = parse_hex(val, e->value, sizeof e->value);
        if (n < 0) {
            fprintf(stderr, "%s:%d: '%s' is not valid hex, skipping\n", path, lineno, name);
            continue;
        }
        snprintf(e->name, sizeof e->name, "%s", name);
        e->len = (size_t)n;
        ks->count++;
    }

    fclose(f);
    snprintf(ks->path, sizeof ks->path, "%s", path);
    ks->loaded = 1;
    return 0;
}

int keys_load(key_store *ks, const char *path) {
    memset(ks, 0, sizeof *ks);

    if (path && *path) return load_file(ks, path);

    const char *env = getenv(ENV_KEY_PATH);
    if (env && *env && load_file(ks, env) == 0) return 0;

    return load_file(ks, DEFAULT_KEY_PATH);
}

const uint8_t *keys_get(const key_store *ks, const char *name, size_t *len) {
    for (int i = 0; i < ks->count; i++) {
        if (strcmp(ks->entry[i].name, name) == 0) {
            if (len) *len = ks->entry[i].len;
            return ks->entry[i].value;
        }
    }
    return NULL;
}

const uint8_t *keys_require(const key_store *ks, const char *name, size_t want_len,
                            char *err, size_t errlen) {
    if (!ks->loaded) {
        char search[256];
        keys_describe_search(search, sizeof search);
        snprintf(err, errlen, "no key file found (looked in: %s)", search);
        return NULL;
    }

    size_t len = 0;
    const uint8_t *v = keys_get(ks, name, &len);
    if (!v) {
        snprintf(err, errlen, "key '%s' not found in %s", name, ks->path);
        return NULL;
    }
    if (want_len && len != want_len) {
        snprintf(err, errlen, "key '%s' in %s is %zu bytes, expected %zu",
                 name, ks->path, len, want_len);
        return NULL;
    }
    return v;
}
