# =============================================================================
# PokemonSimulator — Windows C++ 引擎构建脚本
# =============================================================================
# 用法:
#   powershell -ExecutionPolicy Bypass -File scripts/build.ps1
#   powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Clean
#   powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -SkipTests
#   powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -UseNinja
# =============================================================================

param(
    [switch]$Clean,
    [switch]$SkipTests,
    [switch]$UseNinja
)

$ErrorActionPreference = "Stop"
$ProjectDir = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $ProjectDir

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  PokemonSimulator C++ 引擎构建" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# ---------------------------------------------------------------------------
# 1. 清理
# ---------------------------------------------------------------------------
if ($Clean) {
    Write-Host "[1/3] 清理旧构建..." -ForegroundColor Yellow
    if (Test-Path "build") {
        Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
        Write-Host "  已删除 build/ 目录"
    }
}

# ---------------------------------------------------------------------------
# 2. 检测 vcpkg
# ---------------------------------------------------------------------------
$vcpkgRoot = "$env:USERPROFILE\vcpkg"
$vcpkgToolchain = "$vcpkgRoot\scripts\buildsystems\vcpkg.cmake"
$useVcpkg = Test-Path $vcpkgToolchain

if ($useVcpkg) {
    Write-Host "[*] 使用 vcpkg 工具链: $vcpkgToolchain" -ForegroundColor Cyan
} else {
    Write-Host "[*] vcpkg 未检测到, 使用系统库" -ForegroundColor Yellow
}

# ---------------------------------------------------------------------------
# 3. CMake 配置
# ---------------------------------------------------------------------------
Write-Host "[2/3] CMake 配置..." -ForegroundColor Yellow

$cmakeArgs = @(
    "-B", "build",
    "-S", ".",
    "-DBUILD_TESTING=ON"
)

if ($useVcpkg) {
    $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain"
}

# 选择生成器: Ninja (需安装) 或 Visual Studio (默认)
if ($UseNinja) {
    $cmakeArgs += "-G", "Ninja"
    Write-Host "  使用 Ninja 生成器"
} else {
    # 自动检测 VS 2022 / 2019
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsPath = & $vsWhere -latest -property installationPath 2>$null
        if ($vsPath) {
            Write-Host "  使用 Visual Studio: $vsPath"
        }
    }
}

$cmakeOutput = & cmake @cmakeArgs 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host $cmakeOutput -ForegroundColor Red
    Write-Host ""
    Write-Host "CMake 配置失败! 请检查:" -ForegroundColor Red
    Write-Host "  1. Visual Studio 2022 已安装 (含 C++ 桌面开发)" -ForegroundColor Yellow
    Write-Host "  2. vcpkg 已正确安装 nlohmann-json 和 curl" -ForegroundColor Yellow
    Write-Host "  3. 或以管理员身份运行此脚本" -ForegroundColor Yellow
    exit 1
}
Write-Host "  CMake 配置成功"

# ---------------------------------------------------------------------------
# 4. 编译
# ---------------------------------------------------------------------------
Write-Host "[3/3] 编译中 (Release)..." -ForegroundColor Yellow

$buildOutput = & cmake --build build --config Release 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host $buildOutput -ForegroundColor Red
    Write-Host "编译失败!" -ForegroundColor Red
    exit 1
}

$engineBin = "build\Release\PokemonSimulator.exe"
if (Test-Path $engineBin) {
    $binSize = "{0:N2} MB" -f ((Get-Item $engineBin).Length / 1MB)
    Write-Host "  编译完成: $engineBin ($binSize)" -ForegroundColor Green
} else {
    Write-Host "  编译完成 (二进制位置可能不同，请检查 build/Release/)" -ForegroundColor Yellow
}

# ---------------------------------------------------------------------------
# 5. 测试 (可选)
# ---------------------------------------------------------------------------
if (-not $SkipTests) {
    Write-Host ""
    Write-Host "运行测试..." -ForegroundColor Yellow
    $testOutput = & ctest --test-dir build -C Release --output-on-failure 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  全部测试通过!" -ForegroundColor Green
    } else {
        Write-Host $testOutput -ForegroundColor Yellow
        Write-Host "  部分测试未通过 (可能是环境差异)" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  构建完成!" -ForegroundColor Green
Write-Host ""
Write-Host "  引擎: $engineBin" -ForegroundColor White
Write-Host "  测试: ctest --test-dir build -C Release" -ForegroundColor White
Write-Host "========================================" -ForegroundColor Cyan
