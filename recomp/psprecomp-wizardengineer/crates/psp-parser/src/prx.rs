//! PRX module structure: relocation-table discovery and SceModuleInfo lookup.
//!
//! Reference: PPSSPP `Core/ELF/ElfReader.cpp` (`LoadInto`) and
//! `Core/HLE/sceKernelModule.cpp`, per `.planning/research/52-prx-format-spec.md`
//! §1-§3. Discovery is section-first (sections of `sh_type` 0x700000A0 always;
//! reloc program headers only when the binary has no section headers), and
//! SceModuleInfo lives in `.rodata.sceModuleInfo` — never `.lib.ent`, which is
//! the export table (issue #52 root cause 2).

use crate::errors::ParseError;
use crate::image::LoadedImage;

pub const PT_PSPREL1: u32 = 0x700000A0;
pub const PT_PSPREL2: u32 = 0x700000A1;
pub const ET_PSP_PRX: u16 = 0xFFA0;

/// Default load base for relocatable PSP user modules.
///
/// PSP user memory starts at 0x08800000 and the kernel pre-reserves its first
/// 0x4000 bytes ("usersystemlib"), so the first user-memory allocation — the
/// main game module — lands at 0x08804000 on both real hardware and PPSSPP
/// (spec R2 §2: `sceKernelMemory.cpp` AllocAt + `ElfReader.cpp` Alloc).
pub const PSP_USER_MODULE_BASE: u32 = 0x0880_4000;

const SHT_REL: u32 = 9;
const SHF_ALLOC: u64 = 0x2;

/// Returns true if the ELF e_type indicates a PSP relocatable module (PRX).
pub fn is_prx(elf: &goblin::elf::Elf) -> bool {
    elf.header.e_type == ET_PSP_PRX
}

/// On-disk encoding of a discovered PSP relocation table.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RelocFormat {
    /// 8-byte `[r_offset, r_info]` entries (`0x700000A0`).
    TypeA,
    /// Packed command stream (`0x700000A1`) — discovery only; application is
    /// deliberately unimplemented (plan D5).
    TypeB,
}

/// One discovered relocation table, located by file offset.
#[derive(Debug, Clone)]
pub struct RelocTable {
    pub format: RelocFormat,
    pub file_offset: usize,
    pub size: usize,
    /// Section/segment name for diagnostics (e.g. ".rel.text", "phdr[2]").
    pub source: String,
}

/// Discover PSP relocation tables, mirroring PPSSPP `ElfReader::LoadInto`.
///
/// Sections with `sh_type == 0x700000A0` are always scanned; `PT_PSPREL1/2`
/// program headers are consulted ONLY when the binary has no section headers.
/// Mirroring PPSSPP's order exactly makes double-application impossible.
/// A table is skipped (with a warning) when its `sh_info` target section
/// exists but lacks `SHF_ALLOC` — PPSSPP's sanity check; `sh_info` is never
/// used for target addressing (spec gotcha 1). The caller must bounds-check
/// `file_offset + size` against the file.
pub fn find_reloc_tables(elf: &goblin::elf::Elf) -> Vec<RelocTable> {
    if elf.section_headers.is_empty() {
        return phdr_reloc_tables(elf);
    }
    section_reloc_tables(elf)
}

/// Sections pass: `sh_type` 0x700000A0 ⇒ Type-A; SHT_REL(9) warned + skipped.
fn section_reloc_tables(elf: &goblin::elf::Elf) -> Vec<RelocTable> {
    let mut tables = Vec::new();
    for (i, sh) in elf.section_headers.iter().enumerate() {
        let source = match elf.shdr_strtab.get_at(sh.sh_name) {
            Some(name) if !name.is_empty() => name.to_string(),
            _ => format!("section[{i}]"),
        };
        match sh.sh_type {
            PT_PSPREL1 => {
                let target = elf.section_headers.get(sh.sh_info as usize);
                if let Some(t) = target {
                    if t.sh_flags & SHF_ALLOC == 0 {
                        tracing::warn!(
                            "Skipping reloc table {source}: its sh_info target section {} \
                             is not SHF_ALLOC (PPSSPP sanity check)",
                            sh.sh_info
                        );
                        continue;
                    }
                }
                tables.push(RelocTable {
                    format: RelocFormat::TypeA,
                    file_offset: sh.sh_offset as usize,
                    size: sh.sh_size as usize,
                    source,
                });
            }
            SHT_REL => tracing::warn!(
                "Section {source} holds traditional SHT_REL relocations — unsupported for \
                 relocatable PSP modules, skipping (PPSSPP does the same)"
            ),
            PT_PSPREL2 => tracing::warn!(
                "Section {source} is typed 0x700000A1 (Type-B packed relocations) — PPSSPP \
                 only honors Type-B as a program-header format on sectionless PRXs and \
                 ignores such sections; skipping it here too. If this module later fails to \
                 relocate, this section is the first suspect"
            ),
            _ => {}
        }
    }
    tables
}

