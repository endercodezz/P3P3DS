//! Synthetic PRX fixture builder for unit tests (issue #52 test plan §5.1).
//!
//! Builds a minimal in-memory PRX byte image: ELF header (e_type 0xFFA0,
//! e_machine MIPS), one PT_LOAD at p_vaddr 0, optional section headers with a
//! shstrtab, and parameterizable reloc tables / SceModuleInfo / libstub
//! contents — so parser tests exercise real goblin-parsed inputs instead of
//! hand-fed structs.

const EHDR_SIZE: usize = 52;
const PHDR_SIZE: usize = 32;
const SHDR_SIZE: usize = 40;
const SHT_PSPREL: u32 = 0x700000A0;
const SHF_ALLOC: u32 = 0x2;
const SHF_EXECINSTR: u32 = 0x4;

/// Parameterizable 0x34-byte SceModuleInfo record (spec R2 §1.3 layout).
#[derive(Debug, Clone)]
pub(crate) struct ModuleInfoFixture {
    pub attrs: u16,
    pub version: u16,
    pub name: &'static str,
    pub gp: u32,
    pub libent: u32,
    pub libent_end: u32,
    pub libstub: u32,
    pub libstub_end: u32,
}

impl ModuleInfoFixture {
    /// Encodes the 0x34-byte on-disk record.
    pub fn encode(&self) -> Vec<u8> {
        let mut out = Vec::with_capacity(0x34);
        out.extend_from_slice(&self.attrs.to_le_bytes());
        out.extend_from_slice(&self.version.to_le_bytes());
        let mut name = [0u8; 28];
        name[..self.name.len().min(28)]
            .copy_from_slice(&self.name.as_bytes()[..self.name.len().min(28)]);
        out.extend_from_slice(&name);
        for field in [
            self.gp,
            self.libent,
            self.libent_end,
            self.libstub,
            self.libstub_end,
        ] {
            out.extend_from_slice(&field.to_le_bytes());
        }
        out
    }
}

/// Parameterizable `PspLibStubEntry` record (spec R2 §6.2 layout).
///
/// `size_words` controls both the encoded length (5 → 20 bytes, 6 → +var_data,
/// 7 → +extra) and the walker's advance; deliberately inconsistent values
/// (e.g. 0 or 4) are encoded as 20-byte records for malformed-input tests.
#[derive(Debug, Clone, Default)]
pub(crate) struct LibStubEntryFixture {
    pub name_va: u32,
    pub size_words: u8,
    pub num_vars: u8,
    pub num_funcs: u16,
    pub nid_data: u32,
    pub first_sym_addr: u32,
    pub var_data: u32,
    pub extra: u32,
}

impl LibStubEntryFixture {
    /// Encodes the on-disk record (`size_words * 4` bytes when size ≥ 5,
    /// else the minimal 20 bytes).
    pub fn encode(&self) -> Vec<u8> {
        let mut out = Vec::with_capacity(28);
        out.extend_from_slice(&self.name_va.to_le_bytes());
        out.extend_from_slice(&0x0101u16.to_le_bytes()); // version
        out.extend_from_slice(&0u16.to_le_bytes()); // flags
        out.push(self.size_words);
        out.push(self.num_vars);
        out.extend_from_slice(&self.num_funcs.to_le_bytes());
        out.extend_from_slice(&self.nid_data.to_le_bytes());
        out.extend_from_slice(&self.first_sym_addr.to_le_bytes());
        if self.size_words >= 6 {
            out.extend_from_slice(&self.var_data.to_le_bytes());
        }
        if self.size_words >= 7 {
            out.extend_from_slice(&self.extra.to_le_bytes());
        }
        out
    }
}

/// One reloc table in the fixture: a section of the given `sh_type` (or a
/// program header of that `p_type` for sectionless fixtures).
struct RelocSpec {
    name: String,
    bytes: Vec<u8>,
    sh_type: u32,
    /// Section index recorded in sh_info; defaults to the first alloc section.
    sh_info: Option<u32>,
}

