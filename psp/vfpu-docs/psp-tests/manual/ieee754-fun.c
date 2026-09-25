
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pspfpu.h>

#include "test-utils.h"

#define RET_ERR(instname, op1, op2, res, expec) { \
	memset(&errlist[errcnt], 0, sizeof(errlist[errcnt])); \
    errlist[errcnt].inst = (instname); \
    errlist[errcnt].inputs[0].iname = "op1"; \
    errlist[errcnt].inputs[1].iname = "op2"; \
    errlist[errcnt].inputs[0].values[0] = op1; \
    errlist[errcnt].inputs[1].values[0] = op2; \
    errlist[errcnt].inputs[0].size = 1; \
    errlist[errcnt].inputs[1].size = 1; \
    errlist[errcnt].rdsz = 1; \
    errlist[errcnt].rd[0] = res; \
    errlist[errcnt].rdex[0] = expec; \
    errcnt++; \
}

#define RET_ERR1(instname, op1, res, expec) { \
	memset(&errlist[errcnt], 0, sizeof(errlist[errcnt])); \
    errlist[errcnt].inst = (instname); \
    errlist[errcnt].inputs[0].iname = "op1"; \
    errlist[errcnt].inputs[0].values[0] = op1; \
    errlist[errcnt].inputs[0].size = 1; \
    errlist[errcnt].rdsz = 1; \
    errlist[errcnt].rd[0] = res; \
    errlist[errcnt].rdex[0] = expec; \
    errcnt++; \
}

// Performs some validation on the way the VFPU handles NaNs and their propagation.
// In general it seems it does not emit any exceptions on NaN (signaling or quiet)
// and it seems to always emit the same kind of NaN encoding.
int check_nans(struct test_error_info *errlist) {
	int errcnt = 0;
	// Remember bit 22 means quiet (1) or signaling (0)
	unsigned qnan1 = 0x7FFFFFFF, qnan2 = 0x7FC00000;
	unsigned snan1 = 0x7FBFFFFF, snan2 = 0x7F800001;
	unsigned mqnan1 = 0xFFFFFFFF, mqnan2 = 0xFFC00000;
	unsigned msnan1 = 0xFFBFFFFF;
	unsigned plusinf  = 0x7F800000;
	unsigned minusinf = 0xFF800000;
	unsigned one = 0x3F800000, mone = 0xBF800000;

	union {
		unsigned u32[32];
		float    f32[32];
	} res;
	memset(&res, 0, sizeof(res));

	// Validate that any nan operation propagates the expected NaN value
	asm (
		"mtv %0, S000\n"              // qNan1
		"mtv %1, S001\n"              // qNan2
		"mtv %2, S002\n"              // sNan1
		"mtv %3, S003\n"              // sNan2
		"mtv %4, S010\n"              // Inf+
		"mtv %5, S011\n"              // Inf-
		"mtv %6, S012\n"              // -qNan1
		"mtv %7, S013\n"              // -sNan1
		"vone.q R100\n"
		"vmov.q R103, R100[-x,-x,-x,-x]\n"
		"vzero.q R200\n"
		"vadd.s S400, S000, S000\n"   // Returns NaN
		"vadd.s S401, S000, S001\n"   // Returns NaN
		"vadd.s S402, S010, S010\n"   // Returns +Inf
		"vadd.s S403, S011, S011\n"   // Returns -Inf
		"vsub.s S410, S010, S010\n"   // Returns NaN (Inf - Inf)
		"vadd.s S411, S010, S011\n"   // Returns NaN (Inf +-Inf)
		"vadd.s S412, S010, S100\n"   // Returns +Inf
		"vadd.s S413, S011, S100\n"   // Returns -Inf
		"vdiv.s S420, S100, S200\n"   // Returns +Inf
		"vdiv.s S421, S103, S200\n"   // Returns -Inf
		"sv.s S400,  0(%8)\n"         // Store results
		"sv.s S401,  4(%8)\n"
		"sv.s S402,  8(%8)\n"
		"sv.s S403, 12(%8)\n"
		"sv.s S410, 16(%8)\n"
		"sv.s S411, 20(%8)\n"
		"sv.s S412, 24(%8)\n"
		"sv.s S413, 28(%8)\n"
		"sv.s S420, 32(%8)\n"
		"sv.s S421, 36(%8)\n"
		"sv.s S422, 40(%8)\n"
		"sv.s S423, 44(%8)\n"
	::"r"(qnan1), "r"(qnan2), "r"(snan1), "r"(snan2),
	  "r"(plusinf), "r"(minusinf), "r"(mqnan1), "r"(msnan1),
	  "r"(&res.u32[0]) : "memory");

	// NaN operations return Nan (as 0x7FFFFFFF)
	if (res.u32[0] != snan2)
		RET_ERR("vadd.s", qnan1, qnan1, res.u32[0], snan2);

	if (res.u32[1] != snan2)
		RET_ERR("vadd.s", qnan1, qnan2, res.u32[1], snan2);

	// Infinity math seems to be IEEE754 compliant
	if (res.u32[2] != plusinf)
		RET_ERR("vadd.s", plusinf, plusinf, res.u32[2], plusinf);

	if (res.u32[3] != minusinf)
		RET_ERR("vadd.s", minusinf, minusinf, res.u32[3], minusinf);

	// Some operations emit negative NaNs (as 0xFFC00000)
	if (res.u32[4] != snan2)
		RET_ERR("vsub.s", plusinf, plusinf, res.u32[4], snan2);

	if (res.u32[5] != snan2)
		RET_ERR("vadd.s", plusinf, minusinf, res.u32[5], snan2);

	if (res.u32[6] != plusinf)
		RET_ERR("vadd.s", plusinf, one, res.u32[6], plusinf);

	if (res.u32[7] != minusinf)
		RET_ERR("vadd.s", minusinf, one, res.u32[7], minusinf);

	if (res.u32[8] != plusinf)
		RET_ERR("vdiv.s", one, 0, res.u32[8], plusinf);

	if (res.u32[9] != minusinf)
		RET_ERR("vdiv.s", mone, 0, res.u32[9], minusinf);

	return errcnt;
}

