//! PRX Type-A relocation application, faithful to PPSSPP's engine.
//!
//! Reference: PPSSPP `Core/ELF/ElfReader.cpp` (`LoadRelocations`) — the normative
//! algorithm per `.planning/research/52-prx-format-spec.md` §4: two-pass
//! original-word cache, HI16 forward-scan pairing with sign carry, and
//! skip-don't-abort handling of malformed entries.
//!
//! Type-B (0x700000A1) packed relocations are intentionally NOT implemented:
//! discovery must reject them with an actionable error (spec §5, plan D5) —
//! no Type-B test binary exists, and silently emitting garbage is worse than
//! an honest failure.

use crate::errors::ParseError;
use crate::types::RelocEntry;
use byteorder::{LittleEndian, ReadBytesExt};
use std::collections::BTreeMap;
use std::io::Cursor;

// MIPS relocation type constants (Type-A numbering; Type-B numbering differs).
pub const R_MIPS_NONE: u8 = 0;
pub const R_MIPS_16: u8 = 1;
pub const R_MIPS_32: u8 = 2;
pub const R_MIPS_26: u8 = 4;
pub const R_MIPS_HI16: u8 = 5;
pub const R_MIPS_LO16: u8 = 6;
pub const R_MIPS_GPREL16: u8 = 7;

/// Relocation types this module knows how to apply.
///
/// `R_MIPS_NONE` is a defined no-op. `R_MIPS_GPREL16` stays deliberately
/// unhandled: PPSSPP ignores it ("almost a notification of a gp-relative
/// operation", spec §4.3) and leaving the word untouched is safe — it remains
/// visible in the #37 report buckets.
pub const HANDLED_RELOC_TYPES: [u8; 6] = [
    R_MIPS_NONE,
    R_MIPS_16,
    R_MIPS_32,
    R_MIPS_26,
    R_MIPS_HI16,
    R_MIPS_LO16,
];

/// Returns true if `r_type` has an implementation in [`apply_relocations`].
///
/// Used by the recompile report to recompute unhandled-relocation counts from
/// the `relocations` array persisted in analysis.json (single source of truth
/// for what "handled" means).
pub fn is_handled_reloc_type(r_type: u8) -> bool {
    HANDLED_RELOC_TYPES.contains(&r_type)
}

/// Outcome statistics from one [`apply_relocations`] run.
#[derive(Debug, Default, Clone, PartialEq, Eq)]
pub struct RelocStats {
    /// Entries applied, including `R_MIPS_NONE` no-ops and paired HI16s.
    pub handled: u64,
    /// `r_type` -> count of entries the engine did not relocate. For unknown
    /// types the target word is left untouched; an unpaired `R_MIPS_HI16` is
    /// also counted here even though its byte result (hi = 0) is written for
    /// byte compatibility with PPSSPP/ghidra-allegrex.
    pub unhandled: BTreeMap<u8, u64>,
    /// Entries skipped for safety (OOB segment index, OOB target offset,
    /// misaligned non-R_MIPS_32 target). Never fatal, always loud.
    pub skipped_bad: u64,
}

impl RelocStats {
    /// Fold another run's counters into this one (per-table application —
    /// PPSSPP applies each reloc section independently, so HI16 pairing never
    /// crosses table boundaries).
    pub fn absorb(&mut self, other: RelocStats) {
        self.handled += other.handled;
        self.skipped_bad += other.skipped_bad;
        for (r_type, count) in other.unhandled {
            *self.unhandled.entry(r_type).or_insert(0) += count;
        }
    }
}

