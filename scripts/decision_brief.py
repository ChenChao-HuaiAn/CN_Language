# -*- coding: utf-8 -*-
"""decision_brief.py——裁决一页纸生成器（239·2026-10-07·诊断轮问题③「用户=唯一决策瓶颈」落地）。

扫任务台账全部挂起/待裁决面（⏸ 任务+备注含 待裁决/待批/待用户/呈报 的活任务），
按优先级排序输出结构化清单：stdout=人类可读一页纸；--json=AI 会话机读（消费后
生成大白话呈报：每件=问题+选项+推荐+可照抄裁决语）。

384：数据源从 plans/021 文件改为看板服务端任务表（382 起台账唯一权威在服务端，
021 已冻结退位——本脚本当时漏切仍读冻结旧账=数据过期，本批收口）。服务不可达
降级读 board_cache.json 最后快照（黄字提示），不静默装新。

用法：
  python scripts/decision_brief.py            # 一页纸（每周或用户令「裁决会」时跑）
  python scripts/decision_brief.py --json     # 机读

设计纪律（v5）：只读不写；零依赖纯标准库；不判断「该不该裁决」——那是 AI/用户的事。
"""
from __future__ import annotations

import argparse
import json
import re
import sys
import urllib.request
from pathlib import Path

本树根 = Path(__file__).resolve().parent.parent
挂因词 = ("待裁决", "待批", "待用户", "呈报", "选择题", "观察", "挂起")
优先级序 = {"P0": 0, "P1": 1, "P2": 2, "P3": 3}
看板地址 = "http://124.222.106.84:8301"


def 拉任务们() -> tuple[list[dict], str]:
    """服务端任务全量 → (任务们, 来源描述)。降级读缓存。"""
    try:
        with urllib.request.urlopen(f"{看板地址}/api/tasks", timeout=8) as r:
            数据 = json.loads(r.read().decode("utf-8"))
        任务们 = 数据.get("任务们", [])
        缓 = 本树根 / "scripts" / "board_cache.json"
        try:
            已有 = json.loads(缓.read_text(encoding="utf-8")) if 缓.exists() else {}
            已有["tasks"] = 任务们
            缓.write_text(json.dumps(已有, ensure_ascii=False), encoding="utf-8")
        except (OSError, ValueError):
            pass
        return 任务们, "服务端台账"
    except OSError:
        缓 = 本树根 / "scripts" / "board_cache.json"
        try:
            任务们 = json.loads(缓.read_text(encoding="utf-8")).get("tasks", [])
            if 任务们:
                return 任务们, "本地缓存（服务不可达·可能是旧快照）"
        except (OSError, ValueError):
            pass
    print("[失败] 看板服务不可达且无缓存——裁决面暂不可知（恢复后重跑）", file=sys.stderr)
    return [], ""


def 收集待裁决(任务们: list[dict]) -> list[dict]:
    """⏸ 任务 ∪ 备注含挂因词的活任务（⬜/🏃）——✅ 已收口不入。"""
    件们: list[dict] = []
    for t in 任务们:
        件 = {"号": t["号"], "标题": t["标题"], "状态": t["状态"],
              "优先级": t["优先级"], "备注": t.get("备注", "")}
        if 件["状态"] == "⏸":
            件["挂因"] = 件["备注"] or "挂起（备注未写因）"
            件们.append(件)
        elif 件["状态"] in ("⬜", "🏃") and any(w in 件["备注"] for w in 挂因词):
            句 = next((句 for 句 in re.split(r"[·;；]", 件["备注"]) if any(w in 句 for w in 挂因词)), "")
            件["挂因"] = 句.strip() or 件["备注"][:80]
            件们.append(件)
    件们.sort(key=lambda x: (优先级序.get(x["优先级"], 9), x["号"]))
    return 件们


def 一页纸(件们: list[dict], 来源: str) -> str:
    行们 = [f"# 裁决一页纸（{len(件们)} 件待拍板·源={来源}）", ""]
    if not 件们:
        行们 += ["（无挂起/待裁决事项——全线在跑）", ""]
        return "\n".join(行们)
    行们.append("| # | 优先级 | 状态 | 事项 | 挂因/待决点 |")
    行们.append("|---|--------|------|------|------------|")
    for 件 in 件们:
        标题 = 件["标题"][:60] + ("…" if len(件["标题"]) > 60 else "")
        挂因 = 件["挂因"][:100] + ("…" if len(件["挂因"]) > 100 else "")
        行们.append(f"| #{件['号']} | {件['优先级']} | {件['状态']} | {标题} | {挂因} |")
    行们 += ["",
             "> 裁决方式：逐件批「按推荐/选甲/选乙/挂起」即可；P3 与机制级小事按 AGENTS §3",
             "> 分级授权 AI 自行裁决留痕，无需占用本页。改 001 规范/安全语义必呈用户。"]
    return "\n".join(行们)


def 主流程() -> int:
    解析 = argparse.ArgumentParser(description="裁决一页纸生成器（239）")
    解析.add_argument("--json", action="store_true", help="机读输出（AI 会话消费）")
    参数 = 解析.parse_args()
    任务们, 来源 = 拉任务们()
    if not 来源:
        return 1
    件们 = 收集待裁决(任务们)
    if 参数.json:
        print(json.dumps(件们, ensure_ascii=False, indent=2))
    else:
        print(一页纸(件们, 来源))
    return 0


if __name__ == "__main__":
    sys.exit(主流程())
