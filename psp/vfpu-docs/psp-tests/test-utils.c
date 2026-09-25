
#include <math.h>
#include <psputils.h>

#include "test-utils.h"

FILE * logfd = NULL;

void fill_errinfo(
	struct test_error_info *errinfo,
	const struct test_decl_info *iinfo,
	void *rdex, void *rd)
{
	memset(errinfo, 0, sizeof(*errinfo));
	errinfo->inst = iinfo->inst;
	errinfo->instw = iinfo->instw;
	errinfo->rdsz = iinfo->rdoutsz;
	errinfo->pfxw = iinfo->pfxw;
	// Copy results (expected and produced)
	memcpy(errinfo->rdex, rdex, 4*iinfo->rdoutsz);
	memcpy(errinfo->rd, rd, 4*iinfo->rdoutsz);
	// Copy input info too
	for (unsigned i = 0; i < 2; i++) {
		errinfo->inputs[i].iname = iinfo->inputs[i].inpname;
		errinfo->inputs[i].size  = iinfo->inputs[i].size;
	}
}

void gen_full_rand(unsigned seed, uint32_t *data, unsigned cnt) {
	uint64_t st = seed;
	for (unsigned i = 0; i < cnt; i++) {
		st = 25214903917 * st + 11;
		st &= 0xffffffffffffULL;
		*data++ = (uint32_t)(st >> 16);
	}
}

float gen_input_f32_(unsigned int n) {
	// Produce some interesting floating point numbers.
	const float picks[] = {
		0.0f, -0.0f, 1.0f, -1.0f, 0.5f, 2.0f, 0.125f, -12.0f,
		1000.f, -1234.5678f, 3.14159265359f, -3.14159265359f,
		3e-8, -3e8, -1e-30, 4294967296.0f,
	};

	if (n < 16)
		return picks[n];
	return ((n & 1) ? 1 : -1) * (float)(n*4) + 1.0f / ((n & 7) * 8 + 1);
}

float gen_input_f32(unsigned int n) {
	// Produce some interesting floating point numbers.
	const float picks[] = {
		0.0f, -0.0f, 1.0f, -1.0f, 0.5f, 2.0f, 0.125f, -4.0f,
		5.f, -6.5f, 3.125f, -3.125f,
		8.0f, -8.0f, 7.75f, -9.5f,
	};
	if (n < 16)
		return picks[n & 15];

	return picks[(7*n) & 15];
}

unsigned gen_input_bit(unsigned int n) {
	uint64_t st = 0;
	for (unsigned i = 0; i < n; i++) {
		st = 25214903917 * st + 11;
		st &= 0xffffffffffffULL;
	}
	return (st >> 16) & 1;
}

unsigned gen_input_u32(unsigned int n) {
	// Produce some interesting floating point numbers.
	const unsigned picks[] = {
		0, 1, 0xFFFFFFFF, 0xFFFFFFFE,
		2, 3, 0xFFFFFFFD, 0xFFFFFFFC,
		0x80000000, 0x81000000, 0xFF000000, 0x7F000000,
		0x7ff00000, 0x7f800000, 0x7fc00000, 0x7fe00000,
		0xfff00000, 0xff800000, 0xffc00000, 0xffe00000,
		0x8000001, 0x7FFFFFF, 0xAA55AA55, 0xAA55AA55,
	};
	if (n < 6*4)
		return picks[n];
	return ((n & 1) ? 0 : 0x80000000) ^ (n * 0xDEADBEEF) ^ n;
}

void gen_inputs_f32(unsigned tn, float *dst, unsigned count) {
	while (count--)
		*dst++ = gen_input_f32(tn + count);
}

void gen_inputs_d32(unsigned tn, double *dst, unsigned count) {
	while (count--)
		*dst++ = gen_input_f32(tn + count);
}

void gen_inputs_u32(unsigned tn, unsigned *dst, unsigned count) {
	while (count--)
		*dst++ = gen_input_u32(tn + count);
}

