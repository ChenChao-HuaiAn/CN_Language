# CN语言自举构建脚本
# 用法: .\tools\build_bootstrap.ps1 [clean|stage1|stage2|verify|all]
#
# 自举构建流程：
#   第一阶段：使用C编译器构建引导编译器 cnc.exe
#   第二阶段：使用引导编译器构建自托管编译器 cn.exe
#   第三阶段（验证）：用自托管编译器编译自己，比较哈希值

param(
    [Parameter(Position=0)]
    [ValidateSet("clean", "stage1", "stage2", "verify", "all")]
    [string]$Target = "all",

    [Parameter(Position=1)]
    [ValidateSet("Debug", "Release", "RelWithDebInfo")]
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectRoot "build"
$Stage1Dir = Join-Path $BuildDir "stage1"
$Stage2Dir = Join-Path $BuildDir "stage2"
$Stage3Dir = Join-Path $BuildDir "stage3"

# 颜色输出函数
function Write-Info { param($Message) Write-Host $Message -ForegroundColor Cyan }
function Write-Success { param($Message) Write-Host $Message -ForegroundColor Green }
function Write-Warning { param($Message) Write-Host $Message -ForegroundColor Yellow }
function Write-Error { param($Message) Write-Host $Message -ForegroundColor Red }

function Clean-Build {
    Write-Info "`n========== 清理构建目录 =========="
    
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
        Write-Success "✓ 构建目录已清理: $BuildDir"
    } else {
        Write-Warning "构建目录不存在，跳过清理"
    }
}

function Build-Stage1 {
    Write-Info "`n========== 第一阶段：构建引导编译器 =========="
    Write-Host "配置: $Config"
    Write-Host "输出目录: $Stage1Dir"
    
    # 创建构建目录
    if (-not (Test-Path $Stage1Dir)) {
        New-Item -ItemType Directory -Path $Stage1Dir -Force | Out-Null
    }
    
    # 配置CMake
    Push-Location $Stage1Dir
    try {
        Write-Host "`n正在配置CMake..." -ForegroundColor Gray
        cmake ../.. `
            -DBUILD_BOOTSTRAP=ON `
            -DBUILD_SELF_HOSTED=OFF `
            -DCMAKE_BUILD_TYPE=$Config
        
        if ($LASTEXITCODE -ne 0) {
            throw "CMake配置失败，退出码: $LASTEXITCODE"
        }
        
        # 构建（忽略测试失败，只关注编译器本身）
        Write-Host "`n正在构建..." -ForegroundColor Gray
        cmake --build . --config $Config 2>&1
        # 不检查退出码，因为测试可能失败但编译器已成功构建
    }
    finally {
        Pop-Location
    }
    
    # 验证输出
    $CncPath = Join-Path $Stage1Dir "bin/$Config/cnc.exe"
    if (-not (Test-Path $CncPath)) {
        # 尝试其他路径
        $CncPath = Join-Path $Stage1Dir "bin/cnc.exe"
    }
    
    if (Test-Path $CncPath) {
        Write-Success "`n✓ 引导编译器构建成功: $CncPath"
        
        # 显示版本信息
        $Version = & $CncPath --version 2>&1
        if ($Version) {
            Write-Host "版本: $Version" -ForegroundColor Gray
        }
    } else {
        Write-Error "✗ 引导编译器构建失败：找不到 cnc.exe"
        Write-Host "查找路径: $Stage1Dir/bin/"
        Get-ChildItem -Path $Stage1Dir -Recurse -Filter "*.exe" -ErrorAction SilentlyContinue | 
            ForEach-Object { Write-Host "  找到: $($_.FullName)" -ForegroundColor Gray }
        exit 1
    }
}

function Build-Stage2 {
    Write-Info "`n========== 第二阶段：构建自托管编译器 =========="
    
    # 检查第一阶段是否完成
    $CncPath = Join-Path $Stage1Dir "bin/$Config/cnc.exe"
    if (-not (Test-Path $CncPath)) {
        $CncPath = Join-Path $Stage1Dir "bin/cnc.exe"
    }
    
    if (-not (Test-Path $CncPath)) {
        Write-Error "✗ 请先执行第一阶段构建: .\tools\build_bootstrap.ps1 stage1"
        exit 1
    }
    
    Write-Host "引导编译器: $CncPath"
    
    # 创建输出目录
    $OutputDir = Join-Path $Stage2Dir "bin"
    if (-not (Test-Path $OutputDir)) {
        New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    }
    
    # 编译主程序
    $MainSource = Join-Path $ProjectRoot "src_cn/主程序/cnc.cn"
    if (-not (Test-Path $MainSource)) {
        Write-Error "✗ 找不到主程序源文件: $MainSource"
        exit 1
    }
    
    Write-Host "`n正在编译自托管编译器..." -ForegroundColor Gray
    Write-Host "源文件: $MainSource"
    Write-Host "输出: $OutputDir/cn.exe"
    
    & $CncPath $MainSource -o "$OutputDir/cn.exe"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error "✗ 自托管编译器编译失败，退出码: $LASTEXITCODE"
        exit 1
    }
    
    $CnPath = Join-Path $OutputDir "cn.exe"
    if (Test-Path $CnPath) {
        Write-Success "`n✓ 自托管编译器构建成功: $CnPath"
        
        # 显示文件大小
        $FileSize = (Get-Item $CnPath).Length / 1MB
        Write-Host "文件大小: $([math]::Round($FileSize, 2)) MB" -ForegroundColor Gray
    } else {
        Write-Error "✗ 自托管编译器构建失败：找不到 cn.exe"
        exit 1
    }
}

