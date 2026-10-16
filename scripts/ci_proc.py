#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ci_proc.py —— 轮级子进程树生命线（935·win 池 E2E 偶发死锁根治·进程树部分）。

背景（2026-10-17 两夜实录 379/932）：ci_daemon 经 subprocess.run(timeout=…) 管轮——
超时/异常时 Windows 只 TerminateProcess 直接子进程（不级联），multiprocessing spawn
worker 孤儿化（932 实录：并行轮主进程死后 6 个 worker 同睡 2h·CPU delta=0——与
「陈锁接管后的下一轮」同树互踩僵死的物质基础）。本模块提供「整树生命线」：

  · win  = Job Object（JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE）：子进程即起即装整树；
           超时=TerminateJobObject 灭全树；daemon 自身被强杀时句柄随进程回收、
           作业关闭连带灭树——人工清场从「杀进程树」降为「杀 daemon 一个」。
  · posix= start_new_session 进程组 + 超时 os.killpg 对称回收。

接口：起子(…) = 全子进程生命线低层入口（Popen+Job Object 装配·935 与 578
ci_round_guard 的融合点——跑追踪 经此启动，轮守卫追踪与整树回收双立）；运行树(…)
= subprocess.run 同构替代（流式日志形态）：超时抛 subprocess.TimeoutExpired（rc
语义由调用方 步骤() 记 -99·诚实红），超时路径落一行 [超时回收] 审计日志（不静默·
935 要求）；子进程默认注入 PYTHONUNBUFFERED=1（僵死现场可查——932/379 实录 E2E
输出因 stdout 块缓冲随僵死进程蒸发）。