// Performs some validation on rounding modes
int check_rounding(struct test_error_info *errlist) {
	int errcnt = 0;

	union {
		float f32;
		uint32_t u32;
	} res;

	enum PspFpuRoundMode pmode = pspFpuGetRoundmode();

	// VFPU always uses round to nearest
	for (unsigned i = 0; i < 4; i++) {
		// Validate that the FPU rounding mode (from the MIPS CPU) has nothing to do with VFPU rounding
		pspFpuSetRoundmode(i);

		asm volatile(
			"mtv %1, S000\n"        // 1.9999999...
			"mtv %2, S001\n"        // 5.96046447754e-08
			"vadd.s S002, S000, S001\n"
			"mfv %0, S002\n"
		:"=r"(res.f32): "r"(0x3fffffff), "r"(0x33800000));
		if (res.u32 != 0x40000000)
			RET_ERR("rounding test (vadd.s)", 0x3fffffff, 0x33800000, res.u32, 0x40000000);

		asm volatile(
			"mtv %1, S000\n"        // 2.9999999...
			"mtv %2, S001\n"        // 5.96046447754e-08
			"vadd.s S002, S000, S001\n"
			"mfv %0, S002\n"
		:"=r"(res.f32): "r"(0x403fffff), "r"(0x33800000));
		if (res.u32 != 0x403fffff)
			RET_ERR("rounding test (vadd.s)", 0x403fffff, 0x33800000, res.u32, 0x403fffff);
	}

	pspFpuSetRoundmode(pmode);  // Restore previous mode

	return errcnt;
}

// Some validation on half-float (float16) format. It seems there are some
// "bugs" (or interesting/weird behaviour) on some instructions, usually at Nans,
// infinite or denormals.
int check_f16(struct test_error_info *errlist) {
	int errcnt = 0;
	unsigned outputs[16];

	// Validate that any nan operation propagates the expected NaN value
	asm (
		"li $v0, 0x0001\n"            // Denormal
		"mtv $v0, S000\n"
		"vh2f.s R000, S000\n"         // Both should be zero
		"mfv $v0, S000\n"
		"sw $v0,  0(%0)\n"

		".word 0xDF800001\n"          // vfim S000, 0x0001 (denormal again)
		"mfv $v0, S000\n"
		"sw $v0,  4(%0)\n"            // Should also be zero

		"li $v0, 0x7E00\n"            // NaN (highest bit set)
		"mtv $v0, S000\n"
		"vh2f.s R000, S000\n"         // Mantissa copied verbatim  0x200
		"mfv $v0, S000\n"
		"sw $v0,  8(%0)\n"

		"li $v0, 0x7C01\n"            // NaN (lowest bit set)
		"mtv $v0, S000\n"
		"vh2f.s R000, S000\n"         // Mantissa copied verbatim  0x1
		"mfv $v0, S000\n"
		"sw $v0, 12(%0)\n"

		".word 0xDF807e00\n"          // vfim S000, 0x7e00 (same as other two tests)
		"mfv $v0, S000\n"
		"sw $v0, 16(%0)\n"            // Should also be zero

		".word 0xDF807c01\n"          // vfim S000, 0x7c01 (same as other two tests)
		"mfv $v0, S000\n"
		"sw $v0, 20(%0)\n"            // Should also be zero
	::"r"(outputs) : "$v0", "memory");

	// Denormals are zero, be it vfim, vh2f or any other
	if (outputs[0] != 0)
		RET_ERR1("vh2f 0x0001", 0x0001, outputs[0], 0);
	if (outputs[1] != 0)
		RET_ERR1("vfim 0x0001", 0x0001, outputs[1], 0);

	if (outputs[2] != 0x7f800200)
		RET_ERR1("vh2f 0x7e00", 0x7e00, outputs[2], 0x7f800200);
	if (outputs[3] != 0x7f800001)
		RET_ERR1("vh2f 0x7c01", 0x7c01, outputs[3], 0x7f800001);
	if (outputs[4] != 0x7f800200)
		RET_ERR1("vfim 0x7e00", 0x7e00, outputs[4], 0x7f800200);
	if (outputs[5] != 0x7f800001)
		RET_ERR1("vfim 0x7c01", 0x7c01, outputs[5], 0x7f800001);

	// Test vf2h and some interesting bugs
	// Validate that any nan operation propagates the expected NaN value
	asm (
		"la $v0, 0x7FFFFFFF\n"        // NaN
		"mtv $v0, S000\n"
		"mtv $v0, S010\n"
		"vf2h.p S000, R000\n"         // Produces two f16 NaN
		"mfv $v0, S000\n"
		"sw $v0,  0(%0)\n"

		"la $v0, 0x7F800001\n"        // NaN
		"mtv $v0, S000\n"
		"mtv $v0, S010\n"
		"vf2h.p S000, R000\n"         // Still produces a valid NaN
		"mfv $v0, S000\n"
		"sw $v0,  4(%0)\n"

		"la $v0, 0x7F802000\n"        // NaN
		"mtv $v0, S000\n"
		"mtv $v0, S010\n"
		"vf2h.p S000, R000\n"         // Will produce Inf due to mantissa truncation
		"mfv $v0, S000\n"
		"sw $v0,  8(%0)\n"

		"la $v0, 0x7FC00000\n"        // NaN (to the highest bit)
		"mtv $v0, S000\n"
		"mtv $v0, S010\n"
		"vf2h.p S000, R000\n"         // Will produce Inf due to mantissa truncation
		"mfv $v0, S000\n"
		"sw $v0, 12(%0)\n"
	::"r"(outputs) : "$v0", "memory");

	if (outputs[0] != 0x7fff7fff)
		RET_ERR1("vf2h 0x7FFFFFFF", 0x7FFFFFFF, outputs[0], 0x7fff7fff);
	if (outputs[1] != 0x7c017c01)
		RET_ERR1("vf2h 0x7F800001", 0x7F800001, outputs[1], 0x7c017c01);
	if (outputs[2] != 0x7c007c00)
		RET_ERR1("vf2h 0x7F802000", 0x7F802000, outputs[2], 0x7c007c00);

	return errcnt;
}

