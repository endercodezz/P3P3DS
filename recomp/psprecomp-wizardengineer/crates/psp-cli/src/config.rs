//! Per-game manifest (`games/<id>/game.toml`) — TOML game configuration.
//!
//! Passed via `--config` (the manifest IS the config file, issue #46 P14).
//! Every key is optional; absence means "use the generic default". A
//! well-behaved game should recompile and boot with NO manifest at all —
//! the manifest exists for curation, never for bring-up table stakes.
//!
//! Schema (full sketch in `.planning/plans/46-47-runtime-portability-plan.md` §4):
//!
//!   [game]
//!   id    = "patapon"                 # compiled into recomp_game_config.h
//!   title = "Patapon (UCUS-98643)"    # informational only
//!
//!   [module]
//!   heap_base = "0x09000000"          # overrides the RECOMP_HEAP_BASE align policy
//!
//!   [boot]
//!   boot_path = "disc0:/PSP_GAME/SYSDIR/BOOT.BIN"   # default; EBOOT games override
//!
//!   [recompile]
//!   force_entries           = ["0x0887E6C0"]   # function starts Ghidra merged away
//!   force_entries_cross_mid = ["0x08827470"]   # only active under PSPRECOMP_CROSS_MID=1
//!   [[recompile.force_mid_entries]]            # mid-entries Ghidra missed
//!   entry  = "0x08827F44"
//!   parent = "0x08827E7C"
//!
//!   [runtime]
//!   asset_layer = "bnd"               # none|bnd ; default "none"
//!
//!   # Existing arrays keep working at top level:
//!   [[stubs]]   name = "sceUtilityGetSystemParamInt"
//!   [[skips]]   address = "0x08804000"  reason = "..."
//!   [[patches]] address = "0x08810004"  instruction = "0x00000000"  reason = "..."
//!
//! `[game]`/`[boot]`/`[module]`/`[runtime]` flow into the generated
//! `output/include/recomp_game_config.h`; the runtime never parses TOML.

use serde::Deserialize;

/// Top-level game configuration loaded from a TOML manifest.
#[derive(Debug, Clone, Default, Deserialize)]
pub struct GameConfig {
    /// `[game]` identity table (id compiled into recomp_game_config.h).
    #[serde(default)]
    pub game: Option<GameSection>,
    /// `[module]` overrides of analysis.json-derived facts.
    #[serde(default)]
    pub module: Option<ModuleSection>,
    /// `[boot]` boot-path override.
    #[serde(default)]
    pub boot: Option<BootSection>,
    /// `[runtime]` runtime-layer choices.
    #[serde(default)]
    pub runtime: Option<RuntimeSection>,
    /// `[recompile]` curated emission inputs (force entries).
    #[serde(default)]
    pub recompile: Option<RecompileSection>,
    /// Functions to replace with no-op HLE stubs (by name).
    #[serde(default)]
    pub stubs: Vec<StubEntry>,
    /// Functions to skip entirely (emitted as empty stubs).
    #[serde(default)]
    pub skips: Vec<SkipEntry>,
    /// Instruction-level patches (override a specific address with a new word).
    #[serde(default)]
    pub patches: Vec<PatchEntry>,
    /// Overrides for batch size (default: 50 or CLI --batch-size).
    #[serde(default)]
    pub functions_per_file: Option<usize>,
}

/// `[game]` — manifest identity.
#[derive(Debug, Clone, Default, Deserialize)]
pub struct GameSection {
    /// Game id; must match the `-DPSPRECOMP_GAME=<id>` runtime build.
    #[serde(default)]
    pub id: String,
    /// Informational title; never consumed programmatically.
    #[serde(default)]
    #[allow(dead_code)]
    pub title: String,
}

/// `[module]` — overrides of analysis.json-derived module facts.
#[derive(Debug, Clone, Default, Deserialize)]
pub struct ModuleSection {
    /// Hex address pinning the runtime heap base (overrides the
    /// align-to-16MB RECOMP_HEAP_BASE policy, plan decision P11/O1).
    #[serde(default)]
    pub heap_base: Option<String>,
}

/// `[boot]` — boot-path override.
#[derive(Debug, Clone, Default, Deserialize)]
pub struct BootSection {
    /// Guest exec path passed to module_start (EBOOT games override).
    #[serde(default)]
    pub boot_path: Option<String>,
}

/// `[runtime]` — runtime-layer choices.
#[derive(Debug, Clone, Default, Deserialize)]
pub struct RuntimeSection {
    /// Asset layer selection: "none" (default) or "bnd".
    #[serde(default)]
    pub asset_layer: Option<String>,
}

