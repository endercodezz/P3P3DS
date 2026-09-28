//! PRX function-import walker over the loaded (relocated) image.
//!
//! Single source of truth for `.lib.stub` walking (issue #52 root cause 3 +
//! R1 open question 4 — the divergent duplicate in `hle_entry_scanner.rs` was
//! consolidated onto this implementation). Layout per PPSSPP
//! `sceKernelModule.cpp:163-189` (spec R2 §6.2); each walk rule below is a
//! real-game special case.

use crate::errors::ParseError;
use crate::image::LoadedImage;
use crate::prx::ModuleInfo;
use crate::types::ImportStub;
use std::collections::HashMap;

/// One decoded `PspLibStubEntry` header (offsets per spec R2 §6.2):
/// `u32 name@0, u16 version@4, u16 flags@6, u8 size_words@8, u8 num_vars@9,
/// u16 num_funcs@10, u32 nid_data@12, u32 first_sym_addr@16`
/// (`u32 var_data@20` if size ≥ 6, `u32 extra@24` if size ≥ 7).
struct LibStubEntry {
    name_va: u32,
    size_words: u8,
    num_vars: u8,
    num_funcs: u16,
    nid_data: u32,
    first_sym_addr: u32,
}

impl LibStubEntry {
    /// Decode the fixed 20-byte header at `va`.
    fn read(image: &LoadedImage, va: u32) -> Result<Self, ParseError> {
        let raw = image.read_bytes(va, 20).map_err(|e| ParseError::Prx {
            message: format!("libstub entry at 0x{va:08X} unreadable: {e}"),
        })?;
        let u32_at = |off: usize| u32::from_le_bytes(raw[off..off + 4].try_into().unwrap());
        Ok(Self {
            name_va: u32_at(0),
            size_words: raw[8],
            num_vars: raw[9],
            num_funcs: u16::from_le_bytes(raw[10..12].try_into().unwrap()),
            nid_data: u32_at(12),
            first_sym_addr: u32_at(16),
        })
    }
}

/// Walk `[module_info.libstub, module_info.libstub_end)` over the relocated
/// image and resolve every function-import slot.
///
/// Walk rules (spec R2 §6 + gotchas 12-13):
/// - advance by `size_words * 4` BYTES (size is in 32-bit words);
/// - `size_words == 0` → warn + break (PPSSPP's infinite-loop guard);
/// - `size_words < 5` → `Err` (layout untrustworthy — likely garbage bounds);
/// - `size_words == 7` with nonzero `extra` → warn (ToW: RM2), still parse;
/// - `nid_data == 0` with `num_funcs > 0` → warn, skip funcs (vars-only);
/// - per func `i`: `nid = *(nid_data + i*4)`,
///   `stub_addr = first_sym_addr + i*8` — pure arithmetic, NEVER a
///   dereference (each slot is 8 bytes of code: `jr $ra; nop`);
/// - `num_vars > 0` → warn with count (variable imports are logged only);
/// - NID-db misses become `NID_0x…` names (the #37 report surfaces them).
///
/// Because the walk runs on the relocated image (plan D6), all pointer
/// fields are already final addresses — `stub_addr` needs no manual rebase.
pub fn parse_import_stubs(
    image: &LoadedImage,
    module_info: &ModuleInfo,
    nid_db: &HashMap<u32, String>,
) -> Result<Vec<ImportStub>, ParseError> {
    let (start, end) = (module_info.libstub, module_info.libstub_end);
    if end < start {
        return Err(ParseError::Prx {
            message: format!(
                "libstub range inverted: 0x{start:08X}..0x{end:08X} — SceModuleInfo \
                 is likely mislocated or corrupt"
            ),
        });
    }
    let mut stubs = Vec::new();
    let mut pos = start;
    while pos < end {
        let entry = LibStubEntry::read(image, pos)?;
        if entry.size_words == 0 {
            tracing::warn!(
                "libstub entry at 0x{pos:08X} has size 0 — stopping the walk \
                 (PPSSPP infinite-loop guard); imports collected so far are kept"
            );
            break;
        }
        if entry.size_words < 5 {
            return Err(ParseError::Prx {
                message: format!(
                    "libstub entry at 0x{pos:08X} has size {} words (< 5) — layout \
                     untrustworthy; SceModuleInfo libstub bounds are likely garbage",
                    entry.size_words
                ),
            });
        }
        warn_oddities(image, &entry, pos)?;
        let library = library_name(image, entry.name_va);
        collect_func_imports(image, &entry, &library, nid_db, &mut stubs)?;
        pos = pos.wrapping_add(u32::from(entry.size_words) * 4);
    }
    Ok(stubs)
}

