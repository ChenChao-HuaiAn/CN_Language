#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""board_tasks.py —— 任务台账模块（382·看板 v2·服务端唯一权威）。

架构裁决（2026-10-10 用户令）：看板做立项入口，plans/021 文档退位——任务台账的
唯一权威从 git 021 文档迁到看板服务端 SQLite 任务表：
  - 立项 = POST /api/task_create（服务端发号·写锁事务保证全局唯一·根治撞号/跳号）
  - 状态流转 = POST /api/task_update（⬜→🏃 认领·→✅ 完成须收口 sha·⏸ 挂起）
  - 021 文档此后只读封存（git 历史永存可考古），AI 零读写
状态机：⬜ 待办 → 🏃 在飞 → ✅ 完成；⏸ 挂起可自 ⬜/🏃 进入、可回 ⬜；✅ 为终态
（不可逆——重开场景立新号，与「号不复用」纪律同源）。
编号规则沿用 308j：新号=全局最大有效号+1（基准=任务表∪发号台账∪各机视野）·
禁用号 353/354 永久跳过。

被 board_service.py import 作 API 层；亦可独立 CLI 运行（部署/运维面）：
  python3 board_tasks.py --db <路径> --migrate --021 <主表.md> [--归档 <归档.md>]…
  python3 board_tasks.py --db <路径> --export <备份.json>     # 每日备份
  python3 board_tasks.py --db <路径> --stats                  # 台账概览
