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
# 384 文档资源（交接/教训/规范覆盖·文档上服务器二期·board_docs.py 模块）：
#   POST /api/handoff_add     {机器,条目,来源?}              交接追加（两行制）
#   GET  /api/handoff         ?machine=X&limit=N             交接查询（倒序·machine 空=三机混流）
#   POST /api/lesson_add      {标题,正文?,权重?,标注?}        教训登记（同标题幂等·权重缺省从标题提取）
#   GET  /api/lessons         ?high_weight=8 | ?index=1      高权重全文区 / 一行索引面
#   POST /api/coverage_update {单元ID,标题?,正例?,边界例?,负例?}  覆盖单元行 upsert（整列替换）
#   POST /api/coverage_exempt_add {用例,理由}                 豁免登记（逗号批量·理由必填）
#   POST /api/coverage_import {单元们:[…],豁免们:[…]}          迁移批量幂等导入
#   GET  /api/coverage        全量 {单元们,豁免们}（check_spec_coverage 门禁源·客户端落缓存降级）
# 403 裁决资源（网页裁决系统·board_adjudication.py 模块）：
#   POST /api/adjudication_add {号,标题,优先级?,讲解?,选项们:[{键,描述,推荐?}],覆盖?}
#                              裁决项上传/修订（AI 侧·已裁决项默认 409 锁定）
#   POST /api/adjudicate      {裁决们:[{号,选项键,裁决人?,意见?,联动备注?}]}
#                              裁决提交（单项/批量·档案落库+台账备注自动联动）
#   GET  /api/adjudications   全量裁决项（附裁决记录·待裁决在前）
# 430 登录鉴权（2026-10-11 用户令「必须只有作者才能裁决」·网页裁决系统安全收口）：
#   POST /api/login   {口令}   网页管理员登录——口令与 CN_BOARD_TOKEN 同源（单一权威），
#                              正确则签发 HttpOnly 会话 cookie（SameSite=Lax·7 天，
#                              SQLite 持久·重启不掉登录态）；错口令 401+小睡防爆破
#   POST /api/logout           注销当前会话+清 cookie
#   GET  /api/me               {已登录:bool}——前端恢复登录态 UI
#   写通道鉴权=Bearer 令牌（CLI/脚本·queue_client.json 同源）∨ 登录会话 cookie（网页）。
#   ★令牌未配置=全部写操作拒绝（fail-closed）——根治旧「未配置则回环放行」：
#     Caddy 反代下 client_address 恒为 127.0.0.1，来源判断在反代场景完全失真，
#     一旦 systemd 环境丢失令牌即公网裸奔（430 立案根因）。
# 过期：心跳断 CN_INTENT_STALE_SEC（默认 1800s）→ 失联态（行保留·UI 灰显+失联徽章）。
# 心跳时戳=服务端收到时刻（911 教训：免疫各机时钟漂移）。任务表=持久落盘永不过期。
# 部署：/etc/systemd/system/cn-board.service（Environment= CN_BOARD_PORT/CN_BOARD_TOKEN/CN_BOARD_DB）
#   + board_www/ 静态目录随 board_service.py 同目录部署（deploy_board.sh 一键）。
# 自检：python3 board_service.py --selftest（内存 db+随机端口·意图/冲突/发号/任务
#   立项流转/迁移导入全链+401 反态）——实现在 board_selftest.py（405 拆分）。
# 迁移：python3 board_tasks.py --db <路径> --migrate --021 <主表.md> [--归档 <归档.md>]*

import json
import os
import re
import secrets
import sqlite3
import sys
import threading
import time
from datetime import datetime
from http.cookies import SimpleCookie
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse, unquote

sys.path.insert(0, str(Path(__file__).resolve().parent))
import board_tasks as 台账   # 任务台账模块（382·发号/CRUD/状态机/迁移）
import board_docs as 文档    # 交接/教训/规范覆盖模块（384·文档上服务器二期）
import board_adjudication as 裁决  # 待裁决项模块（403·网页裁决系统）

# ===== 可调常量 =====
端口 = int(os.environ.get("CN_BOARD_PORT", "8301"))
令牌 = os.environ.get("CN_BOARD_TOKEN", "")
db路径 = os.environ.get("CN_BOARD_DB",
                        str(os.path.dirname(os.path.abspath(__file__)) + "/board_state.db"))
