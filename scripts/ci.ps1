# CN 语言编译器 CI 门禁脚本（B-2 2026-08）
# 本地/远程 CI 统一入口：构建（零警告）+ 单测 + E2E
# 用法：powershell -ExecutionPolicy Bypass -File scripts/ci.ps1
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $root

Write-Host "[CI] 0/4 ASCII 标识符门禁（跨机-a 防复发，GCC9 拒 UTF-8 标识符）..." -ForegroundColor Cyan
python scripts/check_ascii_idents.py
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] ASCII 标识符门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 0.5/4 关键字清单同步门禁（plans/024 §7.5 单一事实源）..." -ForegroundColor Cyan
python scripts/check_keywords_sync.py
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 关键字清单同步门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 0.7/4 规范覆盖映射门禁（支柱一·plans/026 §2.1·B3 口径可测格 100%——247-a 起 --strict）..." -ForegroundColor Cyan
python scripts/check_spec_coverage.py --strict
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 规范覆盖映射门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 1/4 构建（/W4 /WX 零警告门禁）..." -ForegroundColor Cyan
& powershell -ExecutionPolicy Bypass -File build.ps1 -Config Debug
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 构建失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 2/4 单元测试..." -ForegroundColor Cyan
& target/Debug/cn_unit_tests.exe
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 单元测试失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 3/4 E2E 测试（--jobs 8：非 v2 用例并行·v2 用例串行，248-a 用户裁决提速）..." -ForegroundColor Cyan
python tests/e2e/run_e2e.py --cn target/Debug/cn.exe --jobs 8
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] E2E 失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 4/4 全部门禁通过" -ForegroundColor Green
