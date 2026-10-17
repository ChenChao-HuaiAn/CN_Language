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


def 建用例号台账(con: sqlite3.Connection) -> None:
    """E2E 用例号台账（554 建·577 升级）——与任务号空间独立（用例号 1~806+
    历史序列）；append-only·号不复用（同任务号「号不复用」纪律）。
    577：加「用例名」列——号名配对唯一=跨分支撞号根治（领号即锁死绑定；
    815/816 实录病根=台账只记号不记名，两分支各自自拟同号互不可见）。"""
    con.execute("""CREATE TABLE IF NOT EXISTS 用例号台账(
        号 TEXT PRIMARY KEY,
        机器 TEXT DEFAULT '', 对话id TEXT DEFAULT '',
        用例名 TEXT DEFAULT '',
        说明 TEXT DEFAULT '',
        时刻 TEXT, 时戳 REAL)""")


def 升级用例号台账(con: sqlite3.Connection) -> None:
    """577：对 554 期已部署库做列升级（CREATE IF NOT EXISTS 不动既有表）——
    幂等（PRAGMA 表结构探测）·旧行 用例名 留空=未配名（兼容放行·补录面见
    case_ledger.py 导入本地）。空名不建唯一索引（避免历史空串互撞）。"""
    列名们 = {r[1] for r in con.execute("PRAGMA table_info(用例号台账)").fetchall()}
    if "用例名" not in 列名们:
        con.execute("ALTER TABLE 用例号台账 ADD COLUMN 用例名 TEXT DEFAULT ''")
    con.execute("CREATE UNIQUE INDEX IF NOT EXISTS 用例名唯一 ON 用例号台账(用例名)"
                " WHERE 用例名 != ''")
    con.commit()


def 号排序键(号: str):
    """「308j」→ (308,'j')·「309」→ (309,'')——主号数值序·字母后缀随主号微序。"""
    m = re.fullmatch(r"([0-9]+)([a-z]?)", str(号))
    if not m:
        return (0, str(号))
    return (int(m.group(1)), m.group(2))


def 合法新号(号: str) -> bool:
    return bool(re.fullmatch(r"[0-9]+[a-z]?", str(号))) and str(号) not in 禁用号


def 下一个号(已占号们) -> str:
    """已占并集取最大返回其下一号（字母后缀进位·纯数字遇禁用号继续跳）。
    字母用满进位到下一主号本身（728z→729）——744 修 off-by-one：原 728z→730
    多跳一号（729 空挂成因·既成空挂不回收）。"""
    有效 = [号排序键(n) for n in 已占号们
            if re.fullmatch(r"[0-9]+[a-z]?", str(n)) and str(n) not in 禁用号]
    if not 有效:
        return "1"
    主, 后缀 = max(有效)
    if 后缀:
        if 后缀 == "z":
            return str(主 + 1)
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


# ===== 视野快照与号占冲突（308j/316·654 自 board_service 外迁+台账真值过滤）=====
# 视野快照保留 6h（陈旧视野不进审计防幽灵占号）——记账与冲突共用同一过期口径。

视野亡秒 = 6 * 3600


def 记视野快照(con: sqlite3.Connection, 上报者: str, 视野号们: list) -> None:
    """记视野快照（6h TTL·各机在飞行号上报·654 自 board_service 外迁）。

    744 编号权威收权后视野不参与发号（客户端禁带号·服务端顺序发放），
    仅作号占冲突审计面（/api/numbers）数据源。
    收视野层**不过滤**：视野里的台账未登记号（树内自立项在飞）照记——654 的
    误报根治只在 号占冲突们() 比对层按台账真值分流，不在此处删数据。
    """
    con.execute("DELETE FROM 视野快照 WHERE 时戳<?", (time.time() - 视野亡秒,))
    con.execute(
        """INSERT INTO 视野快照(上报者,号们,时戳,时刻) VALUES(?,?,?,?)
           ON CONFLICT(上报者) DO UPDATE SET 号们=excluded.号们,
           时戳=excluded.时戳, 时刻=excluded.时刻""",
        (上报者, json.dumps(视野号们, ensure_ascii=False), time.time(), 时刻()))
    con.commit()


