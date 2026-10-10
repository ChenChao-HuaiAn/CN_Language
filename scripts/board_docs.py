#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""board_docs.py —— 共写文档资源模块（384·文档上服务器二期·服务端唯一权威）。

架构裁决（2026-10-10 用户令「一步到位全治」·承 382 任务台账上服务器之后半场）：
交接/教训/规范覆盖三份多机共写文档（提交史 541/91/145 次·冲突热区）的唯一权威
从 git 文档迁到看板服务端 SQLite：
  - 交接（原 交接.md·按机分节两行制）= POST /api/handoff_add + GET /api/handoff
  - 教训（原 项目记忆/教训.md·三层记忆模型）= POST /api/lesson_add + GET /api/lessons
    （服务端单层化：正文非空=高权重全文条目·正文空=一行索引条目；淘汰三判据
    由登记者在标注列执行，原「归档切片」概念消失——服务端即全量库）
  - 规范覆盖（原 tests/e2e/coverage_map.md·check_spec_coverage 门禁数据源）=
    GET /api/coverage + POST /api/coverage_update /api/coverage_exempt_add
    + POST /api/coverage_import（迁移批量幂等）
    客户端门禁拉取后落本地缓存（gitignore）·服务不可达降级读缓存不卡门禁。
git 历史永存可考古；四文档本体随后续提交删除（384 终局面）。

被 board_service.py import 作 API 层；亦可独立 CLI 运行（迁移/备份/概览）：
  python3 board_docs.py --db <路径> --export <备份.json>
  python3 board_docs.py --db <路径> --stats