/// In-memory PRX builder. Alloc sections land in the single PT_LOAD segment
/// (va == offset within the segment, p_vaddr 0); reloc tables are non-alloc
/// sections of `sh_type` 0x700000A0/0x700000A1 — or, for a sectionless
/// fixture, extra program headers of that p_type.
pub(crate) struct PrxFixture {
    seg: Vec<u8>,
    /// (section name, va, size) for in-segment sections, in layout order.
    alloc_sections: Vec<(String, u32, u32)>,
    relocs: Vec<RelocSpec>,
    bss: u32,
    e_entry: u32,
    emit_section_headers: bool,
    /// Emit reloc program headers even when section headers are present
    /// (tests that phdrs are ignored when sections exist).
    reloc_phdrs_too: bool,
    /// VA (within the segment) that PT_LOAD p_paddr points at as a file offset.
    p_paddr_va: Option<u32>,
    /// OR 0x80000000 (the kernel bit) into the emitted p_paddr.
    kernel_bit: bool,
}

impl PrxFixture {
    pub fn new() -> Self {
        Self {
            seg: Vec::new(),
            alloc_sections: Vec::new(),
            relocs: Vec::new(),
            bss: 0,
            e_entry: 0,
            emit_section_headers: true,
            reloc_phdrs_too: false,
            p_paddr_va: None,
            kernel_bit: false,
        }
    }

    /// Appends an in-segment PROGBITS section at the next 4-aligned va.
    pub fn alloc_bytes(mut self, name: &str, bytes: &[u8]) -> Self {
        while !self.seg.len().is_multiple_of(4) {
            self.seg.push(0);
        }
        let va = self.seg.len() as u32;
        self.seg.extend_from_slice(bytes);
        self.alloc_sections
            .push((name.to_string(), va, bytes.len() as u32));
        self
    }

    /// `.text` contents as instruction words.
    pub fn text(self, words: &[u32]) -> Self {
        let bytes: Vec<u8> = words.iter().flat_map(|w| w.to_le_bytes()).collect();
        self.alloc_bytes(".text", &bytes)
    }

    /// `.rodata.sceModuleInfo` contents; also points PT_LOAD p_paddr at it
    /// (the PRX convention, spec R2 §1.3).
    pub fn module_info(self, mi: &ModuleInfoFixture) -> Self {
        let mut this = self.alloc_bytes(".rodata.sceModuleInfo", &mi.encode());
        this.p_paddr_va = Some(this.va_of(".rodata.sceModuleInfo"));
        this
    }

    /// Module info reachable ONLY via the p_paddr file-offset convention —
    /// the record lives in a section that is NOT named `.rodata.sceModuleInfo`
    /// (the ET_EXEC-without-section / misnamed-section case).
    pub fn module_info_via_paddr_only(self, mi: &ModuleInfoFixture) -> Self {
        let mut this = self.alloc_bytes(".rodata.modinfo.hidden", &mi.encode());
        this.p_paddr_va = Some(this.va_of(".rodata.modinfo.hidden"));
        this
    }

    /// Sets the kernel bit (0x80000000) on the emitted PT_LOAD p_paddr.
    pub fn kernel_bit_on_paddr(mut self) -> Self {
        self.kernel_bit = true;
        self
    }

    /// `.lib.stub` contents (raw PspLibStubEntry records; for the T4 walker tests).
    pub fn libstub(self, bytes: &[u8]) -> Self {
        self.alloc_bytes(".lib.stub", bytes)
    }

    /// A Type-A reloc table from (r_offset, r_info) pairs.
    pub fn reloc(self, name: &str, entries: &[(u32, u32)]) -> Self {
        let bytes: Vec<u8> = entries
            .iter()
            .flat_map(|&(off, info)| {
                let mut e = off.to_le_bytes().to_vec();
                e.extend_from_slice(&info.to_le_bytes());
                e
            })
            .collect();
        self.reloc_raw(name, bytes)
    }

    /// A Type-A reloc table with an explicit sh_info section index.
    pub fn reloc_with_info(self, name: &str, entries: &[(u32, u32)], sh_info: u32) -> Self {
        let mut this = self.reloc(name, entries);
        this.relocs.last_mut().expect("just pushed").sh_info = Some(sh_info);
        this
    }

    /// A Type-A reloc table from raw bytes (e.g. deliberately truncated).
    pub fn reloc_raw(self, name: &str, bytes: Vec<u8>) -> Self {
        self.reloc_typed(name, bytes, SHT_PSPREL)
    }

