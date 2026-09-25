#!/usr/bin/env python3

# Copyright 2025 David Guillen Fandos <david@davidgf.net>

# Testing helper for binutils/as
#
# This script can compare the output of two "as" builds (for PSP's allegrex)
# It generates text input asm files and runs them through as. It then uses
# objcopy on the output file to generate a raw binary file and compares them.
#
# It also checks assembly errors. In general it only generates valid instruction
# inputs.

import argparse, re, subprocess, struct, uuid, os, instparse, reginfo, multiprocessing, itertools
from tqdm import tqdm

COLRED = '\033[91m'
COLGREEN = '\033[92m'
COLRESET = '\033[0m'

parser = argparse.ArgumentParser(prog='asm-tester')
parser.add_argument('--reference', dest='reference', required=True, help='Path (or executable within PATH) to invoke reference `as`')
parser.add_argument('--undertest', dest='undertest', required=True, help='Path (or executable within PATH) to invoke for `as`')
parser.add_argument('--objcopy', dest='objcopy', required=True, help='Path (or executable within PATH) to invoke for `objcopy`')
parser.add_argument('--instr', dest='instregex', default=".*", help='Instructions to emit (a regular expression)')
parser.add_argument('--threads', dest='nthreads', type=int, default=8, help='Number of threads to use')

args = parser.parse_args()

def tmpfile(itnum=0, name="as"):
  return "/tmp/%s-test-%d-%s" % (name, itnum, str(uuid.uuid4()))

