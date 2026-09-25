@echo off
rem Resolve CMake without requiring it to be in the global PATH.
rem On success this script defines PSPRECOMP_CMAKE and PSPRECOMP_CTEST.

set "PSPRECOMP_CMAKE="
set "PSPRECOMP_CTEST="

for /f "delims=" %%I in ('where cmake.exe 2^>nul') do if not defined PSPRECOMP_CMAKE set "PSPRECOMP_CMAKE=%%~fI"
if defined PSPRECOMP_CMAKE goto :FOUND

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
  for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -property installationPath`) do (
    if exist "%%I\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
      set "PSPRECOMP_CMAKE=%%I\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
      goto :FOUND
    )
  )
)

for %%E in (Community Professional Enterprise BuildTools) do (
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "PSPRECOMP_CMAKE=%ProgramFiles%\Microsoft Visual Studio\2022\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    goto :FOUND
  )
)

if exist "%ProgramFiles%\CMake\bin\cmake.exe" (
  set "PSPRECOMP_CMAKE=%ProgramFiles%\CMake\bin\cmake.exe"
  goto :FOUND
)

if exist "%ProgramFiles(x86)%\CMake\bin\cmake.exe" (
  set "PSPRECOMP_CMAKE=%ProgramFiles(x86)%\CMake\bin\cmake.exe"
  goto :FOUND
)

exit /b 2

:FOUND
for %%I in ("%PSPRECOMP_CMAKE%") do set "PSPRECOMP_CTEST=%%~dpIctest.exe"
if not exist "%PSPRECOMP_CTEST%" set "PSPRECOMP_CTEST=ctest.exe"
exit /b 0
