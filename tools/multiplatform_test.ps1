# CN_Language 多平台构建与运行验证脚本 (Windows PowerShell)
# 用途：验证编译器在 Windows 平台上的完整功能
# 执行：.\tools\multiplatform_test.ps1

param(
    [string]$BuildDir = "build_multiplatform",
    [string]$ReportDir = "test_reports",
    [switch]$CleanBuild = $false
)

Write-Host "===== CN_Language 多平台构建与运行验证 (Windows) =====" -ForegroundColor Cyan
Write-Host "时间: $(Get-Date)" -ForegroundColor Cyan
Write-Host "操作系统: Windows" -ForegroundColor Cyan
Write-Host "架构: $([System.Environment]::Is64BitOperatingSystem ? 'x64' : 'x86')" -ForegroundColor Cyan
Write-Host "PowerShell 版本: $($PSVersionTable.PSVersion)" -ForegroundColor Cyan
Write-Host ""

# 记录开始时间
$start_time = Get-Date

# 检查依赖工具
Write-Host "[检查] 验证依赖工具..." -ForegroundColor Yellow
$tools_ok = $true

# 检查 CMake
$cmake_version = $null
try {
    $cmake_version = cmake --version 2>&1 | Select-String "cmake version" | ForEach-Object { $_.ToString() }
    if ($cmake_version) {
        Write-Host "  ✓ CMake: $cmake_version" -ForegroundColor Green
    } else {
        Write-Host "  ✗ CMake 未安装或不在 PATH 中" -ForegroundColor Red
        $tools_ok = $false
    }
} catch {
    Write-Host "  ✗ CMake 未安装或不在 PATH 中" -ForegroundColor Red
    $tools_ok = $false
}

# 检查 C 编译器 (MSVC, MinGW, Clang)
$cc_found = $false
$cc_name = ""

# 尝试检测 MSVC
try {
    $vsinfo = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath 2>$null
    if ($vsinfo) {
        Write-Host "  ✓ C 编译器: MSVC (Visual Studio)" -ForegroundColor Green
        $cc_found = $true
        $cc_name = "MSVC"
    }
} catch {}

# 尝试检测 GCC
if (-not $cc_found) {
    $gcc_version = gcc --version 2>&1 | Select-Object -First 1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  ✓ C 编译器: GCC - $gcc_version" -ForegroundColor Green
        $cc_found = $true
        $cc_name = "GCC"
    }
}

# 尝试检测 Clang
if (-not $cc_found) {
    $clang_version = clang --version 2>&1 | Select-Object -First 1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  ✓ C 编译器: Clang - $clang_version" -ForegroundColor Green
        $cc_found = $true
        $cc_name = "Clang"
    }
}

if (-not $cc_found) {
    Write-Host "  ✗ 未找到 C 编译器 (MSVC/GCC/Clang)" -ForegroundColor Red
    $tools_ok = $false
}

if (-not $tools_ok) {
    Write-Host ""
    Write-Host "❌ 依赖工具检查失败，请安装所需工具后重试" -ForegroundColor Red
    exit 1
}

Write-Host ""

# 1. 清理并重新构建
if ($CleanBuild -and (Test-Path $BuildDir)) {
    Write-Host "[1/6] 清理旧构建目录..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
    Write-Host "  ✓ 已清理" -ForegroundColor Green
} elseif (-not (Test-Path $BuildDir)) {
    Write-Host "[1/6] 创建构建目录..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    Write-Host "  ✓ 已创建" -ForegroundColor Green
} else {
    Write-Host "[1/6] 使用现有构建目录..." -ForegroundColor Yellow
}
Write-Host ""

# 2. CMake 配置
Write-Host "[2/6] CMake 配置..." -ForegroundColor Yellow
Push-Location $BuildDir
try {
    $cmake_config = cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | Out-String
    $cmake_config_result = $LASTEXITCODE
} catch {
    Write-Host "❌ CMake 配置异常: $($_.Exception.Message)" -ForegroundColor Red
    Pop-Location
    exit 1
}
Pop-Location

if ($cmake_config_result -ne 0) {
    Write-Host "❌ CMake 配置失败" -ForegroundColor Red
    Write-Host $cmake_config
    exit 1
}
Write-Host "  ✓ 配置成功" -ForegroundColor Green
Write-Host ""

