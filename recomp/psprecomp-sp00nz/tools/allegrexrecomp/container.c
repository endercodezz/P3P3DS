/* allegrexrecomp — container parsing. See container.h. */

#include "container.h"
#include "decode.h"

#include <stdlib.h>
#include <string.h>

/* Little-endian readers. The PSP is LE and so is every host we build for, but
 * going through these keeps the parsers honest and endian-independent. */
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* ---- format sniffing ----------------------------------------------------- */

psp_format psp_sniff(const uint8_t *d, size_t len) {
    if (len < 16) return FMT_UNKNOWN;
    if (!memcmp(d, "\x7f" "ELF", 4))  return FMT_ELF;
    if (!memcmp(d, "~PSP", 4))        return FMT_PSP;
    if (!memcmp(d, "~SCE", 4))        return FMT_SCE;
    if (!memcmp(d, "\0PBP", 4))       return FMT_PBP;
    /* ISO9660: the primary volume descriptor sits at sector 16 with "CD001".
     * A 16-byte sniff cannot see that, so callers handle ISO separately; we
     * only report it here when handed a large enough buffer. */
    if (len > 16 * ISO_SECTOR + 6 &&
        !memcmp(d + 16 * ISO_SECTOR + 1, "CD001", 5)) return FMT_ISO9660;
    return FMT_UNKNOWN;
}

const char *psp_format_name(psp_format f) {
    switch (f) {
    case FMT_ISO9660: return "ISO9660 disc image";
    case FMT_PBP:     return "PBP container";
    case FMT_PSP:     return "~PSP encrypted PRX";
    case FMT_SCE:     return "~SCE wrapper";
    case FMT_ELF:     return "ELF32";
    default:          return "unknown";
    }
}

/* ---- blob ---------------------------------------------------------------- */

int psp_blob_read(const char *path, psp_blob *out) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    long n = ftell(f);
    if (n < 0) { fclose(f); return -1; }
    rewind(f);
    uint8_t *buf = (uint8_t *)malloc((size_t)n ? (size_t)n : 1);
    if (!buf) { fclose(f); return -1; }
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); fclose(f); return -1; }
    fclose(f);
    out->data = buf;
    out->size = (size_t)n;
    return 0;
}

void psp_blob_free(psp_blob *b) {
    free(b->data);
    b->data = NULL;
    b->size = 0;
}

/* ---- ISO9660 ------------------------------------------------------------- */

/* Recursively walk one directory extent. `count` is both the running total and
 * the write cursor into `out`; `out` may be NULL to count only. */
static int iso_walk(FILE *f, uint32_t lba, uint32_t size, const char *prefix,
                    iso_entry *out, int max_entries, int *count, int depth) {
    if (depth > 8) return 0;   /* ISO9660 caps at 8; also stops cycles */

    uint32_t nsec = (size + ISO_SECTOR - 1) / ISO_SECTOR;
    uint8_t *buf = (uint8_t *)malloc((size_t)nsec * ISO_SECTOR);
    if (!buf) return -1;
    if (fseek(f, (long)lba * ISO_SECTOR, SEEK_SET) != 0 ||
        fread(buf, 1, (size_t)nsec * ISO_SECTOR, f) != (size_t)nsec * ISO_SECTOR) {
        free(buf);
        return -1;
    }

    uint32_t off = 0;
    const uint32_t total = nsec * ISO_SECTOR;
    while (off < total) {
        uint8_t rlen = buf[off];
        if (rlen == 0) {
            /* Records never straddle a sector; skip the tail padding. */
            off = (off / ISO_SECTOR + 1) * ISO_SECTOR;
            continue;
        }
        if (off + rlen > total) break;

        const uint8_t *rec = buf + off;
        uint32_t ext_lba  = rd32(rec + 2);
        uint32_t ext_size = rd32(rec + 10);
        uint8_t  flags    = rec[25];
        uint8_t  namelen  = rec[32];
        off += rlen;

        /* Skip the "." and ".." self/parent records. */
        if (namelen == 1 && (rec[33] == 0x00 || rec[33] == 0x01)) continue;
        if (namelen > 200) continue;

        char name[224];
        memcpy(name, rec + 33, namelen);
        name[namelen] = '\0';
        char *semi = strchr(name, ';');   /* strip the ISO version suffix */
        if (semi) *semi = '\0';

        char full[256];
        snprintf(full, sizeof full, "%s/%s", prefix, name);

        int is_dir = (flags & 0x02) != 0;
        if (out && *count < max_entries) {
            iso_entry *e = &out[*count];
            snprintf(e->path, sizeof e->path, "%s", full);
            e->lba = ext_lba;
            e->size = ext_size;
            e->is_dir = is_dir;
        }
        (*count)++;

        if (is_dir)
            iso_walk(f, ext_lba, ext_size, full, out, max_entries, count, depth + 1);
    }

    free(buf);
    return 0;
}

