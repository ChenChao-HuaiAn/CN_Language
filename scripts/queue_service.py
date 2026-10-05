#!/usr/bin/env python3
# 服务端队列/状态服务（920 治理专项·TX_01 部署·业界对照=bors/homu 服务端队列）：
#   治「git 被当状态数据库用」——看板集成队列/门禁结果的机器状态迁到本服务（SQLite 承载），
#   三机 integrate.py 的队列操作改走 HTTP（不可达自动回退看板直推旧路径=降级不失效）；
#   状态转移零 git 提交（治 920 诊断的 56% 单文件看板提交污染）。
# 1021 预验执行器池化：新增「预验任务表」=多 runner 认领池（TX_02 与家机 WSL2 实例谁空闲
#   谁认领·治单 TX_02 串行排队）——task_enqueue/task_claim/task_heartbeat/task_complete；
#   心跳断 CN_TASK_STALE_SEC（默认 300s）服务端回收重派（家机睡眠/关机不黑洞）；
#   心跳时戳=服务端收到时刻（免疫 WSL 时钟漂移）；认领原子性靠 写锁+单线程写保证。
# API（POST 须 Bearer 令牌·GET 只读放行）：
#   GET  /            人类可读结果页（最新门禁+历史+队列+预验任务池）
#   GET  /api/state   {队列:[…], 门禁最近:[…], 预验任务:[…]}
#   GET  /api/task_result?分支=…   单个预验任务 {状态,绿,详情}（详情=daemon 结果 JSON）
#   POST /api/join    {分支,基线,写集摘要}      报名（幂等）
#   POST /api/update  {分支,状态}               改状态（排队/集成中/已踢出/已完成）
#   POST /api/touch   {分支}                    重报时刻（失败恢复重排队·保序不刷=幂等）
#   POST /api/clear   {分支们:[…]}              集成销账清行
#   POST /api/report  {ci_daemon 结果 JSON}     门禁结果上报
#   POST /api/task_enqueue  {分支,sha}           预验任务入队（幂等·已完成绿=直接复用·已完成红=重置重跑）
#   POST /api/task_claim    {runner}             认领最旧排队任务（含回收心跳超时任务·原子）
#   POST /api/task_heartbeat {runner,分支}        心跳续约
#   POST /api/task_complete {runner,分支,绿,结果}  完成上报（校验认领者·防回收后旧 runner 复活覆盖）
# 部署：/etc/systemd/system/cn-queue.service（Environment= 端口/令牌/DB 路径）。

import json
import os
import sqlite3
import sys
import threading
import time
from datetime import datetime
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

# ===== 可调常量 =====
端口 = int(os.environ.get("CN_QUEUE_PORT", "8300"))
令牌 = os.environ.get("CN_QUEUE_TOKEN", "")
db路径 = os.environ.get("CN_QUEUE_DB",
                        str(Path(__file__).resolve().parent / "queue_state.db"))
门禁历史保留 = 200
任务历史保留 = 200          # 预验任务完成行裁剪（同门禁口径）
心跳超时秒 = int(os.environ.get("CN_TASK_STALE_SEC", "300"))   # 心跳断此秒数=回收重派（家机睡眠不黑洞）

写锁 = threading.Lock()          # SQLite 写串行化（读靠 WAL 并发）
时区时刻 = lambda: datetime.now().strftime("%m-%d %H:%M:%S")   # 911 教训：秒级戳