降级面（显式声明·禁静默）：Job 创建/配置失败 → 日志一行告警 + 超时改 taskkill /T
整树兜底；单进程 assign 失败（进程已秒退等）静默返回 False——豁免边界=「无活进程
可管」或「已退出」，不影响超时兜底路径。
"""
from __future__ import annotations

import ctypes
import os
import signal
import subprocess
import sys

是win = sys.platform.startswith("win")

_JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x2000
_JobObjectExtendedLimitInformation = 9


if 是win:
    import ctypes.wintypes as _w

    class _IO计数(ctypes.Structure):
        _fields_ = [("读操作", ctypes.c_ulonglong), ("写操作", ctypes.c_ulonglong),
                    ("其他操作", ctypes.c_ulonglong), ("读字节", ctypes.c_ulonglong),
                    ("写字节", ctypes.c_ulonglong), ("其他字节", ctypes.c_ulonglong)]

    class _基本限制(ctypes.Structure):
        _fields_ = [("进程用户时间", ctypes.c_longlong), ("作业用户时间", ctypes.c_longlong),
                    ("限制标志", _w.DWORD), ("最小工作集", ctypes.c_size_t),
                    ("最大工作集", ctypes.c_size_t), ("活动进程上限", _w.DWORD),
                    ("亲和掩码", ctypes.c_size_t), ("优先级类", _w.DWORD),
                    ("调度类", _w.DWORD)]

    class _扩展限制(ctypes.Structure):
        _fields_ = [("基本", _基本限制), ("IO计数", _IO计数),
                    ("进程内存限", ctypes.c_size_t), ("作业内存限", ctypes.c_size_t),
                    ("进程内存峰值", ctypes.c_size_t), ("作业内存峰值", ctypes.c_size_t)]

    def _内核():
        k = ctypes.WinDLL("kernel32", use_last_error=True)
        k.CreateJobObjectW.restype = ctypes.c_void_p
        k.CreateJobObjectW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p]
        k.OpenProcess.restype = ctypes.c_void_p
        k.OpenProcess.argtypes = [ctypes.c_uint, ctypes.c_int, ctypes.c_uint]
        k.AssignProcessToJobObject.restype = ctypes.c_int
        k.AssignProcessToJobObject.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        k.SetInformationJobObject.restype = ctypes.c_int
        k.SetInformationJobObject.argtypes = [ctypes.c_void_p, ctypes.c_int,
                                              ctypes.c_void_p, ctypes.c_uint]
        k.TerminateJobObject.restype = ctypes.c_int
        k.TerminateJobObject.argtypes = [ctypes.c_void_p, ctypes.c_uint]
        k.CloseHandle.restype = ctypes.c_int
        k.CloseHandle.argtypes = [ctypes.c_void_p]
        return k


def 创建作业():
    """建 Job Object 并设 KILL_ON_JOB_CLOSE（句柄=生命线锚）。不可用=None（显式降级）。"""
    if not 是win:
        return None
    k = _内核()
    作业 = k.CreateJobObjectW(None, None)
    if not 作业:
        return None
    信息 = _扩展限制()
    信息.基本.限制标志 = _JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
    if not k.SetInformationJobObject(作业, _JobObjectExtendedLimitInformation,
                                     ctypes.byref(信息), ctypes.sizeof(信息)):
        k.CloseHandle(作业)
        return None
    return 作业


def 加入作业(作业, pid) -> bool:
    """把存活进程装进作业（此后其全部后代自动在作业内）。进程已秒退/无权限=False
    ——豁免边界：无活进程可管（超时兜底路径独立于本返回值）。"""
    if 作业 is None or not 是win:
        return False
    k = _内核()
    进程 = k.OpenProcess(0x0100 | 0x0001, False, int(pid))  # SET_QUOTA|TERMINATE
    if not 进程:
        return False
    try:
        return bool(k.AssignProcessToJobObject(作业, 进程))
    finally:
        k.CloseHandle(进程)


def 终止作业(作业, 退出码: int = 1) -> None:
    if 作业 is None or not 是win:
        return
    _内核().TerminateJobObject(作业, 退出码)


def 关闭作业(作业) -> None:
    """关句柄——KILL_ON_JOB_CLOSE：作业内残余进程（如有）随内核回收，堵「轮尾漏网」。"""
    if 作业 is None or not 是win:
        return
    _内核().CloseHandle(作业)


def 起子(命令: list, cwd=None, env=None, **popen_kwargs):
    """低层统一启动（935 融合点：全仓子进程生命线单一入口·578 轮守卫经 ci_round_guard
    .跑追踪 亦走此处）：Popen + Job Object 装配 + PYTHONUNBUFFERED 注入。
    返回 (进程, 作业, 装入)；调用方负责超时语义与收尾，用毕必须 关闭作业
    （KILL_ON_JOB_CLOSE：残余后代随句柄回收）。popen_kwargs 透传 Popen（stdout/
    stderr/creationflags/text 等）；env 显式传入=以此为基础（仍注入 PYTHONUNBUFFERED）。"""
    作业 = 创建作业()
    环境 = dict(env if env is not None else os.environ)
    环境.setdefault("PYTHONUNBUFFERED", "1")
    if 是win and 作业 is None:
        print("  [作业] Job Object 不可用——降级：超时以 taskkill /T 整树兜底",
              file=sys.stderr, flush=True)
    try:
        p = subprocess.Popen(命令, cwd=cwd, env=环境, start_new_session=(not 是win),
                             **popen_kwargs)
    except BaseException:      # 命令不存在/中断等：作业空载关闭，防句柄泄漏
        关闭作业(作业)
        raise
    装入 = 加入作业(作业, p.pid) if 作业 is not None else False
    return p, 作业, 装入


def 运行树(命令: list, 日志, 超时秒: int, cwd=None, **kwargs) -> subprocess.CompletedProcess:
    """subprocess.run 同构替代 + 整树生命线（流式日志形态·模块头详述）。kwargs 透传
    Popen（stdout/stderr 已由本函数固定为 日志/合并）。超时：整树回收后抛 TimeoutExpired。"""
    p, 作业, 装入 = 起子(命令, cwd=cwd, env=kwargs.pop("env", None),
                         stdout=日志, stderr=subprocess.STDOUT, **kwargs)
    try:
        rc = p.wait(timeout=超时秒)
    except subprocess.TimeoutExpired:
        超时回收(p, 作业, 装入, 日志, 超时秒)
        raise
    finally:
        关闭作业(作业)
    return subprocess.CompletedProcess(命令, rc)


def 超时回收(p, 作业, 装入: bool, 日志, 超时秒: int) -> None:
    """超时路径：整树回收 + 尸体收敛 + 一行 [超时回收] 审计（935 要求：幂等·可审计·
    不静默）。935+578 融合后由 运行树（流式）与 ci_round_guard.跑追踪（捕获/流式）
    共用；日志=None/不可写（capture 模式）=审计落 stderr（daemon runner 日志可审计）。"""
    if 作业 is not None:
        终止作业(作业)
        方式 = "Job Object 整树终止"
    elif 是win:
        subprocess.run(["taskkill", "/T", "/F", "/PID", str(p.pid)],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        方式 = "taskkill /T 整树"
    else:
        try:
            os.killpg(p.pid, signal.SIGKILL)
        except OSError:
            try:
                os.kill(p.pid, signal.SIGKILL)
            except OSError:
                pass
        方式 = "killpg 进程组"
    if not 装入 and 作业 is not None:
        # 主进程未装入作业（秒退竞态成功重开时已错过）——补 taskkill /T 兜其树
        if 是win:
            subprocess.run(["taskkill", "/T", "/F", "/PID", str(p.pid)],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        p.kill()          # 直接子兜底（作业缺失且整树杀失败时仍保底）
    except OSError:
        pass
    try:
        p.wait(timeout=30)    # 尸体收敛（subprocess.run 超时同语义）
    except (subprocess.TimeoutExpired, OSError):
        pass
    流 = 日志 if hasattr(日志, "write") else sys.stderr
    print("  [超时回收] 超时 %ds——已整树回收（%s·主 pid=%d）" % (超时秒, 方式, p.pid),
          file=流, flush=True)
