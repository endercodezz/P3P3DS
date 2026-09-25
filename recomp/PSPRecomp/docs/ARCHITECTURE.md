# PSPRecomp architecture

PSPRecomp has two layers: a reusable PSP execution framework and one or more title profiles.

## Framework

`psprecomp_core` provides:

- Allegrex/MIPS instruction decoding and architectural state;
- PSP ELF/PRX loading and relocation support;
- guest memory and EDRAM address handling;
- NID/import registration;
- generated-function registration and dispatch;
- bounded generated-unit chaining;
- scheduler visibility boundaries;
- AOT hot-register and fast-memory support.

The root tools provide executable analysis and generic C++ generation. Nothing in the framework is supposed to require a specific game address or game asset.

## Profiles

A profile supplies everything needed to turn the framework into a native build for one title: generated guest code, HLE functions, title bootstrap, compatibility behavior, renderer/audio/input integration, native guest-leaf replacements and build packaging.

The profile boundary is intentional. Optimizations that are valid because of a measured address, ABI or data layout in one game stay in that game's profile even when they use reusable runtime APIs.

## Generated execution

Guest functions are emitted ahead of time as C++ and registered at their guest addresses. Generated units may chain directly when the runtime can prove that the target has not been replaced by an import/HLE/host override. Visibility boundaries materialize cached architectural state before host code or scheduling can inspect or replace the guest context.

## Native fast paths

`Runtime::register_native_fast_path(address, callback)` is the extension point for a profile to replace a measured guest leaf without adding title-specific code to `psprecomp_core`. Profile-generated code enters through `Runtime::invoke_native_fast_path()` and falls back to the generated function if no profile callback is registered.
