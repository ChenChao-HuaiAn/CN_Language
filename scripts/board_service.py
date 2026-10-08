#!/usr/bin/env python3
# 会话意图看板服务（308a·TX_01 部署·业界对照=中心化看板 SSOT+租约）：
#   治「认领状态四处漂移」——285 撞车实录：主树 021 行滞后（226 立项行随分支·设计内窗口）
#   +本地 refs 过期（13 在飞分支只见 1），两机各信各的账。本服务=会话意图的单一事实源：
#   机器+对话id 登记「在做/计划任务号」，心跳租约超时失联灰显，网页看板聚合+冲突高亮。
#   git 仍是代码与 021 归档的真相（收口照旧）；本服务只管「谁正在做/打算做什么」实时层。
#   丙案配套：wt.py 立项前 ls-remote 硬核对 + intent.py 客户端 CLI（本服务不可达时降级不阻断）。
# API（POST 须 Bearer 令牌·GET 只读放行·照 queue_service 口径）：
#   GET  /                   网页看板（单文件内嵌·轮询 /api/board）
#   GET  /api/intents        {意图们:[…], 失联秒}
#   GET  /api/board          聚合 {意图们, 在飞分支们, 快照021, 冲突们, 更新时刻}
#   POST /api/intent         {机器,对话id,在做,计划,备注}   登记/更新（即心跳·幂等）
#   POST /api/intent_release {机器,对话id}                  注销（会话收工）
#   POST /api/report_flights {分支们:[{分支,提交,时刻}]}     在飞分支上报（客户端 ls-remote 结果
#                                ·服务端不持 git 凭据不依赖外网——三机任一活着看板即有数据）
#   POST /api/report_021     {行们:[{号,标题,状态,优先级,备注}]}  021 就绪队列快照上报
# 过期：心跳断 CN_INTENT_STALE_SEC（默认 1800s）→ 失联态（行保留·UI 灰显+失联徽章——
#   不自动删：接管语义=人看板判断后重新 claim 覆盖，同 queue --takeover 精神）。
# 心跳时戳=服务端收到时刻（911 教训：免疫各机时钟漂移）。
# 部署：/etc/systemd/system/cn-board.service（Environment= CN_BOARD_PORT/CN_BOARD_TOKEN/CN_BOARD_DB）。
# 自检：python3 board_service.py --selftest（内存 db+随机端口·登记/心跳/冲突/失联/注销全链+401 反态）。

import json
import os
import re
import sqlite3
import sys
import threading
import time
from datetime import datetime
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

# ===== 可调常量 =====
端口 = int(os.environ.get("CN_BOARD_PORT", "8301"))
令牌 = os.environ.get("CN_BOARD_TOKEN", "")
db路径 = os.environ.get("CN_BOARD_DB",
                        str(os.path.dirname(os.path.abspath(__file__)) + "/board_state.db"))
失联秒 = int(os.environ.get("CN_INTENT_STALE_SEC", "1800"))   # 心跳断此秒数=失联态（行保留灰显）
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
    return con


连接 = 建库()


def 解析任务号们(文本: str) -> list:
    """「110, 276」/「110，276」→ ['110','276']——容错中英文逗号与空白；非法片段丢弃。"""
    if not 文本:
        return []
    出 = []
    for 片 in re.split(r"[,，\s]+", str(文本)):
        片 = 片.strip()
        if 片 and re.fullmatch(r"[0-9]+[a-z]?", 片):
            出.append(片)
    return 出


def 意图行转字典(行) -> dict:
    (对话id, 机器, 在做, 计划, 备注, 心跳时刻, 心跳时戳, 登记时刻) = 行
    陈旧秒 = time.time() - 心跳时戳
    return {"会话键": f"{机器}-{对话id}", "机器": 机器, "对话id": 对话id,
            "在做": 在做, "计划": 计划, "备注": 备注,
            "心跳时刻": 心跳时刻, "时戳": 心跳时戳, "失联": 陈旧秒 > 失联秒,
            "失联秒": int(陈旧秒), "登记时刻": 登记时刻}


def 全部意图() -> list:
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


