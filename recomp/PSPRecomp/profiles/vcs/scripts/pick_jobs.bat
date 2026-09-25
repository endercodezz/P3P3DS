@echo off
rem Picks a memory-aware compile parallelism for the VCS profile.
rem
rem Each generated AOT unit is a ~500 KB translation unit compiled at /Ox /Ob3,
rem which routinely peaks above 2 GB in a single cl.exe front-end.  Driving the
rem build with one worker per logical core exhausts physical memory on machines
rem with many cores relative to RAM, so cap the workers by available memory too.
rem
rem Sets JOBS.  Override with PSPRECOMP_JOBS=<n> in the environment.
rem Callers must use /m:1 and rely on cl.exe /MP<JOBS> for parallelism, so the
rem total number of concurrent front-ends stays equal to JOBS.

set "JOBS="
if defined PSPRECOMP_JOBS (
  set "JOBS=%PSPRECOMP_JOBS%"
  goto :DONE
)

set "CORES=%NUMBER_OF_PROCESSORS%"
if not defined CORES set "CORES=8"

rem ~3 GB of headroom per concurrent cl.exe front-end.
set "MEMJOBS="
for /f "usebackq delims=" %%I in (`powershell -NoProfile -NonInteractive -Command ^
  "[int][math]::Floor((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory/3GB)" 2^>nul`) do set "MEMJOBS=%%I"
if not defined MEMJOBS set "MEMJOBS=%CORES%"
if "%MEMJOBS%"=="0" set "MEMJOBS=2"

set "JOBS=%CORES%"
if %MEMJOBS% LSS %CORES% set "JOBS=%MEMJOBS%"
if %JOBS% LSS 2 set "JOBS=2"

:DONE
echo Compile workers: %JOBS% (cores: %NUMBER_OF_PROCESSORS%, memory-capped; override with PSPRECOMP_JOBS)
exit /b 0
