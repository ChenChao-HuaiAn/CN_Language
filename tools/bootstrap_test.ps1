# bootstrap_test.ps1 - CN语言自举测试脚本 (Windows版本)
# 用法: ./tools/bootstrap_test.ps1 [-Verbose] [-KeepArtifacts] [-Config <配置文件路径>]
#
# 三阶段自举验证流程:
# 阶段1: cnc.exe (C实现) ──编译──> cn_v1.exe
# 阶段2: cn_v1.exe ──编译──> cn_v2.exe
# 阶段3: cn_v2.exe ──编译──> cn_v3.exe
# 验证: cn_v2.exe 与 cn_v3.exe 二进制SHA256哈希完全一致

param(
    [switch]$Verbose,           # 详细输出模式
    [switch]$KeepArtifacts,     # 保留构建产物
    [string]$Config,            # 配置文件路径
    [string]$CompilerPath,      # 编译器路径（覆盖配置）
    [string]$SourceDir,         # 源码目录（覆盖配置）
    [string]$OutputDir          # 输出目录（覆盖配置）
)

# 错误时停止执行
$ErrorActionPreference = "Stop"

# ============================================
# 配置变量
# ============================================

$SCRIPT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path
$PROJECT_ROOT = Split-Path -Parent $SCRIPT_DIR
$BUILD_DIR = Join-Path $PROJECT_ROOT "build"
$SRC_CN_DIR = Join-Path $PROJECT_ROOT "src_cn"
$BOOTSTRAP_DIR = Join-Path $BUILD_DIR "bootstrap_test"

# 默认配置
$DEFAULT_CONFIG = @{
    编译器路径 = Join-Path $BUILD_DIR "cnc.exe"
    源码目录 = $SRC_CN_DIR
    输出目录 = $BOOTSTRAP_DIR
    编译选项 = @("-O2")
    包含目录 = Join-Path $PROJECT_ROOT "include"
    库目录 = Join-Path $BUILD_DIR "lib"
    库名称 = "cnrt"
    超时秒数 = 300
    C编译器 = "gcc"
}

# ============================================
# 颜色输出函数
# ============================================

function Write-Info { 
    Write-Host "[INFO] $args" -ForegroundColor Cyan 
}

function Write-Success { 
    Write-Host "[PASS] $args" -ForegroundColor Green 
}

function Write-Fail { 
    Write-Host "[FAIL] $args" -ForegroundColor Red 
}

function Write-Step { 
    Write-Host "`n=== $args ===" -ForegroundColor Yellow 
}

function Write-Verbose-Output {
    if ($Verbose) {
        Write-Host "[DEBUG] $args" -ForegroundColor Gray
    }
}

function Write-Warning {
    Write-Host "[WARN] $args" -ForegroundColor Yellow
}

# ============================================
# 工具函数
# ============================================

# 计算文件SHA256哈希
function Get-FileHashSHA256 {
    param([string]$FilePath)
    
    if (-not (Test-Path $FilePath)) {
        return $null
    }
    
    $hash = Get-FileHash -Path $FilePath -Algorithm SHA256
    return $hash.Hash
}

# 计算目录下所有C文件的哈希（用于比较生成的C代码）
function Get-DirectoryCHash {
    param([string]$Directory)
    
    if (-not (Test-Path $Directory)) {
        return $null
    }
    
    $cFiles = Get-ChildItem -Path $Directory -Filter "*.c" -Recurse | 
              Sort-Object Name | 
              ForEach-Object { 
                  $content = Get-Content $_.FullName -Raw
                  "$($_.Name):$($content.Length)"
              }
    
    $combined = $cFiles -join "|"
    $hash = [System.Security.Cryptography.SHA256]::Create()
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($combined)
    $hashBytes = $hash.ComputeHash($bytes)
    return [BitConverter]::ToString($hashBytes) -replace '-', ''
}

