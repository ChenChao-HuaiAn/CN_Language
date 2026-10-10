#!/usr/bin/env python3
# 会话意图+任务台账看板服务（308a 立·382 看板 v2·TX_01 部署）：
#   308a：治「认领状态四处漂移」——285 撞车实录：主树 021 行滞后（226 立项行随分支·
#   设计内窗口）+本地 refs 过期，两机各信各的账。本服务=会话意图的单一事实源。
#   382 架构裁决（2026-10-10 用户令）：看板做立项入口，plans/021 文档退位——
#   任务台账唯一权威=本服务 SQLite 任务表（board_tasks.py 模块）：
#     立项 /api/task_create（服务端发号全局唯一）·流转 /api/task_update（认领/挂起/完成）
#     查询 /api/tasks·/api/task/<号>——021 文档此后只读封存，AI 零读写。
#   意图/在飞分支/心跳实时层照旧（三机任一活着看板即有实时数据）。
# API（POST 须 Bearer 令牌·GET 只读放行·照 queue_service 口径）：
#   GET  /                    网页看板（board_www/ 静态三件·墨韵设计系统）
#   GET  /api/intents         {意图们:[…], 失联秒}
#   GET  /api/board           聚合 {意图们, 在飞分支们, 任务们, 任务字典, 快照021,
#                                  冲突们, 号占冲突们, 失联秒, 时刻}
#   GET  /api/tasks           任务台账全量（含就绪判定·服务端排序）
#   GET  /api/task/<号>       单任务详情
#   POST /api/intent          {机器,对话id,在做,计划,备注}   登记/更新（即心跳·幂等）
#   POST /api/intent_release  {机器,对话id}                  注销（会话收工）
#   POST /api/report_flights  {分支们:[{分支,提交,时刻}]}     在飞分支上报（客户端 ls-remote 结果
#                                 ·服务端不持 git 凭据不依赖外网——三机任一活着看板即有数据）
#   POST /api/report_021      旧 021 快照通道（382 退役中·仅保旧客户端兼容不炸）
#   POST /api/task_create     {标题,请求号?,前置?,优先级?,备注?,机器?,来源?}  立项+发号（201/409）
#   POST /api/task_update     {号,状态?,分支?,收口sha?,归属?,标题?,前置?,优先级?,备注?} 流转
#   POST /api/claim_number    308j 发号/核对（382 后仅视野记账价值·任务表已为唯一权威）
# 过期：心跳断 CN_INTENT_STALE_SEC（默认 1800s）→ 失联态（行保留·UI 灰显+失联徽章）。
# 心跳时戳=服务端收到时刻（911 教训：免疫各机时钟漂移）。任务表=持久落盘永不过期。
# 部署：/etc/systemd/system/cn-board.service（Environment= CN_BOARD_PORT/CN_BOARD_TOKEN/CN_BOARD_DB）
#   + board_www/ 静态目录随 board_service.py 同目录部署（deploy_board.sh 一键）。
# 自检：python3 board_service.py --selftest（内存 db+随机端口·意图/冲突/发号/任务
#   立项流转/迁移导入全链+401 反态）。
# 迁移：python3 board_tasks.py --db <路径> --migrate --021 <主表.md> [--归档 <归档.md>]*

import json
import os
import re
import sqlite3
import sys
import threading
import time
from datetime import datetime
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

sys.path.insert(0, str(Path(__file__).resolve().parent))
import board_tasks as 台账   # 任务台账模块（382·发号/CRUD/状态机/迁移）

# ===== 可调常量 =====
端口 = int(os.environ.get("CN_BOARD_PORT", "8301"))
令牌 = os.environ.get("CN_BOARD_TOKEN", "")
db路径 = os.environ.get("CN_BOARD_DB",
                        str(os.path.dirname(os.path.abspath(__file__)) + "/board_state.db"))
失联秒 = int(os.environ.get("CN_INTENT_STALE_SEC", "900"))    # 心跳断此秒数=会话静默（15min·AI 活跃时路过续约密·打断后及时体现；行保留·任务态不受影响）
自动清秒 = int(os.environ.get("CN_INTENT_PURGE_SEC", "7200"))  # 静默超此秒数=自动注销（会话被杀无 release 兜底）
快照保留秒 = 3600                                             # 在飞分支/021 快照超过此秒数不再展示（陈旧数据防误导）
在飞保留条数 = 60                                             # 在飞分支表裁剪上限

写锁 = threading.Lock()          # SQLite 写串行化（读靠 WAL 并发·同 queue_service）
时刻 = lambda: datetime.now().strftime("%m-%d %H:%M:%S")


def 建库(路径: str = db路径) -> sqlite3.Connection:
    con = sqlite3.connect(路径, check_same_thread=False)
    con.execute("PRAGMA journal_mode=WAL")
    con.execute("""CREATE TABLE IF NOT EXISTS 意图(
        对话id TEXT PRIMARY KEY,
        机器 TEXT,
        在做 TEXT DEFAULT '',
        计划 TEXT DEFAULT '',
        备注 TEXT DEFAULT '',
        心跳时刻 TEXT,
        心跳时戳 REAL,
        登记时刻 TEXT)""")
    con.execute("""CREATE TABLE IF NOT EXISTS 在飞分支(
        分支 TEXT PRIMARY KEY,
        提交 TEXT, 提交题 TEXT DEFAULT '', 时刻 TEXT,
        上报者 TEXT, 上报时戳 REAL, 已并入 INTEGER DEFAULT 0)""")
    con.execute("""CREATE TABLE IF NOT EXISTS 快照(
        键 TEXT PRIMARY KEY,
        内容 TEXT,
        上报时戳 REAL)""")
    # 308j（用户裁决乙+·服务器发号权威）：发号台账=append-only 流水（号唯一·原子取 max+1）；
    # 视野快照=各客户端「本机全部在飞行号」（含分支树内号）最近一次上报——发号基准=台账∪视野
    # ∪021 快照并集，服务端无 git 也拥有最全视野；同号出现在 ≥2 上报者视野=占号冲突。
    con.execute("""CREATE TABLE IF NOT EXISTS 发号台账(
        号 TEXT PRIMARY KEY,
        机器 TEXT, 对话id TEXT DEFAULT '', 描述 TEXT DEFAULT '',
        时刻 TEXT, 时戳 REAL)""")
    con.execute("""CREATE TABLE IF NOT EXISTS 视野快照(
        上报者 TEXT PRIMARY KEY,
        号们 TEXT, 时戳 REAL, 时刻 TEXT)""")
    台账.建任务表(con)      # 382：任务台账主表（服务端唯一权威）
    return con


