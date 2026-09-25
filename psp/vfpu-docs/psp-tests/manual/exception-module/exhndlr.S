//
// MIT License
//
// Copyright (c) 2023 David Guillen Fandos <david@davidgf.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

// Exception handler for testing purposes
// This catches exceptions that match an expected EPC, otherwise falls back to
// whatever previous exception handler was set up.

#define OFF_MAGIC1    0

#define OFF_ARMED     8
#define OFF_EX_EPC   12

#define OFF_EXCNT    16
#define OFF_CAUSE    20
#define OFF_STATUS   24
#define OFF_BADVAD   28
#define OFF_GPR      32

#define MAGIC_VAL_1    0xdead
#define MAGIC_VAL_2    0xf1a6
#define MAGIC_VAL_3    0xbaad
#define MAGIC_VAL_4    0xf00d

#define check_magic(num)                     \
	lhu $v1,  (OFF_MAGIC1 + (num-1)*2)($v0); \
	xori $v1, MAGIC_VAL_##num;               \
	bnez $v1, _bad_excep

.set noat
.set noreorder

.extern exc_block
.extern previous_handler

.global _tests_exception_handler

_tests_exception_handler:
	nop     // This is the SceExceptionHandler header
	nop     // Seems like the kernel jumps to handler+8, so here:

	// Validate whether this exception was expected at all.
	// Note that v0 and v1 are saved by the COP0.
	la $v0, exc_block
	lw $v0, ($v0)

	check_magic(1)
	check_magic(2)
	check_magic(3)
	check_magic(4)

	// Check if this is actually armed
	lw $v1, OFF_ARMED($v0)
	beqz $v1, _bad_excep

	// Verify if the EPC does also match
	lw $v1, OFF_EX_EPC($v0)
	cfc0 $v0, $0              // Load EPC value
	bne $v0, $v1, _bad_excep
	nop

	// Proceed to dump the CPU state
	la $v0, exc_block
	lw $v0, ($v0)

	sw  $0, (OFF_GPR +  0*4)($v0)
	sw  $1, (OFF_GPR +  1*4)($v0)
	sw  $4, (OFF_GPR +  4*4)($v0)
	sw  $5, (OFF_GPR +  5*4)($v0)
	sw  $6, (OFF_GPR +  6*4)($v0)
	sw  $7, (OFF_GPR +  7*4)($v0)
	sw  $8, (OFF_GPR +  8*4)($v0)
	sw  $9, (OFF_GPR +  9*4)($v0)
	sw $10, (OFF_GPR + 10*4)($v0)
	sw $11, (OFF_GPR + 11*4)($v0)
	sw $12, (OFF_GPR + 12*4)($v0)
	sw $13, (OFF_GPR + 13*4)($v0)
	sw $14, (OFF_GPR + 14*4)($v0)
	sw $15, (OFF_GPR + 15*4)($v0)
	sw $16, (OFF_GPR + 16*4)($v0)
	sw $17, (OFF_GPR + 17*4)($v0)
	sw $18, (OFF_GPR + 18*4)($v0)
	sw $19, (OFF_GPR + 19*4)($v0)
	sw $20, (OFF_GPR + 20*4)($v0)
	sw $21, (OFF_GPR + 21*4)($v0)
	sw $22, (OFF_GPR + 22*4)($v0)
	sw $23, (OFF_GPR + 23*4)($v0)
	sw $24, (OFF_GPR + 24*4)($v0)
	sw $25, (OFF_GPR + 25*4)($v0)
	sw $28, (OFF_GPR + 28*4)($v0)
	sw $29, (OFF_GPR + 29*4)($v0)
	sw $30, (OFF_GPR + 30*4)($v0)
	sw $31, (OFF_GPR + 31*4)($v0)

	cfc0 $v1, $4    // Stored v0
	sw $v1, (OFF_GPR + 2*4)($v0)
	cfc0 $v1, $5    // Stored v1
	sw $v1, (OFF_GPR + 3*4)($v0)

	mflo $v1
	sw $v1, (OFF_GPR + 32*4)($v0)
	mfhi $v1
	sw $v1, (OFF_GPR + 33*4)($v0)

	// Now dump the COP0 variables
	mfc0 $v1, $8
	sw $v1, OFF_BADVAD($v0)
	cfc0 $v1, $3
	sw $v1, OFF_CAUSE($v0)
	cfc0 $v1, $2
	sw $v1, OFF_STATUS($v0)

	// Increment exception count
	lw $v1, (OFF_EXCNT)($v0)
	addiu $v1, $v1, 1
	sw $v1, (OFF_EXCNT)($v0)

	// Now skip the offending instruction
	cfc0 $v1, $0       // EPC value
	addiu $v1, $v1, 4  // Next inst
	mtc0 $v1, $14      // New EPC value

	// Restore trashed registers
	lw $v1, (OFF_GPR + 3*4)($v0)
	lw $v0, (OFF_GPR + 2*4)($v0)

	nop
	nop
	nop
	eret   // Resume execution!
	nop
	nop
	nop
	nop


// If the exception seems unexpected, pass it to whatever exception handler we have.
// Usually, if running under psplink, this will be psplink's exception handler, which
// very nicely dumps the exception. It also allows us to use debugging (since breakpoints)
// also raise exceptions. We do not want to prevent these from happening.
_bad_excep:
	la $v0, previous_handler
	lw $v0, ($v0)
	jr $v0
	nop
	nop


// These imports are not present in the SDK
// TODO: Add them to the SDK and use them directly

#include "pspimport.s"

IMPORT_START  "ExceptionManagerForKernel",0x00010011
IMPORT_FUNC   "ExceptionManagerForKernel",0xDF83875E,sceKernelGetActiveDefaultExceptionHandler