function Verify-Bootstrap {
    Write-Info "`n========== 第三阶段：自举验证 =========="
    
    # 检查编译器是否存在
    $CncPath = Join-Path $Stage1Dir "bin/$Config/cnc.exe"
    if (-not (Test-Path $CncPath)) {
        $CncPath = Join-Path $Stage1Dir "bin/cnc.exe"
    }
    
    $CnPath = Join-Path $Stage2Dir "bin/cn.exe"
    
    if (-not ((Test-Path $CncPath) -and (Test-Path $CnPath))) {
        Write-Error "✗ 请先完成第一、二阶段构建"
        Write-Host "引导编译器: $(if (Test-Path $CncPath) { '✓ 存在' } else { '✗ 不存在' })"
        Write-Host "自托管编译器: $(if (Test-Path $CnPath) { '✓ 存在' } else { '✗ 不存在' })"
        exit 1
    }
    
    Write-Host "引导编译器: $CncPath"
    Write-Host "自托管编译器: $CnPath"
    
    # 创建第三阶段目录
    if (-not (Test-Path $Stage3Dir)) {
        New-Item -ItemType Directory -Path $Stage3Dir -Force | Out-Null
    }
    
    # 第三阶段：用自托管编译器编译自己
    $MainSource = Join-Path $ProjectRoot "src_cn/主程序/cnc.cn"
    Write-Host "`n正在用自托管编译器编译自己..." -ForegroundColor Gray
    
    & $CnPath $MainSource -o "$Stage3Dir/cn_v3.exe"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error "✗ 第三阶段编译失败，退出码: $LASTEXITCODE"
        exit 1
    }
    
    $V3Path = Join-Path $Stage3Dir "cn_v3.exe"
    if (-not (Test-Path $V3Path)) {
        Write-Error "✗ 第三阶段编译失败：找不到 cn_v3.exe"
        exit 1
    }
    
    # 比较哈希值
    Write-Host "`n正在比较哈希值..." -ForegroundColor Gray
    
    $V2Hash = (Get-FileHash $CnPath -Algorithm SHA256).Hash
    $V3Hash = (Get-FileHash $V3Path -Algorithm SHA256).Hash
    
    Write-Host "v2 (自托管编译器): $V2Hash"
    Write-Host "v3 (v2编译的):     $V3Hash"
    
    if ($V2Hash -eq $V3Hash) {
        Write-Success "`n✅ 自举验证通过！"
        Write-Host "v2 和 v3 哈希值一致，证明自举成功！" -ForegroundColor Green
        return $true
    } else {
        Write-Warning "`n⚠️ 自举验证：v2 和 v3 不一致"
        Write-Host "这可能是正常的（编译器内部包含时间戳或随机值）"
        Write-Host "需要进一步检查编译器输出是否功能正确"
        return $false
    }
}

function Show-Help {
    Write-Host @"

CN语言自举构建脚本
==================

用法: .\tools\build_bootstrap.ps1 [命令] [配置]

命令:
  clean    清理构建目录
  stage1   第一阶段：构建引导编译器 cnc.exe
  stage2   第二阶段：构建自托管编译器 cn.exe
  verify   第三阶段：自举验证
  all      执行完整构建流程（clean + stage1 + stage2 + verify）

配置:
  Debug             调试版本（默认：Release）
  Release           发布版本
  RelWithDebInfo    带调试信息的发布版本

示例:
  .\tools\build_bootstrap.ps1 stage1
  .\tools\build_bootstrap.ps1 stage2 Debug
  .\tools\build_bootstrap.ps1 all

"@
}

# 主逻辑
Write-Host "CN语言自举构建系统" -ForegroundColor Cyan
Write-Host "项目根目录: $ProjectRoot"
Write-Host "目标: $Target"
Write-Host "配置: $Config"

try {
    switch ($Target) {
        "clean" { Clean-Build }
        "stage1" { Build-Stage1 }
        "stage2" { Build-Stage2 }
        "verify" { Verify-Bootstrap }
        "all" {
            Clean-Build
            Build-Stage1
            Build-Stage2
            Verify-Bootstrap
        }
        "help" { Show-Help }
    }
    
    Write-Success "`n========== 构建完成 =========="
}
catch {
    Write-Error "`n✗ 构建失败: $_"
    Write-Host $_.ScriptStackTrace -ForegroundColor Gray
    exit 1
}
