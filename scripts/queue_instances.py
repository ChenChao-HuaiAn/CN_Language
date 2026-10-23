#!/usr/bin/env python3
# 云端实例动态面（1392·2026-10-22 用户令「实例管理界面·每个实例的实时动态」）：
#   实例持久化表 + 版本指纹 + 空闲报活写入口 + 动态派生快照——8300 门禁窗口
#   「实例」页签数据层。此前实例身份只在内存 dict 实例注册（服务重启即丢·仅
#   claim 刷新）与 预验任务.认领者 列（仅执行中行可见·限快照 50 行窗口）——
#   无实例维度查询面。本模块职责：
#   ①实例表（SQLite 持久·重启不丢）：claim/heartbeat/complete/instance_ping
#     四写入点刷新最近活跃时戳（服务端盖章·免疫客户端时钟漂移）；
#   ②instance_ping 空闲报活（daemon 空闲也每 60s 报·带版本指纹；旧 daemon 不发
#     =渐进部署服务端照常，活跃度退化为最后 claim/complete 时刻）；
#   ③动态快照派生：执行中/心跳超时/疑似离线/空闲 四态 + 当前任务 + 最近完成
#     + 版本（树头·daemon指纹）——任务信息一律从 预验任务 派生（任务行=唯一
#     权威·实例表不存任务防双写漂移）。
#   版本口径：树头=实例仓 HEAD（跑轮 checkout 被验分支会变·参考面）；daemon指纹
#   =ci_daemon.py 内容 sha256 前 10（进程启动版·scp 直更同样反映·版本稳定指标）。
#   「版本不一致」判定在前端按众数派比对（服务端无权威最新版本源·不冒充权威）。
#   #1426（2026-10-23 用户令「同机最多 1~2 个分片轮 + 优先独立机」）：实例表与
#   分片表记录物理机键（hostname·平台-架构·指纹6·508 标识），服务端按物理机限流。
#   旧 daemon 不上报=空值显式（限流侧按独立机器保守放行+告警·不静默归并错配）。
#   依赖单向：queue_service → 本模块（禁反引）；自测断言见 queue_selftest.py。

from __future__ import annotations   # P1-1：3.8 注解惰性化（list[...] 定义期不求值）

import sqlite3
import time


# instance_ping 周期（daemon 侧 60s）×3——超此秒数未报活=疑似离线。
# 仅对「发过版本上报」的实例判定（旧版 daemon 空闲不报活·不误标）。
报活在线秒 = 180
# 旧版 daemon（从未上报版本）空闲不报活：按最后活动时刻给 30 分钟宽限，超时=疑似离线。
# 此前对其完全豁免导致关机实例永远显示「空闲」（用户 10-23 报深度机/单位机四台关机）。
离线宽限秒 = 1800


def 建实例表(con) -> None:
    """实例表建表（建库 时调用·须在 queue_service.建库 的 commit 前）。
    版本上报时刻 NULL=旧版 daemon 从未上报指纹（疑似离线判定对其豁免）。
    #1460：另建 池设置 KV 表（「更新实例」广播戳持久化——服务重启/实例重启后
    仍知上次广播时刻，daemon 据此判「是否已执行过本次对齐」）。"""
    con.execute("""CREATE TABLE IF NOT EXISTS 实例(
        名称 TEXT PRIMARY KEY,
        平台 TEXT,                  -- win-x64/linux-x86_64/linux-arm64（claim/ping 自报）
        注册时刻 TEXT,              -- 首次出现时刻（秒级戳·仅登记不回刷）
        最近活跃时戳 INTEGER,       -- epoch·服务端盖章：claim/heartbeat/complete/ping 四点刷新
        树头 TEXT,                  -- 实例仓 HEAD sha 前 10（daemon 启动时自报·跑轮会变·参考面）
        daemon指纹 TEXT,            -- ci_daemon.py 内容 sha256 前 10（进程启动版·版本稳定指标）
        物理机 TEXT,                -- #1426 物理机键（hostname·平台-架构·指纹6·NULL=旧 daemon）
        版本上报时刻 TEXT)           -- 最近一次带指纹上报时刻（NULL=旧版 daemon 未上报过）""")
    try:   # #1426 物理机指纹列：存量库 ALTER 补（列已在=重复列错吞掉·同 queue_service 先例）
        con.execute("ALTER TABLE 实例 ADD COLUMN 物理机 TEXT")
    except sqlite3.OperationalError:
        pass
    con.execute("""CREATE TABLE IF NOT EXISTS 池设置(
        键 TEXT PRIMARY KEY,
        值 TEXT)""")


