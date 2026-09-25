#!/usr/bin/env python3

# Copyright 2022 David Guillen Fandos <david@davidgf.net>

# Generates VFPU asm tests to run on the PSP
# The aim is to validate the instruction documentation.
# It validates instruction behaviour (as indicated by pseudo-C-code as well as
# instruction prefix validity.

import os, math, argparse, hashlib, re, itertools, instparse

parser = argparse.ArgumentParser(prog='gen-tests')
parser.add_argument('--instruction', dest='instregex', default=".*", type=str, help='Instruction name (regex)')
parser.add_argument('--output', dest='output', required=True, help='Output file template')
parser.add_argument('--check-prefixes', dest='prefcheck', action="store_true", help='Generate information on prefixes')
parser.add_argument('--num-tests', dest='testcnt', default=128, type=int, help='Number of tests')
parser.add_argument('--num-imm-tests', dest='testimmcnt', default=32, type=int, help='Number of immediate tests')
parser.add_argument('--rndseed', dest='rndseed', default=0xdeadbeef, type=int, help='Random seed')

args = parser.parse_args()

rndseed = args.rndseed.to_bytes(32, 'big')
def rndnum(minv, maxv):
	global rndseed
	rndseed = hashlib.sha256(rndseed).digest()
	intval = int.from_bytes(rndseed, 'big')
	return intval % (maxv - minv + 1) + minv


if os.path.exists(args.output):
	print("Output file already exists!")
	os._exit(1)

def pref_checks(allregs, regsizes):
	# Generate: no prefixes
	# Dest: mask, sat
	# Rs/Rt: swizzle, abs, neg, constant
	def getpfxw(base, value):
		ret = base.copy(); 
		ret.fill("imm24", value)
		assert ret.basemask() == 0xFFFFFFFF
		return ret.baseword()
	swzc = [None, 0xE4, 0xE1, 0xC9, 0x39]

	pd = instparse.insts["vpfxd"].encoding()
	ps = instparse.insts["vpfxs"].encoding()
	pt = instparse.insts["vpfxt"].encoding()

	for r, renc in [("rs", ps), ("rt", pt)]:
		# Do not accept register if unused or matrix/other
		if r not in regsizes or allregs[r].split(":")[0] not in ["single", "vector"]:
			continue
		yield (r, "swz", getpfxw(renc, swzc[regsizes[r]]))   # swizzle
		yield (r, "neg", getpfxw(renc, 0x500E4))   # negate
		yield (r, "neg", getpfxw(renc, 0xA00E4))
		yield (r, "neg", getpfxw(renc, 0xF00E4))
		yield (r, "abs", getpfxw(renc, 0x005E4))   # abs (some)
		yield (r, "abs", getpfxw(renc, 0x00AE4))
		yield (r, "abs", getpfxw(renc, 0x00FE4))
		yield (r, "cst", getpfxw(renc, 0x0F4A7))   # constants (1/2,1,1/4,2)

	if "rd" in allregs:
		yield ("rd", "msk", getpfxw(pd, 0x00F00))   # Mask all elements
		yield ("rd", "sat", getpfxw(pd, 0x00055))   # Do sat  0:1
		yield ("rd", "sat", getpfxw(pd, 0x000FF))   # Do sat -1:1

	yield (None, None, None)   # Do one round without prefix of course!

def compare_code(rtype, iobj, val, ref, ncnt):
	if rtype == "f32":
		einfo = iobj.extrattrs()
		if "accuracy" in einfo:
			if "absolute" in einfo["accuracy"]:
				return "fpabserrn(%s, %s, %e, %d)" % (val, ref, einfo["accuracy"]["absolute"], ncnt)
			elif "relative" in einfo["accuracy"]:
				return "fprelerrn(%s, %s, %e, %d)" % (val, ref, einfo["accuracy"]["relative"], ncnt)
			else:
				assert False
		else:
			# Exact results can suffer from rounding
			return "fpdifn(%s, %s, %d)" % (val, ref, ncnt)
	elif rtype == "u32":
		return "memcmp(%s, %s, %d*sizeof(uint32_t))" % (val, ref, ncnt)
	elif rtype == "s32":
		return "memcmp(%s, %s, %d*sizeof(int32_t))" % (val, ref, ncnt)

