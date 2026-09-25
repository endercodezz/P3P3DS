#!/usr/bin/env python3

# Copyright 2022 David Guillen Fandos <david@davidgf.net>

# Generates VFPU asm tests to validate register collisions
# It works like this:
#  - Generate a bunch of random inputs, use some valid regs
#  - Do it again with some potentially invalid regs
#  - Compare results, if they are different, found them!

import os, math, argparse, re, itertools, hashlib, instparse, reginfo

parser = argparse.ArgumentParser(prog='gen-tests')
parser.add_argument('--instruction', dest='instregex', default=".*", type=str, help='Instruction name (regex)')
parser.add_argument('--output', dest='output', required=True, help='Output file')
parser.add_argument('--num-tests', dest='testcnt', default=4, type=int, help='Number of tests')
parser.add_argument('--num-imm-tests', dest='testimmcnt', default=64, type=int, help='Number of immediate tests')
parser.add_argument('--rndseed', dest='rndseed', default=0xdeadbeef, type=int, help='Random seed')

args = parser.parse_args()

rndseed = args.rndseed.to_bytes(32, 'big')
def rndnum(minv, maxv):
	global rndseed
	rndseed = hashlib.sha256(rndseed).digest()
	intval = int.from_bytes(rndseed, 'big')
	return intval % (maxv - minv + 1) + minv

MIPS_REG_A0 = 4
MIPS_REG_A1 = 5

if os.path.exists(args.output):
	print("Output file already exists!")
	os._exit(1)

# Calculate operand size, supports reduction and expansion for vector.
def expsize(regtype, lanes):
	modif = 1
	if ":" in regtype:
		regtype, modif = regtype.split(":")
		modif = {"D": 2, "Q": 4, "H": 0.5, "T": 0.25}[modif]

	if regtype == "single":
		return 1
	return int(modif*lanes)

# Full element size (vector, matrix...)
def cntelem(regtype, lanes):
	cnts = {"single": 1, "vector": lanes, "matrix": lanes*lanes, "vfpucc": 6}
	regtype = regtype.split(":")[0]
	return cnts[regtype]

# Emits a mem instruction with a variable for vfpu reg (instead of a fixed value)
def emit_mem_partial(opcode, offset, gpr):
	instenc = instparse.insts[opcode].encoding()
	flds = instenc.fields()
	return "((%d) << %d) | ((%s) << %d)" % (gpr, flds["gpr"]["lsb"], offset, flds["offset"]["lsb"])

# Emits mem instruction for a specific VFPU register
def emit_mem_full(opcode, vfpureg, offset, gpr):
	instenc = instparse.insts[opcode].encoding()
	flds = instenc.fields()
	instenc.fill("rtlo", vfpureg & ((1 << flds["rtlo"]["size"])-1))
	instenc.fill("rthi", vfpureg >> flds["rtlo"]["size"])
	instenc.fill("gpr", gpr)
	instenc.fill("offset", offset)
	assert instenc.basemask() == 0xFFFFFFFF  # Full instruction
	return instenc.baseword()

# Can't do full ranges, too big of a binary, test some corner cases instead
def smart_range(minval, maxval):
	if abs(maxval - minval) < args.testimmcnt:
		return range(minval, maxval + 1)
	cases = []
	for i in range(args.testimmcnt):
		cases.append(rndnum(minval, maxval))
	return [minval, maxval] + cases

