
#include <string.h>
#include <stdio.h>

#include "test-utils.h"
#include "exception-module/exception_info.h"

static struct {
  // We do 16 experiments (16 different alignments)
  // for each we test the 64 possible VFPU registers
  // and we store the 32 (32bit) FPU registers
  uint32_t res_lvl[16][64][32];
  uint32_t res_lvr[16][64][32];
  uint32_t res_ulv[16][64][32];
} res;

// Same as above, but a bitfield indicates corrupted register number
static struct {
  uint32_t res_lvl[16][64];
  uint32_t res_lvr[16][64];
  uint32_t res_ulv[16][64];
} maskres;

#define FILL_ERR(errmsg, v, ve)  \
{                                \
  errs[errcnt].msg = (errmsg);   \
  errs[errcnt].value = (v);      \
  errs[errcnt].valueex = (ve);   \
  errcnt++;                      \
}

// Validates ulv instruction behaviour, buggy in PSP FAT (1000) devices
// The instruction will corrupt regular FPU registers in a deterministic way

static void test_unaligned_loads() {
	// Reset struct for helper assembly to fill in
	memset(&res, ~0, sizeof(res));

	asm volatile (
		".set noreorder\n"

		"move $s3, %0\n"       // Output buffer pointer
		"move $s4, $0\n"       // Unalignment in bytes

		// We loop for every unalignment type (0, 4, 8, ... 60 byte
		// (we use some internal aligned buffer)
		"100:\n"
		"li $a0, 0*16*64*32*4\n"
		"addu $a0, $a0, $s3\n"
		"jal do_test_lvl\n"   // Do test for lvl.q instruction
		"move $a1, $s4\n"

		"li $a0, 1*16*64*32*4\n"
		"addu $a0, $a0, $s3\n"
		"jal do_test_lvr\n"   // Do test for lvr.q instruction
		"move $a1, $s4\n"

		"li $a0, 2*16*64*32*4\n"
		"addu $a0, $a0, $s3\n"
		"jal do_test_ulv\n"   // Do test for lvr+lvl instructions
		"move $a1, $s4\n"

		// Advance to the next unaligment
		"addiu $s3, $s3, 64*32*4\n"
		"addiu $s4, $s4, 4\n"       // 4 bytes increment
		"li $a0, 64\n"
		"bne $a0, $s4, 100b\n"
		"nop\n"

		".set noreorder\n"
	::"r"(&res)
	// We use these registers as temporary regs (and do calls)
	:"ra", "memory", "v0", "v1", "a0", "a1", "a2", "s3", "s4",
	// We are gonna trash all of these registers of course!
	"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
	"f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
	"f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
	"f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31");

	// We repeat this for 16 different mis alignments (unal*4)
	for (unsigned unal = 0; unal < 16; unal++) {
		// For each 64 VFPU registers (for *lv.q)
		for (unsigned vfpu_reg = 0; vfpu_reg < 64; vfpu_reg++) {
			// Check what happened to the FPU registers
			uint32_t lvlmask = 0, lvrmask = 0, ulvmask = 0;
			for (unsigned fpu_reg = 0; fpu_reg < 32; fpu_reg++) {
				if (res.res_lvl[unal][vfpu_reg][fpu_reg] != (0x449a0000 | (fpu_reg << 8)))
					lvlmask |= (1 << fpu_reg);
				if (res.res_lvr[unal][vfpu_reg][fpu_reg] != (0x449a0000 | (fpu_reg << 8)))
					lvrmask |= (1 << fpu_reg);
				if (res.res_ulv[unal][vfpu_reg][fpu_reg] != (0x449a0000 | (fpu_reg << 8)))
					ulvmask |= (1 << fpu_reg);
			}

			// Accumulate with any previous errors
			maskres.res_lvl[unal][vfpu_reg] |= lvlmask;
			maskres.res_lvr[unal][vfpu_reg] |= lvrmask;
			maskres.res_ulv[unal][vfpu_reg] |= ulvmask;
		}
	}
}

