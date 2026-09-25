#!/usr/bin/env python3

# Copyright 2022 David Guillen Fandos <david@davidgf.net>

# Instruction YAML parsing

import yaml, ast, copy, re

rdesc = yaml.load(open("inst-vfpu-desc.yaml", "r"), Loader=yaml.Loader)

def safe_eval(expr, instwidth):
	if isinstance(expr, int):
		return expr
	allops = {
		ast.Add: lambda x, y: x + y,
		ast.Sub: lambda x, y: x - y,
		ast.Mult: lambda x, y: x * y,
		ast.Div: lambda x, y: x / y,
		ast.USub: lambda x: -x,
	}
	def _eval(node):
		if isinstance(node, ast.Expression):
			return _eval(node.body)
		elif isinstance(node, ast.Name):
			varsd = {"N": instwidth}
			return varsd[node.id]
		elif isinstance(node, ast.Constant):
			return node.value
		elif isinstance(node, ast.BinOp):
			return allops[type(node.op)](_eval(node.left), _eval(node.right))
		elif isinstance(node, ast.UnaryOp):
			return unOps[type(node.op)](_eval(node.operand))
		else:
			raise SyntaxError("No handler for this operation")
	return _eval(ast.parse(expr, mode='eval'))

class Instruction(object):
	def __init__(self,
		name, bname, flavor, title, desc, encoding, opdinfo,
		regcompat, valprefix, perfinfo,
		pcode, pprologue, pepilogue, extrafns,
		extra_attrs, numelems):
		self._name = name
		self._bname = bname
		self._flavor = flavor
		self._title = title
		self._description = desc
		self._encoding = encoding
		self._opdinfo = opdinfo
		self._pcode = pcode
		self._pprologue = pprologue
		self._pepilogue = pepilogue
		self._extrafns = extrafns
		self._extra_attrs = extra_attrs
		self._numelems = numelems
		self._regcompat = regcompat
		self._valprefix = valprefix
		self._perfinfo = perfinfo

	def name(self):
		return self._name

	def basename(self):
		return self._bname

	def perfinfo(self):
		return self._perfinfo

	def flavor(self):
		return self._flavor

	def title(self):
		return self._title

	def description(self):
		return self._description

	def encoding(self):
		return copy.deepcopy(self._encoding)

	# Body of the instruction execution
	def pcode(self):
		return self._pcode

	# Prologue of the previous code
	def pprologue(self):
		return self._pprologue

	# Epilogue of the previous pcode
	def pepilogue(self):
		return self._pepilogue

	# Extra functions used by the code
	def extrafns(self):
		return self._extrafns

	def extrattrs(self):
		return self._extra_attrs

	def numelems(self):
		return self._numelems

	def immediates(self):
		return self._opdinfo.immediates()

	def inputs(self):
		return self._opdinfo.inputs()

	def outputs(self):
		return self._opdinfo.outputs()

	def syntax(self):
		return self._opdinfo.syntax().replace("%opcode", self._name).replace("%", "")

	def raw_syntax(self):
		return self._opdinfo.syntax().replace("%opcode", self._name)

	def register_compat(self):
		return self._regcompat

	def prefix_compat(self):
		return self._valprefix


class OperandInfo(object):
	def __init__(self, opinfo):
		self._syntax = opinfo["syntax"]
		self._immediates = {}
		for iname, iinfo in opinfo.get("immediates", {}).items():
			self._immediates[iname] = iinfo
		self._inputs = {}
		for iname, itype in opinfo.get("inputs", {}).items():
			self._inputs[iname] = itype
		self._outputs = {}
		for iname, itype in opinfo.get("outputs", {}).items():
			self._outputs[iname] = itype

	def syntax(self):
		return self._syntax

	def immediates(self):
		return self._immediates

	def inputs(self):
		return self._inputs

	def outputs(self):
		return self._outputs

	def copy(self):
		return copy.deepcopy(self)

class Encoding(object):
	def __init__(self, descr):
		# Expand the encoding bits (sequence of chars) into something usable
		e = descr["encoding"]
		fields = descr["fields"]
		self._fields = dict((v, k) for k, v in fields.items())
		assert len(self._fields) == len(fields)
		self._baseword = 0
		self._basemask = 0

		while e:
			c = e[0]

			# Fixed bits are allowed
			if c in "01":
				e = e[1:]
				if c == "1":
					self._baseword |= (1 << (len(e)))
				self._basemask |= (1 << (len(e)))
				continue

			# This is a field, count the number of bits it has
			nbits = len(e) - len(e.strip(c))
			e = e[nbits:]
			assert c in fields
			field_name = fields[c]
			self._fields[field_name] = {"lsb": len(e), "size": nbits, "value": None}

	def fields(self):
		# Returns all instruction fields and their current value
		return copy.deepcopy(self._fields)

	def baseword(self):
		return self._baseword

	def basemask(self):
		return self._basemask

	def fill(self, field, value):
		if isinstance(value, int):
			nbits = self._fields[field]["size"]
			if value < 0:
				# Allow encoding negative integers (as 2's complement)
				lowestval = -2**(nbits-1)
				assert value >= lowestval
				value = (2**nbits) + value   # Calculate the 2's complement

			assert nbits >= len(bin(value))-2
		else:
			assert len(value) == self._fields[field]["size"]
			value = int(value, 2)

		# Fills a field with a value
		assert field in self._fields
		self._fields[field]["value"] = value

		# Add fields to the baseword and mask
		self._baseword |= (value << self._fields[field]["lsb"])
		self._basemask |= (((1 << self._fields[field]["size"]) - 1) << self._fields[field]["lsb"])

	def copy(self):
		return copy.deepcopy(self)

	def __str__(self):
		fexp = ", ".join("%s:(%d,%d)[%s]" % (k, v["lsb"], v["size"], v["value"]) for k, v in self._fields.items())
		return "Encoding: 0x%08x/0x%08x {%s}" % (self._baseword, self._basemask, fexp)

