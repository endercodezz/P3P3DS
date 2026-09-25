
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "test-utils.h"

#define RET_ERR2(instname, opmtx, opv, resv, expecv) { \
    memset(&errlist[errcnt], 0, sizeof(errlist[errcnt])); \
    errlist[errcnt].inst = (instname); \
    errlist[errcnt].inputs[0].iname = "matrix-2x2"; \
    errlist[errcnt].inputs[1].iname = "vector-2"; \
    errlist[errcnt].inputs[0].size = 4; \
    memcpy(errlist[errcnt].inputs[0].values, opmtx, sizeof(float)*2*2); \
    errlist[errcnt].inputs[1].size = 2; \
    memcpy(errlist[errcnt].inputs[1].values, opmtx, sizeof(float)*2); \
    errlist[errcnt].rdsz = 2; \
    memcpy(errlist[errcnt].rd, resv, sizeof(float)*2); \
    memcpy(errlist[errcnt].rdex, expecv, sizeof(float)*2); \
    errcnt++; \
}


// Validates the vhftm2/3 VFPU bug. This causes some output registers
// to behave incorrectly, overwriting the correct result but in a different
// position within the row/col register.
// Whenever a register above 64 is used, the output is shifted by one column/row


int assert_vhtfm_bugs(struct test_error_info *errlist) {
	int errcnt = 0;
	const float mtx2[4] = { 1.0f, 4.0f, -3.0f, 1.5f };
	const float vin2[2] = { 1.25f, -3.5f };

	// vhtfm2.p  When output is RX0Y, result is as expected (SX0Y, SX1Y)
	//                       is CXY0, result is as expected (SXY0, SXY1)
	//           When output is RX20, result gets placed in (SX1Y, SX2Y), instead of SX2Y, SX3Y
	//           When output is CX20, result gets placed in (SXY1, SXY2), instead of SXY2, SXY3


	// Expected valued (for the full row)
	const float vtfm2_expected[4] = {11.75f, -0.25f, 0, 0};
	const float vtfm2_expected_shifted[4] = {0, 0, 11.75f, -0.25f};
	const float vhtfm2_expected[4] = {-1.75f, 6.5f, 0, 0};
	const float vhtfm2_expected_bugged[4] = {0, -1.75f, 6.5f, 0};  // BUG: shifted by one row

	float outtfm[4], outtfm_nobug[4], outhtfm[4], outhtfm_bug_row[4], outhtfm_bug_col[4];

	// Validate reading from CC regs
	asm (
		"vmzero.q  M200\n"
		"vmzero.q  M300\n"
		"vmzero.q  M400\n"
		"vmzero.q  M500\n"
		"vmzero.q  M600\n"

		"lv.s  S000,  0(%5)\n"
		"lv.s  S010,  4(%5)\n"
		"lv.s  S001,  8(%5)\n"
		"lv.s  S011, 12(%5)\n"

		"lv.s  S100,  0(%6)\n"
		"lv.s  S110,  4(%6)\n"

		"vtfm2.p   R200, M000, R100\n"  // This one is a regular transform
		"vtfm2.p   R320, M000, R100\n"  // This is the same as above

		"vhtfm2.p  R400, M000, R100\n"  // This one works just fine
		"vhtfm2.p  R520, M000, R100\n"  // This one writes elements shifted by one position (BUG)
		"vhtfm2.p  C602, M000, R100\n"  // Same BUG but in a column register

		"sv.s  S200,  0(%0)\n"
		"sv.s  S210,  4(%0)\n"
		"sv.s  S220,  8(%0)\n"
		"sv.s  S230, 12(%0)\n"

		"sv.s  S300,  0(%1)\n"
		"sv.s  S310,  4(%1)\n"
		"sv.s  S320,  8(%1)\n"
		"sv.s  S330, 12(%1)\n"

		"sv.s  S400,  0(%2)\n"
		"sv.s  S410,  4(%2)\n"
		"sv.s  S420,  8(%2)\n"
		"sv.s  S430, 12(%2)\n"

		"sv.s  S500,  0(%3)\n"
		"sv.s  S510,  4(%3)\n"
		"sv.s  S520,  8(%3)\n"
		"sv.s  S530, 12(%3)\n"

		"sv.s  S600,  0(%4)\n"
		"sv.s  S601,  4(%4)\n"
		"sv.s  S602,  8(%4)\n"
		"sv.s  S603, 12(%4)\n"
	::"r"(outtfm), "r"(outtfm_nobug), "r"(outhtfm), "r"(outhtfm_bug_row), "r"(outhtfm_bug_col),
	  "r"(mtx2), "r"(vin2) :"memory");

	// outtfm must match vtfm2_expected
	if (memcmp(outtfm, vtfm2_expected, sizeof(vtfm2_expected)))
		RET_ERR2("vtfm2.p", mtx2, vin2, outtfm, vtfm2_expected);

	// outtfm_nobug must match vtfm2_expected_shifted (since we write at col #2)
	if (memcmp(outtfm_nobug, vtfm2_expected_shifted, sizeof(vtfm2_expected_shifted)))
		RET_ERR2("vtfm2.p", mtx2, vin2, outtfm_nobug, vtfm2_expected_shifted);

	// outhtfm must match vhtfm2_expected
	if (memcmp(outhtfm, vhtfm2_expected, sizeof(vhtfm2_expected)))
		RET_ERR2("vhtfm2.p", mtx2, vin2, outhtfm, vhtfm2_expected);

	// outhtfm_bug_row must match vhtfm2_expected_bugged
	if (memcmp(outhtfm_bug_row, vhtfm2_expected_bugged, sizeof(vhtfm2_expected_bugged)))
		RET_ERR2("vhtfm2.p", mtx2, vin2, outhtfm_bug_row, vhtfm2_expected_bugged);

	// same for the column pattern
	if (memcmp(outhtfm_bug_col, vhtfm2_expected_bugged, sizeof(vhtfm2_expected_bugged)))
		RET_ERR2("vhtfm2.p", mtx2, vin2, outhtfm_bug_col, vhtfm2_expected_bugged);




	// vhtfm3.t  When output is RX0Y, result is as expected (SX0Y, SX1Y, SX2Y)
	//                       is CXY0, result is as expected (SXY0, SXY1, SXY2)
	//           When output is RX20, result gets placed in (SX2Y, SX3Y, SX0Y), instead of SX1Y, SX2Y, SX3Y
	//           When output is CX20, result gets placed in (SXY2, SXY3, SXY0), instead of SXY1, SXY2, SXY3


	// Repeating for .t mode, similar behaviour
	const float mtx3[9] = { 1.0f, 4.0f, 0.1875f, -3.0f, 1.5f, -2.0f, 2.0f, 0.125f, -0.625f };
	const float vin3[3] = { 1.25f, -2.75f, 1.125f };

	// Expected valued (for the full row)
	const float vtfm3_expected[4] = {11.75f, 1.015625f, 5.03125f, 0};
	const float vtfm3_expected_shifted[4] = {0, 11.75f, 1.015625f, 5.03125f};
	const float vhtfm3_expected[4] = {11.5f, 1.0f, 5.109375f, 0};
	const float vhtfm3_expected_bugged[4] = {5.109375f, 0, 11.5f, 1.0f};  // BUG: shifted by one row

	// Validate reading from CC regs
	asm (
		"vmzero.q  M200\n"
		"vmzero.q  M300\n"
		"vmzero.q  M400\n"
		"vmzero.q  M500\n"
		"vmzero.q  M600\n"

		"lv.s  S000,  0(%5)\n"
		"lv.s  S010,  4(%5)\n"
		"lv.s  S020,  8(%5)\n"
		"lv.s  S001, 12(%5)\n"
		"lv.s  S011, 16(%5)\n"
		"lv.s  S021, 20(%5)\n"
		"lv.s  S002, 24(%5)\n"
		"lv.s  S012, 28(%5)\n"
		"lv.s  S022, 32(%5)\n"

		"lv.s  S100,  0(%6)\n"
		"lv.s  S110,  4(%6)\n"
		"lv.s  S120,  8(%6)\n"

		"vtfm3.t   R200, M000, R100\n"  // This one is a regular transform
		"vtfm3.t   R310, M000, R100\n"  // This is the same as above

		"vhtfm3.t  R400, M000, R100\n"  // This one works just fine
		"vhtfm3.t  R510, M000, R100\n"  // This one writes elements shifted by one position (BUG)
		"vhtfm3.t  C601, M000, R100\n"  // Same but for column patterns

		"sv.s  S200,  0(%0)\n"
		"sv.s  S210,  4(%0)\n"
		"sv.s  S220,  8(%0)\n"
		"sv.s  S230, 12(%0)\n"

		"sv.s  S300,  0(%1)\n"
		"sv.s  S310,  4(%1)\n"
		"sv.s  S320,  8(%1)\n"
		"sv.s  S330, 12(%1)\n"

		"sv.s  S400,  0(%2)\n"
		"sv.s  S410,  4(%2)\n"
		"sv.s  S420,  8(%2)\n"
		"sv.s  S430, 12(%2)\n"

		"sv.s  S500,  0(%3)\n"
		"sv.s  S510,  4(%3)\n"
		"sv.s  S520,  8(%3)\n"
		"sv.s  S530, 12(%3)\n"

		"sv.s  S600,  0(%4)\n"
		"sv.s  S601,  4(%4)\n"
		"sv.s  S602,  8(%4)\n"
		"sv.s  S603, 12(%4)\n"
	::"r"(outtfm), "r"(outtfm_nobug), "r"(outhtfm), "r"(outhtfm_bug_row), "r"(outhtfm_bug_col),
	  "r"(mtx3), "r"(vin3) :"memory");

	// outtfm must match vtfm3_expected
	if (memcmp(outtfm, vtfm3_expected, sizeof(vtfm3_expected)))
		RET_ERR2("vtfm3.t", mtx3, vin3, outtfm, vtfm3_expected);

	// outtfm_nobug must match vtfm2_expected_shifted (since we write at col #2)
	if (memcmp(outtfm_nobug, vtfm3_expected_shifted, sizeof(vtfm3_expected_shifted)))
		RET_ERR2("vtfm3.t", mtx3, vin3, outtfm_nobug, vtfm3_expected_shifted);

	// outhtfm must match vhtfm2_expected
	if (memcmp(outhtfm, vhtfm3_expected, sizeof(vhtfm3_expected)))
		RET_ERR2("vhtfm3.t", mtx3, vin3, outhtfm, vhtfm3_expected);

	// outhtfm_bug_row must match vhtfm2_expected_bugged
	if (memcmp(outhtfm_bug_row, vhtfm3_expected_bugged, sizeof(vhtfm3_expected_bugged)))
		RET_ERR2("vhtfm3.t", mtx3, vin3, outhtfm_bug_row, vhtfm3_expected_bugged);

	// again same output (check column pattern)
	if (memcmp(outhtfm_bug_col, vhtfm3_expected_bugged, sizeof(vhtfm3_expected_bugged)))
		RET_ERR2("vhtfm3.t", mtx3, vin3, outhtfm_bug_col, vhtfm3_expected_bugged);

	logprintf("VFPU bugs finished with %d unexpected errors\n", errcnt);
	return errcnt;
}