// Main function: here we repeatedly call the unaligned load function
// to ensure we get some corruption (since there's a small random factor)

int check_unaligned_vfpu_load_bug(struct check_error_info *errs, int is_fat) {
	int errcnt = 0;
	// Reset masks before running tests
	memset(&maskres, 0, sizeof(maskres));

	// We run this a few times, since corruption can, under some circumstances,
	// act up and not corrupt the register (Or corrupt it with a "valid" value)
	for (unsigned i = 0; i < 64; i++)
		test_unaligned_loads();

	// Now we validate the mask results
	for (unsigned unal = 0; unal < 16; unal++) {
		for (unsigned vfpu_reg = 0; vfpu_reg < 64; vfpu_reg++) {
			// All registers should be OK except for fpu_reg == (vfpu_reg & 31)
			// We check for:
			//  - No corruptions (model 2000+)
			//  - Corruption only in the right register
			if (is_fat) {
				// Check that no register was corrupted!
				if (maskres.res_lvl[unal][vfpu_reg] != (1 << (vfpu_reg & 31)))
					FILL_ERR("lvl.q corrupted a VFPU register!", maskres.res_lvl[unal][vfpu_reg], (1 << (vfpu_reg & 31)))
				if (maskres.res_lvr[unal][vfpu_reg] != (1 << (vfpu_reg & 31)))
					FILL_ERR("lvr.q corrupted a VFPU register!", maskres.res_lvr[unal][vfpu_reg], (1 << (vfpu_reg & 31)))
				if (maskres.res_ulv[unal][vfpu_reg] != (1 << (vfpu_reg & 31)))
					FILL_ERR("ulv.q corrupted a VFPU register!", maskres.res_ulv[unal][vfpu_reg], (1 << (vfpu_reg & 31)))			
			} else {
				// Check that no register was corrupted!
				if (maskres.res_lvl[unal][vfpu_reg])
					FILL_ERR("lvl.q corrupted a VFPU register!", maskres.res_lvl[unal][vfpu_reg], 0)
				if (maskres.res_lvr[unal][vfpu_reg])
					FILL_ERR("lvr.q corrupted a VFPU register!", maskres.res_lvr[unal][vfpu_reg], 0)
				if (maskres.res_ulv[unal][vfpu_reg])
					FILL_ERR("ulv.q corrupted a VFPU register!", maskres.res_ulv[unal][vfpu_reg], 0)
			}
			if (errcnt >= 64)
				return errcnt;
		}
	}

	return errcnt;
}

#define CHECK_TEST_P(errmsg, cmpa, cmpb) \
  if ( (cmpa) != (cmpb) ) {            \
    errs[errcnt].msg = (errmsg);       \
    errs[errcnt].value = (cmpa);       \
    errs[errcnt].valueex = (cmpb);     \
    errcnt++;                          \
  }

#define CHECK_OUTPUT(errmsg, cmpptr, v0, v1, v2, v3) \
{ \
  CHECK_TEST_P(errmsg, (cmpptr)[0], v0); \
  CHECK_TEST_P(errmsg, (cmpptr)[1], v1); \
  CHECK_TEST_P(errmsg, (cmpptr)[2], v2); \
  CHECK_TEST_P(errmsg, (cmpptr)[3], v3); \
}

#define CHECK_MEM_P(errmsg, cmpa, cmpb) \
  if ( (cmpa) != (cmpb) ) {            \
    errs[errcnt].msg = (errmsg);       \
    errs[errcnt].value = (cmpa);       \
    errs[errcnt].valueex = (cmpb);     \
    errcnt++;                          \
  } \