int iso_list(FILE *f, iso_entry *out, int max_entries) {
    uint8_t pvd[ISO_SECTOR];
    if (fseek(f, 16 * ISO_SECTOR, SEEK_SET) != 0) return -1;
    if (fread(pvd, 1, ISO_SECTOR, f) != ISO_SECTOR) return -1;
    if (memcmp(pvd + 1, "CD001", 5) != 0) return -1;

    /* The root directory record is embedded at offset 156 of the PVD. */
    uint32_t root_lba  = rd32(pvd + 156 + 2);
    uint32_t root_size = rd32(pvd + 156 + 10);

    int count = 0;
    if (iso_walk(f, root_lba, root_size, "", out, max_entries, &count, 0) != 0)
        return -1;
    return count;
}

int iso_extract(FILE *f, const iso_entry *e, const char *dst_path) {
    if (e->is_dir) return -1;
    FILE *o = fopen(dst_path, "wb");
    if (!o) return -1;
    if (fseek(f, (long)e->lba * ISO_SECTOR, SEEK_SET) != 0) { fclose(o); return -1; }

    uint8_t chunk[64 * 1024];
    uint32_t left = e->size;
    while (left) {
        uint32_t want = left < sizeof chunk ? left : (uint32_t)sizeof chunk;
        if (fread(chunk, 1, want, f) != want) { fclose(o); return -1; }
        if (fwrite(chunk, 1, want, o) != want) { fclose(o); return -1; }
        left -= want;
    }
    fclose(o);
    return 0;
}

/* ---- PARAM.SFO ----------------------------------------------------------- */

int sfo_dump(const uint8_t *d, size_t len, FILE *out) {
    if (len < 0x14 || rd32(d) != 0x46535000u /* "\0PSF" */) return -1;

    uint32_t key_off  = rd32(d + 0x08);
    uint32_t data_off = rd32(d + 0x0C);
    uint32_t count    = rd32(d + 0x10);
    if (count > 256) return -1;   /* sanity: real SFOs have a few dozen keys */

    for (uint32_t i = 0; i < count; i++) {
        size_t e = 0x14 + (size_t)i * 16;
        if (e + 16 > len) break;
        uint16_t ko    = rd16(d + e);
        uint16_t fmt   = rd16(d + e + 2);
        uint32_t dlen  = rd32(d + e + 4);
        uint32_t dof   = rd32(d + e + 12);

        if (key_off + ko >= len) break;
        const char *key = (const char *)(d + key_off + ko);

        if ((size_t)data_off + dof + dlen > len) break;
        const uint8_t *val = d + data_off + dof;

        if (fmt == 0x0404 && dlen >= 4)
            fprintf(out, "  %-18s %u\n", key, rd32(val));
        else
            fprintf(out, "  %-18s %.*s\n", key, (int)dlen, (const char *)val);
    }
    return 0;
}

/* ---- PBP ----------------------------------------------------------------- */

