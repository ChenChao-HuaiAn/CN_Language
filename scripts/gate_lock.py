#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""gate_lock.py —— 全量门禁串行锁（449-a·AGENTS.md §8.8）+ 树锁（555·一树一会话）。

多任务并行时两个全量门禁同时跑会互抢 CPU/内存（78/79 OOM 前科），本锁保证
**全机同一时刻至多一个全量门禁在飞**；锁放主树 target/（跨 worktree 共享——
worktree 的 git-common-dir 都指回主树 .git）。

555 增树域（--scope tree，默认 machine 向后兼容）：每棵 worktree 自己一把
`本树根/target/tree.lock`（脚本所在仓库根·不经 git-common-dir 反推主树·
CN_TREE_LOCK_DIR 覆盖）——同树并发 E2E/构建互踩（v2p 缓存双写/清场互杀 rc=143）
根治，跨树天然并行。环境豁免 CN_TREE_LOCK_HELD=1=嵌套链外层已持锁，acquire/run
跳过取锁防自死锁（495-a/595-a 同族防线）；run 形态 tree 域向子进程注入该变量。

566（2026-10-13·审查 P0-2/P1-1）：machine 域同构补齐——run 形态向子进程注入
CN_GATE_LOCK_HELD=1（run_e2e --allow-heavy 嵌套链见之跳过取机器锁·原零写点死代码
根治）；释放(仅属主=pid) 归属校验——fork 进程池 worker 继承的 atexit 释放闭包
只删真正属于自己进程的锁（917 陈锁接管路径不受影响）。

  acquire [--timeout 秒] [--owner PID] [--scope machine|tree] 阻塞获取锁（默认等待 2h；
                                 陈锁=存活锚死秒级接管〔917〕/心跳停 4h 兜底）
  release [--scope]          释放锁（Ctrl+C 中断门禁后手动清锁用）
  run [--scope] -- <命令...> 获取→执行→无论成败释放（推荐用法，透传退出码）
  status [--scope]           查看当前持锁者

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
    覆盖 = os.environ.get("CN_GATE_LOCK_DIR")   # 291：win CI 池树（独立克隆）跨树指向
    if 覆盖:                                    # 主开发树锁目录——与人工门禁全机互斥
        return Path(覆盖)
    结果 = subprocess.run(["git", "rev-parse", "--git-common-dir"],
                          capture_output=True, text=True, encoding="utf-8", cwd=本树根)
    共同 = (结果.stdout or "").strip()
    路径 = Path(共同) if Path(共同).is_absolute() else (本树根 / 共同)
    return 路径.resolve().parent / "target" / "gate.lock"


def 树锁目录() -> Path:
    """555：树域锁目录=脚本所在仓库根/target/tree.lock——每棵 worktree 各自一把
    （不经 git-common-dir 反推主树·跨树天然并行）；CN_TREE_LOCK_DIR 覆盖（自测用）。"""
    覆盖 = os.environ.get("CN_TREE_LOCK_DIR")
    if 覆盖:
        return Path(覆盖)
    return 本树根 / "target" / "tree.lock"


def 按域锁目录(域: str) -> Path:
    return 树锁目录() if 域 == "tree" else 主树锁目录()


def 树锁豁免() -> bool:
    """555：CN_TREE_LOCK_HELD 真值=嵌套链外层已持树锁（gate_quick→run_e2e 注入）——
    acquire/run 跳过取锁防「外层持锁内层再等」自死锁（495-a/595-a 同族防线）。"""
    return bool(os.environ.get("CN_TREE_LOCK_HELD"))


def 机器锁豁免() -> bool:
    """566（审查 P0-2）：CN_GATE_LOCK_HELD 真值=嵌套链外层已持 machine 域锁——
    gate_lock run 形态（本文件）与 ci_daemon 带门禁锁跑（win 持锁跑三段）注入，
    run_e2e --allow-heavy 与本脚本 acquire/run 见之跳过取锁，防「外层已持机器锁
    内层再等同一把」无限自死锁（树锁 CN_TREE_LOCK_HELD 同构防线）。"""
    return bool(os.environ.get("CN_GATE_LOCK_HELD"))


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


