//! `recompile_report.json` — structured diagnostics for every silent failure
//! path in the recompile pipeline (issue #37).
//!
//! Written to `<output>/recompile_report.json` at the end of every
//! `psprecomp recompile` run. Schema (documented in DEBUGGING.md, "#37
//! recompile report"):
//!
//! ```json
//! {
//!   "schema_version": 1,
//!   "generated_at": "2026-06-12T01:23:45Z",
//!   "counts": {
//!     "functions_total": 14104,
//!     "functions_by_source": {"ghidra": 9713, "binary_scan": 4360, ...},
//!     "discovery": {"force": 1, "raw_scan": N, "prologue": N,
//!                   "gap_start": N, "gap_rescued": N},
//!     "mid_entries": 2022,
//!     "batch_files": 283,
//!     "imports_total": 237,
//!     "unresolved_nids": 0,
//!     "decode_errors": 4,
//!     "relocations_total": 0,
//!     "unhandled_relocations": 0,
//!     "dedup_renames": 0,
//!     "dispatch_missing_targets": 0
//!   },
//!   "decode_errors": [{"address": "0x089DBCF4", "name": "...", "error": "..."}],
//!   "unresolved_nids": [{"nid": "0x...", "stub_addr": "0x...", "fallback_name": "NID_0x..."}],
//!   "unhandled_relocations": {"7": 123},
//!   "dispatch_audit": {"static_lookup_targets": N, "missing_targets": ["0x..."]},
//!   "dedup_renames": [{"address": "0x...", "original_name": "...", "unique_name": "..."}]
//! }
//! ```
//!
//! All addresses are hex strings ("0x%08X"), matching analysis.json style.
//! Keys of `unhandled_relocations` are decimal r_type values as strings.

use std::collections::{BTreeMap, HashMap, HashSet};
use std::path::Path;

use serde::Serialize;

use psp_parser::analysis_json::AnalysisJson;

/// Current report schema version. Bump when fields change shape or meaning.
pub const SCHEMA_VERSION: u32 = 1;

/// Top-level structure serialized to `recompile_report.json`.
#[derive(Debug, Serialize)]
pub struct RecompileReport {
    pub schema_version: u32,
    /// UTC timestamp, ISO-8601 with seconds precision (e.g. "2026-06-12T01:23:45Z").
    pub generated_at: String,
    pub counts: ReportCounts,
    /// Functions emitted as empty stubs because the decoder errored.
    pub decode_errors: Vec<DecodeErrorEntry>,
    /// Imports whose NID was missing from the database (fallback-named stubs).
    pub unresolved_nids: Vec<UnresolvedNidEntry>,
    /// Decimal r_type (string key) -> count of relocation entries with no
    /// implementation; recomputed from analysis.json's `relocations` array.
    pub unhandled_relocations: BTreeMap<String, u64>,
    pub dispatch_audit: DispatchAudit,
    /// Name collisions resolved with the `_ADDR` hex suffix (ODR safety).
    pub dedup_renames: Vec<DedupRenameEntry>,
}

/// Self-check counts; pair with `--expect-functions` / `--expect-mid-entries`.
#[derive(Debug, Serialize)]
pub struct ReportCounts {
    pub functions_total: usize,
    /// analysis.json `source` field -> count (ghidra, hle_scan, binary_scan, ...).
    pub functions_by_source: BTreeMap<String, usize>,
    /// Recompile-time discovery pass breakdown (all land as source=binary_scan).
    pub discovery: DiscoveryCounts,
    pub mid_entries: usize,
    pub batch_files: usize,
    pub imports_total: usize,
    pub unresolved_nids: usize,
    pub decode_errors: usize,
    pub relocations_total: usize,
    pub unhandled_relocations: u64,
    pub dedup_renames: usize,
    pub dispatch_missing_targets: usize,
}

/// Per-pass breakdown of `enhance_function_discovery` (recompile stage).
#[derive(Debug, Default, Clone, Copy, Serialize)]
pub struct DiscoveryCounts {
    pub force: usize,
    pub raw_scan: usize,
    pub prologue: usize,
    pub gap_start: usize,
    pub gap_rescued: usize,
}

/// A function stubbed out because `psp_decoder::decode_function` failed.
#[derive(Debug, Serialize)]
pub struct DecodeErrorEntry {
    /// Hex string, e.g. "0x089DBCF4"
    pub address: String,
    pub name: String,
    pub error: String,
}