# 3. 编译构建
Write-Host "[3/6] 编译构建..." -ForegroundColor Yellow
Push-Location $BuildDir
try {
    $build_output = cmake --build . --config Release 2>&1 | Out-String
    $build_result = $LASTEXITCODE
} catch {
    Write-Host "❌ 构建异常: $($_.Exception.Message)" -ForegroundColor Red
    Pop-Location
    exit 1
}
Pop-Location

if ($build_result -ne 0) {
    Write-Host "❌ 构建失败" -ForegroundColor Red
    Write-Host $build_output
    exit 1
}
Write-Host "  ✓ 构建成功" -ForegroundColor Green
Write-Host ""

# 4. 运行核心测试套件
Write-Host "[4/6] 运行核心测试套件..." -ForegroundColor Yellow

# 运行单元测试
Push-Location $BuildDir
$unit_test_output = ctest -R "unit" -C Release --output-on-failure 2>&1 | Out-String
$unit_test_result = $LASTEXITCODE
Pop-Location

# 运行集成测试
Push-Location $BuildDir
$integration_test_output = ctest -R "integration" -C Release --output-on-failure 2>&1 | Out-String
$integration_test_result = $LASTEXITCODE
Pop-Location

# 统计测试结果
$unit_test_count = ($unit_test_output | Select-String "tests passed").ToString() -replace '.*?(\d+) tests? passed.*', '$1'
$integration_test_count = ($integration_test_output | Select-String "tests passed").ToString() -replace '.*?(\d+) tests? passed.*', '$1'

if ($unit_test_result -eq 0) {
    Write-Host "  ✓ 单元测试通过 ($unit_test_count 项)" -ForegroundColor Green
} else {
    Write-Host "  ✗ 单元测试失败" -ForegroundColor Red
}

if ($integration_test_result -eq 0) {
    Write-Host "  ✓ 集成测试通过 ($integration_test_count 项)" -ForegroundColor Green
} else {
    Write-Host "  ✗ 集成测试失败" -ForegroundColor Red
}
Write-Host ""

# 5. 核心功能验证
Write-Host "[5/6] 核心功能验证..." -ForegroundColor Yellow

# 查找 cnc 可执行文件
$cnc_exe = ""
if (Test-Path "$BuildDir\Release\cnc.exe") {
    $cnc_exe = "$BuildDir\Release\cnc.exe"
} elseif (Test-Path "$BuildDir\Debug\cnc.exe") {
    $cnc_exe = "$BuildDir\Debug\cnc.exe"
} elseif (Test-Path "$BuildDir\cnc.exe") {
    $cnc_exe = "$BuildDir\cnc.exe"
}

$test_passed = 0
$test_failed = 0

if ($cnc_exe -and (Test-Path $cnc_exe)) {
    Write-Host "  → 使用编译器: $cnc_exe" -ForegroundColor Cyan
    
    # 测试 1: 编译 hello_world.cn
    Write-Host "    [1] 测试编译 hello_world.cn..." -ForegroundColor Gray
    $temp_output = "$BuildDir\test_hello.c"
    & $cnc_exe examples\hello_world.cn -o $temp_output 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0 -and (Test-Path $temp_output)) {
        Write-Host "      ✓ hello_world.cn 编译成功" -ForegroundColor Green
        $test_passed++
    } else {
        Write-Host "      ✗ hello_world.cn 编译失败" -ForegroundColor Red
        $test_failed++
    }
    
    # 测试 2: 编译 array_examples.cn
    Write-Host "    [2] 测试编译 array_examples.cn..." -ForegroundColor Gray
    $temp_output = "$BuildDir\test_array.c"
    & $cnc_exe examples\array_examples.cn -o $temp_output 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0 -and (Test-Path $temp_output)) {
        Write-Host "      ✓ array_examples.cn 编译成功" -ForegroundColor Green
        $test_passed++
    } else {
        Write-Host "      ✗ array_examples.cn 编译失败" -ForegroundColor Red
        $test_failed++
    }
    
    # 测试 3: 编译 function_examples.cn
    Write-Host "    [3] 测试编译 function_examples.cn..." -ForegroundColor Gray
    $temp_output = "$BuildDir\test_function.c"
    & $cnc_exe examples\function_examples.cn -o $temp_output 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0 -and (Test-Path $temp_output)) {
        Write-Host "      ✓ function_examples.cn 编译成功" -ForegroundColor Green
        $test_passed++
    } else {
        Write-Host "      ✗ function_examples.cn 编译失败" -ForegroundColor Red
        $test_failed++
    }
    
    # 测试 4: freestanding 模式编译
    Write-Host "    [4] 测试 freestanding 模式..." -ForegroundColor Gray
    $temp_output = "$BuildDir\test_freestanding.c"
    & $cnc_exe examples\freestanding_example.cn --freestanding -o $temp_output 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0 -and (Test-Path $temp_output)) {
        Write-Host "      ✓ freestanding 模式编译成功" -ForegroundColor Green
        $test_passed++
    } else {
        Write-Host "      ✗ freestanding 模式编译失败" -ForegroundColor Red
        $test_failed++
    }
    
    Write-Host ""
    Write-Host "  功能验证结果: $test_passed 通过, $test_failed 失败" -ForegroundColor Cyan
} else {
    Write-Host "  ✗ 未找到 cnc 可执行文件" -ForegroundColor Red
    $test_failed = 4
}
Write-Host ""

