# =============================================================================
# PokemonSimulator — Windows 一键部署脚本
# =============================================================================
# 用法:
#   powershell -ExecutionPolicy Bypass -File scripts/setup-server.ps1
#   powershell -ExecutionPolicy Bypass -File scripts/setup-server.ps1 -Mode minimal
#   powershell -ExecutionPolicy Bypass -File scripts/setup-server.ps1 -Mode dev
#   powershell -ExecutionPolicy Bypass -File scripts/setup-server.ps1 -Help
# =============================================================================

param(
    [ValidateSet("full", "minimal", "dev")]
    [string]$Mode = "full",
    [switch]$Help
)

# ---------------------------------------------------------------------------
# 颜色输出
# ---------------------------------------------------------------------------
function Write-Log  { Write-Host "[INFO]  $(Get-Date -Format 'HH:mm:ss') $args" -ForegroundColor Green }
function Write-Warn { Write-Host "[WARN]  $(Get-Date -Format 'HH:mm:ss') $args" -ForegroundColor Yellow }
function Write-Err  { Write-Host "[ERROR] $(Get-Date -Format 'HH:mm:ss') $args" -ForegroundColor Red }
function Write-Info { Write-Host "[*]    $args" -ForegroundColor Cyan }
function Write-Hdr  {
    Write-Host ""
    Write-Host "============================================================" -ForegroundColor Blue
    Write-Host "  $args" -ForegroundColor Blue
    Write-Host "============================================================" -ForegroundColor Blue
}

if ($Help) {
    Write-Host "用法: .\setup-server.ps1 [-Mode full|minimal|dev] [-Help]"
    Write-Host ""
    Write-Host "  -Mode full     完整部署: Docker + C++ 引擎 + 前端"
    Write-Host "  -Mode minimal  最小部署: 仅 C++ 引擎 (无 Docker, 无前端)"
    Write-Host "  -Mode dev      开发模式: Python API + Node.js 前端 (跳过 Docker 大数据栈)"
    Write-Host "  -Help          显示此帮助信息"
    exit 0
}

# ---------------------------------------------------------------------------
# 前置检查
# ---------------------------------------------------------------------------
Write-Hdr "PokemonSimulator Windows 一键部署脚本"

# 检测是否以管理员权限运行
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin -and $Mode -ne "dev") {
    Write-Warn "建议以管理员权限运行 (Docker/symlink 可能需要管理员权限)"
}

$ProjectDir = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Write-Log "模式: $Mode"
Write-Log "项目目录: $ProjectDir"

# 检测 Windows 版本
$osInfo = Get-CimInstance Win32_OperatingSystem
Write-Log "检测到操作系统: $($osInfo.Caption)"

# ---------------------------------------------------------------------------
# 阶段 1: 检查基础工具链
# ---------------------------------------------------------------------------
Write-Hdr "阶段 1/5: 检查基础工具链"

# Git
try {
    $gitVersion = git --version 2>&1
    Write-Log "Git 可用: $gitVersion"
} catch {
    Write-Err "Git 未安装，请从 https://git-scm.com/download/win 安装"
    exit 1
}

# CMake
try {
    $cmakeVersion = cmake --version 2>&1 | Select-Object -First 1
    Write-Log "CMake 可用: $cmakeVersion"
} catch {
    Write-Err "CMake 未安装，请从 https://cmake.org/download/ 安装"
    exit 1
}

# Visual Studio / MSVC
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msvcFound = $false
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -property installationPath 2>$null
    if ($vsPath) {
        Write-Log "Visual Studio: $vsPath"
        $msvcFound = $true
    }
}
if (-not $msvcFound) {
    Write-Warn "Visual Studio 未检测到，C++ 编译需要 VS 2022 (Community 免费版)"
    Write-Info "下载: https://visualstudio.microsoft.com/downloads/"
    Write-Info "安装时勾选 '使用 C++ 的桌面开发' 工作负载"
    if ($Mode -ne "dev") {
        Write-Err "无法继续: C++ 编译需要 Visual Studio"
        exit 1
    }
}

# Python
if ($Mode -ne "minimal") {
    try {
        $pyVersion = python --version 2>&1
        Write-Log "Python 可用: $pyVersion"
    } catch {
        try {
            $pyVersion = python3 --version 2>&1
            Write-Log "Python3 可用: $pyVersion"
        } catch {
            Write-Err "Python 未安装，请从 https://www.python.org/downloads/ 安装 Python 3.10+"
            exit 1
        }
    }
}

