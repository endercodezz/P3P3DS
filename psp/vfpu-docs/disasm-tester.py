#!/usr/bin/env python3

# Copyright 2025 David Guillen Fandos <david@davidgf.net>

# Testing helper for binutils/objdump
#
# This script can compare the output of two objdump builds (for PSP's allegrex)
# It generates binary blobs and runs them through objdump (binary mode) and
# compares their output.

import argparse, re, subprocess, struct, uuid, os, instparse, multiprocessing
from tqdm import tqdm

parser = argparse.ArgumentParser(prog='disasm-tester')
parser.add_argument('--reference', dest='reference', required=True, help='Path (or executable within PATH) to invoke reference `objdump`')
parser.add_argument('--undertest', dest='undertest', required=True, help='Path (or executable within PATH) to invoke for `objdump`')
parser.add_argument('--chunksize', dest='chunksize', type=int, default=128*1024, help='Block size (instruction count)')
parser.add_argument('--instr', dest='instregex', default=".*", help='Instructions to emit (a regular expression)')
parser.add_argument('--threads', dest='nthreads', type=int, default=8, help='Number of threads to use')

args = parser.parse_args()

def tmpfile(itnum=0):
  return "/tmp/objdump-test-%d-%s" % (itnum, str(uuid.uuid4()))

def run_sidebyside(binfile):
  p1 = subprocess.Popen([args.reference, "-D", "-b", "binary", "-m", "mips:allegrex", binfile],
    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  p2 = subprocess.Popen([args.undertest, "-D", "-b", "binary", "-m", "mips:allegrex", binfile],
    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  outp1 = p1.communicate()
  outp2 = p2.communicate()
  p1.wait()
  p2.wait()
  exit_code1 = p1.poll()
  exit_code2 = p2.poll()

  success = (exit_code1 == 0 and exit_code2 == 0 and outp1 == outp2)
  return (success, exit_code1, exit_code2, outp1, outp2)

def itchunk(num, chunksize):
  i = 0
  while i < num:
    yield i, min(num, i+chunksize)
    i += chunksize

# Generate instruction patterns, along with their "empty" bit count.
allinsts = []
for instname, iobj in instparse.insts.items():
  if re.match(args.instregex, instname):
    # Get all empty fields
    fds = iobj.encoding().fields()
    fds = sorted((k, v["lsb"], v["size"])
                 for k, v in fds.items() if v["value"] is None)
    nbits = sum(x[2] for x in fds)

    allinsts.append((instname, iobj, fds, nbits))

# Aggregate all bits toghether to get a number of instructions to generate
total_insts = sum(1 << x[3] for x in allinsts)
print("Testing %dM instructions!" % (total_insts // 1000000))

# Generate a list of chunks to process, to divide the work.
work = []
for instname, iobj, fds, nbits in allinsts:
  for start, stop in itchunk(1 << nbits, args.chunksize):
    work.append((instname, iobj, fds, start, stop))

def process_block(instname, iobj, fds, start, stop):
  binfile = tmpfile()

  # Base word to fill
  baseword = iobj.encoding().baseword()

  # Generate al possible bit fields
  with open(binfile, "wb") as fd:
    for n in range(start, stop):
      w, offset = baseword, 0
      for fld, lsb, size in fds:
        w |= ((n >> offset) & ((1 << size) - 1)) << lsb
        offset += size
      fd.write(struct.pack("<I", w))

  # Run the disassemblers now!
  success, ec1, ec2, out1, out2 = run_sidebyside(binfile)
  if not success:
    return (instname, ec1, ec2, binfile)
    os.exit(1)

  os.unlink(binfile)
  return None

res = []
with multiprocessing.Pool(processes=args.nthreads) as executor:
  for instname, iobj, fds, start, stop in work:
    r = executor.apply_async(process_block, (instname, iobj, fds, start, stop))
    res.append(r)

  executor.close()

  for r in tqdm(res):
    v = r.get()
    if v is not None:
      print(v)


