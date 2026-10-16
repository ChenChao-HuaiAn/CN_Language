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

578（2026-10-16·任务 #578）：长驻持有者（win 池 runner=ci_daemon --loop）作
acquire 存活锚时 owner_pid 永活——917 判据①「锚死→秒级接管」对「锚活+轮卡」
退化 4h（实录：daemon 持锁但子进程消失僵死 55 分钟·锁无可清判据）。修=轮心跳：
  · 获取(轮心跳=True) 在 info.json 登记「轮心跳」标记+「轮锚」字段（轮内活跃
    子进程 pid·由持有者用 启动轮心跳 线程续约）；
  · 判据③（是陈锁）：轮心跳标记真 且 轮锚已死/未登记 且 info mtime 停超
    轮心跳阈值（默认 300s·CN_ROUND_HB_TIMEOUT 覆盖）→ 陈锁（分钟级接管）；
  · 持有者侧探测（scripts/ci_round_guard.py）=轮内有无活子进程+主线程进度——
    轮卡死/僵死时停续约，心跳自然陈旧；子进程长跑（构建/E2E）期间持续续约，
    不误清活锁。

  acquire [--timeout 秒] [--owner PID] [--scope machine|tree] 阻塞获取锁（默认等待 2h；
                                 陈锁=存活锚死秒级接管〔917〕/心跳停 4h 兜底）
  release [--scope]          释放锁（Ctrl+C 中断门禁后手动清锁用）
  run [--scope] -- <命令...> 获取→执行→无论成败释放（推荐用法，透传退出码）
  status [--scope]           查看当前持锁者

实现：os.mkdir 原子目录锁（跨平台无 msvcrt/fcntl 差异）；持锁期间后台线程每
60s 触碰 info 文件=心跳；Ctrl+C/命令失败均走 finally 释放。