# Node.js (前端)
if ($Mode -eq "full" -or $Mode -eq "dev") {
    try {
        $nodeVersion = node --version 2>&1
        Write-Log "Node.js 可用: $nodeVersion"
        $npmVersion = npm --version 2>&1
        Write-Log "npm 可用: v$npmVersion"
    } catch {
        Write-Err "Node.js 未安装，请从 https://nodejs.org/ 安装 Node.js 20 LTS"
        exit 1
    }
}

# Docker Desktop (Windows)
if ($Mode -eq "full") {
    try {
        $dockerVersion = docker --version 2>&1
        Write-Log "Docker 可用: $dockerVersion"
    } catch {
        Write-Warn "Docker Desktop 未安装或未运行"
        Write-Info "下载: https://www.docker.com/products/docker-desktop/"
        Write-Info "如果不需要大数据组件(Kafka/HDFS/Spark)，使用 -Mode dev 即可"
    }
}

Write-Log "基础工具链检查完成"

# ---------------------------------------------------------------------------
# 阶段 2: 安装 C++ 依赖库 (vcpkg 或手动)
# ---------------------------------------------------------------------------
Write-Hdr "阶段 2/5: 安装 C++ 依赖库"

$vcpkgRoot = "$env:USERPROFILE\vcpkg"
$vcpkgExe = "$vcpkgRoot\vcpkg.exe"
$useVcpkg = $false

if (Test-Path $vcpkgExe) {
    $useVcpkg = $true
    Write-Log "vcpkg 已安装: $vcpkgRoot"
} else {
    Write-Info "vcpkg 未安装, 尝试自动安装..."
    try {
        git clone https://github.com/Microsoft/vcpkg.git $vcpkgRoot 2>&1 | Out-Null
        & "$vcpkgRoot\bootstrap-vcpkg.bat" 2>&1 | Out-Null
        if (Test-Path $vcpkgExe) {
            $useVcpkg = $true
            Write-Log "vcpkg 安装完成"
        }
    } catch {
        Write-Warn "vcpkg 自动安装失败, 将尝试使用系统库"
    }
}

if ($useVcpkg) {
    Write-Log "安装 nlohmann_json..."
    & $vcpkgExe install nlohmann-json:x64-windows 2>&1 | Out-Null
    Write-Log "安装 libcurl..."
    & $vcpkgExe install curl:x64-windows 2>&1 | Out-Null

    # 集成到 MSVC
    & $vcpkgExe integrate install 2>&1 | Out-Null
    Write-Log "vcpkg 依赖安装完成"
} else {
    Write-Info "手动安装依赖:"
    Write-Info "  nlohmann_json: https://github.com/nlohmann/json/releases (header-only, 放到 include/)"
    Write-Info "  libcurl: Windows 自带或通过 vcpkg 安装"
}

# ---------------------------------------------------------------------------
# 阶段 3: 编译 C++ 引擎
# ---------------------------------------------------------------------------
Write-Hdr "阶段 3/5: 编译 C++ 对战引擎"

Set-Location $ProjectDir

# 清理旧构建
if (Test-Path "build") {
    Write-Log "清理旧构建..."
    Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
}

$cmakeArgs = @(
    "-B", "build",
    "-S", ".",
    "-DBUILD_TESTING=ON"
)

if ($useVcpkg) {
    $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgRoot\scripts\buildsystems\vcpkg.cmake"
}

Write-Log "配置 CMake..."
$cmakeOutput = & cmake @cmakeArgs 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Err "CMake 配置失败:"
    Write-Host $cmakeOutput
    exit 1
}

Write-Log "编译中 (Release 模式)..."
$buildOutput = & cmake --build build --config Release 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Err "编译失败:"
    Write-Host $buildOutput
    exit 1
}
Write-Log "C++ 引擎编译完成!"

# 运行测试
Write-Log "运行测试..."
try {
    & ctest --test-dir build -C Release --output-on-failure 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Log "全部测试通过!"
    } else {
        Write-Warn "部分测试未通过，请检查"
    }
} catch {
    Write-Warn "无法运行测试 (可能需要先构建测试目标)"
}

# ---------------------------------------------------------------------------
# 阶段 4: 构建前端 + Python API
# ---------------------------------------------------------------------------
Write-Hdr "阶段 4/5: 构建前端 & 安装 Python 依赖"

