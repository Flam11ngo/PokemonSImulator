@echo off
title PokemonSimulator - Stop Bot

echo ========================================
echo   Stop Battle Bot
echo ========================================
echo.

powershell -Command "$apiPid = (Get-NetTCPConnection -LocalPort 9000 -ErrorAction SilentlyContinue | Where-Object State -eq 'Listen' | Select-Object -First 1).OwningProcess; if ($apiPid) { $bots = Get-Process -Name python -ErrorAction SilentlyContinue | Where-Object { $_.Id -ne $apiPid }; if ($bots) { $bots | Stop-Process -Force; Write-Host '  Bot stopped' } else { Write-Host '  Bot not running' } } else { Write-Host '  API server not found' }"

echo.
pause