# 6. 生成详细报告
Write-Host "[6/6] 生成验证报告..." -ForegroundColor Yellow
if (-not (Test-Path $ReportDir)) {
    New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
}

$end_time = Get-Date
$duration = $end_time - $start_time
$timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$report_file = "$ReportDir/multiplatform_windows_$timestamp.txt"

@"
===== CN_Language 多平台构建与运行验证报告 =====
平台: Windows
架构: $([System.Environment]::Is64BitOperatingSystem ? 'x64' : 'x86')
编译器: $cc_name
开始时间: $start_time
结束时间: $end_time
总耗时: $($duration.TotalSeconds) 秒

--- CMake 配置 ---
结果: $(if ($cmake_config_result -eq 0) { "成功" } else { "失败" })
$cmake_config

--- 构建结果 ---
结果: $(if ($build_result -eq 0) { "成功" } else { "失败" })
$build_output

--- 单元测试结果 ---
结果: $(if ($unit_test_result -eq 0) { "通过" } else { "失败" })
$unit_test_output

--- 集成测试结果 ---
结果: $(if ($integration_test_result -eq 0) { "通过" } else { "失败" })
$integration_test_output

--- 核心功能验证 ---
通过: $test_passed / $($test_passed + $test_failed)
失败: $test_failed / $($test_passed + $test_failed)

===============================================
"@ | Out-File -FilePath $report_file -Encoding utf8

Write-Host "  ✓ 报告已保存至: $report_file" -ForegroundColor Green
Write-Host ""

# 最终结果判断
$all_passed = ($cmake_config_result -eq 0) -and 
              ($build_result -eq 0) -and 
              ($unit_test_result -eq 0) -and 
              ($integration_test_result -eq 0) -and
              ($test_failed -eq 0)

Write-Host "===== 验证完成 =====" -ForegroundColor Cyan
Write-Host "总耗时: $([Math]::Round($duration.TotalSeconds, 2)) 秒" -ForegroundColor Cyan

if ($all_passed) {
    Write-Host ""
    Write-Host "✅ Windows 平台验证全部通过！" -ForegroundColor Green
    Write-Host "   - CMake 配置: 成功" -ForegroundColor Green
    Write-Host "   - 构建: 成功" -ForegroundColor Green
    Write-Host "   - 单元测试: 通过" -ForegroundColor Green
    Write-Host "   - 集成测试: 通过" -ForegroundColor Green
    Write-Host "   - 功能验证: 通过" -ForegroundColor Green
    exit 0
} else {
    Write-Host ""
    Write-Host "❌ Windows 平台验证失败！详情请查看报告: $report_file" -ForegroundColor Red
    if ($cmake_config_result -ne 0) { Write-Host "   - CMake 配置失败" -ForegroundColor Red }
    if ($build_result -ne 0) { Write-Host "   - 构建失败" -ForegroundColor Red }
    if ($unit_test_result -ne 0) { Write-Host "   - 单元测试失败" -ForegroundColor Red }
    if ($integration_test_result -ne 0) { Write-Host "   - 集成测试失败" -ForegroundColor Red }
    if ($test_failed -gt 0) { Write-Host "   - 功能验证失败 ($test_failed 项)" -ForegroundColor Red }
    exit 1
}