连接 = 建库()


def 解析任务号们(文本: str) -> list:
    return 台账.解析任务号们(文本)   # 382：解析归台账模块（单一实现）


def 意图行转字典(行) -> dict:
    (对话id, 机器, 在做, 计划, 备注, 心跳时刻, 心跳时戳, 登记时刻) = 行
    陈旧秒 = time.time() - 心跳时戳
    return {"会话键": f"{机器}-{对话id}", "机器": 机器, "对话id": 对话id,
            "在做": 在做, "计划": 计划, "备注": 备注,
            "心跳时刻": 心跳时刻, "时戳": 心跳时戳, "失联": 陈旧秒 > 失联秒,
            "失联秒": int(陈旧秒), "登记时刻": 登记时刻}


自动清秒 = int(os.environ.get("CN_INTENT_PURGE_SEC", "7200"))   # 失联超此秒数=自动注销（会话被杀无 release 兜底）


def 全部意图() -> list:
    # 308h：失联超自动清秒的意图行惰性注销（30min 失联灰显→2h 清除）
    连接.execute("DELETE FROM 意图 WHERE 心跳时戳<?", (time.time() - 自动清秒,))
    # 318：收口惰性清——在做号任务表状态 ✅ → 直接删行（382 起查任务表实时真相，
    # 不再依赖 report_021 的任务字典快照——快照会 1h 过期，任务表永不过期）。
    完成号 = {r[0] for r in 连接.execute(
        "SELECT 号 FROM 任务 WHERE 状态=?", (台账.状态_完成,)).fetchall()}
    if 完成号:
        连接.executemany("DELETE FROM 意图 WHERE 在做=?",
                         [(号,) for 号 in 完成号])
    连接.commit()
    行们 = 连接.execute("SELECT * FROM 意图 ORDER BY 机器, 对话id").fetchall()
    return [意图行转字典(r) for r in 行们]


def 任务号声明图(意图们: list) -> dict:
    """任务号 → [声明它的会话键们]——在做+计划都算声明（计划=意图占用·打架预警面）。"""
    号主 = {}
    for 意图 in 意图们:
        号们 = ([意图["在做"]] if 意图["在做"] else []) + 解析任务号们(意图["计划"])
        for 号 in 号们:
            号主.setdefault(号, []).append(意图["会话键"])
    return 号主


def 冲突检测(意图们: list) -> list:
    """同任务号被 ≥2 会话声明 → 冲突条目（跨机打架预警·285 撞车面）。"""
    号主 = 任务号声明图(意图们)
    出 = []
    for 号 in sorted(号主, key=lambda x: (len(x), x)):
        主们 = 号主[号]
        if len(主们) > 1:
            出.append({"号": 号, "会话们": 主们})
    return 出


# ===== 308j 发号权威（382 起任务表为唯一权威·本节保留视野记账与历史流水兼容）=====

禁用号 = 台账.禁用号   # {353,354}（2026-10-05 用户令永久禁用·与台账模块同源）


def 号排序键(号: str):
    return 台账.号排序键(号)


def 收视野并取基准(上报者: str, 视野号们: list) -> set:
    """记视野快照·返回 发号基准并集=台账∪视野∪021 快照行号。"""
    亡秒 = 6 * 3600     # 视野快照保留 6h（陈旧视野不进基准防幽灵占号）
    连接.execute("DELETE FROM 视野快照 WHERE 时戳<?", (time.time() - 亡秒,))
    连接.execute(
        """INSERT INTO 视野快照(上报者,号们,时戳,时刻) VALUES(?,?,?,?)
           ON CONFLICT(上报者) DO UPDATE SET 号们=excluded.号们,
           时戳=excluded.时戳, 时刻=excluded.时刻""",
        (上报者, json.dumps(视野号们, ensure_ascii=False), time.time(), 时刻()))
    连接.commit()
    基准 = {r[0] for r in 连接.execute("SELECT 号 FROM 发号台账").fetchall()}
    for 行们 in 连接.execute("SELECT 号们 FROM 视野快照").fetchall():
        try:
            基准 |= set(json.loads(行们[0]))
        except (ValueError, TypeError):
            pass
    基准 |= 台账.已占号集(连接)   # 382：任务表全量号进发号基准（唯一权威）
    return 基准