const char *const PBP_SEGMENT_NAMES[PBP_NUM_SEGMENTS] = {
    "PARAM.SFO", "ICON0.PNG", "ICON1.PMF", "PIC0.PNG",
    "PIC1.PNG",  "SND0.AT3",  "DATA.PSP",  "DATA.PSAR"
};

int pbp_parse(const uint8_t *d, size_t len, pbp_info *out) {
    if (len < 0x28 || memcmp(d, "\0PBP", 4) != 0) return -1;

    memset(out, 0, sizeof *out);
    out->version = rd32(d + 4);
    for (int i = 0; i < PBP_NUM_SEGMENTS; i++)
        out->offset[i] = rd32(d + 8 + i * 4);

    /* A segment runs to the next segment's offset; the last runs to EOF.
     * Offsets are non-decreasing, and equal offsets mean an empty segment. */
    for (int i = 0; i < PBP_NUM_SEGMENTS; i++) {
        uint32_t end = (i + 1 < PBP_NUM_SEGMENTS) ? out->offset[i + 1] : (uint32_t)len;
        out->size[i] = (end > out->offset[i]) ? end - out->offset[i] : 0;
    }
    return 0;
}

/* ---- ~PSP header --------------------------------------------------------- */

int psp_header_parse(const uint8_t *d, size_t len, psp_header *out) {
    if (len < PSP_HEADER_SIZE || memcmp(d, "~PSP", 4) != 0) return -1;

    memset(out, 0, sizeof *out);
    out->mod_attr  = rd16(d + 0x04);
    out->comp_attr = rd16(d + 0x06);
    out->mod_ver_lo = d[0x08];
    out->mod_ver_hi = d[0x09];
    memcpy(out->modname, d + 0x0A, 28);
    out->modname[28] = '\0';
    out->nsegments      = d[0x27];
    out->elf_size       = rd32(d + 0x28);
    out->psp_size       = rd32(d + 0x2C);
    out->boot_entry     = rd32(d + 0x30);
    out->modinfo_offset = rd32(d + 0x34);
    out->bss_size       = rd32(d + 0x38);
    for (int i = 0; i < 4; i++) {
        out->seg_address[i] = rd32(d + 0x44 + i * 4);
        out->seg_size[i]    = rd32(d + 0x54 + i * 4);
    }
    out->devkit_version = rd32(d + 0x78);
    out->decrypt_mode   = d[0x7C];
    /* The key tag lives at 0xD0. Offset 0x130 — which some references give —
     * lands in the encrypted key material, where every module yields a
     * different plausible-looking 32-bit value. That produces a convincing
     * table of "distinct per-module tags" that is entirely noise. Verified
     * against an independent decryptor: 0xD0 matches the tag it reports for
     * both a mode 9 and a mode 10 module, and 0x130 matches neither. */
    out->tag            = rd32(d + 0xD0);
    return 0;
}

/* ---- ELF / PRX ----------------------------------------------------------- */

