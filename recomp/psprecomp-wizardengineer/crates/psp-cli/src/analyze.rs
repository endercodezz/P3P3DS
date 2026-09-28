//! Full `psprecomp analyze` pipeline: Ghidra invocation, Rust post-processing, mid-entry
//! detection, and analysis.json output.

use anyhow::{bail, Context, Result};
use base64::Engine as _;
use psp_parser::analysis_json::{JsonFunction, JsonMidEntry, JsonXref};
use std::path::{Path, PathBuf};
use std::process::Command;

/// Invoke Ghidra headless analysis and write raw JSON to `output`.
///
/// Checks for the kotcrab ghidra-allegrex extension and warns if it can't be
/// located (detection is heuristic); it hard-fails only when no Allegrex
/// language is present at all — see `check_allegrex_extension`.
/// For relocatable PRX inputs, `prx_base` pins the loader and image base
/// (`-loader PspElfLoader -loader-imagebase <hex>`) so ghidra-allegrex
/// rebases + relocates the image natively (plan D1); `None` (ET_EXEC) keeps
/// the invocation bit-for-bit unchanged (loader pinning deferred, D13).
pub fn run_ghidra_analysis(
    binary: &Path,
    ghidra_dir: &Path,
    output: &Path,
    prx_base: Option<u32>,
) -> Result<()> {
    // Issue #75: warn (don't false-block) when the ghidra-allegrex extension
    // can't be located; hard-fail only when no Allegrex language is present.
    check_allegrex_extension(ghidra_dir)?;

    // Locate ExtractAnalysis.java relative to the binary
    let script = find_analysis_script()?;
    let script_dir = script
        .parent()
        .context("ExtractAnalysis.java has no parent directory")?
        .to_str()
        .context("script directory path not UTF-8")?
        .to_owned();
    let headless = ghidra_dir.join("support/analyzeHeadless");

    // Ghidra aborts if the project parent directory doesn't exist (e.g. a
    // fresh boot cleared /tmp) — create it instead of failing cryptically.
    std::fs::create_dir_all("/tmp/ghidra_projects")
        .context("Failed to create /tmp/ghidra_projects")?;

    let mut args: Vec<String> = [
        "/tmp/ghidra_projects",
        "psprecomp_analysis",
        "-import",
        binary.to_str().context("binary path not UTF-8")?,
        "-scriptPath",
        &script_dir,
        "-postScript",
        "ExtractAnalysis.java",
        output.to_str().context("output path not UTF-8")?,
        "-deleteProject",
        "-overwrite",
    ]
    .iter()
    .map(|s| s.to_string())
    .collect();
    if let Some(base) = prx_base {
        // Bare hex, no 0x — the format analyzeHeadless expects (R3 §8).
        args.extend([
            "-loader".into(),
            "PspElfLoader".into(),
            "-loader-imagebase".into(),
            format!("{base:x}"),
        ]);
    }

    let status = Command::new(&headless)
        .args(&args)
        .status()
        .with_context(|| format!("Failed to execute {}", headless.display()))?;

    if !status.success() {
        bail!("Ghidra headless failed with exit code {:?}", status.code());
    }
    Ok(())
}

/// Verify the Allegrex tooling Ghidra will use, with DX-safe failure modes.
///
/// Detection of the kotcrab extension is heuristic (it can live in the install
/// tree or a per-user Ghidra settings dir, and its loader is packaged inside a
/// jar), so a miss is a loud WARNING rather than a hard error — false-blocking a
/// correctly-configured user is worse than the original issue-#75 silent pass.
/// (The downstream byte-equality gate validates *relocation*, not instruction
/// decode, so it does NOT catch a stock-Allegrex VFPU mis-decode — this warning,
/// not that gate, is the safeguard.) A hard error is reserved for "no Allegrex
/// language present at all", where Ghidra cannot produce usable output.
fn check_allegrex_extension(ghidra_dir: &Path) -> Result<()> {
    if allegrex_extension_installed(ghidra_dir) {
        return Ok(());
    }
    if !stock_allegrex_present(ghidra_dir) {
        bail!(
            "No Allegrex processor found under {} — is --ghidra-dir pointing at a valid \
             Ghidra install (its libexec dir)? Install Ghidra 12.0.2 PUBLIC + the \
             ghidra-allegrex v21.3 extension (asset \
             ghidra_12.0.2_PUBLIC_20260310_ghidra-allegrex.zip) — see README.",
            ghidra_dir.join("Ghidra/Processors/Allegrex").display()
        );
    }
    eprintln!(
        "warning: kotcrab ghidra-allegrex extension not detected under {} or your Ghidra \
         user-settings Extensions dir.\n  \
         Stock Ghidra's bundled Allegrex lacks the PspElfLoader + VFPU-complete language \
         this pipeline requires; analysis may be incorrect. Install ghidra-allegrex v21.3 \
         (Ghidra 12.0.2 build), asset ghidra_12.0.2_PUBLIC_20260310_ghidra-allegrex.zip, \
         from https://github.com/kotcrab/ghidra-allegrex/releases/tag/v21.3 — see README.\n  \
         If you installed it via the Ghidra GUI this check may not see it and you can \
         ignore this warning. Otherwise the analysis will be wrong — stock Allegrex \
         mis-decodes VFPU instructions — so install the extension and re-run.",
        ghidra_dir.join("Ghidra/Extensions").display()
    );
    Ok(())
}