    /// A reloc table with an explicit sh_type / p_type (0x700000A1 for Type-B).
    pub fn reloc_typed(mut self, name: &str, bytes: Vec<u8>, sh_type: u32) -> Self {
        self.relocs.push(RelocSpec {
            name: name.to_string(),
            bytes,
            sh_type,
            sh_info: None,
        });
        self
    }

    /// Emit reloc program headers even though section headers are present.
    pub fn reloc_phdrs_alongside_sections(mut self) -> Self {
        self.reloc_phdrs_too = true;
        self
    }

    /// Extends p_memsz beyond p_filesz (zero-filled BSS).
    pub fn bss(mut self, size: u32) -> Self {
        self.bss = size;
        self
    }

    /// Sets e_entry (a va inside the segment).
    #[allow(dead_code)]
    pub fn entry(mut self, va: u32) -> Self {
        self.e_entry = va;
        self
    }

    /// Stripped-PRX variant: e_shnum == 0; reloc tables become program headers.
    #[allow(dead_code)]
    pub fn without_section_headers(mut self) -> Self {
        self.emit_section_headers = false;
        self
    }

    /// The va assigned to an alloc section added earlier (test convenience).
    pub fn va_of(&self, name: &str) -> u32 {
        self.alloc_sections
            .iter()
            .find(|(n, _, _)| n == name)
            .unwrap_or_else(|| panic!("fixture has no alloc section named {name}"))
            .1
    }

    /// Serializes the fixture to ELF bytes.
    pub fn build(&self) -> Vec<u8> {
        let lay = self.layout();
        let mut out = Vec::with_capacity(lay.e_shoff + SHDR_SIZE * lay.shnum);
        self.write_ehdr(&mut out, &lay);
        self.write_phdrs(&mut out, &lay);
        pad_to(&mut out, lay.p_offset);
        out.extend_from_slice(&self.seg);
        for (spec, &off) in self.relocs.iter().zip(&lay.reloc_offsets) {
            pad_to(&mut out, off);
            out.extend_from_slice(&spec.bytes);
        }
        pad_to(&mut out, lay.shstrtab_offset);
        out.extend_from_slice(&lay.shstrtab);
        if self.emit_section_headers {
            pad_to(&mut out, lay.e_shoff);
            self.write_shdrs(&mut out, &lay);
        }
        out
    }

    /// True when reloc tables are also emitted as program headers.
    fn emits_reloc_phdrs(&self) -> bool {
        !self.emit_section_headers || self.reloc_phdrs_too
    }

    fn layout(&self) -> Layout {
        let phnum = if self.emits_reloc_phdrs() {
            1 + self.relocs.len()
        } else {
            1
        };
        let p_offset = align(EHDR_SIZE + PHDR_SIZE * phnum, 16);
        let mut cursor = p_offset + self.seg.len();
        let mut reloc_offsets = Vec::new();
        for spec in &self.relocs {
            cursor = align(cursor, 4);
            reloc_offsets.push(cursor);
            cursor += spec.bytes.len();
        }
        let (shstrtab, name_offsets) = self.build_shstrtab();
        let shstrtab_offset = align(cursor, 4);
        let e_shoff = align(shstrtab_offset + shstrtab.len(), 4);
        let shnum = if self.emit_section_headers {
            2 + self.alloc_sections.len() + self.relocs.len()
        } else {
            0
        };
        Layout {
            phnum,
            p_offset,
            reloc_offsets,
            shstrtab,
            name_offsets,
            shstrtab_offset,
            e_shoff,
            shnum,
        }
    }

    /// shstrtab bytes + name offsets ordered [.shstrtab, alloc..., reloc...].
    fn build_shstrtab(&self) -> (Vec<u8>, Vec<u32>) {
        let mut tab = vec![0u8];
        let mut offsets = Vec::new();
        let alloc_names = self.alloc_sections.iter().map(|(n, _, _)| n.as_str());
        let reloc_names = self.relocs.iter().map(|spec| spec.name.as_str());
        for name in std::iter::once(".shstrtab")
            .chain(alloc_names)
            .chain(reloc_names)
        {
            offsets.push(tab.len() as u32);
            tab.extend_from_slice(name.as_bytes());
            tab.push(0);
        }
        (tab, offsets)
    }

