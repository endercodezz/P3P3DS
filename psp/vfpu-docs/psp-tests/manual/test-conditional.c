
#include <math.h>
#include <string.h>
#include <pspsdk.h>

#include "test-utils.h"

#define RET_ERR(instname, op1, op2, res, expec) { \
	memset(&errlist[errcnt], 0, sizeof(errlist[errcnt])); \
    errlist[errcnt].inst = (instname); \
    errlist[errcnt].inputs[0].iname = "op1"; \
    errlist[errcnt].inputs[1].iname = "op2"; \
    memcpy(&errlist[errcnt].inputs[0].values[0], op1, sizeof(op1)); \
    memcpy(&errlist[errcnt].inputs[1].values[0], op2, sizeof(op2)); \
    errlist[errcnt].inputs[0].size = sizeof(op1) / 4; \
    errlist[errcnt].inputs[1].size = sizeof(op2) / 4; \
    errlist[errcnt].rdsz = 1; \
    errlist[errcnt].rd[0] = res; \
    errlist[errcnt].rdex[0] = expec; \
    errcnt++; \
}

#define RET_ERRS(instname, res, expec) { \
	memset(&errlist[errcnt], 0, sizeof(errlist[errcnt])); \
    errlist[errcnt].inst = (instname); \
    errlist[errcnt].rdsz = 1; \
    errlist[errcnt].rd[0] = res; \
    errlist[errcnt].rdex[0] = expec; \
    errcnt++; \
}

