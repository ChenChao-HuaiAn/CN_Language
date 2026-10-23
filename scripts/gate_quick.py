#!/usr/bin/env python3
# L1 快速门禁（920 治理·三级门禁第一级——提交前·目标 ≤3 分钟增量）：
#   ① 静态三检查（ascii 标识符/关键字同步/门禁注册审计·秒级）
#   ② 增量构建（win=Ninja+sccache〔wt.py 开发树〕；linux=Make 增量）——零警告口径不变
#   ③ 全量单测（1410 例·实测秒级——无需做受影响选择）
# 不含 E2E：L2=影响面子集（任务分支收工）、L3=云端全量（TX_02 每推送+每日兜底·AGENTS v4）。
# 用法：python scripts/gate_quick.py   （rc=0=可提交；任何一步红=修复后再跑）

# 兼容 python3.8（深度机/单位机系统 python）：注解里的 list[str] 等泛型下标与
# X | None 联合在 3.8 会 TypeError——future import 使注解字符串化（242 轮 13b0e034 后断线·287 修复）。
from __future__ import annotations

# 332（2026-10-09 用户质询「失联是怎么回事」）：L1 每轮必跑——挂路过即心跳
#   （同 task_board 308k：开工后写码期无 task_board 触点·L1 是唯一高频脚本·
#   失败静默降级不阻断门禁；492：起线程逻辑归纳进 intent.挂载路过续约）
def _路过续约():
    try:
        import importlib.util as _ilu
        from pathlib import Path as _Path
        _p = _Path(__file__).resolve().parent / "intent.py"
        if not _p.exists():
            return
        _spec = _ilu.spec_from_file_location("cn_board_intent", _p)
        _mod = _ilu.module_from_spec(_spec)
        _spec.loader.exec_module(_mod)
        _mod.挂载路过续约()
    except Exception:
        pass


_路过续约()

import os
import subprocess
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
是win = sys.platform.startswith("win")

# 555 树锁：同树并发门禁串行化（多会话共用一棵树时 E2E/构建互踩——target/v2p 缓存
#   双写·后起清场杀先起者 rc=143 实录——根治）。取放/心跳/陈锁接管全复用 gate_lock；
#   CN_NO_TREE_LOCK=1 逃生门；CN_TREE_LOCK_HELD=1（嵌套链外层已持锁）豁免防自死锁。
#   atexit 释放：Ctrl+C（KeyboardInterrupt 传播=解释器正常退出路径）也覆盖。
sys.path.insert(0, str(仓库根 / "scripts"))
import gate_lock as _树锁
import ci_proc as _进程树   # 860：子进程整树生命线（935：Job Object/进程组·超时整树回收）

_gq树锁 = None
if os.environ.get("CN_TREE_LOCK_HELD"):
    print("[gate_lock] 继承外层树锁（CN_TREE_LOCK_HELD=1）", flush=True)
elif os.environ.get("CN_NO_TREE_LOCK"):
    print("[gate_lock] 树锁豁免（CN_NO_TREE_LOCK）", flush=True)
else:
    # 860 核对：锁获取=gate_lock 获取(0)（595-a run 形态语义：排队无限等+
    #   917 陈锁秒级接管+578 轮心跳；等待 30s 一打印且有上限判据链），
    #   不构成「无超时挂死」面——超时回收只作用于步骤子进程（下述 跑），
    #   锁等待与超时路径互不阻塞（回收动作走 ci_proc 直接杀树·不经锁）。
    _gq树锁 = _树锁.获取(0, 形态="run", owner_pid=os.getpid(), 锁=_树锁.树锁目录())
    if _gq树锁 is None:
        raise SystemExit("[gate_lock] 树锁等待超时——同树另一 E2E/构建仍在飞"
                         "（gate_lock status --scope tree 查看）")
    _树锁.心跳线程(_gq树锁 / "info.json")
    import atexit
    atexit.register(lambda: _树锁.释放(_gq树锁))

