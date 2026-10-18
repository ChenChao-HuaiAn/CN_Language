#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""合并队列脚本（AGENTS.md §5/§7——develop 唯一入关口）。

状态机全图（976 立·每转移的触发条件/动作/失败恢复+关键不变量+selftest 覆盖矩阵+
演进评估）= docs/integrate状态机-976.md——改动本文件前先对照该图。

职责：把当前任务分支安全集成回 develop——
  前置自检 → fetch → rebase（冲突自己解·禁止 develop 手解）→ 写集分类（仅门禁深度）
  → 门禁（跑在合并结果上：快速门禁恒跑；写集触及 src/tests 时全量门禁）
  → push gitcode develop（ff-only·CAS 竞争输家自动 rebase 重试 ≤3 次）→ 补推 github 镜像。
  写集语义（1008）=**分支自身改动**（三点间改动 <基线>...<HEAD>）——报名在 rebase 之前、
  分支基线常落后 develop tip，两点 diff 会把 develop 反向差异虚算入写集（#993：4→86 文件）。
v3（581-a·2026-09-21 用户裁决）：**无任何集成前置的他机回签/批准面**——跨平台正确性由
集成后异步验收+修复义务保障（AGENTS.md §7·异步验收不需他机回签）。
业界对照=rustc/bors 合并队列、GitHub merge queue 的脚本化实现。

批量集成（911 立·637/770 演进·**830 并行化改造 2026-10-15 用户裁决·最高优先级**）：
废除「等待批主」串行闸门（旧模型「有人集成中→其余全等」=池并行而入池通道串行的根源）。
新模型=**人人立即组链入池·预验并行**：
  ① 报名后立即组链（单人一条链·基于 fetch 后最新 develop）→ 快速门禁 → 推 ci/预验
     分支入池——**不等任何其他批、不看池忙闲**（池内排队由 runner 空闲认领天然实现·
     一个任务一个实例·禁拼车·禁等待）；
  ② 预验绿 → push develop（ff-only CAS）：develop 未变直接推；已变则重组链接到新头 +
     L1 自检（分钟级·不占池）→ **直接推·不重跑云端全量**（用户裁决：预验绿后不需要
     对齐头重验——组合正确性由 develop 平台轮转全量权威守护）；
  ③ 复用面三刀（防重复算力）：组链 cherry-pick 确定性化（GIT_COMMITTER_DATE=原提交
     时刻·同基同内容=同 sha→服务端幂等天然命中）+「预验绿复用」（历史绿记录+写集面
     逐字节等价→免重跑）+ 会话内「云端首批已绿」标志（CAS 重组轮免重跑）。
组合正确性权威=develop 头变化的三平台轮转全量验证（win→linux→arm64·770 立·每轮
一个平台一个实例）；兜底红走红治理（广播→引入者修→48h 可 revert）。cherry-pick
冲突=任务停「排队」+显形提示需人工解冲突后重跑（git 物理边界）。--gather/--seal-now/
--takeover 已随并行化退役（接受即忽略不报错）。他机看板写入全走 plumbing 直推
（临时 index+commit-tree·零 checkout 不扰动工作树·v5 起看板段已废档）。

用法（在任务分支上运行）：
  python3 scripts/integrate.py                    # 集成：报名→立即组链入池→池空闲即被认领跑全量
  python3 scripts/integrate.py --wait             # 同上并挂机等池预验结果·绿后自动 push（推荐）
  python3 scripts/integrate.py --join             # 仅报名（不组链·稍后重跑集成）
  python3 scripts/integrate.py --status           # 查看当前集成队列
  python3 scripts/integrate.py --solo             # 旧单分支路径（逃生门：队列机制异常时不经队列直集成）
  python3 scripts/integrate.py --drop 任务/895  # 门禁红归因踢出（兼容保留）
  python3 scripts/integrate.py --dry-run          # 演练：全步骤、push 以 --dry-run 代替（不真推）
  python3 scripts/integrate.py --selftest         # 正反例自测（纯函数面·CI 式）

