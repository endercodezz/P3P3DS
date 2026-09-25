#!/usr/bin/env python3

# Copyright 2022 David Guillen Fandos <david@davidgf.net>

# Generates human-readable documentation.

import os, argparse, mako.template, instparse, markdown, re, util
from htmlmin.main import minify

parser = argparse.ArgumentParser(prog='gen-docs')
parser.add_argument('--format', dest='format', default="html", type=str, help='Output format: html, pdf')
parser.add_argument('--output', dest='output', required=True, help='Output file')

args = parser.parse_args()

class MatrixProcessor(markdown.preprocessors.Preprocessor):
	RE_HEAD_FOOT = "^[ ]*[-]{27}[ ]*$"
	RE_MTX_CNT = "^[ ]*[ A-Z0-9\\|\\+-]+[ ]*$"
	RE_HOR_SEP = "^[\\|\\+-]+$"

	def __init__(self, md, config):
		super().__init__(md)
		self.config = config

	def mtxrender(self, lines):
		ret = '<text x="150" y="25" class="small" text-anchor="middle" dominant-baseline="middle">%s</text>' % lines[0].strip()
		linen, mc = 1, [[False]*4]*4
		for i in range(4):  # Draw weak separation lines
			for j in range(4):
				ret += '<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="rgb(150,150,150)" stroke-width="1px" />' % (
					75*(i), 50*(j+1), 75*i, 50*(j+2))
				ret += '<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="rgb(150,150,150)" stroke-width="1px" />' % (
					75*(i), 50*(j+1), 75*(i+1), 50*(j+1))
		for r in [x.strip() for x in lines[1:]]:
			if re.match(self.RE_HOR_SEP, r):
				ret += '<line x1="0" y1="%d" x2="300" y2="%d" stroke="black" stroke-width="3px" />' % (50*linen, 50*linen)
			else:
				for j in range(5):
					if r[j*7] in "+|":
						ret += '<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="black" stroke-width="3px" />' % (j*75, linen*50, j*75, (linen+1)*50)
				for j in range(4):
					t = r[j*7+1:j*7+7].strip()
					if t:
						mc[linen-1][j] = True
						ret += '<text x="%d" y="%d" class="small" text-anchor="middle" dominant-baseline="middle">%s</text>' % (j*75+36, linen*50+25, t)
				linen += 1
		return '<div class="matrixsvgdiv"><svg class="matrixsvg" viewbox="-5 0 310 255" height="100%">' + ret + '</svg></div>'

	def run(self, lines):
		olines, i = [], 0
		while i < len(lines):
			matched = False
			if re.match(self.RE_HEAD_FOOT, lines[i]):  # Find a matrix start
				for j in range(i+1, len(lines)):       # Find the closing line
					if re.match(self.RE_HEAD_FOOT, lines[j]) and j-i >= 4 and j-i <= 8:
						if all(re.match(self.RE_MTX_CNT, l) for l in lines[i+1:j]):
							olines[-1] = self.mtxrender(lines[i-1:j+1])
							i = j  # Skip matrix
							matched = True

			if not matched:
				olines.append(lines[i])
			i += 1

		return olines


class MatrixExtension(markdown.extensions.Extension):
	def __init__(self, **kwargs):
		self.config = {
			'use_align_attribute': [False, 'True to use align attribute instead of style.'],
		}
		super().__init__(**kwargs)

	def extendMarkdown(self, md):
		processor = MatrixProcessor(md, self.getConfigs())
		md.preprocessors.register(processor, 'vfpu-matrix', 100)

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

if os.path.exists(args.output):
	print("Output file already exists!")
	os._exit(1)

# Process manual pages
pretoc, posttoc = "", ""
prepages, postpages = [], []
for doc in ["introduction"]:
	mdtxt = open("docs/%s.md" % doc, "r").read()
	md = markdown.Markdown(extensions=['toc', 'fenced_code', MatrixExtension()])
	prepages.append(md.convert(mdtxt))
	pretoc += md.toc

for doc in ["bugs", "references"]:
	mdtxt = open("docs/%s.md" % doc, "r").read()
	md = markdown.Markdown(extensions=['toc', 'fenced_code'])
	postpages.append(md.convert(mdtxt))
	posttoc += md.toc