def run_sidebyside(asmfile):
  # Process asm files and generate two object files. Then proceed to dump them.
  objf1 = tmpfile(name="obj")
  objf2 = tmpfile(name="obj")

  p1 = subprocess.run([args.reference, "-march=allegrex", "-o", objf1, asmfile],
    stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
  p2 = subprocess.run([args.undertest, "-march=allegrex", "-o", objf2, asmfile],
    stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

  exit_code1 = p1.returncode
  exit_code2 = p2.returncode
  if exit_code1 != 0 or exit_code2 != 0:
    return (False, exit_code1, exit_code2)

  rawf1 = tmpfile(name="bin")
  rawf2 = tmpfile(name="bin")

  p1 = subprocess.run([args.objcopy, '-O', 'binary', '--only-section=.text', objf1, rawf1],
    stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
  p2 = subprocess.run([args.objcopy, '-O', 'binary', '--only-section=.text', objf2, rawf2],
    stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

  assert p1.returncode == 0 and p2.returncode == 0

  os.unlink(objf1)
  os.unlink(objf2)

  # Compare both files
  same = open(rawf1, "rb").read() == open(rawf1, "rb").read()
  os.unlink(rawf1)
  os.unlink(rawf2)

  return (same, None, None)

def run_errors(expected_insts, asmfile):
  # These files should produce a ton of errors, each line should be invalid.
  objf1 = tmpfile(name="obj")

  p1 = subprocess.Popen([args.reference, "-march=allegrex", "-o", objf1, asmfile],
    stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)

  outp1 = p1.communicate()
  p1.wait()
  exit_code1 = p1.poll()

  if exit_code1 != 0:
    # Validate there's error lines
    errlog = outp1[1].decode("ascii").strip()
    numerrs = errlog.count("\n")
    if numerrs == expected_insts:
      return (True, None)

  return (False, asmfile)

def dict_product(indict):
  return (dict(zip(indict.keys(), values)) for values in itertools.product(*indict.values()))

def expsize(regtype, lanes):
  modif = 1
  if ":" in regtype:
    regtype, modif = regtype.split(":")
    modif = {"D": 2, "Q": 4, "H": 0.5, "T": 0.25}[modif]
  return int(modif*lanes)

# Given a dict of regs and types, returns all the permutations of register names for the
# instruction. Returns a reg map (from reg name to subregs) as well.
def gencombs(instname, variables, elemcnt):
  # FIXME: Some weird case with load/store insts:
  if not elemcnt and instname.endswith(".q"):
    elemcnt = 4

  combos, subreginfo = {}, {}
  for v, vtype in variables.items():
    if vtype == "gpr":
      combos[v] = ["$%d" % i for i in range(32)]
    else:
      nume = expsize(vtype, elemcnt)
      regtype = vtype.split(":")[0]
      combos[v] = []
      for regnum, subregs in reginfo.genvect(regtype, nume):
        regname = reginfo.regpname(regtype, nume, regnum)
        combos[v].append(regname)
        subreginfo[regname] = subregs
  return (dict_product(combos), subreginfo)

# Given a list of immediates generate all their possible values and combinations
def genimms(imms, numel):
  combos = {}
  for v, iinfo in imms.items():
    combos[v] = []
    if iinfo.get("type", None) == "enum":
      cs = iinfo["enum"]
      if isinstance(cs, dict):
        combos[v] = cs["0sptq"[numel]]
      else:
        combos[v] = cs
    else:
      for val in range(iinfo["minval"], iinfo["maxval"] + 1):
        combos[v].append(str(val))
  return dict_product(combos)

# Checks whether a reg combination is legal according to the reg overlap restrictions
def check_overlap(iobj, regcomb, subreginfo):
  if iobj.register_compat() == "no-overlap":
    for oreg in iobj.outputs():
      for ireg in iobj.inputs():
        subregso = subreginfo[regcomb[oreg]]
        subregsi = subreginfo[regcomb[ireg]]
        if set(subregso) & set(subregsi):
          return False       # Found common registers
  elif iobj.register_compat() == "partial-overlap":
    for oreg in iobj.outputs():
      for ireg in iobj.inputs():
        subregso = subreginfo[regcomb[oreg]]
        subregsi = subreginfo[regcomb[ireg]]
        if set(subregso) & set(subregsi):
          if subregso != subregsi:
            return False       # Found common non-identical registers
  return True

# Generate instruction patterns, along with their "empty" bit count.
allinsts = []
for instname, iobj in instparse.insts.items():
  if re.match(args.instregex, instname):
    allinsts.append((instname, iobj))

# Aggregate all bits toghether to get a number of instructions to generate
print("Testing %d different instructions!" % len(allinsts))

def process_block(instname, iobj, validinsts):
  regs = iobj.inputs() | iobj.outputs()

  if any(k for k, v in regs.items() if v.split(":")[0] not in ["single", "vector", "matrix", "vfpucc", "gpr"]):
    # TODO Support other reg types!
    print("Instruction", instname, "has some unsupported inputs/outputs", iobj.raw_syntax())
    return (True, instname, 0)

  # No need to allocate CC registers :D
  regs = {k:v for k, v in regs.items() if v != "vfpucc"}

  asmfile = tmpfile()

  # Generate al possible bit fields
  numinsts = 0
  with open(asmfile, "w") as fd:
    regit, subreginfo = gencombs(instname, regs, iobj.numelems())
    for varcomb in regit:
      # Fake one immediate if there are none. Something nicer would be better tho.
      imms = iobj.immediates() or {'dummyimm': {'type': 'interger', 'minval': 0, 'maxval': 0}}

      for immcomb in genimms(imms, iobj.numelems()):
        istr = iobj.raw_syntax()
        for vname, vval in varcomb.items():
          istr = istr.replace("%" + vname, vval)
        for iname, ival in immcomb.items():
          istr = istr.replace("%" + iname, ival)

        # Validate that this combination of registers is even valid
        if check_overlap(iobj, varcomb, subreginfo) == validinsts:
          fd.write(istr + "\n")
          numinsts += 1

  if numinsts > 0:
    # Run the disassemblers now!
    if validinsts:
      success, ec1, ec2 = run_sidebyside(asmfile)
      if not success:
        return (False, instname, ec1, ec2, asmfile)
    else:
      success, outp = run_errors(numinsts, asmfile)
      if not success:
        return (False, instname, asmfile)

  os.unlink(asmfile)
  return (True, instname, numinsts)

res = []
finfo = []
with multiprocessing.Pool(processes=args.nthreads) as executor:
  for instname, iobj in allinsts:
    res.append((executor.apply_async(process_block, (instname, iobj, True)), True))
    res.append((executor.apply_async(process_block, (instname, iobj, False)), False))

  executor.close()

  totalinsts, badinsts = 0, 0
  for r, t in tqdm(res):
    succ, *info = r.get()
    if succ is False:
      print(info)
    else:
      if t:
        totalinsts += info[1]
        finfo.append(("%s : %d " + COLGREEN + "good instructions" + COLRESET) % (info[0], info[1]))
      else:
        if info[1]:
          badinsts += info[1]
          finfo.append(("%s : %d " + COLRED + "bad instructions" + COLRESET) % (info[0], info[1]))

print("\n".join(finfo))
print("--------------")
print("Tested a total of %d instructions" % totalinsts)
print("Validated a total of %d bad instructions" % badinsts)


