#!/usr/bin/env pwsh
# 验证启动代码和链接脚本的完整脚本
# 用法: ./verify_boot.ps1

param(
    [string]$KernelSource = "kernels/minimal_kernel.cn",
    [string]$OutputImage = "test_kernel_verify.elf",
    [switch]$RunQemu = $false
)

$ErrorActionPreference = "Stop"

Write-Host "=== 启动代码与链接脚本验证 ===" -ForegroundColor Cyan
Write-Host ""

# 步骤 1: 构建内核
Write-Host "[步骤 1/3] 构建内核镜像..." -ForegroundColor Yellow
try {
    & ./scripts/build_kernel.ps1 -KernelSource $KernelSource -OutputImage $OutputImage
    if ($LASTEXITCODE -ne 0) {
        throw "内核构建失败"
    }
} catch {
    Write-Host "构建失败: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "✓ 内核构建成功" -ForegroundColor Green
Write-Host ""

# 步骤 2: 验证镜像文件
Write-Host "[步骤 2/3] 验证镜像文件..." -ForegroundColor Yellow

if (-not (Test-Path $OutputImage)) {
    Write-Host "错误: 未生成镜像文件: $OutputImage" -ForegroundColor Red
    exit 1
}

$ImageInfo = Get-Item $OutputImage
Write-Host "  镜像文件: $OutputImage"
Write-Host "  文件大小: $($ImageInfo.Length) 字节"

# 在非 Windows 平台上，可以使用 file 命令检查文件类型
if (-not ($IsWindows -or $env:OS -match "Windows")) {
    try {
        $FileType = & file $OutputImage 2>&1
        Write-Host "  文件类型: $FileType"
        
        if ($FileType -match "ELF.*x86-64") {
            Write-Host "  ✓ 确认为 x86_64 ELF 格式" -ForegroundColor Green
        }
    } catch {
        Write-Host "  ⚠ 无法检查文件类型（需要 file 命令）" -ForegroundColor Yellow
    }
} else {
    Write-Host "  ⚠ Windows 平台，跳过 ELF 格式验证" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "✓ 镜像验证完成" -ForegroundColor Green
Write-Host ""

# 步骤 3: QEMU 测试（可选）
if ($RunQemu) {
    Write-Host "[步骤 3/3] 运行 QEMU 测试..." -ForegroundColor Yellow
    
    # 检查 QEMU 是否可用
    try {
        $null = Get-Command qemu-system-x86_64 -ErrorAction Stop
    } catch {
        Write-Host "  ⚠ QEMU 未安装，跳过运行测试" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "=== 验证完成（跳过 QEMU 测试）===" -ForegroundColor Green
        exit 0
    }
    
    try {
        & ./scripts/run_qemu_test.ps1 -KernelImage $OutputImage -TimeoutSeconds 3
        if ($LASTEXITCODE -ne 0) {
            throw "QEMU 测试失败"
        }
    } catch {
        Write-Host "QEMU 测试失败: $($_.Exception.Message)" -ForegroundColor Red
        exit 1
    }
    
    Write-Host ""
    Write-Host "✓ QEMU 测试完成" -ForegroundColor Green
    Write-Host ""
    Write-Host "=== 所有验证通过！===" -ForegroundColor Green
} else {
    Write-Host "[步骤 3/3] QEMU 测试（跳过）" -ForegroundColor Gray
    Write-Host "  提示: 使用 -RunQemu 参数启用 QEMU 测试"
    Write-Host ""
    Write-Host "=== 验证完成 ===" -ForegroundColor Green
}

# 清理临时文件
if (Test-Path $OutputImage) {
    Remove-Item $OutputImage -Force
}