/// An import whose NID was not in the database (silently fallback-named).
#[derive(Debug, Serialize)]
pub struct UnresolvedNidEntry {
    /// Hex string, e.g. "0xD632ACDB"
    pub nid: String,
    /// Hex string
    pub stub_addr: String,
    /// The "NID_0x%08X" name the pipeline used in place of a real symbol.
    pub fallback_name: String,
}

/// Static dispatch-coverage audit: every statically-emitted RECOMP_LOOKUP
/// target must exist in the dispatch table or it is a guaranteed LOOKUP_MISS.
#[derive(Debug, Serialize)]
pub struct DispatchAudit {
    /// Total distinct statically-known lookup targets the emitter generated.
    pub static_lookup_targets: usize,
    /// Targets absent from the dispatch table (hex strings, sorted).
    pub missing_targets: Vec<String>,
}

/// A function renamed by `dedup_function_names` for ODR safety.
#[derive(Debug, Serialize)]
pub struct DedupRenameEntry {
    /// Hex string
    pub address: String,
    pub original_name: String,
    pub unique_name: String,
}

/// Everything `build_report` needs, bundled to keep the signature small.
pub struct ReportInputs<'a> {
    pub analysis: &'a AnalysisJson,
    pub discovery: DiscoveryCounts,
    pub batch_file_count: usize,
    /// Collected during batch emission (one entry per stubbed function).
    pub decode_errors: Vec<DecodeErrorEntry>,
    /// Union of statically-emitted RECOMP_LOOKUP targets (incl. constructors).
    pub static_lookup_targets: HashSet<u32>,
    /// addr -> unique C++ name map from `dedup_function_names`.
    pub unique_names: &'a HashMap<u32, String>,
}

/// Assemble the full report from the recompile pipeline's collected state.
pub fn build_report(inputs: ReportInputs) -> RecompileReport {
    let analysis = inputs.analysis;

    let mut functions_by_source: BTreeMap<String, usize> = BTreeMap::new();
    for f in &analysis.functions {
        *functions_by_source.entry(f.source.clone()).or_insert(0) += 1;
    }

    let unresolved_nids = collect_unresolved_nids(analysis);
    let unhandled_relocations = collect_unhandled_relocations(analysis);
    let dispatch_audit = audit_dispatch_targets(analysis, &inputs.static_lookup_targets);
    let dedup_renames = collect_dedup_renames(analysis, inputs.unique_names);

    let mut decode_errors = inputs.decode_errors;
    decode_errors.sort_by(|a, b| a.address.cmp(&b.address));

    let counts = ReportCounts {
        functions_total: analysis.functions.len(),
        functions_by_source,
        discovery: inputs.discovery,
        mid_entries: analysis.mid_entries.len(),
        batch_files: inputs.batch_file_count,
        imports_total: analysis.imports.len(),
        unresolved_nids: unresolved_nids.len(),
        decode_errors: decode_errors.len(),
        relocations_total: analysis.relocations.len(),
        unhandled_relocations: unhandled_relocations.values().sum(),
        dedup_renames: dedup_renames.len(),
        dispatch_missing_targets: dispatch_audit.missing_targets.len(),
    };

    warn_on_silent_paths(&counts, &unhandled_relocations, &dispatch_audit);

    RecompileReport {
        schema_version: SCHEMA_VERSION,
        generated_at: iso8601_utc_now(),
        counts,
        decode_errors,
        unresolved_nids,
        unhandled_relocations,
        dispatch_audit,
        dedup_renames,
    }
}

/// Imports whose name is the NID fallback pattern (database miss at analyze time).
fn collect_unresolved_nids(analysis: &AnalysisJson) -> Vec<UnresolvedNidEntry> {
    analysis
        .imports
        .iter()
        .filter(|imp| psp_parser::nid::parse_fallback_name(&imp.name).is_some())
        .map(|imp| UnresolvedNidEntry {
            nid: imp.nid.clone(),
            stub_addr: imp.stub_addr.clone(),
            fallback_name: imp.name.clone(),
        })
        .collect()
}

/// Count relocation entries whose r_type has no implementation.
///
/// Relocations are applied at analyze time, but every entry (with its r_type)
/// is persisted in analysis.json's `relocations` array — so the counts are
/// recomputed here, keeping the report derivable from analysis.json alone.
fn collect_unhandled_relocations(analysis: &AnalysisJson) -> BTreeMap<String, u64> {
    let mut map: BTreeMap<String, u64> = BTreeMap::new();
    for rel in &analysis.relocations {
        if !psp_parser::reloc::is_handled_reloc_type(rel.r_type) {
            *map.entry(rel.r_type.to_string()).or_insert(0) += 1;
        }
    }
    map
}

