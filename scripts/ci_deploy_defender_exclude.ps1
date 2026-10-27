<#
.SYNOPSIS
    CI runner Windows Defender 排除部署脚本（win 池部署面·只入库不自动执行）。

.DESCRIPTION
    来源：plans/034-三平台验证轮耗时归因.md §5B-①（部署项）·任务 #1633；
    #1718（2026-10-27 用户批准方案甲+乙）重构：枚举式排除改为「父目录一条 +
    编译链进程排除 + 历史枚举项一键清理」——解决枚举式三缺口（新 worktree
    首建不吃排除/删树后死路径残留/主树 target 不在 wt* 模式内漏排除）。

    背景：win 每个普通 e2e 用例固定开销中位 27.4s（linux 0.8s / arm64 0.7s），
    归因指向「链接+运行+杀软/文件系统实时扫描」段（#1633 三段计时实测：开销在
    cn.exe 编译链段，产物执行仅 0.05s）。本脚本对下列对象加 Defender 排除，
    并用 Get-MpPreference 回读自验（每项显式报告 已生效/未生效，不静默）：

      1. 目录排除（方案甲·#1718）：排除父目录一条（默认=C:\Users\ChenChao\Documents\CN，
         即 $仓库根 的父目录）——覆盖主树 target、全部现有 worktree（wt*\target）、
         以及将来任何新建 worktree（全在该目录下）；新树零动作、删树零残留。
         依据：Defender 路径排除不支持通配符（官方规定），无法用 wt*\target 模式
         一条覆盖，故按「父目录」粒度收敛。安全面=该开发目录实时扫描关闭（CI
         开发机排除 workspace 的行业常规做法·用户 2026-10-27 批准）。

      2. 进程排除（方案乙·#1718）：cl.exe / link.exe / ml64.exe / ninja.exe /
         cmake.exe——与路径无关，任何目录下的构建 IO 均不扫（新树/旧树/任意位置
         自动受益·零维护）；另保留 vswhere 探测到的 ml64/link 具体路径排除
         （更精确，与进程排除叠加无害；探测失败显式警告非静默）。

      3. 历史清理（#1718·迁移收尾）：默认把旧的「逐 worktree 枚举」排除项
         （形如 <父目录>\wt*\...\target 与 <父目录>\CN_Language_C\target）
         一键 Remove-MpPreference——只匹配本父目录下该形状，绝不触碰其他排除
         （Program Files 工具路径/用户其他目录）；-KeepWorktreeEntries 可跳过。

    安全设计：
      * 支持 -WhatIf 预演（只回显将加/将删的变更，不触碰系统配置）；
      * 执行前完整回显将加/将删清单；
      * 非管理员权限直接报错退出（Defender 排除须管理员）；
      * 幂等：已存在的排除项跳过（回显「已存在·跳过」），不重复添加；
      * 清理幂等：无匹配历史项时回显「无需清理」。

.PARAMETER 仓库根
    仓库根目录（默认=本脚本所在 scripts\ 的父目录）。其父目录即目录排除目标。

.PARAMETER KeepWorktreeEntries
    跳过历史逐 worktree 枚举项清理（默认执行清理·仅调试/回溯时用）。

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\ci_deploy_defender_exclude.ps1 -WhatIf
    （预演：回显将加的父目录/进程排除与将清理的历史枚举项，不改系统）

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\ci_deploy_defender_exclude.ps1
    （真实部署：管理员 PowerShell 中执行；结尾 Get-MpPreference 回读自验逐项核对）

.NOTES
    验收口径（plans/034 §6-2）：部署后用 #1633 三段计时观测面复测——
    python tests/e2e/run_e2e.py --filter <用例> --json <路径> 的「用例三段耗时」
    编译段中位对比部署前（27.4s 基线），收益机械可验。
    变更史：1633 初版（逐 wt 枚举）→1671（$PSScriptRoot 时序修复）→1718（甲+乙
    +清理·用户批准「父目录一条覆盖现在与未来所有树·清单长期只有几行」）。
#>
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$仓库根 = "",
    [switch]$KeepWorktreeEntries
)

