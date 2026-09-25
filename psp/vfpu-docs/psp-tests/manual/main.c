
// PSP VFPU manual tests
// Created by David Guillen Fandos <david@davidgf.net>

#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspfpu.h>
#include <psppower.h>
#include <kubridge.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "test-utils.h"
#include "exception-module/exception_info.h"

#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX_TESTS  512

PSP_MODULE_INFO("PSP VFPU test suite", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

struct test_error_info err_info[MAX_TESTS] = {0};
struct check_error_info check_info[MAX_TESTS] = {0};

int run_tests(struct test_error_info *, struct prefix_usage_info *);
int check_nans(struct test_error_info *);
int check_f16(struct test_error_info *errlist);
int check_vidt(struct test_error_info *errlist);
int check_rounding(struct test_error_info *errlist);
int check_conditionals(struct test_error_info *);
int validate_ccregs(struct check_error_info *);
int assert_vhtfm_bugs(struct test_error_info *);
int check_unaligned_vfpu_load_bug(struct check_error_info *errs, int is_fat);
int check_unaligned_vfpu_memops(struct check_error_info *errs);
int check_aligned_vfpu_memops_exception(struct check_error_info *errs,
                                        exception_control_block *ecb);
int check_allegrex_insts(struct check_error_info *errs, exception_control_block *ecb);
int check_allegrex_fpu(struct check_error_info *errs);
int check_delay_slot_ub(struct test_error_info *errlist);

unsigned read_vfpu_cc_reg() {
	unsigned ret;
	asm("mfvc %0, $135\n":"=r"(ret));
	return ret;
}

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

exception_control_block ctrl_block;
void *mod_args[1] = {
	&ctrl_block,
};

int main() {
	int pspmod = kuKernelGetModel();
	int isfat = pspmod == 0;

	// Initialize clock and logging facilities
	scePowerSetClockFrequency(333, 333, 166);
	#ifdef _BUILD_EBOOT_
	pspDebugScreenInit();
	logfd = fopen("ms0://test_manual_results.txt", "wb");
	#endif

	logprintf(">>>>>>> VFPU automated tests <<<<<<\n");
	logprintf("PSP model: %d\n", pspmod);
	logprintf("PSP VFPU_REV: %x\n", read_vfpu_cc_reg());

	// Disable division exceptions to compare easily
	pspFpuSetEnable(0);

	// Try to load the exception catche module
	exception_control_block *ecb = NULL;
	SceKernelLMOption opts;
	memset(&opts, 0, sizeof(opts));
	opts.size = sizeof(opts);
	opts.mpidtext = PSP_MEMORY_PARTITION_KERNEL;
	opts.mpiddata = PSP_MEMORY_PARTITION_KERNEL;
	opts.position = 0;
	opts.access = 1;
	int module_id = sceKernelLoadModule("exception_module.prx", 0, &opts);
	if (module_id < 0)
		module_id = sceKernelLoadModule("exception-module/exception_module.prx", 0, &opts);

	if (module_id < 0) {
		logprintf("[!!!] Could not load exception_module.prx kernel module. Some tests will be skipped!\n");
	} else {
		ecb = &ctrl_block;
		int ldstat = 0;
		sceKernelStartModule(module_id, sizeof(mod_args), mod_args, &ldstat, 0);
	}

	// Check MIPS allegrex instructions
	int checkcnt = check_allegrex_insts(check_info, ecb);

	// Run validation tests for CC
	checkcnt += validate_ccregs(&check_info[checkcnt]);
	// Validate regular functioning of VFPU unaligned instructions
	checkcnt += check_unaligned_vfpu_memops(&check_info[checkcnt]);
	// Validate exception throwing for VFPU aligned instructions
	checkcnt += check_aligned_vfpu_memops_exception(&check_info[checkcnt], ecb);
	// Validate memops silicon bugs on FAT models
	checkcnt += check_unaligned_vfpu_load_bug(&check_info[checkcnt], isfat);
	// Validate FPU instructions too
	checkcnt += check_allegrex_fpu(&check_info[checkcnt]);

	// Check comparison and branch instructions
	int nerrs = check_conditionals(&err_info[0]);
	// Undefined behaviour tests
	nerrs += check_delay_slot_ub(&err_info[nerrs]);
	// Run corner cases tests first (NaN, Inf, rounding...)
	nerrs += check_nans(&err_info[nerrs]);
	// Validate known bugs and check they are reproducible
	nerrs += assert_vhtfm_bugs(&err_info[nerrs]);

	// Run corner cases regarding f16 support
	nerrs += check_f16(&err_info[nerrs]);
	// Some manual tests for vidt instructions
	nerrs += check_vidt(&err_info[nerrs]);
	// Verify rounding behavior
	nerrs += check_rounding(&err_info[nerrs]);

	// Manual test reporting
	logprintf("======= Sanity check reports ======\n");
	if (!checkcnt)
		logprintf(" All tests passed OK!\n");
	for (int i = 0; i < checkcnt; i++) {
		logprintf("Reported err: %s\n", check_info[i].msg);
		logprintf("Value: %08x Expected: %08x\n", check_info[i].value, check_info[i].valueex);
	}

	// Inspect test results and inform the user about the results
	logprintf("======= Instruction validation reports ======\n");
	if (!nerrs)
		logprintf(" All tests passed OK!\n");
	for (int i = 0; i < nerrs; i++)
		print_error_info(&err_info[i]);

	logprintf("ALL tests completed!\n");

	#ifdef _BUILD_EBOOT_
	fclose(logfd);
	#endif

	sceKernelExitGame();
}


