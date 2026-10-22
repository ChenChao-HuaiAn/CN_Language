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
#   依赖单向：queue_service → 本模块（禁反引）；自测断言见 queue_selftest.py。

import time


# instance_ping 周期（daemon 侧 60s）×3——超此秒数未报活=疑似离线。
# 仅对「发过版本上报」的实例判定（旧版 daemon 空闲不报活·不误标）。
报活在线秒 = 180


def 建实例表(con) -> None:
    """实例表建表（建库 时调用·须在 queue_service.建库 的 commit 前）。
    版本上报时刻 NULL=旧版 daemon 从未上报指纹（疑似离线判定对其豁免）。"""
    con.execute("""CREATE TABLE IF NOT EXISTS 实例(
        名称 TEXT PRIMARY KEY,
        平台 TEXT,                  -- win-x64/linux-x86_64/linux-arm64（claim/ping 自报）
        注册时刻 TEXT,              -- 首次出现时刻（秒级戳·仅登记不回刷）
        最近活跃时戳 INTEGER,       -- epoch·服务端盖章：claim/heartbeat/complete/ping 四点刷新
        树头 TEXT,                  -- 实例仓 HEAD sha 前 10（daemon 启动时自报·跑轮会变·参考面）
        daemon指纹 TEXT,            -- ci_daemon.py 内容 sha256 前 10（进程启动版·版本稳定指标）
        版本上报时刻 TEXT)           -- 最近一次带指纹上报时刻（NULL=旧版 daemon 未上报过）""")


def 回填种子(con, 时刻函数) -> None:
    """部署首屏种子（服务启动·建实例表 后调用一次）：从 预验任务 现存认领者回填
    实例行（INSERT OR IGNORE——已有行不动）。执行中行=唯一可靠在跑信号（活跃时戳
    取其心跳时戳）；完成行认领者=历史实例（活跃时戳取现在=种子近似·该实例下次
    活动即修正）。回填行 平台/版本 NULL 显式——等该实例下次 claim/ping 自报补全。"""
    现在 = int(time.time())
    for 认领者, 心跳 in con.execute(
            "SELECT 认领者, max(心跳时戳) FROM 预验任务 "
            "WHERE 状态='执行中' AND 认领者 IS NOT NULL GROUP BY 认领者").fetchall():
        con.execute("INSERT OR IGNORE INTO 实例(名称,注册时刻,最近活跃时戳) VALUES(?,?,?)",
                    (认领者, 时刻函数(), 心跳 or 现在))
    for (认领者,) in con.execute(
            "SELECT DISTINCT 认领者 FROM 预验任务 "
            "WHERE 状态='完成' AND 认领者 IS NOT NULL").fetchall():
        con.execute("INSERT OR IGNORE INTO 实例(名称,注册时刻,最近活跃时戳) VALUES(?,?,?)",
                    (认领者, 时刻函数(), 现在))


