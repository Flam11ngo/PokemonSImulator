@echo off
REM === PokemonSimulator Nginx Launcher ===
REM Replaces __ROOT__ placeholder with actual project path, then starts nginx.

set NGINX_DIR=E:\nginx-1.30.3
set PROJECT_DIR=%~dp0..\..

if not exist "%NGINX_DIR%\nginx.exe" (
    echo [ERROR] nginx not found at %NGINX_DIR%
    echo Please install nginx for Windows from https://nginx.org/en/download.html
    pause
    exit /b 1
)

echo [*] Generating nginx.conf...
set ROOT=%PROJECT_DIR:\=/%
powershell -Command "(Get-Content '%PROJECT_DIR%\nginx\nginx.conf') -replace '__ROOT__', '%ROOT%' | Set-Content '%NGINX_DIR%\conf\nginx.conf'"
echo [*] Starting nginx...
start /B "%NGINX_DIR%\nginx.exe" -c "%NGINX_DIR%\conf\nginx.conf"
echo [*] nginx started on http://localhost:80
pause
