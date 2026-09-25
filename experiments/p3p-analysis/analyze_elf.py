#!/usr/bin/env python3
"""
Independent P3P Executable Analysis Tool (Relocation-Aware)
Parses the decrypted Sony PSP PRX/ELF file to extract:
- ELF headers, segments, sections (distinguishing executable vs data sections)
- Simulated guest memory load and full PRX relocation application
- Sony PSP Module Info, Export tables, Import tables (NIDs) read from relocated memory
- Relocation distribution (R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16)
- Disassembly breakdown of the actual .text section (MIPS, FPU, VFPU, control flow)
- Documented findings cross-checked against PSPRecomp
"""

import sys
import struct
import json
from collections import Counter, defaultdict

def analyze_p3p_elf(elf_path, base_addr=0x08804000):
    with open(elf_path, "rb") as f:
        data = f.read()

    # 1. ELF Header
    magic = data[:4]
    if magic != b"\x7fELF":
        raise ValueError(f"Invalid ELF magic: {magic}")

    ei_class = data[4]
    ei_data = data[5]
    e_type, e_machine, e_version, e_entry, e_phoff, e_shoff, e_flags = struct.unpack_from("<HHI4I", data, 0x10)
    e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<6H", data, 0x28)

    # 2. Program Headers (Segments)
    segments = []
    seg_spans = []
    seg_addrs = []
    for i in range(e_phnum):
        p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = struct.unpack_from(
            "<8I", data, e_phoff + i * e_phentsize
        )
        runtime_addr = base_addr + p_vaddr
        segments.append({
            "index": i,
            "type": p_type,
            "type_hex": f"0x{p_type:08X}",
            "offset": p_offset,
            "vaddr_relative": f"0x{p_vaddr:08X}",
            "vaddr_runtime": f"0x{runtime_addr:08X}",
            "filesz": p_filesz,
            "memsz": p_memsz,
            "flags": p_flags,
            "flags_desc": (("R" if p_flags & 4 else "-") + ("W" if p_flags & 2 else "-") + ("X" if p_flags & 1 else "-")),
            "align": p_align
        })
        seg_addrs.append(runtime_addr)
        seg_spans.append((p_type, p_offset, p_filesz, p_memsz, runtime_addr))

    # 3. Section Headers
    sections = []
    strtab = b""
    if e_shnum > 0 and e_shstrndx < e_shnum:
        sh_str = data[e_shoff + e_shstrndx * e_shentsize : e_shoff + (e_shstrndx + 1) * e_shentsize]
        str_offset, str_size = struct.unpack_from("<II", sh_str, 16)
        strtab = data[str_offset : str_offset + str_size]

    text_section = None
    rodata_sections = []
    data_sections = []
    rel_sections = []
    exec_sections = []

    for i in range(e_shnum):
        sh = data[e_shoff + i * e_shentsize : e_shoff + (i + 1) * e_shentsize]
        name_idx, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link, sh_info, sh_addralign, sh_entsize = struct.unpack_from(
            "<10I", sh, 0
        )
        sec_name = strtab[name_idx:].split(b"\x00")[0].decode("latin1", "ignore")
        sec_info = {
            "index": i,
            "name": sec_name,
            "type": sh_type,
            "type_hex": f"0x{sh_type:08X}",
            "flags": sh_flags,
            "is_executable": bool(sh_flags & 0x4), # SHF_EXECINSTR
            "addr": f"0x{sh_addr:08X}",
            "runtime_addr": f"0x{sh_addr + base_addr:08X}",
            "offset": sh_offset,
            "size": sh_size,
            "link": sh_link,
            "info": sh_info,
            "addralign": sh_addralign,
            "entsize": sh_entsize
        }
        sections.append(sec_info)

        if sh_type == 0x700000A0: # SHT_PRXREL
            rel_sections.append(sec_info)
        if sh_flags & 0x4: # SHF_EXECINSTR
            exec_sections.append(sec_name)
        if sec_name == ".text":
            text_section = sec_info
        elif ".rodata" in sec_name:
            rodata_sections.append(sec_info)
        elif ".data" in sec_name:
            data_sections.append(sec_info)

    # 4. Simulate Guest Memory & Apply Relocations (matches PSPRecomp Elf32Image)
    max_addr = max(addr + p_memsz for p_type, p_offset, p_filesz, p_memsz, addr in seg_spans if p_type == 1)
    mem_size = max_addr - base_addr + 0x10000
    mem = bytearray(mem_size)

    def mem_write(addr, b):
        off = addr - base_addr
        mem[off : off + len(b)] = b

    def mem_read32(addr):
        off = addr - base_addr
        return struct.unpack_from("<I", mem, off)[0]

    def mem_write32(addr, val):
        off = addr - base_addr
        struct.pack_into("<I", mem, off, val & 0xFFFFFFFF)

    def relocated_hi(val):
        signed_low = struct.unpack("<h", struct.pack("<H", val & 0xFFFF))[0]
        corrected = (val - signed_low) & 0xFFFFFFFF
        return (corrected >> 16) & 0xFFFF

    # Load PT_LOAD segments into memory
    for p_type, p_offset, p_filesz, p_memsz, addr in seg_spans:
        if p_type == 1 and p_filesz > 0:
            mem_write(addr, data[p_offset : p_offset + p_filesz])

    # Apply PRX relocations
    relocation_stats = Counter()
    total_relocs = 0
    for rel_sec in rel_sections:
        rel_data = data[rel_sec["offset"] : rel_sec["offset"] + rel_sec["size"]]
        count = rel_sec["size"] // 8
        rels = [struct.unpack_from("<II", rel_data, k * 8) for k in range(count)]
        orig_ops = []
        valid = []
        for r_offset, r_info in rels:
            p_seg = (r_info >> 8) & 0xFF
            if p_seg >= len(seg_addrs):
                valid.append(False)
                orig_ops.append(0)
                continue
            patch_addr = seg_addrs[p_seg] + r_offset
            valid.append(True)
            orig_ops.append(mem_read32(patch_addr))

        for k in range(count):
            total_relocs += 1
            if not valid[k]:
                relocation_stats["invalid"] += 1
                continue
            r_offset, r_info = rels[k]
            r_type = r_info & 0xF
            p_seg = (r_info >> 8) & 0xFF
            t_seg = (r_info >> 16) & 0xFF
            if t_seg >= len(seg_addrs):
                relocation_stats["invalid"] += 1
                continue
            patch_addr = seg_addrs[p_seg] + r_offset
            relocate_to = seg_addrs[t_seg]
            op = orig_ops[k]

            if r_type == 2: # R_MIPS_32
                op = (op + relocate_to) & 0xFFFFFFFF
                relocation_stats["R_MIPS_32"] += 1
            elif r_type == 4: # R_MIPS_26
                op = (op & 0xFC000000) | (((op & 0x03FFFFFF) + (relocate_to >> 2)) & 0x03FFFFFF)
                relocation_stats["R_MIPS_26"] += 1
            elif r_type == 5: # R_MIPS_HI16
                paired = False
                identity = r_info >> 8
                for j in range(k + 1, count):
                    c_type = rels[j][1] & 0xF
                    if c_type == 5:
                        continue
                    if c_type != 6 or (rels[j][1] >> 8) != identity or not valid[j]:
                        continue
                    low = struct.unpack("<h", struct.pack("<H", orig_ops[j] & 0xFFFF))[0]
                    val = (((op & 0xFFFF) << 16) + low + relocate_to) & 0xFFFFFFFF
                    op = (op & 0xFFFF0000) | relocated_hi(val)
                    paired = True
                    break
                if not paired:
                    relocation_stats["invalid"] += 1
                    continue
                relocation_stats["R_MIPS_HI16"] += 1
            elif r_type == 6: # R_MIPS_LO16
                op = (op & 0xFFFF0000) | ((op + relocate_to) & 0xFFFF)
                relocation_stats["R_MIPS_LO16"] += 1
            else:
                relocation_stats[f"TYPE_{r_type}"] += 1

            mem_write32(patch_addr, op)

    # 5. Sony PSP Module Info (.rodata.sceModuleInfo) from RELOCATED memory
    module_info = {}
    mod_sec = next((s for s in sections if s["name"] in (".rodata.sceModuleInfo", ".sceModuleInfo")), None)
    if mod_sec:
        mod_addr = int(mod_sec["runtime_addr"], 16)
        mod_off = mod_addr - base_addr
        attrs, ver_minor, ver_major = struct.unpack_from("<HBB", mem, mod_off)
        mod_name = mem[mod_off + 4 : mod_off + 32].split(b"\x00")[0].decode("latin1", "ignore")
        gp_val = mem_read32(mod_addr + 32)
        ent_top = mem_read32(mod_addr + 36)
        ent_end = mem_read32(mod_addr + 40)
        stub_top = mem_read32(mod_addr + 44)
        stub_end = mem_read32(mod_addr + 48)
        module_info = {
            "name": mod_name,
            "attributes": attrs,
            "version": f"{ver_major}.{ver_minor}",
            "address": f"0x{mod_addr:08X}",
            "gp": f"0x{gp_val:08X}",
            "export_top": f"0x{ent_top:08X}",
            "export_end": f"0x{ent_end:08X}",
            "stub_top": f"0x{stub_top:08X}",
            "stub_end": f"0x{stub_end:08X}"
        }

    # 6. Library Imports from RELOCATED memory
    imports_by_lib = defaultdict(list)
    total_import_stubs = 0
    if module_info and "stub_top" in module_info:
        cursor = int(module_info["stub_top"], 16)
        end_cursor = int(module_info["stub_end"], 16)
        while cursor < end_cursor:
            libname_ptr = mem_read32(cursor)
            length_words = mem[cursor - base_addr + 8]
            count = struct.unpack_from("<H", mem, cursor - base_addr + 10)[0]
            nid_table = mem_read32(cursor + 12)
            stub_table = mem_read32(cursor + 16)
            table_bytes = max(20, length_words * 4)

            lib_off = libname_ptr - base_addr
            lib_name = mem[lib_off : lib_off + 128].split(b"\x00")[0].decode("latin1", "ignore")

            funcs = []
            for f_idx in range(count):
                fn_nid = mem_read32(nid_table + f_idx * 4)
                fn_stub = stub_table + f_idx * 8
                funcs.append({"nid": f"0x{fn_nid:08X}", "stub_address": f"0x{fn_stub:08X}"})
                total_import_stubs += 1

            imports_by_lib[lib_name].extend(funcs)
            cursor += table_bytes

    # 7. Disassembly Analysis of Actual .text Section (Only SHF_EXECINSTR)
    instruction_stats = Counter()
    vfpu_stats = Counter()
    j_targets = set()
    jal_targets = set()
    j_count = 0
    jal_count = 0
    indirect_jumps = []
    indirect_calls = []

    if text_section:
        t_addr = int(text_section["runtime_addr"], 16)
        t_off = t_addr - base_addr
        t_size = text_section["size"]
        tdata = mem[t_off : t_off + t_size]

        for i in range(0, len(tdata), 4):
            word = struct.unpack_from("<I", tdata, i)[0]
            pc = t_addr + i

            if word == 0:
                instruction_stats["nop"] += 1
                continue

            op = (word >> 26) & 0x3F
            rs = (word >> 21) & 0x1F
            rt = (word >> 16) & 0x1F
            rd = (word >> 11) & 0x1F
            sa = (word >> 6) & 0x1F
            fn = word & 0x3F

            if op == 0x00: # SPECIAL
                if fn == 0x08: # jr
                    instruction_stats["jr"] += 1
                    indirect_jumps.append({"pc": f"0x{pc:08X}", "reg": rs})
                elif fn == 0x09: # jalr
                    instruction_stats["jalr"] += 1
                    indirect_calls.append({"pc": f"0x{pc:08X}", "reg": rs})
                elif fn == 0x0C: # syscall
                    instruction_stats["syscall"] += 1
                else:
                    instruction_stats["special"] += 1
            elif op == 0x02: # j (unconditional jump)
                instruction_stats["j"] += 1
                j_count += 1
                target = (pc & 0xF0000000) | ((word & 0x03FFFFFF) << 2)
                j_targets.add(target)
            elif op == 0x03: # jal (jump and link - function call site)
                instruction_stats["jal"] += 1
                jal_count += 1
                target = (pc & 0xF0000000) | ((word & 0x03FFFFFF) << 2)
                jal_targets.add(target)
            elif op in (0x04, 0x05, 0x06, 0x07, 0x14, 0x15, 0x16, 0x17): # Branches
                instruction_stats["branch"] += 1
            elif op == 0x10: # COP0
                instruction_stats["cop0"] += 1
            elif op == 0x11: # COP1 (FPU)
                instruction_stats["cop1_fpu"] += 1
            elif op in (0x12, 0x18, 0x19, 0x1A, 0x1B, 0x32, 0x34, 0x35, 0x36, 0x37, 0x3A, 0x3C, 0x3D, 0x3E, 0x3F):
                instruction_stats["vfpu"] += 1
                vfpu_stats[f"op_0x{op:02X}"] += 1
            else:
                instruction_stats["standard_mips"] += 1

    combined_targets = j_targets | jal_targets

    summary = {
        "elf": {
            "type": e_type,
            "is_psp_prx": (e_type == 0xFFA0),
            "machine": e_machine,
            "entry_relative": f"0x{e_entry:08X}",
            "entry_runtime": f"0x{e_entry + base_addr:08X}",
            "segments_count": e_phnum,
            "sections_count": e_shnum,
            "executable_sections": exec_sections
        },
        "module": module_info,
        "segments": segments,
        "sections": sections,
        "relocations": {
            "total": total_relocs,
            "types": dict(relocation_stats)
        },
        "imports": {
            "total_stubs": total_import_stubs,
            "library_counts": {k: len(v) for k, v in imports_by_lib.items()},
            "details": imports_by_lib
        },
        "text_analysis": {
            "section_name": text_section["name"] if text_section else None,
            "section_addr": text_section["runtime_addr"] if text_section else None,
            "total_instructions": len(tdata) // 4 if text_section else 0,
            "note": "Opcode counts reflect opcode-family classification from raw instruction words; this is not a proof that all encodings are supported by the AOT recompiler.",
            "instruction_categories": dict(instruction_stats),
            "vfpu_opcode_distribution": dict(vfpu_stats),
            "control_flow_targets": {
                "direct_j_occurrences": j_count,
                "direct_j_unique_targets": len(j_targets),
                "direct_jal_occurrences": jal_count,
                "direct_jal_unique_targets": len(jal_targets),
                "combined_unique_targets": len(combined_targets),
                "note": "Target counts are destination addresses of branch/jump instructions and do not represent verified function boundaries."
            },
            "indirect_jumps_jr_count": len(indirect_jumps),
            "indirect_calls_jalr_count": len(indirect_calls)
        }
    }

    return summary

