#!/usr/bin/env python3
# 服务端队列/状态服务（920 治理专项·TX_01 部署·业界对照=bors/homu 服务端队列）：
#   治「git 被当状态数据库用」——看板集成队列/门禁结果的机器状态迁到本服务（SQLite 承载），
#   三机 integrate.py 的队列操作改走 HTTP（不可达自动回退看板直推旧路径=降级不失效）；
#   状态转移零 git 提交（治 920 诊断的 56% 单文件看板提交污染）。
# 1021 预验执行器池化：新增「预验任务表」=多 runner 认领池（TX_02 与家机 WSL2 实例谁空闲
#   谁认领·治单 TX_02 串行排队）——task_enqueue/task_claim/task_heartbeat/task_complete；
#   心跳断 CN_TASK_STALE_SEC（默认 300s）服务端回收重派（家机睡眠/关机不黑洞）；
#   心跳时戳=服务端收到时刻（免疫 WSL 时钟漂移）；认领原子性靠 写锁+单线程写保证。
# 770 develop 验证平台轮转（2026-10-14 用户三条裁决）：①develop 头变化必验不可跳过——
#   ci/兜底-* 任务由服务端按 win-x64→linux-x86_64→linux-arm64 游标轮转指派平台，平台内
#   「随机挑空闲」由认领竞争天然形成（同平台多实例谁先空闲谁领）；该平台全忙（注册实例全在
#   执行中）→认领限制直接放宽 NULL=顺序由下一平台的空闲机器执行；行排队超
#   CN_DEVELOP_PLAT_SWITCH_SEC 仍无人领（平台饿死兜底）→claim 时惰性放宽 NULL。
#   ②预验全池随机空闲（现状认领模型即语义）。③/api/state 增「池忙闲」=integrate 忙时
#   拼车组链窗口与巡检的忙闲判定源。客户端对兜底任务传的认领限制一律忽略（服务端轮转唯一权威）。
# API（POST 须 Bearer 令牌·GET 只读放行）：
#   GET  /            人类可读结果页（最新门禁+历史+队列+预验任务池）
#   GET  /api/state   {队列:[…], 门禁最近:[…], 预验任务:[…]}
#   GET  /api/task_result?分支=…   单个预验任务 {状态,绿,详情}（详情=daemon 结果 JSON）
#   POST /api/join    {分支,基线,写集摘要}      报名（幂等）
#   POST /api/update  {分支,状态}               改状态（排队/集成中/已踢出/已完成）
#   POST /api/touch   {分支}                    重报时刻（失败恢复重排队·保序不刷=幂等）
#   POST /api/heartbeat {分支}                  562：只刷更新时刻（批主/等待者活会话证明·
#                                             不改状态——回收与 --wait 接管判定以心跳新鲜度为据）
#   POST /api/clear   {分支们:[…]}              集成销账清行
#   POST /api/report  {ci_daemon 结果 JSON}     门禁结果上报
#   POST /api/task_enqueue  {分支,sha,源分支?,认领限制?}  预验任务入队（幂等·已完成绿=直接复用·已完成红=重置重跑；
#                                                 源分支=发起预验的任务分支·222 网页分支列显示用·缺省兼容旧客户端；
#                                                 认领限制=平台白名单逗号串（337·如 win-x64,linux-x86_64）·缺省 NULL=不限·291 混合池默认不变）
#   POST /api/task_claim    {runner,平台?}       认领最旧排队任务（含回收心跳超时任务·原子；337 起自报平台=只领
#                                                 不限或白名单含本平台的任务·旧客户端不报=空串只领不限任务·不会错跑）
#   POST /api/task_heartbeat {runner,分支}        心跳续约
#   POST /api/task_complete {runner,分支,绿,结果}  完成上报（校验认领者·防回收后旧 runner 复活覆盖）
# 部署：/etc/systemd/system/cn-queue.service（Environment= 端口/令牌/DB 路径）。

import json
import os
import re
import sqlite3
import sys
import threading
import time
import urllib.error
import urllib.request
from datetime import datetime, timedelta
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
集成中超时秒 = int(os.environ.get("CN_QUEUE_INTEGRATE_STALE_SEC", "7200"))   # 集成中无更新超此秒数=降级已踢出（456·0=禁用）
排队超时秒 = int(os.environ.get("CN_QUEUE_QUEUED_STALE_SEC", "3600"))   # 562：排队行无写活动超此秒数=降级已踢出（报名会话失联孤儿·0=禁用）
看板服务地址 = os.environ.get("CN_BOARD_URL", "http://127.0.0.1:8301")   # 562：已并入检测数据源（任务台账·同机默认）
# 562：并入检测退避窗口——覆盖「不可达」（URLError/timeout）与「HTTPError 非 404」（5xx/401
#   等看板病态；404=任务不在台账的正常空结果免退避）。检测在 写锁 内同步调用（enqueue/claim
#   热路径·562 修复审查 P2-2）——退避窗口内直接核不了=放行，防「端口在但持续 500」时
#   每次写操作都同步等一次 HTTP、写锁持有期全队列写串行停顿（3s×每 60s 一次全队列卡顿）。
并入检测退避秒 = 60
# 770 develop 平台轮转：游标序与跳平台阈值——轮转=跨头平台轮换（win→linux→arm64 循环）；
#   阈值=轮转限制任务排队超此秒数无人领（该平台全忙/离线）→claim 惰性放宽不限（防平台饿死）。
兜底轮转平台序 = ("win-x64", "linux-x86_64", "linux-arm64")
兜底跳平台阈值秒 = int(os.environ.get("CN_DEVELOP_PLAT_SWITCH_SEC", "300"))
# 830 兜底饥饿保底（2026-10-15 用户裁决·并行化配套）：兜底（develop 头验证）排队行
#   年龄超此秒数 → claim 优先派兜底——防任务级预验长期占满池时 develop 平台轮转验证
#   被无限推迟（原两级查询严格「预验优先」=兜底可被饿死·develop 权威验证名存实亡）。
兜底饥饿秒 = int(os.environ.get("CN_FALLBACK_HUNGER_SEC", "900"))

写锁 = threading.Lock()          # SQLite 写串行化（读靠 WAL 并发）
时区时刻 = lambda: datetime.now().strftime("%m-%d %H:%M:%S")   # 911 教训：秒级戳
# 770：兜底轮转游标（内存态·重启从头轮起无妨）与实例注册表（runner→平台·claim 自报刷新·
#   服务重启重建——冷启动平台有空闲保守 True 不阻塞轮转，限制领不上由阈值放宽兜底）。
兜底轮转游标 = {"i": 0}
实例注册: dict[str, str] = {}