def 号占冲突们() -> list:
    """同号出现在 ≥2 上报者视野 → 占号冲突（309 双占型·客户端各自树内立项互相不可见）。"""
    视野 = 连接.execute("SELECT 上报者,号们 FROM 视野快照 WHERE 时戳>?",
                       (time.time() - 6 * 3600,)).fetchall()
    号主 = {}
    for 上报者, 号们文本 in 视野:
        try:
            for 号 in json.loads(号们文本):
                号主.setdefault(str(号), []).append(上报者)
        except (ValueError, TypeError):
            pass
    return [{"号": 号, "上报者们": 主们} for 号, 主们 in sorted(号主.items(), key=号排序键)
            if len(主们) > 1]


def 交集标注(意图们: list, 在飞们: list) -> None:
    """就地补每意图/每分支的交叉标注：在飞分支 任务/号 vs 意图声明号 对账。
    371：补「在做主们」=该号活意图（不失联）的在做会话键——归属显示口径。
    原「上报者」=最后上报视野的机器，随上报者整体翻转非任务归属（371 实证：
    家机 358/359 曾整体标深度机）——UI/CLI 一律改按 在做主们 显示归属。"""
    声明号 = set(任务号声明图(意图们).keys())
    for 飞 in 在飞们:
        m = re.match(r"任务/(.+)$", 飞["分支"])
        飞["号"] = m.group(1) if m else ""
        飞["已登记意图"] = bool(飞["号"]) and 飞["号"] in 声明号
        飞["在做主们"] = sorted(i["会话键"] for i in 意图们
                              if not i.get("失联") and i.get("在做", "") == 飞["号"])


def 全部在飞() -> list:
    门槛 = time.time() - 快照保留秒
    行们 = 连接.execute(
        "SELECT 分支,提交,提交题,时刻,上报者,上报时戳,已并入 FROM 在飞分支 "
        "WHERE 上报时戳>? ORDER BY 分支", (门槛,)).fetchall()
    return [{"分支": r[0], "提交": r[1], "提交题": r[2], "时刻": r[3],
             "上报者": r[4], "上报时戳": r[5], "已并入": bool(r[6])} for r in 行们]


def 取快照(键: str):
    行 = 连接.execute("SELECT 内容,上报时戳 FROM 快照 WHERE 键=?", (键,)).fetchone()
    if not 行 or 行[1] < time.time() - 快照保留秒:
        return None
    try:
        return json.loads(行[0])
    except (ValueError, TypeError):
        return None


def 聚合视图() -> dict:
    意图们 = 全部意图()
    在飞们 = 全部在飞()
    交集标注(意图们, 在飞们)
    # 382.2（用户令「远端在飞分支必须实时体现真实 git 分支」）：合成行逻辑废除——
    # 在飞分支=TX_01 gitcron 每 3 分钟 ls-remote 的真实投影（board_flights_cron.py），
    # 分支没推就不显示（真相）；331 旧「意图独有号合成未推行」掩盖真实状态·删。
    任务们 = 台账.全部任务(连接)   # 382：任务台账全量（服务端唯一权威·含就绪判定）
    # 308d 兼容：队列×在飞对撞——在飞分支上报是实时的，待办号出现在在飞区=疑似他机认领中。
    在飞号 = {f["号"] for f in 在飞们 if f.get("号")}
    for 行 in 任务们:
        行["疑似认领"] = 行["号"] in 在飞号
    return {"意图们": 意图们, "在飞分支们": 在飞们,
            "任务们": 任务们,                        # 382 新字段：全量台账（UI 主数据源）
            "任务字典": 台账.任务字典图(连接),        # 兼容旧客户端（实时版·永不过期）
            "快照021": [行 for 行 in 任务们 if 行["状态"] == 台账.状态_待办][:20],  # 兼容旧 UI 语义
            "冲突们": 冲突检测(意图们),
            "号占冲突们": 号占冲突们(),
            "失联秒": 失联秒, "时刻": 时刻()}


