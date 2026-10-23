#!/usr/bin/env python3
# 预验任务分片面（1297·2026-10-21 用户令「多实例并行跑一个任务·最大化云端算力」；
#   #1401 重型面再拆片+自适应分片度）：
#   一个预验任务的 E2E 全量拆 n 片（片 0=主实例自带·78_v2 自举链；片 1=79_v2 自举
#   闭环；组外普通用例片由空闲实例认领），片 1..n-1 登记为「缺口」由池内空闲实例
#   自主认领（召唤只登记·不指派——空闲实例轮询时「先领片后领任务」天然自适应，
#   池忙无人领=主实例串行补跑，退化为单实例零损失）。
#   830 规则修订备案：原「一个任务一个实例」本意=禁拼车禁等待；分片不违背本意
#   （多任务忙时每任务仍一片·闲时自动多片）。
# 数据模型：独立新表 预验分片（UNIQUE(分支,片序)）——不碰父表 预验任务 结构；
#   片 0 不落表（主实例自己的 e2e 就是片 0，父任务 task_complete 语义零改动），
#   表内只有片 1..n-1，聚合与 integrate/看板 全走父行=协议零破坏。
# 红即停（902）不破：任一片红=整轮必红，主实例收到即定案，不重跑不复验。
# #1426（2026-10-23 用户令「同机最多 1~2 个分片轮 + 优先独立机」）：认领按**物理机**
#   限并发——同一物理机（键=hostname·平台-架构·指纹6·508 标识）同时执行的分片轮
#   不超过上限（默认 1·CN_SHARD_PER_MACHINE 可配 2）；超限实例不领片（留给其他
#   物理机空闲实例），不阻塞主实例召唤。优先独立机=本机已有执行中片且存在「执行数
#   更少、未达上限、有空闲活跃实例」的其他物理机→本机不抢（比较执行数避免互让死锁）；
#   让位宽限窗（CN_SHARD_MACHINE_GRACE_SEC·默认 120s）后放开，防让位对象不领片饿死。
#   计入面=从片/抢补片行 + 主实例片 0（已召唤分片的执行中父任务）——主从同机也算
#   两轮，否则 2 轮同机绕过上限。主实例召唤/抢补补偿**不受限**（缺片必红=不能用
#   正确性换限流；补偿由主实例仲裁单点跑）。整任务/兜底轮不经过本模块=不受限。
#   旧 daemon 不报指纹=按「未报指纹:<runner>」独立键保守放行+显式告警（不把未知
#   指纹归并成同一机器=不静默错配限流）。
# 依赖方向：queue_service → 本模块（单向·本模块禁 import queue_service）；时刻解析
#   复用 queue_instances.时刻秒（第三处解析点起提炼·不复制）。
# con/时戳 由调用方注入（写锁内单线程使用·与 queue_service 既有语义一致）。
from __future__ import annotations   # P1-1：3.8 注解惰性化（dict/list/set 泛型定义期不求值）

import os
import sqlite3
import time

import queue_instances

分片数下限 = 2                       # =1 即不分片（召唤无意义）
分片数上限 = 12                      # 防滥用上界（#1401：任务少时一个任务吃满空闲实例·ci_shard.分片数上界 同值）
片心跳超时秒 = 300                   # 与父任务 CN_TASK_STALE_SEC 同量级（从实例死亡→片回缺口可重领/被主抢补）

# #1426 同机分片并发上限（默认 1：win 三实例同机三片把普通片 192 例 5~8 分钟拖到
#   1893s 实录；可配 2）。上限只约束从实例自主领片；主实例召唤/抢补不受限。
分片每机上限默认 = 1
分片每机上限环境 = "CN_SHARD_PER_MACHINE"
# 让位宽限窗：缺口片召唤后此秒数内，本机已有片时优先让给更空闲的其他物理机；
#   超窗放开（任何未达上限的机器可领）——防让位对象不领片时片永久饿死。
让位宽限默认秒 = 120
让位宽限环境 = "CN_SHARD_MACHINE_GRACE_SEC"
# 空闲实例判定活跃窗（与 #1428 win 让位/池忙闲同口径：30 分钟内活跃）
空闲活跃秒 = 1800
未报指纹前缀 = "未报指纹:"
_未报指纹告警: set[str] = set()


