#!/usr/bin/env python3
# 1021 预验池自测（本地临时端口·跑完即删临时 DB·不触真实服务）：
#   覆盖验收判据①②③的单元面：入队幂等/双 runner 并发认领互斥/心跳续约/超时回收重派/
#   完成校验认领者/红重置重跑/已完成绿复用/state 与 task_result 快照/旧 API 回归（join/report）。
# 用法：python scripts/test_pool_selftest.py   （退出码 0=全绿）

import json
import os
import subprocess
import sys
import tempfile
import threading
import time
import urllib.request
from pathlib import Path

端口 = 18300
基址 = "http://127.0.0.1:%d" % 端口
令牌 = "pool-test-token"
临时目录 = tempfile.mkdtemp(prefix="cn_pool_test_")
db文件 = os.path.join(临时目录, "pool_test.db")

通过 = 0


def 断言(条件, 说明):
    global 通过
    assert 条件, "自测失败：" + 说明
    通过 += 1
    print("  ✓ %s" % 说明)


def 调(操作, 数据, 期望ok=True):
    请求 = urllib.request.Request(
        基址 + "/api/" + 操作, data=json.dumps(数据, ensure_ascii=False).encode("utf-8"),
        headers={"Content-Type": "application/json", "Authorization": "Bearer " + 令牌})
    with urllib.request.urlopen(请求, timeout=5) as r:
        回 = json.loads(r.read().decode("utf-8"))
    if 期望ok:
        assert 回.get("ok"), "%s 应成功：%s" % (操作, 回)
    return 回


def 取(路径):
    with urllib.request.urlopen(基址 + 路径, timeout=5) as r:
        return json.loads(r.read().decode("utf-8"))


def main() -> int:
    环境 = dict(os.environ, CN_QUEUE_PORT=str(端口), CN_QUEUE_TOKEN=令牌,
               CN_QUEUE_DB=db文件, CN_TASK_STALE_SEC="3")   # 心跳超时缩到 3s 便于测回收
    服务 = subprocess.Popen([sys.executable, str(Path(__file__).parent / "queue_service.py")],
                            env=环境, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    try:
        for _ in range(30):
            try:
                取("/api/state")
                break
            except Exception:
                time.sleep(0.2)
        print("== ①入队/幂等/校验 ==")
        调("task_enqueue", {"分支": "ci/预验-aaaaaaaaaa", "sha": "a" * 40})
        调("task_enqueue", {"分支": "ci/预验-aaaaaaaaaa", "sha": "a" * 40})   # 幂等
        状态 = 取("/api/state")
        断言(len(状态["预验任务"]) == 1, "幂等入队不重复")
        回 = 调("task_enqueue", {"分支": "任务/坏分支", "sha": "b" * 40}, 期望ok=False)
        断言("ci/预验-" in 回["说明"], "非 ci/预验- 分支被拒")

        print("== ②双 runner 并发认领互斥（1 任务 20 并发认领仅 1 得手） ==")
        结果们 = []
        锁 = threading.Lock()

        def 认领(runner):
            回 = 调("task_claim", {"runner": runner})
            with 锁:
                结果们.append(回.get("任务"))

        线程们 = [threading.Thread(target=认领, args=("runner-%02d" % i,)) for i in range(20)]
        for t in 线程们:
            t.start()
        for t in 线程们:
            t.join()
        得手 = [r for r in 结果们 if r]
        断言(len(得手) == 1 and 得手[0]["分支"] == "ci/预验-aaaaaaaaaa", "并发认领仅 1 得手且是最旧任务")

        print("== ③心跳续约与超时回收重派 ==")
        time.sleep(4)   # 超 CN_TASK_STALE_SEC=3s 不心跳
        回 = 调("task_claim", {"runner": "runner-B"})
        断言(回["任务"] and 回["任务"]["分支"] == "ci/预验-aaaaaaaaaa", "心跳断→回收重派给 runner-B")
        回 = 调("task_heartbeat", {"runner": "runner-A", "分支": "ci/预验-aaaaaaaaaa"}, 期望ok=False)
        断言("认领者" in 回["说明"], "旧认领者 A 心跳被拒")
        调("task_heartbeat", {"runner": "runner-B", "分支": "ci/预验-aaaaaaaaaa"})
        time.sleep(1.2)
        回 = 调("task_claim", {"runner": "runner-C"})
        断言(回["任务"] is None, "B 刚续约心跳未被回收（C 认领不到）")

        print("== ④完成校验认领者+详情存取 ==")
        结果 = {"sha": "a" * 40, "绿": True, "总秒": 480, "步骤": {"e2e": {"rc": 0}}}
        回 = 调("task_complete", {"runner": "runner-A", "分支": "ci/预验-aaaaaaaaaa",
                                  "绿": True, "结果": 结果}, 期望ok=False)
        断言("认领者" in 回["说明"], "冒名完成被拒")
        调("task_complete", {"runner": "runner-B", "分支": "ci/预验-aaaaaaaaaa",
                             "绿": True, "结果": 结果})
        单 = 取("/api/task_result?sha=aaaaaaaaaa")
        断言(单["状态"] == "完成" and 单["绿"] and 单["详情"]["总秒"] == 480, "task_result 详情可取回")

        print("== ⑤已完成绿入队=复用·已完成红入队=重置 ==")
        调("task_enqueue", {"分支": "ci/预验-aaaaaaaaaa", "sha": "a" * 40})
        单 = 取("/api/task_result?sha=aaaaaaaaaa")
        断言(单["状态"] == "完成", "绿结果复用不重跑")
        调("task_enqueue", {"分支": "ci/预验-bbbbbbbbbb", "sha": "b" * 40})
        回 = 调("task_claim", {"runner": "runner-D"})
        断言(回["任务"] and 回["任务"]["分支"] == "ci/预验-bbbbbbbbbb", "新任务可认领")
        调("task_complete", {"runner": "runner-D", "分支": "ci/预验-bbbbbbbbbb", "绿": False,
                             "结果": {"sha": "b" * 40, "绿": False}})
        调("task_enqueue", {"分支": "ci/预验-bbbbbbbbbb", "sha": "b" * 40})   # 红结果→重置重跑
        单 = 取("/api/task_result?sha=bbbbbbbbbb")
        断言(单["状态"] == "排队", "红结果重置为排队（CAS 重试语义）")
        回 = 调("task_claim", {"runner": "runner-E"})
        断言(回["任务"] and 回["任务"]["分支"] == "ci/预验-bbbbbbbbbb", "重置后可再次认领")

        print("== ⑥旧 API 回归（join/report/state 兼容） ==")
        调("join", {"分支": "任务/家机-1021-预验池化", "基线": "9d6d2bbc", "写集摘要": "自测"})
        调("report", {"sha": "c" * 40, "平台": "linux-x86_64", "绿": True, "总秒": 883})
        状态 = 取("/api/state")
        断言(any(q["分支"] == "任务/家机-1021-预验池化" for q in 状态["队列"]), "join 旧路径正常")
        断言(any(g["sha"] == "c" * 40 for g in 状态["门禁最近"]), "report 旧路径正常")
        断言("预验任务" in 状态, "state 含预验任务快照")
        print("\n[全部通过] %d 项断言·临时 DB=%s" % (通过, db文件))
        return 0
    finally:
        服务.terminate()
        try:
            服务.wait(timeout=5)
        except Exception:
            服务.kill()


if __name__ == "__main__":
    raise SystemExit(main())
