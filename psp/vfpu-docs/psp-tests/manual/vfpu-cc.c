
// Written by David Guillen Fandos <david@davidgf.net>

// Validate VFPU control registers and their behaviour
// Extracted from http://hitmen.c02.at/files/yapspd/psp_doc/chap4.html
// and other forums/websites/PPSSPP code.


#include "test-utils.h"

#define VFPU_PFXS   128    // Prefix registers
#define VFPU_PFXT   129
#define VFPU_PFXD   130

#define VFPU_CC     131    // Condition code

#define VFPU_RCX0   136    // Random state registers
#define VFPU_RCX1   137
#define VFPU_RCX2   138
#define VFPU_RCX3   139
#define VFPU_RCX4   140
#define VFPU_RCX5   141
#define VFPU_RCX6   142
#define VFPU_RCX7   143


// Default selectors: x,y,z,w (no constant nor abs/neg)
#define DEF_SRC_PFX   ((0b11 << 6) | (0b10 << 4) | (0b01 << 2) | (0b00))
// No masking nor saturation
#define DEF_DST_PFX   (0)

#define RET_ERR(errmsg, val, valex) { \
  errinfo[errcnt].msg = errmsg; \
  errinfo[errcnt].value = (val); \
  errinfo[errcnt].valueex = (valex); \
  errcnt++; \
}

static void readrndreg(unsigned int *regs) {
	// Check on random registers.
	unsigned int t = 0;
	asm (
		// Sets source registers and dest
		"mfvc %0, $136\n"
		"sw   %0,  0(%1)\n"
		"mfvc %0, $137\n"
		"sw   %0,  4(%1)\n"
		"mfvc %0, $138\n"
		"sw   %0,  8(%1)\n"
		"mfvc %0, $139\n"
		"sw   %0, 12(%1)\n"
		"mfvc %0, $140\n"
		"sw   %0, 16(%1)\n"
		"mfvc %0, $140\n"
		"sw   %0, 16(%1)\n"
		"mfvc %0, $141\n"
		"sw   %0, 20(%1)\n"
		"mfvc %0, $142\n"
		"sw   %0, 24(%1)\n"
		"mfvc %0, $143\n"
		"sw   %0, 28(%1)\n"
	:"+r"(t): "r"(&regs[0]): "memory");
}

