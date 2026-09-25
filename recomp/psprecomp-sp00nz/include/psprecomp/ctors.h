/* psprecomp — static constructor discovery and execution. See src/ctors.c. */
#ifndef PSPRECOMP_CTORS_H
#define PSPRECOMP_CTORS_H

#include <stdint.h>

/* Search [lo, hi) for the longest null-terminated run of pointers into
 * [code_lo, code_hi). Returns 0 and fills the outputs on success, -1 if no
 * candidate was found. Candidates are proposals, not proof: vtables have the
 * same shape, and only the caller knows whether the entries have callers. */
int psp_ctors_find(uint32_t lo, uint32_t hi, uint32_t code_lo, uint32_t code_hi,
                   uint32_t *out_addr, int *out_count);

/* Call every entry in a null-terminated constructor table. Returns the number
 * run. Must happen before the module uses any global object. */
int psp_ctors_run(uint32_t table);

#endif