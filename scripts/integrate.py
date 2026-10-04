#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""合并队列脚本（AGENTS.md §5/§7——develop 唯一入关口）。

职责：把当前任务分支安全集成回 develop——
  前置自检 → fetch → rebase（冲突自己解·禁止 develop 手解）→ 写集分类（仅门禁深度）
  → 门禁（跑在合并结果上：快速门禁恒跑；写集触及 src/tests 时全量门禁）
  → push gitcode develop（ff-only·CAS 竞争输家自动 rebase 重试 ≤3 次）→ 补推 github 镜像。
v3（581-a·2026-09-21 用户裁决）：**无任何集成前置的他机回签/批准面**——跨平台正确性由
集成后异步验收+修复义务保障（AGENTS.md §7·异步验收不需他机回签）。
业界对照=rustc/bors 合并队列、GitHub merge queue 的脚本化实现。

批量集成（911 立·用户批准方案甲·bors 式）：根治 872 式「竞争→rebase→全量重跑」循环——
**批=原子集成单位**（一次全量门禁验整批 N 分支·竞争作废归零）。形态=拼车：
第一个报名者=批主（先到先得·341-a 同哲学），批主报名后固定收拢期 10 分钟（攒满 5 个提前
封批·队列仅自己时立即单飞不等），按报名序 cherry-pick 叠链（develop→成员1→成员2→…），
对链顶跑一次门禁（天然跑在整批合并结果上），绿则 ff push 整批进 develop、删全部成员远端
分支、清队列行；cherry-pick 冲突=踢出后到者（批不因成员卡死·被踢者解完冲突报下批）；门禁红=
打印成员×写集归因辅助，--drop <分支> 踢出重组链重验；批主失联 30 分钟后成员可 --takeover
接管组批。他机看板写入全部走 plumbing 直推（临时 index+commit-tree·零 checkout 不扰动工作树）。

用法（在任务分支上运行）：
  python3 scripts/integrate.py                    # 批量集成：报名→等待成为批主→组批→门禁→整批 push
  python3 scripts/integrate.py --join             # 仅报名进看板集成队列（不组批·报名后可先做别的）
  python3 scripts/integrate.py --status           # 查看当前集成队列
  python3 scripts/integrate.py --solo             # 旧单分支路径（逃生门：队列机制异常时不经队列直集成）
  python3 scripts/integrate.py --seal-now         # 批主用：跳过收拢期立即封批（多人排队时）
  python3 scripts/integrate.py --takeover         # 批主失联 ≥30 分钟时成员接管组批
  python3 scripts/integrate.py --drop 任务/家机-895-x  # 门禁红归因后踢出指定成员重组链重验
  python3 scripts/integrate.py --dry-run          # 演练：全步骤、push 以 --dry-run 代替（不真推）
  python3 scripts/integrate.py --selftest         # 正反例自测（纯函数面·CI 式）