/// True when the kotcrab ghidra-allegrex extension is installed for `ghidra_dir`.
///
/// Scans both the install tree (`<ghidra_dir>/Ghidra/Extensions/`, flat) and the
/// per-user Ghidra settings dirs the GUI "Install Extensions" flow writes to —
/// `--ghidra-dir` points only at the install tree, so a GUI install would
/// otherwise be invisible.
fn allegrex_extension_installed(ghidra_dir: &Path) -> bool {
    extension_search_roots(ghidra_dir)
        .iter()
        .any(|root| root_has_allegrex(root))
}

/// Extension directories to scan: the install tree plus any per-user Ghidra
/// settings dir (`$GHIDRA_USER_DIR/Extensions`, `~/.ghidra/.ghidra_<ver>/Extensions`).
fn extension_search_roots(ghidra_dir: &Path) -> Vec<PathBuf> {
    let user_dir = std::env::var_os("GHIDRA_USER_DIR").map(PathBuf::from);
    let home = std::env::var_os("HOME").map(PathBuf::from);
    extension_search_roots_from(ghidra_dir, user_dir.as_deref(), home.as_deref())
}

/// Pure path-assembly behind [`extension_search_roots`]; takes the resolved
/// `$GHIDRA_USER_DIR` and `$HOME` as arguments so the user-settings discovery is
/// testable without mutating process-global env in parallel tests.
fn extension_search_roots_from(
    ghidra_dir: &Path,
    user_dir: Option<&Path>,
    home: Option<&Path>,
) -> Vec<PathBuf> {
    let mut roots = vec![ghidra_dir.join("Ghidra/Extensions")];
    if let Some(user_dir) = user_dir {
        roots.push(user_dir.join("Extensions"));
    }
    if let Some(home) = home {
        if let Ok(entries) = std::fs::read_dir(home.join(".ghidra")) {
            for e in entries.flatten() {
                if e.file_name().to_string_lossy().starts_with(".ghidra_") {
                    roots.push(e.path().join("Extensions"));
                }
            }
        }
    }
    roots
}

/// True when `root` holds an extension dir whose name contains "allegrex" (the
/// reliable signal) or that carries a `PspElfLoader` marker (best-effort — the
/// loader usually lives inside a jar, so this mainly catches unpacked installs).
fn root_has_allegrex(root: &Path) -> bool {
    let Ok(entries) = std::fs::read_dir(root) else {
        return false;
    };
    entries.flatten().any(|e| {
        // file_type() does not follow symlinks; fall back to a path stat so a
        // symlinked extension dir still counts.
        let is_dir = e
            .file_type()
            .map(|t| t.is_dir() || (t.is_symlink() && e.path().is_dir()))
            .unwrap_or(false);
        is_dir
            && (e.file_name().to_string_lossy().to_lowercase().contains("allegrex")
                || dir_has_pspelfloader(&e.path()))
    })
}

/// True when stock Ghidra's bundled Allegrex SLEIGH module is present. Stock
/// Ghidra ships this without the extension's PspElfLoader/VFPU completeness, so
/// presence alone does NOT satisfy the extension check — it only distinguishes
/// "extension missing" (warn) from "no Allegrex at all" (hard error).
fn stock_allegrex_present(ghidra_dir: &Path) -> bool {
    ghidra_dir.join("Ghidra/Processors/Allegrex").is_dir()
}

/// True when `dir` (one extension, already known to be a directory) contains a
/// `PspElfLoader` marker entry. The caller has proven `dir` is a directory, so a
/// read failure here is a real fault (permissions, broken FS) — surface it
/// rather than silently treating it as "not installed".
fn dir_has_pspelfloader(dir: &Path) -> bool {
    let entries = match std::fs::read_dir(dir) {
        Ok(entries) => entries,
        Err(err) => {
            eprintln!("warning: could not read extension dir {}: {err}", dir.display());
            return false;
        }
    };
    entries.flatten().any(|e| {
        e.file_name()
            .to_string_lossy()
            .to_lowercase()
            .contains("pspelfloader")
    })
}

/// Find ExtractAnalysis.java relative to the executable or in `$PWD/analysis/`.
fn find_analysis_script() -> Result<std::path::PathBuf> {
    let candidates = [
        std::env::current_dir()?.join("analysis/ExtractAnalysis.java"),
        std::env::current_exe()?
            .parent()
            .unwrap_or(Path::new("."))
            .join("../analysis/ExtractAnalysis.java"),
    ];
    for path in &candidates {
        if path.exists() {
            return Ok(path.clone());
        }
    }
    bail!("ExtractAnalysis.java not found; expected at ./analysis/ExtractAnalysis.java");
}

