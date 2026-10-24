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
  - 任务/*：只删「已并入 develop 且非空分支且不在台账在飞」的——
    · 台账保护（864 二轮·710 保护意图的正确替代数据源）：8301 服务端台账
      状态=🏃 的任务分支名一律不删（在飞任务·含认领后未推进的空壳分支——
      空壳分支头=旧 develop 点必为 develop 祖先，「已并入」判据对其恒真无
      甄别力，实测 731/777 等在飞分支被误删后立此防线）；服务不可达时任务段
      整体保守跳过一条不删；
    · 有差量（含 rebase 重写导致 cherry 误报）或分支头==develop 头（刚建未
      动工窗口期）的一律不动，交人工甄别；
  - ci/预验-*：只删「已消费完」的——服务端（8300 预验任务表·快照 miss 再走
    /api/task_result 历史行全表查）该记录状态 ∈ 终态{完成/已取消/已踢出}，或
    该分支 sha 已并入 develop，两者满足其一才删；服务端记录为 排队/执行中的
    一律保留（进行中预验绝不删——判据失误会卡别的会话的集成）；服务不可达时
    预验段整体保守跳过一条不删；
  - 默认 dry-run 只打印清单，--run 才真删。

用法：
  python scripts/branch_cleanup.py             # 演练：列出可删分支
  python scripts/branch_cleanup.py --run       # 执行删除（gitcode）
  python scripts/branch_cleanup.py --selftest  # 判据纯函数自测（不碰 git/网络）
"""

from __future__ import annotations   # P1-1：3.8 注解惰性化（dict | None 等定义期不求值）

import argparse
import json
import os
import re
import subprocess
import sys
import urllib.error
import urllib.request
from urllib.parse import quote

主远程 = "gitcode"
集成分支 = "develop"
预验前缀 = "ci/预验-"
# 预验任务表状态字段全部取值（queue_service.py 建表口径）：排队/执行中/完成/已取消/已踢出
预验终态 = {"完成", "已取消", "已踢出"}   # 终态=已消费完（绿并入/红拦截/562 孤儿取消/组链冲突踢出）
预验进行中 = {"排队", "执行中"}           # 非终态=可能正在跑·绝不删（红线：卡别人集成=事故）


def 运行(命令: list) -> subprocess.CompletedProcess:
    return subprocess.run(命令, capture_output=True)


def 输出(命令: list) -> str:
    return 运行(命令).stdout.decode("utf-8", "replace").strip()


def 读服务配置() -> dict:
    """queue_client.json = 队列服务地址（与 integrate.py 同源）；令牌不从此读——
    统一走 intent.鉴权头() 兜底链（本树→主树副本→环境变量·1425 单点）。"""
    配置路径 = os.path.join(os.path.dirname(os.path.abspath(__file__)), "queue_client.json")
    with open(配置路径, encoding="utf-8") as f:
        return json.load(f)


def _加鉴权头(请求) -> None:
    """1425/P2-A 全站登录墙：队列/看板 GET Bearer 写不可重定向头（跨源跳转不转发）；
    intent 助手不可用=stderr 一行诊断后裸请求。"""
    try:
        import intent
        intent.加鉴权头(请求)
    except Exception as e:
        print(f"[黄] intent 鉴权助手不可用，本次裸请求（诊断见上）：{e}", file=sys.stderr)


def _失败文案(服务: str, e: Exception) -> str:
    """读取失败文案分流（1425）：401/403=鉴权失败；连接层失败才叫「不可达」。"""
    if isinstance(e, urllib.error.HTTPError):
        鉴权失败 = e.code in (401, 403)   # P3-D：本地兜底
        try:
            import intent
            鉴权失败 = intent.鉴权失败码(e.code)
            if 鉴权失败:
                return intent.鉴权失败文案(服务, e.code)
        except Exception as ex:
            print(f"[黄] intent 鉴权助手不可用，按裸码兜底（诊断见上）：{ex}",
                  file=sys.stderr)
        if 鉴权失败:
            return f"{服务}鉴权失败（{e.code}）——令牌校验失败（详见上方诊断）"
        return f"{服务}返回 HTTP {e.code}"
    if isinstance(e, OSError):
        return f"{服务}不可达（{e}）"
    return f"{服务}响应异常（{type(e).__name__}: {e}）"


def 预验服务端状态表() -> dict | None:
    """GET 8300 /api/state → {预验分支名: 状态}（预验任务表最新 50 行快照）。

    返回 None=服务不可达（调用方对预验段整体保守跳过·一条不删）；
    服务可达但某分支查无记录（快照 LIMIT 50 裁剪老行）→ 该分支走历史行查询再兜底。
    """
    try:
        配置 = 读服务配置()
    except (OSError, ValueError, KeyError) as e:
        print(f"[预验段] 队列服务配置不可读（scripts/queue_client.json）：{e}——保守跳过")
        return None
    try:
        请求 = urllib.request.Request(配置["url"].rstrip("/") + "/api/state")
        _加鉴权头(请求)
        with urllib.request.urlopen(请求, timeout=8) as 响应:
            态 = json.loads(响应.read().decode("utf-8"))
        return {行.get("分支", ""): 行.get("状态", "") for 行 in (态.get("预验任务") or [])}
    except Exception as e:
        print(f"[预验段] {_失败文案('队列服务', e)}——保守跳过（一条不删）")
        return None


def 预验sha10(短名: str) -> str:
    """分支名 → sha10（#1221 复审 P0）：先剥 -m6<hex> 非幂等后缀再取前 10 位。

    复用 ci_round.预验分支sha10 单点解析；旧树无该模块（渐进部署）=本地同规则
    兜底。纯函数（selftest 面）。"""
    try:
        from ci_round import 预验分支sha10
        return 预验分支sha10(短名)
    except ImportError:   # 旧树无 ci_round（渐进部署）——本地同规则兜底
        return re.sub(r"-m6[0-9a-f]+$", "", str(短名 or "")[len(预验前缀):])[:10]


def 预验历史状态(短名: str):
    """快照 miss 后按分支名查历史行（/api/task_result·服务端全表保留 200 行）。

    分支名契约 ci/预验-<sha10>[-m6<hex>]（#1221 复审 P0：#1519 同族——M6 非幂等
    后缀必须先剥掉再取 sha10，否则正则不匹配→历史证据恒 None→只能走 git 判据）。
    复用 ci_round.预验分支sha10 单点解析；并带 branch=精确分支名（服务端先按名查，
    多 M6 重试行不串）。返回状态字符串；None=查无记录/服务异常（调用方落 git
    证据判据·不视为错误——历史保留窗口外的更老分支本就该保守）。
    """
    sha10 = 预验sha10(短名)
    if not re.fullmatch(r"[0-9a-f]{7,40}", sha10 or ""):
        return None
    try:
        配置 = 读服务配置()
        请求 = urllib.request.Request(
            配置["url"].rstrip("/") + "/api/task_result?sha=" + sha10
            + "&branch=" + quote(短名, safe=""))
        _加鉴权头(请求)
        with urllib.request.urlopen(请求, timeout=8) as 响应:
            详情 = json.loads(响应.read().decode("utf-8"))
        if not 详情.get("ok", True):   # 服务端 404 显式无此任务（648 miss 不静默·这里=合法查无）
            return None
        return 详情.get("状态")
    except Exception as e:
        # 401/403 不得静默成「查无记录」——点名鉴权失败后仍按无记录保守保留（安全方向）；
        # 404=正常查无、连接层失败=快照段已整段跳过，均不重复刷屏。
        if isinstance(e, urllib.error.HTTPError):
            try:
                import intent
                if intent.鉴权失败码(e.code):
                    print(f"[预验段] 历史查询 {intent.鉴权失败文案('队列服务', e.code)}"
                          f"——按无记录保守保留")
            except Exception:
                pass
        return None


def 台账在飞分支集解析(任务们: list) -> set:
    """8301 /api/tasks 响应「任务们」→ 在飞（🏃）任务的分支名集合（纯函数·--selftest 覆盖面）。

    「分支」字段为空的行兜底合成「任务/<号>」（服务端 379 行同款兜底·防旧数据
    漏保护）；⬜（待认领）/⏸（挂起）/✅（已销账）行不入保护面。
    """
    保护 = set()
    for 行 in 任务们 or []:
        if 行.get("状态") != "🏃":
            continue
        分支 = (行.get("分支") or "").strip()
        保护.add(分支 if 分支 else f"任务/{行.get('号')}")
    return 保护


def 台账在飞分支集() -> tuple[set | None, str]:
    """GET 8301 /api/tasks → (在飞分支集合, 失败文案)；失败时集合=None。

    864 二轮：710 的看板文件在飞保护自 382 看板退位后空载（本轮实测恒 0 条），
    服务端台账 🏃 行=其正确替代数据源。地址复用 board_cli.服务地址() 权威实现
    （CN_BOARD_URL 可覆盖）；1425 登录墙：GET 有令牌就带 Bearer（intent 单点）。
    """
    try:
        import board_cli
        请求 = urllib.request.Request(board_cli.服务地址().rstrip("/") + "/api/tasks")
        _加鉴权头(请求)
        with urllib.request.urlopen(请求, timeout=8) as 响应:
            数据 = json.loads(响应.read().decode("utf-8"))
        return 台账在飞分支集解析(数据.get("任务们") or []), ""
    except Exception as e:
        return None, _失败文案("看板台账（8301）", e)


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
        print("[预验段] 保守跳过（一条不删）")
        return [], [(b, "服务不可读·保守跳过") for b in 全部预验]
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
    """判据纯函数自测（--selftest·不碰 git/网络）——预验段删除判据+台账在飞解析。"""
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

    # #1221 复审 P0：M6 非幂等分支名 sha10 解析（旧普通名/新 -m6 名/非法名）
    for 名, 期望 in (("ci/预验-abcdef0123", "abcdef0123"),
                     ("ci/预验-abcdef0123-m6ab12cd", "abcdef0123"),
                     ("develop", "")):
        实 = 预验sha10(名)
        通过 = 实 == 期望
        失败 += 0 if 通过 else 1
        print(f"  [{'PASS' if 通过 else 'FAIL'}] sha10({名!r})={实!r}｜#1221 M6 后缀剥离")

    # 台账在飞解析（864 二轮·mock 8301 /api/tasks 响应行）
    台账用例们 = [
        ([{"号": "572", "状态": "🏃", "分支": "任务/572"}], {"任务/572"},
         "🏃 行分支入保护面（在飞正例）"),
        ([{"号": "777", "状态": "🏃", "分支": ""}], {"任务/777"},
         "🏃 行分支空→兜底合成 任务/<号>（空壳分支保护）"),
        ([{"号": "401", "状态": "✅", "分支": "任务/401"}], set(),
         "✅ 行不入保护面（已销账可删）"),
        ([{"号": "402", "状态": "⬜", "分支": ""}], set(),
         "⬜ 待认领行不入保护面（⬜ 行无远端分支）"),
        ([{"号": "403", "状态": "⏸", "分支": "任务/403"}], set(),
         "⏸ 挂起行不入保护面（用户令口径：只保 🏃）"),
        ([{"号": "572", "状态": "🏃", "分支": " 任务/572 "}], {"任务/572"},
         "分支字段带空白→strip 后入保护面"),
        ([{"号": "728x", "状态": "🏃", "分支": "任务/728x"},
          {"号": "731", "状态": "🏃", "分支": "任务/731"},
          {"号": "100", "状态": "✅", "分支": "任务/100"}], {"任务/728x", "任务/731"},
         "混合行只取 🏃（字母后缀号同规）"),
        (None, set(), "响应无任务们=空集合（不炸不误保）"),
    ]
    for 任务们, 期望集, 说明 in 台账用例们:
        实际 = 台账在飞分支集解析(任务们)
        通过 = 实际 == 期望集
        失败 += 0 if 通过 else 1
        print(f"  [{'PASS' if 通过 else 'FAIL'}] 台账解析 {说明}（得 {len(实际)} 条）")

    总数 = len(用例们) + len(台账用例们)
    print(f"自测 {总数 - 失败}/{总数} 通过")
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

    可删, 保留 = [], []
    # —— 任务段（864 二轮判据：台账在飞保护 + 只删已并入 develop 且非空分支）——
    # 710 的看板文件在飞保护自 382 看板退位后空载（实测恒 0 条·一轮实录 731/777 等
    # 在飞分支漏保护被误删），8301 服务端台账 🏃 行=其正确替代数据源；服务不可达=
    # 任务段整体保守跳过（一条不删——宁少删不误删）。
    在飞集, 台账错误 = 台账在飞分支集()
    if 在飞集 is None:
        print(f"[任务段] {台账错误}——保守跳过（一条不删）")
        保留 += [(b, "台账不可读·保守跳过") for b in 全部]
    else:
        develop头 = 输出(["git", "rev-parse", f"{主远程}/{集成分支}"])
        print(f"[任务段] 远程任务分支 {len(全部)} 条｜台账在飞（🏃）保护 {len(在飞集)} 条")
        for b in 全部:
            短名 = b.replace(f"{主远程}/", "", 1)
            if 短名 in 在飞集 or b in 在飞集:
                保留.append((b, "台账在飞（🏃）——绝不删"))
                continue
            判定 = 运行(["git", "merge-base", "--is-ancestor", b, f"{主远程}/{集成分支}"])
            if 判定.returncode != 0:
                保留.append((b, "未并入 develop（差量交人工甄别·不自动删）"))
            elif 输出(["git", "rev-parse", b]) == develop头:
                # 864 补 710 盲区：分支头==develop 头=刚建未动工窗口期（台账可能尚未
                # 同步可见）——保留交下轮，宁可少删一轮。
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
