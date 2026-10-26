#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ci_platform_timing.py —— 三平台验证轮分阶段耗时归因（任务 #1429·只读观测工具）。

背景：三平台（win-x64 / linux-x86_64 / linux-arm64）验证轮墙钟相差数倍，此前无
  「平台 × 步骤」的专项归因面——#1385 只做门禁步骤滚动告警（超阈才响），#1299 只
  修 linux 构建缓存，缺一张能回答「慢在哪一步」的横表。本脚本把队列服务门禁表
  （/api/gate_result 详情.步骤）与 8300 预验任务详情汇成「平台 × 步骤」中位/分位/极差，
  并把分片形态、M6（79_v2）单例、普通用例 worker 纯耗时、收片等待（残差）单列。

口径（全部显式，不猜）：
  · 数据源=TX_01 队列服务 GET /api/state（门禁最近 N 行）+ GET /api/gate_result
    （每行详情）；也可 --input 读离线快照（同一形状）复算，零网络。
  · **主轮**=详情.步骤含静态门禁步骤（网站示例verify 或 check_spec_coverage）——
    从实例片轮只有 同步/清产物/配置/构建/e2e（ci_daemon.跑分片轮 步骤集），据此
    机械排除，避免把「片轮 200s」混进整轮中位（会把平台耗时拉低数倍）。
  · **可比较轮**=主轮 ∧ 步骤.e2e并行 有「秒」∧ 总秒 > 300——排除过期早退
    （develop 已前进→跳过构建/门禁，总秒几十秒）、构建红提前终止、池回收残轮。
  · 每平台按时刻倒序取最近 --limit 轮（默认 30）。
  · 步骤耗时=步骤记录里的「秒」；统计不过滤 rc（e2e 红仍跑完全量，红轮耗时有
    信息量），但构建/配置在 rc≠0 时为失败前残值，构建中位单独只用 rc=0 样本。
  · 「e2e并行」=整轮 e2e 墙钟（ci_m6.e2e段：单实例=run_e2e 墙钟；#1632·plans/034
    §5A ④ 起分片/退化=真实墙钟（e2e 起始→收片完成·收片等待计入）。#1632 前的历史轮
    分片=各片步骤秒的 max、退化=各片秒之和——从片串行认领时 max 低估真实墙钟，
    本脚本「残差」列仍量化该缺口（历史轮）：残差=总秒-(构建+单测+静态门禁+
    verify+e2e并行)，单实例轮≈0，#1632 后的新分片轮残差应趋零（等待已计入 e2e 秒）。
  · M6=79_v2 自举闭环单例耗时（详情.M6.执行.耗时秒，仅写集触发轮有）；78_v2 与
    普通用例耗时来自 详情.e2e明细.用例耗时（worker 纯耗时·不含池内排队）。
  · arm64 近期轮按平台限制跳过 78_v2/79_v2（e2e明细.跳过 非 0、用例耗时无两项），
    与 win/linux 全量轮**不同口径，不并入同一比较**——报告中单列。

用法：
  python scripts/ci_platform_timing.py --fetch --limit 30 \
         --out target/perf/ci_platform_timing.md --json target/perf/ci_gate_snapshot.json
  python scripts/ci_platform_timing.py --input target/perf/ci_gate_snapshot.json --out ...
  python scripts/ci_platform_timing.py --selftest        # 零网络正反自测

边界：本脚本只服务 CI 观测面，不改预验/分片/门禁语义；API 不可达=显式报错退出
  （exit 2），不静默产出空表（648 miss 不可静默精神对观测工具的同类要求）。