# Generate instruction list and all its attributes
instlist = []
for instname, iobj in instparse.insts.items():
	# Generate prefix support magic
	FULLSRC = "Full support (swizzle, abs(), neg() and constants)"
	PARTSRC = "Partial support (swizzle only)"
	FULLDST = "Full support (masking and saturation)"
	PARTDST = "Partial support (masking only)"
	NOSUPPT = "Not supported"
	pfxmap = {
		"s": { "rs": FULLSRC },
		"S": { "rs": PARTSRC },
		"t": { "rt": FULLSRC },
		"d": { "rd": FULLDST },
		"D": { "rd": PARTDST },
		"s0": {}, "t0": {}, "d0": {},
	}
	if iobj.flavor() == "s":
		pfxmap["s0"] = pfxmap["s"]
	if iobj.flavor() == "s":
		pfxmap["t0"] = pfxmap["t"]
	if iobj.flavor() == "s":
		pfxmap["d0"] = pfxmap["d"]
	prefdict = {}
	for elem in iobj.prefix_compat():
		prefdict.update(pfxmap[elem])
	rem = (set(iobj.inputs().keys()) | set(iobj.outputs().keys())) - set(prefdict.keys())
	for reg in set(["rs", "rt", "rd"]) & rem:
		prefdict[reg] = NOSUPPT

	# Generate pseudo-code
	# Make it more human readable :)
	pcode = iobj.pcode()
	if pcode:
		if pcode.startswith("@") or pcode.startswith("%"):
			(outreg, outregt), = iobj.outputs().items()
			lansz = expsize(outregt, iobj.numelems())
			regsz = cntelem(outregt, lansz)

			numit = regsz if pcode.startswith("@") else lansz
			# Expand this pcode for each lane
			pcode = [pcode[2:]] * numit
			for i in range(numit):
				c = i % lansz
				r = i // lansz
				pcode[i] = pcode[i].replace("{i}", str(i)).replace("{c}", str(c)).replace("{r}", str(r))
			pcode = ";\n".join(pcode)

		pcode = re.sub(r"[\n\s]+", " ", pcode)
		pcode = [x.strip(" \t;") for x in pcode.split(";")]
		pcode = [ util.simplify_code(x) or x for x in pcode ]

		pcodeep = iobj.pepilogue()
		if pcodeep:
			pcodeep = re.sub(r"[\n\s]+", " ", pcodeep)
			pcodeep = [x.strip(" \t;") for x in pcodeep.split(";")]
			pcodeep = [ util.simplify_code(x) or x for x in pcodeep ]
			pcode += pcodeep

	imms = {}
	for immname, imminfo in iobj.immediates().items():
		if "enum" in imminfo:
			cs = imminfo["enum"]
			if isinstance(cs, dict):
				imms = cs["0sptq"[iobj.numelems()]]
			else:
				imms = cs
			imms = {i + imminfo["minval"]: x for i, x in enumerate(imms)}

	# Inst encoding, fields and fixed bits
	instlist.append({
		"name": instname,
		"title": iobj.title(),
		"syntax": iobj.syntax(),
		"description": iobj.description(),
		"auxfns": iobj.extrafns(),
		"prefixes": prefdict.items(),
		"accuracy": iobj.extrattrs().get("accuracy", None),
		"perf-info": iobj.perfinfo(),
		"hazard": iobj.extrattrs().get("hazard", None),
		"register_compat": iobj.register_compat(),
		"bugs": iobj.extrattrs().get("bugs", None),
		"notes": iobj.extrattrs().get("notes", None),
		"pseudocode": "\n".join(pcode) if pcode else None,
		"fields": iobj.encoding().fields(),
		"baseword": iobj.encoding().baseword(),
		"basemask": iobj.encoding().basemask(),
		"imm-values": imms,
	})

tmpl = open("docs/templates/template-%s.html" % args.format, "r").read()
t = mako.template.Template(tmpl)
docout = t.render(
	instr_list=instlist,
	functions=instparse.functions,
	prepages=prepages,
	postpages=postpages,
	pretoc=pretoc, posttoc=posttoc,
)

# Process non-autogenerated docs too.
with open(args.output, "w") as ofd:
	ofd.write(minify(docout))