# $PSScriptRoot 在参数默认值表达式求值期尚未填充（PowerShell 已知时序）——
#   运行期兜底解析（随批修复 1633 实测 WhatIf 即触发·1633 收口后补）。
if (-not $仓库根) { $仓库根 = Split-Path -Parent $PSScriptRoot }

$ErrorActionPreference = "Stop"

# ---- 管理员权限检查（Defender 排除须管理员·非管理员显式报错退出，不降级） ----
$当前身份 = [Security.Principal.WindowsIdentity]::GetCurrent()
$主体 = New-Object Security.Principal.WindowsPrincipal($当前身份)
if (-not $主体.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "需要管理员权限（Defender 排除变更被系统要求管理员）——请以管理员 PowerShell 重跑。本次未做任何变更。"
    exit 1
}

Write-Host "=== Defender 排除部署（#1633 + #1718 甲+乙·用户已批准 2026-10-27）===" -ForegroundColor Cyan
Write-Host "仓库根: $仓库根"
if ($WhatIfPreference) {
    Write-Host "[预演模式 -WhatIf] 只回显将执行的变更，不修改系统配置" -ForegroundColor Yellow
}

$父目录 = Split-Path -Parent $仓库根

# ---- 历史枚举项判据（纯函数·可独立验证：#1718 清理只认本父目录形状·防误删） ----
function 判断为历史枚举项 {
    param([string]$条目, [string]$父)
    if ([string]::IsNullOrWhiteSpace($条目) -or [string]::IsNullOrWhiteSpace($父)) { return $false }
    $规范 = $条目.TrimEnd('\')
    # 形状1：<父>\wt<any>...\target（旧脚本逐 worktree 枚举）
    $前缀 = (Join-Path $父 "wt")
    if ($规范.StartsWith($前缀, [StringComparison]::OrdinalIgnoreCase) -and
        $规范.EndsWith("\target", [StringComparison]::OrdinalIgnoreCase)) { return $true }
    # 形状2：<父>\CN_Language_C\target（主树 target 若曾被单独加入）
    $主树 = (Join-Path $父 "CN_Language_C\target")
    if ($规范.Equals($主树, [StringComparison]::OrdinalIgnoreCase)) { return $true }
    return $false
}

# ---- 方案甲：目录排除 = 父目录一条（覆盖主树+全部现有/未来 worktree） ----
$目录排除项 = @($父目录)

# ---- 方案乙：ml64/link 路径探测（vswhere·与 run_e2e.py 找MSVC工具链 同源逻辑） ----
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
    # 显式警告非静默：vswhere/目录探测未命中——进程名排除仍覆盖（任意 VS 版本路径）
    Write-Warning "vswhere 未探测到 ml64.exe/link.exe 具体路径——仅按进程名排除（ml64.exe/link.exe·覆盖任意安装路径）"
}

# ---- 方案乙：编译链进程排除（与路径无关·新树旧树自动覆盖） ----
$进程排除项 = @("cl.exe", "link.exe", "ml64.exe", "ninja.exe", "cmake.exe")

# ---- 执行前完整回显 ----
Write-Host "`n将添加的排除项：" -ForegroundColor Cyan
Write-Host "  目录排除（ExclusionPath·方案甲·父目录一条覆盖全部现有与未来树）:"
$目录排除项 | ForEach-Object { Write-Host "    $_" }
Write-Host "  进程排除（ExclusionProcess·方案乙·编译链·路径无关）:"
Write-Host ("    " + ($进程排除项 -join " "))
if ($工具路径排除项) {
    Write-Host "  路径排除（ExclusionPath·工具链具体路径·与进程排除叠加）:"
    $工具路径排除项 | ForEach-Object { Write-Host "    $_" }
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
    if ($种类 -eq "路径") {
        if ($PSCmdlet.ShouldProcess($值, "Add-MpPreference -ExclusionPath")) { Add-MpPreference -ExclusionPath $值 }
    }
    else {
        if ($PSCmdlet.ShouldProcess($值, "Add-MpPreference -ExclusionProcess")) { Add-MpPreference -ExclusionProcess $值 }
    }
    Write-Host "  [已添加] $种类 $值" -ForegroundColor Green
}

