
// Written by David Guillen Fandos <david@davidgf.net>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <psppower.h>
#include <kubridge.h>

#include "test-utils.h"

PSP_MODULE_INFO("VFPU accuracy tests", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);

#define NUM_TESTS (16*1024*1024)   // This takes a while :P

// Some well-formed rand float number (with
inline unsigned int rndf32(int maxexp, bool allowneg) {
	unsigned mantissa = (rand() ^ (rand() << 8) ^ (rand() << 16)) & 0x7FFFFF;
	unsigned exponent = rand() % (128 + maxexp);
	unsigned ret = mantissa | (exponent << 23);

	if (allowneg && (rand() & 1))
		return ret | 0x80000000;
	return ret;
}

int main() {
	scePowerSetClockFrequency(333, 333, 166);

	#ifdef _BUILD_EBOOT_
	pspDebugScreenInit();
	logfd = fopen("ms0://test_accuracy_results.txt", "wb");
	#endif

	logprintf(">>>>>>> VFPU aprox instructions accuracy tests <<<<<<\n");
	logprintf("PSP model: %d\n", kuKernelGetModel());

	srand(0xdeadbeef);

	// Sin/Cos work well as long as input is < 2^32
	double max_cos_err = 0.0f;
	for (unsigned i = 0; i < NUM_TESTS; i++) {
		union {
			float f32;
			unsigned int u32;
		} inpn;
		inpn.u32 = rndf32(31, true);
		double expected = cos(M_PI * (double)inpn.f32 / 2.0f);
		asm (
			"mtv %1, S000\n"
			"vcos.s S000, S000\n"
			"mfv %0, S000\n"
		:"=r"(inpn.u32): "r"(inpn.u32));
		double calculated = inpn.f32;
		double abserr = fabs(calculated - expected);
		max_cos_err = abserr > max_cos_err ? abserr : max_cos_err;
	}
	logprintf("Max absolute cos() error: %.16e\n", max_cos_err);

	double max_sin_err = 0.0f;
	for (unsigned i = 0; i < NUM_TESTS; i++) {
		union {
			float f32;
			unsigned int u32;
		} inpn;
		inpn.u32 = rndf32(31, true);
		double expected = sin(M_PI * (double)inpn.f32 / 2.0f);
		asm (
			"mtv %1, S000\n"
			"vsin.s S000, S000\n"
			"mfv %0, S000\n"
		:"=r"(inpn.u32): "r"(inpn.u32));
		double calculated = inpn.f32;
		double abserr = fabs(calculated - expected);
		max_sin_err = abserr > max_sin_err ? abserr : max_sin_err;
	}
	logprintf("Max absolute sin() error: %.16e\n", max_sin_err);

	double max_sqrt_err = 0.0f;
	for (unsigned i = 0; i < NUM_TESTS; i++) {
		union {
			float f32;
			unsigned int u32;
		} inpn;
		inpn.u32 = rndf32(120, false);
		// Cannot divide by zero to calculate relative error
		// Use 1e-37, as smaller numbers will fall in subnormal territory
		// (VFPU considers denormals as "zero")
		if (inpn.f32 < 1e-37) continue;
		double expected = sqrt((double)inpn.f32);
		asm (
			"mtv %1, S000\n"
			"vsqrt.s S000, S000\n"
			"mfv %0, S000\n"
		:"=r"(inpn.u32): "r"(inpn.u32));
		double calculated = inpn.f32;
		double abserr = fabs(calculated - expected);
		if (expected != 0) {
			double relerr = abserr / expected;
			max_sqrt_err = relerr > max_sqrt_err ? relerr : max_sqrt_err;
		}
	}
	logprintf("Max relative sqrt() error: %.16e\n", max_sqrt_err);

	double max_rcp_err = 0.0f;
	for (unsigned i = 0; i < NUM_TESTS; i++) {
		union {
			float f32;
			unsigned int u32;
		} inpn;
		inpn.u32 = rndf32(120, true);
		// Same as above
		if (inpn.f32 < 1e-37) continue;
		double expected = (double)1.0f / (double)inpn.f32;
		asm (
			"mtv %1, S000\n"
			"vrcp.s S000, S000\n"
			"mfv %0, S000\n"
		:"=r"(inpn.u32): "r"(inpn.u32));
		double calculated = inpn.f32;
		double abserr = fabs(calculated - expected);
		if (expected != 0) {
			double relerr = abserr / expected;
			max_rcp_err = relerr > max_rcp_err ? relerr : max_rcp_err;
		}
	}
	logprintf("Max relative rcp() error: %.16e\n", max_rcp_err);

	double max_rsq_err = 0.0f;
	for (unsigned i = 0; i < NUM_TESTS; i++) {
		union {
			float f32;
			unsigned int u32;
		} inpn;
		inpn.u32 = rndf32(120, true);
		// Simiarly as above
		if (inpn.f32 < 1e-37) continue;
		double expected = (double)1.0f / sqrt((double)inpn.f32);
		asm (
			"mtv %1, S000\n"
			"vrsq.s S000, S000\n"
			"mfv %0, S000\n"
		:"=r"(inpn.u32): "r"(inpn.u32));
		double calculated = inpn.f32;
		double abserr = fabs(calculated - expected);
		if (expected != 0) {
			double relerr = abserr / expected;
			max_rsq_err = relerr > max_rsq_err ? relerr : max_rsq_err;
		}
	}
	logprintf("Max relative rsq() error: %.16e\n", max_rsq_err);

	double max_exp2_err = 0.0f;
	for (unsigned i = 0; i < NUM_TESTS; i++) {
		union {
			float f32;
			unsigned int u32;
		} inpn;
		// Exponents bigger than 2^7 (128) will result in overflow
		inpn.u32 = rndf32(6, true);
		// Anything below -127 can land in denormal territory which is zero.
		if (inpn.f32 < -126) continue;
		double oin = inpn.f32;
		double expected = exp2((double)inpn.f32);
		asm (
			"mtv %1, S000\n"
			"vexp2.s S000, S000\n"
			"mfv %0, S000\n"
		:"=r"(inpn.u32): "r"(inpn.u32));
		double calculated = inpn.f32;
		double abserr = fabs(calculated - expected);
		if (expected != 0) {
			double relerr = abserr / expected;
			max_exp2_err = relerr > max_exp2_err ? relerr : max_exp2_err;
			if (relerr > 0.9f)
			logprintf("%e %e %e (%e)\n", calculated, expected, relerr, oin);
		}
	}
	logprintf("Max relative exp2() error: %.16e\n", max_exp2_err);

	double max_log2_err = 0.0f;
	for (unsigned i = 0; i < NUM_TESTS; i++) {
		union {
			float f32;
			unsigned int u32;
		} inpn;
		inpn.u32 = rndf32(126, false);
		if (inpn.f32 < 1e-37) continue;
		double expected = log2((double)inpn.f32);
		asm (
			"mtv %1, S000\n"
			"vlog2.s S000, S000\n"
			"mfv %0, S000\n"
		:"=r"(inpn.u32): "r"(inpn.u32));
		double calculated = inpn.f32;
		double abserr = fabs(calculated - expected);
		max_log2_err = abserr > max_log2_err ? abserr : max_log2_err;
	}
	logprintf("Max absolute log2() error: %.16e\n", max_log2_err);

	double max_asin_err = 0.0f;
	for (unsigned i = 0; i < NUM_TESTS; i++) {
		union {
			float f32;
			unsigned int u32;
		} inpn;
		inpn.u32 = rndf32(-2, true);
		double expected = asin((double)inpn.f32) / M_PI_2;
		asm (
			"mtv %1, S000\n"
			"vasin.s S000, S000\n"
			"mfv %0, S000\n"
		:"=r"(inpn.u32): "r"(inpn.u32));
		double calculated = inpn.f32;
		double abserr = fabs(calculated - expected);
		max_asin_err = abserr > max_asin_err ? abserr : max_asin_err;
	}
	logprintf("Max absolute asin() error (0-0.5): %.16e\n", max_asin_err);

	max_asin_err = 0.0f;
	for (unsigned i = 0; i < NUM_TESTS; i++) {
		union {
			float f32;
			unsigned int u32;
		} inpn;
		inpn.u32 = rndf32(-1, true);
		double expected = asin((double)inpn.f32) / M_PI_2;
		asm (
			"mtv %1, S000\n"
			"vasin.s S000, S000\n"
			"mfv %0, S000\n"
		:"=r"(inpn.u32): "r"(inpn.u32));
		double calculated = inpn.f32;
		double abserr = fabs(calculated - expected);
		max_asin_err = abserr > max_asin_err ? abserr : max_asin_err;
	}
	logprintf("Max absolute asin() error: %.16e\n", max_asin_err);

	#ifdef _BUILD_EBOOT_
	fclose(logfd);
	#endif
	sceKernelExitGame();
}


