//! `output/fingerprint.json` + `output/include/recomp_fingerprint.h` — build
//! fingerprinting so that a runtime configured against a stale `output/` fails
//! fast instead of silently invalidating every conclusion (issue #36).
//!
//! Written at the end of every `psprecomp recompile` run, next to
//! `recompile_report.json`. Consumed by:
//! - `runtime/CMakeLists.txt` via `runtime/cmake/check_fingerprint.py`
//!   (configure-time staleness check, overridable with
//!   `-DPSPRECOMP_ALLOW_STALE=ON`);
//! - `runtime/src/main.cpp`, which prints the one-line
//!   `[RT] output fingerprint: ...` boot banner from the generated header.
//!
//! ## Hash recipe (version 1) — MUST stay in sync with
//! `runtime/cmake/check_fingerprint.py`
//!
//! 1. **File set:** every `*.rs` file, recursively, under each directory in
//!    `hash_recipe.roots` (`crates/psp-emitter/src`, `crates/psp-decoder/src`,
//!    `crates/psp-ir/src`, `crates/psp-optimizer/src`) plus each file in
//!    `hash_recipe.extra_files` (the psp-cli sources that shape emission:
//!    `recompile.rs`, `config.rs`, `hle_entry_scanner.rs`). Paths are
//!    repo-root-relative with `/` separators.
//! 2. **Per-file hash:** SHA-256 of the raw file bytes, lowercase hex.
//! 3. **Combined hash** (`emitter_sources_hash`): SHA-256 over the UTF-8
//!    concatenation of `"{path}\n{per_file_hash}\n"` for every file, sorted by
//!    path (byte order).
//!
//! Deterministic by construction: identical source trees produce an identical
//! `emitter_sources_hash`. Timestamps and git state are recorded for human
//! context only and do NOT feed the hash. Hashing file *contents* (not a git
//! commit) means uncommitted edits change the fingerprint — the exact trap
//! this exists to catch.
//!
//! The configure-time check recomputes per-file hashes for **exactly the files
//! listed in `files[]`** (so the two sides can never disagree about the list),
//! and additionally re-enumerates `hash_recipe.roots`/`extra_files` from the
//! JSON to catch source files *added* after the recompile (which a recorded
//! list alone cannot cover).
//!
//! What the fingerprint does NOT cover: runtime/ sources (CMake rebuilds those
//! itself), the NID database, game config TOMLs passed via `--config`, and
//! Cargo dependency versions. A `cargo update` that changes codegen without
//! touching these sources will not be detected.

use std::path::{Path, PathBuf};

use anyhow::Context;
use serde::Serialize;
use sha2::{Digest, Sha256};

/// Current fingerprint schema version. Bump when fields change shape/meaning.
pub const SCHEMA_VERSION: u32 = 1;

/// Hash recipe version (file-set rules + combine rule). Bump together with
/// `runtime/cmake/check_fingerprint.py` when the recipe changes.
pub const HASH_RECIPE_VERSION: u32 = 1;

/// Directories whose `*.rs` files (recursive) determine codegen.
const SOURCE_ROOTS: &[&str] = &[
    "crates/psp-emitter/src",
    "crates/psp-decoder/src",
    "crates/psp-ir/src",
    "crates/psp-optimizer/src",
];

/// Individual psp-cli sources that shape emission (discovery passes, game
/// config semantics, coalesce/mid-entry injection). main.rs/report.rs/analyze.rs
/// are deliberately excluded: they do not influence the generated C++.
const EXTRA_FILES: &[&str] = &[
    "crates/psp-cli/src/recompile.rs",
    "crates/psp-cli/src/config.rs",
    "crates/psp-cli/src/hle_entry_scanner.rs",
];

/// Sentinel value for `emitter_sources_hash` when the Rust sources are not on
/// disk (binary invoked outside the repo). The configure check treats it as
/// "unverifiable" (loud warning, not an error).
pub const HASH_UNAVAILABLE: &str = "unavailable";