/// Diff statically-emitted lookup targets against the dispatch table's
/// address set (function entries + mid-entries + the 0x0 sentinel + the
/// generated-table import stubs, which the runtime registers at
/// `psp_hle_init()` before any recompiled code runs — issue #40).
fn audit_dispatch_targets(
    analysis: &AnalysisJson,
    targets: &HashSet<u32>,
) -> DispatchAudit {
    let mut dispatch_addrs: HashSet<u32> = HashSet::with_capacity(
        analysis.functions.len() + analysis.mid_entries.len() + analysis.imports.len() + 1,
    );
    dispatch_addrs.insert(0);
    dispatch_addrs.extend(analysis.functions.iter().filter_map(|f| parse_hex(&f.address)));
    dispatch_addrs.extend(analysis.mid_entries.iter().filter_map(|me| parse_hex(&me.addr)));
    dispatch_addrs.extend(analysis.imports.iter().filter_map(|imp| parse_hex(&imp.stub_addr)));

    let mut missing: Vec<u32> = targets.difference(&dispatch_addrs).copied().collect();
    missing.sort_unstable();
    DispatchAudit {
        static_lookup_targets: targets.len(),
        missing_targets: missing.iter().map(|a| format!("0x{a:08X}")).collect(),
    }
}

/// Functions whose emitted C++ name differs from their analysis.json name.
fn collect_dedup_renames(
    analysis: &AnalysisJson,
    unique_names: &HashMap<u32, String>,
) -> Vec<DedupRenameEntry> {
    let mut renames: Vec<DedupRenameEntry> = analysis
        .functions
        .iter()
        .filter_map(|f| {
            let addr = parse_hex(&f.address)?;
            let unique = unique_names.get(&addr)?;
            if *unique != f.name {
                Some(DedupRenameEntry {
                    address: f.address.clone(),
                    original_name: f.name.clone(),
                    unique_name: unique.clone(),
                })
            } else {
                None
            }
        })
        .collect();
    renames.sort_by(|a, b| a.address.cmp(&b.address));
    renames
}

/// One aggregate warn per silent-path category that found anything.
fn warn_on_silent_paths(
    counts: &ReportCounts,
    unhandled_relocs: &BTreeMap<String, u64>,
    dispatch_audit: &DispatchAudit,
) {
    if counts.unresolved_nids > 0 {
        tracing::warn!(
            "{} imported NIDs missing from the database (fallback-named stubs); \
             see unresolved_nids in recompile_report.json",
            counts.unresolved_nids,
        );
    }
    for (r_type, count) in unhandled_relocs {
        tracing::warn!(
            "Unhandled relocation type {r_type}: {count} entries were left \
             unrelocated at analyze time"
        );
    }
    if !dispatch_audit.missing_targets.is_empty() {
        tracing::warn!(
            "{} statically-emitted RECOMP_LOOKUP targets are NOT in the dispatch \
             table — guaranteed LOOKUP_MISS if reached; see dispatch_audit in \
             recompile_report.json",
            dispatch_audit.missing_targets.len(),
        );
    }
}

/// One-paragraph human summary printed at the end of recompile.
pub fn human_summary(report: &RecompileReport, report_path: &Path) -> String {
    let c = &report.counts;
    format!(
        "Recompiled {} functions ({} mid-entries, {} batch files). \
         Silent-path audit: {} decode errors, {} unresolved NIDs, \
         {} unhandled relocations, {} dispatch targets missing, \
         {} dedup renames. Report: {}",
        c.functions_total,
        c.mid_entries,
        c.batch_files,
        c.decode_errors,
        c.unresolved_nids,
        c.unhandled_relocations,
        c.dispatch_missing_targets,
        c.dedup_renames,
        report_path.display(),
    )
}

/// Fail with a clear error if an `--expect-*` flag does not match reality.
pub fn check_expectation(kind: &str, actual: usize, expected: Option<usize>) -> anyhow::Result<()> {
    match expected {
        Some(exp) if exp != actual => anyhow::bail!(
            "--expect-{kind} mismatch: expected {exp}, got {actual} \
             (see recompile_report.json counts for the breakdown)"
        ),
        _ => Ok(()),
    }
}

/// Current UTC time as ISO-8601 ("YYYY-MM-DDTHH:MM:SSZ"), no external deps.
/// Shared with the build fingerprint (issue #36).
pub fn iso8601_utc_now() -> String {
    let secs = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0);
    format_iso8601(secs)
}

