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
# API（1425 全站登录墙：GET/POST 一律须鉴权——Bearer 令牌（机器·daemon/CLI/integrate）
#   ∨ 登录会话 cookie（网页人·用户名+密码 CN_WEB_USER/CN_WEB_PASS·web_auth 公共模块）。
#   未鉴权 GET 页面→自包含登录页 200；未鉴权 API→401 JSON。豁免仅登录三件
#   POST /api/login、POST /api/logout、GET /api/me。回环消费（源分支已并入→8301 台账）
#   与全部出站已带 Bearer（1425 同批））：
#   GET  /            门禁窗口（842 重构：queue_www 墨韵 UI·三页签+失败详情+归因）
#   GET  /queue.css|/queue.js|/tokens.css  静态资源（回静态白名单·tokens 回退 board_www）
#   GET  /api/state   {队列:[…], 门禁最近:[…], 预验任务:[…], 池忙闲:{…}}
#        825：池忙闲补「排队」数（并行度观测·池饱和排队可见性数据源）；预验任务行补
#        「失败原因」（完成红首因——红步骤名+失败用例名·8300 池页签徽标/integrate --status）。
#   GET  /api/task_result?sha=&branch=  预验/兜底任务详情 {状态,绿,详情,失败原因,…}
#                                  （842 双前缀通用化；#1221：branch=本轮精确分支名——
#                                  ci/预验-<sha10>[-m6<hex>]——服务端先按 branch 精确查，
#                                  sha 前缀最新序号次之；旧同名行不遮蔽新 M6 行）
#   GET  /api/gate_result?sha=&plat=  门禁行详情 {绿,总秒,时刻,详情}（842·红行查看·参数名 ASCII）
#   POST /api/join    {分支,基线,写集摘要}      报名（幂等）
#   POST /api/update  {分支,状态}               改状态（排队/集成中/已踢出/已完成）
#   POST /api/touch   {分支}                    重报时刻（失败恢复重排队·保序不刷=幂等）
#   POST /api/heartbeat {分支}                  562：只刷更新时刻（批主/等待者活会话证明·
#                                             不改状态——回收与 --wait 接管判定以心跳新鲜度为据）
#   POST /api/clear   {分支们:[…]}              集成销账清行
#   POST /api/report  {ci_daemon 结果 JSON}     门禁结果上报
#   POST /api/task_enqueue  {分支,sha,源分支?,认领限制?,内容键?,M6必跑?}
#                                                 预验任务入队（幂等·已完成绿=直接复用·已完成红=重置重跑·
#                                                 已取消=源分支未并入则复活排队（#1221 P1-A）；
#                                                 源分支=发起预验的任务分支·222 网页分支列显示用·缺省兼容旧客户端；
#                                                 认领限制=平台白名单逗号串（337·如 win-x64,linux-x86_64）·缺省 NULL=不限；
#                                                 M6必跑=true→本轮必须原生跑 79_v2·派发按实例 M6原生 过滤；
#                                                 排队行 >CN_QUEUE_TASK_REAP_SEC（默认 6h）→reaper 已取消）
#   POST /api/task_claim    {runner,平台?,物理机?,M6原生?}  认领最旧排队任务（含回收心跳超时任务·原子；
#                                                 337 自报平台过滤；#1221：M6原生=false/0=交叉模拟→不领
#                                                 M6必跑任务（旧客户端不报=None 按未知放行·轮内 fail-closed 兜底））
#   POST /api/task_heartbeat {runner,分支}        心跳续约
#   POST /api/task_complete {runner,分支,绿,结果}  完成上报（校验认领者·防回收后旧 runner 复活覆盖）
#   POST /api/task_shard_claim {runner,平台?,物理机?,M6原生?}  #1426：从实例领缺口片——服务端按
#                                 物理机键（hostname·平台-架构·指纹6）限并发（默认同机
#                                 1 片·CN_SHARD_PER_MACHINE 可配 2）；超限/让位返回
#                                 片=None+限流 说明（不领片·不阻塞主实例召唤）。主实例
#                                 抢补（带 分支+片序）不受限=正确性补偿路径。旧 daemon
#                                 不报物理机=按 runner 独立机器保守放行+显式告警。
#   POST /api/instance_ping {runner,平台?,树头?,daemon指纹?}  1392：实例空闲报活+版本指纹
#   POST /api/instance_refresh {}  #1460：实例面板「更新实例」——广播刷新戳（下随
#                                  instance_ping 响应下发·空闲 daemon 对齐 develop 树头）
#                                                 （daemon 空闲也每 60s 报·旧 daemon 不发=渐进
#                                                 部署服务端照常——活跃度退化为最后 claim/complete）
# 1425 网页登录（web_auth 公共模块·照看板 430/1425 口径）：
#   POST /api/login {用户名,密码}  正确→签发 HttpOnly 会话 cookie（cn_queue_session·
#                                  SQLite 持久·7 天）；错凭据 401+小睡防爆破
#   POST /api/logout               注销当前会话+清 cookie
#   GET  /api/me                   {已登录:bool}
#   GET  /api/state「实例动态」=1392 实例页签数据源（逐实例 执行中/心跳超时/疑似离线/空闲
#                                                 +当前任务+最近完成+版本·实现在 queue_instances）。
#       975（87 篇反思①·863 五连静默机械化）：结果含认领失败形状（步骤.同步.rc 非零
#       =fetch 预验分支失败/轮内同步异常）→ 行级「连续同类失败计数」+达阈值（默认 3）
#       journal 播报 `[975 预验失败告警]`；计数字段随 /api/state 预验任务行与
#       /api/task_result 详情流出（8300 门禁窗口池页签徽标面）。
#       825 残余 A：完成红一律落「失败原因」=首因摘要（红步骤名+失败用例名——真实红
#       显形：构建/单测/e2e 红不再只有 rc；绿=1/重置重跑清 NULL）。
# 部署：/etc/systemd/system/cn-queue.service（Environment= 端口/令牌/DB 路径/
#   CN_WEB_USER/CN_WEB_PASS 网页凭据——未配置=网页登录全拒 fail-closed 1425）。
# 自检：python3 queue_service.py --selftest（842 起实现体外迁=queue_selftest.py·惰性导入）。

