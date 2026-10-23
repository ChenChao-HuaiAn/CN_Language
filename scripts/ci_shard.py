#!/usr/bin/env python3
# 预验分片 runner 侧（1297·2026-10-21 用户令「多实例并行跑一个任务·最大化云端算力」；
#   1369 退化策略）：
#   池协议客户端 + 聚合纯函数 + 主实例分片编排——主实例（召唤/宽限/收片/抢补/聚合）与
#   从实例（领片/交片）共用的协议原语在此单点；真跑（构建/e2e 步骤执行/步骤封装）留在
#   ci_daemon（依赖单向：ci_daemon → 本模块，本模块禁 import ci_daemon——步骤记录器/
#   读明细/片心跳由调用方注入）。
# 分片模型：重型组各占一片（片 0=78_v2 自举链由主实例跑、片 1=79_v2 自举闭环由
#   从实例领——#1401 拆重型面），普通用例分片 组数..n-1 由空闲实例自主认领；主实例
#   收片时缺口抢补（服务端 CAS 防双跑）；任一片红=整轮红（红即停 902·不等待剩余片）。
#   #1401 自适应分片度：n 按池空闲实例/执行中预验任务数动态定（CN_SHARD_N 可固定覆盖）。
# 1369（2026-10-21·集成通道阻塞根治）：召唤后 CN_SHARD_GRACE_SEC（默认 90s）内无任何
#   从片认领（片快照无 执行中/完成）→主实例立即退化单实例序列（已完成的从片照收、
#   未完成的片本机补跑；不再空等 5400s 收片窗）；收片窗独立为 CN_SHARD_RECV_TIMEOUT_SEC
#   （默认 1800s），未齐片只报缺失=诚实红（门禁面不放宽：e2e 仍必须全量跑完）。

import hashlib
import json
import os
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request
from pathlib import Path
from urllib.parse import quote


def _仓库根():
    return Path(__file__).resolve().parent.parent


def _env整数(名: str, 默认: int, 下限: int) -> int:
    """1369：分片时序环境变量解析——未设=默认；非法值打告警后用默认（配置面
    非编译器语义面·不静默吞键）。"""
    原 = os.environ.get(名)
    if 原 is None or not 原.strip():
        return 默认
    try:
        return max(下限, int(原))
    except ValueError:
        print("[分片配置] %s=%r 非整数——改用默认 %ds" % (名, 原, 默认), flush=True)
        return 默认

分片数上界 = 12        # #1401：自适应分片度上限（池空闲多时一个任务吃满实例）
分片数默认兜底 = 3     # 池状态不可达/旧服务端无实例动态时的保守值（现网既有口径）
收片轮询秒 = 30        # 主实例收片轮询周期
认领宽限秒 = _env整数("CN_SHARD_GRACE_SEC", 90, 0)            # 1369：首片认领宽限窗
收片窗秒 = _env整数("CN_SHARD_RECV_TIMEOUT_SEC", 1800, 60)    # 1369：收片总窗（旧=5400）


def 显式分片数():
    """CN_SHARD_N 显式覆盖：None=自适应分片度；1=强制不分片；≥2=固定片数。
    非法值告警后按自适应（配置面非编译器语义面·不静默吞键）。"""
    原 = os.environ.get("CN_SHARD_N")
    if 原 is None or not str(原).strip():
        return None
    try:
        return max(1, int(原))
    except ValueError:
        print("[分片配置] CN_SHARD_N=%r 非整数——改用自适应分片度" % 原, flush=True)
        return None


def 分片启用() -> bool:
    """分片总开关（从实例据此「先领片后领任务」）：未设 CN_SHARD_N=自适应
    （池闲则分片）；显式 1=不分片；显式 ≥2=固定片数。"""
    固 = 显式分片数()
    return True if 固 is None else 固 > 1


def 池状态(池地址: str):
    """GET /api/state（免令牌·与看板同源）——不可达/坏 JSON=None（调用方用兜底）。"""
    try:
        with urllib.request.urlopen(池地址.rstrip("/") + "/api/state", timeout=10) as r:
            return json.loads(r.read().decode("utf-8"))
    except Exception:
        return None