/// Top-level structure serialized to `output/fingerprint.json`.
#[derive(Debug, Serialize)]
pub struct BuildFingerprint {
    pub schema_version: u32,
    /// UTC timestamp, ISO-8601 (e.g. "2026-06-12T01:23:45Z"). Context only —
    /// not part of any hash.
    pub generated_at: String,
    /// Combined SHA-256 over the codegen-determining Rust sources (module doc).
    pub emitter_sources_hash: String,
    /// Git state at recompile time — human context only (content hashes above
    /// are what catch uncommitted edits).
    pub git: GitInfo,
    pub analysis: AnalysisInfo,
    pub flags: FingerprintFlags,
    pub counts: FingerprintCounts,
    pub hash_recipe: HashRecipe,
    /// Repo-root-relative path -> SHA-256, sorted by path. The configure-time
    /// check recomputes hashes for exactly these files.
    pub files: Vec<FileHash>,
}

/// `git describe --always --dirty` style context.
#[derive(Debug, Serialize)]
pub struct GitInfo {
    /// Short commit hash, or "unknown" when git is unavailable.
    pub commit: String,
    /// True when the working tree had uncommitted changes at recompile time.
    pub dirty: bool,
}

/// Identity of the analysis.json the output was generated from.
#[derive(Debug, Serialize)]
pub struct AnalysisInfo {
    /// The path as passed on the command line (may be relative or a symlink).
    pub path: String,
    /// SHA-256 of the file bytes, lowercase hex.
    pub sha256: String,
}

/// Recompile-time environment flags that change codegen and are invisible
/// afterwards. `PSPRECOMP_CROSS_MID=1` is the historical trap.
#[derive(Debug, Serialize)]
pub struct FingerprintFlags {
    pub cross_mid: bool,
}

/// Headline counts, mirroring `recompile_report.json` for quick eyeballing.
#[derive(Debug, Serialize)]
pub struct FingerprintCounts {
    pub functions: usize,
    pub mid_entries: usize,
    pub batch_files: usize,
}

/// The file-set rules, embedded so the configure script enumerates the same
/// roots without duplicating the list in two languages.
#[derive(Debug, Serialize)]
pub struct HashRecipe {
    pub version: u32,
    pub roots: Vec<String>,
    pub extra_files: Vec<String>,
}

/// One hashed source file (repo-root-relative path, `/` separators).
#[derive(Debug, Clone, Serialize)]
pub struct FileHash {
    pub path: String,
    pub sha256: String,
}

/// Inputs gathered by `run_recompile` for [`build_fingerprint`].
pub struct FingerprintInputs<'a> {
    pub analysis_path: &'a Path,
    pub cross_mid: bool,
    pub counts: FingerprintCounts,
}

/// Repository root, derived from this crate's manifest dir at compile time.
///
/// Valid for the documented `cargo run` workflow (the binary lives in the
/// repo's target/). If the directory no longer exists (binary copied
/// elsewhere), source hashing degrades to [`HASH_UNAVAILABLE`].
pub fn repo_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("..").join("..")
}

/// Assemble the full fingerprint. Never fails the recompile: source-collection
/// problems degrade to `emitter_sources_hash = "unavailable"` with a warning.
pub fn build_fingerprint(inputs: FingerprintInputs) -> BuildFingerprint {
    let root = repo_root();
    let files = match collect_source_hashes(&root, SOURCE_ROOTS, EXTRA_FILES) {
        Ok(files) => files,
        Err(e) => {
            tracing::warn!(
                "fingerprint: cannot hash emitter sources under {} ({e:#}); \
                 staleness checking will be unavailable for this output dir",
                root.display(),
            );
            Vec::new()
        }
    };
    let emitter_sources_hash = if files.is_empty() {
        HASH_UNAVAILABLE.to_string()
    } else {
        combined_hash(&files)
    };

    let analysis_sha256 = sha256_file(inputs.analysis_path).unwrap_or_else(|e| {
        tracing::warn!("fingerprint: cannot hash {}: {e:#}", inputs.analysis_path.display());
        HASH_UNAVAILABLE.to_string()
    });

    BuildFingerprint {
        schema_version: SCHEMA_VERSION,
        generated_at: crate::report::iso8601_utc_now(),
        emitter_sources_hash,
        git: git_info(&root),
        analysis: AnalysisInfo {
            path: inputs.analysis_path.display().to_string(),
            sha256: analysis_sha256,
        },
        flags: FingerprintFlags { cross_mid: inputs.cross_mid },
        counts: inputs.counts,
        hash_recipe: HashRecipe {
            version: HASH_RECIPE_VERSION,
            roots: SOURCE_ROOTS.iter().map(|s| s.to_string()).collect(),
            extra_files: EXTRA_FILES.iter().map(|s| s.to_string()).collect(),
        },
        files,
    }
}