# 860（2026-10-15 实录：gate_quick 单测挂死烧满一核 18.4h·父进程持树锁阻塞整树·
#   强杀恢复）：单测/构建等全部子进程步骤=显式超时，超时=整树回收（ci_proc 运行树=
#   935 生命线：win Job Object KILL_ON_JOB_CLOSE / posix 进程组 killpg），步骤记
#   rc=-99 诚实红 + 一行 [超时回收] 审计（ci_proc 落日志）。分级与覆盖：
#     CN_GATE_STATIC_TIMEOUT 静态检查（默认 300s·脚本秒级冗余大）
#     CN_GATE_BUILD_TIMEOUT  构建/配置（默认 600s·冷全量构建由 sccache 兜速度）
#     CN_GATE_TEST_TIMEOUT   单测（默认 300s·1410 例实测秒级——860 实录挂死即此步）
#     CN_GATE_L2_TIMEOUT     --l2 E2E 影响面子集（默认 1800s·重负载本就排除在外）
#   ≤0 = 显式不限时（逃生门·仅调试用；诚实红语义不受影响）。
def _超时(环境名: str, 默认: int):
    try:
        值 = int(os.environ.get(环境名, str(默认)))
    except ValueError:
        print(f"[警告] {环境名} 非整数——用默认 {默认}s", flush=True)
        return 默认
    return 值 if 值 > 0 else None


静态超时 = _超时("CN_GATE_STATIC_TIMEOUT", 300)
构建超时 = _超时("CN_GATE_BUILD_TIMEOUT", 600)
单测超时 = _超时("CN_GATE_TEST_TIMEOUT", 300)
L2超时 = _超时("CN_GATE_L2_TIMEOUT", 1800)


def 跑(命令: list[str], 超时秒=None, **kw) -> subprocess.CompletedProcess:
    """子进程统一入口（860）：经 ci_proc.运行树 整树运行——超时=整树回收+
    [超时回收] 审计行（935）；超时不再抛异常炸进程，记 rc=-99 返回（诚实红由
    步 打印）。stdout/stderr 合并流式进本进程 stdout（ci_proc 固定形态）。
    锁安全：本函数不动锁；回收路径（Job Object/ killpg）独立于 gate_lock。"""
    print("  $", " ".join(str(c) for c in 命令)[:100], flush=True)
    try:
        return _进程树.运行树([str(c) for c in 命令], sys.stdout, 超时秒, cwd=仓库根, **kw)
    except subprocess.TimeoutExpired:
        return subprocess.CompletedProcess(命令, -99)


def 步(名: str, 命令: list[str], 超时秒=None, **kw) -> bool:
    r = 跑(命令, 超时秒=超时秒, **kw)
    注 = "（超时回收·见上行 [超时回收] 审计；CN_GATE_*_TIMEOUT 可调）" if r.returncode == -99 else ""
    print(f"[{名}] {'✓' if r.returncode == 0 else '✗ rc=' + str(r.returncode) + 注}", flush=True)
    return r.returncode == 0


print("== L1 快速门禁（920·提交前）==", flush=True)

# ① 静态检查族（与 ci.ps1 第 0~0.8 步同源·不依赖构建产物；242 增 file_length 冻结线·
#    334 增分层依赖冻结线——AGENTS §3 单向依赖的执法点：宿主 include 方向+v2 导入方向双面·
#    554 增用例编号唯一性——487/489/496 治理链的防增量机制·882 增静默值登记制硬拦——648
#    立法配套：四面四红+冻结基线，无逃生门 flag，唯一合法通道=miss_registry.json 补条·
#    896 增部署清单一致性——deploy_*.sh 拷贝清单 ⊇ 服务运行依赖闭包（AST 全扫）+静态
#    资源覆盖，406/824 漏部署类防复发）
#    871 增语法接受面↔语义实现面同步——762/775 病例（parser 收下语法而语义静默丢弃）
#    立法化：新增节点类型/零消费 AST 字段必须登记语义实现锚+用例，锚/用例存在性核）
#    1170 增并行 codegen 标签前缀表守卫——1145 并行发射按三后端 prefix 表平移函数内
#    局部标签编号；新增 ptrCheckCounter_ 派生前缀未同步表=并行漏平移产物静默漂移。
#    机械核对「发射源 ⇔ 前缀表」三面（缺失/陈旧/未分类）+并行入口接线。
#    894 增验证机器自身考核——「考官被考」缺口（云端验证机器报绿是否可信）：坏结果
#    注入四态（绿/红/超时/空结果）× 三端判定面（聚合/落盘上报/解析放行/服务端存储/
#    跨端管线）+ 元断言（判别力自证·防自检空转）；a 面阴性对照轮设计=plans/028 §十六。
#    纯本机零池依赖秒级（临时 DB/目录隔离·零 HTTP）。
for 名 in ("check_ascii_idents", "check_keywords_sync", "check_registry_audit",
           "check_case_numbers", "check_miss_gate", "check_deploy_manifest",
           "check_syntax_semantic_sync", "check_verifier_selfcheck",
           "check_parallel_prefix"):
    if not 步(名, [sys.executable, f"scripts/{名}.py"], 超时秒=静态超时):
        raise SystemExit(1)
