/* psprecomp — Allegrex CPU state. See include/psprecomp/cpu.h. */

#include "psprecomp/cpu.h"

#include <string.h>

psp_cpu_state psp_cpu;

const char *const psp_reg_names[PSP_NUM_GPR] = {
    "zero","at","v0","v1","a0","a1","a2","a3",
    "t0","t1","t2","t3","t4","t5","t6","t7",
    "s0","s1","s2","s3","s4","s5","s6","s7",
    "t8","t9","k0","k1","gp","sp","fp","ra"
};

void psp_cpu_reset(void) {
    memset(&psp_cpu, 0, sizeof psp_cpu);
    /* $sp is set by the loader from the module's stack allocation, not here —
     * a reset CPU with a null stack is the correct starting point, and a game
     * that faults on a null $sp is telling us the loader did not run. */
}
