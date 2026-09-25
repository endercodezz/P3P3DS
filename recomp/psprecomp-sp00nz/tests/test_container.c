/* Container tests. Every buffer here is built in the test itself — there is no
 * disc image, no PBP, and no game module in this repo. The point is that the
 * parsers reject malformed input rather than walking off the end of it, which
 * is the failure mode that matters when you point the tool at 4,000 dumps.
 */

#include "container.h"

#include <stdio.h>
#include <stdlib.h>
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

static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static void put16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}

static void test_sniff(void) {
    uint8_t buf[16];

    memset(buf, 0, sizeof buf);
    memcpy(buf, "\x7f" "ELF", 4);
    CHECK(psp_sniff(buf, 16) == FMT_ELF, "sniff ELF");

    memcpy(buf, "~PSP", 4);
    CHECK(psp_sniff(buf, 16) == FMT_PSP, "sniff ~PSP");

    memcpy(buf, "~SCE", 4);
    CHECK(psp_sniff(buf, 16) == FMT_SCE, "sniff ~SCE");

    memcpy(buf, "\0PBP", 4);
    CHECK(psp_sniff(buf, 16) == FMT_PBP, "sniff PBP");

    memcpy(buf, "junk", 4);
    CHECK(psp_sniff(buf, 16) == FMT_UNKNOWN, "sniff unknown");

    /* A short buffer must not be read past. */
    CHECK(psp_sniff(buf, 2) == FMT_UNKNOWN, "sniff refuses a short buffer");
}

static void test_pbp(void) {
    /* A PBP is a header of eight offsets; segment sizes are the gaps between
     * them. This is the layout WTF's game-sharing microgames use. */
    uint8_t buf[0x100];
    memset(buf, 0, sizeof buf);
    memcpy(buf, "\0PBP", 4);
    put32(buf + 4, 0x00010000);

    /* PARAM.SFO at 0x28 (len 0x18), then an empty ICON0, then DATA.PSP. */
    const uint32_t offsets[PBP_NUM_SEGMENTS] = {
        0x28, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x80
    };
    for (int i = 0; i < PBP_NUM_SEGMENTS; i++)
        put32(buf + 8 + i * 4, offsets[i]);

    pbp_info p;
    CHECK(pbp_parse(buf, sizeof buf, &p) == 0, "parse a well-formed PBP");
    CHECK(p.size[0] == 0x18, "PARAM.SFO size derived from the next offset: got %u", p.size[0]);
    CHECK(p.size[1] == 0, "an empty segment has zero size");
    CHECK(p.size[6] == 0x40, "DATA.PSP size: got %u", p.size[6]);
    CHECK(p.size[7] == sizeof buf - 0x80, "the last segment runs to EOF: got %u", p.size[7]);

    /* Garbage must be rejected, not parsed into nonsense. */
    memcpy(buf, "XXXX", 4);
    CHECK(pbp_parse(buf, sizeof buf, &p) != 0, "reject a bad PBP magic");
    memcpy(buf, "\0PBP", 4);
    CHECK(pbp_parse(buf, 8, &p) != 0, "reject a truncated PBP header");
}

static void test_psp_header(void) {
    uint8_t buf[PSP_HEADER_SIZE];
    memset(buf, 0, sizeof buf);
    memcpy(buf, "~PSP", 4);
    put16(buf + 0x04, 0x0000);          /* mod_attr */
    put16(buf + 0x06, 0x0002);          /* comp_attr */
    buf[0x08] = 0x01;                   /* ver lo */
    buf[0x09] = 0x01;                   /* ver hi */
    memcpy(buf + 0x0A, "testmodule", 10);
    buf[0x27] = 2;                      /* nsegments */
    put32(buf + 0x28, 0x00100000);      /* elf_size */
    put32(buf + 0x2C, 0x00080000);      /* psp_size */
    put32(buf + 0x30, 0x08900000);      /* boot_entry */
    put32(buf + 0x44, 0x08900000);      /* seg 0 address */
    put32(buf + 0x54, 0x00040000);      /* seg 0 size */
    /* The key tag lives at 0xD0. It is deliberately pinned here: an earlier
     * revision read it from 0x130, which lands in the encrypted key material
     * and yields a different plausible-looking value per module. */
    put32(buf + 0xD0, 0x4C949AF0);
    put32(buf + 0x130, 0xDEADBEEF);     /* must NOT be mistaken for the tag */

    psp_header h;
    CHECK(psp_header_parse(buf, sizeof buf, &h) == 0, "parse a ~PSP header");
    CHECK(strcmp(h.modname, "testmodule") == 0, "module name: got \"%s\"", h.modname);
    CHECK(h.nsegments == 2, "segment count");
    CHECK(h.elf_size == 0x00100000, "decrypted ELF size");
    CHECK(h.boot_entry == 0x08900000, "boot entry");
    CHECK(h.seg_address[0] == 0x08900000, "segment 0 address");
    CHECK(h.tag == 0x4C949AF0, "key tag read from 0xD0, not 0x130");

    /* The module name field is not guaranteed NUL-terminated on disc; the
     * parser must terminate it itself rather than running into the next field. */
    memset(buf + 0x0A, 'A', 28);
    CHECK(psp_header_parse(buf, sizeof buf, &h) == 0, "parse with a full name field");
    CHECK(strlen(h.modname) == 28, "over-long module name is terminated at 28");

    CHECK(psp_header_parse(buf, 16, &h) != 0, "reject a truncated ~PSP header");
}

