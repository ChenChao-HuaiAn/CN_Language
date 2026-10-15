#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""分支生命周期兜底清理工具（710 治本·AGENTS.md §8.1「集成即删分支」的配套）。

背景：协议 v2「分支即开发」（2026-09-18 起）每轮一条任务分支，集成分支进入
develop 后远程分支原设计无删除步骤——7 天积压 367 条（9-18~9-24 实测），
710 轮已一次性清理 347 条并落地 integrate.py「集成即删」钩子。本工具负责
三类残余的定期兜底：
  ① 未走到集成的放弃/中止任务分支（integrate.py 钩子覆盖不到）；
  ② 历史遗漏；
  ③ ci/预验-* 已消费残留（864：863 起非 --wait 路径保留预验分支——远端实测
    66 条历史残留·本工具兜底清理）。

安全设计（防误删·与 710 轮人工甄别口径一致）：
  - 任务/*：只删「已并入 develop 且非空分支」的——有差量（含 rebase 重写导致
    cherry 误报）或分支头==develop 头（刚建未动工的在飞分支·864 补 710 盲区）
    的一律不动，交人工甄别；
  - ci/预验-*：只删「已消费完」的——服务端（8300 预验任务表·快照 miss 再走
    /api/task_result 历史行全表查）该记录状态 ∈ 终态{完成/已取消/已踢出}，或
    该分支 sha 已并入 develop，两者满足其一才删；服务端记录为 排队/执行中的
    一律保留（进行中预验绝不删——判据失误会卡别的会话的集成）；服务不可达时
    预验段整体保守跳过一条不删；
  - 看板当前在飞/挂等行引用的分支名自动排除（窄通道实时广播为准·382 起该源
    渐空载·空分支保护兜底）；
  - 默认 dry-run 只打印清单，--run 才真删。

用法：
  python scripts/branch_cleanup.py             # 演练：列出可删分支
  python scripts/branch_cleanup.py --run       # 执行删除（gitcode）
  python scripts/branch_cleanup.py --selftest  # 判据纯函数自测（不碰 git/网络）
"""

import argparse
import json
import os
import re
import subprocess
import sys
import urllib.request

主远程 = "gitcode"
集成分支 = "develop"
看板文件 = "三机任务看板.md"
预验前缀 = "ci/预验-"
# 预验任务表状态字段全部取值（queue_service.py 建表口径）：排队/执行中/完成/已取消/已踢出
预验终态 = {"完成", "已取消", "已踢出"}   # 终态=已消费完（绿并入/红拦截/562 孤儿取消/组链冲突踢出）
预验进行中 = {"排队", "执行中"}           # 非终态=可能正在跑·绝不删（红线：卡别人集成=事故）


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


def 读服务配置() -> dict:
    """queue_client.json = 队列服务地址+令牌（与 integrate.py 同源）。"""
    配置路径 = os.path.join(os.path.dirname(os.path.abspath(__file__)), "queue_client.json")
    with open(配置路径, encoding="utf-8") as f:
        return json.load(f)


def 预验服务端状态表() -> dict | None:
    """GET 8300 /api/state → {预验分支名: 状态}（预验任务表最新 50 行快照）。

    返回 None=服务不可达（调用方对预验段整体保守跳过·一条不删）；
    服务可达但某分支查无记录（快照 LIMIT 50 裁剪老行）→ 该分支走历史行查询再兜底。
    """
    try:
        配置 = 读服务配置()
        请求 = urllib.request.Request(
            配置["url"].rstrip("/") + "/api/state",
            headers={"Authorization": "Bearer " + 配置.get("令牌", "")})
        with urllib.request.urlopen(请求, timeout=8) as 响应:
            态 = json.loads(响应.read().decode("utf-8"))
        return {行.get("分支", ""): 行.get("状态", "") for 行 in (态.get("预验任务") or [])}
    except Exception:
        return None


def 预验历史状态(短名: str):
    """快照 miss 后按分支名 sha10 查历史行（/api/task_result·服务端全表保留 200 行）。

    分支名契约 ci/预验-<sha10> 本身携带 sha10（服务端 842 按 sha10 全表查·不受
    快照 LIMIT 50 裁剪）。返回状态字符串；None=查无记录/服务异常（调用方落 git
    证据判据·不视为错误——历史保留窗口外的更老分支本就该保守）。
    """
    sha10 = 短名[len(预验前缀):]
    if not re.fullmatch(r"[0-9a-f]{7,40}", sha10 or ""):
        return None
    try:
        配置 = 读服务配置()
        请求 = urllib.request.Request(
            配置["url"].rstrip("/") + "/api/task_result?sha=" + sha10,
            headers={"Authorization": "Bearer " + 配置.get("令牌", "")})
        with urllib.request.urlopen(请求, timeout=8) as 响应:
            详情 = json.loads(响应.read().decode("utf-8"))
        if not 详情.get("ok", True):   # 服务端 404 显式无此任务（648 miss 不静默·这里=合法查无）
            return None
        return 详情.get("状态")
    except Exception:
        return None


def 预验可删判定(记录状态, 已并入: bool) -> tuple:
    """单条 ci/预验-* 分支删除判据（纯函数·--selftest 覆盖面）。

    记录状态=None=服务端无该分支记录（快照裁剪/历史行淘汰/服务降级 ssh 直发的老分支）。
    进行中判定优先级最高：服务端记录为 排队/执行中 时即使 sha 已并入 develop 也保留
    （双保险——红线=不许删进行中的预验分支，判据失误会卡别的会话的集成）。
    返回 (可删?, 理由)。
    """
    if 记录状态 in 预验进行中:
        return False, f"服务端记录进行中（{记录状态}）——绝不删"
    if 记录状态 in 预验终态:
        return True, f"服务端记录已消费（{记录状态}）"
    # 记录缺失（None/未知状态）→ 仅 git 证据充分才删：分支内容已完整进 develop
    # ⇒ 该预验已绿已消费（绿轮幂等复用不重跑·进行中预验的 sha 不可能是 develop 祖先）。
    if 已并入:
        return True, "服务端无记录·sha 已并入 develop"
    return False, "服务端无记录且未并入 develop——保守保留"


def 预验段分类(全部预验: list) -> tuple:
    """ci/预验-* 段分类 → (可删清单, 保留清单[(分支, 理由)])。

    服务不可达=整段保守跳过（一条不删·任务纪律）；分支已并入判定用远端 ref
    完整 sha（merge-base --is-ancestor·无 sha10 前缀碰撞面）。
    """
    可删, 保留 = [], []
    状态表 = 预验服务端状态表()
    if 状态表 is None:
        print("[预验段] 队列服务不可达——保守跳过（一条不删）")
        return [], [(b, "服务不可达·保守跳过") for b in 全部预验]
    print(f"[预验段] 远程预验分支 {len(全部预验)} 条｜服务端预验快照 {len(状态表)} 行")
    历史查数 = 0
    for b in 全部预验:
        短名 = b.replace(f"{主远程}/", "", 1)
        已并入 = 运行(["git", "merge-base", "--is-ancestor", b,
                       f"{主远程}/{集成分支}"]).returncode == 0
        记录状态 = 状态表.get(短名)
        if 记录状态 is None:   # 快照裁剪的老分支→历史行全表查（200 行保留窗口内恢复终态证据）
            历史查数 += 1
            记录状态 = 预验历史状态(短名)
        判, 理由 = 预验可删判定(记录状态, 已并入)
        if 判:
            可删.append(b)
        else:
            保留.append((b, 理由))
    if 历史查数:
        print(f"[预验段] 快照 miss 走历史行查询 {历史查数} 条")  # 逐条串行 GET·服务正常时秒级
    return 可删, 保留


def 自测() -> int:
    """预验段删除判据纯函数自测（--selftest·不碰 git/网络）。"""
    用例们 = [
        (("排队", True), False, "排队=进行中·即使已并入也保留（优先级最高）"),
        (("执行中", True), False, "执行中=进行中·即使已并入也保留"),
        (("完成", False), True, "完成=已消费（绿并入/红拦截）"),
        (("已取消", False), True, "已取消=已消费（562 孤儿/手动取消）"),
        (("已踢出", False), True, "已踢出=已消费（组链冲突被踢）"),
        ((None, True), True, "无记录·已并入 develop=git 证据充分"),
        ((None, False), False, "无记录·未并入=保守保留"),
        (("", True), True, "异常空状态同无记录·已并入才删"),
        (("", False), False, "异常空状态·未并入=保守保留"),
    ]
    失败 = 0
    for (状态, 并入), 期望可删, 说明 in 用例们:
        可删, 理由 = 预验可删判定(状态, 并入)
        通过 = 可删 == 期望可删
        失败 += 0 if 通过 else 1
        print(f"  [{'PASS' if 通过 else 'FAIL'}] 状态={状态!r} 并入={并入} → 可删={可删}｜{说明}")
    print(f"自测 {len(用例们) - 失败}/{len(用例们)} 通过")
    return 1 if 失败 else 0


def 主流程() -> int:
    解析器 = argparse.ArgumentParser(description="远程任务/预验分支兜底清理（只删安全面·默认演练）")
    解析器.add_argument("--run", action="store_true", help="执行删除（缺省=演练只打印）")
    解析器.add_argument("--selftest", action="store_true", help="判据纯函数自测（不碰 git/网络）")
    参数 = 解析器.parse_args()
    if 参数.selftest:
        return 自测()

    print("[1] fetch --prune …")
    运行(["git", "fetch", "--prune", 主远程])

    远程分支 = 输出(["git", "branch", "-r", "--format=%(refname:short)"]).splitlines()
    全部 = [l for l in 远程分支 if l.startswith(f"{主远程}/任务/")]
    预验全部 = [l for l in 远程分支 if l.startswith(f"{主远程}/{预验前缀}")]
    保护 = 看板在飞分支()
    print(f"[2] 远程任务分支 {len(全部)} 条｜远程预验分支 {len(预验全部)} 条｜看板在飞/挂等保护 {len(保护)} 条")

    可删, 保留 = [], []
    # —— 任务段（710 原判据：看板保护 + 只删已并入 develop；864 补空分支保护）——
    develop头 = 输出(["git", "rev-parse", f"{主远程}/{集成分支}"])
    for b in 全部:
        短名 = b.replace(f"{主远程}/", "", 1)
        if 短名 in 保护 or b in 保护:
            保留.append((b, "看板在飞/挂等"))
            continue
        判定 = 运行(["git", "merge-base", "--is-ancestor", b, f"{主远程}/{集成分支}"])
        if 判定.returncode != 0:
            保留.append((b, "未并入 develop（差量交人工甄别·不自动删）"))
        elif 输出(["git", "rev-parse", b]) == develop头:
            # 864 补 710 盲区：分支头==develop 头=刚建未动工的在飞分支（分支存在=认领·
            # 382 后看板文件保护已空载·此为其替代防线）——删了会掐断认领广播，保留交下轮。
            保留.append((b, "空分支（头==develop 头·疑似刚建在飞）"))
        else:
            可删.append(b)
    # —— 预验段（864 新判据：只删已消费完·服务不可达整段跳过）——
    预验可删, 预验保留 = 预验段分类(预验全部)
    可删 += 预验可删
    保留 += 预验保留

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
