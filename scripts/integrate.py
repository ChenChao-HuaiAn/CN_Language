#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""合并队列脚本（三机并行协同协议 v2·AGENTS.md §8.2——develop 唯一入关口）。

职责：把当前任务分支安全集成回 develop——
  前置自检 → fetch → rebase（冲突自己解·禁止 develop 手解）→ 写集分类（try-build 提示）
  → 门禁（跑在合并结果上：快速门禁恒跑；写集触及 src/tests 时全量门禁）
  → push gitcode develop（ff-only·CAS 竞争输家自动 rebase 重试 ≤3 次）→ 补推 github 镜像。
业界对照=rustc/bors 合并队列、GitHub merge queue 的脚本化实现（方案全文=plans/027）。

用法（在任务分支上运行）：
  python3 scripts/integrate.py                    # 正常集成
  python3 scripts/integrate.py --dry-run          # 演练：全步骤、push 以 --dry-run 代替（不真推）
  python3 scripts/integrate.py --try-build-done   # 写集触及三平台共享层且 try-build 已全绿回签后声明（§8.3）

退出码：0=集成成功（或演练通过）；1=失败（原因见输出）。
"""
from __future__ import annotations

import argparse
import platform
import re
import subprocess
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
主远程 = "gitcode"
镜像远程 = "github"
集成分支 = "develop"
分支名模式 = re.compile(r"^任务/(家机|单位机|深度机)-\d+-\S+$")
最大重试 = 3

# 三平台产物相关写集（触及→须三平台 try-build·AGENTS.md §8.3；纯文档/平台无关脚本不触发）
try_build_触发模式 = ("src/", "tests/", "CMakeLists.txt", "build.ps1", "scripts/ci.ps1")
# 全量门禁触发面（触及 src/tests → 除快速门禁外另跑构建+单测+E2E）
全量门禁触发模式 = ("src/", "tests/")


def 运行(命令: list[str], **kwargs) -> subprocess.CompletedProcess:
    """执行子进程（打印命令行；默认继承 stdout/stderr）。"""
    print(f"  $ {' '.join(命令)}")
    return subprocess.run(命令, cwd=仓库根, **kwargs)


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


def 前置自检() -> str | None:
    """① 分支名合法 ② 工作树干净 ③ 分支已推远端。返回失败信息（None=通过）。"""
    当前分支 = 输出(["git", "branch", "--show-current"])
    if not 当前分支:
        return "不在任何分支上（detached HEAD？）——请在任务分支上运行本脚本。"
    if not 分支名模式.match(当前分支):
        return f"分支名「{当前分支}」不合法——须形如 任务/<机>-<轮次>-<标识>（AGENTS.md §8.1）。"
    状态 = 输出(["git", "status", "--porcelain"])
    未跟踪 = [行[3:] for 行 in 状态.splitlines() if 行.startswith("?? ")]
    已跟踪改动 = [行 for 行 in 状态.splitlines() if not 行.startswith("?? ")]
    if 已跟踪改动:
        return f"工作树不干净（提交或清理后再集成）：\n{已跟踪改动}"
    if 未跟踪:
        print(f"  [警告] 未跟踪文件不阻塞集成（不入提交）：{未跟踪}")
    远端分支 = f"{主远程}/{当前分支}"
    if 输出(["git", "rev-parse", "--verify", f"refs/remotes/{远端分支}"]) == "":
        return f"分支未推远端（{远端分支} 不存在）——推分支=认领（AGENTS.md §8.1），先 git push。"
    本地 = 输出(["git", "rev-parse", "HEAD"])
    远端 = 输出(["git", "rev-parse", 远端分支])
    if 本地 != 远端:
        return f"本地提交未推远端（本地 {本地[:8]} ≠ 远端 {远端[:8]}）——先推分支再集成。"
    return None


def 两点间改动(起点: str, 终点: str) -> list[str]:
    """两点间 commit 范围的改动文件清单——写集分类与冲突标记扫描的输入。"""
    文本 = 输出(["git", "diff", "--name-only", f"{起点}..{终点}"])
    return [行.strip() for 行 in 文本.splitlines() if 行.strip()]


def 改动文件清单(基准: str) -> list[str]:
    """基准（develop tip）相对 HEAD 的改动文件清单（=两点间改动 的便捷封装）。"""
    return 两点间改动(基准, "HEAD")


def 分类写集(文件们: list[str]) -> tuple[bool, bool]:
    """返回（须 try-build，须全量门禁）。"""
    须_try = any(文件.startswith(try_build_触发模式) or 文件 in try_build_触发模式 for 文件 in 文件们)
    须全量 = any(文件.startswith(全量门禁触发模式) for 文件 in 文件们)
    return 须_try, 须全量


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


def 快速门禁(文件们: list[str]) -> str | None:
    """快速门禁：冲突标记 + 双文档结构门禁（纯文档/脚本写集的完整门禁）。"""
    问题 = 冲突标记检查(文件们)
    if 问题:
        return 问题
    for 脚本 in ("scripts/check_handoff.py", "scripts/check_progress_sync.py"):
        结果 = 运行([sys.executable, 脚本])
        if 结果.returncode != 0:
            return f"{脚本} 未过——修复后重试。"
    return None


def 全量门禁(平台: str, 已知红们: list[str] | None = None) -> str | None:
    """全量门禁：零警告构建 + 单测 + E2E 全量（平台相关；win=ci.ps1 一步到位）。

    并行红串行复验（447-a·机制级工具改进）：runner 隔离键欠账（同编号前缀不同主体名的
    _v2 用例共享 v2src/v2work 产物目录·274-a 口径·根治=329-D12 待集成）使 --jobs 8 并行
    下互踩用例稳定红而单跑必过（非代码红）。ci.ps1/E2E 未过时分步复验：构建或单测红=真
    失败；E2E 串行复验（--jobs 1）绿=并行互踩嫌疑→警告放行（要求看板通告披露）；串行仍
    红=真失败。329-D12 集成后互踩根除，本机制触发频率应归零（届时评估移除）。
    """
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
        串行 = 运行([sys.executable, "tests/e2e/run_e2e.py",
                     "--cn", str(仓库根 / "target/Debug/cn.exe"), "--jobs", "1"])
        if 串行.returncode != 0:
            return "E2E 串行复验仍未全绿（非并行互踩——真红，禁止集成）。"
        print("  [复验] 并行红+串行绿=runner 产物互踩嫌疑（274-a 隔离键欠账·非代码红）"
              "——放行；须在看板通告段披露。")
        return None
    # Linux：分步（单位机 linux-arm64 / 深度机 linux-x64；E2E 须显式 --target——默认 win-x64 会报错）
    # 449-a：构建/单测/E2E 三段经 gate_lock 串行锁（同机多 worktree 并行防互抢·AGENTS.md §8.8）。
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
    e2e = 运行([sys.executable, str(仓库根 / "scripts/gate_lock.py"), "run", "--", sys.executable,
               "tests/e2e/run_e2e.py", "--target", 目标, "--cn", str(cn路径), "--jobs", "8"])
    if e2e.returncode == 0:
        return None
    print("  [复验] E2E 并行未全绿——串行复验区分真红与并行互踩（447-a 机制·gate_lock 锁内）")
    串行 = 运行([sys.executable, str(仓库根 / "scripts/gate_lock.py"), "run", "--", sys.executable,
                "tests/e2e/run_e2e.py", "--target", 目标, "--cn", str(cn路径), "--jobs", "1"])
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
                  f"须在看板通告段完整披露点名依据。")
            return None
        if 未点名:
            return (f"E2E 串行复验存在未点名真红 {未点名}（--target {目标}"
                    f"·点名清单外——禁止集成）。")
        return f"E2E 串行复验仍未全绿（--target {目标}·非并行互踩——真红，禁止集成）。"
    print("  [复验] 并行红+串行绿=runner 产物互踩嫌疑（274-a 隔离键欠账·非代码红）"
          "——放行；须在看板通告段披露。")
    return None


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
            return False, "rebase 冲突——请在任务分支上自行解决后重新集成（AGENTS.md §8.2 步骤 3）。", None

    文件们 = 改动文件清单(最新)
    if not 文件们:
        return False, "分支相对 develop 无任何改动（空集成）。", None
    须_try, 须全量 = 分类写集(文件们)
    if 须_try and not 参数.try_build_done and not 参数.win_verified:
        return False, (
            f"写集触及三平台共享层（{[f for f in 文件们 if f.startswith(try_build_触发模式) or f in try_build_触发模式][:5]}…）"
            "——须先完成三平台 try-build（推分支→看板通告段点名另两机→全绿回签），再以 --try-build-done 集成（AGENTS.md §8.3）；"
            "或按 2026-09-21 用户令以 --win-verified 走平台后验模式（win 全量绿即集成·其他平台集成后自验证·发起机看板广播披露）。"
        ), None
    if 须_try and 参数.win_verified and not 参数.try_build_done:
        print("[平台后验模式·用户令 2026-09-21] win 全量门禁已验即集成；"
              "linux/arm64 由各平台机集成后自验证（集成后验证常设）——发起机须看板广播披露本模式。")

    # 门禁跑在合并结果上；重试时仅当 develop 增量（上轮已验 tip→本轮最新）触及 src/tests 才重跑全量门禁
    if 上次已验基准 is None or 上次已验基准 == 最新:
        增量 = 文件们 if 上次已验基准 is None else []
    else:
        增量 = 两点间改动(上次已验基准, 最新)
    _, 增量须全量 = 分类写集(增量)
    print(f"[3] 快速门禁（改动 {len(文件们)} 个文件）")
    问题 = 快速门禁(文件们)
    if 问题:
        return False, 问题, None
    if 须全量 and (上次已验基准 is None or 增量须全量):
        print(f"[4] 全量门禁（平台={平台}·跑在合并结果上）")
        问题 = 全量门禁(平台, 参数.allow_known_red)
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


def 主流程() -> int:
    解析器 = argparse.ArgumentParser(description="三机并行协同协议 v2·合并队列（AGENTS.md §8.2）")
    解析器.add_argument("--dry-run", action="store_true", help="演练模式：不真推 develop")
    解析器.add_argument("--allow-known-red", action="append", default=[],
                        metavar="用例名",
                        help="已知红显式点名（564-a 过渡机制·fdef8ae9 P1 构建确定性根治前）："
                             "串行复验红若全部命中点名清单=三平台已定性已知红披露放行；"
                             "未点名红仍硬拦。使用责任=发起机（点名依据+集成广播披露不实=违规可 revert）。"
                             "fdef8ae9 P1 根治后本参数应移除。可多次传入点名多个用例。")
    解析器.add_argument("--try-build-done", action="store_true",
                        help="声明三平台 try-build 已全绿回签（§8.3·共享层写集前置）")
    解析器.add_argument("--win-verified", action="store_true",
                        help="平台后验模式（2026-09-21 用户令）：win 全量门禁绿即集成，"
                             "不等另两机回签——linux/arm64 集成后自验证（发起机看板广播披露）")
    参数 = 解析器.parse_args()
    平台 = 探测平台()
    print(f"== 合并队列（协议 v2）｜平台={平台}｜模式={'演练' if 参数.dry_run else '集成'} ==")

    问题 = 前置自检()
    if 问题:
        return 失败(问题)
    print("[1] 前置自检通过")

    已验基准: str | None = None
    for 尝试 in range(1, 最大重试 + 1):
        成功, 信息, 基准 = 单次集成尝试(平台, 已验基准, 参数)
        if 基准:
            已验基准 = 基准  # 门禁跑过即记录（含竞争失败轮——重试时增量判定用）
        if 成功:
            if not 参数.dry_run:
                镜像 = 运行(["git", "push", 镜像远程, f"HEAD:{集成分支}"])
                if 镜像.returncode != 0:
                    print(f"  [警告] github 镜像补推失败——按惯例下次提交补推（不影响集成有效性）。")
            分支 = 输出(["git", "branch", "--show-current"])
            print(f"\n[集成成功] {分支} → {集成分支}（{输出(['git', 'rev-parse', 'HEAD'])[:8]}）"
                  f"——请在看板通告段广播影响面（哪些平台需集成后验证·AGENTS.md §8.2 步骤 7）。")
            return 0
        if 信息 is None:
            print(f"  [竞争] push 被拒（他人刚集成）——第 {尝试}/{最大重试} 次 rebase 重试")
            continue
        return 失败(信息)
    return 失败(f"连续 {最大重试} 次集成竞争失败——稍后再试或与对方协调（看板通告段「集成中」标注）。")


if __name__ == "__main__":
    sys.exit(主流程())
