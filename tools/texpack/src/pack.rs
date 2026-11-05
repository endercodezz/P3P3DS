//! PPSSPP texture pack (`textures.ini` + image files) as P3P packs ship it.
//!
//! The format is reproduced from PPSSPP's documented behaviour
//! (references/ppsspp/GPU/Common/TextureReplacer.cpp is a behavioural
//! reference only, AGENTS.md section 4): `[options] hash` is required;
//! `[hashes]` maps `<cachekey: up to 16 hex><hash: up to 8 hex>[_<mip level>]`
//! to a file name (empty or `|` = "do not replace"); files named after a key in
//! the pack root are entries too. Path traversal (`..`) is rejected.

use std::collections::BTreeMap;
use std::fmt;
use std::fs;
use std::path::{Path, PathBuf};

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct Key {
    pub cachekey: u64,
    pub hash: u32,
}

impl fmt::Display for Key {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:016x}{:08x}", self.cachekey, self.hash)
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum HashKind {
    Quick,
    Xxh32,
    Xxh64,
}

#[derive(Debug, Default)]
pub struct Options {
    pub hash: Option<HashKind>,
    pub ignore_address: bool,
    pub reduce_hash: bool,
    pub ignore_mipmap: bool,
    pub version: Option<i64>,
}

/// One replacement: file per mip level (None = "do not replace").
#[derive(Debug, Default)]
pub struct Entry {
    pub levels: BTreeMap<u32, Option<String>>,
}

#[derive(Debug, Default)]
pub struct Pack {
    pub options: Options,
    pub entries: BTreeMap<Key, Entry>,
    /// Sections other than options/hashes, with their line counts.
    pub other_sections: BTreeMap<String, usize>,
    pub warnings: Vec<String>,
}

#[derive(Debug)]
pub enum PackError {
    Io(PathBuf, std::io::Error),
    NoHashType,
    BadHashType(String),
}

impl fmt::Display for PackError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            PackError::Io(p, e) => write!(f, "{}: {e}", p.display()),
            PackError::NoHashType => f.write_str("textures.ini: [options] hash is not set"),
            PackError::BadHashType(h) => write!(f, "textures.ini: unsupported hash type '{h}'"),
        }
    }
}

/// Parses `<cachekey><hash>[_level]` like PPSSPP's `%16llx%8x_%d`: up to 16 hex
/// digits, then up to 8, then an optional level. Returns None for anything else.
pub fn parse_key(text: &str) -> Option<(Key, u32)> {
    let (keypart, level) = match text.split_once('_') {
        Some((k, l)) if !l.is_empty() && l.bytes().all(|b| b.is_ascii_digit()) => (k, l.parse().ok()?),
        Some(_) => return None,
        None => (text, 0),
    };
    if keypart.is_empty() || keypart.len() > 24 || !keypart.bytes().all(|b| b.is_ascii_hexdigit()) {
        return None;
    }
    let (ck, h) = keypart.split_at(keypart.len().min(16));
    let cachekey = u64::from_str_radix(ck, 16).ok()?;
    let hash = if h.is_empty() { 0 } else { u32::from_str_radix(h, 16).ok()? };
    Some((Key { cachekey, hash }, level))
}

fn parse_bool(v: &str) -> Option<bool> {
    match v.to_ascii_lowercase().as_str() {
        "true" | "1" | "yes" | "on" => Some(true),
        "false" | "0" | "no" | "off" => Some(false),
        _ => None,
    }
}

fn has_parent_component(path: &str) -> bool {
    path.split('/').any(|c| c == "..")
}

/// Parses textures.ini text. `root_files` are the file names in the pack root
/// (for hash-named entries); pass an empty slice to skip them.
pub fn parse_ini(text: &str, root_files: &[String]) -> Result<Pack, PackError> {
    let mut pack = Pack::default();
    let mut section = String::new();
    let mut hash_value: Option<String> = None;
    let text = text.strip_prefix('\u{feff}').unwrap_or(text);
    for (n, raw) in text.lines().enumerate() {
        let line = raw.trim();
        if line.is_empty() || line.starts_with('#') || line.starts_with(';') {
            continue;
        }
        if let Some(name) = line.strip_prefix('[').and_then(|l| l.strip_suffix(']')) {
            section = name.trim().to_ascii_lowercase();
            continue;
        }
        let Some((key, value)) = line.split_once('=') else {
            pack.warnings.push(format!("line {}: not 'key = value', ignored", n + 1));
            continue;
        };
        let (key, value) = (key.trim(), value.trim());
        match section.as_str() {
            "options" => match key.to_ascii_lowercase().as_str() {
                "hash" => hash_value = Some(value.to_string()),
                "ignoreaddress" | "reducehash" | "ignoremipmap" => match parse_bool(value) {
                    Some(b) => match key.to_ascii_lowercase().as_str() {
                        "ignoreaddress" => pack.options.ignore_address = b,
                        "reducehash" => pack.options.reduce_hash = b,
                        _ => pack.options.ignore_mipmap = b,
                    },
                    None => pack.warnings.push(format!("line {}: '{key}' is not a boolean, ignored", n + 1)),
                },
                "version" => pack.options.version = value.parse().ok(),
                _ => pack.warnings.push(format!("line {}: option '{key}' not used by p3p-texpack", n + 1)),
            },
            "hashes" => {
                let Some((k, level)) = parse_key(key) else {
                    pack.warnings.push(format!("line {}: unsupported key '{key}', ignored", n + 1));
                    continue;
                };
                let file = value.replace('\\', "/");
                if has_parent_component(&file) {
                    pack.warnings.push(format!("line {}: '{file}' leaves the pack directory, ignored", n + 1));
                    continue;
                }
                let file = if file.is_empty() || file == "|" { None } else { Some(file) };
                pack.entries.entry(k).or_default().levels.insert(level, file);
            }
            other => *pack.other_sections.entry(other.to_string()).or_default() += 1,
        }
    }
    let hash = hash_value.ok_or(PackError::NoHashType)?;
    pack.options.hash = Some(match hash.to_ascii_lowercase().as_str() {
        "quick" => HashKind::Quick,
        "xxh32" => HashKind::Xxh32,
        "xxh64" => HashKind::Xxh64,
        _ => return Err(PackError::BadHashType(hash)),
    });
    if pack.options.hash == Some(HashKind::Quick) && (pack.options.reduce_hash || pack.options.ignore_address) {
        pack.warnings.push("reduceHash/ignoreAddress need xxh32 or xxh64; PPSSPP turns them off with 'quick'".into());
        pack.options.reduce_hash = false;
        pack.options.ignore_address = false;
    }
    // Hash-named files in the root: <24 hex>[_level].<ext>. [hashes] lines win.
    for name in root_files {
        let Some((stem, _ext)) = name.rsplit_once('.') else { continue };
        let plausible = stem.len() == 24 || (stem.len() >= 26 && stem.len() <= 27 && stem.as_bytes()[24] == b'_');
        if !plausible {
            continue;
        }
        if let Some((k, level)) = parse_key(stem) {
            pack.entries.entry(k).or_default().levels.entry(level).or_insert_with(|| Some(name.clone()));
        }
    }
    Ok(pack)
}

