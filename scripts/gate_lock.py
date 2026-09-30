#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""gate_lock.py —— 全量门禁串行锁（449-a·AGENTS.md §8.8）。

多任务并行时两个全量门禁同时跑会互抢 CPU/内存（78/79 OOM 前科），本锁保证
**全机同一时刻至多一个全量门禁在飞**；锁放主树 target/（跨 worktree 共享——
worktree 的 git-common-dir 都指回主树 .git）。

  acquire [--timeout 秒] [--owner PID] 阻塞获取锁（默认等待 2h；陈锁=存活锚死秒级接管〔917〕
                                 /心跳停 4h 兜底）
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


def pid存活(pid) -> bool:
    """探测进程是否存活（917·任务 109：陈锁秒级接管主判据）。
    POSIX: os.kill(pid, 0)（信号 0=纯探测·PermissionError=别人的活进程）；
    Windows: os.kill 的 sig=0 无特判会走 TerminateProcess——绝不可用于探测，
    用 ctypes OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION)+GetExitCodeProcess。
    无法判定一律保守判「活」（误清活锁=两场门禁并行，代价 >> 多等一会）。"""
    try:
        pid = int(pid)
    except (TypeError, ValueError):
        return False
    if pid <= 0:
        return False
    if sys.platform != "win32":
        try:
            os.kill(pid, 0)
        except ProcessLookupError:
            return False
        except OSError:
            return True
        return True
    import ctypes
    内核 = ctypes.WinDLL("kernel32", use_last_error=True)
    PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
    STILL_ACTIVE = 259
    ERROR_ACCESS_DENIED = 5
    句柄 = 内核.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
    if not 句柄:
        # ACCESS_DENIED=进程存在但无权限（保守判活）；其余（含 INVALID_PARAMETER）=不存在
        return ctypes.get_last_error() == ERROR_ACCESS_DENIED
    try:
        码 = ctypes.c_ulong()
        if 内核.GetExitCodeProcess(句柄, ctypes.byref(码)):
            return 码.value == STILL_ACTIVE
        return True  # 查询失败保守判活
    finally:
        内核.CloseHandle(句柄)


def 是陈锁(锁: Path) -> bool:
    """917（任务 109·021:173）：双判据——
    ① 持锁存活锚已死=立即陈锁（主场景=TaskStop/强杀门禁后锁残留·排队者秒级接管·
       旧 4h 空等根除）。锚=run 形态的 gate_lock 自身 pid / acquire 分离形态由调用方
       传的 --owner pid（如 ci.ps1 传 $PID）。**acquire 未带 owner 的锁其 pid 是已
       退出的 acquire 子进程、不可按 pid 判死**——无锚（旧 info/旧调用）回退②。
    ② 心跳停超 4h=陈锁（原判据保留·兜底 pid 复用与无锚形态）。
    pid 复用方向安全：死 pid 被新进程占→探测「活」→退回②（不误清）。"""
    信息 = 读信息(锁)
    owner = 信息.get("owner_pid")
    if owner is None and 信息.get("形态") == "run":
        owner = 信息.get("pid")
    if owner and not pid存活(owner):
        return True
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


def 获取(超时秒: int, 形态: str = "run", owner_pid: int | None = None) -> Path | None:
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
                陈锁信息 = 读信息(锁)
                owner = 陈锁信息.get("owner_pid") or (陈锁信息.get("pid") if 陈锁信息.get("形态") == "run" else None)
                因 = (f"持锁存活锚 pid={owner} 已死（917 秒级接管）" if owner and not pid存活(owner)
                      else f"心跳停超 {陈锁阈值 // 3600}h")
                print(f"[gate_lock] 陈锁接管（{因}：{陈锁信息.get('命令', '?')}）", file=sys.stderr)
                释放(锁, 静默=True)
                continue
            if 超时秒 and time.time() - 起始 > 超时秒:
                return None
            信息 = 读信息(锁)
            持有者 = 信息.get("命令") or 持有者 or "?"  # 锁刚被释放的间隙保留上一次读到的持有者
            if time.time() - 上次打印 >= 30:
                # 595-a（方案甲③）：等待进度改 stdout——全缓冲/管道下 stderr 进度
                #   对等待方不可见（595 轮排队 2h 零感知教训）；flush 保实时。
                print(f"[gate_lock] 排队等待中 {int(time.time() - 起始)}s（{持有者} 持有）", flush=True)
                上次打印 = time.time()
            time.sleep(2)
            continue
        # owner_pid（917·任务 109）：存活锚——run 形态=gate_lock 自身（贯穿命令执行）；
        #   acquire 分离形态=调用方进程（ci.ps1 传 $PID）——锚死=陈锁秒级接管。
        信息 = {"pid": os.getpid(), "形态": 形态, "owner_pid": owner_pid or os.getpid(),
                "命令": " ".join(sys.argv[1:]),
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
    p取.add_argument("--owner", type=int, default=None, metavar="PID",
                     help="持锁存活锚 pid（917·任务 109）：acquire 分离形态传调用方 pid"
                          "（如 ci.ps1 传 $PID）——锚进程死=陈锁秒级接管；缺省=acquire 子进程"
                          "自身（返回即退·不可作锚→回退心跳 4h 判据）")
    子.add_parser("release", help="释放锁")
    p跑 = 子.add_parser("run", help="获取→执行命令→释放（透传退出码）")
    p跑.add_argument("命令", nargs=argparse.REMAINDER)
    子.add_parser("status", help="查看持锁者")
    参数 = 解析器.parse_args()

    if 参数.命令 == "status":
        锁 = 主树锁目录()
        if 锁.exists():
            信息 = 读信息(锁)
            owner = 信息.get("owner_pid") or (信息.get("pid") if 信息.get("形态") == "run" else None)
            锚态 = (f"锚 pid={owner} {'活' if pid存活(owner) else '死'}" if owner else "无锚（回退心跳 4h）")
            print(f"被持有：pid={信息.get('pid')} 形态={信息.get('形态', '?')} {锚态} "
                  f"命令={信息.get('命令')} 自 {信息.get('获取时间')}"
                  f"{'（陈锁）' if 是陈锁(锁) else ''}")
        else:
            print("空闲")
        return 0
    if 参数.命令 == "release":
        释放(主树锁目录())
        return 0
    # 595-a（方案甲①）：run 形态等待默认无限（0）——串行队列本义=排队总能轮到，
    #   2h 硬超时制造「假放弃+假退出码」；真死锁由陈锁 4h 心跳接管兜底。
    #   acquire 形态维持 7200（ci.ps1 内部用·保留快速失败语义），可 --timeout 覆盖。
    锁 = 获取(参数.timeout if 参数.命令 == "acquire" else 0,
             形态=参数.命令, owner_pid=getattr(参数, "owner", None))
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
    # 595-a 双层锁防护（方案甲②）：ci.ps1 等门禁脚本**内置 acquire**，再被 run 包一层
    #   =外层持锁、内层等自己 → 自死锁到超时（495-a 互等 85 分钟/595-a 同款再犯）。
    #   命中内置锁名单 → 拒绝双层包装并提示裸跑（脚本自己管锁）。
    命令文本 = " ".join(命令)
    for 内置 in ("ci.ps1", "integrate.py"):
        if 内置 in 命令文本:
            释放(锁)
            print(f"[gate_lock] 拒绝：{内置} 已内置 acquire/release，请裸跑（勿再包 gate_lock run —— 双层=自死锁）", file=sys.stderr)
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