/// Provenance sidecar for the `<output>.ghidra_raw.json` cache (plan D9).
///
/// A cached raw JSON may be reused only when this meta matches the current
/// run — otherwise a stale base-0 cache would silently defeat a rebased
/// re-run (the issue #52 failure mode).
#[derive(Debug, PartialEq, Eq, serde::Serialize, serde::Deserialize)]
struct GhidraCacheMeta {
    /// SHA-256 of the input binary bytes.
    binary_sha256: String,
    /// "PspElfLoader" when pinned (PRX), "auto" otherwise.
    loader: String,
    /// Bare-hex image base passed to the loader (e.g. "8804000"), or None.
    imagebase: Option<String>,
}

impl GhidraCacheMeta {
    /// Meta describing the current invocation.
    fn for_run(binary_sha256: String, prx_base: Option<u32>) -> Self {
        match prx_base {
            Some(base) => Self {
                binary_sha256,
                loader: "PspElfLoader".into(),
                imagebase: Some(format!("{base:x}")),
            },
            None => Self { binary_sha256, loader: "auto".into(), imagebase: None },
        }
    }
}

/// True when the cache meta sidecar exists and matches the current run.
fn ghidra_cache_valid(meta_path: &Path, expected: &GhidraCacheMeta) -> bool {
    let Ok(s) = std::fs::read_to_string(meta_path) else {
        return false;
    };
    match serde_json::from_str::<GhidraCacheMeta>(&s) {
        Ok(meta) => meta == *expected,
        Err(_) => false,
    }
}

/// Lowercase-hex SHA-256 of `data`.
fn sha256_hex(data: &[u8]) -> String {
    use sha2::{Digest, Sha256};
    format!("{:x}", Sha256::digest(data))
}

/// Parse a hex address string like "0x08804000" to u32.
fn parse_hex_addr(s: &str) -> Option<u32> {
    let t = s.trim().trim_start_matches("0x").trim_start_matches("0X");
    u32::from_str_radix(t, 16).ok()
}

/// Byte-equality gate (plan T6/D2): Ghidra's per-block SHA-256 exports must
/// match the psp-parser relocated segment bytes.
///
/// Runs for both ELF and PRX (for ET_EXEC it verifies Ghidra didn't perturb
/// bytes — free regression assurance). Blocks not fully contained in one
/// rebased segment are Ghidra-synthesized and skipped; a missing "blocks" key
/// (legacy cache, or Ghidra skipped) only warns.
fn verify_ghidra_block_hashes(
    ghidra_data: &serde_json::Value,
    seg_bases: &[u32],
    seg_datas: &[Vec<u8>],
) -> Result<()> {
    let Some(blocks) = ghidra_data.get("blocks").and_then(|b| b.as_array()) else {
        tracing::warn!(
            "ghidra_raw.json has no \"blocks\" key (legacy cache or Ghidra skipped); \
             byte-equality gate not enforced"
        );
        return Ok(());
    };
    let mut verified = 0usize;
    for block in blocks {
        let name = block["name"].as_str().unwrap_or("?");
        let (Some(start), Some(size), Some(ghidra_sha)) = (
            block["start"].as_str().and_then(parse_hex_addr),
            block["size"].as_u64(),
            block["sha256"].as_str(),
        ) else {
            bail!("malformed block entry in ghidra_raw.json: {block}");
        };
        let slice = seg_bases.iter().zip(seg_datas).find_map(|(&base, data)| {
            let off = start.checked_sub(base)? as usize;
            let end = off.checked_add(size as usize)?;
            data.get(off..end)
        });
        let Some(slice) = slice else {
            continue; // Ghidra-synthesized block outside every segment
        };
        let ours = sha256_hex(slice);
        if ours != ghidra_sha.to_lowercase() {
            bail!(
                "byte-equality gate FAILED for block {name} [start 0x{start:08X}, size \
                 {size}]: ghidra sha256 {ghidra_sha} != psp-parser sha256 {ours} — \
                 relocation engine divergence (reloc.rs vs ghidra-allegrex); see \
                 .planning/plans/52-prx-support-plan.md §1 D1 fallback"
            );
        }
        verified += 1;
    }
    tracing::info!(
        "Byte-equality gate: {verified}/{} Ghidra blocks verified against relocated segments",
        blocks.len()
    );
    Ok(())
}