# 1299（2026-10-22 审查裁决）：ci_daemon 自测挂 L1 静态段——缓存配置解析（off/auto/
#    显式/非法值/引号/分号/路径存在）+ linux 构建链行为级（假步骤）+执行机归因字段
#    每次提交机械复核；纯本机零构建零池依赖（秒级·--selftest 惰性导入自测体）。
if not 步("ci_daemon 自测", [sys.executable, "scripts/ci_daemon.py", "--selftest"],
         超时秒=静态超时):
    raise SystemExit(1)
# 1217 返工（2026-10-23 增量复审 P2-4）：窥孔 sanitizer 机械锁的判别力自测挂 L1
#    静态段——纯合成 gtest 输出、零编译器、秒级；防自测自身退化（计数/源码差集/
#    关键名/SKIP 断言空转）。完整 sanitizer 编译跑仍留 #1486（不挂 L1，避免每次
#    提交多约 1 分钟）。自测用显式 if 失败（非 assert），PYTHONOPTIMIZE=1 下照样红。
if not 步("窥孔 sanitizer 锁自测",
          [sys.executable, "scripts/test_check_peephole_sanitizer.py"],
          超时秒=静态超时):
    raise SystemExit(1)
# 1013（2026-10-17 用户裁决）：官网文档覆盖对照 --strict 挂 L1——基线零（61/61 已覆盖·
#    未归类 0·1013 归类 943 轮新增 1068/1069 归零）即等价棘轮·只拦新增：新 E2E 用例
#    须在 doc_coverage 台账归类（特性行或兜底桶），未归类/缺口/豁免理由空即红；
#    豁免唯一合法通道=台账『归类豁免』（理由必填）。纯本地文件实测 <1s。
if not 步("check_doc_coverage", [sys.executable, "scripts/check_doc_coverage.py", "--strict"],
         超时秒=静态超时):
    raise SystemExit(1)
# 930（2026-10-18）：E2E 用例 v2 覆盖通道守卫挂 L1——306 轮 662/1025「自述 dual
#    实为探针（用例本体未配 双编译对照.txt）」验收缺口立法化：用例无 v2 通道
#    （双编译对照.txt/v2闭环.txt/v2锚定链.txt）必须在脚本内嵌豁免清单登记
#    （案号+理由·只减不增棘轮）；新用例静默 host-only 即红。同族=853 死配置族
#    （906 已闸）/855 DN 轻路径断言恒真（已修）。纯本地目录扫描 <1s。
if not 步("check_dual_coverage", [sys.executable, "scripts/check_dual_coverage.py"],
         超时秒=静态超时):
    raise SystemExit(1)
if not 步("file_length 冻结线", [sys.executable, "scripts/check_file_length.py", "--freeze"],
         超时秒=静态超时):
    raise SystemExit(1)
if not 步("layer_deps 冻结线", [sys.executable, "scripts/check_layer_deps.py"],
         超时秒=静态超时):
    raise SystemExit(1)