/// Warn on real-game oddities: size-7 entries with nonzero `extra`
/// (Tales of the World: RM2) and variable imports (logged only; application
/// is out of scope for the static recompiler).
fn warn_oddities(image: &LoadedImage, entry: &LibStubEntry, pos: u32) -> Result<(), ParseError> {
    if entry.size_words >= 7 {
        let extra = image
            .read_u32(pos.wrapping_add(24))
            .map_err(|e| ParseError::Prx {
                message: format!("libstub entry at 0x{pos:08X} (size 7) truncated: {e}"),
            })?;
        if extra != 0 {
            tracing::warn!(
                "libstub entry at 0x{pos:08X}: size 7 with nonzero extra word \
                 0x{extra:08X} (ToW:RM2 case) — parsing anyway"
            );
        }
    }
    if entry.num_vars > 0 {
        tracing::warn!(
            "libstub entry at 0x{pos:08X} declares {} variable import(s) — recorded \
             in log only; variable-import application is not implemented",
            entry.num_vars
        );
    }
    Ok(())
}

/// Resolve the library name string, falling back to "unknown" for a NULL or
/// unmapped pointer (warned, never fatal — the NIDs still matter).
fn library_name(image: &LoadedImage, name_va: u32) -> String {
    if name_va == 0 {
        return "unknown".to_string();
    }
    match image.read_cstr(name_va, 64) {
        Ok(name) => name,
        Err(e) => {
            tracing::warn!("libstub library name at 0x{name_va:08X} unreadable: {e}");
            "unknown".to_string()
        }
    }
}

