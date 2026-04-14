# CN语言编译器内存泄漏检测脚本 (Windows)
# 使用 AddressSanitizer (ASan) 检测内存问题

param(
    [string]$BuildType = "Debug",
    [string]$TestFile = "",
    [switch]$Clean = $false,
    [switch]$Help = $false
)

$ErrorActionPreference = "Stop"

if ($Help) {
    Write-Host @"
CN语言编译器内存泄漏检测脚本

用法:
    .\run_asan.ps1 [选项]

选项:
    -BuildType    构建类型 (默认: Debug)
    -TestFile     指定测试文件路径 (可选)
    -Clean        清理构建目录后重新构建
    -Help         显示帮助信息

示例:
    .\run_asan.ps1                           # 构建并运行基本测试
    .\run_asan.ps1 -TestFile tests\test.cn   # 测试指定文件
    .\run_asan.ps1 -Clean                    # 清理后重新构建

"@
    exit 0
}

$ProjectRoot = $PSScriptRoot | Split-Path -Parent
$BuildDir = Join-Path $ProjectRoot "build_asan"

Write-Host "=== CN语言编译器内存泄漏检测 ===" -ForegroundColor Cyan
Write-Host "项目目录: $ProjectRoot"
Write-Host "构建目录: $BuildDir"
Write-Host "构建类型: $BuildType"
Write-Host ""

# 设置 ASan 环境变量
# 注意: Windows/MSVC 不支持 detect_leaks 选项
$env:ASAN_OPTIONS = "abort_on_error=1:detect_stack_use_after_return=1:check_initialization_order=1"
Write-Host "ASAN_OPTIONS: $($env:ASAN_OPTIONS)" -ForegroundColor Yellow

# 查找并添加 ASan DLL 到 PATH
$VsWherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $VsWherePath) {
    $VsInstallPath = & $VsWherePath -latest -property installationPath
    $MsvcPath = Join-Path $VsInstallPath "VC\Tools\MSVC"
    if (Test-Path $MsvcPath) {
        $LatestMsvc = Get-ChildItem $MsvcPath | Sort-Object -Descending | Select-Object -First 1
        $AsanDllPath = Join-Path $LatestMsvc.FullName "bin\Hostx64\x64"
        if (Test-Path $AsanDllPath) {
            $env:PATH = "$AsanDllPath;$($env:PATH)"
            Write-Host "已添加 ASan DLL 路径: $AsanDllPath" -ForegroundColor Green
        }
    }
}

# 清理构建目录
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "清理构建目录..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

# 配置 CMake
if (-not (Test-Path $BuildDir)) {
    Write-Host "配置 CMake (启用 ASan)..." -ForegroundColor Green
    cmake -B $BuildDir -S $ProjectRoot `
        -DCMAKE_BUILD_TYPE=$BuildType `
        -DENABLE_ASAN=ON
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake 配置失败!" -ForegroundColor Red
        exit 1
    }
}

# 构建
Write-Host "构建项目..." -ForegroundColor Green
cmake --build $BuildDir --config $BuildType

if ($LASTEXITCODE -ne 0) {
    Write-Host "构建失败!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "=== 运行内存检测 ===" -ForegroundColor Cyan

$CncPath = Join-Path $BuildDir "src\cnc.exe"

if (-not (Test-Path $CncPath)) {
    # 尝试在 Debug 子目录查找
    $CncPath = Join-Path $BuildDir "src\$BuildType\cnc.exe"
}

if (-not (Test-Path $CncPath)) {
    Write-Host "找不到编译器可执行文件: $CncPath" -ForegroundColor Red
    exit 1
}

Write-Host "编译器路径: $CncPath"

# 运行测试
if ($TestFile -ne "") {
    $TestFilePath = Join-Path $ProjectRoot $TestFile
    if (-not (Test-Path $TestFilePath)) {
        Write-Host "测试文件不存在: $TestFilePath" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "测试文件: $TestFilePath" -ForegroundColor Yellow
    Write-Host ""
    
    & $CncPath $TestFilePath
} else {
    # 运行基本测试
    Write-Host "运行编译器基本测试..." -ForegroundColor Yellow
    Write-Host ""
    
    # 测试帮助命令
    Write-Host "--- 测试: --help ---" -ForegroundColor Cyan
    & $CncPath --help
    
    Write-Host ""
    Write-Host "--- 测试: --version ---" -ForegroundColor Cyan
    & $CncPath --version
    
    # 如果有测试文件目录，运行简单测试
    $TestDir = Join-Path $ProjectRoot "tests"
    if (Test-Path $TestDir) {
        $SimpleTests = Get-ChildItem -Path $TestDir -Filter "*.cn" -Recurse | Select-Object -First 3
        
        foreach ($Test in $SimpleTests) {
            Write-Host ""
            Write-Host "--- 测试: $($Test.Name) ---" -ForegroundColor Cyan
            & $CncPath $Test.FullName
        }
    }
}

Write-Host ""
Write-Host "=== 内存检测完成 ===" -ForegroundColor Green
Write-Host "如果没有错误输出，说明未检测到内存问题。" -ForegroundColor Green
Write-Host "如果检测到内存问题，ASan 会输出详细的错误报告。" -ForegroundColor Yellow