# ② 增量构建（win=Ninja+sccache〔ninja_build.cmd=vcvars+cache 一体〕；linux=Make 增量）
if 是win:
    # 1281：集成批树/CI 新树 target/ 不入库——wt.py 生成的 Ninja 构建脚本不存在时
    #   原逻辑直接 `cmd /c target\ninja_build.cmd` → L1 构建步必红
    #   （实际报错：'target\ninja_build.cmd' 不是内部或外部命令）。此处按 wt.py
    #   同款模板就地生成 setup+build（同参数 Ninja+sccache 全量构建，不弱化检查），
    #   再走统一增量构建步——与 linux 分支「首次配置 + 增量构建」同构。
    if not (仓库根 / "target/ninja_build.cmd").exists():
        try:
            import wt as _wt
            vcvars, ninja, sccache = _wt.找vcvars(), _wt.找ninja(), _wt.找sccache()
        except Exception as 异常:
            print(f"[构建] 新树生成 Ninja 脚本失败（import wt：{异常}）", flush=True)
            raise SystemExit(1)
        if vcvars is None or ninja is None:
            print("[构建] 新树缺少 vcvars64.bat / ninja.exe——无法生成 Ninja 构建脚本",
                  flush=True)
            raise SystemExit(1)
        (仓库根 / "target").mkdir(exist_ok=True)
        公共 = dict(树=str(仓库根),
                    缓存目录=os.environ.get(
                        "SCCACHE_DIR", str(Path.home() / "Documents" / "sccache-cache")),
                    缓存上限=os.environ.get("SCCACHE_CACHE_SIZE", "5G"))
        if sccache is not None:
            配置脚本 = _wt.设置脚本模板.format(
                vcvars=vcvars, ninja=ninja, sccache=sccache, **公共)
        else:
            # 无 sccache 机器：去掉 compiler launcher 的最小配置（慢但正确）
            配置脚本 = (
                "@echo off\n"
                f'call "{vcvars}" >nul 2>&1\n'
                "where cl >nul 2>&1 || (echo [FAIL] cl not found after vcvars & exit /b 1)\n"
                f"cd /d {仓库根}\n"
                "cmake -G Ninja -S . -B target\\build-ninja "
                f'-DCMAKE_MAKE_PROGRAM="{ninja}" -DCMAKE_BUILD_TYPE=Debug\n'
                "if errorlevel 1 (echo [FAIL] cmake configure & exit /b 1)\n"
                "echo [OK] ninja tree configured\n")
        (仓库根 / "target" / "ninja_setup.cmd").write_text(配置脚本, encoding="ascii")
        (仓库根 / "target" / "ninja_build.cmd").write_text(
            _wt.构建脚本模板.format(vcvars=vcvars, **公共), encoding="ascii")
        if not 步("首次配置", ["cmd", "/c", "target\\ninja_setup.cmd"], 超时秒=构建超时):
            raise SystemExit(1)
    if not 步("增量构建", ["cmd", "/c", "target\\ninja_build.cmd"], 超时秒=构建超时):
        raise SystemExit(1)
else:
    if not (仓库根 / "target/build-quick/CMakeCache.txt").exists():
        if not 步("首次配置", ["cmake", "-S", ".", "-B", "target/build-quick"],
                 超时秒=构建超时):
            raise SystemExit(1)
    if not 步("增量构建", ["cmake", "--build", "target/build-quick", "--parallel"],
             超时秒=构建超时):
        raise SystemExit(1)

# ②.5 产物新鲜度机械核验（872·652 集成反复卡 L1 实录：旧 obj 假新鲜→LNK2005
#   多重定义假红·同代码 clean 全量绿/增量红）：构建后核对「obj 旧于源/链接滞后/
#   HEAD 判据」，红=构建机制失灵（继续跑单测=测旧物）——诚实红拦下；游离历史
#   残留只黄提示不阻断。探针实证与复现见提交信息（target/probe872）。
构建目录 = "target/build-ninja" if 是win else "target/build-quick"
if not 步("产物新鲜度", [sys.executable, "scripts/check_build_freshness.py",
                        "--构建目录", 构建目录], 超时秒=静态超时):
    raise SystemExit(1)


