@echo off
setlocal EnableExtensions EnableDelayedExpansion
for %%I in ("%~dp0..\..\..") do set "REPO=%%~fI"
set "BUILD=%REPO%\out\vcs-release"
call "%~dp0pick_jobs.bat"
set "CMAKE_EXE="
set "VSWHERE="

for /f "delims=" %%I in ('where cmake.exe 2^>nul') do if not defined CMAKE_EXE set "CMAKE_EXE=%%~fI"
if not defined CMAKE_EXE if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles%\CMake\bin\cmake.exe"
if not defined CMAKE_EXE if exist "%ProgramFiles(x86)%\CMake\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles(x86)%\CMake\bin\cmake.exe"
for /f "delims=" %%I in ('where vswhere.exe 2^>nul') do if not defined VSWHERE set "VSWHERE=%%~fI"
if not defined VSWHERE if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not defined VSWHERE if exist "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not defined CMAKE_EXE if defined VSWHERE (
  for /f "usebackq delims=" %%I in (`"!VSWHERE!" -latest -version "[17.0,18.0)" -products * -property installationPath`) do (
    if exist "%%I\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_EXE=%%I\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  )
)
if not defined CMAKE_EXE for %%E in (Community Professional Enterprise BuildTools) do (
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles%\Microsoft Visual Studio\2022\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)
if not defined CMAKE_EXE goto :NO_CMAKE
for %%I in ("!CMAKE_EXE!") do set "CTEST_EXE=%%~dpIctest.exe"
if not exist "!CTEST_EXE!" set "CTEST_EXE=ctest.exe"

rem MSBuild /m and cl.exe /MP multiply: keep MSBuild serial across projects and
rem let /MP%JOBS% be the single source of compile parallelism.
set "CMAKE_BUILD_PARALLEL_LEVEL=1"
echo ================================================================
echo VCS - PERFORMANCE INCREMENTAL BUILD
echo Build pipeline restored to the last known-good pre-reorganization behavior.
echo CMake: !CMAKE_EXE!
echo Compile workers: %JOBS% ^| AOT /MP%JOBS% ^| MSBuild /m:1
echo AOT inlining: /Ob3 hot measured units ^| /Ob0 cold units
echo Link: host/core LTCG only ^| generated AOT /GL- ^| LTCG status visible
echo Build dir preserved: %BUILD%
echo ================================================================

echo [1/7] Configuring without deleting existing objects...
"%CMAKE_EXE%" -S "%REPO%" -B "%BUILD%" -G "Visual Studio 17 2022" -A x64 ^
  -DPSPRECOMP_PROFILE=vcs ^
  -DPSPRECOMP_GENERATED_OPT_LEVEL=3 ^
  -DPSPRECOMP_LTO=ON ^
  -DPSPRECOMP_NATIVE_AVX2=ON ^
  -DPSPRECOMP_AOT_ASSUME_NO_WRITE_WATCH=ON ^
  -DPSPRECOMP_AOT_PRODUCTION_FASTPATHS=ON ^
  -DPSPRECOMP_MSVC_CGTHREADS=0 ^
  -DPSPRECOMP_MSVC_MP_JOBS=%JOBS% ^
  -DPSPRECOMP_PROFILE_GUIDED_AOT=ON ^
  -DPSPRECOMP_HOT_GENERATED_OPT_LEVEL=3 ^
  -DPSPRECOMP_GENERATED_INLINE_LEVEL=0 ^
  -DPSPRECOMP_HOT_GENERATED_INLINE_LEVEL=3 ^
  -DPSPRECOMP_VCS_AOT_LTO=OFF ^
  -DPSPRECOMP_BUILD_TESTS=ON ^
  -DPSPRECOMP_BUILD_PROFILE_TESTS=ON
if errorlevel 1 goto :FAIL

echo [2/7] Building VCS executable first...
echo       The generated AOT units compile normally; the final link no longer receives their LTCG IR.
"%CMAKE_EXE%" --build "%BUILD%" --config Release --parallel 1 --target VCSNative -- /m:1
if errorlevel 1 goto :FAIL

echo [2b/7] Building tests and probes...
"%CMAKE_EXE%" --build "%BUILD%" --config Release --parallel 1 --target ^
  psprecomp_tests vcs_profile_tests vcs_config_tests audio_resampler_tests vcs_bootstrap_paths_tests vcs_dx12_probe vcs_dx12_ge_probe ^
  -- /m:1
if errorlevel 1 goto :FAIL

echo [3/7] Running regression tests...
"!CTEST_EXE!" --test-dir "%BUILD%" -C Release --output-on-failure
if errorlevel 1 goto :TEST_FAIL

echo [4/7] DX12 device/swapchain probe...
"%BUILD%\bin\Release\vcs_dx12_probe.exe"
if errorlevel 1 goto :DX12_FAIL

echo [5/7] GE compatibility probe...
set "PSPRECOMP_DX12_GE_STRICT=1"
set "PSPRECOMP_GE_PARALLEL_VERTEX_DECODE=0"
set "PSPRECOMP_GE_DIRECT_NONINDEXED_DRAW=0"
set "PSPRECOMP_DX12_PACKED_0115=0"
set "PSPRECOMP_DX12_NATIVE_INDEXED_DRAW=0"
set "PSPRECOMP_DX12_BATCH_MERGE=0"
"%BUILD%\bin\Release\vcs_dx12_ge_probe.exe"
if errorlevel 1 goto :GE_FAIL
set "PSPRECOMP_DX12_GE_STRICT="

echo [6/7] GE production probe...
set "PSPRECOMP_DX12_GE_STRICT=1"
set "PSPRECOMP_GE_GPU_HW_CULL=1"
set "PSPRECOMP_GE_PARALLEL_VERTEX_DECODE=0"
set "PSPRECOMP_GE_DIRECT_NONINDEXED_DRAW=1"
set "PSPRECOMP_DX12_PACKED_0115=1"
set "PSPRECOMP_DX12_NATIVE_INDEXED_DRAW=1"
set "PSPRECOMP_DX12_BATCH_MERGE=1"
"%BUILD%\bin\Release\vcs_dx12_ge_probe.exe"
if errorlevel 1 goto :GE_PROD_FAIL
set "PSPRECOMP_DX12_GE_STRICT="

echo [7/7] Installing current VCS config...
copy /Y "%REPO%\profiles\vcs\config\VCSNative.ini" "%BUILD%\bin\Release\VCSNative.ini" >nul

echo.
echo BUILD OK: %BUILD%\bin\Release\VCSNative.exe
exit /b 0

:NO_CMAKE
echo ERROR: CMake not found in PATH, standalone install, or Visual Studio 2022.
echo Visual Studio Installer ^> Modify ^> Individual components ^> CMake tools for Windows.
pause
exit /b 2
:TEST_FAIL
echo ERROR: regression tests failed.
pause
exit /b 8
:DX12_FAIL
echo ERROR: DX12 probe failed.
pause
exit /b 6
:GE_FAIL
echo ERROR: compatibility GE probe failed.
pause
exit /b 7
:GE_PROD_FAIL
echo ERROR: production GE probe failed.
pause
exit /b 9
:FAIL
echo ERROR: VCS performance build failed. Send the FIRST real compiler error above.
pause
exit /b 5