/// Parse Type-A relocation entries from raw 8-byte `[r_offset, r_info]` records.
///
/// Bit positions per PPSSPP/PRXTool/ghidra-allegrex (plan D10):
/// `type = r_info & 0xF`, `ofs_base = (r_info >> 8) & 0xFF`,
/// `addr_base = (r_info >> 16) & 0xFF`.
///
/// A length that is not a multiple of 8 means a truncated table: actionable
/// `Err`, never a panic.
pub fn parse_type_a_entries(raw: &[u8]) -> Result<Vec<RelocEntry>, ParseError> {
    if !raw.len().is_multiple_of(8) {
        return Err(ParseError::Reloc {
            message: format!(
                "Type-A relocation table length {} is not a multiple of 8 — \
                 truncated or corrupt table",
                raw.len()
            ),
        });
    }
    let mut cur = Cursor::new(raw);
    let mut entries = Vec::with_capacity(raw.len() / 8);
    while cur.position() < raw.len() as u64 {
        let offset = cur.read_u32::<LittleEndian>()?;
        let r_info = cur.read_u32::<LittleEndian>()?;
        entries.push(RelocEntry {
            offset,
            r_type: (r_info & 0xF) as u8,
            ofs_base: ((r_info >> 8) & 0xFF) as u8,
            addr_base: ((r_info >> 16) & 0xFF) as u8,
        });
    }
    Ok(entries)
}

/// Apply Type-A relocations to mutable segment data, PPSSPP-faithfully.
///
/// Two passes (spec §4.2): pass 1 caches every valid entry's original target
/// word — so duplicate relocations at one address never compound, and HI16
/// pairing reads the pre-relocation LO16 word; pass 2 computes each result
/// from the cached word and writes it. Malformed entries (out-of-bounds
/// segment index or target offset, misaligned non-R_MIPS_32 target) are
/// skipped and counted in [`RelocStats::skipped_bad`] — loud, never fatal,
/// matching PPSSPP's skip-don't-abort semantics.
///
/// `segments_data`: each segment's bytes (length == p_memsz), indexed by
/// segment number. `segment_bases`: the rebased virtual base address of each
/// segment (parallel to `segments_data`).
pub fn apply_relocations(
    segments_data: &mut [Vec<u8>],
    segment_bases: &[u32],
    entries: &[RelocEntry],
) -> Result<RelocStats, ParseError> {
    if segments_data.len() != segment_bases.len() {
        return Err(ParseError::Reloc {
            message: format!(
                "segment data/base count mismatch: {} data buffers vs {} bases",
                segments_data.len(),
                segment_bases.len()
            ),
        });
    }
    let mut stats = RelocStats::default();
    let reloc_ops = cache_original_words(segments_data, segment_bases, entries, &mut stats);
    let pass = Applier {
        bases: segment_bases,
        entries,
        reloc_ops: &reloc_ops,
    };
    for (i, cached) in reloc_ops.iter().enumerate() {
        if cached.is_some() {
            pass.apply_one(segments_data, i, &mut stats);
        }
    }
    for (&r_type, &count) in &stats.unhandled {
        if r_type == R_MIPS_HI16 {
            continue; // unpaired HI16s are warned individually with accurate wording
        }
        tracing::warn!(
            "Unhandled relocation type {r_type}: {count} entries left untouched \
             (target words unchanged — addresses in affected code/data are unrelocated)"
        );
    }
    Ok(stats)
}

/// Pass 1: cache the original target word of every valid entry.
///
/// Invalid entries get `None` and are counted in `stats.skipped_bad` with one
/// aggregate warning (PPSSPP `relocOps` semantics: pairing later reads these
/// cached words, and PPSSPP zero-fills slots for skipped entries).
fn cache_original_words(
    data: &[Vec<u8>],
    bases: &[u32],
    entries: &[RelocEntry],
    stats: &mut RelocStats,
) -> Vec<Option<u32>> {
    let (mut oob_seg, mut oob_off, mut misaligned) = (0u64, 0u64, 0u64);
    let mut ops = Vec::with_capacity(entries.len());
    for e in entries {
        let seg = e.ofs_base as usize;
        if seg >= data.len() {
            oob_seg += 1;
            ops.push(None);
            continue;
        }
        let off = e.offset as usize;
        // Reloc into bss is legal (buffers are memsz-sized); beyond memsz is bad.
        if off.saturating_add(4) > data[seg].len() {
            oob_off += 1;
            ops.push(None);
            continue;
        }
        let addr = bases[seg].wrapping_add(e.offset);
        if addr & 3 != 0 && e.r_type != R_MIPS_32 {
            misaligned += 1;
            ops.push(None);
            continue;
        }
        ops.push(Some(u32::from_le_bytes(
            data[seg][off..off + 4].try_into().unwrap(),
        )));
    }
    stats.skipped_bad = oob_seg + oob_off + misaligned;
    if stats.skipped_bad > 0 {
        tracing::warn!(
            "Skipped {} malformed relocation entries ({oob_seg} bad segment index, \
             {oob_off} target offset beyond segment, {misaligned} misaligned non-R_MIPS_32)",
            stats.skipped_bad
        );
    }
    ops
}