def 获取(超时秒: int, 形态: str = "run", owner_pid: int | None = None,
         锁: Path | None = None) -> Path | None:
    if 锁 is None:          # 555：锁目录参数化（树域由调用方传 树锁目录()·缺省=machine 域）
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


def 释放(锁: Path, 静默: bool = False, 仅属主: int | None = None) -> None:
    """仅属主（566·审查 P1-1）：非 None 时先校验 info.json 的 owner_pid==仅属主 才删——
    Linux fork 进程池 worker 继承父进程 atexit 注册的释放闭包，worker 完成任务正常
    退出时会执行它；无校验则 worker 误删真正持锁者（主进程）的锁=全机互斥静默失效。
    归属不符（或锁已不在）即跳过不动。917 陈锁接管路径不传 仅属主——行为不变。"""
    if 仅属主 is not None and 读信息(锁).get("owner_pid") != 仅属主:
        return
    try:
        (锁 / "info.json").unlink(missing_ok=True)
        锁.rmdir()
    except OSError:
        pass
    if not 静默:
        print("[gate_lock] 已释放", file=sys.stderr)


def 主流程() -> int:
    解析器 = argparse.ArgumentParser(
        description="全量门禁串行锁（machine=全机一把跨 worktree 共享·tree=单树一把〔555〕）")
    子 = 解析器.add_subparsers(dest="命令", required=True)
    域帮助 = ("锁域：machine=全机一把（默认·主树 target/gate.lock 跨 worktree 共享）；"
             "tree=本树一把（本树根 target/tree.lock·同树互斥跨树并行·555）")
    p取 = 子.add_parser("acquire", help="阻塞获取锁")
    p取.add_argument("--timeout", type=int, default=7200, help="等待上限秒（0=无限）")
    p取.add_argument("--owner", type=int, default=None, metavar="PID",
                     help="持锁存活锚 pid（917·任务 109）：acquire 分离形态传调用方 pid"
                          "（如 ci.ps1 传 $PID）——锚进程死=陈锁秒级接管；缺省=acquire 子进程"
                          "自身（返回即退·不可作锚→回退心跳 4h 判据）")
    p取.add_argument("--scope", choices=["machine", "tree"], default="machine", help=域帮助)
    p释 = 子.add_parser("release", help="释放锁")
    p释.add_argument("--scope", choices=["machine", "tree"], default="machine", help=域帮助)
    p跑 = 子.add_parser("run", help="获取→执行命令→释放（透传退出码）")
    p跑.add_argument("--scope", choices=["machine", "tree"], default="machine", help=域帮助)
    p跑.add_argument("命令", nargs=argparse.REMAINDER)
    p态 = 子.add_parser("status", help="查看持锁者")
    p态.add_argument("--scope", choices=["machine", "tree"], default="machine", help=域帮助)
    参数 = 解析器.parse_args()

    域 = 参数.scope
    锁目录 = 按域锁目录(域)
    # 555 树域 + 566 machine 域（审查 P0-2）：环境豁免=嵌套链外层已持本域锁——
    #   release 拒删（锁归外层）·acquire/run 跳过取锁直跑（防嵌套自死锁）。
    豁免 = 树锁豁免() if 域 == "tree" else 机器锁豁免()
    豁免变量 = "CN_TREE_LOCK_HELD" if 域 == "tree" else "CN_GATE_LOCK_HELD"

    if 参数.命令 == "status":
        if 锁目录.exists():
            信息 = 读信息(锁目录)
            owner = 信息.get("owner_pid") or (信息.get("pid") if 信息.get("形态") == "run" else None)
            锚态 = (f"锚 pid={owner} {'活' if pid存活(owner) else '死'}" if owner else "无锚（回退心跳 4h）")
            print(f"被持有：pid={信息.get('pid')} 形态={信息.get('形态', '?')} {锚态} "
                  f"命令={信息.get('命令')} 自 {信息.get('获取时间')}"
                  f"{'（陈锁）' if 是陈锁(锁目录) else ''}")
        else:
            print("空闲")
        return 0
    if 参数.命令 == "release":
        if 豁免:
            # 555/566：豁免环境（嵌套子进程）的锁归外层持有者——删了=拆外层互斥，拒绝
            print(f"[gate_lock] 继承外层{域}锁（{豁免变量}=1）——release 跳过（锁归外层持有者）",
                  file=sys.stderr)
            return 0
        释放(锁目录)
        return 0
    # 555：树域环境豁免——外层已持锁（CN_TREE_LOCK_HELD=1），acquire/run 直接跳过取锁
    #   （不建锁目录不等待），run 直跑透传退出码。防嵌套链自死锁（最高风险点）。
    #   566：machine 域同构豁免（CN_GATE_LOCK_HELD=1·审查 P0-2 根治死代码）。
    if 豁免:
        print(f"[gate_lock] 继承外层{域}锁（{豁免变量}=1）", file=sys.stderr)
        if 参数.命令 == "acquire":
            return 0
        命令 = 参数.命令
        if 命令 and 命令[0] == "--":
            命令 = 命令[1:]
        if not 命令:
            print("[gate_lock] run 缺少命令", file=sys.stderr)
            return 1
        return subprocess.run(命令).returncode
    # 595-a（方案甲①）：run 形态等待默认无限（0）——串行队列本义=排队总能轮到，
    #   2h 硬超时制造「假放弃+假退出码」；真死锁由陈锁 4h 心跳接管兜底。
    #   acquire 形态维持 7200（ci.ps1 内部用·保留快速失败语义），可 --timeout 覆盖。
    锁 = 获取(参数.timeout if 参数.命令 == "acquire" else 0,
             形态=参数.命令, owner_pid=getattr(参数, "owner", None), 锁=锁目录)
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
        # 555：tree 域向子进程注入 CN_TREE_LOCK_HELD=1——嵌套链（gate_quick→run_e2e→
        #   再调 gate_lock/run_e2e）见之即豁免跳过取锁，防自死锁。
        # 566（审查 P0-2）：machine 域同构注入 CN_GATE_LOCK_HELD=1（原先 env=None 不
        #   注入=豁免变量全仓零写点死代码）——嵌套链内层 run_e2e --allow-heavy 等
        #   见之跳过取机器锁，防「外层持机器锁内层再等同一把」无限自死锁。
        环境 = dict(os.environ,
                    **({"CN_TREE_LOCK_HELD": "1"} if 域 == "tree"
                       else {"CN_GATE_LOCK_HELD": "1"}))
        return subprocess.run(命令, env=环境).returncode
    except KeyboardInterrupt:
        print("\n[gate_lock] 中断——释放锁", file=sys.stderr)
        return 130
    finally:
        释放(锁)


