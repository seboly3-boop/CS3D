@echo off
setlocal
cd /d "%~dp0"
echo ============================================================
echo  CS3D - Phase 1 build (x64 Release)
echo  Requires: Visual Studio 2022 (C++ workload), CMake 3.20+
echo  First run downloads GLFW 3.4 + GLM 1.0.1 (internet needed)
echo ============================================================

where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] CMake not found in PATH.
  echo Install CMake 3.20+ and reopen this terminal.
  pause & exit /b 1
)

cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
  echo [ERROR] CMake configure failed.
  echo Install "Visual Studio 2022 Build Tools" with the
  echo "Desktop development with C++" workload, then re-run.
  pause & exit /b 1
)

cmake --build build --config Release --parallel
if errorlevel 1 (
  echo [ERROR] Build failed. See output above.
  pause & exit /b 1
)

echo.
echo [OK] Built build\Release\CS3D.exe  -  launch with RUN.bat
pause