/// Pass-2 application state: immutable per-run lookup tables.
struct Applier<'a> {
    bases: &'a [u32],
    entries: &'a [RelocEntry],
    reloc_ops: &'a [Option<u32>],
}

impl Applier<'_> {
    /// Apply entry `i` (validated by pass 1) against its original cached word.
    fn apply_one(&self, data: &mut [Vec<u8>], i: usize, stats: &mut RelocStats) {
        let e = &self.entries[i];
        let op = self.reloc_ops[i].expect("pass 1 validated this entry");
        // PPSSPP: an out-of-range ADDR_BASE still applies, with relocate_to = 0.
        let relocate_to = self.bases.get(e.addr_base as usize).copied().unwrap_or(0);
        let new_word = match e.r_type {
            R_MIPS_NONE => {
                stats.handled += 1;
                return;
            }
            R_MIPS_16 | R_MIPS_LO16 => {
                (op & 0xFFFF_0000) | ((op & 0xFFFF).wrapping_add(relocate_to) & 0xFFFF)
            }
            R_MIPS_32 => op.wrapping_add(relocate_to),
            R_MIPS_26 => {
                (op & 0xFC00_0000)
                    | ((op & 0x03FF_FFFF).wrapping_add(relocate_to >> 2) & 0x03FF_FFFF)
            }
            R_MIPS_HI16 => {
                self.apply_hi16(data, i, relocate_to, stats);
                return;
            }
            other => {
                *stats.unhandled.entry(other).or_insert(0) += 1;
                return;
            }
        };
        write_word(data, e, new_word);
        stats.handled += 1;
    }

    /// HI16: forward-scan for the LO16 (or R_MIPS_16) partner, spec §4.4.
    ///
    /// Both outcomes write the byte result PPSSPP/ghidra-allegrex would write.
    /// An orphan HI16 (no partner) writes hi = 0 exactly like PPSSPP — byte
    /// compatibility with the reference engines — but is ALSO counted in
    /// `unhandled` and warned loudly, because a zeroed `lui` silently corrupts
    /// addresses (plan D11, amended for byte compatibility).
    fn apply_hi16(&self, data: &mut [Vec<u8>], r: usize, relocate_to: u32, stats: &mut RelocStats) {
        let e = &self.entries[r];
        let op_hi = self.reloc_ops[r].expect("pass 1 validated this entry");
        match self.find_lo16_partner(r) {
            Some(t) => {
                // Original (pre-relocation) partner word, low half sign-extended.
                let lo = i64::from(sign_extend_16(self.reloc_ops[t].unwrap_or(0)));
                let cur = ((i64::from(op_hi & 0xFFFF) << 16) + lo + i64::from(relocate_to)) as u32;
                // Split with sign carry: hi = (cur - (s16)(cur & 0xFFFF)) >> 16.
                let lo16 = sign_extend_16(cur) as u32;
                let hi16 = cur.wrapping_sub(lo16) >> 16;
                write_word(data, e, (op_hi & 0xFFFF_0000) | (hi16 & 0xFFFF));
                stats.handled += 1;
            }
            None => {
                write_word(data, e, op_hi & 0xFFFF_0000);
                *stats.unhandled.entry(R_MIPS_HI16).or_insert(0) += 1;
                tracing::warn!(
                    "Unpaired R_MIPS_HI16 at segment {} offset 0x{:08X}: no LO16 partner; \
                     wrote hi = 0 (PPSSPP-compatible) — the lui immediate is now zeroed and \
                     addresses built from it are corrupt",
                    e.ofs_base,
                    e.offset
                );
            }
        }
    }

    /// Find the partner entry index for the HI16 at `r` (forward scan).
    ///
    /// Consecutive HI16s all pair with the same later LO16 (each is patched
    /// separately); interleaved unrelated types are scanned past (real games:
    /// AC Bloodlines, GTA:VCS); `R_MIPS_16` is accepted as a partner
    /// (MotorStorm: Arctic Edge). Mismatched segment fields only warn.
    fn find_lo16_partner(&self, r: usize) -> Option<usize> {
        let e_r = &self.entries[r];
        for (t, e_t) in self.entries.iter().enumerate().skip(r + 1) {
            match e_t.r_type {
                R_MIPS_HI16 => continue,
                R_MIPS_LO16 => {}
                R_MIPS_16 => {
                    tracing::warn!("R_MIPS_16 entry {t} accepted as HI16 partner for entry {r}");
                }
                other => {
                    tracing::debug!("HI16 pairing scan skipping r_type {other} at entry {t}");
                    continue;
                }
            }
            if (e_t.ofs_base, e_t.addr_base) != (e_r.ofs_base, e_r.addr_base) {
                tracing::warn!(
                    "HI16/LO16 pair segment fields differ (hi entry {r}: ofs {} addr {}; \
                     lo entry {t}: ofs {} addr {})",
                    e_r.ofs_base,
                    e_r.addr_base,
                    e_t.ofs_base,
                    e_t.addr_base
                );
            }
            return Some(t);
        }
        None
    }
}

