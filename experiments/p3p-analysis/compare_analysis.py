#!/usr/bin/env python3
"""
Automated Cross-Verification Script for P3P Analysis Reports
Compares outputs from:
1. PSPRecomp analyzer (p3p_report.json and p3p_report_imports.csv)
2. Independent PRX/ELF analyzer (independent_analysis.json)

Verifies:
- ELF type, load base, runtime entry
- Relocation total count and per-type breakdown
- PSP module name, version, $gp, and stub table range
- Total import stub count and library count
- Exact library names and per-library stub counts
- Exact multiset & canonical sorted list match for all 221 import tuples (library, NID, stub_address)

Exits with code 0 on complete agreement, code 1 on mismatch.
"""

import sys
import json
import csv
from collections import Counter
from pathlib import Path

def normalize_hex(val):
    if isinstance(val, int):
        return f"0x{val:08X}"
    if isinstance(val, str):
        v = val.strip()
        if v.startswith("0x") or v.startswith("0X"):
            return f"0x{int(v, 16):08X}"
        if v.isdigit():
            return f"0x{int(v, 10):08X}"
    return str(val)

def compare_reports(recomp_json_path, independent_json_path, recomp_imports_csv_path):
    errors = []

    if not Path(recomp_json_path).exists():
        print(f"Error: {recomp_json_path} does not exist.")
        return 1
    if not Path(independent_json_path).exists():
        print(f"Error: {independent_json_path} does not exist.")
        return 1

    with open(recomp_json_path, "r", encoding="utf-8") as f:
        recomp_data = json.load(f)
    with open(independent_json_path, "r", encoding="utf-8") as f:
        indep_data = json.load(f)

    print("====================================================")
    print("   P3P Executable Analysis Cross-Verification")
    print("====================================================")

    # 1. ELF Header & Entry
    r_entry = normalize_hex(recomp_data.get("entry_runtime"))
    i_entry = normalize_hex(indep_data.get("elf", {}).get("entry_runtime"))
    if r_entry != i_entry:
        errors.append(f"Entry runtime mismatch: PSPRecomp={r_entry}, Independent={i_entry}")
    else:
        print(f"Entry Runtime PC:       {r_entry} [MATCH]")

    r_type = recomp_data.get("elf_type")
    i_type = indep_data.get("elf", {}).get("type")
    if r_type != i_type:
        errors.append(f"ELF type mismatch: PSPRecomp={r_type}, Independent={i_type}")
    else:
        print(f"ELF Type:               {r_type} [MATCH]")

    # 2. Relocations
    r_rel = recomp_data.get("relocations", {})
    i_rel = indep_data.get("relocations", {})
    r_total = r_rel.get("total")
    i_total = i_rel.get("total")
    if r_total != i_total:
        errors.append(f"Relocation total mismatch: PSPRecomp={r_total}, Independent={i_total}")
    else:
        print(f"Relocations Total:      {r_total} [MATCH]")

    # Per-type breakdown
    rel_type_keys = [
        ("r_mips_26", "R_MIPS_26"),
        ("r_mips_32", "R_MIPS_32"),
        ("r_mips_hi16", "R_MIPS_HI16"),
        ("r_mips_lo16", "R_MIPS_LO16")
    ]
    for r_k, i_k in rel_type_keys:
        r_v = r_rel.get(r_k, 0)
        i_v = i_rel.get("types", {}).get(i_k, 0)
        if r_v != i_v:
            errors.append(f"Relocation {i_k} mismatch: PSPRecomp={r_v}, Independent={i_v}")
        else:
            print(f"  {i_k:<12}:         {r_v} [MATCH]")

    # 3. Module Info
    r_mod = recomp_data.get("module", {})
    i_mod = indep_data.get("module", {})

    r_name = r_mod.get("name")
    i_name = i_mod.get("name")
    if r_name != i_name:
        errors.append(f"Module name mismatch: PSPRecomp={r_name}, Independent={i_name}")
    else:
        print(f"Module Name:            {r_name} [MATCH]")

    r_ver = r_mod.get("version")
    i_ver = i_mod.get("version")
    if r_ver != i_ver:
        errors.append(f"Module version mismatch: PSPRecomp={r_ver}, Independent={i_ver}")
    else:
        print(f"Module Version:         {r_ver} [MATCH]")

    r_gp = normalize_hex(r_mod.get("gp"))
    i_gp = normalize_hex(i_mod.get("gp"))
    if r_gp != i_gp:
        errors.append(f"Module GP mismatch: PSPRecomp={r_gp}, Independent={i_gp}")
    else:
        print(f"Module GP:              {r_gp} [MATCH]")

    r_stub_top = normalize_hex(r_mod.get("stub_top"))
    i_stub_top = normalize_hex(i_mod.get("stub_top"))
    r_stub_end = normalize_hex(r_mod.get("stub_end"))
    i_stub_end = normalize_hex(i_mod.get("stub_end"))
    if r_stub_top != i_stub_top or r_stub_end != i_stub_end:
        errors.append(f"Stub range mismatch: PSPRecomp={r_stub_top}-{r_stub_end}, Independent={i_stub_top}-{i_stub_end}")
    else:
        print(f"Module Stub Range:      {r_stub_top} - {r_stub_end} [MATCH]")

    # 4. Import Stubs & Libraries
    r_imp_count = recomp_data.get("import_count")
    i_imp_count = indep_data.get("imports", {}).get("total_stubs")
    if r_imp_count != i_imp_count:
        errors.append(f"Total import stubs mismatch: PSPRecomp={r_imp_count}, Independent={i_imp_count}")
    else:
        print(f"Total Import Stubs:     {r_imp_count} [MATCH]")

    r_libs = recomp_data.get("import_libraries", {})
    i_libs = indep_data.get("imports", {}).get("library_counts", {})

    if len(r_libs) != len(i_libs):
        errors.append(f"Library count mismatch: PSPRecomp={len(r_libs)}, Independent={len(i_libs)}")
    else:
        print(f"Import Libraries Count: {len(r_libs)} [MATCH]")

    all_libs = sorted(set(r_libs.keys()) | set(i_libs.keys()))
    for lib in all_libs:
        rc = r_libs.get(lib, 0)
        ic = i_libs.get(lib, 0)
        if rc != ic:
            errors.append(f"Library '{lib}' stub count mismatch: PSPRecomp={rc}, Independent={ic}")

    if not any("Library" in e for e in errors):
        print(f"All {len(all_libs)} library names & counts match exactly [MATCH]")

    # 5. Exact Multiset & Canonical List (Library, NID, stub_address) Comparison
    recomp_stubs = []
    if Path(recomp_imports_csv_path).exists():
        with open(recomp_imports_csv_path, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                recomp_stubs.append((
                    row["library"].strip(),
                    normalize_hex(row["nid"]),
                    normalize_hex(row["stub_address"])
                ))
    else:
        errors.append(f"Missing {recomp_imports_csv_path} for detailed stub verification")

    indep_stubs = []
    details = indep_data.get("imports", {}).get("details", {})
    for lib, funcs in sorted(details.items()):
        for fn in funcs:
            indep_stubs.append((
                lib.strip(),
                normalize_hex(fn["nid"]),
                normalize_hex(fn["stub_address"])
            ))

    recomp_counter = Counter(recomp_stubs)
    indep_counter = Counter(indep_stubs)

    diff_counter_r = recomp_counter - indep_counter
    diff_counter_i = indep_counter - recomp_counter

    if diff_counter_r:
        errors.append(f"Excess tuples in PSPRecomp analysis ({len(diff_counter_r)}): {list(diff_counter_r.items())[:5]}")
    if diff_counter_i:
        errors.append(f"Excess tuples in Independent analysis ({len(diff_counter_i)}): {list(diff_counter_i.items())[:5]}")

    recomp_sorted = sorted(recomp_stubs)
    indep_sorted = sorted(indep_stubs)

    if recomp_sorted != indep_sorted:
        errors.append(f"Canonical sorted order mismatch between import stub lists ({len(recomp_sorted)} vs {len(indep_sorted)})")

    if not errors and len(recomp_stubs) == 221:
        print(f"Exact multiset match of all {len(recomp_stubs)} import tuples across both analyzers [MATCH]")

    print("====================================================")
    if errors:
        print(f"[FAILED] Found {len(errors)} discrepancy(ies):")
        for err in errors:
            print(f"  - {err}")
        return 1
    else:
        print("[VERIFIED] All analysis metrics match deterministically across both tools.")
        return 0

if __name__ == "__main__":
    recomp_json = sys.argv[1] if len(sys.argv) > 1 else "experiments/p3p-analysis/p3p_report.json"
    indep_json = sys.argv[2] if len(sys.argv) > 2 else "experiments/p3p-analysis/independent_analysis.json"
    recomp_csv = sys.argv[3] if len(sys.argv) > 3 else "experiments/p3p-analysis/p3p_report_imports.csv"
    sys.exit(compare_reports(recomp_json, indep_json, recomp_csv))
