@echo off
setlocal EnableExtensions
cd /d "%~dp0"
title matenOS - build and run

echo [1/2] compiling matenOS...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1"
if errorlevel 1 goto :build_failed
if not exist "%~dp0build\matenos.img" goto :build_failed

set "QEMU="
if exist "C:\Program Files\qemu\qemu-system-i386w.exe" set "QEMU=C:\Program Files\qemu\qemu-system-i386w.exe"
if not defined QEMU if exist "C:\Program Files\qemu\qemu-system-i386.exe" set "QEMU=C:\Program Files\qemu\qemu-system-i386.exe"
if not defined QEMU if exist "C:\Program Files (x86)\qemu\qemu-system-i386w.exe" set "QEMU=C:\Program Files (x86)\qemu\qemu-system-i386w.exe"
if not defined QEMU if exist "C:\Program Files (x86)\qemu\qemu-system-i386.exe" set "QEMU=C:\Program Files (x86)\qemu\qemu-system-i386.exe"
if not defined QEMU for /f "delims=" %%Q in ('where qemu-system-i386 2^>nul') do if not defined QEMU set "QEMU=%%Q"
if not defined QEMU for /f "delims=" %%Q in ('where qemu-system-i386w 2^>nul') do if not defined QEMU set "QEMU=%%Q"

if not defined QEMU goto :no_qemu

echo [2/2] starting...
start "matenOS" "%QEMU%" -name matenOS -m 32 -boot a -vga std -full-screen -fda "%~dp0build\matenos.img" -serial "file:%~dp0build\serial.log"
exit /b 0

:no_qemu
echo.
echo QEMU not found. Install QEMU for Windows first.
pause
exit /b 1

:build_failed
echo.
echo Build failed. See the compiler output above.
pause
exit /b 1
