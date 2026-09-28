//! PRX load-time plumbing for `analyze` (issue #52 T5).
//!
//! Load-base computation, Type-A relocation discovery/application, hard-error
//! import parsing from the relocated image, and the post-Ghidra merge fixes
//! (module-entry rename, out-of-image jal-artifact filter). Every helper is a
//! deliberate no-op for ET_EXEC inputs (`load_base == 0`, `is_prx == false`)
//! so the Patapon byte-identity gate (plan D7) holds.

use anyhow::{bail, Context, Result};
use psp_parser::analysis_json::{JsonFunction, JsonModuleInfo};
use psp_parser::prx::ModuleInfo;
use psp_parser::types::{ImportStub, RelocEntry};
use psp_parser::{elf, imports, prx, reloc};
use std::collections::HashMap;

/// Effective load base for this run (plan D3, computed exactly once).
///
/// Relocatable PRX modules rebase to the CLI override or
/// `prx::PSP_USER_MODULE_BASE`; ET_EXEC binaries load where linked (base 0),
/// and an override is ignored with a warning.
pub fn compute_load_base(is_prx: bool, cli_override: Option<u32>) -> u32 {
    if !is_prx {
        if let Some(base) = cli_override {
            tracing::warn!(
                "--load-base 0x{base:08X} ignored: binary is ET_EXEC and loads where linked"
            );
        }
        return 0;
    }
    cli_override.unwrap_or(prx::PSP_USER_MODULE_BASE)
}

/// Discover and apply Type-A relocation tables onto the rebased segments.
///
/// Bails on Type-B tables (plan D5) and on tables outside the file bounds.
/// `seg_bases` must already be rebased — relocation addends derive from them.
/// Returns the parsed entries for the analysis.json `relocations[]` record.
pub fn apply_prx_relocations(
    raw_data: &[u8],
    elf_obj: &goblin::elf::Elf,
    seg_data_vecs: &mut [Vec<u8>],
    seg_bases: &[u32],
) -> Result<Vec<RelocEntry>> {
    let tables = prx::find_reloc_tables(elf_obj);
    let type_b: Vec<&str> = tables
        .iter()
        .filter(|t| t.format == prx::RelocFormat::TypeB)
        .map(|t| t.source.as_str())
        .collect();
    if !type_b.is_empty() {
        bail!(
            "Type-B (0x700000A1) packed relocations not yet supported (found in {}); \
             see .planning/research/52-prx-format-spec.md §5",
            type_b.join(", ")
        );
    }
    // Apply per table (PPSSPP applies each reloc section independently —
    // HI16 pairing must not scan across table boundaries).
    let mut all_entries = Vec::new();
    let mut stats = reloc::RelocStats::default();
    for t in &tables {
        let end = t
            .file_offset
            .checked_add(t.size)
            .filter(|&end| end <= raw_data.len())
            .with_context(|| {
                format!(
                    "reloc table {} out of file bounds: offset 0x{:X} + size 0x{:X} exceeds \
                     file size 0x{:X}",
                    t.source,
                    t.file_offset,
                    t.size,
                    raw_data.len()
                )
            })?;
        let entries = reloc::parse_type_a_entries(&raw_data[t.file_offset..end])
            .with_context(|| format!("parsing reloc table {}", t.source))?;
        stats.absorb(
            reloc::apply_relocations(seg_data_vecs, seg_bases, &entries)
                .with_context(|| format!("applying reloc table {}", t.source))?,
        );
        all_entries.extend(entries);
    }
    tracing::info!(
        "Applied {} relocations from {} tables ({} skipped, {} unhandled types — \
         see warnings above)",
        stats.handled,
        tables.len(),
        stats.skipped_bad,
        stats.unhandled.len(),
    );
    Ok(all_entries)
}

/// Parse SceModuleInfo + the libstub import table from the relocated image.
///
/// Hard errors on any failure (plan D8): a commercial PRX without imports is
/// not a thing, and silent `imports: []` is exactly the issue #52 bug class.
/// All pointer fields are final addresses because the image is already
/// relocated (plan D6).
pub fn parse_prx_imports(
    elf_obj: &goblin::elf::Elf,
    load_base: u32,
    seg_bases: &[u32],
    seg_data_vecs: &[Vec<u8>],
    nid_map: &HashMap<u32, String>,
) -> Result<(Vec<ImportStub>, ModuleInfo)> {
    let image = psp_parser::image::LoadedImage::new(seg_bases, seg_data_vecs);
    let mi_va = prx::locate_module_info_va(elf_obj, load_base)
        .context("PRX module info location failed")?;
    let mi = prx::parse_module_info(&image, mi_va)
        .with_context(|| format!("PRX import parsing failed (module info at 0x{mi_va:08X})"))?;
    tracing::info!(
        "SceModuleInfo '{}' at 0x{mi_va:08X}: gp=0x{:08X}, libstub 0x{:08X}..0x{:08X}",
        mi.name,
        mi.gp,
        mi.libstub,
        mi.libstub_end
    );
    let stubs = imports::parse_import_stubs(&image, &mi, nid_map)
        .with_context(|| format!("PRX import parsing failed (module info at 0x{mi_va:08X})"))?;
    Ok((stubs, mi))
}

