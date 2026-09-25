@echo off
call "%~dp0scripts\build_release.bat"
exit /b %errorlevel%