def 树锁运行入口(主体) -> int:
    """555：树锁宿主入口包装（run_e2e.py / gate_quick.py 入口共用）——同树并发
    E2E/构建串行化（target/v2p 缓存互踩·清场互杀 rc=143 根治），跨树不受影响。
    CN_NO_TREE_LOCK 真值=逃生门完全跳过；CN_TREE_LOCK_HELD 真值=嵌套链外层已持锁
    豁免直跑。取锁等待无限（同 run 形态语义·排队进度 30s 一打印），持锁期后台心跳，
    finally 释放（异常/Ctrl+C 均覆盖）。主体=无参可调用，返回值透传为退出码。"""
    if os.environ.get("CN_NO_TREE_LOCK"):
        print("[gate_lock] 树锁豁免（CN_NO_TREE_LOCK）", flush=True)
        return 主体()
    if os.environ.get("CN_TREE_LOCK_HELD"):
        print("[gate_lock] 继承外层树锁（CN_TREE_LOCK_HELD=1）", flush=True)
        return 主体()
    锁 = 获取(0, 形态="run", owner_pid=os.getpid(), 锁=树锁目录())
    if 锁 is None:
        print("[gate_lock] 树锁等待超时——同树另一 E2E/构建仍在飞（gate_lock status --scope tree 查看）",
              file=sys.stderr)
        return 2
    心跳线程(锁 / "info.json")
    try:
        return 主体()
    except KeyboardInterrupt:
        print("\n[gate_lock] 中断——释放树锁", file=sys.stderr)
        return 130
    finally:
        释放(锁)


if __name__ == "__main__":
    sys.exit(主流程())
