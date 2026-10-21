#!/usr/bin/env python3
# 预验分片 runner 侧（1297·2026-10-21 用户令「多实例并行跑一个任务·最大化云端算力」）：
#   池协议客户端 + 聚合纯函数——主实例（召唤/收片/抢补/聚合）与从实例（领片/交片）
#   共用的协议原语在此单点；真跑（构建/e2e 命令/步骤封装）留在 ci_daemon（依赖单向：
#   ci_daemon → 本模块，本模块禁 import ci_daemon·池调用函数由调用方注入）。
# 分片模型：片 0=主实例自带（含 78/79 重型独立面·run_e2e 新分片语义），片 1..n-1
#   服务端登记缺口由空闲实例自主认领；主实例收片时缺口抢补（服务端 CAS 防双跑）；
#   任一片红=整轮红（红即停 902·不等待剩余片）。

import json
import os
import time
import urllib.error
import urllib.request
from pathlib import Path


def _仓库根():
    return Path(__file__).resolve().parent.parent

分片数默认 = max(1, int(os.environ.get("CN_SHARD_N", "3")))   # 1=不分片（现状）；现网推荐 3
收片轮询秒 = 30        # 主实例收片轮询周期


def 池配置() -> dict:
    """池连接参数（1297 自 ci_daemon 归一：池 HTTP 协议客户端单点化·原样搬移）——
    env CN_POOL_URL/CN_POOL_TOKEN 优先，否则 ci_config.json（gitignore）的
    池地址/池令牌。未配置=空地址（池段整体跳过=纯 TX_02 旧行为）。"""
    配置 = {}
    配置文件 = _仓库根() / "ci_config.json"
    if 配置文件.exists():
        try:
            配置.update(json.loads(配置文件.read_text(encoding="utf-8")))
        except Exception:
            pass
    return {"池地址": os.environ.get("CN_POOL_URL") or 配置.get("池地址", ""),
            "池令牌": os.environ.get("CN_POOL_TOKEN") or 配置.get("池令牌", "")}


