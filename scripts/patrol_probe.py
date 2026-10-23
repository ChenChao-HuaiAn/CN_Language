#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""patrol_probe.py —— 接棒判死「先打招呼」探询客户端（任务 728l·教训 #150）。

背景：看板巡检自动接棒以「静默 27min + 无进程 + 改动冻结」三信号判死，但实录
（教训 #150·535 二次派工撞车）证明 Agent 长推理/大段写码可静默超 27 分钟——期间
无任何工具调用，三信号全静默也未必死亡。本脚本把「判死与接管之间」的缓冲确认
机械化：三信号命中后**不得直接接管**，顺序=探询→等待窗口→复查第四信号→才可接管。

协议全文（含巡检 prompt 增补文本）= plans/028 §十四「接棒判死探询协议」：
  1. 探询：python scripts/patrol_probe.py 探询 <号> [--窗口 30] [--理由 "…"] [--交接]
     任务备注追加带时间戳探询行（服务端备注=整字段替换语义·本脚本先读旧再追加，
     不覆盖历史），并落本地快照（~/.cache/cn_patrol_probe_<号>.json·复查精度增强）。
  2. 等待：窗口期内不做任何接管动作（默认 30 分钟·长推理高风险任务建议 --窗口 45）。
  3. 复查：python scripts/patrol_probe.py 复查 <号>
     采集第四信号：S1 任务更新时戳 / S2 意图心跳 / S3 远端分支新提交 / S4 本机树
     活动（源码 mtime + 树锁心跳）/ S5 本机进程引用——任一出现新活动=活跃放行（不
     接管）；窗口满且全部可得信号静默=静默确认（可接管）；看板服务不可达=未知不接管。
  4. 接管：仅复查 exit 0（静默确认）才执行原接管流程（wt.py create <号> --takeover）。

exit code：探询 0=探询行已写入·1=未写入（不得接管）；复查 0=静默确认可接管·
1=不接管（活跃放行/观察中/未知/非在飞）——巡检自动化以 exit code 为机械闸门。

降级铁律（fail-safe·宁慢勿误）：看板服务不可达时探询写不进、任务与意图信号读不到
——一律禁止接管，恢复后重试（漏报仅延迟接管·误接管撞车双倍损失）。
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
仓库根 = 脚本目录.parent
缓存目录 = Path.home() / ".cache"
默认窗口分 = 30          # 探询窗口默认时长（任务 728l 建议 15~30·长推理高风险 --窗口 45）
备注上限 = 2000          # 服务端备注字段上限（board_tasks 净化截断线·超限禁写防截沿）
更新容差秒 = 90          # 无快照时排除探询自身写入的容差（分钟精度+写入延迟）
活动容差秒 = 60          # 树/脉动信号基准回退（探询时刻为分钟精度·防同分钟边界误判）
树锁新鲜秒 = 180         # 树锁心跳新鲜窗口（gate_lock 树域 3 个 60s 心跳周期）
扫描最大深度 = 4         # 本机树源码 mtime 扫描深度（覆盖 scripts/src/tests/e2e 各层）
跳过目录名 = {"target", "v2p", "node_modules", "build", "out", "__pycache__"}
探询正则 = re.compile(r"\[探询 (\d{2}-\d{2} \d{2}:\d{2})([^\]]*)\]")
状态_在飞 = "🏃"         # 与 board_tasks 同源（服务端「待接棒」口径：仅 🏃 适用接管）
判决_活跃, 判决_观察, 判决_静默, 判决_未知, 判决_非在飞 = (
    "活跃放行", "观察中", "静默确认", "未知不接管", "非在飞不适用")

计数 = {"通过": 0, "失败": []}


def 签(名称: str, 条件: bool, 详情: str = "") -> None:
    if 条件:
        计数["通过"] += 1
        print(f"  [签] {名称}")
    else:
        计数["失败"].append(名称)
        print(f"  [✗] {名称} {详情}")


def 好(文本: str) -> None:
    print(f"[好] {文本}")


def 黄(文本: str) -> None:
    print(f"[黄] {文本}")


def 失败(文本: str) -> None:
    print(f"[失败] {文本}")


# ===== 服务访问（令牌链/机器名/HTTP 契约全部复用既有实现·归纳零复制）=====

def 服务调用(方法: str, 路径: str, 体=None, 超时=15):
    """委托 board_cli.调服务（(码,响应) 契约·Bearer 令牌链与 Content-Type 复用）。"""
    import board_cli
    return board_cli.调服务(方法, 路径, 体, 超时)