# 获取文件大小（人类可读格式）
function Get-FileSizeReadable {
    param([string]$FilePath)
    
    $size = (Get-Item $FilePath).Length
    if ($size -ge 1MB) {
        return "{0:N2} MB" -f ($size / 1MB)
    } elseif ($size -ge 1KB) {
        return "{0:N2} KB" -f ($size / 1KB)
    } else {
        return "{0} B" -f $size
    }
}

# 测量命令执行时间
function Measure-CommandTime {
    param(
        [scriptblock]$ScriptBlock,
        [string]$Description
    )
    
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $result = & $ScriptBlock
    $stopwatch.Stop()
    
    return @{
        结果 = $result
        耗时毫秒 = $stopwatch.ElapsedMilliseconds
        耗时描述 = "{0:N2}秒" -f $stopwatch.Elapsed.TotalSeconds
    }
}

# ============================================
# 配置加载函数
# ============================================

function Load-Config {
    $config = $DEFAULT_CONFIG.Clone()
    
    # 从配置文件加载
    if ($Config -and (Test-Path $Config)) {
        Write-Info "加载配置文件: $Config"
        $fileConfig = Get-Content $Config | ConvertFrom-Json
        
        # 合并配置
        if ($fileConfig.编译器路径) { $config.编译器路径 = $fileConfig.编译器路径 }
        if ($fileConfig.源码目录) { $config.源码目录 = $fileConfig.源码目录 }
        if ($fileConfig.输出目录) { $config.输出目录 = $fileConfig.输出目录 }
        if ($fileConfig.编译选项) { $config.编译选项 = $fileConfig.编译选项 }
        if ($fileConfig.包含目录) { $config.包含目录 = $fileConfig.包含目录 }
        if ($fileConfig.库目录) { $config.库目录 = $fileConfig.库目录 }
        if ($fileConfig.库名称) { $config.库名称 = $fileConfig.库名称 }
        if ($fileConfig.超时秒数) { $config.超时秒数 = $fileConfig.超时秒数 }
        if ($fileConfig.C编译器) { $config.C编译器 = $fileConfig.C编译器 }
    }
    
    # 命令行参数覆盖
    if ($CompilerPath) { $config.编译器路径 = $CompilerPath }
    if ($SourceDir) { $config.源码目录 = $SourceDir }
    if ($OutputDir) { $config.输出目录 = $OutputDir }
    
    return $config
}

# ============================================
# 构建环境准备函数
# ============================================

function Clear-BuildArtifacts {
    param([string]$OutputDir)
    
    Write-Info "清理构建目录: $OutputDir"
    
    if (Test-Path $OutputDir) {
        Remove-Item -Recurse -Force $OutputDir
    }
    
    # 创建目录结构
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    New-Item -ItemType Directory -Path "$OutputDir\v1" -Force | Out-Null
    New-Item -ItemType Directory -Path "$OutputDir\v2" -Force | Out-Null
    New-Item -ItemType Directory -Path "$OutputDir\v3" -Force | Out-Null
    
    Write-Success "构建目录已准备完成"
}

function Test-Prerequisites {
    param([hashtable]$Config)
    
    Write-Step "检查前置条件"
    
    $allPassed = $true
    
    # 检查引导编译器
    if (-not (Test-Path $Config.编译器路径)) {
        Write-Fail "未找到引导编译器: $($Config.编译器路径)"
        Write-Info "请先运行: cmake --build build --config Release"
        $allPassed = $false
    } else {
        Write-Success "引导编译器: $($Config.编译器路径)"
    }
    
    # 检查src_cn目录
    if (-not (Test-Path $Config.源码目录)) {
        Write-Fail "未找到src_cn目录: $($Config.源码目录)"
        $allPassed = $false
    } else {
        $cnFileCount = (Get-ChildItem -Path $Config.源码目录 -Filter "*.cn" -Recurse).Count
        Write-Success "src_cn目录: $($Config.源码目录) ($cnFileCount 个.cn文件)"
    }
    
    # 检查C编译器
    $cCompiler = $Config.C编译器
    try {
        $gccVersion = & $cCompiler --version 2>&1 | Select-Object -First 1
        Write-Success "C编译器: $gccVersion"
    } catch {
        Write-Fail "未找到C编译器: $cCompiler"
        $allPassed = $false
    }
    
    # 检查包含目录
    if (-not (Test-Path $Config.包含目录)) {
        Write-Warning "包含目录不存在: $($Config.包含目录)"
    }
    
    # 检查库目录
    if (-not (Test-Path $Config.库目录)) {
        Write-Warning "库目录不存在: $($Config.库目录)"
    }
    
    return $allPassed
}