def _环境整数(名: str, 默认: int, 下限: int) -> int:
    """#1426 限流配置解析：未设=默认；非法值显式告警后用默认（配置面不静默吞键·
    1369 同口径）；下限钳制（上限配置语义见调用点）。"""
    原 = os.environ.get(名)
    if 原 is None or not str(原).strip():
        return 默认
    try:
        值 = int(原)
    except ValueError:
        print("[分片限流] %s=%r 非整数——改用默认 %d" % (名, 原, 默认), flush=True)
        return 默认
    if 值 < 下限:
        print("[分片限流] %s=%d 非法（下限 %d）——按 %d"
              % (名, 值, 下限, 下限), flush=True)
        return 下限
    return 值


def _每机上限() -> int:
    """#1426 每物理机同时执行分片轮上限（1~分片数上限）。0/负值非法——关闭分片的
    正确开关是 CN_SHARD_N=1（客户端不召唤/不领片），不是把限流设为 0。"""
    return min(分片数上限, _环境整数(分片每机上限环境, 分片每机上限默认, 1))


def 建表(con) -> None:
    con.execute("""CREATE TABLE IF NOT EXISTS 预验分片(
        序号 INTEGER PRIMARY KEY AUTOINCREMENT,
        分支 TEXT, sha TEXT,            -- 父任务分支与 sha（冗余存·父完成行裁剪后片仍可读）
        片序 INTEGER, 片数 INTEGER,
        状态 TEXT DEFAULT '缺口',       -- 缺口/执行中/完成
        认领者 TEXT, 心跳时戳 INTEGER,  -- 心跳=服务端收到时刻（免疫客户端时钟漂移·同父任务口径）
        物理机 TEXT,                    -- #1426 执行该片的物理机键（限流计数/审计面）
        绿 INTEGER, 详情 TEXT, 秒 INTEGER,
        认领时刻 TEXT, 更新时刻 TEXT,
        UNIQUE(分支, 片序))""")
    try:   # #1426 物理机列：存量库 ALTER 补（列已在=重复列错吞掉·同 queue_service 先例）
        con.execute("ALTER TABLE 预验分片 ADD COLUMN 物理机 TEXT")
    except sqlite3.OperationalError:
        pass


def _实例机(con, runner: str) -> str:
    """runner → 已报物理机键（实例表）；无行/旧 daemon 未报=空串（不猜）。"""
    if not runner:
        return ""
    行 = con.execute("SELECT 物理机 FROM 实例 WHERE 名称=?", (runner,)).fetchone()
    return str(行[0] or "").strip() if 行 else ""


def _机键(con, runner: str, 行机: str) -> str:
    """分片行/认领请求的物理机键：行上直存 → 实例表反查 → 「未报指纹:<runner>」。
    缺失指纹按独立机器保守放行（每个 runner 一个键=不归并·不静默错配限流），
    同 runner 只告警一次。"""
    机 = str(行机 or "").strip() or _实例机(con, runner)
    if 机:
        return 机
    if runner not in _未报指纹告警:
        _未报指纹告警.add(runner)
        print("[分片限流] %s 未报物理机指纹——按独立机器保守放行（#1426 显式降级）"
              % (runner or "?"), flush=True)
    return 未报指纹前缀 + (runner or "?")


def _机器执行数(con) -> dict[str, int]:
    """物理机 → 正在执行的分片轮数（#1426 限流计数）：①从片/抢补片行（执行中）；
    ②主实例片 0——已召唤分片的执行中父任务不落片行，但主实例确在跑一个分片轮，
    计入其物理机（主从同机=2 轮也受上限约束）。须持写锁。"""
    出: dict[str, int] = {}
    for 认领者, 行机, 数 in con.execute(
            "SELECT 认领者,物理机,COUNT(*) FROM 预验分片 "
            "WHERE 状态='执行中' GROUP BY 认领者,物理机").fetchall():
        键 = _机键(con, str(认领者 or ""), 行机)
        出[键] = 出.get(键, 0) + int(数)
    for 认领者, 行机 in con.execute(
            "SELECT DISTINCT t.认领者, i.物理机 FROM 预验任务 t "
            "LEFT JOIN 实例 i ON i.名称=t.认领者 "
            "WHERE t.状态='执行中' AND t.认领者 IS NOT NULL "
            "AND EXISTS (SELECT 1 FROM 预验分片 s WHERE s.分支=t.分支)").fetchall():
        键 = _机键(con, str(认领者 or ""), 行机)
        出[键] = 出.get(键, 0) + 1
    return 出


def _平台可领(限制: str, 平台: str) -> bool:
    """337 认领限制判定（与 SQL 过滤同口径）：空=不限；非空须白名单含平台。
    平台空=不认作可领（未知平台不占让位名额·防让给领不了的旧实例）。"""
    限制 = str(限制 or "").strip()
    if not 限制:
        return True
    return bool(平台) and ("," + 平台 + ",") in ("," + 限制 + ",")