from __future__ import annotations   # P1-1：3.8 注解惰性化（float | None 等定义期不求值）

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

# 1297：结果解析面（975/825）与分片面外迁——本文件加分片协议时守 900 警戒线
#   （「先拆再加」·973 外迁 ci_build 同款）；预验失败告警阈值 常量随迁 queue_result
#   （该功能专属配置·单一事实源）。依赖单向：本文件 → 三者（三者禁反引本文件）。
# 1392：实例动态面（实例表/空闲报活/动态快照）外迁 queue_instances——同款先拆再加。
import queue_result
import queue_shard
import queue_instances
import web_auth              # 网页登录墙公共模块（1425·凭据/会话/登录页）
import ci_duration           # 1385：耗时滚动基线（门禁写入即评估·快照/详情徽标数据源）

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
# #1221 复审 P0：预验任务「排队」行 reaper——M6 非幂等分支每次重跑唯一，残留排队行
#   无人消费=永久排队行与白跑（branch_cleanup 只清远端分支·不清任务行）。默认 6h。
预验排队回收秒 = int(os.environ.get("CN_QUEUE_TASK_REAP_SEC", "21600"))
看板服务地址 = os.environ.get("CN_BOARD_URL", "http://127.0.0.1:8301")   # 562：已并入检测数据源（任务台账·同机默认）
会话cookie = web_auth.队列cookie名   # 1425：网页登录会话 cookie 名（与看板 8301 异名——
                                     #   Caddy /board/ 与 /queue/ 同域同名 cookie 会互顶）
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
# 975 预验失败告警阈值已随迁 queue_result（#1221 P3：删除本文件死常量）。

写锁 = threading.Lock()          # SQLite 写串行化（读靠 WAL 并发）
时区时刻 = lambda: datetime.now().strftime("%m-%d %H:%M:%S")   # 911 教训：秒级戳
# 770：兜底轮转游标（内存态·重启从头轮起无妨）。1392：实例注册表由内存 dict 改
#   queue_instances 实例表持久承载（重启不丢·claim/ping 双入口写入）——平台忙闲
#   判定与池忙闲快照改读表（旧内存版服务重启即归零=轮转限制放宽误触发）。
兜底轮转游标 = {"i": 0}


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
    """770：该平台注册实例数 > 该平台执行中行数=有空闲；实例表无该平台实例
    （冷启动/从未 claim·1392 起为持久表重启不丢）→保守 True（不阻塞轮转——
    限制照打，领不上由阈值放宽兜底）。须持写锁。"""
    们 = [r[0] for r in con.execute(
        "SELECT 名称 FROM 实例 WHERE 平台=?", (平台,)).fetchall()]
    if not 们:
        return True
    占用 = {行[0] for 行 in con.execute(
        "SELECT 认领者 FROM 预验任务 WHERE 状态='执行中' AND 认领者 IS NOT NULL").fetchall()}
    return sum(1 for r in 们 if r in 占用) < len(们)


