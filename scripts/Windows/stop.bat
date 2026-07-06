@echo off
title PokemonSimulator - Stop

echo ========================================
echo   PokemonSimulator - Stop Services
echo ========================================
echo.

echo [1/3] Stopping Python API server ...
taskkill /f /im python.exe 2>nul && echo   stopped || echo   not running

echo [2/3] Stopping Node.js frontend ...
taskkill /f /im node.exe 2>nul && echo   stopped || echo   not running

echo [3/3] Freeing WSL port 8000 ...
wsl bash -c "fuser -k 8000/tcp 2>/dev/null" 2>nul

echo.
echo ========================================
echo   All services stopped.
echo ========================================
timeout /t 3 /nobreak >nul