Write-Host "`n执行排除添加：" -ForegroundColor Cyan
foreach ($目录 in $目录排除项) { 添加排除 -种类 "路径" -值 $目录 -已有 $已有路径 }
foreach ($进程名 in $进程排除项) { 添加排除 -种类 "进程" -值 $进程名 -已有 $已有进程 }
foreach ($工具 in $工具路径排除项) { 添加排除 -种类 "路径" -值 $工具 -已有 $已有路径 }

# ---- 历史枚举项清理（#1718 迁移收尾·只删本父目录 wt*\target 形状） ----
Write-Host "`n历史逐 worktree 枚举项清理：" -ForegroundColor Cyan
$待清理 = @($已有路径 | Where-Object { 判断为历史枚举项 -条目 $_ -父 $父目录 })
if ($KeepWorktreeEntries) {
    Write-Host "  [跳过] -KeepWorktreeEntries 已指定——保留现有 $($待清理.Count) 条枚举项"
}
elseif ($待清理.Count -eq 0) {
    Write-Host "  [无需清理] 未发现历史枚举项"
}
else {
    Write-Host "  将清理 $($待清理.Count) 条（只删本父目录 wt*\target 形状·父目录/工具路径/其他排除不触碰）："
    foreach ($项 in $待清理) {
        if ($WhatIfPreference) {
            Write-Host "  [WhatIf 将删除] 路径 $项"
        }
        elseif ($PSCmdlet.ShouldProcess($项, "Remove-MpPreference -ExclusionPath")) {
            Remove-MpPreference -ExclusionPath $项
            Write-Host "  [已删除] 路径 $项" -ForegroundColor Green
        }
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
foreach ($进程名 in $进程排除项) {
    if ($回读进程 -contains $进程名) { Write-Host "  [已生效] 进程 $进程名" -ForegroundColor Green }
    else { Write-Host "  [未生效] 进程 $进程名" -ForegroundColor Red; $未生效 += 1 }
}
foreach ($工具 in $工具路径排除项) {
    if ($回读路径 -contains $工具) { Write-Host "  [已生效] 工具路径 $工具" -ForegroundColor Green }
    else { Write-Host "  [未生效] 工具路径 $工具" -ForegroundColor Red; $未生效 += 1 }
}
# 清理结果核对：历史枚举项应为 0（-KeepWorktreeEntries 或 WhatIf 时跳过该断言）
$残留枚举 = @($回读路径 | Where-Object { 判断为历史枚举项 -条目 $_ -父 $父目录 })
if (-not $KeepWorktreeEntries -and -not $WhatIfPreference) {
    if ($残留枚举.Count -eq 0) { Write-Host "  [已生效] 历史枚举项已清零" -ForegroundColor Green }
    else { Write-Host "  [未生效] 历史枚举项残留 $($残留枚举.Count) 条" -ForegroundColor Red; $未生效 += 1 }
}
elseif ($残留枚举.Count -gt 0) {
    Write-Host "  [提示] 历史枚举项现存 $($残留枚举.Count) 条（本次未清理）" -ForegroundColor Yellow
}

Write-Host "`n当前全部排除概览："
Write-Host ("  ExclusionPath($($回读路径.Count) 条)  : " + ($回读路径 -join " | "))
Write-Host ("  ExclusionProcess($($回读进程.Count) 条): " + ($回读进程 -join " | "))

if ($WhatIfPreference) {
    Write-Host "`n[预演结束] 未修改任何系统配置——去掉 -WhatIf 重跑即真实部署。" -ForegroundColor Yellow
    exit 0
}
if ($未生效 -gt 0) {
    Write-Error "有 $未生效 项排除未生效（可能被组策略/防篡改拦截）——请人工核查 Defender 管理界面。"
    exit 1
}
Write-Host "`n全部排除项已生效。验收：部署后用 #1633 三段计时复测编译段中位（基线 27.4s·plans/034 §3.2）。" -ForegroundColor Cyan