/// `[recompile]` — curated per-game emission inputs.
#[derive(Debug, Clone, Default, Deserialize)]
pub struct RecompileSection {
    /// Function starts Ghidra merged into larger functions (hex strings).
    /// Replaces the former hardcoded FORCE_ENTRIES const (issue #46).
    #[serde(default)]
    pub force_entries: Vec<String>,
    /// Force entries only active under PSPRECOMP_CROSS_MID=1 (former
    /// FORCE_ENTRIES_D2 const — see the D2/19G/19H notes in game.toml).
    #[serde(default)]
    pub force_entries_cross_mid: Vec<String>,
    /// Mid-entries Ghidra missed: {entry, parent} hex-address pairs
    /// (former FORCE_MID_ENTRIES const).
    #[serde(default)]
    pub force_mid_entries: Vec<ForceMidEntry>,
}

/// One forced mid-entry: an entry address inside an existing parent function.
#[derive(Debug, Clone, Deserialize)]
pub struct ForceMidEntry {
    /// Mid-entry address (hex string).
    pub entry: String,
    /// Parent function start address (hex string).
    pub parent: String,
}

/// A function name to replace with an HLE no-op stub.
#[derive(Debug, Clone, Deserialize)]
pub struct StubEntry {
    /// Function name as it appears in analysis.json.
    pub name: String,
}

/// A function address to skip entirely (emitted as an empty stub).
#[derive(Debug, Clone, Deserialize)]
pub struct SkipEntry {
    /// Hex address string, e.g. "0x08804000".
    pub address: String,
    #[serde(default)]
    #[allow(dead_code)]
    pub reason: String,
}

/// An instruction-level patch replacing a word at a specific address.
#[derive(Debug, Clone, Deserialize)]
pub struct PatchEntry {
    /// Hex address string.
    pub address: String,
    /// Replacement instruction word as hex string.
    pub instruction: String,
    #[serde(default)]
    #[allow(dead_code)]
    pub reason: String,
}

impl GameConfig {
    /// Game id ("" with no manifest / no `[game]` table).
    pub fn game_id(&self) -> &str {
        self.game.as_ref().map(|g| g.id.as_str()).unwrap_or("")
    }

    /// Boot exec path (PSP-universal default when unset).
    pub fn boot_path(&self) -> &str {
        self.boot
            .as_ref()
            .and_then(|b| b.boot_path.as_deref())
            .unwrap_or("disc0:/PSP_GAME/SYSDIR/BOOT.BIN")
    }

    /// Manifest heap-base pin, parsed (0 = no override; use the align policy).
    pub fn heap_override(&self) -> anyhow::Result<u32> {
        match self.module.as_ref().and_then(|m| m.heap_base.as_deref()) {
            None => Ok(0),
            Some(s) => parse_hex(s)
                .ok_or_else(|| anyhow::anyhow!("[module] heap_base is not a hex address: {s:?}")),
        }
    }

    /// True when `[runtime] asset_layer = "bnd"`.
    pub fn asset_layer_is_bnd(&self) -> anyhow::Result<bool> {
        match self.runtime.as_ref().and_then(|r| r.asset_layer.as_deref()) {
            None | Some("none") => Ok(false),
            Some("bnd") => Ok(true),
            Some(other) => anyhow::bail!(
                "[runtime] asset_layer must be \"none\" or \"bnd\", got {other:?}"
            ),
        }
    }

    /// `[recompile] force_entries`, parsed.
    pub fn force_entries(&self) -> anyhow::Result<Vec<u32>> {
        parse_hex_list(
            self.recompile.as_ref().map(|r| r.force_entries.as_slice()).unwrap_or(&[]),
            "force_entries",
        )
    }

    /// `[recompile] force_entries_cross_mid`, parsed.
    pub fn force_entries_cross_mid(&self) -> anyhow::Result<Vec<u32>> {
        parse_hex_list(
            self.recompile
                .as_ref()
                .map(|r| r.force_entries_cross_mid.as_slice())
                .unwrap_or(&[]),
            "force_entries_cross_mid",
        )
    }

    /// `[recompile] force_mid_entries` as (entry, parent) pairs, parsed.
    pub fn force_mid_entries(&self) -> anyhow::Result<Vec<(u32, u32)>> {
        let entries = self
            .recompile
            .as_ref()
            .map(|r| r.force_mid_entries.as_slice())
            .unwrap_or(&[]);
        entries
            .iter()
            .map(|fme| {
                let entry = parse_hex(&fme.entry).ok_or_else(|| {
                    anyhow::anyhow!("force_mid_entries entry is not hex: {:?}", fme.entry)
                })?;
                let parent = parse_hex(&fme.parent).ok_or_else(|| {
                    anyhow::anyhow!("force_mid_entries parent is not hex: {:?}", fme.parent)
                })?;
                Ok((entry, parent))
            })
            .collect()
    }
}