def _时刻文本转戳(文本: str):
    """预验任务.更新时刻（"MM-DD HH:MM:SS"）→ epoch；解析失败=None（调用方兜底·
    不静默编造）。跨年边界（1 月读取 12 月文本=比现在晚）按「晚 1 天以上=去年」修正。"""
    try:
        结构 = time.strptime(str(文本), "%m-%d %H:%M:%S")
    except (TypeError, ValueError):
        return None
    今年 = time.localtime().tm_year
    戳 = int(time.mktime((今年,) + 结构[1:]))
    if 戳 - time.time() > 86400:
        戳 = int(time.mktime((今年 - 1,) + 结构[1:]))
    return 戳


def 时刻秒(文本: str):
    """#1426 归纳公共面：秒级戳文本（MM-DD HH:MM:SS）→ epoch；坏串=None（不猜）。
    实例聚合与分片让位宽限共用（第三处解析点起提炼·单一事实源）。"""
    return _时刻文本转戳(文本)


def 回填种子(con, 时刻函数) -> None:
    """部署首屏种子（服务启动·建实例表 后调用一次）：从 预验任务 现存认领者回填
    实例行（INSERT OR IGNORE——已有行不动）。执行中行=唯一可靠在跑信号（活跃时戳
    取其心跳时戳）；完成行认领者=历史实例（活跃时戳取该实例最后一次完成时刻——
    此前用「现在」作种子近似，把早已关机的实例显示成刚活跃过（用户 10-23 报深度机/
    单位机关机仍显示空闲）。回填行 平台/版本 NULL 显式——等该实例下次 claim/ping 自报。"""
    现在 = int(time.time())
    for 认领者, 心跳 in con.execute(
            "SELECT 认领者, max(心跳时戳) FROM 预验任务 "
            "WHERE 状态='执行中' AND 认领者 IS NOT NULL GROUP BY 认领者").fetchall():
        con.execute("INSERT OR IGNORE INTO 实例(名称,注册时刻,最近活跃时戳) VALUES(?,?,?)",
                    (认领者, 时刻函数(), 心跳 or 现在))
    for 认领者, 活跃 in _最近完成时戳(con).items():
        con.execute("INSERT OR IGNORE INTO 实例(名称,注册时刻,最近活跃时戳) VALUES(?,?,?)",
                    (认领者, 时刻函数(), 活跃))


def _最近完成时戳(con) -> dict:
    """每实例最后一次完成时刻（预验任务.更新时刻 文本→epoch·取最大）。"""
    最近: dict = {}
    for 认领者, 更新时刻 in con.execute(
            "SELECT 认领者, 更新时刻 FROM 预验任务 "
            "WHERE 状态='完成' AND 认领者 IS NOT NULL").fetchall():
        戳 = _时刻文本转戳(更新时刻)
        if 戳 and 戳 > 最近.get(认领者, 0):
            最近[认领者] = 戳
    return 最近