935（2026-10-17 两夜实录 379/932）：陈锁接管清尸——清护面（树锁运行入口）在
判定「锚已死」时先回收死锚的存活后代（进程树枚举·PID/PPID 链），治「强杀不留
活口」病根（Windows 不级联杀→spawn worker 孤儿化与新轮同树互踩）。
"""
from __future__ import annotations

import argparse
import json
import os
import signal
import subprocess
import sys
import threading
import time
from pathlib import Path

本树根 = Path(__file__).resolve().parent.parent
心跳间隔 = 60
陈锁阈值 = 4 * 3600  # 心跳停超 4 小时视为持锁进程已死，自动接管
# 578：轮心跳（轮级判据）参数——长驻持有者形态的补足判据③（见 是陈锁 docstring）。
轮心跳间隔秒 = 30   # 轮心跳线程续约周期（CN_ROUND_HB_INTERVAL 覆盖·自测收窄用）
轮心跳阈值秒 = 300  # 轮锚死/未登记 + 心跳停超此值 = 陈锁（CN_ROUND_HB_TIMEOUT 覆盖）


def 轮心跳间隔() -> int:
    """578：轮心跳续约周期（环境变量动态读取——自测跨进程收窄·daemon 用缺省）。"""
    return int(os.environ.get("CN_ROUND_HB_INTERVAL", str(轮心跳间隔秒)))


def 轮心跳阈值() -> int:
    """578：轮心跳停超此秒数（且轮锚死/未登记）判陈锁（环境变量动态读取·自测收窄用）。"""
    return int(os.environ.get("CN_ROUND_HB_TIMEOUT", str(轮心跳阈值秒)))


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
    """917（任务 109·021:173）：双判据 + 578 轮心跳判据③——
    ① 持锁存活锚已死=立即陈锁（主场景=TaskStop/强杀门禁后锁残留·排队者秒级接管·
       旧 4h 空等根除）。锚=run 形态的 gate_lock 自身 pid / acquire 分离形态由调用方
       传的 --owner pid（如 ci.ps1 传 $PID）。**acquire 未带 owner 的锁其 pid 是已
       退出的 acquire 子进程、不可按 pid 判死**——无锚（旧 info/旧调用）回退②。
    ② 心跳停超 4h=陈锁（原判据保留·兜底 pid 复用与无锚形态）。
    ③ 578：info 带「轮心跳」标记（长驻池 runner 形态）时 owner 永活使①失效——
       轮锚（轮内活跃子进程 pid）已死/未登记 且 info mtime 停超 轮心跳阈值
       （默认 300s）→ 陈锁（轮卡死/僵死·原只能等② 4h）。轮锚活=子进程在跑=
       轮在推进（不判死——乱清活锁代价 >> 多等）；轮心跳标记不存在=旧格式
       锁，退回①②行为不变。
    pid 复用方向安全：死 pid 被新进程占→探测「活」→退回②（不误清）。"""
    信息 = 读信息(锁)
    owner = 信息.get("owner_pid")
    if owner is None and 信息.get("形态") == "run":
        owner = 信息.get("pid")
    if owner and not pid存活(owner):
        return True
    try:
        停跳 = time.time() - (锁 / "info.json").stat().st_mtime
    except OSError:
        return False
    if 信息.get("轮心跳"):
        轮锚 = 信息.get("轮锚")
        if (轮锚 is None or not pid存活(轮锚)) and 停跳 > 轮心跳阈值():
            return True
    return 停跳 > 陈锁阈值


def 更新轮心跳(锁: Path, 轮锚=None) -> None:
    """578：轮心跳续约（原子重写 info.json=刷新 mtime + 可更新「轮锚」字段）。
    仅由 启动轮心跳 线程调用；读-改-写 + os.replace 原子替换（排队方永不读到
    半截 JSON）。锁已被接管（新锁无「轮心跳」标记）即不动——防续约方复活他人锁。
    任何 OSError 原样抛出由线程体吞——续约失败=心跳自然陈旧，走同一接管路径。"""
    信息 = 读信息(锁)
    if not 信息.get("轮心跳"):
        return
    if 轮锚 is not None:
        信息["轮锚"] = 轮锚
    临时 = 锁 / "info.json.tmp"
    临时.write_text(json.dumps(信息, ensure_ascii=False), encoding="utf-8")
    os.replace(临时, 锁 / "info.json")


def 启动轮心跳(锁: Path, 探测=None, 间隔: int | None = None) -> threading.Event:
    """578：轮心跳线程——长驻持有者（ci_daemon --loop）持锁跑轮期间续约 info.json。
    探测()=(轮锚 pid 或 None, 该刷新 bool)（scripts/ci_round_guard.py 的轮守卫合成：
    有活子进程=轮在推进→真；无活子进程但主线程进度新鲜于阈值=步骤间隙→真；
    否则=轮卡死/僵死→假）：
      · 该刷新=真 → 更新轮心跳（touch/轮锚刷新）；
      · 该刷新=假 → 不续约——mtime 自然陈旧，排队方按 是陈锁 判据③ 接管。
    探测=None=无条件续约（最小形态）。返回 stop 事件（set() 即停·持锁方 finally 调）。
    线程体全异常吞并不退出（单次续约失败不杀线程；持续失败=mtime 陈旧→同一接管
    路径，保守方向=宁可判死也不放过活锁封死的判据悖论）。"""
    周期 = 间隔 or 轮心跳间隔()
    停 = threading.Event()

    def 敲():
        while not 停.wait(周期):
            try:
                轮锚, 该刷新 = (None, True) if 探测 is None else 探测()
                if not 该刷新:
                    continue
                更新轮心跳(锁, 轮锚=轮锚)
            except Exception:
                continue

    threading.Thread(target=敲, daemon=True).start()
    return 停


# ═══════ 935（2026-10-17 两夜实录 379/932）：陈锁接管清尸 ═══════
# 病根：917「锚死=秒级接管」隐含假设「锚死 ⇒ 其作业已停」——multiprocessing 进程池
# 打破它：主进程被 TerminateProcess（Windows 不级联）后 spawn worker 孤儿化
# （932：并行轮主进程死后 6 个 worker 睡 2h·CPU delta=0），接管者/下一轮与「前代
# 进程树」同树互踩=僵死链。修复：清护面（仅 树锁运行入口=run_e2e 传）在判定
# 「锚已死」时先把死锚的存活后代整树回收，再接管——任一时刻恰一个有效持有者。

def _进程表() -> list:
    """全量 (pid, ppid) 快照。win=Toolhelp32（纯 stdlib）；posix=/proc/<pid>/stat。"""
    if sys.platform == "win32":
        import ctypes
        import ctypes.wintypes as w

        class 条目(ctypes.Structure):
            _fields_ = [("dwSize", w.DWORD), ("cntUsage", w.DWORD), ("pid", w.DWORD),
                        ("堆", ctypes.c_void_p), ("模块", w.DWORD), ("线程数", w.DWORD),
                        ("ppid", w.DWORD), ("基类", ctypes.c_long), ("标志", w.DWORD),
                        ("名", ctypes.c_wchar * 260)]

        内核 = ctypes.WinDLL("kernel32", use_last_error=True)
        内核.CreateToolhelp32Snapshot.restype = ctypes.c_void_p
        内核.CreateToolhelp32Snapshot.argtypes = [w.DWORD, w.DWORD]
        内核.Process32FirstW.argtypes = [ctypes.c_void_p, ctypes.POINTER(条目)]
        内核.Process32NextW.argtypes = [ctypes.c_void_p, ctypes.POINTER(条目)]
        快照 = 内核.CreateToolhelp32Snapshot(0x2, 0)
        if not 快照:
            return []
        出, 项 = [], 条目()
        项.dwSize = ctypes.sizeof(条目)
        try:
            ok = 内核.Process32FirstW(快照, ctypes.byref(项))
            while ok:
                出.append((int(项.pid), int(项.ppid)))
                ok = 内核.Process32NextW(快照, ctypes.byref(项))
        finally:
            内核.CloseHandle(快照)
        return 出
    出 = []
    for 目录 in Path("/proc").iterdir():
        if not 目录.name.isdigit():
            continue
        try:
            内容 = (目录 / "stat").read_text(encoding="utf-8", errors="replace")
            出.append((int(目录.name), int(内容[内容.rfind(")") + 2:].split()[1])))
        except (OSError, ValueError, IndexError):
            continue
    return 出


def 收集后代(锚pid: int, 表: list) -> list:
    """表中 PPID 链递归指向锚 pid 的全体存活进程（锚自身的历代子孙）。"""
    出, 边界 = [], {锚pid}
    while True:
        新 = [p for p, q in 表 if q in 边界 and p not in 边界]
        if not 新:
            return 出
        出 += 新
        边界 |= set(新)


def 清理死树(锚pid) -> list:
    """回收死锚 pid 的存活后代（强杀不级联留下的「僵尸树」）。返回被杀 pid 列表；
    清理是接管前的保护动作——任何异常=空列表，绝不阻断接管（锁仍按 917 流程释放）。"""
    try:
        锚pid = int(锚pid)
    except (TypeError, ValueError):
        return []
    if 锚pid <= 0:
        return []
    被杀 = []
    try:
        for p in 收集后代(锚pid, _进程表()):
            try:
                os.kill(p, signal.SIGTERM if sys.platform == "win32" else signal.SIGKILL)
                被杀.append(p)
            except OSError:
                pass
    except Exception:
        return 被杀
    return 被杀


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
         锁: Path | None = None, 轮心跳: bool = False, 清护: bool = False) -> Path | None:
    """578 轮心跳 + 935 清护（rebase 融合·两参数正交、各自语义保留）：
    轮心跳=True：info 登记「轮心跳」标记——长驻持有者（win 池 runner）由 启动轮心跳
    线程按轮守卫探测续约；轮卡死/僵死时排队方按 是陈锁 判据③ 分钟级接管（owner
    永活使 917 判据①失效的形态补足）。
    清护=True：陈锁接管且「锚已死」（判据①）时先整树回收死锚的存活后代再接管——
    仅树锁运行入口传（E2E 轮=multiprocessing 进程池所有者·孤儿 worker 是互踩源）。
    两者缺省 False=行为与各自引入前完全一致（machine 域等不动）。"""
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
                # 935+578 融合（rebase 合 935）：判据①（917 锚死）→②（578 轮心跳·owner
                #   永活形态的补足判据）→③清护动作（935·清护=True 且锚死时先整树回收
                #   死锚后代再接管）；三者不得互相削弱，各自审计文案保留。
                轮锚 = 陈锁信息.get("轮锚")
                锚死 = bool(owner) and not pid存活(owner)
                if 锚死:
                    因 = f"持锁存活锚 pid={owner} 已死（917 秒级接管）"
                elif 陈锁信息.get("轮心跳") and (轮锚 is None or not pid存活(轮锚)):
                    因 = (f"轮心跳停超 {轮心跳阈值()}s（578 轮级判据·"
                          f"轮锚 pid={轮锚} 已死/未登记——长驻持有者轮卡死/僵死）")
                else:
                    因 = f"心跳停超 {陈锁阈值 // 3600}h"
                print(f"[gate_lock] 陈锁接管（{因}：{陈锁信息.get('命令', '?')}）", file=sys.stderr)
                if 清护 and 锚死:
                    # 935（2026-10-17 两夜实录 379/932）：锚死≠其作业已停——multiprocessing
                    # spawn worker 在主进程被强杀后孤儿化（Windows 不级联杀），接管者与
                    # 「前代进程树」同树互踩。接管前先回收死锚的存活后代（清尸·幂等）。
                    被杀 = 清理死树(owner)
                    if 被杀:
                        print(f"[gate_lock] 清尸：死锚 pid={owner} 的后代残留 {len(被杀)} 个已回收"
                              f"（pid={','.join(map(str, 被杀))}·935）", file=sys.stderr)
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
        # 轮心跳（578）：长驻持有者（win 池 runner）登记「轮心跳」标记——owner 永活
        #   形态由轮心跳线程续约，轮卡死/僵死时排队方按判据③接管（原只能等 4h）。
        信息 = {"pid": os.getpid(), "形态": 形态, "owner_pid": owner_pid or os.getpid(),
                "命令": " ".join(sys.argv[1:]),
                "获取时间": time.strftime("%Y-%m-%d %H:%M:%S")}
        if 轮心跳:
            信息["轮心跳"] = True
            信息["轮锚"] = None
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
    归属不符（或锁已不在）即跳过不动。917 陈锁接管路径不传 仅属主——行为不变。
    578：清目录内全部残留（info.json + 轮心跳原子写残留的 info.json.tmp）再 rmdir
    ——原只删 info.json，残留文件使 rmdir 失败=锁清不掉。"""
    if 仅属主 is not None and 读信息(锁).get("owner_pid") != 仅属主:
        return
    try:
        for 残留 in 锁.glob("*"):
            try:
                残留.unlink()
            except OSError:
                pass
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
    域名 = "树锁" if 域 == "tree" else "机器锁"   # 566 文案对齐（test_tree_lock 断言中文域）

    if 参数.命令 == "status":
        if 锁目录.exists():
            信息 = 读信息(锁目录)
            owner = 信息.get("owner_pid") or (信息.get("pid") if 信息.get("形态") == "run" else None)
            锚态 = (f"锚 pid={owner} {'活' if pid存活(owner) else '死'}" if owner else "无锚（回退心跳 4h）")
            轮态 = ""
            if 信息.get("轮心跳"):   # 578：轮心跳判据可见（停跳秒数/轮锚/阈值）
                try:
                    停跳 = int(time.time() - (锁目录 / "info.json").stat().st_mtime)
                except OSError:
                    停跳 = -1
                轮锚 = 信息.get("轮锚")
                轮态 = (f" 轮心跳=停{停跳}s（轮锚 pid={轮锚}"
                        f"{' 活' if 轮锚 is not None and pid存活(轮锚) else ' 死/未登记'}"
                        f"·阈值 {轮心跳阈值()}s）")
            print(f"被持有：pid={信息.get('pid')} 形态={信息.get('形态', '?')} {锚态} "
                  f"命令={信息.get('命令')} 自 {信息.get('获取时间')}{轮态}"
                  f"{'（陈锁）' if 是陈锁(锁目录) else ''}")
        else:
            print("空闲")
        return 0
    if 参数.命令 == "release":
        if 豁免:
            # 555/566：豁免环境（嵌套子进程）的锁归外层持有者——删了=拆外层互斥，拒绝
            print(f"[gate_lock] 继承外层{域名}（{豁免变量}=1）——release 跳过（锁归外层持有者）",
                  file=sys.stderr)
            return 0
        释放(锁目录)
        return 0
    # 555：树域环境豁免——外层已持锁（CN_TREE_LOCK_HELD=1），acquire/run 直接跳过取锁
    #   （不建锁目录不等待），run 直跑透传退出码。防嵌套链自死锁（最高风险点）。
    #   566：machine 域同构豁免（CN_GATE_LOCK_HELD=1·审查 P0-2 根治死代码）。
    if 豁免:
        print(f"[gate_lock] 继承外层{域名}（{豁免变量}=1）", file=sys.stderr)
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
    锁 = 获取(0, 形态="run", owner_pid=os.getpid(), 锁=树锁目录(), 清护=True)
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