def 写集推导() -> tuple[list[str] | None, str]:
    """HEAD 相对 gitcode/develop 的写集（core.quotepath=false·中文路径原样输出·
    979 口径）；返回 (写集, 失败原因)：失败/超时=(None, 原因)——调用方自定
    照跑或诚实红（992 归纳自 L2 段与官网示例验证段两处同构 git 调用）。"""
    try:
        基准 = subprocess.run(["git", "merge-base", "HEAD", "gitcode/develop"], cwd=仓库根,
                           capture_output=True, text=True, timeout=120)
        if 基准.returncode != 0:
            return None, "merge-base rc=%d（gitcode/develop 不可达？）" % 基准.returncode
        r = subprocess.run(["git", "-c", "core.quotepath=false", "diff", "--name-only",
                            (基准.stdout or "").strip()],
                           cwd=仓库根, capture_output=True, text=True, timeout=120)
        if r.returncode != 0:
            return None, "diff rc=%d" % r.returncode
        return (r.stdout or "").split(), ""
    except subprocess.TimeoutExpired:
        return None, "git 调用超时 120s"


# ③ 全量单测（产物落源根 target/〔CMakeLists CMAKE_RUNTIME_OUTPUT_DIRECTORY〕）
单测 = next((p for p in [
    仓库根 / "target/cn_unit_tests.exe", 仓库根 / "target/cn_unit_tests",
    仓库根 / "target/build-ninja/cn_unit_tests.exe",
    仓库根 / "target/build-quick/cn_unit_tests",
] if p.exists()), None)
if 单测 is None:
    print("✗ 未找到单测产物（target/cn_unit_tests）——构建是否成功？")
    raise SystemExit(1)
if not 步("单测", [单测], 超时秒=单测超时):
    raise SystemExit(1)

# ③.5 官网文档示例验证（992·条件步·本机实测 239 例 36s）：写集触 website/（文档
#   数据）·src/·stdlib/（编译器语义/产物）·plans/001（语义裁决）→ 示例可能漂移，
#   必须编跑全量比对；写集不含=跳过（本地日常零税——云端全量面由 ci_daemon 静态段
#   恒跑兜底）。写集 git 取不到=照跑（fail-safe·与 189 纯文档判定同哲学）。
_写集, _写集失败 = 写集推导()

# ③.45 两级缓存正确性自测（1271 + 1213·条件步·真实编译器全链）：写集触及缓存
#   实现面（driver_ast_cache.* / driver_program_cache.* / driver_cache_util.* /
#   ast_serialize* / 两个自测脚本本身）才跑——缓存键/身份/原子写/双路径一致性的
#   回归不会被日常无关改动拖慢；写集取不到=fail-safe 照跑（与官网示例验证同哲学·
#   云端全量兜底）。两步各自独立：
#   ① test_ast_cache_env_key.py（1271）：宏集合条目分离/损坏与版本显形/多进程并发
#     写（3 轮）/编译器身份/键编码；
#   ② test_cache_invalidation_matrix.py（1213）：AST+程序缓存双级六类失效键完备性
#     （每类断言必失效且只失效对应条目）+ CN_AST_CACHE_VERIFY/CN_PROGRAM_CACHE_VERIFY
#     双路径命中后完整重编译逐字节对照（含漂移注入真抓捕）。
_触及缓存面 = _写集 is None or any(
    p.startswith(("src/cn_compiler/driver/driver_ast_cache.",
                  "src/cn_compiler/driver/driver_program_cache.",
                  "src/cn_compiler/driver/driver_cache_util.",
                  "src/cn_compiler/model/ast_serialize",
                  "scripts/test_ast_cache_env_key.py",
                  "scripts/test_cache_invalidation_matrix.py"))
    for p in _写集)
if not _触及缓存面:
    print("[verify] 写集未触两级缓存实现面——跳过缓存正确性自测（云端全量兜底）", flush=True)
