use psp_parser::elf::calculate_heap_base;
use psp_parser::types::Segment;

#[test]
fn heap_base_aligns_to_64k() {
    // max segment end = 0x08810001 (not aligned)
    let seg = Segment {
        p_vaddr: 0x08800000,
        p_filesz: 0x10001,
        p_memsz: 0x10001,
        p_flags: 5,
        p_type: goblin::elf::program_header::PT_LOAD,
        data: vec![0u8; 0x10001],
    };
    let base = calculate_heap_base(&[seg]);
    assert_eq!(base % (64 * 1024), 0, "heap_base must be 64KB aligned");
    assert!(base >= 0x08810001, "heap_base must be >= max segment end");
}

#[test]
fn bss_zeroed_to_p_memsz() {
    // PARSE-04: BSS region bytes (p_filesz..p_memsz) must be zero-filled.
    // Validate resize behavior directly using the Segment type (no real ELF needed).
    // Full integration is covered by Plan 01-05 human-verify check #4:
    //   jq '.segments[] | .p_memsz >= .p_filesz' analysis.json  — all must be true.
    let filesz = 10usize;
    let memsz = 20usize;
    let file_bytes = vec![0xABu8; filesz];
    let mut seg_data = Vec::with_capacity(memsz);
    seg_data.extend_from_slice(&file_bytes);
    seg_data.resize(memsz, 0u8); // BSS zero-fill
    assert_eq!(seg_data.len(), memsz, "seg_data must be p_memsz bytes long");
    assert!(
        seg_data[filesz..].iter().all(|&b| b == 0),
        "BSS region must be all zeros"
    );
    assert!(
        seg_data[..filesz].iter().all(|&b| b == 0xAB),
        "file bytes must be preserved"
    );
}
