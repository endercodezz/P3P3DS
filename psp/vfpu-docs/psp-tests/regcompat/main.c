
// PSP VFPU reg compatibility tests
// Created by David Guillen Fandos <david@davidgf.net>

#include <pspkernel.h>
#include <psppower.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <kubridge.h>

#include <math.h>
#include <stdio.h>

#include "test-utils.h"

#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX_TESTS  512

PSP_MODULE_INFO("PSP VFPU test suite", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

struct register_compat_info regcmpinfo[MAX_TESTS] = {0};
void run_reg_sanity_tests();
int run_reg_tests(struct register_compat_info *reginfo);

int main() {
	scePowerSetClockFrequency(333, 333, 166);
	int print_regcol_err = 0;
	#ifdef _BUILD_EBOOT_
	pspDebugScreenInit();
	logfd = fopen("ms0://test_regcompat_results.txt", "wb");
	#endif

	logprintf(">>>>>>> Instruction register compatibility tests <<<<<<\n");
	logprintf("PSP model: %d\n", kuKernelGetModel());

	run_reg_sanity_tests();

	int regtstcnt = run_reg_tests(&regcmpinfo[0]);

	logprintf("======= Instruction register compatibility matrix ======\n");
	for (unsigned i = 0; i < regtstcnt; i++) {
		if (regcmpinfo[i].inst_name) {
			unsigned cerr = regcmpinfo[i].compat_overlap_errors;
			unsigned ierr = regcmpinfo[i].incompat_overlap_errors;
			const char *sanity = regcmpinfo[i].nonoverlap_errors ? "[ERR!]" : "[ OK ]";
			const char *compat = (!cerr && !ierr) ? "[ALL-REGS]" :
			                     (!cerr)          ? "[COMPAT-R]" : "[NO-OVERL]";

			logprintf("%-10s: %s %s\n", regcmpinfo[i].inst_name, sanity, compat);
		}
	}

	if (print_regcol_err) {
		logprintf("======= Instruction register compatibility report ======\n");
		for (unsigned i = 0; i < regtstcnt; i++) {
			if (regcmpinfo[i].inst_name) {
				logprintf("%s: %d %d\n", regcmpinfo[i].inst_name, regcmpinfo[i].test_cnt, regcmpinfo[i].test_errs);
				for (unsigned j = 0; j < MIN(regcmpinfo[i].test_errs, REG_PAIRS_DEBUGS); j++)
					logprintf("Reginfo [%d] %d %d %d\n", j,
						regcmpinfo[i].reg_pairs[j][0],
						regcmpinfo[i].reg_pairs[j][1],
						regcmpinfo[i].reg_pairs[j][2]);
			}
		}
	}

	logprintf("Tests done!\n");

	#ifdef _BUILD_EBOOT_
	fclose(logfd);
	#endif
	sceKernelExitGame();
}


