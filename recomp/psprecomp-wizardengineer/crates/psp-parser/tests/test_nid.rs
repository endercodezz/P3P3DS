use psp_parser::nid::{load_nid_database, resolve_nid};
use std::path::PathBuf;

/// Default NID database location, relative to this crate.
/// Override with the `PSP_NIDDB_PATH` environment variable.
const DEFAULT_NIDDB_PATH: &str =
    concat!(env!("CARGO_MANIFEST_DIR"), "/../../data/niddb/ppsspp_niddb.xml");

fn niddb_path() -> PathBuf {
    std::env::var_os("PSP_NIDDB_PATH")
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from(DEFAULT_NIDDB_PATH))
}

#[test]
fn nid_db_loads_and_resolves_sentinels() {
    let path = niddb_path();
    if !path.exists() {
        eprintln!("SKIP: NID DB not found at {}", path.display());
        return;
    }
    let db = load_nid_database(&path).expect("NID DB must parse");
    assert!(!db.is_empty(), "NID DB must not be empty");
    // Verify the two critical sentinel NIDs
    assert_eq!(
        resolve_nid(&db, 0xD632ACDB),
        "module_start",
        "module_start NID must resolve correctly"
    );
    assert_eq!(
        resolve_nid(&db, 0xCEE8593C),
        "module_stop",
        "module_stop NID must resolve correctly"
    );
}

#[test]
fn unknown_nid_produces_fallback_string() {
    use std::collections::HashMap;
    let db: HashMap<u32, String> = HashMap::new();
    assert_eq!(resolve_nid(&db, 0xDEADBEEF), "NID_0xDEADBEEF");
}