/// Reads `<dir>/textures.ini` and lists the root for hash-named files.
pub fn load(dir: &Path) -> Result<Pack, PackError> {
    let ini_path = dir.join("textures.ini");
    let bytes = fs::read(&ini_path).map_err(|e| PackError::Io(ini_path.clone(), e))?;
    let text = String::from_utf8_lossy(&bytes);
    let mut root = Vec::new();
    for entry in fs::read_dir(dir).map_err(|e| PackError::Io(dir.to_path_buf(), e))? {
        let entry = entry.map_err(|e| PackError::Io(dir.to_path_buf(), e))?;
        if entry.file_type().map(|t| t.is_file()).unwrap_or(false) {
            root.push(entry.file_name().to_string_lossy().into_owned());
        }
    }
    root.sort();
    parse_ini(&text, &root)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn key_forms() {
        let (k, l) = parse_key("0000000012345678abcdef01").unwrap();
        assert_eq!((k.cachekey, k.hash, l), (0x12345678, 0xabcdef01, 0));
        let (k, l) = parse_key("0000000012345678abcdef01_2").unwrap();
        assert_eq!((k.hash, l), (0xabcdef01, 2));
        // Shorter keys parse like sscanf: the first 16 digits are the cache key.
        let (k, _) = parse_key("00000000123456").unwrap();
        assert_eq!((k.cachekey, k.hash), (0x123456, 0));
        assert!(parse_key("").is_none());
        assert!(parse_key("xyz").is_none());
        assert!(parse_key("0000000012345678abcdef01_").is_none());
        assert!(parse_key("0000000012345678abcdef01_a").is_none());
        assert!(parse_key("0000000012345678abcdef0123").is_none());
    }

    #[test]
    fn parses_typical_pack() {
        let ini = "\u{feff}# comment\n[options]\nhash = xxh64\nignoreAddress = true\nversion = 1\n\n[hashes]\n\
                   0000000000000000abcdef01 = ui/menu.png\n\
                   0000000000000000abcdef01_1 = ui\\menu_1.png\n\
                   000000000000000011111111 = |\n\
                   000000000000000022222222 = ../../etc/passwd\n\
                   garbage = x.png\n[hashranges]\n08800000,512,512 = 480,272\n";
        let roots = vec!["0000000000000000333333334.png".to_string(), "000000000000000033333333.png".to_string()];
        let pack = parse_ini(ini, &roots).unwrap();
        assert_eq!(pack.options.hash, Some(HashKind::Xxh64));
        assert!(pack.options.ignore_address);
        assert_eq!(pack.options.version, Some(1));
        let k = Key { cachekey: 0, hash: 0xabcdef01 };
        let e = &pack.entries[&k];
        assert_eq!(e.levels[&0].as_deref(), Some("ui/menu.png"));
        assert_eq!(e.levels[&1].as_deref(), Some("ui/menu_1.png"));
        assert_eq!(pack.entries[&Key { cachekey: 0, hash: 0x11111111 }].levels[&0], None);
        assert!(!pack.entries.contains_key(&Key { cachekey: 0, hash: 0x22222222 }));
        assert_eq!(
            pack.entries[&Key { cachekey: 0, hash: 0x33333333 }].levels[&0].as_deref(),
            Some("000000000000000033333333.png")
        );
        assert_eq!(pack.other_sections["hashranges"], 1);
        assert_eq!(pack.warnings.len(), 2, "{:?}", pack.warnings);
    }

    #[test]
    fn requires_known_hash_type() {
        assert!(matches!(parse_ini("[hashes]\n", &[]), Err(PackError::NoHashType)));
        assert!(matches!(parse_ini("[options]\nhash = md5\n", &[]), Err(PackError::BadHashType(_))));
        let quick = parse_ini("[options]\nhash = quick\nreduceHash = true\n", &[]).unwrap();
        assert!(!quick.options.reduce_hash);
    }
}
