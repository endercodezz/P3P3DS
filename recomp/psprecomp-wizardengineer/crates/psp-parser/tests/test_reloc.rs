use psp_parser::reloc::{
    apply_relocations, parse_type_a_entries, R_MIPS_26, R_MIPS_32, R_MIPS_HI16, R_MIPS_LO16,
};
use psp_parser::types::RelocEntry;

// Type-B (0x700000A1) packed relocations are intentionally unsupported: the
// old varint parser matched no real format and was deleted (issue #52, plan
// D5). Discovery in the analyze pipeline rejects Type-B with an actionable
// error instead of routing entries through the Type-A switch.

// ────────────────────────────────────────────────────────────
// Type-A parsing tests
// ────────────────────────────────────────────────────────────

#[test]
fn parse_type_a_r_mips_32_single_entry() {
    // 8-byte Type-A entry: offset=0x00000010, r_info=(addr_base<<16)|(ofs_base<<8)|2
    let raw: &[u8] = &[
        0x10, 0x00, 0x00, 0x00, // offset = 0x10
        0x02, 0x00, 0x00, 0x00, // r_type=2 (R_MIPS_32), ofs_base=0, addr_base=0
    ];
    let entries = parse_type_a_entries(raw).expect("parse must succeed");
    assert_eq!(entries.len(), 1);
    assert_eq!(entries[0].offset, 0x10);
    assert_eq!(entries[0].r_type, R_MIPS_32);
    assert_eq!(entries[0].ofs_base, 0);
    assert_eq!(entries[0].addr_base, 0);
}

#[test]
fn parse_type_a_multiple_entries() {
    // Two 8-byte entries; PPSSPP bit positions (plan D10):
    // type = r_info & 0xF, ofs_base = (r_info >> 8) & 0xFF,
    // addr_base = (r_info >> 16) & 0xFF.
    let raw: &[u8] = &[
        // Entry 1: offset=4, R_MIPS_26, ofs_base=0, addr_base=0
        0x04, 0x00, 0x00, 0x00, //
        0x04, 0x00, 0x00, 0x00, //
        // Entry 2: offset=8, R_MIPS_HI16, ofs_base=1, addr_base=2
        // r_info = (2<<16)|(1<<8)|5 = 0x00020105; LE bytes: [0x05, 0x01, 0x02, 0x00]
        0x08, 0x00, 0x00, 0x00, //
        0x05, 0x01, 0x02, 0x00, //
    ];
    let entries = parse_type_a_entries(raw).expect("parse must succeed");
    assert_eq!(entries.len(), 2);
    assert_eq!(entries[0].r_type, R_MIPS_26);
    assert_eq!(entries[1].r_type, R_MIPS_HI16);
    assert_eq!(entries[1].ofs_base, 1);
    assert_eq!(entries[1].addr_base, 2);
}

#[test]
fn parse_type_a_truncated_table_is_an_error() {
    let err = parse_type_a_entries(&[0u8; 9]).unwrap_err();
    assert!(err.to_string().contains("not a multiple of 8"));
}

// ────────────────────────────────────────────────────────────
// Type-A application tests
// ────────────────────────────────────────────────────────────

#[test]
fn apply_r_mips_32_adds_segment_base() {
    // Segment 0: base=0x08804000, data has u32 0x00000100 at offset 4.
    let seg_base: u32 = 0x08804000;
    let mut seg_data = vec![0u8; 8];
    seg_data[4..8].copy_from_slice(&0x00000100u32.to_le_bytes());

    let entries = vec![RelocEntry {
        offset: 4,
        r_type: R_MIPS_32,
        ofs_base: 0,
        addr_base: 0,
    }];

    let mut segs = [seg_data];
    apply_relocations(&mut segs, &[seg_base], &entries).unwrap();
    let result = u32::from_le_bytes(segs[0][4..8].try_into().unwrap());
    assert_eq!(result, 0x08804100u32);
}

#[test]
fn apply_r_mips_26_encodes_jump_target() {
    // jal 0x10 at offset 0: 0x0C000004. R_MIPS_26 adds relocate_to >> 2 to the
    // 26-bit field: 4 + (0x08804000 >> 2) = 0x02201004 => word 0x0E201004,
    // i.e. jal 0x08804010.
    let seg_base: u32 = 0x08804000;
    let mut segs = [0x0C000004u32.to_le_bytes().to_vec()];
    let entries = vec![RelocEntry {
        offset: 0,
        r_type: R_MIPS_26,
        ofs_base: 0,
        addr_base: 0,
    }];
    apply_relocations(&mut segs, &[seg_base], &entries).unwrap();
    let word = u32::from_le_bytes(segs[0][0..4].try_into().unwrap());
    assert_eq!(word, 0x0E201004);
    assert_eq!(
        (word & 0x03FFFFFF) << 2,
        0x08804010,
        "jump target must be rebased"
    );
}

#[test]
fn hi16_lo16_pair_reconstructs_address() {
    // lui $a0, 0x0000 / addiu $a0, $a0, 0x0100 around base 0x08804000:
    // combined = 0x08804100 => hi 0x0880, lo 0x4100.
    let seg_base: u32 = 0x08804000;
    let mut seg = 0x3C040000u32.to_le_bytes().to_vec();
    seg.extend_from_slice(&0x24840100u32.to_le_bytes());
    let mut segs = [seg];
    let entries = vec![
        RelocEntry {
            offset: 0,
            r_type: R_MIPS_HI16,
            ofs_base: 0,
            addr_base: 0,
        },
        RelocEntry {
            offset: 4,
            r_type: R_MIPS_LO16,
            ofs_base: 0,
            addr_base: 0,
        },
    ];
    let stats = apply_relocations(&mut segs, &[seg_base], &entries).unwrap();
    assert_eq!(stats.handled, 2);
    let hi = u32::from_le_bytes(segs[0][0..4].try_into().unwrap());
    let lo = u32::from_le_bytes(segs[0][4..8].try_into().unwrap());
    assert_eq!(hi, 0x3C040880);
    assert_eq!(lo, 0x24844100);
    let rebuilt = ((hi & 0xFFFF) << 16).wrapping_add((lo & 0xFFFF) as i16 as i32 as u32);
    assert_eq!(rebuilt, 0x08804100);
}
