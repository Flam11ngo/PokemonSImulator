@echo off
title PokemonSimulator - Data Daemon
cd /d "%~dp0\..\.."
echo ========================================
echo   Data Daemon - Kafka Data Generator
echo   70 battle bots + 30 UI bots
echo ========================================
node engine-adapter\data_daemon.js
pause
