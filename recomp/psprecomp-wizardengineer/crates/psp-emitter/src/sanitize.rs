//! C++ identifier sanitization for the PSP recompiler emitter.
//!
//! Based directly on PS2Recomp `code_generator.cpp` `sanitizeIdentifierBody`
//! + `isReservedCxxIdentifier` + `kKeywords` (C++20 keyword set).

/// Complete C++20 keyword set — identifiers matching any keyword get the `psp_` prefix.
///
/// Source: PS2Recomp code_generator.cpp kKeywords (verified against ISO C++20 standard).
const CPP_KEYWORDS: &[&str] = &[
    "alignas",
    "alignof",
    "and",
    "and_eq",
    "asm",
    "auto",
    "bitand",
    "bitor",
    "bool",
    "break",
    "case",
    "catch",
    "char",
    "char8_t",
    "char16_t",
    "char32_t",
    "class",
    "compl",
    "concept",
    "const",
    "consteval",
    "constexpr",
    "constinit",
    "const_cast",
    "continue",
    "co_await",
    "co_return",
    "co_yield",
    "decltype",
    "default",
    "delete",
    "do",
    "double",
    "dynamic_cast",
    "else",
    "enum",
    "explicit",
    "export",
    "extern",
    "false",
    "float",
    "for",
    "friend",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "mutable",
    "namespace",
    "new",
    "noexcept",
    "not",
    "not_eq",
    "nullptr",
    "operator",
    "or",
    "or_eq",
    "private",
    "protected",
    "public",
    "register",
    "reinterpret_cast",
    "requires",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "static_assert",
    "static_cast",
    "struct",
    "switch",
    "template",
    "this",
    "thread_local",
    "throw",
    "true",
    "try",
    "typedef",
    "typeid",
    "typename",
    "union",
    "unsigned",
    "using",
    "virtual",
    "void",
    "volatile",
    "wchar_t",
    "while",
    "xor",
    "xor_eq",
];

/// Sanitize a raw name into a valid C++ identifier.
///
/// Rules applied in order:
/// 1. Replace non-alphanumeric, non-underscore characters with `_`.
/// 2. If the result starts with a digit, prepend `_`.
/// 3. If the result starts with `__` or `_[A-Z]` (reserved by C++ standard), prepend `psp_`.
/// 4. If the result is a C++ keyword, prepend `psp_`.
///
/// # Examples
///
/// ```
/// use psp_emitter::sanitize_identifier;
/// assert_eq!(sanitize_identifier("break"), "psp_break");
/// assert_eq!(sanitize_identifier("FUN_08804000"), "FUN_08804000");
/// assert_eq!(sanitize_identifier("__foo"), "psp___foo");
/// assert_eq!(sanitize_identifier("1func"), "_1func");
/// assert_eq!(sanitize_identifier("func.name"), "func_name");
/// ```
pub fn sanitize_identifier(name: &str) -> String {
    // Step 1: replace non-alphanumeric / non-underscore with '_'
    let mut s: String = name
        .chars()
        .map(|c| if c.is_ascii_alphanumeric() || c == '_' { c } else { '_' })
        .collect();

    // Step 2: if starts with a digit, prepend '_'
    if s.starts_with(|c: char| c.is_ascii_digit()) {
        s.insert(0, '_');
    }

    // Step 3: reserved C++ identifiers — starts with __ or _[A-Z]
    let is_reserved = s.starts_with("__")
        || (s.starts_with('_')
            && s.chars().nth(1).map_or(false, |c| c.is_ascii_uppercase()));
    if is_reserved {
        s.insert_str(0, "psp_");
    }

    // Step 4: C++ keyword collision
    if CPP_KEYWORDS.contains(&s.as_str()) {
        s.insert_str(0, "psp_");
    }

    s
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_keyword_sanitized() {
        assert_eq!(sanitize_identifier("break"), "psp_break");
    }

    #[test]
    fn test_double_underscore() {
        assert_eq!(sanitize_identifier("__foo"), "psp___foo");
    }

    #[test]
    fn test_leading_digit() {
        assert_eq!(sanitize_identifier("1func"), "_1func");
    }

    #[test]
    fn test_normal_name_unchanged() {
        assert_eq!(sanitize_identifier("FUN_08804000"), "FUN_08804000");
    }

    #[test]
    fn test_special_chars() {
        assert_eq!(sanitize_identifier("func.name"), "func_name");
    }

    #[test]
    fn test_underscore_upper_reserved() {
        assert_eq!(sanitize_identifier("_Foo"), "psp__Foo");
    }

    #[test]
    fn test_switch_keyword() {
        assert_eq!(sanitize_identifier("switch"), "psp_switch");
    }

    #[test]
    fn test_normal_underscore_lower_unchanged() {
        assert_eq!(sanitize_identifier("_foo"), "_foo");
    }
}