/// Phdr fallback (sectionless/stripped PRX): `PT_PSPREL1` ⇒ A, `PT_PSPREL2` ⇒ B.
fn phdr_reloc_tables(elf: &goblin::elf::Elf) -> Vec<RelocTable> {
    elf.program_headers
        .iter()
        .enumerate()
        .filter_map(|(i, ph)| {
            let format = match ph.p_type {
                PT_PSPREL1 => RelocFormat::TypeA,
                PT_PSPREL2 => RelocFormat::TypeB,
                _ => return None,
            };
            Some(RelocTable {
                format,
                file_offset: ph.p_offset as usize,
                size: ph.p_filesz as usize,
                source: format!("phdr[{i}]"),
            })
        })
        .collect()
}

/// Decoded 0x34-byte `PspModuleInfo` record (spec R2 §1.3).
///
/// When parsed from the relocated image (plan D6), the five pointer fields
/// (`gp`, `libent*`, `libstub*`) are already final virtual addresses.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ModuleInfo {
    pub attrs: u16,
    pub version: u16,
    /// `char[28]`, NUL-trimmed.
    pub name: String,
    pub gp: u32,
    pub libent: u32,
    pub libent_end: u32,
    pub libstub: u32,
    pub libstub_end: u32,
}

/// Virtual address of SceModuleInfo.
///
/// Priority (PPSSPP `ElfReader.cpp:443-454`):
/// 1. section `.rodata.sceModuleInfo` → `load_base + sh_addr`;
/// 2. fallback (stripped PRX, or an ELF without that section): the first
///    PT_LOAD's `p_paddr & 0x7FFFFFFF` is a FILE OFFSET (kernel bit masked,
///    spec gotcha 2), converted via
///    `va = load_base + p_vaddr + (file_off - p_offset)`.
///
/// NEVER `.lib.ent` — that is the 16-byte export table; decoding module info
/// there yields garbage libstub bounds (issue #52 root cause 2).
pub fn locate_module_info_va(elf: &goblin::elf::Elf, load_base: u32) -> Result<u32, ParseError> {
    for sh in &elf.section_headers {
        if elf.shdr_strtab.get_at(sh.sh_name) == Some(".rodata.sceModuleInfo") {
            return Ok(load_base.wrapping_add(sh.sh_addr as u32));
        }
    }
    let ph = elf
        .program_headers
        .iter()
        .find(|ph| ph.p_type == goblin::elf::program_header::PT_LOAD)
        .ok_or_else(|| ParseError::Prx {
            message: "SceModuleInfo not found: no .rodata.sceModuleInfo section and no \
                      PT_LOAD program header"
                .into(),
        })?;
    let file_off = (ph.p_paddr as u32) & 0x7FFF_FFFF;
    let seg_rel = file_off
        .checked_sub(ph.p_offset as u32)
        .filter(|&rel| u64::from(rel) < ph.p_filesz)
        .ok_or_else(|| ParseError::Prx {
            message: format!(
                "SceModuleInfo not found: no .rodata.sceModuleInfo section, and PT_LOAD \
                 p_paddr 0x{:08X} (masked file offset 0x{file_off:08X}) does not point \
                 inside the segment's file image (offset 0x{:X}, filesz 0x{:X})",
                ph.p_paddr, ph.p_offset, ph.p_filesz
            ),
        })?;
    Ok(load_base
        .wrapping_add(ph.p_vaddr as u32)
        .wrapping_add(seg_rel))
}

/// Parse the 0x34-byte `PspModuleInfo` at `va` from the loaded image.
///
/// Layout (spec R2 §1.3): attrs u16 @0x00, version u16 @0x02, name char[28]
/// @0x04, gp u32 @0x20, libent @0x24, libentend @0x28, libstub @0x2C,
/// libstubend @0x30. Per plan D6 the image must already be relocated, so the
/// pointer fields read here are final addresses.
pub fn parse_module_info(image: &LoadedImage, va: u32) -> Result<ModuleInfo, ParseError> {
    let raw = image.read_bytes(va, 0x34).map_err(|e| ParseError::Prx {
        message: format!("SceModuleInfo at 0x{va:08X} unreadable: {e}"),
    })?;
    let u16_at = |off: usize| u16::from_le_bytes(raw[off..off + 2].try_into().unwrap());
    let u32_at = |off: usize| u32::from_le_bytes(raw[off..off + 4].try_into().unwrap());
    let name_end = raw[4..32].iter().position(|&b| b == 0).unwrap_or(28);
    Ok(ModuleInfo {
        attrs: u16_at(0),
        version: u16_at(2),
        name: String::from_utf8_lossy(&raw[4..4 + name_end]).into_owned(),
        gp: u32_at(0x20),
        libent: u32_at(0x24),
        libent_end: u32_at(0x28),
        libstub: u32_at(0x2C),
        libstub_end: u32_at(0x30),
    })
}