def 校正种子活跃(con) -> None:
    """存量修正（服务启动·回填种子 后调用）：旧版 daemon 无报活，实例表活跃时戳若
    比其最后完成时刻晚 10 分钟以上，说明是旧种子回填的「现在」虚值——按最后完成
    时刻修正，否则关机实例永远显示「刚活跃」（用户 10-23 报四台关机仍空闲）。
    只动 无执行中任务 且 从未上报版本 的行：在跑/新版报活=真信号，不覆盖。"""
    执行中 = {r[0] for r in con.execute(
        "SELECT DISTINCT 认领者 FROM 预验任务 "
        "WHERE 状态='执行中' AND 认领者 IS NOT NULL").fetchall()}
    最近 = _最近完成时戳(con)
    for 名称, 活跃, 版时 in con.execute(
            "SELECT 名称,最近活跃时戳,版本上报时刻 FROM 实例").fetchall():
        完成戳 = 最近.get(名称)
        if (名称 not in 执行中 and 版时 is None and 完成戳
                and (活跃 or 0) - 完成戳 > 600):
            con.execute("UPDATE 实例 SET 最近活跃时戳=? WHERE 名称=?", (完成戳, 名称))


def 实例认领(con, 时刻函数, runner: str, 平台: str, 物理机: str = "") -> None:
    """task_claim/task_shard_claim 认领点挂钩（须持写锁·调用方 commit）：注册/刷
    平台+物理机指纹+最近活跃。平台/物理机空串（旧客户端不报）不覆盖已有值
    （COALESCE 保最近一次自报）。#1426：物理机键=分片限流分组数据源。"""
    con.execute(
        "INSERT INTO 实例(名称,平台,物理机,注册时刻,最近活跃时戳) VALUES(?,?,?,?,?) "
        "ON CONFLICT(名称) DO UPDATE SET 最近活跃时戳=excluded.最近活跃时戳,"
        "平台=COALESCE(NULLIF(excluded.平台,''),实例.平台),"
        "物理机=COALESCE(NULLIF(excluded.物理机,''),实例.物理机)",
        (runner, 平台 or None, 物理机 or None, 时刻函数(), int(time.time())))


def 实例心跳(con, 时刻函数, runner: str) -> None:
    """task_heartbeat 成功点挂钩（须持写锁）：刷新最近活跃。行不在=补登裸行
    （实例表被人工清理后心跳先于 claim 到达的自愈窗）。"""
    con.execute(
        "INSERT INTO 实例(名称,注册时刻,最近活跃时戳) VALUES(?,?,?) "
        "ON CONFLICT(名称) DO UPDATE SET 最近活跃时戳=excluded.最近活跃时戳",
        (runner, 时刻函数(), int(time.time())))


def 实例完成(con, 时刻函数, runner: str) -> None:
    """task_complete 成功点挂钩（须持写锁）：刷新最近活跃（完成后空闲态由
    实例动态快照 派生·不在表上存状态）。"""
    con.execute(
        "INSERT INTO 实例(名称,注册时刻,最近活跃时戳) VALUES(?,?,?) "
        "ON CONFLICT(名称) DO UPDATE SET 最近活跃时戳=excluded.最近活跃时戳",
        (runner, 时刻函数(), int(time.time())))