/// Detect mid-function entry points from Ghidra xrefs (V1-compatible approach).
///
/// V1's `_find_midentry_points()` used xrefs from Ghidra's ReferenceManager —
/// DATA references from .data/.rodata pointing to addresses inside known function
/// bodies (but not at function starts). This produces ~532 entries for Patapon BOOT.BIN.
///
/// Branch-scan (previous approach) found 31,042 — too broad for Phase 2.
fn detect_mid_entries_from_xrefs(
    functions: &[JsonFunction],
    xrefs: &[JsonXref],
) -> Vec<JsonMidEntry> {
    use std::collections::HashSet;

    let func_starts: HashSet<u64> = functions
        .iter()
        .filter_map(|f| {
            u64::from_str_radix(f.address.trim_start_matches("0x"), 16).ok()
        })
        .collect();

    let mut sorted_funcs: Vec<(u64, u64, String)> = functions
        .iter()
        .filter_map(|f| {
            let start = u64::from_str_radix(f.address.trim_start_matches("0x"), 16).ok()?;
            Some((start, start + f.size, f.address.clone()))
        })
        .collect();
    sorted_funcs.sort_by_key(|(s, _, _)| *s);

    let mut mid_set: HashSet<u64> = HashSet::new();
    let mut mid_entries: Vec<JsonMidEntry> = Vec::new();

    for xref in xrefs {
        let to_addr = match u64::from_str_radix(
            xref.to_addr.trim_start_matches("0x").trim_start_matches("0X"),
            16,
        ) {
            Ok(a) => a,
            Err(_) => continue,
        };
        if func_starts.contains(&to_addr) {
            continue;
        }
        // Instruction addresses are 4-aligned; an unaligned xref target is
        // data noise (e.g. a RAW_SCAN word that happens to look like a code
        // pointer), and the emitter could never label it (issue #52 Gate A:
        // one such target at 0x08B3124E broke the parent's mid-entry switch).
        if to_addr % 4 != 0 {
            continue;
        }

        let pos = sorted_funcs.partition_point(|(s, _, _)| *s <= to_addr);
        if pos == 0 {
            continue;
        }
        let (start, end, parent_addr) = &sorted_funcs[pos - 1];
        if to_addr < *start || to_addr >= *end {
            continue;
        }

        if mid_set.insert(to_addr) {
            mid_entries.push(JsonMidEntry {
                addr: format!("0x{to_addr:08X}"),
                parent_addr: parent_addr.clone(),
            });
        }
    }

    mid_entries
}

/// Run HLE entry point scan: discover function pointers passed to HLE APIs.
///
/// Builds the stub address map from either PRX imports or ELF .lib.stub,
/// scans the .text segment for JAL call sites, and merges new entries into
/// the functions list.
fn run_hle_entry_scan(
    elf_obj: &goblin::elf::Elf,
    nid_map: &std::collections::HashMap<u32, String>,
    import_stubs: &[psp_parser::types::ImportStub],
    segments: &[psp_parser::types::Segment],
    seg_data_vecs: &[Vec<u8>],
    functions: &mut Vec<JsonFunction>,
) -> Result<crate::hle_entry_scanner::HleDiscoveryResult> {
    use crate::hle_entry_scanner;

    // Build HLE stub address map
    let stub_map = if !import_stubs.is_empty() {
        // PRX: build from analysis.json imports directly
        hle_entry_scanner::build_stub_map_from_imports(import_stubs)
    } else {
        // ELF (like Patapon): discover via the consolidated .lib.stub walker.
        // seg_data_vecs holds the loaded segment bytes; bases come from the
        // rebased segment vaddrs (ET_EXEC rebases by 0, i.e. linked addresses).
        let seg_bases: Vec<u32> = segments.iter().map(|s| s.p_vaddr).collect();
        hle_entry_scanner::discover_import_stubs_for_elf(
            elf_obj,
            &seg_bases,
            seg_data_vecs,
            nid_map,
        )
    };

    if stub_map.is_empty() {
        tracing::warn!("No HLE funcptr API stubs found; skipping scan");
        return Ok(hle_entry_scanner::HleDiscoveryResult {
            call_sites_scanned: 0,
            discoveries: Vec::new(),
            new_entries: Vec::new(),
            size_corrections: Vec::new(),
            functions_before: functions.len(),
        });
    }

    // Find .text segment (PF_X = 0x1)
    let text_seg = segments.iter().enumerate().find(|(_, s)| {
        s.p_flags & 0x1 != 0 // PF_X: executable
    });

    let (text_bytes, text_base) = match text_seg {
        Some((idx, seg)) => (&seg_data_vecs[idx][..], seg.p_vaddr),
        None => {
            tracing::warn!("No executable segment found; skipping HLE scan");
            return Ok(hle_entry_scanner::HleDiscoveryResult {
                call_sites_scanned: 0,
                discoveries: Vec::new(),
                new_entries: Vec::new(),
                size_corrections: Vec::new(),
                functions_before: functions.len(),
            });
        }
    };

    // Scan for HLE entry points
    let result = hle_entry_scanner::scan_hle_entries(
        text_bytes, text_base, &stub_map, functions,
    );

    tracing::info!(
        "HLE scan: {} call sites, {} discoveries ({} new), {} known",
        result.call_sites_scanned,
        result.discoveries.len(),
        result.new_entries.len(),
        result.discoveries.len() - result.new_entries.len()
    );

    // Merge new entries into functions list
    for entry in &result.new_entries {
        functions.push(JsonFunction {
            name: format!("FUN_{:08X}", entry.address),
            address: format!("0x{:08X}", entry.address),
            size: entry.estimated_size,
            is_external: false,
            is_thunk: false,
            source: "hle_scan".into(),
        });
    }

    Ok(result)
}

