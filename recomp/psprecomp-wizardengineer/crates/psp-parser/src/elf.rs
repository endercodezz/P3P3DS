use crate::errors::ParseError;
use crate::types::Segment;
use goblin::elf::program_header::PT_LOAD;

/// Parse an ELF/PRX binary from raw bytes.
///
/// Returns the parsed goblin Elf struct. Two common non-ELF inputs from real
/// game dumps get actionable errors instead of a cryptic bad-magic failure
/// (found bringing up an untested game, issue #37 spirit): the `~PSP`
/// encrypted-executable header, and all-zero dummy BOOT.BIN placeholders.
pub fn parse_elf(data: &[u8]) -> Result<goblin::elf::Elf<'_>, ParseError> {
    if data.starts_with(b"~PSP") {
        return Err(ParseError::EncryptedPsp);
    }
    if data.len() >= 64 && data[..64].iter().all(|&b| b == 0) {
        return Err(ParseError::DummyZeroes);
    }
    Ok(goblin::elf::Elf::parse(data)?)
}

#[cfg(test)]
mod parse_guard_tests {
    use super::*;

    #[test]
    fn encrypted_psp_magic_gets_actionable_error() {
        let mut data = b"~PSP".to_vec();
        data.resize(256, 0xAA);
        let err = parse_elf(&data).unwrap_err();
        assert!(matches!(err, ParseError::EncryptedPsp));
        assert!(err.to_string().contains("encrypted ~PSP"));
    }

    #[test]
    fn all_zero_dummy_gets_actionable_error() {
        let data = vec![0u8; 4096];
        let err = parse_elf(&data).unwrap_err();
        assert!(matches!(err, ParseError::DummyZeroes));
        assert!(err.to_string().contains("dummy"));
    }

    #[test]
    fn real_elf_magic_still_reaches_goblin() {
        // \x7fELF but truncated: must NOT hit the guards; goblin reports it.
        let data = b"\x7fELF".to_vec();
        let err = parse_elf(&data).unwrap_err();
        assert!(matches!(err, ParseError::Elf(_)));
    }
}

/// Extract all PT_LOAD segments. BSS region (p_memsz > p_filesz) is zeroed.
///
/// CRITICAL: Always uses p_memsz for the output slice size.
/// Missing this zeroing causes the dlmalloc infinite loop (V1 pitfall #1).
pub fn extract_segments(data: &[u8], elf: &goblin::elf::Elf) -> Vec<Segment> {
    elf.program_headers
        .iter()
        .filter(|ph| ph.p_type == PT_LOAD)
        .map(|ph| {
            let filesz = ph.p_filesz as usize;
            let memsz = ph.p_memsz as usize;
            // Copy file bytes, then zero-fill BSS.
            let file_end = (ph.p_offset as usize) + filesz;
            let file_bytes = &data[ph.p_offset as usize..file_end];
            let mut seg_data = Vec::with_capacity(memsz);
            seg_data.extend_from_slice(file_bytes);
            seg_data.resize(memsz, 0u8); // zero-fill BSS
            Segment {
                p_vaddr: ph.p_vaddr as u32,
                p_filesz: ph.p_filesz as u32,
                p_memsz: ph.p_memsz as u32,
                p_flags: ph.p_flags,
                p_type: ph.p_type,
                data: seg_data,
            }
        })
        .collect()
}

/// Add `load_base` to every segment's `p_vaddr` (no-op when 0).
///
/// Relocatable PRX modules are linked at 0 and rebased to the PSP user-module
/// load base (`crate::prx::PSP_USER_MODULE_BASE` by default) at load time;
/// ET_EXEC binaries load where linked and pass `load_base == 0`. Everything
/// downstream (heap base, relocation bases, JSON segment records, the HLE
/// scan) derives from the rebased `p_vaddr` values automatically.
pub fn rebase_segments(segments: &mut [Segment], load_base: u32) {
    if load_base == 0 {
        return;
    }
    for seg in segments {
        seg.p_vaddr = seg.p_vaddr.wrapping_add(load_base);
    }
}

#[cfg(test)]
mod rebase_tests {
    use super::*;

    fn seg(p_vaddr: u32) -> Segment {
        Segment {
            p_vaddr,
            p_filesz: 16,
            p_memsz: 32,
            p_flags: 5,
            p_type: goblin::elf::program_header::PT_LOAD,
            data: vec![0; 32],
        }
    }

    #[test]
    fn rebase_adds_load_base_to_every_vaddr() {
        let mut segs = [seg(0), seg(0x1000)];
        rebase_segments(&mut segs, 0x0880_4000);
        assert_eq!(segs[0].p_vaddr, 0x0880_4000);
        assert_eq!(segs[1].p_vaddr, 0x0880_5000);
    }

    #[test]
    fn rebase_zero_is_a_noop() {
        let mut segs = [seg(0x0880_4000)];
        rebase_segments(&mut segs, 0);
        assert_eq!(segs[0].p_vaddr, 0x0880_4000);
    }
}