/// Write `word` at the entry's pass-1-validated (segment, offset) target.
fn write_word(data: &mut [Vec<u8>], e: &RelocEntry, word: u32) {
    let off = e.offset as usize;
    data[e.ofs_base as usize][off..off + 4].copy_from_slice(&word.to_le_bytes());
}

/// Sign-extend the low 16 bits of a u32 to i32.
fn sign_extend_16(value: u32) -> i32 {
    ((value & 0xFFFF) as i16) as i32
}

#[cfg(test)]
mod tests {
    use super::*;

    fn entry(offset: u32, r_type: u8) -> RelocEntry {
        RelocEntry {
            offset,
            r_type,
            ofs_base: 0,
            addr_base: 0,
        }
    }

    fn seg_of_words(words: &[u32]) -> Vec<u8> {
        words.iter().flat_map(|w| w.to_le_bytes()).collect()
    }

    fn word_at(seg: &[u8], off: usize) -> u32 {
        u32::from_le_bytes(seg[off..off + 4].try_into().unwrap())
    }

    const BASE: u32 = 0x0880_4000;

    #[test]
    fn r_info_bit_positions_match_ppsspp() {
        // r_info = 0x00020105 => type 5, ofs_base 1, addr_base 2 (plan D10).
        let mut raw = 0x0000_0010u32.to_le_bytes().to_vec();
        raw.extend_from_slice(&0x0002_0105u32.to_le_bytes());
        let entries = parse_type_a_entries(&raw).unwrap();
        assert_eq!(entries.len(), 1);
        assert_eq!(entries[0].offset, 0x10);
        assert_eq!(entries[0].r_type, R_MIPS_HI16);
        assert_eq!(entries[0].ofs_base, 1);
        assert_eq!(entries[0].addr_base, 2);
    }

    #[test]
    fn truncated_table_errors_not_panics() {
        let err = parse_type_a_entries(&[0u8; 12]).unwrap_err();
        assert!(err.to_string().contains("not a multiple of 8"));
    }

