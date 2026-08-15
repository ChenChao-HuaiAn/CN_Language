# CN 语言编译器 CI 门禁脚本（B-2 2026-08）
# 本地/远程 CI 统一入口：构建（零警告）+ 单测 + E2E
# 用法：powershell -ExecutionPolicy Bypass -File scripts/ci.ps1
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $root

Write-Host "[CI] 1/3 构建（/W4 /WX 零警告门禁）..." -ForegroundColor Cyan
& powershell -ExecutionPolicy Bypass -File build.ps1 -Config Debug
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 构建失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 2/3 单元测试..." -ForegroundColor Cyan
& target/Debug/cn_unit_tests.exe
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 单元测试失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 3/3 E2E 测试..." -ForegroundColor Cyan
python tests/e2e/run_e2e.py --cn target/Debug/cn.exe
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] E2E 失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 全部门禁通过" -ForegroundColor Green
