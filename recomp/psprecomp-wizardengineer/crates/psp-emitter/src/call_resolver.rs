//! Two-level call resolution for jal/jalr/jr sites.
//!
//! Resolution priority:
//! 1. NID import stub address → HLE stub call (NID resolved function name)
//! 2. Statically-known function in any section → direct call by cpp_name
//! 3. Unknown / indirect → RECOMP_LOOKUP dispatch
//!
//! Source: RESEARCH.md §Pattern 5; N64Recomp resolve_jal() pattern.

use std::collections::HashMap;
use psp_parser::analysis_json::{JsonFunction, JsonImport};
use crate::sanitize::sanitize_identifier;

#[derive(Debug, Clone, PartialEq)]
pub enum CallTarget {
    /// Call a HLE firmware stub by name (NID-resolved import).
    Hle(String),
    /// Direct call to a statically-known recompiled function.
    Direct(String),
    /// Indirect call via RECOMP_LOOKUP dispatch table.
    Lookup(u32),
}

/// Build the import stub address map from analysis.json imports.
///
/// Returns a map from stub_addr (as u32) to HLE function name.
pub fn build_import_map(imports: &[JsonImport]) -> HashMap<u32, String> {
    imports.iter().filter_map(|imp| {
        let addr: u32 = u64::from_str_radix(
            imp.stub_addr.trim_start_matches("0x"), 16
        ).ok()? as u32;
        Some((addr, imp.name.clone()))
    }).collect()
}

/// Build the function address map for direct call resolution.
///
/// Returns a map from vaddr (as u32) to sanitized C++ function name.
pub fn build_func_map(functions: &[JsonFunction]) -> HashMap<u32, String> {
    functions.iter().filter_map(|func| {
        let addr: u32 = u64::from_str_radix(
            func.address.trim_start_matches("0x"), 16
        ).ok()? as u32;
        Some((addr, sanitize_identifier(&func.name)))
    }).collect()
}

/// Resolve a call target virtual address to the appropriate call form.
///
/// `target_vaddr`: the jal/jalr/jr target address.
/// `import_map`: stub_addr → HLE name (from `build_import_map`).
/// `func_map`: vaddr → cpp_name (from `build_func_map`).
pub fn resolve_call(
    target_vaddr: u32,
    import_map: &HashMap<u32, String>,
    func_map: &HashMap<u32, String>,
) -> CallTarget {
    // Priority 1: NID import stub
    if let Some(hle_name) = import_map.get(&target_vaddr) {
        return CallTarget::Hle(hle_name.clone());
    }
    // Priority 2: Statically known function
    if let Some(cpp_name) = func_map.get(&target_vaddr) {
        return CallTarget::Direct(cpp_name.clone());
    }
    // Priority 3: Unknown — use dispatch table at runtime
    CallTarget::Lookup(target_vaddr)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn make_import(stub_addr: &str, name: &str) -> JsonImport {
        JsonImport {
            nid: "0x00000000".into(),
            stub_addr: stub_addr.into(),
            name: name.into(),
            module_name: "test".into(),
        }
    }

    fn make_func(addr: &str, name: &str) -> JsonFunction {
        JsonFunction {
            name: name.into(),
            address: addr.into(),
            size: 16,
            is_external: false,
            is_thunk: false,
            source: "ghidra".into(),
        }
    }

    #[test]
    fn resolves_hle_import() {
        let imports = [make_import("0x08804000", "sceKernelSleep")];
        let import_map = build_import_map(&imports);
        let func_map = HashMap::new();
        let result = resolve_call(0x08804000, &import_map, &func_map);
        assert_eq!(result, CallTarget::Hle("sceKernelSleep".into()));
    }

    #[test]
    fn resolves_direct_call() {
        let funcs = [make_func("0x08810000", "FUN_08810000")];
        let func_map = build_func_map(&funcs);
        let import_map = HashMap::new();
        let result = resolve_call(0x08810000, &import_map, &func_map);
        assert_eq!(result, CallTarget::Direct("FUN_08810000".into()));
    }

    #[test]
    fn resolves_lookup_for_unknown() {
        let result = resolve_call(0x08899999, &HashMap::new(), &HashMap::new());
        assert_eq!(result, CallTarget::Lookup(0x08899999));
    }

    #[test]
    fn import_takes_priority_over_func() {
        // Same address in both maps — import wins
        let imports = [make_import("0x08804000", "sceKernelSleep")];
        let funcs = [make_func("0x08804000", "FUN_08804000")];
        let import_map = build_import_map(&imports);
        let func_map = build_func_map(&funcs);
        let result = resolve_call(0x08804000, &import_map, &func_map);
        assert_eq!(result, CallTarget::Hle("sceKernelSleep".into()));
    }
}