    #[test]
    fn each_type_applies_exact_words() {
        // Words: [16, 32, 26, NONE, HI16, LO16] at offsets 0,4,8,12,16,20.
        let mut segs = vec![seg_of_words(&[
            0x1234_0010, // R_MIPS_16
            0x0000_0010, // R_MIPS_32
            0x0C00_0004, // R_MIPS_26 (jal 0x10)
            0xDEAD_BEEF, // R_MIPS_NONE
            0x3C04_0000, // R_MIPS_HI16 (lui a0, 0)
            0x2484_0010, // R_MIPS_LO16 (addiu a0, a0, 0x10)
        ])];
        let entries = vec![
            entry(0, R_MIPS_16),
            entry(4, R_MIPS_32),
            entry(8, R_MIPS_26),
            entry(12, R_MIPS_NONE),
            entry(16, R_MIPS_HI16),
            entry(20, R_MIPS_LO16),
        ];
        let stats = apply_relocations(&mut segs, &[BASE], &entries).unwrap();
        assert_eq!(stats.handled, 6);
        assert!(stats.unhandled.is_empty());
        assert_eq!(stats.skipped_bad, 0);
        assert_eq!(word_at(&segs[0], 0), 0x1234_4010); // (0x10 + base) & 0xFFFF
        assert_eq!(word_at(&segs[0], 4), 0x0880_4010); // 0x10 + base
        assert_eq!(word_at(&segs[0], 8), 0x0E20_1004); // field 4 + (base >> 2)
        assert_eq!(word_at(&segs[0], 12), 0xDEAD_BEEF); // NONE: untouched
        assert_eq!(word_at(&segs[0], 16), 0x3C04_0880); // hi of 0x08804010
        assert_eq!(word_at(&segs[0], 20), 0x2484_4010); // lo of 0x08804010
    }

    #[test]
    fn hi16_lo16_sign_carry() {
        // lo = 0x9000 (negative as s16): combined = 0 - 0x7000 + base = 0x087FD000.
        // lo16 = (s16)0xD000 = -0x3000 => hi = (0x087FD000 + 0x3000) >> 16 = 0x0880,
        // one more than the naive 0x087F.
        let mut segs = vec![seg_of_words(&[0x3C04_0000, 0x2484_9000])];
        let entries = vec![entry(0, R_MIPS_HI16), entry(4, R_MIPS_LO16)];
        let stats = apply_relocations(&mut segs, &[BASE], &entries).unwrap();
        assert_eq!(stats.handled, 2);
        assert_eq!(
            word_at(&segs[0], 0) & 0xFFFF,
            0x0880,
            "sign carry must bump hi"
        );
        assert_eq!(word_at(&segs[0], 4) & 0xFFFF, 0xD000);
    }

    #[test]
    fn consecutive_hi16s_pair_with_one_lo16() {
        // Two HI16s then one LO16: both his pair with it; LO16 also standalone.
        let mut segs = vec![seg_of_words(&[0x3C04_0000, 0x3C05_0001, 0x2484_0010])];
        let entries = vec![
            entry(0, R_MIPS_HI16),
            entry(4, R_MIPS_HI16),
            entry(8, R_MIPS_LO16),
        ];
        let stats = apply_relocations(&mut segs, &[BASE], &entries).unwrap();
        assert_eq!(stats.handled, 3);
        assert!(stats.unhandled.is_empty());
        assert_eq!(word_at(&segs[0], 0), 0x3C04_0880); // hi(0x00000010 + base)
        assert_eq!(word_at(&segs[0], 4), 0x3C05_0881); // hi(0x00010010 + base)
        assert_eq!(word_at(&segs[0], 8), 0x2484_4010); // lo standalone
    }

    #[test]
    fn pairing_scans_past_interleaved_types() {
        // HI16, R_MIPS_32, LO16: the HI16 must pair with the LO16 (entry 2).
        let mut segs = vec![seg_of_words(&[0x3C04_0000, 0x0000_0020, 0x2484_0010])];
        let entries = vec![
            entry(0, R_MIPS_HI16),
            entry(4, R_MIPS_32),
            entry(8, R_MIPS_LO16),
        ];
        let stats = apply_relocations(&mut segs, &[BASE], &entries).unwrap();
        assert_eq!(stats.handled, 3);
        assert_eq!(word_at(&segs[0], 0), 0x3C04_0880);
        assert_eq!(word_at(&segs[0], 4), 0x0880_4020);
        assert_eq!(word_at(&segs[0], 8), 0x2484_4010);
    }