# Calculate operand size, supports reduction and expansion for vector.
def expsize(regtype, lanes):
	modif = 1
	if ":" in regtype:
		regtype, modif = regtype.split(":")
		modif = {"D": 2, "Q": 4, "H": 0.5, "T": 0.25}[modif]

	return int(modif*lanes)

# Full element size (vector, matrix...)
def cntelem(regtype, lanes):
	cnts = {"single": 1, "vector": lanes, "matrix": lanes*lanes, "vfpucc": 6}
	regtype = regtype.split(":")[0]
	return cnts[regtype]

def expand_pcode(pcode, regsz, lanesz):
	if pcode is None:
		return ""
	# Actual C emulation code
	if pcode.startswith("@") or pcode.startswith("%"):
		numit = regsz if pcode.startswith("@") else lanesz
		# Expand this pcode for each lane
		pcode = [pcode[2:]] * numit
		for i in range(numit):
			c = i % lanesz
			r = i // lanesz
			pcode[i] = pcode[i].replace("{i}", str(i)).replace("{c}", str(c)).replace("{r}", str(r))
		pcode = ";\n  ".join(pcode)

	return pcode

# Can't do full ranges, too big of a binary, test some corner cases instead
def smart_range(minval, maxval):
	if abs(maxval - minval) < args.testimmcnt:
		return range(minval, maxval + 1)
	cases = []
	for i in range(args.testimmcnt):
		cases.append(rndnum(minval, maxval))
	return [minval, maxval] + cases

# Group instructions by flavour to save on file count
instgroups = {}
for instname, iobj in instparse.insts.items():
	if not re.match(args.instregex, instname):
		continue
	if iobj.numelems() == 0:
		continue   # Only pure VFPU insts for now
	if iobj.pcode() is None:
		continue

	instgroups[iobj.basename()] = instgroups.get(iobj.basename(), []) + [(instname, iobj)]

