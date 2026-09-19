#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""gate_lock.py —— 全量门禁串行锁（449-a·AGENTS.md §8.8）。

多任务并行时两个全量门禁同时跑会互抢 CPU/内存（78/79 OOM 前科），本锁保证
**全机同一时刻至多一个全量门禁在飞**；锁放主树 target/（跨 worktree 共享——
worktree 的 git-common-dir 都指回主树 .git）。

  acquire [--timeout 秒]     阻塞获取锁（默认等待 2h；陈锁=心跳停 4h 自动接管）
  release                    释放锁（Ctrl+C 中断门禁后手动清锁用）
  run -- <命令...>           获取→执行→无论成败释放（推荐用法，透传退出码）
  status                     查看当前持锁者

实现：os.mkdir 原子目录锁（跨平台无 msvcrt/fcntl 差异）；持锁期间后台线程每
60s 触碰 info 文件=心跳；Ctrl+C/命令失败均走 finally 释放。
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import threading
import time
from pathlib import Path

本树根 = Path(__file__).resolve().parent.parent
心跳间隔 = 60
陈锁阈值 = 4 * 3600  # 心跳停超 4 小时视为持锁进程已死，自动接管


def 主树锁目录() -> Path:
    结果 = subprocess.run(["git", "rev-parse", "--git-common-dir"],
                          capture_output=True, text=True, encoding="utf-8", cwd=本树根)
    共同 = (结果.stdout or "").strip()
    路径 = Path(共同) if Path(共同).is_absolute() else (本树根 / 共同)
    return 路径.resolve().parent / "target" / "gate.lock"


def 读信息(锁: Path) -> dict:
    try:
        return json.loads((锁 / "info.json").read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


def 是陈锁(锁: Path) -> bool:
    try:
        停跳 = time.time() - (锁 / "info.json").stat().st_mtime
        return 停跳 > 陈锁阈值
    except OSError:
        return False


def 心跳线程(信息文件: Path) -> threading.Thread:
    def 敲():
        while True:
            time.sleep(心跳间隔)
            try:
                信息文件.touch()
            except OSError:
                return
    t = threading.Thread(target=敲, daemon=True)
    t.start()
    return t


def 获取(超时秒: int) -> Path | None:
    锁 = 主树锁目录()
    锁.parent.mkdir(parents=True, exist_ok=True)
    起始 = time.time()
    上次打印 = 0.0
    持有者 = ""
    while True:
        try:
            锁.mkdir()
        except FileExistsError:
            if 是陈锁(锁):
                print(f"[gate_lock] 陈锁接管（心跳停超 {陈锁阈值 // 3600}h：{读信息(锁).get('命令', '?')}）", file=sys.stderr)
                释放(锁, 静默=True)
                continue
            if 超时秒 and time.time() - 起始 > 超时秒:
                return None
            信息 = 读信息(锁)
            持有者 = 信息.get("命令") or 持有者 or "?"  # 锁刚被释放的间隙保留上一次读到的持有者
            if time.time() - 上次打印 >= 30:
                print(f"[gate_lock] 等待中 {int(time.time() - 起始)}s（{持有者} 持有）", file=sys.stderr)
                上次打印 = time.time()
            time.sleep(2)
            continue
        信息 = {"pid": os.getpid(), "命令": " ".join(sys.argv[1:]),
                "获取时间": time.strftime("%Y-%m-%d %H:%M:%S")}
        try:
            (锁 / "info.json").write_text(json.dumps(信息, ensure_ascii=False), encoding="utf-8")
        except OSError:
            pass
        print(f"[gate_lock] 已获得（{信息['获取时间']}）", file=sys.stderr)
        return 锁


def 释放(锁: Path, 静默: bool = False) -> None:
    try:
        (锁 / "info.json").unlink(missing_ok=True)
        锁.rmdir()
    except OSError:
        pass
    if not 静默:
        print("[gate_lock] 已释放", file=sys.stderr)


def 主流程() -> int:
    解析器 = argparse.ArgumentParser(description="全量门禁串行锁（跨 worktree 共享）")
    子 = 解析器.add_subparsers(dest="命令", required=True)
    p取 = 子.add_parser("acquire", help="阻塞获取锁")
    p取.add_argument("--timeout", type=int, default=7200, help="等待上限秒（0=无限）")
    子.add_parser("release", help="释放锁")
    p跑 = 子.add_parser("run", help="获取→执行命令→释放（透传退出码）")
    p跑.add_argument("命令", nargs=argparse.REMAINDER)
    子.add_parser("status", help="查看持锁者")
    参数 = 解析器.parse_args()

    if 参数.命令 == "status":
        锁 = 主树锁目录()
        if 锁.exists():
            信息 = 读信息(锁)
            print(f"被持有：pid={信息.get('pid')} 命令={信息.get('命令')} 自 {信息.get('获取时间')}"
                  f"{'（陈锁）' if 是陈锁(锁) else ''}")
        else:
            print("空闲")
        return 0
    if 参数.命令 == "release":
        释放(主树锁目录())
        return 0
    锁 = 获取(参数.timeout if 参数.命令 == "acquire" else 7200)
    if 锁 is None:
        print("[gate_lock] 等待超时——另一门禁仍在飞，稍后再试或 gate_lock status 查看", file=sys.stderr)
        return 2
    if 参数.命令 == "acquire":
        # acquire/finally-release 分离形态（ci.ps1 用）：拿锁即返回，调用方跑门禁主体，
        # 结束时 finally 调 release；调用方进程被杀则心跳停、4h 后陈锁自动接管。
        return 0
    # run 形态
    命令 = 参数.命令
    if 命令 and 命令[0] == "--":
        命令 = 命令[1:]
    if not 命令:
        释放(锁)
        print("[gate_lock] run 缺少命令", file=sys.stderr)
        return 1
    心跳线程(锁 / "info.json")
    try:
        return subprocess.run(命令).returncode
    except KeyboardInterrupt:
        print("\n[gate_lock] 中断——释放锁", file=sys.stderr)
        return 130
    finally:
        释放(锁)


if __name__ == "__main__":
    sys.exit(主流程())