def _有更空闲机器(con, 我机: str, 限: int, 限制: str) -> bool:
    """#1426 优先独立机：本机已有执行中片时，是否存在「执行数更少、未达上限、
    有空闲活跃实例」的其他物理机——有则本机不抢（让独立机补位）。
    只认已报指纹且有平台的实例（未知不占名额）；比较执行数（而非固定机器序）
    避免多机互让死锁：都空闲/执行数相等=先到先得。须持写锁。"""
    计数 = _机器执行数(con)
    我数 = 计数.get(我机, 0)
    if 我数 <= 0:
        return False
    执行任务 = {r[0] for r in con.execute(
        "SELECT 认领者 FROM 预验任务 WHERE 状态='执行中' AND 认领者 IS NOT NULL")}
    执行片 = {r[0] for r in con.execute(
        "SELECT 认领者 FROM 预验分片 WHERE 状态='执行中' AND 认领者 IS NOT NULL")}
    现在 = int(time.time())
    for 名称, 机, 平台, 活跃 in con.execute(
            "SELECT 名称,物理机,平台,最近活跃时戳 FROM 实例").fetchall():
        机 = str(机 or "").strip()
        平台 = str(平台 or "").strip()
        if not 机 or not 平台 or 机 == 我机:
            continue
        if 名称 in 执行任务 or 名称 in 执行片:
            continue
        if 现在 - int(活跃 or 0) > 空闲活跃秒:
            continue
        if 计数.get(机, 0) >= 限:
            continue
        if not _平台可领(限制, 平台):
            continue
        if 计数.get(机, 0) < 我数:
            return True
    return False


def 召唤(con, 时戳, 数据: dict) -> dict:
    """task_shard_enqueue：主实例召唤——为执行中的父任务登记 片1..n-1 缺口行（幂等：
    已有分片行=不重复登记不空推）。校验父任务 执行中∧认领者匹配（防给排队/别家
    任务挂片）；片数 限 [分片数下限, 分片数上限]。"""
    runner, 分支 = str(数据.get("runner", "")), str(数据.get("分支", ""))
    片数 = 数据.get("片数")
    if not runner or not 分支:
        return {"ok": False, "说明": "runner/分支缺失"}
    if not isinstance(片数, int) or not (分片数下限 <= 片数 <= 分片数上限):
        return {"ok": False, "说明": "片数须 %d~%d" % (分片数下限, 分片数上限)}
    父 = con.execute("SELECT 认领者,sha FROM 预验任务 WHERE 分支=? AND 状态='执行中'",
                     (分支,)).fetchone()
    if 父 is None:
        return {"ok": False, "说明": "父任务不在执行中 " + 分支}
    if 父[0] != runner:
        return {"ok": False, "说明": "非本实例认领的任务 " + 分支}
    已有 = con.execute("SELECT COUNT(*) FROM 预验分片 WHERE 分支=?", (分支,)).fetchone()[0]
    if 已有:
        return {"ok": True, "说明": "已有分片行 %d（幂等不重登记）" % 已有}
    for 片序 in range(1, 片数):
        con.execute("INSERT OR IGNORE INTO 预验分片(分支,sha,片序,片数,状态,更新时刻) "
                    "VALUES(?,?,?,?,'缺口',?)", (分支, str(父[1] or ""), 片序, 片数, 时戳()))
    print("[分片召唤] %s 片数=%d（缺口 %d 片待空闲实例认领·1297）"
          % (分支, 片数, 片数 - 1), flush=True)
    return {"ok": True, "缺口": 片数 - 1}


def _片认领过滤() -> str:
    """认领限制过滤（337 同款口径）：父任务白名单为空=不限；非空=含本平台才可领。"""
    return (" AND (t.认领限制 IS NULL OR t.认领限制='' "
            "OR ','||t.认领限制||',' LIKE '%,'||?||',%')")