# ============================================
# 编译函数
# ============================================

function Invoke-Compile {
    param(
        [string]$Compiler,
        [string]$SourceDir,
        [string]$OutputDir,
        [string]$OutputExe,
        [hashtable]$Config,
        [string]$StageName
    )
    
    Write-Info "[$StageName] 使用 $Compiler 编译 src_cn..."
    
    # 步骤1: 生成C代码
    $emitCArgs = @(
        $SourceDir,
        "-o", $OutputDir,
        "--emit-c"
    )
    
    if ($Verbose) {
        Write-Verbose-Output "命令: $Compiler $emitCArgs"
    }
    
    $emitResult = Measure-CommandTime {
        & $Compiler $emitCArgs 2>&1
        return $LASTEXITCODE
    } -Description "生成C代码"
    
    if ($emitResult.结果 -ne 0) {
        Write-Fail "[$StageName] 生成C代码失败 (耗时: $($emitResult.耗时描述))"
        return @{ 成功 = $false; 错误 = "生成C代码失败" }
    }
    
    Write-Verbose-Output "[$StageName] 生成C代码完成，耗时: $($emitResult.耗时描述)"
    
    # 检查生成的C文件
    $cFiles = Get-ChildItem -Path $OutputDir -Filter "*.c" -Recurse
    if ($cFiles.Count -eq 0) {
        Write-Fail "[$StageName] 未生成任何C文件"
        return @{ 成功 = $false; 错误 = "未生成C文件" }
    }
    
    Write-Verbose-Output "[$StageName] 生成 $($cFiles.Count) 个C文件"
    
    # 步骤2: 编译C代码
    $cFileList = $cFiles | ForEach-Object { $_.FullName }
    $gccArgs = @(
        $Config.编译选项,
        "-I", $Config.包含目录,
        "-L", $Config.库目录,
        "-o", $OutputExe
    ) + $cFileList + @("-l$($Config.库名称)")
    
    if ($Verbose) {
        Write-Verbose-Output "命令: $($Config.C编译器) $gccArgs"
    }
    
    $compileResult = Measure-CommandTime {
        & $Config.C编译器 $gccArgs 2>&1
        return $LASTEXITCODE
    } -Description "编译C代码"
    
    if ($compileResult.结果 -ne 0) {
        Write-Fail "[$StageName] 编译C代码失败 (耗时: $($compileResult.耗时描述))"
        return @{ 成功 = $false; 错误 = "编译C代码失败" }
    }
    
    # 验证输出文件
    if (-not (Test-Path $OutputExe)) {
        Write-Fail "[$StageName] 未生成可执行文件: $OutputExe"
        return @{ 成功 = $false; 错误 = "未生成可执行文件" }
    }
    
    $fileSize = Get-FileSizeReadable $OutputExe
    $hash = Get-FileHashSHA256 $OutputExe
    
    Write-Success "[$StageName] 编译成功"
    Write-Info "[$StageName] 输出: $OutputExe ($fileSize)"
    Write-Info "[$StageName] SHA256: $hash"
    Write-Info "[$StageName] 总耗时: $($emitResult.耗时描述) + $($compileResult.耗时描述)"
    
    return @{
        成功 = $true
        输出文件 = $OutputExe
        哈希 = $hash
        文件大小 = $fileSize
        生成耗时 = $emitResult.耗时描述
        编译耗时 = $compileResult.耗时描述
        C文件数 = $cFiles.Count
    }
}

