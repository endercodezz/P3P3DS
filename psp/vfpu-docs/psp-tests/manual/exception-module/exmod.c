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

// This is a kernel module meant to be used by psp-tests/manual.
// The goal is to test exception behaviour (ie. for memops, trap instructions)
//
// The user app loads this module and passes a control block pointer. Before
// raising an exception, it must initialize the control block with some
// information (i.e. what instruction it expects will trap) so that we do not
// capture unexpected exceptions (like a bug in the actual test).
// On exceptions the module dumps information to the control block and resumes
// execution by skipping the offending instruction. The user app is then
// responsible for validating whatever it is testing.

#include <pspkernel.h>
#include <pspexception.h>

#include "exception_info.h"

extern void _tests_exception_handler(void);

// This is a kernel module for exception handling

PSP_MODULE_INFO("tests-exception-handler", PSP_MODULE_KERNEL | PSP_MODULE_NO_STOP | PSP_MODULE_SINGLE_LOAD | PSP_MODULE_SINGLE_START, 1, 1);

exception_control_block *exc_block;
void *previous_handler;

void *sceKernelGetActiveDefaultExceptionHandler();

int module_start(SceSize args, void *argp) {
	// It takes two arguments, two pointers. "args" is specified in bytes!
	if (args != sizeof(void*))
		return -1;

	exc_block = ((void **)argp)[0];

	if (!exc_block)
		return -1;

	// Resolve the current default exception handler, to which we can fallback.
	previous_handler = sceKernelGetActiveDefaultExceptionHandler();

	// Override the previous handler with ours.
	return sceKernelRegisterDefaultExceptionHandler((void *)_tests_exception_handler);
}