def 认领(con, 时戳, 数据: dict) -> dict:
    """task_shard_claim（写锁内原子）：
    ① 从实例领片（{runner,平台,物理机?}）——回收心跳超时片后取最旧缺口片（父任务
      须执行中·父认领限制按平台过滤），按物理机限流（#1426）后置执行中+认领者+
      物理机。
    ② 主实例抢补指定片（{runner,分支,片序}）——仅 缺口（含刚回收的）可抢：防主实例
      补跑与从实例认领双跑（服务端 CAS 单点仲裁）。抢补=正确性补偿路径，**不受
      物理机限流**（缺片必须由主实例补跑，否则整轮必红=用正确性换限流）。"""
    runner = str(数据.get("runner", ""))
    if not runner:
        return {"ok": False, "说明": "runner 标识缺失"}
    现在 = int(time.time())
    # 片心跳超时回收：执行中片心跳断→回缺口（可被从实例重领/主实例抢补）。
    # #1426：物理机归属随回收清空（旧机键只对执行中片有意义·防止陈旧归因计入限流）。
    con.execute("UPDATE 预验分片 SET 状态='缺口',认领者=NULL,物理机=NULL,心跳时戳=NULL,"
                "更新时刻=? WHERE 状态='执行中' AND 心跳时戳 IS NOT NULL AND 心跳时戳<?",
                (时戳(), 现在 - 片心跳超时秒))
    分支 = str(数据.get("分支", ""))
    if 分支:
        # ② 指定片抢补（主实例）——父任务须执行中；片须缺口态
        片序 = 数据.get("片序")
        if not isinstance(片序, int):
            return {"ok": False, "说明": "片序缺失或非法"}
        我机 = _机键(con, runner, str(数据.get("物理机", "")))
        cur = con.execute(
            "UPDATE 预验分片 SET 状态='执行中',认领者=?,物理机=?,心跳时戳=?,认领时刻=?,"
            "更新时刻=? WHERE 分支=? AND 片序=? AND 状态='缺口' AND "
            "分支 IN (SELECT 分支 FROM 预验任务 WHERE 状态='执行中')",
            (runner, 我机, 现在, 时戳(), 时戳(), 分支, 片序))
        if cur.rowcount == 0:
            return {"ok": False, "说明": "片不可抢（不在缺口/父任务不在执行中）%s#%s" % (分支, 片序)}
        行 = con.execute("SELECT sha,片数 FROM 预验分片 WHERE 分支=? AND 片序=?",
                         (分支, 片序)).fetchone()
        return {"ok": True, "片": {"分支": 分支, "sha": str(行[0] or ""),
                                   "片序": 片序, "片数": 行[1]}}
    # ① 从实例领任意缺口片——父任务执行中+认领限制平台过滤，最旧优先
    平台 = str(数据.get("平台", ""))
    行 = con.execute("SELECT s.分支,s.片序,s.片数,t.sha,s.更新时刻,t.认领限制 "
                     "FROM 预验分片 s JOIN 预验任务 t ON t.分支=s.分支 "
                     "WHERE s.状态='缺口' AND t.状态='执行中'" + _片认领过滤() +
                     " ORDER BY s.序号 LIMIT 1", (平台,)).fetchone()
    if 行 is None:
        return {"ok": True, "片": None}
    分支, 片序, 片数 = 行[0], 行[1], 行[2]
    sha, 片更新, 限制 = str(行[3] or ""), 行[4], str(行[5] or "")
    我机 = _机键(con, runner, str(数据.get("物理机", "")))
    限 = _每机上限()
    计数 = _机器执行数(con)
    我数 = 计数.get(我机, 0)
    if 我数 >= 限:
        print("[分片限流] %s 物理机 %s 已执行 %d 片（上限 %d）——本片留给其他物理机（#1426）"
              % (runner, 我机, 我数, 限), flush=True)
        return {"ok": True, "片": None, "限流": "物理机上限", "物理机": 我机,
                "说明": "物理机 %s 分片轮已达上限 %d" % (我机, 限)}
    宽限 = _环境整数(让位宽限环境, 让位宽限默认秒, 0)
    片戳 = queue_instances.时刻秒(片更新)
    龄 = (time.time() - 片戳) if 片戳 is not None else None
    if (龄 is not None and 龄 < 宽限 and not 我机.startswith(未报指纹前缀)
            and _有更空闲机器(con, 我机, 限, 限制)):
        print("[分片让位] %s 物理机 %s 已有 %d 片——让给更空闲的其他物理机（#1426）"
              % (runner, 我机, 我数), flush=True)
        return {"ok": True, "片": None, "限流": "让位独立机", "物理机": 我机,
                "说明": "本机已有片且存在更空闲的其他物理机——本片不抢"}
    con.execute("UPDATE 预验分片 SET 状态='执行中',认领者=?,物理机=?,心跳时戳=?,"
                "认领时刻=?,更新时刻=? WHERE 分支=? AND 片序=? AND 状态='缺口'",
                (runner, 我机, 现在, 时戳(), 时戳(), 分支, 片序))
    print("[分片认领] %s#%d/%d → %s（%s·1297 从实例补位）"
          % (分支, 片序, 片数, runner, 我机), flush=True)
    return {"ok": True, "片": {"分支": 分支, "sha": sha, "片序": 片序, "片数": 片数}}


