
#include <math.h>
#include <string.h>
#include <pspsdk.h>

#include "test-utils.h"

#define RET_ERRS(instname, res, expec) { \
    memset(&errlist[errcnt], 0, sizeof(errlist[errcnt])); \
    errlist[errcnt].inst = (instname); \
    errlist[errcnt].rdsz = 1; \
    errlist[errcnt].rd[0] = res; \
    errlist[errcnt].rdex[0] = expec; \
    errcnt++; \
}

// Delay slots have a significant amount of undefined behaviour. Check it here.
// Summarizing: Branches in delay slot seem to always be the branch taken (so
// the first branch is disregarded) with one exception: when the first branch
// is conditional and the second one is not.

// A possible explanation is that jumps are executed one cycle earlier than
// branches. This is backed by the jump to jump latency being 3 cycles and
// the conditional branch latency being 4 cycles. The CPU likely requires
// the branch to go through the ALU/EXEC stage (to compare the registers)
// whereas the jump only requires a register read (for JR) or nothing at all
// (for j).
//
// So two branches with the same latency are executed, the second one "wins".
// In case of jump+branch, the branch also wins, since it executes later too.
// For the only exceptional case (branch+jump) the branch takes precedence,
// likely due to both branches trying to change the PC during the same cycle
// (likely ALU/EXEC for the branch and RF/REG for the jump). We assume that
// "older" instructions "win".