def 号占冲突们(con: sqlite3.Connection) -> list:
    """同号出现在 ≥2 上报者视野 → 占号冲突（309 双占型·客户端各自树内立项互相不可见）。

    654 根治（实测 404 条全误报）：客户端视野=主表/归档/树内 021 行四源并集，✅ 历史
    号常驻每棵树快照——原「只比 ≥2 上报者」致任意两树互报。比对前按服务端台账真值
    分流：✅ 已收口号=全机可见历史·剔除；⬜/🏃/⏸ 活跃号与台账未登记号（树内自立项
    未登记·309 双占型主面）保留。冲突项补「未登记」标注供审计（API 只加字段不减）。"""
    视野 = con.execute("SELECT 上报者,号们 FROM 视野快照 WHERE 时戳>?",
                       (time.time() - 视野亡秒,)).fetchall()
    号主 = {}
    for 上报者, 号们文本 in 视野:
        try:
            for 号 in json.loads(号们文本):
                号主.setdefault(str(号), []).append(上报者)
        except (ValueError, TypeError):
            pass
    状态图 = {r[0]: r[1] for r in con.execute("SELECT 号,状态 FROM 任务").fetchall()}
    出 = []
    for 号, 主们 in sorted(号主.items(), key=号排序键):
        if len(主们) < 2:
            continue
        状态 = 状态图.get(号)
        if 状态 == 状态_完成:
            continue    # 654：✅ 历史号各树全可见·非冲突（误报根治主面）
        出.append({"号": 号, "上报者们": 主们,
                   "未登记": 状态 is None})   # 台账无此号=树内自立项未登记（审计标注）
    return 出


# ===== 空洞审计（949·跳号可见可查——服务端权威发号的自证面）=====

# 史实常量（744 立法诱因·既成空挂不回收）：家机指定 728 占号段一次性跳过 665~727，
# 729 为「728z→730」off-by-one 多跳（744 已修算法）——两段同为该事故既成洞。
# 号列=TEXT（主键），集合一律字符串形态与 禁用号 同构·免类型双轨。
既成洞_728占号段 = frozenset(str(n) for n in range(665, 730))


def 空洞成因(起: int, 止: int) -> str:
    """空洞段成因闭集三类（949）——纯函数面·自测可直调无需造库。"""
    段号们 = {str(n) for n in range(起, 止 + 1)}
    if 段号们 <= 禁用号:
        return "永久禁用号"
    if 段号们 <= 既成洞_728占号段:
        return "728占号段既成洞（744 立法诱因）"
    return "021时代散洞（迁移前既成）"


def 空洞段们(con: sqlite3.Connection) -> list:
    """台账号序列空洞审计（949）：连续空洞分段+成因+形成上界——「跳号」从隐忧变
    可查事实。成因闭集三类：永久禁用号（353/354）／728占号段既成洞／021时代散洞
    （迁移前自拟号时代既成）。形成上界=段后最小已占号的发放/创建时刻（洞不晚于它
    形成；728 段上界为 10-14 补账时刻、实际形成更早——上界语义「不晚于」仍成立）。
    744 收权（2026-10-14）后服务端顺序发放零跳号——审计面供随时自证：若出现
    形成上界晚于收权的新段即异常，须立案排查。"""
    主号 = sorted(int(m.group(1)) for n in 已占号集(con)
                 if (m := re.fullmatch(r"(\d+)", str(n))))
    if not 主号:
        return []
    占 = set(主号)
    # 号文形态双轨（迁移行五字前导零 '002' vs 发号行裸数字 '2'）——上界取数建
    # int 归一键（960·前导零键 str(int) 取不到→上界空串的 miss 修复）
    def 归一时刻图(sql: str):
        图: dict[int, str] = {}
        for 号文, 时刻文 in con.execute(sql):
            m = re.fullmatch(r"0*(\d+)", str(号文))
            if m:
                图.setdefault(int(m.group(1)), 时刻文)
        return 图
    发号时刻 = 归一时刻图("SELECT 号,时刻 FROM 发号台账")
    任务创建 = 归一时刻图("SELECT 号,创建时刻 FROM 任务")
    段们: list[tuple[int, int]] = []
    起 = 前 = None
    for n in range(1, 主号[-1] + 1):
        if n in 占:
            if 起 is not None:
                段们.append((起, 前))
                起 = None
            continue
        if 起 is None:
            起 = n
        前 = n
    if 起 is not None:
        段们.append((起, 前))
    出 = []
    for a, b in 段们:
        后邻 = next((x for x in range(b + 1, 主号[-1] + 1) if x in 占), None)
        上界 = ""
        if 后邻 is not None:
            上界 = 发号时刻.get(后邻) or 任务创建.get(后邻) or ""
        出.append({"起": a, "止": b, "个数": b - a + 1, "成因": 空洞成因(a, b),
                   "形成上界": 上界})
    return 出


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