失联秒 = int(os.environ.get("CN_INTENT_STALE_SEC", "900"))    # 心跳断此秒数=会话静默（15min·AI 活跃时路过续约密·打断后及时体现；行保留·任务态不受影响）
自动清秒 = int(os.environ.get("CN_INTENT_PURGE_SEC", "7200"))  # 静默超此秒数=自动注销（会话被杀无 release 兜底）
快照保留秒 = 3600                                             # 在飞分支/021 快照超过此秒数不再展示（陈旧数据防误导）
在飞保留条数 = 60                                             # 在飞分支表裁剪上限
会话秒 = 7 * 24 * 3600                                        # 登录会话有效期（430·7 天后须重登）
会话cookie = "cn_board_session"                               # 登录会话 cookie 名（HttpOnly·JS 不可读）

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
    con.execute("""CREATE TABLE IF NOT EXISTS 会话(
        会话id TEXT PRIMARY KEY,
        时戳 REAL)""")
    台账.建任务表(con)      # 382：任务台账主表（服务端唯一权威）
    文档.建表(con)          # 384：交接/教训/规范覆盖（文档上服务器二期）
    裁决.建表(con)          # 403：裁决项/裁决记录（网页裁决系统）
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
    """GET 只读放行；POST 三类：login/logout 豁免鉴权，其余须 Bearer 令牌（CLI/脚本）
    或登录会话 cookie（网页·430）；令牌未配置=全拒 fail-closed（不再按来源回环放行）。"""

    def log_message(self, fmt, *args):   # 静默访问日志（systemd journal 只留业务行）
        pass

    def _回JSON(self, 码: int, 对象: dict, 头们: list = None):
        体 = json.dumps(对象, ensure_ascii=False).encode("utf-8")
        self.send_response(码)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        for k, v in (头们 or []):
            self.send_header(k, v)
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

    def _取会话id(self) -> str:
        蛋糕 = SimpleCookie(self.headers.get("Cookie", ""))
        项 = 蛋糕.get(会话cookie)
        return 项.value if 项 else ""

    def _会话合法(self) -> bool:
        sid = self._取会话id()
        if not sid:
            return False
        with 写锁:
            行 = 连接.execute("SELECT 时戳 FROM 会话 WHERE 会话id=?", (sid,)).fetchone()
        return bool(行) and 行[0] >= time.time() - 会话秒

    def _已鉴权(self) -> bool:
        """写通道鉴权（430）：Bearer 令牌（CLI/脚本）∨ 登录会话 cookie（网页）。
        令牌未配置一律拒（fail-closed）——根治旧「回环放行」：Caddy 反代下
        client_address 恒为 127.0.0.1，来源判断在反代场景完全失真（430 立案根因）。"""
        if not 令牌:
            return False
        if self.headers.get("Authorization", "") == f"Bearer {令牌}":
            return True
        return self._会话合法()

    def _登录(self, 体: dict):
        """网页管理员登录（430）：口令与 CN_BOARD_TOKEN 同源（单一权威）→签发
        HttpOnly 会话 cookie（JS 不可读·SameSite=Lax·SQLite 持久重启不掉）。
        错口令小睡防在线爆破；compare_digest 防时序侧信道。"""
        口令 = str(体.get("口令", ""))
        # compare_digest 须比字节（str 版拒非 ASCII——口令含中文时原样比较会 TypeError 断连）
        if not 令牌 or not secrets.compare_digest(口令.encode("utf-8"), 令牌.encode("utf-8")):
            time.sleep(0.6)
            return self._回JSON(401, {"错误": "口令不符"})
        sid = secrets.token_urlsafe(32)
        now = time.time()
        with 写锁:
            连接.execute("DELETE FROM 会话 WHERE 时戳<?", (now - 会话秒,))   # 过期惰性清
            连接.execute("INSERT INTO 会话(会话id,时戳) VALUES(?,?)", (sid, now))
            连接.commit()
        return self._回JSON(200, {"好": True}, 头们=[(
            "Set-Cookie",
            f"{会话cookie}={sid}; Max-Age={会话秒}; Path=/; HttpOnly; SameSite=Lax")])

    def _登出(self):
        sid = self._取会话id()
        if sid:
            with 写锁:
                连接.execute("DELETE FROM 会话 WHERE 会话id=?", (sid,))
                连接.commit()
        return self._回JSON(200, {"好": True}, 头们=[(
            "Set-Cookie", f"{会话cookie}=; Max-Age=0; Path=/; HttpOnly; SameSite=Lax")])

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
        if 路径 == "/api/me":
            return self._回JSON(200, {"已登录": self._已鉴权()})
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
        # —— 384 文档资源（交接/教训/规范覆盖·GET 只读）——
        if 路径 == "/api/handoff":
            q = urlparse(self.path).query
            参数 = dict(p.split("=", 1) for p in q.split("&") if "=" in p)
            机器 = unquote(参数.get("machine", ""))
            上限 = 参数.get("limit", "20")
            with 写锁:
                return self._回JSON(200, {"条目们": 文档.交接查询(连接, 机器, 上限),
                                          "时刻": 时刻()})
        if 路径 == "/api/lessons":
            q = urlparse(self.path).query
            参数 = dict(p.split("=", 1) for p in q.split("&") if "=" in p)
            with 写锁:
                if 参数.get("high_weight"):
                    return self._回JSON(200, {"条目们": 文档.教训查询(
                        连接, 高权重=int(参数["high_weight"]))})
                if 参数.get("index"):
                    return self._回JSON(200, {"条目们": 文档.教训查询(连接, 索引=True)})
                return self._回JSON(200, {"条目们": 文档.教训查询(连接)})
        if 路径 == "/api/coverage":
            with 写锁:
                return self._回JSON(200, 文档.覆盖全量(连接))
        if 路径 == "/api/adjudications":
            with 写锁:
                return self._回JSON(200, {"裁决项们": 裁决.裁决项列表(连接),
                                          "时刻": 时刻()})
        return self._回JSON(404, {"错误": "未知路径"})

    def do_POST(self):
        路径 = urlparse(self.path).path
        if 路径 == "/api/login":                       # 登录本身豁免鉴权（430）
            return self._登录(self._读JSON体())
        if 路径 == "/api/logout":                      # 登出无破坏性·同豁免
            return self._登出()
        if not self._已鉴权():
            return self._回JSON(401, {"错误": "未登录或令牌不符（写操作须管理员）"})
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
        # —— 384 文档资源（交接/教训/规范覆盖·POST 须令牌）——
        if 路径 == "/api/handoff_add":
            with 写锁:
                码, 响应 = 文档.交接添加(连接, 体)
                return self._回JSON(码, 响应)
        if 路径 == "/api/lesson_add":
            with 写锁:
                码, 响应 = 文档.教训添加(连接, 体)
                return self._回JSON(码, 响应)
        if 路径 == "/api/coverage_update":
            with 写锁:
                码, 响应 = 文档.覆盖单元更新(连接, 体)
                return self._回JSON(码, 响应)
        if 路径 == "/api/coverage_exempt_add":
            with 写锁:
                码, 响应 = 文档.覆盖豁免添加(连接, 体)
                return self._回JSON(码, 响应)
        if 路径 == "/api/coverage_import":
            with 写锁:
                码, 响应 = 文档.覆盖批量导入(连接, 体)
                return self._回JSON(码, 响应)
        # —— 403 裁决资源（网页裁决·POST 须令牌）——
        if 路径 == "/api/adjudication_add":
            with 写锁:
                码, 响应 = 裁决.裁决项设置(连接, 体)
                return self._回JSON(码, 响应)
        if 路径 == "/api/adjudicate":
            with 写锁:
                码, 响应 = 裁决.批量提交(连接, 体)
                return self._回JSON(码, 响应)
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


def main() -> int:
    if "--selftest" in sys.argv:
        import board_selftest    # 405 拆分：自检实现外迁（延迟导入避免环）
        return board_selftest.自检()
    if not 令牌:
        print("警告：CN_BOARD_TOKEN 未配置——登录与全部写操作将被拒绝"
              "（fail-closed·430 根治回环放行）", flush=True)
    服务 = ThreadingHTTPServer(("0.0.0.0", 端口), 处理器)
    print(f"board_service 监听 :{端口}（失联阈值 {失联秒}s·db={db路径}）", flush=True)
    服务.serve_forever()
    return 0


if __name__ == "__main__":
    sys.exit(main())