if ($Mode -eq "full" -or $Mode -eq "dev") {
    # Python 依赖
    Write-Log "安装 Python API 依赖..."
    Set-Location "$ProjectDir\api-server"
    try {
        python -m pip install fastapi uvicorn[standard] aiofiles websockets --quiet 2>&1 | Out-Null
        Write-Log "Python 依赖安装完成"
    } catch {
        Write-Warn "pip 安装失败, 请手动执行: pip install fastapi uvicorn aiofiles websockets"
    }

    # 前端构建
    Write-Log "构建前端..."
    Set-Location "$ProjectDir\frontend"
    try {
        npm install --no-audit --no-fund 2>&1 | Out-Null
        npm run build 2>&1 | Out-Null
        Write-Log "前端构建完成 → frontend/dist/"
    } catch {
        Write-Warn "前端构建失败, 请手动执行: cd frontend && npm install && npm run build"
    }
}

Set-Location $ProjectDir

# ---------------------------------------------------------------------------
# 阶段 5: 生成启动脚本 & 完成
# ---------------------------------------------------------------------------
Write-Hdr "阶段 5/5: 生成启动脚本"

# 生成启动批处理文件
$startBat = @"
@echo off
REM PokemonSimulator Windows 启动脚本 (自动生成)
echo ========================================
echo   PokemonSimulator 启动
echo ========================================
cd /d "$ProjectDir"

echo [1/2] 启动 API Server...
start "PokemonSimulator-API" cmd /c "cd api-server && python standalone_server.py"
timeout /t 3 /nobreak >nul

echo [2/2] 启动前端开发服务器...
start "PokemonSimulator-Frontend" cmd /c "cd frontend && npx vite --host 0.0.0.0"
timeout /t 2 /nobreak >nul

echo.
echo ========================================
echo   服务已启动!
echo.
echo   前端:  http://localhost:5173
echo   API:   http://localhost:8000
echo   WS:    ws://localhost:8000/ws
echo   健康:  http://localhost:8000/api/v1/health
echo ========================================
pause
"@

$startBatPath = "$ProjectDir\scripts\start.bat"
Set-Content -Path $startBatPath -Value $startBat -Encoding ASCII
Write-Log "生成启动脚本: scripts/start.bat"

# 生成停止批处理文件
$stopBat = @"
@echo off
REM PokemonSimulator Windows 停止脚本 (自动生成)
echo 停止 PokemonSimulator 服务...

for /f "tokens=5" %%a in ('netstat -ano ^| findstr ":8000.*LISTENING"') do (
    taskkill /PID %%a /F 2>nul
    echo API Server (PID %%a) 已停止
)

for /f "tokens=5" %%a in ('netstat -ano ^| findstr ":5173.*LISTENING"') do (
    taskkill /PID %%a /F 2>nul
    echo Frontend (PID %%a) 已停止
)

echo 所有服务已停止
pause
"@

$stopBatPath = "$ProjectDir\scripts\stop.bat"
Set-Content -Path $stopBatPath -Value $stopBat -Encoding ASCII
Write-Log "生成停止脚本: scripts/stop.bat"

# ---------------------------------------------------------------------------
# 完成
# ---------------------------------------------------------------------------
Write-Hdr "部署完成!"
Write-Log "模式: $Mode"
Write-Log "项目目录: $ProjectDir"

Write-Host ""
Write-Host "快速启动:" -ForegroundColor Cyan

if ($Mode -eq "full" -or $Mode -eq "dev") {
    Write-Host ""
    Write-Host "  方式 1 (推荐): 双击 scripts\start.bat" -ForegroundColor White
    Write-Host ""
    Write-Host "  方式 2 (手动):" -ForegroundColor White
    Write-Host "    终端 1: cd api-server && python standalone_server.py" -ForegroundColor Gray
    Write-Host "    终端 2: cd frontend && npx vite --host 0.0.0.0" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  然后访问:" -ForegroundColor White
    Write-Host "    前端:  http://localhost:5173" -ForegroundColor Green
    Write-Host "    API:   http://localhost:8000" -ForegroundColor Green
    Write-Host "    WS:    ws://localhost:8000/ws" -ForegroundColor Green
    Write-Host "    健康:  http://localhost:8000/api/v1/health" -ForegroundColor Green
}

if ($Mode -eq "minimal" -or $Mode -eq "full") {
    Write-Host ""
    Write-Host "  C++ 引擎:" -ForegroundColor White
    Write-Host "    二进制: $ProjectDir\build\Release\PokemonSimulator.exe" -ForegroundColor Gray
    Write-Host "    cd build && ctest -C Release --output-on-failure" -ForegroundColor Gray
    Write-Host "    .\build\Release\PokemonSimulator.exe --daemon" -ForegroundColor Gray
}

Write-Host ""
Write-Host "  停止服务: 双击 scripts\stop.bat" -ForegroundColor White