def 实例认领(con, 时刻函数, runner: str, 平台: str) -> None:
    """task_claim 认领成功点挂钩（须持写锁·调用方 commit）：注册/刷平台+最近活跃。
    平台空串（旧客户端不报）不覆盖已有平台（COALESCE 保最近一次自报）。"""
    con.execute(
        "INSERT INTO 实例(名称,平台,注册时刻,最近活跃时戳) VALUES(?,?,?,?) "
        "ON CONFLICT(名称) DO UPDATE SET 最近活跃时戳=excluded.最近活跃时戳,"
        "平台=COALESCE(NULLIF(excluded.平台,''),实例.平台)",
        (runner, 平台 or None, 时刻函数(), int(time.time())))


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
    daemon 从未上报」与「上报过」——疑似离线判定只对后者生效）。"""
    runner = str(数据.get("runner", ""))
    if not runner:
        return {"ok": False, "说明": "runner 标识缺失"}
    平台 = str(数据.get("平台", "")).strip()
    树头 = str(数据.get("树头", "")).strip()
    指纹 = str(数据.get("daemon指纹", "")).strip()
    带指纹 = bool(树头 or 指纹)
    版时 = 时刻函数() if 带指纹 else None
    con.execute(
        "INSERT INTO 实例(名称,平台,注册时刻,最近活跃时戳,树头,daemon指纹,版本上报时刻) "
        "VALUES(?,?,?,?,?,?,?) "
        "ON CONFLICT(名称) DO UPDATE SET "
        "最近活跃时戳=excluded.最近活跃时戳,"
        "平台=COALESCE(NULLIF(excluded.平台,''),实例.平台),"
        "树头=COALESCE(NULLIF(excluded.树头,''),实例.树头),"
        "daemon指纹=COALESCE(NULLIF(excluded.daemon指纹,''),实例.daemon指纹),"
        "版本上报时刻=COALESCE(excluded.版本上报时刻,实例.版本上报时刻)",
        (runner, 平台 or None, 时刻函数(), int(time.time()),
         树头 or None, 指纹 or None, 版时))
    return {"ok": True}


def 实例动态快照(con, 心跳超时秒值: int) -> list[dict]:
    """实例动态派生（/api/state「实例动态」字段·须持写锁）。
    状态四分：
      执行中  =持有执行中任务行且其心跳时戳距今<心跳超时秒（跑轮心跳 60s/次）；
      心跳超时=持有执行中行但心跳≥心跳超时秒（等下次任意 claim 触发回收重派的
              窗口期·警示红——300s 后第一个认领动作会回收该行）；
      疑似离线=空闲且发过版本上报（新版 daemon）但最近活跃距今>报活在线秒
              （空闲报活断流——真关机/daemon 挂·黄警）；
      空闲    =其余（含旧版 daemon 空闲——旧 daemon 空闲不报活·最后活动时刻
              仅参考不判离线·不误标）。
    当前任务/最近完成 均从 预验任务 派生（任务行=唯一权威·实例表不存任务）。
    排序：心跳超时→执行中→疑似离线→空闲（同级按名称稳定序）。"""
    现在 = int(time.time())
    行们 = con.execute("SELECT 名称,平台,注册时刻,最近活跃时戳,树头,daemon指纹,版本上报时刻 "
                       "FROM 实例").fetchall()
    当前 = {r[0]: r for r in con.execute(
        "SELECT 认领者,分支,sha,源分支,认领时刻,心跳时戳 FROM 预验任务 "
        "WHERE 状态='执行中' AND 认领者 IS NOT NULL").fetchall()}
    # max(序号) 聚合配裸列=SQLite 文档保证取自命中行（每实例最近一条完成）
    最近完成们 = {r[0]: r for r in con.execute(
        "SELECT 认领者,分支,sha,绿,更新时刻,max(序号) FROM 预验任务 "
        "WHERE 状态='完成' AND 认领者 IS NOT NULL GROUP BY 认领者").fetchall()}
    出 = []
    for 名称, 平台, 注册, 活跃, 树头, 指纹, 版时 in 行们:
        任务 = 当前.get(名称)
        当前任务 = None
        if 任务 is not None:
            心跳 = 任务[5] or 0
            状态 = "执行中" if (现在 - 心跳) < 心跳超时秒值 else "心跳超时"
            当前任务 = {"分支": 任务[1], "sha": 任务[2],
                        "源分支": 任务[3], "认领时刻": 任务[4]}
        elif 版时 is not None and 活跃 and (现在 - 活跃) > 报活在线秒:
            状态 = "疑似离线"
        else:
            状态 = "空闲"
        完成 = 最近完成们.get(名称)
        出.append({"名称": 名称, "平台": 平台 or "", "状态": 状态,
                   "注册时刻": 注册, "最近活跃时戳": 活跃,
                   "树头": 树头 or "", "daemon指纹": 指纹 or "", "版本上报时刻": 版时,
                   "当前任务": 当前任务,
                   "最近完成": ({"分支": 完成[1], "sha": 完成[2], "绿": bool(完成[3]),
                                 "时刻": 完成[4]}) if 完成 else None})
    权 = {"心跳超时": 0, "执行中": 1, "疑似离线": 2, "空闲": 3}
    出.sort(key=lambda r: (权[r["状态"]], r["名称"]))
    return 出