int elf_parse(const uint8_t *d, size_t len, elf_info *out) {
    if (len < 52 || memcmp(d, "\x7f" "ELF", 4) != 0) return -1;
    if (d[4] != 1 || d[5] != 1) return -1;   /* ELFCLASS32, ELFDATA2LSB */

    memset(out, 0, sizeof *out);
    out->type  = rd16(d + 16);
    out->entry = rd32(d + 24);

    uint32_t phoff     = rd32(d + 28);
    uint16_t phentsize = rd16(d + 42);
    uint16_t phnum     = rd16(d + 44);
    if (phentsize < 32) return -1;

    int n = 0;
    for (uint16_t i = 0; i < phnum && n < 8; i++) {
        size_t p = (size_t)phoff + (size_t)i * phentsize;
        if (p + 32 > len) break;
        uint32_t p_type = rd32(d + p);
        if (p_type != 1) continue;           /* PT_LOAD only */

        elf_segment *s = &out->seg[n++];
        s->offset = rd32(d + p + 4);
        s->addr   = rd32(d + p + 8);
        s->filesz = rd32(d + p + 16);
        s->memsz  = rd32(d + p + 20);
        s->flags  = rd32(d + p + 24);

        /* First executable (PF_X) segment is the .text we hand to the decoder. */
        if ((s->flags & 0x1) && out->text_size == 0) {
            out->text_addr   = s->addr;
            out->text_offset = s->offset;
            out->text_size   = s->filesz;
        }
    }
    out->nsegments = n;

    /* Section headers, when present, tell us where the code actually is.
     * They are optional and sometimes stripped, so everything here is
     * best-effort on top of the segment information already gathered. */
    uint32_t shoff     = rd32(d + 32);
    uint16_t shentsize = rd16(d + 46);
    uint16_t shnum     = rd16(d + 48);
    uint16_t shstrndx  = rd16(d + 50);
    if (!shoff || shentsize < 40 || !shnum || shstrndx >= shnum) return 0;

    out->shoff = shoff;
    out->shentsize = shentsize;
    out->shnum = shnum;

    /* Locate the section-name string table via its own section header. */
    size_t strhdr = (size_t)shoff + (size_t)shstrndx * shentsize;
    if (strhdr + 40 > len) return 0;
    uint32_t stroff = rd32(d + strhdr + 16);
    uint32_t strsz  = rd32(d + strhdr + 20);
    if ((size_t)stroff + strsz > len) return 0;

    out->nsections = shnum;

    for (uint16_t i = 0; i < shnum; i++) {
        size_t sh = (size_t)shoff + (size_t)i * shentsize;
        if (sh + 40 > len) break;

        uint32_t nameoff = rd32(d + sh);
        uint32_t addr    = rd32(d + sh + 12);
        uint32_t off     = rd32(d + sh + 16);
        uint32_t size    = rd32(d + sh + 20);
        if (nameoff >= strsz) continue;

        const char *name = (const char *)(d + stroff + nameoff);
        /* The name table is bounded above, but a missing terminator would let
         * strcmp run past it; cap the comparison length instead. */
        size_t avail = strsz - nameoff;

        if (avail > 5 && strncmp(name, ".text", 6) == 0) {
            if ((size_t)off + size <= len) {
                out->text_addr = addr;
                out->text_offset = off;
                out->text_size = size;
                out->text_from_section = 1;
            }
        } else if (avail > 13 && strncmp(name, ".sceStub.text", 13) == 0) {
            /* Prefix match, deliberately not an exact one. A module carries
             * either a single `.sceStub.text` or one section *per firmware
             * library* (`.sceStub.text.sceCtrl`, `.sceStub.text.sceGe_user`,
             * ...). Matching the exact name only finds nothing at all on the
             * per-library layout — which reports zero imports and lets every
             * firmware call be mistaken for an internal function.
             *
             * The sections are contiguous in practice, so their union is the
             * range to test membership against. */
            if (!out->stub_size) {
                out->stub_addr = addr;
                out->stub_offset = off;
                out->stub_size = size;
            } else {
                uint32_t hi = out->stub_addr + out->stub_size;
                if (addr + size > hi) hi = addr + size;
                if (addr < out->stub_addr) {
                    out->stub_offset = off;
                    out->stub_addr = addr;
                }
                out->stub_size = hi - out->stub_addr;
            }
        } else if (avail > 21 && strncmp(name, ".rodata.sceModuleInfo", 22) == 0) {
            out->modinfo_addr = addr;
            out->modinfo_offset = off;
            out->modinfo_size = size;
        }
    }
    return 0;
}

/* ---- PSP module info ----------------------------------------------------- */