def 机器名(显式: str = "") -> str:
    import intent
    return 显式 or intent.机器名()


def 读任务(号: str):
    """→ (任务dict|None, 服务可达bool, 错误文本, 鉴权失败bool)。404=可达但任务不存在。

    1425 登录墙：401/403 单独置鉴权失败位——调用方据此打印「鉴权失败」而非
    「任务不存在」/「服务不可达」（真因不吞）。"""
    try:
        码, 响应 = 服务调用("GET", f"/api/task/{号}")
    except (OSError, ValueError) as e:
        return None, False, str(e), False
    if 码 in (401, 403):
        try:
            import intent
            文案 = intent.鉴权失败文案("看板服务", 码)
        except Exception:
            文案 = f"看板服务鉴权失败（{码}）——检查令牌/登录墙"
        return None, True, 文案, True
    if 码 != 200:
        return None, True, str(响应.get("错误") or f"HTTP {码}"), False
    return 响应.get("任务") or {}, True, "", False


def 写备注(号: str, 备注: str):
    """POST /api/task_update 只带 备注（服务端整字段替换→调用方先读旧再追加）；
    不传状态=不触发服务端认领上板链路。→ (码, 响应)。"""
    return 服务调用("POST", "/api/task_update", {"号": 号, "备注": 备注})


# ===== 探询行：构造 / 解析 / 备注追加 =====

def 构造探询行(时刻文本: str, 窗口: int, 基线: str, 来源: str, 理由: str,
              紧凑: bool = False) -> str:
    """探询行格式（可解析头 + 人类说明尾）：
    [探询 10-14 07:30 窗口30 基线a1b2c3d4 来源机器] 说明文本…"""
    来源 = re.sub(r"\s+", "_", (来源 or "未知").strip()) or "未知"
    头 = f"[探询 {时刻文本} 窗口{窗口} 基线{基线 or '无'} 来源{来源}]"
    if 紧凑:   # 备注接近 2000 上限时的降级形态（仍可解析全部四要素）
        return f"{头} 三信号静默探询：请回话（备注/分支/心跳任一即放行），逾期按接管流程接棒"
    尾 = ("巡检判死三信号静默（心跳/进程/改动冻结）——请所有者回话：更新备注、推分支或"
           "续意图心跳任一即自动放行；逾期无任何第四信号将按接管流程接棒")
    if 理由:
        尾 += f"（理由：{理由[:120]}）"
    return f"{头} {尾}"


def 解析探询行(备注: str):
    """取最后一条 [探询 …] → dict；无 → None。token 缺失豁免边界：备注被服务端
    上限截断时窗口 token 可能丢失——回退构造端同名默认常量（不缩短等待时长·
    方向保守），基线/来源缺失则显式转「不可比/未知」由复查信号层分流。"""
    匹配们 = list(探询正则.finditer(备注 or ""))
    if not 匹配们:
        return None
    m = 匹配们[-1]
    窗口, 基线, 来源 = 默认窗口分, "", ""
    for token in m.group(2).split():
        if token.startswith("窗口") and token[2:].isdigit():
            窗口 = int(token[2:])
        elif token.startswith("基线"):
            基线 = token[2:]
        elif token.startswith("来源"):
            来源 = token[2:]
    return {"时刻文本": m.group(1), "窗口": 窗口, "基线": 基线,
            "来源": 来源, "原文": m.group(0)}


def 追加备注(旧备注: str, 新行: str, 上限: int = 备注上限):
    """空格拼接追加（服务端净化把换行折空格·备注本就是单行语义）；超上限返回
    None=显式拒绝——禁服务端静默截断把探询行截掉（那会让复查失去留痕依据）。"""
    旧 = (旧备注 or "").strip()
    if not 旧:
        return 新行 if len(新行) <= 上限 else None
    候选 = f"{旧} {新行}"
    return 候选 if len(候选) <= 上限 else None


# ===== 时刻工具（探询行分钟精度 ↔ epoch 双向）=====

def 时刻文本() -> str:
    return datetime.now().strftime("%m-%d %H:%M")


def 时刻到时戳(文本: str):
    """'MM-DD HH:MM' → epoch；非法返回 None（显式不可得·调用方禁静默兜底）。"""
    m = re.match(r"(\d{2})-(\d{2}) (\d{2}):(\d{2})", 文本 or "")
    if not m:
        return None
    月, 日, 时, 分 = (int(x) for x in m.groups())
    现在 = datetime.now()
    try:
        dt = datetime(现在.year, 月, 日, 时, 分)
        if (dt - 现在).total_seconds() > 180 * 86400:   # 跨年回看（探询窗口 ≤1h·此判安全）
            dt = datetime(现在.year - 1, 月, 日, 时, 分)
    except ValueError:
        return None
    return dt.timestamp()


