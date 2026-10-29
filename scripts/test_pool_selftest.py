#!/usr/bin/env python3
# 1021 预验池自测（本地临时端口·跑完即删临时 DB·不触真实服务）：
#   覆盖验收判据①②③的单元面：入队幂等/双 runner 并发认领互斥/心跳续约/超时回收重派/
#   完成校验认领者/红重置重跑/已完成绿复用/state 与 task_result 快照/旧 API 回归（join/report）。
#   562 扩：批主心跳（heartbeat 只刷时刻不改状态）/排队行孤儿回收（超时降级已踢出·
#   心跳保活不误收）/预验已并入检测（mock 看板台账三态：✅拒入·⬜放行·查无放行）。
# 用法：python scripts/test_pool_selftest.py   （退出码 0=全绿）

import json
import os
import subprocess
import sys
import tempfile
import threading
import time
import urllib.error
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

端口 = 18300
基址 = "http://127.0.0.1:%d" % 端口
令牌 = "pool-test-token"
临时目录 = tempfile.mkdtemp(prefix="cn_pool_test_")
db文件 = os.path.join(临时目录, "pool_test.db")
看板端口 = 18302
看板基址 = "http://127.0.0.1:%d" % 看板端口

通过 = 0


def 断言(条件, 说明):
    global 通过
    assert 条件, "自测失败：" + 说明
    通过 += 1
    print("  ✓ %s" % 说明)


class 假看板(BaseHTTPRequestHandler):
    """562 已并入检测的 mock 任务台账：任务/777=✅（已并入 develop）·任务/778=⬜（未并入）·
    其余=404（任务不在台账——核不了放行·宁漏检不误杀）。"""

    def do_GET(self):
        if self.path.endswith("/api/task/777"):
            体, 码 = json.dumps({"任务": {"号": "777", "状态": "✅"}}).encode("utf-8"), 200
        elif self.path.endswith("/api/task/778"):
            体, 码 = json.dumps({"任务": {"号": "778", "状态": "⬜"}}).encode("utf-8"), 200
        else:
            体, 码 = json.dumps({"错误": "无此任务"}, ensure_ascii=False).encode("utf-8"), 404
        self.send_response(码)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(体)))
        self.end_headers()
        self.wfile.write(体)

    def log_message(self, fmt, *args):
        pass


def 调(操作, 数据, 期望ok=True):
    请求 = urllib.request.Request(
        基址 + "/api/" + 操作, data=json.dumps(数据, ensure_ascii=False).encode("utf-8"),
        headers={"Content-Type": "application/json"})
    请求.add_unredirected_header("Authorization", "Bearer " + 令牌)   # P2-A
    with urllib.request.urlopen(请求, timeout=5) as r:
        回 = json.loads(r.read().decode("utf-8"))
    if 期望ok:
        assert 回.get("ok"), "%s 应成功：%s" % (操作, 回)
    return 回


def 取(路径):
    # 1425 全站登录墙：GET 亦须鉴权——测试实例凭 CN_QUEUE_TOKEN 走机器通道
    请求 = urllib.request.Request(基址 + 路径)
    请求.add_unredirected_header("Authorization", "Bearer " + 令牌)   # P2-A
    with urllib.request.urlopen(请求, timeout=5) as r:
        return json.loads(r.read().decode("utf-8"))


