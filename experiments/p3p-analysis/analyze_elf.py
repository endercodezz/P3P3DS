#!/usr/bin/env python3
"""
Independent P3P Executable Analysis Tool
Parses the decrypted Sony PSP PRX/ELF file to extract:
- ELF headers, segments, sections
- Sony PSP Module Info, Export tables, Import tables (NIDs)
- Relocations (R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16)
- Instruction breakdown across the .text section (MIPS, FPU, VFPU, control flow)
- Indirect calls and jump tables
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
    for i in range(e_phnum):
        p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = struct.unpack_from(
            "<8I", data, e_phoff + i * e_phentsize
        )
        segments.append({
            "index": i,
            "type": p_type,
            "type_hex": f"0x{p_type:08X}",
            "offset": p_offset,
            "vaddr_relative": f"0x{p_vaddr:08X}",
            "vaddr_runtime": f"0x{p_vaddr + base_addr:08X}",
            "filesz": p_filesz,
            "memsz": p_memsz,
            "flags": p_flags,
            "align": p_align
        })

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
            "addr": sh_addr,
            "runtime_addr": sh_addr + base_addr,
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
        if sec_name == ".text":
            text_section = sec_info
        elif ".rodata" in sec_name:
            rodata_sections.append(sec_info)
        elif ".data" in sec_name:
            data_sections.append(sec_info)

    # 4. PSP Relocation Analysis
    relocation_stats = Counter()
    total_relocs = 0
    for rel_sec in rel_sections:
        rel_data = data[rel_sec["offset"] : rel_sec["offset"] + rel_sec["size"]]
        pos = 0
        while pos + 8 <= len(rel_data):
            r_offset, r_info = struct.unpack_from("<II", rel_data, pos)
            pos += 8
            r_type = r_info & 0xFF
            total_relocs += 1
            if r_type == 1:
                relocation_stats["R_MIPS_16"] += 1
            elif r_type == 2:
                relocation_stats["R_MIPS_32"] += 1
            elif r_type == 4:
                relocation_stats["R_MIPS_26"] += 1
            elif r_type == 5:
                relocation_stats["R_MIPS_HI16"] += 1
            elif r_type == 6:
                relocation_stats["R_MIPS_LO16"] += 1
            else:
                relocation_stats[f"TYPE_{r_type}"] += 1

    # 5. Sony PSP Module Info (.rodata.sceModuleInfo)
    module_info = {}
    mod_sec = next((s for s in sections if s["name"] == ".rodata.sceModuleInfo"), None)
    if mod_sec:
        mdata = data[mod_sec["offset"] : mod_sec["offset"] + mod_sec["size"]]
        attrs, ver_minor, ver_major = struct.unpack_from("<HBB", mdata, 0)
        mod_name = mdata[4:32].split(b"\x00")[0].decode("latin1", "ignore")
        gp_val, ent_top, ent_end, stub_top, stub_end = struct.unpack_from("<5I", mdata, 32)
        module_info = {
            "name": mod_name,
            "attributes": attrs,
            "version": f"{ver_major}.{ver_minor}",
            "gp": f"0x{gp_val + base_addr:08X}",
            "export_top": f"0x{ent_top + base_addr:08X}",
            "export_end": f"0x{ent_end + base_addr:08X}",
            "stub_top": f"0x{stub_top + base_addr:08X}",
            "stub_end": f"0x{stub_end + base_addr:08X}"
        }

    # 6. Library Imports (.lib.stub & .rodata.sceNid)
    stub_sec = next((s for s in sections if s["name"] == ".lib.stub"), None)
    imports_by_lib = defaultdict(list)
    total_import_stubs = 0
    if stub_sec:
        sdata = data[stub_sec["offset"] : stub_sec["offset"] + stub_sec["size"]]
        pos = 0
        while pos + 20 <= len(sdata):
            modname_ptr, version, flags, length_words, num_funcs, num_vars, nid_table_ptr, stub_table_ptr = struct.unpack_from(
                "<IH2B2H2I", sdata, pos
            )
            table_bytes = max(20, length_words * 4)
            if table_bytes == 0:
                break

            # Resolve module name from file
            name_file_off = None
            for s in sections:
                if s["addr"] <= modname_ptr < s["addr"] + s["size"]:
                    name_file_off = s["offset"] + (modname_ptr - s["addr"])
                    break
            lib_name = "unknown"
            if name_file_off:
                lib_name = data[name_file_off : name_file_off + 64].split(b"\x00")[0].decode("latin1", "ignore")

            # Resolve NIDs
            nid_file_off = None
            for s in sections:
                if s["addr"] <= nid_table_ptr < s["addr"] + s["size"]:
                    nid_file_off = s["offset"] + (nid_table_ptr - s["addr"])
                    break

            stub_file_off = None
            for s in sections:
                if s["addr"] <= stub_table_ptr < s["addr"] + s["size"]:
                    stub_file_off = s["offset"] + (stub_table_ptr - s["addr"])
                    break

            funcs = []
            if nid_file_off and stub_file_off:
                for f_idx in range(num_funcs):
                    fn_nid = struct.unpack_from("<I", data, nid_file_off + f_idx * 4)[0]
                    fn_stub = stub_table_ptr + f_idx * 8 + base_addr
                    funcs.append({"nid": f"0x{fn_nid:08X}", "stub_address": f"0x{fn_stub:08X}"})
                    total_import_stubs += 1

            if lib_name != "unknown" or funcs:
                imports_by_lib[lib_name].extend(funcs)

            pos += table_bytes

    # 7. Disassembly Analysis of Actual .text Section
    instruction_stats = Counter()
    control_flow = Counter()
    vfpu_stats = Counter()
    jump_targets = set()
    indirect_jumps = []
    indirect_calls = []

    if text_section:
        tdata = data[text_section["offset"] : text_section["offset"] + text_section["size"]]
        text_runtime_base = text_section["runtime_addr"]

        for i in range(0, len(tdata), 4):
            word = struct.unpack_from("<I", tdata, i)[0]
            pc = text_runtime_base + i

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
            elif op == 0x02: # j
                instruction_stats["j"] += 1
                target = (pc & 0xF0000000) | ((word & 0x03FFFFFF) << 2)
                jump_targets.add(target)
            elif op == 0x03: # jal
                instruction_stats["jal"] += 1
                target = (pc & 0xF0000000) | ((word & 0x03FFFFFF) << 2)
                jump_targets.add(target)
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

    summary = {
        "elf": {
            "type": e_type,
            "is_psp_prx": (e_type == 0xFFA0),
            "machine": e_machine,
            "entry_relative": f"0x{e_entry:08X}",
            "entry_runtime": f"0x{e_entry + base_addr:08X}",
            "segments_count": e_phnum,
            "sections_count": e_shnum
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
            "total_instructions": len(tdata) // 4 if text_section else 0,
            "instruction_categories": dict(instruction_stats),
            "vfpu_opcode_distribution": dict(vfpu_stats),
            "direct_jump_targets_count": len(jump_targets),
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
    print(f"Module: {res['module']['name']} v{res['module']['version']}")
    print(f"Entry: {res['elf']['entry_runtime']}")
    print(f"Relocations total: {res['relocations']['total']}")
    print(f"Imports total: {res['imports']['total_stubs']} across {len(res['imports']['library_counts'])} libraries")
    print(f"Text instructions: {res['text_analysis']['total_instructions']}")
    print(f"  FPU (COP1): {res['text_analysis']['instruction_categories'].get('cop1_fpu', 0)}")
    print(f"  VFPU: {res['text_analysis']['instruction_categories'].get('vfpu', 0)}")
    print(f"  Direct J/JAL targets: {res['text_analysis']['direct_jump_targets_count']}")
    print(f"  Indirect Jumps (jr): {res['text_analysis']['indirect_jumps_jr_count']}")
    print(f"  Indirect Calls (jalr): {res['text_analysis']['indirect_calls_jalr_count']}")
