#!/usr/bin/env pwsh
# QEMU 测试运行脚本
# 用法: ./run_qemu_test.ps1 <内核镜像文件>

param(
    [Parameter(Mandatory=$true)]
    [string]$KernelImage,
    
    [int]$TimeoutSeconds = 5,
    [string]$QemuPath = "qemu-system-x86_64"
)

$ErrorActionPreference = "Stop"

Write-Host "=== QEMU 内核测试脚本 ===" -ForegroundColor Cyan
Write-Host ""

# 检查内核镜像是否存在
if (-not (Test-Path $KernelImage)) {
    Write-Host "错误: 找不到内核镜像: $KernelImage" -ForegroundColor Red
    exit 1
}

# 检查 QEMU 是否可用
try {
    $null = Get-Command $QemuPath -ErrorAction Stop
} catch {
    Write-Host "错误: 找不到 QEMU: $QemuPath" -ForegroundColor Red
    Write-Host "请确保 QEMU 已安装并在 PATH 中" -ForegroundColor Yellow
    exit 1
}

# 创建串口输出文件
$SerialOutput = "serial_output.txt"
if (Test-Path $SerialOutput) {
    Remove-Item $SerialOutput
}

Write-Host "启动 QEMU 虚拟机..." -ForegroundColor Yellow
Write-Host "内核镜像: $KernelImage"
Write-Host "超时时间: $TimeoutSeconds 秒"
Write-Host ""

# QEMU 启动参数：
# -kernel: 直接加载内核镜像
# -nographic: 无图形界面
# -serial: 串口输出到文件
# -no-reboot: 崩溃后不重启
# -display none: 禁用显示
try {
    $QemuProcess = Start-Process -FilePath $QemuPath `
        -ArgumentList "-kernel", $KernelImage, `
                      "-nographic", `
                      "-serial", "file:$SerialOutput", `
                      "-no-reboot", `
                      "-display", "none" `
        -NoNewWindow `
        -PassThru
    
    Write-Host "QEMU 进程已启动 (PID: $($QemuProcess.Id))" -ForegroundColor Green
    
    # 等待指定时间
    $Waited = 0
    while ($Waited -lt $TimeoutSeconds -and -not $QemuProcess.HasExited) {
        Start-Sleep -Seconds 1
        $Waited++
        Write-Host "." -NoNewline
    }
    Write-Host ""
    
    # 停止 QEMU
    if (-not $QemuProcess.HasExited) {
        Write-Host "超时，停止 QEMU..." -ForegroundColor Yellow
        Stop-Process -Id $QemuProcess.Id -Force
    }
    
    # 等待进程完全退出
    $QemuProcess.WaitForExit(2000)
    
    # 读取串口输出
    Write-Host ""
    Write-Host "=== 串口输出 ===" -ForegroundColor Cyan
    if (Test-Path $SerialOutput) {
        $Output = Get-Content $SerialOutput -Raw
        if ($Output) {
            Write-Host $Output
        } else {
            Write-Host "(无输出)" -ForegroundColor Gray
        }
    } else {
        Write-Host "(未生成串口输出文件)" -ForegroundColor Gray
    }
    
    Write-Host ""
    Write-Host "=== 测试完成 ===" -ForegroundColor Green
    
} catch {
    Write-Host ""
    Write-Host "=== 测试失败 ===" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
} finally {
    # 清理
    if (Test-Path $SerialOutput) {
        Remove-Item $SerialOutput
    }
}
