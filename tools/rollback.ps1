# rollback.ps1 - CN语言自举回退脚本 (Windows版本)
# 用法: ./tools/rollback.ps1 [-Backup] [-Force]
#
# 当自举过程出现问题时，回退到C版本编译器继续开发

param(
    [switch]$Backup,    # 备份当前CN源码
    [switch]$Force      # 强制重新构建C版本
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
$BACKUP_DIR = Join-Path $PROJECT_ROOT "backup"
$LOG_DIR = Join-Path $PROJECT_ROOT "logs"

# ============================================
# 颜色定义
# ============================================

$RED = "Red"
$GREEN = "Green"
$YELLOW = "Yellow"
$CYAN = "Cyan"

# ============================================
# 日志函数
# ============================================

function Write-Log {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

# ============================================
# 备份CN源码
# ============================================

function Backup-CNSource {
    Write-Log "备份CN源码..." $CYAN
    
    if (-not (Test-Path $SRC_CN_DIR)) {
        Write-Log "未找到src_cn目录，跳过备份" $YELLOW
        return $null
    }
    
    # 创建备份目录
    if (-not (Test-Path $BACKUP_DIR)) {
        New-Item -ItemType Directory -Path $BACKUP_DIR -Force | Out-Null
    }
    
    # 生成备份目录名
    $Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $BackupPath = Join-Path $BACKUP_DIR "src_cn_$Timestamp"
    
    # 执行备份
    Copy-Item -Path $SRC_CN_DIR -Destination $BackupPath -Recurse
    
    $fileCount = (Get-ChildItem -Path $BackupPath -Recurse -File).Count
    Write-Log "✓ CN源码已备份到: $BackupPath" $GREEN
    Write-Log "  共 $fileCount 个文件" $GREEN
    
    return $BackupPath
}

# ============================================
# 恢复C版本编译器
# ============================================

function Restore-CVersion {
    Write-Log "`n恢复C版本编译器..." $CYAN
    
    $Stage0Dir = Join-Path $BUILD_DIR "stage0"
    $CncPath = Join-Path $Stage0Dir "bin/Release/cnc.exe"
    
    # 检查C版本是否存在且不需要强制重建
    if ((Test-Path $CncPath) -and (-not $Force)) {
        $fileInfo = Get-Item $CncPath
        Write-Log "✓ C版本编译器已存在: $CncPath" $GREEN
        Write-Log "  文件大小: $($fileInfo.Length) 字节" $GREEN
        Write-Log "  修改时间: $($fileInfo.LastWriteTime)" $GREEN
        return $true
    }
    
    # 需要重新构建
    if ($Force) {
        Write-Log "强制重新构建C版本编译器..." $YELLOW
    } else {
        Write-Log "C版本编译器不存在，开始构建..." $YELLOW
    }
    
    # 创建构建目录
    if (-not (Test-Path $Stage0Dir)) {
        New-Item -ItemType Directory -Path $Stage0Dir -Force | Out-Null
    }
    
    # 执行CMake配置
    Write-Log "配置CMake..." $YELLOW
    Push-Location $Stage0Dir
    try {
        cmake $PROJECT_ROOT -DBUILD_BOOTSTRAP=ON -DCMAKE_BUILD_TYPE=Release 2>&1 | Out-Null
        
        # 执行构建
        Write-Log "构建C版本编译器..." $YELLOW
        cmake --build . --config Release 2>&1 | Out-Null
    }
    finally {
        Pop-Location
    }
    
    # 验证构建结果
    if (Test-Path $CncPath) {
        $fileInfo = Get-Item $CncPath
        Write-Log "✓ C版本编译器构建成功" $GREEN
        Write-Log "  文件路径: $CncPath" $GREEN
        Write-Log "  文件大小: $($fileInfo.Length) 字节" $GREEN
        return $true
    } else {
        Write-Log "✗ C版本编译器构建失败" $RED
        Write-Log "请检查构建日志: $Stage0Dir/CMakeFiles/CMakeOutput.log" $YELLOW
        return $false
    }
}

# ============================================
# 清理自举产物
# ============================================

function Clear-BootstrapArtifacts {
    Write-Log "`n清理自举产物..." $CYAN
    
    $cleaned = 0
    
    # 清理stage1
    $stage1Dir = Join-Path $BUILD_DIR "stage1"
    if (Test-Path $stage1Dir) {
        Remove-Item -Path $stage1Dir -Recurse -Force
        Write-Log "  已删除: $stage1Dir" $GREEN
        $cleaned++
    }
    
    # 清理stage2
    $stage2Dir = Join-Path $BUILD_DIR "stage2"
    if (Test-Path $stage2Dir) {
        Remove-Item -Path $stage2Dir -Recurse -Force
        Write-Log "  已删除: $stage2Dir" $GREEN
        $cleaned++
    }
    
    # 清理stage3
    $stage3Dir = Join-Path $BUILD_DIR "stage3"
    if (Test-Path $stage3Dir) {
        Remove-Item -Path $stage3Dir -Recurse -Force
        Write-Log "  已删除: $stage3Dir" $GREEN
        $cleaned++
    }
    
    # 清理bootstrap_test
    $bootstrapTestDir = Join-Path $BUILD_DIR "bootstrap_test"
    if (Test-Path $bootstrapTestDir) {
        Remove-Item -Path $bootstrapTestDir -Recurse -Force
        Write-Log "  已删除: $bootstrapTestDir" $GREEN
        $cleaned++
    }
    
    if ($cleaned -eq 0) {
        Write-Log "  无需清理" $YELLOW
    } else {
        Write-Log "✓ 已清理 $cleaned 个目录" $GREEN
    }
}

# ============================================
# 更新回退日志
# ============================================

function Update-RollbackLog {
    param([string]$BackupPath)
    
    # 确保日志目录存在
    if (-not (Test-Path $LOG_DIR)) {
        New-Item -ItemType Directory -Path $LOG_DIR -Force | Out-Null
    }
    
    $RollbackLog = Join-Path $LOG_DIR "rollback.log"
    $Timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    
    $entry = "[$Timestamp] 回退执行"
    if ($BackupPath) {
        $entry += ", 备份目录: $BackupPath"
    }
    if ($Force) {
        $entry += ", 强制重建"
    }
    
    Add-Content -Path $RollbackLog -Value $entry
    Write-Log "回退日志已更新: $RollbackLog" $GREEN
}

# ============================================
# 显示使用说明
# ============================================

function Show-Usage {
    Write-Log "`n使用说明:" $CYAN
    Write-Log "  1. C版本编译器位于: build/stage0/bin/Release/cnc.exe"
    Write-Log "  2. 使用C版本编译器继续开发CN源码"
    Write-Log "  3. 修复问题后可重新运行自举流程"
    Write-Log ""
    Write-Log "示例命令:" $YELLOW
    Write-Log "  # 编译单个文件"
    Write-Log "  .\build\stage0\bin\Release\cnc.exe src_cn\主程序\cnc.cn -o output.exe"
    Write-Log ""
    Write-Log "  # 重新运行自举"
    Write-Log "  .\tools\bootstrap.ps1"
}

# ============================================
# 主流程
# ============================================

Write-Log "========================================" $CYAN
Write-Log "CN语言自举回退流程" $CYAN
Write-Log "========================================" $CYAN

# 备份CN源码（如果请求）
$BackupPath = $null
if ($Backup) {
    $BackupPath = Backup-CNSource
}

# 恢复C版本编译器
$Success = Restore-CVersion

if ($Success) {
    # 清理自举产物
    Clear-BootstrapArtifacts
    
    # 更新回退日志
    Update-RollbackLog $BackupPath
    
    # 显示使用说明
    Show-Usage
    
    Write-Log "`n✅ 回退成功！可使用C版本编译器继续开发" $GREEN
    exit 0
} else {
    Write-Log "`n✗ 回退失败" $RED
    Write-Log "请手动检查构建环境" $YELLOW
    exit 1
}