退出码：0=集成成功（或已入池待重跑续推/演练/自测）；1=失败（原因见输出）；
2=保留（历史等待态·并行化后不再返回）。
"""
from __future__ import annotations

import argparse
import json
import os
import platform
import re
import subprocess
import sys
import threading
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import board_closeout   # 392：伞批销账号解析（登记制·提交信息 [销账:号,…] 行）

仓库根 = Path(__file__).resolve().parent.parent
主远程 = "gitcode"
镜像远程 = "github"
集成分支 = "develop"
分支名模式 = re.compile(r"^任务/(\d+[a-z]?|(?:家机|单位机|深度机)-\d+-\S+)$")  # 新格式=任务/<任务号>（196 立规）；旧段=过渡兼容存量在飞
最大重试 = 3

# ── 批量集成参数（911·830 并行化后仅保留单人路径常量）──────────────────────────
# 830（2026-10-15 用户裁决）：等待批主/拼车收拢/失联接管全部退役——原六常量随并行化
#   删除（一个任务一个实例·纯排队等池空闲·无批主概念）：组队窗口分钟
#   （CN_TEAMUP_WINDOW_MIN）/批收拢期分钟/批上限/批等待轮询秒/批等待上限分钟/
#   批主失联接管分钟。
心跳间隔秒 = int(os.environ.get("CN_QUEUE_HEARTBEAT_SEC", "120"))   # 562：队列行心跳周期
# （集成期间定期刷服务端行更新时刻=活会话证明——服务端超时回收以心跳新鲜度为据）

# 全量门禁触发面（触及下列任一路径 → 除快速门禁外另跑构建+单测+E2E；v3 起无 try-build 分类面——
# v2 的 try_build_触发模式 随 581-a 废除，写集分类仅区分门禁深度）
# 077（852 立·857 补）：v2 自举编译器源码/stdlib（行为面·Lang 侧）触发全量门禁；
# 876（用户裁决按甲办·0928 审计第 4 条收口）：构建编排三件补回——581-a 收窄时误失触发，
#   构建/门禁编排改动不跑全量=验证者免检自相矛盾，补回消除免检面（AGENTS §4 同步）
全量门禁触发模式 = ("src/", "tests/", "CN语言编译器v2/", "stdlib/",
                   "CMakeLists.txt", "build.ps1", "scripts/ci.ps1")

# ── 云端预验门禁（1008·用户裁决 2026-10-03 方案甲·test-then-commit）──────────────────
# 触及全量面的集成：链顶（=合并结果）推 TX_02 预验分支跑 linux-x86_64 全量，绿才 push
# develop——红根本进不来（红灯窗口期结构性消除）。旧模式（941 fast-lane·commit-then-test）
# 纯文档轮保留预验跳过面（走云端每推送自动验）；--no-cloud-gate 逃生门已硬禁（770·2026-10-14 用户令「只有预验绿才能并入 develop」）。
# **默认开**（1015 翻开·2026-10-04）：1008 立的启用条件「develop 回绿」已达成（1013 批八
# 终验云端 c4c28311 轮绿·linux 红=0）——触及全量面的集成默认走 test-then-commit。
# 红轮实测数据（TX_02 ci-logs 107 轮）：绿轮 13~16min·红轮带串行复验 33~40min——等待期
云端预验默认开 = True
# 770：--no-cloud-gate 逃生门已硬禁（parse 后即拒）——TX_02 异常时等待恢复+巡检治理。
预验远程分支前缀 = "ci/预验-"
预验轮询间隔秒 = 60
预验总超时分钟 = 240   # 含池排队等待（830 并行化后池常饱和·排队期须覆盖）+全量跑轮（原 75）
池空降级秒 = 300       # 排队 5 分钟无人认领**且池有空闲**=池空信号（1021·337 降级守卫同源判据；
                       #   830：池全忙时的排队是正常态——池忙不误判降级，见 池轮询预验）


def 运行(命令: list[str], **kwargs) -> subprocess.CompletedProcess:
    """执行子进程（打印命令行；默认继承 stdout/stderr）。"""
    print(f"  $ {' '.join(命令)}")
    return subprocess.run(命令, cwd=仓库根, **kwargs)


def 运行捕获(命令: list[str]) -> subprocess.CompletedProcess:
    """执行子进程并捕获输出（回放保持可见性）。

    723-a（机制级·安全网失效修复）：E2E 门禁的「已知红点名」消费 `结果.stdout`
    做解析，而 运行() 默认继承 stdout/stderr ⇒ CompletedProcess.stdout 恒为 None
    ⇒ 失败清单恒空 ⇒ `未点名`为空 ⇒ **未点名红也被放行**（点名机制形同虚设·
    722 集成实测：串行红 113/127_v2/79_v2 三项被解析为空清单后「披露放行」）。
    本函数捕获后回放（stdout 原样、stderr 归错误流），解析与可见性兼具。
    """
    结果 = subprocess.run(命令, cwd=仓库根, capture_output=True, text=True)
    if 结果.stdout:
        print(结果.stdout, end="")
    if 结果.stderr:
        print(结果.stderr, end="", file=sys.stderr)
    return 结果


def 输出(命令: list[str]) -> str:
    """执行子进程并返回 stdout（去尾换行）。"""
    结果 = subprocess.run(命令, cwd=仓库根, capture_output=True, text=True)
    return 结果.stdout.strip()


def 探测平台() -> str:
    """本机平台键（win / linux-arm64 / linux-x64）——决定全量门禁命令与 E2E --target。"""
    if platform.system() == "Windows":
        return "win"
    架构 = platform.machine().lower()
    return "linux-arm64" if 架构 in ("aarch64", "arm64") else "linux-x64"


def 失败(信息: str) -> int:
    print(f"\n[集成失败] {信息}")
    return 1


def 前置自检(远程: str = 主远程) -> str | None:
    """① 分支名合法 ② 工作树干净 ③ 分支已推远端。返回失败信息（None=通过）。"""
    当前分支 = 输出(["git", "branch", "--show-current"])
    if not 当前分支:
        return "不在任何分支上（detached HEAD？）——请在任务分支上运行本脚本。"
    # develop=主集成分支白名单（账目轮/立法轮直接在 develop 提交后跑本脚本=历史惯例·196 轮收紧时误拦）
    if 当前分支 != "develop" and not 分支名模式.match(当前分支):
        return f"分支名「{当前分支}」不合法——须形如 任务/<021任务号>（如 任务/087·196 立规）；旧格式 任务/<机>-<轮>-<标识> 仅存量在飞过渡（AGENTS.md §2/§7）。"
    状态 = 输出(["git", "status", "--porcelain"])
    未跟踪 = [行[3:] for 行 in 状态.splitlines() if 行.startswith("?? ")]
    已跟踪改动 = [行 for 行 in 状态.splitlines() if not 行.startswith("?? ")]
    if 已跟踪改动:
        return f"工作树不干净（提交或清理后再集成）：\n{已跟踪改动}"
    if 未跟踪:
        print(f"  [警告] 未跟踪文件不阻塞集成（不入提交）：{未跟踪}")
    远端分支 = f"{远程}/{当前分支}"
    if 输出(["git", "rev-parse", "--verify", f"refs/remotes/{远端分支}"]) == "":
        return f"分支未推远端（{远端分支} 不存在）——推分支=认领（AGENTS.md §7），先 git push。"
    本地 = 输出(["git", "rev-parse", "HEAD"])
    远端 = 输出(["git", "rev-parse", 远端分支])
    if 本地 != 远端:
        return f"本地提交未推远端（本地 {本地[:8]} ≠ 远端 {远端[:8]}）——先推分支再集成。"
    return None


def 三点间改动(起点: str, 终点: str) -> list[str]:
    """「终点侧自身改动」文件清单——三点语义 <起点>...<终点>（merge-base(起点,终点)→终点）。

    1008 修复（#993 实录）：原两点 diff <起点>..<终点> 在终点（分支）基线落后起点
    （develop tip）时，把「起点有而终点没有的反向差异」也算进写集——#993 实际 4 文件
    被算成 86 文件 → 误判触全量面白跑云端预验 2604 秒；归因辅助同频误导。报名发生在
    rebase 之前，落后基线是常见态而非边缘场景。三点=A...B：自 merge-base(A,B) 至 B
    =「本分支自身改动」；终点基于起点（rebase 后/两 develop tip 祖先链）时
    merge-base=起点，三点与两点等价——既有调用点语义不变。
    """
    # core.quotepath=false：中文路径默认被转义为带前导双引号的八进制形态（"tests/e2e/...），
    # 令 分类写集 的 startswith 匹配失效 → 触及中文路径写集被静默降级为快速门禁（586-a 实测：
    # 93 个 tests 文件全中文目录名，全量门禁被跳过）；冲突标记检查的 仓库根/路径 拼装同样受害。
    文本 = 输出(["git", "-c", "core.quotepath=false", "diff", "--name-only", f"{起点}...{终点}"])
    return [行.strip().strip('"') for 行 in 文本.splitlines() if 行.strip()]


def 改动文件清单(基准: str) -> list[str]:
    """基准（develop tip）相对 HEAD 的改动文件清单（=三点间改动 的便捷封装）。"""
    return 三点间改动(基准, "HEAD")


def 分类写集(文件们: list[str]) -> bool:
    """返回 须全量门禁与否。v3：写集分类仅区分门禁深度，不存在集成前置的他机批准面（AGENTS §7 异步验收）。"""
    return any(文件.startswith(全量门禁触发模式) for 文件 in 文件们)


def 临时用例检查(文件们: list[str], 树中存在查=None) -> str | None:
    """纪律 2（1008·用户裁决 2026-10-03）：临时/探针用例禁入 develop。

    tests/e2e/ 下用例目录名（路径第三段）含 tmp（大小写不敏感）即拦——1005 清 7 例存量
    （990/991/993/995/998/1002/1003tmp）后的增量防线；临时探针留在任务分支用后删，
    转正须按用例目录约定命名（<纯数字编号>_<名称>）。

    228 轮（2026-10-06）：净新增判据——批结果树（HEAD，快速门禁恒在组链 checkout
    后执行）中存在、且 develop tip 中不存在 的 tmp 路径才拦。两个死锁面实证：
    ①清污提交（删 90_tmp221*）被纯名字匹配误拦——毒文件既进不来也出不去；
    ②tmp 已被直推进 develop 时（221 r31 实录），任何批的组链树都继承它们，
    「HEAD 存在即拦」=全队列永久死锁。判据改查两棵树（git cat-file -e·不依赖
    工作区）：净新增拦（防线保持）·历史遗留放行（解死锁）·删除态放行（清污通）。

    562：树中存在查=可注入树查询（selftest 纯函数面用·缺省=git cat-file 真查）——
    修复存量红：990tmp 用例清理出 develop 后 selftest ④ 段按真树判定恒放行=断言失败
    （develop e110257c 实测红·selftest「无 git 副作用」设计被违背）。"""
    疑似 = sorted({f for f in 文件们
                   if f.startswith("tests/e2e/") and len(f.split("/")) > 2
                   and "tmp" in f.split("/")[2].lower()})
    def 树中存在(修订: str, 路径: str) -> bool:
        if 树中存在查 is not None:
            return bool(树中存在查(修订, 路径))
        return 运行捕获(["git", "cat-file", "-e", f"{修订}:{路径}"]).returncode == 0
    违规 = sorted({f.split("/")[2] for f in 疑似
                   if 树中存在("HEAD", f)
                   and not 树中存在(f"{主远程}/{集成分支}", f)})
    if 违规:
        return ("tests/e2e/ 临时用例 " + "、".join(违规)
                + " 禁止入 develop（用后删或按 <编号>_<名称> 转正命名后重报——1008 纪律 2）")
    return None


def 冲突标记检查(文件们: list[str]) -> str | None:
    """改动文件中的 git 冲突标记成对检查（233-a 事故铁律：禁止冲突标记入 develop）。"""
    for 文件 in 文件们:
        路径 = 仓库根 / 文件
        if not 路径.exists() or not 路径.is_file():
            continue
        try:
            内容 = 路径.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        if re.search(r"^<<<<<<< ", 内容, re.M) and re.search(r"^>>>>>>> ", 内容, re.M):
            return f"文件「{文件}」含 git 冲突标记——分支主人自己解冲突后重来（禁止带标记集成·233-a）。"
    return None


def 板查询(路径: str):
    """看板服务只读查询（382）——失败返回 None 不阻断集成。"""
    import urllib.request
    try:
        with urllib.request.urlopen(板服务URL.rstrip("/") + 路径, timeout=4) as 响应:
            return json.loads(响应.read().decode("utf-8"))
    except Exception:
        return None


def 总账收口(分支们: list, 新tip: str, remote: str = "", 额外号们: list | None = None) -> None:
    """382 看板 v2：集成成功后服务端任务台账自动销账——成员分支号逐个
    /api/task_update 置 ✅+sha10（✅⇔sha 铁律服务端化·台账唯一权威在服务端）。
    392 补盲区：伞批成员号（不入分支名）经提交信息 [销账:号,…] 登记行收集，
    经 额外号们 并入一并销账——登记制零猜测（提及≠销账·解析面见 board_closeout）。
    plans/021 文档已退位：不再剪切/归档/commit 021（git 面零接触）。
    失败仅警告（task_board --check 下轮对账兜底），随后播报本轮解锁（就绪前 3）。"""
    if not 分支们:
        return
    号们 = [m.group(1) for 分支 in 分支们
            for m in [re.match(r"^任务/([0-9]+[a-z]?)$", 分支)] if m]
    for 号 in (额外号们 or []):
        if 号 not in 号们:
            号们.append(号)
    if not 号们:
        return
    销们 = []
    for 号 in 号们:
        if 板调用("/api/task_update", {"号": 号, "状态": "✅", "收口sha": 新tip[:10]}):
            销们.append(号)
        else:
            print(f"  [警告] 服务端台账销账失败（#{号}）——恢复后 task_board --check 兜底/手动补销。")
    if 销们:
        print(f"  [台账] 服务端任务台账自动销账：{['#'+n for n in 销们]}（✅ {新tip[:10]}）")
        解锁 = 板查询("/api/tasks")
        就绪 = [t for t in (解锁 or {}).get("任务们", []) if t.get("就绪")][:3]
        if 就绪:
            print("  [台账] 本轮解锁（就绪队列前 3）：")
            for t in 就绪:
                print(f"      #{t['号']} {t['优先级']} {t['标题'][:56]}")


def 快速门禁(文件们: list[str]) -> str | None:
    """快速门禁（v5·1016）：冲突标记 + 临时用例拦截 + 021 任务总账账实检查。

    v4 的 check_handoff/check_progress_sync/check_language_philosophy 三挂点废除
    （文档结构检查=误报折腾税·详=AGENTS v5）——025/更新日志/看板废档后无检查面；
    check_language_philosophy 收窄为「语义变更轮手跑」（AGENTS §3.1 维持）。
    021 账实检查（task_board.py --check：🏃⇔分支存在防漏销账/✅⇔sha/依赖环/抢跑）
    = v5 唯一文档门禁——真防任务丢失，零形态误报。
    """
    问题 = 冲突标记检查(文件们)
    if 问题:
        return 问题
    问题 = 临时用例检查(文件们)
    if 问题:
        return 问题
    结果 = 运行([sys.executable, "scripts/task_board.py", "--check"])
    if 结果.returncode != 0:
        return "task_board 账实检查未过——修复后重试（021 任务总账）。"
    return None


def 解析E2E红用例行(文本: str) -> list[str]:
    """run_e2e 输出文本 → 失败用例名清单（汇总行「✗ 用例名: …」形态·447-a 复验点名消费）。

    976 归纳：win 分步复验与 linux 串行复验两处内联逐字同构（startswith(✗)→
    split(:)[0]）——抽一处；与 解析预验红用例们（云端结果 JSON 文本的正则面）
    消费不同协议形态，各自保留。
    868：判行首前先剥 ANSI SGR 色转义——「✗ 名: 因」若被颜色序列包裹（\x1b[31m✗…），
    原 startswith 不中=红清单凭空丢失（丢失面由调用点空清单守卫兜底：解析失败=硬红）。
    """
    色转义 = re.compile(r"\x1b\[[0-9;]*m")
    失败们: list[str] = []
    for 原行 in (文本 or "").splitlines():
        行 = 色转义.sub("", 原行).strip()
        if 行.startswith("✗"):
            失败们.append(行[1:].split(":")[0].strip())
    return 失败们


def 全量门禁(平台: str, 已知红们: list[str] | None = None, 快速通道: bool = False) -> str | None:
    """全量门禁：零警告构建 + 单测 + E2E 全量（平台相关；win=ci.ps1 一步到位）。

    快速通道（920·用户裁决 2026-10-01「快速集即集成·全量云端事后兜底」）：
    快速通道=True 时本机仅跑 L1（gate_quick.py=静态三检查+增量构建+全量单测·分钟级），
    全量门禁由 TX_02 云 CI 对 develop 每推送自动跑（红灯治理兜底：引入者优先修·
    超 48h 可 revert·不冻结其他集成——Chromium CQ/rustc bors 同构·AGENTS §5/§7）。

    并行红串行复验（447-a·机制级工具改进）：runner 隔离键欠账已随 329-D12/T55 根治
    （557 轮集成·隔离键=编号+完整名 md5——558 轮实测 210_v2/457/458/459 互踩假红全消）。
    本机制保留兜底新形态：ci.ps1/E2E 未过时分步复验——构建或单测红=真失败；E2E 串行
    复验（--jobs 1）绿=并行环境因素嫌疑（产物互踩/挂死类）→警告放行（要求看板通告
    披露）；串行仍红=真失败。
    """
    if 快速通道:
        print("  [快速通道·920 用户裁决] 本机 L1（gate_quick：静态三检查+增量构建+全量单测）"
              "——全量由 TX_02 云端对 develop 事后自动兜底（红灯治理适用）")
        r = 运行([sys.executable, "scripts/gate_quick.py"])
        return None if r.returncode == 0 else "L1 快速门禁未过（gate_quick.py——修复后重试）。"
    if 平台 == "win":
        # 449-a：gate_lock 串行锁在 ci.ps1 内部（acquire/finally-release）——此处勿再嵌套（死锁）。
        结果 = 运行(["powershell", "-ExecutionPolicy", "Bypass", "-File", "scripts/ci.ps1"])
        if 结果.returncode == 0:
            return None
        print("  [复验] ci.ps1 未过——分步复验区分真红与并行互踩（447-a 机制）")
        构建 = 运行(["cmake", "--build", "target/build", "--config", "Debug"])
        if 构建.returncode != 0:
            return "构建失败（零警告要求——见编译输出）。"
        单测 = 运行([str(仓库根 / "target/Debug/cn_unit_tests.exe")])
        if 单测.returncode != 0:
            return "单元测试未全过。"
        串行 = 运行捕获([sys.executable, "tests/e2e/run_e2e.py",
                     "--cn", str(仓库根 / "target/Debug/cn.exe"), "--jobs", "1",
                     "--full-reason", "integrate win 分步串行复验（447-a）"])
        if 串行.returncode != 0:
            # 591-a（T100·170 集成实测）：win 分支补接 --allow-known-red 点名消费
            #   （与锁内版 190-215 段同逻辑——原 win ci.ps1 路径漏接·参数已有实现
            #   未达此处）：串行红全部命中点名=已定性已知红披露放行；任一未点名仍拦。
            失败们 = 解析E2E红用例行(串行.stdout)
            if not 失败们:
                # 868 空清单守卫（722「空集合恒真」残留加固）：串行 rc 非零=E2E 有红，
                #   但 ✗ 行一个都没解析出（被杀/截断/入口态错误等）——空清单≠无红，
                #   解析失败=硬红（原逻辑空清单落「未点名为空」=披露放行——放红漏洞）。
                return ("E2E 串行复验 rc 非零但失败清单解析为空（✗ 行未取得——解析失败=硬红"
                        "·868）——禁止集成；本机重跑 E2E 复核失败清单后重试。")
            未点名 = [f for f in 失败们 if f not in (已知红们 or [])]
            if 未点名:
                return "E2E 串行复验仍未全绿（非并行互踩——真红，禁止集成）。"
            print(f"  [复验] 串行红 {失败们} 全部命中 --allow-known-red 点名清单——披露放行。")
        print("  [复验] 并行红+串行绿=并行环境因素嫌疑（隔离键已根治 557 轮·新形态须披露定位）"
              "——放行；须在后续提交信息以〔通告〕前缀披露。")
        return None
    # Linux：分步（单位机 linux-arm64 / 深度机 linux-x64；E2E 须显式 --target——默认 win-x64 会报错）
    # 449-a：构建/单测/E2E 三段经 gate_lock 串行锁（同机多 worktree 并行防互抢·AGENTS.md §7）。
    配置 = 运行(["cmake", "-S", ".", "-B", "target/build"])
    if 配置.returncode != 0:
        return "cmake 配置失败。"
    构建 = 运行([sys.executable, str(仓库根 / "scripts/gate_lock.py"), "run", "--",
               "cmake", "--build", "target/build", "--parallel"])
    if 构建.returncode != 0:
        return "构建失败（零警告要求——见编译输出）。"
    单测路径 = 仓库根 / "target/build/tests/unit/cn_unit_tests"
    if not 单测路径.exists():
        单测路径 = 仓库根 / "target/build/cn_unit_tests"
    if not 单测路径.exists():
        # CMAKE_RUNTIME_OUTPUT_DIRECTORY 指向 target/（CMakeLists 16 行）——产物实际落 target/ 根
        单测路径 = 仓库根 / "target/cn_unit_tests"
    单测 = 运行([sys.executable, str(仓库根 / "scripts/gate_lock.py"), "run", "--", str(单测路径)])
    if 单测.returncode != 0:
        return "单元测试未全过。"
    cn路径 = 仓库根 / "target/build/cn"
    if not cn路径.exists():
        cn路径 = 仓库根 / "target/cn"
    # 本机平台键（win/linux-x64）→ run_e2e.py 目标名（win-x64/linux-arm64/linux-x86_64）
    目标 = {"win": "win-x64", "linux-x64": "linux-x86_64", "linux-arm64": "linux-arm64"}[平台]
    # gate_lock 串行锁（449-a）× 并行红串行复验（447-a）合成：锁内 E2E 并行，红后锁内串行复验
    # 794 防线（ulimit -v 32GB 包裹）已由 798 摘除（机制级·随证据备案）：v2 全树编译
    #   地址空间「按需分配·随上限水涨船高」（746-a 实测 8335MB@8GB→16484MB@16GB），
    #   32GB 包裹下 AS 顶满→malloc 失败→未检查空指针→fix_p SIGSEGV（-11 假红·
    #   本轮 78/79_v2 集成态实锤；794 自身「16GB 掐死 -11 假红」同族第三例）。
    #   OOM 连坐防护由 797-a 机械化承接（run_e2e 每子进程 RLIMIT_AS 8GB+RSS 轮询顶+
    #   guardian oom_guard kill 10% 系统层）——包裹冗余且有害，摘除后全树编译 AS 不受限
    #   （与隔离单跑环境一致·隔离单跑 78/79_v2/197_v2 全绿实证）。
    # 917 退役「--jobs 2」：794 压并行的前提（v2p 单进程峰值 15GB×多 worker 叠加=32GB
    #   耗尽·9-26 两次全局 OOM）已被内存治理消灭——065→074 后 v2p 全树编译峰值 RSS
    #   20.75GB→227.8MB（885 轮实测·宿主 cn 307MB），8 并行叠加≈2GB 无压力；flock
    #   全树编译串行互斥同轮摘除（run_e2e 917）——78/79 三跳串行长尾（arm64 单跳
    #   ~8 分钟）并入并行。回退路径=git 历史回取（794/797-a 原文在案）。
    e2e = 运行捕获(["bash", "-c",
                 "exec " + sys.executable + " " +
                 str(仓库根 / "scripts/gate_lock.py") + " run -- " + sys.executable +
                 " tests/e2e/run_e2e.py --target " + 目标 + " --cn " + str(cn路径) +
                 " --jobs 8 --full-reason 'integrate linux 分步全量门禁（gate_lock 锁内）'"])
    if e2e.returncode == 0:
        return None
    print("  [复验] E2E 并行未全绿——串行复验区分真红与并行互踩（447-a 机制·gate_lock 锁内）")
    串行 = 运行捕获(["bash", "-c",
                  "exec " + sys.executable + " " +
                  str(仓库根 / "scripts/gate_lock.py") + " run -- " + sys.executable +
                  " tests/e2e/run_e2e.py --target " + 目标 + " --cn " + str(cn路径) +
                  " --jobs 1 --full-reason 'integrate linux 串行复验（447-a·gate_lock 锁内）'"])
    if 串行.returncode != 0:
        # 564-a（过渡机制·fdef8ae9 P1「v2p 构建确定性缺失」根治前）：--allow-known-red
        #   显式点名机制——串行红若【全部】命中点名清单=「三平台已定性已知红」披露放行；
        #   任一未点名红仍硬拦（机制不弱化）。使用责任=发起机：点名依据+集成广播披露
        #   不实=违规可 revert。fdef8ae9 P1 根治后本分支应移除（移交条款）。
        失败们 = 解析E2E红用例行(串行.stdout)
        if not 失败们:
            # 868 空清单守卫（win 面同款·722「空集合恒真」残留加固）：串行 rc 非零但
            #   ✗ 行一个都没解析出——空清单≠无红，解析失败=硬红（原逻辑「已知红们 非空
            #   且未点名空」在空清单时恒真=点名流放红漏洞；未点名时靠尾行兜底不暴露）。
            return (f"E2E 串行复验 rc 非零但失败清单解析为空（✗ 行未取得——解析失败=硬红"
                    f"·868·--target {目标}）——禁止集成；重跑 E2E 复核失败清单后重试。")
        未点名 = [f for f in 失败们 if f not in (已知红们 or [])]
        if 已知红们 and not 未点名:
            print(f"  [复验] 串行红 {失败们} 全部命中 --allow-known-red 点名清单"
                  f"（fdef8ae9 P1 批次抽签/三平台已定性已知红）——披露放行；"
                  f"须在后续提交信息以〔通告〕前缀完整披露点名依据。")
            return None
        if 未点名:
            return (f"E2E 串行复验存在未点名真红 {未点名}（--target {目标}"
                    f"·点名清单外——禁止集成）。")
        return f"E2E 串行复验仍未全绿（--target {目标}·非并行互踩——真红，禁止集成）。"
    print("  [复验] 并行红+串行绿=runner 产物互踩嫌疑（274-a 隔离键欠账·非代码红）"
          "——放行；须在后续提交信息以〔通告〕前缀披露。")
    return None


# ═══════════════════════════════════════════════════════════════════════════
# 云端预验门禁（1008·方案甲 test-then-commit）——链顶推 TX_02 预验分支跑全量，绿才进 develop
# ═══════════════════════════════════════════════════════════════════════════

def 读CI配置() -> dict:
    """TX_02 连接参数——scripts/ci_client.json（gitignore·形如 {"ssh":"TX_02","目录":"/home/ubuntu/cn-ci"}）
    可选覆盖，缺省内置默认（三机 ssh config 均配 TX_02 别名·028 §二）。"""
    配置 = {"ssh": "TX_02", "目录": "/home/ubuntu/cn-ci"}
    文件 = 仓库根 / "scripts/ci_client.json"
    if 文件.exists():
        try:
            配置.update(json.loads(文件.read_text(encoding="utf-8")))
        except Exception as e:
            print(f"  [警告] ci_client.json 解析失败（用默认 {配置}）：{e}")
    return 配置


def 解析预验结果(文本: str) -> tuple[bool, str]:
    """预验结果 JSON →（绿与否, 摘要）。纯函数（selftest 面）。

    868（预验红解析放行面核查）：解析面任何「看不懂」一律判红（硬红·宁拦勿放），
    禁翻绿——坏 JSON=红（原行为）；JSON 合法但非对象（字符串/数组等空壳）=红；
    对象壳内字段形态坏（步骤非映射等）原为 AttributeError 裸崩=调用方无捕获，
    同属「解析失败」语义，此处显式归红（顺带消崩溃面）。
    """
    try:
        d = json.loads(文本)
    except Exception:
        return False, "预验结果 JSON 解析失败：" + 文本[:120].replace("\n", " ")
    try:
        摘要 = "总秒=%s·日志=%s" % (d.get("总秒"), d.get("日志"))
        红步骤 = [k for k, s in (d.get("步骤") or {}).items() if s.get("rc", 0) != 0]
        尾部 = (d.get("日志尾部") or [])[-6:]
        尾部文本 = " | ".join(str(x).strip() for x in 尾部 if str(x).strip())[:400]
    except Exception:
        return False, "预验结果形态异常（非对象/字段形态坏——判红·868）：" + 文本[:120].replace("\n", " ")
    绿原 = d.get("绿")
    if not (isinstance(绿原, bool) or 绿原 in (0, 1)):
        # 868：绿字段形态收紧——合法仅 bool 与 0/1 数值（runner 契约·through 服务端
        #   INTEGER 列回读两形态）；字符串/None/其他=坏源形态判红——原 bool("false")
        #   =True 的字符串假绿面清零（宁拦勿放）。
        return False, "预验结果「绿」字段非法（%r——判红·868）：" % (绿原,) + 文本[:120].replace("\n", " ")
    绿 = bool(绿原)
    if 红步骤:
        摘要 += "·红步骤=" + "、".join(str(k) for k in 红步骤)
    if 尾部:
        摘要 += "\n  尾部：" + 尾部文本
    return 绿, 摘要


def 解析预验红用例们(结果文本):
    """216：从 runner 结果 JSON 文本提取 失败用例名集合（✗ 用例名: 形态行）。"""
    import re as _re
    return _re.findall(r"✗\s+(\S+?):", 结果文本 or "")


def develop已含链顶(链顶: str) -> bool:
    """562 修复（审查 P1-1）：核 develop 是否已含链顶（merge-base --is-ancestor·先 fetch
    保新鲜）——「已取消/已并入」收场判定的缺省 git 真查（selftest 注入面见 已并入收场）。
    fetch 失败不炸（降级用本地 remote-tracking 缓存判）；对象缺失=False 走拦截（保守）。"""
    运行(["git", "fetch", 主远程, 集成分支])
    return 运行(["git", "merge-base", "--is-ancestor",
                 链顶, f"{主远程}/{集成分支}"]).returncode == 0


def 已并入收场(链顶: str, 已含链顶查=None) -> tuple[bool, str | None]:
    """562 修复（审查 P1-1）：源分支已并入 develop（服务端核出台账 ✅·他批已集成）时
    预验作废的收场判定——旧路径把它当「执行中」白等 75 分钟总超时再降级 ssh 直发重跑
    全量（13~40 分钟云算力纯浪费）。返回 (拿到, 原因)，两态都即时收场不降级：
      develop 已含链顶 → (True, None) 放行（push develop 为 no-op——与池绿放行同契约）；
      未含（竞态窗口 develop 又前进·本链合并结果已过时）→ (True, 原因) 拦截（ff-only
      push 必拒——直接给「rebase 重报」出路，省一轮注定失败的 push）。
    已含链顶查=可注入（selftest 纯函数面·缺省 git 真查，沿用 562 临时用例检查 注入模式）。"""
    查 = 已含链顶查 or develop已含链顶
    if 查(链顶):
        print("  [预验] ✓ develop 已含本链链顶（他批已集成覆盖）——预验作废直接放行"
              "（push 为 no-op·免白等与重跑全量）")
        return True, None
    print("  [预验] ✗ develop 未含本链链顶——本链合并结果已过时（源分支经他批并入）")
    return True, ("源分支已并入 develop（他批已集成·本轮预验作废）而 develop 未含本链链顶"
                  "——本链合并结果已过时：请 rebase 最新 develop 后重新集成（562）。")


def 构造预验入队载荷(预验分支: str, 链顶: str, 源分支: str,
                    认领限制: str = "", 内容键: str = "") -> dict:
    """task_enqueue 载荷（服务端契约：分支/sha 必带·源分支 222·认领限制 337·内容键 830）。

    976 归纳：主入队（云端预验门禁）与池行 miss 防御性重入队（池轮询预验）两处
    逐字段同构——空值省略（服务端缺省语义=不限/NULL·旧服务端忽略未知字段）。
    """
    载荷 = {"分支": 预验分支, "sha": 链顶, "源分支": 源分支}
    if 内容键:
        载荷["内容键"] = 内容键
    if 认领限制:
        载荷["认领限制"] = 认领限制
    return 载荷


def 池轮询预验(预验分支: str, 链顶: str, 点名清单: list, 源分支: str = "",
               认领限制: str = "", 内容键: str = "", 已含链顶查=None, 状态查=None,
               轮询间隔: float | None = None,
               降级秒: int | None = None) -> tuple[bool, str | None]:
    """1021 池路径轮询：TX_01 预验任务表 → 认领者（TX_02/家机实例）跑完→取详情判绿红。
    返回 (是否拿到结论, 拦截原因)：绿=(True,None)·红=(True,原因)；
    TX_01 连续 3 轮不可达、任务持续排队超 池空降级秒（**且池有空闲实例**=真池空）或
    总超时=(False,None)——调用方降级 ssh 直发（830：池全忙时的排队是正常态·继续等）。
    源分支=222 防御性重入队时补带（网页分支列显示）；内容键=830 预验复用键随行补带。
    认领限制=337 平台白名单逗号串（防御性重入队时随行补带·池空降级守卫判据）。
    562 修复（审查 P1-1）：池行状态=「已取消」（runner 认领时服务端核出源分支已并入
    develop）→ 经 已并入收场 即时收场（放行/拦截），不再白等总超时后降级重跑。
    已含链顶查/状态查/轮询间隔/降级秒=可注入（selftest 纯函数面·缺省真查真等）。"""
    import time as _time
    截止 = _time.time() + 预验总超时分钟 * 60
    连续失败 = 0
    排队起始: float | None = None
    间隔 = 预验轮询间隔秒 if 轮询间隔 is None else 轮询间隔
    本地降级秒 = 池空降级秒 if 降级秒 is None else 降级秒
    while _time.time() < 截止:   # 池空降级秒=模块级常量（337 提升可注入·集中区惯例）
        _time.sleep(间隔)
        状态 = (状态查 or 服务调用)("/api/state")
        if 状态 is None:
            连续失败 += 1
            print(f"  [预验] TX_01 池查询失败（连续 {连续失败}/3）……")
            if 连续失败 >= 3:
                return False, None
            continue
        连续失败 = 0
        行 = next((t for t in (状态.get("预验任务") or []) if t.get("分支") == 预验分支), None)
        if 行 is None:
            # 任务行不在（服务重启丢表/异常裁剪）——重新入队防御（337：认领限制随行补带；
            #   830：内容键随行补带——防重入队后预验复用键丢失）
            服务调用("/api/task_enqueue",
                     构造预验入队载荷(预验分支, 链顶, 源分支, 认领限制, 内容键))
            continue
        if 行.get("状态") == "已取消":
            # 562 修复（审查 P1-1）：runner 认领时服务端核出源分支已并入 develop（他批
            # 已集成销账）——本预验作废。旧路径落入「执行中」分支白等 75 分钟总超时再
            # 降级 ssh 直发重跑全量（13~40 分钟云算力浪费）——即时收场见 已并入收场。
            print("  [预验] 池任务已取消——源分支已并入 develop（他批已集成）·本轮预验作废")
            return 已并入收场(链顶, 已含链顶查)
        if 行.get("状态") != "完成":
            if 行.get("状态") == "排队":
                # 830（2026-10-15 用户裁决）：池全忙时的排队=正常态（并行化后池常饱和）——
                #   不算池空信号：有空闲实例即被 runner 认领，继续等（旧逻辑此处误判
                #   「池空」→降级 ssh 直发绕池，与一个任务一个实例的并行模型相悖）。
                池忙闲 = (状态.get("池忙闲") or {})
                if 池忙闲.get("池全忙"):
                    排队起始 = None   # 池忙期间不计池空（等实例空出）
                    # 825 残余 B：补排队数（并行度观测——等的人可见自己在池里的排队面）
                    print(f"  [预验] 池全忙排队中（注册 {池忙闲.get('注册实例')}·执行中 "
                          f"{池忙闲.get('执行中')}·排队 {池忙闲.get('排队')}）"
                          "——出现空闲实例即被认领，继续等待……")
                    continue
                if 排队起始 is None:
                    排队起始 = _time.time()
                elif _time.time() - 排队起始 > 本地降级秒:
                    if 认领限制 and "linux-x86_64" not in 认领限制:
                        # 337 降级守卫：限制不含 linux-x86_64（=TX_02 平台）时 ssh 直发违背
                        #   发起方意图——守限制继续等（runner 上线即领·总超时仍兜底不黑洞）
                        print(f"  [预验] 池内有空闲实例却持续 5 分钟无人认领·认领限制 [{认领限制}] 不含 linux-x86_64"
                              "——跳过 TX_02 降级·守限制继续等待")
                        排队起始 = _time.time()   # 重置计时·每 5 分钟提醒一次
                        continue
                    # 1026 降级竞态治理：取消池内排队行再降级——防后来 runner 认领到
                    #   已被 ssh 直发接手的任务（双跑浪费+池红记录误挂·d48b5c5f91 实录）
                    服务调用("/api/task_cancel", {"分支": 预验分支})
                    print("  [预验] 池有空闲实例却持续 5 分钟无人认领（池异常·runner 未部署）——已取消池任务·降级 ssh 直发 TX_02")
                    return False, None
            else:
                排队起始 = None     # 已被认领（执行中）——重置池空计时
            余分 = int((截止 - _time.time()) // 60)
            print(f"  [预验] 池任务 {行.get('状态')}（认领者={行.get('认领者') or '待派'}·余约 {余分} 分钟）……")
            continue
        详情回 = 服务调用("/api/task_result?sha=" + 链顶[:10])
        文本 = json.dumps(详情回.get("详情"), ensure_ascii=False) if (详情回 and 详情回.get("详情")) else ""
        绿, 摘要 = 解析预验结果(文本)
        if 绿:
            print(f"  [预验] ✓ 池绿（{摘要}）——放行 push develop（develop 轮按池绿/本地档跳过·零重复算力）")
            return True, None
        # 216：批预验已知红点名放行——红用例**全部**命中 --allow-pool-red 清单=披露放行
        #   （使用责任=发起机：点名依据+集成广播披露·不实可 revert；未点名红仍硬拦）
        # 868 核查锚：放行要求红清单**非空**自证（红用例们216 and …）——空清单（✗ 行
        #   未取得）落下方拦截分支，不会以「未点名为空」翻绿（722「空集合恒真」同族
        #   面已非空自证；勿回改为「未点名为空即放行」形态）。
        红用例们216 = 解析预验红用例们(文本)
        未点名216 = [u for u in 红用例们216 if u not in 点名清单]
        if 红用例们216 and not 未点名216:
            print(f"  [预验] 红 {len(红用例们216)} 例全部命中 --allow-pool-red 点名清单"
                  f"（{'、'.join(红用例们216)}）——披露放行 push develop"
                  "（使用责任=发起机·广播披露·不实可 revert）")
            return True, None
        if 未点名216:
            print(f"  [预验] 红 {len(红用例们216)} 例·未点名红（{'、'.join(未点名216)}）"
                  "——硬拦（--allow-pool-red 点名可放行）")
        elif not 红用例们216:
            # 868 显形（空清单与无红区分）：轮红但 ✗ 用例清单未取得（日志尾部窗口未含
            #   行/形态未见）——此为「未显形红」非「无红」，同样硬拦；定界线索=摘要红步骤
            #   /runner 日志/e2e明细（详情 JSON 全文）。
            print(f"  [预验] 轮红但 ✗ 用例清单未取得（空清单=未显形红·非无红——868 显形）"
                  "——硬拦；核 runner 日志/e2e明细定界失败用例。")
        return True, (f"云端预验红（链顶 {链顶[:10]}·develop 未收到该提交）——{摘要}\n"
                      "  修复走任务分支（修复中间态禁推 develop·1008 纪律 1），绿后重新集成。")
    return False, None


def 预验前置检查(链顶: str, 最新develop: str,
                 祖先查询=None, 产物残留查询=None) -> str | None:
    """637 用户立法（2026-10-14·与单人封批同批落地·最高优先级）：预验前必须满足两前置——

    ①rebase 新鲜：链顶必须包含**当时最新** develop（最新 tip 是链顶祖先）——
      批路径由「组链 checkout -B 批分支 <fetch 后最新>」结构性保证，此处显式断言
      防未来路径回归（过时链预验=验了个寂寞·绿也作废）；
    ②干净构建：历史编译中间产物（target/ 构建树+target/audit2 的 v2p 指纹缓存）
      必须**全部清零**后才能预验——防脏产物假绿/增量混合态不可复现（483/551 超时
      作废循环诱因之一）。执行面=runner 认领后首步（ci_daemon 跑一轮·ci/预验-* 轮
      「清产物」步骤）；本端可注入 产物残留查询 做清单断言（selftest 纯函数面/
      本机全量面）。

    返回 None=前置满足；非 None=拒绝原因（不推预验分支）。
    """
    if 祖先查询 is None:
        def 祖先查询(基: str, 孙: str) -> bool:
            return 运行(["git", "merge-base", "--is-ancestor", 基, 孙]).returncode == 0
    if not 祖先查询(最新develop, 链顶):
        return ("预验前置①不满足（637 用户立法）：链顶未基于最新 develop——"
                "先 fetch+rebase 到最新再集成（过时链禁预验·禁集成）。")
    if 产物残留查询 is not None:
        残留们 = 产物残留查询()
        if 残留们:
            return ("预验前置②不满足（637 用户立法）：历史编译产物未清零（"
                    + "、".join(残留们) + "）——清零后才能预验。")
    return None


# 830：非 --wait 模式哨兵——入池成功但未取到结论时，云端预验门禁 返回此值；
#   批流程识别后打印续推提示并以 0 退出（不 push·链与队列行保留，重跑本命令续推）。
# 976 三态协议统一（云端预验门禁 返回值 ↔ 批流程标志位映射）：云端预验门禁 返回 =
#   None（放行 push develop）| str（拦截原因·调用方失败退出）| 已入池哨兵（仅非 --wait
#   路径；批流程转「已入池待续」早退 exit 0——solo 恒 --wait 语义·不产生哨兵）。
已入池哨兵 = "__已入池待续推__"


def 查可复用预验绿(源分支: str, 内容键: str, 状态查=None) -> bool:
    """830 预验绿复用（用户裁决「预验绿后不需要对齐头重验·再测还是绿灯」的服务端侧兑现）：
    服务端存在「源分支=本任务 且 内容键（=任务分支 tip sha）== 当前内容键 且 完成·绿」
    的历史预验记录 → 该内容已全量验证过绿，免重跑云端全量（develop 变了也不重测——
    组合正确性由 develop 平台轮转全量权威守护·830 用户裁决）。

    内容键=任务分支 tip（原始提交）——人 rebase/改分支后 tip 变=内容变了→不复用（正确）。
    查询失败/无命中/内容键空 → False（保守回正常预验·宁多跑不漏放）。
    状态查=可注入（selftest 纯函数面·同 池轮询预验 惯例）。"""
    if not 源分支.startswith("任务/") or not 内容键:
        return False
    态 = (状态查 or 服务调用)("/api/state")
    if not 态:
        return False
    return any(str(行.get("源分支") or "") == 源分支
               and str(行.get("内容键") or "") == 内容键
               and 行.get("状态") == "完成" and 行.get("绿")
               for 行 in (态.get("预验任务") or []))


def 云端预验门禁(链顶: str, 参数: argparse.Namespace,
                 源分支: str = "", 等待: bool = True) -> str | None:
    """链顶（=合并结果）推预验分支触发全量（linux-x86_64 面），绿放行 push develop、红拦截。
    1021 执行器池化两层降级：①TX_01 任务池（多 runner 认领·谁空闲谁跑·一个任务一个实例）→
    ②池不可达/超时=ssh 直发 TX_02（1008 原路径保留·仍为预验非绕过）。
    770：--no-cloud-gate 逃生门已硬禁（parse 后即拒）——返回 None=放行；str=拦截原因；
    已入池哨兵=非 --wait 模式入池成功待续推（调用方以 0 退出·不 push）。
    830：源分支=调用方传原始任务分支（批流程 HEAD 已切 batch/* 时勿取当前分支——
    board_ci 归类/562 已并入检测/预验复用键均按「任务/<号>」形态消费）；等待=False
    =入池即回（哨兵）、不挂机等结果。三态协议详见 已入池哨兵 定义处。"""
    import time as _time
    if 参数.dry_run:
        print("  [预验·演练] 跳过云端真跑（--dry-run）——正式集成对链顶真跑全量预验")
        return None
    # 637 用户立法：预验前两前置——①链顶含当时最新 develop（显式断言·组链结构性保证的回归防线）；
    # ②产物清零由 runner 认领后首步执行（ci_daemon 跑一轮·ci/预验-* 轮清产物步骤——此处托付声明）
    最新 = 输出(["git", "rev-parse", f"{参数.remote}/{集成分支}"])
    前置问题 = 预验前置检查(链顶, 最新)
    if 前置问题:
        return 前置问题
    print("  [预验前置·637] ①链顶含最新 develop ✓ ②产物清零=runner 认领后首步"
          "（ci_daemon 清产物步骤·target/ 全清含 v2p 指纹缓存）")
    # 830：源分支=原始任务分支（调用方传；缺省=当前分支回退·兼容 solo 路径在任务分支上直调）
    源分支 = 源分支 or (输出(["git", "branch", "--show-current"]) or "未知")
    # 830 复用面：同源分支+同内容键（=任务分支 tip）已有完成·绿 → 免重跑云端全量
    #   （用户裁决：预验绿后不需要对齐头重验——内容未变·绿照用·组合正确性由 develop
    #   平台轮转全量权威守护；人改分支后 tip 变=内容变→不复用，正确地重跑）
    内容键 = (输出(["git", "rev-parse", f"{参数.remote}/{源分支}"])
             if 源分支.startswith("任务/") else "")
    if 查可复用预验绿(源分支, 内容键):
        print(f"  [预验·复用] {源分支}（内容键 {内容键[:10]}）已有完成·绿记录——"
              "免重跑云端全量（830 复用面：内容未变·绿照用·组合由 develop 轮转权威守护）")
        return None
    配置 = 读CI配置()
    预验分支 = 预验远程分支前缀 + 链顶[:10]
    sha10 = 链顶[:10]
    推 = 运行(["git", "push", 参数.remote, f"{链顶}:refs/heads/{预验分支}"])
    if 推.returncode != 0:
        return f"预验分支推送失败（{预验分支}）——检查远程权限后重试。"
    # 远端预验分支生命周期（863 立·901 补全——删除前提不变量）：**删除 ⟺ 池行已确认
    #   终态（完成/已取消/明确拒绝未建行）**——runner 认领后首步 git fetch 该分支同步，
    #   池行可能仍活时删=runner 同步 fatal: couldn't find remote ref → 预验全数同步失败
    #   被踢+975「同步失败」告警（746/764/758/644/728f 五连实测·--wait 路径轮询到结果后
    #   才删故无恙——对照组实证）。置位保留的三处来路：①非 --wait 哨兵（入池即返·runner
    #   尚未 fetch）；②337 池守卫拦截（行活·待白名单 runner 上线）；③降级 ssh/入队响应
    #   丢失（901——行未确认终态）。分支收口=runner 消费转终态/branch_cleanup 兜底/
    #   下一轮同 sha 集成（池轮询拿到结论走 finally 删除）。
    保留预验分支 = False
    try:
        # ── ① 池路径：入队（幂等·已完成绿直接复用不重跑）+轮询认领结果
        # 222：附带源分支（=发起集成的任务分支）——TX_01 网页预验池「分支」列显示归属任务；
        #   830：board_ci 归类/562 已并入检测/预验复用键均按「任务/<号>」形态消费（批流程传参）
        # 337 认领限制：--pool-limit 平台白名单——任务只被白名单内平台 runner 认领，
        #   定向规避已知环境红平台（335 单位机 arm64 老工具链）；缺省不限=291 混合池默认不变
        认领限制 = ",".join(参数.pool_limit)
        入队 = 服务调用("/api/task_enqueue",
                        构造预验入队载荷(预验分支, 链顶, 源分支, 认领限制, 内容键))
        if 入队 is not None and 入队.get("ok"):
            print(f"  [预验] 已入 TX_01 任务池（{预验分支}·源 {源分支}·认领限制 {认领限制 or '不限'}）")
            if not 等待:
                print("  [预验] 非 --wait：不挂机等结果——池有空闲实例即跑（一个任务一个实例）·"
                      "绿后重跑本命令续推（或加 --wait 挂机等结果）")
                保留预验分支 = True   # 863：入池即返——runner 尚未 fetch，删分支=预验同步必败
                return 已入池哨兵
            拿到, 原因 = 池轮询预验(预验分支, 链顶, 参数.allow_pool_red, 源分支, 认领限制,
                                    内容键=内容键)
            if 拿到:
                return 原因
            if 认领限制 and "linux-x86_64" not in 认领限制:
                # 337 降级守卫：池超时/不可达且限制不含 linux-x86_64——TX_02 直发违背限制，
                #   不静默改平台重跑（同 sha 换平台口径不一致）·显式拦截并给出路
                #   （863 同族：池行仍活——白名单 runner 上线后仍会认领 fetch，分支必须保留）
                保留预验分支 = True
                return (f"池路径超时/不可达·认领限制 [{认领限制}] 不含 linux-x86_64（TX_02 ssh 直发违背限制）"
                        "——等待限制平台 runner 上线后重试·或修正 --pool-limit（770：逃生门已硬禁）。")
            # 901 修复（863 同族残余面·深度机 10-16 立案）：降级 ssh 前置位保留——池轮询
            #   未拿结论=池行可能仍在排队/执行中（池空降级的 task_cancel 是尽力而为可失败；
            #   总超时/3 连不可达路径未取消），runner 稍后认领同 sha 任务首步 git fetch 该
            #   分支——删=fetch fatal→975「同步失败」告警+假红（863 五连同族）。删除前提=
            #   池行确认终态或从未建行（入队三态分流）——降级路径两者皆未确认，一律保留；
            #   收口随 runner 消费/下一轮同 sha 集成/branch_cleanup 兜底。
            保留预验分支 = True
            print("  [预验] 池路径超时/不可达——降级 ssh 直发 TX_02（028 §四）……")
        else:
            拒说明 = str((入队 or {}).get("说明", ""))
            if 入队 is not None:
                # 562：入队被拒（如源分支已并入 develop 的 562 拒入）——说明随行打印保诊断
                print(f"  [预验] 入队被拒：{拒说明}")
                if "已并入" in 拒说明:
                    # 562 修复（审查 P1-1 同族）：服务端已核出源分支并入 develop（台账 ✅）
                    # ——预验无意义，ssh 直发重跑全量同为浪费。按「已取消」同款收场判定：
                    # develop 已含链顶→放行（push no-op）；未含→拦截给 rebase 出路。
                    return 已并入收场(链顶)[1]
                # 明确拒绝（非已并入）=服务端确认未建行（拒绝路径无写入）——降级 ssh 照删
                #   （无 runner 可认领·保留=无消费者死残留）
            else:
                # 入队响应丢失（超时/网络/队列服务未配置）——请求可能已到达建行而响应未回，
                #   行活跃与否不确定：保守保留（miss 不静默——不确定不当「未建」处置：
                #   误删=runner fetch fatal·975 告警；保留=残留由收口面兜底）
                保留预验分支 = True
            print("  [预验] TX_01 池不可用——降级 ssh 直发 TX_02（028 §四）……")
        # ── ② ssh 直发降级（1008 原路径原样保留）
        # rm 旧预验结果（同 sha 二次预验=CAS 竞争重试路径·旧文件不可当本轮结果）
        # 1019 缺陷修复（两轮实验定音）：真根因=链尾 `&` 使整条 `cd && rm && nohup` 链异步化，
        #   bash fork subshell 执行链并**持有 ssh 会话管道**直到链内命令（daemon wait-lock 数十
        #   分钟）结束——sshd 不出 EOF→ssh 挂→subprocess timeout=30 必崩（< /dev/null 只切
        #   nohup 自身 stdin 救不了·变体矩阵实证：链尾 & 必挂 10s/组内 & 0.4s）。故 & 收进
        #   { } 组内+组级三管道切断，前置 cd/rm 保持前台（失败即停不误触发）。
        触发 = subprocess.run(
            ["ssh", "-o", "ConnectTimeout=15", 配置["ssh"],
             "cd %s && rm -f ci-logs/预验_%s.json && { nohup env CN_CI_BRANCH=%s "
             "python3 scripts/ci_daemon.py --once --force --wait-lock "
             "> ci-logs/预验启动_%s.log 2>&1 </dev/null & } >/dev/null 2>&1 </dev/null; echo 触发成功"
             % (配置["目录"], sha10, 预验分支, sha10)],
            capture_output=True, text=True, timeout=30)
        if 触发.returncode != 0 or "触发成功" not in (触发.stdout or ""):
            return ("云端预验触发失败（ssh %s）：%s——TX_02 不可达或目录异常（028 §四）。"
                    "等待恢复重试（770：--no-cloud-gate 逃生门已硬禁）。"
                    % (配置["ssh"], (触发.stderr or 触发.stdout or "").strip()[:200]))
        print(f"  [预验] TX_02 已触发（{预验分支}·全量 linux-x86_64 面"
              f"·绿轮实测 13~16min/红轮 33~40min·含锁排队）")
        截止 = _time.time() + 预验总超时分钟 * 60
        while _time.time() < 截止:
            _time.sleep(预验轮询间隔秒)
            取 = subprocess.run(
                ["ssh", "-o", "ConnectTimeout=15", 配置["ssh"],
                 "cat %s/ci-logs/预验_%s.json 2>/dev/null" % (配置["目录"], sha10)],
                capture_output=True, text=True, timeout=30)
            if 取.returncode == 0 and 取.stdout.strip():
                绿, 摘要 = 解析预验结果(取.stdout)
                if 绿:
                    print(f"  [预验] ✓ 绿（{摘要}）——放行 push develop（daemon 已写 latest·轮询轮免重跑）")
                    return None
                # 216：ssh 降级路径同款已知红点名放行（与池路径同判据同责任）
                # 868 核查锚：同池路径——放行要求红清单非空自证（空清单落拦截·不翻绿）。
                红用例们216b = 解析预验红用例们(取.stdout)
                未点名216b = [u for u in 红用例们216b if u not in 参数.allow_pool_red]
                if 红用例们216b and not 未点名216b:
                    print(f"  [预验] 红 {len(红用例们216b)} 例全部命中 --allow-pool-red 点名清单"
                          f"（{'、'.join(红用例们216b)}）——披露放行 push develop"
                          "（使用责任=发起机·广播披露·不实可 revert）")
                    return None
                if not 红用例们216b:
                    # 868 显形（同池路径）：轮红但 ✗ 清单未取得=未显形红·非无红——硬拦。
                    print(f"  [预验] 轮红但 ✗ 用例清单未取得（空清单=未显形红·非无红——868"
                          " 显形）——硬拦；核 runner 日志/e2e明细定界。")
                return (f"云端预验红（链顶 {sha10}·develop 未收到该提交）——{摘要}\n"
                        "  修复走任务分支（修复中间态禁推 develop·1008 纪律 1），绿后重新集成。")
            余分 = int((截止 - _time.time()) // 60)
            print(f"  [预验] 等待云端结果（余约 {余分} 分钟）……")
        return (f"云端预验超时（{预验总超时分钟} 分钟——TX_02 忙/挂死·028 §四处置）。"
                "等待恢复重试（770：--no-cloud-gate 逃生门已硬禁）。")
    finally:
        if 保留预验分支:
            # 863/901：池行可能仍待 runner 认领消费（认领后首步 git fetch 本分支）——本轮
            #   不删；分支收口随 runner 消费转终态/branch_cleanup 兜底/下一轮同 sha 集成。
            print(f"  [提示] 预验分支 {预验分支} 保留（池行可能待 runner 认领 fetch·删除前提未满足）")
        else:
            清理 = 运行(["git", "push", 参数.remote, "--delete", 预验分支])
            if 清理.returncode != 0:
                print(f"  [警告] 预验分支 {预验分支} 删除失败——branch_cleanup.py 兜底。")
        # 1020（#180）：杀 TX_02 上本 sha 的 wait-lock 残留 daemon（按 sha 分拣·
        #   1017 实录：绿轮放行/超时后 daemon 仍排队占队=挡他批）——pkill 无命中
        #   无害（绿轮正常路径 daemon 已退出）。
        subprocess.run(
            ["ssh", "-o", "ConnectTimeout=15", 配置["ssh"],
             "pkill -f '预验_%s' 2>/dev/null; exit 0" % sha10],
            capture_output=True, text=True, timeout=30)


# ═══════════════════════════════════════════════════════════════════════════
# 批量集成（911·bors 式）——看板集成队列段：解析/变换（纯文本·selftest 面）+ plumbing 直推
# ═══════════════════════════════════════════════════════════════════════════

看板文件名 = "三机任务看板.md"
队列段标题 = "## 集成队列（bors 式批组建·911 立·AGENTS.md §8.2-B）"
时刻格式 = "%m-%d %H:%M:%S"   # 秒级（911 演练实录：分钟级使同分钟内批主排队尾——组链序=报名序=冲突踢后到者，必须有秒）


def 现在时刻() -> str:
    return datetime.now().strftime(时刻格式)


# 976：原 解析时刻（分钟级旧行兼容解析）随看板写侧退役删除——读侧（解析队列/--status）
#   按字符串序消费时刻，无解析面（830 退役失联判定后即无调用点）。

# 830（2026-10-15 用户裁决）：失联判定/批主接管面随并行化退役——原「行新鲜时刻/
#   失联分支们」两纯函数删除（旧用途：--wait 自动接管失联批主；新模型无批主——
#   任务失败由服务端超时回收队列行·任务自身重报即续推）。


def 解析队列(看板文本: str) -> list[dict]:
    """解析看板集成队列段的行们。行格式：| 分支 | 基线 | 报名时刻 | 写集摘要 | 状态 |"""
    行们 = 看板文本.splitlines()
    段起 = next((n for n, 行 in enumerate(行们) if 行.strip() == 队列段标题), None)
    if 段起 is None:
        return []
    出: list[dict] = []
    for 行 in 行们[段起 + 1:]:
        if 行.startswith("## "):   # 段终于下一个二级标题
            break
        if not 行.startswith("|"):
            continue
        列们 = [列.strip() for 列 in 行.split("|")[1:-1]]
        if len(列们) != 5 or 列们[0] == "分支" or set(列们[0]) <= set("-: "):
            continue  # 表头/分隔行
        出.append({"分支": 列们[0], "基线": 列们[1], "时刻": 列们[2],
                   "摘要": 列们[3], "状态": 列们[4], "行": 行.rstrip()})
    return 出


# 976 清理（v5·1016 看板直推废档的收尾）：看板段**写侧**函数族（原 队列表头行/队列加行/
#   队列改状态/队列清行们/队列重报/队列改时刻）在直推通道退役后全部失去调用点——删除。
#   读侧（解析队列/远端看板文本/队列段标题）保留：队列读 双源合并与 --status 仍消费
#   看板段存量行（旧版机未升级 920 服务层期间的报名行兼容面）。


# 830（2026-10-15 用户裁决）：选批成员（637 单人封批/--gather 收拢/770 忙时拼车三路）
#   随并行化退役——新模型每个任务独立组链入池（成员=自己·恒单人·无批主无拼车无等待），
#   原函数删除；组链调用点直接构造成员=自己一条。


# 976 清理：原 直推看板提交（plumbing 直推：临时 index+commit-tree 免 checkout 提交看板）
#   随 v5·1016 看板直推废档后即不可达（函数首行 return 占位、实现体永久死代码）——
#   连同恒真返回值参与的控制流一并删除：批流程报名段的「服务端不可达→失败」判定
#   直取 join 结果（行为等价·见批流程 [2] 段）；--join 仅报名路径恒失败语义显式化
#   （见 批流程_join_only）。


def 远端看板文本(远程: str = 主远程) -> str:
    运行(["git", "fetch", 远程])
    父 = 输出(["git", "rev-parse", f"{远程}/{集成分支}"])
    return subprocess.run(["git", "show", f"{父}:{看板文件名}"],
                          cwd=仓库根, capture_output=True).stdout.decode("utf-8")


# ── 队列服务层（920·协议 v4 五节点）：服务端优先+看板回退（行为超集·降级不失效）─────────
# 治「git 被当状态数据库」（920 诊断：56% 提交只改看板·917 单批 8 对起批/排队乒乓）：
#   中间态（集成中/失败恢复/封批）只进服务端**不再写 develop**=零 git 提交；
#   边界（报名/销账）双写（服务端为主+看板兼容未升级 920 版的他机——三机升级后看板段写入退役）。
# 配置：环境变量 CN_QUEUE_URL，或 scripts/queue_client.json 的 "url"（gitignore·不入库）
#   形如 {"url": "http://<TX_01>:8300", "令牌": "..."}。无配置=纯看板路径（现状行为）。
# 556：令牌统一走 intent.读令牌()（本树→主树副本→环境变量 CN_BOARD_TOKEN/CN_QUEUE_TOKEN
#   ——治令牌轮换后旧 worktree 静默 401）；url 仍=CN_QUEUE_URL 或本树文件（url 不轮换）。
队列服务URL = os.environ.get("CN_QUEUE_URL", "")
_客户端配置 = 仓库根 / "scripts" / "queue_client.json"
if not 队列服务URL and _客户端配置.exists():
    try:
        队列服务URL = str(json.loads(_客户端配置.read_text(encoding="utf-8")).get("url", ""))
    except Exception:
        pass
try:
    import intent
    队列服务令牌 = intent.读令牌()
except Exception:
    队列服务令牌 = ""


def 服务调用(路径: str, 数据: dict | None = None) -> dict | None:
    """GET（数据=None）/POST JSON——任何失败返回 None（调用方回退看板路径）。"""
    if not 队列服务URL:
        return None
    import urllib.request
    try:
        if 数据 is None:
            请求 = urllib.request.Request(队列服务URL.rstrip("/") + 路径,
                                         headers={"Authorization": "Bearer " + 队列服务令牌})
        else:
            请求 = urllib.request.Request(队列服务URL.rstrip("/") + 路径,
                                         data=json.dumps(数据, ensure_ascii=False).encode("utf-8"),
                                         headers={"Content-Type": "application/json",
                                                  "Authorization": "Bearer " + 队列服务令牌})
        with urllib.request.urlopen(请求, timeout=4) as 响应:
            return json.loads(响应.read().decode("utf-8"))
    except Exception:
        return None


# 830（2026-10-15 用户裁决）：原「池全忙查询」函数随忙时拼车退役——池忙闲快照改由
#   池轮询预验 内联消费（/api/state 响应的「池忙闲」字段：池全忙=排队是正常态·继续等）。



def 队列读(远程: str = 主远程) -> list[dict] | None:
    """服务端队列（字段适配成看板行形态）+看板段双源合并——不可用返回 None（调用方读看板）。

    过渡期双源合并（920）：旧版机（未升级服务层）报名只写看板段——纯服务端视图会漏掉
    它们；服务端行权威（状态/时刻以服务端为准），看板段独有行并入（分支键去重）。
    三机全部升级后看板段自然清空=退化为纯服务端读。
    """
    态 = 服务调用("/api/state")
    if 态 is None:
        return None
    行们 = [{"分支": r.get("分支", ""), "基线": r.get("基线", ""),
             "时刻": r.get("报名时刻", ""), "摘要": r.get("写集摘要", ""),
             "状态": r.get("状态", ""), "更新": r.get("更新时刻", "")} for r in 态.get("队列", [])]
    try:
        看板行们 = 解析队列(远端看板文本(远程))
    except Exception:
        看板行们 = []
    已知 = {行["分支"] for 行 in 行们}
    行们.extend(行 for 行 in 看板行们 if 行["分支"] not in 已知)
    return 行们


def 池行龄分(时刻: str) -> int | None:
    """825 残余 B：入队时刻（%m-%d %H:%M:%S 秒级戳·无年份补当前年）→ 已等分钟。
    坏串/未来时刻=None（只印可算的·宁缺不编——服务端 行龄秒 同口径的服务端外版本）。"""
    try:
        差 = (datetime.now() - datetime.strptime(
            时刻, "%m-%d %H:%M:%S").replace(year=datetime.now().year)).total_seconds()
    except (TypeError, ValueError):
        return None
    return int(差 // 60) if 差 >= 0 else None


def 打印验证池状态() -> None:
    """825 残余 B（并行度观测·池饱和排队可见性）：--status 增验证池段——池忙闲
    （注册/执行中/排队/全忙）+排队与执行中行（来源/认领限制/已等分钟/排队原因）。
    用户 10-15 报「预检才两个·池实例空转」的观察面（原 --status 只见集成队列）。
    服务不可达=一行提示（降级不失效·对齐 队列读 口径；数据面=/api/state 825 版
    池忙闲含排队数）。"""
    态 = 服务调用("/api/state")
    if 态 is None:
        print("[验证池] 队列服务不可达/未配置——跳过（仅集成队列视图）")
        return
    忙 = 态.get("池忙闲") or {}
    注 = 忙.get("注册实例", 0)
    if 忙.get("池全忙"):
        状 = "池全忙（排队是正常态·有空闲实例即被认领）"
    elif 注:
        状 = "有空闲实例"
    else:
        状 = "暂无注册实例（runner 未上线）"
    print(f"[验证池] 注册 {注} · 执行中 {忙.get('执行中', 0)} · 排队 {忙.get('排队', 0)} — {状}")
    行们 = [t for t in (态.get("预验任务") or [])
            if t.get("状态") in ("排队", "执行中")]   # 完成/已取消行不上桌（详情在页面看）
    for 行 in 行们:
        分支 = str(行.get("分支") or "")
        源 = str(行.get("源分支") or 分支)
        龄 = 池行龄分(str(行.get("入队时刻") or ""))
        等 = f"·已等 {龄} 分" if 龄 is not None else ""
        if 行.get("状态") == "执行中":
            print(f"  执行中｜{源}｜{分支}｜runner {行.get('认领者') or '?'}{等}")
            continue
        if 忙.get("池全忙"):
            因 = "池全忙——等任一实例空出（先空闲先认领）"
        elif 行.get("认领限制"):
            因 = f"限 {行.get('认领限制')} 平台实例认领——该平台空闲即被领走"
        else:
            因 = "池有空闲实例——待实例认领（数秒级）"
        print(f"  排队｜{源}｜{分支}｜限 {行.get('认领限制') or '不限'}{等}｜{因}")


板服务URL = os.environ.get("CN_BOARD_URL", "http://124.222.106.84:8301")


def 板调用(路径: str, 数据: dict) -> bool:
    """看板服务写操作（308h·意图生命周期联动）——失败仅返回 False 不阻断集成。"""
    import urllib.request
    try:
        请求 = urllib.request.Request(板服务URL.rstrip("/") + 路径,
                                     data=json.dumps(数据, ensure_ascii=False).encode("utf-8"),
                                     headers={"Content-Type": "application/json",
                                              "Authorization": "Bearer " + 队列服务令牌})
        with urllib.request.urlopen(请求, timeout=4) as 响应:
            return bool(json.loads(响应.read().decode("utf-8")).get("好"))
    except Exception:
        return False


def 队列写(操作: str, 数据: dict) -> bool:
    """写操作（join/update/touch/clear）——服务不可用/未配置返回 False（回退看板直推）。"""
    结果 = 服务调用("/api/" + 操作, 数据)
    if 结果 is None:
        return False
    if not 结果.get("ok"):
        print(f"  [队列服务] {操作} 被拒：{结果.get('说明', '?')}——回退看板路径。")
    return bool(结果.get("ok"))


def 队列心跳(分支: str) -> None:
    """562：队列行心跳（完全静默）——刷服务端行更新时刻=活会话证明，服务端排队/集成中
    回收与成员 --wait 失联判定以心跳新鲜度为据。服务不可达/旧服务端未知操作一律静默
    （心跳是优化面：失败退化为旧阈值行为·服务端超时兜底语义不变）。"""
    if 队列服务URL:
        服务调用("/api/heartbeat", {"分支": 分支})


def 报名行(分支: str, 基线: str, 摘要: str) -> dict:
    return {"分支": 分支, "基线": 基线, "时刻": 现在时刻(), "摘要": 摘要, "状态": "排队"}


def 批写集(成员们: list[dict], 基底: str) -> list[str]:
    """批内全部成员写集并集（去重·分类门禁深度的输入）。

    成员写集=分支自身改动（三点·1008）——基底（最新 develop）常领先成员基线，
    两点 diff 会把 develop 自 merge-base 起的反向差异算入（#993：4 文件虚大成 86）。"""
    全: list[str] = []
    for 行 in 成员们:
        全.extend(三点间改动(基底, 行["分支"]))
    return sorted(set(全))


def 组链(成员们: list[dict], 基底: str, 参数: argparse.Namespace) -> tuple[str | None, list[str], str | None, dict]:
    """按报名序 cherry-pick 叠链：基底→成员1→成员2→…（链分支=当前 HEAD 检出态）。

    冲突=踢出该成员（cherry-pick --abort 后继续下一位·批不因成员卡死——被踢者本机解完
    冲突报下批）；--drop 点名的成员直接不入链。返回（链顶 sha, [(分支, 原因), …], 错误,
    伞批销账映射{分支:[号,…]}——392：成员提交信息 [销账:…] 登记号·组链时分支尚在，
    是收集唯一可靠时机）。调用方须已把 HEAD 切到批分支（基底）——本函数只管叠不建。
    """
    踢出: list[tuple[str, str]] = []   # (分支, 原因∈{归因, 冲突})——销账按原因标态
    销账映射: dict[str, list] = {}
    点名 = set(参数.drop or [])
    for 行 in 成员们:
        分支 = 行["分支"]
        if 分支 in 点名:
            踢出.append((分支, "归因"))
            print(f"  [组链] {分支} 被 --drop 点名——不入链（门禁红归因踢出·重验走剩余成员）")
            continue
        远端分支 = f"{参数.remote}/{分支}"
        if 输出(["git", "rev-parse", "--verify", f"refs/remotes/{远端分支}"]) == "":
            踢出.append((分支, "冲突"))
            print(f"  [组链] {分支} 远端不存在（fetch 后仍缺）——踢出")
            continue
        提交们 = 输出(["git", "rev-list", "--reverse", f"{基底}..{远端分支}"]).split()
        if not 提交们:
            踢出.append((分支, "冲突"))
            print(f"  [组链] {分支} 相对基底无提交（空分支？）——踢出")
            continue
        伞账号 = board_closeout.收集提交们销账号(提交们)
        if 伞账号:
            销账映射[分支] = 伞账号
            print(f"  [组链] {分支} 伞批销账登记：{伞账号}")
        print(f"  [组链] 叠入 {分支}（{len(提交们)} 提交）")
        冲突 = False
        for 提交 in 提交们:
            # 830：cherry-pick 确定性化——GIT_COMMITTER_DATE=原提交时刻，同基同内容=同 sha
            #   （服务端预验按链顶 sha 幂等天然命中：换会话重跑/多人重跑免重复全量·审计链稳定）
            原时刻 = 输出(["git", "show", "-s", "--format=%cI", 提交]) or datetime.now().isoformat()
            拾取 = 运行(["git", "cherry-pick", 提交],
                        env=dict(os.environ, GIT_COMMITTER_DATE=原时刻))
            if 拾取.returncode != 0:
                运行(["git", "cherry-pick", "--abort"])
                冲突 = True
                break
        if 冲突:
            踢出.append((分支, "冲突"))
            print(f"  [组链] {分支} cherry-pick 冲突——踢出（批主本机分支可自行解冲突后重报；"
                  f"他机分支请回自己分支解完报下批）")
    if 输出(["git", "rev-parse", "HEAD"]) == 基底:
        return None, 踢出, "批内全部成员被踢出（无一可叠）——请检查成员分支状态。", 销账映射
    return 输出(["git", "rev-parse", "HEAD"]), 踢出, None, 销账映射
def 单次集成尝试(平台: str, 上次已验基准: str | None, 参数: argparse.Namespace) -> tuple[bool, str | None, str | None]:
    """一轮 fetch→rebase→门禁→push。返回（成功与否, 失败信息, 本轮已验证的 develop 基准）。"""
    运行(["git", "fetch", 主远程])
    最新 = 输出(["git", "rev-parse", f"{主远程}/{集成分支}"])

    # rebase 到最新 develop：冲突=分支主人自己解（自动 abort·禁止 develop 手解）
    基底 = 输出(["git", "merge-base", "HEAD", 最新])
    if 基底 != 最新:
        print(f"[2] rebase 到最新 {集成分支}（{最新[:8]}）")
        rebase = 运行(["git", "rebase", 最新])
        if rebase.returncode != 0:
            运行(["git", "rebase", "--abort"])
            return False, "rebase 冲突——请在任务分支上自行解决后重新集成（AGENTS.md §5）。", None

    文件们 = 改动文件清单(最新)
    if not 文件们:
        return False, "分支相对 develop 无任何改动（空集成）。", None
    须全量 = 分类写集(文件们)

    # 门禁跑在合并结果上；重试时仅当 develop 增量（上轮已验 tip→本轮最新）触及 src/tests 才重跑全量门禁
    if 上次已验基准 is None or 上次已验基准 == 最新:
        增量 = 文件们 if 上次已验基准 is None else []
    else:
        增量 = 三点间改动(上次已验基准, 最新)   # 两 develop tip=祖先链·三点=两点等价
    增量须全量 = 分类写集(增量)
    print(f"[3] 快速门禁（改动 {len(文件们)} 个文件）")
    问题 = 快速门禁(文件们)
    if 问题:
        return False, 问题, None
    if 须全量 and (上次已验基准 is None or 增量须全量):
        print(f"[4] 全量门禁（平台={平台}·跑在合并结果上）")
        问题 = 全量门禁(平台, 参数.allow_known_red, 参数.fast_lane)
        if 问题:
            return False, 问题, None
        if 参数.cloud_gate:
            print("[4.5] 云端预验门禁（1008·链顶=合并结果·绿才 push develop）")
            问题 = 云端预验门禁(输出(["git", "rev-parse", "HEAD"]), 参数)
            if 问题:
                return False, 问题, None
    elif 须全量:
        print("[4] 全量门禁跳过（develop 增量纯文档·已验部分仍有效）")

    push_命令 = ["git", "push", "--dry-run" if 参数.dry_run else None, 主远程, f"HEAD:{集成分支}"]
    push_命令 = [c for c in push_命令 if c]
    print(f"[5] push {主远程} {集成分支}（ff-only·CAS）")
    结果 = 运行(push_命令)
    if 结果.returncode != 0:
        # None=竞争失败（外层 rebase 重试）；最新=本轮门禁已验证的 develop tip（供增量判定）
        return False, None, 最新
    return True, None, 最新


def solo流程(平台: str, 参数: argparse.Namespace) -> int:
    """旧单分支集成路径（--solo 逃生门·AGENTS.md §5 原流程——队列机制异常时不经队列直集成）。"""
    print(f"== 合并队列（solo 逃生门）｜平台={平台}｜模式={'演练' if 参数.dry_run else '集成'} ==")
    问题 = 前置自检(参数.remote)
    if 问题:
        return 失败(问题)
    print("[1] 前置自检通过")
    已验基准: str | None = None
    for 尝试 in range(1, 最大重试 + 1):
        成功, 信息, 基准 = 单次集成尝试(平台, 已验基准, 参数)
        if 基准:
            已验基准 = 基准
        if 成功:
            if not 参数.dry_run:
                镜像 = 运行(["git", "push", 镜像远程, f"HEAD:{集成分支}"])
                if 镜像.returncode != 0:
                    print("  [警告] github 镜像补推失败——按惯例下次提交补推（不影响集成有效性）。")
            分支 = 输出(["git", "branch", "--show-current"])
            if 分支.startswith("任务/") and not 参数.dry_run:
                # 392：删远端分支前收伞批销账登记（引用删后提交历史不可达）
                solo基底 = 输出(["git", "merge-base", f"{参数.remote}/{分支}", "HEAD"])
                伞账号 = board_closeout.收集远端分支销账号(参数.remote, 分支, solo基底)
                清理 = 运行(["git", "push", 参数.remote, "--delete", 分支])
                if 清理.returncode == 0:
                    print(f"  [清理] 远程任务分支 {分支} 已删（内容已入 {集成分支}·AGENTS.md §7 集成即删）。")
                    总账收口([分支], 输出(["git", "rev-parse", "HEAD"]), 额外号们=伞账号)
                else:
                    print("  [警告] 远程任务分支删除失败（不影响集成有效性）"
                          "——稍后 python scripts/branch_cleanup.py 兜底。")
            print(f"\n[集成成功·solo] {分支} → {集成分支}（{输出(['git', 'rev-parse', 'HEAD'])[:8]}）"
                  "——在后续提交信息以〔通告〕前缀广播验收请求（集成基线 commit+变更要点+影响面·AGENTS.md §7 异步验收）。")
            return 0
        if 信息 is None:
            print(f"  [竞争] push 被拒（他人刚集成）——第 {尝试}/{最大重试} 次 rebase 重试")
            continue
        return 失败(信息)
    return 失败(f"连续 {最大重试} 次集成竞争失败——稍后再试或与对方协调（看板通告段「集成中」标注）。")


def 批流程(参数: argparse.Namespace) -> int:
    """批量集成主路径（911 立·830 并行化）：报名→立即组链入池（不等别的批）→门禁→
    云端预验（池空闲即被认领·非 --wait 入池即返）→push（CAS 重组+L1+预验绿复用）→销账。"""

    平台 = 探测平台()
    当前分支 = 输出(["git", "branch", "--show-current"])
    print(f"== 合并队列（协议 v3·bors 批量集成）｜平台={平台}｜分支={当前分支}"
          f"｜模式={'演练' if 参数.dry_run else '集成'} ==")
    问题 = 前置自检(参数.remote)
    if 问题:
        return 失败(问题)
    print("[1] 前置自检通过（分支已推远端·工作树干净）")

    # ── ② 报名（幂等：已在排队不重复；冲突出批/集成中失联自愈则刷新）─────────────────
    运行(["git", "fetch", 参数.remote])
    基线 = 输出(["git", "rev-parse", f"{参数.remote}/{集成分支}"])
    写集 = 改动文件清单(基线)
    if not 写集:
        return 失败("分支相对 develop 无任何改动（空集成）。")
    摘要 = f"{len(写集)} 文件" + ("·触全量面" if 分类写集(写集) else "·纯文档/脚本")
    新行 = 报名行(当前分支, 基线[:8], 摘要)

    if 参数.dry_run:
        print(f"  [演练] 跳过看板报名直推（内存行代用）：{当前分支} @ {新行['时刻']} {摘要}")
    else:
        # 920：服务端报名（权威·实时）——v5（1016）看板直推废档后为唯一报名通道：
        #   不可达=失败退出（三选一指引），无「看板单写回退」面。976 等价化简：原直推
        #   壳恒返回失败信息经 `问题 and not 服务成` 参与判定——直取 join 结果。
        服务成 = 队列写("join", {"分支": 当前分支, "基线": 新行["基线"], "写集摘要": 摘要})
        if not 服务成:
            return 失败("TX_01 队列服务不可达（join 失败）——三选一：①稍后重试本命令"
                        "（服务恢复即批路径）②python scripts/integrate.py --solo"
                        "（立即单机集成·全门禁）③python scripts/integrate.py --solo-if-unreachable"
                        "（探测降级：可达走批/不可达自动 solo）")
        # 1020（#180·方案乙）：报名后自检——join ok≠态正确（旧版幂等吞重报：
        #   已踢出态停留→排队们无我→批主选不出→全员死等 1017 实录）。核「自己行=
        #   排队」，异常态 touch 重置再核，仍异常报错不进死等。
        自检 = next((行 for 行 in (队列读(参数.remote) or [])
                     if 行["分支"] == 当前分支), None)
        if 自检 is None or 自检["状态"] != "排队":
            队列写("touch", {"分支": 当前分支})
            自检 = next((行 for 行 in (队列读(参数.remote) or [])
                         if 行["分支"] == 当前分支), None)
            if 自检 is None or 自检["状态"] != "排队":
                return 失败(f"报名自检失败：队列行态={自检['状态'] if 自检 else '无行'}"
                            "（touch 后仍未排队）——检查服务端 join 语义。")
        print(f"[2] 报名入队：{当前分支} @ {新行['时刻']}（{摘要}）｜服务端✓")

    # ── ③ 并行模型（830·2026-10-15 用户裁决）：不再等待任何批主/不看池忙闲──────────
    #   报名后立即组链入池——池内排队由 runner 空闲认领天然实现（一个任务一个实例·
    #   谁先空闲谁跑）；develop 推进由 push 阶段 CAS 处理（重组+L1+预验绿复用·不重跑
    #   云端全量）。原「等待批主/拼车收拢/失联自动接管」整段退役（见文件头 830 说明）。
    print("  [并行] 立即组链入池（不等其他批·池有空闲实例即被认领·830 并行模型）")

    # ── ④ 组链：标记集成中→组链（单人并行）→门禁→云端预验→push（CAS 重试）─────────
    if not 参数.dry_run:
        # 920·中间态只进服务端（不写 develop=治起批/排队乒乓提交）
        if 队列写("update", {"分支": 当前分支, "状态": "集成中"}):
            print("  [队列服务] 已标集成中（服务端·零 git 提交——920 治乒乓）")
        else:
            print("  [警告] 队列服务不可达——状态未标（不影响流程·重跑自愈）")
    # 562 集成心跳：集成期间后台线程定期刷队列行更新时刻=活会话证明——服务端超时回收
    #   以心跳新鲜度为据（治 444：长门禁无行更新被误判失联）。finally 停止；不可达静默。
    _停心跳 = threading.Event()
    if not 参数.dry_run:
        def _心跳循环():
            while not _停心跳.wait(心跳间隔秒):
                队列心跳(当前分支)
        threading.Thread(target=_心跳循环, daemon=True, name="集成心跳").start()
    print(f"[3] 组链（单人并行·任务={当前分支}）")

    原分支 = 当前分支
    批分支 = "batch/" + 当前分支.replace("任务/", "", 1)
    成功 = False
    已入池待续 = False    # 830：非 --wait 入池后早退标志=已入池哨兵 的批流程侧
                          #   （不 push·不触发失败恢复；三态协议见 已入池哨兵 定义处）
    链顶 = None
    踢出们: list[str] = []
    已验基准: str | None = None
    云端首批已绿 = False  # 830：会话内云端预验首批绿——CAS 重组轮免重跑（832 复用绿面）
    # 830：成员恒=自己（一个任务一条链·单人并行——无批主/无拼车/无成员候选推导）
    实际成员 = [{"分支": 当前分支, "摘要": 摘要, "时刻": 新行["时刻"]}]
    try:
        for 尝试 in range(1, 最大重试 + 1):
            运行(["git", "fetch", 参数.remote])
            最新 = 输出(["git", "rev-parse", f"{参数.remote}/{集成分支}"])
            建链 = 运行(["git", "checkout", "-B", 批分支, 最新])
            if 建链.returncode != 0:
                return 失败("批分支创建失败（工作树状态异常）——请检查 git 状态。")
            链顶, 新踢出, 错误, 伞批销账 = 组链(实际成员, 最新, 参数)
            踢出们 = 新踢出
            if 参数.dry_run:
                踢出们 = [对 for 对 in 新踢出 if 对[0] != 当前分支]
            踢出名们 = [对[0] for 对 in 踢出们]
            if 错误:
                return 失败(错误)
            实际成员 = [行 for 行 in 实际成员 if 行["分支"] not in 踢出名们]
            批文件们 = 批写集(实际成员, 最新)
            须全量 = 分类写集(批文件们)
            print(f"[4] 批门禁（成员 {len(实际成员)}·写集 {len(批文件们)} 文件"
                  f"{'·全量' if 须全量 else '·快速'}）"
                  + (f"（踢出：{'、'.join(f'{b}[{因}]' for b, 因 in 踢出们)}）" if 踢出们 else ""))
            if 已验基准 is None or 已验基准 == 最新:
                增量须全量 = 须全量 if 已验基准 is None else False
            else:
                增量须全量 = 分类写集(三点间改动(已验基准, 最新))   # 两 develop tip·三点=两点等价
            问题 = 快速门禁(批文件们)
            if 问题:
                return 失败(问题)
            # 6.5 批形态（--verified-same-content·用户令「直接集成」条款）：单飞批（成员=自己）
            #   且链顶树与本机已验树逐字节一致 → 免重跑全量（红线：diff 非空/多成员批=必须全量）
            等价免验 = False
            if (须全量 and 参数.verified_same_content and 已验基准 is None
                    and len(实际成员) == 1):
                # 6.5 口径：差异仅限「文档面」（看板集成队列行=起批/报名标记·本机制的固有噪音）——
                #   判定限定写集文件内逐字节一致（排除看板队列行差异·与 6.5 ①「或差异仅文档面」对齐）
                差异 = 输出(["git", "diff", "--stat", 链顶, 实际成员[0]["分支"],
                            "--"] + [f for f in 批文件们 if f != 看板文件名])
                if 差异 == "":
                    等价免验 = True
                    print("[5] 全量门禁复用（6.5 批形态：写集文件树==本机已验树·单飞批·"
                          "差异仅看板队列行·--verified-same-content 显式声明——"
                          "提交/通告须披露「直推·复用已验门禁」）")
                else:
                    print("  [6.5] 链顶树与本分支 HEAD 在写集面存在实质差异——等价前提不成立，走全量。")
            # 830（2026-10-15 用户裁决）：重组轮（已验基准非空）只保留 L1 自检（fast_lane=
            #   gate_quick·分钟级·不占池）——不再重跑云端全量（预验绿复用/内容键判据）；
            #   真全量模式（--no-fast-lane）沿用增量判定避免重复跑全量。
            重组轮 = 已验基准 is not None
            if 须全量 and not 等价免验 and (参数.fast_lane or 增量须全量):
                if 重组轮:
                    print("[5] 重组后 L1 自检（830：链路已重新接到新 develop——分钟级编译+单测·"
                          "不重跑云端全量·预验绿按内容键复用）")
                else:
                    print(f"[5] 全量门禁（平台={平台}·跑在合并结果上）")
                问题 = 全量门禁(平台, 参数.allow_known_red, 参数.fast_lane)
                if 问题:
                    print("  [归因辅助] 批成员×写集（对照失败日志定位破坏者后 --drop 踢出重验）：")
                    for 行 in 实际成员:
                        成员写集 = 三点间改动(最新, 行["分支"])   # 分支自身改动（1008 虚大修复）
                        print(f"    - {行['分支']}：{成员写集}")
                    return 失败(问题 + "（批门禁红——修复后重跑本命令续推）")
                if 参数.cloud_gate and not 云端首批已绿:
                    print(f"[5.5] 云端预验门禁（1008·链顶 {链顶[:8]}·绿才 push develop）")
                    问题 = 云端预验门禁(链顶, 参数, 源分支=当前分支, 等待=参数.wait)
                    if 问题 == 已入池哨兵:
                        已入池待续 = True
                        break   # 830 非 --wait：入池成功·不 push（重跑续推）
                    if 问题:
                        return 失败(问题 + "（批预验红——修复后重跑续推·已入池结果按内容键复用）")
                    云端首批已绿 = True
            elif 须全量 and not 等价免验:
                print("[5] 全量门禁跳过（develop 增量纯文档·已验部分仍有效）")
            已验基准 = 最新
            push_命令 = ["git", "push", "--dry-run" if 参数.dry_run else None,
                         参数.remote, f"HEAD:{集成分支}"]
            push_命令 = [c for c in push_命令 if c]
            print(f"[6] push {参数.remote} {集成分支}（ff-only·CAS）")
            结果 = 运行(push_命令)
            if 结果.returncode == 0:
                成功 = True
                break
            print(f"  [竞争] push 被拒（他批/他机刚推）——第 {尝试}/{最大重试} 次重组链"
                  "（重组+L1·不重跑云端全量·830）")
        if not 成功 and not 已入池待续:
            return 失败(f"连续 {最大重试} 次集成竞争失败——稍后重跑本命令续推（报名仍在·幂等）。")
    finally:
        _停心跳.set()   # 562：批流程结束（成功/失败）即停心跳——行交回失败恢复/销账路径
        运行(["git", "checkout", 原分支])
        运行(["git", "branch", "-D", 批分支])
        # 830 失败显形：队列行回「排队」——失败原因已随失败信息打印（可见），重跑本命令
        #   续推（已入池的预验结果按内容键复用·不重跑）。非 --wait 入池早退（已入池待续）
        #   =行保持「集成中」不动（看板由预验信号驱动显示「预检排队/预检中」）。
        if not 成功 and not 已入池待续 and not 参数.dry_run:
            if 队列写("touch", {"分支": 当前分支}):    # 920·中间态服务端（零 git 提交）
                print("  [队列服务] 失败恢复：队列行回「排队」——修复后重跑本命令续推"
                      "（已入池的预验结果按内容键复用·不重跑）")
            else:
                print("  [警告] 队列服务不可达——失败恢复未生效（服务恢复后重跑本命令即自愈）")

    if 已入池待续:
        # 830 非 --wait：入池成功即返回（池空闲实例会被 runner 认领跑全量·一个任务一个实例）
        print(f"\n[已入池] {当前分支} 的预验已在池中排队——池有空闲实例即被认领跑全量。"
              "绿后重跑本命令续推 develop（或加 --wait 挂机等结果）。")
        return 0

    # ── ⑤ 销账：删成员远端分支→清队列行→镜像补推→验收提示 ─────────────────────────
    新tip = 链顶
    if not 参数.dry_run:
        for 行 in 实际成员:
            清理 = 运行(["git", "push", 参数.remote, "--delete", 行["分支"]])
            if 清理.returncode == 0:
                print(f"  [清理] 远程任务分支 {行['分支']} 已删（内容已随批入 {集成分支}·AGENTS.md §7）。")
            else:
                print(f"  [警告] 远程任务分支 {行['分支']} 删除失败——稍后 branch_cleanup.py 兜底。")
            # 308h：意图生命周期联动——任务收口=挂在该任务上的会话意图自动注销
            # （治「会话收工不 release」残留·失败仅警告不阻断集成有效性）
            号0 = 行["分支"].split("/", 1)[-1]
            if 板调用("/api/intent_release", {"在做": 号0}):
                print(f"  [清理] 看板意图（在做#{号0}）已随收口注销。")
            else:
                print(f"  [提示] 看板服务不可达——意图#{号0} 等失联自动清（降级不阻断）。")
            # 316：同步清本机该会话登记缓存——否则收尾 021 播报跑 task_board --ready
            # 触发路过续约读缓存重发=把刚注销的意图复活（实测 313 收口后心跳复活实录）
            for _c in Path.home().joinpath(".cache").glob(
                    f"cn_board_intent_last_*{号0}*.json"):
                try:
                    _c.unlink()
                except OSError:
                    pass

        # 920：服务端销账（权威·实时）——v5（1016）看板销账直推废除后为唯一销账面
        #   （976：原 销账变换 闭包+直推调用随废档删除；021 总账自动收口含解锁播报）
        队列写("clear", {"分支们": [行["分支"] for 行 in 实际成员]})
        for 分支, _原因 in 踢出们:
            队列写("update", {"分支": 分支, "状态": "已踢出"})
        # 392：伞批登记号并集（组链收集）随批销账——成员号不入分支名的漏销盲区
        # 只取实际入链成员的登记号（被踢出者未进 develop·销其登记号=错销）
        实际分支们 = {行["分支"] for 行 in 实际成员}
        伞批并集 = sorted({号 for 分支, 们 in 伞批销账.items()
                          if 分支 in 实际分支们 for 号 in 们})
        总账收口([行["分支"] for 行 in 实际成员], 新tip, 额外号们=伞批并集)
        镜像 = 运行(["git", "push", 镜像远程, f"{集成分支}"])
        if 镜像.returncode != 0:
            print("  [警告] github 镜像补推失败——按惯例下次提交补推（不影响集成有效性）。")
    print(f"\n[批集成成功] 成员 {len(实际成员)}：")
    for 行 in 实际成员:
        print(f"    - {行['分支']}（{行['摘要']}）")
    if 踢出们:
        print("  踢出（出批·解完冲突/修复后重新报名）："
              + "、".join(f"{b}[{'归因' if 因 == '归因' else '冲突'}]" for b, 因 in 踢出们))
    print(f"  develop 新基线：{新tip[:8]}"
          "——请批主在提交信息以〔通告〕前缀广播**验收请求**（基线+成员清单+影响面·AGENTS.md §7），"
          "各成员看板本机小节照常清行销账。")
    return 0


def 主流程() -> int:
    解析器 = argparse.ArgumentParser(
        description="三机并行协同协议 v3·合并队列（AGENTS.md §5·bors 批量集成·911）")
    解析器.add_argument("--dry-run", action="store_true", help="演练模式：不真推 develop")
    解析器.add_argument("--join", action="store_true",
                        help="仅报名进看板集成队列（不组批——报名后可先做别的·稍后重跑组批）")
    解析器.add_argument("--status", action="store_true", help="查看当前集成队列（只读）")
    解析器.add_argument("--no-fast-lane", action="store_true",
                        help="显式回退本机全量门禁（941·默认开后想本机全量验证时用）")
    解析器.add_argument("--fast-lane", action="store_true",
                        help="快速通道（保持兼容·941 起默认开）：本机仅 L1（gate_quick·分钟级）"
                             "即集成——全量由 TX_02 云端对 develop 事后自动跑+红灯治理兜底"
                             "（Chromium CQ/rustc bors 同构）")
    解析器.add_argument("--cloud-gate", action="store_true",
                        help="云端预验门禁显式开（1008·方案甲 test-then-commit·1015 起默认开）："
                             "触及全量面的集成把链顶推 TX_02 预验分支跑 linux 全量，"
                             "绿才 push develop——红根本进不来。默认开时此参数=显式确认（幂等）")
    解析器.add_argument("--no-cloud-gate", action="store_true",
                        help="[770 已硬禁] 传入即拒绝（2026-10-14 用户令：只有预验绿才能并入 develop）")
    解析器.add_argument("--solo", action="store_true",
                        help="旧单分支路径逃生门（不经队列直集成——队列机制异常时用）")
    解析器.add_argument("--solo-if-unreachable", action="store_true",
                        help="240：报名前探测 TX_01——不可达自动转 solo（可达=正常批·默认关保批串行化）")
    解析器.add_argument("--selftest", action="store_true", help="正反例自测（纯函数面·CI 式）")
    解析器.add_argument("--wait", action="store_true",
                        help="挂机等待池预验结果并自动 push（830：池排队久时推荐；"
                             "非 --wait=组链入池即返回·重跑本命令续推）")
    # 830（2026-10-15 用户裁决）：--gather/--seal-now/--takeover 随并行化退役——
    #   接受即忽略（不炸他机旧命令行习惯·同 --try-build-done 先例）。
    解析器.add_argument("--seal-now", action="store_true", help=argparse.SUPPRESS)
    解析器.add_argument("--gather", action="store_true", help=argparse.SUPPRESS)
    解析器.add_argument("--takeover", action="store_true", help=argparse.SUPPRESS)
    解析器.add_argument("--drop", action="append", default=[], metavar="分支",
                        help="门禁红归因后踢出指定成员重组链重验（可多次）")
    解析器.add_argument("--remote", default=主远程, metavar="远程",
                        help=f"远程名（默认 {主远程}·自测模拟远端用）")
    解析器.add_argument("--verified-same-content", action="store_true",
                        help="批形态（--batch·AGENTS §5）：声明本机已对当前分支树完整跑过全量门禁且全绿——"
                             "单飞批+链顶树与本分支 diff 为空时免重跑全量（快速门禁恒跑；"
                             "提交/看板通告须披露「直推·复用已验门禁」；diff 非空自动回落全量）")
    解析器.add_argument("--allow-known-red", action="append", default=[],
                        metavar="用例名",
                        help="已知红显式点名（564-a 过渡机制·fdef8ae9 P1 构建确定性根治前）："
                             "串行复验红若全部命中点名清单=三平台已定性已知红披露放行；"
                             "未点名红仍硬拦。使用责任=发起机（点名依据+集成广播披露不实=违规可 revert）。"
                             "fdef8ae9 P1 根治后本参数应移除。可多次传入点名多个用例。")
    解析器.add_argument("--allow-pool-red", action="append", default=[],
                        metavar="用例名",
                        help="216 批预验已知红点名：云端预验（池/ssh 路径）红若**全部**命中点名清单="
                             "披露放行（使用责任=发起机：点名依据+集成广播披露·不实可 revert）；"
                             "未点名红仍硬拦。场景=环境敏感用例（606 判零防线 WSL 池红 vs TX_02 门禁绿）"
                             "锁死集成链。可多次传入点名多个用例。")
    解析器.add_argument("--pool-limit", action="append", default=[],
                        metavar="平台",
                        help="337 预验池认领限制（平台白名单）：任务只被白名单内平台 runner 认领"
                             "（win-x64/linux-x86_64/linux-arm64·runner 运行时自报）——定向规避已知"
                             "环境红平台（如 335 单位机 arm64 老工具链缺库自动链接支持）。可多次传入"
                             "或逗号分隔；缺省不限=291 混合随机池默认不变。白名单不含 linux-x86_64 时"
                             "池空不降级 TX_02 直发（守限制等待·总超时兜底）。")
    # v2 遗留旗标（581-a 废除·接受即忽略——v3 下「本机门禁绿即集成」本就是默认行为，
    # 保留解析仅为不炸他机旧命令行习惯）
    解析器.add_argument("--try-build-done", action="store_true",
                        help=argparse.SUPPRESS)
    解析器.add_argument("--win-verified", action="store_true",
                        help=argparse.SUPPRESS)
    参数 = 解析器.parse_args()
    # 337：--pool-limit 支持逗号分隔（"win-x64,linux-x86_64"）与多次传入两形态归一
    参数.pool_limit = [p.strip() for v in 参数.pool_limit for p in v.split(",") if p.strip()]
    # 941（用户裁决 2026-10-02）：快速通道默认开——--no-fast-lane 显式回退本机全量
    参数.fast_lane = not 参数.no_fast_lane
    # 1008（用户裁决 2026-10-03·方案甲）：云端预验门禁——1015 起**默认开**（develop 回绿
    #   达成·1013 终验）；显式旗优先于默认（--no-cloud-gate 逃生门）。
    参数.cloud_gate = (not 参数.no_cloud_gate) and (云端预验默认开 or 参数.cloud_gate)
    if 参数.no_cloud_gate:
        # 770（2026-10-14 用户令）：逃生门硬禁——「只有预验绿才能并入 develop」铁律化。
        #   背景案=752（10-13 22:33 后零预验绿记录仍 67 提交进 develop 的验证真空）。
        return 失败("[770] --no-cloud-gate 逃生门已硬禁（2026-10-14 用户令：只有预验绿才能并入 develop）"
                    "——集成必须过云端预验；TX_01/池全停时等待恢复（每 2 小时 CI 巡检自动治理），"
                    "或先把任务分支留档待集成。")
    if 参数.try_build_done or 参数.win_verified:
        print("[v3] --try-build-done/--win-verified 已随协议 v3 废除（接受即忽略）——"
              "本机门禁绿即集成，跨平台由集成后异步验收保障（AGENTS.md §7）。")
    if 参数.gather or 参数.seal_now or 参数.takeover:
        print("[830] --gather/--seal-now/--takeover 已随并行化退役（接受即忽略）——"
              "新模型每个任务独立组链入池·无批主无拼车·失败重跑本命令即续推（AGENTS.md §5）。")

    if 参数.selftest:
        import integrate_selftest   # 637：自测面拆出·函数内延迟导入（彼时本模块已加载完毕·无循环）
        return integrate_selftest.自测()
    if 参数.status:
        队列 = 队列读(参数.remote)                      # 920：服务端优先（双源合并）
        if 队列 is None:
            队列 = 解析队列(远端看板文本(参数.remote))
        if not 队列:
            print("[集成队列] 空（无人排队——bors 批组建·AGENTS.md §5）")
        else:
            print("[集成队列]（bors 批组建·AGENTS.md §5）")
            for 行 in sorted(队列, key=lambda r: r["时刻"]):
                print(f"  {行['状态']}｜{行['时刻']}｜{行['分支']}｜{行['摘要']}（基线 {行['基线']}）")
        打印验证池状态()   # 825 残余 B：并行度观测（池忙闲+排队/执行中行+排队原因）
        return 0

    平台 = 探测平台()
    if 参数.solo_if_unreachable and not 参数.solo:
        if 队列读(参数.remote) is None:
            print("[240] TX_01 队列不可达——自动降级 solo 全门禁路径（--solo-if-unreachable）")
            print("      （恢复后建议回归批路径：批=立即组链入池·池并行预验·一个任务一个实例）")
            参数.solo = True
        else:
            print("[240] TX_01 可达——走正常批路径（--solo-if-unreachable 未触发）")
    if 参数.solo:
        return solo流程(平台, 参数)
    if 参数.join:
        return 批流程_join_only(参数)
    return 批流程(参数)


def 批流程_join_only(参数: argparse.Namespace) -> int:
    """--join：仅报名（报名后立即返回 0——组批留给稍后重跑）。

    v5（1016）披露·976 核实：看板直推废档后本路径的服务端报名面未随迁移=恒失败
    （缺陷在案·修复或退役待独立立项）。本函数显式保持恒失败行为（exit 1）——
    文案沿用原直推壳输出（行为等价），修复立项前 --join 请改走默认路径或 --solo。
    """
    问题 = 前置自检(参数.remote)
    if 问题:
        return 失败(问题)
    运行(["git", "fetch", 参数.remote])
    基线 = 输出(["git", "rev-parse", f"{参数.remote}/{集成分支}"])
    if not 改动文件清单(基线):
        return 失败("分支相对 develop 无任何改动（空集成）。")
    # 976：原 报名变换+直推看板提交（恒失败壳）显式化——保持恒失败与原文案（行为等价）。
    return 失败("v5 看板已废档——跳过直推（服务端队列为主·稍后重试或 --solo）")


if __name__ == "__main__":
    os.environ["CN_INTEGRATE"] = "1"   # 238：本进程一切 push 均为集成合法面（githooks pre-push 白名单）
    sys.exit(主流程())
