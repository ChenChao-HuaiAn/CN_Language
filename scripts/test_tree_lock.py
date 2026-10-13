#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_tree_lock.py —— 树锁（555）自测：纯 python·自跑自断言·exit 0/1。

零触碰真实锁：树锁/machine 锁目录分别用 CN_TREE_LOCK_DIR / CN_GATE_LOCK_DIR
指进临时目录；四场景：
  ① 并发竞争：A 持锁期间 B 排队，A 释放后 B 才获得（时序断言）；
  ② 陈锁接管：持锁进程被 kill → 排队者秒级接管（gate_lock 917 存活锚判据）；
  ③ 环境豁免与注入：CN_TREE_LOCK_HELD=1 → acquire/run 立即通过且不建锁目录；
    run 形态 tree 域向子进程注入该变量、machine 域不注入；
  ④ 域隔离：tree 锁不拦 machine 域；status --scope tree 可用；release 后可再取。
用法：python scripts/test_tree_lock.py   （rc=0=全绿）
"""
import json
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
GATE_LOCK = 脚本目录 / "gate_lock.py"
sys.path.insert(0, str(脚本目录))
import gate_lock  # noqa: E402  （pid存活 探测复用·与被测物同一实现）

通过 = 0


def 断言(条件, 说明):
    global 通过
    assert 条件, "自测失败：" + 说明
    通过 += 1
    print("  ✓ %s" % 说明)


def 净环境():
    环境 = dict(os.environ)
    环境.pop("CN_TREE_LOCK_HELD", None)
    环境.pop("CN_NO_TREE_LOCK", None)
    return 环境


def 跑锁(参数, 环境=None, 超时=120):
    合并 = 净环境()
    if 环境:
        合并.update(环境)
    return subprocess.run([sys.executable, str(GATE_LOCK), *参数],
                          capture_output=True, text=True, encoding="utf-8",
                          errors="replace", env=合并, timeout=超时)


def 场景1_并发竞争(临时: str) -> None:
    锁目录 = Path(tempfile.mkdtemp(prefix="tl1_", dir=临时)) / "tree.lock"
    域 = {"CN_TREE_LOCK_DIR": str(锁目录)}
    起 = time.time()
    # A：run 形态持锁睡 3 秒（真取锁→心跳→finally 释放全链路）
    A = subprocess.Popen([sys.executable, str(GATE_LOCK), "run", "--scope", "tree", "--",
                          sys.executable, "-c", "import time; time.sleep(3); print('A-done')"],
                         env={**净环境(), **域}, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    time.sleep(1.2)   # 确保 A 已先拿到锁（python 双层启动 ~0.6s）
    B = 跑锁(["acquire", "--scope", "tree", "--timeout", "30"], 环境=域)
    结A = time.time()
    rcA = A.wait(timeout=60)
    A.stdout.close()
    结B = time.time()
    断言(rcA == 0, "场景① A run 形态 rc=0（持锁睡 3s 后正常释放）")
    断言(B.returncode == 0, "场景① B acquire 最终 rc=0（A 释放后获得）")
    断言(结B - 起 >= 2.5, f"场景① B 真排队了（B 获得时刻距 A 启动 {结B - 起:.1f}s ≥ A 持锁期 3s）")
    断言(结B >= 结A - 0.5, "场景① 时序：B 获得不早于 A 释放（串行接力·无并发重叠）")


def 场景2_陈锁接管(临时: str) -> None:
    锁目录 = Path(tempfile.mkdtemp(prefix="tl2_", dir=临时)) / "tree.lock"
    域 = {"CN_TREE_LOCK_DIR": str(锁目录)}
    # 造一把「持锁进程将死」的锁：真进程作存活锚 → kill → 917 双判据①秒级判陈锁
    锚 = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
    time.sleep(0.5)
    锁目录.mkdir(parents=True)
    (锁目录 / "info.json").write_text(json.dumps(
        {"pid": 锚.pid, "形态": "acquire", "owner_pid": 锚.pid,
         "命令": "自测·将被 kill 的持锁锚", "获取时间": time.strftime("%Y-%m-%d %H:%M:%S")},
        ensure_ascii=False), encoding="utf-8")
    锚.kill()
    锚.wait()
    等死 = 0.0
    while gate_lock.pid存活(锚.pid) and 等死 < 10:   # Windows TerminateProcess 异步·等探测判死
        time.sleep(0.2)
        等死 += 0.2
    断言(not gate_lock.pid存活(锚.pid), "场景② 持锁锚已 kill 且 pid 探测判死")
    起 = time.time()
    B = 跑锁(["acquire", "--scope", "tree", "--timeout", "30"], 环境=域)
    耗时 = time.time() - 起
    断言(B.returncode == 0, "场景② 排队者接管成功 rc=0")
    断言(耗时 < 15, f"场景② 秒级接管（{耗时:.1f}s < 15s·走 4h 心跳兜底必超时）")
    断言("陈锁接管" in (B.stderr or ""), "场景② 接管原因可见（917 存活锚判死）")
    跑锁(["release", "--scope", "tree"], 环境=域)   # B 拿锁即退·残留锁清掉


def 场景3_环境豁免与注入(临时: str) -> None:
    锁目录 = Path(tempfile.mkdtemp(prefix="tl3_", dir=临时)) / "tree.lock"
    # ③-a 豁免：CN_TREE_LOCK_HELD=1 → acquire/run 立即通过且不建锁目录
    起 = time.time()
    A = 跑锁(["acquire", "--scope", "tree"], 环境={"CN_TREE_LOCK_DIR": str(锁目录),
                                                   "CN_TREE_LOCK_HELD": "1"})
    断言(A.returncode == 0 and time.time() - 起 < 10, "场景③ 豁免 acquire 立即 rc=0")
    断言(not 锁目录.exists(), "场景③ 豁免不建锁目录（跳过取锁）")
    B = 跑锁(["run", "--scope", "tree", "--", sys.executable, "-c", "print('hi')"],
             环境={"CN_TREE_LOCK_DIR": str(锁目录), "CN_TREE_LOCK_HELD": "1"})
    断言(B.returncode == 0 and "继承外层树锁" in (B.stderr or ""),
         "场景③ 豁免 run 直跑透传 rc 并打印继承提示")
    # ③-b 注入：tree 域 run 形态向子进程注入 CN_TREE_LOCK_HELD=1
    C = 跑锁(["run", "--scope", "tree", "--", sys.executable, "-c",
              "import os,sys; sys.exit(0 if os.environ.get('CN_TREE_LOCK_HELD')=='1' else 3)"],
             环境={"CN_TREE_LOCK_DIR": str(锁目录)})
    断言(C.returncode == 0, "场景③ tree 域 run 向子进程注入 CN_TREE_LOCK_HELD=1")
    # ③-c machine 域不注入（env=None 继承父环境·既有行为不变）
    机器锁目录 = Path(tempfile.mkdtemp(prefix="tl3m_", dir=临时)) / "gate.lock"
    D = 跑锁(["run", "--scope", "machine", "--", sys.executable, "-c",
              "import os,sys; sys.exit(0 if os.environ.get('CN_TREE_LOCK_HELD') is None else 3)"],
             环境={"CN_GATE_LOCK_DIR": str(机器锁目录)})
    断言(D.returncode == 0, "场景③ machine 域 run 不注入（行为不变）")


def 场景4_域隔离(临时: str) -> None:
    树锁 = Path(tempfile.mkdtemp(prefix="tl4_", dir=临时)) / "tree.lock"
    机器锁 = Path(tempfile.mkdtemp(prefix="tl4m_", dir=临时)) / "gate.lock"
    域树 = {"CN_TREE_LOCK_DIR": str(树锁)}
    域机 = {"CN_GATE_LOCK_DIR": str(机器锁)}
    # A：tree 域持锁 6 秒（活锁·心跳新鲜——非陈锁）
    A = subprocess.Popen([sys.executable, str(GATE_LOCK), "run", "--scope", "tree", "--",
                          sys.executable, "-c", "import time; time.sleep(6)"],
                         env={**净环境(), **域树}, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(1.5)   # 确保 A 已持锁
    B = 跑锁(["acquire", "--scope", "machine", "--timeout", "10"], 环境=域机)
    断言(B.returncode == 0, "场景④ tree 锁持有时 machine 域照常获得（域隔离）")
    C = 跑锁(["acquire", "--scope", "tree", "--timeout", "3"], 环境=域树)
    断言(C.returncode == 2, "场景④ 同域（tree）活锁仍互斥（--timeout 超时 rc=2）")
    S = 跑锁(["status", "--scope", "tree"], 环境=域树)
    断言(S.returncode == 0 and "被持有" in (S.stdout or ""), "场景④ status --scope tree 显示持锁者")
    rcA = A.wait(timeout=60)
    断言(rcA == 0, "场景④ A run 形态正常结束释放")
    D = 跑锁(["acquire", "--scope", "tree", "--timeout", "10"], 环境=域树)
    断言(D.returncode == 0, "场景④ A 释放后同域可再获得")
    跑锁(["release", "--scope", "machine"], 环境=域机)
    跑锁(["release", "--scope", "tree"], 环境=域树)


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="cn_tree_lock_test_") as 临时:
        print("== 场景① 并发竞争（排队等待·释放接力） ==")
        场景1_并发竞争(临时)
        print("== 场景② 陈锁接管（917 存活锚判死·秒级） ==")
        场景2_陈锁接管(临时)
        print("== 场景③ 环境豁免与注入（嵌套防自死锁） ==")
        场景3_环境豁免与注入(临时)
        print("== 场景④ 域隔离（tree/machine 互不相干） ==")
        场景4_域隔离(临时)
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