/* Layout, confirmed against a real module rather than taken from a header:
 *
 *   0x00  u16   attribute
 *   0x02  u8[2] version (minor, major)
 *   0x04  char  name[28]
 *   0x20  u32   gp_value
 *   0x24  u32   ent_top      exports
 *   0x28  u32   ent_end
 *   0x2C  u32   stub_top     imports
 *   0x30  u32   stub_end
 *                            = 0x34 bytes
 *
 * The widely-circulated struct has `attribute` as a u32 and `name` at 0x06,
 * which shifts every field by two and yields a plausible-looking but wrong
 * parse. The size settles it: `.rodata.sceModuleInfo` is exactly 52 (0x34)
 * bytes, which only the layout above produces. Reading is byte-wise because
 * the struct is packed and the u32s land on odd alignments. */
int psp_modinfo_parse(const uint8_t *d, size_t len, uint32_t file_offset,
                      psp_module_info *out) {
    if ((size_t)file_offset + 0x34 > len) return -1;
    const uint8_t *m = d + file_offset;

    memset(out, 0, sizeof *out);
    out->attribute  = rd16(m + 0x00);
    out->version[0] = m[0x02];
    out->version[1] = m[0x03];
    memcpy(out->name, m + 0x04, 28);
    out->name[28]   = '\0';
    out->gp_value   = rd32(m + 0x20);
    out->ent_top    = rd32(m + 0x24);
    out->ent_end    = rd32(m + 0x28);
    out->stub_top   = rd32(m + 0x2C);
    out->stub_end   = rd32(m + 0x30);
    return 0;
}

int psp_collect_imports(const uint8_t *d, size_t len,
                        const psp_module_info *mi, uint32_t load_bias,
                        psp_import_entry *out, int max) {
    if (mi->stub_end <= mi->stub_top) return 0;

    int found = 0;

    /* Each library the module imports from gets one PspLibStub entry:
     *
     *   0x00 u32  name        pointer to the library's name string
     *   0x04 u16  version
     *   0x06 u16  attribute
     *   0x08 u8   entLen      entry size in words (5 -> 20 bytes)
     *   0x09 u8   varCount
     *   0x0A u16  funcCount
     *   0x0C u32  nidTable    funcCount NIDs
     *   0x10 u32  stubTable   funcCount thunks, 8 bytes each
     *
     * entLen is honoured rather than assumed: some modules use a longer entry,
     * and striding by a fixed 20 bytes walks off into the middle of the next
     * one and produces convincing nonsense. */
    uint32_t e = mi->stub_top;
    while (e + 0x14 <= mi->stub_end) {
        size_t off = (uint32_t)(e + load_bias);
        if (off + 0x14 > len) break;

        uint32_t name_ptr  = rd32(d + off + 0x00);
        uint8_t  ent_len   = d[off + 0x08];
        uint16_t func_cnt  = rd16(d + off + 0x0A);
        uint32_t nid_table = rd32(d + off + 0x0C);
        uint32_t stub_tbl  = rd32(d + off + 0x10);

        uint32_t stride = ent_len ? (uint32_t)ent_len * 4 : 0x14;

        /* Library name, bounded so a bogus pointer cannot run off the buffer. */
        char lib[32];
        lib[0] = '\0';
        size_t np = (uint32_t)(name_ptr + load_bias);
        if (name_ptr && np < len) {
            size_t n = 0;
            while (n < sizeof lib - 1 && np + n < len && d[np + n]) {
                lib[n] = (char)d[np + n];
                n++;
            }
            lib[n] = '\0';
        }

        size_t nids  = (uint32_t)(nid_table + load_bias);
        size_t stubs = (uint32_t)(stub_tbl + load_bias);
        for (uint16_t i = 0; i < func_cnt; i++) {
            if (nids + (size_t)i * 4 + 4 > len) break;
            if (found < max) {
                psp_import_entry *it = &out[found];
                it->nid  = rd32(d + nids + (size_t)i * 4);
                it->addr = stub_tbl + (uint32_t)i * 8;   /* 8 bytes per thunk */
                snprintf(it->lib, sizeof it->lib, "%s", lib[0] ? lib : "unknown");
            }
            found++;
        }
        (void)stubs;

        if (stride < 0x14) break;                 /* malformed; do not loop */
        e += stride;
    }
    return found;
}