def 建库() -> sqlite3.Connection:
    con = sqlite3.connect(db路径, check_same_thread=False)
    con.execute("PRAGMA journal_mode=WAL")
    con.execute("""CREATE TABLE IF NOT EXISTS 队列(
        分支 TEXT PRIMARY KEY, 基线 TEXT, 报名时刻 TEXT, 写集摘要 TEXT,
        状态 TEXT DEFAULT '排队', 更新时刻 TEXT)""")
    con.execute("""CREATE TABLE IF NOT EXISTS 门禁(
        sha TEXT, 平台 TEXT, 绿 INTEGER, 总秒 INTEGER, 时刻 TEXT, 详情 TEXT,
        PRIMARY KEY(sha, 平台))""")
    con.execute("""CREATE TABLE IF NOT EXISTS 预验任务(
        序号 INTEGER PRIMARY KEY AUTOINCREMENT,
        分支 TEXT UNIQUE, sha TEXT,
        状态 TEXT DEFAULT '排队',      -- 排队/执行中/完成
        认领者 TEXT, 心跳时戳 INTEGER,  -- 心跳=服务端收到时刻（免疫客户端时钟漂移）
        绿 INTEGER, 详情 TEXT,          -- 详情=ci_daemon 结果 JSON 原样
        入队时刻 TEXT, 更新时刻 TEXT)""")
    con.commit()
    return con


con = 建库()


def 队列快照() -> list[dict]:
    行们 = con.execute("SELECT 分支,基线,报名时刻,写集摘要,状态,更新时刻 FROM 队列 "
                       "ORDER BY 报名时刻").fetchall()
    return [dict(zip(("分支", "基线", "报名时刻", "写集摘要", "状态", "更新时刻"), r)) for r in 行们]


def 门禁快照(限: int = 20) -> list[dict]:
    行们 = con.execute("SELECT sha,平台,绿,总秒,时刻 FROM 门禁 "
                       "ORDER BY 时刻 DESC LIMIT ?", (限,)).fetchall()
    return [dict(zip(("sha", "平台", "绿", "总秒", "时刻"), r)) for r in 行们]


def 预验任务快照() -> list[dict]:
    """预验任务池快照（不含 详情 大字段——integrate 取详情走 /api/task_result）。"""
    行们 = con.execute("SELECT 序号,分支,sha,状态,认领者,绿,入队时刻,更新时刻 "
                       "FROM 预验任务 ORDER BY 序号 DESC LIMIT 50").fetchall()
    return [dict(zip(("序号", "分支", "sha", "状态", "认领者", "绿", "入队时刻", "更新时刻"), r))
            for r in 行们]


