/* allegrexrecomp — the container stack between a disc dump and MIPS code.
 *
 * Getting from a UMD/PSN dump to something you can decode means peeling four
 * layers, and a real game uses all of them:
 *
 *   .iso            ISO9660, 2048-byte sectors        -> PSP_GAME/SYSDIR/EBOOT.BIN
 *   PBP             plain 8-segment container         -> DATA.PSP (a PRX)
 *   ~PSP            encrypted PRX wrapper (KIRK)      -> an ELF, once decrypted
 *   ~SCE            simple 0x40-byte wrapper          -> a ~PSP or an ELF
 *   ELF/PRX         ELF32 MIPS LE, e_type 0xFFA0      -> the code we decode
 *
 * Only the ~PSP layer needs crypto. Everything else here is plain parsing, and
 * plenty of real content — including WTF's game-sharing microgames — is PBP all
 * the way down to an unencrypted PRX, which is why bring-up can start before
 * the decryptor exists. See docs/CONTAINERS.md.
 */
#ifndef ALLEGREX_CONTAINER_H
#define ALLEGREX_CONTAINER_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FMT_UNKNOWN = 0,
    FMT_ISO9660,   /* a UMD or PSN disc image */
    FMT_PBP,       /* \0PBP container */
    FMT_PSP,       /* ~PSP encrypted PRX */
    FMT_SCE,       /* ~SCE wrapper */
    FMT_ELF,       /* plain ELF32 (incl. PRX, e_type 0xFFA0) */
} psp_format;

/* Sniff a buffer's leading bytes. `len` must be at least 16. */
psp_format psp_sniff(const uint8_t *data, size_t len);
const char *psp_format_name(psp_format f);

/* ---- a whole file slurped into memory ------------------------------------ */

typedef struct {
    uint8_t *data;
    size_t   size;
} psp_blob;

int  psp_blob_read(const char *path, psp_blob *out);
void psp_blob_free(psp_blob *b);

/* ---- ISO9660 ------------------------------------------------------------- */

#define ISO_SECTOR 2048

typedef struct {
    char     path[256];   /* "/PSP_GAME/SYSDIR/EBOOT.BIN" */
    uint32_t lba;
    uint32_t size;
    int      is_dir;
} iso_entry;

/* Walk the directory tree. Returns the number of entries written to `out`,
 * or -1 on error. Pass out=NULL to just count. */
int iso_list(FILE *f, iso_entry *out, int max_entries);

/* Copy one entry's extent out of the image. */
int iso_extract(FILE *f, const iso_entry *e, const char *dst_path);

/* ---- PARAM.SFO ----------------------------------------------------------- */

/* Print every key/value in a PARAM.SFO blob. This is where DISC_ID, TITLE and
 * PSP_SYSTEM_VER live — the identity a game repo records in its NOTES.md. */
int sfo_dump(const uint8_t *data, size_t len, FILE *out);

/* ---- PBP ----------------------------------------------------------------- */

#define PBP_NUM_SEGMENTS 8

extern const char *const PBP_SEGMENT_NAMES[PBP_NUM_SEGMENTS];

typedef struct {
    uint32_t version;
    uint32_t offset[PBP_NUM_SEGMENTS];
    uint32_t size[PBP_NUM_SEGMENTS];   /* derived from adjacent offsets */
} pbp_info;

int pbp_parse(const uint8_t *data, size_t len, pbp_info *out);

/* ---- ~PSP header --------------------------------------------------------- */

#define PSP_HEADER_SIZE 0x150

typedef struct {
    char     modname[29];
    uint16_t mod_attr;
    uint16_t comp_attr;
    uint8_t  mod_ver_lo, mod_ver_hi;
    uint8_t  nsegments;
    uint32_t elf_size;      /* size of the decrypted ELF */
    uint32_t psp_size;      /* size of this ~PSP file */
    uint32_t boot_entry;
    uint32_t modinfo_offset;
    uint32_t bss_size;
    uint32_t seg_address[4];
    uint32_t seg_size[4];
    uint32_t devkit_version;
    uint8_t  decrypt_mode;
    uint32_t tag;           /* selects the KIRK key set */
} psp_header;

int psp_header_parse(const uint8_t *data, size_t len, psp_header *out);

/* ---- ELF / PRX ----------------------------------------------------------- */

#define ET_PSP_PRX 0xFFA0

typedef struct {
    uint32_t addr;
    uint32_t offset;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
} elf_segment;

