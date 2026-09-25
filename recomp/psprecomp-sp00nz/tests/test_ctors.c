/* Static-constructor discovery and execution.
 *
 * The table is built by hand here rather than taken from a module: the point
 * is to pin the shape the search accepts and, more importantly, the shapes it
 * must reject. A vtable is a run of code pointers too, and mistaking one for a
 * constructor table would call arbitrary methods with no arguments during
 * start-up — a failure that would look like anything except its cause.
 */

#include "psprecomp/ctors.h"
#include "psprecomp/mem.h"
#include "psprecomp/cpu.h"
#include "psprecomp/dispatch.h"

#include <stdio.h>

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

#define DATA     0x08900000u
#define CODE_LO  0x08000000u
#define CODE_HI  0x08100000u

static int g_ran;
static void ctor_a(void) { g_ran |= 1; }
static void ctor_b(void) { g_ran |= 2; }
static void ctor_c(void) { g_ran |= 4; }

static void write_table(uint32_t at, const uint32_t *v, int n) {
    for (int i = 0; i < n; i++) psp_write32(at + (uint32_t)i * 4, v[i]);
    psp_write32(at + (uint32_t)n * 4, 0);        /* terminator */
}

static void test_find(void) {
    const uint32_t t[] = { 0x08001000u, 0x08002000u, 0x08003000u };
    write_table(DATA, t, 3);

    uint32_t addr = 0;
    int count = 0;
    CHECK(psp_ctors_find(DATA, DATA + 0x100, CODE_LO, CODE_HI, &addr, &count) == 0,
          "a null-terminated run of code pointers should be found");
    CHECK(addr == DATA, "table address: 0x%08X", addr);
    CHECK(count == 3, "entry count: %d", count);
}

/* A run that is not null-terminated is not a constructor table. Vtables are
 * usually followed by more data, and accepting one would be worse than
 * finding nothing. */
static void test_rejects_unterminated(void) {
    const uint32_t t[] = { 0x08001000u, 0x08002000u };
    write_table(DATA + 0x200, t, 2);
    psp_write32(DATA + 0x208, 0xDEADBEEFu);      /* overwrite the terminator */

    uint32_t addr = 0;
    int count = 0;
    CHECK(psp_ctors_find(DATA + 0x200, DATA + 0x210, CODE_LO, CODE_HI,
                         &addr, &count) != 0,
          "an unterminated run must not be accepted");
}

/* One pointer followed by zero is far too weak a signal — it occurs all over
 * ordinary data. */
static void test_rejects_single(void) {
    psp_write32(DATA + 0x300, 0x08001000u);
    psp_write32(DATA + 0x304, 0);

    uint32_t addr = 0;
    int count = 0;
    CHECK(psp_ctors_find(DATA + 0x300, DATA + 0x310, CODE_LO, CODE_HI,
                         &addr, &count) != 0,
          "a single pointer must not be accepted as a table");
}

/* Pointers outside the code range are data, however plausible they look. */
static void test_rejects_data_pointers(void) {
    const uint32_t t[] = { 0x08900000u, 0x08900100u, 0x08900200u };
    write_table(DATA + 0x400, t, 3);

    uint32_t addr = 0;
    int count = 0;
    CHECK(psp_ctors_find(DATA + 0x400, DATA + 0x420, CODE_LO, CODE_HI,
                         &addr, &count) != 0,
          "pointers outside the code range are not constructors");
}

static void test_run(void) {
    psp_dispatch_reset();
    psp_register(0x08001000u, ctor_a);
    psp_register(0x08002000u, ctor_b);
    psp_register(0x08003000u, ctor_c);

    const uint32_t t[] = { 0x08001000u, 0x08002000u, 0x08003000u };
    write_table(DATA + 0x500, t, 3);

    g_ran = 0;
    CHECK(psp_ctors_run(DATA + 0x500) == 3, "should run three constructors");
    CHECK(g_ran == 7, "all three should have run, got mask %d", g_ran);
}

/* An empty table is legitimate: a module with no global constructors. */
static void test_run_empty(void) {
    psp_write32(DATA + 0x600, 0);
    CHECK(psp_ctors_run(DATA + 0x600) == 0, "an empty table runs nothing");
}

int main(void) {
    if (psp_mem_init() != 0) { printf("memory init failed\n"); return 1; }
    psp_cpu_reset();

    test_find();
    test_rejects_unterminated();
    test_rejects_single();
    test_rejects_data_pointers();
    test_run();
    test_run_empty();

    psp_mem_free();
    printf(failures ? "ctors: %d failure(s)\n" : "ctors: all tests passed\n", failures);
    return failures ? 1 : 0;
}