def 处理写(名: str, 数据: dict) -> dict:
    """统一写入口（写锁内）——返回 {ok, 说明}。"""
    with 写锁:
        if 名 == "join":
            分支 = str(数据.get("分支", ""))
            if not 分支.startswith("任务/"):
                return {"ok": False, "说明": "分支名须 任务/ 开头"}
            已在 = con.execute("SELECT 状态 FROM 队列 WHERE 分支=?", (分支,)).fetchone()
            if 已在:
                # 1020（#180·方案乙·用户裁决 2026-10-05）：重报重置语义——旧幂等
                #   「已在就什么都不做」吞重报：被踢出分支重报名后状态停「已踢出」
                #   永不回「排队」→批主 min(排队们) 选不出→全员死等（1017 实录
                #   [等待] None 20 分钟）；基线不刷新=分支 rebase 后旧行基线陈旧
                #   （组链错基）。现语义：集成中=批主在飞不动；其余态（排队/已踢出/
                #   归因出批/冲突出批）一律重置「排队」+刷新基线/摘要/时刻（重报者
                #   时刻刷新=重排队尾·批主时刻刷新=收拢窗口重开·基线变需重攒批）。
                旧态 = 已在[0]
                if 旧态 == "集成中":
                    return {"ok": True, "说明": "集成中（批主在飞·不改）"}
                con.execute("UPDATE 队列 SET 状态='排队',基线=?,写集摘要=?,报名时刻=?,"
                            "更新时刻=? WHERE 分支=?",
                            (str(数据.get("基线", "")), str(数据.get("写集摘要", ""))[:120],
                             时区时刻(), 时区时刻(), 分支))
                con.commit()
                return {"ok": True, "说明": "重报重置排队（原态 " + 旧态 + "）"}
            con.execute("INSERT INTO 队列(分支,基线,报名时刻,写集摘要,状态,更新时刻) "
                        "VALUES(?,?,?,?,'排队',?)",
                        (分支, str(数据.get("基线", "")), 时区时刻(),
                         str(数据.get("写集摘要", ""))[:120], 时区时刻()))
        elif 名 == "update":
            分支, 新状态 = str(数据.get("分支", "")), str(数据.get("状态", ""))
            # 1020（#180）：状态集对齐看板版 队列状态们 四态（旧集缺 冲突出批/
            #   归因出批——批主踢人标态在服务端静默 ok=False·同族不对称第二处）
            if 新状态 not in ("排队", "集成中", "冲突出批", "归因出批", "已踢出", "已完成"):
                return {"ok": False, "说明": "非法状态 " + 新状态}
            cur = con.execute("UPDATE 队列 SET 状态=?,更新时刻=? WHERE 分支=?",
                              (新状态, 时区时刻(), 分支))
            if cur.rowcount == 0:
                return {"ok": False, "说明": "无此行 " + 分支}
        elif 名 == "touch":
            分支 = str(数据.get("分支", ""))
            con.execute("UPDATE 队列 SET 状态='排队',更新时刻=? WHERE 分支=?",
                        (时区时刻(), 分支))
        elif 名 == "clear":
            for 分支 in 数据.get("分支们", []):
                con.execute("DELETE FROM 队列 WHERE 分支=?", (str(分支),))
        elif 名 == "report":
            sha, 平台 = str(数据.get("sha", "")), str(数据.get("平台", ""))
            if len(sha) < 7:
                return {"ok": False, "说明": "sha 缺失"}
            # 190（1035 轮）：状态=执行中（daemon 开跑即上报·绿=None）→门禁表
            #   实时显示在跑轮（1030 误判「漏跑」根因=执行中不可见）；完成轮
            #   （无 状态 字段·绿 布尔）照常覆盖同 sha 行。僵死执行中行由
            #   完成行覆盖或被 时刻 DESC 淘汰（不单设超时——daemon 存活属
            #   另一监控域）。
            是执行中 = str(数据.get("状态", "完成")) == "执行中"
            con.execute("INSERT OR REPLACE INTO 门禁(sha,平台,绿,总秒,时刻,详情) "
                        "VALUES(?,?,?,?,?,?)",
                        (sha, 平台,
                         None if 是执行中 else (1 if 数据.get("绿") else 0),
                         None if 是执行中 else int(数据.get("总秒", 0)),
                         时区时刻(),
                         json.dumps(数据, ensure_ascii=False)[:20000]))
            con.execute("DELETE FROM 门禁 WHERE (sha,平台) NOT IN "
                        "(SELECT sha,平台 FROM 门禁 ORDER BY 时刻 DESC LIMIT ?)",
                        (门禁历史保留,))
        elif 名 == "task_enqueue":
            # 预验任务入队（幂等）：新分支→排队；已完成绿→原样保留（integrate 直接取结果·省一次全量）；
            # 已完成红→重置排队（同 sha CAS 重试路径·对齐 ssh 直发 rm 旧文件重跑语义·也给 flaky 一次机会）；
            # 排队/执行中→不动。
            分支, sha = str(数据.get("分支", "")), str(数据.get("sha", ""))
            if not 分支.startswith("ci/预验-") or len(sha) < 7:
                return {"ok": False, "说明": "分支须 ci/预验- 开头且 sha 缺失"}
            已有 = con.execute("SELECT 状态,绿 FROM 预验任务 WHERE 分支=?", (分支,)).fetchone()
            if 已有 is None:
                con.execute("INSERT INTO 预验任务(分支,sha,状态,入队时刻,更新时刻) "
                            "VALUES(?,?,'排队',?,?)", (分支, sha, 时区时刻(), 时区时刻()))
            elif 已有[0] == "完成" and not 已有[1]:
                con.execute("UPDATE 预验任务 SET 状态='排队',认领者=NULL,心跳时戳=NULL,"
                            "绿=NULL,详情=NULL,更新时刻=? WHERE 分支=?", (时区时刻(), 分支))
        elif 名 == "task_claim":
            # 认领（写锁内原子）：①回收心跳超时的执行中任务→重置排队；②取最旧排队任务置执行中。
            runner = str(数据.get("runner", ""))
            if not runner:
                return {"ok": False, "说明": "runner 标识缺失"}
            现在 = int(time.time())
            con.execute("UPDATE 预验任务 SET 状态='排队',认领者=NULL,心跳时戳=NULL,更新时刻=? "
                        "WHERE 状态='执行中' AND 心跳时戳 IS NOT NULL AND 心跳时戳<?",
                        (时区时刻(), 现在 - 心跳超时秒))
            行 = con.execute("SELECT 分支,sha FROM 预验任务 WHERE 状态='排队' "
                             "ORDER BY 序号 LIMIT 1").fetchone()
            if 行 is None:
                return {"ok": True, "任务": None}
            con.execute("UPDATE 预验任务 SET 状态='执行中',认领者=?,心跳时戳=?,更新时刻=? "
                        "WHERE 分支=?", (runner, 现在, 时区时刻(), 行[0]))
            return {"ok": True, "任务": {"分支": 行[0], "sha": 行[1]}}
        elif 名 == "task_heartbeat":
            runner, 分支 = str(数据.get("runner", "")), str(数据.get("分支", ""))
            cur = con.execute("UPDATE 预验任务 SET 心跳时戳=?,更新时刻=? "
                              "WHERE 分支=? AND 认领者=? AND 状态='执行中'",
                              (int(time.time()), 时区时刻(), 分支, runner))
            if cur.rowcount == 0:
                return {"ok": False, "说明": "任务不在执行中或认领者不匹配 " + 分支}
        elif 名 == "task_cancel":
            # 1026 降级竞态治理：integrate 池空 300s 降级 ssh 前调用——删「排队」行，
            #   防止后来 runner 认领到已被降级路径接手的任务=双跑浪费+池红记录误挂。
            #   「执行中」不删（runner 在真跑·让它跑完留档）；「完成」幂等无操作。
            分支 = str(数据.get("分支", ""))
            cur = con.execute("DELETE FROM 预验任务 WHERE 分支=? AND 状态='排队'", (分支,))
            return {"ok": True, "取消": cur.rowcount}
        elif 名 == "task_complete":
            # 完成上报：校验认领者（回收重派后旧 runner 复活=拒绝·重派者重新跑出的结果为准）
            runner, 分支 = str(数据.get("runner", "")), str(数据.get("分支", ""))
            结果 = 数据.get("结果")
            绿 = 1 if 数据.get("绿") else 0
            cur = con.execute("UPDATE 预验任务 SET 状态='完成',绿=?,详情=?,心跳时戳=NULL,更新时刻=? "
                              "WHERE 分支=? AND 认领者=? AND 状态='执行中'",
                              (绿, json.dumps(结果, ensure_ascii=False)[:20000],
                               时区时刻(), 分支, runner))
            if cur.rowcount == 0:
                return {"ok": False, "说明": "任务不在执行中或认领者不匹配（可能已被回收重派）" + 分支}
            con.execute("DELETE FROM 预验任务 WHERE 状态='完成' AND 序号 NOT IN "
                        "(SELECT 序号 FROM 预验任务 WHERE 状态='完成' ORDER BY 序号 DESC LIMIT ?)",
                        (任务历史保留,))
        else:
            return {"ok": False, "说明": "未知操作 " + 名}
        con.commit()
    return {"ok": True}