/// Parse SceModuleInfo + the libstub import table for an ET_EXEC image
/// (issue #40 — the generated NID binding table needs `imports[]` for every
/// format, not just PRX).
///
/// Commercial ET_EXEC binaries (Patapon) carry the same SceModuleInfo and
/// `.lib.stub` structures as PRX, so the one libstub walker serves both. A
/// missing/unreadable SceModuleInfo degrades LOUDLY to no imports (mirrors
/// [`build_module_facts`] — synthetic/homebrew ELFs may lack the record, and
/// `recompile` refuses an import-free analysis.json anyway); a record that
/// parses but whose libstub walk fails is a hard error — that is real data
/// with a broken walk, the exact silent-empty-imports bug class of issue #52.
pub fn parse_elf_imports(
    elf_obj: &goblin::elf::Elf,
    seg_bases: &[u32],
    seg_data_vecs: &[Vec<u8>],
    nid_map: &HashMap<u32, String>,
) -> Result<(Vec<ImportStub>, Option<ModuleInfo>)> {
    let image = psp_parser::image::LoadedImage::new(seg_bases, seg_data_vecs);
    let mi = match prx::locate_module_info_va(elf_obj, 0)
        .and_then(|va| prx::parse_module_info(&image, va))
    {
        Ok(mi) => mi,
        Err(e) => {
            tracing::warn!(
                "No usable SceModuleInfo in ET_EXEC ({e}); imports[] stays empty — \
                 a commercial binary should never hit this, and recompile refuses \
                 an import-free analysis.json"
            );
            return Ok((vec![], None));
        }
    };
    tracing::info!(
        "SceModuleInfo '{}': gp=0x{:08X}, libstub 0x{:08X}..0x{:08X}",
        mi.name,
        mi.gp,
        mi.libstub,
        mi.libstub_end
    );
    let stubs = imports::parse_import_stubs(&image, &mi, nid_map)
        .context("ET_EXEC import parsing failed (SceModuleInfo present but libstub walk broke)")?;
    Ok((stubs, Some(mi)))
}

/// Build the analysis.json `module{}` facts block (issue #47 Phase 2).
///
/// name/gp come from SceModuleInfo via the same locate+parse path the
/// import walkers use — Patapon's ET_EXEC BOOT.BIN carries the record too,
/// so both formats are served by one code path. The caller passes the
/// already-parsed record (`parse_prx_imports` / `parse_elf_imports`) when it
/// found one; only an ET_EXEC whose record was unusable re-attempts here and
/// degrades loudly to `name = file stem, gp = 0`. Text extent mirrors
/// PPSSPP's `ElfReader` (see `psp_parser::elf::text_extent`).
pub fn build_module_facts(
    elf_obj: &goblin::elf::Elf,
    image: &psp_parser::image::LoadedImage,
    load_base: u32,
    entry_va: u32,
    prx_mi: Option<&ModuleInfo>,
    file_stem: &str,
) -> JsonModuleInfo {
    let (name, gp) = match prx_mi {
        Some(mi) => (mi.name.clone(), mi.gp),
        None => match prx::locate_module_info_va(elf_obj, load_base)
            .and_then(|va| prx::parse_module_info(image, va))
        {
            Ok(mi) => {
                tracing::info!("SceModuleInfo '{}': gp=0x{:08X}", mi.name, mi.gp);
                (mi.name, mi.gp)
            }
            Err(e) => {
                tracing::warn!(
                    "No usable SceModuleInfo ({e}); module facts degrade to \
                     name=\"{file_stem}\", gp=0x00000000 — if this binary uses \
                     $gp-relative addressing the runtime boot GP will be wrong"
                );
                (file_stem.to_string(), 0)
            }
        },
    };
    let (text_start, text_size) = elf::text_extent(elf_obj, load_base);
    tracing::info!(
        "Module facts: name=\"{name}\" entry=0x{entry_va:08X} gp=0x{gp:08X} \
         text=0x{text_start:08X}+0x{text_size:X}"
    );
    JsonModuleInfo {
        name,
        entry: format!("0x{entry_va:08X}"),
        gp: format!("0x{gp:08X}"),
        text_start: format!("0x{text_start:08X}"),
        text_size: format!("0x{text_size:08X}"),
    }
}

