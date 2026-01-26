#!/usr/bin/env pwsh
# 构建 CN Language OS 内核演示程序
# 
# 用法:
#   pwsh build_os_kernel.ps1                    # 仅编译验证
#   pwsh build_os_kernel.ps1 -Build            # 构建完整内核镜像（需要 Linux）
#   pwsh build_os_kernel.ps1 -Build -RunQemu   # 构建并在 QEMU 中运行

param(
    [switch]$Build = $false,        # 构建完整 ELF 镜像（需要 Linux 或交叉编译器）
    [switch]$RunQemu = $false,      # 在 QEMU 中运行
    [int]$TimeoutSeconds = 10       # QEMU 运行超时时间
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host "  CN Language OS 内核演示程序构建脚本" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host ""

# 定义路径
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ExamplesDir = $ScriptDir
$ProjectRoot = Split-Path -Parent $ExamplesDir
$BuildDir = Join-Path $ProjectRoot "build"
$CncExe = Join-Path $BuildDir "src\Release\cnc.exe"
$RuntimeLibDir = Join-Path $BuildDir "src\runtime\Release"

# 源文件
$KernelSource = Join-Path $ExamplesDir "os_kernel_demo.cn"
$BootCode = Join-Path $ExamplesDir "boot_kernel_demo.c"
$LinkerScript = Join-Path $ProjectRoot "tests\integration\os\boot\linker.ld"

# 输出文件
$KernelC = Join-Path $ExamplesDir "os_kernel_demo.c"
$KernelObj = Join-Path $ExamplesDir "os_kernel_demo.o"
$BootObj = Join-Path $ExamplesDir "boot_kernel_demo.o"
$KernelElf = Join-Path $ExamplesDir "os_kernel_demo.elf"

Write-Host "[信息] 工作目录: $ExamplesDir" -ForegroundColor Gray
Write-Host "[信息] 编译器: $CncExe" -ForegroundColor Gray
Write-Host ""

# 检查文件是否存在
if (-not (Test-Path $KernelSource)) {
    Write-Host "[错误] 找不到内核源文件: $KernelSource" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $BootCode)) {
    Write-Host "[错误] 找不到启动代码: $BootCode" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $CncExe)) {
    Write-Host "[错误] 找不到编译器: $CncExe" -ForegroundColor Red
    Write-Host "[提示] 请先构建项目: cmake --build build" -ForegroundColor Yellow
    exit 1
}