int check_delay_slot_ub(struct test_error_info *errlist) {
	int errcnt = 0;

	// Disable interrupts while running the tests to avoid funny business from
	// interrupting us and getting unexpected results.
	unsigned st = pspSdkDisableInterrupts();

	// Check jump+jump (so a jump in the delay slot of another jump)
	// Second branch wins.
	unsigned outval;
	asm (
		".set noreorder\n"
		"li %0, 0\n"
		"nop; nop;\n"

		"j 1f\n"         // First jump
		"j 2f\n"         // Second jump, seems to be the one taken in this case
		"ori %0, 0x1\n"  // Check whether this delay slot is actually executed
		"j 3f; nop; nop\n"

		"2:\n"
		"  ori %0, 0x8\n"    // Record these instructions
		"  ori %0, 0x10\n"   // Just in case only one is actually executed
		"  j 3f; nop\n"

		"1:\n"
		"  ori %0, 0x2\n"    // Same as above. This is actually executed
		"  ori %0, 0x4\n"    // This as well
		"  j 3f; nop\n"

		"3: nop\n"
		".set reorder\n"
	:"=r"(outval)::"memory");

	if (outval != 0x18)
		RET_ERRS("j + j delay slot, unexpected path", outval, 0x18);

	// Check conditional branch + jump (so a jump in the delay slot of a taken branch)
	// First branch wins.
	asm (
		".set noreorder\n"
		"li %0, 0\n"
		"nop; nop;\n"

		"beq $0, $0, 1f\n"    // Always taken
		"j 2f\n"              // Second jump, seems to be ignored in this case
		"ori %0, 0x1\n"       // Check whether this delay slot is actually executed
		"j 3f; nop; nop\n"

		"2:\n"
		"  ori %0, 0x8\n"    // Record these instructions
		"  ori %0, 0x10\n"
		"  j 3f; nop\n"

		"1:\n"
		"  ori %0, 0x2\n"    // Path taken by the CPU
		"  ori %0, 0x4\n"
		"  j 3f; nop\n"

		"3: nop\n"
		".set reorder\n"
	:"=r"(outval)::"memory");

	if (outval != 0x6)
		RET_ERRS("beq(1) + j delay slot, unexpected path", outval, 0x6);

	// Same as above but the conditional branch is not taken. In this case
	// it is as if the branch was a nop (as expected).
	// This case is not that relevant (not taken branches are like a NOP)
	asm (
		".set noreorder\n"
		"li %0, 0\n"
		"nop; nop;\n"

		"bne $0, $0, 1f\n"    // Never taken
		"j 2f\n"              // Second jump, this should be taken
		"ori %0, 0x1\n"       // Check whether this delay slot is actually executed
		"j 3f; nop; nop\n"

		"2:\n"
		"  ori %0, 0x8\n"    // Record these instructions
		"  ori %0, 0x10\n"
		"  j 3f; nop\n"

		"1:\n"
		"  ori %0, 0x2\n"    // Taken path
		"  ori %0, 0x4\n"
		"  j 3f; nop\n"

		"3: nop\n"
		".set reorder\n"
	:"=r"(outval)::"memory");

	if (outval != 0x19)
		RET_ERRS("beq(2) + j delay slot, unexpected path", outval, 0x19);

	// Check jump + conditional branch: second branch wins.
	asm (
		".set noreorder\n"
		"li %0, 0\n"
		"nop; nop;\n"

		"j 1f\n"              // First jump
		"beq $0, $0, 2f\n"    // Always taken branch
		"ori %0, 0x1\n"       // Check whether this delay slot is actually executed
		"j 3f; nop; nop\n"

		"2:\n"
		"  ori %0, 0x8\n"    // Record these instructions
		"  ori %0, 0x10\n"   // These two are actually executed here.
		"  j 3f; nop\n"

		"1:\n"
		"  ori %0, 0x2\n"    // Not taken at all
		"  ori %0, 0x4\n"
		"  j 3f; nop\n"

		"3: nop\n"
		".set reorder\n"
	:"=r"(outval)::"memory");

	if (outval != 0x18)
		RET_ERRS("j + beq delay slot, unexpected path", outval, 0x18);

	// Check conditional branch + conditional branch. Second branch wins.
	asm (
		".set noreorder\n"
		"li %0, 0\n"
		"nop; nop;\n"

		"beq $0, $0, 1f\n"    // Always taken branch
		"beq $0, $0, 2f\n"    // Always taken branch
		"ori %0, 0x1\n"       // Check whether this delay slot is actually executed
		"j 3f; nop; nop\n"

		"2:\n"
		"  ori %0, 0x8\n"    // Record these instructions
		"  ori %0, 0x10\n"   // These two are actually executed here.
		"  j 3f; nop\n"

		"1:\n"
		"  ori %0, 0x2\n"    // Not taken at all
		"  ori %0, 0x4\n"
		"  j 3f; nop\n"

		"3: nop\n"
		".set reorder\n"
	:"=r"(outval)::"memory");

	if (outval != 0x18)
		RET_ERRS("beq + beq delay slot, unexpected path", outval, 0x18);

	// Just some extra validation that jump reg also works like any other jump
	asm (
		".set noreorder\n"
		"li %0, 0\n"
		"la $ra, 1f\n"
		"nop; nop;\n"

		"jr $ra\n"       // First jump
		"j 2f\n"         // Second jump, seems to be the one taken in this case
		"ori %0, 0x1\n"  // Check whether this delay slot is actually executed
		"j 3f; nop; nop\n"

		"2:\n"
		"  ori %0, 0x8\n"    // Record these instructions
		"  ori %0, 0x10\n"   // Just in case only one is actually executed
		"  j 3f; nop\n"

		"1:\n"
		"  ori %0, 0x2\n"    // Same as above. This is actually executed
		"  ori %0, 0x4\n"    // This as well
		"  j 3f; nop\n"

		"3: nop\n"
		".set reorder\n"
	:"=r"(outval)::"memory", "$ra");

	if (outval != 0x18)
		RET_ERRS("jr + j delay slot, unexpected path", outval, 0x18);

	// More checks with jr and branch now
	asm (
		".set noreorder\n"
		"li %0, 0\n"
		"la $ra, 2f\n"
		"nop; nop;\n"

		"beq $0, $0, 1f\n"   // First jump
		"jr $ra\n"           // Second jump, seems to be the one taken in this case
		"ori %0, 0x1\n"      // Check whether this delay slot is actually executed
		"j 3f; nop; nop\n"

		"2:\n"
		"  ori %0, 0x8\n"    // Record these instructions
		"  ori %0, 0x10\n"   // Just in case only one is actually executed
		"  j 3f; nop\n"

		"1:\n"
		"  ori %0, 0x2\n"    // Same as above. This is actually executed
		"  ori %0, 0x4\n"    // This as well
		"  j 3f; nop\n"

		"3: nop\n"
		".set reorder\n"
	:"=r"(outval)::"memory", "$ra");

	if (outval != 0x6)
		RET_ERRS("beq + jr delay slot, unexpected path", outval, 0x6);

	pspSdkEnableInterrupts(st);

	return errcnt;
}