def 已占用例号集(con: sqlite3.Connection) -> set:
    return {str(r[0]) for r in con.execute("SELECT 号 FROM 用例号台账").fetchall()}


def 已占用例名集(con: sqlite3.Connection) -> dict:
    """用例名 → 号（仅已配名行·旧空名行=未配名兼容放行）。577 号名配对唯一判据面。"""
    return {r[1]: str(r[0]) for r in con.execute(
        "SELECT 号, 用例名 FROM 用例号台账 WHERE 用例名 != ''").fetchall()}


def 领用例号(con: sqlite3.Connection, 体: dict):
    """E2E 用例编号发放（554 建·577 号名配对唯一·744 编号权威收权）。
    编号权威归服务器（2026-10-14 用户令）：客户端只报用例名（描述）——禁带
    「请求号下限/指定号」（带了 400 拒·旧通道已废）；服务端按台账纯数字最大号+1
    顺序发放，原子递增=多机并发安全绝不重号。
    577 号名配对唯一：台账记「号↔用例名」绑定，此后号或名任一被占即 409；
    用例名必填（E2E 请求必须带描述·防「占号不配名」台账脏行）。
    返回 (HTTP码, 响应)。"""
    非法带号 = (str(体.get("请求号下限", "") or "").strip()
                or str(体.get("指定号", "") or "").strip())
    if 非法带号:
        return 400, {"错误": f"客户端禁带编号请求（收到 {非法带号}）——"
                             "E2E 用例编号由服务器按顺序发放（2026-10-14 用户令）·"
                             "只报用例名领号；游离用例走改号流程重领"}
    机器 = 净化(体.get("机器"), 60)
    对话id = 净化(体.get("对话id"), 40)
    用例名 = 净化(体.get("用例名"), 200)
    说明 = 净化(体.get("说明"), 200)
    if not 用例名:
        return 400, {"错误": "用例名 必填——E2E 领号只报用例名描述（2026-10-14 用户令）"}
    号名图 = {号: 名 for 号, 名 in con.execute(
        "SELECT 号, 用例名 FROM 用例号台账").fetchall()}
    已占 = set(号名图)
    占名图 = {名: 号 for 号, 名 in 号名图.items() if 名}
    if 用例名 in 占名图:
        return 409, {"错误": f"用例「{用例名}」已于号 {占名图[用例名]} 登记（名字占用）——"
                             f"换名或复用既有号"}
    台账最大 = max((int(x) for x in 已占 if re.fullmatch(r"[0-9]+", x)), default=0)
    候选 = 台账最大 + 1
    while str(候选) in 已占:
        候选 += 1
    号 = str(候选)
    现 = time.time()
    try:
        con.execute(
            "INSERT INTO 用例号台账(号,机器,对话id,用例名,说明,时刻,时戳)"
            " VALUES(?,?,?,?,?,?,?)",
            (号, 机器, 对话id, 用例名, 说明, 时刻(), 现))
    except sqlite3.IntegrityError:
        # 号主键或用例名唯一索引撞车（双写并发防御·写锁外亦安全）
        con.rollback()
        return 409, {"错误": f"号 {号} 或用例名「{用例名}」已被占用——请重领"}
    con.commit()
    return 201, {"好": True, "号": 号, "用例名": 用例名, "台账最大": 台账最大,
                 "占用数": len(已占)}