/// Format unix seconds as ISO-8601 UTC (Howard Hinnant's civil_from_days).
fn format_iso8601(unix_secs: u64) -> String {
    let days = (unix_secs / 86_400) as i64;
    let rem = unix_secs % 86_400;
    let (h, m, s) = (rem / 3600, (rem % 3600) / 60, rem % 60);

    let z = days + 719_468;
    let era = z.div_euclid(146_097);
    let doe = z.rem_euclid(146_097);
    let yoe = (doe - doe / 1460 + doe / 36_524 - doe / 146_096) / 365;
    let y = yoe + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = doy - (153 * mp + 2) / 5 + 1;
    let month = if mp < 10 { mp + 3 } else { mp - 9 };
    let year = if month <= 2 { y + 1 } else { y };

    format!("{year:04}-{month:02}-{d:02}T{h:02}:{m:02}:{s:02}Z")
}

#[cfg(test)]
mod tests {
    use super::*;
    use psp_parser::analysis_json::{
        JsonFunction, JsonImport, JsonMidEntry, JsonReloc,
    };

    fn func(addr: u32, name: &str) -> JsonFunction {
        JsonFunction {
            name: name.into(),
            address: format!("0x{addr:08X}"),
            size: 16,
            is_external: false,
            is_thunk: false,
            source: "ghidra".into(),
        }
    }

    fn minimal_analysis() -> AnalysisJson {
        AnalysisJson {
            binary_path: "BOOT.BIN".into(),
            module_name: "test".into(),
            heap_base: "0x08AE0000".into(),
            module: None,
            prx: None,
            functions: vec![func(0x0880_4000, "FUN_08804000")],
            imports: vec![],
            relocations: vec![],
            xrefs: vec![],
            constructors: vec![],
            mid_entries: vec![],
            segments: vec![],
        }
    }