else:
    if not 步("AST 缓存环境键自测", [
            sys.executable, "scripts/test_ast_cache_env_key.py",
            "--cn", str(单测.parent / ("cn.exe" if 是win else "cn"))],
             超时秒=_超时("CN_GATE_STATIC_TIMEOUT", 600)):
        raise SystemExit(1)
    if not 步("两级缓存失效矩阵/双路径验证自测", [
            sys.executable, "scripts/test_cache_invalidation_matrix.py",
            "--cn", str(单测.parent / ("cn.exe" if 是win else "cn"))],
             超时秒=_超时("CN_GATE_STATIC_TIMEOUT", 600)):
        raise SystemExit(1)

if _写集 is None:
    print("[verify] 写集取不到（%s）——照跑官网示例验证（fail-safe）" % _写集失败, flush=True)
    _跑示例验证 = True
else:
    _跑示例验证 = any(p.startswith(("website/", "src/", "stdlib/", "plans/001"))
                   for p in _写集)
    if not _跑示例验证:
        print("[verify] 写集未触 website/src/stdlib/plans/001——跳过（云端全量兜底）", flush=True)

# ②.7 v2 程序级缓存回归（#1273·适配 1272 返工版）：缓存正确性需要「同一工作区
#   多次编译 + 改导入文件/-O/目标/-D/货舱特性 + 破坏条目 + 替换编译器自身 + 构造
#   读写失败」等跨运行状态，E2E 单例（一次编译+一次输出比对）无法无损表达；独立
#   脚本直驱 v2p 真实链路（CNV2PC02/.cnvpc/环境旗标/VERIFY/货舱键），实测 ~5s
#   （另加 v2p 构建 ~12s）。写集触及缓存实现面=跑，未触=跳过（云端全量兜底）；
#   写集取不到=fail-safe 照跑。触发面=缓存模块/主流程挂点/运行时缓存原语/其单测/
#   回归脚本自身——防「改了缓存实现却没跑缓存回归」。
_缓存触发面 = (
    "CN语言编译器v2/驱动/程序缓存.cn",
    "CN语言编译器v2/主.cn",
    "CN语言编译器v2/协议常量/命令行.cn",
    "CN语言编译器v2/词法/预处理.cn",
    "CN语言编译器v2/语法/解析基础.cn",
    "CN语言编译器v2/IR/IR布局.cn",
    "CN语言编译器v2/代码生成/汇编落盘.cn",
    "src/runtime/file_api.cpp",
    "src/runtime/input_api.cpp",
    "src/runtime/intern_api.cpp",
    "src/runtime/runtime.hpp",
    "src/runtime/system_api.cpp",
    "tests/unit/runtime/test_intern_api.cpp",
    "scripts/test_v2_program_cache.py",
    "scripts/v2_program_cache_testlib.py",
)
_跑缓存回归 = _写集 is None or any(
    p in _缓存触发面 or p.startswith("CN语言编译器v2/驱动/程序缓存") for p in _写集)
if _跑缓存回归:
    if not 步("v2 程序缓存回归", [sys.executable, "scripts/test_v2_program_cache.py",
                                  "--cn", str(单测.parent / ("cn.exe" if 是win else "cn"))],
             超时秒=_超时("CN_GATE_CACHE_TEST_TIMEOUT", 900)):
        raise SystemExit(1)
else:
    print("[verify] 写集未触程序级缓存面——跳过缓存回归（云端全量兜底）", flush=True)

