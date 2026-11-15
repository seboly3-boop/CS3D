@echo off
cd /d "%~dp0"
if not exist "build\Release\CS3D.exe" (
  echo [ERROR] build\Release\CS3D.exe not found. Run BUILD.bat first.
  pause & exit /b 1
)
build\Release\CS3D.exe
if errorlevel 1 pause