def 行龄秒(更新: str) -> float | None:
    """更新时刻（%m-%d %H:%M:%S 秒级戳·无年份补当前年）距今秒数；坏串=None（宁窄不放）。
    跨年旧行得负差→调用处按「>阈值」比较自然为假=不放宽（同 456 宁漏收口径）。"""
    try:
        return (datetime.now() -
                datetime.strptime(更新, "%m-%d %H:%M:%S").replace(year=datetime.now().year)
                ).total_seconds()
    except ValueError:
        return None


def 平台有空闲(平台: str) -> bool:
    """770：该平台注册实例数 > 该平台执行中行数=有空闲；注册表无该平台实例（冷启动/
    从未 claim）→保守 True（不阻塞轮转——限制照打，领不上由阈值放宽兜底）。须持写锁。"""
    们 = [r for r, p in 实例注册.items() if p == 平台]
    if not 们:
        return True
    占用 = {行[0] for 行 in con.execute(
        "SELECT 认领者 FROM 预验任务 WHERE 状态='执行中' AND 认领者 IS NOT NULL").fetchall()}
    return sum(1 for r in 们 if r in 占用) < len(们)


def 池忙闲快照() -> dict:
    """770：池忙闲（integrate 忙时拼车窗口与巡检的判定源）——注册实例数/执行中数/池全忙。
    全忙=有注册实例且执行中数≥注册数（每台都在跑）；无注册（冷启动）=不算全忙。须持写锁。"""
    执行中 = con.execute(
        "SELECT COUNT(*) FROM 预验任务 WHERE 状态='执行中'").fetchone()[0]
    注册 = len(实例注册)
    return {"注册实例": 注册, "执行中": 执行中,
            "池全忙": bool(注册 and 执行中 >= 注册)}


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
        源分支 TEXT,                    -- 222：发起预验的任务分支（如 任务/217）·旧行 NULL→网页回退显示预验分支名
        状态 TEXT DEFAULT '排队',      -- 排队/执行中/完成
        认领者 TEXT, 心跳时戳 INTEGER,  -- 心跳=服务端收到时刻（免疫客户端时钟漂移）
        绿 INTEGER, 详情 TEXT,          -- 详情=ci_daemon 结果 JSON 原样
        内容键 TEXT,                    -- 830：任务分支 tip sha（预验绿复用键·旧行 NULL）
        入队时刻 TEXT, 更新时刻 TEXT)""")
    try:   # 222 旧库兼容：表已存在时 CREATE IF NOT EXISTS 不加列·ALTER 补（列已在=重复列错吞掉）
        con.execute("ALTER TABLE 预验任务 ADD COLUMN 源分支 TEXT")
    except sqlite3.OperationalError:
        pass
    try:   # 337 认领限制：平台白名单逗号串（NULL=不限·291 混合随机池默认不变）
        con.execute("ALTER TABLE 预验任务 ADD COLUMN 认领限制 TEXT")
    except sqlite3.OperationalError:
        pass
    try:   # 830 内容键：任务分支 tip sha（预验绿复用键·integrate 传·旧行/旧客户端 NULL）
        con.execute("ALTER TABLE 预验任务 ADD COLUMN 内容键 TEXT")
    except sqlite3.OperationalError:
        pass
    con.commit()
    return con


con = 建库()


def 队列快照() -> list[dict]:
    行们 = con.execute("SELECT 分支,基线,报名时刻,写集摘要,状态,更新时刻 FROM 队列 "
                       "ORDER BY 报名时刻").fetchall()
    return [dict(zip(("分支", "基线", "报名时刻", "写集摘要", "状态", "更新时刻"), r)) for r in 行们]


def 门禁快照(限: int = 200) -> list[dict]:
    """814：默认限 20→200（=门禁历史保留同口径）——8301 阶段徽标按收口 sha 在此
    查 ✅ 任务的 develop 验证轮，高频推送日 20 条几小时就滚出窗口（徽标消隐）。"""
    行们 = con.execute("SELECT sha,平台,绿,总秒,时刻 FROM 门禁 "
                       "ORDER BY 时刻 DESC LIMIT ?", (限,)).fetchall()
    return [dict(zip(("sha", "平台", "绿", "总秒", "时刻"), r)) for r in 行们]


def 预验任务快照() -> list[dict]:
    """预验任务池快照（不含 详情 大字段——integrate 取详情走 /api/task_result）。"""
    行们 = con.execute("SELECT 序号,分支,sha,源分支,状态,认领者,绿,认领限制,入队时刻,更新时刻,内容键 "
                       "FROM 预验任务 ORDER BY 序号 DESC LIMIT 50").fetchall()
    return [dict(zip(("序号", "分支", "sha", "源分支", "状态", "认领者", "绿", "认领限制",
                      "入队时刻", "更新时刻", "内容键"), r))
            for r in 行们]


def 超时死行(状态: str, 阈值秒: int) -> list[tuple[str, str, float]]:
    """取指定状态队列行中 更新时刻超阈值的（分支, 更新时刻, 静默秒）。
    更新时刻=时区时刻() 无年份（911 秒级戳）——补当前年解析；跨年旧行得负差
    →视为未知跳过（宁漏收不误收；元旦后一天的跨年行等下轮人工）。调用方须持写锁。"""
    出: list[tuple[str, str, float]] = []
    现在 = datetime.now()
    for 分支, 更新 in con.execute(
            "SELECT 分支,更新时刻 FROM 队列 WHERE 状态=?", (状态,)).fetchall():
        try:
            差 = (现在 - datetime.strptime(更新, "%m-%d %H:%M:%S").replace(year=现在.year)
                  ).total_seconds()
        except ValueError:
            continue
        if 0 <= 差 and 差 > 阈值秒:
            出.append((分支, 更新, 差))
    return 出


def 回收死行() -> None:
    """456+562：队列死行回收——会话死亡不能永挂队列（批主选举 min(排队) 被孤儿卡死=
    全员死等·507/476/495 等八行报名后失联卡队列数小时实录）。
    「集成中」超 集成中超时秒（默认 2h）→已踢出（456·批主 562 起有心跳维持新鲜，
    活批不误收）；「排队」超 排队超时秒（默认 1h）→已踢出降权（562·心跳活着的行
    更新时刻常新不回收=492「不误删活跃会话」守卫；误收无损——join 重报即回排队
    （1020 语义）·--wait 等待者 touch 自愈（1020 已有））。调用方须持写锁。"""
    if 集成中超时秒 > 0:
        for 分支, 更新, 差 in 超时死行("集成中", 集成中超时秒):
            con.execute("UPDATE 队列 SET 状态='已踢出',更新时刻=? WHERE 分支=?",
                        (时区时刻(), 分支))
            con.commit()   # 自提交：调用方（update 无此行等）可能提前 return
            print("[回收] 集成中超时 %s（更新 %s·静默 %.0f 分钟）→已踢出"
                  % (分支, 更新, 差 / 60), flush=True)
    if 排队超时秒 > 0:
        for 分支, 更新, 差 in 超时死行("排队", 排队超时秒):
            con.execute("UPDATE 队列 SET 状态='已踢出',更新时刻=? WHERE 分支=?",
                        (时区时刻(), 分支))
            con.commit()
            print("[回收] 排队超时 %s（更新 %s·静默 %.0f 分钟）→已踢出降权（562 孤儿回收）"
                  % (分支, 更新, 差 / 60), flush=True)


_并入检测上次失败 = [0.0]   # 562：看板故障退避戳（time.time()）——退避窗口内核不了=放行


def 并入检测退避否(e: Exception) -> bool:
    """562 修复（审查 P2-2）：并入检测异常是否置退避戳——仅 404 免退避（任务不在台账=
    服务可达的正常空结果，重试必有同解），其余全退避：URLError/timeout=不可达（既有
    语义）·HTTPError 5xx/401 等=看板病态（562 修复点：旧路径 HTTPError 一律不退避，
    「端口在但持续 500」时写锁内每次同步等一次 HTTP=全队列写串行停顿）。纯函数（selftest 面）。"""
    if isinstance(e, urllib.error.HTTPError):
        return e.code != 404
    return True


def 源分支已并入(源分支: str) -> bool | None:
    """562：核预验任务源分支是否已并入 develop——数据源=看板任务台账（GET /api/task/<号>
    只读免鉴权）：状态 ✅=integrate 成功销账（✅⇔sha 铁律）=源分支已并入 develop 的
    服务端唯一权威信号。返回 True=已并入；False=未并入；None=核不了（源分支空/非
    任务/<号> 格式/任务不在台账/看板不可达——宁放行不误杀，不堵队列派发）。"""
    # 830：兼容两形态——任务/<号>（源头归一）与 batch/<号>（历史批链行·830 前客户端传批分支名）
    m = re.match(r"^(?:任务|batch)/([0-9]+[a-z]?)$", (源分支 or "").strip())
    if not m:
        return None
    if time.time() - _并入检测上次失败[0] < 并入检测退避秒:
        return None   # 退避窗口（上一次故障）——直接放行
    try:
        with urllib.request.urlopen(看板服务地址.rstrip("/") + "/api/task/" + m.group(1),
                                    timeout=3) as 响应:
            任务 = json.loads(响应.read().decode("utf-8")).get("任务") or {}
    except Exception as e:
        if 并入检测退避否(e):   # 仅 404 免退避·其余全退避（审查 P2-2·见函数注释）
            _并入检测上次失败[0] = time.time()   # 看板故障——退避·防写锁内反复同步等 HTTP
        return None
    if not 任务:
        return None
    return 任务.get("状态") == "✅"


def 处理写(名: str, 数据: dict) -> dict:
    """统一写入口（写锁内）——返回 {ok, 说明}。"""
    with 写锁:
        回收死行()   # 456+562：任何写活动顺手扫集成中/排队死行（会话死亡后靠他批写活动触发）
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
        elif 名 == "heartbeat":
            # 562 批主/等待者心跳：只刷新更新时刻（活会话证明·不改状态不刷报名时刻=保序）
            #   ——批主集成中/成员 --wait 等待期间定期调，服务端回收与客户端 --wait
            #   接管判定以心跳新鲜度为据（治 444：win 全量门禁 40+ 分钟无行更新被误判失联）。
            #   行不在=ok False（已被销账/踢出——心跳方静默忽略）。
            分支 = str(数据.get("分支", ""))
            cur = con.execute("UPDATE 队列 SET 更新时刻=? WHERE 分支=?",
                              (时区时刻(), 分支))
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
            源分支 = str(数据.get("源分支", "")) or None   # 222：发起预验的任务分支（旧客户端不带=NULL）
            # 337 认领限制：平台白名单逗号串（如 win-x64,linux-x86_64）——NULL/空=不限（291 混合池默认）
            认领限制 = str(数据.get("认领限制", "")).strip() or None
            # 830 内容键：任务分支 tip sha（预验绿复用键·integrate 传·旧客户端 NULL/空）
            内容键 = str(数据.get("内容键", "")).strip() or None
            # 189 兜底入池：放宽 ci/兜底- 前缀（develop 兜底任务经池派发·非真 git 分支）
            if not (分支.startswith("ci/预验-") or 分支.startswith("ci/兜底-")) or len(sha) < 7:
                return {"ok": False, "说明": "分支须 ci/预验-/ci/兜底- 开头且 sha 合法"}
            已有 = con.execute("SELECT 状态,绿 FROM 预验任务 WHERE 分支=?", (分支,)).fetchone()
            if 已有 is None:
                # 562 已并入检测：源分支已并入 develop（台账 ✅ 销账）=预验无意义——
                #   拒绝入池（防矛盾场景与防御性重入队复活）；None/False=放行（宁漏检不误杀）。
                if 源分支已并入(源分支) is True:
                    return {"ok": False,
                            "说明": "源分支 %s 已并入 develop（台账已销账）——预验无意义·拒绝入池（562）" % 源分支}
                if 分支.startswith("ci/兜底-"):
                    # 770 develop 平台轮转：游标 win→linux-x86_64→linux-arm64 循环指派平台——
                    #   该平台有空闲→限制=该平台（轮转语义落位·平台内随机空闲由认领竞争天然形成）；
                    #   该平台全忙→限制=NULL（放宽·先空闲先领=用户裁决「顺序由下一轮平台执行」）。
                    #   客户端传的限制对兜底一律忽略（服务端轮转唯一权威）；行饿死由 claim 阈值放宽兜底。
                    #   打标仅在新建行时（幂等重发不空推游标）。
                    轮平台 = 兜底轮转平台序[兜底轮转游标["i"] % len(兜底轮转平台序)]
                    兜底轮转游标["i"] += 1
                    认领限制 = 轮平台 if 平台有空闲(轮平台) else None
                    print("[兜底轮转] %s → %s（%s）" % (分支, 轮平台,
                          "指派" if 认领限制 else "全忙放宽不限"), flush=True)
                    # 830 陈头去重：新头入池=更老的「排队中」兜底行直接取消——最新头内容
                    #   包含旧头（线性历史），旧头无须再验（只在最新头的基础上验一次·用户
                    #   裁决「在新 sha 基础上验证」）；执行中的旧头不动（让它跑完·跑前
                    #   「过期跳过」已兜）。减少池占坑。
                    cur = con.execute("DELETE FROM 预验任务 WHERE 状态='排队' "
                                      "AND 分支 LIKE 'ci/兜底-%' AND 分支 != ?", (分支,))
                    if cur.rowcount:
                        print("[兜底去重] 新头 %s 入池——取消更老的排队兜底行 %d 条（只验最新头·830）"
                              % (分支, cur.rowcount), flush=True)
                con.execute("INSERT INTO 预验任务(分支,sha,源分支,认领限制,内容键,状态,入队时刻,更新时刻) "
                            "VALUES(?,?,?,?,?,'排队',?,?)",
                            (分支, sha, 源分支, 认领限制, 内容键, 时区时刻(), 时区时刻()))
            elif 已有[0] == "完成" and not 已有[1] and not 分支.startswith("ci/兜底-"):
                # 189：兜底任务完成红=终态（红结果已上门禁表·不重置）——防池内死循环重跑
                #   （TX_02 下一周期 enqueue 幂等直达、runner 不再被派）；预验维持红=重置重跑
                #   （同 sha CAS 重试路径·给 flaky 一次机会）。337：认领限制随重发刷新（发起方最新意图为准）。
                con.execute("UPDATE 预验任务 SET 状态='排队',认领者=NULL,心跳时戳=NULL,"
                            "绿=NULL,详情=NULL,源分支=COALESCE(?,源分支),认领限制=?,"
                            "内容键=COALESCE(?,内容键),更新时刻=? WHERE 分支=?",
                            (源分支, 认领限制, 内容键, 时区时刻(), 分支))
        elif 名 == "task_claim":
            # 认领（写锁内原子）：①回收心跳超时的执行中任务→重置排队；②取最旧排队任务置执行中。
            runner = str(数据.get("runner", ""))
            if not runner:
                return {"ok": False, "说明": "runner 标识缺失"}
            # 337 认领限制：runner 自报平台（win-x64/linux-x86_64/linux-arm64·294 三分）——
            #   只领「不限」或白名单含本平台的任务；旧客户端不报=空串→只领不限任务（不会错跑限制任务）。
            平台337 = str(数据.get("平台", ""))
            if 平台337:
                实例注册[runner] = 平台337   # 770：注册表刷新（平台忙闲判定与池全忙快照的数据源）
            现在 = int(time.time())
            con.execute("UPDATE 预验任务 SET 状态='排队',认领者=NULL,心跳时戳=NULL,更新时刻=? "
                        "WHERE 状态='执行中' AND 心跳时戳 IS NOT NULL AND 心跳时戳<?",
                        (时区时刻(), 现在 - 心跳超时秒))
            # 770 跳平台兜底：轮转限制的兜底任务排队超阈值仍无人领（该平台全忙/离线饿死）→
            #   限制惰性放宽 NULL——任何平台空闲实例可领（用户裁决「该平台全忙由下一轮平台执行」
            #   的运行期兜底形态；入池时的即时放宽是快路径，此处是慢路径保险）。坏串行龄 None=不动。
            for 分支770, 更新770 in con.execute(
                    "SELECT 分支,更新时刻 FROM 预验任务 WHERE 状态='排队' AND 分支 LIKE 'ci/兜底-%' "
                    "AND 认领限制 IS NOT NULL AND 认领限制!=''").fetchall():
                龄 = 行龄秒(更新770)
                if 龄 is not None and 龄 > 兜底跳平台阈值秒:
                    con.execute("UPDATE 预验任务 SET 认领限制=NULL,更新时刻=? WHERE 分支=?",
                                (时区时刻(), 分支770))
                    print("[兜底放宽] %s 排队超 %ds 无人领——限制放宽不限（770 跳平台兜底）"
                          % (分支770, 兜底跳平台阈值秒), flush=True)
            # 189/770 兜底池化：预验任务优先认领（集成吞吐保序）——池内同存 ci/预验-* 与
            #   ci/兜底-* 时先派预验（兜底=develop 已入库的稳态验证·晚跑无损）；无预验排队
            #   才派兜底。零 schema 变更=按分支前缀两级查询。337：两级查询同带认领限制过滤
            #   （兜底限制=770 服务端轮转平台·全忙/超阈值放宽 NULL）。
            认领过滤337 = ("AND (认领限制 IS NULL OR 认领限制='' "
                           "OR ','||认领限制||',' LIKE '%,'||?||',%')")
            # 830 兜底饥饿保底（2026-10-15 用户裁决·并行化配套）：兜底排队行年龄超阈值 →
            #   本次认领先派兜底（develop 平台轮转验证保底——防任务级预验长期占满池把
            #   develop 权威验证无限饿死）。阈值内维持「预验优先」两级保序（189/770 语义不变）。
            兜底饥饿行 = None
            for 行h in con.execute(
                    "SELECT 分支,sha,源分支,更新时刻 FROM 预验任务 WHERE 状态='排队' "
                    "AND 分支 LIKE 'ci/兜底-%' " + 认领过滤337 + " ORDER BY 序号",
                    (平台337,)).fetchall():
                龄h = 行龄秒(行h[3])
                if 龄h is not None and 龄h > 兜底饥饿秒:
                    兜底饥饿行 = (行h[0], 行h[1], 行h[2])
                    break
            if 兜底饥饿行 is not None:
                if 源分支已并入(兜底饥饿行[2]) is True:
                    con.execute("UPDATE 预验任务 SET 状态='已取消',更新时刻=? WHERE 分支=?",
                                (时区时刻(), 兜底饥饿行[0]))
                    con.commit()   # 自提交：作废后落回正常两级派发
                    print("[取消] 兜底饥饿行 %s 源分支已并入 develop——作废落回正常派发（562）"
                          % 兜底饥饿行[0], flush=True)
                else:
                    con.execute("UPDATE 预验任务 SET 状态='执行中',认领者=?,心跳时戳=?,更新时刻=? "
                                "WHERE 分支=?",
                                (runner, 现在, 时区时刻(), 兜底饥饿行[0]))
                    print("[兜底饥饿] 兜底排队超 %ds——优先派 %s（830 保底：develop 验证不被预验饿死）"
                          % (兜底饥饿秒, 兜底饥饿行[0]), flush=True)
                    return {"ok": True, "任务": {"分支": 兜底饥饿行[0], "sha": 兜底饥饿行[1]}}
            # 562 已并入检测：选出任务后核源分支——已并入 develop（台账 ✅）=孤儿预验
            #   （任务已集成销账·555/556 实录：回收后反复重派执行浪费池容量）→置「已取消」
            #   不派发·继续选下一个；None/False=照常派发（宁漏检不误杀）。已取消行不参与
            #   认领/不复活（enqueue 见已存在不动）·随完成行裁剪清理（防堆积）。
            for _ in range(64):   # 防呆上限：极端全取消场景不空转
                行 = con.execute("SELECT 分支,sha,源分支 FROM 预验任务 WHERE 状态='排队' AND 分支 LIKE 'ci/预验-%' "
                                 + 认领过滤337 + " ORDER BY 序号 LIMIT 1",
                                 (平台337,)).fetchone()
                if 行 is None:
                    行 = con.execute("SELECT 分支,sha,源分支 FROM 预验任务 WHERE 状态='排队' "
                                     + 认领过滤337 + " ORDER BY 序号 LIMIT 1",
                                     (平台337,)).fetchone()
                if 行 is None:
                    break
                if 源分支已并入(行[2]) is True:
                    con.execute("UPDATE 预验任务 SET 状态='已取消',更新时刻=? WHERE 分支=?",
                                (时区时刻(), 行[0]))
                    con.commit()   # 自提交：继续选下一个不受影响
                    print("[取消] 预验 %s 源分支 %s 已并入 develop（台账销账）——任务作废不再派发（562）"
                          % (行[0], 行[2]), flush=True)
                    continue
                con.execute("UPDATE 预验任务 SET 状态='执行中',认领者=?,心跳时戳=?,更新时刻=? "
                            "WHERE 分支=?", (runner, 现在, 时区时刻(), 行[0]))
                return {"ok": True, "任务": {"分支": 行[0], "sha": 行[1]}}
            return {"ok": True, "任务": None}
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
            con.execute("DELETE FROM 预验任务 WHERE 状态 IN ('完成','已取消') AND 序号 NOT IN "
                        "(SELECT 序号 FROM 预验任务 WHERE 状态 IN ('完成','已取消') "
                        "ORDER BY 序号 DESC LIMIT ?)",
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
<body><h2>最新门禁（develop 每头必验·win→linux→arm64 轮转·770）</h2>__门禁表__
<h2>验证池（预验=全池随机空闲·develop=平台轮转·1021/770）</h2>__任务表__
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
        # 561：fail-closed（对齐看板 430）——令牌未配置=写操作全拒（旧址 令牌=="" 全放行）
        return bool(令牌) and self.headers.get("Authorization", "") == "Bearer " + 令牌

    def do_GET(self):
        from urllib.parse import parse_qs
        路径 = urlparse(self.path).path
        if 路径 == "/api/state":
            with 写锁:
                回收死行()   # 456+562：批主/报名会话死亡后无写活动，靠轮询/网页查看触发回收
                return self.回JSON({"队列": 队列快照(), "门禁最近": 门禁快照(),
                                    "预验任务": 预验任务快照(),
                                    "池忙闲": 池忙闲快照()})   # 770：integrate 拼车窗口/巡检判定源
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
                "<tr><td>%s</td><td>%s</td><td>%s</td><td class='%s'>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>"
                % (r["源分支"] or r["分支"], r["sha"][:10], r["状态"],
                   ("绿" if r["绿"] else "红") if r["状态"] == "完成" and r["绿"] is not None else "",
                   ("绿" if r["绿"] else "红") if r["状态"] == "完成" and r["绿"] is not None else "—",
                   r["认领者"] or "—", r["认领限制"] or "不限", r["入队时刻"], r["更新时刻"])
                for r in 预验任务快照()) or "<tr><td colspan=8>空</td></tr>"
            页 = (HTML页.replace("__门禁表__", "<table><tr><th>SHA</th><th>平台</th><th>结果</th>"
                  "<th>耗时</th><th>时刻</th></tr>" + 门禁行 + "</table>")
                  .replace("__任务表__", "<table><tr><th>分支</th><th>SHA</th><th>状态</th><th>结果</th>"
                           "<th>认领者</th><th>认领限制</th><th>入队</th><th>更新</th></tr>" + 任务行 + "</table>")
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

        # ── 456 集成中失联回收：超时降级已踢出/未超时保护/坏串宁漏收 ──
        处理写("join", {"分支": "任务/乙-2-y", "基线": "aaa", "写集摘要": "1 文件"})
        处理写("update", {"分支": "任务/乙-2-y", "状态": "集成中"})
        旧戳 = (datetime.now() - timedelta(seconds=集成中超时秒 * 2)).strftime("%m-%d %H:%M:%S")
        con.execute("UPDATE 队列 SET 更新时刻=? WHERE 分支='任务/乙-2-y'", (旧戳,))
        处理写("join", {"分支": "任务/丁-4-w", "基线": "a", "写集摘要": ""})   # 无关行写活动触发扫描
        行乙 = next(r2 for r2 in 队列快照() if r2["分支"] == "任务/乙-2-y")
        行甲 = next(r2 for r2 in 队列快照() if r2["分支"] == "任务/甲-1-x")
        断言(行乙["状态"] == "已踢出", "456 集成中超 2 倍阈值=回收降级已踢出")
        断言(行甲["状态"] == "集成中", "456 同扫未超时行=保护不动（不误伤在飞批主）")
        r = 处理写("join", {"分支": "任务/乙-2-y", "基线": "bbb", "写集摘要": "2 文件"})
        断言("原态 已踢出" in r.get("说明", ""), "456 回收行批主 join=重报回排队（1020 语义兼容）")
        处理写("join", {"分支": "任务/丙-3-z", "基线": "a", "写集摘要": ""})
        处理写("update", {"分支": "任务/丙-3-z", "状态": "集成中"})
        con.execute("UPDATE 队列 SET 更新时刻='坏串' WHERE 分支='任务/丙-3-z'")
        处理写("join", {"分支": "任务/戊-5-v", "基线": "a", "写集摘要": ""})   # 触发扫描
        断言(next(r2 for r2 in 队列快照() if r2["分支"] == "任务/丙-3-z")["状态"] == "集成中",
             "456 更新时刻坏串=宁漏收不误收且不炸服务")
        处理写("update", {"分支": "任务/甲-1-x", "状态": "冲突出批"})
        r = 处理写("join", {"分支": "任务/甲-1-x", "基线": "eee", "写集摘要": "6 文件"})
        断言(队列快照()[0]["状态"] == "排队", "冲突出批重报=回排队")
        r = 处理写("join", {"分支": "错误名", "基线": "x", "写集摘要": ""})
        断言(not r["ok"], "非任务/ 分支拒绝")

        # ── 189/770 兜底池化：入队放宽/claim 预验优先/兜底红终态（轮转语义见下 770 段） ──
        r = 处理写("task_enqueue", {"分支": "ci/兜底-aaaa000001", "sha": "a" * 40})
        断言(r["ok"], "189 兜底任务入队放宽（ci/兜底- 前缀）")
        行770a = next(r2 for r2 in 预验任务快照() if r2["分支"] == "ci/兜底-aaaa000001")
        断言(行770a["认领限制"] == "win-x64",
             "770 轮转首棒=win-x64（注册表冷启动保守视为有空闲·限制照打）")
        处理写("task_enqueue", {"分支": "ci/预验-aaaa000002", "sha": "b" * 40})
        r = 处理写("task_enqueue", {"分支": "develop", "sha": "c" * 40})
        断言(not r["ok"], "189 非法前缀仍拒（develop 裸名不入池）")
        t = 处理写("task_claim", {"runner": "r1"})["任务"]
        断言(t and t["分支"] == "ci/预验-aaaa000002", "189 claim 预验优先（集成吞吐保序）")
        t = 处理写("task_claim", {"runner": "r2", "平台": "win-x64"})["任务"]
        断言(t and t["分支"] == "ci/兜底-aaaa000001", "189 预验空后派兜底（770 轮转平台实例领到）")
        断言(处理写("task_claim", {"runner": "r3"})["任务"] is None, "189 池空")
        处理写("task_complete", {"runner": "r2", "分支": "ci/兜底-aaaa000001", "绿": False,
                                 "结果": {"绿": False}})
        处理写("task_enqueue", {"分支": "ci/兜底-aaaa000001", "sha": "a" * 40})
        断言(处理写("task_claim", {"runner": "r4", "平台": "win-x64"})["任务"] is None,
             "189 兜底完成红=终态不重置（防池内死循环重跑）")
        处理写("task_complete", {"runner": "r1", "分支": "ci/预验-aaaa000002", "绿": False,
                                 "结果": {"绿": False}})
        处理写("task_enqueue", {"分支": "ci/预验-aaaa000002", "sha": "b" * 40})
        t = 处理写("task_claim", {"runner": "r5"})["任务"]
        断言(t and t["分支"] == "ci/预验-aaaa000002", "189 预验完成红维持重置重跑（flaky 机会·原语义）")
        处理写("task_complete", {"runner": "r5", "分支": "ci/预验-aaaa000002", "绿": True,
                                 "结果": {"绿": True}})

        # ── 337 认领限制：平台白名单（runner 自报平台过滤·旧客户端只领不限任务） ──
        处理写("task_enqueue", {"分支": "ci/预验-bbbb000003", "sha": "d" * 40,
                                "认领限制": "win-x64,linux-x86_64"})
        行337 = next(r2 for r2 in 预验任务快照() if r2["分支"] == "ci/预验-bbbb000003")
        断言(行337["认领限制"] == "win-x64,linux-x86_64", "337 入队白名单已存（快照可见）")
        断言(处理写("task_claim", {"runner": "r_arm", "平台": "linux-arm64"})["任务"] is None,
             "337 白名单不含 arm64=arm64 runner 领不到（335 环境红定向规避）")
        断言(处理写("task_claim", {"runner": "r_old"})["任务"] is None,
             "337 旧客户端不报平台=只领不限任务（不会错跑限制任务）")
        t = 处理写("task_claim", {"runner": "r_win", "平台": "win-x64"})["任务"]
        断言(t and t["分支"] == "ci/预验-bbbb000003", "337 白名单含 win-x64=win runner 领到")
        处理写("task_complete", {"runner": "r_win", "分支": "ci/预验-bbbb000003", "绿": False,
                                 "结果": {"绿": False}})
        处理写("task_enqueue", {"分支": "ci/预验-bbbb000003", "sha": "d" * 40})   # 重发不带限制→刷新不限
        t = 处理写("task_claim", {"runner": "r_arm", "平台": "linux-arm64"})["任务"]
        断言(t and t["分支"] == "ci/预验-bbbb000003", "337 完成红重发不带限制=刷新为不限（发起方最新意图）")
        处理写("task_complete", {"runner": "r_arm", "分支": "ci/预验-bbbb000003", "绿": True,
                                 "结果": {"绿": True}})   # 770 清场：防占用 arm64 忙闲判定影响轮转断言
        # ── 770 develop 平台轮转：客户端限制忽略/游标循环（win→linux→arm64）/池忙闲 ──
        处理写("task_enqueue", {"分支": "ci/兜底-cccc000004", "sha": "e" * 40, "认领限制": "win-x64"})
        行770 = next(r2 for r2 in 预验任务快照() if r2["分支"] == "ci/兜底-cccc000004")
        断言(行770["认领限制"] == "linux-x86_64",
             "770 兜底客户端限制被忽略（服务端轮转唯一权威）·游标第二棒=linux-x86_64")
        断言(处理写("task_claim", {"runner": "r_win", "平台": "win-x64"})["任务"] is None,
             "770 非轮转平台领不到轮转限制任务（win 棒已过）")
        t = 处理写("task_claim", {"runner": "r_linux", "平台": "linux-x86_64"})["任务"]
        断言(t and t["分支"] == "ci/兜底-cccc000004",
             "770 轮转平台空闲实例领到（平台内随机=认领竞争天然形成）")
        处理写("task_complete", {"runner": "r_linux", "分支": "ci/兜底-cccc000004", "绿": True,
                                 "结果": {"绿": True}})
        处理写("task_enqueue", {"分支": "ci/兜底-dddd000005", "sha": "f" * 40})   # daemon 路径=不带限制
        行770b = next(r2 for r2 in 预验任务快照() if r2["分支"] == "ci/兜底-dddd000005")
        断言(行770b["认领限制"] == "linux-arm64", "770 游标第三棒=linux-arm64（win→linux→arm64 循环）")
        t = 处理写("task_claim", {"runner": "r_arm", "平台": "linux-arm64"})["任务"]
        断言(t and t["分支"] == "ci/兜底-dddd000005", "770 arm64 轮转棒领到")
        处理写("task_complete", {"runner": "r_arm", "分支": "ci/兜底-dddd000005", "绿": True,
                                 "结果": {"绿": True}})
        处理写("task_enqueue", {"分支": "ci/兜底-eeee000006", "sha": "6" * 40})
        行770c = next(r2 for r2 in 预验任务快照() if r2["分支"] == "ci/兜底-eeee000006")
        断言(行770c["认领限制"] == "win-x64", "770 游标循环回归=第四棒回 win-x64")
        t = 处理写("task_claim", {"runner": "r_win2", "平台": "win-x64"})["任务"]
        断言(t and t["分支"] == "ci/兜底-eeee000006", "770 win 棒回归后 win 实例领到")
        处理写("task_complete", {"runner": "r_win2", "分支": "ci/兜底-eeee000006", "绿": True,
                                 "结果": {"绿": True}})

        # ── 770 平台全忙→限制放宽 NULL（顺序由下一平台执行·用户裁决） ──
        处理写("task_enqueue", {"分支": "ci/预验-g111111111", "sha": "7" * 40})
        处理写("task_enqueue", {"分支": "ci/预验-g222222222", "sha": "8" * 40})
        t = 处理写("task_claim", {"runner": "w1", "平台": "win-x64"})["任务"]
        断言(t and t["分支"] == "ci/预验-g111111111", "770 前置：w1 注册 win 并领预验（预验优先）")
        t = 处理写("task_claim", {"runner": "w2", "平台": "win-x64"})["任务"]
        断言(t and t["分支"] == "ci/预验-g222222222", "770 前置：w2 注册 win 并领预验")
        兜底轮转游标["i"] = 0   # 游标重置到 win 棒（直控内存态·测全忙分支）
        for _虚 in ("r2", "r4", "r_win", "r_win2"):   # 清前段虚构 win 实例·隔离「win 2/2 全忙」场景
            实例注册.pop(_虚, None)
        处理写("task_enqueue", {"分支": "ci/兜底-ffff000007", "sha": "9" * 40})
        行770d = next(r2 for r2 in 预验任务快照() if r2["分支"] == "ci/兜底-ffff000007")
        断言(行770d["认领限制"] is None,
             "770 轮转平台全忙（win 2/2 执行中）→限制放宽 NULL=顺序由下一平台执行")
        t = 处理写("task_claim", {"runner": "l1", "平台": "linux-x86_64"})["任务"]
        断言(t and t["分支"] == "ci/兜底-ffff000007", "770 放宽后 linux 空闲实例领到 develop 验证")
        处理写("task_complete", {"runner": "l1", "分支": "ci/兜底-ffff000007", "绿": True,
                                 "结果": {"绿": True}})

        # ── 770 轮转限制排队超阈值→claim 惰性放宽（平台饿死兜底·win 可领 linux 棒） ──
        兜底轮转游标["i"] = 1   # 游标重置到 linux 棒
        处理写("task_enqueue", {"分支": "ci/兜底-h111111111", "sha": "a" * 40})
        行770e = next(r2 for r2 in 预验任务快照() if r2["分支"] == "ci/兜底-h111111111")
        断言(行770e["认领限制"] == "linux-x86_64", "770 前置：限制=linux-x86_64（游标棒位）")
        旧戳770 = (datetime.now() - timedelta(seconds=兜底跳平台阈值秒 * 2)).strftime("%m-%d %H:%M:%S")
        con.execute("UPDATE 预验任务 SET 更新时刻=? WHERE 分支='ci/兜底-h111111111'", (旧戳770,))
        t = 处理写("task_claim", {"runner": "w3", "平台": "win-x64"})["任务"]
        断言(t and t["分支"] == "ci/兜底-h111111111",
             "770 排队超阈值无人领→claim 惰性放宽不限（跨平台领到·坏串/未超时不动）")
        处理写("task_complete", {"runner": "w3", "分支": "ci/兜底-h111111111", "绿": True,
                                 "结果": {"绿": True}})
        忙闲770 = 池忙闲快照()
        断言(忙闲770["注册实例"] >= 5 and 忙闲770["执行中"] >= 2,
             "770 池忙闲快照：注册实例/执行中字段可用（562 执行中行+win 预验行）")
        断言(忙闲770["池全忙"] is False, "770 池全忙判定=执行中<注册（此景存在空闲实例）")

        # ── 830 兜底饥饿保底 + 陈头去重 + 内容键（2026-10-15 用户裁决·并行化配套）──
        处理写("task_enqueue", {"分支": "ci/预验-8308308300", "sha": "a" * 40,
                               "内容键": "k" * 40})
        行830k = next(r2 for r2 in 预验任务快照() if r2["分支"] == "ci/预验-8308308300")
        断言(行830k["内容键"] == "k" * 40, "830 内容键入队存取（预验绿复用键·快照可见）")
        # 饥饿保底：兜底排队行年龄超阈值 → 优先于更早序号的预验排队行派发
        处理写("task_enqueue", {"分支": "ci/兜底-830aaa0001", "sha": "b" * 40})
        con.execute("UPDATE 预验任务 SET 认领限制=NULL, 更新时刻=? WHERE 分支='ci/兜底-830aaa0001'",
                    ((datetime.now() - timedelta(seconds=兜底饥饿秒 * 2)).strftime("%m-%d %H:%M:%S"),))
        处理写("task_claim", {"runner": "r830a"})   # 不带平台=只领不限行（上面已置 NULL）
        行饿 = next((r2 for r2 in 预验任务快照()
                    if r2["分支"] == "ci/兜底-830aaa0001"), None)
        断言(行饿 and 行饿["状态"] == "执行中" and 行饿["认领者"] == "r830a",
             "830 兜底饥饿保底：兜底排队超阈值优先派（压过更早序号的预验排队行）")
        处理写("task_complete", {"runner": "r830a", "分支": "ci/兜底-830aaa0001",
                                "绿": True, "结果": {"绿": True}})
        # 陈头去重：新头入池取消更老的排队兜底行
        处理写("task_enqueue", {"分支": "ci/兜底-830bbb0002", "sha": "c" * 40})
        处理写("task_enqueue", {"分支": "ci/兜底-830ccc0003", "sha": "d" * 40})
        旧行830 = next((r2 for r2 in 预验任务快照() if r2["分支"] == "ci/兜底-830bbb0002"), None)
        新行830 = next((r2 for r2 in 预验任务快照() if r2["分支"] == "ci/兜底-830ccc0003"), None)
        断言(旧行830 is None and 新行830 is not None,
             "830 陈头去重：新头入池取消更老的排队兜底行（只验最新头·线性历史包含）")
        # 清场：饥饿已消费 → 回归预验优先保序；两行分别领掉完成（保持池净）
        con.execute("UPDATE 预验任务 SET 认领限制=NULL WHERE 分支 IN "
                    "('ci/兜底-830ccc0003','ci/预验-8308308300')")
        处理写("task_claim", {"runner": "r830b"})
        领b = next((r2 for r2 in 预验任务快照() if r2["分支"] == "ci/预验-8308308300"), None)
        断言(领b and 领b["状态"] == "执行中" and 领b["认领者"] == "r830b",
             "830 清场：饥饿兜底已消费·预验优先保序回归（领到预验行）")
        处理写("task_complete", {"runner": "r830b", "分支": "ci/预验-8308308300",
                                "绿": True, "结果": {"绿": True}})
        处理写("task_claim", {"runner": "r830c"})
        处理写("task_complete", {"runner": "r830c", "分支": "ci/兜底-830ccc0003",
                                "绿": True, "结果": {"绿": True}})

        # ── 562 批主心跳：heartbeat=只刷更新时刻·不改状态——活批不被回收误杀（444 根治） ──
        #   （真实批主心跳 2 分钟≪集成中超时 2h·行时刻恒新鲜；此处直接断言心跳语义本身）
        旧戳562 = (datetime.now() - timedelta(seconds=集成中超时秒 * 2)).strftime("%m-%d %H:%M:%S")
        处理写("join", {"分支": "任务/心-6-p", "基线": "a", "写集摘要": ""})
        处理写("update", {"分支": "任务/心-6-p", "状态": "集成中"})
        断言(处理写("heartbeat", {"分支": "任务/心-6-p"})["ok"], "562 heartbeat 集成中行=ok")
        _time = __import__("time")
        _time.sleep(1.1)   # 秒级戳——跨秒才能比出刷新
        断言(处理写("heartbeat", {"分支": "任务/心-6-p"})["ok"], "562 heartbeat 幂等可反复调")
        行心 = next(r2 for r2 in 队列快照() if r2["分支"] == "任务/心-6-p")
        断言(行心["状态"] == "集成中" and 行心["更新时刻"] > 旧戳562,
             "562 心跳刷新更新时刻且不改状态（长门禁活批证明·444 根治）")
        处理写("join", {"分支": "任务/心-6-q", "基线": "a", "写集摘要": ""})   # 无关写活动触发回收扫描
        断言(next(r2 for r2 in 队列快照() if r2["分支"] == "任务/心-6-p")["状态"] == "集成中",
             "562 心跳维持的活批过扫描不误收（456 回收以更新时刻为据）")
        断言(not 处理写("heartbeat", {"分支": "任务/无此行"})["ok"],
             "562 heartbeat 无此行=ok False（心跳方静默忽略）")

        # ── 562 排队行孤儿回收：超时降级已踢出（批主选举不再被失联行卡死）·心跳保活不误收 ──
        处理写("join", {"分支": "任务/孤-7-r", "基线": "a", "写集摘要": ""})
        孤旧戳 = (datetime.now() - timedelta(seconds=排队超时秒 * 2)).strftime("%m-%d %H:%M:%S")
        con.execute("UPDATE 队列 SET 更新时刻=? WHERE 分支='任务/孤-7-r'", (孤旧戳,))
        处理写("join", {"分支": "任务/孤-7-s", "基线": "a", "写集摘要": ""})   # 活跃行+触发扫描
        断言(next(r2 for r2 in 队列快照() if r2["分支"] == "任务/孤-7-r")["状态"] == "已踢出",
             "562 排队行超时无写活动=降级已踢出（507/476 孤儿不再卡队列）")
        断言(next(r2 for r2 in 队列快照() if r2["分支"] == "任务/孤-7-s")["状态"] == "排队",
             "562 新报名行未超时=保护不动（492 不误删活跃会话守卫）")
        处理写("join", {"分支": "任务/孤-7-t", "基线": "a", "写集摘要": ""})
        _time.sleep(1.1)
        处理写("heartbeat", {"分支": "任务/孤-7-t"})   # 等待者心跳保活
        行孤 = next(r2 for r2 in 队列快照() if r2["分支"] == "任务/孤-7-t")
        断言(行孤["状态"] == "排队" and 行孤["更新时刻"] > 孤旧戳,
             "562 等待者心跳保活=更新时刻恒新鲜（排队回收不误收·492 守卫）")
        处理写("join", {"分支": "任务/孤-7-u", "基线": "a", "写集摘要": ""})   # 触发扫描
        断言(next(r2 for r2 in 队列快照() if r2["分支"] == "任务/孤-7-t")["状态"] == "排队",
             "562 心跳保活行过回收扫描不动")
        r = 处理写("join", {"分支": "任务/孤-7-r", "基线": "b", "写集摘要": ""})
        断言("原态 已踢出" in r.get("说明", ""), "562 排队回收行重报=回排队（1020 语义无损）")
        con.execute("UPDATE 队列 SET 更新时刻='坏串' WHERE 分支='任务/孤-7-s'")
        处理写("join", {"分支": "任务/孤-7-u", "基线": "b", "写集摘要": ""})   # 触发扫描
        断言(next(r2 for r2 in 队列快照() if r2["分支"] == "任务/孤-7-s")["状态"] == "排队",
             "562 更新时刻坏串=宁漏收不误收且不炸服务（456 口径同构）")

        # ── 562 预验已并入检测：源分支已并入 develop=孤儿预验拒入/取消不再重派（555/556） ──
        真检测562 = 源分支已并入
        源分支已并入 = lambda 源: True   # 模拟看板台账：核出已并入
        r = 处理写("task_enqueue", {"分支": "ci/预验-f111111111", "sha": "1" * 40,
                                    "源分支": "任务/555"})
        断言(not r["ok"] and "已并入" in r.get("说明", ""),
             "562 enqueue 核出源分支已并入=拒入")
        源分支已并入 = lambda 源: False   # 模拟：未并入——放行入池
        处理写("task_enqueue", {"分支": "ci/预验-f111111111", "sha": "1" * 40,
                                "源分支": "任务/555"})
        处理写("task_enqueue", {"分支": "ci/预验-f222222222", "sha": "2" * 40,
                                "源分支": "任务/556"})
        处理写("task_enqueue", {"分支": "ci/预验-f333333333", "sha": "3" * 40})   # 旧客户端无源分支
        源分支已并入 = lambda 源: 源 == "任务/555"   # 555→True·556→False·None 源→False
        t = 处理写("task_claim", {"runner": "r562"})["任务"]
        断言(t and t["分支"] == "ci/预验-f222222222",
             "562 claim 跳过已并入任务派下一个（不浪费池容量重派）")
        断言(next(r2 for r2 in 预验任务快照()
                  if r2["分支"] == "ci/预验-f111111111")["状态"] == "已取消",
             "562 已并入任务=置已取消（不再参与认领·防反复重派）")
        t = 处理写("task_claim", {"runner": "r562b"})["任务"]
        断言(t and t["分支"] == "ci/预验-f333333333",
             "562 无源分支任务照常派发（旧客户端兼容·宁漏检不误杀）")
        断言(处理写("task_claim", {"runner": "r562c"})["任务"] is None, "562 取消/领完=池空")
        r = 处理写("task_enqueue", {"分支": "ci/预验-f111111111", "sha": "1" * 40,
                                    "源分支": "任务/555"})
        断言(r["ok"], "562 enqueue 见已取消行=不动不复活（幂等防重入队复活）")
        源分支已并入 = 真检测562   # 还原真实现（防泄漏影响后续段）

        # ── 562 修复（审查 P2-2）：HTTPError 仅 404 免退避·其余纳入退避戳——
        #    写锁内同步 HTTP 停顿防线（「端口在但持续 500」不再每次写操作都等一次 HTTP） ──
        假HTTP错 = lambda 码: urllib.error.HTTPError("http://x/api/task/1", 码, "msg", None, None)
        断言(并入检测退避否(假HTTP错(500)) is True, "562 HTTPError 5xx=置退避（写锁内不再反复同步等 HTTP）")
        断言(并入检测退避否(假HTTP错(502)) is True and 并入检测退避否(假HTTP错(401)) is True,
             "562 HTTPError 502/401 同=置退避（除 404 外全部病态纳入）")
        断言(并入检测退避否(假HTTP错(404)) is False,
             "562 HTTPError 404=免退避（任务不在台账=正常空结果·重试必有同解）")
        断言(并入检测退避否(TimeoutError()) is True and 并入检测退避否(OSError()) is True,
             "562 不可达/超时=置退避（既有语义保持）")

        print("selftest %d 项全过" % n绑定[0])
        raise SystemExit(0)

    print("[启动] 端口=%d db=%s 令牌=%s 任务池心跳超时=%ds"
          % (端口, db路径, "已设" if 令牌 else "未设（写操作全拒·fail-closed 561）", 心跳超时秒),
          flush=True)
    ThreadingHTTPServer(("0.0.0.0", 端口), 处理器).serve_forever()