# ②.8 已知长度快路径回归（#1310·2026-10-23 审查要求）：字符串字面量/顶层常量
#   下标的越界检查必须使用编译期长度（不调 __cn_str_len）——行为用例证明不了
#   「优化被回退」（结果仍对、只是变慢），本门禁按产物计数机械化：宿主 IR 与
#   v2 产物 asm 各跑「1 字面量+1 变量」探针（期望 1）与「2 变量」对照（期望 2），
#   另含 v2 源码链路断言。写集触及下标族/快路径实现或检查脚本时跑；写集取不到
#   =fail-safe 照跑。v2p 未就绪时脚本黄字跳过产物段（L2/云端 v2p 预热后覆盖）。
_快路径触发面 = (
    "src/cn_compiler/ir/ir.cpp",
    "src/cn_compiler/ir/ir_expr_index.cpp",
    "src/cn_compiler/ir/ir_generator.hpp",
    "CN语言编译器v2/IR/生成表达式下标.cn",
    "scripts/check_knownlen_fastpath.py",
)
_跑快路径回归 = _写集 is None or any(p in _快路径触发面 for p in _写集)
if _跑快路径回归:
    if not 步("已知长度快路径回归",
              [sys.executable, "scripts/check_knownlen_fastpath.py",
               "--cn", str(单测.parent / ("cn.exe" if 是win else "cn"))],
              超时秒=_超时("CN_GATE_KNOWNLEN_TIMEOUT", 600)):
        raise SystemExit(1)
else:
    print("[verify] 写集未触字符串下标快路径面——跳过已知长度回归（云端全量兜底）", flush=True)

if _跑示例验证:
    if not 步("官网示例验证", [sys.executable, "website/tools/gen_docs.py", "--verify"],
             超时秒=_超时("CN_GATE_VERIFY_TIMEOUT", 600),
             env=dict(os.environ, CN_EXE=str(单测.parent / ("cn.exe" if 是win else "cn")))):
        raise SystemExit(1)

# ③.6 交叉汇编守卫序列对照（887·835 病根立法化·2026-10-16 用户令立案）：内嵌
#   探针 × 三后端 --target compile -O0（只产汇编不调 as/ld·实测 ~0.2s）——
#   溢出/判零/移位域守卫判定序列机械对照（ERROR 块向前重建谓词+语义期望对表+
#   三平台互照），835「条件码写反」同族笔误必红。条件步与官网示例验证同源写集
#   判定（触代码生成相关面 src//v2//plans/001 时跑）；写集取不到=fail-safe 照跑。
_跑守卫对照 = _写集 is None or any(
    p.startswith(("src/", "CN语言编译器v2/", "plans/001",
                  "scripts/check_cross_asm_guards.py")) for p in _写集)
if not _跑守卫对照:
    print("[verify] 写集未触 src/v2/plans/001——跳过交叉汇编守卫对照（云端兜底）", flush=True)
else:
    if not 步("交叉汇编守卫对照", [sys.executable, "scripts/check_cross_asm_guards.py",
                                  "--cn", str(单测.parent / ("cn.exe" if 是win else "cn"))],
             超时秒=静态超时):
        raise SystemExit(1)

# ④ L2 影响面子集（--l2·924 首版保守映射：写集中新增/改动的 E2E 用例目录全跑 +
#    触 CN语言编译器v2/ 树加跑 v2 面；src 模块→用例关键词精映射=后续演进（025 §1.99 边界）。
#    映射不覆盖的改动=诚实兜底：提示走 L3 云端全量，不静默漏。）

