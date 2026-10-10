#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# task_board.py —— 任务台账命令行客户端（382·看板 v2·服务端唯一权威）
#
# 382 架构裁决（2026-10-10 用户令）：任务台账唯一权威=看板服务端 SQLite 任务表
# （board_service:8301 /api/*·board_tasks.py），plans/021 文档已退位（冻结封存·
# AI 零读写）。本脚本由「021 本地解析器」转型为「服务端台账 CLI」：
#
#   --ready    就绪队列摘要（⬜ 且前置全 ✅·服务端判定+排序·人读）
#   --check    账实对账（服务端台账 ⇔ 远端分支·integrate 快速门禁挂载点）
#   --json     机读输出（--ready 配套·供 standup 等脚本消费）
#
# --check 对账面（正反两态·866 教训）：
#   🔴 台账 🏃 任务但远端无 任务/<号> 分支（认领未 push/漏销账面）
#   🔴 台账 ✅ 任务缺收口 sha（服务端已强制·此条=冗余防御）
#   🔴 前置引用不存在号 / 前置环（服务端立项不查环·此处兜底）
#   🟡 远端 任务/<号> 分支但台账无此任务（迁移窗口期旧任务·收口时补账）
#   🟡 台账 ⬜ 任务但远端已有分支（疑似认领未流转——wt.py create 补 🏃）
# 服务不可达：--ready/--json 报错 exit 1；--check 黄字降级 exit 0（门禁不卡集成）。
# 旧本地 021 解析（读全表/依赖图/跳号/归档合并）随 021 退位废除——编号唯一性由
# 服务端发号机械保证（board_tasks.py·撞号/跳号/禁用号/复用全在 /api/task_create 拦）。
#
# 用法：
#   python scripts/task_board.py --ready          # 开工前看一眼：该做什么
#   python scripts/task_board.py --check          # 提交前账实对账（integrate 挂同款）
#   python scripts/task_board.py --json           # 机读就绪队列

import json
import os
import re
import socket
import subprocess
import sys
import urllib.request
from pathlib import Path

板服务地址 = os.environ.get("CN_BOARD_URL", "http://124.222.106.84:8301").rstrip("/")


def 机名() -> str:
    return os.environ.get("CN_MACHINE_NAME", "").strip() or socket.gethostname()


def 服务调(路径: str):
    """GET 只读查询——不可达返回 None（调用方按场景降级/报错）。"""
    try:
        请求 = urllib.request.Request(板服务地址 + 路径, method="GET")
        with urllib.request.urlopen(请求, timeout=8) as r:
            return json.loads(r.read().decode("utf-8"))
    except Exception:
        return None


def 输出(命令: list[str]) -> str:
    结果 = subprocess.run(命令, capture_output=True, text=True, encoding="utf-8",
                          errors="replace")
    return (结果.stdout or "").strip()


def 拉台账() -> list | None:
    r = 服务调("/api/tasks")
    return None if r is None else r.get("任务们")


def cmd_ready(机读: bool) -> int:
    任务们 = 拉台账()
    if 任务们 is None:
        print(f"[失败] 看板服务不可达（{板服务地址}）——就绪队列需服务端台账（382）",
              file=sys.stderr)
        return 1
    if 机读:
        就绪们 = [{"号": t["号"], "标题": t["标题"], "优先级": t["优先级"],
                   "前置": t["前置"]} for t in 任务们 if t["就绪"]]
        print(json.dumps({"就绪们": 就绪们[:20], "全量数": len(任务们)},
                         ensure_ascii=False))
        return 0
    就绪们 = [t for t in 任务们 if t["就绪"]]
    在飞们 = [t for t in 任务们 if t["状态"] == "🏃"]
    挂起们 = [t for t in 任务们 if t["状态"] == "⏸"]
    print(f"# 任务台账（服务端唯一权威·共 {len(任务们)} 项：就绪 {len(就绪们)}"
          f"·在飞 {len(在飞们)}·挂起 {len(挂起们)}）")
    print("# 认领：wt.py create <号>——或看板网页「＋ 新建任务」立项")
    if not 就绪们:
        print("# （暂无就绪任务——所有待办的前置尚未完成）")
    for t in 就绪们[:12]:
        疑 = "（⚠ 远端已有分支·疑似认领中）" if t.get("疑似认领") else ""
        print(f"#{t['号']} [{t['优先级']}] {t['标题'][:64]}{疑}")
    if len(就绪们) > 12:
        print(f"…（另有 {len(就绪们) - 12} 项就绪·看板网页看全量）")
    return 0