/// Text extent (text_start, text_size) per PPSSPP `ElfReader` semantics.
///
/// Mirrors `Core/HLE/sceKernelModule.cpp __KernelLoadELFFromPtr`:
/// - when a section named `.text` exists: `text_start = load_base + sh_addr`
///   of `.text`, and `text_size = GetTotalTextSize()` — the sum of `sh_size`
///   over sections with SHF_ALLOC set, SHF_WRITE clear, SHF_STRINGS clear
///   (this includes `.sceStub.text`, `.lib.*`, `.rodata.sce*` — Patapon:
///   0x001D4E04);
/// - otherwise (stripped/sectionless): `text_start = load_base + first
///   PT_LOAD p_vaddr` and `text_size = GetTotalTextSizeFromSeg()` — the sum
///   of `p_filesz` over PF_X segments.
///
/// NOTE: a previous runtime hardcoding used the exec phdr's p_filesz
/// (Patapon 0x244D30) for text_size; that is what PPSSPP only does for
/// sectionless inputs. Patapon has sections, so the faithful value is
/// 0x001D4E04 (issue #47 Phase 2 differential check, documented in
/// DEBUGGING.md).
pub fn text_extent(elf: &goblin::elf::Elf, load_base: u32) -> (u32, u32) {
    use goblin::elf::program_header::PF_X;
    use goblin::elf::section_header::{SHF_ALLOC, SHF_STRINGS, SHF_WRITE};

    let text_section = elf
        .section_headers
        .iter()
        .find(|sh| elf.shdr_strtab.get_at(sh.sh_name) == Some(".text"));
    if let Some(text) = text_section {
        let total: u32 = elf
            .section_headers
            .iter()
            .filter(|sh| {
                let f = sh.sh_flags as u32;
                (f & SHF_ALLOC != 0) && (f & SHF_WRITE == 0) && (f & SHF_STRINGS == 0)
            })
            .map(|sh| sh.sh_size as u32)
            .fold(0u32, u32::wrapping_add);
        return (load_base.wrapping_add(text.sh_addr as u32), total);
    }
    let start = elf
        .program_headers
        .iter()
        .find(|ph| ph.p_type == PT_LOAD)
        .map(|ph| load_base.wrapping_add(ph.p_vaddr as u32))
        .unwrap_or(load_base);
    let total: u32 = elf
        .program_headers
        .iter()
        .filter(|ph| ph.p_type == PT_LOAD && ph.p_flags & PF_X != 0)
        .map(|ph| ph.p_filesz as u32)
        .fold(0u32, u32::wrapping_add);
    (start, total)
}

#[cfg(test)]
mod text_extent_tests {
    use super::*;
    use crate::test_fixtures::{ModuleInfoFixture, PrxFixture};

    fn mi() -> ModuleInfoFixture {
        ModuleInfoFixture {
            attrs: 2,
            version: 0x0101,
            name: "fixture",
            gp: 0,
            libent: 0,
            libent_end: 0,
            libstub: 0,
            libstub_end: 0,
        }
    }

    #[test]
    fn section_path_sums_alloc_nonwrite_sections() {
        // .text (8 words = 32 bytes) + .rodata.sceModuleInfo (0x34 bytes):
        // both SHF_ALLOC, neither SHF_WRITE -> PPSSPP GetTotalTextSize sums
        // them; text_start is .text's VA + load_base.
        let fixture = PrxFixture::new().text(&[0u32; 8]).module_info(&mi());
        let text_va = fixture.va_of(".text");
        let bytes = fixture.build();
        let elf = parse_elf(&bytes).unwrap();
        let (start, size) = text_extent(&elf, 0x0880_4000);
        assert_eq!(start, 0x0880_4000 + text_va);
        assert_eq!(size, 32 + 0x34);
    }

    #[test]
    fn sectionless_path_sums_pf_x_filesz() {
        // Stripped fixture: no section headers -> PPSSPP
        // GetTotalTextSizeFromSeg (PF_X segments' p_filesz; fixture phdr is
        // RWX) and text_start = first PT_LOAD vaddr + load_base.
        let fixture = PrxFixture::new()
            .text(&[0u32; 8])
            .module_info(&mi())
            .without_section_headers();
        let bytes = fixture.build();
        let elf = parse_elf(&bytes).unwrap();
        let seg_filesz = elf
            .program_headers
            .iter()
            .find(|ph| ph.p_type == PT_LOAD)
            .unwrap()
            .p_filesz as u32;
        let (start, size) = text_extent(&elf, 0x0880_4000);
        assert_eq!(start, 0x0880_4000);
        assert_eq!(size, seg_filesz);
    }
}

/// Calculate heap base as max(segment end addresses) rounded up to 64KB.
///
/// ASSERT: heap_base > every segment's p_vaddr + p_memsz (V1 pitfall #8).
pub fn calculate_heap_base(segments: &[Segment]) -> u32 {
    const ALIGN_64K: u32 = 64 * 1024;
    let max_end = segments
        .iter()
        .map(|s| s.p_vaddr.saturating_add(s.p_memsz))
        .max()
        .unwrap_or(0);
    let aligned = max_end.wrapping_add(ALIGN_64K - 1) & !(ALIGN_64K - 1);
    assert!(
        aligned >= max_end,
        "heap_base 0x{aligned:08X} must be >= max segment end 0x{max_end:08X}"
    );
    aligned
}
