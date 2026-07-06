# =============================================================================
# PokemonSimulator Windows 服务重启脚本
# =============================================================================
# 用法:
#   powershell -ExecutionPolicy Bypass -File scripts/restart.ps1
#   powershell -ExecutionPolicy Bypass -File scripts/restart.ps1 -NoFrontend
# =============================================================================

param(
    [switch]$NoFrontend
)

$ErrorActionPreference = "Stop"
$ProjectDir = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$LogDir = "$ProjectDir\logs"

if (-not (Test-Path $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
}

# ---------------------------------------------------------------------------
# 颜色输出
# ---------------------------------------------------------------------------
function Write-Ok  { Write-Host "  [OK] $args" -ForegroundColor Green }
function Write-Warn { Write-Host "  [WARN] $args" -ForegroundColor Yellow }
function Write-Fail { Write-Host "  [FAIL] $args" -ForegroundColor Red; exit 1 }

# ---------------------------------------------------------------------------
# 工具函数: 通过端口杀进程
# ---------------------------------------------------------------------------
function Stop-PortProcess($port) {
    $connections = Get-NetTCPConnection -LocalPort $port -ErrorAction SilentlyContinue | Where-Object { $_.State -eq 'Listen' }
    if ($connections) {
        foreach ($conn in $connections) {
            try {
                $proc = Get-Process -Id $conn.OwningProcess -ErrorAction SilentlyContinue
                if ($proc) {
                    Stop-Process -Id $conn.OwningProcess -Force -ErrorAction SilentlyContinue
                    Write-Ok "已停止进程 $($proc.ProcessName) (PID=$($conn.OwningProcess), 端口 $port)"
                }
            } catch {
                Write-Warn "无法停止 PID=$($conn.OwningProcess)"
            }
        }
    } else {
        Write-Host "  端口 $port 无监听进程"
    }
}

# ---------------------------------------------------------------------------
# 获取本机 IP
# ---------------------------------------------------------------------------
function Get-LanIP {
    try {
        $ip = (Get-NetIPAddress -AddressFamily IPv4 | Where-Object {
            $_.InterfaceAlias -notmatch "Loopback" -and $_.PrefixOrigin -ne "WellKnown"
        } | Select-Object -First 1).IPAddress
        if ($ip) { return $ip }
    } catch {}
    return "127.0.0.1"
}

$LanIP = Get-LanIP

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  PokemonSimulator Windows 服务重启" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# ---- 1. 停止旧服务 ----
Write-Host "[1/3] 停止旧服务..." -ForegroundColor Yellow
Stop-PortProcess 8000
Stop-PortProcess 5173
Start-Sleep -Seconds 1

# ---- 2. 启动 API Server ----
Write-Host "[2/3] 启动 API Server..." -ForegroundColor Yellow
Set-Location $ProjectDir

$apiLog = "$LogDir\api-server.log"
$apiProc = Start-Process -FilePath "python" `
    -ArgumentList "api-server\standalone_server.py" `
    -NoNewWindow -PassThru `
    -RedirectStandardOutput $apiLog `
    -RedirectStandardError $apiLog

Write-Host "  API Server 启动中 (PID=$($apiProc.Id))..."

# 等待 API 就绪
$ready = $false
for ($i = 0; $i -lt 30; $i++) {
    Start-Sleep -Milliseconds 500
    try {
        $response = Invoke-WebRequest -Uri "http://localhost:8000/api/v1/health" -UseBasicParsing -TimeoutSec 2
        if ($response.StatusCode -eq 200) {
            Write-Ok "API Server 就绪 (PID=$($apiProc.Id))"
            $ready = $true
            break
        }
    } catch {}
}
if (-not $ready) {
    Write-Fail "API Server 启动超时，查看日志: $apiLog"
}

# ---- 3. 启动前端 (可选) ----
if (-not $NoFrontend) {
    Write-Host "[3/3] 启动 Vue 前端..." -ForegroundColor Yellow
    Set-Location "$ProjectDir\frontend"

    $feLog = "$LogDir\frontend.log"
    $feProc = Start-Process -FilePath "npx" `
        -ArgumentList "vite --host 0.0.0.0" `
        -NoNewWindow -PassThru `
        -RedirectStandardOutput $feLog `
        -RedirectStandardError $feLog

    Write-Host "  Frontend 启动中 (PID=$($feProc.Id))..."

    $feReady = $false
    for ($i = 0; $i -lt 30; $i++) {
        Start-Sleep -Milliseconds 500
        try {
            $response = Invoke-WebRequest -Uri "http://localhost:5173/" -UseBasicParsing -TimeoutSec 2
            if ($response.StatusCode -eq 200) {
                Write-Ok "Frontend 就绪 (PID=$($feProc.Id))"
                $feReady = $true
                break
            }
        } catch {}
    }
    if (-not $feReady) {
        Write-Warn "Frontend 启动较慢，查看日志: $feLog"
    }
}

Set-Location $ProjectDir

# ---------------------------------------------------------------------------
# 打印信息
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  服务已全部启动" -ForegroundColor Green
Write-Host ""
Write-Host "  前端:  http://localhost:5173" -ForegroundColor White
Write-Host "  API:   http://localhost:8000" -ForegroundColor White
Write-Host "  WS:    ws://localhost:8000/ws" -ForegroundColor White
Write-Host ""
Write-Host "  局域网访问:" -ForegroundColor White
Write-Host "    http://${LanIP}:5173" -ForegroundColor Green
Write-Host ""
Write-Host "  日志: $LogDir" -ForegroundColor Gray
Write-Host ""
Write-Host "  停止服务:" -ForegroundColor Yellow
Write-Host "    .\scripts\stop.bat" -ForegroundColor White
Write-Host "========================================" -ForegroundColor Cyan