#define CHECK_MEMREG(errmsg, cmpptr, v0, v1, v2, v3, v4, v5, v6, v7) \
{ \
  CHECK_MEM_P(errmsg, (cmpptr)[0], 0); \
  CHECK_MEM_P(errmsg, (cmpptr)[1], 0); \
  CHECK_MEM_P(errmsg, (cmpptr)[2], 0); \
  CHECK_MEM_P(errmsg, (cmpptr)[3], 0); \
  CHECK_MEM_P(errmsg, (cmpptr)[4], v0); \
  CHECK_MEM_P(errmsg, (cmpptr)[5], v1); \
  CHECK_MEM_P(errmsg, (cmpptr)[6], v2); \
  CHECK_MEM_P(errmsg, (cmpptr)[7], v3); \
  CHECK_MEM_P(errmsg, (cmpptr)[8], v4); \
  CHECK_MEM_P(errmsg, (cmpptr)[9], v5); \
  CHECK_MEM_P(errmsg, (cmpptr)[10], v6); \
  CHECK_MEM_P(errmsg, (cmpptr)[11], v7); \
}

// Regular ulv/usv validation: validates which memory blocks are read/written
// by each left/right instruction.
// TODO: Test unaligned cases and verify they generate an exception. This needs
// exception support (which might require a kernel module to be loaded and
// support for resuming from an exception, ie. non fatal).

const uint32_t ref_buffer[] __attribute__((aligned(16))) = {
	0xBADDA5A0, 0xBADDA5A1, 0xBADDA5A2, 0xBADDA5A3,
	0xDEAD0101, 0xDEAD0202, 0xDEAD0303, 0xDEAD0404,
	0xDEAD0505, 0xDEAD0606, 0xDEAD0707, 0xDEAD0808,
	0xDEAD0909, 0xDEAD0A0A, 0xDEAD0B0B, 0xDEAD0C0C,
	0xDEAD0D0D, 0xDEAD0E0E, 0xDEAD0F0F, 0xDEAD0F1F,
};

static uint32_t tmpbuf[64] __attribute__((aligned(16)));

static uint32_t savebuf[16][4*3] __attribute__((aligned(16))) = {0};

static uint32_t savebuf2[2][4*3] __attribute__((aligned(16))) = {0};