/// Write `fingerprint.json` and `include/recomp_fingerprint.h` into the
/// output directory.
pub fn write_outputs(output_dir: &Path, fp: &BuildFingerprint) -> anyhow::Result<()> {
    let json_path = output_dir.join("fingerprint.json");
    std::fs::write(&json_path, serde_json::to_string_pretty(fp)?)
        .with_context(|| format!("Failed to write {}", json_path.display()))?;

    let header_path = output_dir.join("include").join("recomp_fingerprint.h");
    std::fs::write(&header_path, emit_header(fp))
        .with_context(|| format!("Failed to write {}", header_path.display()))?;

    tracing::info!(
        "Fingerprint: {} (cross_mid={}) -> {}",
        fp.emitter_sources_hash,
        if fp.flags.cross_mid { 1 } else { 0 },
        json_path.display(),
    );
    Ok(())
}

/// Generated header consumed by `runtime/src/main.cpp` for the boot banner.
pub fn emit_header(fp: &BuildFingerprint) -> String {
    format!(
        "// Generated by `psprecomp recompile` (issue #36). DO NOT EDIT.\n\
         // Identifies the output/ this runtime was built against.\n\
         #pragma once\n\
         #define RECOMP_FINGERPRINT_HASH \"{}\"\n\
         #define RECOMP_FINGERPRINT_CROSS_MID {}\n\
         #define RECOMP_FINGERPRINT_TIMESTAMP \"{}\"\n",
        fp.emitter_sources_hash,
        if fp.flags.cross_mid { 1 } else { 0 },
        fp.generated_at,
    )
}

/// Hash every file in the recipe's file set (module doc, step 1+2).
///
/// Returns entries sorted by repo-root-relative path. Errors if a root
/// directory or extra file is missing — degraded handling is the caller's
/// decision.
pub fn collect_source_hashes(
    repo_root: &Path,
    roots: &[&str],
    extra_files: &[&str],
) -> anyhow::Result<Vec<FileHash>> {
    let mut rel_paths: Vec<String> = Vec::new();
    for root in roots {
        let dir = repo_root.join(root);
        let mut found = Vec::new();
        walk_rs_files(&dir, &mut found)
            .with_context(|| format!("walking {}", dir.display()))?;
        for abs in found {
            rel_paths.push(rel_unix_path(repo_root, &abs)?);
        }
    }
    for extra in extra_files {
        let abs = repo_root.join(extra);
        anyhow::ensure!(abs.is_file(), "extra source file missing: {}", abs.display());
        rel_paths.push(extra.to_string());
    }
    rel_paths.sort();
    rel_paths.dedup();

    rel_paths
        .into_iter()
        .map(|rel| {
            let sha256 = sha256_file(&repo_root.join(&rel))
                .with_context(|| format!("hashing {rel}"))?;
            Ok(FileHash { path: rel, sha256 })
        })
        .collect()
}

/// Combined hash (module doc, step 3): SHA-256 over `"{path}\n{hash}\n"`
/// concatenated in sorted-path order.
pub fn combined_hash(files: &[FileHash]) -> String {
    let mut sorted: Vec<&FileHash> = files.iter().collect();
    sorted.sort_by(|a, b| a.path.cmp(&b.path));
    let mut hasher = Sha256::new();
    for f in sorted {
        hasher.update(f.path.as_bytes());
        hasher.update(b"\n");
        hasher.update(f.sha256.as_bytes());
        hasher.update(b"\n");
    }
    hex(&hasher.finalize())
}

/// Recursively collect `*.rs` files under `dir` (sorted traversal for
/// reproducible error ordering; final order is fixed by the caller's sort).
fn walk_rs_files(dir: &Path, out: &mut Vec<PathBuf>) -> anyhow::Result<()> {
    let mut entries: Vec<PathBuf> = std::fs::read_dir(dir)
        .with_context(|| format!("read_dir {}", dir.display()))?
        .map(|e| e.map(|e| e.path()))
        .collect::<Result<_, _>>()?;
    entries.sort();
    for path in entries {
        if path.is_dir() {
            walk_rs_files(&path, out)?;
        } else if path.extension().and_then(|e| e.to_str()) == Some("rs") {
            out.push(path);
        }
    }
    Ok(())
}

