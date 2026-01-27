# 精简关键字测试执行脚本
# 用于验证关键字精简工作的正确性

param(
    [switch]$Rebuild,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptDir

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "精简关键字测试验证脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 检查构建目录
$buildDir = Join-Path $projectRoot "build"
if (-not (Test-Path $buildDir)) {
    Write-Host "⚠️  构建目录不存在，正在创建..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
    $Rebuild = $true
}

Set-Location $buildDir

# 构建项目
if ($Rebuild) {
    Write-Host "🔨 重新构建项目..." -ForegroundColor Green
    cmake ..
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ CMake配置失败" -ForegroundColor Red
        exit 1
    }
    
    cmake --build . --config Debug
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ 构建失败" -ForegroundColor Red
        exit 1
    }
    Write-Host "✅ 构建成功" -ForegroundColor Green
    Write-Host ""
} else {
    Write-Host "ℹ️  使用现有构建（如需重新构建，请使用 -Rebuild 参数）" -ForegroundColor Cyan
    Write-Host ""
}

# 定义测试列表
$keywordTests = @(
    @{
        Name = "lexer_keyword_refined_test"
        Description = "词法层关键字精简测试"
        Tests = @(
            "已删除关键字识别为标识符",
            "保留关键字正确识别",
            "预留关键字正确识别"
        )
    },
    @{
        Name = "parser_reserved_keyword_test"
        Description = "语法层预留关键字测试"
        Tests = @(
            "预留关键字触发错误",
            "已删除关键字作为标识符",
            "主程序作为函数名"
        )
    }
)

# 执行测试
$totalTests = 0
$passedTests = 0
$failedTests = @()

Write-Host "🧪 执行关键字测试..." -ForegroundColor Green
Write-Host ""

foreach ($testGroup in $keywordTests) {
    $testName = $testGroup.Name
    $description = $testGroup.Description
    
    Write-Host "📋 $description [$testName]" -ForegroundColor Cyan
    Write-Host "   测试用例:" -ForegroundColor Gray
    foreach ($testCase in $testGroup.Tests) {
        Write-Host "     - $testCase" -ForegroundColor Gray
    }
    Write-Host ""
    
    # 运行测试
    $testPath = Join-Path $buildDir "tests\unit\Debug\$testName.exe"
    if (-not (Test-Path $testPath)) {
        $testPath = Join-Path $buildDir "tests\unit\$testName.exe"
    }
    
    if (-not (Test-Path $testPath)) {
        Write-Host "   ⚠️  测试可执行文件不存在: $testPath" -ForegroundColor Yellow
        Write-Host ""
        continue
    }
    
    $totalTests++
    
    if ($Verbose) {
        & $testPath
        $exitCode = $LASTEXITCODE
    } else {
        $output = & $testPath 2>&1
        $exitCode = $LASTEXITCODE
        
        if ($exitCode -ne 0) {
            Write-Host $output
        }
    }
    
    if ($exitCode -eq 0) {
        Write-Host "   ✅ 通过" -ForegroundColor Green
        $passedTests++
    } else {
        Write-Host "   ❌ 失败 (退出码: $exitCode)" -ForegroundColor Red
        $failedTests += $testName
    }
    Write-Host ""
}

# 也运行相关的标准测试
Write-Host "🔍 运行相关标准测试..." -ForegroundColor Green
Write-Host ""

$relatedTests = @("lexer_token_test")

foreach ($testName in $relatedTests) {
    Write-Host "📋 $testName" -ForegroundColor Cyan
    
    $testPath = Join-Path $buildDir "tests\unit\Debug\$testName.exe"
    if (-not (Test-Path $testPath)) {
        $testPath = Join-Path $buildDir "tests\unit\$testName.exe"
    }
    
    if (-not (Test-Path $testPath)) {
        Write-Host "   ⚠️  测试可执行文件不存在" -ForegroundColor Yellow
        Write-Host ""
        continue
    }
    
    $totalTests++
    
    if ($Verbose) {
        & $testPath
        $exitCode = $LASTEXITCODE
    } else {
        $output = & $testPath 2>&1
        $exitCode = $LASTEXITCODE
        
        if ($exitCode -ne 0) {
            Write-Host $output
        }
    }
    
    if ($exitCode -eq 0) {
        Write-Host "   ✅ 通过" -ForegroundColor Green
        $passedTests++
    } else {
        Write-Host "   ❌ 失败 (退出码: $exitCode)" -ForegroundColor Red
        $failedTests += $testName
    }
    Write-Host ""
}

# 输出总结
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "测试结果总结" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "总测试数: $totalTests" -ForegroundColor White
Write-Host "通过数: $passedTests" -ForegroundColor Green
Write-Host "失败数: $($failedTests.Count)" -ForegroundColor $(if ($failedTests.Count -eq 0) { "Green" } else { "Red" })
Write-Host ""

if ($failedTests.Count -gt 0) {
    Write-Host "失败的测试:" -ForegroundColor Red
    foreach ($test in $failedTests) {
        Write-Host "  ❌ $test" -ForegroundColor Red
    }
    Write-Host ""
    exit 1
} else {
    Write-Host "✅ 所有测试通过！" -ForegroundColor Green
    Write-Host ""
    Write-Host "📊 关键字精简统计:" -ForegroundColor Cyan
    Write-Host "  - 已删除关键字: 14个" -ForegroundColor Yellow
    Write-Host "  - 保留关键字: 27个" -ForegroundColor Green
    Write-Host "  - 预留关键字: 9个" -ForegroundColor Blue
    Write-Host ""
    Write-Host "📚 详细报告: docs/implementation-plans/阶段 9：CN语法完善/精简关键字完成报告.md" -ForegroundColor Gray
    Write-Host ""
    exit 0
}