"""
from __future__ import annotations

import json
import re
import sqlite3
import sys
import time
from datetime import datetime

# ===== 规则常量（与 wt.py/task_board.py 同源纪律的服务端化）=====
禁用号 = {"353", "354"}                                       # 2026-10-05 用户令永久禁用
状态_待办, 状态_在飞, 状态_挂起, 状态_完成 = "⬜", "🏃", "⏸", "✅"
合法状态们 = (状态_待办, 状态_在飞, 状态_挂起, 状态_完成)
合法优先级们 = ("P0", "P1", "P2", "P3")

时刻 = lambda: datetime.now().strftime("%m-%d %H:%M:%S")


def 建任务表(con: sqlite3.Connection) -> None:
    """任务台账主表——号为主键即唯一性机械保证（INSERT 撞号直接 IntegrityError）。"""
    con.execute("""CREATE TABLE IF NOT EXISTS 任务(
        号 TEXT PRIMARY KEY,
        标题 TEXT NOT NULL,
        状态 TEXT DEFAULT '⬜',
        前置 TEXT DEFAULT '',
        优先级 TEXT DEFAULT 'P2',
        备注 TEXT DEFAULT '',
        分支 TEXT DEFAULT '',
        收口sha TEXT DEFAULT '',
        归属 TEXT DEFAULT '',
        创建时刻 TEXT, 创建时戳 REAL,
        更新时刻 TEXT, 更新时戳 REAL,
        来源 TEXT DEFAULT 'api')""")


def 号排序键(号: str):
    """「308j」→ (308,'j')·「309」→ (309,'')——主号数值序·字母后缀随主号微序。"""
    m = re.fullmatch(r"([0-9]+)([a-z]?)", str(号))
    if not m:
        return (0, str(号))
    return (int(m.group(1)), m.group(2))


def 合法新号(号: str) -> bool:
    return bool(re.fullmatch(r"[0-9]+[a-z]?", str(号))) and str(号) not in 禁用号


def 下一个号(已占号们) -> str:
    """已占并集取最大返回其下一号（字母后缀进位·纯数字遇禁用号继续跳）。"""
    有效 = [号排序键(n) for n in 已占号们
            if re.fullmatch(r"[0-9]+[a-z]?", str(n)) and str(n) not in 禁用号]
    if not 有效:
        return "1"
    主, 后缀 = max(有效)
    if 后缀:
        if 后缀 == "z":
            主 += 1
            后缀 = ""
        else:
            return f"{主}{chr(ord(后缀) + 1)}"
    候选 = 主 + 1
    while str(候选) in 禁用号:
        候选 += 1
    return str(候选)


def 解析任务号们(文本) -> list:
    """「110, 276」/「110，276」→ ['110','276']——容错中英文逗号与空白；非法片段丢弃。"""
    if not 文本:
        return []
    出 = []
    for 片 in re.split(r"[,，\s]+", str(文本)):
        片 = 片.strip()
        if 片 and re.fullmatch(r"[0-9]+[a-z]?", 片):
            出.append(片)
    return 出


def 净化(文本, 上限: int) -> str:
    """单行化+去首尾+截断——任务字段一律防换行破坏与超长。"""
    if 文本 is None:
        return ""
    return re.sub(r"[\r\n]+", " ", str(文本)).strip()[:上限]


def 已占号集(con: sqlite3.Connection) -> set:
    """发号基准=任务表∪发号台账（历史流水含未立项号·防号复用）。"""
    占 = {r[0] for r in con.execute("SELECT 号 FROM 任务").fetchall()}
    占 |= {r[0] for r in con.execute("SELECT 号 FROM 发号台账").fetchall()}
    return 占


# ===== 行转字典（API 输出面）=====

def _行转任务(行) -> dict:
    return {"号": 行[0], "标题": 行[1], "状态": 行[2], "前置": 行[3],
            "优先级": 行[4], "备注": 行[5], "分支": 行[6], "收口sha": 行[7],
            "归属": 行[8], "创建时刻": 行[9], "更新时刻": 行[11],
            "更新时戳": 行[12], "来源": 行[13]}


任务字段 = "号,标题,状态,前置,优先级,备注,分支,收口sha,归属,创建时刻,创建时戳,更新时刻,更新时戳,来源"


def 单任务(con: sqlite3.Connection, 号: str):
    行 = con.execute(f"SELECT {任务字段} FROM 任务 WHERE 号=?", (str(号),)).fetchone()
    return _行转任务(行) if 行 else None


def 僵尸疑天数() -> float:
    """392 僵尸行侦测阈值（天）——环境变量 BOARD_ZOMBIE_DAYS 可调，缺省 7。"""
    import os
    try:
        return max(1.0, float(os.environ.get("BOARD_ZOMBIE_DAYS", "7")))
    except ValueError:
        return 7.0


def 全部任务(con: sqlite3.Connection) -> list:
    """全量任务·附「就绪」判定（⬜ 且前置全部 ✅——服务端算好前端免算）。
    392 僵尸疑：⬜ 且从未认领（分支空）且超期无更新 → 就绪=False+僵尸疑=True
    （疑似已修未销/死行——修完集成却漏销的行冒充待办诱惑重复认领·373/332 五行实录；
    挂起 ⏸ 不判——待裁决行长挂是常态）。人核实后 /api/task_update 置 ✅ 或重开。"""
    们 = [_行转任务(r) for r in con.execute(
        f"SELECT {任务字段} FROM 任务").fetchall()]
    们.sort(key=lambda t: 号排序键(t["号"]))
    状态图 = {t["号"]: t["状态"] for t in 们}
    超期秒 = 僵尸疑天数() * 86400
    现 = time.time()
    for t in 们:
        前置们 = 解析任务号们(t["前置"])
        t["前置们"] = 前置们
        t["僵尸疑"] = (t["状态"] == 状态_待办 and not t["分支"]
                       and (现 - float(t["更新时戳"] or 0)) > 超期秒)
        t["就绪"] = (t["状态"] == 状态_待办 and not t["僵尸疑"] and
                     all(状态图.get(p) == 状态_完成 for p in 前置们))
    # 就绪优先·同级按优先级·再按号；僵尸疑沉底——前端列表默认序
    优先序 = {p: i for i, p in enumerate(合法优先级们)}
    们.sort(key=lambda t: (not t["就绪"], t["僵尸疑"],
                          优先序.get(t["优先级"], 9), 号排序键(t["号"])))
    return 们


def 任务字典图(con: sqlite3.Connection) -> dict:
    """号→{标题,状态,优先级}——兼容旧客户端（intent.py 收口惰性清等）的实时版。"""
    return {r[0]: {"标题": r[1], "状态": r[2], "优先级": r[3]}
            for r in con.execute("SELECT 号,标题,状态,优先级 FROM 任务").fetchall()}


def 待办快照们(con: sqlite3.Connection, 上限: int = 20) -> list:
    """旧 /api/board「快照021」兼容字段：待办任务按默认序截前 N——旧语义不炸。"""
    return [{"号": t["号"], "标题": t["标题"], "状态": t["状态"],
             "优先级": t["优先级"], "疑似认领": False}
            for t in 全部任务(con) if t["状态"] == 状态_待办][:上限]


# ===== 立项 / 状态流转（写锁由调用方持有·事务在函数内提交）=====

def 创建任务(con: sqlite3.Connection, 体: dict):
    """立项+发号。请求号空=服务端发放（max+1）·非空=指定号（已占 409·禁用号 403）。
    返回 (HTTP码, 响应)。双写任务表+发号台账（/api/numbers 流水继续完整）。"""
    标题 = 净化(体.get("标题"), 300)
    if not 标题:
        return 400, {"错误": "标题 必填"}
    优先级 = str(体.get("优先级", "P2") or "P2").strip().upper()
    if 优先级 not in 合法优先级们:
        return 400, {"错误": f"优先级 须为 {'/'.join(合法优先级们)}"}
    前置们 = 解析任务号们(体.get("前置", ""))
    备注 = 净化(体.get("备注"), 2000)
    机器 = 净化(体.get("机器"), 60)
    来源 = 净化(体.get("来源"), 20) or "api"
    请求号 = str(体.get("请求号", "") or "").strip()
    if 请求号:
        if not re.fullmatch(r"[0-9]+[a-z]?", 请求号):
            return 400, {"错误": "请求号 格式非法（数字+可选小写字母后缀，如 382/308a）"}
        if 请求号 in 禁用号:
            return 403, {"错误": f"号 {请求号} 为永久禁用号（353/354·2026-10-05 用户令）"}
        if 请求号 in 已占号集(con):
            return 409, {"错误": f"号 {请求号} 已被占（任务表/发号台账）——"
                                 f"新任务请留空请求号由服务端发放"}
        号 = 请求号
    else:
        号 = 下一个号(已占号集(con))
        if 号 in 禁用号:   # 双保险（下一个号 已跳，此处防未来改动回归）
            return 500, {"错误": f"发号异常：{号}"}
    现 = time.time()
    con.execute(
        f"INSERT INTO 任务({任务字段}) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
        (号, 标题, 状态_待办, ",".join(前置们), 优先级, 备注,
         净化(体.get("分支"), 120), "", 净化(体.get("归属"), 80),
         时刻(), 现, 时刻(), 现, 来源))
    con.execute(
        "INSERT INTO 发号台账(号,机器,对话id,描述,时刻,时戳) VALUES(?,?,?,?,?,?)",
        (号, 机器 or 来源, 净化(体.get("对话id"), 40), 标题[:200], 时刻(), 现))
    con.commit()
    return 201, {"好": True, "号": 号, "任务": 单任务(con, 号)}


def 更新任务(con: sqlite3.Connection, 体: dict):
    """状态流转+字段修订。⬜→🏃 自动补分支 任务/<号>·→✅ 须 8~40 位 hex 收口 sha·
    ✅ 终态不可逆。返回 (HTTP码, 响应)。"""
    号 = str(体.get("号", "")).strip()
    旧 = 单任务(con, 号)
    if not 旧:
        return 404, {"错误": f"任务 {号 or '(空)'} 不存在——先 /api/task_create 立项"}
    if 旧["状态"] == 状态_完成:
        return 400, {"错误": f"任务 {号} 已完成（终态不可改）——重开场景请立新号"}
    新状态 = str(体.get("状态", "") or "").strip() or 旧["状态"]
    if 新状态 not in 合法状态们:
        return 400, {"错误": f"状态 须为 {'/'.join(合法状态们)}"}
    收口sha = str(体.get("收口sha", "") or "").strip()
    if 新状态 == 状态_完成:
        if not re.fullmatch(r"[0-9a-f]{8,40}", 收口sha):
            return 400, {"错误": "完成（✅）须带 8~40 位 hex 收口 sha（账实铁律 ✅⇔sha 的服务端化）"}
    else:
        收口sha = ""
    分支 = 净化(体.get("分支"), 120)
    if 新状态 == 状态_在飞 and not 分支:
        分支 = f"任务/{号}"     # 分支命名纪律：分支名=任务/<号>（服务端兜底合成）
    新值 = {
        "标题": 净化(体.get("标题", 旧["标题"]) or 旧["标题"], 300),
        "状态": 新状态,
        "前置": ",".join(解析任务号们(体.get("前置", 旧["前置"]))
                         if 体.get("前置") is not None else 解析任务号们(旧["前置"])),
        "优先级": str(体.get("优先级", 旧["优先级"]) or 旧["优先级"]).strip().upper(),
        "备注": 净化(体.get("备注", 旧["备注"]) if 体.get("备注") is not None else 旧["备注"], 2000),
        "分支": 分支 or (旧["分支"] if 新状态 != 状态_待办 else ""),
        "收口sha": 收口sha,
        "归属": 净化(体.get("归属"), 80) or 旧["归属"],
    }
    if 新值["优先级"] not in 合法优先级们:
        return 400, {"错误": f"优先级 须为 {'/'.join(合法优先级们)}"}
    con.execute(
        """UPDATE 任务 SET 标题=?,状态=?,前置=?,优先级=?,备注=?,分支=?,收口sha=?,
           归属=?,更新时刻=?,更新时戳=? WHERE 号=?""",
        (新值["标题"], 新值["状态"], 新值["前置"], 新值["优先级"], 新值["备注"],
         新值["分支"], 新值["收口sha"], 新值["归属"], 时刻(), time.time(), 号))
    con.commit()
    return 200, {"好": True, "任务": 单任务(con, 号)}


# ===== 021 迁移（一次性·幂等）=====

def 解析021行(行文本: str):
    """021 表格行 → 任务字典；非任务行（表头/分隔/空/里程碑 3 列行）返回 None。
    与 task_board.py 解析任务行 同构的最小面（号|标题|状态|前置|优先级|备注）。"""
    s = 行文本.strip()
    if not s.startswith("|"):
        return None
    段们 = [x.strip() for x in s.strip("|").split("|")]
    if len(段们) < 6:
        return None
    号 = 段们[0]
    if not re.fullmatch(r"[0-9]+[a-z]?", 号):
        return None
    m = re.match(r"^([⬜🏃⏸✅])\s*([0-9a-f]{8,40})?$", 段们[2])
    if not m:
        return None
    前置段 = 段们[3]
    mb = re.search(r"分支\s*=\s*(任务/[0-9]+[a-z]?)", 段们[5])
    return {"号": 号, "标题": 净化(段们[1], 300), "状态": m.group(1),
            "收口sha": m.group(2) or "",
            "前置": "" if 前置段 in ("—", "-") else ",".join(解析任务号们(前置段)),
            "优先级": 段们[4] if 段们[4] in 合法优先级们 else "P2",
            "备注": 净化(段们[5], 2000),
            "分支": mb.group(1) if mb else ""}


def 迁移导入(con: sqlite3.Connection, 主表文本: str, 归档文本们: list) -> dict:
    """021 主表+归档切片 → 任务表。幂等（任务表已有号跳过·**只查任务表**——发号台账
    只记「发过号」不等于「有任务行」（旧制度行住分支树 021），382 迁移实录：
    在飞任务因台账有发号记录被误跳→台账缺行。归档行一律按 ✅ 记。
    返回 {导入, 跳过, 样例们}。"""
    导入 = 跳过 = 0
    样例们 = []
    for 文本, 是否归档 in [(主表文本, False)] + [(t, True) for t in 归档文本们]:
        for 行 in 文本.splitlines():
            t = 解析021行(行)
            if not t:
                continue
            已有 = con.execute("SELECT 1 FROM 任务 WHERE 号=?", (t["号"],)).fetchone()
            if 已有:
                跳过 += 1
                continue
            状态 = 状态_完成 if (是否归档 or t["状态"] == 状态_完成) else t["状态"]
            现 = time.time()
            con.execute(
                f"INSERT INTO 任务({任务字段}) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                (t["号"], t["标题"], 状态, t["前置"], t["优先级"], t["备注"],
                 t["分支"], t["收口sha"], "", 时刻(), 现, 时刻(), 现, "迁移021"))
            导入 += 1
            if len(样例们) < 5:
                样例们.append(f'{t["号"]} {状态} {t["标题"][:24]}')
    con.commit()
    return {"导入": 导入, "跳过": 跳过, "样例们": 样例们}


# ===== CLI（部署/备份/概览·TX_01 上独立运行）=====

def 自测() -> int:
    """--selftest：僵尸疑判定正反例（392·sqlite 内存库·无文件副作用）。"""
    con = sqlite3.connect(":memory:")
    建任务表(con)
    con.execute("""CREATE TABLE 发号台账(
        号 TEXT PRIMARY KEY, 机器 TEXT, 对话id TEXT DEFAULT '',
        描述 TEXT DEFAULT '', 时刻 TEXT, 时戳 REAL)""")
    con.execute("INSERT INTO 发号台账(号,机器,对话id,描述,时刻,时戳) VALUES('900','测机','','自测','',0)")
    新 = time.time()
    行们 = [
        # (号, 状态, 分支, 更新时戳, 期望僵尸疑, 期望就绪)
        ("901", 状态_待办, "", 新 - 8 * 86400, True, False),    # ⬜ 无分支超期=僵尸疑
        ("902", 状态_待办, "任务/902", 新 - 8 * 86400, False, True),  # 有分支在飞过=非僵尸
        ("903", 状态_待办, "", 新 - 1 * 86400, False, True),    # 新行未超期=正常就绪
        ("904", 状态_挂起, "", 新 - 30 * 86400, False, False),  # ⏸ 挂起行长挂是常态=不判
        ("905", 状态_完成, "", 新 - 30 * 86400, False, False),  # ✅ 终态=不判
    ]
    for 号, 状态, 分支, 时戳 in [(r[0], r[1], r[2], r[3]) for r in 行们]:
        con.execute(
            f"INSERT INTO 任务({任务字段}) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
            (号, f"自测{号}", 状态, "", "P2", "", 分支, "", "",
             时刻(), 新, 时刻(), 时戳, "selftest"))
    图 = {t["号"]: t for t in 全部任务(con)}
    例数 = 0
    for 号, _状态, _分支, _时戳, 期望僵尸, 期望就绪 in 行们:
        例数 += 1
        好 = 图[号]["僵尸疑"] == 期望僵尸 and 图[号]["就绪"] == 期望就绪
        print(f"  [{'✓' if 好 else '✗'}] 僵尸疑 #{号}：僵尸={图[号]['僵尸疑']}（期望 {期望僵尸}）"
              f" 就绪={图[号]['就绪']}（期望 {期望就绪}）")
        assert 好, f"僵尸疑判定失败：#{号}"
    序 = [t["号"] for t in 全部任务(con) if t["僵尸疑"]]
    assert 序 == ["901"], f"僵尸行应沉底（就绪区外）·实际排序提取={序}"
    例数 += 1
    print(f"  [✓] 僵尸行排序沉底（就绪区外）")
    print(f"[selftest] {例数} 例全过 ✓")
    return 0


def _cli() -> int:
    参数 = sys.argv[1:]
    if "--selftest" in 参数:
        return 自测()
    def 值(名: str, 默认=""):
        return 参数[参数.index(名) + 1] if 名 in 参数 else 默认
    db = 值("--db")
    if not db:
        print("用法: board_tasks.py --db <路径> [--migrate --021 <主表.md> (--归档 <归档.md>)*]"
              " [--export <备份.json>] [--stats]")
        return 2
    con = sqlite3.connect(db, check_same_thread=False)
    con.execute("PRAGMA journal_mode=WAL")
    建任务表(con)
    # 发号台账（CLI 独立运行时也要有——已占号基准含台账历史流水·与 board_service.建库 同构）
    con.execute("""CREATE TABLE IF NOT EXISTS 发号台账(
        号 TEXT PRIMARY KEY,
        机器 TEXT, 对话id TEXT DEFAULT '', 描述 TEXT DEFAULT '',
        时刻 TEXT, 时戳 REAL)""")
    con.commit()
    if "--migrate" in 参数:
        主表 = 值("--021")
        if not 主表:
            print("[败] --migrate 须 --021 <主表.md>")
            return 2
        归档们 = []
        i = 0
        while i < len(参数):
            if 参数[i] == "--归档":
                归档们.append(参数[i + 1])
                i += 2
            else:
                i += 1
        with open(主表, encoding="utf-8") as f:
            主文本 = f.read()
        档文本们 = []
        for 路径 in 归档们:
            with open(路径, encoding="utf-8") as f:
                档文本们.append(f.read())
        结 = 迁移导入(con, 主文本, 档文本们)
        print(f"[迁移] 导入 {结['导入']}·跳过(已占) {结['跳过']}·样例: {结['样例们']}")
        return 0
    if "--export" in 参数:
        出 = 值("--export")
        体 = {"导出时刻": 时刻(), "任务们": 全部任务(con)}
        with open(出, "w", encoding="utf-8") as f:
            json.dump(体, f, ensure_ascii=False, indent=1)
        print(f"[备份] {len(体['任务们'])} 任务 → {出}")
        return 0
    if "--stats" in 参数:
        们 = 全部任务(con)
        按 = {}
        for t in 们:
            按[t["状态"]] = 按.get(t["状态"], 0) + 1
        print(f"[台账] 共 {len(们)} 任务·分布 {按}·下一号 {下一个号(已占号集(con))}")
        return 0
    print("[败] 未指定动作（--migrate/--export/--stats）")
    return 2


if __name__ == "__main__":
    sys.exit(_cli())
