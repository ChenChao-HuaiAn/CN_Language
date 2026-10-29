#!/usr/bin/env python3
# develop 兜底面（1297 自 ci_daemon.py 拆出·原样搬移+初始化注入）：
#   上次已验SHA（免跑判定基线）/纯文档变更（189 免跑判定）/兜底入池（770 池派发）/
#   兜底过期+过期跳过结果（660 跑前后双保险）——拆出动机=ci_daemon 加分片时守 1000
#   行硬红线（「先拆再加」·973 外迁 ci_build 同款）。
# 依赖方向：ci_daemon → 本模块（单向·禁反引）；池 HTTP 走 ci_shard（1297 池协议
#   单点）；直跑收口走 ci_round（#1795 develop头处理 外迁块消费·无环）；
#   仓库根/平台/日志目录/远端名 由 ci_daemon 启动时 初始化() 注入。

from __future__ import annotations   # P1-1：3.8 注解惰性化（str | None 等定义期不求值）

import json
import os
import subprocess
import time
from datetime import datetime
from pathlib import Path

import ci_shard            # 池协议客户端单点（1297）
import ci_report           # 免跑留档=报告面（develop 语义 latest+result+上报）
import ci_round            # 直跑收口（#1221·develop 降级直跑路径消费）

_仓库根: Path = Path(__file__).resolve().parent.parent
_平台 = ""
_日志目录 = _仓库根 / "ci-logs"
_远端名 = "origin"


def 初始化(仓库根: Path, 平台: str, 日志目录: Path, 远端名: str) -> None:
    """ci_daemon 启动时注入运行环境（原模块全局引用改经本模块全局）。"""
    global _仓库根, _平台, _日志目录, _远端名
    _仓库根, _平台, _日志目录, _远端名 = 仓库根, 平台, 日志目录, 远端名


def 上次已验SHA() -> str | None:
    """免跑判定基线=最近一次 develop 验证绿的树头。latest.json 的 sha 即该值
    （develop 轮绿/预验绿链顶都写 latest——预验绿链顶=验绿后 push 的 develop 头）。
    读不到（冷启动/损坏）→ None（调用方照跑=保守）。"""
    try:
        return json.loads((_日志目录 / "latest.json").read_text(encoding="utf-8")).get("sha")
    except Exception:
        return None


def 纯文档变更(基线sha: str, 新sha: str) -> tuple[bool, str]:
    """①纯文档轮免兜底判定（189）：基线..新 的变更文件**全部 .md**→真（免跑）。
    fail-safe 默认（Rust 同款哲学：默认保守）——fetch/diff 任何异常、基线缺失、
    出现任何非 .md 文件（代码/脚本/配置·尤其 scripts/——242 坏脚本广播教训）
    一律判假=照跑。返回 (判定, 缘由/文件摘要)。"""
    try:
        取 = subprocess.run(["git", "fetch", _远端名, "develop"], capture_output=True,
                            text=True, encoding="utf-8", errors="replace",
                            cwd=_仓库根, timeout=120)
        if 取.returncode != 0:
            return False, "fetch 失败照跑：%s" % (取.stderr or "").strip()[:120]
        diff = subprocess.run(["git", "-c", "core.quotepath=false", "diff",
                               "--no-renames", "--name-only",
                               "%s..%s" % (基线sha, 新sha)],
                              capture_output=True, text=True, encoding="utf-8",
                              errors="replace", cwd=_仓库根, timeout=120)
        if diff.returncode != 0:
            return False, "diff 失败照跑：%s" % (diff.stderr or "").strip()[:120]
        文件们 = [l for l in diff.stdout.splitlines() if l.strip()]
        if not 文件们:
            return True, "变更集为空（基线即新头）"
        非md = [f for f in 文件们 if not f.lower().endswith(".md")]
        if 非md:
            return False, "含非文档 %d 个（首=%s）" % (len(非md), 非md[0][:80])
        return True, "纯文档 %d 个全 .md" % len(文件们)
    except Exception as e:
        return False, "判定异常照跑：%s" % str(e)[:120]