def 时戳到文本(时戳: float) -> str:
    return datetime.fromtimestamp(时戳).strftime("%m-%d %H:%M")


# ===== 本地快照（复查精度增强·非留痕权威——权威=服务端备注探询行）=====

def 快照路径(号: str) -> Path:
    return 缓存目录 / f"cn_patrol_probe_{号}.json"


def 写快照(号: str, 数据: dict) -> None:
    try:
        缓存目录.mkdir(parents=True, exist_ok=True)
        快照路径(号).write_text(json.dumps(数据, ensure_ascii=False), encoding="utf-8")
    except OSError as e:
        黄(f"本地快照写入失败（{e}）——复查退无快照口径（仅精度面·探询行留痕不受影响）")


def 读快照(号: str):
    try:
        return json.loads(快照路径(号).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None      # 无快照/坏快照=无快照口径（调用方注释说明降级边界）


# ===== 第四信号采集（S3~S5 走 git/本机·看板服务挂时仍可得）=====

def 远端分支sha(号: str):
    """(可得, sha前10|'无'|'不可得')——直连 gitcode·不依赖看板服务。"""
    try:
        r = subprocess.run(["git", "ls-remote", "gitcode", f"refs/heads/任务/{号}"],
                           capture_output=True, text=True, cwd=仓库根, timeout=30)
    except (OSError, subprocess.SubprocessError):
        return False, "不可得"
    if r.returncode != 0:
        return False, "不可得"
    行 = r.stdout.strip()
    return (True, 行.split()[0][:10]) if 行 else (True, "无")


def 本机树路径(号: str):
    """本机 worktree 中分支=任务/<号> 的树路径（wt.py 建的树都在 worktree 列表）。"""
    try:
        r = subprocess.run(["git", "worktree", "list", "--porcelain"],
                           capture_output=True, text=True, cwd=仓库根, timeout=15)
    except (OSError, subprocess.SubprocessError):
        return None
    树 = None
    for 段 in r.stdout.split("\n\n"):
        路径 = ""
        for 行 in 段.splitlines():
            if 行.startswith("worktree "):
                路径 = 行[len("worktree "):].strip()
            elif 行.strip() == f"branch refs/heads/任务/{号}" and 路径:
                树 = Path(路径)
    return 树


def 树最新mtime(树: Path, 最大深度: int = 扫描最大深度):
    """树内源文件最新 mtime（排除构建缓存 target/v2p、Python 缓存 __pycache__ 与
    点开头目录）——写码活动信号；脚本运行副产物不进判定，防自噪声假活跃。"""
    最新 = None

    def 走(目录: Path, 深度: int) -> None:
        nonlocal 最新
        try:
            项们 = list(os.scandir(目录))
        except OSError:
            return
        for 项 in 项们:
            if 项.name.startswith(".") or 项.name in 跳过目录名:
                continue
            try:
                if 项.is_dir(follow_symlinks=False):
                    if 深度 < 最大深度:
                        走(Path(项.path), 深度 + 1)
                elif 项.is_file(follow_symlinks=False):
                    mt = 项.stat(follow_symlinks=False).st_mtime
                    if 最新 is None or mt > 最新:
                        最新 = mt
            except OSError:
                continue

    走(树, 0)
    return 最新


def 树锁心跳mtime(树: Path):
    """树锁 info.json mtime（gate_quick/run_e2e 持锁期 60s 心跳触碰）——构建/E2E 在跑。"""
    try:
        return (树 / "target" / "tree.lock" / "info.json").stat().st_mtime
    except OSError:
        return None


def 进程引用(号: str, 树):
    """ps 快照中命令行引用树路径或 任务/<号> 的进程。win 无 ps=不可得。排除面：
    巡检自身进程祖链（bash→python 全链，防把巡检者 shell 当工作进程）+ 命令行含
    patrol_probe 的进程（本工具链）。教训 #150④：长跑命令期间也可能无树内进程
    ——本信号只作正向活动证据（有=活），无=不构成死亡证明。"""
    try:
        r = subprocess.run(["ps", "-eo", "pid=,ppid=,args="],
                           capture_output=True, text=True, timeout=10)
    except (OSError, subprocess.SubprocessError):
        return False, []
    段们 = [行.strip().split(None, 2) for 行 in r.stdout.splitlines()]
    父表 = {int(段[0]): int(段[1]) for 段 in 段们
            if len(段) >= 2 and 段[0].isdigit() and 段[1].isdigit()}
    排除, 游标 = set(), os.getpid()
    while 游标 and 游标 not in 排除:
        排除.add(游标)
        游标 = 父表.get(游标, 0)
    模式们 = [f"任务/{号}"] + ([str(树)] if 树 else [])
    命中 = []
    for 段 in 段们:
        if len(段) < 3:
            continue
        pid, 命令 = int(段[0]), 段[2]
        if pid in 排除 or "patrol_probe" in 命令:
            continue
        if any(p in 命令 for p in 模式们):
            命中.append(命令[:100])
        if len(命中) >= 3:
            break
    return True, 命中


def 意图心跳(号: str, 基线时戳: float):
    """→(可得, 活动, 说明)。/api/board 意图行 在做=号 且心跳时戳>基线=活动。"""
    try:
        码, 数据 = 服务调用("GET", "/api/board", 超时=10)
        if 码 != 200:
            return False, False, f"不可得（HTTP {码}）"
    except (OSError, ValueError) as e:
        return False, False, f"不可得（{e}）"
    相关 = [i for i in (数据.get("意图们") or []) if (i.get("在做") or "") == 号]
    if not 相关:
        return False, False, "无该号在飞意图行（会话未登记·不可判）"
    for i in 相关:
        if float(i.get("时戳") or 0) > 基线时戳:
            return True, True, f"心跳续约：{i.get('会话键')} 心跳 {i.get('心跳时刻', '')}"
    行 = 相关[0]
    失联 = "已失联" if 行.get("失联") else f"失联秒 {行.get('失联秒')}"
    return True, False, f"{len(相关)} 行意图均无新心跳（{行.get('会话键')} 心跳停 {行.get('心跳时刻', '')}·{失联}）"


def 本机树活动(树, 基准时戳: float):
    """→(可得, 活动, 说明)。树锁心跳（构建/E2E 在跑）与源码 mtime（在写码）双判据。"""
    if 树 is None:
        return False, False, "本机无该号树（信号在他机或已删）"
    依据 = []
    锁m = 树锁心跳mtime(树)
    if 锁m and 锁m > 基准时戳:
        依据.append(f"树锁心跳 {时戳到文本(锁m)}（构建/E2E 在跑）")
    鲜 = 树最新mtime(树)
    if 鲜 and 鲜 > 基准时戳:
        依据.append(f"树内文件 mtime {时戳到文本(鲜)}（在写码）")
    if 依据:
        return True, True, "；".join(依据)
    尾 = []
    if 锁m:
        尾.append(f"树锁心跳停在 {时戳到文本(锁m)}")
    if 鲜:
        尾.append(f"树内文件 mtime 停在 {时戳到文本(鲜)}")
    return True, False, "；".join(尾) or "树为空或扫描不可读"


def 采集信号(号: str, 探询时戳: float, 基线sha: str, 快照, 任务: dict,
            服务可达: bool) -> list:
    """五路第四信号 → [{'名','可得','活动','说明'}]。基准=探询时戳-活动容差秒。"""
    基准 = 探询时戳 - 活动容差秒
    信号们 = []
    # S1 任务更新时戳（服务端权威·探询自身写入用快照记录的写入时戳排除）
    if not 服务可达:
        信号们.append({"名": "S1 任务更新", "可得": False, "活动": False,
                       "说明": "不可得（看板服务不可达）"})
    else:
        排线 = (float(快照.get("更新时戳") or 0) if 快照 else 0.0) or (探询时戳 + 更新容差秒)
        现 = float(任务.get("更新时戳") or 0)
        活动 = 现 > 排线 + 0.5
        信号们.append({"名": "S1 任务更新", "可得": True, "活动": 活动,
                       "说明": f"更新时刻 {任务.get('更新时刻', '')}——"
                               f"{'探询后另有更新（有人回话/动账）' if 活动 else '探询后无新更新'}"})
    # S2 意图心跳
    可得2, 活动2, 说明2 = 意图心跳(号, 探询时戳) if 服务可达 else (False, False, "不可得（看板服务不可达）")
    信号们.append({"名": "S2 意图心跳", "可得": 可得2, "活动": 活动2, "说明": 说明2})
    # S3 远端分支新提交
    可得3, 现sha = 远端分支sha(号)
    if not 可得3:
        信号们.append({"名": "S3 远端分支", "可得": False, "活动": False, "说明": 现sha})
    elif 基线sha in ("", "不可得"):
        信号们.append({"名": "S3 远端分支", "可得": False, "活动": False,
                       "说明": f"探询时基线不可得——本轮 sha {现sha} 不可比"})
    else:
        活动3 = 现sha != 基线sha
        信号们.append({"名": "S3 远端分支", "可得": True, "活动": 活动3,
                       "说明": f"{基线sha} → {现sha}——"
                               f"{'有新提交（分支被推进）' if 活动3 else '无新提交'}"})
    # S4/S5 共享一次本机树定位（免重复 git worktree list 子进程）
    树 = 本机树路径(号)
    # S4 本机树活动（源码 mtime + 树锁心跳）
    可得4, 活动4, 说明4 = 本机树活动(树, 基准)
    信号们.append({"名": "S4 本机树活动", "可得": 可得4, "活动": 活动4, "说明": 说明4})
    # S5 本机进程引用
    可得5, 命中 = 进程引用(号, 树)
    if not 可得5:
        信号们.append({"名": "S5 本机进程", "可得": False, "活动": False,
                       "说明": "不可得（无 ps·win 面）"})
    else:
        信号们.append({"名": "S5 本机进程", "可得": True, "活动": bool(命中),
                       "说明": "；".join(命中) if 命中 else "无引用该树/分支的进程"})
    return 信号们


# ===== 判决（纯函数·可离线自测）=====

def 判决(信号们: list, 探询时戳: float, 窗口: int, 服务可达: bool, 任务状态: str,
        现在: float = None):
    """→ (结论, 依据列表)。顺序=活动事实 > 状态适用性 > 服务可达 > 窗口 > 静默。
    静默确认只要求「服务可达（S1/S2 可得）+ 全部可得信号无活动」——S3~S5 在他机
    本就不可得，不可得≠有活动；窗口期正是给所有者的回话机会。"""
    现在 = 现在 if 现在 is not None else time.time()
    截止 = 探询时戳 + 窗口 * 60
    活动们 = [f"{s['名']}：{s['说明']}" for s in 信号们 if s.get("活动")]
    if 活动们:
        return 判决_活跃, 活动们
    if 任务状态 and 任务状态 != 状态_在飞:
        return 判决_非在飞, [f"任务状态 {任务状态}（非 🏃 在飞）——服务端「待接棒」口径仅适用在飞"]
    if not 服务可达:
        return 判决_未知, ["看板服务不可达：任务更新/意图心跳不可得——fail-safe 不接管"]
    缺 = [s["名"] for s in 信号们 if not s.get("可得")]
    if 现在 < 截止:
        剩 = max(1, int((截止 - 现在 + 59) // 60))
        注 = f"（信号缺：{'、'.join(缺)}）" if 缺 else ""
        return 判决_观察, [f"窗口未满：截止 {时戳到文本(截止)}·剩余约 {剩} 分钟{注}"]
    return 判决_静默, ["窗口已满且全部可得信号无新活动"] + [
        f"{s['名']}：{s['说明']}" for s in 信号们]


# ===== 子命令 =====

def 命令探询(a) -> int:
    if a.窗口 < 1 or a.窗口 > 600:
        失败("--窗口 须在 1~600 分钟")
        return 2
    任务, 可达, 错误, 鉴权败 = 读任务(a.号)
    if not 可达:
        黄(f"看板服务不可达（{错误}）——探询行写不进=不得接管（fail-safe·恢复后重试）")
        return 1
    if 鉴权败:
        失败(f"{错误}——探询行写不进=不得接管（fail-safe·修复令牌后重试）")
        return 1
    if not 任务:
        失败(f"任务 {a.号} 不存在（{错误}）——先立项再探询")
        return 1
    状态 = str(任务.get("状态") or "")
    if 状态 != 状态_在飞:
        黄(f"任务 {a.号} 当前状态 {状态}（非在飞 🏃）——探询对象通常是在飞任务，仍写入")
    来源 = 机器名(a.机器)
    可得, 基线 = 远端分支sha(a.号)
    基线文本 = 基线 if (可得 and 基线) else "不可得"
    现在, 文本 = time.time(), 时刻文本()
    新备注, 形态 = None, "完整"
    for 紧凑 in (False, True):
        行 = 构造探询行(文本, a.窗口, 基线文本, 来源, a.理由, 紧凑)
        新备注 = 追加备注(任务.get("备注", ""), 行)
        if 新备注 is not None:
            形态 = "完整" if not 紧凑 else "紧凑"
            break
    if 新备注 is None:
        失败(f"任务备注已达上限（{len(任务.get('备注', ''))}/{备注上限}）——探询行写不进"
             f"即不得接管；请人工清理备注或改用 --交接 通道后重试")
        return 1
    码, 响应 = 写备注(a.号, 新备注)
    if 码 != 200:
        失败(f"备注写入失败：{码} {响应.get('错误', 响应)}")
        return 1
    写时戳 = float((响应.get("任务") or {}).get("更新时戳") or 现在)
    写快照(a.号, {"探询时戳": 现在, "更新时戳": 写时戳, "窗口": a.窗口,
                  "基线": 基线文本, "机器": 来源, "时刻文本": 文本})
    截止 = 时戳到文本(现在 + a.窗口 * 60)
    好(f"探询行已写入 #{a.号} 备注（{形态}形态·窗口 {a.窗口}m·截止 {截止}·基线{基线文本}）")
    if a.交接:
        命令探询交接(a, 截止, 基线文本, 来源)
    print(f"[提示] 等待窗口后运行：python scripts/patrol_probe.py 复查 {a.号}（exit 0 才允许接管）")
    return 0


def 命令探询交接(a, 截止: str, 基线文本: str, 来源: str) -> None:
    """交接区全局注目通道（可选·失败不阻断——备注通道已留痕）。"""
    条目 = (f"【探询 {a.号}】巡检判死三信号静默——已落探询行到任务备注（窗口 {a.窗口}m·"
            f"截止 {截止}·基线{基线文本}）；请所有者回话（备注/推分支/续意图心跳任一即"
            f"自动放行）；逾期无第四信号按接管流程接棒。"
            f"复查=python scripts/patrol_probe.py 复查 {a.号}")
    try:
        码, 响应 = 服务调用("POST", "/api/handoff_add",
                            {"机器": 来源, "条目": 条目, "来源": "patrol_probe"})
        好("交接区探询条已登记") if 码 in (200, 201) else 黄(f"交接登记失败（{码}）——备注通道已留痕，不阻断")
    except (OSError, ValueError) as e:
        黄(f"交接登记失败（{e}）——备注通道已留痕，不阻断")


def 命令复查(a) -> int:
    任务, 可达, 错误, 鉴权败 = 读任务(a.号)
    if not 可达:
        return 服务不可达复查(a.号, 错误)
    if 鉴权败:
        黄(f"{错误}——探询行/任务/意图信号均不可得")
        print("[fail-safe] 鉴权失败=不接管（宁慢勿误）：修复令牌后重试 复查")
        return 1
    if not 任务:
        失败(f"任务 {a.号} 不存在（{错误}）")
        return 1
    状态 = str(任务.get("状态") or "")
    备注 = str(任务.get("备注") or "")
    探询 = 解析探询行(备注)
    if not 探询:
        失败(f"#{a.号} 备注无探询行——禁止跳过探询直接接管；先运行："
             f"python scripts/patrol_probe.py 探询 {a.号}")
        return 1
    快照 = 读快照(a.号)
    探询时戳 = float(快照.get("探询时戳") or 0) if 快照 else 0.0
    if not 探询时戳:
        探询时戳 = 时刻到时戳(探询["时刻文本"]) or 0.0
    if not 探询时戳:
        失败("探询行时刻不可解析——留痕损坏，不得接管")
        return 1
    print(f"[任务] #{a.号} 状态{状态} {str(任务.get('标题') or '')[:60]}"
          f"（更新 {任务.get('更新时刻', '')}）")
    print(f"[探询] {探询['原文'][:130]}")
    信号们 = 采集信号(a.号, 探询时戳, 探询["基线"], 快照, 任务, True)
    for s in 信号们:
        标 = "活动" if s["活动"] else ("无活动" if s["可得"] else "不可得")
        print(f"[信号] {s['名']}：{标}——{s['说明']}")
    结论, 依据 = 判决(信号们, 探询时戳, 探询["窗口"], True, 状态)
    if 结论 == 判决_静默:
        print(f"[判决·{结论}] " + "；".join(依据))
        print(f"[接管] 允许按原接管流程执行：python scripts/wt.py create {a.号} --takeover"
              f"（二次派工任务书须含「发现同树活跃会话立即停手升级请示」条款·教训 #150③）")
        return 0
    print(f"[判决·{结论}] " + "；".join(依据) + "——不接管")
    if 结论 == 判决_观察:
        print(f"[提示] 窗口满后再运行：python scripts/patrol_probe.py 复查 {a.号}")
    elif 结论 == 判决_非在飞:
        print("[提示] 非在飞任务不适用接管流程——如为僵尸行请人工核实置 ✅ 或重开")
    return 1


def 服务不可达复查(号: str, 错误: str) -> int:
    """fail-safe：服务不可达=探询行/任务/意图全部读不到——不接管，恢复后重试。
    尽力打印 git/本机现状供人工参考（现状≠探询复查·不作接管依据）。"""
    黄(f"看板服务不可达（{错误}）——探询行/任务/意图信号均不可得")
    print("[fail-safe] 服务不可达=不接管（宁慢勿误）：恢复后重试 复查")
    可得, sha = 远端分支sha(号)
    print(f"[现状] 远端分支：{'sha ' + sha if 可得 else '不可得'}")
    树 = 本机树路径(号)
    if 树:
        锁m = 树锁心跳mtime(树)
        if 锁m:
            鲜 = time.time() - 锁m < 树锁新鲜秒
            隔 = int(time.time() - 锁m)
            print(f"[现状] 树锁心跳：{'新鲜——构建/E2E 疑在跑（强烈建议不接管）' if 鲜 else f'停在 {隔}s 前'}")
        可得p, 命中 = 进程引用(号, 树)
        print(f"[现状] 本机进程：{'命中——' + '；'.join(命中) if 命中 else ('无命中' if 可得p else '不可得')}")
    else:
        print("[现状] 本机无该号树（进程/树锁信号不在本机）")
    return 1


# ===== 自测 =====

def 自测离线() -> None:
    计数["通过"], 计数["失败"] = 0, []
    print("[离线] 探询行构造/解析/追加/判决纯逻辑断言")
    行 = 构造探询行("10-14 07:30", 30, "abc1234def", "测试机", "自测理由")
    解 = 解析探询行("旧备注 " + 行)
    签("探询行往返：时刻", bool(解) and 解["时刻文本"] == "10-14 07:30")
    签("探询行往返：窗口", bool(解) and 解["窗口"] == 30)
    签("探询行往返：基线", bool(解) and 解["基线"] == "abc1234def")
    签("探询行往返：来源", bool(解) and 解["来源"] == "测试机")
    签("无探询行=None", 解析探询行("普通备注无标记") is None)
    两条 = (构造探询行("10-14 06:00", 15, "old", "A", "") + " 中断 " +
            构造探询行("10-14 07:30", 45, "new", "B", ""))
    解2 = 解析探询行(两条)
    签("多条探询取最后一条", bool(解2) and 解2["基线"] == "new" and 解2["窗口"] == 45)
    签("追加保留旧备注（不覆盖）", 追加备注("先前备注", 行).startswith("先前备注 "))
    签("空旧备注直接写入", 追加备注("", 行) == 行)
    签("空白旧备注直接写入", 追加备注("  \t ", 行) == 行)
    紧凑 = 构造探询行("10-14 07:30", 30, "abc", "机", "理由", 紧凑=True)
    签("超限完整行拒·紧凑行可写", 追加备注("字" * 1900, 行) is None
       and 追加备注("字" * 1900, 紧凑) is not None)
    签("备注满=显式拒绝（非静默截断）", 追加备注("字" * 备注上限, "任意探询行") is None)
    t0 = time.time()
    签("时刻往返（分钟精度±90s）", abs((时刻到时戳(时戳到文本(t0)) - t0)) < 90)
    签("非法时刻=显式不可得", 时刻到时戳("99-99 99:99") is None
       and 时刻到时戳("") is None and 时刻到时戳("乱文本") is None)
    无活动 = [{"名": "S1", "可得": True, "活动": False, "说明": "x"}]
    有活动 = [{"名": "S3", "可得": True, "活动": True, "说明": "分支新提交"}]

    def 判(信号们, 服务=True, 状态=状态_在飞, 时戳=None, 窗口=30):
        基准 = time.time() - 10 if 时戳 is None else 时戳
        return 判决(信号们, 基准, 窗口, 服务, 状态, time.time())

    签("判决·活动=活跃放行", 判(有活动)[0] == 判决_活跃)
    签("判决·无活动窗口内=观察中", 判(无活动)[0] == 判决_观察)
    签("判决·无活动窗口满=静默确认", 判(无活动, 时戳=time.time() - 3600)[0] == 判决_静默)
    签("判决·服务不可达=未知不接管（窗口满也否）",
       判(无活动, 服务=False, 时戳=time.time() - 3600)[0] == 判决_未知)
    签("判决·服务不可达+活动=活跃放行（保守方向）",
       判(有活动, 服务=False)[0] == 判决_活跃)
    签("判决·非在飞不适用", 判(无活动, 状态="⬜", 时戳=time.time() - 3600)[0] == 判决_非在飞)
    签("判决·活动优先于状态", 判(有活动, 状态="⬜")[0] == 判决_活跃)
    签("判决·窗口剩余提示", "剩余约" in 判(无活动)[1][0])


def 自测在线(号: str) -> None:
    """真实往返（写生产台账·仅指定号·测后恢复原备注）：探询写入→读回断言→恢复。"""
    print(f"[在线] 真实往返 #{号}（写探询行→读回断言→恢复原备注）")
    任务, 可达, 错误, 鉴权败 = 读任务(号)
    if not 可达:
        黄(f"看板服务不可达（{错误}）——在线往返跳过（离线断言已跑）")
        return
    if 鉴权败:
        黄(f"{错误}——在线往返跳过（离线断言已跑）")
        return
    if not 任务:
        黄(f"任务 {号} 不存在（{错误}）——在线往返跳过")
        return
    原备注 = str(任务.get("备注") or "").strip()
    行 = 构造探询行(时刻文本(), 5, "自测基线", "selftest", "在线往返自测（测后恢复）")
    新备注 = 追加备注(原备注, 行)
    if 新备注 is None:
        黄("备注过长无法在线往返（离线长度保护断言已跑）")
        return
    码, 响应 = 写备注(号, 新备注)
    签("在线：探询写入 200", 码 == 200, f"码={码} {响应.get('错误', '')}")
    if 码 != 200:
        return
    任务2, _, _, _ = 读任务(号)
    回读 = str((任务2 or {}).get("备注") or "")
    解 = 解析探询行(回读)
    签("在线：读回含可解析探询行", bool(解))
    签("在线：窗口=5 可解析", bool(解) and 解["窗口"] == 5)
    签("在线：旧备注未被覆盖（前缀保留）", (not 原备注) or 回读.startswith(原备注))
    码3, 响应3 = 写备注(号, 原备注)
    签("在线：原备注已恢复", 码3 == 200, f"码={码3} {响应3.get('错误', '')}")
    任务3, _, _, _ = 读任务(号)
    签("在线：恢复读回一致", str((任务3 or {}).get("备注") or "") == 原备注)


def 命令自测(a) -> int:
    自测离线()
    if a.在线:
        自测在线(a.号)
    print(f"[自测] 通过 {计数['通过']}·失败 {len(计数['失败'])}")
    for 名 in 计数["失败"]:
        print(f"  [✗] {名}")
    return 0 if not 计数["失败"] else 1


# ===== 入口 =====

def main() -> int:
    解析器 = argparse.ArgumentParser(
        description="接棒判死探询客户端（728l·教训 #150：先打招呼再接管）")
    子 = 解析器.add_subparsers(dest="命令", required=True)
    p探 = 子.add_parser("探询", help="判死三信号命中后写探询行到任务备注（缓冲确认第一步）")
    p探.add_argument("号", help="任务号（如 728l）")
    p探.add_argument("--窗口", type=int, default=默认窗口分,
                     help=f"探询窗口分钟（默认 {默认窗口分}·长推理高风险任务建议 45）")
    p探.add_argument("--理由", default="", help="探询缘由一句话（写入探询行）")
    p探.add_argument("--交接", action="store_true", help="额外在看板交接区落一条（全局注目）")
    p探.add_argument("--机器", default="", help="探询者标识覆盖（默认自动机器名）")
    p复 = 子.add_parser("复查", help="窗口后复查第四信号（exit 0=静默确认·可接管）")
    p复.add_argument("号", help="任务号")
    p测 = 子.add_parser("selftest", help="自测（离线断言·--在线 加真实往返）")
    p测.add_argument("--在线", action="store_true", help="真实探询→读回→恢复原备注往返")
    p测.add_argument("--号", default="728l", help="在线往返任务号（默认 728l·测后恢复原备注）")
    参数 = 解析器.parse_args()
    if 参数.命令 == "探询":
        return 命令探询(参数)
    if 参数.命令 == "复查":
        return 命令复查(参数)
    return 命令自测(参数)


if __name__ == "__main__":
    sys.exit(main())