def 交集标注(意图们: list, 在飞们: list) -> None:
    """就地补每意图/每分支的交叉标注：在飞分支 任务/号 vs 意图声明号 对账。"""
    声明号 = set(任务号声明图(意图们).keys())
    for 飞 in 在飞们:
        m = re.match(r"任务/(.+)$", 飞["分支"])
        飞["号"] = m.group(1) if m else ""
        飞["已登记意图"] = bool(飞["号"]) and 飞["号"] in 声明号


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
    快照 = 取快照("021")
    if 快照:
        # 308d：队列×在飞对撞——021 快照是 develop 态（226 立规认领态住任务分支·
        # 主表 ⬜ 天然滞后），在飞分支上报是实时的：号出现在在飞区=疑似他机认领中。
        # 保持优先级原序（不沉底——沉底+UI 截前 N 条=警示被截掉·黄标原位更显眼）。
        在飞号 = {f["号"] for f in 在飞们 if f.get("号")}
        for 行 in 快照:
            行["疑似认领"] = str(行.get("号", "")) in 在飞号
    return {"意图们": 意图们, "在飞分支们": 在飞们,
            "快照021": 快照, "任务字典": 取快照("任务字典"),
            "冲突们": 冲突检测(意图们),
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

    def do_GET(self):
        路径 = urlparse(self.path).path
        if 路径 in ("/", "/board"):
            return self._回页面()
        if 路径 == "/api/intents":
            with 写锁:
                return self._回JSON(200, {"意图们": 全部意图(), "失联秒": 失联秒})
        if 路径 == "/api/board":
            with 写锁:
                return self._回JSON(200, 聚合视图())
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
        机器 = str(体.get("机器", "")).strip()
        对话id = str(体.get("对话id", "")).strip()
        if not 机器 or not 对话id:
            return self._回JSON(400, {"错误": "机器与对话id 必填"})
        with 写锁:
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
                连接.execute(
                    """INSERT INTO 快照(键,内容,上报时戳) VALUES('任务字典',?,?)
                       ON CONFLICT(键) DO UPDATE SET 内容=excluded.内容, 上报时戳=excluded.上报时戳""",
                    (json.dumps(任务字典, ensure_ascii=False), time.time()))
            连接.commit()
        return self._回JSON(200, {"好": True})

    def _回页面(self):
        体 = 看板页面().encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(体)))
        self.end_headers()
        self.wfile.write(体)