"""
from __future__ import annotations

import json
import os
import statistics
import sys
import urllib.error
import urllib.request
from datetime import datetime
from pathlib import Path

平台序 = ("win-x64", "linux-x86_64", "linux-arm64")
主轮标志步骤 = ("网站示例verify", "check_spec_coverage")
重型用例 = ("79_v2_自举闭环", "78_v2_自举链构建")
最小整轮秒 = 300
仓库根 = Path(__file__).resolve().parent.parent
默认客户端配置 = 仓库根 / "scripts" / "queue_client.json"


# ═══════════════ 数据拉取（只读·失败显式） ═══════════════

def 读客户端配置(路径: Path = 默认客户端配置) -> tuple[str, str]:
    """读 scripts/queue_client.json（url/令牌）——缺失/坏 JSON=显式异常，不静默空跑。"""
    数据 = json.loads(Path(路径).read_text(encoding="utf-8"))
    url = str(数据.get("url") or "").strip()
    令牌 = str(数据.get("令牌") or "").strip()
    if not url or not 令牌:
        raise RuntimeError("队列客户端配置缺 url/令牌：%s" % 路径)
    return url.rstrip("/"), 令牌


def 池GET(url: str, 令牌: str, 路径: str, 超时秒: float = 40.0) -> dict:
    """GET 队列服务 API（1425 登录墙：Bearer 令牌）——非 200/坏 JSON=异常上抛。"""
    请求 = urllib.request.Request(url + 路径, headers={"Authorization": "Bearer " + 令牌})
    try:
        with urllib.request.urlopen(请求, timeout=超时秒) as 回:
            原 = 回.read().decode("utf-8")
    except urllib.error.HTTPError as e:
        raise RuntimeError("队列服务 %s 返回 HTTP %s" % (路径, e.code)) from e
    except Exception as e:                       # 网络类异常统一显式（不静默降级）
        raise RuntimeError("队列服务 %s 不可达：%r" % (路径, e)) from e
    try:
        return json.loads(原)
    except ValueError as e:
        raise RuntimeError("队列服务 %s 返回非 JSON（前 120 字：%r）"
                           % (路径, 原[:120])) from e


def 拉快照(每平台上限: int = 30, 客户端配置: Path = 默认客户端配置) -> dict:
    """拉最近门禁行 + 每行 gate_result 详情，构成与 --input 同形状的快照。

    门禁快照 200 行按平台取满「每平台上限」为主（win 历史样本少，能取多少取多少）。
    单线程顺序请求=对服务端最温和（每行一次 GET·200 行约 75s）。
    """
    url, 令牌 = 读客户端配置(客户端配置)
    状态 = 池GET(url, 令牌, "/api/state")
    行们 = list(状态.get("门禁最近") or [])
    计数: dict[str, int] = {}
    取行 = []
    for 行 in 行们:                              # 已按时刻倒序；每平台截断
        平台 = 行.get("平台")
        if 平台 not in 平台序:
            continue
        if 计数.get(平台, 0) >= 每平台上限:
            continue
        计数[平台] = 计数.get(平台, 0) + 1
        取行.append(行)
    出 = []
    for 行 in 取行:
        路径 = "/api/gate_result?plat=%s&sha=%s" % (行.get("平台"), 行.get("sha"))
        详情 = 池GET(url, 令牌, 路径)
        出.append({**行, "_gate": 详情})
    有效 = {k: v for k, v in 状态.items() if k != "预验任务"}   # 预验任务=大列表，不落快照
    return {"拉取时刻": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "state": 有效, "门禁": 出}


# ═══════════════ 记录整理（纯函数·可自测） ═══════════════

def 详情(row: dict) -> dict:
    return row.get("_gate", {}).get("详情") or {}


def 步骤们(row: dict) -> dict:
    return 详情(row).get("步骤") or {}


def 有秒(项) -> bool:
    return isinstance(项, dict) and isinstance(项.get("秒"), (int, float))


def 是主轮(row: dict) -> bool:
    """主实例整轮判据：跑一轮/分片段都过静态门禁；跑分片轮（从片）不过。
    机械、不依赖「片序」字段（门禁表不落该字段·实测 None）。"""
    键 = 步骤们(row)
    return any(k in 键 for k in 主轮标志步骤)


def 取分片秒(row: dict) -> dict:
    """两种分片上报形态归一：旧=步骤 e2e片N.秒；新=e2e明细.分片[].秒。"""
    出: dict[int, float] = {}
    for 名, 项 in 步骤们(row).items():
        if 名.startswith("e2e片") and 名[len("e2e片"):].isdigit() and 有秒(项):
            出[int(名[len("e2e片"):])] = float(项["秒"])
    for 片 in (详情(row).get("e2e明细") or {}).get("分片") or []:
        if isinstance(片, dict) and isinstance(片.get("片序"), int) and isinstance(片.get("秒"), (int, float)):
            出.setdefault(片["片序"], float(片["秒"]))
    return 出


def 整理(row: dict) -> dict:
    """门禁行 → 归因记录（字段缺失=None，由渲染层显式显示「-」）。"""
    详 = 详情(row)
    键 = 步骤们(row)
    构建 = 键.get("构建") or {}
    用例耗时 = (详.get("e2e明细") or {}).get("用例耗时")
    M6 = ((详.get("M6") or {}).get("执行") or {}).get("耗时秒")
    分片秒 = 取分片秒(row)
    分片形态 = 键.get("e2e并行", {}).get("分片形态") or 详.get("分片形态")
    if 分片形态 is None and 分片秒:
        分片形态 = "分片(旧形态)"                  # 旧 daemon 只有 e2e片N 步骤·无形态字段
    return {
        "平台": row.get("平台"), "时刻": row.get("时刻"), "sha": (row.get("sha") or "")[:10],
        "绿": bool(row.get("绿")), "总秒": row.get("总秒") if isinstance(row.get("总秒"), int) else None,
        "分支": 详.get("分支"), "runner": 详.get("runner"),
        "构建并行度": 详.get("CN_BUILD_JOBS") or 详.get("构建并行度"),
        "步骤": 键, "构建": 构建, "e2e秒": (键.get("e2e并行") or {}).get("秒"),
        "分片秒": 分片秒, "分片形态": 分片形态,
        "用例耗时": 用例耗时 if isinstance(用例耗时, dict) else None,
        "M6耗时": M6 if isinstance(M6, (int, float)) else None,
        "构建缓存态": 构建.get("缓存态"),
    }


def 残差(记: dict) -> float | None:
    """总秒 - 计入步骤秒之和（构建/单测/静态门禁/verify/e2e并行/片步骤之外的单步）。
    单实例轮≈0（同步/清产物秒级）；分片轮>0=收片/串行认领等待未计入 e2e。"""
    if 记["总秒"] is None:
        return None
    排除 = {"e2e片%d" % k for k in 记["分片秒"]}
    和 = sum(float(v["秒"]) for k, v in 记["步骤"].items()
             if 有秒(v) and k not in 排除 and k != "M6平台保障")
    return 记["总秒"] - 和


def 筛选(行们: list[dict], 每平台上限: int = 30) -> tuple[dict, list[str]]:
    """返回 (平台→记录列表, 排除说明行)。排除=非主轮/无 e2e 秒/总秒≤300。"""
    平台桶: dict[str, list[dict]] = {}
    排除 = []
    for 行 in 行们:
        平台 = 行.get("平台")
        if 平台 not in 平台序:
            continue
        if len(平台桶.get(平台, [])) >= 每平台上限:
            continue
        if not 是主轮(行):
            排除.append("%s %s 非主轮（从实例片轮·无静态门禁步骤）" % (平台, (行.get("sha") or "")[:10]))
            continue
        记 = 整理(行)
        if 记["e2e秒"] is None:
            排除.append("%s %s 无 e2e 秒（过期早退/构建红/未跑 e2e）" % (平台, 记["sha"]))
            continue
        if 记["总秒"] is None or 记["总秒"] <= 最小整轮秒:
            排除.append("%s %s 总秒 %s≤%d（早退/异常残轮）" % (平台, 记["sha"], 记["总秒"], 最小整轮秒))
            continue
        平台桶.setdefault(平台, []).append(记)
    for 平 in 平台桶:
        平台桶[平].sort(key=lambda r: r["时刻"] or "", reverse=True)
        平台桶[平] = 平台桶[平][:每平台上限]
    return 平台桶, 排除


# ═══════════════ 统计（纯函数） ═══════════════

def 中位(vals) -> float | None:
    vals = [v for v in vals if isinstance(v, (int, float))]
    return statistics.median(vals) if vals else None


def 分位(vals, q: float):
    vals = sorted(v for v in vals if isinstance(v, (int, float)))
    if not vals:
        return None
    return vals[min(len(vals) - 1, max(0, int(len(vals) * q)))]


def 统计步骤(记录们: list[dict]) -> dict:
    """步骤名 → {n, 中位, p90, 最小, 最大}（e2e片N 归并为「e2e片(各片)」单行）。"""
    桶: dict[str, list[float]] = {}
    for 记 in 记录们:
        for 名, 项 in 记["步骤"].items():
            if 名.startswith("e2e片") or 名 == "M6平台保障" or not 有秒(项):
                continue
            桶.setdefault(名, []).append(float(项["秒"]))
        for 值 in 记["分片秒"].values():
            桶.setdefault("e2e片(各片)", []).append(值)
    出 = {}
    for 名, vals in 桶.items():
        出[名] = {"n": len(vals), "中位": 中位(vals), "p90": 分位(vals, 0.9),
                  "最小": min(vals), "最大": max(vals)}
    return dict(sorted(出.items(), key=lambda kv: -(kv[1]["中位"] or 0)))


def 统计平台(记录们: list[dict]) -> dict:
    总 = [r["总秒"] for r in 记录们]
    e2e = [r["e2e秒"] for r in 记录们]
    分片轮 = [r for r in 记录们 if r["分片秒"]]
    单轮 = [r for r in 记录们 if not r["分片秒"]]
    构建零 = [r["构建"]["秒"] for r in 记录们
              if 有秒(r["构建"]) and r["构建"].get("rc") == 0]
    残差们 = [残差(r) for r in 记录们]
    普通, 重型 = [], []
    for r in 记录们:
        u = r["用例耗时"]
        if not u:
            continue
        for 名, 值 in u.items():
            if not isinstance(值, (int, float)):
                continue
            (重型 if 名 in 重型用例 else 普通).append(float(值))
    M6 = [r["M6耗时"] for r in 记录们 if r["M6耗时"] is not None]
    重79 = [r["用例耗时"]["79_v2_自举闭环"] for r in 记录们
            if r["用例耗时"] and isinstance(r["用例耗时"].get("79_v2_自举闭环"), (int, float))
            and r["用例耗时"]["79_v2_自举闭环"] > 0]
    重78 = [r["用例耗时"]["78_v2_自举链构建"] for r in 记录们
            if r["用例耗时"] and isinstance(r["用例耗时"].get("78_v2_自举链构建"), (int, float))
            and r["用例耗时"]["78_v2_自举链构建"] > 0]
    最近5 = 记录们[:5]                         # 记录已按时刻倒序
    分片残差 = [残差(r) for r in 分片轮 if 残差(r) is not None]
    return {
        "n": len(记录们), "绿": sum(1 for r in 记录们 if r["绿"]),
        "总秒": {"中位": 中位(总), "p90": 分位(总, 0.9), "最小": min(总), "最大": max(总)},
        "e2e秒": {"中位": 中位(e2e), "最小": min(e2e), "最大": max(e2e)},
        "构建秒": {"n": len(构建零), "中位": 中位(构建零),
                  "无缓存": [r["构建"]["秒"] for r in 记录们
                          if 有秒(r["构建"]) and r["构建缓存态"] == "无缓存"]},
        "分片轮": {"n": len(分片轮), "总中位": 中位([r["总秒"] for r in 分片轮]),
                  "e2e中位": 中位([r["e2e秒"] for r in 分片轮]),
                  "残差超100": sum(1 for v in 分片残差 if v > 100),
                  "残差最大": max(分片残差) if 分片残差 else None},
        "单实例轮": {"n": len(单轮), "总中位": 中位([r["总秒"] for r in 单轮]),
                    "e2e中位": 中位([r["e2e秒"] for r in 单轮])},
        "残差": {"中位": 中位(残差们), "p90": 分位(残差们, 0.9),
                "最大": max((v for v in 残差们 if v is not None), default=None)},
        "M6": {"n": len(M6), "中位": 中位(M6),
               "最小": min(M6) if M6 else None, "最大": max(M6) if M6 else None},
        "重型79": {"n": len(重79), "中位": 中位(重79)},
        "重型78": {"n": len(重78), "中位": 中位(重78)},
        "普通用例": {"n": len(普通), "中位": 中位(普通),
                    "p90": 分位(普通, 0.9),
                    "超20秒占比": (round(sum(1 for v in 普通 if v > 20) / len(普通), 3)
                                if 普通 else None)},
        "最近5轮": {"n": len(最近5), "总中位": 中位([r["总秒"] for r in 最近5]),
                   "e2e中位": 中位([r["e2e秒"] for r in 最近5]),
                   "M6中位": 中位([r["M6耗时"] for r in 最近5 if r["M6耗时"] is not None])},
        "步骤": 统计步骤(记录们),
    }


def 按runner(记录们: list[dict]) -> list[dict]:
    桶: dict[str, list[dict]] = {}
    for r in 记录们:
        桶.setdefault(r["runner"] or "<旧版无 runner>", []).append(r)
    出 = []
    for 名, rs in 桶.items():
        出.append({"runner": 名, "n": len(rs),
                  "总中位": 中位([r["总秒"] for r in rs]),
                  "e2e中位": 中位([r["e2e秒"] for r in rs]),
                  "M6中位": 中位([r["M6耗时"] for r in rs if r["M6耗时"] is not None]),
                  "jobs": next((r["构建并行度"] for r in rs if r["构建并行度"]), None)})
    return sorted(出, key=lambda x: -(x["总中位"] or 0))


def 汇总(快照: dict, 每平台上限: int = 30) -> dict:
    桶, 排除 = 筛选(快照.get("门禁") or [], 每平台上限)
    出 = {"拉取时刻": 快照.get("拉取时刻"), "排除": 排除,
           "平台": {p: 统计平台(桶[p]) for p in 平台序 if 桶.get(p)},
           "runner": {p: 按runner(桶[p]) for p in 平台序 if 桶.get(p)}}
    return 出


# ═══════════════ 渲染（markdown） ═══════════════

def _整(v) -> str:
    return "-" if v is None else str(int(round(v)))


def _比(v, n=2) -> str:
    return "-" if v is None else ("%." + str(n) + "f") % v


def 渲染(汇: dict, 每平台上限: int) -> str:
    行 = ["# 三平台验证轮分阶段耗时（自动生成·任务 #1429）", "",
          "数据源：TX_01 队列服务 `/api/state`+`/api/gate_result`；拉取时刻 %s；"
          "每平台最近 ≤%d 个整轮（主轮∧有 e2e 秒∧总秒>%d）。" % (
              汇.get("拉取时刻") or "-", 每平台上限, 最小整轮秒), "",
          "口径：步骤秒=门禁详情.步骤.秒（e2e 红轮仍计入·构建中位只取 rc=0）；"
          "e2e 秒=整轮 e2e 墙钟（单实例=run_e2e 墙钟；#1632 起分片/退化=真实墙钟·"
          "历史轮分片=max 片秒/退化=片秒和）；"
          "残差=总秒-计入步骤秒和（#1632 前分片轮即收片/串行认领等待·新分片轮应趋零）；"
          "M6=79_v2 单例；普通用例=逐用例 worker 纯耗时（排除 78/79）。", ""]
    行 += ["## 1. 平台总览", "",
           "| 平台 | n | 绿 | 总秒中位 | 总秒 p90 | 总秒极差 | 最近5轮总中位 | e2e 中位 | 构建中位(rc0) |",
           "|---|---|---|---|---|---|---|---|---|"]
    for p in 平台序:
        统 = 汇["平台"].get(p)
        if not 统:
            continue
        行.append("| %s | %d | %d | %s | %s | %s-%s | %s | %s | %s |" % (
            p, 统["n"], 统["绿"], _整(统["总秒"]["中位"]), _整(统["总秒"]["p90"]),
            _整(统["总秒"]["最小"]), _整(统["总秒"]["最大"]),
            _整(统["最近5轮"]["总中位"]), _整(统["e2e秒"]["中位"]),
            _整(统["构建秒"]["中位"])))
    行 += ["", "## 1b. e2e 结构与重型/普通用例", "",
           "| 平台 | e2e 中位 | 79_v2 中位(n) | 78_v2 中位(n) | 普通用例中位(n·>20s占比) | 分片轮 n/总中位/e2e中位 | 分片轮残差>100s | 单实例轮 n/总中位/e2e中位 |",
           "|---|---|---|---|---|---|---|---|"]
    for p in 平台序:
        统 = 汇["平台"].get(p)
        if not 统:
            continue
        f, s, j = 统["分片轮"], 统["单实例轮"], 统["普通用例"]
        行.append("| %s | %s | %s (%d) | %s (%d) | %s (%d·%s) | %d / %s / %s | %d（最大 %s） | %d / %s / %s |" % (
            p, _整(统["e2e秒"]["中位"]),
            _整(统["重型79"]["中位"]), 统["重型79"]["n"],
            _整(统["重型78"]["中位"]), 统["重型78"]["n"],
            _比(j["中位"], 1), j["n"], _比(j["超20秒占比"], 2),
            f["n"], _整(f["总中位"]), _整(f["e2e中位"]),
            f["残差超100"], _整(f["残差最大"]),
            s["n"], _整(s["总中位"]), _整(s["e2e中位"])))
    行 += ["", "## 2. 平台 × 步骤（秒）", ""]
    for p in 平台序:
        统 = 汇["平台"].get(p)
        if not 统:
            continue
        行 += ["### %s（n=%d）" % (p, 统["n"]), "",
               "| 步骤 | n | 中位 | p90 | 极差 |", "|---|---|---|---|---|"]
        for 名, 项 in 统["步骤"].items():
            行.append("| %s | %d | %s | %s | %s-%s |" % (
                名, 项["n"], _整(项["中位"]), _整(项["p90"]), _整(项["最小"]), _整(项["最大"])))
        行.append("")
    行 += ["## 3. 平台 × 执行机", "",
           "| 平台 | runner | n | 构建并行度 | 总秒中位 | e2e 中位 | M6 中位 |", "|---|---|---|---|---|---|---|"]
    for p in 平台序:
        for r in 汇["runner"].get(p, []):
            行.append("| %s | %s | %d | %s | %s | %s | %s |" % (
                p, r["runner"], r["n"], r["jobs"] or "-", _整(r["总中位"]),
                _整(r["e2e中位"]), _整(r["M6中位"])))
    行 += ["", "## 4. 排除样本（不参与比较）", ""]
    行 += ["- " + x for x in (汇.get("排除") or [])[:40]] or ["- 无"]
    行.append("")
    return "\n".join(行)


# ═══════════════ 自测（零网络） ═══════════════

def _假行(平台: str, sha: str, 总秒: int, 步骤: dict, *, 绿=True, 分支="ci/预验-x",
          用例耗时=None, M6=None, 分片=None, runner="r1") -> dict:
    """造门禁行（与 /api/gate_result 同形状）供自测。"""
    详 = {"sha": sha, "分支": 分支, "平台": 平台, "步骤": 步骤, "runner": runner,
          "绿": 绿, "总秒": 总秒, "时刻": "2026-10-26T00:00:00"}
    if 用例耗时 is not None:
        详["e2e明细"] = {"用例耗时": 用例耗时, "总计": len(用例耗时), "跳过": 0}
    if M6 is not None:
        详["M6"] = {"执行": {"状态": "通过", "耗时秒": M6}}
    if 分片 is not None:
        详.setdefault("e2e明细", {})["分片"] = 分片
    return {"平台": 平台, "sha": sha, "绿": 绿, "总秒": 总秒,
            "时刻": "10-26 00:00:00", "_gate": {"绿": 绿, "总秒": 总秒, "详情": 详}}


def 自测() -> int:
    红 = []
    def 查(条件, 说明):
        if not 条件:
            红.append(说明)

    全步骤 = {"网站示例verify": {"rc": 0, "秒": 2}, "check_spec_coverage": {"rc": 0, "秒": 0},
              "构建": {"rc": 0, "秒": 100}, "单测": {"rc": 0, "秒": 1},
              "e2e并行": {"rc": 0, "秒": 300}}
    行们 = [
        # 主轮·分片（新形态 e2e明细.分片 + M6 + 用例耗时）
        _假行("linux-x86_64", "a" * 40, 900, {**全步骤, "构建": {"rc": 0, "秒": 120}},
              用例耗时={"79_v2_自举闭环": 500.0, "78_v2_自举链构建": 300.0, "01_hello": 1.0,
                      "02_control": 1.2}, M6=500.0,
              分片=[{"片序": 1, "秒": 400.0}, {"片序": 2, "秒": 250.0}], runner="m1"),
        # 主轮·旧形态 e2e片N
        _假行("linux-x86_64", "b" * 40, 800, {**全步骤, "e2e并行": {"rc": 0, "秒": 400},
                                            "e2e片1": {"rc": 0, "秒": 350}}, runner="m2"),
        # 从实例片轮：无静态门禁步骤 → 必须排除
        _假行("linux-x86_64", "c" * 40, 400, {"构建": {"rc": 0, "秒": 100},
                                            "e2e并行": {"rc": 0, "秒": 300}}, runner="s1"),
        # 过期早退：无 e2e 秒 → 必须排除
        _假行("win-x64", "d" * 40, 40, {"网站示例verify": {"rc": 0, "秒": 1},
                                      "e2e并行": {"rc": 0, "说明": "过期早退"}}),
        # win 整轮
        _假行("win-x64", "e" * 40, 3000, {**全步骤, "构建": {"rc": 0, "秒": 30},
                                        "e2e并行": {"rc": 1, "秒": 2800}},
              用例耗时={"79_v2_自举闭环": 1800.0, "78_v2_自举链构建": 700.0, "01_hello": 40.0},
              M6=1800.0, 绿=False),
    ]
    快照 = {"拉取时刻": "t", "门禁": 行们}
    桶, 排除 = 筛选(快照["门禁"], 30)
    查(len(桶.get("linux-x86_64", [])) == 2, "从片轮应被排除（linux 仅剩 2 主轮）")
    查(len(桶.get("win-x64", [])) == 1, "过期早退应被排除（win 仅剩 1 轮）")
    查(len(排除) == 2, "排除说明应有 2 条（实际 %d）" % len(排除))
    查(桶["linux-x86_64"][0]["sha"].startswith("a"), "同平台按时刻倒序（稳定序）")
    记 = 桶["linux-x86_64"][0]
    查(记["分片秒"] == {1: 400.0, 2: 250.0}, "新形态分片归一失败：%r" % (记["分片秒"],))
    查(记["M6耗时"] == 500.0, "M6 提取失败")
    查(abs((残差(记) or 0) - (900 - (2 + 0 + 120 + 1 + 300))) < 1e-6,
       "残差口径错（应=总秒-计入步骤和，不含各片秒）")
    旧 = 桶["linux-x86_64"][1]
    查(旧["分片秒"] == {1: 350.0}, "旧形态 e2e片N 识别失败：%r" % (旧["分片秒"],))
    查(旧["分片形态"] == "分片(旧形态)", "旧形态分片形态标注失败")
    统 = 统计平台(桶["linux-x86_64"])
    查(统["n"] == 2 and 统["分片轮"]["n"] == 2, "分片轮计数错")
    查(统["总秒"]["中位"] == 850, "总秒中位错：%r" % (统["总秒"]["中位"],))
    查(统["普通用例"]["n"] == 2 and abs(统["普通用例"]["中位"] - 1.1) < 1e-6,
       "普通用例中位应排除 78/79（实际 %r）" % (统["普通用例"],))
    查(统["重型79"]["n"] == 1 and 统["重型79"]["中位"] == 500, "79_v2 统计错")
    查(统["重型78"]["n"] == 1 and 统["重型78"]["中位"] == 300, "78_v2 统计错")
    查(统["M6"]["n"] == 1 and 统["M6"]["中位"] == 500, "M6 统计错")
    查(统["构建秒"]["n"] == 2 and 统["构建秒"]["中位"] == 110, "构建中位错")
    汇 = 汇总(快照, 30)
    文 = 渲染(汇, 30)
    查("## 2. 平台 × 步骤" in 文 and "linux-x86_64（n=2）" in 文, "渲染缺平台步骤表")
    查("e2e片(各片)" in 文, "渲染缺各片秒归并行")
    if 红:
        for x in 红:
            print("[自测红] " + x)
        return 1
    print("[自测绿] ci_platform_timing：过滤/分片双形态/残差/M6/普通用例/渲染 全绿")
    return 0


# ═══════════════ CLI ═══════════════

def main(argv: list[str]) -> int:
    参 = list(argv[1:])
    if "--selftest" in 参:
        return 自测()
    每平台上限 = 30
    输入 = 输出 = 快照出 = None
    取数 = False
    客户配置 = 默认客户端配置
    i = 0
    while i < len(参):
        a = 参[i]
        if a in ("--fetch",):
            取数 = True
        elif a == "--limit" and i + 1 < len(参):
            i += 1; 每平台上限 = int(参[i])
        elif a == "--input" and i + 1 < len(参):
            i += 1; 输入 = Path(参[i])
        elif a == "--out" and i + 1 < len(参):
            i += 1; 输出 = Path(参[i])
        elif a == "--json" and i + 1 < len(参):
            i += 1; 快照出 = Path(参[i])
        elif a == "--client" and i + 1 < len(参):
            i += 1; 客户配置 = Path(参[i])
        else:
            print("未知参数：%s" % a, file=sys.stderr)
            return 2
        i += 1
    try:
        if 输入 is not None:
            快照 = json.loads(Path(输入).read_text(encoding="utf-8"))
        elif 取数:
            快照 = 拉快照(每平台上限, 客户配置)
        else:
            print("须给 --fetch 或 --input <快照.json>（或 --selftest）", file=sys.stderr)
            return 2
    except Exception as e:                       # 显式失败（不静默空表）
        print("[错误] 取数失败：%s" % e, file=sys.stderr)
        return 2
    if 快照出 is not None:
        快照出.parent.mkdir(parents=True, exist_ok=True)
        快照出.write_text(json.dumps(快照, ensure_ascii=False), encoding="utf-8")
    汇 = 汇总(快照, 每平台上限)
    文 = 渲染(汇, 每平台上限)
    if 输出 is not None:
        输出.parent.mkdir(parents=True, exist_ok=True)
        输出.write_text(文, encoding="utf-8")
        print("已写 %s" % 输出)
    else:
        print(文)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
