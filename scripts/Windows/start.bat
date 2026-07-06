@echo off
title PokemonSimulator - Services
cd /d "%~dp0\..\.."

:: ---- PATH setup ----
set "PYTHON_HOME=%LOCALAPPDATA%\Programs\Python\Python313"
set "PYTHON_SCRIPTS=%PYTHON_HOME%\Scripts"
set "PATH=%PYTHON_HOME%;%PYTHON_SCRIPTS%;%PATH%"
set "ROOT=%CD%"

echo ========================================
echo   PokemonSimulator - One Click Start
echo ========================================
echo.
echo [*] Project root: %ROOT%

:: ---- Log dir ----
if not exist "%ROOT%\logs" mkdir "%ROOT%\logs"

:: ---- Free port 8000 from WSL ----
echo [*] Checking port 8000 (WSL) ...
wsl bash -c "fuser -k 8000/tcp 2>/dev/null" 2>nul
timeout /t 1 /nobreak >nul

:: ---- Kill old services ----
echo [*] Cleaning old processes ...
taskkill /f /im python.exe 2>nul
taskkill /f /im node.exe 2>nul
timeout /t 1 /nobreak >nul

:: ---- Start API server (Showdown engine via Node.js) ----
echo [1/2] Starting API server (port 9000) ...
start "PokemonSim-API" /min cmd /c "cd /d %ROOT%\api-server && "%PYTHON_HOME%\python.exe" standalone_server.py >> %ROOT%\logs\api.log 2>&1"
echo   - logs/api.log

:: ---- Start frontend ----
echo [2/2] Starting frontend (Vite) ...
start "PokemonSim-Frontend" /min cmd /c "cd /d %ROOT%\frontend && node node_modules\vite\bin\vite.js --host 0.0.0.0 >> %ROOT%\logs\frontend.log 2>&1"
echo   - logs/frontend.log

:: ---- Wait for startup ----
timeout /t 3 /nobreak >nul

:: ---- Check ----
echo.
echo [*] Checking ports ...
netstat -ano | findstr ":9000 " | findstr "LISTENING" >nul && echo   API  :9000 - OK || echo   API  :9000 - waiting...
netstat -ano | findstr ":5173 " | findstr "LISTENING" >nul && echo   Vite :5173 - OK || echo   Vite :5173 - waiting...

:: ---- Done ----
echo.
echo ========================================
echo   Services started (minimized windows)
echo.
echo   Frontend : http://localhost:5173
echo   API      : http://localhost:9000
echo   Health   : http://localhost:9000/api/v1/health
echo   Logs     : %ROOT%\logs\
echo.
echo   Run stop.bat to stop all services.
echo   Run restart_bot.bat to restart bot.
echo ========================================
pause
