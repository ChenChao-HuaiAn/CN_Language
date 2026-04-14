# bootstrap.ps1 - CN语言自举引导脚本 (Windows版本)
# 用法: ./tools/bootstrap.ps1 [-Config 配置文件] [-SkipVerify] [-Verbose]
#
# 四阶段自举引导流程:
# 阶段0: 构建引导编译器 (C实现的cnc.exe)
# 阶段1: cnc.exe ──编译──> cn_v1.exe
# 阶段2: cn_v1.exe ──编译──> cn_v2.exe
# 阶段3: cn_v2.exe ──编译──> cn_v3.exe
# 验证: cn_v2.exe 与 cn_v3.exe 二进制SHA256哈希完全一致

param(
    [string]$Config = "tools/bootstrap_config.json",
    [switch]$SkipVerify,
    [switch]$Verbose
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

# 日志目录
$LogDir = Join-Path $PROJECT_ROOT "logs"
$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$LogFile = Join-Path $LogDir "bootstrap_$Timestamp.log"

# ============================================
# 颜色定义
# ============================================

$RED = "Red"
$GREEN = "Green"
$YELLOW = "Yellow"
$CYAN = "Cyan"
$GRAY = "Gray"

# ============================================
# 日志函数
# ============================================

function Write-Log {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    
    $TimestampedMsg = "[$(Get-Date -Format 'HH:mm:ss')] $Message"
    Write-Host $TimestampedMsg -ForegroundColor $Color
    
    # 确保日志目录存在
    if (-not (Test-Path $LogDir)) {
        New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
    }
    
    Add-Content -Path $LogFile -Value $TimestampedMsg
}

function Write-Debug-Log {
    param([string]$Message)
    
    if ($Verbose) {
        Write-Log $Message $GRAY
    }
}

# ============================================
# 前置条件检查
# ============================================

function Test-Prerequisites {
    Write-Log "检查前置条件..." $CYAN
    
    # 检查C编译器
    $hasCompiler = $false
    if (Get-Command cl -ErrorAction SilentlyContinue) {
        $hasCompiler = $true
        Write-Debug-Log "找到MSVC编译器: cl.exe"
    } elseif (Get-Command gcc -ErrorAction SilentlyContinue) {
        $hasCompiler = $true
        Write-Debug-Log "找到GCC编译器: gcc.exe"
    } elseif (Get-Command clang -ErrorAction SilentlyContinue) {
        $hasCompiler = $true
        Write-Debug-Log "找到Clang编译器: clang.exe"
    }
    
    if (-not $hasCompiler) {
        Write-Log "错误: 未找到C编译器 (需要MSVC/GCC/Clang)" $RED
        return $false
    }
    
    # 检查CMake
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        Write-Log "错误: 未找到CMake" $RED
        return $false
    }
    Write-Debug-Log "找到CMake: $(cmake --version | Select-Object -First 1)"
    
    # 检查源码目录
    if (-not (Test-Path $SRC_CN_DIR)) {
        Write-Log "错误: 未找到src_cn目录" $RED
        return $false
    }
    
    # 检查主程序入口
    $mainFile = Join-Path $SRC_CN_DIR "主程序/cnc.cn"
    if (-not (Test-Path $mainFile)) {
        Write-Log "警告: 未找到主程序入口: $mainFile" $YELLOW
    }
    
    Write-Log "✓ 前置条件检查通过" $GREEN
    return $true
}

# ============================================
# 阶段0: 构建引导编译器
# ============================================

function Build-Stage0 {
    Write-Log "`n========== 阶段0: 构建引导编译器 ==========" $CYAN
    
    $Stage0Dir = Join-Path $BUILD_DIR "stage0"
    $Stage0BinDir = Join-Path $Stage0Dir "bin/Release"
    $CncPath = Join-Path $Stage0BinDir "cnc.exe"
    
    # 检查是否已存在
    if (Test-Path $CncPath) {
        Write-Log "引导编译器已存在: $CncPath" $GREEN
        Write-Debug-Log "跳过阶段0构建"
        return $CncPath
    }
    
    # 创建构建目录
    if (-not (Test-Path $Stage0Dir)) {
        New-Item -ItemType Directory -Path $Stage0Dir -Force | Out-Null
    }
    
    # 执行CMake配置
    Write-Log "配置CMake..." $YELLOW
    Push-Location $Stage0Dir
    try {
        cmake $PROJECT_ROOT -DBUILD_BOOTSTRAP=ON -DCMAKE_BUILD_TYPE=Release 2>&1 | ForEach-Object {
            Write-Debug-Log $_
        }
        
        # 执行构建
        Write-Log "构建引导编译器..." $YELLOW
        cmake --build . --config Release 2>&1 | ForEach-Object {
            Write-Debug-Log $_
        }
    }
    finally {
        Pop-Location
    }
    
    # 验证构建结果
    if (Test-Path $CncPath) {
        $fileInfo = Get-Item $CncPath
        Write-Log "✓ 引导编译器构建成功" $GREEN
        Write-Debug-Log "文件路径: $CncPath"
        Write-Debug-Log "文件大小: $($fileInfo.Length) 字节"
        return $CncPath
    } else {
        Write-Log "✗ 引导编译器构建失败" $RED
        Write-Log "请检查构建日志: $Stage0Dir/CMakeFiles/CMakeOutput.log" $YELLOW
        return $null
    }
}

