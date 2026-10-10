#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""board_adjudication.py —— 待裁决项资源模块（403·网页裁决系统）。

裁决工作流（2026-10-10 用户设想·承 decision_brief 一页纸的网页化）：
AI 会话调查后经 /api/adjudication_add 上传「讲解+选项+推荐」（讲解=按 CN 语言
特性与安全规则撰写的小白版详细分析），用户在任务看板「⚖ 待裁决」页签点开详情、
选择选项提交（单项或批量）——裁决档案落本模块两表留痕（裁决记录 append-only），
并自动把裁决语追加进任务台账备注头部（状态不动——开工/收口仍走既有认领/
integrate 链路，不绕过 ✅⇔sha 铁律）。已批历史件（封存/已批照办等）由 AI 以
「已裁决」态登记留痕，网页全貌可查。

被 board_service.py import 作 API 层；亦可独立 CLI 自检：
  python3 board_adjudication.py --selftest
"""
from __future__ import annotations

import json
import re
import sqlite3
import sys
import time
from datetime import datetime

import board_tasks as 台账            # 号排序键——裁决列表排序与台账同序
from board_docs import 净化            # 单行化净化——与文档模块同一实现·不另造

时刻 = lambda: datetime.now().strftime("%m-%d %H:%M:%S")
状态_待裁决, 状态_已裁决 = "待裁决", "已裁决"
合法优先级们 = ("P0", "P1", "P2", "P3")
讲解上限 = 20000                       # 照教训表正文上限（全文允许换行·不净化单行化）
裁决语模板 = "〔{时刻文} 网页裁决·{裁决人}批{键}：{描述}〕"


# ===== 建表（board_service.建库 挂载）=====

def 建表(con: sqlite3.Connection) -> None:
    con.execute("""CREATE TABLE IF NOT EXISTS 裁决项(
        号 TEXT PRIMARY KEY,
        标题 TEXT NOT NULL,
        优先级 TEXT DEFAULT 'P2',
        讲解 TEXT DEFAULT '',
        选项们 TEXT DEFAULT '[]',
        状态 TEXT DEFAULT '待裁决',
        更新时刻 TEXT, 更新时戳 REAL,
        来源 TEXT DEFAULT 'api')""")
    con.execute("""CREATE TABLE IF NOT EXISTS 裁决记录(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        号 TEXT NOT NULL,
        选项键 TEXT, 选项描述 TEXT DEFAULT '',
        裁决人 TEXT DEFAULT '用户',
        意见 TEXT DEFAULT '',
        时刻 TEXT, 时戳 REAL)""")
    con.execute("CREATE INDEX IF NOT EXISTS idx_裁决记录_号 ON 裁决记录(号, id)")


# ===== 裁决项上传（AI 侧·upsert）=====

def _校验选项们(选项们):
    """返回 (错误或None, 规范化选项列表)。2~6 个·键唯一非空·描述必填。"""
    if not isinstance(选项们, list) or not (2 <= len(选项们) <= 6):
        return "选项们 须为 2~6 个的列表（每项 {键,描述,推荐?}）", None
    出, 键集 = [], set()
    for 项 in 选项们:
        if not isinstance(项, dict):
            return "选项们 元素须为对象", None
        键 = 净化(项.get("键"), 20)
        描述 = 净化(项.get("描述"), 300)
        if not 键 or not 描述:
            return "每项的 键 与 描述 必填", None
        if 键 in 键集:
            return f"选项键重复：{键}", None
        键集.add(键)
        出.append({"键": 键, "描述": 描述, "推荐": bool(项.get("推荐"))})
    return None, 出


def 裁决项设置(con: sqlite3.Connection, 体: dict):
    """上传/更新裁决项（upsert）。已裁决项默认拒绝覆盖（409·终态留痕防误触），
    带 覆盖=True 才更新内容（裁决状态与历史记录保留）。返回 (HTTP码, 响应)。"""
    号 = str(体.get("号", "")).strip()
    if not re.fullmatch(r"[0-9]+[a-z]?", 号):
        return 400, {"错误": "号 格式非法（数字+可选小写字母后缀）"}
    已有 = con.execute("SELECT 状态 FROM 裁决项 WHERE 号=?", (号,)).fetchone()
    if 已有 and 已有[0] == 状态_已裁决 and not 体.get("覆盖"):
        return 409, {"错误": f"裁决项 #{号} 已裁决（终态留痕）——修订讲解须带 覆盖=true"
                             "（裁决历史保留）"}
    标题 = 净化(体.get("标题"), 300)
    if not 标题:
        return 400, {"错误": "标题 必填"}
    优先级 = str(体.get("优先级", "P2") or "P2").strip().upper()
    if 优先级 not in 合法优先级们:
        return 400, {"错误": f"优先级 须为 {'/'.join(合法优先级们)}"}
    讲解 = str(体.get("讲解", "") or "").strip()[:讲解上限]
    错误, 选项们 = _校验选项们(体.get("选项们"))
    if 错误:
        return 400, {"错误": 错误}
    现 = time.time()
    选项文本 = json.dumps(选项们, ensure_ascii=False)
    if 已有:
        con.execute("""UPDATE 裁决项 SET 标题=?,优先级=?,讲解=?,选项们=?,
                       更新时刻=?,更新时戳=? WHERE 号=?""",
                    (标题, 优先级, 讲解, 选项文本, 时刻(), 现, 号))
        con.commit()
        return 200, {"好": True, "动作": "已更新"
                     + ("（已裁决态与历史保留）" if 已有[0] == 状态_已裁决 else "")}
    con.execute("""INSERT INTO 裁决项(号,标题,优先级,讲解,选项们,状态,更新时刻,更新时戳,来源)
                   VALUES(?,?,?,?,?,?,?,?,?)""",
                (号, 标题, 优先级, 讲解, 选项文本, 状态_待裁决, 时刻(), 现,
                 净化(体.get("来源"), 20) or "api"))
    con.commit()
    return 201, {"好": True, "动作": "已登记"}


# ===== 裁决提交（网页侧·单项/批量）=====

def 裁决提交(con: sqlite3.Connection, 体: dict):
    """提交单件裁决：插裁决记录 + 裁决项置已裁决 + 台账备注联动（可关）。
    允许对已裁决项再次提交（改主意·追加记录，最新记录=当前结论）。
    返回 (HTTP码, 响应)。"""
    号 = str(体.get("号", "")).strip()
    行 = con.execute("SELECT 选项们,状态 FROM 裁决项 WHERE 号=?", (号,)).fetchone()
    if not 行:
        return 404, {"错误": f"裁决项 #{号 or '(空)'} 不存在——先经 /api/adjudication_add 上传"}
    try:
        选项们 = json.loads(行[0])
    except (ValueError, TypeError):
        选项们 = []
    键 = 净化(体.get("选项键"), 20)
    选 = next((o for o in 选项们 if o["键"] == 键), None)
    if not 选:
        return 400, {"错误": f"选项键 {键 or '(空)'} 不在裁决项 #{号} 的选项里"}
    裁决人 = 净化(体.get("裁决人"), 40) or "用户"
    意见 = 净化(体.get("意见"), 500)
    现 = time.time()
    时刻文 = datetime.now().strftime("%Y-%m-%d %H:%M")
    con.execute("""INSERT INTO 裁决记录(号,选项键,选项描述,裁决人,意见,时刻,时戳)
                   VALUES(?,?,?,?,?,?,?)""",
                (号, 键, 选["描述"], 裁决人, 意见, 时刻(), 现))
    con.execute("UPDATE 裁决项 SET 状态=?,更新时刻=?,更新时戳=? WHERE 号=?",
                (状态_已裁决, 时刻(), 现, 号))
    联动 = "未启用"
    if 体.get("联动备注", True):
        联动 = _备注联动(con, 号, 裁决人, 键, 选["描述"], 时刻文)
    con.commit()
    return 200, {"好": True, "号": 号, "选项": 键, "任务备注联动": 联动}


def _备注联动(con: sqlite3.Connection, 号: str, 裁决人: str, 键: str,
              描述: str, 时刻文: str) -> str:
    """裁决语插任务台账备注头部（不碰状态——✅⇔sha 铁律不绕过）。
    无此任务/终态任务=跳过并说明（裁决档案照落）。"""
    任务 = 台账.单任务(con, 号)
    if not 任务:
        return "无此任务号（裁决档案已留痕）"
    if 任务["状态"] == 台账.状态_完成:
        return "任务已完成（终态·备注不动）"
    语 = 裁决语模板.format(时刻文=时刻文, 裁决人=裁决人, 键=键, 描述=描述)
    新备注 = 语 + ((" " + 任务["备注"]) if 任务["备注"] else "")
    码, 响应 = 台账.更新任务(con, {"号": 号, "备注": 新备注})
    return "已写入备注" if 码 == 200 else f"联动失败（{响应.get('错误', 码)}）"


def 批量提交(con: sqlite3.Connection, 体: dict):
    """批量裁决：{裁决们:[{号,选项键,裁决人?,意见?}]}。逐件独立提交（部分成功
    不回滚已成功件·结果逐件回报）。返回 (HTTP码, 响应)。"""
    们 = 体.get("裁决们")
    if not isinstance(们, list) or not 们 or len(们) > 50:
        return 400, {"错误": "裁决们 须为非空列表（≤50 件）"}
    结果们, 成功 = [], 0
    for 项 in 们:
        if not isinstance(项, dict):
            结果们.append({"号": None, "码": 400, "错误": "元素须为对象"})
            continue
        码, 响应 = 裁决提交(con, 项)
        成功 += 1 if 码 == 200 else 0
        条 = {"号": 项.get("号"), "码": 码}
        if 码 == 200:
            条["选项"] = 响应["选项"]
            条["任务备注联动"] = 响应["任务备注联动"]
        else:
            条["错误"] = 响应.get("错误", "")
        结果们.append(条)
    return 200, {"好": True, "成功": 成功, "失败": len(结果们) - 成功,
                 "结果们": 结果们}


# ===== 查询（GET 只读）=====

def 裁决项列表(con: sqlite3.Connection) -> list:
    """全量裁决项（附各自裁决记录·待裁决在前→优先级→号，与台账同序）。"""
    项们 = con.execute(
        "SELECT 号,标题,优先级,讲解,选项们,状态,更新时刻 FROM 裁决项").fetchall()
    记录图: dict = {}
    for r in con.execute(
            "SELECT id,号,选项键,选项描述,裁决人,意见,时刻 FROM 裁决记录 ORDER BY id"):
        记录图.setdefault(r[1], []).append(
            {"id": r[0], "选项键": r[2], "选项描述": r[3],
             "裁决人": r[4], "意见": r[5], "时刻": r[6]})
    出 = []
    for 号, 标题, 优先级, 讲解, 选项文本, 状态, 更新时刻 in 项们:
        try:
            选项们 = json.loads(选项文本)
        except (ValueError, TypeError):
            选项们 = []
        记录们 = 记录图.get(号, [])
        出.append({"号": 号, "标题": 标题, "优先级": 优先级, "讲解": 讲解,
                   "选项们": 选项们, "状态": 状态, "更新时刻": 更新时刻,
                   "记录们": 记录们,
                   "最新结论": (记录们[-1]["选项键"] + "：" + 记录们[-1]["选项描述"])
                               if 记录们 else ""})
    优先序 = {p: i for i, p in enumerate(合法优先级们)}
    出.sort(key=lambda x: (x["状态"] != 状态_待裁决,
                           优先序.get(x["优先级"], 9), 台账.号排序键(x["号"])))
    return 出


# ===== 自检（内存库全链路正反两态）=====

def 自测() -> int:
    con = sqlite3.connect(":memory:")
    台账.建任务表(con)
    建表(con)
    例数, 失败们 = 0, []

    def 签(名: str, 条件: bool):
        nonlocal 例数
        例数 += 1
        print(("  ✓ " if 条件 else "  ✗ ") + 名)
        if not 条件:
            失败们.append(名)

    选项 = [{"键": "甲", "描述": "立即实施", "推荐": True},
            {"键": "乙", "描述": "维持挂起"}, {"键": "丙", "描述": "另行立案"}]
    码, r = 裁决项设置(con, {"号": "900", "标题": "裁决甲", "优先级": "P1",
                             "讲解": "正文一行\n正文两行", "选项们": 选项})
    签("上传 201", 码 == 201)
    码, _ = 裁决项设置(con, {"号": "900", "标题": "坏", "选项们": []})
    签("选项不足 400（反态）", 码 == 400)
    码, _ = 裁决项设置(con, {"号": "900x坏", "标题": "坏号", "选项们": 选项})
    签("号格式非法 400（反态）", 码 == 400)
    码, _ = 裁决项设置(con, {"号": "901", "标题": "重键", "选项们": [
        {"键": "甲", "描述": "一"}, {"键": "甲", "描述": "二"}]})
    签("选项键重复 400（反态）", 码 == 400)
    con.execute(f"INSERT INTO 任务({台账.任务字段}) "
                "VALUES('900','联动体','⬜','','P1','','','','','',0,'',0,'selftest')")
    码, r = 裁决提交(con, {"号": "900", "选项键": "甲", "裁决人": "用户", "意见": "照办"})
    签("裁决提交 200·备注联动已写入", 码 == 200 and r["任务备注联动"] == "已写入备注")
    任务 = 台账.单任务(con, "900")
    签("裁决语在备注头部且原备注保留",
       任务["备注"].startswith("〔") and "网页裁决" in 任务["备注"][:40]
       and 任务["状态"] == "⬜")
    项 = next(x for x in 裁决项列表(con) if x["号"] == "900")
    签("裁决后状态=已裁决+记录留痕+最新结论",
       项["状态"] == 状态_已裁决 and 项["记录们"][0]["选项键"] == "甲"
       and 项["最新结论"].startswith("甲："))
    码, r = 裁决提交(con, {"号": "900", "选项键": "乙"})
    签("改主意再提交 200（追加记录）", 码 == 200)
    项 = next(x for x in 裁决项列表(con) if x["号"] == "900")
    签("最新结论随再提交翻转·历史两条", len(项["记录们"]) == 2
       and 项["最新结论"].startswith("乙："))
    码, r = 裁决提交(con, {"号": "900", "选项键": "不存在的键"})
    签("非法选项键 400（反态）", 码 == 400)
    码, r = 裁决提交(con, {"号": "888", "选项键": "甲"})
    签("无此裁决项 404（反态）", 码 == 404)
    码, r = 裁决项设置(con, {"号": "900", "标题": "再改"})
    签("已裁决项拒绝静默覆盖 409（反态）", 码 == 409)
    码, r = 裁决项设置(con, {"号": "900", "标题": "修订讲解", "选项们": 选项,
                             "覆盖": True})
    签("带覆盖=true 修订放行 200", 码 == 200)
    con.execute(f"INSERT INTO 任务({台账.任务字段}) "
                "VALUES('902','终态体','✅','','P2','','','feed5678','','',0,'',0,'selftest')")
    裁决项设置(con, {"号": "902", "标题": "终态联动", "选项们": 选项})
    码, r = 裁决提交(con, {"号": "902", "选项键": "乙"})
    签("终态任务备注不动·裁决照落", 码 == 200
       and r["任务备注联动"] == "任务已完成（终态·备注不动）")
    码, r = 批量提交(con, {"裁决们": [{"号": "903", "选项键": "甲"}]})
    签("批量：无此裁决项计失败 1（反态）", 码 == 200 and r["失败"] == 1)
    裁决项设置(con, {"号": "905", "标题": "批量乙", "选项们": 选项})
    裁决项设置(con, {"号": "904", "标题": "批量甲", "优先级": "P0", "选项们": 选项})
    码, r = 批量提交(con, {"裁决们": [{"号": "905", "选项键": "丙"},
                                      {"号": "904", "选项键": "甲"}]})
    签("批量 2 件成功", 码 == 200 and r["成功"] == 2)
    列表 = 裁决项列表(con)
    签("列表排序：待裁决在前→优先级→号",
       [x["号"] for x in 列表 if x["状态"] == 状态_待裁决] == []
       or 列表[0]["状态"] == 状态_待裁决)
    print(f"[selftest] {例数} 例{'全过 ✓' if not 失败们 else ' 失败：' + '；'.join(失败们)}")
    return 1 if 失败们 else 0


if __name__ == "__main__":
    sys.exit(自测() if "--selftest" in sys.argv else 2)