if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else "profiles/p3p/game/eboot.elf"
    out_path = sys.argv[2] if len(sys.argv) > 2 else "experiments/p3p-analysis/independent_analysis.json"
    res = analyze_p3p_elf(path)
    with open(out_path, "w") as out:
        json.dump(res, out, indent=2)
    print(f"Independent analysis written to {out_path}")
    print(f"ELF Type: {res['elf']['type']} (is_psp_prx: {res['elf']['is_psp_prx']})")
    print(f"Module: {res['module']['name']} v{res['module']['version']} at {res['module']['address']}")
    print(f"GP: {res['module']['gp']}")
    print(f"Entry: {res['elf']['entry_runtime']}")
    print(f"Relocations total: {res['relocations']['total']} {res['relocations']['types']}")
    print(f"Imports total: {res['imports']['total_stubs']} across {len(res['imports']['library_counts'])} libraries")
    print(f"Executable sections: {res['elf']['executable_sections']}")
    print(f"Text instructions: {res['text_analysis']['total_instructions']}")
    print(f"  FPU (COP1): {res['text_analysis']['instruction_categories'].get('cop1_fpu', 0)}")
    print(f"  VFPU: {res['text_analysis']['instruction_categories'].get('vfpu', 0)}")
    cf = res['text_analysis']['control_flow_targets']
    print(f"  Direct J: {cf['direct_j_occurrences']} sites -> {cf['direct_j_unique_targets']} unique targets")
    print(f"  Direct JAL: {cf['direct_jal_occurrences']} sites -> {cf['direct_jal_unique_targets']} unique targets")
    print(f"  Combined unique J/JAL targets: {cf['combined_unique_targets']}")
    print(f"  Indirect Jumps (jr): {res['text_analysis']['indirect_jumps_jr_count']}")
    print(f"  Indirect Calls (jalr): {res['text_analysis']['indirect_calls_jalr_count']}")
