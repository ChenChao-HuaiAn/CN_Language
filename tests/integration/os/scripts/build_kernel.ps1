#!/usr/bin/env pwsh
# 编译内核的构建脚本
# 用法: ./build_kernel.ps1 <内核源文件.cn> <输出镜像文件>

param(
    [Parameter(Mandatory=$true)]
    [string]$KernelSource,
    
    [Parameter(Mandatory=$true)]
    [string]$OutputImage,
    
    [string]$CncPath = "../../../build/src/cnc.exe",
    [string]$BootCode = "boot/boot.c",
    [string]$LinkerScript = "boot/linker.ld"
)

$ErrorActionPreference = "Stop"

Write-Host "=== CN Language 内核构建脚本 ===" -ForegroundColor Cyan
Write-Host ""

# 检查文件是否存在
if (-not (Test-Path $KernelSource)) {
    Write-Host "错误: 找不到内核源文件: $KernelSource" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $CncPath)) {
    Write-Host "错误: 找不到 cnc 编译器: $CncPath" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $BootCode)) {
    Write-Host "错误: 找不到启动代码: $BootCode" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $LinkerScript)) {
    Write-Host "错误: 找不到链接脚本: $LinkerScript" -ForegroundColor Red
    exit 1
}

# 创建临时目录
$TempDir = "temp_build"
if (Test-Path $TempDir) {
    Remove-Item -Recurse -Force $TempDir
}
New-Item -ItemType Directory -Path $TempDir | Out-Null

try {
    # 步骤 1: 使用 freestanding 模式编译 CN 源文件
    Write-Host "[1/4] 编译 CN 内核源文件..." -ForegroundColor Yellow
    $KernelC = Join-Path $TempDir "kernel.c"
    # 使用 -c 选项仅生成 C 文件
    & $CncPath $KernelSource --freestanding -c
    if ($LASTEXITCODE -ne 0) {
        throw "CN 编译失败"
    }
    
    # cnc 生成的 C 文件与源文件同名，只是扩展名为 .c
    $GeneratedC = $KernelSource -replace '\.cn$', '.c'
    if (-not (Test-Path $GeneratedC)) {
        throw "找不到生成的 C 文件: $GeneratedC"
    }
    
    # 移动到临时目录
    Move-Item -Path $GeneratedC -Destination $KernelC -Force
    Write-Host "  ✓ 生成 C 代码: $KernelC" -ForegroundColor Green
    
    # 步骤 2: 编译启动代码
    Write-Host "[2/4] 编译启动代码..." -ForegroundColor Yellow
    $BootObj = Join-Path $TempDir "boot.o"
    & gcc -c $BootCode -o $BootObj -ffreestanding -nostdlib -O2
    if ($LASTEXITCODE -ne 0) {
        throw "启动代码编译失败"
    }
    Write-Host "  ✓ 生成目标文件: $BootObj" -ForegroundColor Green
    
    # 步骤 3: 编译内核 C 代码
    Write-Host "[3/4] 编译内核 C 代码..." -ForegroundColor Yellow
    $KernelObj = Join-Path $TempDir "kernel.o"
    $IncludeDir = "../../../include"
    & gcc -c $KernelC -o $KernelObj -ffreestanding -nostdlib -O2 -I$IncludeDir
    if ($LASTEXITCODE -ne 0) {
        throw "内核 C 代码编译失败"
    }
    Write-Host "  ✓ 生成目标文件: $KernelObj" -ForegroundColor Green
    
    # 步骤 4: 链接生成内核镜像
    Write-Host "[4/4] 链接内核镜像..." -ForegroundColor Yellow
    & gcc -T $LinkerScript -o $OutputImage $BootObj $KernelObj -ffreestanding -nostdlib -lgcc
    if ($LASTEXITCODE -ne 0) {
        throw "链接失败"
    }
    Write-Host "  ✓ 生成内核镜像: $OutputImage" -ForegroundColor Green
    
    Write-Host ""
    Write-Host "=== 构建成功！===" -ForegroundColor Green
    Write-Host "内核镜像: $OutputImage"
    
} catch {
    Write-Host ""
    Write-Host "=== 构建失败 ===" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
} finally {
    # 清理临时目录
    if (Test-Path $TempDir) {
        Remove-Item -Recurse -Force $TempDir
    }
}
