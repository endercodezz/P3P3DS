/* psprecomp — static constructors.
 *
 * A C++ module registers its global constructors in a null-terminated array of
 * function pointers, and something must walk that array before the program
 * uses any global object. On PSP that walker lives in crt0, which a PRX gets
 * from the *loader* rather than from its own image — so a recompiled module
 * contains the constructors and the table, and nothing that calls them.
 *
 * Nothing calls them, and no diagnostic fires. Global objects simply stay as
 * `.bss` left them: linked-list heads that should be self-referential read as
 * index 0, which is a valid index, so the first walk over one loops forever.
 * Vtable slots read as zero and virtual calls dispatch to address 0. In
 * Lumberjack this presented as a ten-billion-iteration spin four call levels
 * away from the actual cause, and as seventeen "uninitialised subsystems"
 * that were never going to initialise.
 *
 * Finding the table is the interesting part. There are no section headers to
 * rely on after decryption, and vtables look almost identical -- both are runs
 * of code pointers in read-only data.
 *
 * A discriminator that seems obvious does NOT work: "constructor entries have
 * no callers". Vtable entries have no callers either -- they are reached
 * through a vptr, never by name -- so the test accepts 1327 candidates in
 * Lumberjack, which is useless. Measured, not assumed.
 *
 * What does separate them, in decreasing order of reliability:
 *
 *   - Null termination. Vtables are generally followed by more data.
 *   - Entry locality: a constructor table's targets cluster in one region
 *     that holds little else, because the compiler emits them together.
 *     Lumberjack's eight all fall in 0x3B2AF8..0x3B2FCC, above .text.
 *   - Arity: constructors take only `this`, so their first use of $a1-$a3
 *     without a preceding write is a strong negative.
 *
 * This implementation uses null termination alone and returns the single
 * longest candidate, which found the right table here but is not sufficient in
 * general -- and returning only the best would hide a second, shorter array.
 * Enumerating candidates and scoring them on locality is the improvement worth
 * making before this is trusted on another title. */

#include "psprecomp/ctors.h"
#include "psprecomp/mem.h"
#include "psprecomp/dispatch.h"
#include "psprecomp/cpu.h"

#include <stdio.h>

int psp_ctors_find(uint32_t lo, uint32_t hi, uint32_t code_lo, uint32_t code_hi,
                   uint32_t *out_addr, int *out_count) {
    uint32_t best_addr = 0;
    int best_count = 0;

    for (uint32_t a = lo; a + 4 < hi; a += 4) {
        int n = 0;
        uint32_t p = a;
        for (;; p += 4) {
            uint32_t v = psp_read32(p);
            if (v < code_lo || v >= code_hi || (v & 3)) break;
            n++;
            if (n > 4096) break;                  /* not a constructor table */
        }
        /* Null-terminated, and long enough not to be a coincidence. */
        if (n >= 2 && psp_read32(p) == 0 && n > best_count) {
            best_count = n;
            best_addr = a;
        }
        if (n) a = p - 4;                          /* skip what was scanned */
    }

    if (!best_count) return -1;
    if (out_addr)  *out_addr = best_addr;
    if (out_count) *out_count = best_count;
    return 0;
}

int psp_ctors_run(uint32_t table) {
    int n = 0;
    for (uint32_t p = table; ; p += 4) {
        uint32_t fn = psp_read32(p);
        if (!fn) break;
        /* Each constructor is an ordinary call with no arguments. $ra is set
         * to zero so a stale value cannot be mistaken for a return address if
         * one of them stores it. */
        psp_cpu.r[PSP_REG_RA] = 0;
        psp_dispatch(fn);
        n++;
        if (n > 4096) break;
    }
    return n;
}
