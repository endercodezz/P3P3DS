
#include <pspkernel.h>
#include <psppower.h>
#include <pspthreadman.h>
#include <kubridge.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test-utils.h"

PSP_MODULE_INFO("PSP VFPU perf test", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

void perf_tests();

#define JUMP_BVT()   \
	"bvt 0, 2f;"     \
	"nop; nop; nop; nop; nop; nop; nop; nop; nop; 2:\n"

#define JUMP_BVF()   \
	"bvf 0, 2f;"     \
	"nop; nop; nop; nop; nop; nop; nop; nop; nop; 2:\n"

#define JUMP_BVTL()  \
	"bvtl 0, 2f;"    \
	"nop; nop; nop; nop; nop; nop; nop; nop; nop; 2:\n"

#define JUMP_BVFL()  \
	"bvfl 0, 2f;"    \
	"nop; nop; nop; nop; nop; nop; nop; nop; nop; 2:\n"

#define JUMP_J()     \
	"j 2f\n"         \
	"nop; nop; nop; nop; nop; nop; nop; nop; nop; 2:\n"

// This recalculates the next jump address in the delay slot
// to avoid data dependecies between instructions.
#define JUMP_JR()    \
	"jr $t0\n"       \
	"addiu $t0, $t0, 10*4\n" \
	"nop; nop; nop; nop; nop; nop; nop; nop; 2:\n"

#define JUMP_BEQ()     \
	"beq $0, $0, 2f\n" \
	"nop; nop; nop; nop; nop; nop; nop; nop; nop; 2:\n"

#define JUMP_BGEZAL()     \
	"bgezal $0, 2f\n"    \
	"nop; nop; nop; nop; nop; nop; nop; nop; nop; 2:\n"

#define FPU_ADD8()            \
	"add.s $f3,  $f1, $f2\n" "add.s $f4,  $f1, $f2\n"  \
	"add.s $f5,  $f1, $f2\n" "add.s $f6,  $f1, $f2\n"  \
	"add.s $f7,  $f1, $f2\n" "add.s $f8,  $f1, $f2\n"  \
	"add.s $f9,  $f1, $f2\n" "add.s $f10, $f1, $f2\n"

#define FPU_MUL8()            \
	"mul.s $f3,  $f1, $f2\n" "mul.s $f4,  $f1, $f2\n"  \
	"mul.s $f5,  $f1, $f2\n" "mul.s $f6,  $f1, $f2\n"  \
	"mul.s $f7,  $f1, $f2\n" "mul.s $f8,  $f1, $f2\n"  \
	"mul.s $f9,  $f1, $f2\n" "mul.s $f10, $f1, $f2\n"

#define FPU_DIV8()    \
	"div.s $f3,  $f1, $f2\n" "div.s $f4,  $f1, $f2\n"  \
	"div.s $f5,  $f1, $f2\n" "div.s $f6,  $f1, $f2\n"  \
	"div.s $f7,  $f1, $f2\n" "div.s $f8,  $f1, $f2\n"  \
	"div.s $f9,  $f1, $f2\n" "div.s $f10, $f1, $f2\n"

#define FPU_SQR8()    \
	"sqrt.s $f3,  $f1\n" "sqrt.s $f4,  $f1\n"  \
	"sqrt.s $f5,  $f1\n" "sqrt.s $f6,  $f1\n"  \
	"sqrt.s $f7,  $f1\n" "sqrt.s $f8,  $f1\n"  \
	"sqrt.s $f9,  $f1\n" "sqrt.s $f10, $f1\n"

#define FPU_ADD8_L()            \
	"add.s $f3, $f1, $f2\n" "add.s $f4, $f1, $f3\n"  \
	"add.s $f5, $f1, $f4\n" "add.s $f6, $f1, $f5\n"  \
	"add.s $f7, $f1, $f6\n" "add.s $f8, $f1, $f7\n"  \
	"add.s $f9, $f1, $f8\n" "add.s $f2, $f1, $f9\n"

#define FPU_MUL8_L()            \
	"mul.s $f3, $f1, $f2\n" "mul.s $f4, $f1, $f3\n"  \
	"mul.s $f5, $f1, $f4\n" "mul.s $f6, $f1, $f5\n"  \
	"mul.s $f7, $f1, $f6\n" "mul.s $f8, $f1, $f7\n"  \
	"mul.s $f9, $f1, $f8\n" "mul.s $f2, $f1, $f9\n"

#define FPU_DIV8_L()            \
	"div.s $f3, $f1, $f2\n" "div.s $f4, $f1, $f3\n"  \
	"div.s $f5, $f1, $f4\n" "div.s $f6, $f1, $f5\n"  \
	"div.s $f7, $f1, $f6\n" "div.s $f8, $f1, $f7\n"  \
	"div.s $f9, $f1, $f8\n" "div.s $f2, $f1, $f9\n"

#define FPU_SQR8_L()    \
	"sqrt.s $f3, $f1\n" "sqrt.s $f4, $f3\n"  \
	"sqrt.s $f5, $f4\n" "sqrt.s $f6, $f5\n"  \
	"sqrt.s $f7, $f6\n" "sqrt.s $f8, $f7\n"  \
	"sqrt.s $f9, $f8\n" "sqrt.s $f1, $f9\n"

#define ALU_MUL8()            \
	"mult $t0, $t1\n" "mult $t0, $t1\n"  \
	"mult $t0, $t1\n" "mult $t0, $t1\n"  \
	"mult $t0, $t1\n" "mult $t0, $t1\n"  \
	"mult $t0, $t1\n" "mult $t0, $t1\n"

#define ALU_DIV8()            \
	"div $t0, $t1\n" "div $t0, $t1\n"  \
	"div $t0, $t1\n" "div $t0, $t1\n"  \
	"div $t0, $t1\n" "div $t0, $t1\n"  \
	"div $t0, $t1\n" "div $t0, $t1\n"

#define ALU_MUL8_L()            \
	"mult $t0, $t1\n" "mflo $t2\n" "mult $t0, $t1\n" "mflo $t3\n" \
	"mult $t0, $t1\n" "mflo $t4\n" "mult $t0, $t1\n" "mflo $t5\n" \
	"mult $t0, $t1\n" "mflo $t6\n" "mult $t0, $t1\n" "mflo $t7\n" \
	"mult $t0, $t1\n" "mflo $t8\n" "mult $t0, $t1\n" "mflo $t9\n"

#define ALU_DIV8_L()            \
	"div $t0, $t1\n" "mflo $t2\n" "div $t0, $t1\n" "mflo $t3\n" \
	"div $t0, $t1\n" "mflo $t4\n" "div $t0, $t1\n" "mflo $t5\n" \
	"div $t0, $t1\n" "mflo $t6\n" "div $t0, $t1\n" "mflo $t7\n" \
	"div $t0, $t1\n" "mflo $t8\n" "div $t0, $t1\n" "mflo $t9\n"

#define VCMP_B()     \
	"vcmp.q NE, R000.q, R000.q\n"   \
	"bvt 0, 1b\n nop\n" \

#define LW_0()      \
	"lw %0, (%0)\n"

#define LW_THR_4(off)         \
	"lw $v0,   0+"#off"(%0);" \
	"lw $v0,   4+"#off"(%0);" \
	"lw $v0,   8+"#off"(%0);" \
	"lw $v0,  12+"#off"(%0)\n"

#define LW_THR(off)          \
	LW_THR_4(off+0)    LW_THR_4(off+16)  LW_THR_4(off+32) LW_THR_4(off+48)  \
	LW_THR_4(off+64)   LW_THR_4(off+80)  LW_THR_4(off+96) LW_THR_4(off+112) \
	LW_THR_4(off+128) LW_THR_4(off+144) LW_THR_4(off+160) LW_THR_4(off+176) \
	LW_THR_4(off+192) LW_THR_4(off+208) LW_THR_4(off+224) LW_THR_4(off+240)

#define LVQ_THR_4(off)            \
	"lv.q R000,   0+"#off"(%0);"  \
	"lv.q R000,  16+"#off"(%0);"  \
	"lv.q R000,  32+"#off"(%0);"  \
	"lv.q R000,  48+"#off"(%0)\n"

#define LVQ_THR(off)          \
	LVQ_THR_4(off+0)   LVQ_THR_4(off+64)  LVQ_THR_4(off+128) LVQ_THR_4(off+192) \
	LVQ_THR_4(off+256) LVQ_THR_4(off+320) LVQ_THR_4(off+384) LVQ_THR_4(off+448)

#define REP8(mc)     \
  mc() mc() mc() mc() mc() mc() mc() mc()

#define REP64(mc)    \
  REP8(mc) REP8(mc) REP8(mc) REP8(mc) REP8(mc) REP8(mc) REP8(mc) REP8(mc)

#define MAX_BUF_SIZE     1024*1024
void * membuf[MAX_BUF_SIZE] __attribute__((aligned(16)));   // 4MiB buffer

inline uint32_t rand24() {
	return rand() ^ (rand() << 8);
}

#define TIME_FPU(INST_MACRO, metric, iname)                                   \
{                                                                             \
  int64_t stt = sceKernelGetSystemTimeWide();                                 \
  asm volatile(                                                               \
    ".set noreorder\n"                                                        \
    "la $a3, 0x3f800000\n" /* 1.0f */                                         \
    "mtc1 $a3, $f1\n"                                                         \
    "mtc1 $a3, $f2\n"                                                         \
    "li $a3, 16384\n"                                                         \
    "1: \n"                                                                   \
    REP8(INST_MACRO) REP8(INST_MACRO)                                         \
    REP8(INST_MACRO) REP8(INST_MACRO)                                         \
    "sub $a3, $a3, 1\n"                                                       \
    "bnez $a3, 1b\n nop \n"                                                   \
    ".set reorder\n"                                                          \
  ::: "$a3", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6",                        \
      "$f7", "$f8", "$f9", "$f10", "memory");                                 \
  uint64_t end = sceKernelGetSystemTimeWide();                                \
  unsigned cyclt = (unsigned)(end - stt) * 333;                               \
  logprintf("%s %s: %f\n", iname, metric,                                     \
            cyclt / (float)(256 * 16384));                                    \
}

#define TIME_ALU(INST_MACRO, metric, iname)                                   \
{                                                                             \
  uint64_t stt = sceKernelGetSystemTimeWide();                                \
  asm volatile(                                                               \
    ".set noreorder\n"                                                        \
    "la $t0, 0x1eadc0fe\n"                                                    \
    "la $t1, 0xf00df00d\n"                                                    \
    "li $a3, 16384\n"                                                         \
    "1: \n"                                                                   \
    REP8(INST_MACRO) REP8(INST_MACRO)                                         \
    REP8(INST_MACRO) REP8(INST_MACRO)                                         \
    "sub $a3, $a3, 1\n"                                                       \
    "bnez $a3, 1b\n nop \n"                                                   \
    ".set reorder\n"                                                          \
  ::: "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5",                        \
      "$t6", "$t7", "$t8", "$t9", "memory");                                  \
  uint64_t end = sceKernelGetSystemTimeWide();                                \
  unsigned cyclt = (unsigned)(end - stt) * 333;                               \
  logprintf("%s %s: %f\n", iname, metric,                                     \
            cyclt / (float)(256 * 16384));                                    \
}

int main() {
	scePowerSetClockFrequency(333, 333, 166);
	#ifdef _BUILD_EBOOT_
	pspDebugScreenInit();
	logfd = fopen("ms0://test_performance_results.txt", "wb");
	#endif

	logprintf(">>>>>>> Instruction performance tests <<<<<<\n");
	logprintf("PSP model: %d\n", kuKernelGetModel());

	logprintf(">>>>>>> Compare/Branch instruction performance tests <<<<<<\n");

	// Calculate jump-to-fetch latency
	// This is the number of cycles "lost" by a branch (minus one due to delay slot)

	{
		uint64_t stt = sceKernelGetSystemTimeWide();
		asm volatile(
			".set noreorder\n"
			"vzero.q R000.q\n"
			"vcmp.q EQ, R000.q, R000.q\n"   // Force bits to one
			"li $a3, 16384\n"
			"1: \n"
			REP64(JUMP_BVT) REP64(JUMP_BVT)
			REP64(JUMP_BVT) REP64(JUMP_BVT)
			"sub $a3, $a3, 1\n"
			"bnez $a3, 1b\n nop \n"
			".set reorder\n"
		::: "$a3", "memory");
		uint64_t end = sceKernelGetSystemTimeWide();
		unsigned cyclt = (unsigned)(end - stt) * 333;
		logprintf("bvt latency: %f\n", cyclt / (float)(256 * 16384));
	}

	{
		uint64_t stt = sceKernelGetSystemTimeWide();
		asm volatile(
			".set noreorder\n"
			"vzero.q R000.q\n"
			"vcmp.q NE, R000.q, R000.q\n"   // Force bits to zero
			"li $a3, 16384;"
			"1: \n"
			REP64(JUMP_BVF) REP64(JUMP_BVF)
			REP64(JUMP_BVF) REP64(JUMP_BVF)
			"sub $a3, $a3, 1\n"
			"bnez $a3, 1b\n nop \n"
			".set reorder\n"
		::: "$a3", "memory");
		uint64_t end = sceKernelGetSystemTimeWide();
		unsigned cyclt = (unsigned)(end - stt) * 333;
		logprintf("bvf latency: %f\n", cyclt / (float)(256 * 16384));
	}

	{
		uint64_t stt = sceKernelGetSystemTimeWide();
		asm volatile(
			".set noreorder\n"
			"vzero.q R000.q\n"
			"vcmp.q EQ, R000.q, R000.q\n"
			"li $a3, 16384\n"
			"1: \n"
			REP64(JUMP_BVTL) REP64(JUMP_BVTL)
			REP64(JUMP_BVTL) REP64(JUMP_BVTL)
			"sub $a3, $a3, 1\n"
			"bnez $a3, 1b\n nop \n"
			".set reorder\n"
		::: "$a3", "memory");
		uint64_t end = sceKernelGetSystemTimeWide();
		unsigned cyclt = (unsigned)(end - stt) * 333;
		logprintf("bvtl latency: %f\n", cyclt / (float)(256 * 16384));
	}

	{
		uint64_t stt = sceKernelGetSystemTimeWide();
		asm volatile(
			".set noreorder\n"
			"vzero.q R000.q\n"
			"vcmp.q NE, R000.q, R000.q\n"
			"li $a3, 16384;"
			"1: \n"
			REP64(JUMP_BVFL) REP64(JUMP_BVFL)
			REP64(JUMP_BVFL) REP64(JUMP_BVFL)
			"sub $a3, $a3, 1\n"
			"bnez $a3, 1b\n nop \n"
			".set reorder\n"
		::: "$a3", "memory");
		uint64_t end = sceKernelGetSystemTimeWide();
		unsigned cyclt = (unsigned)(end - stt) * 333;
		logprintf("bvfl latency: %f\n", cyclt / (float)(256 * 16384));
	}

	{
		uint64_t stt = sceKernelGetSystemTimeWide();
		asm volatile(
			".set noreorder\n"
			"li $a3, 16384;"
			"1: \n"
			REP64(JUMP_J) REP64(JUMP_J)
			REP64(JUMP_J) REP64(JUMP_J)
			"sub $a3, $a3, 1\n"
			"bnez $a3, 1b\n nop \n"
			".set reorder\n"
		::: "$a3", "memory");
		uint64_t end = sceKernelGetSystemTimeWide();
		unsigned cyclt = (unsigned)(end - stt) * 333;
		logprintf("j latency: %f\n", cyclt / (float)(256 * 16384));
	}

	{
		uint64_t stt = sceKernelGetSystemTimeWide();
		asm volatile(
			".set noreorder\n"
			"li $a3, 16384;"
			"la $t1, 2f\n"
			"move $t0, $t1\n"
			"1: \n"
			REP64(JUMP_JR) REP64(JUMP_JR)
			REP64(JUMP_JR) REP64(JUMP_JR)
			"sub $a3, $a3, 1\n"
			"bnez $a3, 1b\n"
			"move $t0, $t1\n"   // Reload pointer (delay slot)
			".set reorder\n"
		::: "$a3", "$t0", "$t1", "memory");
		uint64_t end = sceKernelGetSystemTimeWide();
		unsigned cyclt = (unsigned)(end - stt) * 333;
		logprintf("jr latency: %f\n", cyclt / (float)(256 * 16384));
	}

	{
		uint64_t stt = sceKernelGetSystemTimeWide();
		asm volatile(
			".set noreorder\n"
			"li $a3, 16384;"
			"1: \n"
			REP64(JUMP_BEQ) REP64(JUMP_BEQ)
			REP64(JUMP_BEQ) REP64(JUMP_BEQ)
			"sub $a3, $a3, 1\n"
			"bnez $a3, 1b\n nop \n"
			".set reorder\n"
		::: "$a3", "memory");
		uint64_t end = sceKernelGetSystemTimeWide();
		unsigned cyclt = (unsigned)(end - stt) * 333;
		logprintf("beq latency: %f\n", cyclt / (float)(256 * 16384));
	}

	{
		uint64_t stt = sceKernelGetSystemTimeWide();
		asm volatile(
			".set noreorder\n"
			"li $a3, 16384;"
			"1: \n"
			REP64(JUMP_BGEZAL) REP64(JUMP_BGEZAL)
			REP64(JUMP_BGEZAL) REP64(JUMP_BGEZAL)
			"sub $a3, $a3, 1\n"
			"bnez $a3, 1b\n nop \n"
			".set reorder\n"
		::: "$a3", "memory", "$ra");
		uint64_t end = sceKernelGetSystemTimeWide();
		unsigned cyclt = (unsigned)(end - stt) * 333;
		logprintf("bgezal latency: %f\n", cyclt / (float)(256 * 16384));
	}

	// Calculate vcmp to bvt/f latency
	// (latency for vcmp->mfvc is different)
	// This is the number of cycles that are "lost" between compare and jump
	// (latency can be hidden by adding instructions in between)

	{
		uint64_t stt = sceKernelGetSystemTimeWide();
		asm volatile(
			".set noreorder\n"
			"vzero.q R000.q\n"
			"li $a3, 16384;"
			"1: \n"
			REP64(VCMP_B) REP64(VCMP_B)
			REP64(VCMP_B) REP64(VCMP_B)
			"sub $a3, $a3, 1\n"
			"bnez $a3, 1b\n nop \n"
			".set reorder\n"
		::: "$a3", "memory");
		uint64_t end = sceKernelGetSystemTimeWide();
		unsigned cyclt = (unsigned)(end - stt) * 333;
		// Remote 1 cycle for branch, 1 for nop
		logprintf("vcmp latency: %f\n", cyclt / (float)(256 * 16384) - 2);
	}

	logprintf(">>>>>>> FPU instruction performance tests <<<<<<\n");

	// Calculate FPU throughput and latency for FPU instructions
	TIME_FPU(FPU_ADD8, "throughput", "add.s");
	TIME_FPU(FPU_MUL8, "throughput", "mul.s");
	TIME_FPU(FPU_DIV8, "throughput", "div.s");
	TIME_FPU(FPU_SQR8, "throughput", "sqrt.s");

	TIME_FPU(FPU_ADD8_L, "latency", "add.s");
	TIME_FPU(FPU_MUL8_L, "latency", "mul.s");
	TIME_FPU(FPU_DIV8_L, "latency", "div.s");
	TIME_FPU(FPU_SQR8_L, "latency", "sqrt.s");

	logprintf(">>>>>>> ALU instruction performance tests <<<<<<\n");

	TIME_ALU(ALU_MUL8, "throughput", "mult");
	TIME_ALU(ALU_DIV8, "throughput", "div");

	TIME_ALU(ALU_MUL8_L, "latency", "mult");
	TIME_ALU(ALU_DIV8_L, "latency", "div");

	logprintf(">>>>>>> Memory latency performance tests <<<<<<\n");

	// Memory latency estimations.
	// Check different buffer sizes, to measure any caches that we might have.
	// We use pointer chasing with a randomly scrambled buffer to ensure the
	// prefetcher doesn't mess with our patterns.
	const unsigned memsizes[] = {
		2048     /*   8KiB */,
		4096     /*  16KiB */,
		8192     /*  32KiB */,
		16384    /*  64KiB */,
		32768    /* 128KiB */,
		65536    /* 256KiB */,
		131072   /* 512KiB */,
		262144   /*   1MiB */,
		524288   /*   2MiB */,
		1048576  /*   4MiB */,
	};
	for (unsigned randac = 0; randac < 2; randac++) {
		for (unsigned j = 0; j < sizeof(memsizes)/sizeof(memsizes[0]); j++) {
			// Generate a linked list, loops at the end.
			const unsigned ptrcnt = memsizes[j];

			for (unsigned i = 0; i < ptrcnt-1; i++)
				membuf[i] = &membuf[i+1];
			membuf[ptrcnt-1] = &membuf[0];

			if (randac) {
				// Randomize the list to generate a random pattern.
				for (unsigned i = 0; i < ptrcnt * 3; i++) {
					unsigned p1 = rand24() & (ptrcnt-1);
					unsigned p2 = rand24() & (ptrcnt-1);
					void *t = membuf[p1];
					membuf[p1] = membuf[p2];
					membuf[p2] = t;
				}
			}

			uint64_t stt = sceKernelGetSystemTimeWide();
			void *bufptr = (void*)&membuf[0];
			asm volatile(
				".set noreorder\n"
				"lui $a3, 1\n"     // 64K iterations
				"1: \n"
				REP64(LW_0) REP64(LW_0)
				REP64(LW_0) REP64(LW_0)
				"sub $a3, $a3, 1\n"
				"bnez $a3, 1b\n nop \n"
				".set reorder\n"
			:"+r"(bufptr):: "$a3", "memory");
			uint64_t end = sceKernelGetSystemTimeWide();
			unsigned cyclt = (unsigned)(end - stt) * 333;
			logprintf("Memory latency for %s accesses (buf size %d): %f\n",
				randac ? "random" : "sequential",
				ptrcnt*4, cyclt / (float)(256 * 65536));
		}
	}

	// This is a throughput test, the data read is not used.
	for (unsigned j = 0; j < sizeof(memsizes)/sizeof(memsizes[0]); j++) {
		const unsigned ptrcnt = memsizes[j];
		uint64_t stt = sceKernelGetSystemTimeWide();
		void *bufptr = (void*)&membuf[0];
		void *bufptr_end = (void*)&membuf[ptrcnt-1];
		asm volatile(
			".set noreorder\n"
			"move $t0, %0\n"
			"lui $a3, 1\n"     // 64K iterations
			"1: \n"
			LW_THR(0) LW_THR(256) LW_THR(512) LW_THR(768)
			"sub $a3, $a3, 1\n"
			"addiu %0, %0, 1024\n"
			"sltu $v0, %0, %1\n"  // 0 if we reach the end
			"bnez $a3, 1b\n"
			"movz %0, $t0, $v0\n"
			".set reorder\n"
		:"+r"(bufptr): "r"(bufptr_end) : "$a3", "$t0", "$v0", "memory");
		uint64_t end = sceKernelGetSystemTimeWide();
		unsigned cyclt = (unsigned)(end - stt) * 333;
		unsigned readmem = 65536 * 256 * 4;  // Bytes read total
		logprintf("Memory throughput for sequential (LW) accesses (buf size %d): %f (%f bytes/cycle)\n",
			ptrcnt*4, cyclt / (float)(256 * 65536), readmem / (float)cyclt);
	}

	// Throughput test but using lv.q instructions instead
	for (unsigned j = 0; j < sizeof(memsizes)/sizeof(memsizes[0]); j++) {
		const unsigned ptrcnt = memsizes[j];
		uint64_t stt = sceKernelGetSystemTimeWide();
		void *bufptr = (void*)&membuf[0];
		void *bufptr_end = (void*)&membuf[ptrcnt-1];
		asm volatile(
			".set noreorder\n"
			"move $t0, %0\n"
			"lui $a3, 1\n"     // 64K iterations
			"1: \n"
			LVQ_THR(0) LVQ_THR(512) LVQ_THR(1024) LVQ_THR(1536)
			"sub $a3, $a3, 1\n"
			"addiu %0, %0, 1024\n"
			"sltu $v0, %0, %1\n"  // 0 if we reach the end
			"bnez $a3, 1b\n"
			"movz %0, $t0, $v0\n"
			".set reorder\n"
		:"+r"(bufptr): "r"(bufptr_end) : "$a3", "$t0", "$v0", "memory");
		uint64_t end = sceKernelGetSystemTimeWide();
		unsigned cyclt = (unsigned)(end - stt) * 333;
		unsigned readmem = 65536 * 128 * 16;  // Bytes read total
		logprintf("Memory throughput for sequential (LV.Q) accesses (buf size %d): %f (%f bytes/cycle)\n",
			ptrcnt*4, cyclt / (float)(128 * 65536), readmem / (float)cyclt);
	}

	// Run automated tests
	logprintf(">>>>>>> VFPU instruction performance tests <<<<<<\n");
	perf_tests();

	#ifdef _BUILD_EBOOT_
	fclose(logfd);
	#endif
	sceKernelExitGame();
}