/* Is `target` plausibly the entry of a function?
 *
 * Inside `.text` the answer is simply yes -- that is what the section is for.
 * Outside it, the bar has to be higher, because most words in `.data` are not
 * code and a relocation pointing at one is usually just a data pointer.
 *
 * The test is that several consecutive words all decode as valid Allegrex
 * instructions. A first attempt used the standard prologue
 * `addiu $sp, $sp, -N` instead, which found only four of this module's eight
 * static initialisers: the others are *leaf* functions that never touch the
 * stack, so they begin with ordinary arithmetic and the prologue test rejected
 * them. Requiring a run of decodable instructions covers both, and is still
 * strict enough that arbitrary data rarely passes -- especially given the
 * candidate already had to be a relocation target pointing into the image. */
#define CODE_PROBE 6

static int looks_like_code(const uint8_t *d, size_t len, const elf_info *e,
                           uint32_t load_bias, uint32_t target,
                           uint32_t text_lo, uint32_t text_hi) {
    if (target >= text_lo && target < text_hi) return 1;

    if (!e->nsegments) return 0;
    const uint32_t lo = e->seg[0].addr;
    const uint32_t hi = lo + e->seg[0].filesz;
    if (target < lo || target + CODE_PROBE * 4 > hi) return 0;

    size_t at = (uint32_t)(target + load_bias);
    if (at + CODE_PROBE * 4 > len) return 0;

    for (int i = 0; i < CODE_PROBE; i++) {
        a_insn in;
        uint32_t w = rd32(d + at + (size_t)i * 4);
        if (!a_decode(w, target + (uint32_t)i * 4, &in) || in.op == A_INVALID)
            return 0;
    }
    return 1;
}

int psp_collect_pointer_seeds(const uint8_t *d, size_t len, const elf_info *e,
                              uint32_t load_bias, uint32_t *out, int max) {
    if (!e->shoff || !e->shnum || !e->text_size) return 0;

    const uint32_t text_lo = e->text_addr;
    const uint32_t text_hi = e->text_addr + e->text_size;
    int found = 0;

    for (uint32_t i = 0; i < e->shnum; i++) {
        size_t sh = (size_t)e->shoff + (size_t)i * e->shentsize;
        if (sh + 40 > len) break;

        uint32_t type = rd32(d + sh + 4);
        /* A PSP PRX tags its relocation sections SHT_PRXRELOC (0x700000A0)
         * rather than the generic SHT_REL (9). The entry layout is identical —
         * only the section type differs — so checking for SHT_REL alone finds
         * nothing at all on a real module. */
        if (type != 9 && type != 0x700000A0u) continue;

        uint32_t off  = rd32(d + sh + 16);
        uint32_t size = rd32(d + sh + 20);
        if ((size_t)off + size > len) continue;

        /* Elf32_Rel is two words: r_offset, r_info. */
        for (uint32_t r = 0; r + 8 <= size; r += 8) {
            uint32_t r_offset = rd32(d + off + r);
            uint32_t r_info   = rd32(d + off + r + 4);

            /* HI16/LO16 pairs are how an address gets *computed in code*
             * rather than stored in a word:
             *
             *     lui   $a0, %hi(target)
             *     addiu $a0, $a0, %lo(target)
             *
             * That is exactly how a PSP module passes its main thread's entry
             * point to sceKernelCreateThread -- so without these, the entire
             * game beyond module_start is unreachable. The relocations come in
             * pairs, and the address is (hi << 16) + (int16)lo. The LO16 half
             * is signed, which is why it cannot simply be OR'd in: a low half
             * of 0x8000 or above borrows from the high half. */
            if ((r_info & 0xFF) == R_MIPS_HI16 && r + 16 <= size) {
                uint32_t lo_info = rd32(d + off + r + 12);
                if ((lo_info & 0xFF) == R_MIPS_LO16) {
                    uint32_t lo_offset = rd32(d + off + r + 8);
                    size_t hi_at = (uint32_t)(r_offset + load_bias);
                    size_t lo_at = (uint32_t)(lo_offset + load_bias);
                    if (hi_at + 4 <= len && lo_at + 4 <= len) {
                        uint32_t hi_imm = rd32(d + hi_at) & 0xFFFF;
                        int32_t  lo_imm = (int16_t)(rd32(d + lo_at) & 0xFFFF);
                        uint32_t target = (hi_imm << 16) + (uint32_t)lo_imm;
                        if (target >= text_lo && target < text_hi && !(target & 3)) {
                            if (found < max) out[found] = target;
                            found++;
                        }
                    }
                }
                continue;
            }

            if ((r_info & 0xFF) != R_MIPS_32) continue;

            /* The relocated word holds the address. A PRX links at zero, so
             * the stored value is already the module-relative address and
             * needs no fixing up — only reading. */
            size_t at = (uint32_t)(r_offset + load_bias);
            if (at + 4 > len) continue;

            uint32_t target = rd32(d + at);
            if (target & 3) continue;            /* not instruction-aligned */
            if (!looks_like_code(d, len, e, load_bias, target, text_lo, text_hi))
                continue;

            if (found < max) out[found] = target;
            found++;
        }
    }
    return found;
}