/// Parse "0x..."/plain-hex address strings (manifest convention: hex strings).
fn parse_hex(s: &str) -> Option<u32> {
    let t = s.trim().trim_start_matches("0x").trim_start_matches("0X");
    u32::from_str_radix(t, 16).ok()
}

fn parse_hex_list(items: &[String], what: &str) -> anyhow::Result<Vec<u32>> {
    items
        .iter()
        .map(|s| {
            parse_hex(s)
                .ok_or_else(|| anyhow::anyhow!("[recompile] {what} entry is not hex: {s:?}"))
        })
        .collect()
}

/// Load game config from a TOML file path. Returns default config if path is None.
pub fn load_config(path: Option<&std::path::Path>) -> anyhow::Result<GameConfig> {
    match path {
        None => Ok(GameConfig::default()),
        Some(p) => {
            let text = std::fs::read_to_string(p)
                .map_err(|e| anyhow::anyhow!("Cannot read config {}: {e}", p.display()))?;
            let cfg: GameConfig = toml::from_str(&text)
                .map_err(|e| anyhow::anyhow!("Invalid TOML in {}: {e}", p.display()))?;
            Ok(cfg)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn empty_toml_parses_to_defaults() {
        let cfg: GameConfig = toml::from_str("").unwrap();
        assert!(cfg.stubs.is_empty());
        assert!(cfg.skips.is_empty());
        assert!(cfg.patches.is_empty());
        assert!(cfg.functions_per_file.is_none());
        assert_eq!(cfg.game_id(), "");
        assert_eq!(cfg.boot_path(), "disc0:/PSP_GAME/SYSDIR/BOOT.BIN");
        assert_eq!(cfg.heap_override().unwrap(), 0);
        assert!(!cfg.asset_layer_is_bnd().unwrap());
        assert!(cfg.force_entries().unwrap().is_empty());
        assert!(cfg.force_entries_cross_mid().unwrap().is_empty());
        assert!(cfg.force_mid_entries().unwrap().is_empty());
    }

    #[test]
    fn stub_entry_parsed() {
        let toml = r#"
        [[stubs]]
        name = "sceKernelSleep"
        "#;
        let cfg: GameConfig = toml::from_str(toml).unwrap();
        assert_eq!(cfg.stubs.len(), 1);
        assert_eq!(cfg.stubs[0].name, "sceKernelSleep");
    }

    #[test]
    fn functions_per_file_override() {
        let toml = "functions_per_file = 100\n";
        let cfg: GameConfig = toml::from_str(toml).unwrap();
        assert_eq!(cfg.functions_per_file, Some(100));
    }

    #[test]
    fn manifest_tables_parsed() {
        let toml = r#"
        [game]
        id = "patapon"
        title = "Patapon (UCUS-98643)"

        [module]
        heap_base = "0x09000000"

        [boot]
        boot_path = "disc0:/PSP_GAME/SYSDIR/EBOOT.BIN"

        [runtime]
        asset_layer = "bnd"

        [recompile]
        force_entries = ["0x0887E6C0"]
        force_entries_cross_mid = ["0x08827470", "0x0882744C"]

        [[recompile.force_mid_entries]]
        entry = "0x08827F44"
        parent = "0x08827E7C"
        "#;
        let cfg: GameConfig = toml::from_str(toml).unwrap();
        assert_eq!(cfg.game_id(), "patapon");
        assert_eq!(cfg.boot_path(), "disc0:/PSP_GAME/SYSDIR/EBOOT.BIN");
        assert_eq!(cfg.heap_override().unwrap(), 0x0900_0000);
        assert!(cfg.asset_layer_is_bnd().unwrap());
        assert_eq!(cfg.force_entries().unwrap(), vec![0x0887_E6C0]);
        assert_eq!(
            cfg.force_entries_cross_mid().unwrap(),
            vec![0x0882_7470, 0x0882_744C]
        );
        assert_eq!(
            cfg.force_mid_entries().unwrap(),
            vec![(0x0882_7F44, 0x0882_7E7C)]
        );
    }

    #[test]
    fn bad_asset_layer_rejected() {
        let toml = "[runtime]\nasset_layer = \"zip\"\n";
        let cfg: GameConfig = toml::from_str(toml).unwrap();
        assert!(cfg.asset_layer_is_bnd().is_err());
    }

    #[test]
    fn bad_force_entry_rejected() {
        let toml = "[recompile]\nforce_entries = [\"not-hex\"]\n";
        let cfg: GameConfig = toml::from_str(toml).unwrap();
        assert!(cfg.force_entries().is_err());
    }
}
