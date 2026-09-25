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

#ifndef __PSPTESTS__EXCEPTION_INFO_H__
#define __PSPTESTS__EXCEPTION_INFO_H__

#include <stdint.h>

#define MAGIC_VAL_1    0xdead
#define MAGIC_VAL_2    0xf1a6
#define MAGIC_VAL_3    0xbaad
#define MAGIC_VAL_4    0xf00d

#define GET_EX_CAUSE(cause)  (((cause) >> 2) & 31)

// From SMIPS Processor Specification
#define EX_MEM_ADD_LD_ERR      4        // Address error/misaligned on load
#define EX_MEM_ADD_ST_ERR      5        // Address error/misaligned on store
#define EX_MEM_BUS_ERR         7        // Bus error exception
#define EX_DEBUG_BREAK         9        // Breakpoint
#define EX_ILLEGAL_INST       10        // Reserved instruction exception
#define EX_COP_ILLEGAL        11        // Coprocessor Unusable
#define EX_ARITH_OVERFLOW     12        // Arithmetic overflow

typedef struct {
	uint32_t cause;
	uint32_t status;
	uint32_t badvaddr;

	uint32_t gpr[32];
	uint32_t mlo, mhi;
} cpu_state;

typedef struct {
	uint16_t magic[4];                 // Must be valid for exception capture

	// This must be filled by the user-app to properly capture the exception
	uint32_t armed;                    // Whether we want to capture or not
	uint32_t expected_epc;             // The expected EPC value

	// This is filled by the kernel module with exception-related information
	uint32_t exception_count;
	cpu_state state;
} exception_control_block;


#endif