int psp_collect_exports(const uint8_t *d, size_t len,
                        const psp_module_info *mi, uint32_t load_bias,
                        uint32_t *out, int max) {
    if (mi->ent_end <= mi->ent_top) return 0;

    int found = 0;
    /* Each entry is 0x10 bytes and describes one exported library. */
    uint32_t e = mi->ent_top;
    while (e + 0x10 <= mi->ent_end) {
        size_t off = (uint32_t)(e + load_bias);
        if (off + 0x10 > len) break;

        /* The export entry has the same shape as an import entry:
         *
         *   0x08 u8   entLen     entry size in words
         *   0x09 u8   varCount
         *   0x0A u16  funcCount
         *   0x0C u32  table
         *
         * An earlier version read nfunc from 0x08 and nvar from 0x09 -- that
         * is entLen and varCount, so both counts were wrong. On this module it
         * gave 4 functions and 3 variables instead of 2 and 3, which made the
         * address half of the table start four entries early: the "function
         * addresses" came out as data pointers and, past the end of the table,
         * as NIDs belonging to the import table that follows it.
         *
         * With the correct counts the exports read exactly as they should:
         * module_start -> 0x000183AC (matching e_entry), module_stop ->
         * 0x000186D8, module_info -> 0x00092558 (the address of the
         * .rodata.sceModuleInfo section). Three independent confirmations. */
        uint8_t  ent_len = d[off + 0x08];
        uint8_t  nvar    = d[off + 0x09];
        uint16_t nfunc   = rd16(d + off + 0x0A);
        uint32_t table   = rd32(d + off + 0x0C);

        /* The table is (nfunc+nvar) NIDs followed by (nfunc+nvar) addresses.
         * We want the function addresses: the first `nfunc` of the second
         * half. Variables are data and are deliberately skipped. */
        uint32_t total = (uint32_t)nfunc + nvar;
        size_t addrs = (size_t)(uint32_t)(table + load_bias) + (size_t)total * 4;
        if (addrs + (size_t)nfunc * 4 > len) continue;

        for (uint16_t i = 0; i < nfunc; i++) {
            uint32_t fn = rd32(d + addrs + (size_t)i * 4);
            if (found < max) out[found] = fn;
            found++;
        }

        /* Honour entLen for the stride rather than assuming 16 bytes. */
        uint32_t stride = ent_len ? (uint32_t)ent_len * 4 : 0x10;
        if (stride < 0x10) break;
        e += stride;
    }
    return found;
}
