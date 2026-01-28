# CN语言输入函数演示 - 运行脚本
# 用法: 在 CN_Language 根目录下运行 .\examples\CN语言验收案例\008_终端输入演示\run_demo.ps1

param(
    [switch]$Simple  # 使用 -Simple 参数运行简单版本
)

$ErrorActionPreference = "Stop"

# 设置路径
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Resolve-Path "$scriptDir\..\..\..\.."
$cncExe = "$projectRoot\build\src\Release\cnc.exe"
$includeDir = "$projectRoot\include"
$libDir = "$projectRoot\build\src\runtime\Release"

# 检查编译器是否存在
if (-not (Test-Path $cncExe)) {
    Write-Host "错误: 找不到 cnc.exe，请先编译项目" -ForegroundColor Red
    Write-Host "  运行: cd $projectRoot; cmake --build build --config Release"
    exit 1
}

# 选择源文件
if ($Simple) {
    $sourceFile = "$scriptDir\simple_input.cn"
    $outputExe = "$scriptDir\simple_input.exe"
} else {
    $sourceFile = "$scriptDir\input_demo.cn"
    $outputExe = "$scriptDir\input_demo.exe"
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "CN语言输入函数演示" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 步骤1: 编译 CN 源码为 C 代码
Write-Host "[步骤1] 编译 CN 代码..." -ForegroundColor Yellow
$cFile = $sourceFile -replace "\.cn$", ".c"
& $cncExe $sourceFile --emit-c
if ($LASTEXITCODE -ne 0) {
    Write-Host "错误: CN 编译失败" -ForegroundColor Red
    exit 1
}
Write-Host "  生成 C 文件: $cFile" -ForegroundColor Green

# 步骤2: 使用 gcc 编译 C 代码
Write-Host "[步骤2] 编译 C 代码..." -ForegroundColor Yellow
$runtimeLib = "$libDir\cn_runtime.lib"
if (-not (Test-Path $runtimeLib)) {
    Write-Host "  警告: 找不到 cn_runtime.lib，尝试静态链接" -ForegroundColor Yellow
    $runtimeLib = ""
}

# 编译命令
$gccCmd = "gcc -I`"$includeDir`" -o `"$outputExe`" `"$cFile`""
if ($runtimeLib) {
    $gccCmd += " `"$runtimeLib`""
}

Write-Host "  执行: $gccCmd" -ForegroundColor Gray
Invoke-Expression $gccCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "错误: GCC 编译失败" -ForegroundColor Red
    exit 1
}
Write-Host "  生成可执行文件: $outputExe" -ForegroundColor Green

# 步骤3: 运行程序
Write-Host ""
Write-Host "[步骤3] 运行程序..." -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

& $outputExe

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "演示完成！" -ForegroundColor Green