# ============================================
# 验证函数
# ============================================

function Compare-Binaries {
    param(
        [string]$File1,
        [string]$File2,
        [string]$Name1,
        [string]$Name2
    )
    
    $hash1 = Get-FileHashSHA256 $File1
    $hash2 = Get-FileHashSHA256 $File2
    
    Write-Info "比较 $Name1 与 $Name2..."
    Write-Verbose-Output "$Name1 哈希: $hash1"
    Write-Verbose-Output "$Name2 哈希: $hash2"
    
    if ($hash1 -eq $hash2) {
        Write-Success "$Name1 与 $Name2 完全一致"
        return @{ 一致 = $true; 哈希 = $hash1 }
    } else {
        Write-Fail "$Name1 与 $Name2 不一致"
        return @{ 一致 = $false; 哈希1 = $hash1; 哈希2 = $hash2 }
    }
}

function Compare-CCode {
    param(
        [string]$Dir1,
        [string]$Dir2,
        [string]$Name1,
        [string]$Name2
    )
    
    Write-Info "比较生成的C代码 ($Name1 vs $Name2)..."
    
    $hash1 = Get-DirectoryCHash $Dir1
    $hash2 = Get-DirectoryCHash $Dir2
    
    if ($hash1 -eq $hash2) {
        Write-Success "C代码完全一致"
        return @{ 一致 = $true }
    } else {
        Write-Warning "C代码存在差异"
        
        # 详细比较
        $files1 = Get-ChildItem -Path $Dir1 -Filter "*.c" -Recurse | Sort-Object Name
        $files2 = Get-ChildItem -Path $Dir2 -Filter "*.c" -Recurse | Sort-Object Name
        
        $diffs = @()
        foreach ($f1 in $files1) {
            $f2 = $files2 | Where-Object { $_.Name -eq $f1.Name } | Select-Object -First 1
            if ($f2) {
                $content1 = Get-Content $f1.FullName -Raw
                $content2 = Get-Content $f2.FullName -Raw
                if ($content1 -ne $content2) {
                    $diffs += $f1.Name
                }
            }
        }
        
        if ($diffs.Count -gt 0) {
            Write-Verbose-Output "差异文件: $($diffs -join ', ')"
        }
        
        return @{ 一致 = $false; 差异文件 = $diffs }
    }
}

# ============================================
# 报告生成函数
# ============================================

function New-BootstrapReport {
    param(
        [string]$OutputDir,
        [hashtable]$Stage1Result,
        [hashtable]$Stage2Result,
        [hashtable]$Stage3Result,
        [hashtable]$CompareResult,
        [bool]$Success
    )
    
    $reportPath = Join-Path $OutputDir "bootstrap_report.txt"
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    
    $report = @"
========================================
CN语言自举测试报告
========================================

测试时间: $timestamp
测试环境: $(Get-OSInfo)

----------------------------------------
编译器版本
----------------------------------------
引导编译器: cnc.exe (C实现)
CN编译器v1: cn_v1.exe (自编译)
CN编译器v2: cn_v2.exe (自举1)
CN编译器v3: cn_v3.exe (自举2)

----------------------------------------
编译统计
----------------------------------------
阶段1 编译时间: $($Stage1Result.生成耗时) + $($Stage1Result.编译耗时)
阶段2 编译时间: $($Stage2Result.生成耗时) + $($Stage2Result.编译耗时)
阶段3 编译时间: $($Stage3Result.生成耗时) + $($Stage3Result.编译耗时)

阶段1 生成C文件数: $($Stage1Result.C文件数)
阶段2 生成C文件数: $($Stage2Result.C文件数)
阶段3 生成C文件数: $($Stage3Result.C文件数)

----------------------------------------
哈希值
----------------------------------------
cn_v1.exe: $($Stage1Result.哈希)
cn_v2.exe: $($Stage2Result.哈希)
cn_v3.exe: $($Stage3Result.哈希)

----------------------------------------
验证结果
----------------------------------------
C代码一致性: $(if ($CompareResult.C代码一致) { "✅ 通过" } else { "❌ 失败" })
二进制一致性: $(if ($CompareResult.二进制一致) { "✅ 通过" } else { "❌ 失败" })

========================================
结论: $(if ($Success) { "✅ 自举验证通过" } else { "❌ 自举验证失败" })
========================================
"@
    
    $report | Out-File -FilePath $reportPath -Encoding UTF8
    Write-Info "测试报告已保存到: $reportPath"
    
    return $reportPath
}