/// Repo-root-relative path with forward slashes (recipe path format).
fn rel_unix_path(repo_root: &Path, abs: &Path) -> anyhow::Result<String> {
    let rel = abs
        .strip_prefix(repo_root)
        .with_context(|| format!("{} not under {}", abs.display(), repo_root.display()))?;
    let parts: Vec<String> = rel
        .components()
        .map(|c| c.as_os_str().to_string_lossy().into_owned())
        .collect();
    Ok(parts.join("/"))
}

/// SHA-256 of a file's bytes, lowercase hex (streamed; analysis.json is large).
/// `pub(crate)`: the generated syscall table (issue #40) stamps the same
/// analysis hash into its header so table and fingerprint always agree.
pub(crate) fn sha256_file(path: &Path) -> anyhow::Result<String> {
    let mut file = std::fs::File::open(path)
        .with_context(|| format!("open {}", path.display()))?;
    let mut hasher = Sha256::new();
    std::io::copy(&mut file, &mut hasher)?;
    Ok(hex(&hasher.finalize()))
}

fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}

/// Git commit + dirty flag, "unknown"/false when git is unavailable.
fn git_info(repo_root: &Path) -> GitInfo {
    let run = |args: &[&str]| -> Option<String> {
        let out = std::process::Command::new("git")
            .arg("-C")
            .arg(repo_root)
            .args(args)
            .output()
            .ok()?;
        out.status.success().then(|| String::from_utf8_lossy(&out.stdout).trim().to_string())
    };
    let commit = run(&["rev-parse", "--short", "HEAD"]).unwrap_or_else(|| "unknown".into());
    let dirty = run(&["status", "--porcelain"]).map(|s| !s.is_empty()).unwrap_or(false);
    GitInfo { commit, dirty }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Build a throwaway source tree: root/src_a/{one.rs,sub/two.rs,ignore.txt}
    /// + root/extra.rs. Returns the tempdir guard.
    fn make_tree() -> tempfile::TempDir {
        let tmp = tempfile::tempdir().unwrap();
        let src = tmp.path().join("src_a");
        std::fs::create_dir_all(src.join("sub")).unwrap();
        std::fs::write(src.join("one.rs"), "fn one() {}\n").unwrap();
        std::fs::write(src.join("sub/two.rs"), "fn two() {}\n").unwrap();
        std::fs::write(src.join("ignore.txt"), "not rust\n").unwrap();
        std::fs::write(tmp.path().join("extra.rs"), "fn extra() {}\n").unwrap();
        tmp
    }

    fn hash_tree(root: &Path) -> (String, Vec<FileHash>) {
        let files = collect_source_hashes(root, &["src_a"], &["extra.rs"]).unwrap();
        (combined_hash(&files), files)
    }

    #[test]
    fn file_set_is_rs_only_recursive_sorted_with_unix_paths() {
        let tmp = make_tree();
        let (_, files) = hash_tree(tmp.path());
        let paths: Vec<&str> = files.iter().map(|f| f.path.as_str()).collect();
        assert_eq!(paths, vec!["extra.rs", "src_a/one.rs", "src_a/sub/two.rs"]);
    }

    #[test]
    fn combined_hash_is_deterministic_across_runs() {
        let tmp = make_tree();
        let (h1, _) = hash_tree(tmp.path());
        let (h2, _) = hash_tree(tmp.path());
        assert_eq!(h1, h2);
        assert_eq!(h1.len(), 64, "sha256 hex");
    }

    #[test]
    fn combined_hash_is_order_independent() {
        let tmp = make_tree();
        let (h, mut files) = hash_tree(tmp.path());
        files.reverse();
        assert_eq!(combined_hash(&files), h);
    }

    #[test]
    fn combined_hash_changes_when_a_source_file_changes() {
        let tmp = make_tree();
        let (before, _) = hash_tree(tmp.path());
        std::fs::write(
            tmp.path().join("src_a/one.rs"),
            "fn one() {}\n// a meaningful emitter change\n",
        )
        .unwrap();
        let (after, _) = hash_tree(tmp.path());
        assert_ne!(before, after);
    }

    #[test]
    fn combined_hash_changes_when_a_source_file_is_added() {
        let tmp = make_tree();
        let (before, _) = hash_tree(tmp.path());
        std::fs::write(tmp.path().join("src_a/three.rs"), "fn three() {}\n").unwrap();
        let (after, _) = hash_tree(tmp.path());
        assert_ne!(before, after);
    }

    #[test]
    fn missing_root_is_an_error() {
        let tmp = make_tree();
        assert!(collect_source_hashes(tmp.path(), &["nope"], &[]).is_err());
        assert!(collect_source_hashes(tmp.path(), &["src_a"], &["nope.rs"]).is_err());
    }

    fn test_fingerprint(tmp: &tempfile::TempDir) -> BuildFingerprint {
        let files = collect_source_hashes(tmp.path(), &["src_a"], &["extra.rs"]).unwrap();
        let analysis = tmp.path().join("analysis.json");
        std::fs::write(&analysis, "{}").unwrap();
        BuildFingerprint {
            schema_version: SCHEMA_VERSION,
            generated_at: "2026-06-12T00:00:00Z".into(),
            emitter_sources_hash: combined_hash(&files),
            git: GitInfo { commit: "abc1234".into(), dirty: true },
            analysis: AnalysisInfo {
                path: analysis.display().to_string(),
                sha256: sha256_file(&analysis).unwrap(),
            },
            flags: FingerprintFlags { cross_mid: true },
            counts: FingerprintCounts { functions: 14104, mid_entries: 2022, batch_files: 283 },
            hash_recipe: HashRecipe {
                version: HASH_RECIPE_VERSION,
                roots: vec!["src_a".into()],
                extra_files: vec!["extra.rs".into()],
            },
            files,
        }
    }

    #[test]
    fn fingerprint_serializes_with_documented_schema() {
        let tmp = make_tree();
        let fp = test_fingerprint(&tmp);
        let value = serde_json::to_value(&fp).unwrap();
        let obj = value.as_object().unwrap();
        let expected_keys = [
            "schema_version",
            "generated_at",
            "emitter_sources_hash",
            "git",
            "analysis",
            "flags",
            "counts",
            "hash_recipe",
            "files",
        ];
        for key in expected_keys {
            assert!(obj.contains_key(key), "missing top-level key: {key}");
        }
        assert_eq!(obj.len(), expected_keys.len(), "undocumented top-level key present");
        assert_eq!(value["schema_version"], SCHEMA_VERSION);
        assert_eq!(value["flags"]["cross_mid"], true);
        assert_eq!(value["git"]["dirty"], true);
        assert_eq!(value["counts"]["functions"], 14104);
        assert_eq!(value["hash_recipe"]["version"], HASH_RECIPE_VERSION);
        assert_eq!(value["files"][0]["path"], "extra.rs");
        // SHA-256 of "{}" — pins the analysis-hash recipe to raw file bytes.
        assert_eq!(
            value["analysis"]["sha256"],
            "44136fa355b3678a1146ad16f7e8649e94fb4fc21fe77e8310c060f61caaff8a"
        );
    }

    #[test]
    fn header_carries_hash_flags_and_timestamp() {
        let tmp = make_tree();
        let fp = test_fingerprint(&tmp);
        let header = emit_header(&fp);
        assert!(header.contains("#pragma once"));
        assert!(header.contains(&format!(
            "#define RECOMP_FINGERPRINT_HASH \"{}\"",
            fp.emitter_sources_hash
        )));
        assert!(header.contains("#define RECOMP_FINGERPRINT_CROSS_MID 1"));
        assert!(header.contains("#define RECOMP_FINGERPRINT_TIMESTAMP \"2026-06-12T00:00:00Z\""));
    }

    #[test]
    fn write_outputs_creates_json_and_header() {
        let tmp = make_tree();
        let fp = test_fingerprint(&tmp);
        let out = tmp.path().join("output");
        std::fs::create_dir_all(out.join("include")).unwrap();
        write_outputs(&out, &fp).unwrap();
        let json: serde_json::Value =
            serde_json::from_str(&std::fs::read_to_string(out.join("fingerprint.json")).unwrap())
                .unwrap();
        assert_eq!(json["emitter_sources_hash"], fp.emitter_sources_hash);
        let header =
            std::fs::read_to_string(out.join("include").join("recomp_fingerprint.h")).unwrap();
        assert!(header.contains(&fp.emitter_sources_hash));
    }
}
