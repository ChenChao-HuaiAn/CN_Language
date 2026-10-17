#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""e2e看门狗 —— run_e2e 内部看门狗三件套（978·935 后续·本夜第三实录 914）。

背景（379/932/914 三次僵死实录）：
  · 914 win2 现场（2026-10-17 11:07 轮·C:/CN_CI/win-pool/wt2）：986 用例中 985 个
    报告完毕（78_v2 为最后一条输出），此后日志停更 30 分钟；11:47:2x run_e2e 主进程
    以退出码 0xFFFFFFFF 横死（无 E2E 汇总/无 tree lock 释放/无 --json 明细，daemon
    收到 rc=4294967295 → [红即停]），6 个 spawn worker 孤儿化静默睡眠（任务书实录
    「6 worker 全部静默挂起·CPU delta=0·无编译器子进程活动」）；13:31 下一轮 917
    陈锁接管（锚 pid=29072 已死）才恢复。
  · 本模块立项前实测（target/probe 对照实验·本机）：`Stop-Process -Force` 杀
    spawn-Pool 主进程 → 退出码 4294967295（0xFFFFFFFF = .NET Process.Kill 语义，
    与 914 result 逐位一致）+ 6 个 worker 父死后 ≥10s 全部存活（孤儿化 100% 复现）；
    win 上 taskkill /F=1、os.kill(SIGTERM)=15、Popen.terminate()=1——0xFFFFFFFF
    唯一匹配外部 Stop-Process 形态。
  · 935 已给 daemon 层整树生命线（Job Object）+ 陈锁清尸 + 行缓冲；本轮补
    run_e2e **进程内部**的自愈与观测面（不依赖 daemon 5400s 兜底）。

三件套（run_e2e 注入点见各函数 docstring）：
  ① 用例心跳（worker 侧 刷心跳）+ 逐用例静默判挂（主进程 判挂死/读心跳）：
     worker 在执行面（每个 运行命令 进出+内存轮询循环）刷「阶段心跳」文件；
     主进程在收集循环中按静默阈值判「该用例挂死」→ 判红并从等待集移除（不拖死
     整池），诊断输出最后目击阶段（935「僵死可观测」的延续——再犯时一行定位卡点）。
  ② 孤儿 worker 自愈（起孤儿监控）：spawn worker 顶层挂父进程监控线程——
     multiprocessing.parent_process().is_alive()（sentinel 句柄级判活·CPython 3.14
     process.py 实测实现·无 pid 复用问题）；父进程以任何方式死亡（含被外部
     Stop-Process 强杀）worker 秒级 os._exit(3) 自退，杜绝「6 worker 静默睡 2h」。
  ③ 池收尾带界等待（等待池worker退出）：close/terminate 后等 worker 退出均有
     超时——僵死 worker 超时即强制回收，收尾路径永不挂死主进程（原 池.join()
     无超时：僵死 worker 在场时主进程永久悬挂=914 形态的另一种）。

边界声明（显式·禁静默；本模块为测试基建观测面，机械门禁不扫 tests/，
条文自觉适用）：
  · 判挂只对「已开工」用例生效（心跳文件存在）——排队中用例由既有整池停摆
    看门狗（--stall-timeout）负责；同一 worker 卡死后续未开工用例无 worker 可跑
    =整池零完成→停摆看门狗收口（不漏判）。判挂后不重建池（卡死 worker 占位由
    停摆兜底；单杀 worker 不在 multiprocessing 公开 API 面内），收尾时 terminate
    统一回收。
  · 心跳写入失败（磁盘/权限）静默忽略=观测面豁免：心跳失败不得影响被测用例
    语义（防线自身故障不阻断被测物）；此时判据侧=「无心跳=未开工不判」，用例
    本体仍有各步显式超时防线（编译 600s/构建 1500s/运行 180s）。
  · 心跳文件 mtime=判据（NTFS 精度 100ns·与本进程同机时钟）；文件内容=阶段
    文本，仅供诊断显示（撕裂读取无害）。心跳目录随目标目录（target/·gitignore）
    ——池启动前 清空心跳目录 防陈旧混淆。
  · 孤儿监控线程对「不释放 GIL 的 C 层死锁」不生效（纯 Python 死锁/子进程等待
    均释放 GIL，可生效）——该极端形态由 ①③ 双兜底。