try {
    # ========================================
    # 步骤 1: 编译 CN 内核源码
    # ========================================
    Write-Host "[1/4] 编译 CN Language 内核源码..." -ForegroundColor Yellow
    Write-Host "  源文件: $(Split-Path -Leaf $KernelSource)" -ForegroundColor Gray
    Write-Host "  目标: C 代码生成" -ForegroundColor Gray
    
    # 使用编译器生成 C 代码
    & $CncExe $KernelSource --freestanding -o $KernelC
    
    if ($LASTEXITCODE -ne 0) {
        throw "CN 编译器失败"
    }
    
    if (-not (Test-Path $KernelC)) {
        throw "生成的 C 文件不存在: $KernelC"
    }
    
    Write-Host "  ✓ CN 编译成功 -> $(Split-Path -Leaf $KernelC)" -ForegroundColor Green
    Write-Host ""
    
    # ========================================
    # 步骤 2: 编译生成的 C 代码
    # ========================================
    Write-Host "[2/4] 编译生成的 C 代码..." -ForegroundColor Yellow
    
    # 检测平台
    $IsWindows = $PSVersionTable.Platform -eq 'Win32NT' -or $null -eq $PSVersionTable.Platform
    
    if ($IsWindows -and -not $Build) {
        Write-Host "  平台: Windows (MinGW)" -ForegroundColor Gray
        Write-Host "  模式: 编译验证（不生成 ELF）" -ForegroundColor Gray
        
        # Windows: 仅编译验证，不生成 ELF
        gcc -c $KernelC -o $KernelObj -ffreestanding -nostdlib -O2 `
            -I"$ProjectRoot\include" `
            -DCN_FREESTANDING `
            -Wall -Wextra
        
        if ($LASTEXITCODE -ne 0) {
            throw "内核 C 代码编译失败"
        }
        
        Write-Host "  ✓ 编译验证通过（生成目标文件）" -ForegroundColor Green
        Write-Host ""
        
        # ========================================
        # 步骤 3: 编译启动代码
        # ========================================
        Write-Host "[3/4] 编译启动代码..." -ForegroundColor Yellow
        
        gcc -c $BootCode -o $BootObj -ffreestanding -nostdlib -O2 `
            -I"$ProjectRoot\include" `
            -Wall -Wextra
        
        if ($LASTEXITCODE -ne 0) {
            throw "启动代码编译失败"
        }
        
        Write-Host "  ✓ 启动代码编译成功" -ForegroundColor Green
        Write-Host ""
        
        Write-Host "[4/4] 跳过链接（Windows 不支持生成 ELF）" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "=====================================================" -ForegroundColor Green
        Write-Host "  编译验证完成！" -ForegroundColor Green
        Write-Host "=====================================================" -ForegroundColor Green
        Write-Host ""
        Write-Host "注意: Windows 上无法生成完整的内核镜像" -ForegroundColor Yellow
        Write-Host "要生成可运行的内核，请在 Linux 上运行此脚本" -ForegroundColor Yellow
        Write-Host "或安装 x86_64-elf-gcc 交叉编译器" -ForegroundColor Yellow
        Write-Host ""
        
    } elseif ($Build) {
        # Linux: 生成完整的 ELF 镜像
        Write-Host "  平台: Linux" -ForegroundColor Gray
        Write-Host "  模式: 完整构建" -ForegroundColor Gray
        
        # 编译内核 C 代码
        gcc -c $KernelC -o $KernelObj -ffreestanding -nostdlib -O2 `
            -I"$ProjectRoot/include" `
            -DCN_FREESTANDING `
            -Wall -Wextra
        
        if ($LASTEXITCODE -ne 0) {
            throw "内核 C 代码编译失败"
        }
        
        Write-Host "  ✓ 内核编译成功" -ForegroundColor Green
        Write-Host ""
        
        # ========================================
        # 步骤 3: 编译启动代码
        # ========================================
        Write-Host "[3/4] 编译启动代码..." -ForegroundColor Yellow
        
        gcc -c $BootCode -o $BootObj -ffreestanding -nostdlib -O2 `
            -I"$ProjectRoot/include" `
            -Wall -Wextra
        
        if ($LASTEXITCODE -ne 0) {
            throw "启动代码编译失败"
        }
        
        Write-Host "  ✓ 启动代码编译成功" -ForegroundColor Green
        Write-Host ""
        
        # ========================================
        # 步骤 4: 链接生成内核镜像
        # ========================================
        Write-Host "[4/4] 链接生成内核镜像..." -ForegroundColor Yellow
        
        # 构建运行时库路径
        $RuntimeLibs = @(
            Join-Path $RuntimeLibDir "libcn_runtime_core.a"
            Join-Path $RuntimeLibDir "libcn_runtime_memory.a"
            Join-Path $RuntimeLibDir "libcn_runtime_interrupt.a"
        )
        
        # 检查运行时库
        foreach ($lib in $RuntimeLibs) {
            if (-not (Test-Path $lib)) {
                Write-Host "  [警告] 运行时库不存在: $(Split-Path -Leaf $lib)" -ForegroundColor Yellow
            }
        }
        
        # 链接
        $RuntimeLibArgs = $RuntimeLibs -join " "
        gcc -o $KernelElf $BootObj $KernelObj `
            -T $LinkerScript `
            -ffreestanding -nostdlib -lgcc `
            $RuntimeLibArgs
        
        if ($LASTEXITCODE -ne 0) {
            throw "链接失败"
        }
        
        if (-not (Test-Path $KernelElf)) {
            throw "内核镜像文件未生成"
        }
        
        $ElfSize = (Get-Item $KernelElf).Length
        Write-Host "  ✓ 内核镜像生成成功" -ForegroundColor Green
        Write-Host "  大小: $ElfSize 字节" -ForegroundColor Gray
        Write-Host "  路径: $KernelElf" -ForegroundColor Gray
        Write-Host ""
        
        # ========================================
        # 步骤 5: 在 QEMU 中运行（可选）
        # ========================================
        if ($RunQemu) {
            Write-Host "[5/5] 在 QEMU 中运行内核..." -ForegroundColor Yellow
            
            $QemuAvailable = $null -ne (Get-Command qemu-system-x86_64 -ErrorAction SilentlyContinue)
            
            if (-not $QemuAvailable) {
                Write-Host "  [跳过] QEMU 未安装" -ForegroundColor Yellow
                Write-Host ""
            } else {
                Write-Host "  启动 QEMU (超时: ${TimeoutSeconds}秒)..." -ForegroundColor Gray
                
                # 启动 QEMU 并捕获输出
                $QemuArgs = @(
                    "-kernel", $KernelElf,
                    "-nographic",
                    "-serial", "stdio",
                    "-no-reboot"
                )
                
                $Process = Start-Process -FilePath "qemu-system-x86_64" `
                    -ArgumentList $QemuArgs `
                    -NoNewWindow -PassThru -RedirectStandardOutput "$ExamplesDir\qemu_output.txt"
                
                # 等待超时
                $Process | Wait-Process -Timeout $TimeoutSeconds -ErrorAction SilentlyContinue
                
                # 强制终止
                if (-not $Process.HasExited) {
                    $Process | Stop-Process -Force
                }
                
                # 读取输出
                if (Test-Path "$ExamplesDir\qemu_output.txt") {
                    Write-Host ""
                    Write-Host "  QEMU 输出:" -ForegroundColor Cyan
                    Write-Host "  " + ("-" * 50) -ForegroundColor Gray
                    Get-Content "$ExamplesDir\qemu_output.txt" | ForEach-Object {
                        Write-Host "  $_" -ForegroundColor White
                    }
                    Write-Host "  " + ("-" * 50) -ForegroundColor Gray
                    Write-Host ""
                }
                
                Write-Host "  ✓ QEMU 运行完成" -ForegroundColor Green
                Write-Host ""
            }
        }
        
        Write-Host "=====================================================" -ForegroundColor Green
        Write-Host "  构建完成！" -ForegroundColor Green
        Write-Host "=====================================================" -ForegroundColor Green
        Write-Host ""
        Write-Host "生成的文件:" -ForegroundColor Cyan
        Write-Host "  - 内核镜像: $KernelElf" -ForegroundColor White
        Write-Host ""
        Write-Host "运行内核:" -ForegroundColor Cyan
        Write-Host "  qemu-system-x86_64 -kernel $KernelElf -nographic -serial stdio" -ForegroundColor White
        Write-Host ""
        
    } else {
        Write-Host "  [提示] 使用 -Build 参数生成完整内核镜像" -ForegroundColor Yellow
        Write-Host ""
    }
    
} catch {
    Write-Host ""
    Write-Host "=====================================================" -ForegroundColor Red
    Write-Host "  构建失败" -ForegroundColor Red
    Write-Host "=====================================================" -ForegroundColor Red
    Write-Host ""
    Write-Host "错误信息: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    exit 1
} finally {
    # 清理临时文件（保留重要产物）
    # 保留: .c, .o, .elf 文件用于调试
}