def 心跳(con, 时戳, 数据: dict) -> dict:
    """task_shard_heartbeat：{runner,分支,片序}——只刷心跳时戳（认领者匹配才续）。"""
    runner, 分支 = str(数据.get("runner", "")), str(数据.get("分支", ""))
    片序 = 数据.get("片序")
    if not isinstance(片序, int):
        return {"ok": False, "说明": "片序缺失或非法"}
    cur = con.execute("UPDATE 预验分片 SET 心跳时戳=?,更新时刻=? "
                      "WHERE 分支=? AND 片序=? AND 认领者=? AND 状态='执行中'",
                      (int(time.time()), 时戳(), 分支, 片序, runner))
    if cur.rowcount == 0:
        return {"ok": False, "说明": "片不在执行中或认领者不匹配 %s#%s" % (分支, 片序)}
    return {"ok": True}


def 完成(con, 时戳, 数据: dict) -> dict:
    """task_shard_complete：{runner,分支,片序,绿,结果,秒}——校验认领者（被回收重派/
    主实例抢补后旧从实例的结果=拒绝·抢补者跑出的结果为准，同 task_complete 口径）。"""
    runner, 分支 = str(数据.get("runner", "")), str(数据.get("分支", ""))
    片序 = 数据.get("片序")
    秒 = 数据.get("秒")
    if not isinstance(片序, int):
        return {"ok": False, "说明": "片序缺失或非法"}
    import json as _json
    详情 = _json.dumps(数据.get("结果"), ensure_ascii=False)[:500000] if 数据.get("结果") is not None else None
    cur = con.execute("UPDATE 预验分片 SET 状态='完成',绿=?,详情=?,秒=?,心跳时戳=NULL,更新时刻=? "
                      "WHERE 分支=? AND 片序=? AND 认领者=? AND 状态='执行中'",
                      (1 if 数据.get("绿") else 0, 详情,
                       int(秒) if isinstance(秒, (int, float)) else None,
                       时戳(), 分支, 片序, runner))
    if cur.rowcount == 0:
        return {"ok": False, "说明": "片不在执行中或认领者不匹配（可能已被回收/抢补）%s#%s" % (分支, 片序)}
    print("[分片完成] %s#%d 绿=%s 秒=%s" % (分支, 片序, bool(数据.get("绿")), 秒), flush=True)
    return {"ok": True}


def 快照(con, 分支: str) -> list[dict]:
    """片状态快照（主实例收片轮询用·GET /api/task_shards?branch=）——不含 详情 大字段
    （片详情消费走 /api/task_shard_result）。#1426：补物理机键（限流/让位审计面）。"""
    行们 = con.execute("SELECT 片序,片数,状态,认领者,物理机,绿,秒,认领时刻,更新时刻 "
                       "FROM 预验分片 WHERE 分支=? ORDER BY 片序", (分支,)).fetchall()
    return [dict(zip(("片序", "片数", "状态", "认领者", "物理机", "绿", "秒",
                      "认领时刻", "更新时刻"), r)) for r in 行们]


def 片详情(con, 分支: str, 片序: int):
    """单片结果详情（主实例聚合 e2e明细 用）——miss=None（调用点显式分流·648）。"""
    行 = con.execute("SELECT 绿,秒,详情 FROM 预验分片 WHERE 分支=? AND 片序=?",
                     (分支, 片序)).fetchone()
    if 行 is None:
        return None
    try:
        import json as _json
        详情 = _json.loads(行[2]) if 行[2] else None
    except ValueError:
        详情 = None
    return {"绿": bool(行[0]) if 行[0] is not None else None, "秒": 行[1], "结果": 详情}


def 清父分片(con, 分支: str) -> None:
    """父任务离开执行中（回收重置排队/已取消）时联动删片行——防孤儿片被认领白跑
    （新派发的父任务会重新召唤·片数可能变化）。调用方须持写锁。"""
    con.execute("DELETE FROM 预验分片 WHERE 分支=?", (分支,))


def 清孤儿分片(con, 保留: int) -> None:
    """父任务完成行裁剪联动：片行随其父分支一起裁（父行不在「最近保留」集内=删）。
    与 queue_service 任务历史裁剪同写锁同事务调用。"""
    con.execute("DELETE FROM 预验分片 WHERE 分支 NOT IN "
                "(SELECT 分支 FROM 预验任务 WHERE 状态 IN ('完成','已取消') "
                "ORDER BY 序号 DESC LIMIT ?) AND 分支 NOT IN "
                "(SELECT 分支 FROM 预验任务 WHERE 状态 IN ('排队','执行中'))", (保留,))
