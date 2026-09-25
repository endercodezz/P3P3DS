@echo off
setlocal EnableExtensions EnableDelayedExpansion
for %%I in ("%~dp0..\..\..") do set "REPO=%%~fI"
set "BUILD=%REPO%\out\vcs-fast"
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

rem MSBuild /m and cl.exe /MP multiply: keep MSBuild serial across projects and
rem let /MP%JOBS% be the single source of compile parallelism.
set "CMAKE_BUILD_PARALLEL_LEVEL=1"
echo ================================================================
echo VCS - FAST INCREMENTAL BUILD
echo Build pipeline restored to the last known-good pre-reorganization behavior.
echo CMake: !CMAKE_EXE! ^| Workers: %JOBS% (/MP%JOBS%, MSBuild /m:1) ^| LTCG: OFF
echo ================================================================
"%CMAKE_EXE%" -S "%REPO%" -B "%BUILD%" -G "Visual Studio 17 2022" -A x64 ^
  -DPSPRECOMP_PROFILE=vcs ^
  -DPSPRECOMP_GENERATED_OPT_LEVEL=2 ^
  -DPSPRECOMP_LTO=OFF ^
  -DPSPRECOMP_VCS_AOT_LTO=OFF ^
  -DPSPRECOMP_MSVC_MP_JOBS=%JOBS% ^
  -DPSPRECOMP_BUILD_TESTS=ON ^
  -DPSPRECOMP_BUILD_PROFILE_TESTS=ON
if errorlevel 1 goto :FAIL
"%CMAKE_EXE%" --build "%BUILD%" --config Release --parallel 1 --target ^
  VCSNative psprecomp_tests vcs_config_tests audio_resampler_tests vcs_bootstrap_paths_tests vcs_dx12_probe vcs_dx12_ge_probe ^
  -- /m:1
if errorlevel 1 goto :FAIL
copy /Y "%REPO%\profiles\vcs\config\VCSNative.ini" "%BUILD%\bin\Release\VCSNative.ini" >nul
echo BUILD FAST OK: %BUILD%\bin\Release\VCSNative.exe
exit /b 0
:NO_CMAKE
echo ERROR: CMake not found. Install/enable "CMake tools for Windows" in Visual Studio Installer.
pause
exit /b 2
:FAIL
echo ERROR: VCS fast build failed. Send the FIRST real compiler error above.
pause
exit /b 5
