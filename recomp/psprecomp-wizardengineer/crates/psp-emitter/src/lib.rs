//! C++17 code emitter for the PSP Allegrex recompiler.
//!
//! Architecture:
//! - `Generator` trait: one `emit_*` method per C++ construct
//! - `CppGenerator`: writes C++17 text via `write!` to a `String`
//! - `TestGenerator`: captures calls as `Vec<String>` for unit tests
//!
//! $zero suppression: `emit_gpr_write(Reg::Zero, _)` is always a no-op.
//! All `emit_gpr_read(Reg::Zero)` calls return literal `"0"`.
pub mod generator;
pub mod cpp_generator;
pub mod function;
pub(crate) mod vfpu;
pub mod sanitize;
// Output modules (added in Plan 02-05):
pub mod dispatch;
pub mod data;
pub mod init_array;
pub mod cmake;
pub mod call_resolver;
pub mod mid_entry;
pub mod batch;
pub mod module_header;
pub mod game_config_header;
pub mod syscall_table;
#[cfg(test)]
mod tests;
pub use generator::{Generator, TestGenerator};
pub use cpp_generator::CppGenerator;
pub use sanitize::sanitize_identifier;
pub use dispatch::emit_dispatch_table;
pub use data::emit_data_sections;
pub use init_array::emit_psp_call_constructors;
pub use cmake::emit_cmake_lists;
pub use batch::emit_function_batches;
pub use module_header::{emit_module_header, ModuleFacts};
pub use game_config_header::{emit_game_config_header, GameChoices};
pub use syscall_table::emit_syscall_table;