def 实例报活(con, 时刻函数, 数据: dict) -> dict:
    """POST /api/instance_ping 写入口（处理写 委托·须持写锁）：UPSERT 实例行+
    版本指纹。runner 必填（缺=ok False 显式拒·不静默）；平台/树头/daemon指纹
    可选（空串不覆盖已有值）。带任一指纹字段=刷 版本上报时刻（区分「旧版
    daemon 从未上报」与「上报过」——疑似离线判定只对后者生效）。
    #1460：响应携 刷新请求 广播戳（实例面板「更新实例」按钮的下发通道——
    旧 daemon 忽略多余字段=向后兼容；每周期 60s 一次，一次广播即全员可见）。"""
    runner = str(数据.get("runner", ""))
    if not runner:
        return {"ok": False, "说明": "runner 标识缺失"}
    平台 = str(数据.get("平台", "")).strip()
    树头 = str(数据.get("树头", "")).strip()
    指纹 = str(数据.get("daemon指纹", "")).strip()
    物理机 = str(数据.get("物理机", "")).strip()   # #1426：空闲实例也落物理机键（让位判定源）
    带指纹 = bool(树头 or 指纹)
    版时 = 时刻函数() if 带指纹 else None
    con.execute(
        "INSERT INTO 实例(名称,平台,物理机,注册时刻,最近活跃时戳,树头,daemon指纹,版本上报时刻) "
        "VALUES(?,?,?,?,?,?,?,?) "
        "ON CONFLICT(名称) DO UPDATE SET "
        "最近活跃时戳=excluded.最近活跃时戳,"
        "平台=COALESCE(NULLIF(excluded.平台,''),实例.平台),"
        "物理机=COALESCE(NULLIF(excluded.物理机,''),实例.物理机),"
        "树头=COALESCE(NULLIF(excluded.树头,''),实例.树头),"
        "daemon指纹=COALESCE(NULLIF(excluded.daemon指纹,''),实例.daemon指纹),"
        "版本上报时刻=COALESCE(excluded.版本上报时刻,实例.版本上报时刻)",
        (runner, 平台 or None, 物理机 or None, 时刻函数(), int(time.time()),
         树头 or None, 指纹 or None, 版时))
    return {"ok": True, "刷新请求": 取刷新请求(con)}


# #1460「更新实例」广播（实例面板按钮→全池空闲实例对齐 develop 树头）：
#   服务端只存一个全局戳（池设置 表），下发靠 instance_ping 响应搭车（daemon 已
#   每 60s 报活·零新增轮询）；客户端空闲点见新戳即 fetch+reset，树头变化后下次
#   报活自然带新指纹（版本列对齐）。重复点击=戳前进（幂等覆盖·无需撤销）。
刷新键 = "实例刷新请求"


def 置刷新请求(con, 时区时刻) -> float:
    """POST /api/instance_refresh 写入口（须持写锁）：记录广播时刻并返回该戳。
    时区时刻 参数与同模块写入口签名对齐（本值需机器可比序=epoch float，非展示文本）。"""
    戳 = time.time()
    con.execute("INSERT INTO 池设置(键,值) VALUES(?,?) "
                "ON CONFLICT(键) DO UPDATE SET 值=excluded.值",
                (刷新键, repr(戳)))
    return 戳


def 取刷新请求(con) -> float:
    """读最新广播戳（须持写锁/同连接）。0.0=从未广播——与 daemon 侧「请求>已执行
    才动作」合取后语义为「无请求无动作」；表值被人工破坏=显式告警按无请求处理
    （不阻断报活主流程，也不静默改值）。"""
    行 = con.execute("SELECT 值 FROM 池设置 WHERE 键=?", (刷新键,)).fetchone()
    if 行 is None:
        return 0.0
    try:
        return float(行[0])
    except (TypeError, ValueError):
        print("[警告] 池设置.%s 值非法（%r）——按无刷新请求处理" % (刷新键, 行[0]),
              flush=True)
        return 0.0