    #[test]
    fn r_mips_16_accepted_as_hi16_partner() {
        let mut segs = vec![seg_of_words(&[0x3C04_0000, 0x1234_0010])];
        let entries = vec![entry(0, R_MIPS_HI16), entry(4, R_MIPS_16)];
        let stats = apply_relocations(&mut segs, &[BASE], &entries).unwrap();
        assert_eq!(stats.handled, 2);
        assert_eq!(word_at(&segs[0], 0), 0x3C04_0880);
        assert_eq!(word_at(&segs[0], 4), 0x1234_4010); // R_MIPS_16 also standalone
    }

    #[test]
    fn pairing_uses_original_lo_word_not_relocated() {
        // The LO16 word at offset 4 is relocated TWICE (duplicate entries), and a
        // HI16 pairs with the second occurrence. The pairing must read the
        // ORIGINAL word (imm 0x4000): cur = 0x4000 + base = 0x08808000 =>
        // hi = 0x0881 (carry). A naive read-after-write would see imm 0x8000
        // (already relocated) => cur = -0x8000 + base => hi = 0x0880. Duplicate
        // LO16s must also not compound (both write from the original word).
        let mut segs = vec![seg_of_words(&[0x3C04_0000, 0x2484_4000])];
        let entries = vec![
            entry(4, R_MIPS_LO16),
            entry(0, R_MIPS_HI16),
            entry(4, R_MIPS_LO16),
        ];
        let stats = apply_relocations(&mut segs, &[BASE], &entries).unwrap();
        assert_eq!(stats.handled, 3);
        assert_eq!(
            word_at(&segs[0], 0) & 0xFFFF,
            0x0881,
            "must use original lo word"
        );
        assert_eq!(
            word_at(&segs[0], 4) & 0xFFFF,
            0x8000,
            "duplicates must not compound"
        );
    }

    #[test]
    fn unpaired_hi16_writes_ppsspp_zero_hi_and_counts() {
        // Byte compatibility with PPSSPP/ghidra-allegrex: orphan HI16 writes
        // hi = 0 (low half zeroed), counted in unhandled, loud warning.
        let mut segs = vec![seg_of_words(&[0x3C04_1234])];
        let entries = vec![entry(0, R_MIPS_HI16)];
        let stats = apply_relocations(&mut segs, &[BASE], &entries).unwrap();
        assert_eq!(stats.handled, 0);
        assert_eq!(stats.unhandled.get(&R_MIPS_HI16), Some(&1));
        assert_eq!(word_at(&segs[0], 0), 0x3C04_0000, "PPSSPP writes hi = 0");
    }

    #[test]
    fn oob_offset_and_segment_are_skipped_not_fatal() {
        let mut segs = vec![seg_of_words(&[0x0000_0010])];
        let entries = vec![
            entry(4, R_MIPS_32), // beyond memsz (segment is 4 bytes)
            RelocEntry {
                offset: 0,
                r_type: R_MIPS_32,
                ofs_base: 7,
                addr_base: 0,
            },
            entry(0, R_MIPS_32), // valid
        ];
        let stats = apply_relocations(&mut segs, &[BASE], &entries).unwrap();
        assert_eq!(stats.skipped_bad, 2);
        assert_eq!(stats.handled, 1);
        assert_eq!(word_at(&segs[0], 0), 0x0880_4010);
    }

    #[test]
    fn oob_addr_base_applies_with_zero() {
        // PPSSPP: OOB ADDR_BASE still applies, relocate_to = 0 (word unchanged).
        let mut segs = vec![seg_of_words(&[0x0000_0010])];
        let entries = vec![RelocEntry {
            offset: 0,
            r_type: R_MIPS_32,
            ofs_base: 0,
            addr_base: 9,
        }];
        let stats = apply_relocations(&mut segs, &[BASE], &entries).unwrap();
        assert_eq!(stats.handled, 1);
        assert_eq!(stats.skipped_bad, 0);
        assert_eq!(word_at(&segs[0], 0), 0x0000_0010);
    }

