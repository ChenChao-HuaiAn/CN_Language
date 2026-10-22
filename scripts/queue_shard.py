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
# 依赖方向：queue_service → 本模块（单向·本模块禁 import queue_service）。
# con/时戳 由调用方注入（写锁内单线程使用·与 queue_service 既有语义一致）。

import time

分片数下限 = 2                       # =1 即不分片（召唤无意义）
分片数上限 = 12                      # 防滥用上界（#1401：任务少时一个任务吃满空闲实例·ci_shard.分片数上界 同值）
片心跳超时秒 = 300                   # 与父任务 CN_TASK_STALE_SEC 同量级（从实例死亡→片回缺口可重领/被主抢补）


def 建表(con) -> None:
    con.execute("""CREATE TABLE IF NOT EXISTS 预验分片(
        序号 INTEGER PRIMARY KEY AUTOINCREMENT,
        分支 TEXT, sha TEXT,            -- 父任务分支与 sha（冗余存·父完成行裁剪后片仍可读）
        片序 INTEGER, 片数 INTEGER,
        状态 TEXT DEFAULT '缺口',       -- 缺口/执行中/完成
        认领者 TEXT, 心跳时戳 INTEGER,  -- 心跳=服务端收到时刻（免疫客户端时钟漂移·同父任务口径）
        绿 INTEGER, 详情 TEXT, 秒 INTEGER,
        认领时刻 TEXT, 更新时刻 TEXT,
        UNIQUE(分支, 片序))""")


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
    ① 从实例领片（{runner,平台}）——回收心跳超时片后取最旧缺口片（父任务须执行中·
      父认领限制按平台过滤），置执行中+认领者。
    ② 主实例抢补指定片（{runner,分支,片序}）——仅 缺口（含刚回收的）可抢：防主实例
      补跑与从实例认领双跑（服务端 CAS 单点仲裁）。"""
    runner = str(数据.get("runner", ""))
    if not runner:
        return {"ok": False, "说明": "runner 标识缺失"}
    现在 = int(time.time())
    # 片心跳超时回收：执行中片心跳断→回缺口（可被从实例重领/主实例抢补）
    con.execute("UPDATE 预验分片 SET 状态='缺口',认领者=NULL,心跳时戳=NULL,更新时刻=? "
                "WHERE 状态='执行中' AND 心跳时戳 IS NOT NULL AND 心跳时戳<?",
                (时戳(), 现在 - 片心跳超时秒))
    分支 = str(数据.get("分支", ""))
    if 分支:
        # ② 指定片抢补（主实例）——父任务须执行中；片须缺口态
        片序 = 数据.get("片序")
        if not isinstance(片序, int):
            return {"ok": False, "说明": "片序缺失或非法"}
        cur = con.execute(
            "UPDATE 预验分片 SET 状态='执行中',认领者=?,心跳时戳=?,认领时刻=?,更新时刻=? "
            "WHERE 分支=? AND 片序=? AND 状态='缺口' AND "
            "分支 IN (SELECT 分支 FROM 预验任务 WHERE 状态='执行中')",
            (runner, 现在, 时戳(), 时戳(), 分支, 片序))
        if cur.rowcount == 0:
            return {"ok": False, "说明": "片不可抢（不在缺口/父任务不在执行中）%s#%s" % (分支, 片序)}
        行 = con.execute("SELECT sha,片数 FROM 预验分片 WHERE 分支=? AND 片序=?",
                         (分支, 片序)).fetchone()
        return {"ok": True, "片": {"分支": 分支, "sha": str(行[0] or ""),
                                   "片序": 片序, "片数": 行[1]}}
    # ① 从实例领任意缺口片——父任务执行中+认领限制平台过滤，最旧优先
    平台 = str(数据.get("平台", ""))
    行 = con.execute("SELECT s.分支,s.片序,s.片数,t.sha FROM 预验分片 s "
                     "JOIN 预验任务 t ON t.分支=s.分支 "
                     "WHERE s.状态='缺口' AND t.状态='执行中'" + _片认领过滤() +
                     " ORDER BY s.序号 LIMIT 1", (平台,)).fetchone()
    if 行 is None:
        return {"ok": True, "片": None}
    分支, 片序, 片数, sha = 行[0], 行[1], 行[2], str(行[3] or "")
    con.execute("UPDATE 预验分片 SET 状态='执行中',认领者=?,心跳时戳=?,认领时刻=?,更新时刻=? "
                "WHERE 分支=? AND 片序=? AND 状态='缺口'",
                (runner, 现在, 时戳(), 时戳(), 分支, 片序))
    print("[分片认领] %s#%d/%d → %s（1297 从实例补位）" % (分支, 片序, 片数, runner), flush=True)
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
    （片详情消费走 /api/task_shard_result）。"""
    行们 = con.execute("SELECT 片序,片数,状态,认领者,绿,秒,认领时刻,更新时刻 "
                       "FROM 预验分片 WHERE 分支=? ORDER BY 片序", (分支,)).fetchall()
    return [dict(zip(("片序", "片数", "状态", "认领者", "绿", "秒", "认领时刻", "更新时刻"), r))
            for r in 行们]


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