// Validates conditional instructions: vcmp, vcmovf/vcmovt and bvt/bvf/bvlt/bvlf
// Performs checks on the condition registers but also on the control flow.
// It uses and also tests instructions vmfvc and vmtvc.
int check_conditionals(struct test_error_info *errlist) {
	int errcnt = 0;
	unsigned int outcodes[16];

	// Disable interrupts since we do cycle accurate checks. We do not want
	// being interrupted since the hazards won't be visible.
	unsigned st = pspSdkDisableInterrupts();

	// Validate reading from CC regs
	asm (
		".set noreorder\n"
		"li $v1, 1\n"
		"li $v0, 0\n"
		"mtvc $v0, $131\n"    // Load zeros

		"vzero.s S000\n"
		"vone.s  S010\n"
		"vneg.s  S020, S010\n"
		"vadd.s  S030, S010, S010\n"
		"vneg.q  R100, R000\n"

		"vcmp.q EQ, R000, R000\n"
		"mfvc $a0, $131\n"    // This reads the previous CC value
		"mfvc $a1, $131\n"    // From here onwards it's valid
		"mfvc $a2, $131\n"
		"mfvc $a3, $131\n"
		"mtvc $v0, $131\n"    // Reset

		"sw $a0, 0(%0)\n"
		"sw $a1, 4(%0)\n"
		"sw $a2, 8(%0)\n"
		"sw $a3,12(%0)\n"

		"vcmp.q EQ, R000, R000\n"
		"vnop\n"
		"mfvc $a0, $131\n"    // This should be now correct (due to nop)
		"sw $a0, 16(%0)\n"
		"mtvc $v0, $131\n"    // Reset

		// Validate that jumps do not have a hazard
		"sw $v0, 20(%0)\n"
		"vcmp.q EQ, R000, R000\n"
		"bvt 0, 1f\n"
		"nop\n"
		"sw $v1, 20(%0)\n"    // Should not happen (write to 1)
		"1:\n"
		"nop\n"

		".set reorder\n"
	::"r"(outcodes):"memory", "v0", "v1", "a0", "a1", "a2", "a3");

	pspSdkEnableInterrupts(st);

	// Result should be true, since all elements are true.
	// However it seems mfvc has 1 cycle/inst CC read hazard that the CPU
	// does not interlock.
	if (outcodes[0] != 0)
		RET_ERRS("vcmp + mfvc [0 latency]", outcodes[0], 0);
	if (outcodes[1] != 0x3f)
		RET_ERRS("vcmp + mfvc [1 latency]", outcodes[1], 0x3f);
	if (outcodes[2] != 0x3f)
		RET_ERRS("vcmp + mfvc [2 latency]", outcodes[2], 0x3f);
	if (outcodes[3] != 0x3f)
		RET_ERRS("vcmp + mfvc [3 latency]", outcodes[3], 0x3f);
	// With a vnop we see the right value as expected
	if (outcodes[4] != 0x3f)
		RET_ERRS("vcmp + mfvc [+vnop latency]", outcodes[4], 0x3f);
	// Branch instructions have proper interlocking, no nop needed.
	if (outcodes[5] != 0)
		RET_ERRS("vcmp + bvt [0 latency]", outcodes[5], 0);

	const float ops[2][4] = {
		{ 0.0f,  1.0f, -1.0f,  2.0f},
		{-0.0f, -1.0f,  1.0f, -2.0f},
	};

	asm (
		".set noreorder\n"
		"li $v0, 0\n"
		"mtvc $v0, $131\n"    // Load zeros

		"mtvc $v0, $131\n"
		"vcmp.q EQ, R000, R000\n"
		"vnop\n"
		"mfvc $a0, $131\n"
		"sw $a0, 0(%0)\n"

		"mtvc $v0, $131\n"
		"vcmp.q NE, R000, R000\n"
		"vnop\n"
		"mfvc $a0, $131\n"
		"sw $a0, 4(%0)\n"

		"mtvc $v0, $131\n"
		"vcmp.q EQ, R000, R100\n"
		"vnop\n"
		"mfvc $a0, $131\n"
		"sw $a0, 8(%0)\n"

		"mtvc $v0, $131\n"
		"vcmp.q NE, R000, R100\n"
		"vnop\n"
		"mfvc $a0, $131\n"
		"sw $a0,12(%0)\n"

		".set reorder\n"
	::"r"(outcodes):"memory", "v0", "a0");

	if (outcodes[0] != 0x3f)
		RET_ERR("vcmp EQ", ops[0], ops[0], outcodes[0], 0x3f);
	if (outcodes[1] != 0)
		RET_ERR("vcmp NE", ops[0], ops[0], outcodes[1], 0);
	if (outcodes[2] != 0x11)
		RET_ERR("vcmp EQ", ops[0], ops[1], outcodes[2], 0x11);
	if (outcodes[3] != 0x1E)
		RET_ERR("vcmp NE", ops[0], ops[1], outcodes[3], 0x1E);

	asm (
		".set noreorder\n"
		"li $v0, 0\n"
		"mtvc $v0, $131\n"    // Load zeros

		// Check that only the relevant bits get set
		"mtvc $v0, $131\n"
		"vcmp.s EQ, S000, S000\n"
		"vnop\n"
		"mfvc $a0, $131\n"
		"sw $a0, 0(%0)\n"

		"mtvc $v0, $131\n"
		"vcmp.p EQ, R000, R000\n"
		"vnop\n"
		"mfvc $a0, $131\n"
		"sw $a0, 4(%0)\n"

		"mtvc $v0, $131\n"
		"vcmp.t EQ, R000, R000\n"
		"vnop\n"
		"mfvc $a0, $131\n"
		"sw $a0, 8(%0)\n"

		".set reorder\n"
	::"r"(outcodes):"memory", "v0", "a0");

	if (outcodes[0] != 0x31)
		RET_ERR("vcmp.s EQ", ops[0], ops[0], outcodes[0], 0x31);
	if (outcodes[1] != 0x33)
		RET_ERR("vcmp.p EQ", ops[0], ops[0], outcodes[0], 0x33);
	if (outcodes[2] != 0x37)
		RET_ERR("vcmp.t EQ", ops[0], ops[0], outcodes[0], 0x37);

	return errcnt;
}