/// Maps a virtual address to a file offset using program headers.
pub fn vaddr_to_file_offset(elf: &goblin::elf::Elf, vaddr: u32) -> Option<usize> {
    use goblin::elf::program_header::PT_LOAD;
    for ph in &elf.program_headers {
        if ph.p_type != PT_LOAD {
            continue;
        }
        let start = ph.p_vaddr as u32;
        let end = start.checked_add(ph.p_filesz as u32)?;
        if vaddr >= start && vaddr < end {
            let offset = (vaddr - start) as usize + ph.p_offset as usize;
            return Some(offset);
        }
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::elf;
    use crate::test_fixtures::{ModuleInfoFixture, PrxFixture};

    fn mi_fixture() -> ModuleInfoFixture {
        ModuleInfoFixture {
            attrs: 0x0002,
            version: 0x0101,
            name: "testmod",
            gp: 0x0008_5BD0,
            libent: 0x40,
            libent_end: 0x50,
            libstub: 0x50,
            libstub_end: 0x64,
        }
    }

    #[test]
    fn sections_present_finds_type_a_tables_and_ignores_reloc_phdrs() {
        // Even with stray reloc program headers, sections win (PPSSPP order).
        let bytes = PrxFixture::new()
            .text(&[0x0000_0010])
            .reloc(".rel.text", &[(0, 2)])
            .reloc_phdrs_alongside_sections()
            .build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        assert!(
            elf_obj
                .program_headers
                .iter()
                .any(|ph| ph.p_type == PT_PSPREL1),
            "fixture must actually contain a reloc phdr"
        );
        let tables = find_reloc_tables(&elf_obj);
        assert_eq!(tables.len(), 1);
        assert_eq!(tables[0].format, RelocFormat::TypeA);
        assert_eq!(tables[0].source, ".rel.text");
        assert_eq!(tables[0].size, 8);
        assert!(
            !tables[0].source.starts_with("phdr"),
            "phdr tables must be ignored when section headers exist"
        );
    }

    #[test]
    fn sectionless_falls_back_to_phdrs_with_correct_formats() {
        let bytes = PrxFixture::new()
            .text(&[0x0000_0010])
            .reloc(".rel.text", &[(0, 2)])
            .reloc_typed(".rel.b", vec![0u8; 16], PT_PSPREL2)
            .without_section_headers()
            .build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        assert!(elf_obj.section_headers.is_empty());
        let tables = find_reloc_tables(&elf_obj);
        assert_eq!(tables.len(), 2);
        assert_eq!(tables[0].format, RelocFormat::TypeA);
        assert_eq!(tables[0].source, "phdr[1]");
        assert_eq!(tables[1].format, RelocFormat::TypeB);
        assert_eq!(tables[1].source, "phdr[2]");
        assert_eq!(tables[1].size, 16);
    }

    #[test]
    fn traditional_sht_rel_section_is_skipped() {
        let bytes = PrxFixture::new()
            .text(&[0x0000_0010])
            .reloc_typed(".rel.text", vec![0u8; 8], SHT_REL)
            .build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        assert!(find_reloc_tables(&elf_obj).is_empty());
    }

    #[test]
    fn type_b_typed_section_is_skipped_like_ppsspp() {
        // 0x700000A1 is only meaningful as a program-header type (sectionless PRX);
        // a SECTION carrying it is skipped with a loud warning, matching PPSSPP.
        let bytes = PrxFixture::new()
            .text(&[0x0000_0010])
            .reloc_typed(".rel.typeb", vec![0u8; 8], PT_PSPREL2)
            .build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        assert!(find_reloc_tables(&elf_obj).is_empty());
    }

    #[test]
    fn non_alloc_sh_info_target_skips_table_but_oob_sh_info_keeps_it() {
        // sh_info = 1 targets .shstrtab (non-ALLOC) => skipped (PPSSPP check);
        // sh_info way out of bounds => no check possible => table kept.
        let bytes = PrxFixture::new()
            .text(&[0x0000_0010])
            .reloc_with_info(".rel.bad", &[(0, 2)], 1)
            .reloc_with_info(".rel.oob", &[(4, 2)], 200)
            .build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        let tables = find_reloc_tables(&elf_obj);
        assert_eq!(tables.len(), 1);
        assert_eq!(tables[0].source, ".rel.oob");
    }

    #[test]
    fn locate_prefers_rodata_scemoduleinfo_over_lib_ent() {
        // Regression for issue #52 root cause 2: .lib.ent (export table, added
        // FIRST so it has the lower section index) must never win.
        let fixture = PrxFixture::new()
            .text(&[0, 0])
            .alloc_bytes(".lib.ent", &[0xEE; 16])
            .module_info(&mi_fixture());
        let mi_va = fixture.va_of(".rodata.sceModuleInfo");
        let bytes = fixture.build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        assert_eq!(locate_module_info_va(&elf_obj, 0).unwrap(), mi_va);
        // load_base is folded into the returned va.
        assert_eq!(
            locate_module_info_va(&elf_obj, PSP_USER_MODULE_BASE).unwrap(),
            PSP_USER_MODULE_BASE + mi_va
        );
    }

    #[test]
    fn locate_sectionless_uses_p_paddr_file_offset_and_masks_kernel_bit() {
        let fixture = PrxFixture::new()
            .text(&[0, 0])
            .module_info(&mi_fixture())
            .kernel_bit_on_paddr()
            .without_section_headers();
        let mi_va = fixture.va_of(".rodata.sceModuleInfo");
        let bytes = fixture.build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        assert!(elf_obj.section_headers.is_empty());
        assert_ne!(
            elf_obj.program_headers[0].p_paddr & 0x8000_0000,
            0,
            "fixture must set the kernel bit"
        );
        // p_paddr is a FILE offset: va = load_base + p_vaddr + (file_off - p_offset).
        assert_eq!(
            locate_module_info_va(&elf_obj, PSP_USER_MODULE_BASE).unwrap(),
            PSP_USER_MODULE_BASE + mi_va
        );
    }

    #[test]
    fn locate_with_sections_but_no_modinfo_section_falls_back_to_p_paddr() {
        // The ET_EXEC-without-section case (PPSSPP falls back whenever the
        // section is missing, not only for fully stripped binaries).
        let fixture = PrxFixture::new()
            .text(&[0, 0])
            .module_info_via_paddr_only(&mi_fixture());
        let mi_va = fixture.va_of(".rodata.modinfo.hidden");
        let bytes = fixture.build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        assert!(!elf_obj.section_headers.is_empty());
        assert_eq!(locate_module_info_va(&elf_obj, 0).unwrap(), mi_va);
    }

    #[test]
    fn locate_without_any_modinfo_is_an_actionable_error() {
        // No modinfo section and p_paddr = 0 (< p_offset): must error, never
        // silently return garbage.
        let bytes = PrxFixture::new()
            .text(&[0, 0])
            .without_section_headers()
            .build();
        let err = locate_module_info_va(&elf::parse_elf(&bytes).unwrap(), 0).unwrap_err();
        let msg = err.to_string();
        assert!(msg.contains("SceModuleInfo not found"), "actionable: {msg}");
        assert!(msg.contains("p_paddr"), "names the failing field: {msg}");
    }

    #[test]
    fn parse_module_info_decodes_fields_exactly() {
        let mi = mi_fixture();
        let fixture = PrxFixture::new().text(&[0, 0]).module_info(&mi);
        let mi_va = fixture.va_of(".rodata.sceModuleInfo");
        let bytes = fixture.build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        let segments = elf::extract_segments(&bytes, &elf_obj);
        let datas: Vec<Vec<u8>> = segments.iter().map(|s| s.data.clone()).collect();
        let bases: Vec<u32> = segments
            .iter()
            .map(|s| PSP_USER_MODULE_BASE + s.p_vaddr)
            .collect();
        let image = LoadedImage::new(&bases, &datas);
        let parsed = parse_module_info(&image, PSP_USER_MODULE_BASE + mi_va).unwrap();
        assert_eq!(
            parsed,
            ModuleInfo {
                attrs: 0x0002,
                version: 0x0101,
                name: "testmod".into(),
                gp: 0x0008_5BD0,
                libent: 0x40,
                libent_end: 0x50,
                libstub: 0x50,
                libstub_end: 0x64,
            }
        );
    }

    #[test]
    fn parse_module_info_unreadable_va_is_an_actionable_error() {
        let datas = [vec![0u8; 16]];
        let image = LoadedImage::new(&[0x0880_4000], &datas);
        let err = parse_module_info(&image, 0x0990_0000).unwrap_err();
        let msg = err.to_string();
        assert!(
            msg.contains("SceModuleInfo at 0x09900000"),
            "actionable: {msg}"
        );
    }
}