def 兜底入池(sha: str) -> bool:
    """develop 验证任务入池（189 引入·770 派发语义升级）：develop 新头提交 ci/兜底-<sha10>
    任务给验证池——服务端按 win→linux→arm64 游标轮转指派平台+平台内随机空闲+全忙放宽
    （queue_service 落地·客户端零派发逻辑）。入池失败（池不可达/拒绝）→ False
    （调用方降级本机跑=三层降级精神·门禁永不因池而缺位）。"""
    配置 = ci_shard.池配置()
    if not 配置["池地址"]:
        return False
    # #1569：写集触发 M6 时随入队上报——服务端据此把 M6 轮限定原生平台
    #   （交叉模拟 arm64 无法原生跑 79_v2，派过去会 fail-closed 造红）。
    try:
        差 = subprocess.run(["git", "diff", "--name-only", "-z", "%s~1" % sha, sha],
                            capture_output=True, text=True, encoding="utf-8",
                            errors="replace", timeout=30)
        文件们 = [x for x in (差.stdout or "").split(chr(0)) if x]
        须M6 = bool(m6_gate.判定写集(文件们).get("触发")) if 差.returncode == 0 else True
    except Exception:
        须M6 = True   # 写集不可判=fail-safe 按 M6 处理（限原生平台·不造 arm64 假红）
    载荷 = {"分支": "ci/兜底-%s" % sha[:10], "sha": sha}
    if 须M6:
        载荷["M6必跑"] = True
    回 = ci_shard.池调用(配置, "task_enqueue", 载荷)
    if 回 and 回.get("ok"):
        print("[兜底入池] ci/兜底-%s（runner 池接手）" % sha[:10], flush=True)
        return True
    print("[兜底入池失败] 降级本机跑：%s" % 回, flush=True)
    return False


# ── #1795 门禁全量轮限频攒批（2026-10-29 用户批 CI 提速方案2）──────────────
#   病根：develop 每个新头都派一轮全量兜底（10-26 实录 66 轮/天×均 38 分钟≈42
#   机器小时），与预验抢同一池 → 预验排队 600s+执行 49 分钟（10-28 峰值）——
#   而每个兜底头都已过云端预验（test-then-commit），中间头的组合态由**更新的头**
#   隐含覆盖（189 过期跳过同款语义）。攒批=限频窗口内的头只验窗口后的最新头。
#   安全网三重：①上轮红例外（修复确认轮不受限）；②窗口过后即使头不变也补验
#   （轮询每 90s 重判·不丢头）；③每日 --force cron 全量兜底不变（daemon 点名面）。
限频间隔默认秒 = 7200