# 592（2026-10-13 用户裁决乙案·与 566 双重防线）：重负载用例 78_v2 自举链构建/
#   79_v2 自举闭环（v2 全树自举编译·单例 8~20 分钟）默认只在云端 CI 跑——L2
#   推导层直接不选入，558/594 实测的「触 v2 树 --l2 卷入重型误报红」在本层根治：
#   ① 写集直接改重型用例目录 → 从用例集剔除并打印说明（本层完全防线）；
#   ② 触 v2 树的「自举」滤词不再下发——实证命中面仅 78/79+806_arm64 溢出负测
#      （arm64 专属）三例，本地无合法命中对象，自举锚验证责任由云端全量承担；
#     「_v2」滤词保留（实证命中 140 轻例+2 重型），其中的重型命中由 run_e2e 566
#      立法默认跳过（SKIP·非失败·账目守恒）兜底——两层各自独立成立·叠加即闭环。
#   名单单点权威=run_e2e.内存保护用例前缀（模块导入·601-a 起与用例共存亡·
#   禁双份定义漂移）。窗口期注记：run_e2e 未含 566 的旧树（历史预验分支）发
#   _v2 滤词仍会真跑重型——566 已在飞区·与本任务改不同文件·先后集成均自愈。
def l2推导子集(改动: list[str], 树根: Path) -> tuple[list[str], list[str]]:
    """由写集推导 --l2 的 run_e2e --filter 子集（592 重型排除在此单点收口）。

    返回 (滤们, 重型排除们)：滤们=写集改动的轻用例目录名（触 v2 树时加 _v2
    面滤词·「自举」已废）；重型排除们=本层拦下的重负载用例名，调用方负责
    打印说明。"""
    名单 = run_e2e.内存保护用例前缀
    # 979 修复②：split 索引 [1]=「e2e」段，用例目录名在 [2]——原 [1] 使任何含
    #   用例改动的写集 filter 恒为「e2e」（零匹配 rc=1·--l2 首战 924 起潜伏，
    #   979 首个触用例写集实锤）。
    # 354 修复③：tests/e2e/ 根级文件（README.md/run_e2e.py 等）的 [2] 段=文件名
    #   非用例目录名（354 轮写集实锤：--filter README.md 零匹配 rc=1）——
    #   is_dir 判据根治：只有真实用例目录才进滤集。
    用例集 = sorted({p.split("/")[2] for p in 改动
                     if p.startswith("tests/e2e/") and len(p.split("/")) > 2
                     and (树根 / "tests" / "e2e" / p.split("/")[2]).is_dir()})
    重型排除们 = [c for c in 用例集 if any(前缀 in c for 前缀 in 名单)]
    滤们 = [c for c in 用例集 if c not in 重型排除们]
    滤们 += ["_v2"] if any(p.startswith("CN语言编译器v2/") for p in 改动) else []
    return 滤们, 重型排除们


if "--l2" in sys.argv:
    # 860：git 只读调用加显式超时（120s）——仓库损坏/网络文件系统挂死同样不纵容；
    #   超时=诚实红退出（2）不裸崩 traceback。992：git 调用归纳进 写集推导()（与
    #   L1 官网示例验证段单一源）；979 的 quotepath=false 口径在该函数内保持。
    改动, 失败原因 = 写集推导()
    if 改动 is None:
        print(f"✗ L2 写集推导失败（{失败原因}）——诚实红退出")
        raise SystemExit(2)
    # 592：导入 run_e2e 取权威名单——模块级仅常量/函数定义，入口有
    #   __main__ 保护，导入零副作用（与 gate_quick→gate_lock 同方向：
    #   门禁脚本依赖被测基础设施的既有先例）。
    sys.path.insert(0, str(仓库根 / "tests" / "e2e"))
    import run_e2e
    滤们, 重型排除们 = l2推导子集(改动, 仓库根)
    if 重型排除们:
        print("[L2] 重负载用例推导层不选·留云端（592·2026-10-13 裁决乙案）: "
              + "、".join(重型排除们))
    if not 滤们:
        print("[L2] 写集无新增/改动用例且未触 v2 树——无需子集（L3 云端全量兜底）")
    else:
        if any(p.startswith("CN语言编译器v2/") for p in 改动):
            print("[L2] 触 v2 树：滤词 _v2 跑轻 v2 面；自举锚（78_v2/79_v2）留云端"
                  "（592——滤词若命中重型·run_e2e 566 默认跳过兜底）")
        目标 = "win-x64" if 是win else "linux-x86_64"
        for 滤 in 滤们:
            # 555：子进程注入 CN_TREE_LOCK_HELD=1——本进程已持树锁，run_e2e 见之
            #   即豁免跳过取锁（防 gate_quick→run_e2e 嵌套自死锁·495-a/595-a 防线）
            if not 步("L2:" + 滤, [sys.executable, "tests/e2e/run_e2e.py",
                                   "--target", 目标, "--cn", str(单测.parent / ("cn.exe" if 是win else "cn")),
                                   "--filter", 滤],
                     超时秒=L2超时,
                     env=dict(os.environ, CN_TREE_LOCK_HELD="1")):
                raise SystemExit(1)


print("== L1 全绿（可提交；L2 影响面收工跑·L3 云端全量自动）==", flush=True)