退出码：0=集成成功（或演练通过/自测通过）；1=失败（原因见输出）；2=等待态（非批主/
收拢期未满·配合 --wait 或稍后重跑）。
"""
from __future__ import annotations

import argparse
import json
import os
import platform
import re
import subprocess
import sys
import tempfile
from datetime import datetime, timedelta
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
主远程 = "gitcode"
镜像远程 = "github"
集成分支 = "develop"
分支名模式 = re.compile(r"^任务/(家机|单位机|深度机)-\d+-\S+$")
最大重试 = 3

# ── 批量集成参数（911·可调常量——跑一周看实态再调·AGENTS.md §5）──────────────────
批收拢期分钟 = 10    # 批主报名后的收拢窗口（后到者在此窗口内报名即入批）
批上限 = 5           # 单批成员上限（含批主·红批二分成本随批增大）
批等待轮询秒 = 60    # --wait 等待批主的轮询间隔
批等待上限分钟 = 120  # --wait 等待总上限（防挂死）
批主失联接管分钟 = 30  # 批主报名后无进展超此时长，成员可 --takeover 接管

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
# 保留为事后兜底：预验跳过面（纯文档轮）与 --no-cloud-gate 逃生门走云端每推送自动验。
# **默认开**（1015 翻开·2026-10-04）：1008 立的启用条件「develop 回绿」已达成（1013 批八
# 终验云端 c4c28311 轮绿·linux 红=0）——触及全量面的集成默认走 test-then-commit。
# 红轮实测数据（TX_02 ci-logs 107 轮）：绿轮 13~16min·红轮带串行复验 33~40min——等待期
云端预验默认开 = True
# 可做六件套文档；TX_02 异常时 --no-cloud-gate 逃生门回退 941 事后兜底。
预验远程分支前缀 = "ci/预验-"
预验轮询间隔秒 = 60
预验总超时分钟 = 75    # 含锁等待（TX_02 正跑 develop 轮时预验排队）+全量跑轮·留余量


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
    if not 分支名模式.match(当前分支):
        return f"分支名「{当前分支}」不合法——须形如 任务/<机>-<轮次>-<标识>（AGENTS.md §7）。"
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


def 两点间改动(起点: str, 终点: str) -> list[str]:
    """两点间 commit 范围的改动文件清单——写集分类与冲突标记扫描的输入。"""
    # core.quotepath=false：中文路径默认被转义为带前导双引号的八进制形态（"tests/e2e/...），
    # 令 分类写集 的 startswith 匹配失效 → 触及中文路径写集被静默降级为快速门禁（586-a 实测：
    # 93 个 tests 文件全中文目录名，全量门禁被跳过）；冲突标记检查的 仓库根/路径 拼装同样受害。
    文本 = 输出(["git", "-c", "core.quotepath=false", "diff", "--name-only", f"{起点}..{终点}"])
    return [行.strip().strip('"') for 行 in 文本.splitlines() if 行.strip()]


def 改动文件清单(基准: str) -> list[str]:
    """基准（develop tip）相对 HEAD 的改动文件清单（=两点间改动 的便捷封装）。"""
    return 两点间改动(基准, "HEAD")


def 分类写集(文件们: list[str]) -> bool:
    """返回 须全量门禁与否。v3：写集分类仅区分门禁深度，不存在集成前置的他机批准面（AGENTS §7 异步验收）。"""
    return any(文件.startswith(全量门禁触发模式) for 文件 in 文件们)


def 临时用例检查(文件们: list[str]) -> str | None:
    """纪律 2（1008·用户裁决 2026-10-03）：临时/探针用例禁入 develop。

    tests/e2e/ 下用例目录名（路径第三段）含 tmp（大小写不敏感）即拦——1005 清 7 例存量
    （990/991/993/995/998/1002/1003tmp）后的增量防线；临时探针留在任务分支用后删，
    转正须按用例目录约定命名（<纯数字编号>_<名称>）。
    """
    违规 = sorted({f.split("/")[2] for f in 文件们
                   if f.startswith("tests/e2e/") and len(f.split("/")) > 2
                   and "tmp" in f.split("/")[2].lower()})
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


def 总账收口021(分支们: list, 新tip: str, remote: str) -> None:
    """v5（1016）：集成成功后 021 任务总账自动收口——成员分支匹配 🏃 行改 ✅+sha10
    （commit+push develop·CAS 失败重试一次·再败警告由 task_board --check 下轮兜底抓漏），
    随后跑 task_board --ready 播报本轮解锁（就绪队列前 3）。取代 v4 看板销账直推。"""
    路径 = 仓库根 / "plans" / "021-任务进度观察表.md"
    if not 路径.exists() or not 分支们:
        return
    # 1017 修复（Python 3.12 兼容）：pathlib.Path.read_text 的 newline 参数为
    #   3.13 新增——本机 3.12 直炸 TypeError（集成 push 已成·收口步崩=021 漏销账）。
    #   改 open(..., newline="") 等价语义（保 CRLF 原样·3.8+ 全版本可用）。
    with open(路径, "r", encoding="utf-8", newline="") as f:
        原文 = f.read()
    行们 = 原文.splitlines(keepends=True)
    改动 = []
    for i, 行 in enumerate(行们):
        st = 行.strip()
        if not st.startswith("|") or "🏃" not in st:
            continue
        for 分支 in 分支们:
            if 分支 in 行:
                段 = [c.strip() for c in st.strip("|").split("|")]
                if len(段) >= 3:
                    段[2] = "✅ " + 新tip[:10]
                    前缀 = 行[:len(行) - len(行.lstrip())]
                    行尾 = "\r\n" if 行.endswith("\r\n") else "\n"
                    行们[i] = 前缀 + "| " + " | ".join(段) + " |" + 行尾
                    改动.append((分支, 段[0]))
                break
    if not 改动:
        return
    路径.write_text("".join(行们), encoding="utf-8", newline="")
    分支0, 号0 = 改动[0]
    ok = False
    for _ in range(2):
        运行(["git", "add", str(路径)])
        运行(["git", "commit", "-m", "021 总账收口：任务#" + 号0 + " ✅ " + 新tip[:10] + "（集成自动·v5）"])
        推 = 运行(["git", "push", remote, "HEAD:develop"])
        if 推.returncode == 0:
            ok = True
            break
        运行(["git", "pull", "--rebase", remote, "develop"])
    if ok:
        print("  [021] 任务总账自动收口：" + str(改动) + "（✅ " + 新tip[:10] + "）")
        r = subprocess.run([sys.executable, "scripts/task_board.py", "--ready"],
                           capture_output=True, text=True, cwd=仓库根)
        就绪 = [l for l in (r.stdout or "").splitlines() if l.strip().startswith("#")]
        if 就绪:
            print("  [021] 本轮解锁（就绪队列前 3）：")
            for l in 就绪[:3]:
                print("      " + l.strip())
    else:
        print("  [警告] 021 总账收口 push 竞争失败——请手动改 ✅ 并 push"
              "（漏收口会被 task_board --check 抓住）。")


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
                     "--cn", str(仓库根 / "target/Debug/cn.exe"), "--jobs", "1"])
        if 串行.returncode != 0:
            # 591-a（T100·170 集成实测）：win 分支补接 --allow-known-red 点名消费
            #   （与锁内版 190-215 段同逻辑——原 win ci.ps1 路径漏接·参数已有实现
            #   未达此处）：串行红全部命中点名=已定性已知红披露放行；任一未点名仍拦。
            失败们 = []
            for 原行 in (串行.stdout or "").splitlines():
                ts = 原行.strip()
                if ts.startswith("✗"):
                    失败们.append(ts[1:].split(":")[0].strip())
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
                 " --jobs 8"])
    if e2e.returncode == 0:
        return None
    print("  [复验] E2E 并行未全绿——串行复验区分真红与并行互踩（447-a 机制·gate_lock 锁内）")
    串行 = 运行捕获(["bash", "-c",
                  "exec " + sys.executable + " " +
                  str(仓库根 / "scripts/gate_lock.py") + " run -- " + sys.executable +
                  " tests/e2e/run_e2e.py --target " + 目标 + " --cn " + str(cn路径) +
                  " --jobs 1"])
    if 串行.returncode != 0:
        # 564-a（过渡机制·fdef8ae9 P1「v2p 构建确定性缺失」根治前）：--allow-known-red
        #   显式点名机制——串行红若【全部】命中点名清单=「三平台已定性已知红」披露放行；
        #   任一未点名红仍硬拦（机制不弱化）。使用责任=发起机：点名依据+集成广播披露
        #   不实=违规可 revert。fdef8ae9 P1 根治后本分支应移除（移交条款）。
        失败们 = []
        for 原行 in (串行.stdout or "").splitlines():
            t = 原行.strip()
            if t.startswith("✗"):
                失败们.append(t[1:].split(":")[0].strip())
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
    """预验结果 JSON →（绿与否, 摘要）。纯函数（selftest 面）。"""
    try:
        d = json.loads(文本)
    except Exception:
        return False, "预验结果 JSON 解析失败：" + 文本[:120].replace("\n", " ")
    摘要 = "总秒=%s·日志=%s" % (d.get("总秒"), d.get("日志"))
    红步骤 = [k for k, s in (d.get("步骤") or {}).items() if s.get("rc", 0) != 0]
    if 红步骤:
        摘要 += "·红步骤=" + "、".join(红步骤)
    尾部 = (d.get("日志尾部") or [])[-6:]
    if 尾部:
        摘要 += "\n  尾部：" + " | ".join(x.strip() for x in 尾部 if x.strip())[:400]
    return bool(d.get("绿")), 摘要


def 云端预验门禁(链顶: str, 参数: argparse.Namespace) -> str | None:
    """链顶（=合并结果）推 TX_02 预验分支触发全量（linux-x86_64 面·后台 nohup），
    轮询 预验_<sha10>.json（daemon 红绿都写·1008 改造）——绿放行 push develop
    （daemon 已写 latest：push 后 develop 轮询轮按「已跑过」跳过=云端零重复算力）；
    红/超时/ssh 不可达=拦截（develop 未收到红提交）。返回 None=放行；str=拦截原因。

    降级不失效（028 §六哲学）：--no-cloud-gate 逃生门=显式回退 941 fast-lane 事后兜底。
    """
    import time as _time
    if 参数.dry_run:
        print("  [预验·演练] 跳过云端真跑（--dry-run）——正式集成对链顶真跑全量预验")
        return None
    配置 = 读CI配置()
    预验分支 = 预验远程分支前缀 + 链顶[:10]
    sha10 = 链顶[:10]
    推 = 运行(["git", "push", 参数.remote, f"{链顶}:refs/heads/{预验分支}"])
    if 推.returncode != 0:
        return f"预验分支推送失败（{预验分支}）——检查远程权限后重试。"
    try:
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
                    "逃生门：--no-cloud-gate 显式回退事后兜底（941）。"
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
                return (f"云端预验红（链顶 {sha10}·develop 未收到该提交）——{摘要}\n"
                        "  修复走任务分支（修复中间态禁推 develop·1008 纪律 1），绿后重新集成。")
            余分 = int((截止 - _time.time()) // 60)
            print(f"  [预验] 等待云端结果（余约 {余分} 分钟）……")
        return (f"云端预验超时（{预验总超时分钟} 分钟——TX_02 忙/挂死·028 §四处置）。"
                "逃生门：--no-cloud-gate 显式回退事后兜底（941）。")
    finally:
        清理 = 运行(["git", "push", 参数.remote, "--delete", 预验分支])
        if 清理.returncode != 0:
            print(f"  [警告] 预验分支 {预验分支} 删除失败——branch_cleanup.py 兜底。")


# ═══════════════════════════════════════════════════════════════════════════
# 批量集成（911·bors 式）——看板集成队列段：解析/变换（纯文本·selftest 面）+ plumbing 直推
# ═══════════════════════════════════════════════════════════════════════════

看板文件名 = "三机任务看板.md"
队列段标题 = "## 集成队列（bors 式批组建·911 立·AGENTS.md §8.2-B）"
队列状态们 = ("排队", "集成中", "冲突出批", "归因出批")
时刻格式 = "%m-%d %H:%M:%S"   # 秒级（911 演练实录：分钟级使同分钟内批主排队尾——组链序=报名序=冲突踢后到者，必须有秒）


def 现在时刻() -> str:
    return datetime.now().strftime(时刻格式)


def 解析时刻(文本: str) -> datetime | None:
    for 格式 in (时刻格式, "%m-%d %H:%M"):   # 兼容分钟级旧行
        try:
            return datetime.strptime(文本.strip(), 格式)
        except ValueError:
            continue
    return None


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


def 队列表头行() -> list[str]:
    return [队列段标题, "", "| 分支 | 基线 | 报名时刻 | 写集摘要 | 状态 |", "|---|---|---|---|---|"]


def 队列加行(看板文本: str, 行: dict) -> str:
    """把报名行加入队列段（幂等：同分支已在段内=不重复加·返回原文）。段不存在则建段（文件末尾）。"""
    if any(项["分支"] == 行["分支"] for 项 in 解析队列(看板文本)):
        return 看板文本
    新表行 = (f"| {行['分支']} | {行['基线']} | {行['时刻']} | {行['摘要']} | {行['状态']} |")
    行们 = 看板文本.splitlines()
    段起 = next((n for n, 行_ in enumerate(行们) if 行_.strip() == 队列段标题), None)
    if 段起 is None:
        块 = 队列表头行() + [新表行, ""]
        尾 = 看板文本.rstrip("\n")
        return 尾 + "\n\n" + "\n".join(块)
    # 段内找最后一个表行位置，其后插入
    插入位 = 段起 + 1
    for n in range(段起 + 1, len(行们)):
        if 行们[n].startswith("## "):
            break
        if 行们[n].startswith("|"):
            插入位 = n + 1
    行们.insert(插入位, 新表行)
    return "\n".join(行们) + ("\n" if 看板文本.endswith("\n") else "")


def 队列改状态(看板文本: str, 分支: str, 新状态: str) -> str:
    """把指定分支行的状态列改写（找不到=原文不动）。"""
    行们 = 看板文本.splitlines()
    for n, 行 in enumerate(行们):
        if not 行.startswith("|"):
            continue
        列们 = [列.strip() for 列 in 行.split("|")[1:-1]]
        if len(列们) == 5 and 列们[0] == 分支:
            列们[4] = 新状态
            行们[n] = "| " + " | ".join(列们) + " |"
    return "\n".join(行们) + ("\n" if 看板文本.endswith("\n") else "")


def 队列清行们(看板文本: str, 分支们: list[str]) -> str:
    """把指定分支们整行移除（集成销账/冲突出批离队）。段内行清空后保留段壳（三机共见结构稳定）。"""
    行们 = 看板文本.splitlines()
    出 = [行 for 行 in 行们
          if not (行.startswith("|")
                  and len([c for c in (行.split("|")[1:-1])]) == 5
                  and 行.split("|")[1].strip() in 分支们
                  and 行.split("|")[1].strip() != "分支")]
    return "\n".join(出) + ("\n" if 看板文本.endswith("\n") else "")


def 队列重报(看板文本: str, 分支: str, 新时刻: str) -> tuple[str, bool]:
    """出批（冲突/归因）/失联恢复后重新排队：该分支行存在则刷新时刻并置「排队」，返回（新文本, 是否变更）。"""
    行们 = 解析队列(看板文本)
    if not any(行["分支"] == 分支 for 行 in 行们):
        return 看板文本, False
    出 = 看板文本
    if any(行["分支"] == 分支 and 行["状态"] == "排队" for 行 in 行们):
        return 看板文本, False   # 已在排队——幂等不动（不刷新时刻·保序）
    出 = 队列改状态(出, 分支, "排队")
    出 = 队列改时刻(出, 分支, 新时刻)
    return 出, True


def 队列改时刻(看板文本: str, 分支: str, 新时刻: str) -> str:
    """把指定分支行的报名时刻列改写（找不到=原文不动）。"""
    行们 = 看板文本.splitlines()
    for n, 行 in enumerate(行们):
        if not 行.startswith("|"):
            continue
        列们 = [列.strip() for 列 in 行.split("|")[1:-1]]
        if len(列们) == 5 and 列们[0] == 分支:
            列们[2] = 新时刻
            行们[n] = "| " + " | ".join(列们) + " |"
    return "\n".join(行们) + ("\n" if 看板文本.endswith("\n") else "")


def 选批成员(排队行们: list[dict], 当前时刻: datetime,
             收拢期分钟: int = 批收拢期分钟, 上限: int = 批上限
             ) -> tuple[list[dict], str | None]:
    """纯函数：由「状态=排队」的行们选出本批成员（按报名时刻序）。

    返回（成员行们, 提示）：提示非 None=暂不可封批（收拢期未满·附剩余分钟），
    成员们此时仍返回窗口内预览（供打印）。单人批（队列仅一人）立即可封不等待。
    """
    序 = sorted(排队行们, key=lambda 行: 行["时刻"])
    if not 序:
        return [], "队列为空"
    批主 = 序[0]
    窗口截止 = 解析时刻(批主["时刻"])  # type: ignore[arg-type]
    if 窗口截止 is None:
        return [], f"批主报名时刻非法：「{批主['时刻']}」"
    窗口截止 = 窗口截止 + timedelta(minutes=收拢期分钟)
    成员 = []
    for 行 in 序:
        if len(成员) >= 上限:
            break
        时刻 = 解析时刻(行["时刻"])
        if 时刻 is None or 时刻 > 窗口截止:
            break
        成员.append(行)
    提示 = None
    if len(序) > 1 and 当前时刻 < 窗口截止:
        剩 = int((窗口截止 - 当前时刻).total_seconds() // 60) + 1
        提示 = (f"收拢期未满（还剩约 {剩} 分钟·窗口内已见 {len(序)} 人报名）——"
                f"等待后到者入批，或 --seal-now 立即封批")
    return 成员, 提示


def 直推看板提交(变换, 提交信息: str, 远程: str = 主远程) -> str | None:
    # v5（1016）：三机看板废档——本直推通道退役（原=服务端不可达时的降级路径）。
    # 服务端队列（TX_01:8300）为主：不可达时调用方打印警告继续（下轮重试同步），
    # 或 --solo 逃生门。保留函数签名防破存量调用点。
    return f"v5 看板已废档——跳过直推（服务端队列为主·稍后重试或 --solo）"
    """plumbing 直推：对远端 develop 的看板做一次文本变换并提交推送。

    临时 index（GIT_INDEX_FILE）+ commit-tree——零 checkout、不扰动工作树与当前分支
    （主树/多会话/任意分支上调用均安全）。变换返回 None=无需变更（直接返回）。
    push 被拒（他人刚推）→ fetch 重试 ≤3。返回失败信息（None=成功）。
    行尾保真（920·清偿 917 通告欠账）：读看板/写 blob 全程 bytes——text=True 的
    universal newlines 会把 CRLF 静默归一成 LF（行尾横跳源头·56f19797 只修了存量）。
    """
    for _ in range(3):
        运行(["git", "fetch", 远程])
        父 = 输出(["git", "rev-parse", f"{远程}/{集成分支}"])
        原文b = subprocess.run(["git", "show", f"{父}:{看板文件名}"],
                               cwd=仓库根, capture_output=True).stdout
        if 原文b == b"" and subprocess.run(["git", "show", f"{父}:{看板文件名}"],
                                           cwd=仓库根, capture_output=True).returncode != 0:
            return f"远端 develop 缺 {看板文件名}——窄通道直推前提不成立。"
        原文 = 原文b.decode("utf-8")          # bytes.decode 不动行尾——变换在 str 层保 \r\n
        新文 = 变换(原文)
        if 新文 is None:
            return None
        blob = subprocess.run(["git", "hash-object", "-w", "--stdin"],
                              input=新文.encode("utf-8"), cwd=仓库根,
                              capture_output=True).stdout.decode().strip()
        索引文件 = Path(tempfile.mkdtemp(prefix="idx-")) / "index"
        环境 = dict(os.environ, GIT_INDEX_FILE=str(索引文件))
        def 索引命令(参数们: list[str]) -> subprocess.CompletedProcess:
            return subprocess.run(["git", *参数们], cwd=仓库根, env=环境,
                                  capture_output=True, text=True)
        索引命令(["read-tree", 父])
        索引命令(["update-index", "--add", "--cacheinfo", f"100644,{blob},{看板文件名}"])
        树 = 索引命令(["write-tree"]).stdout.strip()
        提交 = subprocess.run(["git", "commit-tree", 树, "-p", 父, "-m", 提交信息],
                              cwd=仓库根, capture_output=True, text=True).stdout.strip()
        推 = subprocess.run(["git", "push", 远程, f"{提交}:{集成分支}"],
                            cwd=仓库根, capture_output=True, text=True)
        if 推.returncode == 0:
            return None
    return f"看板窄通道直推连续 3 次竞争失败（他人高频推送）——稍后重试。"


def 远端看板文本(远程: str = 主远程) -> str:
    运行(["git", "fetch", 远程])
    父 = 输出(["git", "rev-parse", f"{远程}/{集成分支}"])
    return subprocess.run(["git", "show", f"{父}:{看板文件名}"],
                          cwd=仓库根, capture_output=True).stdout.decode("utf-8")


# ── 队列服务层（920·协议 v4 五节点）：服务端优先+看板回退（行为超集·降级不失效）─────────
# 治「git 被当状态数据库」（920 诊断：56% 提交只改看板·917 单批 8 对起批/排队乒乓）：
#   中间态（集成中/失败恢复/封批）只进服务端**不再写 develop**=零 git 提交；
#   边界（报名/销账）双写（服务端为主+看板兼容未升级 920 版的他机——三机升级后看板段写入退役）。
# 配置：环境变量 CN_QUEUE_URL / CN_QUEUE_TOKEN，或 scripts/queue_client.json（gitignore·不入库）
#   形如 {"url": "http://<TX_01>:8300", "令牌": "..."}。无配置=纯看板路径（现状行为）。
队列服务URL = os.environ.get("CN_QUEUE_URL", "")
队列服务令牌 = os.environ.get("CN_QUEUE_TOKEN", "")
_客户端配置 = 仓库根 / "scripts" / "queue_client.json"
if not 队列服务URL and _客户端配置.exists():
    try:
        _cfg = json.loads(_客户端配置.read_text(encoding="utf-8"))
        队列服务URL = str(_cfg.get("url", ""))
        队列服务令牌 = str(_cfg.get("令牌", ""))
    except Exception:
        pass


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
             "状态": r.get("状态", "")} for r in 态.get("队列", [])]
    try:
        看板行们 = 解析队列(远端看板文本(远程))
    except Exception:
        看板行们 = []
    已知 = {行["分支"] for 行 in 行们}
    行们.extend(行 for 行 in 看板行们 if 行["分支"] not in 已知)
    return 行们


def 队列写(操作: str, 数据: dict) -> bool:
    """写操作（join/update/touch/clear）——服务不可用/未配置返回 False（回退看板直推）。"""
    结果 = 服务调用("/api/" + 操作, 数据)
    if 结果 is None:
        return False
    if not 结果.get("ok"):
        print(f"  [队列服务] {操作} 被拒：{结果.get('说明', '?')}——回退看板路径。")
    return bool(结果.get("ok"))


def 报名行(分支: str, 基线: str, 摘要: str) -> dict:
    return {"分支": 分支, "基线": 基线, "时刻": 现在时刻(), "摘要": 摘要, "状态": "排队"}


def 批写集(成员们: list[dict], 基底: str) -> list[str]:
    """批内全部成员写集并集（去重·分类门禁深度的输入）。"""
    全: list[str] = []
    for 行 in 成员们:
        全.extend(两点间改动(基底, 行["分支"]))
    return sorted(set(全))


def 组链(成员们: list[dict], 基底: str, 参数: argparse.Namespace) -> tuple[str | None, list[str], str | None]:
    """按报名序 cherry-pick 叠链：基底→成员1→成员2→…（链分支=当前 HEAD 检出态）。

    冲突=踢出该成员（cherry-pick --abort 后继续下一位·批不因成员卡死——被踢者本机解完
    冲突报下批）；--drop 点名的成员直接不入链。返回（链顶 sha, [(分支, 原因), …], 错误）。
    调用方须已把 HEAD 切到批分支（基底）——本函数只管叠不建。
    """
    踢出: list[tuple[str, str]] = []   # (分支, 原因∈{归因, 冲突})——销账按原因标态
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
        print(f"  [组链] 叠入 {分支}（{len(提交们)} 提交）")
        冲突 = False
        for 提交 in 提交们:
            拾取 = 运行(["git", "cherry-pick", 提交])
            if 拾取.returncode != 0:
                运行(["git", "cherry-pick", "--abort"])
                冲突 = True
                break
        if 冲突:
            踢出.append((分支, "冲突"))
            print(f"  [组链] {分支} cherry-pick 冲突——踢出（批主本机分支可自行解冲突后重报；"
                  f"他机分支请回自己分支解完报下批）")
    if 输出(["git", "rev-parse", "HEAD"]) == 基底:
        return None, 踢出, "批内全部成员被踢出（无一可叠）——请检查成员分支状态。"
    return 输出(["git", "rev-parse", "HEAD"]), 踢出, None


def 自测() -> int:
    """--selftest：纯函数正反例（CI 式·无 git 副作用）。"""
    空板 = "# 看板\n\n## 各机小节\n"
    案例数 = 0

    def 断言(条件: bool, 说明: str) -> None:
        nonlocal 案例数
        案例数 += 1
        标记 = "✓" if 条件 else "✗"
        print(f"  [{标记}] {说明}")
        if not 条件:
            raise AssertionError(说明)

    # ① 时刻解析
    断言(解析时刻("09-30 14:05") is not None, "时刻解析：正常「09-30 14:05」")
    断言(解析时刻("2026-09-30") is None, "时刻解析：坏格式拒绝（非 MM-DD HH:MM）")
    断言(解析时刻("99-99 99:99") is None, "时刻解析：越界值拒绝")

    # ② 队列加行/幂等/改状态/清行
    板1 = 队列加行(空板, {"分支": "任务/家机-895-x", "基线": "aaaa", "时刻": "09-30 14:00",
                        "摘要": "src/ir 两文件", "状态": "排队"})
    断言(len(解析队列(板1)) == 1, "加行：段不存在时建段且行可解析")
    断言(队列段标题 in 板1, "加行：段标题按规范落盘")
    板2 = 队列加行(板1, {"分支": "任务/家机-895-x", "基线": "aaaa", "时刻": "09-30 14:10",
                        "摘要": "重复报名", "状态": "排队"})
    断言(解析队列(板2) == 解析队列(板1), "加行幂等：同分支重复报名不产生第二行")
    板3 = 队列加行(板2, {"分支": "任务/单位机-911-y", "基线": "bbbb", "时刻": "09-30 14:06",
                        "摘要": "scripts", "状态": "排队"})
    断言(len(解析队列(板3)) == 2, "加行：第二成员入段")
    板4 = 队列改状态(板3, "任务/家机-895-x", "集成中")
    断言([x for x in 解析队列(板4) if x["分支"] == "任务/家机-895-x"][0]["状态"] == "集成中",
         "改状态：指定行状态切换")
    断言(队列改状态(板4, "任务/深度机-999-z", "集成中") == 板4, "改状态：未知分支原文不动")
    板5 = 队列清行们(板4, ["任务/家机-895-x"])
    断言(len(解析队列(板5)) == 1, "清行：销账移除指定分支行")
    断言(队列段标题 in 板5, "清行后段壳保留（三机共见结构稳定）")

    # ③ 选批成员：窗口/上限/单人立即/收拢期提示
    行a = {"分支": "A", "基线": "-", "时刻": "09-30 14:00", "摘要": "-", "状态": "排队"}
    行b = {"分支": "B", "基线": "-", "时刻": "09-30 14:06", "摘要": "-", "状态": "排队"}
    行c = {"分支": "C", "基线": "-", "时刻": "09-30 14:20", "摘要": "-", "状态": "排队"}
    成员, 提示 = 选批成员([行a, 行b, 行c], 解析时刻("09-30 14:20"))
    断言(成员 == [行a, 行b], "选批：窗口外（14:20>批主+10min）不入批·窗口内按序入选")
    断言(提示 is None, "选批：当前时刻≥窗口截止时无收拢期提示")
    _, 提示2 = 选批成员([行a, 行b], 解析时刻("09-30 14:03"))
    断言(提示2 is not None and "收拢期未满" in 提示2, "选批：窗口未满且多人时给收拢期提示")
    成员3, 提示3 = 选批成员([行a], 解析时刻("09-30 14:01"))
    断言(成员3 == [行a] and 提示3 is None, "选批：单人批立即可封（不等待）")
    行d = {"分支": "D", "基线": "-", "时刻": "09-30 14:01", "摘要": "-", "状态": "排队"}
    行e = {"分支": "E", "基线": "-", "时刻": "09-30 14:02", "摘要": "-", "状态": "排队"}
    行f = {"分支": "F", "基线": "-", "时刻": "09-30 14:03", "摘要": "-", "状态": "排队"}
    行g = {"分支": "G", "基线": "-", "时刻": "09-30 14:04", "摘要": "-", "状态": "排队"}
    行h = {"分支": "H", "基线": "-", "时刻": "09-30 14:05", "摘要": "-", "状态": "排队"}
    成员4, 提示4 = 选批成员([行a, 行b, 行d, 行e, 行f, 行g, 行h], 解析时刻("09-30 14:12"))
    断言(len(成员4) == 批上限 and 成员4[0]["分支"] == "A",
         f"选批：上限截断（{批上限} 个·超出者等下批）且批主=最早报名者")
    成员5, 提示5 = 选批成员([], 解析时刻("09-30 14:00"))
    断言(成员5 == [] and 提示5 is not None, "选批：空队列给提示")

    # ④ 临时用例拦截（1008 纪律 2）
    断言(临时用例检查(["src/a.cpp", "tests/e2e/501_正常用例/x.cn"]) is None,
         "tmp 拦截：正常用例名与非 e2e 路径放行")
    断言(临时用例检查(["tests/e2e/990tmp_128最小/x.cn"]) is not None,
         "tmp 拦截：tmp 目录名拒绝")
    断言(临时用例检查(["tests/e2e/990Tmp_测试/x.cn"]) is not None,
         "tmp 拦截：大小写不敏感（Tmp）")
    断言(临时用例检查(["tests/e2e/501_x/TMP说明.md"]) is None,
         "tmp 拦截：tmp 仅判用例目录名（路径第三段）——用例内文件名放行")
    断言(临时用例检查(["docs/tmp笔记.md", "src/tmp.cpp"]) is None,
         "tmp 拦截：e2e 外路径放行")

    # ⑤ 预验结果解析（1008）
    绿, 摘 = 解析预验结果('{"绿": true, "总秒": 800, "日志": "run_x.log",'
                          '"步骤": {"构建": {"rc": 0}}, "日志尾部": []}')
    断言(绿 and "run_x.log" in 摘, "预验解析：绿 JSON 判绿+日志名入摘要")
    红, 摘2 = 解析预验结果('{"绿": false, "总秒": 2000, "日志": "run_y.log",'
                           '"步骤": {"构建": {"rc": 0}, "e2e": {"rc": 1}},'
                           '"日志尾部": ["✗ 524_条件编译双编译: x"]}')
    断言(not 红 and "e2e" in 摘2 and "524" in 摘2, "预验解析：红 JSON 判红+红步骤与尾部入摘要")
    断言(not 解析预验结果("非JSON文本")[0], "预验解析：坏 JSON 判红不炸")
    断言(临时用例检查(批写集([{"分支": "任务/家机-9-x"}], "aaaa")) is None,
         "tmp 拦截：批写集空路径不越界")

    print(f"\n[selftest] {案例数} 例全过 ✓")
    return 0


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
        增量 = 两点间改动(上次已验基准, 最新)
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
                清理 = 运行(["git", "push", 参数.remote, "--delete", 分支])
                if 清理.returncode == 0:
                    print(f"  [清理] 远程任务分支 {分支} 已删（内容已入 {集成分支}·AGENTS.md §7 集成即删）。")
                    总账收口021([分支], 输出(["git", "rev-parse", "HEAD"]), 参数.remote)
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
    """批量集成主路径（911·AGENTS.md §5）：报名→（等待）→组批→组链→门禁→整批 push→销账。"""
    import time as _time

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

    def 报名变换(文: str):
        重报文, 变更 = 队列重报(文, 当前分支, 新行["时刻"])
        if 变更:
            return 重报文
        加行文 = 队列加行(文, 新行)
        return None if 加行文 == 文 else 加行文

    if 参数.dry_run:
        print(f"  [演练] 跳过看板报名直推（内存行代用）：{当前分支} @ {新行['时刻']} {摘要}")
    else:
        # 920·边界双写：服务端（权威·实时）+看板段（兼容未升级 920 版的他机）
        服务成 = 队列写("join", {"分支": 当前分支, "基线": 新行["基线"], "写集摘要": 摘要})
        问题 = 直推看板提交(报名变换, f"集成队列：{当前分支} 报名（911 批量集成·{摘要}）", 参数.remote)
        if 问题 and not 服务成:
            return 失败(问题)
        print(f"[2] 报名入队：{当前分支} @ {新行['时刻']}（{摘要}）"
              + ("｜服务端✓" if 服务成 else "｜服务端不可达·看板单写（回退）"))

    # ── ③ 等待成为批主（--wait 轮询；非 --wait 一次性判定后退出码 2）───────────────────
    等待起 = datetime.now()
    while True:
        队列 = 队列读(参数.remote) if not 参数.dry_run else None      # 920：服务端优先（实时）
        if 队列 is None:
            文 = 远端看板文本(参数.remote) if not 参数.dry_run else ""
            队列 = 解析队列(文) if not 参数.dry_run else [新行]
        排队们 = [行 for 行 in 队列 if 行["状态"] == "排队"]
        集成中们 = [行 for 行 in 队列 if 行["状态"] == "集成中"]
        我在排队 = any(行["分支"] == 当前分支 for 行 in 排队们)
        失联们 = []
        提示: str | None = None
        for 行 in 集成中们:
            起 = 解析时刻(行["时刻"])
            if 起 and (datetime.now() - 起).total_seconds() > 批主失联接管分钟 * 60:
                失联们.append(行["分支"])
        活跃集成中 = [行 for 行 in 集成中们 if 行["分支"] not in 失联们]
        if not 我在排队 and not any(行["分支"] == 当前分支 for 行 in 队列):
            return 失败("本分支不在集成队列（可能已被他批集成销账）——若分支仍未入 develop 请重报。")
        if 活跃集成中:
            提示 = (f"批集成进行中（{'、'.join(行['分支'] for 行 in 活跃集成中)}）——"
                    f"等待其完成（失联超 {批主失联接管分钟} 分钟可 --takeover 接管）")
        else:
            批主 = min(排队们, key=lambda 行: 行["时刻"]) if 排队们 else None
            if 批主 and (批主["分支"] == 当前分支 or 参数.takeover):
                成员, 收拢提示 = 选批成员(排队们, datetime.now())
                if 收拢提示 and len(排队们) > 1 and not 参数.seal_now:
                    提示 = 收拢提示
                else:
                    break   # ── 封批组链 ──
            elif 批主:
                起 = 解析时刻(批主["时刻"])
                可接管 = (起 and (datetime.now() - 起).total_seconds() > 批主失联接管分钟 * 60)
                提示 = (f"批主={批主['分支']}（第 {[x['分支'] for x in sorted(排队们, key=lambda r: r['时刻'])].index(当前分支) + 1}"
                        f" 位等待中）——{'已失联可 --takeover 接管' if 可接管 else '正常在飞·等其组批'}")
        if 参数.wait:
            if (datetime.now() - 等待起).total_seconds() > 批等待上限分钟 * 60:
                return 失败(f"等待批主超时（{批等待上限分钟} 分钟）——稍后重跑或 --takeover。")
            print(f"  [等待] {提示}")
            _time.sleep(批等待轮询秒)
            continue
        print(f"  [等待] {提示}（重跑本命令继续·或加 --wait 挂轮询）")
        return 2

    # ── ④ 组批：标记集成中→组链→门禁→push（CAS 重试）────────────────────────────
    def 标集成中(文: str):
        新 = 队列改状态(文, 当前分支, "集成中")
        return None if 新 == 文 else 新

    if not 参数.dry_run:
        # 920·中间态只进服务端（不写 develop=治起批/排队乒乓提交）；服务不可达回退看板直推
        if 队列写("update", {"分支": 当前分支, "状态": "集成中"}):
            print("  [队列服务] 已标集成中（服务端·零 git 提交——920 治乒乓）")
        else:
            直推看板提交(标集成中, f"集成队列：{当前分支} 批主起批（收拢期满封批）", 参数.remote)
    接管标 = "·takeover" if 参数.takeover else ""
    封批标 = "·seal-now" if 参数.seal_now else ""
    print(f"[3] 封批组链（批主={当前分支}{接管标}{封批标}）")

    原分支 = 当前分支
    批分支 = "batch/" + 当前分支.replace("任务/", "", 1)
    成功 = False
    链顶 = None
    踢出们: list[str] = []
    已验基准: str | None = None
    try:
        for 尝试 in range(1, 最大重试 + 1):
            运行(["git", "fetch", 参数.remote])
            最新 = 输出(["git", "rev-parse", f"{参数.remote}/{集成分支}"])
            队列 = 队列读(参数.remote)                       # 920：服务端优先（双源合并）
            if 队列 is None:
                队列 = 解析队列(远端看板文本(参数.remote))
            排队们 = [行 for 行 in 队列 if 行["状态"] == "排队"]
            # 批主自己行已被标「集成中」（组批前置直推）——成员候选必须含它，
            #   否则批主分支不进链（911 演练实录：链=develop→B→C 唯独无 A·幸未到 push）
            我行 = next((行 for 行 in 队列 if 行["分支"] == 当前分支), None)
            if 我行 and 我行["状态"] == "集成中" and 我行 not in 排队们:
                排队们 = [我行] + 排队们   # 批主行置首——组链序=报名序·批主最先叠（冲突踢后到者）
            成员, _ = 选批成员(排队们, datetime.now())
            成员 = 成员 if any(行["分支"] == 当前分支 for 行 in 成员) else \
                [行 for 行 in 成员 if 行["分支"] != 当前分支] + \
                [行 for 行 in 排队们 if 行["分支"] == 当前分支]
            建链 = 运行(["git", "checkout", "-B", 批分支, 最新])
            if 建链.returncode != 0:
                return 失败("批分支创建失败（工作树状态异常）——请检查 git 状态。")
            链顶, 新踢出, 错误 = 组链(成员, 最新, 参数)
            踢出们 = 新踢出
            if 参数.dry_run:
                踢出们 = [对 for 对 in 新踢出 if 对[0] != 当前分支]
            踢出名们 = [对[0] for 对 in 踢出们]
            if 错误:
                return 失败(错误)
            实际成员 = [行 for 行 in 成员 if 行["分支"] not in 踢出名们]
            批文件们 = 批写集(实际成员, 最新)
            须全量 = 分类写集(批文件们)
            print(f"[4] 批门禁（成员 {len(实际成员)}·写集 {len(批文件们)} 文件"
                  f"{'·全量' if 须全量 else '·快速'}）"
                  + (f"（踢出：{'、'.join(f'{b}[{因}]' for b, 因 in 踢出们)}）" if 踢出们 else ""))
            if 已验基准 is None or 已验基准 == 最新:
                增量须全量 = 须全量 if 已验基准 is None else False
            else:
                增量须全量 = 分类写集(两点间改动(已验基准, 最新))
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
            if 须全量 and not 等价免验 and (已验基准 is None or 增量须全量):
                print(f"[5] 全量门禁（平台={平台}·跑在整批合并结果上）")
                问题 = 全量门禁(平台, 参数.allow_known_red, 参数.fast_lane)
                if 问题:
                    print("  [归因辅助] 批成员×写集（对照失败日志定位破坏者后 --drop 踢出重验）：")
                    for 行 in 实际成员:
                        成员写集 = 两点间改动(最新, 行["分支"])
                        print(f"    - {行['分支']}：{成员写集}")
                    return 失败(问题 + "（批门禁红——归因后 --drop <分支> 重组链重验）")
                if 参数.cloud_gate:
                    print(f"[5.5] 云端预验门禁（1008·链顶 {链顶[:8]}·绿才整批 push develop）")
                    问题 = 云端预验门禁(链顶, 参数)
                    if 问题:
                        return 失败(问题 + "（批预验红——整批留在分支·修复后重报）")
            elif 须全量 and not 等价免验:
                print("[5] 全量门禁跳过（develop 增量纯文档·已验部分仍有效）")
            已验基准 = 最新
            push_命令 = ["git", "push", "--dry-run" if 参数.dry_run else None,
                         参数.remote, f"HEAD:{集成分支}"]
            push_命令 = [c for c in push_命令 if c]
            print(f"[6] push {参数.remote} {集成分支}（ff-only·整批 CAS）")
            结果 = 运行(push_命令)
            if 结果.returncode == 0:
                成功 = True
                break
            print(f"  [竞争] push 被拒（他批/他机刚推）——第 {尝试}/{最大重试} 次重组链重试")
        if not 成功:
            return 失败(f"连续 {最大重试} 次批集成竞争失败——稍后重跑（队列报名仍在·幂等）。")
    finally:
        运行(["git", "checkout", 原分支])
        运行(["git", "branch", "-D", 批分支])
        # 失败恢复：批主行回「排队」——否则「集成中」残留会让重跑等待自己（死锁）
        if not 成功 and not 参数.dry_run:
            def 恢复排队(文: str):
                新 = 队列改状态(文, 当前分支, "排队")
                return None if 新 == 文 else 新
            if 队列写("touch", {"分支": 当前分支}):    # 920·中间态服务端（零 git 提交）
                print("  [队列服务] 批主失败恢复排队（服务端）")
            else:
                直推看板提交(恢复排队, f"集成队列：{当前分支} 批主失败恢复排队", 参数.remote)

    # ── ⑤ 销账：删成员远端分支→清队列行→镜像补推→验收提示 ─────────────────────────
    新tip = 链顶
    if not 参数.dry_run:
        for 行 in 实际成员:
            清理 = 运行(["git", "push", 参数.remote, "--delete", 行["分支"]])
            if 清理.returncode == 0:
                print(f"  [清理] 远程任务分支 {行['分支']} 已删（内容已随批入 {集成分支}·AGENTS.md §7）。")
            else:
                print(f"  [警告] 远程任务分支 {行['分支']} 删除失败——稍后 branch_cleanup.py 兜底。")

        def 销账变换(文: str):
            新 = 队列清行们(文, [行["分支"] for 行 in 实际成员])
            for 分支, 原因 in 踢出们:
                新 = 队列改状态(新, 分支, "归因出批" if 原因 == "归因" else "冲突出批")
            return None if 新 == 文 else 新

        # 920·边界双写：服务端销账（权威）+看板段（兼容旧机）
        队列写("clear", {"分支们": [行["分支"] for 行 in 实际成员]})
        for 分支, _原因 in 踢出们:
            队列写("update", {"分支": 分支, "状态": "已踢出"})
        # v5（1016）：看板销账直推废除——021 总账自动收口（含解锁播报）
        总账收口021([行["分支"] for 行 in 实际成员], 新tip, 参数.remote)
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
                        help="云端预验门禁显式关（逃生门）：回退 941 fast-lane 事后兜底"
                             "（TX_02 不可达/挂死时·028 §四处置后恢复）")
    解析器.add_argument("--solo", action="store_true",
                        help="旧单分支路径逃生门（不经队列直集成——队列机制异常时用）")
    解析器.add_argument("--selftest", action="store_true", help="正反例自测（纯函数面·CI 式）")
    解析器.add_argument("--wait", action="store_true",
                        help=f"等待成为批主（轮询 {批等待轮询秒}s·上限 {批等待上限分钟} 分钟）")
    解析器.add_argument("--seal-now", action="store_true",
                        help="批主用：跳过收拢期立即封批（多人排队时）")
    解析器.add_argument("--takeover", action="store_true",
                        help=f"批主失联超 {批主失联接管分钟} 分钟时接管组批")
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
    # v2 遗留旗标（581-a 废除·接受即忽略——v3 下「本机门禁绿即集成」本就是默认行为，
    # 保留解析仅为不炸他机旧命令行习惯）
    解析器.add_argument("--try-build-done", action="store_true",
                        help=argparse.SUPPRESS)
    解析器.add_argument("--win-verified", action="store_true",
                        help=argparse.SUPPRESS)
    参数 = 解析器.parse_args()
    # 941（用户裁决 2026-10-02）：快速通道默认开——--no-fast-lane 显式回退本机全量
    参数.fast_lane = not 参数.no_fast_lane
    # 1008（用户裁决 2026-10-03·方案甲）：云端预验门禁——1015 起**默认开**（develop 回绿
    #   达成·1013 终验）；显式旗优先于默认（--no-cloud-gate 逃生门）。
    参数.cloud_gate = (not 参数.no_cloud_gate) and (云端预验默认开 or 参数.cloud_gate)
    if 参数.no_cloud_gate:
        print("[1008] --no-cloud-gate：云端预验门禁显式关闭——回退 941 fast-lane 事后兜底"
              "（TX_02 异常时的逃生门·028 §四）。")
    if 参数.try_build_done or 参数.win_verified:
        print("[v3] --try-build-done/--win-verified 已随协议 v3 废除（接受即忽略）——"
              "本机门禁绿即集成，跨平台由集成后异步验收保障（AGENTS.md §7）。")

    if 参数.selftest:
        return 自测()
    if 参数.status:
        队列 = 队列读(参数.remote)                      # 920：服务端优先（双源合并）
        if 队列 is None:
            队列 = 解析队列(远端看板文本(参数.remote))
        if not 队列:
            print("[集成队列] 空（无人排队——bors 批组建·AGENTS.md §5）")
            return 0
        print("[集成队列]（bors 批组建·AGENTS.md §5）")
        for 行 in sorted(队列, key=lambda r: r["时刻"]):
            print(f"  {行['状态']}｜{行['时刻']}｜{行['分支']}｜{行['摘要']}（基线 {行['基线']}）")
        return 0

    平台 = 探测平台()
    if 参数.solo:
        return solo流程(平台, 参数)
    if 参数.join:
        return 批流程_join_only(参数)
    return 批流程(参数)


def 批流程_join_only(参数: argparse.Namespace) -> int:
    """--join：仅报名（报名后立即返回 0——组批留给稍后重跑）。"""
    问题 = 前置自检(参数.remote)
    if 问题:
        return 失败(问题)
    当前分支 = 输出(["git", "branch", "--show-current"])
    运行(["git", "fetch", 参数.remote])
    基线 = 输出(["git", "rev-parse", f"{参数.remote}/{集成分支}"])
    写集 = 改动文件清单(基线)
    if not 写集:
        return 失败("分支相对 develop 无任何改动（空集成）。")
    摘要 = f"{len(写集)} 文件" + ("·触全量面" if 分类写集(写集) else "·纯文档/脚本")
    新行 = 报名行(当前分支, 基线[:8], 摘要)

    def 报名变换(文: str):
        重报文, 变更 = 队列重报(文, 当前分支, 新行["时刻"])
        if 变更:
            return 重报文
        加行文 = 队列加行(文, 新行)
        return None if 加行文 == 文 else 加行文

    问题 = 直推看板提交(报名变换, f"集成队列：{当前分支} 报名（911 批量集成·{摘要}）", 参数.remote)
    if 问题:
        return 失败(问题)
    print(f"[报名成功] {当前分支} @ {新行['时刻']}（{摘要}）——稍后重跑 integrate.py 组批集成。")
    return 0


if __name__ == "__main__":
    sys.exit(主流程())