def main() -> int:
    环境 = dict(os.environ, CN_QUEUE_PORT=str(端口), CN_QUEUE_TOKEN=令牌,
               CN_WEB_USER="池自测", CN_WEB_PASS="池自测·密码1",        # 1425：网页凭据（fail-closed 不缺配）
               CN_QUEUE_DB=db文件, CN_TASK_STALE_SEC="3",   # 心跳超时缩到 3s 便于测回收
               CN_QUEUE_QUEUED_STALE_SEC="3",               # 562：排队回收阈值缩到 3s
               CN_BOARD_URL=看板基址)                        # 562：已并入检测指向 mock 台账

    def 排空(管道):
        """持续消费子进程 stdout——PIPE 无人读=缓冲满后服务 print 阻塞挂死（562 实测：
        请求日志+回收打印积累满缓冲→GET /api/state 超时）。"""
        for _行 in iter(管道.readline, b""):
            pass

    看板服务 = ThreadingHTTPServer(("127.0.0.1", 看板端口), 假看板)
    threading.Thread(target=看板服务.serve_forever, daemon=True).start()
    服务 = subprocess.Popen([sys.executable, str(Path(__file__).parent / "queue_service.py")],
                            env=环境, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    threading.Thread(target=排空, args=(服务.stdout,), daemon=True).start()
    try:
        for _ in range(30):
            try:
                取("/api/state")
                break
            except Exception:
                time.sleep(0.2)
        print("== ⓿鉴权 fail-closed（561·对齐看板 430） ==")
        try:
            请求 = urllib.request.Request(
                基址 + "/api/task_claim", data=json.dumps({"runner": "无令牌"}).encode("utf-8"),
                headers={"Content-Type": "application/json"})
            urllib.request.urlopen(请求, timeout=5)
            断言(False, "无令牌 POST 应 401")
        except urllib.error.HTTPError as e:
            断言(e.code == 401, "无令牌 POST 被拒 401")
        取("/api/state")
        # 1425：GET 亦须鉴权（原「只读面无令牌放行」废除）——无令牌裸 GET=401
        try:
            urllib.request.urlopen(基址 + "/api/state", timeout=5)
            断言(False, "无令牌 GET 应 401（1425 登录墙）")
        except urllib.error.HTTPError as e:
            断言(e.code == 401, "无令牌 GET /api/state 被拒 401（1425 登录墙·正反两态）")
        端口2 = 18301
        环境2 = dict(os.environ, CN_QUEUE_PORT=str(端口2), CN_QUEUE_DB=db文件 + ".2")
        环境2.pop("CN_QUEUE_TOKEN", None)
        服务2 = subprocess.Popen([sys.executable, str(Path(__file__).parent / "queue_service.py")],
                                env=环境2, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        threading.Thread(target=排空, args=(服务2.stdout,), daemon=True).start()
        try:
            for _ in range(30):
                try:
                    urllib.request.urlopen("http://127.0.0.1:%d/api/state" % 端口2, timeout=2).read()
                    break
                except urllib.error.HTTPError:
                    break   # 1425：401=服务已起且登录墙在工作（无令牌服务·探活即成）
                except Exception:
                    time.sleep(0.2)
            try:
                请求 = urllib.request.Request(
                    "http://127.0.0.1:%d/api/task_claim" % 端口2,
                    data=json.dumps({"runner": "x"}).encode("utf-8"),
                    headers={"Content-Type": "application/json"})
                请求.add_unredirected_header("Authorization", "Bearer whatever")   # P2-A
                urllib.request.urlopen(请求, timeout=5)
                断言(False, "未配置令牌服务 POST 应 401")
            except urllib.error.HTTPError as e:
                断言(e.code == 401, "令牌未配置=写操作全拒 401（fail-closed·旧址全放行）")
        finally:
            服务2.terminate()
            try:
                服务2.wait(timeout=5)
            except Exception:
                服务2.kill()

        print("== ①入队/幂等/校验 ==")
        调("task_enqueue", {"分支": "ci/预验-aaaaaaaaaa", "sha": "a" * 40})
        调("task_enqueue", {"分支": "ci/预验-aaaaaaaaaa", "sha": "a" * 40})   # 幂等
        状态 = 取("/api/state")
        断言(len(状态["预验任务"]) == 1, "幂等入队不重复")
        断言(状态["预验任务"][0]["源分支"] is None, "不带源分支入队=NULL（旧行回退语义·222）")
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
        调("task_enqueue", {"分支": "ci/预验-bbbbbbbbbb", "sha": "b" * 40,
                            "源分支": "任务/333"})   # 红结果→重置重跑（222：重置时刷新源分支）
        单 = 取("/api/task_result?sha=bbbbbbbbbb")
        断言(单["状态"] == "排队", "红结果重置为排队（CAS 重试语义）")
        行 = next(t for t in 取("/api/state")["预验任务"] if t["分支"] == "ci/预验-bbbbbbbbbb")
        断言(行["源分支"] == "任务/333", "红重置重入队刷新源分支（COALESCE 覆盖）")
        回 = 调("task_claim", {"runner": "runner-E"})
        断言(回["任务"] and 回["任务"]["分支"] == "ci/预验-bbbbbbbbbb", "重置后可再次认领")
        print("== ⑥task_cancel（1026 降级竞态治理） ==")
        调("task_enqueue", {"分支": "ci/预验-cccccccccc", "sha": "c" * 40})
        回 = 调("task_cancel", {"分支": "ci/预验-cccccccccc"})
        断言(回["取消"] == 1, "排队任务被取消（删行）")
        回 = 调("task_cancel", {"分支": "ci/预验-cccccccccc"})
        断言(回["取消"] == 0, "重复取消幂等（行已不在）")
        状态 = 取("/api/state")
        断言(not any(t["分支"] == "ci/预验-cccccccccc" for t in 状态["预验任务"]), "取消后 state 无此任务")

        print("== ⑦源分支存取（222 网页分支列显示） ==")
        调("task_enqueue", {"分支": "ci/预验-eeeeeeeeee", "sha": "e" * 40, "源分支": "任务/222"})
        行 = next(t for t in 取("/api/state")["预验任务"] if t["分支"] == "ci/预验-eeeeeeeeee")
        断言(行["源分支"] == "任务/222", "入队附带源分支已存（222 网页分支列显示）")

        print("== ⑧旧 API 回归（join/report/state 兼容） ==")
        调("join", {"分支": "任务/家机-1021-预验池化", "基线": "9d6d2bbc", "写集摘要": "自测"})
        调("report", {"sha": "c" * 40, "平台": "linux-x86_64", "绿": True, "总秒": 883})
        状态 = 取("/api/state")
        断言(any(q["分支"] == "任务/家机-1021-预验池化" for q in 状态["队列"]), "join 旧路径正常")
        断言(any(g["sha"] == "c" * 40 for g in 状态["门禁最近"]), "report 旧路径正常")
        断言("预验任务" in 状态, "state 含预验任务快照")
        # 190（1035 轮）：执行中上报——绿=None 存门禁表·完成上报覆盖同 sha 行
        调("report", {"sha": "d" * 40, "平台": "linux-x86_64", "绿": None,
                      "总秒": None, "状态": "执行中"})
        状态 = 取("/api/state")
        执行中行 = next(g for g in 状态["门禁最近"] if g["sha"] == "d" * 40)
        断言(执行中行["绿"] is None, "执行中上报 绿=None")
        调("report", {"sha": "d" * 40, "平台": "linux-x86_64", "绿": False, "总秒": 900})
        状态 = 取("/api/state")
        完成行 = next(g for g in 状态["门禁最近"] if g["sha"] == "d" * 40)
        断言(完成行["绿"] == 0 and 完成行["总秒"] == 900, "完成上报覆盖执行中行")

        print("== ⑨批主心跳与排队行孤儿回收（562） ==")
        调("join", {"分支": "任务/562等待者", "基线": "9d6d2bbc", "写集摘要": "心跳面"})
        断言(调("heartbeat", {"分支": "任务/562等待者"})["ok"], "heartbeat 排队行=ok")
        行 = next(q for q in 取("/api/state")["队列"] if q["分支"] == "任务/562等待者")
        断言(行["状态"] == "排队", "heartbeat 只刷更新时刻不改状态")
        回 = 调("heartbeat", {"分支": "任务/无此行"}, 期望ok=False)
        断言("无此行" in 回["说明"], "heartbeat 无此行=拒（心跳方静默忽略）")
        time.sleep(4)   # 无写活动超 CN_QUEUE_QUEUED_STALE_SEC=3s
        行 = next(q for q in 取("/api/state")["队列"] if q["分支"] == "任务/562等待者")
        断言(行["状态"] == "已踢出", "排队行超时无写活动=降级已踢出（507/476 孤儿不再卡队列）")
        调("join", {"分支": "任务/562等待者", "基线": "9d6d2bbc", "写集摘要": "重报"})
        for _ in range(5):   # 心跳保活跨 2 倍阈值——活会话行不误回收（492 守卫）
            time.sleep(1)
            调("heartbeat", {"分支": "任务/562等待者"})
        行 = next(q for q in 取("/api/state")["队列"] if q["分支"] == "任务/562等待者")
        断言(行["状态"] == "排队", "等待者心跳保活=跨超时阈值不误回收")
        调("join", {"分支": "任务/562批主", "基线": "9d6d2bbc", "写集摘要": "集成中心跳"})
        调("update", {"分支": "任务/562批主", "状态": "集成中"})
        调("heartbeat", {"分支": "任务/562批主"})
        行 = next(q for q in 取("/api/state")["队列"] if q["分支"] == "任务/562批主")
        断言(行["状态"] == "集成中", "集成中行心跳=状态保持（444 长门禁活批证明）")

        print("== ⑩预验已并入检测（562·mock 看板台账三态） ==")
        # 收尾前段遗留：⑤ runner-E 领走的执行中任务（久不心跳会被回收重派混入派发断言）
        #   与 ⑦ 遗留的排队任务（序号更小会先被选出）
        调("task_complete", {"runner": "runner-E", "分支": "ci/预验-bbbbbbbbbb", "绿": True,
                             "结果": {"绿": True}})
        调("task_cancel", {"分支": "ci/预验-eeeeeeeeee"})
        回 = 调("task_enqueue", {"分支": "ci/预验-7770000001", "sha": "7" * 40,
                                 "源分支": "任务/777"}, 期望ok=False)
        断言("已并入" in 回["说明"], "源分支已并入 develop（台账✅）=拒入（预验无意义）")
        调("task_enqueue", {"分支": "ci/预验-7780000002", "sha": "8" * 40,
                            "源分支": "任务/778"})
        回 = 调("task_claim", {"runner": "r562"})
        断言(回["任务"] and 回["任务"]["分支"] == "ci/预验-7780000002", "台账⬜未并入=照常派发")
        调("task_complete", {"runner": "r562", "分支": "ci/预验-7780000002", "绿": True,
                             "结果": {"绿": True}})
        调("task_enqueue", {"分支": "ci/预验-7790000003", "sha": "9" * 40,
                            "源分支": "任务/779"})
        回 = 调("task_claim", {"runner": "r562b"})
        断言(回["任务"] and 回["任务"]["分支"] == "ci/预验-7790000003",
             "台账查无任务（404）=核不了放行（宁漏检不误杀）")

        print("== ⑪825 失败原因字段与并行度观测（完成红首因/池忙闲排队） ==")
        # 收尾前段遗留：⑩ r562b 领走的执行中行（先于入队完成·防混入排队计数与派发）
        调("task_complete", {"runner": "r562b", "分支": "ci/预验-7790000003",
                             "绿": True, "结果": {"绿": True}})
        假sha = "c825c82501" + "0" * 30
        预验分支 = "ci/预验-c825c82501"
        调("task_enqueue", {"分支": 预验分支, "sha": 假sha, "源分支": "任务/825"})
        状态 = 取("/api/state")
        断言(状态["池忙闲"]["排队"] == 1, "池忙闲「排队」=1（825 残余 B·并行度观测数据面）")
        回 = 调("task_claim", {"runner": "r825"})
        断言(回["任务"] and 回["任务"]["分支"] == 预验分支, "825 前置：认领到本行")
        调("task_complete", {"runner": "r825", "分支": 预验分支, "绿": False,
                             "结果": {"绿": False, "步骤": {"同步": {"rc": 0}, "构建": {"rc": 0},
                                      "e2e": {"rc": 1}},
                                      "e2e明细": {"失败用例": [{"名称": "01_hello", "原因": "x"}]}}})
        状态 = 取("/api/state")
        任务行 = next(t for t in 状态["预验任务"] if t["分支"] == 预验分支)
        断言(任务行["失败原因"] == "e2e：失败 1 例（01_hello）",
             "完成红失败原因=首因（红步骤+用例名·快照面·825 残余 A）")
        结果详情 = 取("/api/task_result?sha=" + 假sha[:10])
        断言(结果详情.get("失败原因") == "e2e：失败 1 例（01_hello）",
             "失败原因随 /api/task_result 详情流出（页面详情面板面）")
        断言(状态["池忙闲"]["排队"] == 0, "完成后忙闲排队归零（随动一致）")

        print("\n[全部通过] %d 项断言·临时 DB=%s" % (通过, db文件))
        return 0
    finally:
        看板服务.shutdown()
        服务.terminate()
        try:
            服务.wait(timeout=5)
        except Exception:
            服务.kill()


if __name__ == "__main__":
    raise SystemExit(main())