def 实例动态快照(con, 心跳超时秒值: int) -> list[dict]:
    """实例动态派生（/api/state「实例动态」字段·须持写锁）。
    状态四分：
      执行中  =持有执行中整任务/分片且其心跳时戳距今<心跳超时秒（跑轮心跳 60s/次）；
      心跳超时=持有执行中行但心跳≥心跳超时秒（等下次任意 claim 触发回收重派的
              窗口期·警示红——300s 后第一个认领动作会回收该行）；
      疑似离线=空闲（无执行中任务/片）且最后活动距今超过判定阈值——发过版本上报
              的新版 daemon 阈值=报活在线秒（180s·应每 60s 报活）；从未上报的旧版
              daemon 阈值=离线宽限秒（1800s·空闲不报活，但关闭/挂死也会停更）；
      空闲    =其余（含旧版 daemon 30 分钟内活跃——最后活动时刻仅参考不判离线）。
    当前任务：整任务优先，其次从实例领的预验分片（#1402——此前只读父任务认领者，
    从实例跑片期间显示「空闲」，看板看不出多实例并行）。分片行带 片序/片数/形态。
    最近完成 从 预验任务 派生（任务行=唯一权威·实例表不存任务）。
    排序：心跳超时→执行中→空闲→疑似离线（离线沉底·#1461 用户令·同级按名称稳定序）。
    #1426：输出物理机键（限流分组观测面·旧 daemon 空串显示 —）。"""
    现在 = int(time.time())
    行们 = con.execute("SELECT 名称,平台,物理机,注册时刻,最近活跃时戳,树头,daemon指纹,"
                       "版本上报时刻 FROM 实例").fetchall()
    当前 = {r[0]: r for r in con.execute(
        "SELECT 认领者,分支,sha,源分支,认领时刻,心跳时戳 FROM 预验任务 "
        "WHERE 状态='执行中' AND 认领者 IS NOT NULL").fetchall()}
    当前片 = {r[0]: r for r in con.execute(
        "SELECT 认领者,分支,sha,片序,片数,心跳时戳,认领时刻 FROM 预验分片 "
        "WHERE 状态='执行中' AND 认领者 IS NOT NULL").fetchall()}
    # max(序号) 聚合配裸列=SQLite 文档保证取自命中行（每实例最近一条完成）
    最近完成们 = {r[0]: r for r in con.execute(
        "SELECT 认领者,分支,sha,绿,更新时刻,max(序号) FROM 预验任务 "
        "WHERE 状态='完成' AND 认领者 IS NOT NULL GROUP BY 认领者").fetchall()}
    出 = []
    for 名称, 平台, 物理机, 注册, 活跃, 树头, 指纹, 版时 in 行们:
        任务 = 当前.get(名称)
        片 = 当前片.get(名称)
        当前任务 = None
        if 任务 is not None:
            心跳 = 任务[5] or 0
            状态 = "执行中" if (现在 - 心跳) < 心跳超时秒值 else "心跳超时"
            当前任务 = {"分支": 任务[1], "sha": 任务[2],
                        "源分支": 任务[3], "认领时刻": 任务[4]}
        elif 片 is not None:
            心跳 = 片[5] or 0
            活跃 = max(活跃 or 0, 心跳)      # 片心跳也是活信号（task_shard_heartbeat）
            状态 = "执行中" if (现在 - 心跳) < 心跳超时秒值 else "心跳超时"
            当前任务 = {"分支": 片[1], "sha": 片[2], "源分支": None,
                        "认领时刻": 片[6], "片序": 片[3], "片数": 片[4],
                        "形态": "分片"}
        elif 活跃 and (现在 - 活跃) > (报活在线秒 if 版时 is not None else 离线宽限秒):
            状态 = "疑似离线"
        else:
            状态 = "空闲"
        完成 = 最近完成们.get(名称)
        出.append({"名称": 名称, "平台": 平台 or "", "物理机": 物理机 or "",
                   "状态": 状态, "注册时刻": 注册, "最近活跃时戳": 活跃,
                   "树头": 树头 or "", "daemon指纹": 指纹 or "", "版本上报时刻": 版时,
                   "当前任务": 当前任务,
                   "最近完成": ({"分支": 完成[1], "sha": 完成[2], "绿": bool(完成[3]),
                                 "时刻": 完成[4]}) if 完成 else None})
    # #1461：疑似离线沉底（用户 10-23 令）——活跃面优先，关机实例沉到列表最下；
    #   此前 离线 排在 空闲 之前，关机实例夹在活跃实例中间干扰扫读。
    权 = {"心跳超时": 0, "执行中": 1, "空闲": 2, "疑似离线": 3}
    出.sort(key=lambda r: (权[r["状态"]], r["名称"]))
    return 出