class 处理器(BaseHTTPRequestHandler):
    """照 queue_service 口径：GET 只读放行；POST 须 Bearer 令牌（令牌未配置则只允许本机回环）。"""

    def log_message(self, fmt, *args):   # 静默访问日志（systemd journal 只留业务行）
        pass

    def _回JSON(self, 码: int, 对象: dict):
        体 = json.dumps(对象, ensure_ascii=False).encode("utf-8")
        self.send_response(码)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(体)))
        self.end_headers()
        self.wfile.write(体)

    def _读JSON体(self) -> dict:
        长度 = int(self.headers.get("Content-Length", "0") or "0")
        if 长度 <= 0 or 长度 > 1 << 20:
            return {}
        try:
            return json.loads(self.rfile.read(长度).decode("utf-8"))
        except (ValueError, UnicodeDecodeError):
            return {}

    def _令牌合法(self) -> bool:
        if not 令牌:
            return self.client_address[0] in ("127.0.0.1", "::1")
        头 = self.headers.get("Authorization", "")
        return 头 == f"Bearer {令牌}"

    静态目录 = Path(__file__).resolve().parent / "board_www"
    静态类型 = {".html": "text/html; charset=utf-8", ".css": "text/css; charset=utf-8",
                ".js": "text/javascript; charset=utf-8", ".svg": "image/svg+xml"}

    def _回静态(self, 文件名: str):
        """board_www/ 静态三件（382·前端从 Python 字符串解放为独立文件）。"""
        文件 = (self.静态目录 / 文件名).resolve()
        if 文件.parent != self.静态目录 or not 文件.is_file():   # 防穿越+只许白名单目录
            return self._回JSON(404, {"错误": "未知路径"})
        体 = 文件.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", self.静态类型.get(文件.suffix, "application/octet-stream"))
        self.send_header("Content-Length", str(len(体)))
        self.send_header("Cache-Control", "no-cache")   # 看板要即改即见
        self.end_headers()
        self.wfile.write(体)

    def do_GET(self):
        路径 = urlparse(self.path).path
        if 路径 in ("/", "/board"):
            return self._回静态("index.html")
        if 路径 in ("/board.css", "/board.js", "/tokens.css"):
            return self._回静态(路径.lstrip("/"))
        if 路径 == "/api/intents":
            with 写锁:
                return self._回JSON(200, {"意图们": 全部意图(), "失联秒": 失联秒})
        if 路径 == "/api/board":
            with 写锁:
                return self._回JSON(200, 聚合视图())
        if 路径 == "/api/tasks":
            with 写锁:
                return self._回JSON(200, {"任务们": 台账.全部任务(连接),
                                          "下一号": 台账.下一个号(台账.已占号集(连接)),
                                          "时刻": 时刻()})
        m = re.fullmatch(r"/api/task/([0-9]+[a-z]?)", 路径)
        if m:
            with 写锁:
                任务 = 台账.单任务(连接, m.group(1))
                return self._回JSON(200 if 任务 else 404,
                                    {"任务": 任务} if 任务 else {"错误": f"任务 {m.group(1)} 不存在"})
        if 路径 == "/api/numbers":
            with 写锁:
                流水 = [{"号": r[0], "机器": r[1], "对话id": r[2], "描述": r[3], "时刻": r[4]}
                       for r in 连接.execute(
                           "SELECT 号,机器,对话id,描述,时刻 FROM 发号台账 ORDER BY 时戳").fetchall()]
                return self._回JSON(200, {"台账": 流水, "下一个": 台账.下一个号(
                    台账.已占号集(连接)), "号占冲突们": 号占冲突们(), "时刻": 时刻()})
        return self._回JSON(404, {"错误": "未知路径"})

    def do_POST(self):
        if not self._令牌合法():
            return self._回JSON(401, {"错误": "令牌缺失或不符"})
        路径 = urlparse(self.path).path
        体 = self._读JSON体()
        if 路径 == "/api/intent":
            return self._登记意图(体)
        if 路径 == "/api/intent_release":
            return self._注销意图(体)
        if 路径 == "/api/report_flights":
            return self._收在飞上报(体)
        if 路径 == "/api/report_021":
            return self._收快照上报(体)
        if 路径 == "/api/task_create":
            with 写锁:
                码, 响应 = 台账.创建任务(连接, 体)
                return self._回JSON(码, 响应)
        if 路径 == "/api/task_update":
            with 写锁:
                码, 响应 = 台账.更新任务(连接, 体)
                return self._回JSON(码, 响应)
        if 路径 == "/api/claim_number":
            return self._收发号(体)
        return self._回JSON(404, {"错误": "未知路径"})

    def _登记意图(self, 体: dict):
        机器 = str(体.get("机器", "")).strip()
        对话id = str(体.get("对话id", "")).strip()
        if not 机器 or not 对话id:
            return self._回JSON(400, {"错误": "机器与对话id 必填"})
        在做 = str(体.get("在做", "")).strip()
        计划 = ",".join(解析任务号们(str(体.get("计划", ""))))
        备注 = str(体.get("备注", "")).strip()[:200]
        with 写锁:
            连接.execute(
                """INSERT INTO 意图(对话id,机器,在做,计划,备注,心跳时刻,心跳时戳,登记时刻)
                   VALUES(?,?,?,?,?,?,?,?)
                   ON CONFLICT(对话id) DO UPDATE SET 机器=excluded.机器, 在做=excluded.在做,
                   计划=excluded.计划, 备注=excluded.备注, 心跳时刻=excluded.心跳时刻,
                   心跳时戳=excluded.心跳时戳""",
                (对话id, 机器, 在做, 计划, 备注, 时刻(), time.time(), 时刻()))
            连接.commit()
        return self._回JSON(200, {"好": True, "会话键": f"{机器}-{对话id}"})

    def _注销意图(self, 体: dict):
        # 308h：两种注销口径——按会话（机器+对话id·会话收工）或按在做任务号
        # （任务收口联动·integrate 调用——挂在该任务上的全部会话意图清除）
        任务号 = str(体.get("在做", "")).strip()
        with 写锁:
            if 任务号:
                连接.execute("DELETE FROM 意图 WHERE 在做=?", (任务号,))
            else:
                机器 = str(体.get("机器", "")).strip()
                对话id = str(体.get("对话id", "")).strip()
                if not 机器 or not 对话id:
                    return self._回JSON(400, {"错误": "须 机器+对话id 或 在做 任务号"})
                连接.execute("DELETE FROM 意图 WHERE 对话id=?", (对话id,))
            连接.commit()
        return self._回JSON(200, {"好": True})

    def _收在飞上报(self, 体: dict):
        分支们 = 体.get("分支们")
        if not isinstance(分支们, list) or len(分支们) > 200:
            return self._回JSON(400, {"错误": "分支们 须为列表（≤200）"})
        上报者 = str(体.get("上报者", "")).strip()[:60]
        现 = time.time()
        with 写锁:
            # 308h：全量替换——上报者视图=远端实时真相，本次没报的旧行=远端已删
            # （原 upsert 只增不删·已收口分支残影挂满 1h 才过期=用户质询面）
            连接.execute("DELETE FROM 在飞分支")
            for 项 in 分支们:
                if not isinstance(项, dict):
                    continue
                名 = str(项.get("分支", "")).strip()
                if not 名.startswith("任务/"):
                    continue
                连接.execute(
                    """INSERT INTO 在飞分支(分支,提交,提交题,时刻,上报者,上报时戳,已并入)
                       VALUES(?,?,?,?,?,?,?)
                       ON CONFLICT(分支) DO UPDATE SET 提交=excluded.提交,
                       提交题=excluded.提交题, 时刻=excluded.时刻,
                       上报者=excluded.上报者, 上报时戳=excluded.上报时戳,
                       已并入=excluded.已并入""",
                    (名[:120], str(项.get("提交", ""))[:12],
                     str(项.get("提交题", ""))[:160], str(项.get("时刻", ""))[:20],
                     上报者, 现, 1 if 项.get("已并入") else 0))
            连接.execute("DELETE FROM 在飞分支 WHERE 上报时戳<?", (现 - 快照保留秒,))
            连接.commit()
        return self._回JSON(200, {"好": True})

    def _收快照上报(self, 体: dict):
        行们 = 体.get("行们")
        if not isinstance(行们, list) or len(行们) > 400:
            return self._回JSON(400, {"错误": "行们 须为列表（≤400）"})
        任务字典 = 体.get("任务字典")
        if 任务字典 is not None and (not isinstance(任务字典, dict)
                                     or len(任务字典) > 1000):
            return self._回JSON(400, {"错误": "任务字典 须为对象（≤1000 号）"})
        with 写锁:
            连接.execute(
                """INSERT INTO 快照(键,内容,上报时戳) VALUES('021',?,?)
                   ON CONFLICT(键) DO UPDATE SET 内容=excluded.内容, 上报时戳=excluded.上报时戳""",
                (json.dumps(行们, ensure_ascii=False), time.time()))
            if 任务字典 is not None:
                # 329：字典合并（并集·同键取本次上报）而非全量覆盖——各机 021 新旧不一，
                # 旧视野覆盖会把新标题抹掉（290/309 标题消失实证·用户报 BUG）。过期惰性清：
                # 快照保留秒 到期整体失效（取快照 已管），合并只影响存活窗口内的多机并集。
                旧字典 = 取快照("任务字典") or {}
                合并 = dict(旧字典)
                合并.update(任务字典)
                连接.execute(
                    """INSERT INTO 快照(键,内容,上报时戳) VALUES('任务字典',?,?)
                       ON CONFLICT(键) DO UPDATE SET 内容=excluded.内容, 上报时戳=excluded.上报时戳""",
                    (json.dumps(合并, ensure_ascii=False), time.time()))
            连接.commit()
        return self._回JSON(200, {"好": True})

    def _收发号(self, 体: dict):
        """308j 发号权威（乙+）——{机器, 对话id?, 上报者?, 视野号们, 描述?, 请求号?}
        请求号空=发新号：基准并集（台账∪各机视野∪021 快照）取 max+1·写台账·原子无竞态；
        请求号非空=核对模式：台账已发或其他机视野占用（309 双占型）→409 拒·否则放行。"""
        机器 = str(体.get("机器", "")).strip()[:60]
        if not 机器:
            return self._回JSON(400, {"错误": "机器 必填"})
        对话id = str(体.get("对话id", "")).strip()[:40]
        上报者 = (str(体.get("上报者", "")).strip() or 机器)[:60]
        原始视野 = 体.get("视野号们")
        视野号们 = [str(n) for n in 原始视野
                    if isinstance(n, (str, int)) and re.fullmatch(r"[0-9]+[a-z]?", str(n))] \
            if isinstance(原始视野, list) else []
        描述 = str(体.get("描述", "")).strip()[:200]
        请求号 = str(体.get("请求号", "")).strip()
        仅视野 = bool(体.get("only_view"))
        with 写锁:
            基准 = 收视野并取基准(上报者, 视野号们)
            if 仅视野:
                # 316：intent.py claim/show 的视野上报通道——只记账不发号
                # （双机视野齐→号占冲突自动亮·治「撞号看板不拦」第一洞）
                return self._回JSON(200, {"好": True, "仅视野": True})
            if 请求号:
                行 = 连接.execute("SELECT 机器,对话id,时刻 FROM 发号台账 WHERE 号=?",
                                  (请求号,)).fetchone()
                if 行:
                    return self._回JSON(409, {
                        "错误": f"号 {请求号} 已由服务端发给 {行[0]}-{行[1]}（{行[2]}）"
                                f"——接棒用远端已存在分支·新事用新号", "已发": True})
                撞视野 = [r[0] for r in 连接.execute(
                    "SELECT 上报者 FROM 视野快照 WHERE 时戳>? AND 号们 LIKE ?",
                    (time.time() - 6 * 3600, f'%"{请求号}"%')).fetchall() if r[0] != 上报者]
                if 撞视野:
                    return self._回JSON(409, {
                        "错误": f"号 {请求号} 出现在他机视野（{', '.join(撞视野)}）"
                                f"——树内占号冲突·先与对方/主表核对再取号", "已发": True,
                        "撞视野": 撞视野})
                return self._回JSON(200, {"好": True, "已发": False})
            号 = 台账.下一个号(基准)
            连接.execute(
                "INSERT INTO 发号台账(号,机器,对话id,描述,时刻,时戳) VALUES(?,?,?,?,?,?)",
                (号, 机器, 对话id, 描述, 时刻(), time.time()))
            连接.commit()
        return self._回JSON(200, {"好": True, "号": 号})