HTML页 = """<!doctype html><html><head><meta charset="utf-8"><title>CN 队列/门禁状态</title>
<style>body{font-family:system-ui;margin:24px;background:#f6f8fa}h2{margin:18px 0 6px}
table{border-collapse:collapse;background:#fff}td,th{border:1px solid #d0d7de;padding:5px 10px;
font-size:14px}.绿{color:#1a7f37;font-weight:600}.红{color:#cf222e;font-weight:600}
.执行中{color:#9a6700;font-weight:600}</style></head>
<body><h2>最新门禁（develop 每推送自动跑·TX_02 云 CI）</h2>__门禁表__
<h2>预验任务池（多 runner 认领·1021）</h2>__任务表__
<h2>集成队列（服务端·零 git 提交）</h2>__队列表__<p>生成于 __时刻__</p></body></html>"""


class 处理器(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        print("[%s] %s" % (时区时刻(), fmt % args), flush=True)

    def 回JSON(self, obj: dict, 码: int = 200):
        体 = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(码)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(体)))
        self.end_headers()
        self.wfile.write(体)

    def 鉴权通过(self) -> bool:
        return 令牌 == "" or self.headers.get("Authorization", "") == "Bearer " + 令牌

    def do_GET(self):
        from urllib.parse import parse_qs
        路径 = urlparse(self.path).path
        if 路径 == "/api/state":
            return self.回JSON({"队列": 队列快照(), "门禁最近": 门禁快照(),
                                "预验任务": 预验任务快照()})
        if 路径 == "/api/task_result":
            # 按 sha10 查（分支=ci/预验-<sha10>·URL 保持纯 ASCII 免编码坑）
            sha10 = (parse_qs(urlparse(self.path).query).get("sha") or [""])[0]
            行 = con.execute("SELECT 状态,绿,详情 FROM 预验任务 WHERE 分支=?",
                             ("ci/预验-" + sha10,)).fetchone()
            if 行 is None:
                return self.回JSON({"ok": False, "说明": "无此任务 ci/预验-" + sha10}, 404)
            return self.回JSON({"状态": 行[0], "绿": bool(行[1]) if 行[1] is not None else None,
                                "详情": json.loads(行[2]) if 行[2] else None})
        if 路径 == "/":
            # 190（1035 轮）：执行中行（绿 IS NULL）显示黄色「执行中」——1030 误判
            #   「六次推送漏跑」根因修除（daemon 在跑但状态页不可见）。
            def 门禁单元格(r):
                if r["绿"] is None:
                    return "执行中", "执行中", "—"
                return (("绿" if r["绿"] else "红"),) * 2 + ("%ss" % r["总秒"],)
            门禁行 = "".join(
                "<tr><td>%s</td><td>%s</td><td class='%s'>%s</td><td>%s</td><td>%s</td></tr>"
                % (r["sha"][:10], r["平台"], 格[0], 格[1], 格[2], r["时刻"])
                for r, 格 in ((r, 门禁单元格(r)) for r in 门禁快照())
            ) or "<tr><td colspan=5>暂无</td></tr>"
            队列行 = "".join("<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>"
                            % (r["分支"], r["基线"][:10], r["报名时刻"], r["状态"], r["写集摘要"][:60])
                            for r in 队列快照()) or "<tr><td colspan=5>空</td></tr>"
            任务行 = "".join(
                "<tr><td>%s</td><td>%s</td><td>%s</td><td class='%s'>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>"
                % (r["分支"], r["sha"][:10], r["状态"],
                   ("绿" if r["绿"] else "红") if r["状态"] == "完成" and r["绿"] is not None else "",
                   ("绿" if r["绿"] else "红") if r["状态"] == "完成" and r["绿"] is not None else "—",
                   r["认领者"] or "—", r["入队时刻"], r["更新时刻"])
                for r in 预验任务快照()) or "<tr><td colspan=7>空</td></tr>"
            页 = (HTML页.replace("__门禁表__", "<table><tr><th>SHA</th><th>平台</th><th>结果</th>"
                  "<th>耗时</th><th>时刻</th></tr>" + 门禁行 + "</table>")
                  .replace("__任务表__", "<table><tr><th>分支</th><th>SHA</th><th>状态</th><th>结果</th>"
                           "<th>认领者</th><th>入队</th><th>更新</th></tr>" + 任务行 + "</table>")
                  .replace("__队列表__", "<table><tr><th>分支</th><th>基线</th><th>报名时刻</th>"
                           "<th>状态</th><th>写集</th></tr>" + 队列行 + "</table>")
                  .replace("__时刻__", 时区时刻()))
            体 = 页.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(体)))
            self.end_headers()
            self.wfile.write(体)
            return
        self.回JSON({"ok": False, "说明": "未知路径"}, 404)

    def do_POST(self):
        if not self.鉴权通过():
            return self.回JSON({"ok": False, "说明": "令牌无效"}, 401)
        名 = urlparse(self.path).path.rsplit("/", 1)[-1]
        长度 = int(self.headers.get("Content-Length", "0"))
        try:
            数据 = json.loads(self.rfile.read(长度).decode("utf-8")) if 长度 else {}
        except Exception as e:
            return self.回JSON({"ok": False, "说明": "JSON 解析失败 " + str(e)}, 400)
        try:
            self.回JSON(处理写(名, 数据))
        except Exception as e:
            self.回JSON({"ok": False, "说明": repr(e)}, 500)


