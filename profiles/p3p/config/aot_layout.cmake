# ULUS-10512 whole-.text AOT layout for `psp_recomp --auto`.
#
# [VERIFIED] `psp_recomp profiles/p3p/game/eboot.elf --auto <dir> 0x08804000 16384`
# on ELF SHA-256 be2abbd43a4ae7ce5aa2f1f146083ffe0d924fc2eb1874e6fcdc21cba40d49db
# reports 21,965 function seeds, 751,674 emitted instruction PCs, 171,024
# registered block entries and 237 translation units (buckets 0..236), i.e.
# the same function list as experiments/p3p-analysis/p3p_report_functions_auto.csv.
# tools/verify_aot_layout.py fails the build if the generator disagrees.
#
# 16 KiB matches the runtime's shift-only unit index fast path
# (recomp/PSPRecomp/src/runtime.cpp, generated_unit_span_ == 16384u) and keeps
# every unit well under Runtime::kGeneratedUnitFastCapacity (512).
set(P3P_AOT_LOAD_BASE "0x08804000")
set(P3P_AOT_UNIT_SPAN "16384")
set(P3P_AOT_UNIT_COUNT 237)