# ============================================
# 阶段1: 首次自举编译
# ============================================

function Build-Stage1 {
    param([string]$CncPath)
    
    Write-Log "`n========== 阶段1: 首次自举编译 ==========" $CYAN
    
    $Stage1Dir = Join-Path $BUILD_DIR "stage1"
    $CnV1Path = Join-Path $Stage1Dir "cn_v1.exe"
    
    # 创建输出目录
    if (-not (Test-Path $Stage1Dir)) {
        New-Item -ItemType Directory -Path $Stage1Dir -Force | Out-Null
    }
    
    # 使用引导编译器编译src_cn
    Write-Log "使用引导编译器编译CN源码..." $YELLOW
    Write-Debug-Log "编译器: $CncPath"
    Write-Debug-Log "源码目录: $SRC_CN_DIR"
    Write-Debug-Log "输出: $CnV1Path"
    
    $mainFile = Join-Path $SRC_CN_DIR "主程序/cnc.cn"
    
    try {
        & $CncPath $mainFile -o $CnV1Path 2>&1 | ForEach-Object {
            Write-Debug-Log $_
        }
    }
    catch {
        Write-Log "编译过程出错: $_" $RED
        return $null
    }
    
    # 验证构建结果
    if (Test-Path $CnV1Path) {
        $fileInfo = Get-Item $CnV1Path
        Write-Log "✓ cn_v1.exe 构建成功" $GREEN
        Write-Debug-Log "文件大小: $($fileInfo.Length) 字节"
        return $CnV1Path
    } else {
        Write-Log "✗ cn_v1.exe 构建失败" $RED
        return $null
    }
}

# ============================================
# 阶段2: 二次自举编译
# ============================================

function Build-Stage2 {
    param([string]$CnV1Path)
    
    Write-Log "`n========== 阶段2: 二次自举编译 ==========" $CYAN
    
    $Stage2Dir = Join-Path $BUILD_DIR "stage2"
    $CnV2Path = Join-Path $Stage2Dir "cn_v2.exe"
    
    # 创建输出目录
    if (-not (Test-Path $Stage2Dir)) {
        New-Item -ItemType Directory -Path $Stage2Dir -Force | Out-Null
    }
    
    # 使用v1编译器编译src_cn
    Write-Log "使用cn_v1编译器编译CN源码..." $YELLOW
    Write-Debug-Log "编译器: $CnV1Path"
    Write-Debug-Log "输出: $CnV2Path"
    
    $mainFile = Join-Path $SRC_CN_DIR "主程序/cnc.cn"
    
    try {
        & $CnV1Path $mainFile -o $CnV2Path 2>&1 | ForEach-Object {
            Write-Debug-Log $_
        }
    }
    catch {
        Write-Log "编译过程出错: $_" $RED
        return $null
    }
    
    # 验证构建结果
    if (Test-Path $CnV2Path) {
        $fileInfo = Get-Item $CnV2Path
        Write-Log "✓ cn_v2.exe 构建成功" $GREEN
        Write-Debug-Log "文件大小: $($fileInfo.Length) 字节"
        return $CnV2Path
    } else {
        Write-Log "✗ cn_v2.exe 构建失败" $RED
        return $null
    }
}

# ============================================
# 阶段3: 三次自举编译
# ============================================

function Build-Stage3 {
    param([string]$CnV2Path)
    
    Write-Log "`n========== 阶段3: 三次自举编译 ==========" $CYAN
    
    $Stage3Dir = Join-Path $BUILD_DIR "stage3"
    $CnV3Path = Join-Path $Stage3Dir "cn_v3.exe"
    
    # 创建输出目录
    if (-not (Test-Path $Stage3Dir)) {
        New-Item -ItemType Directory -Path $Stage3Dir -Force | Out-Null
    }
    
    # 使用v2编译器编译src_cn
    Write-Log "使用cn_v2编译器编译CN源码..." $YELLOW
    Write-Debug-Log "编译器: $CnV2Path"
    Write-Debug-Log "输出: $CnV3Path"
    
    $mainFile = Join-Path $SRC_CN_DIR "主程序/cnc.cn"
    
    try {
        & $CnV2Path $mainFile -o $CnV3Path 2>&1 | ForEach-Object {
            Write-Debug-Log $_
        }
    }
    catch {
        Write-Log "编译过程出错: $_" $RED
        return $null
    }
    
    # 验证构建结果
    if (Test-Path $CnV3Path) {
        $fileInfo = Get-Item $CnV3Path
        Write-Log "✓ cn_v3.exe 构建成功" $GREEN
        Write-Debug-Log "文件大小: $($fileInfo.Length) 字节"
        return $CnV3Path
    } else {
        Write-Log "✗ cn_v3.exe 构建失败" $RED
        return $null
    }
}