int validate_ccregs(struct check_error_info *errinfo) {
	// Validate the prefix registers: they store the 24 bit payload.
	int errcnt = 0;
	unsigned pfs, pft, pfd;
	asm (
		// Sets source registers and dest
		"vpfxd [m,,,]\n"
		"vpfxs [y,y,y,y]\n"
		"vpfxt [z,z,z,z]\n"
		"mfvc %0, $128\n"   // Read source prefix
		"mfvc %1, $129\n"   // Read source prefix
		"mfvc %2, $130\n"   // Read source prefix
	:"=r"(pfs), "=r"(pft), "=r"(pfd));

	// Destination prefix should have channel 0 masked
	if (pfd != 0x100)
		RET_ERR("Bad dest prefix mismatch", pfd, 0x100);
	// We overrode the swizzling modes
	if (pfs != 0x55)
		RET_ERR("Bad source prefix mismatch", pfs, 0x55);
	if (pft != 0xAA)
		RET_ERR("Bad target prefix mismatch", pft, 0xAA);

	asm (
		"vadd.q R000, R000, R000\n"   // Ensure we clear the registers
		"mfvc %0, $128\n"   // Read source prefix
		"mfvc %1, $129\n"   // Read source prefix
		"mfvc %2, $130\n"   // Read source prefix
	:"=r"(pfs), "=r"(pft), "=r"(pfd));

	// Destination prefix should be zero (no mask, no saturation)
	if (pfd != DEF_DST_PFX)
		RET_ERR("Bad dest prefix mismatch", pfd, DEF_DST_PFX);
	// Source/Target use default swizzling
	if (pfs != DEF_SRC_PFX)
		RET_ERR("Bad source prefix mismatch", pfs, DEF_SRC_PFX);
	if (pft != DEF_SRC_PFX)
		RET_ERR("Bad target prefix mismatch", pft, DEF_SRC_PFX);

	// Validate which bits are actually updated on src/dst insts.
	asm (
		".word 0xdcffffff\n"  // Set all mask bits
		".word 0xddffffff\n"
		".word 0xdeffffff\n"

		"mfvc %0, $128\n"   // Read source prefix
		"mfvc %1, $129\n"   // Read source prefix
		"mfvc %2, $130\n"   // Read source prefix
	:"=r"(pfs), "=r"(pft), "=r"(pfd));

	// Destination prefix should only have 12 bits set
	if (pfd != 0xfff)
		RET_ERR("Dest prefix sets too many bits", pfd, 0xfff);
	if (pfs != 0xfffff)
		RET_ERR("Source rs prefix sets too many bits", pfs, 0xfffff);
	if (pft != 0xfffff)
		RET_ERR("Source rt prefix sets too many bits", pft, 0xfffff);

	// Check on random registers, they should change whenever rndi/f is called
	unsigned int regsA[8], regsB[8];
	readrndreg(regsA);
	asm("vrndi.s S000\n");
	readrndreg(regsB);
	if (!memcmp(regsA, regsB, sizeof(regsA)))
		RET_ERR("vrndi.s doesn't cause RCX register updates", 0, 0);
	asm("vrndf1.p R000\n");
	readrndreg(regsA);
	if (!memcmp(regsA, regsB, sizeof(regsA)))
		RET_ERR("vrndf1.p doesn't cause RCX register updates", 0, 0);
	asm("vrndf2.q R000\n");
	readrndreg(regsB);
	if (!memcmp(regsA, regsB, sizeof(regsA)))
		RET_ERR("vrndf2.p doesn't cause RCX register updates", 0, 0);
	asm("vone.s S000\nvrnds.s S000\n");
	readrndreg(regsA);
	if (!memcmp(regsA, regsB, sizeof(regsA)))
		RET_ERR("vrnds.s doesn't cause RCX register updates", 0, 0);

	// Validate hazards from various CC regs
	unsigned int outcodes[8];
	asm (
		".set noreorder\n"
		"li $v1, 1\n"
		"li $v0, 0\n"
		"mtvc $v0, $131\n"    // Load zero

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

		"vpfxd [m,m,m,m]\n"   // Retest with prefixes
		"mfvc $a0, $130\n"
		"vpfxs [-x,-y,-z,-w]\n"
		"mfvc $a1, $128\n"
		"vpfxt [-x,-y,-z,-w]\n"
		"mfvc $a2, $129\n"

		"sw $a0,16(%0)\n"
		"sw $a1,20(%0)\n"
		"sw $a2,24(%0)\n"

		".set reorder\n"
	::"r"(outcodes):"memory", "v0", "v1", "a0", "a1", "a2", "a3");

	// Result should be true, since all elements are true.
	// However it seems mfvc has 1 cycle/inst CC read hazard that the CPU
	// does not interlock.
	if (outcodes[0] != 0)
		RET_ERR("vcmp + mfvc [0 latency]", outcodes[0], 0);
	if (outcodes[1] != 0x3f)
		RET_ERR("vcmp + mfvc [1 latency]", outcodes[1], 0x3f);
	if (outcodes[2] != 0x3f)
		RET_ERR("vcmp + mfvc [2 latency]", outcodes[2], 0x3f);
	if (outcodes[3] != 0x3f)
		RET_ERR("vcmp + mfvc [3 latency]", outcodes[3], 0x3f);

	// Prefix instruction writes to the prefix stack, and mfvc does not consume prefixes
	// There's no hazard (likely due to pipeline latency, to RAW hazard)
	if (outcodes[4] != 0x00000f00)
		RET_ERR("vpfxd + mfvc [0 latency]", outcodes[4], 0x00000f00);
	if (outcodes[5] != 0x000f00e4)
		RET_ERR("vpfxs + mfvc [0 latency]", outcodes[5], 0x000f00e4);
	if (outcodes[6] != 0x000f00e4)
		RET_ERR("vpfxt + mfvc [0 latency]", outcodes[6], 0x000f00e4);

	// Clear up prefix state for other tests!!
	asm ("vnop; vadd.s S000, S000, S000; vnop;");

	return errcnt;
}