def 补配用例名(con: sqlite3.Connection, 体: dict):
    """已发放存量号补配用例名（811·744 编号权威收权下「已领号未配名」的补录正路）。
    744 禁的是「请求**新号**时客户端带编号」（发号通道一律服务端顺序发放）；本通道
    不发新号——只对台账「已发放且用例名空」的存量号补录其名（658 轮 914 领号未配名
    实证：黄提示建议补录但导入路径已被 744 硬拒·账实一致性缺口）。
    防滥用约束（号名配对唯一·577 同判据）：
      - 号须已在台账且未配名——台账无此号 404 拒（配名不得变相指定新号）；
      - 已配名号 409 拒改（号名绑定锁死·防篡改 815/816 型撞号源头）；
      - 名被占 409（与 领用例号 同源判据·防异号同名）。
    返回 (HTTP码, 响应)。"""
    号 = str(体.get("号", "") or "").strip()
    if not re.fullmatch(r"[0-9]+[a-z]?", 号):
        return 400, {"错误": "号 须为数字（可带单字母后缀）——只对台账已发放存量号补名"}
    用例名 = 净化(体.get("用例名"), 200)
    if not 用例名:
        return 400, {"错误": "用例名 必填——配对即锁定，空名无意义"}
    行 = con.execute("SELECT 用例名 FROM 用例号台账 WHERE 号=?", (号,)).fetchone()
    if 行 is None:
        return 404, {"错误": f"号 {号} 不在用例号台账——补名只对已发放存量号，"
                             "新用例请走 /api/case_number 领号（服务端顺序发放·744）"}
    if 行[0]:
        return 409, {"错误": f"号 {号} 已配名「{行[0]}」——号名绑定锁死不可改（防篡改·"
                             "815/816 型撞号防线）；确系登记错误走人工处置"}
    占名图 = 已占用例名集(con)
    if 用例名 in 占名图:
        return 409, {"错误": f"用例「{用例名}」已于号 {占名图[用例名]} 登记（名字占用）——"
                             "换名或先核对既有登记"}
    try:
        cur = con.execute(
            "UPDATE 用例号台账 SET 用例名=?, 时刻=?, 时戳=? WHERE 号=? AND 用例名=''",
            (用例名, 时刻(), time.time(), 号))
    except sqlite3.IntegrityError:
        # 用例名唯一索引撞车（双写并发防御·写锁外亦安全）
        con.rollback()
        return 409, {"错误": f"用例名「{用例名}」并发撞唯一索引——请重试核对"}
    if cur.rowcount == 0:
        # 并发窗口内被抢先配名（WHERE 用例名='' 落空）——显式报冲突不静默成功
        con.rollback()
        现名 = con.execute("SELECT 用例名 FROM 用例号台账 WHERE 号=?", (号,)).fetchone()
        return 409, {"错误": f"号 {号} 已被并发配名「{现名[0] if 现名 else '?'}」——请核对"}
    con.commit()
    return 200, {"好": True, "号": 号, "用例名": 用例名}


def 查用例号(con: sqlite3.Connection, 用例名: str) -> list:
    """577：台账按用例名查询（含未配名号位·建目录前自检面）——用例名子串匹配，
    v2专用: 前缀名亦按裸主题命中（如 266_不安全边界 命中「v2专用:不安全边界」）；
    "查得到"与"占用判定"同源（同一台账·防两套账）。返回 [{号,用例名,机器,时刻}]。"""
    名 = 净化(用例名, 200)
    裸 = 名[4:] if 名.startswith("v2专用:") else 名
    行们 = con.execute(
        "SELECT 号,用例名,机器,时刻 FROM 用例号台账 WHERE 用例名 LIKE ?"
        " OR (用例名 LIKE 'v2专用:%' AND 用例名 LIKE ?) ORDER BY 时戳",
        (f"%{名}%", f"%{裸}%")).fetchall()
    return [{"号": r[0], "用例名": r[1], "机器": r[2], "时刻": r[3]} for r in 行们]