# ===== 网页看板（308a 波3·设计方向=值班室监控台：Grafana 信息密度纪律+Linear 排版克制）
#   拨盘：DESIGN_VARIANCE 6/10·MOTION 3/10（状态灯呼吸+刷新淡入）·DENSITY 6/10
def 看板页面() -> str:
    return """<!DOCTYPE html>
<html lang="zh-CN"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CN · 三机任务看板</title>
<style>
:root{
 --bg:#0b0f14; --surface:#121922; --surface2:#182130; --fg:#e6edf3; --dim:#93a1b0;
 --border:#233042; --accent:#4cc2ff; --ok:#3fb950; --warn:#d9a53a; --danger:#f85149;
 --plan:#79c0ff; --radius:8px;
 --space1:4px; --space2:8px; --space3:14px; --space4:22px; --space5:32px;
 --font:system-ui,-apple-system,"PingFang SC","Microsoft YaHei",sans-serif;
 --mono:ui-monospace,"Cascadia Mono","JetBrains Mono",Consolas,monospace;
 --shadow:0 1px 3px rgba(0,0,0,.4); --dur:.18s; --ease:cubic-bezier(.3,.7,.4,1);
}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:14px/1.55 var(--font)}
a{color:var(--accent);text-decoration:none}
button:focus-visible,a:focus-visible{outline:2px solid var(--accent);outline-offset:2px}
/* —— 顶栏 —— */
header{position:sticky;top:0;z-index:5;display:flex;align-items:baseline;gap:var(--space3);
 background:color-mix(in srgb,var(--bg) 88%,transparent);backdrop-filter:blur(6px);
 border-bottom:1px solid var(--border);padding:var(--space3) var(--space5);}
h1{font-size:15px;font-weight:650;letter-spacing:.04em;margin:0}
h1 .dim{font-weight:400}
#状态灯{width:8px;height:8px;border-radius:50%;background:var(--ok);
 align-self:center;animation:呼吸 2.4s var(--ease) infinite}
#状态灯.断{background:var(--danger);animation:none}
@keyframes 呼吸{0%,100%{opacity:1}50%{opacity:.35}}
#元信息{margin-left:auto;color:var(--dim);font-size:12px;font-variant-numeric:tabular-nums}
/* —— 主区 —— */
main{max-width:1280px;margin:0 auto;padding:var(--space4) var(--space5) var(--space5);
 display:grid;grid-template-columns:minmax(340px,1fr) minmax(380px,1.15fr);
 gap:var(--space4);align-items:start}
@media(max-width:900px){main{grid-template-columns:1fr;padding:var(--space3)}}
h2{font-size:12px;font-weight:600;color:var(--dim);letter-spacing:.14em;margin:0 0 var(--space2)}
/* —— 冲突横幅 —— */
#冲突带{display:none;max-width:1280px;margin:var(--space3) auto 0;padding:0 var(--space5)}
@media(max-width:900px){#冲突带{padding:0 var(--space3)}}
.冲突条{border:1px solid var(--danger);border-left:4px solid var(--danger);
 background:color-mix(in srgb,var(--danger) 10%,var(--surface));
 border-radius:var(--radius);padding:var(--space2) var(--space3);
 font-size:13px;margin-bottom:var(--space2);animation:入场 var(--dur) var(--ease)}
.冲突条 b{font-family:var(--mono)}
@keyframes 入场{from{opacity:0;transform:translateY(-4px)}to{opacity:1}}
/* —— 机器卡片（会话意图） —— */
.机卡{background:var(--surface);border:1px solid var(--border);border-radius:var(--radius);
 box-shadow:var(--shadow);margin-bottom:var(--space3);overflow:hidden}
.机头{display:flex;align-items:center;gap:var(--space2);padding:var(--space2) var(--space3);
 background:var(--surface2);border-bottom:1px solid var(--border);
 font-weight:650;font-size:13px}
.机头 .灯{width:7px;height:7px;border-radius:50%;background:var(--ok);
 animation:呼吸 2.4s var(--ease) infinite}
.机头.全失联 .灯{background:var(--dim);animation:none}
.机头 .副{margin-left:auto;color:var(--dim);font-size:11px;font-weight:400}
.会话{padding:var(--space3);border-bottom:1px solid var(--border)}
.会话:last-child{border-bottom:0}
.会话.失联卡{opacity:.48}
.会话 .行1{display:flex;align-items:center;gap:var(--space2);flex-wrap:wrap}
.会话 .对话{font-family:var(--mono);font-size:12px;color:var(--dim)}
.号牌{font-family:var(--mono);font-weight:700;font-size:13px;color:var(--bg);
 background:var(--accent);border-radius:5px;padding:1px 8px}
.会话.失联卡 .号牌{background:var(--dim)}
.标签{display:inline-block;font-family:var(--mono);font-size:11px;color:var(--plan);
 border:1px solid color-mix(in srgb,var(--plan) 45%,transparent);
 border-radius:4px;padding:0 6px;margin:2px 2px 0 0}
.失联徽{font-size:11px;color:var(--warn);border:1px solid var(--warn);
 border-radius:4px;padding:0 6px}
.备注行{color:var(--dim);font-size:12px;margin-top:var(--space1)}
.心跳行{margin-left:auto;font-size:11px;color:var(--dim);font-variant-numeric:tabular-nums}
.空态{color:var(--dim);font-size:13px;padding:var(--space4);text-align:center}
.空态 code{font-family:var(--mono);color:var(--accent)}
/* —— 在飞分支 / 021 队列 —— */
.面板{background:var(--surface);border:1px solid var(--border);
 border-radius:var(--radius);box-shadow:var(--shadow);
 padding:var(--space3);margin-bottom:var(--space4)}
.飞行{display:flex;align-items:center;gap:var(--space2);padding:var(--space2) 0;
 border-bottom:1px solid var(--border);font-size:13px}
.飞行:last-child{border-bottom:0}
.飞行.僵尸{opacity:.55}
.飞行 .分支名{font-family:var(--mono);color:var(--fg)}
.飞行 .题{color:var(--dim);font-size:12px;overflow:hidden;text-overflow:ellipsis;
 margin-top:1px}
.黄标{font-size:11px;color:var(--warn);white-space:nowrap;align-self:center}
.绿标{font-size:11px;color:var(--ok);white-space:nowrap;align-self:center}
.灰标{font-size:11px;color:var(--dim);white-space:nowrap;align-self:center}
.状态徽{font-size:10px;border:1px solid;border-radius:4px;padding:0 5px;
 margin-left:6px;vertical-align:1px;white-space:nowrap}
.队列行{display:flex;gap:var(--space2);align-items:baseline;padding:3px 0;font-size:13px}
.队号{font-family:var(--mono);color:var(--accent);min-width:44px}
.P0{color:var(--danger)} .P1{color:var(--warn)} .P2{color:var(--plan)} .P3{color:var(--dim)}
.队题{color:var(--dim);overflow:hidden;text-overflow:ellipsis;white-space:nowrap;flex:1}
#错误条{display:none;position:fixed;bottom:var(--space4);left:50%;transform:translateX(-50%);
 background:var(--danger);color:#fff;border-radius:var(--radius);
 padding:var(--space2) var(--space4);font-size:13px;box-shadow:var(--shadow)}
</style></head><body>
<header>
 <div id="状态灯" class="断"></div>
 <h1>CN · 三机任务看板 <span class="dim">／ 会话意图实时互通</span></h1>
 <span id="元信息">连接中…</span>
</header>
<div id="冲突带"></div>
<main>
 <section>
  <h2>会话意图 — 谁在做什么、接下来做什么</h2>
  <div id="意图区"><div class="空态">加载中…</div></div>
 </section>
 <section>
  <h2>远端在飞分支 — 分支存在即认领</h2>
  <div class="面板" id="在飞区"><div class="空态">加载中…</div></div>
  <h2>021 就绪队列 — 可认领任务</h2>
  <div class="面板" id="队列区"><div class="空态">暂无上报</div></div>
 </section>
</main>
<div id="错误条">服务连接中断，正在重试…</div>
<script>
const 拉取=async()=>{try{
  const r=await fetch('api/board');if(!r.ok)throw new Error('HTTP '+r.status);
  const d=await r.json();
  document.getElementById('状态灯').classList.remove('断');
  document.getElementById('错误条').style.display='none';
  document.getElementById('元信息').textContent='数据时刻 '+d.时刻+' · 每 8s 自动刷新';
  渲染冲突(d.冲突们||[]);渲染意图(d.意图们||[]);渲染在飞(d.在飞分支们||[], d.任务字典||{});渲染队列(d.快照021);
}catch(e){
  document.getElementById('状态灯').classList.add('断');
  document.getElementById('错误条').style.display='block';
}};
const 相对时=s=>{if(!s)return'';const 分=Math.floor((Date.now()/1000-s)/60);
 if(分<1)return'刚刚';if(分<60)return 分+' 分钟前';
 const 时=Math.floor(分/60);if(时<24)return 时+' 小时前';
 return Math.floor(时/24)+' 天前';};
const 转义=s=>String(s??'').replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
function 渲染冲突(冲突们){
 const 带=document.getElementById('冲突带');
 带.innerHTML=冲突们.map(c=>'<div class="冲突条">⚠ 任务号 <b>#'+转义(c.号)+
   '</b> 被 '+c.会话们.map(转义).join(' 与 ')+
   ' 同时声明——开工前先核对对方状态，避免两机同做一号</div>').join('');
 带.style.display=冲突们.length?'block':'none';}
function 渲染意图(意图们){
 const 区=document.getElementById('意图区');
 if(!意图们.length){区.innerHTML='<div class="空态">暂无会话登记<br><br>'+
   '<code>python scripts/intent.py claim &lt;任务号&gt; --planned &lt;后续号们&gt;</code><br>登记后 8 秒内全网可见</div>';return;}
 const 按机={};意图们.forEach(i=>(按机[i.机器]=按机[i.机器]||[]).push(i));
 区.innerHTML=Object.entries(按机).map(([机,们])=>{
   const 全失联=们.every(i=>i.失联);
   return '<div class="机卡"><div class="机头'+(全失联?' 全失联':'')+'"><span class="灯"></span>'+
     转义(机)+'<span class="副">'+们.length+' 个会话</span></div>'+
     们.map(i=>{
       const 计划=(i.计划||'').split(',').filter(Boolean).map(p=>'<span class="标签">#'+转义(p)+'</span>').join('');
       return '<div class="会话'+(i.失联?' 失联卡':'')+'"><div class="行1">'+
        '<span class="对话">'+转义(i.对话id)+'</span>'+
        (i.在做?'<span class="号牌">#'+转义(i.在做)+'</span>':'<span class="对话">未挂任务</span>')+
        (i.失联?'<span class="失联徽">失联</span>':'')+
        '<span class="心跳行">'+相对时(i.时戳)+'</span></div>'+
        (计划?'<div style="margin-top:4px">'+计划+'</div>':'')+
        (i.备注?'<div class="备注行">'+转义(i.备注)+'</div>':'')+
       '</div>';}).join('')+'</div>';}).join('');}
function 渲染在飞(们, 字典){
 const 区=document.getElementById('在飞区');
 if(!们.length){区.innerHTML='<div class="空态">暂无在飞数据——intent.py claim 时自动上报</div>';return;}
 const 状态徽={'⬜':['待办','#93a1b0'],'🏃':['在飞','#3fb950'],'⏸':['挂起','#d9a53a'],'✅':['已完成','#79c0ff']};
 区.innerHTML=们.map(f=>{
   const 号=f.号||''; const t=字典[号];
   const 标 = f.已并入 ? '<span class="灰标">已并入 develop · 待删</span>'
            : (f.已登记意图 ? '<span class="绿标">意图已登记</span>'
                           : '<span class="黄标">未登记意图</span>');
   const 徽 = t&&状态徽[t.状态] ? '<span class="状态徽" style="color:'+状态徽[t.状态][1]+
                ';border-color:'+状态徽[t.状态][1]+'">'+状态徽[t.状态][0]+'</span>' : '';
   const 题 = t ? 转义(t.标题) : '<span style="opacity:.6">'+转义(f.提交题||f.提交)+'</span>';
   return '<div class="飞行'+(f.已并入?' 僵尸':'')+'"><div style="min-width:0;flex:1">'+
     '<div><span class="分支名">'+转义(f.分支)+'</span> '+徽+'</div>'+
     '<div class="题" style="white-space:normal">'+题+'</div></div>'+标+'</div>';}).join('');}
function 渲染队列(快照){
 const 区=document.getElementById('队列区');
 if(!快照||!快照.length){区.innerHTML='<div class="空态">暂无 021 快照上报</div>';return;}
 区.innerHTML=快照.slice(0,20).map(t=>'<div class="队列行"><span class="P'+
   转义(t.优先级||'3')+'">'+转义(t.优先级||'—')+'</span><span class="队号">#'+
   转义(t.号)+'</span><span class="队题"'+(t.疑似认领?' style="text-decoration:line-through;opacity:.55"':'')+'>'+
   转义(t.标题||'')+'</span>'+
   (t.疑似认领?'<span class="黄标">⚠ 疑似认领中</span>':'')+'</div>').join('');}
拉取();setInterval(拉取,8000);
</script></body></html>"""


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
        签("在飞上报只收 任务/ 前缀", len(r["在飞分支们"]) == 1)
        f = r["在飞分支们"][0]
        签("交集标注：308a 在飞=已登记意图", f["已登记意图"] is True)
        连接2 = globals()["连接"]
        连接2.execute("UPDATE 意图 SET 心跳时戳=? WHERE 对话id='a1'", (time.time() - 999,))
        连接2.commit()
        码, r = 调("GET", "/api/board")
        深度行 = next(i for i in r["意图们"] if i["对话id"] == "a1")
        签("心跳超时→失联态", 深度行["失联"] is True)
        码, r = 调("POST", "/api/intent_release", {"机器": "家机", "对话id": "b3"})
        码, r = 调("GET", "/api/board")
        签("注销后冲突消除", not any(c["号"] == "308a" for c in r.get("冲突们", [])))
        码, r = 调("POST", "/api/report_021", {"行们": [
            {"号": "002", "标题": "甲", "优先级": "P0"},
            {"号": "110", "标题": "乙", "优先级": "P1"}],
            "任务字典": {"110": {"标题": "整64 FFI arm64", "状态": "⬜", "优先级": "P1"},
                         "211": {"标题": "v2 返回局部结果变量双放", "状态": "✅", "优先级": "P1"}}})
        调("POST", "/api/report_flights", {"上报者": "深度机", "分支们": [
            {"分支": "任务/110", "提交": "abc123", "时刻": "", "已并入": False},
            {"分支": "任务/211", "提交": "def456", "时刻": "", "已并入": True}]})
        码, r = 调("GET", "/api/board")
        飞 = {f["分支"]: f for f in r["在飞分支们"]}
        签("308e 已并入字段透传", 飞["任务/211"]["已并入"] is True
           and 飞["任务/110"]["已并入"] is False)
        字 = r.get("任务字典") or {}
        签("308e 任务字典上板（标题/状态）",
           字.get("110", {}).get("标题") == "整64 FFI arm64"
           and 字.get("211", {}).get("状态") == "✅")
        调("POST", "/api/report_flights", {"上报者": "深度机", "分支们": [
            {"分支": "任务/110", "提交": "abc123", "时刻": ""}]})
        码, r = 调("GET", "/api/board")
        快 = {行["号"]: 行 for 行 in r["快照021"]}
        签("308d 队列×在飞对撞：110 在飞=疑似认领", 快["110"]["疑似认领"] is True)
        签("308d 未在飞行保持可认领", 快["002"]["疑似认领"] is False)
        签("308d 队列保持优先级原序（002 P0 在 110 P1 前）",
           [行["号"] for 行 in r["快照021"]].index("110")
           > [行["号"] for 行 in r["快照021"]].index("002"))
        码, r = 调("POST", "/api/intent", {"对话id": "无机器"})
        签("缺机器参数 400", 码 == 400)
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
        签("看板页 200 且含看板字样", resp.status == 200 and "任务看板" in 页)
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