"""
from __future__ import annotations

import json
import re
import sqlite3
import sys
import time
from datetime import datetime

时刻 = lambda: datetime.now().strftime("%m-%d %H:%M:%S")
合法机器们 = ("家机", "深度机", "单位机")


def 净化(文本, 上限: int) -> str:
    if 文本 is None:
        return ""
    return re.sub(r"[\r\n]+", " ", str(文本)).strip()[:上限]


# ===== 建表（board_service.建库 挂载）=====

def 建表(con: sqlite3.Connection) -> None:
    con.execute("""CREATE TABLE IF NOT EXISTS 交接(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        机器 TEXT NOT NULL,
        条目 TEXT NOT NULL,
        时刻 TEXT, 时戳 REAL,
        来源 TEXT DEFAULT 'api')""")
    con.execute("CREATE INDEX IF NOT EXISTS idx_交接_机器 ON 交接(机器, id)")
    con.execute("""CREATE TABLE IF NOT EXISTS 教训(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        标题 TEXT NOT NULL,
        正文 TEXT DEFAULT '',
        权重 INTEGER DEFAULT 5,
        标注 TEXT DEFAULT '活跃',
        时刻 TEXT, 时戳 REAL,
        来源 TEXT DEFAULT 'api')""")
    con.execute("""CREATE TABLE IF NOT EXISTS 覆盖单元(
        单元ID TEXT PRIMARY KEY,
        标题 TEXT DEFAULT '',
        正例 TEXT DEFAULT '', 边界例 TEXT DEFAULT '', 负例 TEXT DEFAULT '')""")
    con.execute("""CREATE TABLE IF NOT EXISTS 覆盖豁免(
        用例 TEXT PRIMARY KEY, 理由 TEXT DEFAULT '')""")


# ===== 交接 =====

def 交接添加(con: sqlite3.Connection, 体: dict):
    """追加一条交接（两行制=标题+指针合一段）。返回 (HTTP码, 响应)。"""
    机器 = 净化(体.get("机器"), 20)
    条目 = str(体.get("条目", "") or "").strip()
    if not 机器 or not 条目:
        return 400, {"错误": "机器 与 条目 必填"}
    if len(条目) > 8000:
        return 400, {"错误": f"条目 超长（{len(条目)}>8000）——两行制请精炼"}
    现 = time.time()
    con.execute("INSERT INTO 交接(机器,条目,时刻,时戳,来源) VALUES(?,?,?,?,?)",
                (机器, 条目, 时刻(), 现, 净化(体.get("来源"), 20) or "api"))
    con.commit()
    return 201, {"好": True, "id": con.execute(
        "SELECT last_insert_rowid()").fetchone()[0]}


def 交接查询(con: sqlite3.Connection, 机器: str = "", 上限: int = 20) -> list:
    """倒序取条目；机器空=全部三机混流（前端用），指定=本机节（standup 用）。"""
    上限 = max(1, min(int(上限 or 20), 200))
    if 机器:
        行们 = con.execute("SELECT id,机器,条目,时刻 FROM 交接 WHERE 机器=? "
                          "ORDER BY id DESC LIMIT ?", (机器, 上限)).fetchall()
    else:
        行们 = con.execute("SELECT id,机器,条目,时刻 FROM 交接 "
                          "ORDER BY id DESC LIMIT ?", (上限,)).fetchall()
    return [{"id": r[0], "机器": r[1], "条目": r[2], "时刻": r[3]} for r in 行们]


# ===== 教训 =====

def 教训权重(标题: str) -> int:
    """从标题尾注「权重 9」/「权重 N」提取·缺省 5。"""
    m = re.search(r"权重\s*(\d+)", 标题)
    return int(m.group(1)) if m else 5


def 教训添加(con: sqlite3.Connection, 体: dict):
    """登记一条教训（高权重带全文·索引行正文可空）。同标题幂等跳过（防迁移/重放重复）。
    返回 (HTTP码, 响应)。"""
    标题 = 净化(体.get("标题"), 400)
    if not 标题:
        return 400, {"错误": "标题 必填"}
    正文 = str(体.get("正文", "") or "").strip()      # 全文允许换行·不净化单行化
    权重 = 体.get("权重")
    权重 = int(权重) if 权重 not in (None, "", 0) else 教训权重(标题)
    权重 = max(1, min(权重, 10))
    标注 = 净化(体.get("标注"), 20) or "活跃"
    已有 = con.execute("SELECT id FROM 教训 WHERE 标题=?", (标题,)).fetchone()
    if 已有:
        return 200, {"好": True, "id": 已有[0], "跳过": True}
    现 = time.time()
    m = re.search(r"(20\d{2}-\d{2}-\d{2})", 标题)   # 标题内嵌日期→时戳（时间倒序读法对齐：
    if m:                                            # 源文件/登记序≠时间序·迁移实证）
        try:
            现 = time.mktime(datetime.strptime(m.group(1), "%Y-%m-%d").timetuple())
        except ValueError:
            pass
    con.execute("INSERT INTO 教训(标题,正文,权重,标注,时刻,时戳,来源) VALUES(?,?,?,?,?,?,?)",
                (标题, 正文[:20000], 权重, 标注, 时刻(), 现,
                 净化(体.get("来源"), 20) or "api"))
    con.commit()
    return 201, {"好": True, "id": con.execute(
        "SELECT last_insert_rowid()").fetchone()[0]}


def 教训查询(con: sqlite3.Connection, 高权重: int = 0, 索引: bool = False) -> list:
    """高权重=N → 正文非空且权重≥N 倒序（全文区·standup 必读面）；
    索引=True → 全部条目一行摘要倒序（扫读面）。都空=全量。"""
    if 高权重:
        行们 = con.execute(
            "SELECT id,标题,权重,标注,时刻 FROM 教训 WHERE 正文!='' AND 权重>=? "
            "ORDER BY 时戳 DESC, id DESC", (int(高权重),)).fetchall()   # id DESC：迁移同秒时戳按源文件序（新者先）
        return [{"id": r[0], "标题": r[1], "权重": r[2], "标注": r[3], "时刻": r[4]}
                for r in 行们]
    if 索引:
        行们 = con.execute("SELECT id,标题,权重,标注,时刻,正文!='' FROM 教训 "
                          "ORDER BY 时戳 DESC, id DESC").fetchall()
        return [{"id": r[0], "标题": r[1], "权重": r[2], "标注": r[3], "时刻": r[4],
                 "有全文": bool(r[5])} for r in 行们]
    行们 = con.execute("SELECT id,标题,正文,权重,标注,时刻 FROM 教训 "
                      "ORDER BY 时戳 DESC, id DESC").fetchall()
    return [{"id": r[0], "标题": r[1], "正文": r[2], "权重": r[3],
             "标注": r[4], "时刻": r[5]} for r in 行们]


# ===== 规范覆盖（check_spec_coverage 门禁数据源·原 coverage_map.md）=====

覆盖三态 = ("正例", "边界例", "负例")


def _覆盖行转字典(行) -> dict:
    return {"单元ID": 行[0], "标题": 行[1], "正例": 行[2],
            "边界例": 行[3], "负例": 行[4]}


def 覆盖全量(con: sqlite3.Connection) -> dict:
    单元们 = [_覆盖行转字典(r) for r in con.execute(
        "SELECT 单元ID,标题,正例,边界例,负例 FROM 覆盖单元 ORDER BY 单元ID").fetchall()]
    豁免们 = [{"用例": r[0], "理由": r[1]} for r in con.execute(
        "SELECT 用例,理由 FROM 覆盖豁免 ORDER BY 用例").fetchall()]
    return {"单元们": 单元们, "豁免们": 豁免们, "时刻": 时刻()}


def 覆盖单元更新(con: sqlite3.Connection, 体: dict):
    """单元行整行 upsert（编辑模型=归入单元格·三列各为逗号分隔用例名整列替换）。"""
    单元id = 净化(体.get("单元ID"), 200)
    if not 单元id:
        return 400, {"错误": "单元ID 必填"}
    列 = {k: 净化(体.get(k), 6000) for k in 覆盖三态}
    con.execute(
        """INSERT INTO 覆盖单元(单元ID,标题,正例,边界例,负例) VALUES(?,?,?,?,?)
           ON CONFLICT(单元ID) DO UPDATE SET 标题=excluded.标题, 正例=excluded.正例,
           边界例=excluded.边界例, 负例=excluded.负例""",
        (单元id, 净化(体.get("标题"), 300), 列["正例"], 列["边界例"], 列["负例"]))
    con.commit()
    行 = con.execute("SELECT 单元ID,标题,正例,边界例,负例 FROM 覆盖单元 WHERE 单元ID=?",
                     (单元id,)).fetchone()
    return 200, {"好": True, "单元": _覆盖行转字典(行)}


def 覆盖豁免添加(con: sqlite3.Connection, 体: dict):
    """豁免用例登记 upsert（用例名可逗号分隔批量）。"""
    用例们 = [c.strip() for c in str(体.get("用例", "")).split(",") if c.strip()]
    理由 = 净化(体.get("理由"), 600)
    if not 用例们 or not 理由:
        return 400, {"错误": "用例 与 理由 必填（豁免须逐个登记理由·防漏映射逃逸）"}
    for 用例 in 用例们:
        con.execute("INSERT INTO 覆盖豁免(用例,理由) VALUES(?,?) "
                    "ON CONFLICT(用例) DO UPDATE SET 理由=excluded.理由",
                    (用例[:120], 理由))
    con.commit()
    return 200, {"好": True, "登记数": len(用例们)}


def 覆盖批量导入(con: sqlite3.Connection, 体: dict) -> tuple:
    """迁移批量幂等导入：{单元们:[{单元ID,标题,正例,边界例,负例}], 豁免们:[{用例,理由}]}。
    已有单元ID/用例跳过。返回 (HTTP码, {导入,跳过})。"""
    单元们 = 体.get("单元们")
    豁免们 = 体.get("豁免们")
    if not isinstance(单元们, list) or not isinstance(豁免们, list):
        return 400, {"错误": "单元们/豁免们 须为列表"}
    if len(单元们) > 500 or len(豁免们) > 2000:
        return 400, {"错误": "批量超限（单元 ≤500·豁免 ≤2000）"}
    导入 = 跳过 = 0
    for u in 单元们:
        if not isinstance(u, dict) or not 净化(u.get("单元ID"), 200):
            continue
        已有 = con.execute("SELECT 1 FROM 覆盖单元 WHERE 单元ID=?",
                          (净化(u["单元ID"], 200),)).fetchone()
        if 已有:
            跳过 += 1
            continue
        con.execute(
            "INSERT INTO 覆盖单元(单元ID,标题,正例,边界例,负例) VALUES(?,?,?,?,?)",
            (净化(u["单元ID"], 200), 净化(u.get("标题"), 300),
             净化(u.get("正例"), 6000), 净化(u.get("边界例"), 6000),
             净化(u.get("负例"), 6000)))
        导入 += 1
    豁导 = 豁跳 = 0
    for e in 豁免们:
        if not isinstance(e, dict):
            continue
        for 用例 in [c.strip() for c in str(e.get("用例", "")).split(",") if c.strip()]:
            已有 = con.execute("SELECT 1 FROM 覆盖豁免 WHERE 用例=?",
                              (用例[:120],)).fetchone()
            if 已有:
                豁跳 += 1
                continue
            con.execute("INSERT INTO 覆盖豁免(用例,理由) VALUES(?,?)",
                        (用例[:120], 净化(e.get("理由"), 600)))
            豁导 += 1
    con.commit()
    return 200, {"好": True, "导入": 导入, "跳过": 跳过,
                 "豁免导入": 豁导, "豁免跳过": 豁跳}


# ===== 文档解析（迁移源·migrate_board_docs.py 复用）=====

def 解析交接文档(文本: str) -> list:
    """交接.md → [{机器,条目}]。按 `## <名>节` 切节（节名含机器关键词）·
    `- **` 开头=条目（容忍 `- - **` 双横线残段·相邻续行并入上一条）。"""
    机器 = ""
    出 = []
    缓 = None

    def 落条():
        nonlocal 缓
        if 缓 and 机器:
            出.append({"机器": 机器, "条目": 缓})
        缓 = None

    for 行 in 文本.splitlines():
        s = 行.strip()
        if s.startswith("## "):
            落条()                      # 节尾先落上一条（勿丢）
            机器 = next((m for m in 合法机器们 if m in s[3:]), "")
            continue
        if not 机器:
            continue
        if s.startswith("- - **"):      # 双横线残段容错（交接.md 实录）
            s = s[2:]
        if s.startswith("- **"):
            落条()
            缓 = s[2:].strip()
        elif 缓 and s:
            缓 += " " + s               # 条目折行并入
    落条()
    return 出


def 解析教训文档(文本: str) -> list:
    """教训.md → [{标题,正文,权重}]。三区状态机：文首散排条目区 →
    「一、」全文区（`## 标题`+段落正文）→「二、」索引区（`- ` 行=一行条目）。
    同标题去重取最长正文（历史残段清洗——803 轮「追加而非替换」事故实证：
    高权重区/索引区节头各出现 2 次）。跳过读法引用块与两节头导航行。"""
    条们 = []
    标题 = ""
    正文行们 = []
    区 = "散排"

    def 落条():
        nonlocal 标题, 正文行们
        if 标题 and not 标题.startswith(("一、", "二、")):
            条们.append({"标题": 标题, "正文": "\n".join(正文行们).strip(),
                         "权重": 教训权重(标题)})
        标题, 正文行们 = "", []

    for 行 in 文本.splitlines():
        s = 行.strip()
        if s.startswith("## "):
            落条()
            名 = s[3:].strip()
            if 名.startswith("一、"):
                区 = "全文"
                continue
            if 名.startswith("二、"):
                区 = "索引"
                continue
            标题 = 名
            正文行们 = []
            continue
        if s.startswith(">"):
            continue                     # 读法引用块
        if 区 == "索引":
            if s.startswith("- "):
                落条()
                标题 = s[2:].strip()     # 索引行当独立条（正文空=一行索引）
            elif 标题 and s:
                标题 += " " + s          # 索引行折行并入
            continue
        正文行们.append(行)
    落条()
    # 同标题去重取最长正文
    最 = {}
    for t in 条们:
        旧 = 最.get(t["标题"])
        if not 旧 or len(t["正文"]) > len(旧["正文"]):
            最[t["标题"]] = t
    return list(最.values())


def 解析覆盖文档(文本: str) -> dict:
    """coverage_map.md → {单元们:[…],豁免们:[…]}。主矩阵 5 列表+豁免区 2 列表，
    `## ` 含「豁免」切段（与 check_spec_coverage.parse_map 同构）。"""
    单元们, 豁免们 = [], []
    区 = "main"
    for 行 in 文本.splitlines():
        s = 行.strip()
        if s.startswith("## "):
            区 = "exempt" if "豁免" in s else "main"
            continue
        if not s.startswith("|"):
            continue
        cells = [c.strip() for c in s.strip("|").split("|")]
        if 区 == "main":
            if len(cells) < 5 or cells[0] in ("单元ID", "") or cells[0].startswith((":-", "---")):
                continue
            单元们.append({"单元ID": cells[0], "标题": cells[1],
                           "正例": cells[2], "边界例": cells[3], "负例": cells[4]})
        else:
            if len(cells) < 2 or cells[0] in ("用例", "") or cells[0].startswith((":-", "---")):
                continue
            豁免们.append({"用例": cells[0], "理由": cells[1]})
    return {"单元们": 单元们, "豁免们": 豁免们}


# ===== CLI（备份/概览）=====

def _cli() -> int:
    参数 = sys.argv[1:]

    def 值(名: str, 默认=""):
        return 参数[参数.index(名) + 1] if 名 in 参数 else 默认

    db = 值("--db")
    if not db:
        print("用法: board_docs.py --db <路径> [--export <备份.json>] [--stats]")
        return 2
    con = sqlite3.connect(db, check_same_thread=False)
    con.execute("PRAGMA journal_mode=WAL")
    建表(con)
    con.commit()
    if "--export" in 参数:
        体 = {"导出时刻": 时刻(),
              "交接们": 交接查询(con, "", 100000),
              "教训们": 教训查询(con),
              "覆盖": 覆盖全量(con)}
        with open(值("--export"), "w", encoding="utf-8") as f:
            json.dump(体, f, ensure_ascii=False, indent=1)
        print(f"[备份] 交接 {len(体['交接们'])}·教训 {len(体['教训们'])}"
              f"·覆盖单元 {len(体['覆盖']['单元们'])} → {值('--export')}")
        return 0
    if "--stats" in 参数:
        交 = con.execute("SELECT 机器,COUNT(*) FROM 交接 GROUP BY 机器").fetchall()
        教 = con.execute("SELECT COUNT(*),SUM(正文!=''),SUM(权重>=8) FROM 教训").fetchone()
        覆 = con.execute("SELECT COUNT(*) FROM 覆盖单元").fetchone()[0]
        豁 = con.execute("SELECT COUNT(*) FROM 覆盖豁免").fetchone()[0]
        print(f"[文档] 交接 {交}·教训 共{教[0]} 全文{教[1]} 高权重{教[2]}"
              f"·覆盖单元 {覆}·豁免 {豁}")
        return 0
    print("[败] 未指定动作（--export/--stats）")
    return 2


if __name__ == "__main__":
    sys.exit(_cli())
