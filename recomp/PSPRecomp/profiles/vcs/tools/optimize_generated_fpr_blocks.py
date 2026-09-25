#!/usr/bin/env python3
"""Conservative basic-block FPR register cache for generated AOT."""
from __future__ import annotations
import argparse, collections, pathlib, re
from dataclasses import dataclass
LABEL_RE = re.compile(r"(?m)^L_[0-9A-F]+:\n")
FPR_RE = re.compile(r"ctx\.fpr\[([0-9]|[12][0-9]|3[01])\]")
@dataclass
class Stats:
    blocks_cached:int=0; registers_cached:int=0; occurrences_replaced:int=0; dirty_registers:int=0
    def add(self,o):
        self.blocks_cached+=o.blocks_cached; self.registers_cached+=o.registers_cached
        self.occurrences_replaced+=o.occurrences_replaced; self.dirty_registers+=o.dirty_registers

def assign(line,name):
    return re.search(rf"(?<![=!<>])\b{re.escape(name)}\s*=(?!=)",line) is not None

def block_transform(label,block,threshold):
    st=Stats()
    if 'rt.' in block or 'ctx.execute_' in block or 'ctx.set_fpr_bits' in block or 'ctx.fpr_bits(' in block:
        return label+block,st
    c=collections.Counter(FPR_RE.findall(block)); sel=sorted(int(r) for r,n in c.items() if n>=threshold)
    if not sel:return label+block,st
    dirty={r for r in sel if re.search(rf'ctx\.fpr\[{r}\]\s*=(?!=)',block)}
    for r in sel:block=block.replace(f'ctx.fpr[{r}]',f'f{r}')
    out=[*(f'    float f{r} = ctx.fpr[{r}];\n' for r in sel)]
    active=set()
    def flush():
        nonlocal active
        if active:
            out.extend(f'    ctx.fpr[{r}] = f{r};\n' for r in sorted(active)); active=set()
    for line in block.splitlines(True):
        if 'if (branch_taken)' in line and active:flush()
        for r in dirty:
            if assign(line,f'f{r}'):active.add(r)
        if ('goto ' in line or 'return;' in line) and active:flush()
        out.append(line)
    flush()
    st.blocks_cached=1;st.registers_cached=len(sel);st.occurrences_replaced=sum(c[str(r)] for r in sel);st.dirty_registers=len(dirty)
    return label+'{\n'+''.join(out)+'}\n',st

def transform(text,threshold=3):
    ms=list(LABEL_RE.finditer(text)); total=Stats()
    if not ms:return text,total
    out=[];last=0
    for i,m in enumerate(ms):
        end=ms[i+1].start() if i+1<len(ms) else text.find('\n}\n\nvoid ',m.end())
        if end<0:end=len(text)
        out.append(text[last:m.start()]);t,st=block_transform(text[m.start():m.end()],text[m.end():end],threshold);out.append(t);total.add(st);last=end
    out.append(text[last:]);return ''.join(out),total

def main():
    ap=argparse.ArgumentParser();ap.add_argument('paths',nargs='+',type=pathlib.Path);ap.add_argument('--threshold',type=int,default=3);ap.add_argument('--check',action='store_true');a=ap.parse_args()
    fs=[]
    for p in a.paths:fs.extend(sorted(p.glob('generated_unit_*.cpp')) if p.is_dir() else [p])
    total=Stats()
    for p in fs:
        s=p.read_text();t,st=transform(s,a.threshold);total.add(st)
        if not a.check and t!=s:p.write_text(t,newline='\n')
    print(f'FPR block cache: files={len(fs)} blocks={total.blocks_cached} locals={total.registers_cached} occurrences={total.occurrences_replaced} dirty={total.dirty_registers} threshold={a.threshold} check={int(a.check)}')
if __name__=='__main__':main()