# Parse aux functions used by some instructions
functions = {}
for fnname, fninfo in rdesc["functions"].items():
	functions[fnname] = fninfo

# Parse encodings first
encodings = {}
for encname, encdescr in rdesc["encodings"].items():
	encodings[encname] = Encoding(encdescr)

# Instruction performance classes
perf_classes = {}
for pname, pinfo in rdesc["pref-classes"].items():
	perf_classes[pname] = pinfo

# Parse operand info tables
operandsinfo = {}
for opname, opinfo in rdesc["instruction-operands"].items():
	operandsinfo[opname] = OperandInfo(opinfo)

flavorinfo = {
  None: {"size": 0},
  "s": {"size": 1},
  "p": {"size": 2, "size-m1": 1},
  "t": {"size": 3, "size-m1": 2},
  "q": {"size": 4, "size-m1": 3},
}

insts = {}
for iname, instd in rdesc["instructions"].items():
	# Instructions with flavors need to be expanded
	for fvn, flavor in enumerate(instd.get("flavors", [None])):
		# Calculate instruction encoding (fixed bits)
		enctype = instd["encoding"]
		assert enctype in encodings

		opdtype = instd["type"]
		assert opdtype in operandsinfo

		regcompat = instd.get("reg-compat", "overlap")
		valprefix = set(re.findall("[stdSTD]0?", instd.get("prefix", "")))

		encoding = encodings[enctype].copy()
		operands = operandsinfo[opdtype].copy()

		# Fill fixed fields (if any avail.)
		for f in encoding.fields():
			if f in instd:
				encoding.fill(f, instd[f])

		# Special fields sizehi/lo for flavored instructions
		iwidth = flavorinfo[flavor]["size"]
		if flavor:
			if "sizelo" in encoding.fields():
				encoding.fill("sizehi", (flavorinfo[flavor]["size"]-1) >> 1)
				encoding.fill("sizelo", (flavorinfo[flavor]["size"]-1)  & 1)
			elif "sizelo-m1" in encoding.fields():
				encoding.fill("sizehi-m1", (flavorinfo[flavor]["size-m1"]-1) >> 1)
				encoding.fill("sizelo-m1", (flavorinfo[flavor]["size-m1"]-1)  & 1)

		pinfo = None
		if "perf-class" in instd:
			pclass = perf_classes[instd["perf-class"]]
			pinfo = {
				"throughput": safe_eval(pclass["throughput"], iwidth),
				"latency": safe_eval(pclass["latency"], iwidth),
			}

		extra_attrs = {}
		if "accuracy" in instd:
			extra_attrs["accuracy"] = instd["accuracy"]
		if "hazard" in instd:
			extra_attrs["hazard"] = instd["hazard"]
		if "bugs" in instd:
			extra_attrs["bugs"] = instd["bugs"]
		if "notes" in instd:
			extra_attrs["notes"] = instd["notes"]
		if "edgecases" in instd:
			extra_attrs["edgecases"] = instd["edgecases"]
		if "alu-mode" in instd:
			extra_attrs["alu-mode"] = instd["alu-mode"]

		codeb = {}
		for cb in ["pprologue", "pepilogue", "pcode"]:
			codeb[cb] = instd.get(cb, None)
			if isinstance(codeb[cb], list):
				codeb[cb] = codeb[cb][fvn]
		fullname = iname + ("." + flavor if flavor else "")
		# Create instruction
		instr = Instruction(fullname, iname, flavor, instd["title"], instd["description"],
		                    encoding, operands, regcompat, valprefix, pinfo,
		                    codeb["pcode"], codeb["pprologue"], codeb["pepilogue"], instd.get("auxcode", []),
		                    extra_attrs, iwidth)
		insts[fullname] = instr


# Some basic validation of instructions
def validate():
	# Ensure there's no overlap between instructions
	ilist = []
	for instname, inst in insts.items():
		word = inst.encoding().baseword()
		mask = inst.encoding().basemask()

		for oname, oword, omask in ilist:
			if oword & omask == word & mask:
				print("%s [%08x|%08x] <-> %s [%08x|%08x]" % (
					instname, word, mask, oname, oword, omask))
				assert False

		ilist.append((instname, word, mask))

if __name__ == '__main__':
	validate()
	for iname, data in insts.items():
		print(iname, data.encoding())