with open(args.output, "w") as ofd:
	for f in ["<string.h>", "<math.h>", "<stdbool.h>", "<stdint.h>",
	          "<stdio.h>", '"test-utils.h"']:
		ofd.write('#include %s\n' % f)

	# Generate some basic tests that ensure that the register mappings
	# are correct as described by reginfo
	isuff = {2: "p", 3: "t", 4: "q"}
	ofd.write("static float outv[128] __attribute__ ((aligned (16)));\n")
	ofd.write("void run_reg_sanity_tests() {\n")
	for mode, elemsize in [
		("vector", 2), ("vector", 3), ("vector", 4),
		("matrix", 2), ("matrix", 3), ("matrix", 4)]:
		for elem in reginfo.genvect(mode, elemsize):
			# Clear up all state and then perform some tainting tests
			regnum, singlst = elem
			ofd.write('asm volatile("%s");\n' % ";".join("vmzero.q M%d00" % i for i in range(8)))

			if mode == "vector":
				instenc = instparse.insts["vone." + isuff[elemsize]].encoding()
				instenc.fill("rd", regnum)
				ofd.write('// vone.%s %s (%d)\n' % (isuff[elemsize], reginfo.regpname(mode, elemsize, regnum), regnum))
				ofd.write('asm volatile(".word 0x%08x");\n' % instenc.baseword())
			else:
				instenc = instparse.insts["vmone." + isuff[elemsize]].encoding()
				instenc.fill("rd", regnum)
				ofd.write('// vmone.%s %s (%d)\n' % (isuff[elemsize], reginfo.regpname(mode, elemsize, regnum), regnum))
				ofd.write('asm volatile(".word 0x%08x");\n' % instenc.baseword())

			assert instenc.basemask() == 0xffffffff				
			ofd.write('save_all_vfpu_regs(&outv[0]);\n')
			expect = ",".join("1" if i in singlst else "0" for i in range(128))
			ofd.write('for (unsigned i = 0; i < 128; i++) {\n')
			ofd.write('const char exp[] = {%s};\n' % expect)
			ofd.write('if (((int)outv[i]) != (int)exp[i]) {\n')
			ofd.write('logprintf("%x\\n");\n' % instenc.baseword())
			ofd.write('logprintf("err %d %f %d\\n", i, outv[i], (int)exp[i]); break;\n')
			ofd.write('}\n')
			ofd.write('}\n')
	ofd.write("}\n")

	regmap = {}
	for mode, elemsize, chsize in [
		("single", 1, 1),
		("vector", 1, 1), ("vector", 2, 2), ("vector", 3, 3), ("vector", 4, 4),
		("matrix", 2, 4), ("matrix", 3, 9), ("matrix", 4, 16)]:
		if mode not in regmap: regmap[mode] = {}
		regmap[mode][elemsize] = {}
		ofd.write("static const unsigned char rmap_%s%d[] = {\n" % (mode, elemsize))
		for elem in reginfo.genvect(mode, elemsize):
			regmap[mode][elemsize][elem[0]] = elem[1]
			ofd.write("%d,\n" % elem[0])
		ofd.write("};\n")
		ofd.write("static const unsigned char subreg_map_%s%d[][%d] = {\n" % (mode, elemsize, chsize))
		for elem in reginfo.genvect(mode, elemsize):
			ofd.write("{%s},\n" % ",".join(str(x) for x in elem[1]))
		ofd.write("};\n")
		ofd.write("static const uint64_t reg_mask_%s%d[][2] = {\n" % (mode, elemsize))
		for elem in reginfo.genvect(mode, elemsize):
			bitmask = 0
			for x in elem[1]: bitmask |= (1 << x)
			ofd.write("{%s},\n" % ",".join("0x%016xULL" % ((bitmask >> (64*i)) & 0xffffffffffffffff) for i in range(2)))
		ofd.write("};\n")

	# Generate pre-baked load/store register immediate (without GPR nor opcode)
	# For a faster codegen (bit shuffling is rather slow on MIPS)
	ofd.write("static const uint32_t reg_load_inst[128] = {\n")
	for rn in range(128):
		instenc = instparse.insts["lv.s"].encoding()
		flds = instenc.fields()
		instenc.fill("rthi", rn >> flds["rtlo"]["size"])
		instenc.fill("rtlo", rn & ((1 << flds["rtlo"]["size"])-1))
		ofd.write("0x%08x," % instenc.baseword())
	ofd.write("};\n")
	ofd.write("static const uint32_t reg_save_inst[128] = {\n")
	for rn in range(128):
		instenc = instparse.insts["sv.s"].encoding()
		flds = instenc.fields()
		instenc.fill("rthi", rn >> flds["rtlo"]["size"])
		instenc.fill("rtlo", rn & ((1 << flds["rtlo"]["size"])-1))
		ofd.write("0x%08x," % instenc.baseword())
	ofd.write("};\n")

	# Generate now the tests for register compatibility
	ofd.write("#define regcol(s1, s2) ((((s1)[0] & (s2)[0]) | ((s1)[1] & (s2)[1])) != 0)\n")
	ofd.write("#define regcompatcol(r1, s1, r2, s2) (((s1)[0] == (s2)[0]) && ((s1)[1] == (s2)[1]) && (((r1) & 0x20) == ((r2) & 0x20)))\n")
	ofd.write("static uint32_t bg_noise[128] __attribute__ ((aligned (16)));\n")
	ofd.write("static uint32_t instb[64];  // JIT buffer\n")
	ofd.write("#define jit_jump0(a0) ((void (*)(void *))instb)(a0);\n")
	ofd.write("#define jit_jump1(a0, a1) ((void (*)(void *, void *))instb)(a0, a1);\n")
	ofd.write("#define jit_jump2(a0, a1, a2) ((void (*)(void *, void *, void *))instb)(a0, a1, a2);\n")
	ofd.write("#define jit_finish() platform_cache_sync(instb, sizeof(instb));\n")
	ofd.write("int run_reg_tests(struct register_compat_info *reginfo) {\n")
	ofd.write("int entrycnt = 0;\n")
	for instname, iobj in instparse.insts.items():
		if iobj.pcode() is None:
			continue
		if not re.match(args.instregex, instname):
			continue
		if re.match("vcmov[tf].*", instname):
			continue    # Tricky to test, and no need really
		if iobj.numelems() == 0:
			continue   # Only pure VFPU insts for now

		# Prepare input registers
		regsin  = iobj.inputs()
		regsout = iobj.outputs()
		allregs = regsin | regsout
		ienc = iobj.encoding()
		# Only support one output reg for now
		assert len(regsout) <= 1
		(oregname, oregtype), = regsout.items()
		if oregtype == "vfpucc":
			continue

		ofd.write("{ // Tests for instruction %s (%s)\n" % (instname, iobj.title()))
		ofd.write("int dbgcnt = 0;\n")
		ofd.write('logprintf("Testing reg compat for %s\\n");\n' % instname)
		ofd.write('reginfo[entrycnt].inst_name = "%s";\n' % instname)

		itregs = sorted([reg for reg in allregs if allregs[reg] != "vfpucc"],
			key=lambda x: ("0" + x if x in regsin else "1" + x))

		# Data type (usually float, can be overriden)
		alutype = {}
		defalutype = iobj.extrattrs().get("alu-mode", "float")
		for nreg, reg in enumerate(regsin):
			if allregs[reg] != "vfpucc":
				# Figure out data type for this input
				if isinstance(defalutype, dict):
					alutype[reg] = defalutype[reg]
				else:
					alutype[reg] = defalutype
			else:
				# CC is implicit, not encoded in the inst
				alutype[reg] = "integer"   # Always int (6 bit)

		# Same for output
		if isinstance(defalutype, dict):
			alutype[oregname] = defalutype[oregname]
		else:
			alutype[oregname] = defalutype

		modesizes = {regn: expsize(allregs[regn], iobj.numelems())
		             for regn in allregs}
		regsizes = {regn: cntelem(allregs[regn], modesizes[regn])
		            for regn in allregs}
		regtypes = {regn: allregs[regn].split(":")[0]
		            for regn in allregs}

		ofd.write("gen_full_rand(0x%x, &bg_noise[0], 128);\n" % ienc.baseword())
		ofd.write("uint32_t basew = 0x%08x;\n" % ienc.baseword())

		# Prepare epilogue
		prolsize = sum(regsizes[x] for x in itregs if x in regsin)
		stubsize = sum(regsizes[x] for x in itregs)
		ofd.write("instb[%d] = 0x03e00008; // jr ra\n" % (stubsize + 1))
		ofd.write("instb[%d] = 0x00000000; // nop\n" % (stubsize + 2))

		# Perform more than one test
		ofd.write("for (unsigned i = 0; i < %d; i++) {\n" % (args.testcnt ** len(regsin)))
		# Generate input data and place it in the designated registers
		regtype = {"integer": "unsigned int", "sinteger": "int", "float": "float"}
		gentype = {"integer": "u32", "sinteger": "s32", "float": "f32"}
		for nreg, reg in enumerate(regsin):
			ofd.write("%s %s_ref[%d];\n" % (regtype[alutype[reg]], reg, regsizes[reg]))
			ofd.write("gen_inputs_%s(i >> %d, &%s_ref[0], %d);\n" % (gentype[alutype[reg]], nreg * math.log2(args.testcnt), reg, regsizes[reg]))

		# Args to pass to the JIT'ed code
		reg_args = []
		for nreg, reg in enumerate(sorted(regsin)):
			if regsin[reg] == "vfpucc":  # VFPU CC uses mtvc
				ofd.write('asm volatile("mtvc %0, $131"::"r"(pack6(vfpu_cc_ref)));\n')
				reg_args.append("0")
			else:                        # Emit the load instructions necessary
				reg_args.append("%s_ref" % reg)

		# Iterate for every immediate (if any)
		immdict = iobj.immediates()
		immn = iobj.immediates().keys()
		imml = [smart_range(immdict[i]["minval"], immdict[i]["maxval"])
		        for i in immn]

		# Generate immediate list (is hot-patched like register numbers)
		immprod = list(itertools.product(*imml))
		if immn:
			ofd.write("const unsigned int immseq[][%d] = {\n" % len(immn));
			for immvals in immprod:
				ofd.write("{" + ",".join("%d" % x for x in immvals) + "},\n")
			ofd.write("};\n")

		# We do not have more than one imm per instruction really
		if immn:
			ofd.write("for (unsigned imcnt = 0; imcnt < %d; imcnt++) {\n" % len(immprod))
			for imnum, imname in enumerate(immn):
				foff, fsz = ienc.fields()[imname]["lsb"], ienc.fields()[imname]["size"]
				ofd.write("basew = (basew & (~(0x%x << %d))) | (immseq[imcnt][%d] << %d);\n" % ((1<<fsz)-1, foff, imnum, foff))
		else:
			ofd.write("{\n")

		ofd.write("%s %s[%d], %s_expected[%d] = {0};\n" % (
			regtype[alutype[oregname]], oregname, regsizes[oregname], oregname, regsizes[oregname]))

		# Generate reference code, this is used as reference value to compare against
		# Using registers 0, 4 and 8 to ensure there's no overlap possible
		psize, basep, basec = 0, 0, 0xffffffff
		ofd.write("// Reference code/output\n")
		ofd.write('load_all_vfpu_regs((float*)&bg_noise[0]);\n')   # Initialize all regs
		for nreg, reg in enumerate(itregs):
			regoff = ienc.fields()[reg]["lsb"]
			assert ienc.fields()[reg]["size"] == 7 and ienc.fields()[reg]["value"] is None
			basep |= ((nreg*4) << regoff)
			basec &= ~(0x7f << regoff)
			for j in range(regsizes[reg]):
				subregs = regmap[regtypes[reg]][modesizes[reg]][nreg*4]
				if reg in regsin:
					ofd.write('instb[%d] = 0x%08x;\n' % (psize + j,
						emit_mem_full("lv.s", subregs[j], j, MIPS_REG_A1 + nreg)))
				else:
					ofd.write('instb[%d + 1] = 0x%08x;\n' % (psize + j,
						emit_mem_full("sv.s", subregs[j], j, MIPS_REG_A0)))
			psize += regsizes[reg]
		ofd.write('instb[%d] = (basew & 0x%08x) | 0x%08x; // Setup instruction word\n' % (prolsize, basec, basep))
		ofd.write("jit_finish();\n")
		ofd.write("jit_jump%d(%s);  // Calls the asm code and returns\n" % (len(reg_args), ",".join([oregname] + reg_args)))

		# Store result for later checks
		ofd.write("memcpy(%s_expected, %s, 4*%d);  // Save for later\n" % (oregname, oregname, regsizes[oregname]))

		preg, psize = None, 0
		for nreg, reg in enumerate(itregs):
			# Insert output reg logic (which needs to act as an epiloge
			if reg not in regsin and (preg in regsin or preg is None):
				# Ensure input registers do not overlap, it's a pain and we only interested in input-output overlap!
				# This code goes here for efficiency
				itinregs = set(itregs) & set(regsin)
				assert len(itinregs) <= 2
				if len(itinregs) == 2:
					op1, op2 = sorted(itinregs)
					ofd.write("if (regcol(reg_mask_%s%d[reg_%s], reg_mask_%s%d[reg_%s]))" % (
						regtypes[op1], modesizes[op1], op1, regtypes[op2], modesizes[op2], op2))
					ofd.write("continue;  // Input overlap!\n")

			regoff = ienc.fields()[reg]["lsb"]
			assert ienc.fields()[reg]["size"] == 7 and ienc.fields()[reg]["value"] is None
			ofd.write("for (unsigned reg_%s = 0; reg_%s < %d; reg_%s++) {\n" % (
				reg, reg, reginfo.regcount(regtypes[reg], modesizes[reg]), reg))
			ofd.write("basew = (basew & (~(0x7f << %d))) | (rmap_%s%d[reg_%s] << %d);\n" % (
				regoff, regtypes[reg], modesizes[reg], reg, regoff))
			ofd.write("for (unsigned p = 0; p < %d; p++)\n" % regsizes[reg]);
			if reg in regsin:
				ofd.write('instb[%d + p] = %s | %s;\n' % (psize,
					emit_mem_partial("lv.s", "p", MIPS_REG_A1 + nreg),
					"reg_load_inst[subreg_map_%s%d[reg_%s][p]]" % (regtypes[reg], modesizes[reg], reg)))
			else:
				ofd.write('instb[%d + p + 1] = %s | %s;\n' % (psize,
					emit_mem_partial("lv.s", "p", MIPS_REG_A0),
					"reg_save_inst[subreg_map_%s%d[reg_%s][p]]" % (regtypes[reg], modesizes[reg], reg)))
			preg = reg
			psize += regsizes[reg]

		# Generate inputs and move them to input registers
		ofd.write('load_all_vfpu_regs((float*)&bg_noise[0]);\n')   # Initialize all regs
		ofd.write('instb[%d] = basew; // Setup instruction word\n' % prolsize)

		ofd.write("jit_finish();\n")
		ofd.write("jit_jump%d(%s);  // Calls the asm code and returns\n" % (len(reg_args), ",".join([oregname] + reg_args)))

		# Perform comparisons now if we have a previous result
		ofd.write("if (memcmp(%s, %s_expected, 4*%d)) {\n" % (oregname, oregname, regsizes[oregname]))
		ofd.write('reginfo[entrycnt].test_errs++;\n')

		for reg in regsin:
			if regsin[reg] == "vfpucc": continue
			ofd.write("int col_%s = regcol(reg_mask_%s%d[reg_%s], reg_mask_%s%d[reg_%s]);\n" % (
				reg, regtypes[reg], modesizes[reg], reg, regtypes[oregname], modesizes[oregname], oregname))
			ofd.write("int compat_%s = regcompatcol(rmap_%s%d[reg_%s], reg_mask_%s%d[reg_%s], rmap_%s%d[reg_%s], reg_mask_%s%d[reg_%s]);\n" % (
				reg, regtypes[reg], modesizes[reg], reg, regtypes[reg], modesizes[reg], reg,
				regtypes[oregname], modesizes[oregname], oregname, regtypes[oregname], modesizes[oregname], oregname))
		
		ofd.write("if (%s) {\n" % "||".join(["0"] + [
			"col_%s" % reg for reg in regsin if regsin[reg] != "vfpucc"]))
		ofd.write("if (%s)\n" % "&&".join(["1"] + [
			"(compat_%s || !col_%s)" % (reg, reg) for reg in regsin if regsin[reg] != "vfpucc"]))
		ofd.write('reginfo[entrycnt].compat_overlap_errors++;\n')
		ofd.write("else\n")
		ofd.write('reginfo[entrycnt].incompat_overlap_errors++;\n')
		ofd.write("} else\n")
		ofd.write('reginfo[entrycnt].nonoverlap_errors++;\n')

		ofd.write('if (dbgcnt < REG_PAIRS_DEBUGS) {\n')
		for nreg, reg in enumerate(itregs):
			ofd.write('reginfo[entrycnt].reg_pairs[dbgcnt][%d] = rmap_%s%d[reg_%s];\n' % (nreg, regtypes[reg], modesizes[reg], reg))
		ofd.write("dbgcnt++;\n")
		ofd.write("}\n")
		ofd.write("}\n")
		ofd.write('reginfo[entrycnt].test_cnt++;\n')

		for reg in reversed(itregs):
			ofd.write("}  // for reg_%s\n" % reg)

		ofd.write("}\n")
		ofd.write("} // test case iteration 'i' end\n")
		ofd.write("} // test case end for %s\n" % instname)
		ofd.write('entrycnt++;\n')

	ofd.write("return entrycnt;\n")
	ofd.write("}\n")


