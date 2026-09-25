#!/usr/bin/env python3

# Copyright 2022 David Guillen Fandos <david@davidgf.net>

# Generates VFPU asm tests to measure instruction latency and throughput
# To measure throughput we run independent instructions back to back and
#  approximate the number of cycles taken.
# For latency measurements we chain instructions and consumers and measure
#  the time taken (subtracting any auxiliar instructions).

import os, math, argparse, re, itertools, random, instparse

parser = argparse.ArgumentParser(prog='gen-tests')
parser.add_argument('--instruction', dest='instregex', default=".*", type=str, help='Instruction name (regex)')
parser.add_argument('--output', dest='output', required=True, help='Output file')
parser.add_argument('--insts-per-bb', dest='instsbb', default=256, type=int, help='Number of instructions in a BB')
parser.add_argument('--insts-iterations', dest='instsit', default=16*1024, type=int, help='Number of BB repetitions')

args = parser.parse_args()

if os.path.exists(args.output):
	print("Output file already exists!")
	os._exit(1)

with open(args.output, "w") as ofd:
	for f in ["<string.h>", "<stdio.h>", "<stdint.h>", "<pspthreadman.h>", '"test-utils.h"']:
		ofd.write('#include %s\n' % f)

	ofd.write("void perf_tests() {\n")

	# Generate instruction list and all its attributes
	for instname, iobj in instparse.insts.items():
		if not re.match(args.instregex, instname):
			continue
		if iobj.numelems() == 0:
			continue   # Only pure VFPU insts for now

		# Prepare input registers
		regsin  = iobj.inputs() #{ reg:rtype for reg, rtype in iobj.inputs().items() if rtype != "vfpucc" }
		regsout = iobj.outputs()
		ienc = iobj.encoding()
		# Only support one output reg for now
		if len(regsout) == 1:
			(oregname, _), = regsout.items()
		else:
			oregname = None

		# Pick some reasonable immediate (?)
		immdict = iobj.immediates()
		immn = iobj.immediates().keys()
		imml = [ (immdict[i]["minval"] + immdict[i]["maxval"]) // 2
		         for i in immn]

		for immval in imml:
			for inum, imname in enumerate(immn):
				assert immval >= 0
				ienc.fill(imname, immval)

		# To avoid instructions having hazards we generate different regs and interleave them
		tencs, cencs = [], []
		for i in range(8):
			e = ienc.copy()
			if oregname and regsout[oregname] != "vfpucc":
				e.fill(oregname, 4 * (i % 8))
			for nreg, reg in enumerate(regsin):
				if regsin[reg] != "vfpucc":
					e.fill(reg, 4 * ((i + 1 + nreg) % 8))
			tencs.append(e)
		# For latency calculations we do exactly the same but generate a vmax.q that depends on it
		for i in range(8):
			e = ienc.copy()
			if oregname and regsout[oregname] != "vfpucc":
				e.fill(oregname, 4 * (i % 8))
			for nreg, reg in enumerate(regsin):
				if regsin[reg] != "vfpucc":
					e.fill(reg, 4 * ((i + 1 + nreg) % 8))

			# vmax.q to depend on previous inst
			venc = instparse.insts["vabs.q"].encoding()
			for ar in ["rs", "rd"]:
				venc.fill(ar, 4 * (i % 8))
			#venc.fill("cond", 1) # EQ
			cencs.append((e, venc))

		# Generate C emulation code out of pcode
		ofd.write("// Tests for instruction %s (%s)\n" % (instname, iobj.title()))

		for e in tencs:
			if e.basemask() != 0xFFFFFFFF:
				print("Error!", instname, "has missing fields")

		for e0, e1 in cencs:
			if e0.basemask() != 0xFFFFFFFF or e1.basemask() != 0xFFFFFFFF:
				print("Error!")

		ofd.write("{\n")

		# Setup initial state. We use 1.0 as values, not sure if instruction performance
		# depends on inputs (it definitely does on x86 arches)
		ofd.write("asm volatile(\n")
		ofd.write('"mtvc $0, $131;"\n')
		for i in range(8):
			ofd.write('"vmone.q M%d00.q;"\n' % i)
		ofd.write('::: "memory");')

		# Throughput calculation
		ofd.write('uint64_t sttime0 = sceKernelGetSystemTimeWide();\n')
		ofd.write("asm volatile(\n")
		ofd.write('"li $a3, %d;"\n' % args.instsit)
		ofd.write('"1:"\n')
		for i in range(args.instsbb):
			ofd.write('".word 0x%08x;"\n' % tencs[i % 8].baseword())
		ofd.write('"sub $a3, $a3, 1;"\n')
		ofd.write('"bnez $a3, 1b; nop;"\n')
		ofd.write('::: "$a3",  "memory");')
		ofd.write('uint64_t edtime0 = sceKernelGetSystemTimeWide();\n')

		# Latency calculation
		ofd.write('uint64_t sttime1 = sceKernelGetSystemTimeWide();\n')
		ofd.write("asm volatile(\n")
		ofd.write('"li $a3, %d;"\n' % args.instsit)
		ofd.write('"1:"\n')
		for i in range(args.instsbb):
			ofd.write('".word 0x%08x;"\n' % cencs[i % 8][0].baseword())
			ofd.write('".word 0x%08x;"\n' % cencs[i % 8][1].baseword())
		ofd.write('"sub $a3, $a3, 1;"\n')
		ofd.write('"bnez $a3, 1b; nop;"\n')
		ofd.write('::: "$a3",  "memory");')
		ofd.write('uint64_t edtime1 = sceKernelGetSystemTimeWide();\n')

		nops = args.instsit*args.instsbb
		ofd.write('unsigned cycth = (unsigned)(edtime0 - sttime0) * 333;\n')
		ofd.write('unsigned cyclt = (unsigned)(edtime1 - sttime1) * 333;\n')
		ofd.write('logprintf("%s: %%f cycles/inst %%f latency\\n", cycth / %d.0f, cyclt / %d.0f - 1);\n' % (instname, nops, nops))

		ofd.write("}\n\n")

	ofd.write("}\n")