"""
from __future__ import annotations

import os
import pathlib
import threading
import time

# 心跳目录名（输出目录=target/ 之下·随 target/ gitignore）
心跳目录名 = ".e2e_hb"

# 孤儿监控轮询间隔秒（父死到 worker 自退的上界=本值+判活调用耗时）
孤儿监控间隔秒 = 2

# 当前用例心跳路径（worker 侧全局·设当前心跳路径 写入；主进程保持 None）
当前心跳路径 = None


# ─────────────────────────── ① 心跳（worker 写 / 主进程读） ───────────────────────────

def 心跳目录(输出目录: pathlib.Path) -> pathlib.Path:
    """该次运行的用例心跳目录（<输出目录>/.e2e_hb）。"""
    return pathlib.Path(输出目录) / 心跳目录名


def 清空心跳目录(输出目录: pathlib.Path) -> None:
    """池启动前清空（防上一轮/旧进程陈旧心跳混淆判据）。删除失败显式告警不静默。"""
    import shutil
    目录 = 心跳目录(输出目录)
    try:
        if 目录.exists():
            shutil.rmtree(目录)
        目录.mkdir(parents=True, exist_ok=True)
    except OSError as 异常:
        print(f"[看门狗] 心跳目录清理失败（{目录}）：{异常}——本轮判据可能受旧心跳影响",
              flush=True)


def 心跳路径(输出目录: pathlib.Path, 用例名: str) -> pathlib.Path:
    """用例名→心跳文件路径（用例名=目录名·文件系统安全·一用例一文件）。"""
    return 心跳目录(输出目录) / (用例名 + ".hb")


def 设当前心跳路径(路径) -> None:
    """worker 侧设置当前用例心跳路径（用例入口调用；None=关闭心跳）。"""
    global 当前心跳路径
    当前心跳路径 = pathlib.Path(路径) if 路径 is not None else None


def 刷心跳(阶段: str) -> None:
    """worker 侧刷心跳：写阶段文本（mtime 随写更新）。主进程（路径 None）空过。
    OSError 静默忽略=观测面豁免（见模块头边界声明）——心跳失败不影响被测语义。"""
    if 当前心跳路径 is None:
        return
    try:
        当前心跳路径.write_text(f"{time.time():.3f} {阶段}\n", encoding="utf-8")
    except OSError:
        pass


def 读心跳(路径: pathlib.Path):
    """读心跳阶段文本（诊断用）。返回 (mtime, 阶段) 或 None（缺失/不可读）。"""
    try:
        信息 = 路径.stat()
        文本 = 路径.read_text(encoding="utf-8", errors="replace").strip()
    except OSError:
        return None
    阶段 = 文本.split(" ", 1)[1] if " " in 文本 else 文本
    return 信息.st_mtime, 阶段


def 判挂死(路径: pathlib.Path, 静默阈值秒: float) -> str:
    """逐用例静默判据。返回 ""（未判挂）或判挂原因（含最后目击阶段与静默秒数）。

    判据：心跳文件存在（=已开工）且 mtime 静默超过阈值 → 挂死。
    未开工（无心跳文件）=不判（排队中·整池停摆看门狗兜底；见模块头边界声明）。
    """
    try:
        信息 = 路径.stat()
    except OSError:
        return ""
    静默 = time.time() - 信息.st_mtime
    if 静默 <= 静默阈值秒:
        return ""
    心跳 = 读心跳(路径)
    if 心跳 is None:
        return f"单用例静默超时（心跳不可读·静默 {静默:.0f}s > {静默阈值秒:.0f}s）"
    阶段 = 心跳[1] or "未知"
    return (f"单用例静默超时（{静默:.0f}s 无心跳 > {静默阈值秒:.0f}s·"
            f"最后阶段: {阶段}）")


# ─────────────────────── ③ 池收尾带界等待（僵死 worker 强制回收） ───────────────────────

def 等待池worker退出(秒: float) -> bool:
    """等本进程全部池 worker 退出（带界）。True=全退；False=超时（仍有存活）。

    用 multiprocessing.active_children()（公开 API·顺带清理已完成子进程尸体）
    按进程名过滤（Pool worker 名含 "PoolWorker"·spawn/fork 两形态；resource_tracker
    等其它子进程不受影响）——不依赖 Pool 私有属性。
    """
    import multiprocessing
    截止 = time.monotonic() + 秒
    while True:
        存活 = [p for p in multiprocessing.active_children()
                if "PoolWorker" in p.name]
        if not 存活:
            return True
        if time.monotonic() >= 截止:
            return False
        time.sleep(0.5)


# ─────────────────────────── ② 孤儿 worker 自愈（父死自杀） ───────────────────────────

_孤儿监控已挂 = False


def _pid存活(pid: int) -> bool:
    """降级判活（win=OpenProcess+GetExitCodeProcess；posix=os.kill(pid,0)）。
    仅用于 parent_process() 异常未就绪的降级通道（见 起孤儿监控 docstring）。"""
    if os.name == "nt":
        import ctypes
        PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
        句柄 = ctypes.windll.kernel32.OpenProcess(
            PROCESS_QUERY_LIMITED_INFORMATION, False, int(pid))
        if not 句柄:
            return False
        try:
            码 = ctypes.c_ulong()
            if not ctypes.windll.kernel32.GetExitCodeProcess(句柄, ctypes.byref(码)):
                return True  # 查询失败=不误判（宁可长眠不误杀）
            return 码.value == 259  # STILL_ACTIVE
        finally:
            ctypes.windll.kernel32.CloseHandle(句柄)
    try:
        os.kill(int(pid), 0)
        return True
    except ProcessLookupError:
        return False
    except PermissionError:
        return True


def 起孤儿监控() -> None:
    """spawn worker 启动时调用（run_e2e 顶层 `if __name__ == "__mp_main__":` 挂载；
    并行用例任务 入口再调一次=幂等双保险）。父死 → os._exit(3) 立即自退——不跑
    finalizer（强杀场景下 multiprocessing 退出钩子无意义，且跳过可避免资源泄漏
    误报噪声）。

    时序（本机实测硬证据）：spawn worker 顶层 import 主模块期间
    multiprocessing.parent_process() 尚未就绪（返回 None——spawn 协议在
    _bootstrap 后才设置；os.getppid() 已可用的时序）——监控线程先等就绪
    （毫秒级），60s 仍未就绪（理论不可达）则打印一行降级诊断并改用父 pid 判活
    （显式降级非静默；pid 复用误判方向=监控长眠·不误杀 worker）。
    判活=parent_process().is_alive()（sentinel 句柄级·无 pid 复用问题）。
    边界：即使监控因异常未挂，孤儿形态仍有 917 陈锁接管清尸（下一轮·分钟~小时级）
    与池树 daemon 侧防线兜底——本监控把「孤儿存活窗口」从小时级压到秒级。
    is_alive() 抛异常（罕见）=本轮按存活处理继续轮询（不误杀方向）。
    """
    global _孤儿监控已挂
    if _孤儿监控已挂:
        return
    _孤儿监控已挂 = True

    def 监控():
        import multiprocessing
        父 = None
        等待截止 = time.monotonic() + 60
        while 父 is None and time.monotonic() < 等待截止:
            time.sleep(0.5)
            父 = multiprocessing.parent_process()
        if 父 is not None:
            父存活 = 父.is_alive
        else:
            import sys
            print("[看门狗] parent_process() 60s 未就绪——降级父 pid 判活（978 边界）",
                  file=sys.stderr, flush=True)
            父进程号 = os.getppid()

            def 父存活():
                return _pid存活(父进程号)
        while True:
            time.sleep(孤儿监控间隔秒)
            try:
                存活 = 父存活()
            except Exception:
                存活 = True
            if not 存活:
                os._exit(3)

    threading.Thread(target=监控, name="孤儿监控", daemon=True).start()
