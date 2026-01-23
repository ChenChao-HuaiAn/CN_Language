#!/usr/bin/env pwsh
# 构建并测试 Hello Kernel 示例
# 这是一个便捷脚本，用于快速构建和测试简单的 CN Language 内核

param(
    [switch]$RunQemu = $false,
    [int]$TimeoutSeconds = 5
)

$ErrorActionPreference = "Stop"

Write-Host "=== CN Language Hello Kernel 构建与测试 ===" -ForegroundColor Cyan
Write-Host ""

# 定义路径
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$OsTestDir = Split-Path -Parent $ScriptDir
$KernelSource = Join-Path $OsTestDir "kernels\hello_kernel.cn"
$BootCode = Join-Path $OsTestDir "boot\boot_hello.c"
$OutputImage = Join-Path $OsTestDir "hello_kernel.elf"
$BuildScript = Join-Path $ScriptDir "build_kernel.ps1"

try {
    # 步骤 1: 构建内核
    Write-Host "[1/2] 构建 Hello Kernel..." -ForegroundColor Yellow
    & $BuildScript -KernelSource $KernelSource -OutputImage $OutputImage -BootCode $BootCode
    
    if ($LASTEXITCODE -ne 0) {
        throw "构建失败"
    }
    
    Write-Host "  ✓ 构建成功" -ForegroundColor Green
    Write-Host ""
    
    # 步骤 2: 运行 QEMU 测试（可选）
    if ($RunQemu) {
        # 检查 QEMU 是否可用
        $qemuAvailable = $null -ne (Get-Command qemu-system-x86_64 -ErrorAction SilentlyContinue)
        
        if (-not $qemuAvailable) {
            Write-Host "[跳过] QEMU 未安装，跳过模拟器测试" -ForegroundColor Yellow
            Write-Host "  提示: 安装 QEMU 后可以使用 -RunQemu 参数进行完整测试" -ForegroundColor Gray
        } else {
            Write-Host "[2/2] 在 QEMU 中测试内核..." -ForegroundColor Yellow
            
            $QemuScript = Join-Path $ScriptDir "run_qemu_test.ps1"
            
            # 预期输出包含内核的关键消息
            $ExpectedOutput = "Hello from CN Kernel"
            
            & $QemuScript -KernelImage $OutputImage -TimeoutSeconds $TimeoutSeconds -ExpectedOutput $ExpectedOutput
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host "  ✓ QEMU 测试通过" -ForegroundColor Green
            } else {
                Write-Host "  ✗ QEMU 测试失败" -ForegroundColor Red
                throw "QEMU 测试未通过"
            }
        }
    } else {
        Write-Host "[提示] 使用 -RunQemu 参数可以在 QEMU 中测试内核" -ForegroundColor Gray
    }
    
    Write-Host ""
    Write-Host "=== 完成！===" -ForegroundColor Green
    Write-Host ""
    Write-Host "下一步："
    Write-Host "  1. 如果你有 QEMU，运行: pwsh $($MyInvocation.MyCommand.Name) -RunQemu"
    Write-Host "  2. 查看生成的镜像: $OutputImage"
    Write-Host "  3. 修改内核源码: $KernelSource"
    Write-Host ""
    
} catch {
    Write-Host ""
    Write-Host "=== 失败 ===" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
} finally {
    # 清理临时文件（保留镜像文件）
}