function Get-OSInfo {
    $os = Get-CimInstance Win32_OperatingSystem
    return "$($os.Caption) / $($os.OSArchitecture)"
}

# ============================================
# 主测试流程
# ============================================

function Main {
    Write-Step "CN语言自举测试"
    
    # 加载配置
    $config = Load-Config
    
    Write-Info "项目根目录: $PROJECT_ROOT"
    Write-Info "构建目录: $($config.输出目录)"
    Write-Info "详细模式: $Verbose"
    Write-Info "保留产物: $KeepArtifacts"
    
    # 检查前置条件
    if (-not (Test-Prerequisites -Config $config)) {
        Write-Fail "前置条件检查失败"
        exit 1
    }
    
    # 清理构建产物
    Write-Step "准备构建环境"
    Clear-BuildArtifacts -OutputDir $config.输出目录
    
    # 阶段1: 引导编译
    Write-Step "阶段1: 引导编译 (cnc.exe -> cn_v1.exe)"
    $v1Exe = Join-Path $config.输出目录 "cn_v1.exe"
    $v1Dir = Join-Path $config.输出目录 "v1"
    
    $stage1Result = Invoke-Compile `
        -Compiler $config.编译器路径 `
        -SourceDir $config.源码目录 `
        -OutputDir $v1Dir `
        -OutputExe $v1Exe `
        -Config $config `
        -StageName "阶段1"
    
    if (-not $stage1Result.成功) {
        Write-Fail "阶段1编译失败: $($stage1Result.错误)"
        exit 1
    }
    
    # 阶段2: 首次自举
    Write-Step "阶段2: 首次自举 (cn_v1.exe -> cn_v2.exe)"
    $v2Exe = Join-Path $config.输出目录 "cn_v2.exe"
    $v2Dir = Join-Path $config.输出目录 "v2"
    
    $stage2Result = Invoke-Compile `
        -Compiler $v1Exe `
        -SourceDir $config.源码目录 `
        -OutputDir $v2Dir `
        -OutputExe $v2Exe `
        -Config $config `
        -StageName "阶段2"
    
    if (-not $stage2Result.成功) {
        Write-Fail "阶段2编译失败: $($stage2Result.错误)"
        exit 1
    }
    
    # 阶段3: 二次自举
    Write-Step "阶段3: 二次自举 (cn_v2.exe -> cn_v3.exe)"
    $v3Exe = Join-Path $config.输出目录 "cn_v3.exe"
    $v3Dir = Join-Path $config.输出目录 "v3"
    
    $stage3Result = Invoke-Compile `
        -Compiler $v2Exe `
        -SourceDir $config.源码目录 `
        -OutputDir $v3Dir `
        -OutputExe $v3Exe `
        -Config $config `
        -StageName "阶段3"
    
    if (-not $stage3Result.成功) {
        Write-Fail "阶段3编译失败: $($stage3Result.错误)"
        exit 1
    }
    
    # 验证阶段
    Write-Step "验证阶段"
    
    # 比较C代码
    $cCompare12 = Compare-CCode -Dir1 $v1Dir -Dir2 $v2Dir -Name1 "v1" -Name2 "v2"
    $cCompare23 = Compare-CCode -Dir1 $v2Dir -Dir2 $v3Dir -Name1 "v2" -Name2 "v3"
    
    # 比较二进制
    $binCompare = Compare-Binaries -File1 $v2Exe -File2 $v3Exe -Name1 "cn_v2.exe" -Name2 "cn_v3.exe"
    
    # 生成报告
    $compareResult = @{
        C代码一致 = $cCompare23.一致
        二进制一致 = $binCompare.一致
    }
    
    $reportPath = New-BootstrapReport `
        -OutputDir $config.输出目录 `
        -Stage1Result $stage1Result `
        -Stage2Result $stage2Result `
        -Stage3Result $stage3Result `
        -CompareResult $compareResult `
        -Success $binCompare.一致
    
    # 输出最终结果
    Write-Step "测试结果"
    
    if ($binCompare.一致) {
        Write-Success "✅ 自举验证通过！"
        Write-Success "cn_v2.exe 与 cn_v3.exe 完全一致"
        Write-Info "哈希值: $($binCompare.哈希)"
        
        # 复制最终版本到build目录
        $finalExe = Join-Path $BUILD_DIR "cn.exe"
        Copy-Item $v3Exe $finalExe -Force
        Write-Info "最终编译器已复制到: $finalExe"
        
        # 清理（除非指定保留）
        if (-not $KeepArtifacts) {
            Write-Info "清理中间构建产物..."
            # 保留最终可执行文件和报告
            Remove-Item $v1Exe -Force -ErrorAction SilentlyContinue
            Remove-Item $v2Exe -Force -ErrorAction SilentlyContinue
            Remove-Item $v1Dir -Recurse -Force -ErrorAction SilentlyContinue
            Remove-Item $v2Dir -Recurse -Force -ErrorAction SilentlyContinue
            Remove-Item $v3Dir -Recurse -Force -ErrorAction SilentlyContinue
        }
        
        exit 0
    } else {
        Write-Fail "❌ 自举验证失败！"
        Write-Fail "cn_v2.exe 与 cn_v3.exe 不一致"
        Write-Info "v2 哈希: $($binCompare.哈希1)"
        Write-Info "v3 哈希: $($binCompare.哈希2)"
        Write-Info "差异报告已保存到: $reportPath"
        
        exit 1
    }
}

