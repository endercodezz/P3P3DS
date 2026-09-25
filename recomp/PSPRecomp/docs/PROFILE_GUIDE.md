# PSPRecomp profile guide

A profile contains everything that is specific to one PSP title. The framework root contains only reusable Allegrex, ELF/PRX, memory, runtime and code-generation infrastructure.

## Recommended layout

```text
profiles/<id>/
  CMakeLists.txt
  README.md
  config/          Profile configuration and analysis inputs
  data/            Redistributable profile data generated from compatible sources
  generated/       Checked-in AOT C++ corpus for the supported executable
  host/            HLE, bootstrap, renderer/audio/input integration and patches
  scripts/         Current build, run and benchmark entry points
  tests/           Profile-specific regression tests
  tools/           Profile-specific generation or conversion helpers
  third_party/     Dependencies and their license notices
  progress/        Local development handoffs/history; ignored by Git
```

Do not put commercial executables, disc images, game assets, decrypted EBOOTs, captures made from copyrighted assets, or local analysis dumps in the repository.

## 1. Start from a game executable you are allowed to analyze

Keep local inputs under `profiles/<id>/game` or another ignored directory. PSPRecomp expects an ELF/PRX input that the generic analyzer can read. Decryption/extraction is deliberately outside the framework.

## 2. Analyze the executable

Build the framework tools and run `psp_analyze` against the local executable. Keep function maps and temporary analysis output under the profile's ignored `analysis` directory until they are ready to become stable profile inputs.

## 3. Generate AOT code

Use the root `psp_recomp` when ordinary PSPRecomp lowering is sufficient. If a title needs verified, address-specific lowering, keep that code in a profile tool rather than hardcoding the address in `tools/codegen_main.cpp` or `src/runtime.cpp`.

The VCS profile demonstrates this split with its own `vcs_recomp` target while the root `psp_recomp` remains game-neutral.

## 4. Implement the host profile

Profile host code owns title-specific HLE behavior, bootstrap rules, display/audio/input integration and compatibility patches. Register guest replacements through `Runtime::register_function()` and PSP imports through `Runtime::register_hle()`.

For heavily measured guest leaves, a profile may register a native implementation with `Runtime::register_native_fast_path()`. Generated profile code can enter it through `Runtime::invoke_native_fast_path()`. The native implementation stays in the profile; the reusable runtime contains no game address.

## 5. Add CMake integration

Each profile supplies `profiles/<id>/CMakeLists.txt`. The root build adds only the profile selected through `-DPSPRECOMP_PROFILE=<id>`. A profile may define its own executable, generators, tests, renderer dependencies and post-build packaging.

A clean framework build must continue to work with:

```bash
cmake -S . -B out/framework -DPSPRECOMP_PROFILE=""
cmake --build out/framework
```

## 6. Keep generated code reproducible

If generated AOT files are checked in, document:

- executable identity/hash expected by the profile;
- stable function-map/config inputs;
- exact profile generator command;
- any deterministic post-generation optimization passes.

Generated C++ may contain game addresses because it belongs to the profile. Reusable root code should not.

## 7. Keep progress material out of the public tree

Use `profiles/<id>/progress` for handoffs, one-off benchmark notes and old stage reports. The root `.gitignore` excludes every profile's `progress` directory. Current build/run scripts should have stable names instead of accumulating stage-numbered copies.

## 8. License boundaries

Keep third-party notices with the profile component that needs them. Do not copy source from a project whose license is incompatible with the intended distribution model. Behavioral observations, hardware specifications and independently written implementations should be documented separately from copied third-party source.
