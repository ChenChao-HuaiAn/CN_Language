# CN_Language 夜间回归测试脚本
# 用途：每日自动运行完整回归测试套件
# 执行：.\tools\nightly_tests.ps1

param(
    [string]$BuildDir = "build",
    [string]$ReportDir = "test_reports"
)

Write-Host "===== CN_Language 夜间回归测试 =====" -ForegroundColor Cyan
Write-Host "时间: $(Get-Date)" -ForegroundColor Cyan
Write-Host ""

# 1. 清理并重新构建
Write-Host "[1/4] 清理并构建..." -ForegroundColor Yellow
if (Test-Path $BuildDir) {
    Remove-Item -Recurse -Force $BuildDir
}
cmake -B $BuildDir -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ CMake 配置失败" -ForegroundColor Red
    exit 1
}

cmake --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ 构建失败" -ForegroundColor Red
    exit 1
}

# 2. 运行所有回归测试
Write-Host "[2/4] 运行所有回归测试..." -ForegroundColor Yellow
Push-Location $BuildDir
$result_regression = ctest -L regression -V --output-on-failure 2>&1 | Out-String
$exit_regression = $LASTEXITCODE
Pop-Location

# 3. 运行所有性能测试
Write-Host "[3/4] 运行所有性能测试..." -ForegroundColor Yellow
Push-Location $BuildDir
$result_performance = ctest -L performance -V --output-on-failure 2>&1 | Out-String
$exit_performance = $LASTEXITCODE
Pop-Location

# 4. 运行所有阶段 7 测试
Write-Host "[4/4] 运行所有阶段 7 测试..." -ForegroundColor Yellow
Push-Location $BuildDir
$result_stage7 = ctest -L stage7 -V --output-on-failure 2>&1 | Out-String
$exit_stage7 = $LASTEXITCODE
Pop-Location

# 5. 生成报告
Write-Host "生成测试报告..." -ForegroundColor Yellow
if (-not (Test-Path $ReportDir)) {
    New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
}
$timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$report_file = "$ReportDir/nightly_report_$timestamp.txt"

@"
===== CN_Language 夜间回归测试报告 =====
时间: $(Get-Date)
构建目录: $BuildDir
构建类型: Release

--- 回归测试结果 ---
退出码: $exit_regression
$result_regression

--- 性能测试结果 ---
退出码: $exit_performance
$result_performance

--- 阶段 7 测试结果 ---
退出码: $exit_stage7
$result_stage7

=======================================
"@ | Out-File -FilePath $report_file -Encoding utf8

Write-Host "测试报告已保存至: $report_file" -ForegroundColor Cyan

# 6. 结果判断与告警
if ($exit_regression -ne 0 -or $exit_performance -ne 0 -or $exit_stage7 -ne 0) {
    Write-Host ""
    Write-Host "❌ 测试失败！请查看报告: $report_file" -ForegroundColor Red
    Write-Host ""
    Write-Host "失败详情:" -ForegroundColor Yellow
    if ($exit_regression -ne 0) { Write-Host "  - 回归测试失败" -ForegroundColor Red }
    if ($exit_performance -ne 0) { Write-Host "  - 性能测试失败" -ForegroundColor Red }
    if ($exit_stage7 -ne 0) { Write-Host "  - 阶段 7 测试失败" -ForegroundColor Red }
    exit 1
} else {
    Write-Host ""
    Write-Host "✅ 所有测试通过！" -ForegroundColor Green
    exit 0
}