/// Append one `ImportStub` per function slot of `entry`.
fn collect_func_imports(
    image: &LoadedImage,
    entry: &LibStubEntry,
    library: &str,
    nid_db: &HashMap<u32, String>,
    out: &mut Vec<ImportStub>,
) -> Result<(), ParseError> {
    if entry.nid_data == 0 {
        if entry.num_funcs > 0 {
            tracing::warn!(
                "libstub entry for '{library}' has {} func(s) but a NULL nid table — \
                 skipping its function imports (vars-only entry)",
                entry.num_funcs
            );
        }
        return Ok(());
    }
    for i in 0..u32::from(entry.num_funcs) {
        let nid_va = entry.nid_data.wrapping_add(i * 4);
        let nid = image.read_u32(nid_va).map_err(|e| ParseError::Prx {
            message: format!("NID table for '{library}' unreadable at 0x{nid_va:08X}: {e}"),
        })?;
        out.push(ImportStub {
            nid,
            // Pure arithmetic: each stub slot is 8 bytes (jr $ra; nop).
            stub_addr: entry.first_sym_addr.wrapping_add(i * 8),
            name: crate::nid::resolve_nid(nid_db, nid),
            module_name: library.to_string(),
        });
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::test_fixtures::LibStubEntryFixture;

    const BASE: u32 = 0x0880_4000;

    /// One-segment image: `[names | nid tables | libstub entries]` at BASE.
    /// Returns (segment bytes, ModuleInfo with libstub bounds).
    fn build_image(entries: &[LibStubEntryFixture], pad_entries: usize) -> (Vec<u8>, ModuleInfo) {
        let mut seg = Vec::new();
        for e in entries {
            seg.extend_from_slice(&e.encode());
        }
        seg.resize(seg.len() + pad_entries, 0);
        let mi = ModuleInfo {
            attrs: 0,
            version: 0x0101,
            name: "testmod".into(),
            gp: 0,
            libent: 0,
            libent_end: 0,
            libstub: BASE,
            libstub_end: BASE + seg.len() as u32,
        };
        (seg, mi)
    }

    /// Appends `name` and a NID table to `blob`, returning their VAs.
    fn add_lib(blob: &mut Vec<u8>, name: &str, nids: &[u32]) -> (u32, u32) {
        let name_va = BASE + 0x1000 + blob.len() as u32;
        blob.extend_from_slice(name.as_bytes());
        blob.push(0);
        while !blob.len().is_multiple_of(4) {
            blob.push(0);
        }
        let nid_va = BASE + 0x1000 + blob.len() as u32;
        for nid in nids {
            blob.extend_from_slice(&nid.to_le_bytes());
        }
        (name_va, nid_va)
    }

    fn walk(
        seg: Vec<u8>,
        blob: Vec<u8>,
        mi: &ModuleInfo,
        nid_db: &HashMap<u32, String>,
    ) -> Result<Vec<ImportStub>, ParseError> {
        let datas = vec![seg, blob];
        let bases = vec![BASE, BASE + 0x1000];
        let image = LoadedImage::new(&bases, &datas);
        parse_import_stubs(&image, mi, nid_db)
    }

    #[test]
    fn walk_two_entries_sizes_5_and_6_yields_exact_stub_addrs() {
        // The dereference-bug regression: stub_addr is first_sym_addr + i*8,
        // never a read of the stub code; advance is size_words * 4 bytes.
        let mut blob = Vec::new();
        let (name_a, nids_a) = add_lib(&mut blob, "sceLibA", &[0x11, 0x22, 0x33]);
        let (name_b, nids_b) = add_lib(&mut blob, "sceLibB", &[0x44, 0x55]);
        let entries = [
            LibStubEntryFixture {
                name_va: name_a,
                size_words: 5,
                num_funcs: 3,
                nid_data: nids_a,
                first_sym_addr: 0x08BE_0000,
                ..LibStubEntryFixture::default()
            },
            LibStubEntryFixture {
                name_va: name_b,
                size_words: 6,
                num_funcs: 2,
                nid_data: nids_b,
                first_sym_addr: 0x08BE_0100,
                ..LibStubEntryFixture::default()
            },
        ];
        let (seg, mi) = build_image(&entries, 0);
        assert_eq!(seg.len(), 20 + 24, "advance must be size_words * 4");
        let stubs = walk(seg, blob, &mi, &HashMap::new()).unwrap();
        assert_eq!(stubs.len(), 5);
        let got: Vec<(u32, u32, &str)> = stubs
            .iter()
            .map(|s| (s.nid, s.stub_addr, s.module_name.as_str()))
            .collect();
        assert_eq!(
            got,
            vec![
                (0x11, 0x08BE_0000, "sceLibA"),
                (0x22, 0x08BE_0008, "sceLibA"),
                (0x33, 0x08BE_0010, "sceLibA"),
                (0x44, 0x08BE_0100, "sceLibB"),
                (0x55, 0x08BE_0108, "sceLibB"),
            ]
        );
    }

    #[test]
    fn size_zero_warns_and_breaks_keeping_prior_imports() {
        let mut blob = Vec::new();
        let (name_a, nids_a) = add_lib(&mut blob, "sceLibA", &[0x11]);
        let entries = [LibStubEntryFixture {
            name_va: name_a,
            size_words: 5,
            num_funcs: 1,
            nid_data: nids_a,
            first_sym_addr: 0x08BE_0000,
            ..LibStubEntryFixture::default()
        }];
        // 20 zero bytes after the entry: a size-0 record stops the walk.
        let (seg, mi) = build_image(&entries, 20);
        let stubs = walk(seg, blob, &mi, &HashMap::new()).unwrap();
        assert_eq!(stubs.len(), 1, "imports before the guard are kept");
    }

    #[test]
    fn size_below_five_is_an_actionable_error() {
        let entries = [LibStubEntryFixture {
            size_words: 4,
            ..LibStubEntryFixture::default()
        }];
        let (seg, mi) = build_image(&entries, 0);
        let err = walk(seg, Vec::new(), &mi, &HashMap::new()).unwrap_err();
        let msg = err.to_string();
        assert!(msg.contains("size 4 words"), "actionable: {msg}");
        assert!(msg.contains("libstub"), "names the structure: {msg}");
    }

    #[test]
    fn truncated_walk_past_image_end_is_an_error_not_a_panic() {
        let entries = [LibStubEntryFixture {
            size_words: 5,
            ..LibStubEntryFixture::default()
        }];
        let (seg, mut mi) = build_image(&entries, 0);
        mi.libstub_end = BASE + 0x800; // beyond the mapped segment
        let err = walk(seg, Vec::new(), &mi, &HashMap::new()).unwrap_err();
        let msg = err.to_string();
        assert!(msg.contains("unreadable"), "actionable: {msg}");
    }

    #[test]
    fn inverted_libstub_range_is_an_actionable_error() {
        let (seg, mut mi) = build_image(&[], 0);
        mi.libstub = BASE + 0x100;
        mi.libstub_end = BASE;
        let err = walk(seg, Vec::new(), &mi, &HashMap::new()).unwrap_err();
        assert!(err.to_string().contains("inverted"), "actionable: {err}");
    }

    #[test]
    fn null_nid_table_with_funcs_skips_entry_without_panicking() {
        let mut blob = Vec::new();
        let (name_a, nids_a) = add_lib(&mut blob, "sceLibA", &[0x11]);
        let entries = [
            LibStubEntryFixture {
                size_words: 5,
                num_funcs: 3,
                nid_data: 0, // vars-only entry shape
                first_sym_addr: 0x08BE_0000,
                ..LibStubEntryFixture::default()
            },
            LibStubEntryFixture {
                name_va: name_a,
                size_words: 5,
                num_funcs: 1,
                nid_data: nids_a,
                first_sym_addr: 0x08BE_0100,
                ..LibStubEntryFixture::default()
            },
        ];
        let (seg, mi) = build_image(&entries, 0);
        let stubs = walk(seg, blob, &mi, &HashMap::new()).unwrap();
        assert_eq!(stubs.len(), 1, "the NULL-nid entry contributes nothing");
        assert_eq!(stubs[0].stub_addr, 0x08BE_0100);
    }

    #[test]
    fn nid_db_miss_degrades_to_fallback_name_and_hit_resolves() {
        let mut blob = Vec::new();
        let (name_a, nids_a) = add_lib(&mut blob, "sceDmac", &[0x617F_3FE6, 0xDEAD_BEEF]);
        let entries = [LibStubEntryFixture {
            name_va: name_a,
            size_words: 5,
            num_funcs: 2,
            nid_data: nids_a,
            first_sym_addr: 0x08BE_0000,
            ..LibStubEntryFixture::default()
        }];
        let (seg, mi) = build_image(&entries, 0);
        let mut db = HashMap::new();
        db.insert(0x617F_3FE6u32, "sceDmacMemcpy".to_string());
        let stubs = walk(seg, blob, &mi, &db).unwrap();
        assert_eq!(stubs[0].name, "sceDmacMemcpy");
        assert_eq!(stubs[1].name, "NID_0xDEADBEEF");
    }

    #[test]
    fn size_seven_with_nonzero_extra_still_parses() {
        let mut blob = Vec::new();
        let (name_a, nids_a) = add_lib(&mut blob, "sceLibA", &[0x11]);
        let entries = [LibStubEntryFixture {
            name_va: name_a,
            size_words: 7,
            num_funcs: 1,
            nid_data: nids_a,
            first_sym_addr: 0x08BE_0000,
            extra: 0x1234_5678,
            ..LibStubEntryFixture::default()
        }];
        let (seg, mi) = build_image(&entries, 0);
        assert_eq!(seg.len(), 28);
        let stubs = walk(seg, blob, &mi, &HashMap::new()).unwrap();
        assert_eq!(stubs.len(), 1);
        assert_eq!(stubs[0].nid, 0x11);
    }

    #[test]
    fn end_to_end_through_the_prx_fixture() {
        // Full pipeline on a synthetic PRX: parse ELF -> segments -> image ->
        // locate module info -> parse imports (all at load base 0x08804000).
        use crate::test_fixtures::{ModuleInfoFixture, PrxFixture};
        use crate::{elf, prx};

        let names = b"sceTest\0";
        let nids: Vec<u8> = [0xAAu32, 0xBB]
            .iter()
            .flat_map(|n| n.to_le_bytes())
            .collect();
        let mut fixture = PrxFixture::new().text(&[0, 0, 0, 0]);
        fixture = fixture.alloc_bytes(".rodata.sceNid.names", names);
        let name_va = fixture.va_of(".rodata.sceNid.names");
        fixture = fixture.alloc_bytes(".rodata.sceNid", &nids);
        let nid_va = fixture.va_of(".rodata.sceNid");
        let stub_entry = LibStubEntryFixture {
            name_va: BASE + name_va,
            size_words: 5,
            num_funcs: 2,
            nid_data: BASE + nid_va,
            first_sym_addr: BASE, // points at .text's stub slots
            ..LibStubEntryFixture::default()
        };
        fixture = fixture.libstub(&stub_entry.encode());
        let libstub_va = fixture.va_of(".lib.stub");
        let mi = ModuleInfoFixture {
            attrs: 0,
            version: 0x0101,
            name: "testmod",
            gp: 0,
            libent: 0,
            libent_end: 0,
            libstub: BASE + libstub_va,
            libstub_end: BASE + libstub_va + 20,
        };
        let bytes = fixture.module_info(&mi).build();

        let elf_obj = elf::parse_elf(&bytes).unwrap();
        let segments = elf::extract_segments(&bytes, &elf_obj);
        let datas: Vec<Vec<u8>> = segments.iter().map(|s| s.data.clone()).collect();
        let bases: Vec<u32> = segments.iter().map(|s| BASE + s.p_vaddr).collect();
        let image = LoadedImage::new(&bases, &datas);
        let mi_va = prx::locate_module_info_va(&elf_obj, BASE).unwrap();
        let parsed = prx::parse_module_info(&image, mi_va).unwrap();
        assert_eq!(parsed.name, "testmod");
        let stubs = parse_import_stubs(&image, &parsed, &HashMap::new()).unwrap();
        assert_eq!(stubs.len(), 2);
        assert_eq!(stubs[0].module_name, "sceTest");
        assert_eq!(stubs[0].nid, 0xAA);
        assert_eq!(stubs[0].stub_addr, BASE);
        assert_eq!(stubs[1].stub_addr, BASE + 8);
    }
}
