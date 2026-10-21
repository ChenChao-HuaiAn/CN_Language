#!/usr/bin/env python3
# develop 兜底面（1297 自 ci_daemon.py 拆出·原样搬移+初始化注入）：
#   上次已验SHA（免跑判定基线）/纯文档变更（189 免跑判定）/兜底入池（770 池派发）/
#   兜底过期+过期跳过结果（660 跑前后双保险）——拆出动机=ci_daemon 加分片时守 1000
#   行硬红线（「先拆再加」·973 外迁 ci_build 同款）。
# 依赖方向：ci_daemon → 本模块（单向·禁反引）；池 HTTP 走 ci_shard（1297 池协议
#   单点）；仓库根/平台/日志目录/远端名 由 ci_daemon 启动时 初始化() 注入。

import json
import subprocess
from datetime import datetime
from pathlib import Path

import ci_shard            # 池协议客户端单点（1297）
import ci_report           # 免跑留档=报告面（develop 语义 latest+result+上报）

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
                            text=True, cwd=_仓库根, timeout=120)
        if 取.returncode != 0:
            return False, "fetch 失败照跑：%s" % (取.stderr or "").strip()[:120]
        diff = subprocess.run(["git", "diff", "--name-only", "%s..%s" % (基线sha, 新sha)],
                              capture_output=True, text=True, cwd=_仓库根, timeout=120)
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
    回 = ci_shard.池调用(配置, "task_enqueue", {"分支": "ci/兜底-%s" % sha[:10], "sha": sha})
    if 回 and 回.get("ok"):
        print("[兜底入池] ci/兜底-%s（runner 池接手）" % sha[:10], flush=True)
        return True
    print("[兜底入池失败] 降级本机跑：%s" % 回, flush=True)
    return False


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