    fn write_ehdr(&self, out: &mut Vec<u8>, lay: &Layout) {
        let mut ident = [0u8; 16];
        ident[..4].copy_from_slice(b"\x7fELF");
        ident[4] = 1; // ELFCLASS32
        ident[5] = 1; // ELFDATA2LSB
        ident[6] = 1; // EV_CURRENT
        out.extend_from_slice(&ident);
        push_u16(out, 0xFFA0); // e_type: ET_PSP_PRX
        push_u16(out, 8); // e_machine: MIPS
        push_u32(out, 1); // e_version
        push_u32(out, self.e_entry);
        push_u32(out, EHDR_SIZE as u32); // e_phoff
        push_u32(
            out,
            if self.emit_section_headers {
                lay.e_shoff as u32
            } else {
                0
            },
        );
        push_u32(out, 0x10A2_3001); // e_flags (Allegrex)
        push_u16(out, EHDR_SIZE as u16);
        push_u16(out, PHDR_SIZE as u16);
        push_u16(out, lay.phnum as u16);
        push_u16(out, SHDR_SIZE as u16);
        push_u16(out, lay.shnum as u16);
        push_u16(out, if self.emit_section_headers { 1 } else { 0 }); // e_shstrndx
    }

    fn write_phdrs(&self, out: &mut Vec<u8>, lay: &Layout) {
        // PT_LOAD; p_paddr carries the modinfo file offset (PRX convention).
        let kernel_bit = if self.kernel_bit { 0x8000_0000 } else { 0 };
        let p_paddr = self
            .p_paddr_va
            .map_or(0, |va| (lay.p_offset as u32 + va) | kernel_bit);
        let fields = [
            1, // PT_LOAD
            lay.p_offset as u32,
            0, // p_vaddr
            p_paddr,
            self.seg.len() as u32,
            self.seg.len() as u32 + self.bss, // p_memsz
            7,                                // p_flags RWX
            16,
        ];
        fields.iter().for_each(|&f| push_u32(out, f));
        if self.emits_reloc_phdrs() {
            for (spec, &off) in self.relocs.iter().zip(&lay.reloc_offsets) {
                let f = [
                    spec.sh_type,
                    off as u32,
                    0,
                    0,
                    spec.bytes.len() as u32,
                    0,
                    4,
                    4,
                ];
                f.iter().for_each(|&v| push_u32(out, v));
            }
        }
    }

    fn write_shdrs(&self, out: &mut Vec<u8>, lay: &Layout) {
        out.extend_from_slice(&[0u8; SHDR_SIZE]); // index 0: SHT_NULL
        let mut names = lay.name_offsets.iter().copied();
        // index 1: .shstrtab
        let shstrtab = Shdr {
            name: names.next().unwrap(),
            sh_type: 3, // SHT_STRTAB
            flags: 0,
            addr: 0,
            offset: lay.shstrtab_offset as u32,
            size: lay.shstrtab.len() as u32,
            info: 0,
            entsize: 0,
        };
        shstrtab.write(out);
        let text_idx = 2u32; // first alloc section, target of reloc sh_info
        for (name, va, size) in &self.alloc_sections {
            let exec = if name == ".text" { SHF_EXECINSTR } else { 0 };
            Shdr {
                name: names.next().unwrap(),
                sh_type: 1, // SHT_PROGBITS
                flags: SHF_ALLOC | exec,
                addr: *va,
                offset: lay.p_offset as u32 + va,
                size: *size,
                info: 0,
                entsize: 0,
            }
            .write(out);
        }
        for (spec, &off) in self.relocs.iter().zip(&lay.reloc_offsets) {
            Shdr {
                name: names.next().unwrap(),
                sh_type: spec.sh_type,
                flags: 0,
                addr: 0,
                offset: off as u32,
                size: spec.bytes.len() as u32,
                info: spec.sh_info.unwrap_or(text_idx),
                entsize: 8,
            }
            .write(out);
        }
    }
}

struct Layout {
    phnum: usize,
    p_offset: usize,
    reloc_offsets: Vec<usize>,
    shstrtab: Vec<u8>,
    name_offsets: Vec<u32>,
    shstrtab_offset: usize,
    e_shoff: usize,
    shnum: usize,
}