void gen_inputs_s32(unsigned tn, int *dst, unsigned count) {
	while (count--)
		*dst++ = (signed)gen_input_u32(tn + count);
}

void gen_inputs_bits(unsigned tn, unsigned *dst, unsigned count) {
	while (count--)
		*dst++ = gen_input_bit(tn + count);
}

unsigned pack6(unsigned *bits) {
	unsigned ret = 0;
	for (unsigned i = 0; i < 6; i++)
		ret |= (bits[i] ? 1 : 0) << i;
	return ret;
}



// Validation function to check for valid resuts (VFPU vs FPU)
// This checks for equality and returns 1 on differences
int fpdif(float a, float b) {
	union {
		float fop[2];
		unsigned uop[2];
	} args;
	args.fop[0] = a;
	args.fop[1] = b;

	if (a == b)
		return 0;

	// VFPU has some weird NaN signs
	if (isnan(a) && isnan(b))
		return 0;

	// Minus and plus zero are equivalent. The way dot/avg/fad operations work
	// can end up producing a different sign for zero in some cases.
	if ((args.uop[0] == 0 || args.uop[0] == 0x80000000) && 
	    (args.uop[1] == 0 || args.uop[1] == 0x80000000))
	    return 0;

	// Check for rounding differences (-/+ mantissa unit)
	//if ((args.uop[0] & 0xFF800000) == (args.uop[1] & 0xFF800000)) {
	//	unsigned m1 = args.uop[0] & 0x7FFFFF;
	//	unsigned m2 = args.uop[1] & 0x7FFFFF;
	//	if ((m1 - m2) <= 1 || (m2 - m1) <= 1)
	//		return 0;
	//}

	if ((args.uop[0] & 0xFF800000) == (args.uop[1] & 0xFF800000)) {
		// Could be a rounding issue, check it out.
		// We shift mantissas deleting trailing zeros (not 100% correct)
		// to go back to the mantissa alignment point (pretty much guessing)
		unsigned m1 = args.uop[0] & 0x7FFFFF;
		unsigned m2 = args.uop[1] & 0x7FFFFF;
		unsigned tz1 = m1 ? __builtin_ctz(m1) : 0;
		unsigned tz2 = m2 ? __builtin_ctz(m2) : 0;
		unsigned common_shift = tz1 < tz2 ? tz1 : tz2;
		m1 >>= common_shift;
		m2 >>= common_shift;
		// Now here there could be a +/- 0.5 difference due to rounding
		if ((m1 - m2) <= 1 || (m2 - m1) <= 1)
			return 0;
	}

	return 1;
}

int fprelerr(float ref, float val, float maxerr) {
	return (fabsf((ref - val) / val) > maxerr);
}

int fpabserr(float ref, float val, float maxerr) {
	return (fabsf(ref - val) > maxerr);
}

int fpdifn(const float *a, const float *b, unsigned n) {
	int ret = 0;
	while (n--)
		ret |= fpdif(*a++, *b++);
	return ret;
}

int fprelerrn(const float *ref, const float *val, float maxerr, unsigned n) {
	int ret = 0;
	while (n--)
		ret |= fprelerr(*ref++, *val++, maxerr);
	return ret;
}

int fpabserrn(const float *ref, const float *val, float maxerr, unsigned n) {
	int ret = 0;
	while (n--)
		ret |= fpabserr(*ref++, *val++, maxerr);
	return ret;
}

void platform_cache_sync(void *baseaddr, unsigned size) {
	char *ptr = (char*)baseaddr;
	for (unsigned i = 0; i < size; i += 128) {
		asm (
			"cache 0x1A,  0(%0)\n"
			"cache 0x1A, 32(%0)\n"
			"cache 0x1A, 64(%0)\n"
			"cache 0x1A, 96(%0)\n"
			"cache 0x08,  0(%0)\n"
			"cache 0x08, 32(%0)\n"
			"cache 0x08, 64(%0)\n"
			"cache 0x08, 96(%0)\n"
		::"r"(ptr):"memory");
		ptr += 128;
	}
}

