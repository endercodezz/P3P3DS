
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <pspdebug.h>

#define REG_PAIRS_DEBUGS    512

struct test_input_info {
	const char *iname;       // Input/reg name
	unsigned int size;       // Used element count
	unsigned int values[16]; // Data
};

struct test_error_info {
	const char *inst;       // Human readable instruction
	unsigned int instw;     // Instruction word
	unsigned int pfxw;      // Prefix instruction

	// Input/Output data
	struct test_input_info inputs[2];

	unsigned int rdsz;       // Output register size
	unsigned int rd[16];     // Output registers
	unsigned int rdex[16];   // Expected outputs
};

struct test_decl_info {
	const char *inst;       // Human readable instruction
	unsigned int instw;     // Instruction word
	unsigned int pfxw;      // Prefix instruction
	unsigned int rdoutsz;   // Size output
	struct {
		const char *inpname; // Input name
		unsigned int size;   // Num elements
	} inputs[2];
};

struct check_error_info {
	const char *msg;         // Human readable error
	unsigned int value;
	unsigned int valueex;
};

struct prefix_usage_info {
	const char *inst_name;     // Instruction name
	// Prefix result bits
	unsigned int rs_tested, rt_tested, rd_tested;
	unsigned int rs_errors, rt_errors, rd_errors;
	// Reported errors
	struct test_error_info rs_swz_err, rs_neg_err, rs_abs_err, rs_cst_err;
	struct test_error_info rt_swz_err, rt_neg_err, rt_abs_err, rt_cst_err;
	struct test_error_info rd_msk_err, rd_sat_err;
};

struct register_compat_info {
	const char *inst_name;    // Instruction name
	unsigned int test_cnt, test_errs;
	unsigned int nonoverlap_errors;         // No overlap whatsovever
	unsigned int compat_overlap_errors;     // "Compatible" overlap (same direction & regsubset)
	unsigned int incompat_overlap_errors;   // "Incompatible" overlap: dijoint set and/or dif direct.
	// Register pairs in case of collisions (for debug purposes)
	unsigned char reg_pairs[REG_PAIRS_DEBUGS][3];
};

#define enum_swz  0
#define enum_neg  1
#define enum_abs  2
#define enum_cst  3
#define enum_msk  0
#define enum_sat  1

void platform_cache_sync(void *baseaddr, unsigned size);

void gen_inputs_f32(unsigned tn, float *dst, unsigned count);
void gen_inputs_d32(unsigned tn, double *dst, unsigned count);
void gen_inputs_u32(unsigned tn, unsigned *dst, unsigned count);
void gen_inputs_s32(unsigned tn, int *dst, unsigned count);
void gen_inputs_bits(unsigned tn, unsigned *dst, unsigned count);

void gen_full_rand(unsigned seed, uint32_t *data, unsigned cnt);

unsigned pack6(unsigned *bits);

void fill_errinfo(struct test_error_info *errinfo, const struct test_decl_info *iinfo, void *rdex, void *rd);

#define fill_reg(entry, inval) \
	memcpy((entry).values, (inval), sizeof((inval)));

#define fill_cc(entry, ccreg) \
	(entry).values[0] = ccreg;

// Must be aligned
void save_all_vfpu_regs(float *outv);
void load_all_vfpu_regs(float *outv);

// Result checking routines
int fpdif(float a, float b);
int fprelerr(float ref, float val, float maxerr);
int fpabserr(float ref, float val, float maxerr);

int fpdifn(const float *a, const float *b, unsigned n);
int fprelerrn(const float *ref, const float *val, float maxerr, unsigned n);
int fpabserrn(const float *ref, const float *val, float maxerr, unsigned n);

extern FILE * logfd;

#ifdef _BUILD_EBOOT_
  // Log to disk and screen
  #define logprintf(...) { \
    pspDebugScreenPrintf(__VA_ARGS__); \
    fprintf(logfd, __VA_ARGS__); \
  }
#else
  // Log to stdout (to be used with gdb/pspsh)
  #define logprintf(...) \
    printf(__VA_ARGS__);
#endif