typedef struct {
    uint16_t type;          /* ET_EXEC (2) or ET_PSP_PRX */
    uint32_t entry;
    int      nsegments;
    elf_segment seg[8];

    /* The extent we hand to the decoder.
     *
     * A PSP PRX typically has ONE PT_LOAD marked rwx covering the whole file —
     * code and data together. Decoding all of it treats megabytes of data as
     * instructions, which inflates every coverage number and buries the real
     * code. When section headers survive (they usually do) `.text` gives the
     * true code extent, which for a real module is a small fraction of the
     * segment. We prefer sections and fall back to the segment. */
    uint32_t text_addr, text_offset, text_size;
    int      text_from_section;   /* 1 if .text, 0 if we fell back to PT_LOAD */

    /* Import thunks — one small stub per firmware function the module calls.
     * Calls that land here are HLE boundaries, not internal functions. */
    uint32_t stub_addr, stub_offset, stub_size;

    /* .rodata.sceModuleInfo — exports, imports and $gp. */
    uint32_t modinfo_addr, modinfo_offset, modinfo_size;

    int      nsections;
    /* Kept so the relocation tables can be re-walked without re-deriving
     * where the section header table is. */
    uint32_t shoff, shentsize, shnum;
} elf_info;

/* MIPS relocation types we care about. `r_info`'s low byte is the type; on a
 * PRX the upper bytes carry segment indices rather than a symbol index. */
#define R_MIPS_32    2
#define R_MIPS_26    4
#define R_MIPS_HI16  5
#define R_MIPS_LO16  6

/* Harvest candidate function pointers from the relocation tables.
 *
 * `jal` reaches functions that are *called* directly, but a PSP module also
 * reaches code through stored pointers: thread entry points, callbacks,
 * vtables, and jump tables. Those are invisible to any control-flow scan —
 * and they are exactly what the loader has to patch, so the relocation tables
 * enumerate them precisely.
 *
 * An R_MIPS_32 relocation names a word that holds an address. If that address
 * lands in `.text` and is instruction-aligned, it is a function pointer. This
 * is enumeration, not guesswork: the linker recorded it because it *is* an
 * address.
 *
 * Returns the number found (may exceed `max`, in which case only `max` were
 * written). */
int psp_collect_pointer_seeds(const uint8_t *data, size_t len, const elf_info *e,
                              uint32_t load_bias, uint32_t *out, int max);

int elf_parse(const uint8_t *data, size_t len, elf_info *out);

/* ---- PSP module info ------------------------------------------------------
 * The PRX descriptor: where the export and import tables live, and the $gp
 * value the module's code assumes. Exported function addresses are the single
 * best seed set for function discovery after the entry point. */

typedef struct {
    uint32_t attribute;
    uint8_t  version[2];
    char     name[29];
    uint32_t gp_value;
    uint32_t ent_top, ent_end;     /* export table */
    uint32_t stub_top, stub_end;   /* import table */
} psp_module_info;

int psp_modinfo_parse(const uint8_t *data, size_t len, uint32_t file_offset,
                      psp_module_info *out);

/* Collect exported function addresses into `out`, returning how many were
 * found (which may exceed `max`, in which case only `max` were written).
 * `load_bias` converts a module virtual address to a file offset. */
int psp_collect_exports(const uint8_t *data, size_t len,
                        const psp_module_info *mi, uint32_t load_bias,
                        uint32_t *out, int max);

/* ---- imports --------------------------------------------------------------
 * The import table names every firmware function a module calls: which
 * library it comes from, its NID (a hash of the function name), and the
 * address of the thunk that calls it.
 *
 * This is what turns `psp_import_089F0488()` into
 * `psp_import_sceCtrl_1F4011E6()` in the generated C, and — more to the point
 * — it *is* the HLE work list. Everything in this table has to exist before a
 * game can run, and nothing outside it does. */

typedef struct {
    uint32_t addr;      /* the thunk address, in .sceStub.text */
    uint32_t nid;       /* hash of the function name */
    char     lib[32];   /* e.g. "sceCtrl", "sceGe_user", "Kernel_Library" */
} psp_import_entry;

int psp_collect_imports(const uint8_t *data, size_t len,
                        const psp_module_info *mi, uint32_t load_bias,
                        psp_import_entry *out, int max);

#ifdef __cplusplus
}
#endif

#endif /* ALLEGREX_CONTAINER_H */