/// Rename the function at the module entry point to "entry" (idempotent).
///
/// The runtime hard-links the `entry` symbol (runtime/src/main.cpp) and the
/// emitter expects it. No-op when a function named "entry" already exists
/// (Patapon: Ghidra already names the e_entry function "entry").
pub fn rename_entry_function(functions: &mut [JsonFunction], entry_va: u32) {
    if functions.iter().any(|f| f.name == "entry") {
        return;
    }
    match functions
        .iter_mut()
        .find(|f| parse_hex_addr(&f.address) == Some(entry_va))
    {
        Some(f) => {
            tracing::info!(
                "Renamed {} at 0x{entry_va:08X} to \"entry\" (module start)",
                f.name
            );
            f.name = "entry".into();
        }
        None => tracing::warn!(
            "No function found at module entry 0x{entry_va:08X}; \"entry\" not assigned"
        ),
    }
}

/// Drop `jal_target` functions outside `[image_start, image_end)`.
///
/// A single stray `jal 0x0` in dead code makes Ghidra's JAL cross-validation
/// synthesize `FUN_00000000` (R3 §4); real functions can only live inside the
/// loaded image. Only the jal_target source is filtered — Ghidra-confirmed
/// functions are never dropped. Returns the number removed.
pub fn drop_out_of_image_jal_targets(
    functions: &mut Vec<JsonFunction>,
    image_start: u32,
    image_end: u32,
) -> usize {
    let before = functions.len();
    functions.retain(|f| {
        if f.source != "jal_target" {
            return true;
        }
        match parse_hex_addr(&f.address) {
            Some(a) if a >= image_start && a < image_end => true,
            Some(a) => {
                tracing::info!(
                    "Dropping jal artifact {} at 0x{a:08X} (outside image \
                     [0x{image_start:08X}, 0x{image_end:08X}))",
                    f.name
                );
                false
            }
            None => true,
        }
    });
    before - functions.len()
}

/// Parse a hex address string like "0x08804000" to u32.
fn parse_hex_addr(s: &str) -> Option<u32> {
    let t = s.trim().trim_start_matches("0x").trim_start_matches("0X");
    u32::from_str_radix(t, 16).ok()
}

#[cfg(test)]
mod tests {
    use super::*;

    fn jf(name: &str, addr: u32, source: &str) -> JsonFunction {
        JsonFunction {
            name: name.into(),
            address: format!("0x{addr:08X}"),
            size: 4,
            is_external: false,
            is_thunk: false,
            source: source.into(),
        }
    }

    #[test]
    fn load_base_default_override_and_et_exec() {
        assert_eq!(compute_load_base(true, None), prx::PSP_USER_MODULE_BASE);
        assert_eq!(compute_load_base(true, Some(0x0900_0000)), 0x0900_0000);
        assert_eq!(compute_load_base(false, None), 0);
        assert_eq!(
            compute_load_base(false, Some(0x0900_0000)),
            0,
            "ignored for ET_EXEC"
        );
    }

    #[test]
    fn entry_rename_is_idempotent_and_targets_the_right_function() {
        let mut funcs = vec![jf("FUN_08BC3D98", 0x08BC3D98, "ghidra")];
        rename_entry_function(&mut funcs, 0x08BC3D98);
        assert_eq!(funcs[0].name, "entry");
        // Second run: no-op ("entry" already exists).
        rename_entry_function(&mut funcs, 0x08BC3D98);
        assert_eq!(funcs.iter().filter(|f| f.name == "entry").count(), 1);
    }

    #[test]
    fn entry_rename_noop_when_entry_already_named_elsewhere() {
        // Patapon shape: Ghidra already named the e_entry function "entry".
        let mut funcs = vec![
            jf("entry", 0x089A_CCD0, "ghidra"),
            jf("FUN_08804000", 0x0880_4000, "ghidra"),
        ];
        rename_entry_function(&mut funcs, 0x089A_CCD0);
        assert_eq!(funcs[0].name, "entry");
        assert_eq!(funcs[1].name, "FUN_08804000");
    }

    #[test]
    fn jal_filter_drops_only_out_of_image_jal_targets() {
        let mut funcs = vec![
            jf("FUN_00000000", 0x0000_0000, "jal_target"), // the R3 §4 artifact
            jf("FUN_08810000", 0x0881_0000, "jal_target"), // in-image: kept
            jf("FUN_00000004", 0x0000_0004, "ghidra"),     // not jal_target: kept
        ];
        let dropped = drop_out_of_image_jal_targets(&mut funcs, 0x0880_4000, 0x08D2_BC80);
        assert_eq!(dropped, 1);
        let names: Vec<&str> = funcs.iter().map(|f| f.name.as_str()).collect();
        assert_eq!(names, ["FUN_08810000", "FUN_00000004"]);
    }
}
