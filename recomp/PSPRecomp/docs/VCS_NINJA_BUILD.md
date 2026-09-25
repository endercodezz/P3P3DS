# VCSNative build with Ninja

Ninja is the build executor used by CMake in `out/vcs-ninja`. It does not make
`VCSNative.exe` faster at runtime. Its benefit is development speed: Ninja
tracks the dependency graph, recompiles only affected files and runs independent
compiler jobs in parallel.

## Requirements

- Visual Studio 2022 with the C++ desktop workload
- CMake (the copy bundled with Visual Studio is sufficient)
- Ninja (also bundled with current Visual Studio CMake installations)

Run the commands from an x64 Visual Studio Developer Command Prompt, or first
call:

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
```

## Configure the current optimized VCS build

Configuration happens once. CMake stores the result in
`out/vcs-ninja/CMakeCache.txt`.

```bat
cmake -S . -B out/vcs-ninja -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DPSPRECOMP_PROFILE=vcs ^
  -DPSPRECOMP_BUILD_TESTS=ON ^
  -DPSPRECOMP_BUILD_PROFILE_TESTS=ON ^
  -DPSPRECOMP_NATIVE_AVX2=ON ^
  -DPSPRECOMP_LTO=ON ^
  -DPSPRECOMP_VCS_AOT_LTO=OFF ^
  -DPSPRECOMP_GENERATED_OPT_LEVEL=3 ^
  -DPSPRECOMP_GENERATED_INLINE_LEVEL=0 ^
  -DPSPRECOMP_PROFILE_GUIDED_AOT=ON ^
  -DPSPRECOMP_HOT_GENERATED_OPT_LEVEL=3 ^
  -DPSPRECOMP_HOT_GENERATED_INLINE_LEVEL=3 ^
  -DPSPRECOMP_AOT_ASSUME_NO_WRITE_WATCH=ON ^
  -DPSPRECOMP_AOT_PRODUCTION_FASTPATHS=ON ^
  -DPSPRECOMP_MSVC_MP_JOBS=1 ^
  -DPSPRECOMP_VCS_MSVC_AVX_FALLBACK_UNITS="0018;0022;0035;0089;0091;0117;0164"
```

The listed fallback translation units use AVX instead of AVX2 because MSVC
19.44 currently crashes internally while compiling those specific large files
with AVX2. The remaining generated units still use AVX2.

## Build

Build the game executable:

```bat
cmake --build out/vcs-ninja --target VCSNative --parallel 4
```

Build the executable and the main VCS checks:

```bat
cmake --build out/vcs-ninja --target VCSNative vcs_config_tests vcs_dx12_ge_probe --parallel 4
```

After a small source change, the same command is incremental. For example, a
change confined to one generated unit compiles that unit and relinks the EXE;
it does not rebuild all 234 AOT translation units.

Unlike a multi-configuration Visual Studio generator, this Ninja directory was
configured directly as `Release`, so `--config Release` is unnecessary.

## Outputs

```text
out/vcs-ninja/bin/Release/VCSNative.exe
out/vcs-ninja/bin/Release/play-4.bat
out/vcs-ninja/bin/Release/play-6.bat
out/vcs-ninja/profiles/vcs/vcs_config_tests.exe
out/vcs-ninja/bin/Release/vcs_dx12_ge_probe.exe
```

`out/vcs-ninja` contains generated build products and should not be committed.
Make permanent source/configuration edits under `profiles/vcs`, then rebuild so
CMake updates the packaged files.

## When a full rebuild happens

Ninja can still rebuild many files when a widely included header changes, when
CMake options change, or when generated AOT sources are regenerated. The first
build of a new output directory is also necessarily a full build. Subsequent
localized changes are where Ninja saves the most time.