# Generate instruction list and all its attributes
fnlist = []
for gname, ilist in instgroups.items():
	ofd = open(args.output % gname, "w")
	for f in ["<string.h>", "<math.h>", "<stdbool.h>", "<stdint.h>",
		      '"func-def.h"', '"test-utils.h"', '"prefix-func.h"']:
		ofd.write('#include %s\n' % f)

	allfns = set()
	for e in ilist:
		allfns |= set(e[1].extrafns())
	for fnname, fninfo in instparse.functions.items():
		if fnname in allfns:
			ofd.write("static " + fninfo["code"])

	for instname, iobj in ilist:
		pcode = iobj.pcode()

		# Prepare input registers
		regsin  = iobj.inputs() #{ reg:rtype for reg, rtype in iobj.inputs().items() if rtype != "vfpucc" }
		regsout = iobj.outputs()
		allregs = regsin | regsout
		ienc = iobj.encoding()
		# Only support one output reg for now
		assert len(regsout) <= 1
		(oregname, oregtype), = regsout.items()

		# Data type (usually float, can be overriden)
		alutype = {}
		defalutype = iobj.extrattrs().get("alu-mode", "float")
		for nreg, reg in enumerate(sorted(regsin)):
			if allregs[reg] != "vfpucc":
				ienc.fill(reg, 4*nreg)
				# Figure out data type for this input
				if isinstance(defalutype, dict):
					alutype[reg] = defalutype[reg]
				else:
					alutype[reg] = defalutype
			else:
				# CC is implicit, not encoded in the inst
				alutype[reg] = "boolean"   # Always int (6 bit)

		# Same for output
		if allregs[oregname] == "vfpucc":
			alutype[oregname] = "boolean"   # Always int (6 bit)
		else:
			ienc.fill(oregname, 4*4)
			if isinstance(defalutype, dict):
				alutype[oregname] = defalutype[oregname]
			else:
				alutype[oregname] = defalutype

		lanesizes = {regn: expsize(allregs[regn], iobj.numelems())
		             for regn in allregs}
		regsizes = {regn: cntelem(allregs[regn], lanesizes[regn])
		            for regn in allregs}

		# Generate C emulation code out of pcode
		fnname = "run_" + instname.replace(".", "_")
		ofd.write("int %s(struct test_error_info *errlist, struct prefix_usage_info *pfxinfo) {\n" % fnname)
		ofd.write("int errcnt = 0;\n")
		fnlist.append(fnname)

		ofd.write("// Tests for instruction %s (%s)\n" % (instname, iobj.title()))
		ofd.write("for (unsigned i = 0; i < %d; i++) {\n" % (args.testcnt ** len(regsin)))

		# Generate input data and place it in the designated registers
		regtype = {"integer": "unsigned int", "sinteger": "int", "float": "float", "boolean": "unsigned int",}
		for reg in sorted(regsin):
			ofd.write("%s %s_ref[%d];\n" % (regtype[alutype[reg]], reg, regsizes[reg]))

		gentype = {"integer": "u32", "sinteger": "s32", "float": "f32", "boolean": "bits"}
		for nreg, reg in enumerate(sorted(regsin)):
			ofd.write("gen_inputs_%s(i >> %d, &%s_ref[0], %d);\n" % (gentype[alutype[reg]], nreg * math.log2(args.testcnt), reg, regsizes[reg]))

		# Iterate for every immediate (if any)
		immdict = iobj.immediates()
		immn = iobj.immediates().keys()
		imml = [smart_range(immdict[i]["minval"], immdict[i]["maxval"])
		        for i in immn]
		# We do not have more than one imm per instruction really
		immproduct = list(itertools.product(*imml))
		# Generate prefix tests
		if args.prefcheck:
			pfxl = list(pref_checks(allregs, regsizes))
		else:
			pfxl = [(None, None, None)]

		ofd.write('pfxinfo->inst_name = "%s";\n' % instname)
		for r in ["rs", "rt", "rd"]:
			if r in [x[0] for x in pfxl]:
				ofd.write('pfxinfo->%s_tested = 1;\n' % r)

		for pfx, immvals in itertools.product(pfxl, immproduct):
			instenc = ienc.copy()
			pfxreg, pfxop, pfxw = pfx

			ofd.write("{\n")
			prefixed = []
			if pfxreg in ["rs", "rt"]:
				ofd.write("%s %s[%d];\n" % (regtype[alutype[pfxreg]], pfxreg, regsizes[pfxreg]))
				ofd.write("pfx_src((uint32_t*)&%s[0], (uint32_t*)&%s_ref[0], 0x%08x, %d);\n" % (pfxreg, pfxreg, pfxw, regsizes[pfxreg]))
				prefixed.append(pfxreg)
			for reg in sorted(set(regsin) - set(prefixed)):
				ofd.write("%s *%s = &%s_ref[0]; // No src prefix\n" % (regtype[alutype[reg]], reg, reg))

			ofd.write("%s %s[%d] = {0}, %s_out[%d] = {0};\n" % (
				regtype[alutype[oregname]], oregname, regsizes[oregname], oregname, regsizes[oregname]))

			# Generate inputs and move them to input registers
			ofd.write('asm volatile("mtvc $0, $131;vmzero.q M400");')   # Clear output state
			for nreg, reg in enumerate(sorted(regsin)):
				if regsin[reg] == "vfpucc":
					# VFPU CC uses mtvc
					ofd.write('asm volatile("mtvc %0, $131"::"r"(pack6(vfpu_cc)));\n')
				else:
					for i in range(regsizes[reg]):
						c = i % lanesizes[reg]
						r = i // lanesizes[reg]
						ofd.write('asm volatile("lv.s S%d%d%d, %d*4(%%0)"::"r"(&%s_ref[0]));\n' % (nreg, r, c, i, reg))

			for inum, imname in enumerate(immn):
				ofd.write("unsigned %s = %d;\n" % (imname, immvals[inum]))
				assert immvals[inum] >= 0
				instenc.fill(imname, immvals[inum])
			
			# Actual C emulation code
			code1 = expand_pcode(pcode, regsizes[oregname], lanesizes[oregname])
			code2 = expand_pcode(iobj.pepilogue(), regsizes[oregname], lanesizes[oregname])
			ofd.write(code1 + ";\n" + code2 + ";\n")

			if pfxreg == oregname:
				ofd.write("%s %s_final[%d] = {0};\n" % (regtype[alutype[pfxreg]], pfxreg, regsizes[pfxreg]))
				ofd.write("pfx_dst((uint32_t*)&%s_final[0], (uint32_t*)&%s[0], 0x%08x, %d);\n" % (pfxreg, pfxreg, pfxw, regsizes[pfxreg]))
			else:
				ofd.write("%s *%s_final = &%s[0]; // No dst prefix\n" % (regtype[alutype[oregname]], oregname, oregname))

			if instenc.basemask() != 0xFFFFFFFF:
				print("Error!", instname, "has missing fields")

			if pfxop:
				ofd.write('asm volatile(".word 0x%08x":::"memory");\n' % pfxw)
			ofd.write('asm volatile(".word 0x%08x":::"memory");\n' % instenc.baseword())

			# Validate output(s) and fill in error reports if necessary
			if oregtype == "vfpucc":
				ofd.write('asm volatile("vnop; mfvc %%0, $131":"=r"(%s_out[0]));\n' % (oregname))
				ofd.write('int mm = (pack6(vfpu_cc_final) != %s_out[0]);\n' % (oregname))
			else:
				for i in range(regsizes[oregname]):
					c = i % lanesizes[oregname]
					r = i // lanesizes[oregname]
					ofd.write('asm volatile("sv.s S%d%d%d, %d*4(%%0)"::"r"(&%s_out[0]));\n' % (4, r, c, i, oregname))
				ofd.write('int mm = (%s);\n' % (compare_code(gentype[alutype[oregname]], iobj,
				                                "&%s_final[0]" % oregname, "&%s_out[0]" % oregname, regsizes[oregname])))

			ofd.write("if (mm) {\n")
			if pfxop:
				ofd.write('struct test_error_info *rep = &pfxinfo->%s_%s_err;\n' % (pfxreg, pfxop))
			else:
				ofd.write('struct test_error_info *rep = &errlist[errcnt];\n')

			# Report an error: copy inputs and their info+data
			ofd.write('static const struct test_decl_info iinfo = {\n')
			ofd.write('.inst = "%s", .instw = 0x%08x,\n' % (instname, instenc.baseword()))
			ofd.write('.pfxw = 0x%08x, .rdoutsz = %d,\n' % (pfxw or 0, regsizes[oregname]))
			ofd.write('.inputs = {\n')
			for reg in sorted(regsin):
				ofd.write('{.inpname = "%s", .size = %d},\n' % (reg, regsizes[reg]))
			ofd.write('}};\n')

			ofd.write('fill_errinfo(rep, &iinfo, %s_final, %s_out);\n' % (oregname, oregname))
			for nreg, reg in enumerate(sorted(regsin)):
				if allregs[reg] == "vfpucc":
					ofd.write('fill_cc(rep->inputs[%d], pack6(vfpu_cc));\n' % nreg)
				else:
					ofd.write('fill_reg(rep->inputs[%d], %s_ref);\n' % (nreg, reg))

			if pfxop:
				# Just annotate whether this test was ok or not for the prefix report info
				ofd.write('pfxinfo->%s_errors |= (1 << enum_%s);\n' % (pfxreg, pfxop))
			else:
				# No need further test
				ofd.write('errcnt++; break;\n')

			ofd.write("}\n")
			ofd.write("}\n")

		ofd.write("}\n\n")

		ofd.write("return errcnt;\n")
		ofd.write("}\n")


with open(args.output % "main", "w") as ofd:
	for f in ["<string.h>", "<math.h>", "<stdbool.h>", "<stdint.h>",
		      '"func-def.h"', '"test-utils.h"', '"prefix-func.h"']:
		ofd.write('#include %s\n' % f)

	for fname in fnlist:
		ofd.write("int %s(struct test_error_info *errlist, struct prefix_usage_info *pfxinfo);\n" % fname)

	ofd.write("int run_tests(struct test_error_info *errlist, struct prefix_usage_info *pfxinfo) {\n")
	ofd.write("int errcnt = 0;\n")
	for fname in fnlist:
		ofd.write("errcnt += %s(&errlist[errcnt], pfxinfo++);\n" % fname)
	ofd.write("return errcnt;\n")
	ofd.write("}\n")