if __name__ == "__main__":
    # 1020（#180）：--selftest=join 场景矩阵（内存库·不启 HTTP）——
    #   验证重报重置语义（首报排队/重报刷新/踢出后重报回排队/集成中保护）。
    if len(sys.argv) > 1 and sys.argv[1] == "--selftest":
        import tempfile
        db路径 = os.path.join(tempfile.mkdtemp(), "selftest.db")
        con = 建库()   # 模块级重绑（__main__ 同作用域·内存态覆盖·服务启动路径不受影响）
        n绑定 = [0]

        def 断言(条件, 说明):
            n绑定[0] += 1
            if not 条件:
                print("✗ 失败: " + 说明)
                raise SystemExit(1)
            print("  ✓ " + 说明)

        r = 处理写("join", {"分支": "任务/甲-1-x", "基线": "aaa", "写集摘要": "2 文件"})
        断言(r["ok"] and "重报" not in r.get("说明", ""), "首报=排队插入")
        断言(队列快照()[0]["状态"] == "排队", "首报态=排队")
        r = 处理写("join", {"分支": "任务/甲-1-x", "基线": "bbb", "写集摘要": "3 文件"})
        断言(r["ok"] and "重报重置排队" in r.get("说明", ""), "排队态重报=重置+刷新")
        行 = next(r2 for r2 in 队列快照() if r2["分支"] == "任务/甲-1-x")
        断言(行["基线"] == "bbb" and 行["状态"] == "排队", "排队态重报基线已刷新")
        处理写("update", {"分支": "任务/甲-1-x", "状态": "已踢出"})
        r = 处理写("join", {"分支": "任务/甲-1-x", "基线": "ccc", "写集摘要": "4 文件"})
        断言(队列快照()[0]["状态"] == "排队" and "原态 已踢出" in r.get("说明", ""),
             "已踢出重报=重置回排队（1017 死等病根）")
        处理写("update", {"分支": "任务/甲-1-x", "状态": "集成中"})
        r = 处理写("join", {"分支": "任务/甲-1-x", "基线": "ddd", "写集摘要": "5 文件"})
        断言(队列快照()[0]["状态"] == "集成中" and 队列快照()[0]["基线"] == "ccc",
             "集成中重报=保护不动")
        处理写("update", {"分支": "任务/甲-1-x", "状态": "冲突出批"})
        r = 处理写("join", {"分支": "任务/甲-1-x", "基线": "eee", "写集摘要": "6 文件"})
        断言(队列快照()[0]["状态"] == "排队", "冲突出批重报=回排队")
        r = 处理写("join", {"分支": "错误名", "基线": "x", "写集摘要": ""})
        断言(not r["ok"], "非任务/ 分支拒绝")
        print("selftest %d 项全过" % n绑定[0])
        raise SystemExit(0)

    print("[启动] 端口=%d db=%s 令牌=%s 任务池心跳超时=%ds"
          % (端口, db路径, "已设" if 令牌 else "未设（不鉴权·仅内网用）", 心跳超时秒),
          flush=True)
    ThreadingHTTPServer(("0.0.0.0", 端口), 处理器).serve_forever()