    #[test]
    fn misaligned_non_r_mips_32_skipped_but_r_mips_32_applied() {
        let mut segs = vec![seg_of_words(&[0x0000_0010, 0x0000_0020])];
        // Original little-endian word at the misaligned offset 2 is 0x00200000.
        assert_eq!(word_at(&segs[0], 2), 0x0020_0000);
        let entries = vec![entry(2, R_MIPS_LO16), entry(2, R_MIPS_32)];
        let stats = apply_relocations(&mut segs, &[BASE], &entries).unwrap();
        assert_eq!(stats.skipped_bad, 1, "misaligned LO16 skipped");
        assert_eq!(stats.handled, 1, "misaligned R_MIPS_32 still applies");
        assert_eq!(word_at(&segs[0], 2), 0x0020_0000u32.wrapping_add(BASE));
    }

    #[test]
    fn mismatched_bases_and_data_is_an_error() {
        let mut segs = vec![seg_of_words(&[0])];
        let err = apply_relocations(&mut segs, &[BASE, 0], &[]).unwrap_err();
        assert!(err.to_string().contains("mismatch"));
    }

    #[test]
    fn unknown_r_type_is_counted_and_word_untouched() {
        // R_MIPS_GPREL16 (7) has no implementation — the silent path #37 surfaces.
        let mut segs = vec![0x1122_3344u32.to_le_bytes().to_vec()];
        let entries = vec![
            entry(0, R_MIPS_GPREL16),
            entry(0, R_MIPS_GPREL16),
            entry(0, 9),
        ];
        let stats = apply_relocations(&mut segs, &[0x0880_0000], &entries).unwrap();
        assert_eq!(stats.unhandled.get(&R_MIPS_GPREL16), Some(&2));
        assert_eq!(stats.unhandled.get(&9), Some(&1));
        assert_eq!(stats.handled, 0);
        let word = u32::from_le_bytes(segs[0][0..4].try_into().unwrap());
        assert_eq!(
            word, 0x1122_3344,
            "unhandled reloc must leave the word untouched"
        );
    }

    #[test]
    fn handled_types_are_applied_and_counted() {
        // R_MIPS_32 at offset 0, R_MIPS_NONE at offset 4.
        let mut segs = vec![{
            let mut v = 0x0000_0010u32.to_le_bytes().to_vec();
            v.extend_from_slice(&0xDEAD_BEEFu32.to_le_bytes());
            v
        }];
        let entries = vec![entry(0, R_MIPS_32), entry(4, R_MIPS_NONE)];
        let stats = apply_relocations(&mut segs, &[0x0880_0000], &entries).unwrap();
        assert_eq!(stats.handled, 2);
        assert!(stats.unhandled.is_empty());
        let word = u32::from_le_bytes(segs[0][0..4].try_into().unwrap());
        assert_eq!(word, 0x0880_0010, "R_MIPS_32 adds the segment base");
    }

    #[test]
    fn hi16_lo16_pair_counts_as_handled() {
        // hi: lui-style word, lo: addiu-style word; base 0x08800000.
        let mut segs = vec![{
            let mut v = 0x3C04_0000u32.to_le_bytes().to_vec(); // lui a0, 0x0000
            v.extend_from_slice(&0x2484_0010u32.to_le_bytes()); // addiu a0, a0, 0x10
            v
        }];
        let entries = vec![entry(0, R_MIPS_HI16), entry(4, R_MIPS_LO16)];
        let stats = apply_relocations(&mut segs, &[0x0880_0000], &entries).unwrap();
        assert_eq!(stats.handled, 2);
        assert!(stats.unhandled.is_empty());
        let hi = u32::from_le_bytes(segs[0][0..4].try_into().unwrap());
        let lo = u32::from_le_bytes(segs[0][4..8].try_into().unwrap());
        assert_eq!(hi & 0xFFFF, 0x0880);
        assert_eq!(lo & 0xFFFF, 0x0010);
    }

    #[test]
    fn handled_type_predicate_matches_implementation() {
        for t in HANDLED_RELOC_TYPES {
            assert!(is_handled_reloc_type(t));
        }
        assert!(!is_handled_reloc_type(R_MIPS_GPREL16));
        assert!(!is_handled_reloc_type(255));
    }
}
