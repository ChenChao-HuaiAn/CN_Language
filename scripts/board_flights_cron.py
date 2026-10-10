#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""board_flights_cron.py —— 在飞分支实时上报器（382.2·TX_01 cron 每 3 分钟）。

382 用户令：「远端在飞分支」必须**实时体现真实的 git 分支**——不再依赖各机 AI
跑 claim 时机会主义上报（没人上报就停在旧时刻=显示缺支 BUG 实录 2026-10-10）。
本脚本部署 TX_01，gitcode 公开仓 https 匿名只读 ls-remote（免凭据·只读无风险），
把 任务/* 分支真相 POST 给本机 board_service（回环·令牌未配时回环放行）。

部署：scp 到 TX_01 /home/ubuntu/ + crontab：
  */3 * * * * python3 /home/ubuntu/board_flights_cron.py >> /home/ubuntu/flights_cron.log 2>&1
语义：走 /api/report_flights 全量替换（cron 是唯一高频写入者·快照永不过期）；
「已并入」判定废弃（分支收口时 integrate 即删远端·ls-remote 看不到=真相；
看得到=真残留该显示）。失败静默退出（下轮再试·cron 语义）。
"""
from __future__ import annotations

import json
import re
import subprocess
import sys
import urllib.request
from datetime import datetime

远端 = "https://gitcode.com/ChenChao_GitCode/CN_Language.git"
板地址 = "http://127.0.0.1:8301"


def 取令牌() -> str:
    """令牌获取序：环境变量 CN_BOARD_TOKEN → systemctl show cn-board 解析（同机
    ubuntu 用户可读·与 cn-queue 同一把锁）→ ~/.queue_token（TOKEN=xxx 键值格式）。"""
    import os
    t = os.environ.get("CN_BOARD_TOKEN", "").strip()
    if t:
        return t
    try:
        r = subprocess.run(["systemctl", "show", "cn-board", "--property=Environment"],
                           capture_output=True, text=True, timeout=10)
        m = re.search(r"CN_BOARD_TOKEN=(\S+)", r.stdout)
        if m:
            return m.group(1)
    except (OSError, subprocess.SubprocessError):
        pass
    try:
        for 行 in open(os.path.expanduser("~/.queue_token"), encoding="utf-8"):
            if 行.strip().startswith("TOKEN="):
                return 行.strip()[6:]
    except OSError:
        pass
    return ""


def 主要() -> int:
    r = subprocess.run(["git", "ls-remote", "--heads", 远端],
                       capture_output=True, text=True, timeout=60)
    if r.returncode != 0:
        print(f"{datetime.now():%m-%d %H:%M:%S} ls-remote 败: {r.stderr.strip()[:120]}")
        return 1
    分支们 = []
    for 行 in r.stdout.splitlines():
        m = re.match(r"^([0-9a-f]{40})\s+refs/heads/(任务/[0-9]+[a-z]?)$", 行.strip())
        if m:
            分支们.append({"分支": m.group(2), "提交": m.group(1)[:10],
                           "时刻": datetime.now().strftime("%m-%d %H:%M:%S"),
                           "已并入": False})
    体 = {"上报者": "TX_01-gitcron", "分支们": 分支们}
    头 = {"Content-Type": "application/json"}
    令牌 = 取令牌()
    if 令牌:
        头["Authorization"] = "Bearer " + 令牌
    请求 = urllib.request.Request(板地址 + "/api/report_flights", method="POST",
        data=json.dumps(体, ensure_ascii=False).encode("utf-8"), headers=头)
    with urllib.request.urlopen(请求, timeout=10) as resp:
        结果 = json.loads(resp.read().decode("utf-8"))
    print(f"{datetime.now():%m-%d %H:%M:%S} 上报 {len(分支们)} 支在飞分支：{结果}")
    return 0


if __name__ == "__main__":
    sys.exit(主要())
