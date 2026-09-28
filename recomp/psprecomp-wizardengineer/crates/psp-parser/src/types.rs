//! Core data types shared across psp-parser modules.

/// A loaded PT_LOAD segment from an ELF/PRX binary.
#[derive(Debug, Clone)]
pub struct Segment {
    pub p_vaddr: u32,
    pub p_filesz: u32,
    /// Always p_memsz (BSS is zero-filled to this size, not p_filesz).
    pub p_memsz: u32,
    pub p_flags: u32,
    pub p_type: u32,
    /// Raw bytes, length == p_memsz. BSS region (p_filesz..p_memsz) is zeroed.
    pub data: Vec<u8>,
}

/// A single PRX/ELF relocation entry.
#[derive(Debug, Clone)]
pub struct RelocEntry {
    pub offset: u32,
    pub r_type: u8,
    pub ofs_base: u8,
    pub addr_base: u8,
}

/// A resolved PSP import stub (NID -> function name + stub address).
#[derive(Debug, Clone)]
pub struct ImportStub {
    pub nid: u32,
    pub stub_addr: u32,
    pub name: String,
    pub module_name: String,
}

/// Parsed binary combining ELF structure and resolved data.
#[derive(Debug)]
pub struct ParsedBinary {
    pub binary_path: String,
    pub module_name: String,
    pub heap_base: u32,
    pub segments: Vec<Segment>,
    pub imports: Vec<ImportStub>,
    pub relocations: Vec<RelocEntry>,
    pub is_prx: bool,
}
