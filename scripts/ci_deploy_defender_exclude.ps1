<#
.SYNOPSIS
    CI runner Windows Defender 排除部署脚本（三 win 实例部署面·只入库不自动执行）。

.DESCRIPTION
    来源：plans/034-三平台验证轮耗时归因.md §5B-①（部署项）·任务 #1633。
    用户已批准：2026-10-26（plans/034 §5B 三条建议中的部署脚本面——本脚本只入库，
    由主会话在部署轮统一执行，本脚本自身不做任何静默系统配置变更）。

    背景：win 每个普通 e2e 用例固定开销中位 27.4s（linux 0.8s / arm64 0.7s），
    归因指向「链接+运行+杀软/文件系统实时扫描」段。本脚本对下列对象加 Defender
    排除，并用 Get-MpPreference 回读自验（每项显式报告 已生效/未生效，不静默）：
      1. 目录排除：本仓库 target\（E2E 产物输出目录=run_e2e --target-dir 默认值；
         target\audit2\v2work*/selfwork*/supply* 等 e2e 工作目录全在其子树内），
         以及同父目录下各 CI worktree（wt*\target\，自动发现·逐项回显）；
      2. 进程/路径排除：ml64.exe / link.exe（MSVC 汇编器/链接器）——优先经 vswhere
         探测具体全路径加路径排除；探测失败时显式警告并退回按进程名排除（覆盖
         任意 VS 版本安装路径·非静默，回读自验同样核对）。

    安全设计：
      * 支持 -WhatIf 预演（只回显将执行的变更，不触碰系统配置）——首次部署先跑
        .\ci_deploy_defender_exclude.ps1 -WhatIf 核对清单；
      * 执行前完整回显将加的排除项清单；
      * 非管理员权限直接报错退出（Defender 排除须管理员）；
      * 幂等：已存在的排除项跳过（回显「已存在·跳过」），不重复添加。

.PARAMETER 仓库根
    仓库根目录（默认=本脚本所在 scripts\ 的父目录）。worktree 部署时传该 worktree 根。

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\ci_deploy_defender_exclude.ps1 -WhatIf
    （预演：只回显将加的排除项，不改系统）

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\ci_deploy_defender_exclude.ps1
    （真实部署：管理员 PowerShell 中执行；结尾 Get-MpPreference 回读自验逐项核对）

.NOTES
    验收口径（plans/034 §6-2）：部署后用 #1633 三段计时观测面复测——
    python tests/e2e/run_e2e.py --filter <用例> --json <路径> 的「用例三段耗时」
    编译段中位对比部署前（27.4s 基线），收益机械可验。
#>
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$仓库根 = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = "Stop"

# ---- 管理员权限检查（Defender 排除须管理员·非管理员显式报错退出，不降级） ----
$当前身份 = [Security.Principal.WindowsIdentity]::GetCurrent()
$主体 = New-Object Security.Principal.WindowsPrincipal($当前身份)
if (-not $主体.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "需要管理员权限（Defender 排除变更被系统要求管理员）——请以管理员 PowerShell 重跑。本次未做任何变更。"
    exit 1
}

Write-Host "=== Defender 排除部署（plans/034 §5B-①·#1633·用户已批准 2026-10-26）===" -ForegroundColor Cyan
Write-Host "仓库根: $仓库根"
if ($WhatIfPreference) {
    Write-Host "[预演模式 -WhatIf] 只回显将执行的变更，不修改系统配置" -ForegroundColor Yellow
}

# ---- ① 目录排除：本仓库 target\ + 兄弟 CI worktree 的 target\（自动发现·逐项回显） ----
$目录排除项 = @(Join-Path $仓库根 "target")
$父目录 = Split-Path -Parent $仓库根
$worktree们 = Get-ChildItem -Path $父目录 -Directory -Filter "wt*" -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -ne $仓库根 }
foreach ($wt in $worktree们) {
    $wt目标 = Join-Path $wt.FullName "target"
    if (Test-Path $wt目标) { $目录排除项 += $wt目标 }
}
if ($worktree们) {
    Write-Host ("发现兄弟 CI worktree: " + ($worktree们.Name -join "、"))
}