void save_all_vfpu_regs(float *outv) {
	// Store them in reg number order
	asm (
		"sv.q R000,  0*16(%0)\n"
		"sv.q R100,  1*16(%0)\n"
		"sv.q R200,  2*16(%0)\n"
		"sv.q R300,  3*16(%0)\n"
		"sv.q R400,  4*16(%0)\n"
		"sv.q R500,  5*16(%0)\n"
		"sv.q R600,  6*16(%0)\n"
		"sv.q R700,  7*16(%0)\n"
		"sv.q R001,  8*16(%0)\n"
		"sv.q R101,  9*16(%0)\n"
		"sv.q R201, 10*16(%0)\n"
		"sv.q R301, 11*16(%0)\n"
		"sv.q R401, 12*16(%0)\n"
		"sv.q R501, 13*16(%0)\n"
		"sv.q R601, 14*16(%0)\n"
		"sv.q R701, 15*16(%0)\n"
		"sv.q R002, 16*16(%0)\n"
		"sv.q R102, 17*16(%0)\n"
		"sv.q R202, 18*16(%0)\n"
		"sv.q R302, 19*16(%0)\n"
		"sv.q R402, 20*16(%0)\n"
		"sv.q R502, 21*16(%0)\n"
		"sv.q R602, 22*16(%0)\n"
		"sv.q R702, 23*16(%0)\n"
		"sv.q R003, 24*16(%0)\n"
		"sv.q R103, 25*16(%0)\n"
		"sv.q R203, 26*16(%0)\n"
		"sv.q R303, 27*16(%0)\n"
		"sv.q R403, 28*16(%0)\n"
		"sv.q R503, 29*16(%0)\n"
		"sv.q R603, 30*16(%0)\n"
		"sv.q R703, 31*16(%0)\n"
	::"r"(outv):"memory");
}

void load_all_vfpu_regs(float *outv) {
	// Store them in reg number order
	asm (
		"lv.q R000,  0*16(%0)\n"
		"lv.q R100,  1*16(%0)\n"
		"lv.q R200,  2*16(%0)\n"
		"lv.q R300,  3*16(%0)\n"
		"lv.q R400,  4*16(%0)\n"
		"lv.q R500,  5*16(%0)\n"
		"lv.q R600,  6*16(%0)\n"
		"lv.q R700,  7*16(%0)\n"
		"lv.q R001,  8*16(%0)\n"
		"lv.q R101,  9*16(%0)\n"
		"lv.q R201, 10*16(%0)\n"
		"lv.q R301, 11*16(%0)\n"
		"lv.q R401, 12*16(%0)\n"
		"lv.q R501, 13*16(%0)\n"
		"lv.q R601, 14*16(%0)\n"
		"lv.q R701, 15*16(%0)\n"
		"lv.q R002, 16*16(%0)\n"
		"lv.q R102, 17*16(%0)\n"
		"lv.q R202, 18*16(%0)\n"
		"lv.q R302, 19*16(%0)\n"
		"lv.q R402, 20*16(%0)\n"
		"lv.q R502, 21*16(%0)\n"
		"lv.q R602, 22*16(%0)\n"
		"lv.q R702, 23*16(%0)\n"
		"lv.q R003, 24*16(%0)\n"
		"lv.q R103, 25*16(%0)\n"
		"lv.q R203, 26*16(%0)\n"
		"lv.q R303, 27*16(%0)\n"
		"lv.q R403, 28*16(%0)\n"
		"lv.q R503, 29*16(%0)\n"
		"lv.q R603, 30*16(%0)\n"
		"lv.q R703, 31*16(%0)\n"
	::"r"(outv):"memory");
}