int check_vidt(struct test_error_info *errlist) {
	int errcnt = 0;
	unsigned outputs[16];
	unsigned fpone = 0x3f800000;

	// vidt behaviour depends on register number really.
	asm (
		"vidt.p R000\n"               // Should be (1, 0)
		"vidt.p R001\n"               // Should be (0, 1)
		"vidt.p R020\n"               // Should be (1, 0)
		"vidt.p R021\n"               // Should be (0, 1)

		"sv.s S000,  0(%0)\n"
		"sv.s S010,  4(%0)\n"
		"sv.s S020,  8(%0)\n"
		"sv.s S030, 12(%0)\n"
		"sv.s S001, 16(%0)\n"
		"sv.s S011, 20(%0)\n"
		"sv.s S021, 24(%0)\n"
		"sv.s S031, 28(%0)\n"
	::"r"(outputs):"memory");

	if (outputs[0] != fpone)
		RET_ERR1("vidt.p R000", 0, outputs[0], fpone);
	if (outputs[1] != 0)
		RET_ERR1("vidt.p R000", 0, outputs[1], 0);
	if (outputs[2] != fpone)
		RET_ERR1("vidt.p R020", 0, outputs[2], fpone);
	if (outputs[3] != 0)
		RET_ERR1("vidt.p R020", 0, outputs[3], 0);

	if (outputs[4] != 0)
		RET_ERR1("vidt.p R001", 0, outputs[4], 0);
	if (outputs[5] != fpone)
		RET_ERR1("vidt.p R001", 0, outputs[5], fpone);
	if (outputs[6] != 0)
		RET_ERR1("vidt.p R021", 0, outputs[6], 0);
	if (outputs[7] != fpone)
		RET_ERR1("vidt.p R021", 0, outputs[7], fpone);

	asm (
		"vidt.q R100\n"               // Should be (1, 0, 0, 0)
		"vidt.q R101\n"               // Should be (0, 1, 0, 0)
		"vidt.q R102\n"               // Should be (0, 0, 1, 0)
		"vidt.q R103\n"               // Should be (0, 0, 0, 1)

		"sv.s S100,  0(%0); sv.s S110,  4(%0); sv.s S120,  8(%0); sv.s S130, 12(%0)\n"
		"sv.s S101, 16(%0); sv.s S111, 20(%0); sv.s S121, 24(%0); sv.s S131, 28(%0)\n"
		"sv.s S102, 32(%0); sv.s S112, 36(%0); sv.s S122, 40(%0); sv.s S132, 44(%0)\n"
		"sv.s S103, 48(%0); sv.s S113, 52(%0); sv.s S123, 56(%0); sv.s S133, 60(%0)\n"
	::"r"(outputs):"memory");

	for (unsigned c = 0; c < 4; c++) {
		for (unsigned r = 0; r < 4; r++) {
			float exp = (c == r) ? fpone : 0;
			if (outputs[c + r*4] != exp)
				RET_ERR1("vidt.q R1XX.q", 0, outputs[c + r*4], exp);
		}
	}

	return errcnt;
}