int check_unaligned_vfpu_memops(struct check_error_info *errs) {
	int errcnt = 0;

	// Validate lvl.q and lvr.q instructions
	// They behave like LWL and LWR but at a 4 byte element level (and 16 byte boundary)
	//
	// ulw rd, off(rb) is transformed into the pair: lwl rd, off+3(rb); lwr rd, off(rb)
	// (check the MIPS manual to see how the offsets work, lwl is a bit counter-intuitive
	// likewise ulv.q rd, off(rb) is expanded as: lvl.q rd, off+12(rb); lvr.q rd, off(rb)

	asm volatile (
		"vmzero.q M000; vmzero.q M100; vmzero.q M200; vmzero.q M300\n"

		"lvl.q R000, 0(%1)\n"   // This one loads ADDR-12, but only the "left" part of the cache line
		"lvr.q R001, 0(%1)\n"   // Whereas this one loads the full cache line
		"lvl.q R002, 4(%1)\n"
		"lvr.q R003, 4(%1)\n"
		"lvl.q R100, 8(%1)\n"
		"lvr.q R101, 8(%1)\n"
		"lvl.q R102,12(%1)\n"
		"lvr.q R103,12(%1)\n"

		// This is how the compiler would expand it
		"lvl.q R200, 0+12(%1)\n"   // In this aligned case both instructions end up
		"lvr.q R201, 0   (%1)\n"   // loading the full contents of the cache line
		"lvl.q R202, 4+12(%1)\n"   // This one loads 1 element from the next 16 byte line
		"lvr.q R203, 4   (%1)\n"   // This one loads the 3 elements in the current 16 byte line
		"lvl.q R300, 8+12(%1)\n"
		"lvr.q R301, 8   (%1)\n"
		"lvl.q R302,12+12(%1)\n"   // Inversely this one loads 3 elements
		"lvr.q R303,12   (%1)\n"   // and this one just one

		"sv.q R000, 0*16(%0)\n"
		"sv.q R001, 1*16(%0)\n"
		"sv.q R002, 2*16(%0)\n"
		"sv.q R003, 3*16(%0)\n"
		"sv.q R100, 4*16(%0)\n"
		"sv.q R101, 5*16(%0)\n"
		"sv.q R102, 6*16(%0)\n"
		"sv.q R103, 7*16(%0)\n"

		"sv.q R200, 8*16(%0)\n"
		"sv.q R201, 9*16(%0)\n"
		"sv.q R202,10*16(%0)\n"
		"sv.q R203,11*16(%0)\n"
		"sv.q R300,12*16(%0)\n"
		"sv.q R301,13*16(%0)\n"
		"sv.q R302,14*16(%0)\n"
		"sv.q R303,15*16(%0)\n"

	::"r"(tmpbuf), "r"(&ref_buffer[4]) : "memory");

	CHECK_OUTPUT("lvl.q off= 0", &tmpbuf[ 0], 0x00000000, 0x00000000, 0x00000000, 0xdead0101);
	CHECK_OUTPUT("lvr.q off= 0", &tmpbuf[ 4], 0xdead0101, 0xdead0202, 0xdead0303, 0xdead0404);
	CHECK_OUTPUT("lvl.q off= 4", &tmpbuf[ 8], 0x00000000, 0x00000000, 0xdead0101, 0xdead0202);
	CHECK_OUTPUT("lvr.q off= 4", &tmpbuf[12], 0xdead0202, 0xdead0303, 0xdead0404, 0x00000000);
	CHECK_OUTPUT("lvl.q off= 8", &tmpbuf[16], 0x00000000, 0xdead0101, 0xdead0202, 0xdead0303);
	CHECK_OUTPUT("lvr.q off= 8", &tmpbuf[20], 0xdead0303, 0xdead0404, 0x00000000, 0x00000000);
	CHECK_OUTPUT("lvl.q off=12", &tmpbuf[24], 0xdead0101, 0xdead0202, 0xdead0303, 0xdead0404);
	CHECK_OUTPUT("lvr.q off=12", &tmpbuf[28], 0xdead0404, 0x00000000, 0x00000000, 0x00000000);

	CHECK_OUTPUT("lvl.q off= 0+12", &tmpbuf[32], 0xdead0101, 0xdead0202, 0xdead0303, 0xdead0404);
	CHECK_OUTPUT("lvr.q off= 0",    &tmpbuf[36], 0xdead0101, 0xdead0202, 0xdead0303, 0xdead0404);
	CHECK_OUTPUT("lvl.q off= 4+12", &tmpbuf[40], 0x00000000, 0x00000000, 0x00000000, 0xdead0505);
	CHECK_OUTPUT("lvr.q off= 4",    &tmpbuf[44], 0xdead0202, 0xdead0303, 0xdead0404, 0x00000000);
	CHECK_OUTPUT("lvl.q off= 8+12", &tmpbuf[48], 0x00000000, 0x00000000, 0xdead0505, 0xdead0606);
	CHECK_OUTPUT("lvr.q off= 8",    &tmpbuf[52], 0xdead0303, 0xdead0404, 0x00000000, 0x00000000);
	CHECK_OUTPUT("lvl.q off=12+12", &tmpbuf[56], 0x00000000, 0xdead0505, 0xdead0606, 0xdead0707);
	CHECK_OUTPUT("lvr.q off=12",    &tmpbuf[60], 0xdead0404, 0x00000000, 0x00000000, 0x00000000);

	// Validate the use of svl and svr instructions
	asm volatile (
		"lv.q R000, 0(%1)\n"     // Load some useful data to write

		// All these write withing the 16 bytes indicated
		"svl.q R000,  0*48 + 0(%0)\n"    // svl should write the next block only
		"svr.q R000,  1*48 + 0(%0)\n"    // svr writes elements in the current 16 byte block
		"svl.q R000,  2*48 + 4(%0)\n"
		"svr.q R000,  3*48 + 4(%0)\n"
		"svl.q R000,  4*48 + 8(%0)\n"
		"svr.q R000,  5*48 + 8(%0)\n"
		"svl.q R000,  6*48 +12(%0)\n"
		"svr.q R000,  7*48 +12(%0)\n"

		// Now do the ulv emulation code
		"svl.q R000,  8*48 + 0+12(%0)\n"   // In this case both write the same (write twice)
		"svr.q R000,  9*48 + 0   (%0)\n"
		"svl.q R000, 10*48 + 4+12(%0)\n"   // This writes one element in the next block
		"svr.q R000, 11*48 + 4   (%0)\n"   // This writes three elements in the same block
		"svl.q R000, 12*48 + 8+12(%0)\n"
		"svr.q R000, 13*48 + 8   (%0)\n"
		"svl.q R000, 14*48 +12+12(%0)\n"
		"svr.q R000, 15*48 +12   (%0)\n"

	::"r"(&savebuf[0][4]), "r"(&ref_buffer[4]) : "memory");

	CHECK_MEMREG("svl.q off= 0", savebuf[ 0], 0xdead0404, 0x00000000, 0x00000000, 0x00000000, 0, 0, 0, 0);
	CHECK_MEMREG("svr.q off= 0", savebuf[ 1], 0xdead0101, 0xdead0202, 0xdead0303, 0xdead0404, 0, 0, 0, 0);
	CHECK_MEMREG("svl.q off= 4", savebuf[ 2], 0xdead0303, 0xdead0404, 0x00000000, 0x00000000, 0, 0, 0, 0);
	CHECK_MEMREG("svr.q off= 4", savebuf[ 3], 0x00000000, 0xdead0101, 0xdead0202, 0xdead0303, 0, 0, 0, 0);
	CHECK_MEMREG("svl.q off= 8", savebuf[ 4], 0xdead0202, 0xdead0303, 0xdead0404, 0x00000000, 0, 0, 0, 0);
	CHECK_MEMREG("svr.q off= 8", savebuf[ 5], 0x00000000, 0x00000000, 0xdead0101, 0xdead0202, 0, 0, 0, 0);
	CHECK_MEMREG("svl.q off=12", savebuf[ 6], 0xdead0101, 0xdead0202, 0xdead0303, 0xdead0404, 0, 0, 0, 0);
	CHECK_MEMREG("svr.q off=12", savebuf[ 7], 0x00000000, 0x00000000, 0x00000000, 0xdead0101, 0, 0, 0, 0);

	CHECK_MEMREG("svl.q off= 0+12", savebuf[ 8], 0xdead0101, 0xdead0202, 0xdead0303, 0xdead0404, 0, 0, 0, 0);
	CHECK_MEMREG("svr.q off= 0",    savebuf[ 9], 0xdead0101, 0xdead0202, 0xdead0303, 0xdead0404, 0, 0, 0, 0);
	CHECK_MEMREG("svl.q off= 4+12", savebuf[10], 0, 0x00000000, 0x00000000, 0x00000000, 0xdead0404, 0, 0, 0);
	CHECK_MEMREG("svr.q off= 4",    savebuf[11], 0, 0xdead0101, 0xdead0202, 0xdead0303, 0x00000000, 0, 0, 0);
	CHECK_MEMREG("svl.q off= 8+12", savebuf[12], 0, 0, 0x00000000, 0x00000000, 0xdead0303, 0xdead0404, 0, 0);
	CHECK_MEMREG("svr.q off= 8",    savebuf[13], 0, 0, 0xdead0101, 0xdead0202, 0x00000000, 0x00000000, 0, 0);
	CHECK_MEMREG("svl.q off=12+12", savebuf[14], 0, 0, 0, 0x00000000, 0xdead0202, 0xdead0303, 0xdead0404, 0);
	CHECK_MEMREG("svr.q off=12",    savebuf[15], 0, 0, 0, 0xdead0101, 0x00000000, 0x00000000, 0x00000000, 0);

	// Validates that lvl/lvr do not care about 1-3 byte misalignment
	char *unal1ptr = ((char*)&ref_buffer[4]) + 3;
	asm volatile (
		"vmzero.q M000\n"
		"lvl.q R000, 0(%1)\n"
		"lvr.q R001, 0(%1)\n"
		"sv.q  R000, 0(%0)\n"
		"sv.q  R001,16(%0)\n"
	::"r"(tmpbuf), "r"(unal1ptr) : "memory");

	// Even tho the pointer was misaligned by one byte it didn't matter to lvl/lvr
	CHECK_OUTPUT("lvl.q off= 3", &tmpbuf[ 0], 0x00000000, 0x00000000, 0x00000000, 0xdead0101);
	CHECK_OUTPUT("lvr.q off= 3", &tmpbuf[ 4], 0xdead0101, 0xdead0202, 0xdead0303, 0xdead0404);

	// Validates that svl/svr do not care about 1-3 byte misalignment either
	char *saveunal1ptr = ((char*)&savebuf2[0][4]) + 3;
	asm volatile (
		"lv.q R000, 0(%1)\n"     // Load some useful data to write

		"svl.q R000,  0(%0)\n"
		"svr.q R001, 48(%0)\n"
	::"r"(saveunal1ptr),"r"(&ref_buffer[4]) : "memory");

	CHECK_MEMREG("svl.q off= 3", savebuf2[0], 0xdead0404, 0x00000000, 0x00000000, 0x00000000, 0, 0, 0, 0);
	CHECK_MEMREG("svr.q off= 3", savebuf2[1], 0xdead0101, 0xdead0202, 0xdead0303, 0xdead0404, 0, 0, 0, 0);

	return errcnt;
}