struct Shdr {
    name: u32,
    sh_type: u32,
    flags: u32,
    addr: u32,
    offset: u32,
    size: u32,
    info: u32,
    entsize: u32,
}

impl Shdr {
    fn write(&self, out: &mut Vec<u8>) {
        for f in [
            self.name,
            self.sh_type,
            self.flags,
            self.addr,
            self.offset,
            self.size,
            0, // sh_link
            self.info,
            4, // sh_addralign
            self.entsize,
        ] {
            push_u32(out, f);
        }
    }
}

fn push_u16(out: &mut Vec<u8>, v: u16) {
    out.extend_from_slice(&v.to_le_bytes());
}

fn push_u32(out: &mut Vec<u8>, v: u32) {
    out.extend_from_slice(&v.to_le_bytes());
}

fn align(v: usize, to: usize) -> usize {
    v.div_ceil(to) * to
}

fn pad_to(out: &mut Vec<u8>, target: usize) {
    assert!(
        out.len() <= target,
        "fixture layout error: {} > {target}",
        out.len()
    );
    out.resize(target, 0);
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{elf, prx, reloc};

    /// r_info for a Type-A entry (PPSSPP bit positions, plan D10).
    fn r_info(r_type: u8, ofs_base: u8, addr_base: u8) -> u32 {
        (u32::from(addr_base) << 16) | (u32::from(ofs_base) << 8) | u32::from(r_type)
    }

    fn pspsht_tables<'e>(elf_obj: &'e goblin::elf::Elf) -> Vec<&'e goblin::elf::SectionHeader> {
        elf_obj
            .section_headers
            .iter()
            .filter(|sh| sh.sh_type == 0x700000A0)
            .collect()
    }

    #[test]
    fn fixture_parses_and_each_reloc_type_applies_end_to_end() {
        let fixture = PrxFixture::new()
            .text(&[
                0x1234_0010, // R_MIPS_16
                0x0000_0010, // R_MIPS_32
                0x0C00_0004, // R_MIPS_26 (jal 0x10)
                0xDEAD_BEEF, // R_MIPS_NONE
                0x3C04_0000, // R_MIPS_HI16
                0x2484_0010, // R_MIPS_LO16
            ])
            .bss(0x100)
            .reloc(
                ".rel.text",
                &[
                    (0, r_info(reloc::R_MIPS_16, 0, 0)),
                    (4, r_info(reloc::R_MIPS_32, 0, 0)),
                    (8, r_info(reloc::R_MIPS_26, 0, 0)),
                    (12, r_info(reloc::R_MIPS_NONE, 0, 0)),
                    (16, r_info(reloc::R_MIPS_HI16, 0, 0)),
                    (20, r_info(reloc::R_MIPS_LO16, 0, 0)),
                ],
            );
        let bytes = fixture.build();

        let elf_obj = elf::parse_elf(&bytes).expect("fixture must parse as ELF");
        assert!(prx::is_prx(&elf_obj), "e_type must be 0xFFA0");
        let segments = elf::extract_segments(&bytes, &elf_obj);
        assert_eq!(segments.len(), 1);
        assert_eq!(
            segments[0].p_memsz as usize,
            segments[0].p_filesz as usize + 0x100
        );

        let tables = pspsht_tables(&elf_obj);
        assert_eq!(tables.len(), 1, "one 0x700000A0 section expected");
        let (start, size) = (tables[0].sh_offset as usize, tables[0].sh_size as usize);
        let entries = reloc::parse_type_a_entries(&bytes[start..start + size]).unwrap();
        assert_eq!(entries.len(), 6);

        let mut datas = vec![segments[0].data.clone()];
        let stats = reloc::apply_relocations(&mut datas, &[0x0880_4000], &entries).unwrap();
        assert_eq!(stats.handled, 6);
        assert_eq!(stats.skipped_bad, 0);
        assert!(stats.unhandled.is_empty());
        let word = |off: usize| u32::from_le_bytes(datas[0][off..off + 4].try_into().unwrap());
        assert_eq!(word(0), 0x1234_4010);
        assert_eq!(word(4), 0x0880_4010);
        assert_eq!(word(8), 0x0E20_1004);
        assert_eq!(word(12), 0xDEAD_BEEF);
        assert_eq!(word(16), 0x3C04_0880);
        assert_eq!(word(20), 0x2484_4010);
    }

    #[test]
    fn fixture_hi16_lo16_sign_carry_roundtrip() {
        // lo imm 0x9000 forces the sign carry: hi 0x0880 (not the naive 0x087F).
        let bytes = PrxFixture::new()
            .text(&[0x3C04_0000, 0x2484_9000])
            .reloc(
                ".rel.text",
                &[
                    (0, r_info(reloc::R_MIPS_HI16, 0, 0)),
                    (4, r_info(reloc::R_MIPS_LO16, 0, 0)),
                ],
            )
            .build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        let segments = elf::extract_segments(&bytes, &elf_obj);
        let t = pspsht_tables(&elf_obj)[0];
        let entries = reloc::parse_type_a_entries(
            &bytes[t.sh_offset as usize..(t.sh_offset + t.sh_size) as usize],
        )
        .unwrap();
        let mut datas = vec![segments[0].data.clone()];
        reloc::apply_relocations(&mut datas, &[0x0880_4000], &entries).unwrap();
        let hi = u32::from_le_bytes(datas[0][0..4].try_into().unwrap());
        let lo = u32::from_le_bytes(datas[0][4..8].try_into().unwrap());
        assert_eq!(hi & 0xFFFF, 0x0880, "sign carry must bump hi");
        assert_eq!(lo & 0xFFFF, 0xD000);
    }

    #[test]
    fn fixture_truncated_reloc_table_is_an_error() {
        let bytes = PrxFixture::new()
            .text(&[0x0000_0010])
            .reloc_raw(".rel.text", vec![0u8; 12]) // 12 % 8 != 0
            .build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        let t = pspsht_tables(&elf_obj)[0];
        let err = reloc::parse_type_a_entries(
            &bytes[t.sh_offset as usize..(t.sh_offset + t.sh_size) as usize],
        )
        .unwrap_err();
        assert!(
            err.to_string().contains("not a multiple of 8"),
            "actionable: {err}"
        );
    }

    #[test]
    fn fixture_module_info_section_and_p_paddr_convention() {
        let mi = ModuleInfoFixture {
            attrs: 0,
            version: 0x0101,
            name: "testmod",
            gp: 0x0008_5BD0,
            libent: 0x40,
            libent_end: 0x50,
            libstub: 0x50,
            libstub_end: 0x64,
        };
        let fixture = PrxFixture::new().text(&[0, 0]).module_info(&mi);
        let mi_va = fixture.va_of(".rodata.sceModuleInfo");
        let bytes = fixture.build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        let sh = elf_obj
            .section_headers
            .iter()
            .find(|sh| elf_obj.shdr_strtab.get_at(sh.sh_name) == Some(".rodata.sceModuleInfo"))
            .expect("modinfo section present");
        assert_eq!(sh.sh_addr as u32, mi_va);
        // p_paddr carries the modinfo *file offset* (PRX convention, R2 §1.3).
        let ph = &elf_obj.program_headers[0];
        assert_eq!(ph.p_paddr as usize, sh.sh_offset as usize);
        let rec = &bytes[sh.sh_offset as usize..sh.sh_offset as usize + 0x34];
        assert_eq!(&rec[4..11], b"testmod");
        assert_eq!(
            u32::from_le_bytes(rec[0x2C..0x30].try_into().unwrap()),
            0x50
        );
    }

    #[test]
    fn fixture_without_section_headers_uses_reloc_phdrs() {
        let bytes = PrxFixture::new()
            .text(&[0x0000_0010])
            .reloc(".rel.text", &[(0, r_info(reloc::R_MIPS_32, 0, 0))])
            .without_section_headers()
            .build();
        let elf_obj = elf::parse_elf(&bytes).unwrap();
        assert!(elf_obj.section_headers.is_empty());
        assert_eq!(elf_obj.program_headers.len(), 2);
        let rel_ph = &elf_obj.program_headers[1];
        assert_eq!(rel_ph.p_type, 0x700000A0);
        let entries = reloc::parse_type_a_entries(
            &bytes[rel_ph.p_offset as usize..(rel_ph.p_offset + rel_ph.p_filesz) as usize],
        )
        .unwrap();
        assert_eq!(entries.len(), 1);
        assert_eq!(entries[0].r_type, reloc::R_MIPS_32);
    }
}
