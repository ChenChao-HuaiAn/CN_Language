#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_round_heartbeat.py —— 578 轮心跳（轮级判据③）自测：纯 python·自跑自断言·exit 0/1。

零触碰真实锁（CN_GATE_LOCK_DIR 指进临时目录·环境变量收窄阈值加速）；场景：
  ① 判据矩阵：是陈锁 直调七态（轮心跳标记×轮锚死活×mtime 新旧×owner 死活）；
  ② 端到端·轮停接管：A 持锁+轮心跳探测假（轮卡死模拟）→ B acquire 分钟级接管；
  ③ 端到端·活轮不误清：A 轮心跳持续续约（探测真）→ B acquire --timeout 超时 rc=2；
  ④ 跑追踪等价：rc/输出/超时 TimeoutExpired/FileNotFoundError 与 subprocess.run 对照
    + 注册/注销/探测三态（活子进程/进度新鲜/进度陈旧）；
  ⑤ daemon 接线结构锁：运行/运行限时 走 跑追踪、带门禁锁跑 登记轮心跳+起停、
    预验清产物 登记进度（文本锁·防接线回退）。
用法：python scripts/test_round_heartbeat.py   （rc=0=全绿）
"""
import json
import os
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
GATE_LOCK = 脚本目录 / "gate_lock.py"
sys.path.insert(0, str(脚本目录))
import gate_lock        # noqa: E402  （判据/常量复用·与被测物同一实现）
import ci_round_guard   # noqa: E402

通过 = 0


def 断言(条件, 说明):
    global 通过
    assert 条件, "自测失败：" + 说明
    通过 += 1
    print("  ✓ %s" % 说明)


def 造锁(目录: Path, info: dict, mtime偏移秒: float = 0.0) -> Path:
    """造一把含指定 info 的锁目录；mtime偏移秒=把 info.json 的 mtime 推到过去。"""
    锁 = 目录 / "gate.lock"
    锁.mkdir(parents=True)
    (锁 / "info.json").write_text(json.dumps(info, ensure_ascii=False), encoding="utf-8")
    if mtime偏移秒:
        时刻 = time.time() - mtime偏移秒
        os.utime(锁 / "info.json", (时刻, 时刻))
    return 锁


def 死pid() -> int:
    """取一个已确定死去的 pid（起短命进程等退出·同 test_tree_lock 场景②手法）。"""
    p = subprocess.Popen([sys.executable, "-c", "pass"])
    p.wait()
    return p.pid


def 净环境() -> dict:
    环境 = dict(os.environ)
    for k in ("CN_GATE_LOCK_DIR", "CN_ROUND_HB_TIMEOUT", "CN_ROUND_HB_INTERVAL"):
        环境.pop(k, None)
    return 环境


def 场景1_判据矩阵(临时: str) -> None:
    基线 = {"pid": 12345, "形态": "acquire", "owner_pid": os.getpid(),
            "命令": "自测·判据矩阵", "获取时间": "自测"}
    老 = 400          # 超轮心跳阈值（默认 300s）、未超 4h
    死, 活 = 死pid(), os.getpid()
    例 = [
        ("轮心跳+轮锚死+mtime 老 → 陈锁", {**基线, "轮心跳": True, "轮锚": 死}, 老, True),
        ("轮心跳+轮锚未登记+mtime 老 → 陈锁", {**基线, "轮心跳": True, "轮锚": None}, 老, True),
        ("轮心跳+轮锚活+mtime 老 → 不判死（子进程在跑=活锁·防误清）",
         {**基线, "轮心跳": True, "轮锚": 活}, 老, False),
        ("轮心跳+轮锚死+mtime 新鲜 → 不判死（刚退出窗口·阈值内）",
         {**基线, "轮心跳": True, "轮锚": 死}, 0, False),
        ("无轮心跳+mtime 老（<4h）→ 旧行为不判死", 基线, 老, False),
        ("无轮心跳+mtime 超 4h → 原判据②仍生效", 基线, 4 * 3600 + 60, True),
        ("owner 死 → 判据①优先（mtime 再新鲜也立判）",
         {**基线, "owner_pid": 死}, 0, True),
    ]
    for i, (说明, 信息, 偏, 期望) in enumerate(例):
        锁 = 造锁(Path(tempfile.mkdtemp(prefix="rh1_%d_" % i, dir=临时)), 信息, 偏)
        得 = gate_lock.是陈锁(锁)
        断言(得 == 期望, "场景① " + 说明)


def 起A进程(锁目录: Path, 探测源码: str) -> subprocess.Popen:
    """起「持有者」模拟进程：获取 acquire 锁（owner=自身·轮心跳登记）+ 轮心跳线程。
    探测源码=注入的探测 lambda 源码文本（如 '(lambda: (None, False))'）。"""
    片段 = (
        "import os, sys, time\n"
        "sys.path.insert(0, %r)\n"
        "import gate_lock\n"
        "锁 = gate_lock.获取(0, 形态='acquire', owner_pid=os.getpid(), 轮心跳=True)\n"
        "assert 锁 is not None, 'A 获取失败'\n"
        "gate_lock.启动轮心跳(锁, 探测=%s)\n"
        "print('A-HOLD', flush=True)\n"
        "time.sleep(180)\n"
    ) % (str(脚本目录), 探测源码)
    环境 = {**净环境(),
            "CN_GATE_LOCK_DIR": str(锁目录),
            "CN_ROUND_HB_TIMEOUT": "4",
            "CN_ROUND_HB_INTERVAL": "1"}
    A = subprocess.Popen([sys.executable, "-c", 片段], env=环境,
                         stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    for _ in range(100):        # 等 A 真持锁（python 双层启动 ~1s）
        if (锁目录 / "info.json").exists():
            return A
        time.sleep(0.1)
    raise AssertionError("自测失败：A 进程未在 10s 内持锁")


def 跑B(锁目录: Path, 超时: str) -> subprocess.CompletedProcess:
    环境 = {**净环境(),
            "CN_GATE_LOCK_DIR": str(锁目录),
            "CN_ROUND_HB_TIMEOUT": "4",
            "CN_ROUND_HB_INTERVAL": "1"}
    return subprocess.run([sys.executable, str(GATE_LOCK), "acquire", "--timeout", 超时],
                          capture_output=True, text=True, encoding="utf-8",
                          errors="replace", env=环境, timeout=120)


def 场景2_轮停接管(临时: str) -> None:
    锁目录 = Path(tempfile.mkdtemp(prefix="rh2_", dir=临时)) / "gate.lock"
    # A：owner 活（永远活·917 判据①失效形态）+ 探测假（轮卡死：无活子进程/进度停滞）
    A = 起A进程(锁目录, "(lambda: (None, False))")
    起 = time.time()
    B = 跑B(锁目录, "40")
    耗时 = time.time() - 起
    A.kill()
    A.wait(timeout=30)
    断言(B.returncode == 0, "场景② 排队者接管成功 rc=0（原只能等 4h）")
    断言(耗时 < 20, f"场景② 分钟级接管（{耗时:.1f}s < 20s·阈值 4s+探测周期）")
    断言("轮心跳" in (B.stderr or ""), "场景② 接管原因=轮心跳判据③可见")
    # B 拿锁即退残留——清场（同环境 release）
    跑B_release = subprocess.run([sys.executable, str(GATE_LOCK), "release"],
                                 capture_output=True, text=True, env={**净环境(),
                                 "CN_GATE_LOCK_DIR": str(锁目录)}, timeout=60)
    断言(跑B_release.returncode == 0, "场景② 清场 release rc=0")


def 场景3_活轮不误清(临时: str) -> None:
    锁目录 = Path(tempfile.mkdtemp(prefix="rh3_", dir=临时)) / "gate.lock"
    # A：探测真+轮锚活（轮在推进：有活子进程=持续续约）——B 必须等（不误清活锁）
    A = 起A进程(锁目录, "(lambda: (os.getpid(), True))")
    B = 跑B(锁目录, "8")
    A.kill()
    A.wait(timeout=30)
    断言(B.returncode == 2, "场景③ 活轮持续续约——B 超时 rc=2（不误清·互斥保持）")
    跑B_release = subprocess.run([sys.executable, str(GATE_LOCK), "release"],
                                 capture_output=True, text=True, env={**净环境(),
                                 "CN_GATE_LOCK_DIR": str(锁目录)}, timeout=60)
    断言(跑B_release.returncode == 0, "场景③ 清场 release rc=0")


def 场景4_跑追踪等价(临时: str) -> None:
    守 = ci_round_guard.守
    命令 = [sys.executable, "-c", "import sys; print('out-ok'); sys.exit(7)"]
    真 = subprocess.run(命令, capture_output=True, text=True, timeout=30)
    得 = 守.跑追踪(命令, 30, capture_output=True, text=True)
    断言(得.returncode == 真.returncode == 7, "场景④ rc 等价（7）")
    断言(得.stdout == 真.stdout == "out-ok\n", "场景④ stdout 捕获等价")
    断言(得.stderr == 真.stderr == "", "场景④ stderr 捕获等价")
    try:
        守.跑追踪([sys.executable, "-c", "import time; time.sleep(30)"], 1,
                 capture_output=True, text=True)
        断言(False, "场景④ 超时应抛 TimeoutExpired")
    except subprocess.TimeoutExpired:
        断言(True, "场景④ 超时抛 TimeoutExpired（daemon 运行限时 转 rc=-99）")
    断言(not 守._活跃, "场景④ 超时路径已注销（表不残留）")
    try:
        守.跑追踪(["不存在的可执行_zz_xyz"], 5)
        断言(False, "场景④ 缺可执行应抛 FileNotFoundError")
    except FileNotFoundError:
        断言(True, "场景④ 缺可执行抛 FileNotFoundError（daemon 运行 转 rc=127）")
    # 注册/注销 + 探测：后台线程跑 3s 子进程，主线程探测应看到活 pid
    结果 = {}

    def 后台():
        结果["r"] = 守.跑追踪([sys.executable, "-c", "import time; time.sleep(3)"], 30)

    t = threading.Thread(target=后台)
    t.start()
    time.sleep(1.5)
    轮锚, 该刷 = 守.探测()
    断言(该刷 and 轮锚 is not None and gate_lock.pid存活(轮锚),
         "场景④ 子进程在表 → 探测(活 pid, 该刷新=真)")
    t.join(30)
    断言(结果["r"].returncode == 0, "场景④ 追踪子进程正常结束")
    轮锚2, 该刷2 = 守.探测()
    断言(该刷2 and 轮锚2 is None, "场景④ 子进程退出+进度新鲜 → 探测(None, 真·步骤间隙)")
    守._进度 = time.time() - (gate_lock.轮心跳阈值() + 100)   # 模拟进度停滞
    轮锚3, 该刷3 = 守.探测()
    断言(not 该刷3 and 轮锚3 is None, "场景④ 进度停超阈值 → 探测假（轮卡死信号）")
    守.进度()   # 复位（防污染后续场景）


def 场景5_daemon接线(临时: str) -> None:
    import inspect
    import ci_daemon
    运行源 = inspect.getsource(ci_daemon.运行)
    限时源 = inspect.getsource(ci_daemon.运行限时)
    守护运行源 = inspect.getsource(ci_round_guard.运行)
    守护限时源 = inspect.getsource(ci_round_guard.运行限时)
    带锁源 = inspect.getsource(ci_daemon.带门禁锁跑)
    清源 = inspect.getsource(ci_daemon.预验清产物)
    断言("ci_round_guard.运行" in 运行源 and "跑追踪" in 守护运行源,
         "场景⑤ 运行 薄壳转发 → 轮守卫.跑追踪（子进程注册/注销）")
    断言("ci_round_guard.运行限时" in 限时源 and "跑追踪" in 守护限时源,
         "场景⑤ 运行限时 薄壳转发 → 轮守卫.跑追踪")
    断言("轮心跳=True" in 带锁源, "场景⑤ 带门禁锁跑 获取登记轮心跳标记")
    断言(".起(" in 带锁源 and ".停()" in 带锁源, "场景⑤ 带门禁锁跑 轮心跳起/停接线")
    断言("进度()" in 清源, "场景⑤ 预验清产物 登记进度（长纯 Python 段防误判）")
    # 释放清残留（578 全清）行为锁：info.json.tmp 残留也要能 rmdir
    锁目录 = Path(tempfile.mkdtemp(prefix="rh5_", dir=临时)) / "gate.lock"
    锁 = gate_lock.获取(0, 形态="run", 锁=锁目录)
    (锁 / "info.json.tmp").write_text("{}", encoding="utf-8")   # 模拟原子写残留
    gate_lock.释放(锁, 静默=True)
    断言(not 锁目录.exists(), "场景⑤ 释放清残留（info.json.tmp）后锁目录删净")


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="cn_round_hb_test_") as 临时:
        print("== 场景① 判据矩阵（轮心跳×轮锚×mtime×owner） ==")
        场景1_判据矩阵(临时)
        print("== 场景② 轮停接管（探测假 → 排队者分钟级接管） ==")
        场景2_轮停接管(临时)
        print("== 场景③ 活轮不误清（探测真续约 → 排队者超时不接管） ==")
        场景3_活轮不误清(临时)
        print("== 场景④ 跑追踪等价（subprocess.run 对照+注册/注销/探测） ==")
        场景4_跑追踪等价(临时)
        print("== 场景⑤ daemon 接线结构锁 + 释放清残留 ==")
        场景5_daemon接线(临时)
    print(f"\n[全部通过] {通过} 项断言")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as 异常:
        print(f"\n[自测失败] {异常}")
        raise SystemExit(1)
    except KeyboardInterrupt:
        print("\n[自测中断]")
        raise SystemExit(130)