# ---- ② ml64/link 路径探测（vswhere·与 run_e2e.py 找MSVC工具链 同源逻辑） ----
$工具路径排除项 = @()
$vswhere = Join-Path (${env:ProgramFiles(x86)}) "Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $安装根 = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath 2>$null
    if ($安装根) {
        $工具根 = Get-ChildItem -Path (Join-Path $安装根 "VC\Tools\MSVC") -Directory -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending | Select-Object -First 1
        if ($工具根) {
            $ml64 = Join-Path $工具根.FullName "bin\Hostx64\x64\ml64.exe"
            $link = Join-Path $工具根.FullName "bin\Hostx64\x64\link.exe"
            if ((Test-Path $ml64) -and (Test-Path $link)) {
                $工具路径排除项 = @($ml64, $link)
            }
        }
    }
}
if (-not $工具路径排除项) {
    # 显式警告非静默：vswhere/目录探测未命中——退回按进程名排除（覆盖任意 VS 版本路径）
    Write-Warning "vswhere 未探测到 ml64.exe/link.exe 具体路径——退回按进程名排除（ml64.exe/link.exe·覆盖任意安装路径）"
}

# ---- 执行前完整回显将加的排除项 ----
Write-Host "`n将添加的排除项：" -ForegroundColor Cyan
Write-Host "  目录排除（ExclusionPath）:"
$目录排除项 | ForEach-Object { Write-Host "    $_" }
if ($工具路径排除项) {
    Write-Host "  路径排除（ExclusionPath·工具链）:"
    $工具路径排除项 | ForEach-Object { Write-Host "    $_" }
}
else {
    Write-Host "  进程排除（ExclusionProcess）: ml64.exe link.exe"
}

# ---- 幂等添加（读当前偏好，已存在=跳过并回显） ----
$当前偏好 = Get-MpPreference
$已有路径 = @($当前偏好.ExclusionPath)
$已有进程 = @($当前偏好.ExclusionProcess)

function 添加排除 {
    param([string]$种类, [string]$值, [object]$已有)
    if ($已有 -contains $值) {
        Write-Host "  [已存在·跳过] $种类 $值"
        return
    }
    if ($WhatIfPreference) {
        Write-Host "  [WhatIf 将添加] $种类 $值"
        return
    }
    if ($种类 -eq "路径") { Add-MpPreference -ExclusionPath $值 }
    else { Add-MpPreference -ExclusionProcess $值 }
    Write-Host "  [已添加] $种类 $值" -ForegroundColor Green
}

Write-Host "`n执行排除添加：" -ForegroundColor Cyan
foreach ($目录 in $目录排除项) { 添加排除 -种类 "路径" -值 $目录 -已有 $已有路径 }
foreach ($工具 in $工具路径排除项) { 添加排除 -种类 "路径" -值 $工具 -已有 $已有路径 }
if (-not $工具路径排除项) {
    foreach ($进程名 in @("ml64.exe", "link.exe")) {
        添加排除 -种类 "进程" -值 $进程名 -已有 $已有进程
    }
}

# ---- Get-MpPreference 回读自验（每项显式报告 已生效/未生效） ----
Write-Host "`n回读自验（Get-MpPreference）：" -ForegroundColor Cyan
$回读 = Get-MpPreference
$回读路径 = @($回读.ExclusionPath)
$回读进程 = @($回读.ExclusionProcess)
$未生效 = 0
foreach ($目录 in $目录排除项) {
    if ($回读路径 -contains $目录) { Write-Host "  [已生效] 目录 $目录" -ForegroundColor Green }
    else { Write-Host "  [未生效] 目录 $目录" -ForegroundColor Red; $未生效 += 1 }
}
foreach ($工具 in $工具路径排除项) {
    if ($回读路径 -contains $工具) { Write-Host "  [已生效] 工具路径 $工具" -ForegroundColor Green }
    else { Write-Host "  [未生效] 工具路径 $工具" -ForegroundColor Red; $未生效 += 1 }
}
if (-not $工具路径排除项) {
    foreach ($进程名 in @("ml64.exe", "link.exe")) {
        if ($回读进程 -contains $进程名) { Write-Host "  [已生效] 进程 $进程名" -ForegroundColor Green }
        else { Write-Host "  [未生效] 进程 $进程名" -ForegroundColor Red; $未生效 += 1 }
    }
}

Write-Host "`n当前全部排除概览："
Write-Host ("  ExclusionPath  : " + ($回读路径 -join " | "))
Write-Host ("  ExclusionProcess: " + ($回读进程 -join " | "))

if ($WhatIfPreference) {
    Write-Host "`n[预演结束] 未修改任何系统配置——去掉 -WhatIf 重跑即真实部署。" -ForegroundColor Yellow
    exit 0
}
if ($未生效 -gt 0) {
    Write-Error "有 $未生效 项排除未生效（可能被组策略/防篡改拦截）——请人工核查 Defender 管理界面。"
    exit 1
}
Write-Host "`n全部排除项已生效。验收：部署后用 #1633 三段计时复测编译段中位（基线 27.4s·plans/034 §3.2）。" -ForegroundColor Cyan