# ============================================
# 自举验证
# ============================================

function Verify-Bootstrap {
    param(
        [string]$V2Path,
        [string]$V3Path
    )
    
    Write-Log "`n========== 自举验证 ==========" $CYAN
    
    # 计算SHA256哈希
    $V2Hash = (Get-FileHash -Path $V2Path -Algorithm SHA256).Hash
    $V3Hash = (Get-FileHash -Path $V3Path -Algorithm SHA256).Hash
    
    Write-Log "cn_v2.exe SHA256: $V2Hash"
    Write-Log "cn_v3.exe SHA256: $V3Hash"
    
    if ($V2Hash -eq $V3Hash) {
        Write-Log "`n✅ 自举验证通过！" $GREEN
        Write-Log "cn_v2.exe 和 cn_v3.exe 二进制完全一致"
        Write-Log "CN语言自举成功！"
        return $true
    } else {
        Write-Log "`n⚠️ 自举验证失败" $YELLOW
        Write-Log "cn_v2.exe 和 cn_v3.exe 二进制不一致"
        
        # 生成差异报告
        $DiffReport = Join-Path $LogDir "diff_report_$Timestamp.txt"
        @"
CN语言自举验证差异报告
生成时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')

cn_v2.exe 信息:
  路径: $V2Path
  大小: $((Get-Item $V2Path).Length) 字节
  SHA256: $V2Hash

cn_v3.exe 信息:
  路径: $V3Path
  大小: $((Get-Item $V3Path).Length) 字节
  SHA256: $V3Hash

结论: 二进制不一致，自举验证失败
"@ | Out-File -FilePath $DiffReport -Encoding UTF8
        
        Write-Log "差异报告已保存到: $DiffReport" $YELLOW
        return $false
    }
}

# ============================================
# 清理函数
# ============================================

function Clear-OldLogs {
    param([int]$KeepDays = 7)
    
    $cutoffDate = (Get-Date).AddDays(-$KeepDays)
    Get-ChildItem -Path $LogDir -Filter "bootstrap_*.log" | 
        Where-Object { $_.LastWriteTime -lt $cutoffDate } | 
        Remove-Item -Force
    
    Get-ChildItem -Path $LogDir -Filter "diff_report_*.txt" | 
        Where-Object { $_.LastWriteTime -lt $cutoffDate } | 
        Remove-Item -Force
}

# ============================================
# 主流程
# ============================================

# 确保日志目录存在
if (-not (Test-Path $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
}

Write-Log "========================================" $CYAN
Write-Log "CN语言自举引导流程" $CYAN
Write-Log "========================================" $CYAN
Write-Log "开始时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
Write-Debug-Log "配置文件: $Config"
Write-Debug-Log "跳过验证: $SkipVerify"

# 清理旧日志
Clear-OldLogs -KeepDays 7

# 检查前置条件
if (-not (Test-Prerequisites)) {
    Write-Log "`n✗ 前置条件检查失败，退出" $RED
    exit 1
}

# 阶段0: 构建引导编译器
$CncPath = Build-Stage0
if (-not $CncPath) {
    Write-Log "`n✗ 阶段0失败，退出" $RED
    exit 1
}

# 阶段1: 首次自举编译
$CnV1Path = Build-Stage1 -CncPath $CncPath
if (-not $CnV1Path) {
    Write-Log "`n✗ 阶段1失败，退出" $RED
    exit 1
}

# 阶段2: 二次自举编译
$CnV2Path = Build-Stage2 -CnV1Path $CnV1Path
if (-not $CnV2Path) {
    Write-Log "`n✗ 阶段2失败，退出" $RED
    exit 1
}

# 阶段3和验证
if (-not $SkipVerify) {
    # 阶段3: 三次自举编译
    $CnV3Path = Build-Stage3 -CnV2Path $CnV2Path
    if (-not $CnV3Path) {
        Write-Log "`n✗ 阶段3失败，退出" $RED
        exit 1
    }
    
    # 自举验证
    $Success = Verify-Bootstrap -V2Path $CnV2Path -V3Path $CnV3Path
    
    Write-Log "`n========================================" $CYAN
    Write-Log "结束时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
    Write-Log "日志文件: $LogFile"
    Write-Log "========================================" $CYAN
    
    if ($Success) {
        exit 0
    } else {
        exit 2
    }
} else {
    Write-Log "`n跳过验证阶段" $YELLOW
    
    Write-Log "`n========================================" $CYAN
    Write-Log "结束时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
    Write-Log "日志文件: $LogFile"
    Write-Log "========================================" $CYAN
    
    exit 0
}