def 限频放行否(现在: float = None) -> tuple[bool, str]:
    """纯读状态判定（零副作用）：当前是否放行 develop 新头的兜底验证。

    判定序：
      ①最近 result_*.json（mtime 最大）绿=False → 放行——红确认轮不受限频
        （修复重推要快速验证·红未清时的持续攒批会掩盖回归）；
      ②latest.json mtime=上次绿验证时刻，距今 ≥ CN_GATE_MIN_INTERVAL_SEC
        （默认 7200s）→ 放行；
      ③否则攒批。latest 缺失/不可读 → 放行（保守——门禁缺位重于算力浪费）；
        红状态读不出 → 跳过①按②判（不放大流量，也不猜红）。
    """
    间隔 = int(os.environ.get("CN_GATE_MIN_INTERVAL_SEC", str(限频间隔默认秒)))
    try:
        红们 = sorted(_日志目录.glob("result_*.json"), key=lambda p: p.stat().st_mtime)
        if 红们:
            最近 = json.loads(红们[-1].read_text(encoding="utf-8"))
            if 最近.get("绿") is False:
                return True, "最近门禁轮红（%s）——确认轮不受限频" % 红们[-1].name
    except (OSError, ValueError):
        pass   # 红状态读不出=不据此放行（落②按绿态限频·不猜红）
    try:
        上次绿 = (_日志目录 / "latest.json").stat().st_mtime
    except OSError:
        return True, "限频状态缺失（latest.json 不可读）——保守放行"
    距秒 = (现在 or time.time()) - 上次绿
    if 距秒 >= 间隔:
        return True, "距上次绿验证 %d 分钟 ≥ 限频 %d 分钟" % (距秒 // 60, 间隔 // 60)
    return False, ("距上次绿验证 %d 分钟 < 限频 %d 分钟——攒批待窗口后的最新头"
                   % (距秒 // 60, 间隔 // 60))


def develop头处理(sha: str, 语境: dict) -> bool:
    """develop 新头派发决策块（#1795 自 ci_daemon 外迁+限频内聚·五路全消费恒 True）：

      ①纯文档免跑（189）；②限频攒批（#1795）；③幂等跳过（770 已入池/已有结论档）；
      ④兜底入池成功（770·池负责）；⑤入池失败降级本机直跑（门禁永不缺位）。
    调用方拿到 True 即执行统一轮间节奏（非常驻 return 0 / 放锁 sleep 重抢锁 continue）。
    语境注入 daemon 侧闭包：已入池/日志目录/交叉模拟实例/平台/带门禁锁跑/仓库根。
    """
    基线 = 上次已验SHA()
    if 基线 and 基线 != sha:
        免, 缘由 = 纯文档变更(基线, sha)
        if 免:
            ci_report.免跑留档(sha, 缘由)
            return True
    # #1795 限频攒批：窗口内不派发（下轮轮询重判——头继续前进则判新头，
    #   头不变窗口过则补验·攒批头由最新头隐含覆盖，见模块头安全网说明）
    放行, 缘由 = 限频放行否()
    if not 放行:
        print("[攒批] %s（%s）" % (sha[:10], 缘由), flush=True)
        return True
    if sha in 语境["已入池"] or list(语境["日志目录"].glob("result_%s_*.json" % sha[:10])):
        # 头已交池负责（本进程入过）/本地已有该 sha 结论档（含红终态）——本机不跑，
        # 等池派发或下一个头（新头≠sha 自然走出此分支）。
        return True
    if 兜底入池(sha):
        语境["已入池"].add(sha)   # 入池成功=池负责（服务端幂等·行已存在亦 ok）
        return True
    if not ci_shard.整任务可认领(语境["交叉模拟实例"]):
        # #1667：交叉模拟实例不直跑全量 e2e（qemu i128 族错值假红无信息量）——
        #   门禁由原生/真机实例承担；池不可达时该面等池恢复，不制造假红。
        print("[整任务让路] 交叉模拟实例不直跑 develop 全量（#1667）——交由原生/真机实例",
              flush=True)
    else:
        print("[降级] 兜底入池失败——本机直跑（门禁永不缺位）：%s" % sha[:10], flush=True)
        print("[开跑] %s @ %s（分支=develop）" % (sha[:10],
              datetime.now().strftime("%H:%M:%S")), flush=True)
        # 190（1035 轮）：开跑即上报「执行中」
        ci_report.上报执行中(sha, "develop", 语境["平台"])
        结果 = 语境["带门禁锁跑"](sha)
        if 结果 is not None:     # None=291 让出（gate_lock 互忙）——不落盘·下轮再战
            ci_round.直跑收口(sha, 结果, ci_report.落盘,
                              仓库根=语境["仓库根"], 轮分支="develop")
    return True


def 兜底过期(任务sha: str, develop头sha: str) -> bool:
    """660（2026-10-14）：兜底轮跑前预检——任务 sha ≠ develop 现头（上方 fetch 的
    FETCH_HEAD）即过期（该 sha 树由新头轮隐含覆盖·189 语义）→跳过整轮零算力；
    头空（rev-parse 异常）保守不跳（fail-safe·走原整轮+跑后复查双保险）。"""
    return bool(develop头sha) and 任务sha != develop头sha


def 过期跳过结果(sha: str, 任务分支: str, 头sha: str) -> dict:
    """189 过期跳过结果构造（660：跑前预检与跑后复查共用=形状单一事实源）：绿=True
    闭环防重派·不落盘防 latest 被非任务 sha 污染（详情文案两处调用完全一致）。"""
    return {"sha": sha, "分支": 任务分支, "平台": _平台, "绿": True,
            "步骤": {"过期跳过": {"rc": 0, "说明": "develop 已前进至 %s——任务 sha 树由新头轮覆盖（189）"
                                 % 头sha[:10]}},
            "总秒": 0, "时刻": datetime.now().isoformat(timespec="seconds"),
            "日志": "", "日志尾部": []}
