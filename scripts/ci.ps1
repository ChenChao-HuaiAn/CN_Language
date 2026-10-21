# CN 语言编译器 CI 门禁脚本（B-2 2026-08）
# 本地/远程 CI 统一入口：构建（零警告）+ 单测 + E2E
# 用法：powershell -ExecutionPolicy Bypass -File scripts/ci.ps1
#       powershell -ExecutionPolicy Bypass -File scripts/ci.ps1 -Coverage   # 追加第 3 层源码覆盖率门禁（linux gcov 口径）
param([switch]$Coverage = $false)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $root

# 449-a：全量门禁串行锁（AGENTS.md §7）——同机多 worktree 并行时至多一个全量门禁在飞
# （防 CPU/内存互抢：78/79 OOM 前科）。锁=主树 target/gate.lock（跨 worktree 共享），
# 阻塞等待至获得；finally 释放（Ctrl+C/失败路径均覆盖）。
# 917（任务 109）：acquire 传 --owner $PID（powershell 自身=存活锚）——本进程被杀=陈锁
# 秒级接管（旧 4h 心跳空等根除）；无锚旧形态回退 4h 心跳判据。
python scripts/gate_lock.py acquire --owner $PID
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 全量门禁锁等待超时（gate_lock status 查看）" -ForegroundColor Red; exit 1 }
try {

Write-Host "[CI] 0.2/4 门禁清单审计（检查网第 9 层·plans/026 §2.6·新门禁必须注册）..." -ForegroundColor Cyan
python scripts/check_registry_audit.py
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 门禁清单审计失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 0/4 ASCII 标识符门禁（跨机-a 防复发，GCC9 拒 UTF-8 标识符）..." -ForegroundColor Cyan
python scripts/check_ascii_idents.py
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] ASCII 标识符门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 0.5/4 关键字清单同步门禁（plans/024 §7.5 单一事实源）..." -ForegroundColor Cyan
python scripts/check_keywords_sync.py
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 关键字清单同步门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 0.7/4 规范覆盖映射门禁（支柱一·plans/026 §2.1·B3 口径可测格 100%——247-a 起 --strict）..." -ForegroundColor Cyan
python scripts/check_spec_coverage.py --strict
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 规范覆盖映射门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 0.8/4 E2E 用例编号唯一性门禁（554 立：同编号多目录=撞号——487/489/496 治理链补防增量机制；发号=board_cli 领用例号）..." -ForegroundColor Cyan
python scripts/check_case_numbers.py
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] E2E 用例编号唯一性门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 0.85/4 官网文档覆盖门禁（1013·check_doc_coverage --strict：新 E2E 用例须 doc_coverage 台账归类（特性行/兜底桶/豁免·理由必填）——未归类/缺口/豁免异常即红；基线零=棘轮只拦新增）..." -ForegroundColor Cyan
python scripts/check_doc_coverage.py --strict
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 官网文档覆盖门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 0.87/4 部署清单一致性门禁（896·deploy_*.sh 拷贝清单 ⊇ 服务运行依赖闭包+静态资源覆盖——406/824 漏部署类防复发）..." -ForegroundColor Cyan
python scripts/check_deploy_manifest.py
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 部署清单一致性门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 0.88/4 E2E 用例 v2 覆盖通道守卫（930·306 轮 662/1025「自述 dual 实为探针」验收缺口立法化：用例须配 v2 通道（双编译对照/v2闭环/锚定链）或豁免登记（案号+理由·只减不增棘轮）——新用例静默 host-only 即红）..." -ForegroundColor Cyan
python scripts/check_dual_coverage.py
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] E2E 用例 v2 覆盖通道守卫失败" -ForegroundColor Red; exit 1 }
Write-Host "[CI] 0.9/4 语法接受面↔语义实现面同步门禁（871·762/775 病例立法化：解析器新接受的节点形态/零消费 AST 字段必须登记语义实现锚+用例——syntax_semantic_registry.json+baseline 冻结基线）..." -ForegroundColor Cyan
python scripts/check_syntax_semantic_sync.py
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 语法接受面↔语义实现面同步门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 0.92/4 并行 codegen 标签前缀表守卫（1170·1145 并行平移依赖三后端 prefix 表：发射源新增 ptrCheckCounter_ 派生前缀未同步表=并行漏平移产物静默漂移；缺失/陈旧/未分类三面+接线机械核对）..." -ForegroundColor Cyan
python scripts/check_parallel_prefix.py
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 并行 codegen 标签前缀表守卫失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 1/4 构建（/W4 /WX 零警告门禁）..." -ForegroundColor Cyan
& powershell -ExecutionPolicy Bypass -File build.ps1 -Config Debug
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 构建失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 1.5/4 CLI 契约门禁（检查网第 2 层·plans/026 §2.7·249-a 建成：子命令×形态退出码矩阵·B4 锚定格）..." -ForegroundColor Cyan
python scripts/check_cli_contract.py --cn target/Debug/cn.exe
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] CLI 契约门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 2/4 单元测试..." -ForegroundColor Cyan
& target/Debug/cn_unit_tests.exe
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 单元测试失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 3/4 E2E 测试（--jobs 8：非 v2 用例并行·v2 用例串行，248-a 用户裁决提速）..." -ForegroundColor Cyan
python tests/e2e/run_e2e.py --cn target/Debug/cn.exe --jobs 8 --full-reason "ci.ps1 本机全量门禁（449-a·gate_lock 锁内）"
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] E2E 失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 3.5/4 arm64 位宽编码门禁（D31 方案C④·258-a：E2E 全量 --target linux-arm64 汇编扫描 movz/movk wN lsl#32/48 零命中+linux_x64 缺发射面）..." -ForegroundColor Cyan
python scripts/check_asm_width.py --cn target/Debug/cn.exe
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] arm64 位宽编码门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 3.55/4 交叉汇编守卫序列对照门禁（887·835 病根立法化：内嵌探针×三后端 -O0 汇编的守卫判定序列机械对照·秒级）..." -ForegroundColor Cyan
python scripts/check_cross_asm_guards.py --cn target/Debug/cn.exe
if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 交叉汇编守卫序列对照门禁失败" -ForegroundColor Red; exit 1 }

Write-Host "[CI] 3.6/4 源码覆盖率门禁（检查网第 3 层·plans/026 §2.8·仅 -Coverage 时跑；linux gcov 口径）..." -ForegroundColor Cyan
if ($Coverage) {
    if (Test-Path "target/cov/cn") {
        python scripts/check_source_coverage.py
        if ($LASTEXITCODE -ne 0) { Write-Host "[CI] 源码覆盖率门禁失败" -ForegroundColor Red; exit 1 }
    } else {
        Write-Host "[CI] 无 coverage 构建产物（target/cov/cn）——win/MSVC 无 gcov 口径=诚实边界，跳过（plans/026 §2.8）" -ForegroundColor Yellow
    }
} else {
    Write-Host "[CI] 跳过（-Coverage 开关未启用）" -ForegroundColor DarkGray
}

Write-Host "[CI] 4/4 全部门禁通过" -ForegroundColor Green

} finally {
    python scripts/gate_lock.py release
}
