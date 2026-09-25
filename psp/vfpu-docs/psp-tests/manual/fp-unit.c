
// Copyright 2023 David Guillen Fandos <david@davidgf.net>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "test-utils.h"

// Tests some features of the Allegrex FPU (COP1) unit.
// Confirms the facts that:
// - There's a one cycle hazard between c.xx.s -> bc1t/f instructions.
// - There's no hazard between c.xx.s -> cfc1 instructions.
// - There's no hazard between ctc1 -> bc1t/f instructions.
// TLDR cfc1/ctc1 present no hazards, but compare/branch do.

#define FILL_ERR(errmsg, v, ve)  \
{                                \
  errs[errcnt].msg = (errmsg);   \
  errs[errcnt].value = (v);      \
  errs[errcnt].valueex = (ve);   \
  errcnt++;                      \
}

// Since we play with hazards and undefined behaviour, we need to repeat tests
// to ensure we didn't just "get lucky" with the result.
#define NUM_RND_TESTS       256

int check_allegrex_fpu(struct check_error_info *errs) {
	int errcnt = 0;

	float opa[NUM_RND_TESTS], opb[NUM_RND_TESTS];
	char randbits[NUM_RND_TESTS];
	for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
		opa[ntest] = rand();
		opb[ntest] = rand();
		randbits[ntest] = rand() & 1;
	}

	// Validates the fact that FPU unit has one cycle hazard between compare
	// and jump instructions. Reading the bit immediately after might result in
	// an invalid value (reading the previous value).
	{
		struct {
			char res_ref[NUM_RND_TESTS];
			char res0[NUM_RND_TESTS];
			char res1[NUM_RND_TESTS];
		} t;
		memset(&t, ~0, sizeof(t));

		// Reference value
		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			asm volatile (
				".set noreorder\n"
				"c.lt.s %1, %2\n"
				"nop; nop; nop\n"   // Seven cycles should do it (~ pipeline length?)
				"nop; nop; nop; nop\n"
				"bc1t 1f; nop\n"
				"sb $0, (%0)\n"     // False
				"1:\n"
			:: "r"(&t.res_ref[ntest]), "f"(opa[ntest]), "f"(opb[ntest]) : "memory");
		}
		// Test 0 cycles (produces invalid results)
		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			asm volatile (
				".set noreorder\n"
				"c.lt.s %1, %2\n"
				"bc1t 1f; nop\n"
				"sb $0, (%0)\n"     // False
				"1:\n"
			:: "r"(&t.res0[ntest]), "f"(opa[ntest]), "f"(opb[ntest]) : "memory");
		}
		// Test 1 cycle (produces the correct result)
		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			asm volatile (
				".set noreorder\n"
				"c.lt.s %1, %2\n"
				"nop\n"
				"bc1t 1f; nop\n"
				"sb $0, (%0)\n"     // False
				"1:\n"
			:: "r"(&t.res1[ntest]), "f"(opa[ntest]), "f"(opb[ntest]) : "memory");
		}

		// Check that one cycle delay is enough (matches the 7 cycle delay)
		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			if (t.res1[ntest] != t.res_ref[ntest]) {
				FILL_ERR("c.lt.s / bc1t presents a 2+ cycle hazard!", t.res1[ntest], t.res_ref[ntest]);
				break;
			}
		}

		// Now check that there's hazards for 0 cycles (at least a few mismatches)
		unsigned mismcnt = 0;
		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			if (t.res0[ntest] != t.res_ref[ntest])
				mismcnt++;
		}
		if (mismcnt == 0)
			FILL_ERR("c.lt.s / bc1t doesn't present a 0-cycle hazard!", 0, mismcnt);
	}

	// Validate that this is not the case for cfc1 instruction (there's no hazard for it).
	{
		uint32_t res_ref[NUM_RND_TESTS], res[NUM_RND_TESTS];

		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			asm volatile (
				".set noreorder\n"
				"c.lt.s %1, %2\n"
				"nop; nop; nop\n"   // Seven cycles should do it (~ pipeline length?)
				"nop; nop; nop; nop\n"
				"cfc1 $v0, $31\n"
				"sw $v0, (%0)\n"
			:: "r"(&res_ref[ntest]), "f"(opa[ntest]), "f"(opb[ntest]) : "$v0", "memory");
		}
		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			asm volatile (
				".set noreorder\n"
				"c.lt.s %1, %2\n"
				"cfc1 $v0, $31\n"
				"sw $v0, (%0)\n"
			:: "r"(&res[ntest]), "f"(opa[ntest]), "f"(opb[ntest]) : "$v0", "memory");
		}

		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			// Check the bit 24 (CC) for mismatches
			if ((res[ntest] & 0x00800000) != (res_ref[ntest] & 0x00800000)) {
				FILL_ERR("c.lt.s / cfc1 presents a hazard!", res[ntest], res_ref[ntest]);
				break;
			}
		}
	}

	// Validate that branch instructions can read the result of ctc1 immediately.
	{
		char res0[NUM_RND_TESTS];
		for (unsigned i = 0; i < NUM_RND_TESTS; i++)
			res0[i] = 1;

		// Test 0 cycles
		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			asm volatile (
				".set noreorder\n"

				"cfc1 $v0, $31\n"
				"la   $v1, ~0x00800000\n"
				"and  $v0, $v0, $v1\n"
				"sll  $v1, %1, 23\n"
				"or   $v0, $v1, $v0\n"

				"ctc1 $v0, $31\n"        // Load the bit to FCR
				"bc1t 1f; nop\n"

				"sb $0, (%0)\n"     // False
				"1:\n"
			:: "r"(&res0[ntest]), "r"(randbits[ntest]) : "$v0", "$v1", "memory");
		}

		// Check that one cycle delay is enough (matches the 7 cycle delay)
		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			if (res0[ntest] != randbits[ntest]) {
				FILL_ERR("ctc1 / bc1t presents a 1+ cycle hazard!", res0[ntest], randbits[ntest]);
				break;
			}
		}
	}

	// Validate mfc1 hazards. Instructions immediately following an mfc1
	// can read the destination register of said mfc1.
	{
		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			uint32_t gprd1, gprd2;
			union {
				float f32;
				uint32_t u32;
			} realres;
			asm volatile (
				".set noreorder\n"
				"add.s %2, %3, %4\n"
				"mfc1 $v0, %2\n"
				"move %0, $v0\n"
				"move %1, $v0\n"
			: "=r"(gprd1), "=r"(gprd2), "=f"(realres.f32)
			: "f"(opa[ntest]), "f"(opb[ntest]) : "$v0");

			if (realres.u32 != gprd1) {
				FILL_ERR("mfc1 GPR[rt] presents a 1 cycle hazard!", gprd1, realres.u32);
				break;
			}
			if (realres.u32 != gprd2) {
				FILL_ERR("mfc1 GPR[rt] presents a 2 cycle hazard!", gprd2, realres.u32);
				break;
			}
		}
	}

	// Validate mtc1 hazards. Instructions immediately following an mtc1
	// can read the newly written value. The pipeline has proper interlocks.
	{
		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			union {
				float f32;
				uint32_t u32;
			} fr1[NUM_RND_TESTS], fr2[NUM_RND_TESTS];

			asm volatile (
				".set noreorder\n"
				"mtc1 %1, $f1\n"
				"mtc1 %2, $f2\n"
				"add.s %0, $f2, $f1\n"
			: "=f"(fr1[ntest].f32)
			: "r"(opa[ntest]), "r"(opb[ntest]) : "$f1", "$f2");

			asm volatile (
				".set noreorder\n"
				"mtc1 %1, $f1\n"
				"mtc1 %2, $f2\n"
				"nop; nop; nop; nop\n"
				"add.s %0, $f2, $f1\n"
			: "=f"(fr2[ntest].f32)
			: "r"(opa[ntest]), "r"(opb[ntest]) : "$f1", "$f2");

			if (fr1[ntest].u32 != fr2[ntest].u32) {
				FILL_ERR("mtc1 FPR[fs] presents a 1 cycle hazard!", fr1[ntest].u32, fr2[ntest].u32);
				break;
			}
		}
	}

	// Validate lwc1 hazards. Instructions immediately following an lwc1
	// can read the newly written value. The pipeline has proper interlocks.
	{
		for (unsigned ntest = 0; ntest < NUM_RND_TESTS; ntest++) {
			union {
				float f32;
				uint32_t u32;
			} fr1[NUM_RND_TESTS], fr2[NUM_RND_TESTS];

			asm volatile (
				".set noreorder\n"
				"lwc1 $f1, (%1)\n"
				"lwc1 $f2, (%2)\n"
				"add.s %0, $f2, $f1\n"
			: "=f"(fr1[ntest].f32)
			: "r"(&opa[ntest]), "r"(&opb[ntest]) : "$f1", "$f2");

			asm volatile (
				".set noreorder\n"
				"lwc1 $f1, (%1)\n"
				"lwc1 $f2, (%2)\n"
				"nop; nop; nop; nop\n"
				"add.s %0, $f2, $f1\n"
			: "=f"(fr2[ntest].f32)
			: "r"(&opa[ntest]), "r"(&opb[ntest]) : "$f1", "$f2");

			if (fr1[ntest].u32 != fr2[ntest].u32) {
				FILL_ERR("lwc1 FPR[fs] presents a 1 cycle hazard!", fr1[ntest].u32, fr2[ntest].u32);
				break;
			}
		}
	}

	return errcnt;
}