// Validates that lv.q/sv.q instructions require 16 byte aligned addresses.
// Otherwise they do generate an exception.

#define _check_unaligned_access(instr, refptr, enumber) {                     \
  /* Setup exception */                                                       \
  memset(ecb, 0, sizeof(*ecb));                                               \
  ecb->magic[0] = MAGIC_VAL_1;                                                \
  ecb->magic[1] = MAGIC_VAL_2;                                                \
  ecb->magic[2] = MAGIC_VAL_3;                                                \
  ecb->magic[3] = MAGIC_VAL_4;                                                \
  ecb->armed    = 1;                                                          \
  asm volatile(                                                               \
    "la $v0, 1f\n"                                                            \
    "sw $v0, 0(%0)\n"                                                         \
    "nop; nop; nop\n"                                                         \
    "1:\n " instr "\n"                                                        \
    "nop; nop; nop; nop\n"                                                    \
  ::"r"(&ecb->expected_epc), "r"((refptr)) : "$v0", "memory");                \
  ecb->armed    = 0;   /* Disable exception catcher */                        \
  /* Validate that the exception did indeed occur */                          \
  if (ecb->exception_count != 1)                                              \
    FILL_ERR("Unaligned memory access not raised! " instr,                    \
                                                 ecb->exception_count, 1);    \
  /* Ensure it is an invalid instruction type */                              \
  unsigned extype = GET_EX_CAUSE(ecb->state.cause);                           \
  if (extype != enumber)                                                      \
    FILL_ERR("Invalid exception raised! " instr, extype, enumber);            \
}

int check_aligned_vfpu_memops_exception(struct check_error_info *errs,
                                        exception_control_block *ecb) {
	int errcnt = 0;
	if (!ecb)
		return 0;

	char *bufptr = (char*)tmpbuf;
	for (unsigned unal = 1; unal < 16; unal++)
		_check_unaligned_access("lv.q R000, 0(%1)", &bufptr[unal], EX_MEM_ADD_LD_ERR);
	for (unsigned unal = 1; unal < 16; unal++)
		_check_unaligned_access("sv.q R000, 0(%1)", &bufptr[unal], EX_MEM_ADD_ST_ERR);

	return errcnt;
}

