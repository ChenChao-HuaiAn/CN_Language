# CN 语言编译器一键构建脚本（B-3 2026-08）
# 用法：.\build.ps1 [-Config Debug|Release] [-Test]
#   -Test   构建后运行全量单测 + E2E
# 自动探测 cmake（VS 2022 自带 或 PATH）
param(
    [string]$Config = "Debug",
    [switch]$Test
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

# ---- 1. 定位 cmake（VS 自带优先，PATH 兜底） ----
$cmakeCandidates = @(
    "C:\Program Files\CMake\bin\cmake.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)
$cmake = $cmakeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $cmake) {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { $cmake = $cmd.Source }
}
if (-not $cmake) {
    Write-Host "[CN] 错误：未找到 cmake。请安装 Visual Studio 2022（含 C++ 桌面开发）或 CMake 并加入 PATH。" -ForegroundColor Red
    exit 1
}
Write-Host "[CN] 使用 cmake: $cmake" -ForegroundColor Cyan

# ---- 2. 配置 + 构建（零警告 /W4 /WX 由 CMakeLists 保证） ----
& $cmake -S . -B target/build
if ($LASTEXITCODE -ne 0) { Write-Host "[CN] CMake 配置失败" -ForegroundColor Red; exit $LASTEXITCODE }
& $cmake --build target/build --config $Config
if ($LASTEXITCODE -ne 0) { Write-Host "[CN] 构建失败（编译警告会被 /WX 拦截）" -ForegroundColor Red; exit $LASTEXITCODE }
Write-Host "[CN] 构建成功: target/$Config/cn.exe" -ForegroundColor Green

# ---- 3. 可选：全量测试门禁 ----
if ($Test) {
    $unitExe = "target/$Config/cn_unit_tests.exe"
    if (Test-Path $unitExe) {
        Write-Host "`n[CN] 运行单元测试..." -ForegroundColor Cyan
        & $unitExe
        if ($LASTEXITCODE -ne 0) { Write-Host "[CN] 单元测试失败" -ForegroundColor Red; exit $LASTEXITCODE }
    }
    Write-Host "[CN] 运行 E2E 测试..." -ForegroundColor Cyan
    python tests/e2e/run_e2e.py --cn "target/$Config/cn.exe"
    if ($LASTEXITCODE -ne 0) { Write-Host "[CN] E2E 测试失败" -ForegroundColor Red; exit $LASTEXITCODE }
    Write-Host "[CN] 全部测试通过 ✓" -ForegroundColor Green
}
