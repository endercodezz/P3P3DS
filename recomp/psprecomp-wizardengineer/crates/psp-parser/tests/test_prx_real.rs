//! Real-binary smoke tests for PRX discovery/relocation/import parsing.
//!
//! The binaries live in gitignored paths (issue #52 test plan §5.1); each
//! test skips silently when its input is absent so CI without game data
//! stays green. Ground-truth numbers come from
//! `.planning/research/52-prx-format-spec.md` (verified against PPSSPP).

use psp_parser::image::LoadedImage;
use psp_parser::{elf, imports, prx, reloc};
use std::collections::{HashMap, HashSet};
use std::path::PathBuf;

fn workspace_file(rel: &str) -> Option<Vec<u8>> {
    let path = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("../..")
        .join(rel);
    match std::fs::read(&path) {
        Ok(data) => Some(data),
        Err(_) => {
            eprintln!("skipping: {rel} not present (gitignored game data)");
            None
        }
    }
}

/// Loads segments rebased to `load_base` and returns (bases, datas).
fn load_segments(
    data: &[u8],
    elf_obj: &goblin::elf::Elf,
    load_base: u32,
) -> (Vec<u32>, Vec<Vec<u8>>) {
    let segments = elf::extract_segments(data, elf_obj);
    segments
        .into_iter()
        .map(|s| (load_base + s.p_vaddr, s.data))
        .unzip()
}

#[test]
fn dothack_ground_truth() {
    let Some(data) = workspace_file("data/dothack/BOOT_DEC.BIN") else {
        return;
    };
    let elf_obj = elf::parse_elf(&data).unwrap();
    assert!(prx::is_prx(&elf_obj), "e_type 0xFFA0 expected");

    // Discovery: 7 section tables (one empty), 188,284 Type-A entries, 0 Type-B.
    let tables = prx::find_reloc_tables(&elf_obj);
    assert_eq!(tables.len(), 7);
    assert!(tables.iter().all(|t| t.format == prx::RelocFormat::TypeA));
    assert_eq!(tables.iter().filter(|t| t.size > 0).count(), 6);

    // Application at the PSP user-module base: every entry handled.
    let base = prx::PSP_USER_MODULE_BASE;
    let (bases, mut datas) = load_segments(&data, &elf_obj, base);
    let mut total_entries = 0usize;
    let mut stats = reloc::RelocStats::default();
    for t in &tables {
        let entries =
            reloc::parse_type_a_entries(&data[t.file_offset..t.file_offset + t.size]).unwrap();
        total_entries += entries.len();
        stats.absorb(reloc::apply_relocations(&mut datas, &bases, &entries).unwrap());
    }
    assert_eq!(total_entries, 188_284);
    assert_eq!(stats.handled, 188_284, "every entry applies");
    assert_eq!(stats.skipped_bad, 0);
    assert!(
        stats.unhandled.is_empty(),
        "unhandled: {:?}",
        stats.unhandled
    );

    // Module info parsed from the relocated image: pointers are final.
    let image = LoadedImage::new(&bases, &datas);
    let mi_va = prx::locate_module_info_va(&elf_obj, base).unwrap();
    let mi = prx::parse_module_info(&image, mi_va).unwrap();
    assert_eq!(mi.name, "hacklink");
    // 0x485BD0 + 0x08804000. (R2 §2 lists 0x08C85BD0 — a transcription
    // error that added 0x08800000 instead of the 0x08804000 load base;
    // verified against the .rel.rodata.sceModuleInfo R_MIPS_32 entry that
    // targets the gp field at 0x3DCFFC.)
    assert_eq!(mi.gp, 0x08C8_9BD0, "post-reloc gp");
    assert_eq!(mi.libstub, 0x08BE_0DA8);
    assert_eq!(mi.libstub_end, 0x08BE_0FD8);

    // Imports: 28 libstub entries -> 243 function imports, 0 var imports.
    let stubs = imports::parse_import_stubs(&image, &mi, &HashMap::new()).unwrap();
    assert_eq!(stubs.len(), 243);
    let libs: HashSet<&str> = stubs.iter().map(|s| s.module_name.as_str()).collect();
    assert_eq!(libs.len(), 28);
    assert_eq!(stubs[0].module_name, "sceDmac");
    assert_eq!(stubs[0].nid, 0x617F_3FE6, "sceDmacMemcpy");
    // All stub slots live in .sceStub.text: [0x08BE05F4, +0x798).
    let stub_range = 0x08BE_05F4..0x08BE_05F4 + 0x798;
    assert!(stubs.iter().all(|s| stub_range.contains(&s.stub_addr)));
}

#[test]
fn patapon_elf_walks_to_the_same_stub_map_as_before_consolidation() {
    // D7-(vi) guard: the consolidated walker must reproduce the ET_EXEC
    // (Patapon) stub set the deleted hle_entry_scanner walker produced —
    // 237 function imports starting at 0x089D7520 (the runtime's
    // PSP_NID_STUB_COUNT and first hardcoded stub address).
    let Some(data) = workspace_file("disc0/PSP_GAME/SYSDIR/BOOT.BIN") else {
        return;
    };
    let elf_obj = elf::parse_elf(&data).unwrap();
    assert!(!prx::is_prx(&elf_obj), "Patapon is ET_EXEC");

    // ET_EXEC: segments load where linked, no relocation, load_base 0.
    let (bases, datas) = load_segments(&data, &elf_obj, 0);
    let image = LoadedImage::new(&bases, &datas);
    let mi_va = prx::locate_module_info_va(&elf_obj, 0).unwrap();
    let mi = prx::parse_module_info(&image, mi_va).unwrap();
    assert_eq!(mi.libstub, 0x089D_7CA4);
    assert_eq!(mi.libstub_end, 0x089D_7EE8);

    let stubs = imports::parse_import_stubs(&image, &mi, &HashMap::new()).unwrap();
    assert_eq!(stubs.len(), 237);
    assert_eq!(stubs[0].stub_addr, 0x089D_7520);
    let libs: HashSet<&str> = stubs.iter().map(|s| s.module_name.as_str()).collect();
    assert_eq!(libs.len(), 29, "29 libstub entries");
}
