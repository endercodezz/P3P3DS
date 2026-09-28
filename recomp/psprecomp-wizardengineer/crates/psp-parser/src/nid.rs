//! NID database parser — stub for Plan 01-03 Task 1 compilation.
//! Full implementation added in Task 2.
use std::collections::HashMap;
use std::path::Path;
use crate::errors::ParseError;

/// Parse the PPSSPP NID database XML and return a NID-to-name map.
pub fn load_nid_database(xml_path: &Path) -> Result<HashMap<u32, String>, ParseError> {
    let content = std::fs::read_to_string(xml_path)?;
    let doc = roxmltree::Document::parse(&content)?;
    let mut db: HashMap<u32, String> = HashMap::new();
    for node in doc.descendants() {
        if node.tag_name().name() != "FUNCTION" {
            continue;
        }
        let nid_text = node
            .children()
            .find(|n| n.tag_name().name() == "NID")
            .and_then(|n| n.text());
        let name_text = node
            .children()
            .find(|n| n.tag_name().name() == "NAME")
            .and_then(|n| n.text());
        if let (Some(nid_s), Some(name)) = (nid_text, name_text) {
            let trimmed = nid_s.trim().trim_start_matches("0x").trim_start_matches("0X");
            if let Ok(nid) = u32::from_str_radix(trimmed, 16) {
                db.insert(nid, name.trim().to_string());
            }
        }
    }
    Ok(db)
}

/// Returns the function name for a NID, or a hex fallback string if unknown.
///
/// The fallback is "NID_0xXXXXXXXX" — never panics. Unresolved NIDs are
/// detectable downstream via [`parse_fallback_name`]; the recompile report
/// lists them so a database miss is never silent.
pub fn resolve_nid(db: &HashMap<u32, String>, nid: u32) -> String {
    db.get(&nid).cloned().unwrap_or_else(|| fallback_name(nid))
}

/// Format the fallback name used when a NID is missing from the database.
pub fn fallback_name(nid: u32) -> String {
    format!("NID_0x{nid:08X}")
}

/// If `name` is an unresolved-NID fallback produced by [`fallback_name`],
/// return the embedded NID; otherwise `None`.
///
/// Keeps the fallback pattern in one place so report code never hardcodes it.
pub fn parse_fallback_name(name: &str) -> Option<u32> {
    let hex = name.strip_prefix("NID_0x")?;
    if hex.len() != 8 {
        return None;
    }
    u32::from_str_radix(hex, 16).ok()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn database_miss_produces_parseable_fallback() {
        let db: HashMap<u32, String> = HashMap::new();
        let name = resolve_nid(&db, 0xDEAD_BEEF);
        assert_eq!(name, "NID_0xDEADBEEF");
        assert_eq!(parse_fallback_name(&name), Some(0xDEAD_BEEF));
    }

    #[test]
    fn database_hit_is_not_a_fallback() {
        let mut db = HashMap::new();
        db.insert(0x1234_5678u32, "sceKernelCreateThread".to_string());
        let name = resolve_nid(&db, 0x1234_5678);
        assert_eq!(name, "sceKernelCreateThread");
        assert_eq!(parse_fallback_name(&name), None);
    }

    #[test]
    fn fallback_parser_rejects_malformed_names() {
        assert_eq!(parse_fallback_name("NID_0x123"), None); // too short
        assert_eq!(parse_fallback_name("NID_0x123456789"), None); // too long
        assert_eq!(parse_fallback_name("NID_0xZZZZZZZZ"), None); // not hex
        assert_eq!(parse_fallback_name("sceGeListEnQueue"), None);
    }
}