def 自检() -> int:
    """端到端正反两态（866 教训：自证必须含对目标真实拦截的正反两态）。"""
    全局连接 = globals()["连接"]
    globals()["连接"] = 建库(":memory:")
    globals()["失联秒"] = 60
    实例 = ThreadingHTTPServer(("127.0.0.1", 0), 处理器)
    端 = 实例.server_address[1]
    线程 = threading.Thread(target=实例.serve_forever, daemon=True)
    线程.start()
    基址 = f"http://127.0.0.1:{端}"
    失败们 = []

    def 签(名: str, 条件: bool):
        print(("  ✓ " if 条件 else "  ✗ ") + 名)
        if not 条件:
            失败们.append(名)

    import urllib.request
    def 调(方法: str, 路径: str, 体=None, 带令牌=True):
        请求 = urllib.request.Request(基址 + 路径, method=方法,
            data=json.dumps(体, ensure_ascii=False).encode("utf-8") if 体 is not None else None)
        if 带令牌:
            请求.add_header("Authorization", "Bearer test-token-308a")
        globals()["令牌"] = "test-token-308a"
        try:
            with urllib.request.urlopen(请求, timeout=5) as r:
                return r.status, json.loads(r.read().decode("utf-8"))
        except urllib.error.HTTPError as e:
            try:    # 409 等错误态的 body 也带断言字段（已发/撞视野）——不能丢
                return e.code, json.loads(e.read().decode("utf-8"))
            except (ValueError, UnicodeDecodeError):
                return e.code, {}

    try:
        码, _ = 调("POST", "/api/intent", {"机器": "测机"}, 带令牌=False)
        签("无令牌 POST 被拒 401（反态）", 码 == 401)
        码, r = 调("POST", "/api/intent", {"机器": "深度机", "对话id": "a1",
                  "在做": "308a", "计划": "110, 276", "备注": "测试"})
        签("登记意图 200", 码 == 200)
        码, r = 调("POST", "/api/intent", {"机器": "家机", "对话id": "b2", "在做": "285"})
        码, r = 调("POST", "/api/intent", {"机器": "家机", "对话id": "b3", "在做": "308a"})
        码, r = 调("GET", "/api/board")
        签("冲突检出：308a 双会话声明", any(c["号"] == "308a" for c in r.get("冲突们", [])))
        签("计划号解析容错（'110, 276'→两项）",
           any(i["计划"] == "110,276" for i in r["意图们"]))
        码, r = 调("POST", "/api/report_flights", {"上报者": "深度机", "分支们": [
            {"分支": "任务/308a", "提交": "fca326a9", "提交题": "立项", "时刻": "10-08 12:00:00"},
            {"分支": "垃圾/xx", "提交": "z"}]})
        码, r = 调("GET", "/api/board")
        支集 = {f["分支"] for f in r["在飞分支们"]}
        签("在飞上报只收 任务/ 前缀", "任务/308a" in 支集 and "垃圾/xx" not in 支集)
        f = r["在飞分支们"][0]
        签("交集标注：308a 在飞=已登记意图", f["已登记意图"] is True)
        签("371 在飞归属=在做主们（双活会话）",
           f["在做主们"] == ["家机-b3", "深度机-a1"])
        连接2 = globals()["连接"]
        连接2.execute("UPDATE 意图 SET 心跳时戳=? WHERE 对话id='a1'", (time.time() - 999,))
        连接2.commit()
        码, r = 调("GET", "/api/board")
        深度行 = next(i for i in r["意图们"] if i["对话id"] == "a1")
        签("心跳超时→失联态", 深度行["失联"] is True)
        f308a = next(f for f in r["在飞分支们"] if f["分支"] == "任务/308a")
        签("371 失联会话剔出在做主们（活会话保留）",
           f308a["在做主们"] == ["家机-b3"])
        码, r = 调("POST", "/api/intent_release", {"机器": "家机", "对话id": "b3"})
        码, r = 调("GET", "/api/board")
        签("注销后冲突消除", not any(c["号"] == "308a" for c in r.get("冲突们", [])))
        f308a = next(f for f in r["在飞分支们"] if f["分支"] == "任务/308a")
        # 已登记意图=声明口径含失联行（308h 2h 才惰性清）——黄标归属只看在做主们
        签("371 全体失联/注销→在做主们空=未登记黄标口径",
           f308a["在做主们"] == [])
        码, r = 调("POST", "/api/report_021", {"行们": [
            {"号": "002", "标题": "甲", "优先级": "P0"},
            {"号": "110", "标题": "乙", "优先级": "P1"}],
            "任务字典": {"110": {"标题": "整64 FFI arm64", "状态": "⬜", "优先级": "P1"}}})
        签("382 旧 report_021 通道兼容不炸（退役·仅收不生效）", 码 == 200)
        调("POST", "/api/report_flights", {"上报者": "深度机", "分支们": [
            {"分支": "任务/211", "提交": "def456", "时刻": "", "已并入": True}]})
        码, r = 调("GET", "/api/board")
        飞 = {f["分支"]: f for f in r["在飞分支们"]}
        签("308e 已并入字段透传", 飞["任务/211"]["已并入"] is True)
        码, r = 调("POST", "/api/intent", {"对话id": "无机器"})
        签("缺机器参数 400", 码 == 400)
        # 308h 生命周期三用例
        调("POST", "/api/report_flights", {"上报者": "甲", "分支们": [
            {"分支": "任务/999", "提交": "old0001", "时刻": ""}]})
        调("POST", "/api/report_flights", {"上报者": "乙", "分支们": [
            {"分支": "任务/888", "提交": "new0001", "时刻": ""}]})
        码, r = 调("GET", "/api/board")
        签("308h 在飞上报全量替换（旧 999 消失）",
           all(f["分支"] != "任务/999" for f in r["在飞分支们"]))
        调("POST", "/api/intent", {"机器": "测机", "对话id": "h1", "在做": "777"})
        码, r = 调("POST", "/api/intent_release", {"在做": "777"})
        码, r = 调("GET", "/api/board")
        签("308h 按在做任务号批量注销",
           all(i["在做"] != "777" for i in r["意图们"]))
        连接3 = globals()["连接"]
        连接3.execute("INSERT INTO 意图(对话id,机器,在做,计划,备注,心跳时刻,心跳时戳,登记时刻) "
                      "VALUES('old','古机','x','','','old',?,'old')", (time.time() - 99999,))
        连接3.commit()
        码, r = 调("GET", "/api/board")
        签("308h 失联超 2h 惰性自动注销",
           all(i["对话id"] != "old" for i in r["意图们"]))
        码, r = 调("POST", "/api/intent", {"机器": "旧机名", "对话id": "c9",
                  "在做": "110"})
        码, r = 调("POST", "/api/intent", {"机器": "新机名", "对话id": "c9",
                  "在做": "110", "计划": "276"})
        码, r = 调("GET", "/api/board")
        同话 = [i for i in r["意图们"] if i["对话id"] == "c9"]
        签("同对话id 改机名=单行搬家（308b 根治面）",
           len(同话) == 1 and 同话[0]["机器"] == "新机名"
           and 同话[0]["会话键"] == "新机名-c9")
        with urllib.request.urlopen(基址 + "/", timeout=5) as resp:
            页 = resp.read().decode("utf-8")
        签("看板页 200 且静态服务（board_www/index.html）",
           resp.status == 200 and "任务看板" in 页)
        签("382 页面三 tab+详情面板+新建表单齐备",
           all(k in 页 for k in ("当前在飞", "远端在飞分支", "可认领任务", "详情面板", "新建任务")))
        with urllib.request.urlopen(基址 + "/board.css", timeout=5) as resp:
            签("静态 board.css 200", resp.status == 200)
        # 308j 发号权威五用例（乙+：原子递增/视野并集/禁用号跳/核对 409/占号冲突）
        码, r = 调("POST", "/api/claim_number", {"机器": "甲机", "对话id": "n1",
                  "上报者": "甲机-主树", "视野号们": ["300", "307", "308i"], "描述": "首号"})
        签("发号：视野并集 max(308i)→发 308j", 码 == 200 and r.get("号") == "308j")
        码, r = 调("POST", "/api/claim_number", {"机器": "乙机", "对话id": "n2",
                  "上报者": "乙机-主树", "视野号们": []})
        签("发号：台账连续递增 308j→308k", 码 == 200 and r.get("号") == "308k")
        码, r = 调("POST", "/api/claim_number", {"机器": "丙机", "上报者": "丙机-主树",
                  "视野号们": ["352"]})
        签("发号：禁用号 353/354 跳过→355", 码 == 200 and r.get("号") == "355")
        码, r = 调("POST", "/api/claim_number", {"机器": "丁机", "上报者": "丁机-主树",
                  "请求号": "308j"})
        签("核对：台账已发号拒 409", 码 == 409 and r.get("已发") is True)
        调("POST", "/api/claim_number", {"机器": "戊机", "上报者": "戊机-主树",
           "视野号们": ["309"]})
        码, r = 调("POST", "/api/claim_number", {"机器": "己机", "上报者": "己机-主树",
                  "请求号": "309", "视野号们": ["309"]})
        签("核对：他机视野占号拒 409（309 双占型）",
           码 == 409 and any("戊机" in x for x in (r.get("撞视野") or [])))
        码, r = 调("GET", "/api/board")
        签("号占冲突上板：309 双视野", any(c["号"] == "309" for c in r.get("号占冲突们", [])))
        码, r = 调("POST", "/api/claim_number", {"机器": "庚机", "上报者": "庚机-主树",
                  "请求号": "400"})
        签("核对：全新号放行", 码 == 200 and r.get("已发") is False)
        台账前 = 调("GET", "/api/numbers")[1]["台账"]
        调("POST", "/api/claim_number", {"机器": "辛机", "上报者": "辛机-主树",
           "视野号们": ["309"], "only_view": True})
        台账后 = 调("GET", "/api/numbers")[1]["台账"]
        签("316 only_view：只记视野不发号", len(台账后) == len(台账前))
        视野撞 = 调("POST", "/api/claim_number", {"机器": "壬机", "上报者": "壬机-主树",
                    "请求号": "309"})
        签("316：辛机视野上报后 309 核对 409（撞号拦截链实锤）",
           视野撞[0] == 409 and 视野撞[1].get("已发") is True)
        调("POST", "/api/intent", {"机器": "测机", "对话id": "k1", "在做": "110"})
        码, r = 调("POST", "/api/task_create", {"标题": "清理面任务", "请求号": "110"})
        码, r = 调("POST", "/api/task_update", {"号": "110", "状态": "✅",
                                                "收口sha": "ab12cd34ef56"})
        码, r = 调("GET", "/api/board")
        签("318 服务端收口惰性清：任务表✅→在做#110 行删",
           all(i["在做"] != "110" for i in r["意图们"]))
        # —— 382 任务台账：立项/发号/流转/就绪（服务端唯一权威正反两态）——
        码, r = 调("POST", "/api/task_create", {"标题": "首任务"})
        首 = r.get("号", "")
        签("382 立项：服务端发号 201·初态 ⬜", 码 == 201 and r["任务"]["状态"] == "⬜")
        码, r = 调("POST", "/api/task_create", {"标题": "二任务", "前置": 首, "优先级": "P1"})
        二 = r.get("号", "")
        签("382 立项：号连续递增+前置登记",
           码 == 201 and 台账.号排序键(二) > 台账.号排序键(首)
           and r["任务"]["前置"] == 首)
        码, r = 调("POST", "/api/task_create", {"标题": ""})
        签("382 立项：缺标题 400（反态）", 码 == 400)
        码, r = 调("POST", "/api/task_create", {"标题": "撞号", "请求号": 首})
        签("382 立项：指定已占号 409", 码 == 409)
        码, r = 调("POST", "/api/task_create", {"标题": "禁用", "请求号": "353"})
        签("382 立项：禁用号 353 拒 403", 码 == 403)
        码, r = 调("POST", "/api/task_create", {"标题": "坏号", "请求号": "38-坏"})
        签("382 立项：请求号格式非法 400", 码 == 400)
        码, r = 调("POST", "/api/task_update", {"号": 首, "状态": "🏃"})
        签("382 认领：⬜→🏃 分支自动合成 任务/<号>",
           码 == 200 and r["任务"]["分支"] == f"任务/{首}" and r["任务"]["状态"] == "🏃")
        码, r = 调("POST", "/api/task_update", {"号": 二, "状态": "✅"})
        签("382 完成：无 sha 拒 400（✅⇔sha 铁律服务端化）", 码 == 400)
        码, r = 调("POST", "/api/task_update", {"号": 二, "状态": "✅", "收口sha": "deadbeef01"})
        签("382 完成：带 sha 200+sha 落账", 码 == 200 and r["任务"]["收口sha"] == "deadbeef01")
        码, r = 调("POST", "/api/task_update", {"号": 二, "状态": "⏸"})
        签("382 终态：✅ 不可再改 400", 码 == 400)
        码, r = 调("POST", "/api/task_update", {"号": "888888", "状态": "⏸"})
        签("382 更新：无此号 404", 码 == 404)
        码, r = 调("POST", "/api/task_update", {"号": 首, "状态": "✅", "收口sha": "cafebabef00d"})
        码, r = 调("POST", "/api/task_create", {"标题": "三任务", "前置": 首})
        三 = r.get("号", "")
        码, r = 调("GET", "/api/tasks")
        图 = {t["号"]: t for t in r["任务们"]}
        签("382 就绪判定：前置✅ 后新任务就绪·未完前置不就绪",
           图[三]["就绪"] is True and 图[首]["就绪"] is False)
        码, r = 调("GET", f"/api/task/{三}")
        签("382 单任务查询", 码 == 200 and r["任务"]["号"] == 三)
        码, r = 调("GET", "/api/task/888888")
        签("382 单任务查询：无此号 404", 码 == 404)
        调("POST", "/api/report_flights", {"上报者": "测机", "分支们": [
            {"分支": f"任务/{三}", "提交": "abc123", "时刻": "", "已并入": False}]})
        码, r = 调("GET", "/api/board")
        图 = {t["号"]: t for t in r["任务们"]}
        签("308d 队列×在飞对撞：在飞区号=疑似认领", 图[三]["疑似认领"] is True)
        码, r = 调("GET", "/api/board")
        字 = r.get("任务字典") or {}
        签("382 任务字典实时生成（兼容字段·永不过期）",
           字.get(三, {}).get("标题") == "三任务" and 字.get(二, {}).get("状态") == "✅")
        # —— 382 迁移导入（幂等·归档强制 ✅）——
        迁 = 台账.迁移导入(globals()["连接"],
                           "| 901 | 迁移甲 | ⬜ | — | P1 | 普通行 |\n"
                           "| 902 | 迁移乙 | ✅ ab12cd34 | 901 | P2 | 分支=任务/902 |\n", [])
        签("382 迁移导入：主表 2 行入库", 迁["导入"] == 2)
        迁 = 台账.迁移导入(globals()["连接"],
                           "| 901 | 重复 | ⬜ | — | P1 | 跳过面 |\n",
                           ["| 904 | 归档丁 | ✅ feed5678 | — | P3 | x |"])
        签("382 迁移导入：幂等跳过+归档行入库", 迁["导入"] == 1 and 迁["跳过"] == 1)
        任务904 = 台账.单任务(globals()["连接"], "904")
        签("382 迁移导入：归档行 ✅+sha 原样",
           任务904["状态"] == "✅" and 任务904["收口sha"] == "feed5678")
        码, r = 调("POST", "/api/task_create", {"标题": "迁移后发号", "请求号": "902"})
        签("382 迁移号进已占基准：指定 902 拒 409", 码 == 409)
        调("POST", "/api/intent", {"机器": "测机", "对话id": "u1", "在做": "887"})
        码, r = 调("GET", "/api/board")
        飞行 = {f["分支"]: f for f in r["在飞分支们"]}
        签("382.2 合成行废除：分支未推不在在飞分支区（真实分支为准）",
           "任务/887" not in 飞行)
        调("POST", "/api/report_flights", {"上报者": "测机", "分支们": [
            {"分支": "任务/887", "提交": "abc123", "时刻": "", "已并入": False}]})
        码, r = 调("GET", "/api/board")
        飞行 = {f["分支"]: f for f in r["在飞分支们"]}
        签("382.2 分支真实推后出现",
           "任务/887" in 飞行 and not 飞行["任务/887"].get("未推"))
    finally:
        实例.shutdown()
        globals()["连接"] = 全局连接
    print(f"自检 {len(失败们)} 失败" if 失败们 else "自检全绿")
    return 1 if 失败们 else 0


def main() -> int:
    if "--selftest" in sys.argv:
        return 自检()
    服务 = ThreadingHTTPServer(("0.0.0.0", 端口), 处理器)
    print(f"board_service 监听 :{端口}（失联阈值 {失联秒}s·db={db路径}）", flush=True)
    服务.serve_forever()
    return 0


if __name__ == "__main__":
    sys.exit(main())
