#!/usr/bin/env python3
# 服务端队列/状态服务（920 治理专项·TX_01 部署·业界对照=bors/homu 服务端队列）：
#   治「git 被当状态数据库用」——看板集成队列/门禁结果的机器状态迁到本服务（SQLite 承载），
#   三机 integrate.py 的队列操作改走 HTTP（不可达自动回退看板直推旧路径=降级不失效）；
#   状态转移零 git 提交（治 920 诊断的 56% 单文件看板提交污染）。
# API（POST 须 Bearer 令牌·GET 只读放行）：
#   GET  /            人类可读结果页（最新门禁+历史+队列）
#   GET  /api/state   {队列:[…], 门禁最近:[…]}
#   POST /api/join    {分支,基线,写集摘要}      报名（幂等）
#   POST /api/update  {分支,状态}               改状态（排队/集成中/已踢出/已完成）
#   POST /api/touch   {分支}                    重报时刻（失败恢复重排队·保序不刷=幂等）
#   POST /api/clear   {分支们:[…]}              集成销账清行
#   POST /api/report  {ci_daemon 结果 JSON}     门禁结果上报
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
            con.execute("INSERT OR REPLACE INTO 门禁(sha,平台,绿,总秒,时刻,详情) "
                        "VALUES(?,?,?,?,?,?)",
                        (sha, 平台, 1 if 数据.get("绿") else 0,
                         int(数据.get("总秒", 0)), 时区时刻(),
                         json.dumps(数据, ensure_ascii=False)[:20000]))
            con.execute("DELETE FROM 门禁 WHERE (sha,平台) NOT IN "
                        "(SELECT sha,平台 FROM 门禁 ORDER BY 时刻 DESC LIMIT ?)",
                        (门禁历史保留,))
        else:
            return {"ok": False, "说明": "未知操作 " + 名}
        con.commit()
    return {"ok": True}


HTML页 = """<!doctype html><html><head><meta charset="utf-8"><title>CN 队列/门禁状态</title>
<style>body{font-family:system-ui;margin:24px;background:#f6f8fa}h2{margin:18px 0 6px}
table{border-collapse:collapse;background:#fff}td,th{border:1px solid #d0d7de;padding:5px 10px;
font-size:14px}.绿{color:#1a7f37;font-weight:600}.红{color:#cf222e;font-weight:600}</style></head>
<body><h2>最新门禁（develop 每推送自动跑·TX_02 云 CI）</h2>__门禁表__
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
        路径 = urlparse(self.path).path
        if 路径 == "/api/state":
            return self.回JSON({"队列": 队列快照(), "门禁最近": 门禁快照()})
        if 路径 == "/":
            门禁行 = "".join("<tr><td>%s</td><td>%s</td><td class='%s'>%s</td><td>%ss</td><td>%s</td></tr>"
                             % (r["sha"][:10], r["平台"], "绿" if r["绿"] else "红",
                                "绿" if r["绿"] else "红", r["总秒"], r["时刻"])
                             for r in 门禁快照()) or "<tr><td colspan=5>暂无</td></tr>"
            队列行 = "".join("<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>"
                            % (r["分支"], r["基线"][:10], r["报名时刻"], r["状态"], r["写集摘要"][:60])
                            for r in 队列快照()) or "<tr><td colspan=5>空</td></tr>"
            页 = (HTML页.replace("__门禁表__", "<table><tr><th>SHA</th><th>平台</th><th>结果</th>"
                  "<th>耗时</th><th>时刻</th></tr>" + 门禁行 + "</table>")
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
        n = 0

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

    print("[启动] 端口=%d db=%s 令牌=%s" % (端口, db路径, "已设" if 令牌 else "未设（不鉴权·仅内网用）"),
          flush=True)
    ThreadingHTTPServer(("0.0.0.0", 端口), 处理器).serve_forever()