    fn inputs<'a>(
        analysis: &'a AnalysisJson,
        unique_names: &'a HashMap<u32, String>,
    ) -> ReportInputs<'a> {
        ReportInputs {
            analysis,
            discovery: DiscoveryCounts::default(),
            batch_file_count: 1,
            decode_errors: vec![],
            static_lookup_targets: HashSet::new(),
            unique_names,
        }
    }

    #[test]
    fn report_serializes_with_documented_schema() {
        let analysis = minimal_analysis();
        let names = HashMap::new();
        let report = build_report(inputs(&analysis, &names));
        let value = serde_json::to_value(&report).unwrap();
        let obj = value.as_object().unwrap();
        let expected_keys = [
            "schema_version",
            "generated_at",
            "counts",
            "decode_errors",
            "unresolved_nids",
            "unhandled_relocations",
            "dispatch_audit",
            "dedup_renames",
        ];
        for key in expected_keys {
            assert!(obj.contains_key(key), "missing top-level key: {key}");
        }
        assert_eq!(obj.len(), expected_keys.len(), "undocumented top-level key present");
        assert_eq!(value["schema_version"], SCHEMA_VERSION);
        let counts = value["counts"].as_object().unwrap();
        for key in [
            "functions_total", "functions_by_source", "discovery", "mid_entries",
            "batch_files", "imports_total", "unresolved_nids", "decode_errors",
            "relocations_total", "unhandled_relocations", "dedup_renames",
            "dispatch_missing_targets",
        ] {
            assert!(counts.contains_key(key), "missing counts key: {key}");
        }
        assert_eq!(
            value["dispatch_audit"]["missing_targets"],
            serde_json::json!([])
        );
    }

    #[test]
    fn unresolved_nid_import_is_listed_by_name() {
        let mut analysis = minimal_analysis();
        analysis.imports = vec![
            JsonImport {
                nid: "0xDEADBEEF".into(),
                stub_addr: "0x08A00000".into(),
                name: "NID_0xDEADBEEF".into(), // database miss fallback
                module_name: "sceMpeg".into(),
            },
            JsonImport {
                nid: "0x12345678".into(),
                stub_addr: "0x08A00008".into(),
                name: "sceKernelCreateThread".into(), // resolved
                module_name: "ThreadManForUser".into(),
            },
        ];
        let names = HashMap::new();
        let report = build_report(inputs(&analysis, &names));
        assert_eq!(report.counts.unresolved_nids, 1);
        assert_eq!(report.unresolved_nids[0].nid, "0xDEADBEEF");
        assert_eq!(report.unresolved_nids[0].stub_addr, "0x08A00000");
        assert_eq!(report.unresolved_nids[0].fallback_name, "NID_0xDEADBEEF");
    }

    #[test]
    fn unhandled_relocations_counted_per_type() {
        let mut analysis = minimal_analysis();
        let rel = |r_type: u8| JsonReloc {
            offset: "0x00000000".into(),
            r_type,
            ofs_base: 0,
            addr_base: 0,
        };
        analysis.relocations = vec![rel(7), rel(7), rel(9), rel(2)]; // 2 is R_MIPS_32
        let names = HashMap::new();
        let report = build_report(inputs(&analysis, &names));
        assert_eq!(report.unhandled_relocations.get("7"), Some(&2));
        assert_eq!(report.unhandled_relocations.get("9"), Some(&1));
        assert_eq!(report.unhandled_relocations.get("2"), None);
        assert_eq!(report.counts.unhandled_relocations, 3);
        assert_eq!(report.counts.relocations_total, 4);
    }

    #[test]
    fn dispatch_audit_reports_targets_missing_from_table() {
        let mut analysis = minimal_analysis();
        analysis.mid_entries = vec![JsonMidEntry {
            addr: "0x08804008".into(),
            parent_addr: "0x08804000".into(),
        }];
        let names = HashMap::new();
        let mut input = inputs(&analysis, &names);
        // 0x08804000 = function entry, 0x08804008 = mid-entry, 0x08900000 = MISS.
        input.static_lookup_targets =
            [0x0880_4000u32, 0x0880_4008, 0x0890_0000].into_iter().collect();
        let report = build_report(input);
        assert_eq!(report.dispatch_audit.static_lookup_targets, 3);
        assert_eq!(report.dispatch_audit.missing_targets, vec!["0x08900000"]);
        assert_eq!(report.counts.dispatch_missing_targets, 1);
    }

    #[test]
    fn dedup_renames_lists_only_renamed_functions() {
        let mut analysis = minimal_analysis();
        analysis.functions = vec![
            func(0x0880_4000, "thunk_FUN_x"),
            func(0x0880_5000, "thunk_FUN_x"),
            func(0x0880_6000, "FUN_08806000"),
        ];
        let mut names = HashMap::new();
        names.insert(0x0880_4000u32, "thunk_FUN_x_08804000".to_string());
        names.insert(0x0880_5000u32, "thunk_FUN_x_08805000".to_string());
        names.insert(0x0880_6000u32, "FUN_08806000".to_string());
        let report = build_report(inputs(&analysis, &names));
        assert_eq!(report.counts.dedup_renames, 2);
        assert_eq!(report.dedup_renames[0].address, "0x08804000");
        assert_eq!(report.dedup_renames[0].unique_name, "thunk_FUN_x_08804000");
        assert_eq!(report.dedup_renames[1].address, "0x08805000");
    }

    #[test]
    fn decode_errors_are_sorted_and_counted() {
        let analysis = minimal_analysis();
        let names = HashMap::new();
        let mut input = inputs(&analysis, &names);
        input.decode_errors = vec![
            DecodeErrorEntry {
                address: "0x089DBCFC".into(),
                name: "FUN_089DBCFC".into(),
                error: "unknown instruction".into(),
            },
            DecodeErrorEntry {
                address: "0x089DBCF4".into(),
                name: "FUN_089DBCF4".into(),
                error: "unknown instruction".into(),
            },
        ];
        let report = build_report(input);
        assert_eq!(report.counts.decode_errors, 2);
        assert_eq!(report.decode_errors[0].address, "0x089DBCF4");
        assert_eq!(report.decode_errors[1].address, "0x089DBCFC");
    }

    #[test]
    fn expectation_mismatch_is_an_error() {
        assert!(check_expectation("functions", 14104, Some(14104)).is_ok());
        assert!(check_expectation("functions", 14104, None).is_ok());
        let err = check_expectation("functions", 14000, Some(14104)).unwrap_err();
        let msg = err.to_string();
        assert!(msg.contains("--expect-functions"), "got: {msg}");
        assert!(msg.contains("14104") && msg.contains("14000"), "got: {msg}");
    }

    #[test]
    fn iso8601_formats_known_epoch() {
        assert_eq!(format_iso8601(0), "1970-01-01T00:00:00Z");
        assert_eq!(format_iso8601(951_868_800), "2000-03-01T00:00:00Z");
        assert_eq!(format_iso8601(1_781_136_000), "2026-06-11T00:00:00Z");
    }
}

/// Parse a hex string (with or without "0x" prefix) into u32.
fn parse_hex(s: &str) -> Option<u32> {
    let trimmed = s.trim_start_matches("0x").trim_start_matches("0X");
    u64::from_str_radix(trimmed, 16).ok().map(|v| v as u32)
}