def cmd_check() -> int:
    任务们 = 拉台账()
    if 任务们 is None:
        print(f"[黄] 看板服务不可达（{板服务地址}）——账实对账跳过（门禁降级放行·恢复后补查）")
        return 0
    远端任务分支 = set()
    for 引用 in 输出(["git", "ls-remote", "--heads", "gitcode"]).splitlines():
        m = re.match(r"^[0-9a-f]+\s+refs/heads/(?:任务|batch)/([0-9]+[a-z]?)$", 引用.strip())
        if m:
            远端任务分支.add(m.group(1))
    图 = {t["号"]: t for t in 任务们}
    红们, 黄们 = [], []
    for t in 任务们:
        号 = t["号"]
        if t["状态"] == "🏃" and 号 not in 远端任务分支:
            红们.append(f"#{号} 台账在飞(🏃)但远端无 任务/{号} 分支——认领未 push 或漏销账")
        if t["状态"] == "✅" and not t.get("收口sha"):
            if t.get("来源") == "迁移021":
                黄们.append(f"#{号} 台账已完成(✅)缺收口 sha——021 迁移旧账（226 立规前旧格式✅行本无 sha·知情放行）")
            else:
                红们.append(f"#{号} 台账已完成(✅)但缺收口 sha（✅⇔sha 铁律）")
        if t["状态"] == "⬜" and 号 in 远端任务分支:
            黄们.append(f"#{号} 台账待办(⬜)但远端已有分支——疑似认领未流转（wt.py create {号} 补 🏃）")
        for p in t.get("前置们", []):
            if p not in 图:
                红们.append(f"#{号} 前置 #{p} 不在台账——引用了不存在的任务号")
    for 号 in 远端任务分支:
        if 号 not in 图:
            黄们.append(f"远端 任务/{号} 分支存在但台账无此任务——迁移窗口期旧任务（收口时服务端补账）")
    # 前置环检测（服务端立项不查环·DFS 三色标记·首环即报）
    颜色: dict[str, int] = {}

    def 走(号: str) -> bool:
        颜色[号] = 1
        for p in 图.get(号, {}).get("前置们", []):
            if p not in 图:
                continue
            if 颜色.get(p) == 1:
                return True
            if 颜色.get(p, 0) == 0 and 走(p):
                return True
        颜色[号] = 2
        return False

    for 号 in 图:
        if 颜色.get(号, 0) == 0 and 走(号):
            红们.append(f"#{号} 处于前置环中——依赖永不可能解锁（服务端补账时须破环）")
            break
    if 红们:
        print("[账实检查] 🔴 红（须修复）：")
        for 红 in 红们:
            print("  " + 红)
    if 黄们:
        print("[账实检查] 🟡 黄（知情放行）：")
        for 黄 in 黄们:
            print("  " + 黄)
    if not 红们 and not 黄们:
        print(f"[账实检查] 全绿：台账 {len(任务们)} 项 ⇔ 远端 {len(远端任务分支)} 支任务分支对账一致")
    return 1 if 红们 else 0


def 路过续约() -> None:
    """382.1：跑台账脚本=AI 在干活的高频信号——后台线程给本会话意图续约
    （308k「路过即心跳」语义回归·治「AI 埋头干活 30 分钟不跑脚本→看板会话静默」
    的误导灰显）。静默失败不阻塞主流程。"""
    try:
        sys.path.insert(0, str(Path(__file__).resolve().parent))
        import intent as 看板
        看板.路过续约()
    except Exception:
        pass


def main() -> int:
    路过续约()
    if "--check" in sys.argv:
        return cmd_check()
    if "--json" in sys.argv:
        return cmd_ready(机读=True)
    return cmd_ready(机读=False)


if __name__ == "__main__":
    sys.exit(main())