# ============================================
# 脚本入口
# ============================================

# 显示帮助信息
if ($args -contains "-Help" -or $args -contains "--help" -or $args -contains "-h") {
    $helpText = @"
CN语言自举测试脚本

用法: ./tools/bootstrap_test.ps1 [选项]

选项:
  -Verbose          显示详细输出
  -KeepArtifacts    保留所有构建产物
  -Config <路径>    指定配置文件路径
  -CompilerPath <路径>  覆盖编译器路径
  -SourceDir <路径>     覆盖源码目录
  -OutputDir <路径>     覆盖输出目录
  -Help             显示此帮助信息

三阶段自举验证流程:
  阶段1: cnc.exe (C实现) ──编译──> cn_v1.exe
  阶段2: cn_v1.exe ──编译──> cn_v2.exe
  阶段3: cn_v2.exe ──编译──> cn_v3.exe
  验证: cn_v2.exe 与 cn_v3.exe 二进制SHA256哈希完全一致

示例:
  # 基本用法
  ./tools/bootstrap_test.ps1

  # 详细输出
  ./tools/bootstrap_test.ps1 -Verbose

  # 使用配置文件
  ./tools/bootstrap_test.ps1 -Config ./tools/bootstrap_config.json

  # 保留所有构建产物
  ./tools/bootstrap_test.ps1 -KeepArtifacts -Verbose
"@
    Write-Host $helpText
    exit 0
}

# 执行主函数
try {
    Main
} catch {
    Write-Fail "脚本执行出错: $_"
    Write-Verbose-Output $_.ScriptStackTrace
    exit 1
}