def 估算分片数(状态, 上界: int = 分片数上界, 兜底: int = 分片数默认兜底) -> int:
    """#1401 纯函数：/api/state 快照 → 分片度（ci_daemon_selftest 直调·单一事实源）。
    n = 1 + 可领片空闲实例 // 执行中预验任务数（任务少时一个任务吃满空闲实例；
    多任务时按任务数均分）。可领片空闲优先用实例动态（状态=空闲且 30 分钟内活跃
    ——排除服务端回填种子的离线旧实例）；无该字段→池忙闲(注册-执行中)。
    下限 1=无空闲即不分片（跳过召唤·避免空等宽限窗）；上限防滥用。"""
    if not isinstance(状态, dict):
        return 兜底
    忙闲 = 状态.get("池忙闲") or {}
    执行中 = int(忙闲.get("执行中") or 0)
    动态们 = 状态.get("实例动态") or []
    现在 = int(time.time())
    if 动态们:
        可用 = sum(1 for r in 动态们
                 if r.get("状态") == "空闲"
                 and 现在 - int(r.get("最近活跃时戳") or 0) < 1800)
    else:
        可用 = max(0, int(忙闲.get("注册实例") or 0) - 执行中)
    预验执行中 = sum(1 for t in (状态.get("预验任务") or [])
                 if t.get("状态") == "执行中"
                 and str(t.get("分支", "")).startswith("ci/预验-"))
    n = 1 + 可用 // max(1, 预验执行中)
    return max(1, min(上界, n))


