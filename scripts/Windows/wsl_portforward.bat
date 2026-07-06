@echo off
echo ========================================
echo   PokemonSimulator WSL2 Port Forward
echo ========================================
echo.

net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Run as Administrator!
    pause
    exit /b 1
)

echo [1/3] Getting WSL IP...
for /f %%i in ('wsl hostname -I') do set WSL_IP=%%i

if "%WSL_IP%"=="" (
    echo [ERROR] Cannot get WSL IP. Is WSL running?
    pause
    exit /b 1
)
echo   WSL IP: %WSL_IP%

echo [2/3] Removing old rules...
netsh interface portproxy delete v4tov4 listenport=5173 2>nul
netsh interface portproxy delete v4tov4 listenport=8000 2>nul

echo [3/3] Adding new rules...
netsh interface portproxy add v4tov4 listenport=5173 listenaddress=0.0.0.0 connectport=5173 connectaddress=%WSL_IP%
netsh interface portproxy add v4tov4 listenport=8000 listenaddress=0.0.0.0 connectport=8000 connectaddress=%WSL_IP%

netsh advfirewall firewall add rule name="PokeSim-5173" dir=in action=allow protocol=TCP localport=5173 >nul 2>&1
netsh advfirewall firewall add rule name="PokeSim-8000" dir=in action=allow protocol=TCP localport=8000 >nul 2>&1

echo.
echo ========================================
echo   Port Forward Ready
echo   Frontend: http://localhost:5173
echo   API:      http://localhost:8000
echo ========================================
pause