def 创建任务(con: sqlite3.Connection, 体: dict):
    """立项+发号。编号权威归服务器（2026-10-14 用户令）：客户端禁带号，请求号非空
    一律 400 拒（旧「指定号」通道已废·跳号事故 665~727 空挂即其漏）；服务端按
    下一个号() 顺序发放。返回 (HTTP码, 响应)。双写任务表+发号台账（/api/numbers 流水继续完整）。"""
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
        return 400, {"错误": f"客户端禁带编号请求（收到 请求号={请求号}）——"
                             "编号由服务器按顺序发放（2026-10-14 用户令）·"
                             "新任务请不带请求号立项；接棒既有任务走 wt.py create <已有号>"}
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
    con.execute("INSERT INTO 发号台账(号,机器,对话id,描述,时刻,时戳) VALUES('900','测机','','自测','10-01 00:00:00',0)")
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
    # 554/577/744：E2E 用例号发放（只报用例名·服务端顺序发放·号名配对唯一）——
    # 744 编号权威收权：带 请求号下限/指定号 一律 400（旧通道已废）
    建用例号台账(con)
    升级用例号台账(con)
    码, r = 领用例号(con, {"机器": "测机", "说明": "自测1", "用例名": "自测甲"})
    例数 += 1
    print(f"  [{'✓' if 码 == 201 and r.get('号') == '1' else '✗'}] 用例号首领（空台账 → 1·带名）")
    assert 码 == 201 and r["号"] == "1", r
    码, r = 领用例号(con, {"机器": "测机2", "用例名": "自测丁"})
    例数 += 1
    print(f"  [{'✓' if 码 == 201 and r.get('号') == '2' else '✗'}] 用例号台账连续递增（1 → 2）")
    assert 码 == 201 and r["号"] == "2", r
    码, r = 领用例号(con, {"机器": "测机3", "请求号下限": "900", "用例名": "自测戊"})
    例数 += 1
    print(f"  [{'✓' if 码 == 400 else '✗'}] 用例号带请求号下限 400 拒（744 编号权威收权）")
    assert 码 == 400, r
    码, r = 领用例号(con, {"用例名": "自测己", "指定号": "9000", "机器": "测机4"})
    例数 += 1
    print(f"  [{'✓' if 码 == 400 else '✗'}] 用例号带指定号 400 拒（存量补录通道已废）")
    assert 码 == 400, r
    码, r = 领用例号(con, {"机器": "测机5"})
    例数 += 1
    print(f"  [{'✓' if 码 == 400 else '✗'}] 用例号缺用例名 400 拒（E2E 请求必须带描述）")
    assert 码 == 400, r
    码, r = 领用例号(con, {"用例名": "自测甲", "机器": "测机6"})
    例数 += 1
    print(f"  [{'✓' if 码 == 409 else '✗'}] 用例名重复领号拒绝（409·名已配 1）")
    assert 码 == 409, r
    # —— 811：已发放存量号补配名（补配用例名·744 收权下黄提示「已领号未配名」补录正路）——
    # 手工造「已发放未配名」历史存量行（554 前形态·领用例号 现强制带名造不出）
    con.execute("INSERT INTO 用例号台账(号,机器,用例名,说明,时刻,时戳)"
                " VALUES('50','自测机','','811 历史未配名','x',0)")
    码, r = 补配用例名(con, {"号": "50", "用例名": "自测庚"})
    例数 += 1
    配后 = {号: 名 for 号, 名 in con.execute(
        "SELECT 号, 用例名 FROM 用例号台账").fetchall()}
    print(f"  [{'✓' if 码 == 200 and 配后.get('50') == '自测庚' else '✗'}]"
          " 补配名：已发放未配名号补名 200+名落账")
    assert 码 == 200 and 配后.get("50") == "自测庚", r
    码, r = 补配用例名(con, {"号": "51", "用例名": "自测辛"})
    例数 += 1
    print(f"  [{'✓' if 码 == 404 else '✗'}] 补配名：台账无此号 404（配名不得变相指定新号）")
    assert 码 == 404, r
    码, r = 补配用例名(con, {"号": "50", "用例名": "自测改名未遂"})
    例数 += 1
    print(f"  [{'✓' if 码 == 409 else '✗'}] 补配名：已配名号 409 拒改（防篡改）")
    assert 码 == 409, r
    码, r = 补配用例名(con, {"号": "2", "用例名": "自测丁"})
    例数 += 1
    print(f"  [{'✓' if 码 == 409 else '✗'}] 补配名：名已被占 409（号名配对唯一同判据）")
    assert 码 == 409, r
    码, r = 补配用例名(con, {"号": "50"})
    例数 += 1
    print(f"  [{'✓' if 码 == 400 else '✗'}] 补配名：缺用例名 400")
    assert 码 == 400, r
    码, r = 补配用例名(con, {"号": "abc", "用例名": "自测壬"})
    例数 += 1
    print(f"  [{'✓' if 码 == 400 else '✗'}] 补配名：号格式非法 400")
    assert 码 == 400, r
    # 744：下一个号 字母进位修复（728z 应进位 729 本身·原 off-by-one 多跳至 730）
    assert 下一个号(["728z"]) == "729", 下一个号(["728z"])
    assert 下一个号(["728y"]) == "728z", 下一个号(["728y"])
    assert 下一个号(["739"]) == "740", 下一个号(["739"])
    assert 下一个号(["738", "353", "354"]) == "739", "纯数字跳禁用号"
    例数 += 1
    print("  [✓] 下一个号 字母进位（728z→729·728y→728z）与禁用号跳过")
    # —— 949：空洞审计（成因闭集三类·纯函数直测+内存库端到端分段）——
    例数 += 1
    好 = (空洞成因(353, 354) == "永久禁用号"
          and "728占号段" in 空洞成因(665, 729)
          and 空洞成因(1, 1).startswith("021时代散洞")
          and 空洞成因(300, 400).startswith("021时代散洞"))   # 跨禁用号混合段=散洞
    print(f"  [{'✓' if 好 else '✗'}] 空洞成因闭集三类（禁用号/728段/散洞·混合段归散洞）")
    assert 好, (空洞成因(353, 354), 空洞成因(665, 729), 空洞成因(1, 1))
    con.execute(
        f"INSERT INTO 任务({任务字段}) VALUES('352','禁用号前邻','⬜','','P2','','','','','{时刻()}',0,'',0,'selftest')")
    con.execute(
        f"INSERT INTO 任务({任务字段}) VALUES('355','禁用号后邻','⬜','','P2','','','','','{时刻()}',0,'',0,'selftest')")
    con.execute(
        f"INSERT INTO 任务({任务字段}) VALUES('730','728段后邻','⬜','','P2','','','','','{时刻()}',0,'',0,'selftest')")
    con.execute(
        f"INSERT INTO 任务({任务字段}) VALUES('002','前导零形态后邻','⬜','','P2','','','','','{时刻()}',0,'',0,'selftest')")
    段 = {(s["起"], s["止"]): s for s in 空洞段们(con)}
    例数 += 1
    # 现库已占={2(前导零'002'),352,355,730,900~905}∪发号{900}——预期段：(1,1)散洞·
    # (3,351)散洞·(353,354)禁用号（353/354 留洞被 352/355 围出）·(356,729)散洞
    # （含 728 段但非纯·全段归散洞）·(731,899)散洞；各段上界=后邻占号时刻
    # （960 前导零双轨：(1,1) 后邻='002' 五字形态·上界须按 int 归一键取到）
    好 = (set(段) == {(1, 1), (3, 351), (353, 354), (356, 729), (731, 899)}
          and 段[(353, 354)]["成因"] == "永久禁用号"
          and all(段[k]["成因"].startswith("021时代散洞")
                  for k in ((1, 1), (3, 351), (356, 729), (731, 899)))
          and all(段[k]["形成上界"] for k in 段))
    print(f"  [{'✓' if 好 else '✗'}] 空洞段们 端到端：五段分段+成因归位+形成上界齐（含前导零后邻）")
    assert 好, 段
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
    建用例号台账(con)      # 577：用例号台账（CLI 独立运行面·与 board_service.建库 同构）
    升级用例号台账(con)    # 577：旧库补「用例名」列升级
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