def 计算分片数(池地址: str) -> int:
    """#1401 分片度：显式 CN_SHARD_N 优先（1=不分片）；否则按池快照自适应
    （池不可达=保守 3·与现网既有 n=3 等价）。n 只决定召唤的片数：多派无人领的
    片由主实例收片时抢补，正确性不受影响。"""
    固 = 显式分片数()
    if 固 is not None:
        return max(1, min(固, 分片数上界))
    return 估算分片数(池状态(池地址))


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
    轮询重试·不静默当空集：空集与不可达语义不同，None=本轮不可判）。
    1369：分支名含中文（ci/预验-*）须 percent-encode——旧直拼 URL 触发
    UnicodeEncodeError 被吞成 None=主实例永不抢补/永不判齐（5400s 超时病根之一）。"""
    try:
        with urllib.request.urlopen(池地址.rstrip("/") + "/api/task_shards?branch="
                                    + quote(分支, safe=""), timeout=10) as r:
            回 = json.loads(r.read().decode("utf-8"))
        return 回.get("分片") if 回.get("ok") else None
    except Exception:
        return None


def 片结果(池地址: str, 池令牌: str, 分支: str, 片序: int):
    """GET 单片结果详情——不可达/无此片=None（调用方显式分流·648）。
    1369：同 片状态们 的 URL 编码修复（中文分支）。"""
    try:
        with urllib.request.urlopen(池地址.rstrip("/") + "/api/task_shard_result?branch="
                                    + quote(分支, safe="") + "&i=" + str(片序),
                                    timeout=15) as r:
            回 = json.loads(r.read().decode("utf-8"))
        return 回 if 回.get("ok") else None
    except Exception:
        return None


def 首片认领(状态们) -> bool:
    """1369 纯函数：快照中出现任一片「执行中/完成」=已有从片接手（召唤有响应）。
    空表=召唤未登记片行（无实例可领）；None=池不可达（不可判·语义见 等待首认领）。"""
    return any(s.get("状态") in ("执行中", "完成") for s in (状态们 or []))


def 等待首认领(读状态, 截止时刻: float, 轮询秒: float | None = None) -> bool:
    """1369：召唤后宽限窗——返回 True=首片已认领（正常分片/收片）；False=退化
    单实例序列。语义分三态（不静默混淆）：
    ①有「执行中/完成」片→True；②空表=召唤无任何登记行→立即 False（不等满窗，
      无实例可领）；③None（池不可达）=不可判→窗内重试，截止仍无证据=False
      （单实例全量仍完整覆盖门禁面，绝不空等 5400s）。"""
    轮询 = 收片轮询秒 if 轮询秒 is None else max(0.01, 轮询秒)
    while True:
        状态们 = 读状态()
        if 状态们:
            if 首片认领(状态们):
                return True
        elif 状态们 is not None:
            return False
        剩 = 截止时刻 - time.time()
        if 剩 <= 0:
            return False
        time.sleep(min(轮询, 剩))


def _歇(截止: float, 轮询: float | None = None) -> None:
    """1369：收片轮询睡眠——不越过截止时刻（窗到即判，不空转一轮）。"""
    轮询 = 收片轮询秒 if 轮询 is None else max(0.01, 轮询)
    time.sleep(min(轮询, max(0.01, 截止 - time.time())))


def _跑片本地(语境: dict, k: int) -> tuple:
    """1369：主实例本机跑片 k（--shard k/n·明细临时文件）→(绿, 秒, 明细)。
    片心跳注入：本机补跑可能超过服务端 300s 回收阈值——不续约=从实例可抢走双跑
    （1369 不重复跑要求的机械保证）；无行片的心跳服务端拒绝=无害。"""
    片明细 = 语境["日志目录"] / ("e2e明细_片%d_%s.json" % (k, 语境["sha"][:10]))
    try:
        片明细.unlink()   # 清残留：本轮未跑到时读侧不误读旧明细
    except OSError:
        pass
    名 = "e2e片%d" % k
    停 = 语境["片心跳"](k) if 语境["片心跳"] else None
    try:
        绿 = 语境["步骤"](
            名, 语境["e2e基"] + 语境["明细参"](片明细)
            + ["--shard", "%d/%d" % (k, 语境["片数"]), "--jobs", str(语境["e2e并行"])],
            语境["e2e超时秒"])
        秒 = (语境["步骤们"].get(名) or {}).get("秒", 0)
    finally:
        if 停 is not None:
            停.set()
    return 绿, 秒, 语境["读明细"](片明细)


def _领并跑(语境: dict, k: int):
    """1369：CAS 抢补片 k 并本机补跑（交片归档）——返回 (绿, 秒, 明细)；
    None=片已被从实例领走/服务端拒绝（调用方等待其完成，绝不双跑）。"""
    抢 = 抢补片(语境["注入"], 语境["runner"], 语境["轮分支"], k)
    if not 抢.get("ok"):
        return None
    绿, 秒, 明细 = _跑片本地(语境, k)
    交片(语境["注入"], 语境["runner"], 语境["轮分支"], k, 绿, 明细, 秒)
    return 绿, 秒, 明细


def _收从片(语境: dict, 行: dict) -> tuple:
    """1369：收从片完成结果（秒/明细从服务端拉·绿已由快照核）→(秒, 明细)。"""
    回 = 片结果(语境["配置"]["池地址"], 语境["配置"]["池令牌"],
             语境["轮分支"], 行.get("片序"))
    return (行.get("秒") or 0), (回.get("结果") if 回 else None)


def _分片收片(语境: dict) -> tuple:
    """1297/1369 正常分片收片：轮询快照→非完成片 CAS 抢补（对 执行中 也发起——
    服务端先按 300s 心跳回收再 CAS：活片拒绝无副作用，死片由此复活，防收片窗空等）
    →全片完成收从片明细。返回 (聚合rc, e2e秒=max片秒, 明细们, 片行们)。"""
    秒们 = [语境["自己秒"]]
    明细们 = [语境["自己明细"]]
    已跑 = {0}
    截止 = time.time() + 收片窗秒
    while time.time() < 截止:
        状态们 = 语境["读快照"]()
        if 状态们 is None:      # 池抖动——轮询重试（不可达≠全绿·不静默当齐）
            _歇(截止)
            continue
        红片 = next((s for s in 状态们
                     if s.get("状态") == "完成" and s.get("绿") == 0), None)
        if 红片 is not None:
            print("  [红即停] 从片 #%s 红——整轮红即停（902·其余片不再等待）"
                  % 红片.get("片序"), file=语境["日志"], flush=True)
            return 1, max(秒们), 明细们, 状态们
        行序 = {s.get("片序") for s in 状态们}
        for s in 状态们:        # 缺口+心跳超时执行中：CAS 抢补本机补跑
            k = s.get("片序")
            if k in 已跑 or s.get("状态") == "完成":
                continue
            回 = _领并跑(语境, k)
            if 回 is None:
                continue        # 已被从实例领走——下轮快照见其执行中/完成
            已跑.add(k)
            绿, 秒, 明细 = 回
            秒们.append(秒)
            明细们.append(明细)
            if not 绿:
                print("  [红即停] 抢补片 #%d 红——整轮红即停（902）" % k,
                      file=语境["日志"], flush=True)
                return 1, max(秒们), 明细们, 语境["读快照"]() or 状态们
        for k in range(1, 语境["片数"]):   # 台账无行片=无实例可领→本机补跑（不重复）
            if k in 已跑 or k in 行序:
                continue
            绿, 秒, 明细 = _跑片本地(语境, k)
            已跑.add(k)
            秒们.append(秒)
            明细们.append(明细)
            if not 绿:
                print("  [红即停] 补跑片 #%d 红——整轮红即停（902）" % k,
                      file=语境["日志"], flush=True)
                return 1, max(秒们), 明细们, 语境["读快照"]() or 状态们
        状态们 = 语境["读快照"]()
        if 状态们 is not None:
            完成集 = {s.get("片序") for s in 状态们 if s.get("状态") == "完成"}
            # 片账收齐判据=1..n-1 每片「本机已跑」或「服务端完成」——不依赖行数
            #   （召唤登记缺行时本机已补跑也算齐·1369 防「全跑完仍空等超时」）。
            if all(k in 已跑 or k in 完成集 for k in range(1, 语境["片数"])):
                if all(s.get("绿") == 1 for s in 状态们 if s.get("状态") == "完成"):
                    for s in 状态们:   # 从片片：秒与明细从服务端收（已跑片不重复计）
                        if s.get("片序") in 已跑 or s.get("状态") != "完成":
                            continue
                        秒, 明细 = _收从片(语境, s)
                        秒们.append(秒)
                        明细们.append(明细)
                    return 0, max(秒们), 明细们, 状态们
                return 1, max(秒们), 明细们, 状态们
        _歇(截止)
    状态们 = 语境["读快照"]() or []
    完成集 = {s.get("片序") for s in 状态们 if s.get("状态") == "完成"}
    缺片 = [k for k in range(1, 语境["片数"]) if k not in 已跑 and k not in 完成集]
    print("  [分片缺失] 收片窗 %ds 到期——已完成 %d/%d 片，缺失片 %s"
          "（诚实红·不挂死整轮）"
          % (收片窗秒, 语境["片数"] - 1 - len(缺片), 语境["片数"] - 1, 缺片 or "无"),
          file=语境["日志"], flush=True)
    return 1, max(秒们), 明细们, 状态们


def _单实例补跑(语境: dict) -> tuple:
    """1369 退化路径（片 0 已跑）：逐片 1..n-1——
    完成片收服务端结果（已由从片跑完→不重跑）；执行中片等其完成（宽限后仍有从片
    接手=不抢跑不重复）；缺口/无行片 CAS 抢补或本机补跑。收片窗同 CN_SHARD_RECV_
    TIMEOUT_SEC；未齐片只报缺失=红（不放宽门禁）。返回 (聚合rc, e2e秒, 明细们)。"""
    秒们 = [语境["自己秒"]]
    明细们 = [语境["自己明细"]]
    缺片: list[int] = []
    红 = False
    截止 = time.time() + 收片窗秒
    for k in range(1, 语境["片数"]):
        while True:
            状态们 = 语境["读快照"]()
            行 = next((s for s in (状态们 or []) if s.get("片序") == k), None)
            if 行 is not None and 行.get("状态") == "完成":
                if 行.get("绿") != 1:
                    print("  [红即停] 从片 #%d 红——整轮红即停（902·不放宽门禁）" % k,
                          file=语境["日志"], flush=True)
                    红 = True
                    break
                秒, 明细 = _收从片(语境, 行)
                秒们.append(秒)
                明细们.append(明细)
                break
            if 状态们 is None:      # 池不可达——窗内重试
                if time.time() >= 截止:
                    缺片.append(k)
                    红 = True
                    break
                _歇(截止)
                continue
            if 行 is not None:      # 执行中/缺口：CAS（活片拒绝=等待；死片回收后得手）
                回 = _领并跑(语境, k)
                if 回 is None:
                    if time.time() >= 截止:
                        缺片.append(k)
                        红 = True
                        break
                    _歇(截止)
                    continue
                绿, 秒, 明细 = 回
            else:                   # 无行=无实例可领→本机补跑
                绿, 秒, 明细 = _跑片本地(语境, k)
            秒们.append(秒)
            明细们.append(明细)
            if not 绿:
                print("  [红即停] 补跑片 #%d 红——整轮红即停（902）" % k,
                      file=语境["日志"], flush=True)
                红 = True
            break
        if 红:
            break
    if 缺片:
        print("  [分片缺失] 收片窗 %ds 到期——缺失片 %s（诚实红·不挂死整轮）"
              % (收片窗秒, 缺片), file=语境["日志"], flush=True)
    # 退化=各片串行补跑，e2e 墙钟近似=各片步骤秒之和（正常分片=并行取 max）
    总秒 = round(sum(s for s in 秒们 if isinstance(s, (int, float))))
    return (1 if (红 or 缺片) else 0), 总秒, 明细们


def 主实例分片段(*, 轮分支: str, 片数: int, 召唤时刻, e2e基: list, 明细参, 步骤,
                步骤们: dict, 日志, sha: str, 日志目录, e2e并行: int, e2e超时秒: int,
                runner: str, 读明细, 片心跳=None) -> tuple:
    """1369 主实例分片编排（ci_daemon 静态门禁与e2e段 调用·步骤/明细/心跳注入）：
    ①宽限窗等待首片认领（CN_SHARD_GRACE_SEC·默认 90s）——无认领立即退化；
    ②片 0 本机跑（重型独立面·run_e2e 语义）；
    ③正常=按片收（已认领片收结果/缺口抢补/红即停/收片窗诚实红）；
      退化=单实例序列（完成片照收不重跑、缺失片本机补跑·明细与单实例轮同格式）。
    返回 (聚合rc, 整轮e2e秒, 合并明细, 分片行们, 形态)；形态=「分片」/「退化单实例」
    ——退化时合并明细不含「分片」键（与现有单实例 --json 格式一致·看板/门禁照常解析）。
    片 0 红=902 红即停：返回片 0 明细（调用方仍写回主明细路径供 8300 页面归因）。"""
    配置 = 池配置()
    读快照 = lambda: 片状态们(配置["池地址"], 配置["池令牌"], 轮分支)
    截止 = (召唤时刻 or time.time()) + 认领宽限秒
    首认领 = bool(配置["池地址"]) and 等待首认领(读快照, 截止)
    形态 = "分片" if 首认领 else "退化单实例"
    if not 首认领:
        print("  [分片退化] 召唤后 %ds 内无任何从片认领——主实例转单实例序列"
              "（完成片照收·缺失片本机补跑·不再空等从片；收片窗 %ds）"
              % (认领宽限秒, 收片窗秒), file=日志, flush=True)
    语境 = {"轮分支": 轮分支, "片数": 片数, "e2e基": e2e基, "明细参": 明细参,
            "步骤": 步骤, "步骤们": 步骤们, "日志": 日志, "sha": sha,
            "日志目录": 日志目录, "e2e并行": e2e并行, "e2e超时秒": e2e超时秒,
            "runner": runner, "读明细": 读明细, "配置": 配置,
            "注入": lambda o, d: 池调用(配置, o, d), "读快照": 读快照,
            "片心跳": 片心跳}
    片0明细 = 日志目录 / ("e2e明细_片0_%s.json" % sha[:10])
    try:
        片0明细.unlink()
    except OSError:
        pass
    if not 步骤("e2e并行", e2e基 + 明细参(片0明细)
                + ["--shard", "0/%d" % 片数, "--jobs", str(e2e并行)], e2e超时秒):
        print("  [红即停] 主片 0 发现失败用例——中止该轮（用户令 2026-10-16·#902·"
              "分片红即停·其余片不再等待）", file=日志, flush=True)
        return (步骤们["e2e并行"]["rc"], 步骤们["e2e并行"].get("秒", 0),
                读明细(片0明细), [], 形态)
    语境["自己秒"] = 步骤们["e2e并行"].get("秒", 0)
    语境["自己明细"] = 读明细(片0明细)
    if 形态 == "退化单实例":
        rc, 秒, 明细们 = _单实例补跑(语境)
        return rc, 秒, 合并e2e明细(明细们, []), [], 形态
    rc, 秒, 明细们, 行们 = _分片收片(语境)
    return rc, 秒, 合并e2e明细(明细们, 行们), 行们, 形态


def e2e支持json() -> bool:
    """842：run_e2e.py 是否已带 --json——跨版本兼容窗口防护（daemon 自更新先于
    工作树 reset 到含本改动的 sha 时，向旧版 run_e2e 传未知参数=argparse 报错=
    e2e 假红）。传参前探源码：探测失败/旧版=不传（结果无 e2e明细·页面降级提示）。
    1369 自 ci_daemon 外迁（分片编排同域单点·ci_daemon 薄壳别名）。"""
    try:
        return "--json" in (_仓库根() / "tests" / "e2e" / "run_e2e.py").read_text(
            encoding="utf-8", errors="replace")
    except OSError:
        return False


def e2e明细参(明细路径) -> list:
    """1297：--json 明细参数（跨版本兼容窗口防护·e2e支持json 探测）——分片形态下
    各片写各自临时明细（主实例聚合后写回主明细路径）。"""
    return ["--json", str(明细路径)] if e2e支持json() else []


def 读e2e明细(路径: Path):
    """842：读 run_e2e --json 明细（失败用例/期望实际 diff）——缺文件/坏 JSON=None
    （e2e 未跑/旧版 run_e2e/未传参三态统一降级：页面显示「无明细」·不炸轮不谎报；
    明细是附加信息，非测量本体）。"""
    try:
        return json.loads(Path(路径).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None


def e2e命令基(cn: Path, 保内存: int, 平台: str, 是win: bool) -> list:
    """1297 提取：e2e 命令公共段（主实例全量/分片与从实例片轮三处共用·纪律二）——
    --target/--cn/--max-mem/--heavy-ci/--full-reason 一致口径；--json/--shard/--jobs
    由调用方按形态追加。566 注释原文见 ci_daemon 静态门禁与e2e段（放行通道语义不变）。"""
    return ([] if 是win else ["nice", "-n", "10"]) + [
        sys.executable, "tests/e2e/run_e2e.py",
        "--target", 平台, "--cn", str(cn),
        "--max-mem-mb", str(保内存),
        "--heavy-ci",
        "--full-reason", "ci_daemon 云端全量门禁（TX_02/池 runner）"]


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


# ═══════════════ 1392 实例空闲报活（池协议客户端同域·服务端 /api/instance_ping） ═══════════════
# daemon 空闲也每 60s 报活+版本指纹——服务端实例表「最近活跃」由此实时（此前只在
# 跑轮期间有心跳·空闲实例活着还是关机服务端不可判）；旧版 daemon 不发=渐进部署，
# 服务端照常（活跃度退化为最后 claim/complete 时刻·疑似离线判定对其豁免）。

报活间隔秒 = 60   # 与 task_heartbeat 周期一致（服务端疑似离线阈值 180s=3 个周期）


def 有非win空闲(状态) -> bool:
    """#1428：池内是否存在空闲的 **linux** 实例（状态=空闲且 30 分钟内活跃）。
    win 三实例与 WSL/xsim 同处一台物理机：win 参与分片会与它们抢 CPU——实测普通片
    192 例从 5~8 分钟拖到 1893s（1419），让位后同型轮 827s（1417）。判断口径保守：
    只认明确 linux-* 平台的空闲实例（平台空/未知的旧 daemon 不计，不误让位）；
    状态不可判（None/池不可达）=False → win 照常补位，不浪费算力。"""
    if not isinstance(状态, dict):
        return False
    现在 = int(time.time())
    for r in 状态.get("实例动态") or []:
        if r.get("状态") != "空闲":
            continue
        if not str(r.get("平台", "")).startswith("linux"):
            continue
        if 现在 - int(r.get("最近活跃时戳") or 0) < 1800:
            return True
    return False


def 本机门禁锁忙(锁目录: Path | None = None) -> bool:
    """#1423：本机共享门禁锁（CN_GATE_LOCK_DIR/主树锁目录）是否被其他活进程持有。
    win 三实例共用一把锁（923 OOM 铁律=同机全量互斥）：同机同时领片时只有先拿锁的
    能真跑，其余排队 15~30 分钟（1420 实录：win2 片7 等锁 29.5 分钟·实际只跑 367s）。
    调用方见忙即跳过本轮领片/领任务=片让给其他机器或主实例收片补跑，不空等。
    锁不存在/陈锁（持有进程已死）=不忙；探测异常=不忙（不阻塞正常领任务）。
    锁目录参数供自测注入（缺省=gate_lock 主树锁目录·win 池经 CN_GATE_LOCK_DIR 覆盖）。"""
    try:
        import gate_lock
        锁 = Path(锁目录) if 锁目录 is not None else gate_lock.主树锁目录()
        if not 锁.exists() or gate_lock.是陈锁(锁):
            return False
        信息 = gate_lock.读信息(锁)
        pid = 信息.get("pid")
        return bool(pid) and gate_lock.pid存活(pid)
    except Exception:
        return False


def 实例指纹(仓库根: Path) -> dict:
    """实例版本指纹（启动时算一次·进程内不变=「这个 daemon 进程是什么版本」）——
    daemon指纹=ci_daemon.py 内容 sha256 前 10（不依赖 git·scp 直更同样反映真实
    版本；常驻进程代码=启动时加载版，跑轮 reset 改磁盘不影响运行中进程·故以启动
    时磁盘版为准）；树头=实例仓 HEAD 前 10（跑轮 checkout 被验分支会变·参考面）。
    取不到=空串显式（前端显示 —·不猜不编；服务端空串不覆盖已有值）。"""
    指纹 = ""
    try:
        # #1418：按文本读（通用换行归一化 CRLF→LF）再哈希——win/linux 同提交同代码
        #   同指纹。此前 read_bytes 直接吃到行尾差异（win 池树 checkout 为 CRLF），
        #   导致同树头的 win/linux 被判「版本不一致」跨平台假警。
        内容 = (仓库根 / "scripts" / "ci_daemon.py").read_text(
            encoding="utf-8", errors="replace").replace("\r\n", "\n")
        指纹 = hashlib.sha256(内容.encode("utf-8")).hexdigest()[:10]
    except OSError:
        pass
    头 = ""
    try:
        r = subprocess.run(["git", "rev-parse", "HEAD"], cwd=仓库根,
                           capture_output=True, text=True, timeout=10)
        if r.returncode == 0:
            头 = r.stdout.strip()[:10]
    except Exception:
        pass
    return {"树头": 头, "daemon指纹": 指纹}


def 启动实例报活(runner: str, 平台: str, 仓库根: Path) -> None:
    """空闲报活后台线程（ci_daemon.主 启动挂载·daemon=True 随进程退出）：每
    报活间隔秒 POST instance_ping（忙闲都发·池不可达静默——与 task_heartbeat
    同口径）。未配置池地址=不启动（纯本地面无服务端可报）。"""
    配置 = 池配置()
    if not 配置["池地址"]:
        return
    指纹 = 实例指纹(仓库根)

    def 跑():
        while True:
            池调用(配置, "instance_ping", {"runner": runner, "平台": 平台, **指纹})
            time.sleep(报活间隔秒)

    threading.Thread(target=跑, daemon=True).start()
