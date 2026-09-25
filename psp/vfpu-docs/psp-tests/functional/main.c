
// PSP VFPU test suite
// Created by David Guillen Fandos <david@davidgf.net>

#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspfpu.h>
#include <psppower.h>
#include <kubridge.h>

#include <math.h>
#include <stdio.h>

#include "test-utils.h"

#define MAX_TESTS  512

PSP_MODULE_INFO("PSP VFPU functional suite", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

struct test_error_info err_info[MAX_TESTS] = {0};
struct prefix_usage_info pfxinfo[MAX_TESTS] = {0};

int run_tests(struct test_error_info *, struct prefix_usage_info *);

void print_error_info(const struct test_error_info *einfo) {
	logprintf("Instruction test error for %s [%08x / %08x]\n", einfo->inst, einfo->instw, einfo->pfxw);
	for (unsigned j = 0; j < 2 && einfo->inputs[j].iname; j++) {
		logprintf("%s:", einfo->inputs[j].iname);
		for (unsigned l = 0; l < einfo->inputs[j].size; l++) {
			logprintf(" %08x", einfo->inputs[j].values[l]);
		}
		logprintf("\n");
	}
	logprintf("rd:");
	for (unsigned j = 0; j < einfo->rdsz; j++) {
		logprintf(" %08x", einfo->rd[j]);
	}
	logprintf(" expected:");
	for (unsigned j = 0; j < einfo->rdsz; j++) {
		logprintf(" %08x", einfo->rdex[j]);
	}
	logprintf("\n");
}

#define check_pfx_elem(rsname, pfxtype)                         \
  if (pfxinfo[i].rsname##_errors & (1 << enum_##pfxtype)) {     \
    logprintf("*** ");                                          \
  } else {                                                      \
    logprintf(#pfxtype " ");                                    \
  }

#define print_pfx_elem(rsname, pfxtype)                         \
  if (pfxinfo[i].rsname##_errors & (1 << enum_##pfxtype))       \
    print_error_info(&pfxinfo[i].rsname##_##pfxtype##_err);     \

#define check_src_pfx(rsname)                                   \
  if (pfxinfo[i].rsname##_tested) {                             \
    logprintf(#rsname " ");                                     \
    check_pfx_elem(rsname, swz);                                \
    check_pfx_elem(rsname, neg);                                \
    check_pfx_elem(rsname, abs);                                \
    check_pfx_elem(rsname, cst);                                \
  }

#define check_dst_pfx(rsname)                                   \
  if (pfxinfo[i].rsname##_tested) {                             \
    logprintf(#rsname " ");                                     \
    check_pfx_elem(rsname, msk);                                \
    check_pfx_elem(rsname, sat);                                \
  }

int main() {
	// Initialize clock and logging facilities
	scePowerSetClockFrequency(333, 333, 166);
	#ifdef _BUILD_EBOOT_
	pspDebugScreenInit();
	logfd = fopen("ms0://test_functional_results.txt", "wb");
	#endif

	logprintf(">>>>>>> VFPU automated functional tests <<<<<<\n");
	logprintf("PSP model: %d\n", kuKernelGetModel());

	int print_pfx_err = 0;

	// Disable division exceptions to compare easily
	pspFpuSetEnable(0);

	int nerrs = run_tests(&err_info[0], &pfxinfo[0]);

	// Inspect test results and inform the user about the results
	logprintf("======= Instruction validation reports ======\n");
	if (!nerrs)
		logprintf(" All tests passed OK!\n");
	for (int i = 0; i < nerrs; i++)
		print_error_info(&err_info[i]);

	logprintf("======= Instruction prefix validity matrix ======\n");
	for (unsigned i = 0; i < MAX_TESTS; i++) {
		if (pfxinfo[i].inst_name) {
			logprintf("%8s: ", pfxinfo[i].inst_name);
			check_src_pfx(rs);
			check_src_pfx(rt);
			check_dst_pfx(rd);
			logprintf("\n");
		}
	}

	// Provides more details (a failed case essentially) for an invalid prefix
	// It is useful to understand why the hardware is unable to use a certain
	// prefix (in some cases you can see how some stuff works but some other
	// doesn't due to how the interal wiring is done).
	if (print_pfx_err) {
		logprintf("======= Instruction prefix validity reports ======\n");
		for (unsigned i = 0; i < MAX_TESTS; i++) {
			if (pfxinfo[i].inst_name) {
				if (pfxinfo[i].rs_tested) {
					print_pfx_elem(rs, swz);
					print_pfx_elem(rs, neg);
					print_pfx_elem(rs, abs);
					print_pfx_elem(rs, cst);
				}
				if (pfxinfo[i].rt_tested) {
					print_pfx_elem(rt, swz);
					print_pfx_elem(rt, neg);
					print_pfx_elem(rt, abs);
					print_pfx_elem(rt, cst);
				}
				print_pfx_elem(rd, msk);
				print_pfx_elem(rd, sat);
			}
		}
	}

	logprintf("ALL tests completed!\n");

	#ifdef _BUILD_EBOOT_
	fclose(logfd);
	#endif

	sceKernelExitGame();
}


