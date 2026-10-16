#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ci_round_guard.py —— 578：轮守卫（win 池 runner 子进程追踪 + 轮心跳联动）。

背景（任务 #578）：win 池 runner（ci_daemon.py --loop 常驻）以自身进程作 gate_lock
的 acquire 存活锚——owner_pid 永活使 917「锚死→秒级接管」判据对「锚活+轮卡」退化 4h
（实录：daemon 持锁但子进程消失僵死 55 分钟·锁无可清判据）。本模块把「轮活着」编码
为两路信号，gate_lock 判据③（轮心跳）消费：
  ① 活跃子进程表：daemon 全部子进程点（运行/运行限时）经 跑追踪 启动——pid 注册/
     退出注销；有活子进程=轮在推进（构建/E2E 长命令全程覆盖）。
  ② 主线程进度时刻：注册/注销时刷新 + 长纯 Python 段（预验清产物）显式 进度()——
     无活子进程但进度新鲜（步骤间隙）=仍在推进。
探测() 合成 (轮锚 pid 或 None, 该刷新 bool)：该刷新=真 → gate_lock 轮心跳线程续约
（touch/轮锚刷新）；假（无活子进程且进度停超 gate_lock.轮心跳阈值）=轮卡死/僵死 →
停续→排队方接管（原 4h 退化 ⇒ 分钟级）。

跑追踪 = subprocess.run 的逐行等价实现（Popen + communicate + 超时 kill 语义照抄
CPython run 源码），仅多注册/注销钩子——调用方（daemon 运行/运行限时）行为零变化。
"""
from __future__ import annotations

import subprocess
import sys
import threading
import time

import gate_lock          # 轮心跳阈值/pid存活 复用（同目录·daemon 启动时 scripts/ 已在 sys.path）

本机win = sys.platform.startswith("win")


class 轮守卫:
    """daemon 进程级单例（模块底部 守）：_锁 保护两字段，多线程安全。
    主线程写（_注册/_注销/进度），轮心跳线程读（探测）。"""

    def __init__(self) -> None:
        self._锁 = threading.Lock()
        self._活跃 = {}          # pid -> 启动时刻
        self._进度 = time.time() # 最近主线程进度（子进程注册/注销·显式段登记）
        self._停 = None          # 轮心跳 stop 事件

    # ── 信号维护 ──
    def 进度(self) -> None:
        """显式进度登记（长纯 Python 段调用：预验清产物 等）——防步骤间隙误判。"""
        with self._锁:
            self._进度 = time.time()

    def _注册(self, pid: int) -> None:
        with self._锁:
            self._活跃[pid] = time.time()
            self._进度 = time.time()

    def _注销(self, pid: int) -> None:
        with self._锁:
            self._活跃.pop(pid, None)
            self._进度 = time.time()

    def 探测(self):
        """轮心跳线程回调：(轮锚 pid 或 None, 该刷新 bool)。
        活子进程存在=轮在推进（轮锚=该 pid·必刷）；无活子进程但进度新鲜于阈值=
        步骤间隙/纯 Python 段（不刷轮锚·该刷）；否则=轮卡死/僵死（不刷→心跳陈旧）。"""
        with self._锁:
            活跃 = list(self._活跃)
            进度 = self._进度
        for pid in 活跃:
            if gate_lock.pid存活(pid):
                return pid, True
        if time.time() - 进度 < gate_lock.轮心跳阈值():
            return None, True
        return None, False

    # ── 轮心跳接线（ci_daemon.带门禁锁跑 用） ──
    def 起(self, 锁) -> None:
        """持锁后启动轮心跳线程（gate_lock.启动轮心跳·探测=本守卫）。"""
        self.停()
        self._停 = gate_lock.启动轮心跳(锁, self.探测)

    def 停(self) -> None:
        """轮结束（finally）停止续约——缺失/重复调用均安全。"""
        if self._停 is not None:
            self._停.set()
            self._停 = None

    # ── 子进程追踪（daemon 运行/运行限时 底层） ──
    def 跑追踪(self, 命令, 超时秒, **kwargs) -> subprocess.CompletedProcess:
        """subprocess.run 等价（CPython 源码平移）+ 注册/注销钩子。

        超时=kill +（win：communicate 补输出；posix：wait）后 raise TimeoutExpired
        ——与 run 逐行同口径；capture_output 翻译与 run 一致（stdout=PIPE/PIPE）；
        FileNotFoundError 由 Popen 原样抛出（调用方照既有 except 捕获）。
        注册先于 communicate、注销在 finally——子进程全程在表（轮心跳据此判活）。"""
        if kwargs.pop("capture_output", False):
            kwargs["stdout"] = subprocess.PIPE
            kwargs["stderr"] = subprocess.PIPE
        with subprocess.Popen(命令, **kwargs) as 进程:
            self._注册(进程.pid)
            try:
                try:
                    出, 误 = 进程.communicate(timeout=超时秒)
                except subprocess.TimeoutExpired as e:
                    进程.kill()
                    if 本机win:
                        # Windows 输出在子线程累积——kill 后 communicate 收尾并补进异常
                        e.stdout, e.stderr = 进程.communicate()
                    else:
                        # POSIX _communicate 已把已读输出填进异常
                        进程.wait()
                    raise
            finally:
                self._注销(进程.pid)
            码 = 进程.poll()
        return subprocess.CompletedProcess(进程.args, 码, 出, 误)


# ── daemon 步骤运行（294/297 语义·578 自 ci_daemon 外迁防行数红线〔660 先例〕） ──
#   两函数原为 ci_daemon 模块级（猴补/调用面保持：daemon 留薄壳转发）；外迁不改行为。

def 运行限时(命令: list, 超时秒: int, **kwargs) -> subprocess.CompletedProcess:
    """subprocess.run 的超时安全壳（294）：超时返回伪 CompletedProcess（rc=-99·与 步骤()
    超时标记同口径）而非抛 TimeoutExpired 炸进程——单位机实例实况：网络抖动 git fetch
    300s 超时未捕获，systemd restart 循环 12 次（轮结果与审计全丢）。
    578：底层=跑追踪（subprocess.run 逐行等价 + 子进程注册/注销）——轮心跳探测据此
    判轮内活跃子进程，行为面零变化。"""
    try:
        return 守.跑追踪(命令, 超时秒, **kwargs)
    except subprocess.TimeoutExpired as e:
        出 = e.stdout if isinstance(e.stdout, str) else (e.stdout or b"").decode("utf-8", "replace")
        return subprocess.CompletedProcess(命令, -99, stdout=出[-400:], stderr="超时 %ds" % 超时秒)


def 运行(命令: list, 日志, 超时秒: int, 工作目录, **kwargs) -> subprocess.CompletedProcess:
    """流式写日志（不落内存大块）+总超时看护；可执行不存在（FileNotFoundError·如服务
    环境 PATH 缺用户级 cmake——单位机实例实录崩循环）记 rc=127 轮红而非炸进程（297·
    与 294 运行限时同族：环境面异常一律步骤红·不进程崩）。578：底层=跑追踪。"""
    print("  $", " ".join(命令[:6]), ("..." if len(命令) > 6 else ""), file=日志, flush=True)
    try:
        return 守.跑追踪(命令, 超时秒, stdout=日志, stderr=subprocess.STDOUT,
                        cwd=工作目录, **kwargs)
    except FileNotFoundError as e:
        print("  [FAIL] 可执行不存在：%s" % e, file=日志, flush=True)
        return subprocess.CompletedProcess(命令, 127)


守 = 轮守卫()