def 池调用(配置: dict, 操作: str, 数据: dict, 超时秒: int = 10):
    """POST TX_01 池 API（1297 自 ci_daemon 归一·原样搬移）。可达但业务拒绝（4xx）→
    读回响应体返回；网络不可达/异常 → None（调用方自行降级：认领失败=回 develop
    轮询，complete 失败=重试后放弃留档）。"""
    try:
        请求 = urllib.request.Request(
            配置["池地址"].rstrip("/") + "/api/" + 操作,
            data=json.dumps(数据, ensure_ascii=False).encode("utf-8"),
            headers={"Content-Type": "application/json",
                     "Authorization": "Bearer " + 配置.get("池令牌", "")})
        with urllib.request.urlopen(请求, timeout=超时秒) as r:
            return json.loads(r.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        try:
            return json.loads(e.read().decode("utf-8"))
        except Exception:
            return None
    except Exception:
        return None


def 召唤从片(池调用, 分支: str, runner: str, 片数: int) -> bool:
    """主实例召唤：为执行中任务登记 片1..n-1 缺口（幂等）。失败仅告警不抛——
    分片是加速面不是正确性面，召唤失败=退化为单实例全量（主实例照样跑片 0 +
    收片时缺口全由自己补跑=与现状等价）。"""
    回 = 池调用("task_shard_enqueue", {"runner": runner, "分支": 分支, "片数": 片数})
    if 回 and 回.get("ok"):
        print("[分片召唤] %s 片数=%d（缺口 %d·空闲实例自主补位）" % (分支, 片数, 片数 - 1),
              flush=True)
        return True
    print("[分片召唤失败·退化单实例] %s：%s" % (分支, 回), flush=True)
    return False


def 领片(池调用, runner: str, 平台: str):
    """从实例领缺口片（无缺口/池不可达 → None·调用方落回领整任务路径）。"""
    回 = 池调用("task_shard_claim", {"runner": runner, "平台": 平台})
    if 回 and 回.get("ok"):
        return 回.get("片")
    return None


def 抢补片(池调用, runner: str, 分支: str, 片序: int) -> dict:
    """主实例抢补指定缺口片（服务端 CAS：仅缺口态可抢·防与从实例认领双跑）。
    返回服务端响应（ok=False=已被别人领走·放弃该片由认领者跑）。"""
    return 池调用("task_shard_claim", {"runner": runner, "分支": 分支, "片序": 片序}) or {"ok": False}


def 交片(池调用, runner: str, 分支: str, 片序: int, 绿: bool, 结果, 秒) -> bool:
    """从实例交片结果（3 次重试·全败 False——片行留在执行中，片心跳断后服务端
    回收回缺口可重领，正确性无损·浪费一次算力与 task_complete 同口径）。"""
    for 试 in range(3):
        回 = 池调用("task_shard_complete", {"runner": runner, "分支": 分支, "片序": 片序,
                                            "绿": bool(绿), "结果": 结果, "秒": 秒})
        if 回 and 回.get("ok"):
            return True
        print("[分片警告] 交片第 %d 次失败：%s——10s 后重试" % (试 + 1, 回), flush=True)
        time.sleep(10)
    return False


def 片状态们(池地址: str, 池令牌: str, 分支: str):
    """GET 片状态快照（免令牌·同 /api/state 口径）——不可达/异常=None（调用方
    轮询重试·不静默当空集：空集与不可达语义不同，None=本轮不可判）。"""
    try:
        with urllib.request.urlopen(池地址.rstrip("/") + "/api/task_shards?branch=" + 分支,
                                    timeout=10) as r:
            回 = json.loads(r.read().decode("utf-8"))
        return 回.get("分片") if 回.get("ok") else None
    except Exception:
        return None


def 片结果(池地址: str, 池令牌: str, 分支: str, 片序: int):
    """GET 单片结果详情——不可达/无此片=None（调用方显式分流·648）。"""
    try:
        with urllib.request.urlopen(池地址.rstrip("/") + "/api/task_shard_result?branch="
                                    + 分支 + "&i=" + str(片序), timeout=15) as r:
            回 = json.loads(r.read().decode("utf-8"))
        return 回 if 回.get("ok") else None
    except Exception:
        return None


def 合并e2e明细(明细们: list, 分片行们: list):
    """聚合纯函数：各片 run_e2e --json 明细合并——计数键求和；失败/未实现用例
    按名保序并集去重（同名=同用例跨片重复上报防线）；附「分片」明细行
    （片序/认领者/秒/绿——8300 页面与门禁详情的观测面）。
    全部输入无有效 dict（旧版 run_e2e/未传 --json）→ None：调用方不写回聚合版
    （宁缺不错·648 不编造假全零明细冒充测量）。"""
    有效们 = [m for m in 明细们 if isinstance(m, dict)]
    if not 有效们:
        return None
    合并 = {"目标平台": "", "时刻": time.strftime("%Y-%m-%d %H:%M:%S"),
            "总计": 0, "通过": 0, "失败": 0, "未实现": 0, "跳过": 0,
            "失败用例": [], "未实现用例": []}
    见败: set[str] = set()
    见未: set[str] = set()
    for 明细 in 有效们:
        for 键 in ("总计", "通过", "失败", "未实现", "跳过"):
            值 = 明细.get(键)
            if isinstance(值, int):
                合并[键] += 值
        if not 合并["目标平台"] and 明细.get("目标平台"):
            合并["目标平台"] = str(明细["目标平台"])
        for 例 in 明细.get("失败用例") or []:
            名 = str(例.get("名称") or "").strip() if isinstance(例, dict) else ""
            if 名 and 名 not in 见败:
                见败.add(名)
                合并["失败用例"].append(例)
        for 例 in 明细.get("未实现用例") or []:
            名 = str(例.get("名称") or "").strip() if isinstance(例, dict) else ""
            if 名 and 名 not in 见未:
                见未.add(名)
                合并["未实现用例"].append(例)
    if 分片行们:
        合并["分片"] = 分片行们
    return 合并


def 片步骤值(rc们: list, 秒们: list) -> dict:
    """聚合纯函数：整轮 e2e 步骤值——rc=全片 0 才 0（否则首个非 0·诚实红）；
    秒=max（墙钟口径：分片并行下整轮 e2e 耗时=最慢片）。"""
    非零 = next((rc for rc in rc们 if rc != 0), 0)
    秒值 = max([s for s in 秒们 if isinstance(s, (int, float))] or [0])
    return {"rc": 非零, "秒": round(秒值)}
