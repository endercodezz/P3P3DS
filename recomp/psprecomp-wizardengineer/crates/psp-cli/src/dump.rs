//! `dump <analysis.json> 0xADDR` — single-function C++ emission (issue #38).
//!
//! Resolves the address against the post-pipeline function list and prints
//! that one function's generated C++ to stdout. The emission goes through the
//! exact same path the batch emitter uses (`recompile::prepare_emission` +
//! `recompile::emit_one_function`: same IR, same global name dedup, same
//! mid-entry handling, same PSPRECOMP_CROSS_MID gating), so the output is
//! byte-identical to the function's text in its batch_*.cpp file for the same
//! flag/env set. Read-only: nothing is written under output/. Only the C++
//! goes to stdout; all diagnostics go to stderr (tracing) so it pipes cleanly.

use std::path::Path;

use psp_parser::analysis_json::{AnalysisJson, JsonFunction};

use crate::recompile::{emit_one_function, parse_hex_u32, prepare_emission, PreparedEmission};

/// Where an address landed relative to the (post-pipeline) function list.
enum Resolution<'a> {
    /// Exactly a function entry — the only emittable case.
    Entry(&'a JsonFunction),
    /// A registered mid-entry inside `parent` (entered via the parent's
    /// prologue switch; it has no standalone C++ body to dump).
    MidEntry { parent_addr: u32, parent: Option<&'a JsonFunction> },
    /// Strictly inside a function body (not its entry, not a mid-entry).
    Inside(&'a JsonFunction),
    /// In no function at all; nearest entries below/above for the error.
    NotFound { below: Option<&'a JsonFunction>, above: Option<&'a JsonFunction> },
}

/// CLI entry: emit one function's C++ to stdout, errors/diagnostics to stderr.
pub fn run_dump_function(
    analysis_path: &Path,
    addr_arg: &str,
    config_path: Option<&Path>,
) -> anyhow::Result<()> {
    let addr = parse_dump_address(addr_arg)?;
    let prep = prepare_emission(analysis_path, config_path)?;
    let cpp = emit_function_cpp(&prep, addr)?;
    // write_all instead of print!: a downstream consumer exiting early (e.g.
    // `| head`) must end the dump quietly, not panic on EPIPE.
    use std::io::Write;
    match std::io::stdout().lock().write_all(cpp.as_bytes()) {
        Err(e) if e.kind() == std::io::ErrorKind::BrokenPipe => Ok(()),
        other => Ok(other?),
    }
}

/// Parse the address argument: hex, case-insensitive, `0x` prefix optional.
fn parse_dump_address(arg: &str) -> anyhow::Result<u32> {
    let trimmed = arg.trim();
    let digits = trimmed
        .strip_prefix("0x")
        .or_else(|| trimmed.strip_prefix("0X"))
        .unwrap_or(trimmed);
    if digits.is_empty() || !digits.chars().all(|c| c.is_ascii_hexdigit()) {
        anyhow::bail!(
            "'{arg}' is not a hex function address (expected e.g. 0x0881E7A8, \
             0x prefix optional) and not a list mode (functions | imports | \
             relocations | segments | mid_entries)"
        );
    }
    u32::from_str_radix(digits, 16)
        .map_err(|_| anyhow::anyhow!("'{arg}' does not fit in a 32-bit address"))
}

/// Resolve `addr` and emit its function, or return an actionable error.
fn emit_function_cpp(prep: &PreparedEmission, addr: u32) -> anyhow::Result<String> {
    match resolve_address(&prep.analysis, addr) {
        Resolution::Entry(func) => Ok(emit_one_function(prep, func).0),
        other => Err(anyhow::anyhow!(resolution_error(addr, &other))),
    }
}

/// Locate `addr` in the post-pipeline function/mid-entry lists.
fn resolve_address(analysis: &AnalysisJson, addr: u32) -> Resolution<'_> {
    let func_at = |a: u32| {
        analysis.functions.iter().find(|f| parse_hex_u32(&f.address) == Some(a))
    };
    if let Some(func) = func_at(addr) {
        return Resolution::Entry(func);
    }
    if let Some(me) = analysis
        .mid_entries
        .iter()
        .find(|me| parse_hex_u32(&me.addr) == Some(addr))
    {
        let parent_addr = parse_hex_u32(&me.parent_addr).unwrap_or(0);
        return Resolution::MidEntry { parent_addr, parent: func_at(parent_addr) };
    }

    let mut below: Option<(u32, &JsonFunction)> = None;
    let mut above: Option<(u32, &JsonFunction)> = None;
    for f in &analysis.functions {
        let Some(start) = parse_hex_u32(&f.address) else { continue };
        if start < addr && addr < start.saturating_add(f.size as u32) {
            return Resolution::Inside(f);
        }
        if start < addr && below.is_none_or(|(b, _)| start > b) {
            below = Some((start, f));
        }
        if start > addr && above.is_none_or(|(a, _)| start < a) {
            above = Some((start, f));
        }
    }
    Resolution::NotFound {
        below: below.map(|(_, f)| f),
        above: above.map(|(_, f)| f),
    }
}

/// Build the actionable error message for a non-entry resolution.
fn resolution_error(addr: u32, resolution: &Resolution<'_>) -> String {
    let describe = |f: &JsonFunction| format!("{} (entry {})", f.name, f.address);
    match resolution {
        Resolution::Entry(_) => unreachable!("Entry is emitted, not an error"),
        Resolution::MidEntry { parent_addr, parent } => {
            let parent_desc = parent.map(describe).unwrap_or_else(|| {
                format!("<unknown parent> (entry 0x{parent_addr:08X})")
            });
            format!(
                "0x{addr:08X} is a mid-entry inside {parent_desc}; mid-entries \
                 are emitted as cases of the parent's dispatch switch — dump \
                 the parent: psprecomp dump <analysis.json> 0x{parent_addr:08X}"
            )
        }
        Resolution::Inside(f) => format!(
            "0x{addr:08X} falls inside {} but is not its entry or a registered \
             mid-entry; dump the containing function: psprecomp dump \
             <analysis.json> {}",
            describe(f),
            f.address,
        ),
        Resolution::NotFound { below, above } => {
            let fmt = |opt: &Option<&JsonFunction>| {
                opt.map(describe).unwrap_or_else(|| "<none>".into())
            };
            format!(
                "no function at 0x{addr:08X} (not an entry, mid-entry, or \
                 inside any function). Nearest below: {}; nearest above: {}",
                fmt(below),
                fmt(above),
            )
        }
    }
}

#[cfg(test)]
mod tests {
    use std::sync::OnceLock;

    use psp_parser::analysis_json::JsonMidEntry;

    use super::*;

    // ---------------------------------------------------------------
    // Synthetic-analysis unit tests (parse + resolution + messages)
    // ---------------------------------------------------------------

    fn func(name: &str, addr: u32, size: u64) -> JsonFunction {
        JsonFunction {
            name: name.into(),
            address: format!("0x{addr:08X}"),
            size,
            is_external: false,
            is_thunk: false,
            source: "ghidra".into(),
        }
    }

    fn analysis() -> AnalysisJson {
        AnalysisJson {
            binary_path: String::new(),
            module_name: "test".into(),
            heap_base: "0x08AE0000".into(),
            module: None,
            prx: None,
            functions: vec![
                func("FUN_08804000", 0x08804000, 0x100),
                func("FUN_08804200", 0x08804200, 0x80),
            ],
            imports: vec![],
            relocations: vec![],
            xrefs: vec![],
            constructors: vec![],
            mid_entries: vec![JsonMidEntry {
                addr: "0x08804040".into(),
                parent_addr: "0x08804000".into(),
            }],
            segments: vec![],
        }
    }

    #[test]
    fn parse_accepts_hex_with_and_without_prefix_any_case() {
        assert_eq!(parse_dump_address("0x0881E7A8").unwrap(), 0x0881E7A8);
        assert_eq!(parse_dump_address("0X0881e7a8").unwrap(), 0x0881E7A8);
        assert_eq!(parse_dump_address("0881e7A8").unwrap(), 0x0881E7A8);
        assert_eq!(parse_dump_address(" 0x10 ").unwrap(), 0x10);
    }

    #[test]
    fn parse_rejects_bad_hex_with_usage_error() {
        for bad in ["", "0x", "zzz", "FUN_08804000", "0xG1", "function"] {
            let err = parse_dump_address(bad).unwrap_err().to_string();
            assert!(err.contains("not a hex function address"), "{bad}: {err}");
            assert!(err.contains("functions | imports"), "{bad}: {err}");
        }
        let err = parse_dump_address("0x100000000").unwrap_err().to_string();
        assert!(err.contains("32-bit"), "{err}");
    }

    #[test]
    fn resolves_exact_entry() {
        let a = analysis();
        match resolve_address(&a, 0x08804000) {
            Resolution::Entry(f) => assert_eq!(f.name, "FUN_08804000"),
            _ => panic!("expected Entry"),
        }
    }

    #[test]
    fn mid_entry_error_names_parent_and_its_entry() {
        let a = analysis();
        let res = resolve_address(&a, 0x08804040);
        assert!(matches!(res, Resolution::MidEntry { .. }));
        let msg = resolution_error(0x08804040, &res);
        assert!(msg.contains("mid-entry"), "{msg}");
        assert!(msg.contains("FUN_08804000"), "{msg}");
        assert!(msg.contains("0x08804000"), "{msg}");
    }

    #[test]
    fn inside_error_names_containing_function() {
        let a = analysis();
        let res = resolve_address(&a, 0x08804044); // inside, not a mid-entry
        assert!(matches!(res, Resolution::Inside(_)));
        let msg = resolution_error(0x08804044, &res);
        assert!(msg.contains("falls inside FUN_08804000"), "{msg}");
        assert!(msg.contains("0x08804000"), "{msg}");
    }

    #[test]
    fn not_found_error_lists_nearest_neighbors() {
        let a = analysis();
        // Gap between the two functions: 0x08804100..0x08804200.
        let res = resolve_address(&a, 0x08804180);
        let msg = resolution_error(0x08804180, &res);
        assert!(msg.contains("no function at 0x08804180"), "{msg}");
        assert!(msg.contains("FUN_08804000"), "{msg}");
        assert!(msg.contains("FUN_08804200"), "{msg}");
        // Below everything: no "nearest below".
        let res = resolve_address(&a, 0x10);
        let msg = resolution_error(0x10, &res);
        assert!(msg.contains("Nearest below: <none>"), "{msg}");
        assert!(msg.contains("FUN_08804000"), "{msg}");
    }

    // ---------------------------------------------------------------
    // Real-flow byte-identity tests against analysis.json
    // ---------------------------------------------------------------
    //
    // These run only when the repo-root analysis.json is present (it is a
    // local artifact, not committed). They share one PreparedEmission and
    // compare the dump path against the in-memory batch emission for the
    // same functions, asserting the dump output appears verbatim in the
    // batch .cpp content. PSPRECOMP_CROSS_MID is intentionally NOT set here
    // (env mutation is racy under the parallel test runner); the CROSS_MID
    // configuration is covered by the end-to-end CLI comparison documented
    // in DEBUGGING.md.

    static PREP: OnceLock<Option<PreparedEmission>> = OnceLock::new();

    fn real_prep() -> Option<&'static PreparedEmission> {
        PREP.get_or_init(|| {
            let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../analysis.json");
            if !path.exists() {
                eprintln!("analysis.json not present; skipping real-flow dump tests");
                return None;
            }
            Some(prepare_emission(&path, None).expect("prepare_emission"))
        })
        .as_ref()
    }

    /// Dump `func` via the single-function path and assert the result appears
    /// verbatim in the in-memory batch emission for the same function.
    fn assert_dump_matches_batch(prep: &PreparedEmission, target: &JsonFunction) {
        let addr = parse_hex_u32(&target.address).unwrap();
        let dumped = emit_function_cpp(prep, addr).expect("dump must emit an entry");
        let batch = psp_emitter::emit_function_batches(
            std::slice::from_ref(target),
            50,
            "",
            |f| emit_one_function(prep, f).0,
            &prep.unique_names,
        )
        .expect("batch emission");
        assert!(
            batch.cpp_files[0].1.contains(&dumped),
            "dump output for {} @ {} must appear byte-identical in the batch file",
            target.name,
            target.address,
        );
    }

    #[test]
    fn dump_entry_function_matches_batch() {
        let Some(prep) = real_prep() else { return };
        let target = prep
            .analysis
            .functions
            .iter()
            .find(|f| f.name == "entry")
            .expect("module start named 'entry'")
            .clone();
        let cpp = emit_function_cpp(prep, parse_hex_u32(&target.address).unwrap()).unwrap();
        assert!(cpp.contains("void entry(uint8_t* rdram, recomp_context* ctx)"), "{cpp}");
        assert_dump_matches_batch(prep, &target);
    }

    #[test]
    fn dump_ordinary_fun_matches_batch() {
        let Some(prep) = real_prep() else { return };
        let target = prep
            .analysis
            .functions
            .iter()
            .find(|f| {
                let addr = parse_hex_u32(&f.address).unwrap_or(0);
                f.name.starts_with("FUN_")
                    && prep.unique_names.get(&addr) == Some(&f.name)
                    && !prep.mid_entry_addr_map.contains_key(&addr)
            })
            .expect("an ordinary FUN_ without dedup or mid-entries")
            .clone();
        assert_dump_matches_batch(prep, &target);
    }

    #[test]
    fn dump_mid_entry_parent_matches_batch() {
        let Some(prep) = real_prep() else { return };
        let target = prep
            .analysis
            .functions
            .iter()
            .find(|f| {
                let addr = parse_hex_u32(&f.address).unwrap_or(0);
                prep.mid_entry_addr_map.contains_key(&addr)
            })
            .expect("a function carrying mid-entries")
            .clone();
        let cpp = emit_function_cpp(prep, parse_hex_u32(&target.address).unwrap()).unwrap();
        assert!(cpp.contains("entry_point"), "mid-entry parent must dispatch: {cpp}");
        assert_dump_matches_batch(prep, &target);
    }

    #[test]
    fn dump_deduplicated_name_matches_batch() {
        let Some(prep) = real_prep() else { return };
        let target = prep
            .analysis
            .functions
            .iter()
            .find(|f| {
                let addr = parse_hex_u32(&f.address).unwrap_or(0);
                prep.unique_names.get(&addr).is_some_and(|u| u != &f.name)
            })
            .expect("an _ADDR-deduplicated function (27 exist in the baseline)")
            .clone();
        let addr = parse_hex_u32(&target.address).unwrap();
        let cpp = emit_function_cpp(prep, addr).unwrap();
        let unique = &prep.unique_names[&addr];
        assert!(
            cpp.contains(&format!("void {}(", psp_emitter::sanitize_identifier(unique))),
            "dump must use the deduplicated name {unique}: {cpp}",
        );
        assert_dump_matches_batch(prep, &target);
    }

    #[test]
    fn dump_real_mid_entry_address_errors_with_parent() {
        let Some(prep) = real_prep() else { return };
        let entries: std::collections::HashSet<u32> = prep
            .analysis
            .functions
            .iter()
            .filter_map(|f| parse_hex_u32(&f.address))
            .collect();
        let me = prep
            .analysis
            .mid_entries
            .iter()
            .find(|me| {
                parse_hex_u32(&me.addr).is_some_and(|a| !entries.contains(&a))
            })
            .expect("a mid-entry that is not also a function entry");
        let addr = parse_hex_u32(&me.addr).unwrap();
        let err = emit_function_cpp(prep, addr).unwrap_err().to_string();
        assert!(err.contains("mid-entry"), "{err}");
        assert!(
            err.to_lowercase().contains(&me.parent_addr.to_lowercase()),
            "error must name the parent entry {}: {err}",
            me.parent_addr,
        );
    }
}