static void test_elf(void) {
    /* Minimal ELF32 LE MIPS with one executable PT_LOAD. */
    uint8_t buf[256];
    memset(buf, 0, sizeof buf);
    memcpy(buf, "\x7f" "ELF", 4);
    buf[4] = 1;                          /* ELFCLASS32 */
    buf[5] = 1;                          /* ELFDATA2LSB */
    put16(buf + 16, ET_PSP_PRX);         /* e_type */
    put32(buf + 24, 0x08900010);         /* e_entry */
    put32(buf + 28, 52);                 /* e_phoff */
    put16(buf + 42, 32);                 /* e_phentsize */
    put16(buf + 44, 1);                  /* e_phnum */

    uint8_t *ph = buf + 52;
    put32(ph +  0, 1);                   /* PT_LOAD */
    put32(ph +  4, 0x80);                /* p_offset */
    put32(ph +  8, 0x08900000);          /* p_vaddr */
    put32(ph + 16, 0x40);                /* p_filesz */
    put32(ph + 20, 0x40);                /* p_memsz */
    put32(ph + 24, 0x5);                 /* PF_R | PF_X */

    elf_info e;
    CHECK(elf_parse(buf, sizeof buf, &e) == 0, "parse a minimal PRX");
    CHECK(e.type == ET_PSP_PRX, "e_type is PRX");
    CHECK(e.entry == 0x08900010, "entry point");
    CHECK(e.nsegments == 1, "one PT_LOAD");
    CHECK(e.text_addr == 0x08900000, "text address picked from the PF_X segment");
    CHECK(e.text_size == 0x40, "text size");
    CHECK(e.text_offset == 0x80, "text file offset");

    /* A non-executable segment must not be mistaken for .text. */
    put32(ph + 24, 0x6);                 /* PF_R | PF_W, no PF_X */
    CHECK(elf_parse(buf, sizeof buf, &e) == 0, "parse with a data-only segment");
    CHECK(e.text_size == 0, "a non-executable segment is not .text");

    /* Wrong class / endianness / magic must all be rejected. */
    put32(ph + 24, 0x5);
    buf[4] = 2;
    CHECK(elf_parse(buf, sizeof buf, &e) != 0, "reject ELFCLASS64");
    buf[4] = 1; buf[5] = 2;
    CHECK(elf_parse(buf, sizeof buf, &e) != 0, "reject big-endian ELF");
    buf[5] = 1;
    CHECK(elf_parse(buf, 8, &e) != 0, "reject a truncated ELF");

    /* A program-header table pointing past the end of the buffer must not be
     * followed — this is the case a corrupt dump actually hits. */
    put32(buf + 28, 0xFFFF0000);
    CHECK(elf_parse(buf, sizeof buf, &e) == 0, "a bogus phoff parses without crashing");
    CHECK(e.nsegments == 0, "a bogus phoff yields no segments");
}

static void test_sfo(void) {
    /* PARAM.SFO: a key table and a data table, both at header-declared offsets.
     * We check that a malformed one is refused rather than walked. */
    uint8_t buf[0x100];
    memset(buf, 0, sizeof buf);
    put32(buf + 0x00, 0x46535000);   /* "\0PSF" */
    put32(buf + 0x04, 0x01010000);   /* version */
    put32(buf + 0x08, 0x30);         /* key table offset */
    put32(buf + 0x0C, 0x50);         /* data table offset */
    put32(buf + 0x10, 1);            /* one entry */

    put16(buf + 0x14, 0);            /* key offset */
    put16(buf + 0x16, 0x0204);       /* UTF-8 */
    put32(buf + 0x18, 10);           /* data length */
    put32(buf + 0x1C, 16);           /* data max */
    put32(buf + 0x20, 0);            /* data offset */

    memcpy(buf + 0x30, "DISC_ID", 8);
    memcpy(buf + 0x50, "ULUS10172", 10);

    /* sfo_dump writes to a FILE*; discard the output, we only care that it
     * accepts a good blob and rejects a bad one without reading out of range. */
    FILE *devnull = tmpfile();
    CHECK(devnull != NULL, "open a scratch file");
    if (devnull) {
        CHECK(sfo_dump(buf, sizeof buf, devnull) == 0, "dump a well-formed SFO");

        put32(buf + 0x00, 0xDEADBEEF);
        CHECK(sfo_dump(buf, sizeof buf, devnull) != 0, "reject a bad SFO magic");

        /* An absurd entry count must be refused, not looped over. */
        put32(buf + 0x00, 0x46535000);
        put32(buf + 0x10, 0xFFFFFFFF);
        CHECK(sfo_dump(buf, sizeof buf, devnull) != 0, "reject an absurd entry count");

        fclose(devnull);
    }
}

int main(void) {
    test_sniff();
    test_pbp();
    test_psp_header();
    test_elf();
    test_sfo();

    if (failures) {
        printf("\n%d check(s) failed\n", failures);
        return 1;
    }
    printf("all container checks passed\n");
    return 0;
}