/// Full analyze pipeline: Ghidra + Rust post-processing → analysis.json.
pub fn run_analyze(
    binary: &std::path::PathBuf,
    output: &std::path::PathBuf,
    ghidra_dir: Option<&std::path::PathBuf>,
    nid_db: &std::path::PathBuf,
    load_base_override: Option<u32>,
) -> Result<()> {
    use base64::engine::general_purpose::STANDARD as B64;
    use psp_parser::{elf, nid, prx};

    tracing::info!("Analyzing {}", binary.display());

    // 1. Read binary
    let raw_data = std::fs::read(binary)
        .with_context(|| format!("Cannot read {}", binary.display()))?;

    // 2. Parse ELF/PRX; compute the load base exactly once (plan D3)
    let elf_obj = elf::parse_elf(&raw_data)?;
    let is_prx = prx::is_prx(&elf_obj);
    tracing::info!("Binary type: {}", if is_prx { "PRX" } else { "ELF" });
    let load_base = crate::prx_load::compute_load_base(is_prx, load_base_override);
    if is_prx {
        tracing::info!("PRX load base: 0x{load_base:08X}");
    }

    // 3. Extract segments (BSS zeroed to p_memsz) and rebase them. Heap base,
    // seg_bases, JSON segment records, and the HLE scan all derive from the
    // rebased p_vaddr values (no-op for ET_EXEC: load_base == 0).
    let mut segments = elf::extract_segments(&raw_data, &elf_obj);
    elf::rebase_segments(&mut segments, load_base);
    let heap_base = elf::calculate_heap_base(&segments);
    tracing::info!("Heap base: 0x{heap_base:08X}");

    // 4. Parse and apply relocations (PRX only)
    let (mut seg_data_vecs, seg_bases): (Vec<Vec<u8>>, Vec<u32>) =
        segments.iter().map(|s| (s.data.clone(), s.p_vaddr)).unzip();
    let all_reloc_entries = if is_prx {
        crate::prx_load::apply_prx_relocations(
            &raw_data,
            &elf_obj,
            &mut seg_data_vecs,
            &seg_bases,
        )?
    } else {
        vec![]
    };

    // 5. Parse NIDs and import stubs for BOTH formats (issue #40 — the
    // generated syscall table consumes imports[]). D6: from the relocated
    // image, so every pointer field is final. D8 (PRX): hard error — never a
    // silent empty imports[]; ET_EXEC degrades loudly only when SceModuleInfo
    // itself is absent (recompile refuses an import-free analysis.json).
    if !nid_db.exists() {
        anyhow::bail!(
            "NID database not found at {}.\n\
             It is not committed (it is PPSSPP-derived; data/ is gitignored).\n\
             Fetch it with: ./scripts/fetch-niddb.sh\n\
             Or pass an existing copy with --nid-db <path>.",
            nid_db.display()
        );
    }
    let nid_map = nid::load_nid_database(nid_db)
        .with_context(|| format!("Cannot load NID DB from {}", nid_db.display()))?;
    let (import_stubs, module_info) = if is_prx {
        let (stubs, mi) = crate::prx_load::parse_prx_imports(
            &elf_obj,
            load_base,
            &seg_bases,
            &seg_data_vecs,
            &nid_map,
        )?;
        (stubs, Some(mi))
    } else {
        crate::prx_load::parse_elf_imports(&elf_obj, &seg_bases, &seg_data_vecs, &nid_map)?
    };
    tracing::info!("Resolved {} import stubs", import_stubs.len());

    // 5.5. Module facts (issue #47 Phase 2): name/gp from SceModuleInfo via
    // the same parser path for both formats; entry = load_base + e_entry;
    // text extent per PPSSPP ElfReader semantics. Consumed by recompile to
    // emit output/include/recomp_module.h for the runtime boot path.
    let entry_va = load_base.wrapping_add(elf_obj.header.e_entry as u32);
    let file_stem = binary
        .file_stem()
        .and_then(|s| s.to_str())
        .unwrap_or("unknown")
        .to_lowercase();
    let module_facts = {
        let image = psp_parser::image::LoadedImage::new(&seg_bases, &seg_data_vecs);
        crate::prx_load::build_module_facts(
            &elf_obj,
            &image,
            load_base,
            entry_va,
            module_info.as_ref(),
            &file_stem,
        )
    };

    // 6. Run Ghidra headless analysis. The raw-output cache is reused only
    // when its meta sidecar matches this run (binary hash, loader, image
    // base) — a stale base-0 cache must never be silently reused (plan D9).
    const GHIDRA_STUB_JSON: &str = r#"{"functions":[],"xrefs":[],"constructors":[]}"#;
    let ghidra_raw_path = output.with_extension("ghidra_raw.json");
    let meta_path = output.with_extension("ghidra_raw.meta.json");
    let expected_meta = GhidraCacheMeta::for_run(sha256_hex(&raw_data), is_prx.then_some(load_base));
    if ghidra_raw_path.exists() && ghidra_cache_valid(&meta_path, &expected_meta) {
        tracing::info!("Using existing {} (cache meta matches)", ghidra_raw_path.display());
    } else if let Some(ghidra) = ghidra_dir {
        if ghidra_raw_path.exists() {
            tracing::warn!(
                "stale/unverified ghidra_raw cache at {} — re-running Ghidra",
                ghidra_raw_path.display()
            );
        }
        run_ghidra_analysis(binary, ghidra, &ghidra_raw_path, is_prx.then_some(load_base))?;
        std::fs::write(&meta_path, serde_json::to_string_pretty(&expected_meta)?)
            .with_context(|| format!("Failed to write {}", meta_path.display()))?;
    } else if ghidra_raw_path.exists()
        && std::fs::read_to_string(&ghidra_raw_path)
            .map(|s| s != GHIDRA_STUB_JSON)
            .unwrap_or(true)
    {
        bail!(
            "ghidra_raw cache at {} has no matching meta sidecar ({}) and cannot be \
             verified — a stale base-0 cache would silently corrupt the analysis. \
             Re-run with --ghidra-dir to regenerate it (and the meta), or delete it.",
            ghidra_raw_path.display(),
            meta_path.display()
        );
    } else {
        tracing::warn!("--ghidra-dir not provided; skipping Ghidra analysis");
        std::fs::write(&ghidra_raw_path, GHIDRA_STUB_JSON)?;
    }

    // 7. Load Ghidra raw output
    let ghidra_json_str = std::fs::read_to_string(&ghidra_raw_path)
        .with_context(|| format!("Cannot read {}", ghidra_raw_path.display()))?;
    let ghidra_data: serde_json::Value =
        serde_json::from_str(&ghidra_json_str).context("Ghidra raw JSON is invalid")?;

    // 7.5. Byte-equality gate (plan T6/D2): hard-fail if Ghidra's relocated
    // block bytes differ from psp-parser's relocated segments.
    verify_ghidra_block_hashes(&ghidra_data, &seg_bases, &seg_data_vecs)?;

    // 8. Merge all data into AnalysisJson
    use psp_parser::analysis_json::*;

    let mut functions: Vec<JsonFunction> =
        serde_json::from_value(ghidra_data["functions"].clone())
            .unwrap_or_default();

    let xrefs: Vec<JsonXref> =
        serde_json::from_value(ghidra_data["xrefs"].clone())
            .unwrap_or_default();

    let constructors: Vec<String> =
        serde_json::from_value(ghidra_data["constructors"].clone())
            .unwrap_or_default();

    // 8.1. Merge fixes (plan T5 item 7; both idempotent no-ops on Patapon):
    // rename the module-start function to "entry" (the runtime hard-links the
    // symbol), and drop jal_target artifacts outside the loaded image.
    crate::prx_load::rename_entry_function(&mut functions, entry_va);
    let image_start = segments.iter().map(|s| s.p_vaddr).min().unwrap_or(0);
    let image_end = segments
        .iter()
        .map(|s| s.p_vaddr.saturating_add(s.p_memsz))
        .max()
        .unwrap_or(0);
    let dropped =
        crate::prx_load::drop_out_of_image_jal_targets(&mut functions, image_start, image_end);
    if dropped > 0 {
        tracing::info!("Dropped {dropped} out-of-image jal_target artifact(s)");
    }

    // 8.5. HLE entry point discovery
    // Scan decoded instructions near import stub call sites for function
    // pointers passed to sceKernelCreateThread, sceKernelCreateCallback, etc.
    let functions_before = functions.len();
    let hle_result = run_hle_entry_scan(
        &elf_obj,
        &nid_map,
        &import_stubs,
        &segments,
        &seg_data_vecs,
        &mut functions,
    )?;
    if functions.len() > functions_before {
        tracing::info!(
            "HLE scan: {} -> {} functions (+{})",
            functions_before,
            functions.len(),
            functions.len() - functions_before
        );
    }

    // 8.6. Correct giant function sizes by clamping to next boundary
    let size_corrections =
        crate::hle_entry_scanner::correct_function_sizes(&mut functions);
    if !size_corrections.is_empty() {
        tracing::info!(
            "Corrected {} giant function sizes",
            size_corrections.len()
        );
    }

    // 8.7. Write discovered_entries.json audit file
    let discovered_path = output.with_extension("").with_file_name(
        "discovered_entries.json",
    );
    crate::hle_entry_scanner::write_discovered_entries_json(
        &hle_result,
        &size_corrections,
        &discovered_path,
    )?;

    // 9. Build segment records (base64-encoded, p_memsz bytes).
    let json_segments: Vec<JsonSegment> = segments
        .iter()
        .zip(seg_data_vecs.iter())
        .map(|(s, d)| JsonSegment {
            p_vaddr: format!("0x{:08X}", s.p_vaddr),
            p_filesz: s.p_filesz as u64,
            p_memsz: s.p_memsz as u64,
            p_flags: s.p_flags,
            data_b64: B64.encode(d),
        })
        .collect();

    // 10. Mid-entry detection via Ghidra xrefs (V1-compatible, ANALYSIS-07).
    // Now runs on the corrected function list (HLE entries + size fixes),
    // which means xrefs that previously pointed "inside" a giant function may
    // now correctly target the newly-added standalone functions instead.
    let mid_entries = detect_mid_entries_from_xrefs(&functions, &xrefs);
    tracing::info!(
        "Detected {} mid-function entry points (xref-based)",
        mid_entries.len()
    );

    let analysis = AnalysisJson {
        binary_path: binary.display().to_string(),
        module_name: file_stem,
        heap_base: format!("0x{heap_base:08X}"),
        // Module facts (issue #47 Phase 2): present for BOTH formats.
        module: Some(module_facts),
        // PRX load provenance (plan T5 item 8): absent for ET_EXEC
        // (skip_serializing_if). Module identity lives in `module` above.
        prx: is_prx.then(|| JsonPrxInfo {
            load_base: format!("0x{load_base:08X}"),
        }),
        functions,
        imports: import_stubs
            .iter()
            .map(|s| JsonImport {
                nid: format!("0x{:08X}", s.nid),
                stub_addr: format!("0x{:08X}", s.stub_addr),
                name: s.name.clone(),
                module_name: s.module_name.clone(),
            })
            .collect(),
        relocations: all_reloc_entries
            .iter()
            .map(|r| JsonReloc {
                offset: format!("0x{:08X}", r.offset),
                r_type: r.r_type,
                ofs_base: r.ofs_base,
                addr_base: r.addr_base,
            })
            .collect(),
        xrefs,
        constructors,
        mid_entries,
        segments: json_segments,
    };

    // 11. Write output
    let json_str =
        serde_json::to_string_pretty(&analysis).context("Failed to serialize analysis.json")?;
    std::fs::write(output, json_str)
        .with_context(|| format!("Failed to write {}", output.display()))?;

    tracing::info!(
        "Analysis complete: {} functions, {} imports, {} mid-entries -> {}",
        analysis.functions.len(),
        analysis.imports.len(),
        analysis.mid_entries.len(),
        output.display()
    );
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn block_json(start: u32, data: &[u8], sha_of: &[u8]) -> serde_json::Value {
        serde_json::json!({
            "blocks": [{
                "name": ".text",
                "start": format!("0x{start:08X}"),
                "size": data.len() as u64,
                "sha256": sha256_hex(sha_of),
            }]
        })
    }

    #[test]
    fn gate_passes_on_matching_block_bytes() {
        let seg = vec![0xAAu8, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF];
        let ghidra = block_json(0x0880_4002, &seg[2..6], &seg[2..6]);
        verify_ghidra_block_hashes(&ghidra, &[0x0880_4000], &[seg]).unwrap();
    }

    #[test]
    fn gate_fails_loudly_on_divergent_bytes() {
        let seg = vec![0u8; 16];
        let other = vec![1u8; 4];
        let ghidra = block_json(0x0880_4000, &other, &other);
        let err = verify_ghidra_block_hashes(&ghidra, &[0x0880_4000], &[seg]).unwrap_err();
        let msg = err.to_string();
        assert!(msg.contains("byte-equality gate FAILED"), "actionable: {msg}");
        assert!(msg.contains(".text"), "names the block: {msg}");
    }

    #[test]
    fn gate_skips_blocks_outside_every_segment_and_warns_on_missing_key() {
        // Ghidra-synthesized block (e.g. an overlay) not contained in a segment.
        let seg = vec![0u8; 8];
        let ghidra = block_json(0x0000_0000, &[1, 2, 3, 4], &[9, 9, 9, 9]);
        verify_ghidra_block_hashes(&ghidra, &[0x0880_4000], &[seg.clone()]).unwrap();
        // Missing "blocks" key (legacy cache): warn-only, never an error.
        let legacy = serde_json::json!({"functions": []});
        verify_ghidra_block_hashes(&legacy, &[0x0880_4000], &[seg]).unwrap();
    }

    #[test]
    fn cache_meta_matches_only_same_binary_loader_and_base() {
        let dir = std::env::temp_dir().join(format!("psprecomp_meta_test_{}", std::process::id()));
        std::fs::create_dir_all(&dir).unwrap();
        let meta_path = dir.join("x.ghidra_raw.meta.json");
        let written = GhidraCacheMeta::for_run("abc123".into(), Some(0x0880_4000));
        std::fs::write(&meta_path, serde_json::to_string(&written).unwrap()).unwrap();

        let same = GhidraCacheMeta::for_run("abc123".into(), Some(0x0880_4000));
        assert!(ghidra_cache_valid(&meta_path, &same));
        // Different base, different binary, or ET_EXEC run: all invalid.
        let other_base = GhidraCacheMeta::for_run("abc123".into(), Some(0x0900_0000));
        assert!(!ghidra_cache_valid(&meta_path, &other_base));
        let other_bin = GhidraCacheMeta::for_run("def456".into(), Some(0x0880_4000));
        assert!(!ghidra_cache_valid(&meta_path, &other_bin));
        let et_exec = GhidraCacheMeta::for_run("abc123".into(), None);
        assert!(!ghidra_cache_valid(&meta_path, &et_exec));
        // Missing sidecar: invalid.
        assert!(!ghidra_cache_valid(&dir.join("missing.json"), &same));
        std::fs::remove_dir_all(&dir).ok();
    }

    #[test]
    fn stock_only_tree_is_not_detected_as_extension() {
        // Issue #75: stock Ghidra 12.x ships Processors/Allegrex but NOT the
        // kotcrab extension. The stock module must NOT satisfy the extension
        // check (that was the false-positive), yet must be recognized as "some
        // Allegrex present" so the policy warns rather than hard-errors.
        let root = tempfile::tempdir().unwrap();
        let g = root.path();
        std::fs::create_dir_all(g.join("Ghidra/Processors/Allegrex/data/languages")).unwrap();
        assert!(stock_allegrex_present(g));
        assert!(
            !allegrex_extension_installed(g),
            "stock Processors/Allegrex must not satisfy the extension check"
        );
    }

    #[test]
    fn extension_at_real_flat_path_is_detected() {
        // A genuine install unzips FLAT to <ghidra>/Ghidra/Extensions/<name>/,
        // NOT nested under a second Ghidra/ — guards the false-negative bug.
        let root = tempfile::tempdir().unwrap();
        let g = root.path();
        let ext = g.join("Ghidra/Extensions/ghidra_12.0.2_PUBLIC_20260310_ghidra-allegrex");
        std::fs::create_dir_all(&ext).unwrap();
        std::fs::write(ext.join("Module.manifest"), b"").unwrap();
        assert!(
            allegrex_extension_installed(g),
            "flat Extensions/<...allegrex> must be detected"
        );
    }

    #[test]
    fn pspelfloader_marker_detects_oddly_named_extension() {
        // An extension dir whose name lacks "allegrex" still passes when it
        // carries the pinned PspElfLoader marker (best-effort fallback).
        let root = tempfile::tempdir().unwrap();
        let g = root.path();
        let ext = g.join("Ghidra/Extensions/some-psp-ext");
        std::fs::create_dir_all(&ext).unwrap();
        std::fs::write(ext.join("PspElfLoader.class"), b"").unwrap();
        assert!(allegrex_extension_installed(g));
    }

    #[test]
    fn user_settings_extensions_dir_is_scanned() {
        // GUI "Install Extensions" unzips into ~/.ghidra/.ghidra_<ver>/Extensions
        // (or $GHIDRA_USER_DIR/Extensions), not the install tree that
        // --ghidra-dir points at. root_has_allegrex must accept it. (Tested via
        // the per-root helper to avoid mutating process env in parallel tests.)
        let root = tempfile::tempdir().unwrap();
        let user_ext = root.path().join(".ghidra/.ghidra_12.0.2_PUBLIC/Extensions");
        std::fs::create_dir_all(user_ext.join("ghidra-allegrex")).unwrap();
        assert!(root_has_allegrex(&user_ext));
    }

    #[test]
    fn search_roots_assemble_user_settings_via_home() {
        // The GUI-install path end to end: HOME/.ghidra/.ghidra_<ver>/Extensions
        // must be produced by the discovery logic (the `.ghidra_` prefix filter +
        // join), exercised without mutating process-global env.
        let home = tempfile::tempdir().unwrap();
        let ext = home
            .path()
            .join(".ghidra/.ghidra_12.0.2_PUBLIC/Extensions/ghidra-allegrex");
        std::fs::create_dir_all(&ext).unwrap();
        let ghidra = tempfile::tempdir().unwrap(); // empty install tree
        let roots = extension_search_roots_from(ghidra.path(), None, Some(home.path()));
        assert!(
            roots
                .iter()
                .any(|r| r.ends_with(".ghidra_12.0.2_PUBLIC/Extensions")),
            "user-settings Extensions dir must be assembled into the search roots: {roots:?}"
        );
        assert!(
            roots.iter().any(|r| root_has_allegrex(r)),
            "the extension under HOME/.ghidra must be found via the assembled roots"
        );
    }

    #[test]
    fn search_roots_assemble_ghidra_user_dir() {
        // $GHIDRA_USER_DIR/Extensions must be a search root.
        let userdir = tempfile::tempdir().unwrap();
        std::fs::create_dir_all(userdir.path().join("Extensions/ghidra-allegrex")).unwrap();
        let ghidra = tempfile::tempdir().unwrap();
        let roots = extension_search_roots_from(ghidra.path(), Some(userdir.path()), None);
        assert!(
            roots.iter().any(|r| root_has_allegrex(r)),
            "the extension under $GHIDRA_USER_DIR must be found"
        );
    }

    #[test]
    fn check_warns_but_succeeds_when_extension_missing_but_stock_present() {
        // The issue-#75 balance: stock Allegrex present, no kotcrab extension ->
        // warn + continue (Ok), NOT a hard block.
        let root = tempfile::tempdir().unwrap();
        std::fs::create_dir_all(root.path().join("Ghidra/Processors/Allegrex")).unwrap();
        check_allegrex_extension(root.path()).expect("missing extension must warn, not fail");
    }

    #[test]
    fn check_hard_errors_when_no_allegrex_at_all() {
        // Neither extension nor stock module -> hard error naming the path.
        let root = tempfile::tempdir().unwrap();
        let err = check_allegrex_extension(root.path()).unwrap_err();
        assert!(
            err.to_string().contains("No Allegrex processor found"),
            "error must be actionable: {err}"
        );
    }

    #[test]
    fn check_passes_silently_when_extension_installed() {
        let root = tempfile::tempdir().unwrap();
        std::fs::create_dir_all(root.path().join("Ghidra/Extensions/ghidra-allegrex")).unwrap();
        check_allegrex_extension(root.path()).expect("installed extension must pass");
    }

    #[test]
    fn cache_meta_serializes_bare_hex_imagebase() {
        let prx = GhidraCacheMeta::for_run("s".into(), Some(0x0880_4000));
        assert_eq!(prx.loader, "PspElfLoader");
        assert_eq!(prx.imagebase.as_deref(), Some("8804000"));
        let elf = GhidraCacheMeta::for_run("s".into(), None);
        assert_eq!(elf.loader, "auto");
        assert_eq!(elf.imagebase, None);
    }
}
