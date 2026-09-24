#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""分支生命周期兜底清理工具（710 治本·AGENTS.md §8.1「集成即删分支」的配套）。

背景：协议 v2「分支即开发」（2026-09-18 起）每轮一条任务分支，集成分支进入
develop 后远程分支原设计无删除步骤——7 天积压 367 条（9-18~9-24 实测），
710 轮已一次性清理 347 条并落地 integrate.py「集成即删」钩子。本工具负责
两类残余的定期兜底：
  ① 未走到集成的放弃/中止分支（integrate.py 钩子覆盖不到）；
  ② 历史遗漏。

安全设计（防误删·与 710 轮人工甄别口径一致）：
  - 只删「已并入 develop」的分支（git merge-base --is-ancestor 判定）——
    有差量（含 rebase 重写导致 cherry 误报）的一律不动，交人工甄别；
  - 看板当前在飞/挂等行引用的分支名自动排除（窄通道实时广播为准）；
  - 默认 dry-run 只打印清单，--run 才真删。

用法：
  python scripts/branch_cleanup.py           # 演练：列出可删分支
  python scripts/branch_cleanup.py --run     # 执行删除（gitcode）
"""

import argparse
import re
import subprocess
import sys

主远程 = "gitcode"
集成分支 = "develop"
看板文件 = "三机任务看板.md"


def 运行(命令: list) -> subprocess.CompletedProcess:
    return subprocess.run(命令, capture_output=True)


def 输出(命令: list) -> str:
    return 运行(命令).stdout.decode("utf-8", "replace").strip()


def 看板在飞分支() -> set:
    """看板各机行引用的任务分支名=在飞/挂等的唯一实时广播，一律排除。"""
    try:
        文本 = 输出(["git", "show", f"{主远程}/{集成分支}:{看板文件}"])
    except Exception:
        return set()
    return set(re.findall(r"任务/[^\s｜|）)，。；』」]+", 文本))


def 主流程() -> int:
    解析器 = argparse.ArgumentParser(description="远程任务分支兜底清理（只删已并入 develop 的安全面·默认演练）")
    解析器.add_argument("--run", action="store_true", help="执行删除（缺省=演练只打印）")
    参数 = 解析器.parse_args()

    print("[1] fetch --prune …")
    运行(["git", "fetch", "--prune", 主远程])

    全部 = [l for l in 输出(["git", "branch", "-r", "--format=%(refname:short)"]).splitlines()
            if l.startswith(f"{主远程}/任务/")]
    保护 = 看板在飞分支()
    print(f"[2] 远程任务分支 {len(全部)} 条｜看板在飞/挂等保护 {len(保护)} 条")

    可删, 保留 = [], []
    for b in 全部:
        短名 = b.replace(f"{主远程}/", "", 1)
        if 短名 in 保护 or b in 保护:
            保留.append((b, "看板在飞/挂等"))
            continue
        判定 = 运行(["git", "merge-base", "--is-ancestor", b, f"{主远程}/{集成分支}"])
        if 判定.returncode == 0:
            可删.append(b)
        else:
            保留.append((b, "未并入 develop（差量交人工甄别·不自动删）"))

    print(f"[3] 可安全删除 {len(可删)} 条｜保留 {len(保留)} 条")
    for b, 理由 in 保留:
        print(f"    保留 {b}（{理由}）")

    if not 可删:
        print("无可删分支，done。")
        return 0
    if not 参数.run:
        for b in 可删:
            print(f"    [演练] 将删除 {b}")
        print(f"共 {len(可删)} 条——确认无误后加 --run 执行。")
        return 0

    失败 = []
    for b in 可删:
        短名 = b.replace(f"{主远程}/", "", 1)
        结果 = 运行(["git", "push", 主远程, "--delete", 短名])
        状态 = "OK" if 结果.returncode == 0 else "失败 " + 结果.stderr.decode("utf-8", "replace")[:120]
        print(f"    删除 {短名}: {状态}")
        if 结果.returncode != 0:
            失败.append(短名)
    print(f"[4] 完成：删 {len(可删) - len(失败)}/{len(可删)}" + (f"｜失败 {len(失败)}" if 失败 else ""))
    return 1 if 失败 else 0


if __name__ == "__main__":
    sys.exit(主流程())
