@echo off
setlocal EnableExtensions
for %%I in ("%~dp0..\..\..") do set "REPO=%%~fI"
for %%I in ("%~dp0..") do set "PROFILE=%%~fI"

set "BIN=%REPO%\out\vcs-release\bin\Release\VCSNative.exe"
if not exist "%BIN%" set "BIN=%REPO%\out\vcs-fast\bin\Release\VCSNative.exe"
if not exist "%BIN%" (
  echo VCSNative.exe not found. Build the VCS profile first.
  exit /b 3
)

set "GAME=%~1"
if "%GAME%"=="" if exist "%PROFILE%\game\PSP_GAME\SYSDIR\EBOOT_DECRYPTED.ELF" set "GAME=%PROFILE%\game"
if "%GAME%"=="" (
  echo Game root not found. Pass it as the first argument or run prepare_game.ps1.
  exit /b 4
)
set "ELF=%GAME%\PSP_GAME\SYSDIR\EBOOT_DECRYPTED.ELF"
if not exist "%ELF%" exit /b 5

set "PSPRECOMP_CONFIG=%PROFILE%\config\VCSNative.ini"
set "PSPRECOMP_GE_BACKEND=directx12"
set "PSPRECOMP_GE_GPU_SKIP_SOFTWARE_RASTER=1"
set "PSPRECOMP_GE_GPU_HW_CULL=1"
set "PSPRECOMP_TIME_TICK_DISPATCHES=4096"
set "PSPRECOMP_DX12_DEBUG=0"
set "PSPRECOMP_DX12_GE_READBACK=0"
set "PSPRECOMP_DX12_GE_STRICT=0"
set "PSPRECOMP_DX12_TEXTURE_UPLOAD_RING=1"
set "PSPRECOMP_GE_ASYNC=1"
set "PSPRECOMP_FRAME_LIMIT=0"
set "PSPRECOMP_FRAME_TIME_DIAG=1"
set "PSPRECOMP_GE_PHASE_DIAG=1"
set "PSPRECOMP_GE_PARALLEL_VERTEX_DECODE=1"
set "PSPRECOMP_GE_PARALLEL_VERTEX_THRESHOLD=768"
set "PSPRECOMP_GE_PARALLEL_VERTEX_MAX_WORKERS=6"
set "PSPRECOMP_GE_DIRECT_NONINDEXED_DRAW=1"
set "PSPRECOMP_DX12_PACKED_0115=1"
set "PSPRECOMP_DX12_NATIVE_INDEXED_DRAW=1"
set "PSPRECOMP_DX12_BATCH_MERGE=1"
set "PSPRECOMP_CHAIN_DEPTH=1024"
set "PSPRECOMP_ENABLE_FAST_088B1554=1"
set "PSPRECOMP_GE_GPU_DUAL_FRAME=0"

"%BIN%" "%ELF%" "%GAME%"
exit /b %ERRORLEVEL%
