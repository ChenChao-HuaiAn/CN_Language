#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""intent.py —— 会话意图登记客户端（308a·board_service 配套 CLI）。

治 285 撞车实录的「意图不可见」：分支存在=认领只回答"号被占了"，不回答"谁、
哪个会话、正在做还是仅仅计划做"。本 CLI 把各机 AI 会话的在做/计划任务号登记到
board_service（TX_01:8301），网页看板实时可见+同号双声明冲突高亮——他机开工前
一眼看到"这个号已被深度机某会话在做"，打架死在源头。

用法：
  python scripts/intent.py claim 308a --planned 110,276 --备注 "看板服务·先做服务端"
  python scripts/intent.py refresh        # 心跳续约（重发最近一次登记内容）
  python scripts/intent.py release        # 会话收工注销
  python scripts/intent.py show           # 查看全服务端意图+冲突+在飞分支

降级原则（照 queue_service 不可达回退旧路径精神）：服务不可达时 claim/release/
refresh 黄字提示后 exit 0（不阻断开发工作——分支存在=认领仍是有效兜底）；
show 无法取数 exit 1。令牌复用 scripts/queue_client.json 的「令牌」字段
（同一把锁守同一栋楼·零新增配置）；URL 可用环境变量 CN_BOARD_URL 覆盖。
会话 id 首次自动生成持久于 ~/.cache/cn_board_session_id；机器名取 CN_MACHINE_NAME
环境变量（优先）或主机名。
"""
from __future__ import annotations

import argparse
import json
import re
import secrets
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
仓库根 = 脚本目录.parent
缓存目录 = Path.home() / ".cache"
会话文件 = 缓存目录 / "cn_board_session_id"
机器文件 = 缓存目录 / "cn_board_machine"
登记缓存 = 缓存目录 / "cn_board_intent_last.json"
默认地址 = "http://124.222.106.84:8301"


def 服务地址() -> str:
    import os
    return os.environ.get("CN_BOARD_URL", 默认地址).rstrip("/")


def 令牌() -> str:
    try:
        return json.loads((脚本目录 / "queue_client.json").read_text(encoding="utf-8")) \
            .get("令牌", "")
    except (OSError, ValueError):
        return ""


def 会话id() -> str:
    if not 会话文件.exists():
        会话文件.parent.mkdir(parents=True, exist_ok=True)
        会话文件.write_text(secrets.token_hex(2), encoding="utf-8")   # 4 位短 id·够三机区分
    return 会话文件.read_text(encoding="utf-8").strip()


def 机器名() -> str:
    import os
    import socket
    环境 = os.environ.get("CN_MACHINE_NAME", "").strip()
    if 环境:
        return 环境
    try:
        return 机器文件.read_text(encoding="utf-8").strip() or socket.gethostname()
    except OSError:
        return socket.gethostname()


def 调服务(方法: str, 路径: str, 体=None, 超时=8):
    请求 = urllib.request.Request(服务地址() + 路径, method=方法,
        data=json.dumps(体, ensure_ascii=False).encode("utf-8") if 体 is not None else None)
    if 令牌():
        请求.add_header("Authorization", f"Bearer {令牌()}")
    with urllib.request.urlopen(请求, timeout=超时) as r:
        return json.loads(r.read().decode("utf-8"))


def 上报在飞分支() -> None:
    """把本机看到的远端在飞任务分支上报给看板（尽力而为·失败静默——
    服务端不持 git 凭据不依赖外网，三机任一活着看板即有在飞数据）。"""
    try:
        r = subprocess.run(["git", "ls-remote", "--heads", "gitcode"],
                           capture_output=True, text=True, cwd=仓库根, timeout=30)
        分支们 = []
        for 行 in r.stdout.splitlines():
            m = re.match(r"^([0-9a-f]+)\s+refs/heads/(任务/.+)$", 行.strip())
            if m:
                分支们.append({"分支": m.group(2), "提交": m.group(1)[:10], "时刻": ""})
        if 分支们:
            调服务("POST", "/api/report_flights",
                   {"上报者": 机器名(), "分支们": 分支们}, 超时=5)
    except (OSError, ValueError, subprocess.SubprocessError):
        pass    # 尽力而为：上报失败不影响登记主流程
    try:
        # 308b：021 就绪队列快照（task_board --ready --json 机读·治看板空面板）
        r = subprocess.run([sys.executable, str(脚本目录 / "task_board.py"),
                            "--ready", "--json"],
                           capture_output=True, text=True, cwd=仓库根, timeout=60)
        行们 = json.loads(r.stdout).get("就绪们", [])
        if 行们:
            调服务("POST", "/api/report_021", {"行们": 行们}, 超时=5)
    except (OSError, ValueError, subprocess.SubprocessError):
        pass


def 落登记(在做: str, 计划: str, 备注: str) -> int:
    体 = {"机器": 机器名(), "对话id": 会话id(),
          "在做": 在做, "计划": 计划, "备注": 备注}
    try:
        调服务("POST", "/api/intent", 体)
        上报在飞分支()
        登记缓存.write_text(json.dumps(体, ensure_ascii=False), encoding="utf-8")
    except (urllib.error.URLError, OSError, ValueError) as e:
        print(f"[黄] 看板服务不可达（{服务地址()}）：{e}——意图未上板，"
              f"分支存在=认领仍是有效兜底（降级不阻断）")
        return 0
    print(f"[好] 意图已上板：{机器名()}-{会话id()} 在做#{在做 or '—'}"
          f" 计划[{计划 or '—'}]（心跳 30min 内 refresh 或重跑 claim 续约）")
    return 0


def 命令认领(参数) -> int:
    return 落登记(参数.任务号, 参数.计划, 参数.备注)


def 命令续约(_参数) -> int:
    if not 登记缓存.exists():
        print("[黄] 无历史登记可续约——先 claim")
        return 0
    旧 = json.loads(登记缓存.read_text(encoding="utf-8"))
    return 落登记(旧.get("在做", ""), 旧.get("计划", ""), 旧.get("备注", ""))


def 命令注销(_参数) -> int:
    try:
        调服务("POST", "/api/intent_release", {"机器": 机器名(), "对话id": 会话id()})
    except (urllib.error.URLError, OSError, ValueError) as e:
        print(f"[黄] 看板服务不可达：{e}——本地缓存已清，服务端行等心跳超时自然失联灰显")
        return 0
    print(f"[好] 已注销：{机器名()}-{会话id()}")
    return 0


def 命令查看(_参数) -> int:
    try:
        数据 = 调服务("GET", "/api/board")
    except (urllib.error.URLError, OSError, ValueError) as e:
        print(f"[失败] 看板服务不可达（{服务地址()}）：{e}")
        return 1
    print(f"会话意图（失联阈值 {数据.get('失联秒')}s·时刻 {数据.get('时刻')}）：")
    for i in 数据.get("意图们", []):
        状态 = "〔失联〕" if i["失联"] else ""
        print(f"  {i['机器']}-{i['对话id']} 在做#{i['在做'] or '—'} "
              f"计划[{i['计划'] or '—'}] 心跳{i['心跳时刻']}{状态}"
              f"{' ' + i['备注'] if i['备注'] else ''}")
    冲突们 = 数据.get("冲突们", [])
    print(f"冲突预警：{'无' if not 冲突们 else ''}")
    for c in 冲突们:
        print(f"  ⚠ #{c['号']} 被 {' 与 '.join(c['会话们'])} 同时声明")
    在飞 = 数据.get("在飞分支们", [])
    print(f"在飞分支（{len(在飞)}）：")
    for f in 在飞:
        标注 = "" if f.get("已登记意图") else "（未登记意图）"
        print(f"  {f['分支']} {f['提交']} {f['时刻']} {f['上报者']}{标注}")
    return 0


def main() -> int:
    解析器 = argparse.ArgumentParser(description="会话意图登记客户端（308a·看板配套）")
    子 = 解析器.add_subparsers(dest="命令", required=True)
    p领 = 子.add_parser("claim", help="登记/更新本会话意图（即心跳）")
    p领.add_argument("任务号", help="正在做的任务号（如 308a）")
    p领.add_argument("--planned", dest="计划", default="",
                     help="后续轮次计划做的任务号（逗号分隔·打架预警面）")
    p领.add_argument("--备注", default="", help="一句话说明（≤200 字）")
    子.add_parser("refresh", help="心跳续约（重发最近登记）")
    子.add_parser("release", help="会话收工注销")
    子.add_parser("show", help="查看全服务端意图+冲突+在飞分支")
    参数 = 解析器.parse_args()
    return {"claim": 命令认领, "refresh": 命令续约,
            "release": 命令注销, "show": 命令查看}[参数.命令](参数)


if __name__ == "__main__":
    sys.exit(main())
