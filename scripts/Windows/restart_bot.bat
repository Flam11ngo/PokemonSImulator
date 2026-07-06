@echo off
title PokemonSimulator - Restart Bot
cd /d "%~dp0\..\py"

echo ========================================
echo   Restart Battle Bot
echo ========================================
echo.

echo [1/2] Killing old bot ...
powershell -Command "$apiPid = (Get-NetTCPConnection -LocalPort 9000 -ErrorAction SilentlyContinue | Where-Object State -eq 'Listen' | Select-Object -First 1).OwningProcess; if ($apiPid) { Get-Process -Name python -ErrorAction SilentlyContinue | Where-Object { $_.Id -ne $apiPid } | Stop-Process -Force }"
timeout /t 1 /nobreak >nul
echo   done.

echo [2/2] Starting new bot ...
start "PokemonSim-Bot" /min cmd /c "cd /d %CD% && python battle_bot.py"
timeout /t 2 /nobreak >nul

echo.
echo Bot restarted.
pause
