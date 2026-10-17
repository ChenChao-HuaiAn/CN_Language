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

_gq树锁 = None
if os.environ.get("CN_TREE_LOCK_HELD"):
    print("[gate_lock] 继承外层树锁（CN_TREE_LOCK_HELD=1）", flush=True)
elif os.environ.get("CN_NO_TREE_LOCK"):
    print("[gate_lock] 树锁豁免（CN_NO_TREE_LOCK）", flush=True)
else:
    _gq树锁 = _树锁.获取(0, 形态="run", owner_pid=os.getpid(), 锁=_树锁.树锁目录())
    if _gq树锁 is None:
        raise SystemExit("[gate_lock] 树锁等待超时——同树另一 E2E/构建仍在飞"
                         "（gate_lock status --scope tree 查看）")
    _树锁.心跳线程(_gq树锁 / "info.json")
    import atexit
    atexit.register(lambda: _树锁.释放(_gq树锁))


def 跑(命令: list[str], **kw) -> subprocess.CompletedProcess:
    print("  $", " ".join(str(c) for c in 命令)[:100], flush=True)
    return subprocess.run([str(c) for c in 命令], cwd=仓库根, **kw)


def 步(名: str, 命令: list[str], **kw) -> bool:
    r = 跑(命令, **kw)
    print(f"[{名}] {'✓' if r.returncode == 0 else '✗ rc=' + str(r.returncode)}", flush=True)
    return r.returncode == 0


print("== L1 快速门禁（920·提交前）==", flush=True)

# ① 静态七检查（与 ci.ps1 第 0~0.8 步同源·不依赖构建产物；242 增 file_length 冻结线·
#    334 增分层依赖冻结线——AGENTS §3 单向依赖的执法点：宿主 include 方向+v2 导入方向双面·
#    554 增用例编号唯一性——487/489/496 治理链的防增量机制·882 增静默值登记制硬拦——648
#    立法配套：四面四红+冻结基线，无逃生门 flag，唯一合法通道=miss_registry.json 补条）
for 名 in ("check_ascii_idents", "check_keywords_sync", "check_registry_audit",
           "check_case_numbers", "check_miss_gate"):
    if not 步(名, [sys.executable, f"scripts/{名}.py"]):
        raise SystemExit(1)
if not 步("file_length 冻结线", [sys.executable, "scripts/check_file_length.py", "--freeze"]):
    raise SystemExit(1)
if not 步("layer_deps 冻结线", [sys.executable, "scripts/check_layer_deps.py"]):
    raise SystemExit(1)

# ② 增量构建（win=Ninja+sccache〔ninja_build.cmd=vcvars+cache 一体〕；linux=Make 增量）
if 是win:
    if not 步("增量构建", ["cmd", "/c", "target\\ninja_build.cmd"]):
        raise SystemExit(1)
else:
    if not (仓库根 / "target/build-quick/CMakeCache.txt").exists():
        if not 步("首次配置", ["cmake", "-S", ".", "-B", "target/build-quick"]):
            raise SystemExit(1)
    if not 步("增量构建", ["cmake", "--build", "target/build-quick", "--parallel"]):
        raise SystemExit(1)

# ③ 全量单测（产物落源根 target/〔CMakeLists CMAKE_RUNTIME_OUTPUT_DIRECTORY〕）
单测 = next((p for p in [
    仓库根 / "target/cn_unit_tests.exe", 仓库根 / "target/cn_unit_tests",
    仓库根 / "target/build-ninja/cn_unit_tests.exe",
    仓库根 / "target/build-quick/cn_unit_tests",
] if p.exists()), None)
if 单测 is None:
    print("✗ 未找到单测产物（target/cn_unit_tests）——构建是否成功？")
    raise SystemExit(1)
if not 步("单测", [单测]):
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
    import subprocess as _sp
    基准 = _sp.run(["git", "merge-base", "HEAD", "gitcode/develop"], cwd=仓库根,
                   capture_output=True, text=True).stdout.strip()
    # 979 修复（586 轮教训同族再犯）：core.quotepath 默认把中文用例目录名转义成
    #   "\346\227\..." 八进制串 → 子串 filter 永不匹配 → L2 子集空转 rc=1。
    #   显式关 quotepath 保证中文路径原样输出（filter 匹配靠它）。
    改动 = (_sp.run(["git", "-c", "core.quotepath=false", "diff", "--name-only", 基准],
                    cwd=仓库根, capture_output=True, text=True).stdout or "").split()
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
                      env=dict(os.environ, CN_TREE_LOCK_HELD="1")):
                raise SystemExit(1)


print("== L1 全绿（可提交；L2 影响面收工跑·L3 云端全量自动）==", flush=True)
