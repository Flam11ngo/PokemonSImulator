@echo off
title PokemonSimulator - Restart

echo ========================================
echo   PokemonSimulator - Restart Services
echo ========================================
echo.

:: Stop
echo [1/2] Stopping services ...
taskkill /f /im python.exe 2>nul
taskkill /f /im node.exe 2>nul
wsl bash -c "fuser -k 8000/tcp 2>/dev/null" 2>nul
timeout /t 2 /nobreak >nul
echo [OK] Stopped.

:: Start (captures PATH + background launch, no child windows)
echo.
echo [2/2] Starting services ...
call "%~dp0start.bat"