def 池忙闲快照() -> dict:
    """770：池忙闲（integrate 忙时拼车窗口与巡检的判定源）——注册实例数/执行中数/池全忙。
    全忙=有注册实例且执行中数≥注册数（每台都在跑）；无注册（冷启动）=不算全忙。
    825 残余 B（并行度观测）：补「排队」=排队任务数——池饱和排队可见性数据源
    （8300 池页签摘要条/integrate --status 验证池段/巡检 B3）。须持写锁。"""
    执行中 = con.execute(
        "SELECT COUNT(*) FROM 预验任务 WHERE 状态='执行中'").fetchone()[0]
    排队 = con.execute(
        "SELECT COUNT(*) FROM 预验任务 WHERE 状态='排队'").fetchone()[0]
    注册 = con.execute("SELECT COUNT(*) FROM 实例").fetchone()[0]   # 1392：持久实例表（重启不归零）
    return {"注册实例": 注册, "执行中": 执行中, "排队": 排队,
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
        失败计数 INTEGER DEFAULT 0,     -- 975：连续同类认领失败次数（真实验证红/绿清零·重跑保留）
        失败类别 TEXT, 失败摘要 TEXT,   -- 975：最近一次认领失败类别（同步失败…）与原因摘要
        告警时刻 TEXT,                  -- 975：最近一次达阈值告警的时戳（NULL=未告警）
        失败原因 TEXT,                  -- 825：最近一次完成红首因（红步骤名+失败用例名·绿/重跑清 NULL）
        M6必跑 INTEGER,                -- #1221：本轮写集触发 M6（派发按实例 M6原生 能力位过滤）
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
    try:   # #1221 M6 必跑位：1=本轮必须原生跑 79_v2（task_claim 按实例 M6原生 过滤）
        con.execute("ALTER TABLE 预验任务 ADD COLUMN M6必跑 INTEGER")
    except sqlite3.OperationalError:
        pass
    try:   # 975 连续同类认领失败计数（863 五连静默机械化·真实验证红/绿清零·重跑保留）
        con.execute("ALTER TABLE 预验任务 ADD COLUMN 失败计数 INTEGER DEFAULT 0")
    except sqlite3.OperationalError:
        pass
    try:   # 975 失败类别（枚举见 认领失败分类·如「同步失败」）
        con.execute("ALTER TABLE 预验任务 ADD COLUMN 失败类别 TEXT")
    except sqlite3.OperationalError:
        pass
    try:   # 975 失败摘要（最近一次认领失败原因摘要·≤200 字·页面/详情展示）
        con.execute("ALTER TABLE 预验任务 ADD COLUMN 失败摘要 TEXT")
    except sqlite3.OperationalError:
        pass
    try:   # 975 告警时刻（达阈值播报时戳·审计面；NULL=从未告警）
        con.execute("ALTER TABLE 预验任务 ADD COLUMN 告警时刻 TEXT")
    except sqlite3.OperationalError:
        pass
    try:   # 825 失败原因（完成红首因：红步骤名+失败用例名·真实红显形面）
        con.execute("ALTER TABLE 预验任务 ADD COLUMN 失败原因 TEXT")
    except sqlite3.OperationalError:
        pass
    try:   # 1070：门禁提交时刻（epoch 秒·daemon 上报）——排序键（提交序取代完成时刻序·
           #   池并行下完成时刻乱序）；NULL=旧版上报行（门禁快照排序显式分流沉底）
        con.execute("ALTER TABLE 门禁 ADD COLUMN 提交时刻 INTEGER")
    except sqlite3.OperationalError:
        pass
    try:   # 1301 排队秒观测：认领时刻（%m-%d %H:%M:%S）——排队秒=认领时刻-入队时刻
           #   （排队与跑轮分开可见·10-21 诊断实证「池空闲排队零」的常态化观测面）
        con.execute("ALTER TABLE 预验任务 ADD COLUMN 认领时刻 TEXT")
    except sqlite3.OperationalError:
        pass
    queue_shard.建表(con)   # 1297：预验分片表（独立新表·不碰父表结构）
    ci_duration.建表(con)   # 1385：耗时样本表 + 门禁.耗时告警列（幂等迁移·须在门禁表之后）
    queue_instances.建实例表(con)   # 1392：实例动态表（独立新表·不碰既有表结构）
    queue_instances.回填种子(con, 时区时刻)   # 1392：部署首屏种子（存量认领者回填·服务启动一次）
    queue_instances.校正种子活跃(con)        # #1402：存量种子「现在」虚值按最后完成时刻修正
    web_auth.建会话表(con)   # 1425：网页登录会话表（同构 board_service 430 表）
    con.commit()
    return con


con = 建库()


def 队列快照() -> list[dict]:
    """848（2026-10-15 用户验收反馈）：最新在前——原 ASC 与门禁/预验证 DESC 相反，
    陈年死行（/api/clear 未清的残留）堆顶遮挡实时视图。消费方兼容已核：integrate
    队列读 按分支 next 命中/--status 客户端自排序（且 830 批主选举退役）·board_ci
    按分支 dict 化·test_pool_selftest 按名命中——全不依赖服务端顺序。
    口径注：秒级戳无年份，字符串 DESC=当年内正确；跨年窗口（元旦后）与尾随旧年行
    错位为门禁/预验快照同款存量共口径（非新引入），旧行治理走 /api/clear。"""
    行们 = con.execute("SELECT 分支,基线,报名时刻,写集摘要,状态,更新时刻 FROM 队列 "
                       "ORDER BY 报名时刻 DESC").fetchall()
    return [dict(zip(("分支", "基线", "报名时刻", "写集摘要", "状态", "更新时刻"), r)) for r in 行们]


def 门禁快照(限: int = 200) -> list[dict]:
    """814/1070 薄壳：实现外迁 queue_result（#1221 P2-6 守 1000 行）。"""
    return queue_result.门禁快照(con, 限)


def 预验任务快照() -> list[dict]:
    """预验任务池快照薄壳（实现外迁 queue_result）。"""
    return queue_result.预验任务快照(con)


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
    # #1221 复审 P0/P2-6：预验排队行 reaper（>6h→已取消·仅 ci/预验-*）——实现外迁
    #   queue_result.预验排队收割（片行随父取消作废·返回明细供审计/测试）。
    queue_result.预验排队收割(con, 时区时刻, 预验排队回收秒, 行龄秒)


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
        # 1425：看板全站登录墙后 GET 亦须鉴权——CN_QUEUE_TOKEN 与 CN_BOARD_TOKEN
        # 同值（systemd 同源配置·430「queue_client.json 同源」先例），直接复用本服务令牌。
        请求 = urllib.request.Request(
            看板服务地址.rstrip("/") + "/api/task/" + m.group(1))
        if 令牌:   # P2-A：Bearer 写不可重定向头（跨源跳转不转发）
            请求.add_unredirected_header("Authorization", "Bearer " + 令牌)
        with urllib.request.urlopen(请求, timeout=3) as 响应:
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
            提交时刻 = 数据.get("提交时刻")
            时戳 = 时区时刻()
            # 1385：耗时评估/播报/列序列化/详情严格 JSON 全部外迁 ci_duration
            #   （门禁写入字段·执行中保留旧告警/评估异常显式错误对象·结果照常入库）。
            耗时列, 详情文本, 播报行们 = ci_duration.门禁写入字段(
                con, 平台, sha, 数据, 时戳, 是执行中=是执行中)
            for 播报行 in 播报行们:
                print(播报行, flush=True)
            con.execute("INSERT OR REPLACE INTO 门禁(sha,平台,绿,总秒,时刻,详情,提交时刻,耗时告警) "
                        "VALUES(?,?,?,?,?,?,?,?)",
                        (sha, 平台,
                         None if 是执行中 else (1 if 数据.get("绿") else 0),
                         None if 是执行中 else int(数据.get("总秒", 0)),
                         时戳,
                         # 842：上限 20000→500000——失败用例明细+提交历史（归因）随
                         #   详情入库（20000 在 40+ 失败用例或长 diff 时截断致页面缺明细）
                         详情文本,
                         # 1070：提交时刻（epoch·排序键）——旧版 daemon 不带/非法类型=None
                         #   显式哨兵（门禁快照 COALESCE 分流沉底·不静默错排）
                         int(提交时刻) if isinstance(提交时刻, (int, float)) else None,
                         # 1385：耗时基线评估对象（执行中保留旧值/未评估=NULL）
                         耗时列))
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
            # #1221：M6 必跑位（integrate 写集触发轮传 true）——task_claim 按实例能力位过滤。
            #   P3 防御：显式类型校验——"false" 字符串 truthy 会造成误派（拒绝非 bool/0/1）。
            原M6必跑 = 数据.get("M6必跑")
            if 原M6必跑 is None or 原M6必跑 is False:
                M6必跑 = 0
            elif 原M6必跑 is True:
                M6必跑 = 1
            elif isinstance(原M6必跑, int) and 原M6必跑 in (0, 1):
                M6必跑 = 原M6必跑
            else:
                return {"ok": False, "说明": "M6必跑 类型非法（须 bool/0/1）"}
            if 原M6必跑 is None and queue_result.M6分支后缀正则.search(分支):
                M6必跑 = 1   # P3：-m6<hex> 非幂等分支缺省即 M6 必跑（客户端漏传防御）
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
                    if M6必跑:
                        # #1569：M6 触发轮只派原生可跑平台（交叉模拟 arm64
                        #   无法原生跑 79_v2——派过去 fail-closed 造红）。限制
                        #   恒非空，避免 claim 的跳平台放宽把行放给 arm64。
                        原生们 = ("win-x64", "linux-x86_64")
                        轮平台 = next((p for p in 原生们 if 平台有空闲(p)),
                                     原生们[兜底轮转游标["i"] % len(原生们)])
                        认领限制 = 轮平台
                    else:
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
                con.execute("INSERT INTO 预验任务(分支,sha,源分支,认领限制,内容键,M6必跑,"
                            "状态,入队时刻,更新时刻) VALUES(?,?,?,?,?,?,'排队',?,?)",
                            (分支, sha, 源分支, 认领限制, 内容键, M6必跑,
                             时区时刻(), 时区时刻()))
            elif 已有[0] == "已取消":
                # #1221 复审 P1-A：已取消行可复活——reaper（6h）取消后同一幂等预验分支
                #   再入队必须回到排队，否则 integrate 池轮询每 60s 重入队无效→挂满 240 分钟。
                #   562 语义：源分支已并入 develop=维持取消（预验无意义）。
                if 源分支已并入(源分支) is True:
                    pass
                else:
                    con.execute("UPDATE 预验任务 SET 状态='排队',认领者=NULL,心跳时戳=NULL,"
                                "认领时刻=NULL,绿=NULL,详情=NULL,失败原因=NULL,"
                                "源分支=COALESCE(?,源分支),认领限制=?,"
                                "内容键=COALESCE(?,内容键),M6必跑=?,更新时刻=? "
                                "WHERE 分支=?",
                                (源分支, 认领限制, 内容键, M6必跑, 时区时刻(), 分支))
            elif 已有[0] == "完成" and not 已有[1] and not 分支.startswith("ci/兜底-"):
                # 189：兜底任务完成红=终态（红结果已上门禁表·不重置）——防池内死循环重跑
                #   （TX_02 下一周期 enqueue 幂等直达、runner 不再被派）；预验维持红=重置重跑
                #   （同 sha CAS 重试路径·给 flaky 一次机会）。337：认领限制随重发刷新（发起方最新意图为准）。
                con.execute("UPDATE 预验任务 SET 状态='排队',认领者=NULL,心跳时戳=NULL,"
                            "绿=NULL,详情=NULL,失败原因=NULL,源分支=COALESCE(?,源分支),认领限制=?,"
                            "内容键=COALESCE(?,内容键),M6必跑=?,更新时刻=? WHERE 分支=?",
                            (源分支, 认领限制, 内容键, M6必跑, 时区时刻(), 分支))
        elif 名 == "task_claim":
            # 认领（写锁内原子）：①回收心跳超时的执行中任务→重置排队；②取最旧排队任务置执行中。
            runner = str(数据.get("runner", ""))
            if not runner:
                return {"ok": False, "说明": "runner 标识缺失"}
            # 337 认领限制：runner 自报平台（win-x64/linux-x86_64/linux-arm64·294 三分）——
            #   只领「不限」或白名单含本平台的任务；旧客户端不报=空串→只领不限任务（不会错跑限制任务）。
            #   1392：自报平台随 claim 写入持久实例表（领到/领空都注册——daemon 每 90s
            #   轮询领空也是「活着」的信号·实例页可见；平台空串不覆盖已有自报值）。
            平台337 = str(数据.get("平台", ""))
            # #1221 复审 P2-2：M6原生 校验/归一提至实例认领之前——坏载荷必须先拒，
            #   不得先落实例表（原顺序坏载荷返回 ok=False 但表已写 1，后续 COALESCE
            #   回查会把它当原生实例、领走 M6 任务）。
            原M6原生 = 数据.get("M6原生")
            if 原M6原生 is None:
                M6原生值, 可领M6 = None, None      # 稍后回查实例表（COALESCE 安全网）
            elif isinstance(原M6原生, bool):
                M6原生值 = 1 if 原M6原生 else 0
                可领M6 = M6原生值
            elif isinstance(原M6原生, int) and 原M6原生 in (0, 1):
                M6原生值 = 原M6原生
                可领M6 = 原M6原生
            else:
                return {"ok": False, "说明": "M6原生 类型非法（须 bool/0/1）"}
            queue_instances.实例认领(con, 时区时刻, runner, 平台337,
                                     str(数据.get("物理机", "")),
                                     M6原生值)   # #1426 物理机键落表
            现在 = int(time.time())
            回收们 = [r[0] for r in con.execute(
                "SELECT 分支 FROM 预验任务 WHERE 状态='执行中' AND 心跳时戳 IS NOT NULL "
                "AND 心跳时戳<?", (现在 - 心跳超时秒,)).fetchall()]
            con.execute("UPDATE 预验任务 SET 状态='排队',认领者=NULL,心跳时戳=NULL,更新时刻=? "
                        "WHERE 状态='执行中' AND 心跳时戳 IS NOT NULL AND 心跳时戳<?",
                        (时区时刻(), 现在 - 心跳超时秒))
            for 分支r in 回收们:   # 1297：父任务回收重派=分片行作废（重派后由新主实例重新召唤）
                queue_shard.清父分片(con, 分支r)
            # 770 跳平台兜底：轮转限制的兜底任务排队超阈值仍无人领（该平台全忙/离线饿死）→
            #   限制惰性放宽 NULL——任何平台空闲实例可领（用户裁决「该平台全忙由下一轮平台执行」
            #   的运行期兜底形态；入池时的即时放宽是快路径，此处是慢路径保险）。坏串行龄 None=不动。
            for 分支770, 更新770 in con.execute(
                    "SELECT 分支,更新时刻 FROM 预验任务 WHERE 状态='排队' AND 分支 LIKE 'ci/兜底-%' "
                    "AND 认领限制 IS NOT NULL AND 认领限制!='' AND COALESCE(M6必跑,0)=0").fetchall():
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
            # #1221 复审 P1-4②：M6 必跑任务只派给「M6 原生可跑」实例（载荷已在上方
            #   校验/归一；载荷缺省=None→回查实例表已存能力位，仍无=未知放行）。
            if 可领M6 is None:
                行实例 = con.execute("SELECT M6原生 FROM 实例 WHERE 名称=?",
                                    (runner,)).fetchone()
                已存 = 行实例[0] if 行实例 else None
                可领M6 = 0 if 已存 == 0 else 1
            认领M6过滤 = " AND (M6必跑 IS NULL OR M6必跑=0 OR ?=1)"
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
                    queue_shard.清父分片(con, 兜底饥饿行[0])   # 1297：父取消=片行作废
                    con.commit()   # 自提交：作废后落回正常两级派发
                    print("[取消] 兜底饥饿行 %s 源分支已并入 develop——作废落回正常派发（562）"
                          % 兜底饥饿行[0], flush=True)
                else:
                    con.execute("UPDATE 预验任务 SET 状态='执行中',认领者=?,心跳时戳=?,认领时刻=?,更新时刻=? "
                                "WHERE 分支=?",
                                (runner, 现在, 时区时刻(), 时区时刻(), 兜底饥饿行[0]))
                    print("[兜底饥饿] 兜底排队超 %ds——优先派 %s（830 保底：develop 验证不被预验饿死）"
                          % (兜底饥饿秒, 兜底饥饿行[0]), flush=True)
                    con.commit()   # #1221 P2-3：提前 return 前提交（WAL 他连接可见）
                    return {"ok": True, "任务": {"分支": 兜底饥饿行[0], "sha": 兜底饥饿行[1]}}
            # 562 已并入检测：选出任务后核源分支——已并入 develop（台账 ✅）=孤儿预验
            #   （任务已集成销账·555/556 实录：回收后反复重派执行浪费池容量）→置「已取消」
            #   不派发·继续选下一个；None/False=照常派发（宁漏检不误杀）。已取消行不参与
            #   认领；重新发起集成时 enqueue 可复活（源分支未并入·#1221 P1-A）——
            #   integrate 轮询遇 已取消 也会主动请求复活。
            for _ in range(64):   # 防呆上限：极端全取消场景不空转
                行 = con.execute("SELECT 分支,sha,源分支 FROM 预验任务 WHERE 状态='排队' AND 分支 LIKE 'ci/预验-%' "
                                 + 认领过滤337 + 认领M6过滤 + " ORDER BY 序号 LIMIT 1",
                                 (平台337, 可领M6)).fetchone()
                if 行 is None:
                    行 = con.execute("SELECT 分支,sha,源分支 FROM 预验任务 WHERE 状态='排队' "
                                     + 认领过滤337 + 认领M6过滤 + " ORDER BY 序号 LIMIT 1",
                                     (平台337, 可领M6)).fetchone()
                if 行 is None:
                    break
                if 源分支已并入(行[2]) is True:
                    con.execute("UPDATE 预验任务 SET 状态='已取消',更新时刻=? WHERE 分支=?",
                                (时区时刻(), 行[0]))
                    queue_shard.清父分片(con, 行[0])   # 1297：父取消=片行作废
                    con.commit()   # 自提交：继续选下一个不受影响
                    print("[取消] 预验 %s 源分支 %s 已并入 develop（台账销账）——任务作废不再派发（562）"
                          % (行[0], 行[2]), flush=True)
                    continue
                con.execute("UPDATE 预验任务 SET 状态='执行中',认领者=?,心跳时戳=?,认领时刻=?,更新时刻=? "
                            "WHERE 分支=?", (runner, 现在, 时区时刻(), 时区时刻(), 行[0]))
                con.commit()   # #1221 P2-3：提前 return 前提交（WAL 他连接可见）
                return {"ok": True, "任务": {"分支": 行[0], "sha": 行[1]}}
            con.commit()   # 实例认领/回收写也须落盘（无任务可领路径）
            return {"ok": True, "任务": None}
        elif 名 == "task_heartbeat":
            runner, 分支 = str(数据.get("runner", "")), str(数据.get("分支", ""))
            cur = con.execute("UPDATE 预验任务 SET 心跳时戳=?,更新时刻=? "
                              "WHERE 分支=? AND 认领者=? AND 状态='执行中'",
                              (int(time.time()), 时区时刻(), 分支, runner))
            if cur.rowcount == 0:
                return {"ok": False, "说明": "任务不在执行中或认领者不匹配 " + 分支}
            queue_instances.实例心跳(con, 时区时刻, runner)   # 1392：实例最近活跃刷新
        elif 名 == "task_cancel":
            # 1026 降级竞态治理：integrate 池空 300s 降级 ssh 前调用——删「排队」行，
            #   防止后来 runner 认领到已被降级路径接手的任务=双跑浪费+池红记录误挂。
            #   「执行中」不删（runner 在真跑·让它跑完留档）；「完成」幂等无操作。
            分支 = str(数据.get("分支", ""))
            cur = con.execute("DELETE FROM 预验任务 WHERE 分支=? AND 状态='排队'", (分支,))
            if cur.rowcount:
                # #1221 复审 P2-3：父行取消=片行作废（与回收路径对称，防孤儿片行）
                queue_shard.清父分片(con, 分支)
            con.commit()   # #1221 P2-3：提前 return 前提交（WAL 他连接可见/重启不丢）
            return {"ok": True, "取消": cur.rowcount}
        elif 名 == "task_complete":
            # 完成上报：校验认领者（回收重派后旧 runner 复活=拒绝·重派者重新跑出的结果为准）
            runner, 分支 = str(数据.get("runner", "")), str(数据.get("分支", ""))
            结果 = 数据.get("结果")
            绿 = 1 if 数据.get("绿") else 0
            # 825 残余 A：完成红首因落「失败原因」（红步骤名+失败用例名·页面直读）；
            #   绿=1 显式清 NULL（同「详情」寿命——红后转绿不残留误导）。
            cur = con.execute("UPDATE 预验任务 SET 状态='完成',绿=?,详情=?,失败原因=?,心跳时戳=NULL,更新时刻=? "
                              "WHERE 分支=? AND 认领者=? AND 状态='执行中'",
                              (绿, json.dumps(结果, ensure_ascii=False)[:500000],
                               None if 绿 else queue_result.真实红首因(结果),
                               时区时刻(), 分支, runner))
            if cur.rowcount == 0:
                return {"ok": False, "说明": "任务不在执行中或认领者不匹配（可能已被回收重派）" + 分支}
            queue_instances.实例完成(con, 时区时刻, runner)   # 1392：实例最近活跃刷新
            # 975（87 篇反思①·863 五连静默机械化）：认领失败计数+阈值告警——真实红/绿
            #   （同步 rc=0）清零连续链（基础设施恢复证据），不告警；告警行走服务端
            #   journal（print）+ 行字段（快照/详情流出）。1297：实现外迁 queue_result.py。
            告警行 = queue_result.记认领失败并告警(
                con, 时区时刻, 分支, *queue_result.认领失败分类(结果))
            if 告警行:
                print(告警行, flush=True)
            con.execute("DELETE FROM 预验任务 WHERE 状态 IN ('完成','已取消') AND 序号 NOT IN "
                        "(SELECT 序号 FROM 预验任务 WHERE 状态 IN ('完成','已取消') "
                        "ORDER BY 序号 DESC LIMIT ?)",
                        (任务历史保留,))
            # 1297：片行随父任务裁剪联动（孤儿片不堆积·父行已滚出保留窗口者删）
            queue_shard.清孤儿分片(con, 任务历史保留)
        elif 名 == "instance_ping":
            # 1392 空闲报活：UPSERT 实例行+版本指纹（daemon 空闲也每 60s 报·旧 daemon
            #   不发=渐进部署服务端照常）。参数缺失（runner 空）=ok False 显式拒（无写入·
            #   回收死行 已自提交）。#1460：响应携刷新广播戳——须本分支自行 commit 后直返
            #   （落底统一 commit 的返回只含 ok·同 1297 教训：提前 return 跳过 commit=
            #   WAL 下他连接读不到写入）。
            回p = queue_instances.实例报活(con, 时区时刻, 数据)
            if not 回p.get("ok"):
                return 回p
            con.commit()
            return 回p
        elif 名 == "instance_refresh":
            # #1460 实例面板「更新实例」按钮：广播全局刷新戳（各实例下次 instance_ping
            #   响应携走·空闲 daemon fetch+reset 对齐 develop 树头）——薄路由委托实例面。
            #   写后自行 commit 再直返（同上一分支 WAL 口径）。
            戳 = queue_instances.置刷新请求(con, 时区时刻)
            con.commit()
            return {"ok": True, "刷新请求": 戳}
        elif 名 in ("task_shard_enqueue", "task_shard_claim",
                    "task_shard_heartbeat", "task_shard_complete"):
            # 1297：分片协议委托 queue_shard（写锁内·统一落下方 commit 后返回富响应——
            #   提前 return 会跳过 commit=WAL 下他连接读不到写入，故不走直返形态）
            if 名 == "task_shard_enqueue":
                回s = queue_shard.召唤(con, 时区时刻, 数据)     # 主实例召唤从片（幂等登记缺口）
            elif 名 == "task_shard_claim":
                # #1426：领片请求先落实例+物理机键——被限流拒绝也要登记（让位判定必须
                #   看到各机空闲实例；空 runner 由 认领 显式拒，此处不落空名行）。
                领runner = str(数据.get("runner", ""))
                if 领runner:
                    queue_instances.实例认领(con, 时区时刻, 领runner,
                                              str(数据.get("平台", "")),
                                              str(数据.get("物理机", "")),
                                              # #1221：M6 原生可跑能力位
                                              数据.get("M6原生"))
                回s = queue_shard.认领(con, 时区时刻, 数据)     # 从实例领缺口片 / 主实例抢补指定片
            elif 名 == "task_shard_heartbeat":
                回s = queue_shard.心跳(con, 时区时刻, 数据)     # 片心跳续约
            else:
                回s = queue_shard.完成(con, 时区时刻, 数据)     # 片结果上报（认领者校验同 task_complete）
            # 1392/#1402：从实例领片/片心跳也是活信号——注册实例并刷平台/活跃。此前只在
            #   整任务 claim/heartbeat/complete 落实例表，从实例跑片期间不在实例表→页签
            #   显示「空闲」且看不到分片（用户 10-22 质问「还是一个实例一个任务」成因之一）。
            #   #1426：认领入口的实例登记已前移到领片调用前（限流拒绝也要登记——让位判定
            #   需要空闲实例可见），此处只剩片心跳续活跃。
            if 名 == "task_shard_heartbeat" and 回s.get("ok"):
                queue_instances.实例心跳(con, 时区时刻, str(数据.get("runner", "")))
            con.commit()
            return 回s
        else:
            return {"ok": False, "说明": "未知操作 " + 名}
        con.commit()
    return {"ok": True}


# ===== 842 结果页静态面与详情查询（门禁窗口墨韵 UI·失败详情与归因数据面） =====
静态目录 = Path(__file__).resolve().parent / "queue_www"
静态白名单 = {"/": ("index.html", "text/html; charset=utf-8"),
            "/queue.css": ("queue.css", "text/css; charset=utf-8"),
            "/queue.js": ("queue.js", "text/javascript; charset=utf-8"),
            "/tokens.css": ("tokens.css", "text/css; charset=utf-8")}


def 回静态(路径: str):
    """842：queue_www 静态查找（门禁窗口新前端）——白名单制防目录穿越（对照
    board_service._回静态）；tokens.css 缺时回退 board_www（本机开发树两目录
    平级；TX_01 部署面由 deploy_queue.sh 生成副本）。返回 (体, MIME) 或 None
    （调用点 404 显式——miss 不静默）。"""
    项 = 静态白名单.get(路径)
    if 项 is None:
        return None
    文件名, mime = 项
    候选 = [静态目录 / 文件名]
    if 文件名 == "tokens.css":
        候选.append(静态目录.parent / "board_www" / 文件名)
    for p in 候选:
        if p.is_file():
            return p.read_bytes(), mime
    return None


def 归因截取(sha: str, 平台: str, 详情) -> dict:
    """1070 薄壳：实现外迁 queue_result（归因算法单点·8300/8301 共用）。"""
    return queue_result.归因截取(con, sha, 平台, 详情)


def 门禁详情查(sha: str, 平台: str):
    """842 薄壳：实现外迁 queue_result（#1221 P2-6 守 1000 行·调用面零改）。"""
    return queue_result.门禁详情查(con, sha, 平台)


def 任务详情查(sha10: str, branch: str = ""):
    """842/#1221 P1-1 薄壳：实现外迁 queue_result（branch 精确/js 最新序号查询序）。"""
    return queue_result.任务详情查(con, sha10, branch)


class 处理器(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        print("[%s] %s" % (时区时刻(), fmt % args), flush=True)

    def 回JSON(self, obj: dict, 码: int = 200, 头们: list = None):
        体 = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(码)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        for k, v in (头们 or []):    # 1425：登录/登出 Set-Cookie 头
            self.send_header(k, v)
        self.send_header("Content-Length", str(len(体)))
        self.end_headers()
        self.wfile.write(体)

    def 回HTML(self, 码: int, 文本: str, 头们: list = None):
        """1425：登录页响应（自包含单文件·deny 嵌框防点击劫持）。"""
        体 = 文本.encode("utf-8")
        self.send_response(码)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("X-Frame-Options", "DENY")
        for k, v in (头们 or []):
            self.send_header(k, v)
        self.send_header("Content-Length", str(len(体)))
        self.end_headers()
        self.wfile.write(体)

    def _取会话id(self) -> str:
        from http.cookies import SimpleCookie
        蛋糕 = SimpleCookie(self.headers.get("Cookie", ""))
        项 = 蛋糕.get(会话cookie)
        return 项.value if 项 else ""

    def 鉴权通过(self) -> bool:
        """561：fail-closed（对齐看板 430）——令牌未配置=机器通道全拒（旧址 令牌==""
        全放行）。1425：扩展双通道——Bearer 令牌（机器·daemon/CLI/integrate）
        ∨ 登录会话 cookie（网页人·用户名+密码签发），GET/POST 同一套。"""
        if 令牌 and self.headers.get("Authorization", "") == "Bearer " + 令牌:
            return True
        return web_auth.会话合法(con, 写锁, self._取会话id())

    def _登录(self, 体: dict):
        """网页登录（1425·web_auth.校验凭据：CN_WEB_USER/CN_WEB_PASS·fail-closed+
        小睡防爆破+compare_digest 防时序）→签发 HttpOnly 会话 cookie。"""
        if not web_auth.校验凭据(str(体.get("用户名", "")), str(体.get("密码", ""))):
            return self.回JSON({"ok": False, "说明": "用户名或密码不符"}, 401)
        sid = web_auth.签会话(con, 写锁)
        return self.回JSON({"ok": True}, 头们=[web_auth.set_cookie头(sid, 会话cookie)])

    def _登出(self):
        web_auth.销会话(con, 写锁, self._取会话id())
        return self.回JSON({"ok": True}, 头们=[web_auth.清_cookie头(会话cookie)])

    def do_GET(self):
        from urllib.parse import parse_qs
        路径 = urlparse(self.path).path
        if 路径 == "/api/me":                       # 登录态查询豁免（无敏感数据）
            return self.回JSON({"已登录": self.鉴权通过()})
        if not self.鉴权通过():                     # 1425 全站登录墙（原「GET 只读放行」废除）
            if 路径.startswith("/api/"):
                return self.回JSON({"ok": False,
                                    "说明": "未登录或令牌不符（1425 登录墙·GET 亦须鉴权）"}, 401)
            return self.回HTML(200, web_auth.登录页HTML("CN · 门禁窗口"))
        if 路径 == "/api/state":
            with 写锁:
                回收死行()   # 456+562：批主/报名会话死亡后无写活动，靠轮询/网页查看触发回收
                return self.回JSON({"队列": 队列快照(), "门禁最近": 门禁快照(),
                                    "预验任务": 预验任务快照(),
                                    "池忙闲": 池忙闲快照(),   # 770：integrate 拼车窗口/巡检判定源
                                    # #1460：「更新实例」上次广播戳（实例页按钮旁显示·0=从未）
                                    "刷新请求": queue_instances.取刷新请求(con),
                                    "实例动态": queue_instances.实例动态快照(con, 心跳超时秒)})   # 1392：实例页签数据源
        if 路径 == "/api/task_result":
            # 按 sha10 查（分支=ci/预验-/ci/兜底-<sha10>·URL 保持纯 ASCII 免编码坑·
            # 842 通用化：双前缀）；#1221 复审 P1-1：客户端可另传 branch=本轮精确分支名
            # （M6 轮为 ci/预验-<sha10>-m6<hex>）——先按 branch 精确查，旧同名行不遮蔽。
            参q = parse_qs(urlparse(self.path).query)
            sha10 = (参q.get("sha") or [""])[0]
            branch参 = (参q.get("branch") or [""])[0]
            # P3 防御：sha 与 board_service /api/ci_result 同规 hex 校验（防无界 LIKE 扫描）；
            #   branch 限定协议前缀形态（M6 轮 -m6<hex> 可选后缀）。
            if sha10 and not re.fullmatch(r"[0-9a-f]{7,40}", sha10):
                return self.回JSON({"ok": False, "说明": "sha 须为 7~40 位 hex"}, 400)
            if branch参 and not queue_result.是预验分支名(branch参):
                return self.回JSON({"ok": False,
                                    "说明": "branch 须为 ci/预验-|ci/兜底-<sha10>[-m6<hex>]"}, 400)
            详情 = 任务详情查(sha10, branch参) if sha10 else None
            if 详情 is None:
                return self.回JSON({"ok": False,
                                    "说明": "无此任务（ci/预验-/ci/兜底-" + sha10 + " 均无）"}, 404)
            return self.回JSON(详情)
        if 路径 == "/api/gate_result":
            # 842：门禁表详情（8300 门禁窗口红行面板·8301 验证红详情代理共用）——
            #   参数名用 ASCII（plat）·URL 保持纯 ASCII 免编码坑（同 task_result 口径）
            参 = parse_qs(urlparse(self.path).query)
            sha = (参.get("sha") or [""])[0]
            平台 = (参.get("plat") or [""])[0]
            if not sha or not 平台:
                return self.回JSON({"ok": False, "说明": "缺少 sha/plat 参数"}, 400)
            行详情 = 门禁详情查(sha, 平台)
            if 行详情 is None:
                return self.回JSON({"ok": False,
                                    "说明": "无此门禁行 " + sha[:10] + " " + 平台}, 404)
            return self.回JSON({**行详情, "归因": 归因截取(sha, 平台, 行详情["详情"])})
        if 路径 == "/api/task_shards":
            # 1297：片状态快照（主实例收片轮询用）——分支=ci/预验-<sha10> 纯 ASCII 免编码
            分支 = (parse_qs(urlparse(self.path).query).get("branch") or [""])[0]
            if not 分支:
                return self.回JSON({"ok": False, "说明": "缺少 branch 参数"}, 400)
            return self.回JSON({"ok": True, "分片": queue_shard.快照(con, 分支)})
        if 路径 == "/api/task_shard_result":
            # 1297：单片结果详情（主实例聚合 e2e明细 用）
            参 = parse_qs(urlparse(self.path).query)
            分支 = (参.get("branch") or [""])[0]
            片序 = (参.get("i") or [""])[0]
            if not 分支 or not 片序.isdigit():
                return self.回JSON({"ok": False, "说明": "缺少 branch/i 参数"}, 400)
            详情 = queue_shard.片详情(con, 分支, int(片序))
            if 详情 is None:
                return self.回JSON({"ok": False, "说明": "无此片 %s#%s" % (分支, 片序)}, 404)
            return self.回JSON({"ok": True, **详情})
        if 路径 in 静态白名单:
            # 842：门禁窗口=queue_www 墨韵前端（三页签+失败详情+归因·前端 fetch
            #   /api/state 渲染）；执行中行（绿 IS NULL）黄标语义由前端三态徽标承担。
            静态 = 回静态(路径)
            if 静态 is None:
                return self.回JSON({"ok": False, "说明": "静态资源缺失 " + 路径 +
                                    "（部署面含 queue_www/·见 scripts/deploy_queue.sh）"}, 404)
            体, mime = 静态
            self.send_response(200)
            self.send_header("Content-Type", mime)
            self.send_header("Content-Length", str(len(体)))
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()
            self.wfile.write(体)
            return
        self.回JSON({"ok": False, "说明": "未知路径"}, 404)

    def do_POST(self):
        路径 = urlparse(self.path).path
        if 路径 == "/api/login":                    # 登录豁免鉴权（1425·墙的钥匙）
            return self._登录(self._读JSON体())
        if 路径 == "/api/logout":                   # 登出无破坏性·同豁免
            return self._登出()
        if not self.鉴权通过():
            return self.回JSON({"ok": False, "说明": "未登录或令牌不符（1425 登录墙）"}, 401)
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

    def _读JSON体(self) -> dict:
        长度 = int(self.headers.get("Content-Length", "0") or "0")
        if 长度 <= 0 or 长度 > 1 << 20:
            return {}
        try:
            return json.loads(self.rfile.read(长度).decode("utf-8"))
        except (ValueError, UnicodeDecodeError):
            return {}


if __name__ == "__main__":
    # 842：自检实现体外迁 queue_selftest.py（先例=board_selftest.py 405/
    #   ci_daemon_selftest.py 660——主模块先拆再加防回涨）；入口命令不变。
    if len(sys.argv) > 1 and sys.argv[1] == "--selftest":
        import queue_selftest
        queue_selftest.跑()

    print("[启动] 端口=%d db=%s 令牌=%s 网页凭据=%s 任务池心跳超时=%ds"
          % (端口, db路径, "已设" if 令牌 else "未设（机器通道全拒·fail-closed 561）",
             "已设" if (os.environ.get("CN_WEB_USER") and os.environ.get("CN_WEB_PASS"))
             else "未设（网页登录全拒·fail-closed 1425）", 心跳超时秒),
          flush=True)
    ThreadingHTTPServer(("0.0.0.0", 端口), 处理器).serve_forever()
